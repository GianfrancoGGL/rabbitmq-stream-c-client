//---------------------------------------------------------------------------
#include "rmqsEnvironment.h"
#include "rmqsList.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
rmqsEnvironment * rmqsEnvironmentCreate(void)
{
    rmqsEnvironment *Environment = (rmqsEnvironment *)rmqsAllocateMemory(sizeof(rmqsEnvironment));

    memset(Environment, 0, sizeof(rmqsEnvironment));

    //
    // Check whether the client is running on a big or little endian machine
    //
    Environment->IsLittleEndianMachine = rmqsIsLittleEndianMachine();

    Environment->Producers = rmqsListCreate(rmqsEnvironmentProducerListDestroy);

    return Environment;
}
//---------------------------------------------------------------------------
void rmqsEnvironmentDestroy(rmqsEnvironment *Environment)
{
    rmqsListDestroy(Environment->Producers);

    rmqsFreeMemory((void *)Environment);
}
//---------------------------------------------------------------------------
rmqsProducer * rmqsEnvironmentProducerCreate(rmqsEnvironment *Environment, char *Host, uint16_t Port)
{
    rmqsProducer *Producer = rmqsProducerCreate(Environment, Host, Port);

    rmqsListAddEnd(Environment->Producers, Producer);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsEnvironmentProducerDestroy(rmqsEnvironment *Environment, rmqsProducer *Producer)
{
    rmqsListDeleteData(Environment->Producers, rmqsListSearchByData(Environment->Producers, Producer));
}
//---------------------------------------------------------------------------
void rmqsEnvironmentProducerListDestroy(void *Producer)
{
    rmqsProducerDestroy((rmqsProducer *)Producer);
}
//---------------------------------------------------------------------------
