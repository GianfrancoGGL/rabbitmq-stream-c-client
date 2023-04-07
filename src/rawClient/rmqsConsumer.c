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
rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, char_t *ConsumerReference, uint32_t FrameMax, uint32_t Heartbeat, uint16_t DefaultCredit, DeliverResultCallback_t DeliverResultCallback)
{
    rmqsConsumer_t *Consumer = (rmqsConsumer_t *)rmqsAllocateMemory(sizeof(rmqsConsumer_t));

    memset(Consumer, 0, sizeof(rmqsConsumer_t));

    Consumer->Client = rmqsClientCreate(ClientConfiguration, rmqsctConsumer, Consumer);
    strncpy(Consumer->ConsumerReference, ConsumerReference, RMQS_MAX_CONSUMER_REFERENCE_LENGTH);
    Consumer->FrameMax = FrameMax;
    Consumer->Heartbeat = Heartbeat;
    Consumer->DefaultCredit = DefaultCredit; 
    Consumer->DeliverResultCallback = DeliverResultCallback;

    return Consumer;
}
//---------------------------------------------------------------------------
void rmqsConsumerDestroy(rmqsConsumer_t *Consumer)
{
    rmqsClientDestroy(Consumer->Client);

    rmqsFreeMemory((void *)Consumer);
}
//---------------------------------------------------------------------------
void rmqsConsumerPoll(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint32_t Timeout, bool_t *ConnectionLost)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    *ConnectionLost = false;

    rmqsTimerStart(Consumer->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Consumer->Client, Socket, WaitMessageTimeout, ConnectionLost);

        if (*ConnectionLost)
        {
            return;
        }

        Time = rmqsTimerGetTime(Consumer->Client->ClientConfiguration->WaitReplyTimer);
            
        WaitMessageTimeout = Timeout - Time;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsSubscribe(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint8_t SubscriptionId, char_t *Stream, rmqsOffsetType OffsetType, uint64_t Offset, uint16_t Credit, rmqsProperty_t *Properties, size_t PropertiesCount)
{
    rmqsClient_t *Client = Consumer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscSubscribe;
    uint16_t Version = 1;
    uint32_t i;
    uint32_t MapSize;
    rmqsProperty_t *Property;
    bool_t ConnectionLost;

    if (PropertiesCount > 0)
    {
        //
        // Calculate the map size
        //
        MapSize = sizeof(MapSize); // The map size field

        for (i = 0; i < PropertiesCount; i++)
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

    for (i = 0; i < PropertiesCount; i++)
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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscSubscribe)
        {
            return false;
        }

        memset(Consumer->SubscriptionStreamTable[SubscriptionId - 1], 0, RMQS_STREAM_MAX_LENGTH + 1);
        strncpy(Consumer->SubscriptionStreamTable[SubscriptionId - 1], Stream, RMQS_STREAM_MAX_LENGTH);

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
void rmqsCredit(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint8_t SubscriptionId, uint16_t Credit)
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
void rmqsStoreOffset(rmqsConsumer_t *Consumer, rmqsSocket Socket, char_t *Reference, char_t *Stream, uint64_t Offset)
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
void rmqsHandleDeliver(rmqsConsumer_t *Consumer, rmqsSocket Socket, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t *SubscriptionId;
    uint32_t *EntryTypeAndSize;
    void *Data;
    uint64_t ChunkOffset;
    bool_t StoreOffset;
    size_t i;

    SubscriptionId = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    Consumer->DeliverInfo.MagicVersion = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    Consumer->DeliverInfo.ChunkType = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    Consumer->DeliverInfo.NumEntries = (uint16_t *)MessagePayload;
    MessagePayload += sizeof(uint16_t);
    
    Consumer->DeliverInfo.NumRecords = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    Consumer->DeliverInfo.Timestamp = (int64_t *)MessagePayload;
    MessagePayload += sizeof(int64_t);

    Consumer->DeliverInfo.Epoch = (uint64_t *)MessagePayload;
    MessagePayload += sizeof(uint64_t);

    Consumer->DeliverInfo.ChunkFirstOffset = (uint64_t *)MessagePayload;
    MessagePayload += sizeof(uint64_t);

    Consumer->DeliverInfo.ChunkCrc = (int32_t *)MessagePayload;
    MessagePayload += sizeof(int32_t);

    Consumer->DeliverInfo.DataLength = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    Consumer->DeliverInfo.TrailerLength = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    Consumer->DeliverInfo.Reserved = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    if (Consumer->Client->ClientConfiguration->IsLittleEndianMachine)
    {
        *Consumer->DeliverInfo.NumEntries = SwapUInt16(*Consumer->DeliverInfo.NumEntries);
        *Consumer->DeliverInfo.NumRecords = SwapUInt32(*Consumer->DeliverInfo.NumRecords);
        *Consumer->DeliverInfo.Timestamp = SwapUInt64(*Consumer->DeliverInfo.Timestamp);
        *Consumer->DeliverInfo.Epoch += SwapUInt64(*Consumer->DeliverInfo.Epoch);
        *Consumer->DeliverInfo.ChunkFirstOffset += SwapUInt64(*Consumer->DeliverInfo.ChunkFirstOffset);
        *Consumer->DeliverInfo.ChunkCrc = SwapUInt32(*Consumer->DeliverInfo.ChunkCrc);
        *Consumer->DeliverInfo.DataLength = SwapUInt32(*Consumer->DeliverInfo.DataLength);
        *Consumer->DeliverInfo.TrailerLength = SwapUInt32(*Consumer->DeliverInfo.TrailerLength);
        *Consumer->DeliverInfo.Reserved = SwapUInt32(*Consumer->DeliverInfo.Reserved);
    }

    ChunkOffset = *Consumer->DeliverInfo.ChunkFirstOffset; 

    for (i = 0; i < *Consumer->DeliverInfo.NumEntries; i++)
    {
        EntryTypeAndSize = (uint32_t *)MessagePayload;
        MessagePayload += sizeof(uint32_t);

        if (Consumer->Client->ClientConfiguration->IsLittleEndianMachine)
        {
            *EntryTypeAndSize = SwapUInt32(*EntryTypeAndSize);
        }

        Data = (uint8_t *)MessagePayload;
        MessagePayload += *EntryTypeAndSize;

        StoreOffset = false;

        Consumer->DeliverResultCallback(*SubscriptionId, (size_t)*EntryTypeAndSize, Data, &Consumer->DeliverInfo, &StoreOffset);

        if (StoreOffset)
        {
            rmqsStoreOffset(Consumer, Socket, Consumer->ConsumerReference, Consumer->SubscriptionStreamTable[*SubscriptionId - 1], ChunkOffset);
        }

        ChunkOffset++;
    }

    rmqsCredit(Consumer, Socket, *SubscriptionId, Consumer->DefaultCredit);
}
//---------------------------------------------------------------------------
