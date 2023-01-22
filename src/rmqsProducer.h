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
    void (*EventsCallback)(rqmsClientEvent, void *);
    rmqsClient_t *Client;
}
rmqsProducer_t;
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(rmqsClientConfiguration_t *ClientConfiguration, const char_t *HostName, uint8_t PublisherId, const char_t *PublisherReference, void (*EventsCallback)(rqmsClientEvent, void *));
void rmqsProducerDestroy(rmqsProducer_t *Producer);
void rmqsProducerHandlerCallback(void *Producer);
rmqsResponseCode rmqsDeclarePublisher(rmqsProducer_t *Producer, const char_t *Stream);
void rmqsPublish(rmqsProducer_t *Producer, rmqsMessage_t *Message);
void rmqsPublishBatch(rmqsProducer_t *Producer, rmqsMessage_t *Messages[], size_t MessageCount);
#endif
//---------------------------------------------------------------------------
