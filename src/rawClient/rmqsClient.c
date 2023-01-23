//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsBroker.h"
#include "rmqsClientConfiguration.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, void *ParentObject)
{
    rmqsClient_t *Client = (rmqsClient_t *)rmqsAllocateMemory(sizeof(rmqsClient_t));

    memset(Client, 0, sizeof(rmqsClient_t));

    Client->ClientConfiguration = ClientConfiguration;
    Client->ParentObject = ParentObject;
    Client->CorrelationId = 1;
    Client->TxStream = rmqsMemBufferCreate();
    Client->RxStream = rmqsMemBufferCreate();
    Client->RxStreamTempBuffer = rmqsMemBufferCreate();

    return Client;
}
//---------------------------------------------------------------------------
void rmqsClientDestroy(rmqsClient_t *Client)
{
    rmqsMemBufferDestroy(Client->TxStream);
    rmqsMemBufferDestroy(Client->RxStream);
    rmqsMemBufferDestroy(Client->RxStreamTempBuffer);

    rmqsFreeMemory((void *)Client);
}
//---------------------------------------------------------------------------
bool_t rqmsClientLogin(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost, rmqsProperty_t *Properties, const size_t PropertiesCount)
{
    rmqsBroker_t *Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Client->ClientConfiguration->BrokerList, 0);
    bool_t PlainAuthSupported;
    rmqsTuneRequest_t TuneRequest, TuneResponse;

    //
    // Once connected, send the peer properties request
    //
    if (rmqsPeerPropertiesRequest(Client, Socket, Properties, PropertiesCount) != rmqsrOK)
    {
        return 0;
    }

    //
    // Then the SASL handshake request
    //
    if (rmqsSaslHandshakeRequest(Client, Socket, &PlainAuthSupported) != rmqsrOK)
    {
        return 0;
    }

    //
    // Next, the authenticate request, based on the supported mechanism
    //
    if (! PlainAuthSupported || rmqsSaslAuthenticateRequest(Client, Socket, RMQS_PLAIN_PROTOCOL, (const char_t *)Broker->Username, (const char_t *)Broker->Password) != rmqsrOK)
    {
        return 0;
    }

    //
    // Wait for the tune message sent by the server after the authentication
    //
    if (! rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        return 0;
    }

    if (Client->RxStream->Size != sizeof(rmqsTuneRequest_t))
    {
        return 0;
    }

    //
    // Read the tune request and prepares the tune response
    //
    memcpy(&TuneRequest, Client->RxStream->Data, sizeof(rmqsTuneRequest_t));
    memcpy(&TuneResponse, Client->RxStream->Data, sizeof(rmqsTuneRequest_t));

    if (Client->ClientConfiguration->IsLittleEndianMachine)
    {
        TuneRequest.Size = SwapUInt32(TuneRequest.Size);
        TuneRequest.Key = SwapUInt16(TuneRequest.Key);
        TuneRequest.Version = SwapUInt16(TuneRequest.Version);
        TuneRequest.FrameMax = Client->ClientMaxFrameSize = SwapUInt32(TuneRequest.FrameMax);
        TuneRequest.Heartbeat = Client->ClientHeartbeat = SwapUInt16(TuneRequest.Heartbeat);
    }

    if (TuneRequest.Key != rmqscTune)
    {
        return 0;
    }

    //
    // Confirm the tune sending the response
    //
    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)&TuneResponse, sizeof(rmqsTuneRequest_t));

    //
    // Finally, issue the open request
    //
    if (rmqsOpenRequest(Client, Socket, VirtualHost) != rmqsrOK)
    {
        return 0;
    }

    return 1;
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerPropertiesRequest(rmqsClient_t *Client, const rmqsSocket Socket, rmqsProperty_t *Properties, size_t PropertiesCount)
{
    uint16_t Key = rmqscPeerProperties;
    uint16_t Version = 1;
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

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToStream(Client->TxStream, MapSize, Client->ClientConfiguration->IsLittleEndianMachine);

    for (i = 0; i < PropertiesCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToStream(Client->TxStream, Property->Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToStream(Client->TxStream, Property->Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)Client->RxStream->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
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
rmqsResponseCode rmqsSaslHandshakeRequest(rmqsClient_t *Client, rmqsSocket Socket, bool_t *PlainAuthSupported)
{
    uint16_t Key = rmqscSaslHandshake;
    uint16_t Version = 1;
    rmqsResponseHandshakeRequest_t *Response;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t *StringLen;
    char_t *String;

    *PlainAuthSupported = 0; // By default assume that the PLAIN auth is not supported

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponseHandshakeRequest_t *)Client->RxStream->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
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
            Data = (char_t *)Client->RxStream->Data + sizeof(rmqsResponseHandshakeRequest_t);

            for (MechanismNo = 1; MechanismNo <= Response->NoOfMechanisms; MechanismNo++)
            {
                StringLen = (uint16_t *)Data;

                if (Client->ClientConfiguration->IsLittleEndianMachine)
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
rmqsResponseCode rmqsSaslAuthenticateRequest(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Mechanism, const char_t *Username, const char_t *Password)
{
    uint16_t Key = rmqscSaslAuthenticate;
    uint16_t Version = 1;
    rmqsResponse_t *Response;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsAddStringToStream(Client->TxStream, (char_t *)Mechanism, Client->ClientConfiguration->IsLittleEndianMachine);

    UsernameLen = strlen(Username);
    PasswordLen = strlen(Password);
    OpaqueDataLen = UsernameLen + PasswordLen + 2; // +1 because there must be 0 before username and password
    OpaqueData = rmqsAllocateMemory(OpaqueDataLen);
    memset(OpaqueData, 0, OpaqueDataLen);
    memcpy(OpaqueData + 1, Username, UsernameLen);
    memcpy(OpaqueData + UsernameLen + 2, Password, PasswordLen);

    rmqsAddBytesToStream(Client->TxStream, OpaqueData, OpaqueDataLen, Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsFreeMemory(OpaqueData);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)Client->RxStream->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
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
rmqsResponseCode rmqsOpenRequest(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost)
{
    uint16_t Key = rmqscOpen;
    uint16_t Version = 1;
    rmqsResponse_t *Response;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToStream(Client->TxStream, VirtualHost, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client->ClientConfiguration, Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Client->ClientConfiguration, Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponse_t *)Client->RxStream->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            Response->Size = SwapUInt32(Response->Size);
            Response->Key = SwapUInt16(Response->Key);
            Response->Key &= 0x7FFF;
            Response->Version = SwapUInt16(Response->Version);
            Response->CorrelationId = SwapUInt32(Response->CorrelationId);
            Response->ResponseCode = SwapUInt16(Response->ResponseCode);
        }

        if (Response->Key != rmqscOpen)
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
