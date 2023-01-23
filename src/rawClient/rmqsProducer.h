//---------------------------------------------------------------------------
#ifndef rmqsProducerH
#define rmqsProducerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMessage.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_PUBLISHER_REFERENCE_LENGTH    256
//---------------------------------------------------------------------------
typedef struct
{
    uint8_t PublisherId;
    char_t PublisherReference[RMQS_MAX_PUBLISHER_REFERENCE_LENGTH + 1];
    rmqsClient_t *Client;
}
rmqsProducer_t;
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(rmqsClientConfiguration_t *ClientConfiguration, const uint8_t PublisherId, const char_t *PublisherReference);
void rmqsProducerDestroy(rmqsProducer_t *Producer);
rmqsResponseCode rmqsDeclarePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const char_t *StreamName);
void rmqsPublish(rmqsProducer_t *Producer, const rmqsSocket Socket, rmqsMessage_t *Message);
void rmqsPublishBatch(rmqsProducer_t *Producer, const rmqsSocket Socket, rmqsMessage_t *Messages[], const size_t MessageCount);
#endif
//---------------------------------------------------------------------------
