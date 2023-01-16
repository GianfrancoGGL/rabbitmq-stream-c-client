//---------------------------------------------------------------------------
#ifndef rmqsProducerH
#define rmqsProducerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_PUBLISHER_REFERENCE_LENGTH    256
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClient_t *Client;
    uint8_t PublisherId;
    char_t PublisherReference[RMQS_MAX_PUBLISHER_REFERENCE_LENGTH + 1];
    void (*EventsCB)(rqmsClientEvent, void *);
}
rmqsProducer_t;
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(void *Environment, const char_t *HostName, uint8_t PublisherId, const char_t *PublisherReference, void (*EventsCB)(rqmsClientEvent, void *));
void rmqsProducerDestroy(rmqsProducer_t *Producer);
void rmqsProducerHandlerCB(void *Producer);
rmqsResponseCode rmqsDeclarePublisher(rmqsProducer_t *Producer, const char_t *Stream);
#endif
//---------------------------------------------------------------------------
