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
    rmqspsNotConnected = 0,
    rmqspsTcpConnected,
    rmqspsReady
}
rmqsProducerStatus;
//---------------------------------------------------------------------------
typedef struct
{
    void *Environment; // This pointer is void because of a circular dependency of environment and producer structs
    char Host[RMQS_MAX_HOSTNAME_LENGTH + 1];
    uint16_t Port;
    rmqsSocket Socket;
    rmqsCorrelationId CorrelationId;
    rmqsStream *TxStream;
    rmqsStream *RxStream;
    rmqsProducerStatus Status;
    char RxSocketBuffer[1024];
    rmqsThread *ProducerThread;
}
rmqsProducer;
//---------------------------------------------------------------------------
rmqsProducer * rmqsProducerCreate(void *Environment, char *Host, uint16_t Port);
void rmqsProducerDestroy(rmqsProducer *Producer);
void rmqsProducerThreadRoutine(void *Parameters, uint8_t *TerminateRequest);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
