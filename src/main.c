//---------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include <string.h>
#if _WIN32
#include <conio.h>
#endif
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
#include "rmqsClientConfiguration.h"
#include "rmqsBroker.h"
#include "rmqsProducer.h"
#include "rmqsThread.h"
#include "rmqsError.h"
#ifdef __cplusplus
}
#endif
#ifdef __BORLANDC__
#include "madExcept.h"
#endif
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma comment(lib, "ws2_32.lib")
#endif
//---------------------------------------------------------------------------
#define ROW_SEPARATOR "============================================================================"

void ProducerEventsCallback(rqmsClientEvent Event, void *EventData);
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    char *BrokerList = "rabbitmq-stream://guest:guest@localhost:5552"; // "rabbitmq-stream://guest:guest@169.254.190.108:5522/a-vhost1;rabbitmq-stream+tls://user2:pass2@host2:5521/a-vhost2";
    rmqsClientConfiguration_t *ClientConfiguration;
    char Error[RMQS_ERR_MAX_STRING_LENGTH];
    rmqsBroker_t *Broker;
    rmqsProducer_t *Producer;
    rmqsTimer_t *Timer;
    uint32_t SleepTime = 10;
    size_t i;

    (void)argc;
    (void)argv;

    ClientConfiguration = rmqsClientConfigurationCreate(BrokerList, false, 0, Error, sizeof(Error));

    if (ClientConfiguration == 0)
    {
        printf("rmqsClientConfigurationCreate - Error: %s\r\n\r\n", Error);
    }
    else
    {
        printf("%s\r\nNo of brokers defined: %d\r\n%s\r\n", ROW_SEPARATOR, (int32_t)ClientConfiguration->BrokerList->Count, ROW_SEPARATOR);

        for (i = 0; i < ClientConfiguration->BrokerList->Count; i++)
        {
            Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, i);

            printf("%d - Host: %s - Port: %d - User: %s - Pass: %s\r\nSchema: %s - VHost: %s - TLS: %d\r\n%s\r\n", (int)(i + 1),
                   Broker->Hostname, (int)Broker->Port, Broker->Username, Broker->Password,
                   Broker->DBSchema, Broker->VirtualHost, Broker->UseTLS ? 1 : 0, ROW_SEPARATOR);
        }
    }

    printf("Creating client...\r\n");
    Producer = rmqsProducerCreate(ClientConfiguration, RMQS_BROKER_DEFAULT_VHOST, 1, "Publisher", ProducerEventsCallback);
    printf("Producer created\r\n");

    printf("Running for %u seconds\r\n", SleepTime);

    Timer = rmqsTimerCreate();
    rmqsTimerStart(Timer);

    while (rmqsTimerGetTime(Timer) < (SleepTime * 1000))
    {
        rmqsThreadSleep(100);
    }

    rmqsTimerDestroy(Timer);

    printf("Destroying producer...\r\n");
    rmqsProducerDestroy(Producer);
    printf("Producer destroyed\r\n");

    printf("Destroying configuration...\r\n");
    rmqsClientConfigurationDestroy(ClientConfiguration);
    printf("Client configuration destroyed\r\n");

    rmqsThreadSleep(2000);

    return 0;
}
//---------------------------------------------------------------------------
void ProducerEventsCallback(rqmsClientEvent Event, void *EventData)
{
    (void)EventData;

    switch (Event)
    {
        case rmqsceConnected:
            printf("Producer connected\r\n");
            break;

        case rmqsceDisconnected:
            printf("Producer disconnected\r\n");
            break;

        case rmqsceReady:
            printf("Producer ready\r\n");
            break;
    }
}
//---------------------------------------------------------------------------
