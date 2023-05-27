//---------------------------------------------------------------------------
#define ENABLE_CRT_MEM_LEAK_CHECK  0
#if ENABLE_CRT_MEM_LEAK_CHECK
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <memory.h>
#include <string.h>
#ifndef __BORLANDC__
#include <inttypes.h>
#endif
//---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

#include "rawClient/rmqsProtocol.h"
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
//---------------------------------------------------------------------------
#if __linux__ || __APPLE__
#define _strcmpi strcasecmp
#endif

#ifdef __BORLANDC__
#pragma comment(lib, "ws2_32.lib")
#endif
//---------------------------------------------------------------------------
#define ROW_SEPARATOR               "============================================================================"
#define PUBLISHER_REFERENCE         "Publisher"
#define CONSUMER_REFERENCE          "Consumer"
//---------------------------------------------------------------------------
typedef struct
{
    rmqsPublisher_t *Publisher;
    rmqsSocket_t Socket;
}
PollThreadParameters_t;
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, PublishResult_t *PublishResultList, size_t PublishingIdCount, bool_t Confirmed);
void DeliverResultCallback(uint8_t SubscriptionId, byte_t *Data, size_t DataSize, rmqsDeliverInfo_t *DeliverInfo, uint64_t MessageOffset, bool_t *StoreOffset);
void MetadataUpdateCallback(uint16_t Code, char_t *Stream);
//---------------------------------------------------------------------------
void PollThreadRoutineCallback(void *Parameters, bool_t *Terminate);
//---------------------------------------------------------------------------
size_t NoOfIterations = 10000;
size_t MessageCount = 100;
size_t ConsumerCreditSize = 1000;
//---------------------------------------------------------------------------
rmqsTimer_t *EncodingTimer = 0;
rmqsTimer_t *PublishTimer = 0;
rmqsTimer_t *PublishWaitTimer = 0;
rmqsTimer_t *PublishConfirmTimer = 0;
rmqsTimer_t *ConsumeTimer = 0;
rmqsTimer_t *DeliverTimer = 0;
//---------------------------------------------------------------------------
size_t MessagesConfirmed = 0;
size_t MessagesNotConfirmed = 0;
size_t MessagesReceived = 0;
//---------------------------------------------------------------------------
bool_t EnableLogging = false;
bool_t TestPublishError = false;
//---------------------------------------------------------------------------
uint32_t TimerResult;
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    char_t *BrokerList = "rabbitmq-stream://rabbit:rabbit@192.168.56.1:5552";
    char_t Error[RMQS_ERR_MAX_STRING_LENGTH] = {0};
    rmqsResponseCode_t ResponseCode = rmqsrOK;
    rmqsClientConfiguration_t *ClientConfiguration = 0;
    rmqsBroker_t *Broker = 0;
    rmqsPublisher_t *Publisher = 0;
    rmqsConsumer_t *Consumer = 0;
    uint8_t PublisherId = 1;
    uint8_t SubscriptionId = 1;
    char_t *Stream = "MY-STREAM";
    rmqsMetadata_t *Metadata = 0;
    uint64_t Sequence;
    rmqsCreateStreamArgs_t CreateStreamArgs = {0};
    bool_t StreamAlredyExists;
    rmqsSocket_t Socket;
    bool_t ConnectionError = false;
    rmqsProperty_t Properties[6];
    rmqsMessage_t *MessageBatch = 0;
    uint32_t PublishWaitingMaxTime = 10 * 1000; // seconds
    uint32_t ConsumeWaitingTime = 5 * 1000; // seconds
    uint64_t PublishingId = 0;
    bool_t ValidOffset;
    uint64_t Offset;
    size_t i, j;
    size_t UsedMemory;
    rmqsThread_t *PollThread;
    PollThreadParameters_t PollThreadParameters;
    uchar_t MessageBody[] = "Hello world!";
    size_t MessageBodySize = 12;

    //
    // First argument is the application path, skipped
    //
    for (i = 1; i < (size_t)argc; i++)
    {
        if (! _strcmpi(argv[i], "--brokers") && i < (size_t)(argc - 1))
        {
            BrokerList = argv[i + 1];
            i++;
        }
        else if (! _strcmpi(argv[i], "--iterations") && (size_t)(argc - 1))
        {
            NoOfIterations = (size_t)atoi(argv[i + 1]);
            i++;
        }
        else if (! _strcmpi(argv[i], "--messagecount") && (size_t)(argc - 1))
        {
            MessageCount = (size_t)atoi(argv[i + 1]);
            i++;
        }
    }

    EncodingTimer = rmqsTimerCreate();
    PublishTimer = rmqsTimerCreate();
    PublishWaitTimer = rmqsTimerCreate();
    PublishConfirmTimer = rmqsTimerCreate();
    ConsumeTimer = rmqsTimerCreate();
    DeliverTimer = rmqsTimerCreate();

    #if _WIN32 || _WIN64
    rmqsInitWinsock();
    #endif

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
    strncpy(Properties[2].Value, "Copyright (c) Gianfranco Giugliano", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[3].Key, "information", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[3].Value, "Licensed under the MIT license", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[4].Key, "version", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[4].Value, "1.0", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[5].Key, "platform", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[5].Value, "C", RMQS_MAX_VALUE_SIZE);

    ClientConfiguration = rmqsClientConfigurationCreate(BrokerList, EnableLogging, "/tmp/rmqs-log.txt", Error, sizeof(Error));

    if (! ClientConfiguration)
    {
        printf("rmqsClientConfigurationCreate - Error: %s\r\n\r\n", Error);
        goto CLEAN_UP;
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
        printf("No valid brokers: %s\r\n\r\n", Error);
        goto CLEAN_UP;
    }

    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //
    // Publisher example
    //
    //---------------------------------------------------------------------------
    printf("Creating publisher...\r\n");
    Publisher = rmqsPublisherCreate(ClientConfiguration, PUBLISHER_REFERENCE, 0, PublishResultCallback, MetadataUpdateCallback);
    printf("Publisher created\r\n");

    Socket = rmqsSocketCreate();

    if (! rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
    {
        printf("Cannot connect to %s\r\n", Broker->Hostname);
        goto CLEAN_UP;
    }

    rmqsSetSocketTxRxBufferSize(Socket, 1024 * 10000, 1024 * 10000);

    printf("Connected to server %s\r\n", Broker->Hostname);

    if (! rmqsClientLogin(Publisher->Client, Socket, Broker->VirtualHost, Properties, 6, &ResponseCode))
    {
        printf("Cannot login to server %s\r\n", Broker->Hostname);
        goto CLEAN_UP;
    }

    printf("Logged in to server %s\r\n", Broker->Hostname);

    if (rmqsDelete(Publisher->Client, Socket, Stream, &ResponseCode))
    {
        printf("Deleted stream %s\r\n", Stream);
    }
    else
    {
        printf("Cannot delete stream %s\r\n", Stream);

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
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

    if (! rmqsCreate(Publisher->Client, Socket, Stream, &CreateStreamArgs, &StreamAlredyExists, &ResponseCode) && ! StreamAlredyExists)
    {
        printf("Cannot create stream %s\r\n", Stream);
        goto CLEAN_UP;
    }

    if (! StreamAlredyExists)
    {
        printf("Created stream %s\r\n", Stream);
    }
    else
    {
        printf("Stream opened %s\r\n", Stream);
    }

    if (! TestPublishError)
    {
        if (! rmqsDeclarePublisher(Publisher, Socket, PublisherId, Stream, &ResponseCode))
        {
            printf("Cannot declare the publisher\r\n");
            goto CLEAN_UP;
        }
    }
    else
    {
        //
        // Test publish error declaring a publisher for a non-existing stream
        //
        rmqsDeclarePublisher(Publisher, Socket, PublisherId, "XYZ", &ResponseCode);
    }

    MessageBatch = (rmqsMessage_t *)rmqsAllocateMemory(sizeof(rmqsMessage_t) * MessageCount);

	for (i = 0; i < MessageCount; i++)
	{
		MessageBatch[i].Data = MessageBody;
		MessageBatch[i].Size = MessageBodySize;
		MessageBatch[i].DeleteData = false;
	}

    //---------------------------------------------------------------------------
    printf("Starting PollThread...\r\n");

    PollThreadParameters.Publisher = Publisher;
    PollThreadParameters.Socket = Socket;

    PollThread = rmqsThreadCreate(PollThreadRoutineCallback, 0, &PollThreadParameters);

    rmqsTimerStart(PublishWaitTimer);

    rmqsThreadStart(PollThread);

    printf("PollThread started\r\n");
    //---------------------------------------------------------------------------

    rmqsTimerStart(PublishTimer);

    for (i = 0; i < NoOfIterations; i++)
    {
        for (j = 0; j < MessageCount; j++)
        {
            MessageBatch[j].PublishingId = ++PublishingId;
        }

        rmqsPublish(Publisher, Socket, PublisherId, MessageBatch, MessageCount);
    }

    TimerResult = rmqsTimerGetTime(PublishTimer);
    printf("%d Messages - (CNT: %u - IT: %u) - Elapsed time: %ums\r\n", (int)(MessageCount * NoOfIterations), (uint32_t)MessageCount, (uint32_t)NoOfIterations, TimerResult);

    //---------------------------------------------------------------------------
    while (rmqsTimerGetTime(PublishWaitTimer) < PublishWaitingMaxTime && MessagesConfirmed < MessageCount * NoOfIterations)
    {
        rmqsThreadSleep(100);
    }

    printf("Stopping PollThread...\r\n");

    rmqsThreadStop(PollThread);
    rmqsThreadDestroy(PollThread);

    printf("PollThread stopped\r\n");
    //---------------------------------------------------------------------------

    rmqsBatchDestroy(MessageBatch, MessageCount);

    rmqsQueryPublisherSequence(Publisher, Socket, Stream, &Sequence, &ResponseCode);

    if (ResponseCode == rmqsrConnectionError)
    {
        goto CLEAN_UP;
    }

    #ifndef __BORLANDC__
    printf("Sequence number: %" PRIu64 "\r\n", Sequence);
    #else
    printf("Sequence number: %lld\r\n", Sequence);
    #endif

    if (rmqsDeletePublisher(Publisher, Socket, PublisherId, &ResponseCode))
    {
        printf("Publisher deleted\r\n");
    }
    else
    {
        printf("Cannot delete the publisher\r\n");

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
    }

    rmqsHeartbeat(Publisher->Client, Socket);

    if (rmqsClientLogout(Publisher->Client, Socket, 0, "Regular shutdown", &ResponseCode))
    {
        printf("Logged out\r\n");
    }
    else
    {
        printf("Cannot logout\r\n");

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
    }

    printf("Messages confirmed: %u/%u\r\n", (uint32_t)MessagesConfirmed, (uint32_t)(MessageCount * NoOfIterations));
    printf("Messages not confirmed: %u/%u\r\n", (uint32_t)MessagesNotConfirmed, (uint32_t)(MessageCount * NoOfIterations));

    rmqsSocketDestroy(&Socket);

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
    Consumer = rmqsConsumerCreate(ClientConfiguration, CONSUMER_REFERENCE, 0, 0, (uint16_t)ConsumerCreditSize, DeliverResultCallback, MetadataUpdateCallback);
    printf("Consumer created - credit size: %u\r\n", (uint32_t)ConsumerCreditSize);

    Socket = rmqsSocketCreate();

    if (! rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
    {
        printf("Cannot connect to %s\r\n", Broker->Hostname);
        goto CLEAN_UP;
    }

    printf("Connected to server %s\r\n", Broker->Hostname);

    if (! rmqsClientLogin(Consumer->Client, Socket, Broker->VirtualHost, Properties, 6, &ResponseCode))
    {
        printf("Cannot login to server%s\r\n", Broker->Hostname);
        goto CLEAN_UP;
    }

    printf("Logged in to server %s\r\n", Broker->Hostname);

    Metadata = rmqsMetadataCreate();

    if (rmqsMetadata(Publisher->Client, Socket, &Stream, 1, Metadata, &ResponseCode))
    {
        printf("Metadata retrieved for stream %s\r\n", Stream);
        rmqsMetadataDestroy(Metadata);
    }
    else
    {
        printf("Cannot retrieve the metadata for stream %s\r\n", Stream);
        rmqsMetadataDestroy(Metadata);
        goto CLEAN_UP;
    }

    if (rmqsSubscribe(Consumer, Socket, SubscriptionId, Stream, rmqsotOffset, 0, Consumer->DefaultCredit, 0, 0, &ResponseCode))
    {
        printf("Subscribed to stream %s\r\n", Stream);
    }
    else
    {
        printf("Cannot subscribe to stream %s\r\n", Stream);
        goto CLEAN_UP;
    }

    printf("Consumer - Timer begin\r\n");

    rmqsTimerStart(ConsumeTimer);

    rmqsConsumerPoll(Consumer, Socket, ConsumeWaitingTime, &ConnectionError);

    if (ConnectionError)
    {
        goto CLEAN_UP;
    }

    TimerResult = rmqsTimerGetTime(ConsumeTimer);
    printf("Consumer - Timer end: %u\r\n", TimerResult);

    if (rmqsQueryOffset(Consumer, Socket, CONSUMER_REFERENCE, Stream, &ValidOffset, &Offset, &ResponseCode))
    {
        #ifndef __BORLANDC__
        printf("QueryOffset - Offset: %" PRIu64 " - Is valid: %d\r\n", Offset, (int)ValidOffset);
        #else
        printf("QueryOffset - Offset: %lld - Is valid: %d\r\n", Offset, (int)ValidOffset);
        #endif
    }
    else
    {
        printf("QueryOffset error\r\n");

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
    }

    if (rmqsUnsubscribe(Consumer, Socket, SubscriptionId, &ResponseCode))
    {
        printf("Unsubscribed from stream %s\r\n", Stream);
    }
    else
    {
        printf("Cannot unsubcribe from stream %s\r\n", Stream);

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
    }

    rmqsHeartbeat(Publisher->Client, Socket);

    if (rmqsClientLogout(Consumer->Client, Socket, 0, "Regular shutdown", &ResponseCode))
    {
        printf("Logged out\r\n");
    }
    else
    {
        printf("Cannot logout\r\n");

        if (ResponseCode == rmqsrConnectionError)
        {
            goto CLEAN_UP;
        }
    }

    rmqsSocketDestroy(&Socket);
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------

CLEAN_UP:
    if (ConnectionError)
    {
        printf("Connection lost\r\n");
    }

    if (ResponseCode != rmqsrOK)
    {
        printf("Response code: %s\r\n", rmqsGetResponseCodeDescription(ResponseCode));
    }

    if (Publisher)
    {
        rmqsPublisherDestroy(Publisher);
    }

    if (Consumer)
    {
        rmqsConsumerDestroy(Consumer);
    }

    if (ClientConfiguration)
    {
        rmqsClientConfigurationDestroy(ClientConfiguration);
    }

    if (EncodingTimer)
    {
        rmqsTimerDestroy(EncodingTimer);
    }

    if (PublishTimer)
    {
        rmqsTimerDestroy(PublishTimer);
    }

    if (PublishWaitTimer)
    {
        rmqsTimerDestroy(PublishWaitTimer);
    }

    if (PublishConfirmTimer)
    {
        rmqsTimerDestroy(PublishConfirmTimer);
    }

    if (ConsumeTimer)
    {
        rmqsTimerDestroy(ConsumeTimer);
    }

    if (DeliverTimer)
    {
        rmqsTimerDestroy(DeliverTimer);
    }

    UsedMemory = rmqsGetUsedMemory();

    printf("Unfreed memory: %u bytes\r\n", (uint32_t)UsedMemory);

    rmqsThreadSleep(5000);

    #if _WIN32 || _WIN64
    rmqsShutdownWinsock();
    #endif

    #if ENABLE_CRT_MEM_LEAK_CHECK
    HANDLE hLogFile = CreateFile(L"C:/TEMP/MemoryLeaks.txt", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, hLogFile);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, hLogFile);
    _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile (_CRT_ASSERT, hLogFile);
    _CrtDumpMemoryLeaks();

    CloseHandle(hLogFile);
    #endif

    return 0;
}
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, PublishResult_t *PublishResultList, size_t PublishingIdCount, bool_t Confirmed)
{
    size_t i;

    (void)PublisherId;
    (void)PublishResultList;

    for (i = 0; i < PublishingIdCount; i++)
    {
        if (Confirmed)
        {
            MessagesConfirmed++;
        }
        else
        {
            MessagesNotConfirmed++;
        }

        if (MessagesConfirmed == 1)
        {
            rmqsTimerStart(PublishConfirmTimer);
        }
        else if (MessagesConfirmed == MessageCount * NoOfIterations)
        {
            TimerResult = rmqsTimerGetTime(PublishConfirmTimer);
            printf("%u Messages - Confirm time: %ums\r\n", (uint32_t)(MessageCount * NoOfIterations), TimerResult);
        }
    }
}
//---------------------------------------------------------------------------
void DeliverResultCallback(uint8_t SubscriptionId, byte_t *Data, size_t DataSize, rmqsDeliverInfo_t *DeliverInfo, uint64_t MessageOffset, bool_t *StoreOffset)
{
    (void)SubscriptionId;
    (void)DataSize;
    (void)Data;
    (void)DeliverInfo;
    (void)MessageOffset;

    MessagesReceived++;

    *StoreOffset = MessagesReceived == 1 || MessagesReceived == MessageCount * NoOfIterations || MessagesReceived % 1000 == 0;

    if (MessagesReceived == 1)
    {
        rmqsTimerStart(DeliverTimer);
    }
    else if (MessagesReceived == MessageCount * NoOfIterations)
    {
        TimerResult = rmqsTimerGetTime(DeliverTimer);
        printf("%u Messages - Receive time: %ums\r\n", (uint32_t)(MessageCount * NoOfIterations), TimerResult);
        printf("Last message: %.*s\r\n", (int)DataSize, (char *)Data);
    }
}
//---------------------------------------------------------------------------
void MetadataUpdateCallback(uint16_t Code, char_t *Stream)
{
    (void)Code;
    (void)Stream;
}
//---------------------------------------------------------------------------
void PollThreadRoutineCallback(void *Parameters, bool_t *Terminate)
{
    PollThreadParameters_t *PollThreadParameters = (PollThreadParameters_t *)Parameters;
    bool_t ConnectionError = false;

    while (! *Terminate)
    {
        rmqsPublisherPoll(PollThreadParameters->Publisher, PollThreadParameters->Socket, 1000, &ConnectionError);
    }
}
//---------------------------------------------------------------------------
