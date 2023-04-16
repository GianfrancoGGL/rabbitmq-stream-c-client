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
#include "rmqsClient.h"
#include "rmqsPublisher.h"
#include "rmqsConsumer.h"
#include "rmqsBroker.h"
#include "rmqsClientConfiguration.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, rmqsClientType_t ClientType, void *ParentObject, MetadataUpdateCallback_t MetadataUpdateCallback)
{
    rmqsClient_t *Client = (rmqsClient_t *)rmqsAllocateMemory(sizeof(rmqsClient_t));

    memset(Client, 0, sizeof(rmqsClient_t));

    Client->ClientConfiguration = ClientConfiguration;
    Client->ClientType = ClientType;
    Client->ParentObject = ParentObject;
    Client->CorrelationId = 0;
    Client->TxQueue = rmqsBufferCreate();
    Client->RxQueue = rmqsBufferCreate();
    Client->MetadataUpdateCallback = MetadataUpdateCallback;

    return Client;
}
//---------------------------------------------------------------------------
void rmqsClientDestroy(rmqsClient_t *Client)
{
    rmqsBufferDestroy(Client->TxQueue);
    rmqsBufferDestroy(Client->RxQueue);

    rmqsFreeMemory((void *)Client);
}
//---------------------------------------------------------------------------
bool_t rmqsClientLogin(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *VirtualHost, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode)
{
    rmqsBroker_t *Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Client->ClientConfiguration->BrokerList, 0);
    bool_t PlainAuthSupported;
    rmqsTuneRequest_t TuneRequest, TuneResponse;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    //
    // Send the peer properties request
    //
    if (! rmqsPeerProperties(Client, Socket, Properties, PropertyCount, ResponseCode))
    {
        return false;
    }

    //
    // Then the SASL handshake request
    //
    if (! rmqsSaslHandshake(Client, Socket, &PlainAuthSupported, ResponseCode))
    {
        return false;
    }

    //
    // Next, the authenticate request, based on the supported mechanism
    //
    if (! PlainAuthSupported || ! rmqsSaslAuthenticate(Client, Socket, RMQS_PLAIN_PROTOCOL, (char_t *)Broker->Username, (char_t *)Broker->Password, ResponseCode))
    {
        return false;
    }

    //
    // Wait for the tune message sent by the server after the authentication
    //
    *ResponseCode = rmqsrOK;

    if (! rmqsWaitMessage(Client, Socket, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }

    if (Client->RxQueue->Tag2 != sizeof(rmqsTuneRequest_t))
    {
        return false;
    }

    //
    // Read the tune request and prepares the tune response
    //
    memcpy(&TuneRequest, Client->RxQueue->Data, sizeof(TuneRequest));
    memcpy(&TuneResponse, Client->RxQueue->Data, sizeof(TuneResponse));

    if (Client->ClientConfiguration->IsLittleEndianMachine)
    {
        TuneRequest.Header.Size = SwapUInt32(TuneRequest.Header.Size);
        TuneRequest.Header.Key = SwapUInt16(TuneRequest.Header.Key);
        TuneRequest.Header.Version = SwapUInt16(TuneRequest.Header.Version);
        TuneRequest.FrameMax = Client->ClientMaxFrameSize = SwapUInt32(TuneRequest.FrameMax);
        TuneRequest.Heartbeat = Client->ClientHeartbeat = SwapUInt16(TuneRequest.Heartbeat);
    }

    if (TuneRequest.Header.Key != rmqscTune)
    {
        return false;
    }

    if (Client->ClientType == rmqsctPublisher)
    {
        TuneResponse.Heartbeat = ((rmqsPublisher_t *)Client->ParentObject)->Heartbeat;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            TuneResponse.Heartbeat = SwapUInt32(TuneResponse.Heartbeat);
        }
    }
    else
    {
        TuneResponse.FrameMax = ((rmqsConsumer_t *)Client->ParentObject)->FrameMax;
        TuneResponse.Heartbeat = ((rmqsConsumer_t *)Client->ParentObject)->Heartbeat;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            TuneResponse.FrameMax = SwapUInt32(TuneResponse.FrameMax);
            TuneResponse.Heartbeat = SwapUInt32(TuneResponse.Heartbeat);
        }
    }

    //
    // Confirm the tune sending the response
    //
    rmqsSendMessage(Client, Socket, (char_t *)&TuneResponse, sizeof(rmqsTuneRequest_t));

    //
    // Finally, issue the open request
    //
    if (! rmqsOpen(Client, Socket, VirtualHost, ResponseCode))
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
bool_t rmqsClientLogout(rmqsClient_t *Client, rmqsSocket_t Socket, uint16_t ClosingCode, char_t *ClosingReason, rmqsResponseCode_t *ResponseCode)
{
    *ResponseCode = rmqsrOK;

    return rmqsClose(Client, Socket, ClosingCode, ClosingReason, ResponseCode);
}
//---------------------------------------------------------------------------
bool_t rmqsPeerProperties(rmqsClient_t *Client, rmqsSocket_t Socket, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscPeerProperties;
    uint16_t Version = 1;
    uint32_t i;
    uint32_t MapSize;
    rmqsProperty_t *Property;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    if (PropertyCount > 0)
    {
        //
        // Calculate the map size
        //
        MapSize = sizeof(MapSize); // The map size field

        for (i = 0; i < PropertyCount; i++)
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
            MapSize += (uint32_t)strlen(Property->Key);
            MapSize += sizeof(uint16_t); // Value length field
            MapSize += (uint32_t)strlen(Property->Value);
        }
    }
    else
    {
        MapSize = 0;
    }

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToBuffer(Client->TxQueue, MapSize, Client->ClientConfiguration->IsLittleEndianMachine);

    for (i = 0; i < PropertyCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToBuffer(Client->TxQueue, Property->Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property->Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscPeerProperties || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsSaslHandshake(rmqsClient_t *Client, rmqsSocket_t Socket, bool_t *PlainAuthSupported, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscSaslHandshake;
    uint16_t Version = 1;
    rmqsSaslHandshakeResponse_t *SaslHandshakeResponse;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t StringLen;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    *PlainAuthSupported = false; // By default assume that the PLAIN auth is not supported

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        //
        // Handshake response is different from a standard response, it has to be reparsed
        //
        SaslHandshakeResponse = (rmqsSaslHandshakeResponse_t *)Client->RxQueue->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            SaslHandshakeResponse->Header.Size = SwapUInt32(SaslHandshakeResponse->Header.Size);
            SaslHandshakeResponse->Header.Key = SwapUInt16(SaslHandshakeResponse->Header.Key);
            SaslHandshakeResponse->Header.Key &= 0x7FFF;
            SaslHandshakeResponse->Header.Version = SwapUInt16(SaslHandshakeResponse->Header.Version);
            SaslHandshakeResponse->CorrelationId = SwapUInt32(SaslHandshakeResponse->CorrelationId);
            SaslHandshakeResponse->ResponseCode = SwapUInt16(SaslHandshakeResponse->ResponseCode);
            SaslHandshakeResponse->NoOfMechanisms = SwapUInt16(SaslHandshakeResponse->NoOfMechanisms);
        }

        *ResponseCode = SaslHandshakeResponse->ResponseCode;

        if (SaslHandshakeResponse->Header.Key != rmqscSaslHandshake || SaslHandshakeResponse->ResponseCode != rmqsrOK)
        {
            return false;
        }

        if (SaslHandshakeResponse->NoOfMechanisms > 0)
        {
            Data = (char_t *)Client->RxQueue->Data + sizeof(rmqsSaslHandshakeResponse_t);

            for (MechanismNo = 1; MechanismNo <= SaslHandshakeResponse->NoOfMechanisms; MechanismNo++)
            {
                rmqsGetUInt16FromMemory(&Data, &StringLen, Client->ClientConfiguration->IsLittleEndianMachine);

                if (StringLen > 0 && ! memcmp(Data, RMQS_PLAIN_PROTOCOL, strlen(RMQS_PLAIN_PROTOCOL)))
                {
                    *PlainAuthSupported = true;
                }

                Data += StringLen;
            }
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsSaslAuthenticate(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Mechanism, char_t *Username, char_t *Password, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscSaslAuthenticate;
    uint16_t Version = 1;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, (char_t *)Mechanism, Client->ClientConfiguration->IsLittleEndianMachine);

    UsernameLen = strlen(Username);
    PasswordLen = strlen(Password);
    OpaqueDataLen = UsernameLen + PasswordLen + 2; // +1 because there must be 0 before username and password
    OpaqueData = rmqsAllocateMemory(OpaqueDataLen);
    memset(OpaqueData, 0, OpaqueDataLen);
    memcpy(OpaqueData + 1, Username, UsernameLen);
    memcpy(OpaqueData + UsernameLen + 2, Password, PasswordLen);

    rmqsAddBytesToBuffer(Client->TxQueue, OpaqueData, OpaqueDataLen, true, Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsFreeMemory(OpaqueData);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscSaslAuthenticate || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsOpen(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *VirtualHost, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscOpen;
    uint16_t Version = 1;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, VirtualHost, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscOpen || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsClose(rmqsClient_t *Client, rmqsSocket_t Socket, uint16_t ClosingCode, char_t *ClosingReason, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscClose;
    uint16_t Version = 1;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, ClosingCode, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, ClosingReason, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscClose || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsCreate(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Stream, rmqsCreateStreamArgs_t *CreateStreamArgs, bool_t *StreamAlreadyExists, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscCreate;
    uint16_t Version = 1;
    size_t NoOfArgs = 0;
    rmqsProperty_t Property;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    *StreamAlreadyExists = false;

    NoOfArgs += CreateStreamArgs->SetMaxLengthBytes ? 1 : 0;
    NoOfArgs += CreateStreamArgs->SetMaxAge ? 1 : 0;
    NoOfArgs += CreateStreamArgs->SetStreamMaxSegmentSizeBytes ? 1 : 0;
    NoOfArgs += CreateStreamArgs->SetQueueLeaderLocator ? 1 : 0;
    NoOfArgs += CreateStreamArgs->SetInitialClusterSize ? 1 : 0;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, (char_t *)Stream, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)NoOfArgs, Client->ClientConfiguration->IsLittleEndianMachine);

    if (CreateStreamArgs->SetMaxLengthBytes)
    {
        memset(&Property, 0, sizeof(Property));

        strncpy(Property.Key, "max-length-bytes", RMQS_MAX_KEY_SIZE);
        sprintf(Property.Value, "%u", (uint32_t)CreateStreamArgs->MaxLengthBytes);

        rmqsAddStringToBuffer(Client->TxQueue, Property.Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property.Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    if (CreateStreamArgs->SetMaxAge)
    {
        strncpy(Property.Key, "max-age", RMQS_MAX_KEY_SIZE);
        strncpy(Property.Value, (char_t *) CreateStreamArgs->MaxAge, RMQS_MAX_VALUE_SIZE);

        rmqsAddStringToBuffer(Client->TxQueue, Property.Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property.Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    if (CreateStreamArgs->SetStreamMaxSegmentSizeBytes)
    {
        strncpy(Property.Key, "stream-max-segment-size-bytes", RMQS_MAX_KEY_SIZE);
        sprintf(Property.Value, "%u", (uint32_t)CreateStreamArgs->StreamMaxSegmentSizeBytes);

        rmqsAddStringToBuffer(Client->TxQueue, Property.Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property.Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    if (CreateStreamArgs->SetQueueLeaderLocator)
    {
        strncpy(Property.Key, "queue-leader-locator", RMQS_MAX_KEY_SIZE);

        switch (CreateStreamArgs->LeaderLocator)
        {
            case rmqssllClientLocal:
            default:
                strncpy(Property.Value, "client-local", RMQS_MAX_VALUE_SIZE);
                break;
            case rmqssllBalanced:
                strncpy(Property.Value, "balanced", RMQS_MAX_VALUE_SIZE);
                break;
            case rmqssllRandom:
                strncpy(Property.Value, "random", RMQS_MAX_VALUE_SIZE);
                break;
            case rmqssllLeasrLeaders:
                strncpy(Property.Value, "least-leaders", RMQS_MAX_VALUE_SIZE);
                break;
        }

        rmqsAddStringToBuffer(Client->TxQueue, Property.Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property.Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    if (CreateStreamArgs->SetInitialClusterSize)
    {
        strncpy(Property.Key, "initial-cluster-size", RMQS_MAX_KEY_SIZE);
        sprintf(Property.Value, "%u", (uint32_t)CreateStreamArgs->InitialClusterSize);

        rmqsAddStringToBuffer(Client->TxQueue, Property.Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property.Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscCreate || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        *StreamAlreadyExists = Client->Response.ResponseCode == rmqsrStreamAlreadyExists;

        if (*StreamAlreadyExists)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsDelete(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Stream, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscDelete;
    uint16_t Version = 1;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, (char_t *)Stream, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscDelete || Client->Response.ResponseCode != rmqsrOK)
        {
            return false;
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsMetadata(rmqsClient_t *Client, rmqsSocket_t Socket, char_t **Streams, size_t StreamCount, rmqsMetadata_t *Metadata, rmqsResponseCode_t *ResponseCode)
{
    uint16_t Key = rmqscMetadata;
    uint16_t Version = 1;
    uint32_t i, j;
    char_t *Data;
    rmqsBrokerMetadata_t *BrokerMetadata;
    rmqsStreamMetadata_t *StreamMetadata;
    bool_t ConnectionError;

    *ResponseCode = rmqsrOK;

    //
    // The metadata structure is cleared to be sure that it doesn't give problems within this function,
    // even if it's supposed to be already cleared, otherwise ther could be memory leaks since
    // the structure pointers are going to be lost
    //
    rmqsMetadataClear(Metadata);

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)StreamCount, Client->ClientConfiguration->IsLittleEndianMachine);

    for (i = 0; i < StreamCount; i++)
    {
        rmqsAddStringToBuffer(Client->TxQueue, *Streams, Client->ClientConfiguration->IsLittleEndianMachine);
        Streams++;
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_DEFAULT_TIMEOUT, &ConnectionError))
    {
        *ResponseCode = Client->Response.ResponseCode;

        if (Client->Response.Header.Key != rmqscMetadata)
        {
            return false;
        }

        Data = (char_t *)Client->RxQueue->Data;
        Data += sizeof(rmqsMsgHeader_t) + sizeof(uint32_t); // Moves the pointer to data over the header and correlation id

        rmqsGetUInt32FromMemory(&Data, &Metadata->BrokerMetadataCount, Client->ClientConfiguration->IsLittleEndianMachine);

        if (Metadata->BrokerMetadataCount > 0)
        {
            Metadata->BrokerMetadataList = rmqsAllocateMemory(sizeof(rmqsBrokerMetadata_t) * Metadata->BrokerMetadataCount);
            memset(Metadata->BrokerMetadataList, 0, sizeof(rmqsBrokerMetadata_t) * Metadata->BrokerMetadataCount);

            for (i = 0; i < Metadata->BrokerMetadataCount; i++)
            {
                BrokerMetadata = &Metadata->BrokerMetadataList[i];

                rmqsGetUInt16FromMemory(&Data, &BrokerMetadata->Reference, Client->ClientConfiguration->IsLittleEndianMachine);
                rmqsGetStringFromMemory(&Data, BrokerMetadata->Host, RQMS_MAX_HOSTNAME_LENGTH, Client->ClientConfiguration->IsLittleEndianMachine);
                rmqsGetUInt32FromMemory(&Data, &BrokerMetadata->Port, Client->ClientConfiguration->IsLittleEndianMachine);
            }
        }

        rmqsGetUInt32FromMemory(&Data, &Metadata->StreamMetadataCount, Client->ClientConfiguration->IsLittleEndianMachine);

        if (Metadata->StreamMetadataCount == 0)
        {
            return true;
        }

        Metadata->StreamMetadataList = rmqsAllocateMemory(sizeof(rmqsStreamMetadata_t) * Metadata->StreamMetadataCount);
        memset(Metadata->StreamMetadataList, 0, sizeof(rmqsStreamMetadata_t) * Metadata->StreamMetadataCount);

        for (i = 0; i < Metadata->StreamMetadataCount; i++)
        {
            StreamMetadata = &Metadata->StreamMetadataList[i];

            rmqsGetStringFromMemory(&Data, StreamMetadata->Stream, RMQS_MAX_STREAM_NAME_LENGTH, Client->ClientConfiguration->IsLittleEndianMachine);
            rmqsGetUInt16FromMemory(&Data, &StreamMetadata->Code, Client->ClientConfiguration->IsLittleEndianMachine);
            rmqsGetUInt16FromMemory(&Data, &StreamMetadata->LeaderReference, Client->ClientConfiguration->IsLittleEndianMachine);
            rmqsGetUInt32FromMemory(&Data, &StreamMetadata->ReplicasReferenceCount, Client->ClientConfiguration->IsLittleEndianMachine);

            if (StreamMetadata->ReplicasReferenceCount > 0)
            {
                StreamMetadata->ReplicasReferences = rmqsAllocateMemory(sizeof(uint16_t) * Metadata->StreamMetadataList[i].ReplicasReferenceCount);
                memset(StreamMetadata->ReplicasReferences, 0, sizeof(uint16_t) * Metadata->StreamMetadataList[i].ReplicasReferenceCount);

                for (j = 0; j < StreamMetadata->ReplicasReferenceCount; j++)
                {
                    rmqsGetUInt16FromMemory(&Data, &StreamMetadata->ReplicasReferences[j], Client->ClientConfiguration->IsLittleEndianMachine);
                }
            }
        }

        return true;
    }
    else
    {
        if (ConnectionError)
        {
            *ResponseCode = rmqsrConnectionError;
        }

        return false;
    }
}
//---------------------------------------------------------------------------
void rmqsHeartbeat(rmqsClient_t *Client, rmqsSocket_t Socket)
{
    rmqsClientConfiguration_t *ClientConfiguration = (rmqsClientConfiguration_t *)Client->ClientConfiguration;
    uint16_t Key = rmqscHeartbeat;
    uint16_t Version = 1;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, (uint32_t)(Client->TxQueue->Size - sizeof(uint32_t)), ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (char_t *)Client->TxQueue->Data, Client->TxQueue->Size);
}
//---------------------------------------------------------------------------
void rmqsHandleMetadataUpdate(rmqsClient_t *Client, rmqsBuffer_t *Buffer)
{
    char_t *MessagePayload = (char_t *)Buffer->Data + sizeof(rmqsMsgHeader_t);
    uint16_t Code;
    char_t Stream[RMQS_MAX_STREAM_NAME_LENGTH + 1];

    rmqsGetUInt16FromMemory(&MessagePayload, &Code, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsGetStringFromMemory(&MessagePayload, Stream, RMQS_MAX_STREAM_NAME_LENGTH, Client->ClientConfiguration->IsLittleEndianMachine);

    Client->MetadataUpdateCallback(Code, Stream);
}
//---------------------------------------------------------------------------
rmqsMetadata_t *rmqsMetadataCreate(void)
{
    rmqsMetadata_t *Metadata = (rmqsMetadata_t *)rmqsAllocateMemory(sizeof(rmqsMetadata_t));

    rmqsMetadataClear(Metadata);

    return Metadata;
}
//---------------------------------------------------------------------------
void rmqsMetadataClear(rmqsMetadata_t *Metadata)
{
    memset(Metadata, 0, sizeof(rmqsMetadata_t));
}
//---------------------------------------------------------------------------
void rmqsMetadataDestroy(rmqsMetadata_t *Metadata)
{
    uint32_t i;

    if (Metadata->BrokerMetadataList != 0)
    {
        rmqsFreeMemory(Metadata->BrokerMetadataList);
    }

    if (Metadata->StreamMetadataList != 0)
    {
        for (i = 0; i < Metadata->StreamMetadataCount; i++)
        {
            if (Metadata->StreamMetadataList[i].ReplicasReferences != 0)
            {
                rmqsFreeMemory(Metadata->StreamMetadataList[i].ReplicasReferences);
            }
        }

        rmqsFreeMemory(Metadata->StreamMetadataList);
    }

    rmqsFreeMemory(Metadata);
}
//---------------------------------------------------------------------------
