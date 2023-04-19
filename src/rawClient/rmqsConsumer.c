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
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsConsumer.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsClientFunc rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, char_t *ConsumerReference, uint32_t FrameMax, uint32_t Heartbeat, uint16_t DefaultCredit, DeliverResultCallback_t DeliverResultCallback, MetadataUpdateCallback_t MetadataUpdateCallback)
{
    rmqsConsumer_t *Consumer = (rmqsConsumer_t *)rmqsAllocateMemory(sizeof(rmqsConsumer_t));

    memset(Consumer, 0, sizeof(rmqsConsumer_t));

    Consumer->Client = rmqsClientCreate(ClientConfiguration, rmqsctConsumer, Consumer, MetadataUpdateCallback);
    strncpy(Consumer->ConsumerReference, ConsumerReference, RMQS_MAX_CONSUMER_REFERENCE_LENGTH);
    Consumer->FrameMax = FrameMax;
    Consumer->Heartbeat = Heartbeat;
    Consumer->DefaultCredit = DefaultCredit;
    Consumer->DeliverResultCallback = DeliverResultCallback;

    return Consumer;
}
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsConsumerDestroy(rmqsConsumer_t *Consumer)
{
    rmqsClientDestroy(Consumer->Client);

    rmqsFreeMemory((void *)Consumer);
}
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsConsumerPoll(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint32_t Timeout, bool_t *ConnectionError)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    *ConnectionError = false;

    rmqsTimerStart(Consumer->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Consumer->Client, Socket, WaitMessageTimeout, ConnectionError);

        if (*ConnectionError)
        {
            return;
        }

        Time = rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer);

        WaitMessageTimeout = Timeout - Time;
    }
}
//---------------------------------------------------------------------------
rmqsClientFunc bool_t rmqsSubscribe(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, char_t *Stream, rmqsOffsetType OffsetType, uint64_t Offset, uint16_t Credit, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscSubscribe;
    uint16_t Version = 1;
    uint32_t i;
    uint32_t MapSize;
    rmqsProperty_t *Property;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    if (PropertyCount > 0)
    {
        //
        // Calculate the map size
        //
        MapSize = sizeof(MapSize); // The map size field

        for (i = 0; i < PropertyCount; i++)
        {
            //
            // For every key/value
            // 2 bytes for the key length field
            // the length of the key
            // 2 bytes for the value length field
            // the length of the value
            //
            Property = &Properties[i];
            MapSize += sizeof(uint16_t); // Key length field
            MapSize += (uint32_t)strlen(Property->Key);
            MapSize += sizeof(uint16_t); // Value length field
            MapSize += (uint32_t)strlen(Property->Value);
        }
    }
    else
    {
        MapSize = 0;
    }

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, SubscriptionId);
    rmqsAddStringToBuffer(Client->TxQueue, Stream, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, (uint16_t)OffsetType, ClientConfiguration->IsLittleEndianMachine);

    if (OffsetType == rmqsotTimestamp)
    {
        rmqsAddInt64ToBuffer(Client->TxQueue, (int64_t)Offset, ClientConfiguration->IsLittleEndianMachine);
    }
    else
    {
        rmqsAddUInt64ToBuffer(Client->TxQueue, (uint64_t)Offset, ClientConfiguration->IsLittleEndianMachine);
    }

    rmqsAddUInt16ToBuffer(Client->TxQueue, Credit, ClientConfiguration->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToBuffer(Client->TxQueue, MapSize, Client->ClientConfiguration->IsLittleEndianMachine);

    for (i = 0; i < PropertyCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToBuffer(Client->TxQueue, Property->Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property->Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscSubscribe || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        memset(Consumer->SubscriptionStreamTable[SubscriptionId - 1], 0, RMQS_MAX_STREAM_NAME_LENGTH + 1);
        strncpy(Consumer->SubscriptionStreamTable[SubscriptionId - 1], Stream, RMQS_MAX_STREAM_NAME_LENGTH);

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
rmqsClientFunc bool_t rmqsUnsubscribe(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, rmqsResponseCode_t *ResponseCode)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscUnsubscribe;
    uint16_t Version = 1;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, SubscriptionId);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscUnsubscribe || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        memset(Consumer->SubscriptionStreamTable[SubscriptionId - 1], 0, RMQS_MAX_STREAM_NAME_LENGTH + 1);

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsCredit(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, uint16_t Credit)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscCredit;
    uint16_t Version = 1;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, SubscriptionId);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Credit, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);
}
//---------------------------------------------------------------------------
rmqsClientFunc bool_t rmqsQueryOffset(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, char_t *Reference, char_t *Stream, bool_t *ValidOffset, uint64_t *Offset, rmqsResponseCode_t *ResponseCode)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscQueryOffset;
    uint16_t Version = 1;
    rmqsQueryOffsetResponse_t *QueryOffsetResponse;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    *ValidOffset = false;
    *Offset = 0;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Reference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Stream, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        //
        // Handshake response is different from a standard response, it has to be reparsed
        //
        QueryOffsetResponse = (rmqsQueryOffsetResponse_t *)Client->RxQueue->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            QueryOffsetResponse->Header.Size = SwapUInt32(QueryOffsetResponse->Header.Size);
            QueryOffsetResponse->Header.Key = SwapUInt16(QueryOffsetResponse->Header.Key);
            QueryOffsetResponse->Header.Key &= 0x7FFF;
            QueryOffsetResponse->Header.Version = SwapUInt16(QueryOffsetResponse->Header.Version);
            QueryOffsetResponse->CorrelationId = SwapUInt32(QueryOffsetResponse->CorrelationId);
            QueryOffsetResponse->ResponseCode = SwapUInt16(QueryOffsetResponse->ResponseCode);
            QueryOffsetResponse->Offset = SwapUInt64(QueryOffsetResponse->Offset);
        }

        *ResponseCode = QueryOffsetResponse->ResponseCode;

        if (QueryOffsetResponse->Header.Key != rmqscQueryOffset)
        {
            return false;
        }
        else if (QueryOffsetResponse->ResponseCode == rmqsrNoOffset)
        {
            return true; // The offset query got a reply, but there isn't an offset, then a true is returned with a non valid offset flag
        }
        else if (QueryOffsetResponse->ResponseCode != rmqsrOK)
        {
            return false;
        }

        *ValidOffset = true;
        *Offset = QueryOffsetResponse->Offset;

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsStoreOffset(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, char_t *Reference, char_t *Stream, uint64_t Offset)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscStoreOffset;
    uint16_t Version = 1;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Reference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Stream, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt64ToBuffer(Client->TxQueue, Offset, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);
}
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsHandleDeliver(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t SubscriptionId;
    uint32_t EntryTypeAndSize = 0;
    uint64_t MessageOffset;
    bool_t StoreOffset;
    size_t i;

    rmqsGetUInt8FromMemory(&MessagePayload, &SubscriptionId);
    rmqsGetInt8FromMemory(&MessagePayload, &Consumer->DeliverInfo.MagicVersion);
    rmqsGetInt8FromMemory(&MessagePayload, &Consumer->DeliverInfo.ChunkType);
    rmqsGetUInt16FromMemory(&MessagePayload, &Consumer->DeliverInfo.NumEntries, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt32FromMemory(&MessagePayload, &Consumer->DeliverInfo.NumRecords, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetInt64FromMemory(&MessagePayload, &Consumer->DeliverInfo.Timestamp, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt64FromMemory(&MessagePayload, &Consumer->DeliverInfo.Epoch, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt64FromMemory(&MessagePayload, &Consumer->DeliverInfo.ChunkFirstOffset, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetInt32FromMemory(&MessagePayload, &Consumer->DeliverInfo.ChunkCrc, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt32FromMemory(&MessagePayload, &Consumer->DeliverInfo.DataLength, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt32FromMemory(&MessagePayload, &Consumer->DeliverInfo.TrailerLength, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetUInt32FromMemory(&MessagePayload, &Consumer->DeliverInfo.Reserved, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);

    MessageOffset = Consumer->DeliverInfo.ChunkFirstOffset;

    for (i = 0; i < Consumer->DeliverInfo.NumEntries; i++)
    {
        rmqsGetUInt32FromMemory(&MessagePayload, &EntryTypeAndSize, Consumer->Client->ClientConfiguration->IsLittleEndianMachine);

        StoreOffset = false;

        Consumer->DeliverResultCallback(SubscriptionId, (byte_t *)MessagePayload, (size_t)EntryTypeAndSize, &Consumer->DeliverInfo, MessageOffset, &StoreOffset);

        MessagePayload += EntryTypeAndSize;

        if (StoreOffset)
        {
            rmqsStoreOffset(Consumer, Socket, Consumer->ConsumerReference, Consumer->SubscriptionStreamTable[SubscriptionId - 1], MessageOffset);
        }

        MessageOffset++;
    }

    rmqsCredit(Consumer, Socket, SubscriptionId, Consumer->DefaultCredit);
}
//---------------------------------------------------------------------------
