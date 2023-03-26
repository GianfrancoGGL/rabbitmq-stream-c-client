/****************************************************************************
MIT License

Copyright (c) 2023 Gianfranco Giugliano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
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
void rmqsProducerPoll(rmqsProducer_t *Producer, const rmqsSocket Socket, uint32_t Timeout, bool_t *ConnectionLost)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    *ConnectionLost = false;

    rmqsTimerStart(Producer->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Producer->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Producer->Client, Socket, WaitMessageTimeout, ConnectionLost);

        if (*ConnectionLost)
        {
            return;
        }

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
    bool_t ConnectionLost;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, PublisherId);
    rmqsAddStringToBuffer(Client->TxQueue, Producer->PublisherReference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, StreamName, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
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
    bool_t ConnectionLost;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, PublisherId);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
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

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToBuffer(Client->TxQueue, PublisherId);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)MessageCount, ClientConfiguration->IsLittleEndianMachine); // Message count

    for (i = 0; i < MessageCount; i++)
    {
        rmqsAddUInt64ToBuffer(Client->TxQueue, Messages[i].PublishingId, ClientConfiguration->IsLittleEndianMachine);
        rmqsAddBytesToBuffer(Client->TxQueue, Messages[i].Data, Messages[i].Size, true, ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);
}
//---------------------------------------------------------------------------
void rmqsHandlePublishResult(uint16_t Key, rmqsProducer_t *Producer, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t *PublisherId;
    uint32_t *NoOfRecords;
    uint64_t *PublishingId;
    uint32_t i;
    size_t PublishIdCount = 0;

    PublisherId = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);
    NoOfRecords = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

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

            Producer->PublishingIdResultArray[PublishIdCount++] = *PublishingId;

            if (PublishIdCount == RMQS_PUBLISHING_ID_RESULT_ARRAY_SIZE || i == *NoOfRecords - 1)
            {
                Producer->PublishResultCallback(*PublisherId, Producer->PublishingIdResultArray, PublishIdCount, true, 0);
                PublishIdCount = 0; // Reset the index of the array containing the list of the publish result
            }

            MessagePayload += sizeof(uint64_t);
        }
    }
    else if (Key == rmqscPublishError)
    {
        Key = Key;
    }
}
//---------------------------------------------------------------------------
