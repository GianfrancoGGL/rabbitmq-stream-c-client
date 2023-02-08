//---------------------------------------------------------------------------
#ifndef rmqsProducerH
#define rmqsProducerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMessage.h"
#include "rmqsTimer.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_PUBLISHER_REFERENCE_LENGTH    256
//---------------------------------------------------------------------------
typedef void (*PublishResultCallback_t)(uint8_t PublisherId, uint64_t PublishingId, bool_t Confirmed, uint16_t Code);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    char_t PublisherReference[RMQS_MAX_PUBLISHER_REFERENCE_LENGTH + 1];
    rmqsTimer_t *PollTimer;
    PublishResultCallback_t PublishResultCallback;
}
rmqsProducer_t;
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(rmqsClientConfiguration_t *ClientConfiguration, const char_t *PublisherReference, PublishResultCallback_t PublishResultCallback);
void rmqsProducerDestroy(rmqsProducer_t *Producer);
void rmqsProducerPoll(rmqsProducer_t *Producer, const rmqsSocket Socket, uint32_t Timeout);
rmqsResponseCode_t rmqsDeclarePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId, const char_t *StreamName);
rmqsResponseCode_t rmqsDeletePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId);
void rmqsPublish(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId, rmqsMessage_t *Messages, const size_t MessageCount);
void rmqsHandlePublishResult(uint16_t Key, rmqsProducer_t *Producer, rmqsMemBuffer_t *Stream);
#endif
//---------------------------------------------------------------------------
