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
int main(int argc, char * argv[])
{
    rmqsEnvironment *Environment;
    rmqsProducer *Producer;

    printf("Creating environment...");
    Environment = rmqsEnvironmentCreate();
    printf("done\r\n");

    printf("Creating producer...");
    Producer = rmqsEnvironmentProducerCreate(Environment, "192.168.1.37", 5552);
    printf("done\r\n");

    #ifdef __WIN32__
    getch();
    #endif

    printf("Destroying producer...");
    rmqsEnvironmentProducerDestroy(Environment, Producer);
    printf("done\r\n");

    printf("Destroying environment...");
    rmqsEnvironmentDestroy(Environment);
    printf("done\r\n");

    #ifdef __WIN32__
    getch();
    #endif

    return 0;
}
//---------------------------------------------------------------------------

