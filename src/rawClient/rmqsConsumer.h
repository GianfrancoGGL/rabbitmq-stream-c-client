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
#ifndef rmqsConsumerH
#define rmqsConsumerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMessage.h"
#include "rmqsTimer.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_CONSUMER_REFERENCE_LENGTH         256
//---------------------------------------------------------------------------
typedef struct
{
    int8_t MagicVersion;
    int8_t ChunkType;
    uint16_t NumEntries;
    uint32_t NumRecords;
    int64_t Timestamp;
    uint64_t Epoch;
    uint64_t ChunkFirstOffset;
    int32_t ChunkCrc;
    uint32_t DataLength;
    uint32_t TrailerLength;
    uint32_t Reserved;
}
rmqsDeliverInfo_t;
//---------------------------------------------------------------------------
typedef void (*DeliverResultCallback_t)(uint8_t SubscriptionId, byte_t *Data, size_t DataSize, rmqsDeliverInfo_t *DeliverInfo, uint64_t MessageOffset, bool_t *StoreOffset);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    char_t ConsumerReference[RMQS_MAX_CONSUMER_REFERENCE_LENGTH + 1];
    uint32_t FrameMax;
    uint32_t Heartbeat;
    uint16_t DefaultCredit;
    char_t SubscriptionStreamTable[256][RMQS_MAX_STREAM_NAME_LENGTH + 1];
    rmqsDeliverInfo_t DeliverInfo;
    DeliverResultCallback_t DeliverResultCallback;
}
rmqsConsumer_t;
//---------------------------------------------------------------------------
typedef enum
{
    rmqsotFirst = 1,
    rmqsotLast,
    rmqsotNext,
    rmqsotOffset,
    rmqsotTimestamp
}
rmqsOffsetType;
//---------------------------------------------------------------------------
rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, char_t *ConsumerReference, uint32_t FrameMax, uint32_t Heartbeat, uint16_t DefaultCredit, DeliverResultCallback_t DeliverResultCallback, MetadataUpdateCallback_t MetadataUpdateCallback);
void rmqsConsumerDestroy(rmqsConsumer_t *Consumer);
void rmqsConsumerPoll(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint32_t Timeout, bool_t *ConnectionError);
//---------------------------------------------------------------------------
bool_t rmqsSubscribe(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, char_t *Stream, rmqsOffsetType OffsetType, uint64_t Offset, uint16_t Credit, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode);
bool_t rmqsUnsubscribe(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, rmqsResponseCode_t *ResponseCode);
void rmqsCredit(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, uint8_t SubscriptionId, uint16_t Credit);
bool_t rmqsQueryOffset(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, char_t *Reference, char_t *Stream, bool_t *ValidOffset, uint64_t *Offset, rmqsResponseCode_t *ResponseCode);
void rmqsStoreOffset(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, char_t *Reference, char_t *Stream, uint64_t Offset);
//---------------------------------------------------------------------------
void rmqsHandleDeliver(rmqsConsumer_t *Consumer, rmqsSocket_t Socket, rmqsBuffer_t *Buffer);
#endif
//---------------------------------------------------------------------------
