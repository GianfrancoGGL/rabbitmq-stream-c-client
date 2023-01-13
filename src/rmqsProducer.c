//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsProducer.h"
#include "rmqsEnvironment.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(void *Environment, void (*EventsCB)(rqmsProducerEvent, void *))
{
    rmqsProducer_t *Producer = (rmqsProducer_t *)rmqsAllocateMemory(sizeof(rmqsProducer_t));

    memset(Producer, 0, sizeof(rmqsProducer_t));

    Producer->Environment = Environment;
    Producer->Status = rmqspsDisconnected;
    Producer->EventsCB = EventsCB;

    //
    // Windows machine, initialize sockets
    //
    #ifdef __WIN32__
    rmqsInitWinsock();
    #endif

    Producer->Socket = rmqsInvalidSocket;
    Producer->CorrelationId = 0;

    Producer->TxStream = rmqsStreamCreate();
    Producer->RxStream = rmqsStreamCreate();
    Producer->RxStreamTempBuffer = rmqsStreamCreate();

    Producer->ProducerThread = rmqsThreadCreate(rmqsProducerThreadRoutine, 0, Producer);
    rmqsThreadStart(Producer->ProducerThread);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsProducerDestroy(rmqsProducer_t *Producer)
{
    if (Producer->Socket != rmqsInvalidSocket)
    {
        rmqsSocketDestroy((rmqsSocket *)&Producer->Socket);
        Producer->Status = rmqspsDisconnected;
        Producer->EventsCB(rmqspeDisconnected, Producer);
    }

    rmqsThreadStop(Producer->ProducerThread);
    rmqsThreadDestroy(Producer->ProducerThread);

    rmqsStreamDestroy(Producer->TxStream);
    rmqsStreamDestroy(Producer->RxStream);
    rmqsStreamDestroy(Producer->RxStreamTempBuffer);

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
    uint8_t ConnectionFailed;

    rmqsProducer_t *Producer = (rmqsProducer_t *)Parameters;
    rmqsEnvironment_t *Environment = Producer->Environment;
    rmqsBroker_t *Broker;
    rmqsProperty_t Properties[6];

    Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Environment->BrokersList, 0);

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
        if (Broker == 0)
        {
            //
            // Brokers list is empty!
            //
            rmqsThreadSleepEx(2, 1000, TerminateRequest);
            continue;
        }

        ConnectionFailed = 0;

        switch (Producer->Status)
        {
            case rmqspsDisconnected:
                Producer->Socket = rmqsSocketCreate();

                rmqsSetTcpNoDelay(Producer->Socket);
                rmqsSetSocketWriteTimeouts(Producer->Socket, 5);
                rmqsSetKeepAlive(Producer->Socket);

                if (rmqsSocketConnect(Broker->Host, Broker->Port, Producer->Socket, 2000))
                {
                    Producer->Status = rmqspsConnected;
                    Producer->EventsCB(rmqspeConnected, Producer);
                }
                else
                {
                    ConnectionFailed = 1;
                    rmqsSocketDestroy((rmqsSocket *)&Producer->Socket);
                }

                break;

            case rmqspsConnected:
                if (rmqsPeerPropertiesRequest(Producer, Producer->CorrelationId++, 6, Properties) == rmqsrOK)
                {
                    Producer->Status = rmqspsReady;
                    Producer->EventsCB(rmqspeReady, Producer);
                }
                else
                {
                    ConnectionFailed = 1;
                    rmqsSocketDestroy((rmqsSocket *)&Producer->Socket);
                    Producer->Status = rmqspsDisconnected;
                    Producer->EventsCB(rmqspeDisconnected, Producer);
                }

                break;

            case rmqspsReady:
                Producer->Status = Producer->Status;

                break;
        }

        if (! ConnectionFailed)
        {
            rmqsThreadSleepEx(2, 1, TerminateRequest);
        }
        else
        {
            rmqsThreadSleepEx(2, 50, TerminateRequest);
        }
    }
}
//---------------------------------------------------------------------------
