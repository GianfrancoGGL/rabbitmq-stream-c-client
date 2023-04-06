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
rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, uint32_t FrameMax, uint32_t Heartbeat, uint16_t DefaultCredit, DeliverResultCallback_t DeliverResultCallback)
{
    rmqsConsumer_t *Consumer = (rmqsConsumer_t *)rmqsAllocateMemory(sizeof(rmqsConsumer_t));

    memset(Consumer, 0, sizeof(rmqsConsumer_t));

    Consumer->Client = rmqsClientCreate(ClientConfiguration, rmqsctConsumer, Consumer);
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
rmqsResponseCode_t rmqsSubscribe(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint8_t SubscriptionId, char_t *StreamName, rmqsOffsetType OffsetType, uint64_t Offset, uint16_t Credit, rmqsProperty_t *Properties, size_t PropertiesCount)
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
    rmqsAddStringToBuffer(Client->TxQueue, StreamName, ClientConfiguration->IsLittleEndianMachine);
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
            return rmqsrWrongReply;
        }

        return Client->Response.ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
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
void rmqsHandleDeliver(rmqsConsumer_t *Consumer, rmqsSocket Socket, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t *SubscriptionId;
    int8_t *MagicVersion;
    int8_t *ChunkType;
    uint16_t *NumEntries;
    uint32_t *NumRecords;
    int64_t *Timestamp;
    uint64_t *Epoch;
    uint64_t *ChunkFirstOffset;
    int32_t *ChunkCrc;
    uint32_t *DataLength;
    uint32_t *TrailerLength;
    uint32_t *Reserved;
    uint32_t *EntryTypeAndSize;
    void *Data;
    size_t i;

    SubscriptionId = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    MagicVersion = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    ChunkType = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);

    NumEntries = (uint16_t *)MessagePayload;
    MessagePayload += sizeof(uint16_t);
    
    NumRecords = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    Timestamp = (int64_t *)MessagePayload;
    MessagePayload += sizeof(int64_t);

    Epoch = (uint64_t *)MessagePayload;
    MessagePayload += sizeof(uint64_t);

    ChunkFirstOffset = (uint64_t *)MessagePayload;
    MessagePayload += sizeof(uint64_t);

    ChunkCrc = (int32_t *)MessagePayload;
    MessagePayload += sizeof(int32_t);

    DataLength = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    TrailerLength = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    Reserved = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    if (Consumer->Client->ClientConfiguration->IsLittleEndianMachine)
    {
        *NumEntries = SwapUInt16(*NumEntries);
        *NumRecords = SwapUInt32(*NumRecords);
        *Timestamp = SwapUInt64(*Timestamp);
        *Epoch += SwapUInt64(*Epoch);
        *ChunkFirstOffset += SwapUInt64(*ChunkFirstOffset);
        *ChunkCrc = SwapUInt32(*ChunkCrc);
        *DataLength = SwapUInt32(*DataLength);
        *TrailerLength = SwapUInt32(*TrailerLength);
        *Reserved = SwapUInt32(*Reserved);
    }

    for (i = 0; i < *NumEntries; i++)
    {
        EntryTypeAndSize = (uint32_t *)MessagePayload;
        MessagePayload += sizeof(uint32_t);

        if (Consumer->Client->ClientConfiguration->IsLittleEndianMachine)
        {
            *EntryTypeAndSize = SwapUInt32(*EntryTypeAndSize);
        }

        Data = (uint8_t *)MessagePayload;
        MessagePayload += *EntryTypeAndSize;

        Consumer->DeliverResultCallback(*SubscriptionId, (size_t)*EntryTypeAndSize, Data);
    }

    rmqsCredit(Consumer, Socket, *SubscriptionId, Consumer->DefaultCredit);
}
//---------------------------------------------------------------------------
