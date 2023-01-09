//---------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include <string.h>
#ifdef __WIN32__
#include <conio.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsEnvironment.h"
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma comment(lib, "ws2_32.lib")
#endif
//---------------------------------------------------------------------------
void ProducerEventsCallback(rqmsProducerEvent Event, void *EventData);
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    rmqsEnvironment *Environment;
    rmqsProducer *Producer;

    printf("Creating environment...\r\n");
    Environment = rmqsEnvironmentCreate();
    printf("Environment created\r\n");

    printf("Creating producer...\r\n");
    Producer = rmqsEnvironmentProducerCreate(Environment, "192.168.1.37", 5552, ProducerEventsCallback);
    printf("Producer created\r\n");

    rmqsThreadSleep(10000);

    printf("Destroying producer...\r\n");
    rmqsEnvironmentProducerDestroy(Environment, Producer);
    printf("Producer destroyed\r\n");

    printf("Destroying environment...\r\n");
    rmqsEnvironmentDestroy(Environment);
    printf("Environment destroyed\r\n");

    rmqsThreadSleep(5000);

    return 0;
}
//---------------------------------------------------------------------------
void ProducerEventsCallback(rqmsProducerEvent Event, void *EventData)
{
    switch (Event)
    {
        case rmqspeConnected:
            printf("Producer connected\r\n");
            break;

        case rmqspeDisconnected:
            printf("Producer disconnected\r\n");
            break;

        case rmqspeReady:
            printf("Producer ready\r\n");
            break;
    }
}
//---------------------------------------------------------------------------

