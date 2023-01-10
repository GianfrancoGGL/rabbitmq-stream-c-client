//---------------------------------------------------------------------------
#ifndef rmqsEnvironmentH
#define rmqsEnvironmentH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsNetwork.h"
#include "rmqsProducer.h"
//---------------------------------------------------------------------------
#define RMQS_BROKER_ITEMS_SEPARATOR    ","
#define MQMS_BROKER_PORT_SEPARATOR     ":"
//---------------------------------------------------------------------------
typedef struct
{
    char Host[RMQS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t Port;
}
rmqsBroker_t;
//---------------------------------------------------------------------------
typedef struct
{
    uint8_t IsLittleEndianMachine;
    rmqsList_t *BrokersList;
    rmqsList_t *ProducersList;
}
rmqsEnvironment_t;
//---------------------------------------------------------------------------
rmqsEnvironment_t * rmqsEnvironmentCreate(char *BrokersList);
void rmqsEnvironmentDestroy(rmqsEnvironment_t *Environment);
rmqsProducer_t * rmqsEnvironmentProducerCreate(rmqsEnvironment_t *Environment, void (*EventsCB)(rqmsProducerEvent, void *));
void rmqsEnvironmentProducerDestroy(rmqsEnvironment_t *Environment, rmqsProducer_t *Producer);
void rmqsEnvironmentBrokersListDestroy(void *Broker);
void rmqsEnvironmentProducersListDestroy(void *Producer);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
