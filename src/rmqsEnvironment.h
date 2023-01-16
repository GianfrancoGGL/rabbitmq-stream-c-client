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
#define RMQS_BROKER_PORT_SEPARATOR     ":"
//---------------------------------------------------------------------------
#define RMQS_MAX_USERNAME_LENGTH       128
#define RMQS_MAX_PASSWORD_LENGTH       128
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
    char_t *Username[RMQS_MAX_USERNAME_LENGTH + 1];
    char_t *Password[RMQS_MAX_PASSWORD_LENGTH + 1];
    rmqsList_t *ProducersList;
    rmqsLogger_t *Logger;
}
rmqsEnvironment_t;
//---------------------------------------------------------------------------
rmqsEnvironment_t * rmqsEnvironmentCreate(char_t *BrokersList, const char_t *Username, const char_t *Password, uint8_t EnableLogging, char_t *LogFileName);
void rmqsEnvironmentDestroy(rmqsEnvironment_t *Environment);
rmqsProducer_t * rmqsEnvironmentProducerCreate(rmqsEnvironment_t *Environment, const char_t *HostName, uint8_t PublisherId, const char_t *PublisherReference, void (*EventsCB)(rqmsClientEvent, void *));
void rmqsEnvironmentProducerDestroy(rmqsEnvironment_t *Environment, rmqsProducer_t *Producer);
void rmqsEnvironmentBrokersListDestroy(void *Broker);
void rmqsEnvironmentProducersListDestroy(void *Producer);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
