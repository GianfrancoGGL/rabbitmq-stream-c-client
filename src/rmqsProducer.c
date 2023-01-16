//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsProducer.h"
#include "rmqsEnvironment.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsProducer_t * rmqsProducerCreate(void *Environment, const char_t *HostName, uint8_t PublisherId, const char_t *PublisherReference, void (*EventsCB)(rqmsClientEvent, void *Producer))
{
    rmqsProducer_t *Producer = (rmqsProducer_t *)rmqsAllocateMemory(sizeof(rmqsProducer_t));

    memset(Producer, 0, sizeof(rmqsProducer_t));

    Producer->Client = rmqsClientCreate(Environment, HostName, EventsCB, Producer, rmqsProducerHandlerCB);
    Producer->PublisherId = PublisherId;
    strncpy(Producer->PublisherReference, PublisherReference, RMQS_MAX_PUBLISHER_REFERENCE_LENGTH);

    return Producer;
}
//---------------------------------------------------------------------------
void rmqsProducerDestroy(rmqsProducer_t *Producer)
{
    rmqsClientDestroy(Producer->Client);

    rmqsFreeMemory((void *)Producer);
}
//---------------------------------------------------------------------------
void rmqsProducerHandlerCB(void *Client)
{
    (void)Client;
    
    /*
    rmqsProducer_t *ProducerObj = (rmqsProducer_t *)Client;
    rmqsResponseCode Response;

    Response = rmqsDeclarePublisher(ProducerObj, "SYNERP_RESULTS");
    Response = Response;
    */

    rmqsThreadSleep(10);
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsDeclarePublisher(rmqsProducer_t *Producer, const char_t *Stream)
{
    rmqsClient_t *Client = Producer->Client;
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)Client->Environment;
    rmqsKey_t Key = rmqscDeclarePublisher;
    rmqsVersion_t Version = 1;
    rmqsResponse_t *Response;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Environment->IsLittleEndianMachine);
    rmqsAddUInt8ToStream(Client->TxStream, Producer->PublisherId);
    rmqsAddStringToStream(Client->TxStream, Producer->PublisherReference, Environment->IsLittleEndianMachine);
    rmqsAddStringToStream(Client->TxStream, Stream, Environment->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(Client->Environment, Client->Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)Client->RxStream->Data;

        if (Environment->IsLittleEndianMachine)
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

