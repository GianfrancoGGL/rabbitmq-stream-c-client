//---------------------------------------------------------------------------
#ifndef rmqsEnvironmentH
#define rmqsEnvironmentH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsLogger.h"
#include "rmqsNetwork.h"
#include "rmqsProducer.h"
//---------------------------------------------------------------------------
#define RMQS_BROKER_ITEMS_SEPARATOR    ","
#define MQMS_BROKER_PORT_SEPARATOR     ":"
//---------------------------------------------------------------------------
typedef struct
{
    char_t Host[RMQS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t Port;
}
rmqsBroker_t;
//---------------------------------------------------------------------------
typedef struct
{
    uint8_t IsLittleEndianMachine;
    rmqsList_t *BrokersList;
    rmqsList_t *ProducersList;
    rmqsLogger_t *Logger;
}
rmqsEnvironment_t;
//---------------------------------------------------------------------------
rmqsEnvironment_t * rmqsEnvironmentCreate(char_t *BrokersList, uint8_t EnableLogging, char_t *LogFileName);
void rmqsEnvironmentDestroy(rmqsEnvironment_t *Environment);
rmqsProducer_t * rmqsEnvironmentProducerCreate(rmqsEnvironment_t *Environment, void (*EventsCB)(rqmsProducerEvent, void *));
void rmqsEnvironmentProducerDestroy(rmqsEnvironment_t *Environment, rmqsProducer_t *Producer);
void rmqsEnvironmentBrokersListDestroy(void *Broker);
void rmqsEnvironmentProducersListDestroy(void *Producer);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
