//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsEnvironment.h"
#include "rmqsList.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
rmqsEnvironment_t * rmqsEnvironmentCreate(char_t *BrokersList, const char_t *Username, const char_t *Password, uint8_t EnableLogging, char_t *LogFileName)
{
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)rmqsAllocateMemory(sizeof(rmqsEnvironment_t));
    char_t *BrokersCopyString, *BrokerCopyString;
    rmqsList_t *TempBrokersList;
    char_t *BrokerItemString, *BrokerFieldString;
    rmqsBroker_t *Broker;
    size_t i;

    memset(Environment, 0, sizeof(rmqsEnvironment_t));

    //
    // Check whether the client is running on a big or little endian machine
    //
    Environment->IsLittleEndianMachine = rmqsIsLittleEndianMachine();

    //
    // Save username and password in the environment variable
    //
    strncpy((char_t *)Environment->Username, Username, RMQS_MAX_USERNAME_LENGTH);
    strncpy((char_t *)Environment->Password, Password, RMQS_MAX_PASSWORD_LENGTH);

    //
    // Create the brokers and producers list
    //
    Environment->BrokersList = rmqsListCreate(rmqsEnvironmentBrokersListDestroy);
    Environment->ProducersList = rmqsListCreate(rmqsEnvironmentProducersListDestroy);

    //
    // Fill the brokers list, that must be declared witht the format host:port and
    // separated by commas
    // For example: 192.168.1.1:5552,192.168.1.2:5552,192.168.1.3:5552
    //

    //
    // strtok modifies the string for which is used, then a copy of the original brokers list is created
    //
    BrokersCopyString = (char_t *)rmqsAllocateMemory(strlen(BrokersList) + 1);
    strcpy(BrokersCopyString, BrokersList);

    //
    // Creates a list for an easy split of host and port with strtok
    //
    TempBrokersList = rmqsListCreate(rmqsGenericListDestroy);

    BrokerItemString = strtok(BrokersCopyString, RMQS_BROKER_ITEMS_SEPARATOR);

    while (BrokerItemString != 0)
    {
        BrokerCopyString = (char_t *)rmqsAllocateMemory(strlen(BrokerItemString) + 1);
        strcpy(BrokerCopyString, BrokerItemString);

        rmqsListAddEnd(TempBrokersList, BrokerCopyString);

        BrokerItemString = strtok(0, RMQS_BROKER_ITEMS_SEPARATOR);
    }

    for (i = 0; i < TempBrokersList->Count; i++)
    {
        Broker = (rmqsBroker_t *)rmqsAllocateMemory(sizeof(rmqsBroker_t));
        memset(Broker, 0, sizeof(rmqsBroker_t));

        BrokerFieldString = strtok((char_t *)rmqsListGetDataByPosition(TempBrokersList, i), RMQS_BROKER_PORT_SEPARATOR);
        strncpy(Broker->Host, BrokerFieldString, RMQS_MAX_HOSTNAME_LENGTH);

        BrokerFieldString = strtok(0, RMQS_BROKER_PORT_SEPARATOR);

        if (BrokerFieldString)
        {
            Broker->Port = (uint16_t)atoi(BrokerFieldString);
        }
        else
        {
            Broker->Port = 0;
        }

        rmqsListAddEnd(Environment->BrokersList, Broker);
    }

    rmqsListDestroy(TempBrokersList);

    rmqsFreeMemory(BrokersCopyString);

    if (EnableLogging)
    {
        Environment->Logger = rmqsLoggerCreate(LogFileName, 0);
    }

    return Environment;
}
//---------------------------------------------------------------------------
void rmqsEnvironmentDestroy(rmqsEnvironment_t *Environment)
{
    rmqsListDestroy(Environment->BrokersList);
    rmqsListDestroy(Environment->ProducersList);

    if (Environment->Logger != 0)
    {
        rmqsLoggerDestroy(Environment->Logger);
    }

    rmqsFreeMemory((void *)Environment);
}
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsEnvironmentProducerCreate(rmqsEnvironment_t *Environment, void (*EventsCB)(rqmsProducerEvent, void *))
{
    rmqsProducer_t *Producer = rmqsProducerCreate(Environment, EventsCB);

    rmqsListAddEnd(Environment->ProducersList, Producer);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsEnvironmentProducerDestroy(rmqsEnvironment_t *Environment, rmqsProducer_t *Producer)
{
    rmqsListDeleteData(Environment->ProducersList, rmqsListSearchByData(Environment->ProducersList, Producer));
}
//---------------------------------------------------------------------------
void rmqsEnvironmentBrokersListDestroy(void *Broker)
{
    rmqsFreeMemory((rmqsBroker_t *)Broker);
}
//---------------------------------------------------------------------------
void rmqsEnvironmentProducersListDestroy(void *Producer)
{
    rmqsProducerDestroy((rmqsProducer_t *)Producer);
}
//---------------------------------------------------------------------------
