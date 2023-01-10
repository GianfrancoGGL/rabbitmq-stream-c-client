//---------------------------------------------------------------------------
#ifndef rmqsProducerH
#define rmqsProducerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsNetwork.h"
#include "rmqsProtocol.h"
#include "rmqsStream.h"
#include "rmqsThread.h"
//---------------------------------------------------------------------------
typedef enum
{
    rmqspeDisconnected = 0,
    rmqspeConnected,
    rmqspeReady
}
rqmsProducerEvent;
//---------------------------------------------------------------------------
typedef enum
{
    rmqspsDisconnected = 0,
    rmqspsConnected,
    rmqspsReady
}
rmqsProducerStatus;
//---------------------------------------------------------------------------
typedef struct
{
    void *Environment; // This pointer is void because of a circular dependency of environment and producer structs
    rmqsProducerStatus Status;
    void (*EventsCB)(rqmsProducerEvent, void *);
    rmqsSocket Socket;
    rmqsCorrelationId CorrelationId;
    rmqsStream_t *TxStream;
    rmqsStream_t *RxStream;
    char RxSocketBuffer[1024];
    rmqsThread_t *ProducerThread;
}
rmqsProducer_t;
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(void *Environment, void (*EventsCB)(rqmsProducerEvent, void *));
void rmqsProducerDestroy(rmqsProducer_t *Producer);
void rmqsProducerThreadRoutine(void *Parameters, uint8_t *TerminateRequest);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
