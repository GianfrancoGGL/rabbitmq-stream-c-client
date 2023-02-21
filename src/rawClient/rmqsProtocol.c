/****************************************************************************
MIT License

Copyright (c) 2023 Gianfranco Giugliano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsProducer.h"
#include "rmqsMemory.h"
#include "rmqsBuffer.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
bool_t rmqsIsLittleEndianMachine(void)
{
    union
    {
        uint32_t i;
        char_t c[4];
    }
    bint = {0x01020304};

    if (bint.c[0] == 1)
    {
       return 0;
    }
    else
    {
       return 1;
    }
}
//---------------------------------------------------------------------------
void rmqsSendMessage(const void *Client, const rmqsSocket Socket, const char_t *Data, size_t DataSize)
{
    const rmqsClient_t *ClientObj = (const rmqsClient_t *)Client;
    uint16_t Key;

    if (ClientObj->ClientConfiguration->Logger)
    {
        Key = *(uint16_t *)(Data + sizeof(uint32_t));

        if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
        {
            Key = SwapUInt16(Key);
        }

        Key &= 0x7FFF;

        rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)Data, DataSize, "TX", rmqsGetMessageDescription(Key), 0);
    }

    send(Socket, (const char_t *)Data, DataSize, 0);
}
//---------------------------------------------------------------------------
bool_t rmqsWaitMessage(const void *Client, const rmqsSocket Socket, const uint32_t RxTimeout)
{
    const rmqsClient_t *ClientObj = (const rmqsClient_t *)Client;
    int32_t RxBytes;
    uint32_t MessageSize;
    rmqsMsgHeader_t MsgHeader;
    uint16_t Key;

    while (true)
    {
        if (ClientObj->RxQueue->Tag1 == true)
        {
            //
            // Tag1 indicates whether it was containing a complete message
            // of Tag2 bytes. Now we're going to read a new message,
            // then the previous one can be dequeued and the complete marker cleared
            //
            rmqsDequeueMessageFromBuffer(ClientObj->RxQueue, ClientObj->RxQueue->Tag2);
            ClientObj->RxQueue->Tag1 = false;
            ClientObj->RxQueue->Tag2 = 0;
        }

        rmqsSetSocketReadTimeouts(Socket, RxTimeout);

        RxBytes = recv(Socket, (char_t *)ClientObj->RxSocketBuffer, RMQS_CLIENT_RX_BUFFER_SIZE, 0);

        if (RxBytes <= 0)
        {
            return false;
        }

        rmqsBufferWrite(ClientObj->RxQueue, (void *)ClientObj->RxSocketBuffer, RxBytes);

        //
        // Is the message length (4 bytes) arrived?
        //
        if (ClientObj->RxQueue->Size < sizeof(uint32_t))
        {
            continue;
        }

        //
        // Message length is once stored in the Tag1 field and eventually with the correct endianness
        //
        MessageSize = *(uint32_t *)ClientObj->RxQueue->Data;

        if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
        {
            MessageSize = SwapUInt32(MessageSize);
        }

        //
        // The number of bytes to wait is the one stored in the 4 bytes of the length + the length itself
        //
        MessageSize += sizeof(uint32_t);

        if (ClientObj->RxQueue->Size < MessageSize)
        {
            return false;
        }

        //
        // Message completed
        // Completed flag is stored in the Tag1 field and the message size in the Tag2 fied
        // so the message will be dequeued in the next function call, once it has
        // been passed to the functionc caller for its analysis.
        //
        ClientObj->RxQueue->Tag1 = true;
        ClientObj->RxQueue->Tag2 = MessageSize;

        if (ClientObj->ClientConfiguration->Logger)
        {
            Key = *(uint16_t *)((char_t *)ClientObj->RxQueue->Data + sizeof(uint32_t));

            if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
            {
                Key = SwapUInt16(Key);
            }

            Key &= 0x7FFF;

            rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)ClientObj->RxQueue->Data, ClientObj->RxQueue->Tag2, "RX", rmqsGetMessageDescription(Key), 0);
        }

        //
        // Check whether is a push message - PublishConfirm or PublishError
        // the header of the message is copied within a header structure to be analyzed
        // without altering the byte endianness that will be changed also by the caller
        // to parse the message
        //
        memcpy(&MsgHeader, ClientObj->RxQueue->Data, sizeof(MsgHeader));

        if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
        {
            MsgHeader.Size = SwapUInt32(MsgHeader.Size);
            MsgHeader.Key = SwapUInt16(MsgHeader.Key);
            MsgHeader.Key &= 0x7FFF;
            MsgHeader.Version = SwapUInt16(MsgHeader.Version);
        }

        if (MsgHeader.Key == rmqscPublishConfirm || MsgHeader.Key == rmqscPublishError)
        {
            //
            // These message are caught and handled by this procedure and not returned to the caller
            //
            if (ClientObj->ClientType == rmqsctProducer)
            {
                rmqsHandlePublishResult(MsgHeader.Key, (rmqsProducer_t *)ClientObj->ParentObject, ClientObj->RxQueue);
            }
        }
        else
        {
            break;
        }
    }

    return true;
}
//---------------------------------------------------------------------------
bool_t rmqsWaitResponse(const void *Client, const rmqsSocket Socket, uint32_t CorrelationId, rmqsResponse_t *Response, const uint32_t RxTimeout)
{
    const rmqsClient_t *ClientObj = (const rmqsClient_t *)Client;
    uint32_t WaitMessageTimeout = RxTimeout;
    uint32_t Time;

    rmqsTimerStart(ClientObj->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(ClientObj->ClientConfiguration->WaitReplyTimer) < RxTimeout)
    {
        if (rmqsWaitMessage(ClientObj, Socket, WaitMessageTimeout))
        {
            if (ClientObj->RxQueue->Size >= sizeof(rmqsResponse_t))
            {
                memcpy(Response, ClientObj->RxQueue->Data, sizeof(rmqsResponse_t));

                if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
                {
                    Response->Header.Size = SwapUInt32(Response->Header.Size);
                    Response->Header.Key = SwapUInt16(Response->Header.Key);
                    Response->Header.Key &= 0x7FFF;
                    Response->Header.Version = SwapUInt16(Response->Header.Version);
                    Response->CorrelationId = SwapUInt32(Response->CorrelationId);
                    Response->ResponseCode = SwapUInt16(Response->ResponseCode);

                    if (rmqsCommandHasCorrelationId(Response->Header.Key) && Response->CorrelationId == CorrelationId)
                    {
                        return true;
                    }
                }
            }

            Time = rmqsTimerGetTime(ClientObj->ClientConfiguration->WaitReplyTimer);

            WaitMessageTimeout = RxTimeout - Time;
        }
    }

    return false;
}
//---------------------------------------------------------------------------
void rmqsDequeueMessageFromBuffer(rmqsBuffer_t *Buffer, const size_t MessageSize)
{
    //
    // Remove the message from the stream, shifting eventual extra bytes to the beginning
    //
    rmqsBufferDelete(Buffer, MessageSize);
}
//---------------------------------------------------------------------------
char_t * rmqsGetMessageDescription(uint16_t Key)
{
    char_t *Description = "Unknown message";

    switch (Key)
    {
        case rmqscDeclarePublisher:
            Description = "DeclarePublisher";
            break;

        case rmqscPublish:
            Description = "Publish";
            break;

        case rmqscPublishConfirm:
            Description = "PublishConfirm";
            break;

        case rmqscPublishError:
            Description = "PublishError";
            break;

        case rmqscQueryPublisherSequence:
            Description = "QueryPublisherSequence";
            break;

        case rmqscDeletePublisher:
            Description = "DeletePublisher";
            break;

        case rmqscSubscribe:
            Description = "Subscribe";
            break;

        case rmqscDeliver:
            Description = "Deliver";
            break;

        case rmqscCredit:
            Description = "Credit";
            break;

        case rmqscStoreOffset:
            Description = "StoreOffset";
            break;

        case rmqscQueryOffset:
            Description = "QueryOffset";
            break;

        case rmqscUnsubscribe:
            Description = "Unsubscribe";
            break;

        case rmqscCreate:
            Description = "Create";
            break;

        case rmqscDelete:
            Description = "Delete";
            break;

        case rmqscMetadata:
            Description = "Metadata";
            break;

        case rmqscMetadataUpdate:
            Description = "MetadataUpdate";
            break;

        case rmqscPeerProperties:
            Description = "PeerProperties";
            break;

        case rmqscSaslHandshake:
            Description = "SaslHandshake";
            break;

        case rmqscSaslAuthenticate:
            Description = "SaslAuthenticate";
            break;

        case rmqscTune:
            Description = "Tune";
            break;

        case rmqscOpen:
            Description = "Open";
            break;

        case rmqscClose:
            Description = "Close";
            break;

        case rmqscHeartbeat:
            Description = "Heartbeat";
            break;

        case rmqscRoute:
            Description = "Route";
            break;

        case rmqscPartitions:
            Description = "Partitions";
            break;

        case rmqscConsumerUpdate:
            Description = "ConsumerUpdate";
            break;

        case rmqscExchangeCommandVersions:
            Description = "ExchangeCommandVersions";
            break;

        case rmqscStreamStats:
            Description = "StreamStats";
            break;
    }

    return Description;
}
//---------------------------------------------------------------------------
//
// Older Borland C++ compilers doesn't support  functions in C files
//
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
//---------------------------------------------------------------------------
 size_t rmqsAddInt8ToBuffer(rmqsBuffer_t *Buffer, int8_t Value)
{
    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddUInt8ToBuffer(rmqsBuffer_t *Buffer, uint8_t Value)
{
    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddInt16ToBuffer(rmqsBuffer_t *Buffer, int16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddUInt16ToBuffer(rmqsBuffer_t *Buffer, uint16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddInt32ToBuffer(rmqsBuffer_t *Buffer, int32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddUInt32ToBuffer(rmqsBuffer_t *Buffer, uint32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddInt64ToBuffer(rmqsBuffer_t *Buffer, int64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddUInt64ToBuffer(rmqsBuffer_t *Buffer, uint64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 size_t rmqsAddStringToBuffer(rmqsBuffer_t *Buffer, const char_t *Value, bool_t IsLittleEndianMachine)
{
    int16_t StringLen;
    size_t BytesAdded;

    if (Value == 0 || *Value == 0)
    {
        StringLen = RMQS_NULL_STRING_LENGTH;
    }
    else
    {
        StringLen = (int16_t)strlen(Value);
    }

    rmqsAddInt16ToBuffer(Buffer, StringLen, IsLittleEndianMachine);
    BytesAdded = sizeof(StringLen);

    if (StringLen != RMQS_NULL_STRING_LENGTH)
    {
        rmqsBufferWrite(Buffer, (void *)Value, StringLen);
        BytesAdded += StringLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------
 size_t rmqsAddBytesToBuffer(rmqsBuffer_t *Buffer, void *Value, size_t ValueLength, bool_t DeclareLength, bool_t IsLittleEndianMachine)
{
    int32_t DataLen;
    size_t BytesAdded;

    if (ValueLength == 0)
    {
        DataLen = RMQS_EMPTY_DATA_LENGTH;
    }
    else
    {
        DataLen = (int32_t)ValueLength;
    }

    if (DeclareLength)
    {
        rmqsAddInt32ToBuffer(Buffer, DataLen, IsLittleEndianMachine);
        BytesAdded = sizeof(DataLen);
    }

    if (DataLen != RMQS_EMPTY_DATA_LENGTH)
    {
        rmqsBufferWrite(Buffer, (void *)Value, DataLen);
        BytesAdded += DataLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

