//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsProducer.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
#define RMQS_NULL_STRING_LENGTH    -1
#define RMQS_EMPTY_DATA_LENGTH     -1
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

        rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)Data, DataSize, "TX", rmqsGetMessageDescription(Key));
    }

    send(Socket, (const char_t *)Data, DataSize, 0);
}
//---------------------------------------------------------------------------
bool_t rmqsWaitMessage(const void *Client, const rmqsSocket Socket, const uint32_t RxTimeout)
{
    const rmqsClient_t *ClientObj = (const rmqsClient_t *)Client;
    bool_t MessageComplete = false, MessageReceived = false;
    int32_t RxBytes;
    uint32_t MessageSize;
    rmqsMsgHeader_t MsgHeader;
    uint16_t Key;

    //
    // Tries to extract the message from the already received bytes, if not enough, read
    // again from the socket
    //
    while (1)
    {
        //
        // Is the message length (4 bytes) arrived?
        //
        if (ClientObj->RxMemBufferTemp->Size >= sizeof(uint32_t))
        {
            //
            // Message length is once stored in the Tag1 field and eventually with the correct endianness
            //
            MessageSize = *(uint32_t *)ClientObj->RxMemBufferTemp->Data;

            if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
            {
                MessageSize = SwapUInt32(MessageSize);
            }

            //
            // The number of bytes to wait is the one stored in the 4 bytes of the length + the length itself
            //
            MessageSize += sizeof(uint32_t);

            if (ClientObj->RxMemBufferTemp->Size >= MessageSize)
            {
                //
                // Message completed!
                //
                MessageComplete = true;

                rmqsMemBufferClear(ClientObj->RxMemBuffer, false);
                rmqsMemBufferWrite(ClientObj->RxMemBuffer, ClientObj->RxMemBufferTemp->Data, MessageSize);

                if (ClientObj->ClientConfiguration->Logger)
                {
                    Key = *(uint16_t *)((char_t *)ClientObj->RxMemBuffer->Data + sizeof(uint32_t));

                    if (ClientObj->ClientConfiguration->IsLittleEndianMachine)
                    {
                        Key = SwapUInt16(Key);
                    }

                    Key &= 0x7FFF;

                    rmqsLoggerRegisterDump(ClientObj->ClientConfiguration->Logger, (void *)ClientObj->RxMemBuffer->Data, ClientObj->RxMemBuffer->Size, "RX", rmqsGetMessageDescription(Key));
                }

                rmqsDequeueMessageFromMemBuffer(ClientObj->RxMemBufferTemp, MessageSize);
            }
        }

        if (MessageComplete)
        {
            //
            // Check whether is a push message - PublishConfirm or PublishError
            // the header of the message is copied within a header structure to be analyzed
            // without altering the byte endianness that will be changed also by the caller
            // to parse the message
            //
            memcpy(&MsgHeader, ClientObj->RxMemBuffer->Data, sizeof(MsgHeader));

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
                    rmqsHandlePublishResult(MsgHeader.Key, (rmqsProducer_t *)ClientObj->ParentObject, ClientObj->RxMemBuffer);
                }
            }
            else
            {
                MessageReceived = true;
                break;
            }
        }

        rmqsSetSocketReadTimeouts(Socket, RxTimeout);

        RxBytes = recv(Socket, (char_t *)ClientObj->RxSocketBuffer, RMQS_CLIENT_RX_BUFFER_SIZE, 0);

        if (RxBytes <= 0)
        {
            break;
        }

        rmqsMemBufferWrite(ClientObj->RxMemBufferTemp, (void *)ClientObj->RxSocketBuffer, RxBytes);

        //
        // New loop to extract the message from the receive stream
        //
    }

    return MessageReceived;
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
            if (ClientObj->RxMemBuffer->Size >= sizeof(rmqsResponse_t))
            {
                memcpy(Response, ClientObj->RxMemBuffer->Data, sizeof(rmqsResponse_t));

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
void rmqsDequeueMessageFromMemBuffer(rmqsMemBuffer_t *MemBuffer, const size_t MessageSize)
{
    //
    // Remove the message from the stream, shifting eventual extra bytes to the beginning
    //
    rmqsMemBufferDelete(MemBuffer, MessageSize);
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

    return(Description);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
size_t rmqsAddInt8ToMemBuffer(rmqsMemBuffer_t *MemBuffer, int8_t Value)
{
    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt8ToMemBuffer(rmqsMemBuffer_t *MemBuffer, uint8_t Value)
{
    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt16ToMemBuffer(rmqsMemBuffer_t *MemBuffer, int16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt16ToMemBuffer(rmqsMemBuffer_t *MemBuffer, uint16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt32ToMemBuffer(rmqsMemBuffer_t *MemBuffer, int32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt32ToMemBuffer(rmqsMemBuffer_t *MemBuffer, uint32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt64ToMemBuffer(rmqsMemBuffer_t *MemBuffer, int64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt64ToMemBuffer(rmqsMemBuffer_t *MemBuffer, uint64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsMemBufferWrite(MemBuffer, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddStringToMemBuffer(rmqsMemBuffer_t *MemBuffer, const char_t *Value, bool_t IsLittleEndianMachine)
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

    rmqsAddInt16ToMemBuffer(MemBuffer, StringLen, IsLittleEndianMachine);
    BytesAdded = sizeof(StringLen);

    if (StringLen != RMQS_NULL_STRING_LENGTH)
    {
        rmqsMemBufferWrite(MemBuffer, (void *)Value, StringLen);
        BytesAdded += StringLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------
size_t rmqsAddBytesToMemBuffer(rmqsMemBuffer_t *MemBuffer, void *Value, size_t ValueLength, bool_t IsLittleEndianMachine)
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

    rmqsAddInt32ToMemBuffer(MemBuffer, DataLen, IsLittleEndianMachine);
    BytesAdded = sizeof(DataLen);

    if (DataLen != RMQS_EMPTY_DATA_LENGTH)
    {
        rmqsMemBufferWrite(MemBuffer, (void *)Value, DataLen);
        BytesAdded += DataLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------

