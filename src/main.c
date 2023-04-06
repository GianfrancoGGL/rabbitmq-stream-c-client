//---------------------------------------------------------------------------
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <inttypes.h>
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
#include "rawClient/rmqsClientConfiguration.h"
#include "rawClient/rmqsBroker.h"
#include "rawClient/rmqsPublisher.h"
#include "rawClient/rmqsConsumer.h"
#include "rawClient/rmqsMemory.h"
#include "rawClient/rmqsThread.h"
#include "rawClient/rmqsLib.h"
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
#define ROW_SEPARATOR              "============================================================================"
#define NO_OF_ITERATION             1
#define MESSAGE_COUNT         1000000
#define CONSUMER_CREDIT_SIZE     1000
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, uint64_t *PublishingIdList, size_t PublishingIdCount, bool_t Confirmed, uint16_t Code);
void DeliverResultCallback(uint8_t SubscriptionId, size_t DataSize, void *Data);
//---------------------------------------------------------------------------
rmqsTimer_t *PerformanceTimer;
rmqsTimer_t *ElapseTimer;
//---------------------------------------------------------------------------
size_t MessagesConfirmed = 0;
size_t MessagesNotConfirmed = 0;
size_t MessagesReceived = 0;
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    char *BrokerList = "rabbitmq-stream://gian:ggi@127.0.0.1:5552";
    rmqsClientConfiguration_t *ClientConfiguration;
    char Error[RMQS_ERR_MAX_STRING_LENGTH];
    rmqsBroker_t *Broker;
    rmqsPublisher_t *Publisher;
    int PublisherId = 1;
    rmqsConsumer_t *Consumer;
    int SubscriptionId = 1;
    char *StreamName = "MY-STREAM";
    uint64_t Sequence;
    rqmsCreateStreamArgs_t CreateStreamArgs = {0};
    rmqsResponseCode_t CreateStreamResponse;
    rmqsTimer_t *ElapseTimer;
    rmqsSocket Socket;
    bool_t ConnectionLost;
    rmqsProperty_t Properties[6];
    rmqsMessage_t *MessageBatch;
    uint32_t PublishWaitingTime = 5000;
    uint32_t ConsumeWaitingTime = 10000;
    uint64_t PublishingId = 0;
    size_t i, j;
    size_t UsedMemory;

    (void)argc;
    (void)argv;

    //---------------------------------------------------------------------------
    //
    // Fill the client properties
    //
    //---------------------------------------------------------------------------
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

    ClientConfiguration = rmqsClientConfigurationCreate(BrokerList, false, 0, Error, sizeof(Error));

    if (ClientConfiguration == 0)
    {
        printf("rmqsClientConfigurationCreate - Error: %s\r\n\r\n", Error);
        return 0;
    }

    printf("%s\r\nNo of brokers defined: %d\r\n%s\r\n", ROW_SEPARATOR, (int32_t)ClientConfiguration->BrokerList->Count, ROW_SEPARATOR);

    for (i = 0; i < ClientConfiguration->BrokerList->Count; i++)
    {
        Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, i);

        printf("%d - Host: %s - Port: %d - User: %s - Pass: %s\r\nSchema: %s - VHost: %s - TLS: %d\r\n%s\r\n", (int)(i + 1),
               Broker->Hostname, (int)Broker->Port, Broker->Username, Broker->Password,
               Broker->DBSchema, Broker->VirtualHost, Broker->UseTLS ? 1 : 0, ROW_SEPARATOR);
    }

    Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, 0);

    if (! Broker)
    {
        rmqsClientConfigurationDestroy(ClientConfiguration);
        return 0;
    }

    PerformanceTimer = rmqsTimerCreate();
    ElapseTimer = rmqsTimerCreate();

    #if _WIN32 || _WIN64
    rmqsInitWinsock();
    #endif

    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //
    // Publisher example
    //
    //---------------------------------------------------------------------------
    printf("Creating publisher...\r\n");
    Publisher = rmqsPublisherCreate(ClientConfiguration, "Publisher", 0, PublishResultCallback);
    printf("Publisher created\r\n");

    Socket = rmqsSocketCreate();

    if (rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
    {
        printf("Connected to %s\r\n", Broker->Hostname);

        if (rqmsClientLogin(Publisher->Client, Socket, Broker->VirtualHost, Properties, 6) == rmqsrOK)
        {
            printf("Logged in to %s\r\n", Broker->Hostname);

            if (rmqsDelete(Publisher->Client, Socket, StreamName))
            {
                printf("Deleted stream %s\r\n", StreamName);
            }
            else
            {
                printf("Cannot delete stream %s\r\n", StreamName);
            }

            CreateStreamArgs.SetMaxLengthBytes = true;
            CreateStreamArgs.MaxLengthBytes = 1000000000;

            CreateStreamArgs.SetMaxAge = true;
            strcpy(CreateStreamArgs.MaxAge, "12h");

            CreateStreamArgs.SetStreamMaxSegmentSizeBytes = true;
            CreateStreamArgs.StreamMaxSegmentSizeBytes = 100000000;

            CreateStreamArgs.SetInitialClusterSize = true;
            CreateStreamArgs.InitialClusterSize = 1;

            CreateStreamArgs.SetQueueLeaderLocator = true;
            CreateStreamArgs.LeaderLocator = rmqssllClientLocal;

            CreateStreamResponse = rmqsCreate(Publisher->Client, Socket, StreamName, &CreateStreamArgs);

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

                if (rmqsDeclarePublisher(Publisher, Socket, PublisherId, StreamName) == rmqsrOK)
                {
                    MessageBatch = (rmqsMessage_t *)rmqsAllocateMemory(sizeof(rmqsMessage_t) * MESSAGE_COUNT);

                    for (i = 0; i < MESSAGE_COUNT; i++)
                    {
                        MessageBatch[i].Data = "Hello world!";
                        MessageBatch[i].Size = 12;
                        MessageBatch[i].DeleteData = false;
                    }

                    rmqsTimerStart(PerformanceTimer);

                    for (i = 0; i < NO_OF_ITERATION; i++)
                    {
                        for (j = 0; j < MESSAGE_COUNT; j++)
                        {
                            MessageBatch[j].PublishingId = ++PublishingId;
                        }

                        rmqsPublish(Publisher, Socket, PublisherId, MessageBatch, MESSAGE_COUNT);
                    }

                    rmqsBatchDestroy(MessageBatch, MESSAGE_COUNT);

                    printf("%d Messages - Elapsed time: %ums\r\n", MESSAGE_COUNT * NO_OF_ITERATION, rmqsTimerGetTime(PerformanceTimer));

                    rmqsQueryPublisherSequence(Publisher, Socket, StreamName, &Sequence);

                    printf("Sequence number: %" PRIu64 "\r\n", Sequence);

                    printf("Wait for publishing %d seconds\r\n", PublishWaitingTime / 1000);

                    printf("Publisher - Timer begin\r\n");

                    rmqsTimerStart(PerformanceTimer);
                    rmqsTimerStart(ElapseTimer);

                    rmqsPublisherPoll(Publisher, Socket, PublishWaitingTime, &ConnectionLost);

                    printf("Publisher - Timer end: %u\r\n", rmqsTimerGetTime(ElapseTimer));

                    if (rmqsDeletePublisher(Publisher, Socket, PublisherId) != rmqsrOK)
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

                if (rqmsClientLogout(Publisher->Client, Socket, 0, "Regular shutdown"))
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

    printf("Messages confirmed: %u/%u\r\n", (uint32_t)MessagesConfirmed, MESSAGE_COUNT * NO_OF_ITERATION);
    printf("Messages not confirmed: %u/%u\r\n", (uint32_t)MessagesNotConfirmed, MESSAGE_COUNT * NO_OF_ITERATION);

    rmqsSocketDestroy(&Socket);

    rmqsPublisherDestroy(Publisher);
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------

    printf("%s\r\n", ROW_SEPARATOR);

    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //
    // Consumer example
    //
    //---------------------------------------------------------------------------
    printf("Creating consumer...\r\n");
    Consumer = rmqsConsumerCreate(ClientConfiguration, 0, 0, CONSUMER_CREDIT_SIZE, DeliverResultCallback);
    printf("Consumer created - credit size: %d\r\n", CONSUMER_CREDIT_SIZE);

    Socket = rmqsSocketCreate();

    if (rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
    {
        printf("Connected to %s\r\n", Broker->Hostname);

        if (rqmsClientLogin(Consumer->Client, Socket, Broker->VirtualHost, Properties, 6) == rmqsrOK)
        {
            printf("Logged in to %s\r\n", Broker->Hostname);
        }
        else
        {
            printf("Cannot login to %s\r\n", Broker->Hostname);
        }

        if (rmqsSubscribe(Consumer, Socket, SubscriptionId, StreamName, rmqsotOffset, 0, Consumer->DefaultCredit, 0, 0) == rmqsrOK)
        {
            printf("Subscribed to stream %s\r\n", StreamName);
        }
        else
        {
            printf("Cannot subscribe to stream %s\r\n", StreamName);
        }

        printf("Consumer - Timer begin\r\n");

        rmqsTimerStart(PerformanceTimer);
        rmqsTimerStart(ElapseTimer);

        rmqsConsumerPoll(Consumer, Socket, ConsumeWaitingTime, &ConnectionLost);

        printf("Consumer - Timer end: %u\r\n", rmqsTimerGetTime(ElapseTimer));

        if (rqmsClientLogout(Consumer->Client, Socket, 0, "Regular shutdown"))
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
        printf("Cannot connect to %s\r\n", Broker->Hostname);
    }

    rmqsSocketDestroy(&Socket);

    rmqsConsumerDestroy(Consumer);
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------

    rmqsClientConfigurationDestroy(ClientConfiguration);

    rmqsTimerDestroy(PerformanceTimer);
    rmqsTimerDestroy(ElapseTimer);

    UsedMemory = rmqsGetUsedMemory();

    printf("Unfreed memory: %u bytes\r\n", (uint32_t)UsedMemory);

    rmqsThreadSleep(5000);

    #if _WIN32 || _WIN64
    rmqsShutdownWinsock();
    #endif

    return 0;
}
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, uint64_t *PublishingIdList, size_t PublishingIdCount, bool_t Confirmed, uint16_t Code)
{
    size_t i;

    (void)PublisherId;
    (void)PublishingIdList;
    (void)Code;

    if (Confirmed)
    {
        for (i = 0; i < PublishingIdCount; i++)
        {
            if (++MessagesConfirmed == MESSAGE_COUNT * NO_OF_ITERATION)
            {
                printf("%d Messages - Confirm time: %ums\r\n", MESSAGE_COUNT * NO_OF_ITERATION, rmqsTimerGetTime(PerformanceTimer));
            }
        }
    }
    else
    {
        MessagesNotConfirmed++;
    }
}
//---------------------------------------------------------------------------
void DeliverResultCallback(uint8_t SubscriptionId, size_t DataSize, void *Data)
{
    if (++MessagesReceived == MESSAGE_COUNT * NO_OF_ITERATION)
    {
        printf("%d Messages - Receive time: %ums\r\n", MESSAGE_COUNT * NO_OF_ITERATION, rmqsTimerGetTime(PerformanceTimer));
    }
}
//---------------------------------------------------------------------------
