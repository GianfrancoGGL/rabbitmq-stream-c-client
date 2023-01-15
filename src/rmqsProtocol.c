//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsEnvironment.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
#define RMQS_NULL_STRING_LENGTH    -1
#define RMQS_EMPTY_DATA_LENGTH     -1
//---------------------------------------------------------------------------
uint8_t rmqsIsLittleEndianMachine(void)
{
    union
    {
        uint32_t i;
        char_t c[4];
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
void rmqsSendMessage(const void *Environment, const rmqsSocket Socket, const char_t *Data, size_t DataSize)
{
    rmqsEnvironment_t *EnvironmentObj = (rmqsEnvironment_t *)Environment;

    if (EnvironmentObj->Logger)
    {
        rmqsLoggerRegisterDump(EnvironmentObj->Logger, (void *)Data, DataSize, "TX", 0);
    }

    send(Socket, (const char_t *)Data, DataSize, 0);
}
//---------------------------------------------------------------------------
uint8_t rmqsWaitMessage(const void *Environment, const rmqsSocket Socket, char_t *RxBuffer, size_t RxBufferSize, rmqsMemBuffer_t *RxStream, rmqsMemBuffer_t *RxStreamTempBuffer, const uint32_t RxTimeout)
{
    rmqsEnvironment_t *EnvironmentObj = (rmqsEnvironment_t *)Environment;
    uint8_t MessageReceived = 0;
    int32_t RxBytes;
    uint32_t MessageSize;

    rmqsMemBufferClear(RxStream, 0);

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
            rmqsMemBufferWrite(RxStream, RxStreamTempBuffer->Data, RxStreamTempBuffer->Size);
            rmqsMemBufferClear(RxStreamTempBuffer, 0);
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
                rmqsMemBufferWrite(RxStreamTempBuffer, (void *)((char_t *)RxStream->Data + MessageSize), RxStream->Size - MessageSize);
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

        rmqsMemBufferWrite(RxStream, RxBuffer, RxBytes);

        //
        // New loop to extract the message from the receive stream
        //
    }

    return MessageReceived;
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerPropertiesRequest(const void *Producer, rmqsCorrelationId_t CorrelationId, uint32_t PropertiesCount, rmqsProperty_t *Properties)
{
    rmqsProducer_t *ProducerObj = (rmqsProducer_t *)Producer;
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)ProducerObj->Environment;
    rmqsKey_t Key = rmqscPeerProperties;
    rmqsVersion_t Version = 1;
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

    rmqsMemBufferClear(ProducerObj->TxStream, 0);

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
    rmqsMemBufferMoveTo(ProducerObj->TxStream, 0);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, ProducerObj->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(ProducerObj->Environment, ProducerObj->Socket, (const char_t *)ProducerObj->TxStream->Data, ProducerObj->TxStream->Size);

    if (rmqsWaitMessage(ProducerObj->Environment, ProducerObj->Socket, ProducerObj->RxSocketBuffer, sizeof(ProducerObj->RxSocketBuffer), ProducerObj->RxStream, ProducerObj->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)ProducerObj->RxStream->Data;

        if (Environment->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16(Response->Key);
            Response->Key &= 0x7FFF;
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
        }

        if (Response->Key != rmqscPeerProperties)
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
rmqsResponseCode rmqsSaslHandshakeRequest(const void *Producer, rmqsCorrelationId_t CorrelationId, uint8_t *PlainAuthSupported)
{
    rmqsProducer_t *ProducerObj = (rmqsProducer_t *)Producer;
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)ProducerObj->Environment;
    rmqsKey_t Key = rmqscSaslHandshake;
    rmqsVersion_t Version = 1;
    rmqsResponseHandshakeRequest_t *Response;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t *StringLen;
    char_t *String;

    *PlainAuthSupported = 0; // By default assume that the PLAIN auth is not supported

    rmqsMemBufferClear(ProducerObj->TxStream, 0);

    rmqsAddUInt32ToStream(ProducerObj->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, CorrelationId, Environment->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(ProducerObj->TxStream, 0);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, ProducerObj->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(ProducerObj->Environment, ProducerObj->Socket, (const char_t *)ProducerObj->TxStream->Data, ProducerObj->TxStream->Size);

    if (rmqsWaitMessage(ProducerObj->Environment, ProducerObj->Socket, ProducerObj->RxSocketBuffer, sizeof(ProducerObj->RxSocketBuffer), ProducerObj->RxStream, ProducerObj->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponseHandshakeRequest_t *)ProducerObj->RxStream->Data;

        if (Environment->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16(Response->Key);
            Response->Key &= 0x7FFF;
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
            Response->NoOfMechanisms = SwapUInt16(Response->NoOfMechanisms);
        }

        if (Response->Key != rmqscSaslHandshake)
        {
            return rmqsrWrongReply;
        }

        if (Response->NoOfMechanisms > 0)
        {
            Data = (char_t *)ProducerObj->RxStream->Data + sizeof(rmqsResponseHandshakeRequest_t);

            for (MechanismNo = 1; MechanismNo <= Response->NoOfMechanisms; MechanismNo++)
            {
                StringLen = (uint16_t *)Data;

                if (Environment->IsLittleEndianMachine)
                {
                    *StringLen = SwapUInt16(*StringLen);
                }

                Data += sizeof(uint16_t);

                if (*StringLen > 0)
                {
                    String = rmqsAllocateMemory(*StringLen + 1);
                    memset(String, 0, *StringLen + 1);
                    strncpy(String, (char_t *)Data, *StringLen);

                    if (! strcmp(String, RMQS_PLAIN_PROTOCOL))
                    {
                        *PlainAuthSupported = 1;
                    }

                    rmqsFreeMemory(String);

                    Data += *StringLen;
                }
            }
        }

        return Response->ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsSaslAuthenticateRequest(const void *Producer, rmqsCorrelationId_t CorrelationId, const char_t *Mechanism, const char_t *Username, const char_t *Password)
{
    rmqsProducer_t *ProducerObj = (rmqsProducer_t *)Producer;
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)ProducerObj->Environment;
    rmqsKey_t Key = rmqscSaslAuthenticate;
    rmqsVersion_t Version = 1;
    rmqsResponse_t *Response;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;

    rmqsMemBufferClear(ProducerObj->TxStream, 0);

    rmqsAddUInt32ToStream(ProducerObj->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(ProducerObj->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, CorrelationId, Environment->IsLittleEndianMachine);

    rmqsAddStringToStream(ProducerObj->TxStream, (char_t *)Mechanism, Environment->IsLittleEndianMachine);

    UsernameLen = strlen(Username);
    PasswordLen = strlen(Password);
    OpaqueDataLen = UsernameLen + PasswordLen + 2; // +1 because there must be NULL before username and password
    OpaqueData = rmqsAllocateMemory(OpaqueDataLen);
    memset(OpaqueData, 0, OpaqueDataLen);
    memcpy(OpaqueData + 1, Username, UsernameLen);
    memcpy(OpaqueData + UsernameLen + 2, Password, PasswordLen);

    rmqsAddBytesToStream(ProducerObj->TxStream, OpaqueData, OpaqueDataLen, Environment->IsLittleEndianMachine);

    rmqsFreeMemory(OpaqueData);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(ProducerObj->TxStream, 0);
    rmqsAddUInt32ToStream(ProducerObj->TxStream, ProducerObj->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(ProducerObj->Environment, ProducerObj->Socket, (const char_t *)ProducerObj->TxStream->Data, ProducerObj->TxStream->Size);

    if (rmqsWaitMessage(ProducerObj->Environment, ProducerObj->Socket, ProducerObj->RxSocketBuffer, sizeof(ProducerObj->RxSocketBuffer), ProducerObj->RxStream, ProducerObj->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)ProducerObj->RxStream->Data;

        if (Environment->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16(Response->Key);
            Response->Key &= 0x7FFF;
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
        }

        if (Response->Key != rmqscSaslAuthenticate)
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
size_t rmqsAddInt8ToStream(rmqsMemBuffer_t *Stream, int8_t Value)
{
    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt8ToStream(rmqsMemBuffer_t *Stream, uint8_t Value)
{
    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt16ToStream(rmqsMemBuffer_t *Stream, int16_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt16ToStream(rmqsMemBuffer_t *Stream, uint16_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt32ToStream(rmqsMemBuffer_t *Stream, int32_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt32ToStream(rmqsMemBuffer_t *Stream, uint32_t Value, uint8_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddStringToStream(rmqsMemBuffer_t *Stream, char_t *Value, uint8_t IsLittleEndianMachine)
{
    rmqsStringLen_t StringLen;
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
        rmqsMemBufferWrite(Stream, (void *)Value, StringLen);
        BytesAdded += StringLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------
size_t rmqsAddBytesToStream(rmqsMemBuffer_t *Stream, void *Value, size_t ValueLength, uint8_t IsLittleEndianMachine)
{
    rmqsDataLen_t DataLen;
    size_t BytesAdded;

    if (ValueLength == 0)
    {
        DataLen = RMQS_EMPTY_DATA_LENGTH;
    }
    else
    {
        DataLen = (rmqsDataLen_t)ValueLength;
    }

    rmqsAddInt32ToStream(Stream, DataLen, IsLittleEndianMachine);
    BytesAdded = sizeof(DataLen);

    if (DataLen != RMQS_EMPTY_DATA_LENGTH)
    {
        rmqsMemBufferWrite(Stream, (void *)Value, DataLen);
        BytesAdded += DataLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------

