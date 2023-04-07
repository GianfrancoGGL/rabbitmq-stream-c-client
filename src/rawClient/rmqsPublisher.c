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
#include "rmqsPublisher.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsPublisher_t * rmqsPublisherCreate(rmqsClientConfiguration_t *ClientConfiguration, char_t *PublisherReference, uint32_t Heartbeat, PublishResultCallback_t PublishResultCallback)
{
    rmqsPublisher_t *Publisher = (rmqsPublisher_t *)rmqsAllocateMemory(sizeof(rmqsPublisher_t));

    memset(Publisher, 0, sizeof(rmqsPublisher_t));

    Publisher->Client = rmqsClientCreate(ClientConfiguration, rmqsctPublisher, Publisher);
    strncpy(Publisher->PublisherReference, PublisherReference, RMQS_MAX_PUBLISHER_REFERENCE_LENGTH);
    Publisher->Heartbeat = Heartbeat;
    Publisher->PublishResultCallback = PublishResultCallback;

    return Publisher;
}
//---------------------------------------------------------------------------
void rmqsPublisherDestroy(rmqsPublisher_t *Publisher)
{
    rmqsClientDestroy(Publisher->Client);

    rmqsFreeMemory((void *)Publisher);
}
//---------------------------------------------------------------------------
void rmqsPublisherPoll(rmqsPublisher_t *Publisher, rmqsSocket Socket, uint32_t Timeout, bool_t *ConnectionLost)
{
    uint32_t WaitMessageTimeout = Timeout;
    uint32_t Time;

    *ConnectionLost = false;

    rmqsTimerStart(Publisher->Client->ClientConfiguration->WaitReplyTimer);

    while (rmqsTimerGetTime(Publisher->Client->ClientConfiguration->WaitReplyTimer) < Timeout)
    {
        rmqsWaitMessage(Publisher->Client, Socket, WaitMessageTimeout, ConnectionLost);

        if (*ConnectionLost)
        {
            return;
        }

        Time = rmqsTimerGetTime(Publisher->Client->ClientConfiguration->WaitReplyTimer);

        WaitMessageTimeout = Timeout - Time;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsDeclarePublisher(rmqsPublisher_t *Publisher, rmqsSocket Socket, uint8_t PublisherId, char_t *Stream)
{
    rmqsClient_t *Client = Publisher->Client;
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
    rmqsAddStringToBuffer(Client->TxQueue, Publisher->PublisherReference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Stream, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscDeclarePublisher)
        {
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsQueryPublisherSequence(rmqsPublisher_t *Publisher, rmqsSocket Socket, char_t *Stream, uint64_t *Sequence)
{
    rmqsClient_t *Client = Publisher->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscQueryPublisherSequence;
    uint16_t Version = 1;
    rmqsQueryPublisherResponse_t *QueryPublisherResponse;
    bool_t ConnectionLost;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Publisher->PublisherReference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, Stream, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscQueryPublisherSequence)
        {
            return false;
        }

        QueryPublisherResponse = (rmqsQueryPublisherResponse_t *)Client->RxQueue->Data;

        *Sequence = QueryPublisherResponse->Sequence;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            *Sequence = SwapUInt64(*Sequence);
        }

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsDeletePublisher(rmqsPublisher_t *Publisher, rmqsSocket Socket, uint8_t PublisherId)
{
    rmqsClient_t *Client = Publisher->Client;
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

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscDeletePublisher)
        {
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
void rmqsPublish(rmqsPublisher_t *Publisher, rmqsSocket Socket, uint8_t PublisherId, rmqsMessage_t *Messages, size_t MessageCount)
{
    rmqsClient_t *Client = Publisher->Client;
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

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);
}
//---------------------------------------------------------------------------
void rmqsHandlePublishResult(uint16_t Key, rmqsPublisher_t *Publisher, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint8_t *PublisherId;
    uint32_t *NoOfRecords;
    uint64_t *PublishingId;
    uint16_t *Code;
    size_t PublishIdCount = 0;
    uint32_t i;

    PublisherId = (uint8_t *)MessagePayload;
    MessagePayload += sizeof(uint8_t);
 
    NoOfRecords = (uint32_t *)MessagePayload;
    MessagePayload += sizeof(uint32_t);

    if (Publisher->Client->ClientConfiguration->IsLittleEndianMachine)
    {
        *NoOfRecords = SwapUInt32(*NoOfRecords);
    }

    for (i = 0; i < *NoOfRecords; i++)
    {
        PublishingId = (uint64_t *)MessagePayload;

        if (Publisher->Client->ClientConfiguration->IsLittleEndianMachine)
        {
            *PublishingId = SwapUInt64(*PublishingId);
        }

        Publisher->PublishResultArray[PublishIdCount].PublishingId = *PublishingId;
        MessagePayload += sizeof(uint64_t);

        if (Key == rmqscPublishError)
        {
            Code = (uint16_t *)MessagePayload;
            MessagePayload += sizeof(uint16_t);

            if (Publisher->Client->ClientConfiguration->IsLittleEndianMachine)
            {
                *Code = SwapUInt16(*Code);
            }

            Publisher->PublishResultArray[PublishIdCount].Code = *Code;
        }
        else
        {
            Publisher->PublishResultArray[PublishIdCount].Code = 0;
        }

        if (++PublishIdCount == RMQS_PUBLISH_RESULT_ARRAY_SIZE || i == *NoOfRecords - 1)
        {
            Publisher->PublishResultCallback(*PublisherId, Publisher->PublishResultArray, PublishIdCount, Key == rmqscPublishConfirm);
            PublishIdCount = 0; // Reset the index of the array containing the list of the publish result
        }
    }
}
//---------------------------------------------------------------------------
