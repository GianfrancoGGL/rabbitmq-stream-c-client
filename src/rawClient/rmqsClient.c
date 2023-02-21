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
bool_t rqmsClientLogin(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost, rmqsProperty_t *Properties, const size_t PropertiesCount)
{
    rmqsBroker_t *Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Client->ClientConfiguration->BrokerList, 0);
    bool_t PlainAuthSupported;
    rmqsTuneRequest_t TuneRequest, TuneResponse;

    //
    // Send the peer properties request
    //
    if (rmqsPeerProperties(Client, Socket, Properties, PropertiesCount) != rmqsrOK)
    {
        return false;
    }

    //
    // Then the SASL handshake request
    //
    if (rmqsSaslHandshake(Client, Socket, &PlainAuthSupported) != rmqsrOK)
    {
        return false;
    }

    //
    // Next, the authenticate request, based on the supported mechanism
    //
    if (! PlainAuthSupported || rmqsSaslAuthenticate(Client, Socket, RMQS_PLAIN_PROTOCOL, (const char_t *)Broker->Username, (const char_t *)Broker->Password) != rmqsrOK)
    {
        return false;
    }

    //
    // Wait for the tune message sent by the server after the authentication
    //
    if (! rmqsWaitMessage(Client, Socket, 1000))
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

    //
    // Confirm the tune sending the response
    //
    rmqsSendMessage(Client, Socket, (const char_t *)&TuneResponse, sizeof(rmqsTuneRequest_t));

    //
    // Finally, issue the open request
    //
    if (rmqsOpen(Client, Socket, VirtualHost) != rmqsrOK)
    {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
bool_t rqmsClientLogout(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason)
{
    if (rmqsClose(Client, Socket, ClosingCode, ClosingReason) == rmqsrOK)
    {
        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
rmqsResponseCode_t rmqsPeerProperties(rmqsClient_t *Client, const rmqsSocket Socket, rmqsProperty_t *Properties, size_t PropertiesCount)
{
    uint16_t Key = rmqscPeerProperties;
    uint16_t Version = 1;
    uint32_t i, MapSize;
    rmqsProperty_t *Property;

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

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToBuffer(Client->TxQueue, MapSize, Client->ClientConfiguration->IsLittleEndianMachine);

    for (i = 0; i < PropertiesCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToBuffer(Client->TxQueue, Property->Key, Client->ClientConfiguration->IsLittleEndianMachine);
        rmqsAddStringToBuffer(Client->TxQueue, Property->Value, Client->ClientConfiguration->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscPeerProperties)
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
rmqsResponseCode_t rmqsSaslHandshake(rmqsClient_t *Client, rmqsSocket Socket, bool_t *PlainAuthSupported)
{
    uint16_t Key = rmqscSaslHandshake;
    uint16_t Version = 1;
    rmqsResponseHandshakeRequest_t *HandshakeResponse;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t *StringLen;
    char_t *String;

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
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        //
        // Handshake response is different from a standard response, it has to be reparsed
        //
        HandshakeResponse = (rmqsResponseHandshakeRequest_t *)Client->RxQueue->Data;

        if (Client->ClientConfiguration->IsLittleEndianMachine)
        {
            HandshakeResponse->Header.Size = SwapUInt32(HandshakeResponse->Header.Size);
            HandshakeResponse->Header.Key = SwapUInt16(HandshakeResponse->Header.Key);
            HandshakeResponse->Header.Key &= 0x7FFF;
            HandshakeResponse->Header.Version = SwapUInt16(HandshakeResponse->Header.Version);
            HandshakeResponse->CorrelationId = SwapUInt32(HandshakeResponse->CorrelationId);
            HandshakeResponse->ResponseCode = SwapUInt16(HandshakeResponse->ResponseCode);
            HandshakeResponse->NoOfMechanisms = SwapUInt16(HandshakeResponse->NoOfMechanisms);
        }

        if (HandshakeResponse->Header.Key != rmqscSaslHandshake)
        {
            return rmqsrWrongReply;
        }

        if (HandshakeResponse->NoOfMechanisms > 0)
        {
            Data = (char_t *)Client->RxQueue->Data + sizeof(rmqsResponseHandshakeRequest_t);

            for (MechanismNo = 1; MechanismNo <= HandshakeResponse->NoOfMechanisms; MechanismNo++)
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

        return HandshakeResponse->ResponseCode;
    }
    else
    {
        return rmqsrNoReply;
    }
}
//---------------------------------------------------------------------------
rmqsResponseCode_t rmqsSaslAuthenticate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Mechanism, const char_t *Username, const char_t *Password)
{
    uint16_t Key = rmqscSaslAuthenticate;
    uint16_t Version = 1;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;

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
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscSaslAuthenticate)
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
rmqsResponseCode_t rmqsOpen(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost)
{
    uint16_t Key = rmqscOpen;
    uint16_t Version = 1;

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
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscOpen)
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
rmqsResponseCode_t rmqsClose(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason)
{
    uint16_t Key = rmqscClose;
    uint16_t Version = 1;

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
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscClose)
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
rmqsResponseCode_t rmqsCreate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *StreamName, const rqmsCreateStreamArgs_t *CreateStreamArgs)
{
    uint16_t Key = rmqscCreate;
    uint16_t Version = 1;
    size_t NoOfArgs = 0;
    rmqsProperty_t Property;

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
    rmqsAddStringToBuffer(Client->TxQueue, (char_t *)StreamName, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, NoOfArgs, Client->ClientConfiguration->IsLittleEndianMachine);

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
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscCreate)
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
rmqsResponseCode_t rmqsDelete(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *StreamName)
{
    uint16_t Key = rmqscDelete;
    uint16_t Version = 1;

    rmqsBufferClear(Client->TxQueue, false);

    rmqsAddUInt32ToBuffer(Client->TxQueue, 0, Client->ClientConfiguration->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToBuffer(Client->TxQueue, Key, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt16ToBuffer(Client->TxQueue, Version, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddUInt32ToBuffer(Client->TxQueue, ++Client->CorrelationId, Client->ClientConfiguration->IsLittleEndianMachine);
    rmqsAddStringToBuffer(Client->TxQueue, (char_t *)StreamName, Client->ClientConfiguration->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsBufferMoveTo(Client->TxQueue, 0);
    rmqsAddUInt32ToBuffer(Client->TxQueue, Client->TxQueue->Size - sizeof(uint32_t), Client->ClientConfiguration->IsLittleEndianMachine);

    rmqsSendMessage(Client, Socket, (const char_t *)Client->TxQueue->Data, Client->TxQueue->Size);

    if (rmqsWaitResponse(Client, Socket, Client->CorrelationId, &Client->Response, 1000))
    {
        if (Client->Response.Header.Key != rmqscDelete)
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

