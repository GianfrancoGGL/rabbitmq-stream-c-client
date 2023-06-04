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
#include "rmqsPublisher.h"
#include "rmqsConsumer.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
#include "rmqsThread.h"
#include "rmqsBuffer.h"
#include "rmqsError.h"
#include "rmqsLib.h"
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsIsLittleEndianMachine(void)
{
    union
    {
        uint32_t i;
        char_t c[4];
    }
    bint = {0x01020304};

    if (bint.c[0] == 1)
    {
       return false;
    }
    else
    {
       return true;
    }
}
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsSendMessage(void *Client, rmqsSocket_t Socket, char_t *Data, size_t DataSize)
{
    rmqsClient_t *ClientObj = (rmqsClient_t *)Client;
    uint16_t Key;
    int32_t Flags = 0;
    bool_t Result;

    if (ClientObj->ClientConfiguration->Logger)
    {
        Key = *(uint16_t *)(Data + sizeof(uint32_t));

        if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
        {
            Key = SwapUInt16(Key);
        }

        Key &= 0x7FFF;

        rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)Data, DataSize, "TX", rmqsGetCommandDescription(Key), 0);
    }

    #if ! (_WIN32 || _WIN64)
    Flags = MSG_NOSIGNAL;
    #endif

    Result = send(Socket, (char_t *)Data, (int32_t)DataSize, Flags) != -1;

    return Result;
}
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsWaitMessage(void *Client, rmqsSocket_t Socket, uint32_t RxTimeout, bool_t *ConnectionError)
{
    rmqsClient_t *ClientObj = (rmqsClient_t *)Client;
    bool_t RxPendingBytes;
    int32_t RxBytes;
    uint32_t MessageSize;
    rmqsMsgHeader_t MsgHeader;
    uint16_t Key;

    *ConnectionError = false;

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

            RxPendingBytes = ClientObj->RxQueue->Size > 0;
        }
        else
        {
            RxPendingBytes = false;
        }

        //
        // If there are still bytes in the stream, first process them, else
        // read from the socket
        //
        if (! RxPendingBytes)
        {
            if (RxTimeout != ClientObj->LastRxTimeout)
            {
                rmqsSetSocketReadTimeout(Socket, RxTimeout);

                ClientObj->LastRxTimeout = RxTimeout;
            }

            rmqsResetLastError();

            RxBytes = recv(Socket, (char_t *)ClientObj->RxSocketBuffer, RMQS_CLIENT_RX_BUFFER_SIZE, 0);

            if (RxBytes <= 0)
            {
                if (rmqsConnectionError())
                {
                    *ConnectionError = true;
                }

                return false;
            }
            else
            {
                /*
                ** Uncomment to log the raw data received
                **
                if (ClientObj->ClientConfiguration->Logger)
                {
                    rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)ClientObj->RxSocketBuffer, RxBytes, "RAW RX", 0, 0);
                }
                **
                **
                */
            }

            rmqsBufferWrite(ClientObj->RxQueue, (void *)ClientObj->RxSocketBuffer, RxBytes);
        }

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

            rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)ClientObj->RxQueue->Data, ClientObj->RxQueue->Tag2, "RX", rmqsGetCommandDescription(Key), 0);
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
            if (ClientObj->ClientType == rmqsctPublisher)
            {
                rmqsHandlePublishResult(MsgHeader.Key, (rmqsPublisher_t *)ClientObj->ParentObject, ClientObj->RxQueue);
            }
        }
        else if (MsgHeader.Key == rmqscDeliver)
        {
            //
            // This message is caught and handled by this procedure and not returned to the caller
            //
            if (ClientObj->ClientType == rmqsctConsumer)
            {
                rmqsHandleDeliver((rmqsConsumer_t *)ClientObj->ParentObject, Socket, ClientObj->RxQueue);
            }
        }
        else if (MsgHeader.Key == rmqscMetadataUpdate)
        {
            rmqsHandleMetadataUpdate(ClientObj, ClientObj->RxQueue);
        }
        else
        {
            break;
        }
    }

    return true;
}
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsWaitResponse(void *Client, rmqsSocket_t Socket, uint32_t CorrelationId, rmqsResponse_t *Response, uint32_t RxTimeout, bool_t *ConnectionError)
{
    rmqsClient_t *ClientObj = (rmqsClient_t *)Client;
    uint32_t WaitMessageTimeout = RxTimeout;
    uint32_t Time;

    *ConnectionError = false;

    rmqsTimerStart(ClientObj->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(ClientObj->ClientConfiguration->WaitReplyTimer) < RxTimeout)
    {
        if (rmqsWaitMessage(ClientObj, Socket, WaitMessageTimeout, ConnectionError))
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
        else
        {
            if (*ConnectionError)
            {
                return false;
            }
        }
    }

    return false;
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsDequeueMessageFromBuffer(rmqsBuffer_t *Buffer, size_t MessageSize)
{
    //
    // Remove the message from the stream, shifting eventual extra bytes to the beginning
    //
    rmqsBufferDelete(Buffer, MessageSize);
}
//---------------------------------------------------------------------------
rmqsProtoFunc char_t * rmqsGetCommandDescription(uint16_t Key)
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
rmqsProtoFunc char_t * rmqsGetResponseCodeDescription(uint16_t ResponseCode)
{
    char_t *Description = "None";

    switch (ResponseCode)
    {
        case rmqsrOK:
            Description = "OK";
            break;

        case rmqsrStreamDoesNotExist:
            Description = "Stream does not exist";
            break;

        case rmqsrSubscriptionIDAlreadyExists:
            Description = "Subscription ID already exists";
            break;

        case rmqsrSubscriptionIDDoesNotExist:
            Description = "Subscription ID does not exist";
            break;

        case rmqsrStreamAlreadyExists:
            Description = "Stream already exists";
            break;

        case rmqsrStreamNotAvailable:
            Description = "Stream not available";
            break;

        case rmqsrSASLMechanismNotSupported:
            Description = "SASL mechanism not supported";
            break;

        case rmqsrAuthenticationFailure:
            Description = "Authentication failure";
            break;

        case rmqsrSASLError:
            Description = "SASL error";
            break;

        case rmqsrSASLChallenge:
            Description = "SASL challenge";
            break;

        case rmqsrSASLAuthenticationFailureLoopback:
            Description = "SASL authentication failure loopback";
            break;

        case rmqsrVirtualHostAccessFailure:
            Description = "Virtual host access failure";
            break;

        case rmqsrUnknownFrame:
            Description = "Unknown frame";
            break;

        case rmqsrFrameTooLarge:
            Description = "Frame too large";
            break;

        case rmqsrInternalError:
            Description = "Internal error";
            break;

        case rmqsrAccessRefused:
            Description = "Access refused";
            break;

        case rmqsrPreconditionFailed:
            Description = "Precondition failed";
            break;

        case rmqsrPublisherDoesNotExist:
            Description = "Publisher does not exist";
            break;

        case rmqsrNoOffset:
            Description = "No offset";
            break;

        case rqmsrInvalidBrokerIndex:
            Description = "Invalid broker index";
            break;

        case rmqsrConnectionError:
            Description = "Connection error";
            break;
    }

    return Description;
}
//---------------------------------------------------------------------------
rmqsProtoFunc size_t rmqsAddInt8ToBuffer(rmqsBuffer_t *Buffer, int8_t Value)
{
    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddUInt8ToBuffer(rmqsBuffer_t *Buffer, uint8_t Value)
{
    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddInt16ToBuffer(rmqsBuffer_t *Buffer, int16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddUInt16ToBuffer(rmqsBuffer_t *Buffer, uint16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddInt32ToBuffer(rmqsBuffer_t *Buffer, int32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddUInt32ToBuffer(rmqsBuffer_t *Buffer, uint32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddInt64ToBuffer(rmqsBuffer_t *Buffer, int64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddUInt64ToBuffer(rmqsBuffer_t *Buffer, uint64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsBufferWrite(Buffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
 rmqsProtoFunc size_t rmqsAddStringToBuffer(rmqsBuffer_t *Buffer, char_t *Value, bool_t IsLittleEndianMachine)
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
 rmqsProtoFunc size_t rmqsAddBytesToBuffer(rmqsBuffer_t *Buffer, void *Value, size_t ValueLength, bool_t DeclareLength, bool_t IsLittleEndianMachine)
{
    int32_t DataLen;
    size_t BytesAdded = 0;

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
rmqsProtoFunc void rmqsGetInt8FromMemory(char_t **Pointer, int8_t *Value)
{
    *Value = *(int8_t *)*Pointer;
    *Pointer += sizeof(int8_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetUInt8FromMemory(char_t **Pointer, uint8_t *Value)
{
    *Value = *(uint8_t *)*Pointer;
    *Pointer += sizeof(uint8_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetInt16FromMemory(char_t **Pointer, int16_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(int16_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt16(*Value);
    }

    *Pointer += sizeof(int16_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetUInt16FromMemory(char_t **Pointer, uint16_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(uint16_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt16(*Value);
    }

    *Pointer += sizeof(uint16_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetInt32FromMemory(char_t **Pointer, int32_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(int32_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt32(*Value);
    }

    *Pointer += sizeof(int32_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetUInt32FromMemory(char_t **Pointer, uint32_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(uint32_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt32(*Value);
    }

    *Pointer += sizeof(uint32_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetInt64FromMemory(char_t **Pointer, int64_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(int64_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt64(*Value);
    }

    *Pointer += sizeof(int64_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetUInt64FromMemory(char_t **Pointer, uint64_t *Value, bool_t IsLittleEndianMachine)
{
    *Value = *(uint64_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        *Value = SwapUInt64(*Value);
    }

    *Pointer += sizeof(uint64_t);
}
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetStringFromMemory(char_t **Pointer, char_t *Value, size_t MaxValueLength, bool_t IsLittleEndianMachine)
{
    uint16_t StringLength = *(uint16_t *)*Pointer;

    if (IsLittleEndianMachine)
    {
        StringLength = SwapUInt16(StringLength);
    }

    *Pointer += sizeof(uint16_t);

    strncpy(Value, *Pointer, MaxValueLength);
    Value[StringLength] = 0;

    *Pointer += StringLength;
}
//---------------------------------------------------------------------------
