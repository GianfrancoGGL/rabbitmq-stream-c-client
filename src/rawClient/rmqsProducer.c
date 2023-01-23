//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsProducer.h"
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(rmqsClientConfiguration_t *ClientConfiguration, const uint8_t PublisherId, const char_t *PublisherReference)
{
    rmqsProducer_t *Producer = (rmqsProducer_t *)rmqsAllocateMemory(sizeof(rmqsProducer_t));

    memset(Producer, 0, sizeof(rmqsProducer_t));

    Producer->PublisherId = PublisherId;
    strncpy(Producer->PublisherReference, PublisherReference, RMQS_MAX_PUBLISHER_REFERENCE_LENGTH);
    Producer->Client = rmqsClientCreate(ClientConfiguration, Producer);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsProducerDestroy(rmqsProducer_t *Producer)
{
    rmqsClientDestroy(Producer->Client);

    rmqsFreeMemory((void *)Producer);
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsDeclarePublisher(rmqsProducer_t *Producer, const rmqsSocket Socket, const char_t *StreamName)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscDeclarePublisher;
    uint16_t Version = 1;
    rmqsResponse_t *Response;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToStream(Client->TxStream, Producer->PublisherId);
    rmqsAddStringToStream(Client->TxStream, Producer->PublisherReference, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToStream(Client->TxStream, StreamName, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)Client->RxStream->Data;

        if (ClientConfiguration->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16(Response->Key);
            Response->Key &= 0x7FFF;
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
        }

        if (Response->Key != rmqscDeclarePublisher)
        {
            return rmqsrWrongReply;
        }

        return Response->ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
void rmqsPublish(rmqsProducer_t *Producer, const rmqsSocket Socket, rmqsMessage_t *Message)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscPublish;
    uint16_t Version = 1;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToStream(Client->TxStream, Producer->PublisherId);
    rmqsAddUInt32ToStream(Client->TxStream, 1, ClientConfiguration->IsLittleEndianMachine); // Message count
    rmqsAddUInt64ToStream(Client->TxStream, Message->PublishingId, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddBytesToStream(Client->TxStream, Message->Data, Message->Size, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);
}
//---------------------------------------------------------------------------
void rmqsPublishBatch(rmqsProducer_t *Producer, const rmqsSocket Socket, rmqsMessage_t *Messages[], const size_t MessageCount)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscPublish;
    uint16_t Version = 1;
    rmqsMessage_t *Message;
    size_t i;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt8ToStream(Client->TxStream, Producer->PublisherId);
    rmqsAddUInt32ToStream(Client->TxStream, MessageCount, ClientConfiguration->IsLittleEndianMachine); // Message count

    for (i = 0; i < MessageCount; i++)
    {
        Message = Messages[i];

        rmqsAddUInt64ToStream(Client->TxStream, Message->PublishingId, ClientConfiguration->IsLittleEndianMachine);
        rmqsAddBytesToStream(Client->TxStream, Message->Data, Message->Size, ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);
}
//---------------------------------------------------------------------------

