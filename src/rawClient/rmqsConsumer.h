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
typedef void (*DeliverResultCallback_t)(uint8_t SubscriptionId, size_t DataSize, void *Data);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    uint32_t FrameMax;
    uint32_t Heartbeat;
    uint16_t DefaultCredit;
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
rmqsConsumer_t * rmqsConsumerCreate(rmqsClientConfiguration_t *ClientConfiguration, uint32_t FrameMax, uint32_t Heartbeat, uint16_t DefaultCredit, DeliverResultCallback_t DeliverResultCallback);
void rmqsConsumerDestroy(rmqsConsumer_t *Consumer);
void rmqsConsumerPoll(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint32_t Timeout, bool_t *ConnectionLost);
rmqsResponseCode_t rmqsSubscribe(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint8_t SubscriptionId, char_t *StreamName, rmqsOffsetType OffsetType, uint64_t Offset, uint16_t Credit, rmqsProperty_t *Properties, size_t PropertiesCount);
void rmqsCredit(rmqsConsumer_t *Consumer, rmqsSocket Socket, uint8_t SubscriptionId, uint16_t Credit);
void rmqsHandleDeliver(rmqsConsumer_t *Consumer, rmqsSocket Socket, rmqsBuffer_t *Buffer);
#endif
//---------------------------------------------------------------------------