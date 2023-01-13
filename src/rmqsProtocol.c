//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsEnvironment.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
#define SwapUInt16(x) ((uint16_t)(x >> 8) & (uint16_t)0x00FF) | ((uint16_t)(x << 8) & (uint16_t)0xFF00)
#define SwapUInt32(x) ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | ((x << 24) & 0xFF000000)
//---------------------------------------------------------------------------
#define RMQS_NULL_STRING_LENGTH    -1
//---------------------------------------------------------------------------
uint8_t rmqsIsLittleEndianMachine(void)
{
    union
    {
        uint32_t i;
        char c[4];
    }
    bint = {0x01020304};

    if (bint.c[0] == 1)
    {
       return 0;
    }
    else
    {
       return 1;
    }
}
//---------------------------------------------------------------------------
void rmqsSendMessage(const void *Environment, const rmqsSocket Socket, const char *Data, size_t DataSize)
{
    rmqsEnvironment_t *EnvironmentObj = (rmqsEnvironment_t *)Environment;

    if (EnvironmentObj->Logger)
    {
        rmqsLoggerRegisterDump(EnvironmentObj->Logger, (void *)Data, DataSize, "TX", 0);
    }

    send(Socket, (const char *)Data, DataSize, 0);
}
//---------------------------------------------------------------------------
uint8_t rmqsWaitMessage(const void *Environment, const rmqsSocket Socket, char *RxBuffer, size_t RxBufferSize, rmqsStream_t *RxStream, rmqsStream_t *RxStreamTempBuffer, const uint32_t RxTimeout)
{
    rmqsEnvironment_t *EnvironmentObj = (rmqsEnvironment_t *)Environment;
    uint8_t MessageReceived = 0;
    int32_t RxBytes;
    uint32_t MessageSize;

    //
    // Tries to extract the message from the already received bytes, if not enough, read
    // again from the socket
    //
    while (1)
    {
        //
        // Unprocessed bytes saved in the temp stream are processed now
        //
        if (RxStreamTempBuffer->Size > 0)
        {
            rmqsStreamWrite(RxStream, RxStreamTempBuffer->Data, RxStreamTempBuffer->Size);
            rmqsStreamClear(RxStreamTempBuffer, 0);
        }

        //
        // Is the message length (4 bytes) arrived?
        //
        if (RxStream->Size >= sizeof(uint32_t))
        {
            //
            // Message length is once stored in the Tag1 field and eventually with the correct endianness
            //
            MessageSize = *(uint32_t *)RxStream->Data;

            if (EnvironmentObj->IsLittleEndianMachine)
            {
                MessageSize = SwapUInt32(MessageSize);
            }

            //
            // The number of bytes to wait is the one stored in the 4 bytes of the length + the length itself
            //
            MessageSize += sizeof(uint32_t);

            if (RxStream->Size >= MessageSize)
            {
                //
                // Message completed!
                //
                MessageReceived = 1;

                if (EnvironmentObj->Logger)
                {
                    rmqsLoggerRegisterDump(EnvironmentObj->Logger, (void *)RxStream->Data, RxStream->Size, "RX", 0);
                }

                //
                // Store the extra bytes in the rx buffer stream
                //
                rmqsStreamWrite(RxStreamTempBuffer, (void *)((char *)RxStream->Data + MessageSize), RxStream->Size - MessageSize);
            }
        }

        if (MessageReceived)
        {
            break;
        }

        rmqsSetSocketReadTimeouts(Socket, RxTimeout);

        RxBytes = recv(Socket, RxBuffer, RxBufferSize, 0);

        if (RxBytes <= 0)
        {
            break;
        }

        rmqsStreamWrite(RxStream, RxBuffer, RxBytes);

        //
        // New loop to extract the message from the receive stream
        //
    }

    return MessageReceived;
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerPropertiesRequest(const void *Producer, rmqsCorrelationId CorrelationId, uint32_t PropertiesCount, rmqsProperty_t *Properties)
{
    rmqsProducer_t *ProducerObj = (rmqsProducer_t *)Producer;
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)ProducerObj->Environment;
    rmqsKey Key = rmqscPeerProperties;
    rmqsVersion Version = 1;
    uint32_t i, MapSize;
    rmqsProperty_t *Property;
    rmqsResponse_t *Response;

    //
    // Calculate the map size
    //
    MapSize = sizeof(MapSize); // The map size field

    for (i = 0; i < PropertiesCount; i++)
    {
        //
        // For every key/value
        // 2 bytes for the key length field
        // the length of the key
        // 2 bytes for the value length field
        // the length of the value
        //
        Property = &Properties[i];
        MapSize += sizeof(uint16_t); // Key length field
        MapSize += strlen(Property->Key);
        MapSize += sizeof(uint16_t); // Value length field
        MapSize += strlen(Property->Value);
    }

    rmqsStreamClear(ProducerObj->TxStream, 0);

    rmqsAddUInt32ToStream(ProducerObj->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, CorrelationId, Environment->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToStream(ProducerObj->TxStream, MapSize, Environment->IsLittleEndianMachine);

    for (i = 0; i < PropertiesCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToStream(ProducerObj->TxStream, Property->Key, Environment->IsLittleEndianMachine);
        rmqsAddStringToStream(ProducerObj->TxStream, Property->Value, Environment->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsStreamMoveTo(ProducerObj->TxStream, 0);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, ProducerObj->TxStream->Size - sizeof(rmqsSize), Environment->IsLittleEndianMachine);

    rmqsSendMessage(ProducerObj->Environment, ProducerObj->Socket, (const char *)ProducerObj->TxStream->Data, ProducerObj->TxStream->Size);

    if (rmqsWaitMessage(ProducerObj->Environment, ProducerObj->Socket, ProducerObj->RxSocketBuffer, sizeof(ProducerObj->RxSocketBuffer), ProducerObj->RxStream, ProducerObj->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)ProducerObj->RxStream->Data;

        if (Environment->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16((Response->Key & 0x7FFF));
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
        }
        
        return Response->ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
size_t rmqsAddInt8ToStream(rmqsStream_t *Stream, int8_t Value)
{
    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt8ToStream(rmqsStream_t *Stream, uint8_t Value)
{
    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt16ToStream(rmqsStream_t *Stream, int16_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt16ToStream(rmqsStream_t *Stream, uint16_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt32ToStream(rmqsStream_t *Stream, int32_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt32ToStream(rmqsStream_t *Stream, uint32_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsStreamWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddStringToStream(rmqsStream_t *Stream, char *Value, uint8_t IsLittleEndianMachine)
{
    rmqsStringLen StringLen;
    size_t BytesAdded;

    if (*Value == 0)
    {
        StringLen = RMQS_NULL_STRING_LENGTH;
    }
    else
    {
        StringLen = (int16_t)strlen(Value);
    }

    rmqsAddInt16ToStream(Stream, StringLen, IsLittleEndianMachine);
    BytesAdded = sizeof(StringLen);

    if (StringLen != RMQS_NULL_STRING_LENGTH)
    {
        rmqsStreamWrite(Stream, (void *)Value, StringLen);
        BytesAdded += StringLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------

