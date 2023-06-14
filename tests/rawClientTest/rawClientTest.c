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

#include "unity.h"

#include "rmqsProtocol.h"
#include "rmqsClientConfiguration.h"
#include "rmqsBroker.h"
#include "rmqsPublisher.h"
#include "rmqsConsumer.h"
#include "rmqsMemory.h"
#include "rmqsThread.h"
#include "rmqsLib.h"
#include "rmqsError.h"

#ifdef __cplusplus
}
#endif
//---------------------------------------------------------------------------
#if __linux__ || __APPLE__
#define _strcmpi strcasecmp
#endif

#ifdef __BORLANDC__
#define _strcmpi strcmpi
#pragma comment(lib, "ws2_32.lib")
#endif
//---------------------------------------------------------------------------
#define RMQS_DB_SCHEMA              "rabbitmq-stream"
#define RMQS_USERNAME               "rabbit"
#define RMQS_PASSWORD               "rabbit"
#define RMQS_SERVER                 "127.0.0.1"
#define RMQS_PORT                   5552
#define RMQS_VIRTUAL_HOST           "/"
//---------------------------------------------------------------------------
#define ROW_SEPARATOR               "============================================================================"
#define PUBLISHER_REFERENCE         "Publisher"
#define CONSUMER_REFERENCE          "Consumer"
#define TEST_MESSAGE_DATA           "Hello world!"
#define TEST_MESSAGE_DATA_LONG      "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//---------------------------------------------------------------------------
void TestFunction(void);
//---------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(TestFunction);

    UNITY_END();

    return 0;
}
//---------------------------------------------------------------------------
void setUp(void)
{
    ;
}
//---------------------------------------------------------------------------
void tearDown(void)
{
    ;
}
//---------------------------------------------------------------------------
typedef struct
{
    rmqsPublisher_t *Publisher;
    rmqsSocket_t Socket;
}
PollThreadParameters_t;
//---------------------------------------------------------------------------
void PublishResultCallback(uint8_t PublisherId, PublishResult_t *PublishResultList, size_t PublishingIdCount, bool_t Confirmed);
void DeliverResultCallback(void *Consumer, uint8_t SubscriptionId, byte_t *Data, size_t DataSize, rmqsDeliverInfo_t *DeliverInfo, uint64_t MessageOffset, bool_t *StoreOffset);
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
bool_t TestAMQP1_0 = false;
bool_t SendLongMessages = false;
bool_t EnableLogging = false;
bool_t TestPublishError = false;
//---------------------------------------------------------------------------
uint32_t TimerResult;
//---------------------------------------------------------------------------
rmqsAMQP1_0DataFrame8 AMQP1_0DataFrame8;
rmqsAMQP1_0DataFrame32 AMQP1_0DataFrame32;
//---------------------------------------------------------------------------
void TestFunction(void)
{
    char_t BrokerList[128] = {0};
    char_t Error[RMQS_ERR_MAX_STRING_LENGTH] = {0};
    char_t OutputString[512] = {0};
    rmqsResponseCode_t ResponseCode = rmqsrOK;
    rmqsClientConfiguration_t *ClientConfiguration = 0;
    rmqsBroker_t *Broker = 0;
    rmqsPublisher_t *Publisher = 0;
    rmqsConsumer_t *Consumer = 0;
    size_t TxRxBufferSize = 0;
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
    uchar_t MessageBody[] = TEST_MESSAGE_DATA;
    size_t MessageBodySize = strlen(TEST_MESSAGE_DATA);
    uchar_t MessageBodyLong[] = TEST_MESSAGE_DATA_LONG;
    size_t MessageBodySizeLong = strlen(TEST_MESSAGE_DATA_LONG);

    sprintf(BrokerList, "%s://%s:%s@%s:%d/;%s://%s:%s@%s:%d/",
            "rabbitmq-stream+tls",
            "anyuser",
            "anypassword",
            "192.168.1.254",
            5553,
            RMQS_DB_SCHEMA,
            RMQS_USERNAME,
            RMQS_PASSWORD,
            RMQS_SERVER,
            RMQS_PORT);

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
        sprintf(OutputString, "rmqsClientConfigurationCreate - Error: %s", Error);
        TEST_FAIL_MESSAGE(OutputString);
    }

    printf("%s\r\nNo of brokers defined: %d\r\n%s\r\n", ROW_SEPARATOR, (int32_t)ClientConfiguration->BrokerList->Count, ROW_SEPARATOR);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ClientConfiguration->BrokerList->Count, 2, "Wrong no of brokers");

    for (i = 0; i < ClientConfiguration->BrokerList->Count; i++)
    {
        Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, i);

        printf("%d - Host: %s - Port: %d - User: %s - Pass: %s\r\nSchema: %s - VHost: %s - TLS: %d\r\n%s\r\n", (int)(i + 1),
               Broker->Hostname, (int)Broker->Port, Broker->Username, Broker->Password,
               Broker->DBSchema, Broker->VirtualHost, Broker->UseTLS ? 1 : 0, ROW_SEPARATOR);
    }

    //
    // Retrieve the second broker
    //
    Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(ClientConfiguration->BrokerList, 1);

    if (! Broker)
    {
        sprintf(OutputString, "No valid brokers: %s", Error);
        TEST_FAIL_MESSAGE(OutputString);
    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->Hostname, RMQS_SERVER, "Unexpected broker host name");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(Broker->Port, RMQS_PORT, "Unexpected broker port");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->Username, RMQS_USERNAME, "Unexpected broker user name");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->Password, RMQS_PASSWORD, "Unexpected broker password");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->DBSchema, RMQS_DB_SCHEMA, "Unexpected broker DB schema");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->VirtualHost, RMQS_VIRTUAL_HOST, "Unexpected broker virtual host");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Broker->UseTLS, 0, "Unexpected broker TLS usage flag");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(Broker->AdvicedHostname, "", "Unexpected broker adviced host name");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(Broker->AdvicedPort, 0, "Unexpected broker adviced port");

    if (TestAMQP1_0)
    {
        printf("AMQP 1.0 format enabled\r\n%s\r\n", ROW_SEPARATOR);
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
    TEST_ASSERT_MESSAGE(Publisher != 0, "Cannot create publisher");
    printf("Publisher created\r\n");

    Socket = rmqsSocketCreate();

    if (! rmqsSocketConnect(Broker->Hostname, Broker->Port, Socket, 500))
    {
        sprintf(OutputString, "Cannot connect to %s", Broker->Hostname);
        TEST_FAIL_MESSAGE(OutputString);
    }

    TxRxBufferSize = NoOfIterations * MessageCount * strlen(SendLongMessages ? TEST_MESSAGE_DATA_LONG : TEST_MESSAGE_DATA);
    TxRxBufferSize = (size_t)((double)TxRxBufferSize * 1.5);

    if (! rmqsSetSocketTxRxBufferSize(Socket, TxRxBufferSize, TxRxBufferSize))
    {
        TEST_FAIL_MESSAGE("Error setting the socket tx and rx buffer size");
    }
    
    printf("Connected to server %s\r\n", Broker->Hostname);

    if (! rmqsClientLogin(Publisher->Client, 1, Socket, Properties, 6, &ResponseCode))
    {
        sprintf(OutputString, "Cannot login to server %s", Broker->Hostname);
        TEST_FAIL_MESSAGE(OutputString);
    }

    printf("Logged in to server %s\r\n", Broker->Hostname);

    if (rmqsDelete(Publisher->Client, Socket, Stream, &ResponseCode))
    {
        printf("Deleted stream %s\r\n", Stream);
    }
    else
    {
        sprintf(OutputString, "Cannot delete stream %s", Stream);
        TEST_FAIL_MESSAGE(OutputString);
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
        sprintf(OutputString, "Cannot create stream %s", Stream);
        TEST_FAIL_MESSAGE(OutputString);
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
            TEST_FAIL_MESSAGE("Cannot declare the publisher");
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
        if (! SendLongMessages)
        {
            MessageBatch[i].Data = MessageBody;
            MessageBatch[i].Size = MessageBodySize;
        }
        else
        {
            MessageBatch[i].Data = MessageBodyLong;
            MessageBatch[i].Size = MessageBodySizeLong;
        }

        MessageBatch[i].DeleteData = false;
        MessageBatch[i].Encoding = TestAMQP1_0 ? rmqsmeAMQP1_0 : rmqsmeNone;
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

        if (! rmqsPublish(Publisher, Socket, PublisherId, MessageBatch, MessageCount))
        {
            TEST_FAIL_MESSAGE("Error while publishing messages");
        }
    }

    TimerResult = rmqsTimerGetTime(PublishTimer);
    printf("%d Messages - (CNT: %u - IT: %u) - Elapsed time: %ums\r\n", (int)(MessageCount * NoOfIterations), (uint32_t)MessageCount, (uint32_t)NoOfIterations, TimerResult);

    TEST_ASSERT_LESS_THAN_UINT32_MESSAGE(SendLongMessages ? 15000 : 2000, TimerResult, "Unexpected publishing time, too long");

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

    if (! rmqsQueryPublisherSequence(Publisher, Socket, Stream, &Sequence, &ResponseCode))
    {
        TEST_FAIL_MESSAGE("Error retrieving the publisher sequence");
    }

    TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)(MessageCount * NoOfIterations), Sequence, "Unexpected publish sequence number");

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
        TEST_FAIL_MESSAGE("Cannot delete the publisher");
    }

    rmqsHeartbeat(Publisher->Client, Socket);

    if (rmqsClientLogout(Publisher->Client, Socket, 0, "Regular shutdown", &ResponseCode))
    {
        printf("Logged out\r\n");
    }
    else
    {
        TEST_FAIL_MESSAGE("Cannot logout");
    }

    printf("Messages confirmed: %u/%u\r\n", (uint32_t)MessagesConfirmed, (uint32_t)(MessageCount * NoOfIterations));
    printf("Messages not confirmed: %u/%u\r\n", (uint32_t)MessagesNotConfirmed, (uint32_t)(MessageCount * NoOfIterations));

    TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)(MessageCount * NoOfIterations), MessagesConfirmed, "Not all messages have been confirmed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, MessagesNotConfirmed, "Some messages weren't confirmed");

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
        sprintf(OutputString, "Cannot connect to %s", Broker->Hostname);
        TEST_FAIL_MESSAGE(OutputString);
    }

    printf("Connected to server %s\r\n", Broker->Hostname);

    if (! rmqsClientLogin(Consumer->Client, 1, Socket, Properties, 6, &ResponseCode))
    {
        sprintf(OutputString, "Cannot login to server %s", Broker->Hostname);
        TEST_FAIL_MESSAGE(OutputString);
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
        sprintf(OutputString, "Cannot retrieve the metadata for stream %s", Stream);
        TEST_FAIL_MESSAGE(OutputString);
    }

    if (rmqsSubscribe(Consumer, Socket, SubscriptionId, Stream, rmqsotOffset, 0, Consumer->DefaultCredit, 0, 0, &ResponseCode))
    {
        printf("Subscribed to stream %s\r\n", Stream);
    }
    else
    {
        sprintf(OutputString, "Cannot subscribe to stream %s", Stream);
        TEST_FAIL_MESSAGE(OutputString);
    }

    printf("Consumer - Timer begin\r\n");

    rmqsTimerStart(ConsumeTimer);

    if (! rmqsConsumerPoll(Consumer, Socket, ConsumeWaitingTime, &ConnectionError))
    {
        TEST_FAIL_MESSAGE("Error while consuming messages");
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

        //
        // Offset must be at least equal t
        //
        TEST_ASSERT_GREATER_OR_EQUAL_INT32_MESSAGE((uint32_t)((MessageCount * NoOfIterations) - 1), Offset, "Unexpected offset");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(true, ValidOffset, "Offset is not valid");
    }
    else
    {
        TEST_FAIL_MESSAGE("QueryOffset error");
    }

    if (rmqsUnsubscribe(Consumer, Socket, SubscriptionId, &ResponseCode))
    {
        printf("Unsubscribed from stream %s\r\n", Stream);
    }
    else
    {
        sprintf(OutputString, "Cannot unsubcribe from stream %s", Stream);
        TEST_FAIL_MESSAGE(OutputString);
    }

    rmqsHeartbeat(Publisher->Client, Socket);

    if (rmqsClientLogout(Consumer->Client, Socket, 0, "Regular shutdown", &ResponseCode))
    {
        printf("Logged out\r\n");
    }
    else
    {
        TEST_FAIL_MESSAGE("Cannot logout");
    }

    rmqsSocketDestroy(&Socket);
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------

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

    sprintf(OutputString, "Unfreed memory: %u bytes\r\n", (uint32_t)UsedMemory);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, (uint32_t)UsedMemory, OutputString);

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
void DeliverResultCallback(void *Consumer, uint8_t SubscriptionId, byte_t *Data, size_t DataSize, rmqsDeliverInfo_t *DeliverInfo, uint64_t MessageOffset, bool_t *StoreOffset)
{
    rmqsConsumer_t *ConsumerObj = (rmqsConsumer_t *)Consumer;

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

        TEST_ASSERT_LESS_THAN_UINT32_MESSAGE(SendLongMessages ? 5000 : 500, TimerResult, "Unexpected deliver time, too long");

        if (rmqsCheckAQMP1_0Message(ConsumerObj->Client, Data, DataSize, &Data, &DataSize))
        {
            //
            // Do any specific action for long messages here
            // The rmqsCheckAQMP1_0Message moves the pointer to the effective message and retrieves the effective size
            // whenever the message is packed with the AMQP 1.0 format
            //
        }

        printf("Last message: %.*s\r\n", (int)DataSize, (char *)Data);

        TEST_ASSERT_EQUAL_STRING_MESSAGE((char *)Data, SendLongMessages ? TEST_MESSAGE_DATA_LONG : TEST_MESSAGE_DATA, "Unexpected message received");
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
