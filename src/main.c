//---------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include <string.h>
#ifdef __WIN32__
#include <conio.h>
#endif
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
#include "rmqsEnvironment.h"
#include "rmqsTimer.h"
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
void ProducerEventsCallback(rqmsClientEvent Event, void *EventData);
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    char *Brokers = "192.168.56.1:5552,192.168.1.37:5553";
    rmqsEnvironment_t *Environment;
    rmqsBroker_t *Broker;
    rmqsProducer_t *Producer;
    rmqsTimer_t *Timer;
    size_t i;

    (void)argc;
    (void)argv;

    #ifdef __BORLANDC__
    StartLeakChecking(false);
    #endif

    printf("Creating environment...\r\n");
    Environment = rmqsEnvironmentCreate(Brokers, "gian", "ggi", 1, "C:\\TEMP\\CommLog.txt");
    printf("Environment created\r\n");

    printf("No of brokers defined: %d\r\n", (int)Environment->BrokersList->Count);

    for (i = 0; i < Environment->BrokersList->Count; i++)
    {
        Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Environment->BrokersList, i);

        printf("Broker %d: %s - %d\r\n", (int)(i + 1), Broker->Host, (int)Broker->Port);
    }

    printf("Creating client...\r\n");
    Producer = rmqsEnvironmentProducerCreate(Environment, "/", 1, "Publisher", ProducerEventsCallback);
    printf("Producer created\r\n");

    printf("Running for 10 seconds\r\n");

    Timer = rmqsTimerCreate();
    rmqsTimerStart(Timer);

    while (rmqsTimerGetTime(Timer) < 30000)
    {
        rmqsThreadSleep(100);
    }

    rmqsTimerDestroy(Timer);

    printf("Destroying client...\r\n");
    rmqsEnvironmentProducerDestroy(Environment, Producer);
    printf("Producer destroyed\r\n");

    printf("Destroying environment...\r\n");
    rmqsEnvironmentDestroy(Environment);
    printf("Environment destroyed\r\n");

    rmqsThreadSleep(5000);

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

