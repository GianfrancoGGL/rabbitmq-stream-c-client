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
#include "rawClient/rmqsClientConfiguration.h"
#include "rawClient/rmqsBroker.h"
#include "rawClient/rmqsProducer.h"
#include "rawClient/rmqsMemory.h"
#include "rawClient/rmqsThread.h"
#include "rawClient/rmqsError.h"
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
#define ROW_SEPARATOR     "============================================================================"
#define MESSAGE_COUNT     10
#define NO_OF_ITERATIONS  10
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, uint64_t PublishingId, bool_t Confirmed, uint16_t Code);
size_t MessagesConfirmed = 0;
size_t MessagesNotConfirmed = 0;
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    (void)argc;
    (void)argv;

    char *BrokerList = "rabbitmq-stream://gian:ggi@192.168.56.1:5552";
    rmqsClientConfiguration_t *ClientConfiguration;
    char Error[RMQS_ERR_MAX_STRING_LENGTH];
    rmqsBroker_t *Broker;
    rmqsProducer_t *Producer;
    const int PublisherId = 1;
    char *StreamName = "MY-STREAM";
    rqmsCreateStreamArgs_t CreateStreamArgs = {0};
    rmqsResponseCode_t CreateStreamResponse;
    rmqsTimer_t *ElapseTimer, *PerformanceTimer;
    rmqsSocket Socket;                             
    rmqsProperty_t Properties[6];
    rmqsMessage_t *MessageBatch;
    uint32_t PublishWaitingTime = 20000;
    uint64_t PublishingId = 0;
    size_t i, j;
    size_t UsedMemory;

    (void)argc;
    (void)argv;

    rmqsInitWinsock();

    //
    // Fill the client properties
    //
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

    ClientConfiguration = rmqsClientConfigurationCreate(BrokerList, true, "C:\\TEMP\\RMQS_LOG.TXT", Error, sizeof(Error));

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
    Producer = rmqsProducerCreate(ClientConfiguration, "Publisher", PublishResultCallback);
    printf("Producer created\r\n");

    Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, 0);

    if (Broker)
    {
        Socket = rmqsSocketCreate();
        rmqsSetTcpNoDelay(Socket);
        rmqsSetSocketTxRxBuffers(Socket, 2000000, 2000000);

        if (rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
        {
            printf("Connected to %s\r\n", Broker->Hostname);

            if (rqmsClientLogin(Producer->Client, Socket, Broker->VirtualHost, Properties, 6) == rmqsrOK)
            {
                printf("Logged in to %s\r\n", Broker->Hostname);

                if (rmqsDelete(Producer->Client, Socket, StreamName))
                {
                    printf("Deleted stream %s\r\n", StreamName);
                }
                else
                {
                    printf("Cannot delete stream %s\r\n", StreamName);
                }

                CreateStreamArgs.SpecifyMaxLengthBytes = true;
                CreateStreamArgs.MaxLengthBytes = 1000000000;

                CreateStreamArgs.SpecifyMaxAge = true;
                strcpy(CreateStreamArgs.MaxAge, "12h");

                CreateStreamArgs.SpecifyStreamMaxSegmentSizeBytes = true;
                CreateStreamArgs.StreamMaxSegmentSizeBytes = 100000000;

                CreateStreamArgs.SpecifyInitialClusterSize = true;
                CreateStreamArgs.InitialClusterSize = 1;

                CreateStreamArgs.SpecifyQueueLeaderLocator = true;
                CreateStreamArgs.LeaderLocator = rmqssllClientLocal;

                CreateStreamResponse = rmqsCreate(Producer->Client, Socket, StreamName, &CreateStreamArgs);

                if (CreateStreamResponse == rmqsrOK || CreateStreamResponse == rmqsrStreamAlreadyExists)
                {
                    if (CreateStreamResponse == rmqsrOK)
                    {
                        printf("Created stream %s\r\n", StreamName);
                    }
                    else
                    {
                        printf("Stream opened %s\r\n", StreamName);
                    }

                    if (rmqsDeclarePublisher(Producer, Socket, PublisherId, StreamName) == rmqsrOK)
                    {
                        MessageBatch = (rmqsMessage_t *)rmqsAllocateMemory(sizeof(rmqsMessage_t) * MESSAGE_COUNT);

                        for (i = 0; i < MESSAGE_COUNT; i++)
                        {
                            MessageBatch[i].PublishingId = 0; // Will be set later
                            MessageBatch[i].Data = "Hello world!";
                            MessageBatch[i].Size = 12;
                            MessageBatch[i].DeleteData = false;
                        }

                        PerformanceTimer = rmqsTimerCreate();
                        rmqsTimerStart(PerformanceTimer);

                        for (i = 0; i < NO_OF_ITERATIONS; i++)
                        {
                            for (j = 0; j < MESSAGE_COUNT; j++)
                            {
                                MessageBatch[j].PublishingId = ++PublishingId;
                            }

                            rmqsPublish(Producer, Socket, PublisherId, MessageBatch, MESSAGE_COUNT);
                        }

                        rmqsFreeMemory(MessageBatch);

                        printf("%d Messages - Elapsed time: %ums\r\n", MESSAGE_COUNT * NO_OF_ITERATIONS, rmqsTimerGetTime(PerformanceTimer));

                        rmqsTimerDestroy(PerformanceTimer);

                        printf("Wait for publishing %d seconds\r\n", PublishWaitingTime / 1000);

                        ElapseTimer = rmqsTimerCreate();
                        rmqsTimerStart(ElapseTimer);
                        printf("Timer begin: %u\r\n", rmqsTimerGetTime(ElapseTimer));

                        rmqsProducerPoll(Producer, Socket, PublishWaitingTime);

                        printf("Timer end: %u\r\n", rmqsTimerGetTime(ElapseTimer));
                        rmqsTimerDestroy(ElapseTimer);

                        if (rmqsDeletePublisher(Producer, Socket, PublisherId) != rmqsrOK)
                        {
                            printf("Cannot delete the publisher\r\n");
                        }
                        else
                        {
                            printf("Publisher deleted\r\n");
                        }
                    }
                    else
                    {
                        printf("Cannot declare the publisher\r\n");
                    }

                    if (rqmsClientLogout(Producer->Client, Socket, 0, "Regular shutdown"))
                    {
                        printf("Logged out\r\n");
                    }
                    else
                    {
                        printf("Cannot logout\r\n");
                    }
                }
                else
                {
                    printf("Cannot create stream %s\r\n", StreamName);
                }
            }
            else
            {
                printf("Cannot login to %s\r\n", Broker->Hostname);
            }
        }
        else
        {
            printf("Cannot connect to %s\r\n", Broker->Hostname);
        }

        rmqsSocketDestroy(&Socket);
    }

    printf("Destroying producer...\r\n");
    rmqsProducerDestroy(Producer);
    printf("Producer destroyed\r\n");

    printf("Destroying configuration...\r\n");
    rmqsClientConfigurationDestroy(ClientConfiguration);
    printf("Client configuration destroyed\r\n");

    printf("Messages confirmed: %u/%u\r\n", MessagesConfirmed, MESSAGE_COUNT * NO_OF_ITERATIONS);
    printf("Messages not confirmed: %u/%u\r\n", MessagesNotConfirmed, MESSAGE_COUNT * NO_OF_ITERATIONS);

    UsedMemory = rmqsGetUsedMemory();

    printf("Unfreed memory: %u bytes\r\n", UsedMemory);

    rmqsThreadSleep(5000);

    rmqsShutdownWinsock();

    return 0;
}
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, uint64_t PublishingId, bool_t Confirmed, uint16_t Code)
{
    (void)PublisherId;
    (void)PublishingId;
    (void)Code;

    if (Confirmed)
    {
        MessagesConfirmed++;
    }
    else
    {
        MessagesNotConfirmed++;
    }
}
//---------------------------------------------------------------------------

