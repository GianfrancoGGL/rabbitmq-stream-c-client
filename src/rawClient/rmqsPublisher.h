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
#ifndef rmqsPublisherH
#define rmqsPublisherH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMessage.h"
#include "rmqsTimer.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_PUBLISHER_REFERENCE_LENGTH         256
#define RMQS_PUBLISH_RESULT_ARRAY_SIZE             1024
//---------------------------------------------------------------------------
typedef struct
{
    uint64_t PublishingId;
    uint16_t Code;
}
PublishResult_t;
//---------------------------------------------------------------------------
typedef void (*PublishResultCallback_t)(uint8_t PublisherId, PublishResult_t *PublishResultList, size_t PublishingIdCount, bool_t Confirmed);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    char_t PublisherReference[RMQS_MAX_PUBLISHER_REFERENCE_LENGTH + 1];
    uint32_t Heartbeat;
    PublishResult_t PublishResultArray[RMQS_PUBLISH_RESULT_ARRAY_SIZE]; // This array allows to buffer multiple ids and then call once the publish result callback
    PublishResultCallback_t PublishResultCallback;
}
rmqsPublisher_t;
//---------------------------------------------------------------------------
rmqsPublisher_t * rmqsPublisherCreate(rmqsClientConfiguration_t *ClientConfiguration, char_t *PublisherReference, uint32_t Heartbeat, PublishResultCallback_t PublishResultCallback, MetadataUpdateCallback_t MetadataUpdateCallback);
void rmqsPublisherDestroy(rmqsPublisher_t *Publisher);
void rmqsPublisherPoll(rmqsPublisher_t *Publisher, rmqsSocket_t Socket, uint32_t Timeout, bool_t *ConnectionError);
//---------------------------------------------------------------------------
bool_t rmqsDeclarePublisher(rmqsPublisher_t *Publisher, rmqsSocket_t Socket, uint8_t PublisherId, char_t *Stream, rmqsResponseCode_t *ResponseCode);
bool_t rmqsQueryPublisherSequence(rmqsPublisher_t *Publisher, rmqsSocket_t Socket, char_t *Stream, uint64_t *Sequence, rmqsResponseCode_t *ResponseCode);
bool_t rmqsDeletePublisher(rmqsPublisher_t *Publisher, rmqsSocket_t Socket, uint8_t PublisherId, rmqsResponseCode_t *ResponseCode);
void rmqsPublish(rmqsPublisher_t *Publisher, rmqsSocket_t Socket, uint8_t PublisherId, rmqsMessage_t *Messages, size_t MessageCount);
//---------------------------------------------------------------------------
void rmqsHandlePublishResult(uint16_t Key, rmqsPublisher_t *Publisher, rmqsBuffer_t *Buffer);
#endif
//---------------------------------------------------------------------------
