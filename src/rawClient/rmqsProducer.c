//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsProducer.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(rmqsClientConfiguration_t *ClientConfiguration, const char_t *PublisherReference, PublishResultCallback_t PublishResultCallback)
{
    rmqsProducer_t *Producer = (rmqsProducer_t *)rmqsAllocateMemory(sizeof(rmqsProducer_t));

    memset(Producer, 0, sizeof(rmqsProducer_t));

    Producer->Client = rmqsClientCreate(ClientConfiguration, rmqsctProducer, Producer);
    strncpy(Producer->PublisherReference, PublisherReference, RMQS_MAX_PUBLISHER_REFERENCE_LENGTH);
    Producer->PollTimer = rmqsTimerCreate();
    Producer->PublishResultCallback = PublishResultCallback;

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsProducerDestroy(rmqsProducer_t *Producer)
{
    rmqsTimerDestroy(Producer->PollTimer);
    rmqsClientDestroy(Producer->Client);

    rmqsFreeMemory((void *)Producer);
}
//---------------------------------------------------------------------------
void rmqsProducerPoll(rmqsProducer_t *Producer, const rmqsSocket Socket, uint32_t Timeout)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    rmqsTimerStart(Producer->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Producer->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Producer->Client, Socket, WaitMessageTimeout);

        Time = rmqsTimerGetTime(Producer->Client->ClientConfiguration->WaitReplyTimer);

        WaitMessageTimeout = Timeout - Time;
    }
}
//---------------------------------------------------------------------------
rmqsResponseCode_t rmqsDeclarePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId, const char_t *StreamName)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscDeclarePublisher;
    uint16_t Version = 1;

    rmqsMemBufferClear(Client->TxMemBuffer, false);

    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToMemBuffer(Client->TxMemBuffer, PublisherId);
    rmqsAddStringToMemBuffer(Client->TxMemBuffer, Producer->PublisherReference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToMemBuffer(Client->TxMemBuffer, StreamName, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxMemBuffer, false);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, Client->TxMemBuffer->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxMemBuffer->Data, Client->TxMemBuffer->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscDeclarePublisher)
        {
            return rmqsrWrongReply;
        }

        return Client->Response.ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
rmqsResponseCode_t rmqsDeletePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscDeletePublisher;
    uint16_t Version = 1;

    rmqsMemBufferClear(Client->TxMemBuffer, false);

    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToMemBuffer(Client->TxMemBuffer, PublisherId);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxMemBuffer, false);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, Client->TxMemBuffer->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxMemBuffer->Data, Client->TxMemBuffer->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscDeletePublisher)
        {
            return rmqsrWrongReply;
        }

        return Client->Response.ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
void rmqsPublish(rmqsProducer_t *Producer, const rmqsSocket Socket, const uint8_t PublisherId, rmqsMessage_t *Messages, const size_t MessageCount)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscPublish;
    uint16_t Version = 1;
    size_t i;

    rmqsMemBufferClear(Client->TxMemBuffer, false);

    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToMemBuffer(Client->TxMemBuffer, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToMemBuffer(Client->TxMemBuffer, PublisherId);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, MessageCount, ClientConfiguration->IsLittleEndianMachine); // Message count

    for (i = 0; i < MessageCount; i++)
    {
        rmqsAddUInt64ToMemBuffer(Client->TxMemBuffer, Messages[i].PublishingId, ClientConfiguration->IsLittleEndianMachine);
        rmqsAddBytesToMemBuffer(Client->TxMemBuffer, Messages[i].Data, Messages[i].Size, ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxMemBuffer, false);
    rmqsAddUInt32ToMemBuffer(Client->TxMemBuffer, Client->TxMemBuffer->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxMemBuffer->Data, Client->TxMemBuffer->Size);
}
//---------------------------------------------------------------------------
void rmqsHandlePublishResult(uint16_t Key, rmqsProducer_t *Producer, rmqsMemBuffer_t *MemBuffer)
{
    char_t *MessagePayload = (char_t *)MemBuffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t *PublisherId;
    uint32_t *NoOfRecords;
    uint64_t *PublishingId;
    uint32_t i;

    PublisherId = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);
    NoOfRecords = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint64_t);

    if (Producer->Client->ClientConfiguration->IsLittleEndianMachine)
    {
        *NoOfRecords = SwapUInt32(*NoOfRecords);
    }

    if (Key == rmqscPublishConfirm)
    {
        for (i = 0; i < *NoOfRecords; i++)
        {
            PublishingId = (uint64_t *)MessagePayload;

            if (Producer->Client->ClientConfiguration->IsLittleEndianMachine)
            {
                *PublishingId = SwapUInt64(*PublishingId);
            }

            Producer->PublishResultCallback(*PublisherId, *PublishingId, true, 0);

            MessagePayload += sizeof(uint64_t);
        }
    }
    else if (Key == rmqscPublishError)
    {
        Key = Key;
    }
}
//---------------------------------------------------------------------------
