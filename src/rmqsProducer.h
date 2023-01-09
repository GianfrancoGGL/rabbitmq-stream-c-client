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
    char Host[RMQS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t Port;
    rmqsProducerStatus Status;
    void (*EventsCB)(rqmsProducerEvent, void *);
    rmqsSocket Socket;
    rmqsCorrelationId CorrelationId;
    rmqsStream *TxStream;
    rmqsStream *RxStream;
    char RxSocketBuffer[1024];
    rmqsThread *ProducerThread;
}
rmqsProducer;
//---------------------------------------------------------------------------
rmqsProducer * rmqsProducerCreate(void *Environment, char *Host, uint16_t Port, void (*EventsCB)(rqmsProducerEvent, void *));
void rmqsProducerDestroy(rmqsProducer *Producer);
void rmqsProducerThreadRoutine(void *Parameters, uint8_t *TerminateRequest);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
