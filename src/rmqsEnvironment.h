//---------------------------------------------------------------------------
#ifndef rmqsEnvironmentH
#define rmqsEnvironmentH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsProducer.h"
//---------------------------------------------------------------------------
typedef struct
{
    uint32_t Option1;
    uint32_t Option2;
    uint32_t Option3;
}
rqmsEnvironmentOptions;
//---------------------------------------------------------------------------
typedef struct
{
    uint8_t IsLittleEndianMachine;
    rqmsEnvironmentOptions Options;
    rmqsList *Producers;
}
rmqsEnvironment;
//---------------------------------------------------------------------------
rmqsEnvironment * rmqsEnvironmentCreate(void);
void rmqsEnvironmentDestroy(rmqsEnvironment *Environment);
rmqsProducer * rmqsEnvironmentProducerCreate(rmqsEnvironment *Environment, char *Host, uint16_t Port, void (*EventsCB)(rqmsProducerEvent, void *));
void rmqsEnvironmentProducerDestroy(rmqsEnvironment *Environment, rmqsProducer *Producer);
void rmqsEnvironmentProducerListDestroy(void *Producer);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
