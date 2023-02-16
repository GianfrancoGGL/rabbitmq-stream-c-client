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
#define RMQS_MAX_PUBLISHER_REFERENCE_LENGTH         256
#define RMQS_PUBLISHING_ID_RESULT_ARRAY_SIZE       1024
//---------------------------------------------------------------------------
typedef void (*PublishResultCallback_t)(const uint8_t PublisherId, uint64_t *PublishingIdList, const size_t PublishingIdCount, const bool_t Confirmed, const uint16_t Code);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    char_t PublisherReference[RMQS_MAX_PUBLISHER_REFERENCE_LENGTH + 1];
    rmqsTimer_t *PollTimer;
    uint64_t PublishingIdResultArray[RMQS_PUBLISHING_ID_RESULT_ARRAY_SIZE]; // This array allows to buffer multiple ids and then call once the publish result callback
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
void rmqsHandlePublishResult(uint16_t Key, rmqsProducer_t *Producer, rmqsBuffer_t *Buffer);
#endif
//---------------------------------------------------------------------------
