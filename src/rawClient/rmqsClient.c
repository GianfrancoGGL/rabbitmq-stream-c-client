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
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, rmqsClientType_t ClientType, void *ParentObject)
{
    rmqsClient_t *Client = (rmqsClient_t *)rmqsAllocateMemory(sizeof(rmqsClient_t));

    memset(Client, 0, sizeof(rmqsClient_t));

    Client->ClientConfiguration = ClientConfiguration;
    Client->ClientType = ClientType;
    Client->ParentObject = ParentObject;
    Client->CorrelationId = 0;
    Client->TxQueue = rmqsBufferCreate();
    Client->RxQueue = rmqsBufferCreate();

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
bool_t rqmsClientLogin(rmqsClient_t *Client, rmqsSocket Socket, char_t *VirtualHost, rmqsProperty_t *Properties, size_t PropertyCount)
{
    rmqsBroker_t *Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Client->ClientConfiguration->BrokerList, 0);
    bool_t PlainAuthSupported;
    rmqsTuneRequest_t TuneRequest, TuneResponse;
    bool_t ConnectionLost;

    //
    // Send the peer properties request
    //
    if (! rmqsPeerProperties(Client, Socket, Properties, PropertyCount))
    {
        return false;
    }

    //
    // Then the SASL handshake request
    //
    if (! rmqsSaslHandshake(Client, Socket, &PlainAuthSupported))
    {
        return false;
    }

    //
    // Next, the authenticate request, based on the supported mechanism
    //
    if (! PlainAuthSupported || ! rmqsSaslAuthenticate(Client, Socket, RMQS_PLAIN_PROTOCOL, (char_t *)Broker->Username, (char_t *)Broker->Password))
    {
        return false;
    }

    //
    // Wait for the tune message sent by the server after the authentication
    //
    if (! rmqsWaitMessage(Client, Socket, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
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
    if (! rmqsOpen(Client, Socket, VirtualHost))
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
bool_t rqmsClientLogout(rmqsClient_t *Client, rmqsSocket Socket, uint16_t ClosingCode, char_t *ClosingReason)
{
    return rmqsClose(Client, Socket, ClosingCode, ClosingReason);
}
//---------------------------------------------------------------------------
bool_t rmqsPeerProperties(rmqsClient_t *Client, rmqsSocket Socket, rmqsProperty_t *Properties, size_t PropertyCount)
{
    uint16_t Key = rmqscPeerProperties;
    uint16_t Version = 1;
    uint32_t i;
    uint32_t MapSize;
    rmqsProperty_t *Property;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscPeerProperties)
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
bool_t rmqsSaslHandshake(rmqsClient_t *Client, rmqsSocket Socket, bool_t *PlainAuthSupported)
{
    uint16_t Key = rmqscSaslHandshake;
    uint16_t Version = 1;
    rmqsSaslHandshakeResponse_t *SaslHandshakeResponse;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t *StringLen;
    char_t *String;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
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

        if (SaslHandshakeResponse->Header.Key != rmqscSaslHandshake)
        {
            return false;
        }

        if (SaslHandshakeResponse->NoOfMechanisms > 0)
        {
            Data = (char_t *)Client->RxQueue->Data + sizeof(rmqsSaslHandshakeResponse_t);

            for (MechanismNo = 1; MechanismNo <= SaslHandshakeResponse->NoOfMechanisms; MechanismNo++)
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
                        *PlainAuthSupported = true;
                    }

                    rmqsFreeMemory(String);

                    Data += *StringLen;
                }
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsSaslAuthenticate(rmqsClient_t *Client, rmqsSocket Socket, char_t *Mechanism, char_t *Username, char_t *Password)
{
    uint16_t Key = rmqscSaslAuthenticate;
    uint16_t Version = 1;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscSaslAuthenticate)
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
bool_t rmqsOpen(rmqsClient_t *Client, rmqsSocket Socket, char_t *VirtualHost)
{
    uint16_t Key = rmqscOpen;
    uint16_t Version = 1;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscOpen)
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
bool_t rmqsClose(rmqsClient_t *Client, rmqsSocket Socket, uint16_t ClosingCode, char_t *ClosingReason)
{
    uint16_t Key = rmqscClose;
    uint16_t Version = 1;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscClose)
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
bool_t rmqsCreate(rmqsClient_t *Client, rmqsSocket Socket, char_t *Stream, rqmsCreateStreamArgs_t *CreateStreamArgs, bool_t *StreamAlreadyExists)
{
    uint16_t Key = rmqscCreate;
    uint16_t Version = 1;
    size_t NoOfArgs = 0;
    rmqsProperty_t Property;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscCreate)
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
        return false;
    }
}
//---------------------------------------------------------------------------
bool_t rmqsDelete(rmqsClient_t *Client, rmqsSocket Socket, char_t *Stream)
{
    uint16_t Key = rmqscDelete;
    uint16_t Version = 1;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscDelete)
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
bool_t rmqsMetadata(rmqsClient_t *Client, rmqsSocket Socket, char_t **Streams, size_t StreamCount)
{
    uint16_t Key = rmqscMetadata;
    uint16_t Version = 1;
    uint32_t i;
    bool_t ConnectionLost;

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

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, RMQS_RX_TIMEOUT_INFINITE, &ConnectionLost))
    {
        if (Client->Response.Header.Key != rmqscMetadata)
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
void rmqsHeartbeat(rmqsClient_t *Client, rmqsSocket Socket)
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
