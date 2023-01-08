//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsProducer.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsProducer * rmqsProducerCreate(void *Environment, char *Host, uint16_t Port)
{
    rmqsProducer *Producer = (rmqsProducer *)rmqsAllocateMemory(sizeof(rmqsProducer));

    memset(Producer, 0, sizeof(rmqsProducer));

    Producer->Environment = Environment;
    strncpy(Producer->Host, Host, RMQS_MAX_HOSTNAME_LENGTH);
    Producer->Port = Port;

    //
    // Windows machine, initialize sockets
    //
    #ifdef __WIN32__
    rmqsInitWinsock();
    #endif

    Producer->Socket = rmqsSocketCreate();
    rmqsSetTcpNoDelay(Producer->Socket);
    rmqsSetSocketTimeouts(Producer->Socket, 5, 5);
    rmqsSetKeepAlive(Producer->Socket);

    Producer->TxStream = rmqsStreamCreate();
    Producer->RxStream = rmqsStreamCreate();

    Producer->ProducerThread = rmqsThreadCreate(rmqsProducerThreadRoutine, 0, Producer);
    rmqsThreadStart(Producer->ProducerThread);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsProducerDestroy(rmqsProducer *Producer)
{
    if (Producer->Socket != rmqsInvalidSocket)
    {
        rmqsSocketDestroy((rmqsSocket *)&Producer->Socket);
    }

    rmqsThreadStop(Producer->ProducerThread);
    rmqsThreadDestroy(Producer->ProducerThread);

    rmqsStreamDestroy(Producer->TxStream);
    rmqsStreamDestroy(Producer->RxStream);

    rmqsFreeMemory((void *)Producer);

    //
    // Windows machine, shutdown sockets
    //
    #ifdef __WIN32__
    rmqsShutdownWinsock();
    #endif
}
//---------------------------------------------------------------------------
void rmqsProducerThreadRoutine(void *Parameters, uint8_t *TerminateRequest)
{
    rmqsProducer *Producer = (rmqsProducer *)Parameters;
    rmqsProperty Properties[6];

    memset(Properties, 0, sizeof(Properties));

    strncpy(Properties[0].Key, "connection_name", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[0].Value, "c-stream-locator", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[1].Key, "product", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[1].Value, "RabbitMQ Stream", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[2].Key, "copyright", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[2].Value, "Copyright (c) Undefined", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[3].Key, "information", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[3].Value, "Licensed under the MPL 2.0. See https://www.rabbitmq.com/", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[4].Key, "version", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[4].Value, "1.0", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[5].Key, "platform", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[5].Value, "C", RMQS_MAX_VALUE_SIZE);

    while (! *TerminateRequest)
    {
        switch (Producer->Status)
        {
            case rmqspsNotConnected:
                if (rmqsConnect(Producer->Host, Producer->Port, Producer->Socket, 2000))
                {
                    Producer->Status = rmqspsTcpConnected;
                }

                break;

            case rmqspsTcpConnected:
                if (rmqsPeerPropertiesRequest(Producer, Producer->CorrelationId++, 6, Properties) == rmqsrOK)
                {
                    Producer->Status = rmqspsReady;
                }
                else
                {
                    Producer->Status = rmqspsNotConnected;
                }

                break;

            case rmqspsReady:
                Producer->Status = Producer->Status;
                
                break;
        }

        rmqsThreadSleepEx(2, 5, TerminateRequest);
    }
}
//---------------------------------------------------------------------------
