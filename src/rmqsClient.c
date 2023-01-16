//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsClient.h"
#include "rmqsEnvironment.h"
#include "rmqsProtocol.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(void *Environment, const char_t *HostName, void (*EventsCB)(rqmsClientEvent, void *), void *OwnerObject, void (*HandlerCB)(void *))
{
    rmqsClient_t *Client = (rmqsClient_t *)rmqsAllocateMemory(sizeof(rmqsClient_t));

    memset(Client, 0, sizeof(rmqsClient_t));

    Client->Environment = Environment;
    strncpy(Client->HostName, HostName, RMQS_CLIENT_HOSTNAME_MAX_SIZE);
    Client->Status = rmqscsDisconnected;
    Client->FrameMax = 0;
    Client->Heartbeat = 0;
    Client->EventsCB = EventsCB;
    Client->OwnerObject = OwnerObject;
    Client->HandlerCB = HandlerCB;

    //
    // Windows machine, initialize sockets
    //
    #ifdef __WIN32__
    rmqsInitWinsock();
    #endif

    Client->Socket = rmqsInvalidSocket;
    Client->CorrelationId = 1;

    Client->TxStream = rmqsMemBufferCreate();
    Client->RxStream = rmqsMemBufferCreate();
    Client->RxStreamTempBuffer = rmqsMemBufferCreate();

    Client->ClientThread = rmqsThreadCreate(rmqsClientThreadRoutine, 0, Client);
    rmqsThreadStart(Client->ClientThread);

    return Client;
}
//---------------------------------------------------------------------------
void rmqsClientDestroy(rmqsClient_t *Client)
{
    if (Client->Socket != rmqsInvalidSocket)
    {
        rmqsSocketDestroy((rmqsSocket *)&Client->Socket);
        Client->Status = rmqscsDisconnected;
        Client->FrameMax = 0;
        Client->Heartbeat = 0;
        Client->EventsCB(rmqsceDisconnected, Client);
    }

    rmqsThreadStop(Client->ClientThread);
    rmqsThreadDestroy(Client->ClientThread);

    rmqsMemBufferDestroy(Client->TxStream);
    rmqsMemBufferDestroy(Client->RxStream);
    rmqsMemBufferDestroy(Client->RxStreamTempBuffer);

    rmqsFreeMemory((void *)Client);

    //
    // Windows machine, shutdown sockets
    //
    #ifdef __WIN32__
    rmqsShutdownWinsock();
    #endif
}
//---------------------------------------------------------------------------
void rmqsClientThreadRoutine(void *Parameters, uint8_t *TerminateRequest)
{
    rmqsClient_t *Client = (rmqsClient_t *)Parameters;
    rmqsEnvironment_t *Environment = Client->Environment;
    rmqsProperty_t Properties[6];
    rmqsBroker_t *Broker;
    uint8_t ConnectionFailed;

    Broker = (rmqsBroker_t *)rmqsListGetDataByPosition(Environment->BrokersList, 0);

    memset(Properties, 0, sizeof(Properties));

    strncpy(Properties[0].Key, "connection_name", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[0].Value, "c-stream-locator", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[1].Key, "product", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[1].Value, "RabbitMQ Stream", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[2].Key, "copyright", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[2].Value, "Copyright (c) Undefined", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[3].Key, "information", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[3].Value, "Licensed under the MPL 2.0. See https://www.rabbitmq.com/", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[4].Key, "version", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[4].Value, "1.0", RMQS_MAX_VALUE_SIZE);

    strncpy(Properties[5].Key, "platform", RMQS_MAX_KEY_SIZE);
    strncpy(Properties[5].Value, "C", RMQS_MAX_VALUE_SIZE);

    while (! *TerminateRequest)
    {
        if (Broker == 0)
        {
            //
            // Brokers list is empty!
            //
            rmqsThreadSleepEx(2, 1000, TerminateRequest);
            continue;
        }

        ConnectionFailed = 0;

        switch (Client->Status)
        {
            case rmqscsDisconnected:
                Client->Socket = rmqsSocketCreate();

                rmqsSetTcpNoDelay(Client->Socket);
                rmqsSetSocketWriteTimeouts(Client->Socket, 5);
                rmqsSetKeepAlive(Client->Socket);

                if (rmqsSocketConnect(Broker->Host, Broker->Port, Client->Socket, 2000))
                {
                    Client->Status = rmqscsConnected;
                    Client->FrameMax = 0;
                    Client->Heartbeat = 0;
                    Client->EventsCB(rmqsceConnected, Client);
                }
                else
                {
                    ConnectionFailed = 1;
                    rmqsSocketDestroy((rmqsSocket *)&Client->Socket);
                }

                break;

            case rmqscsConnected:
                if (rqmsClientLogin(Client, Properties))
                {
                    Client->Status = rmqscsReady;
                    Client->EventsCB(rmqsceReady, Client);
                }
                else
                {
                    ConnectionFailed = 1;
                    rmqsSocketDestroy((rmqsSocket *)&Client->Socket);
                    Client->Status = rmqscsDisconnected;
                    Client->FrameMax = 0;
                    Client->Heartbeat = 0;
                    Client->EventsCB(rmqsceDisconnected, Client);
                }

                break;

            case rmqscsReady:
                Client->HandlerCB(Client->OwnerObject);

                break;
        }

        if (! ConnectionFailed)
        {
            rmqsThreadSleepEx(2, 1, TerminateRequest);
        }
        else
        {
            rmqsThreadSleepEx(2, 50, TerminateRequest);
        }
    }
}
//---------------------------------------------------------------------------
uint8_t rqmsClientLogin(rmqsClient_t *Client, rmqsProperty_t *Properties)
{
    rmqsEnvironment_t *Environment = Client->Environment;
    uint8_t PlainAuthSupported;
    rmqsTuneRequest_t TuneRequest, TuneResponse;

    //
    // Once connected, send the peer properties request
    //
    if (rmqsPeerPropertiesRequest(Client, 6, Properties) != rmqsrOK)
    {
        return 0;
    }

    //
    // Then the SASL handshake request
    //
    if (rmqsSaslHandshakeRequest(Client, &PlainAuthSupported) != rmqsrOK)
    {
        return 0;
    }

    //
    // Next, the authenticate request, based on the supported mechanism
    //
    if (! PlainAuthSupported || rmqsSaslAuthenticateRequest(Client, RMQS_PLAIN_PROTOCOL, (const char_t *)Environment->Username, (const char_t *)Environment->Password) != rmqsrOK)
    {
        return 0;
    }

    //
    // Wait for the tune message sent by the server after the authentication
    //
    if (! rmqsWaitMessage(Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
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

    if (Environment->IsLittleEndianMachine)
    {
        TuneRequest.Size = SwapUInt32(TuneRequest.Size);
        TuneRequest.Key = SwapUInt16(TuneRequest.Key);
        TuneRequest.Version = SwapUInt16(TuneRequest.Version);
        TuneRequest.FrameMax = Client->FrameMax = SwapUInt32(TuneRequest.FrameMax);
        TuneRequest.Heartbeat = Client->Heartbeat = SwapUInt16(TuneRequest.Heartbeat);
    }

    if (TuneRequest.Key != rmqscTune)
    {
        return 0;
    }

    //
    // Confirm the tune sending the response
    //
    rmqsSendMessage(Environment, Client->Socket, (const char_t *)&TuneResponse, sizeof(rmqsTuneRequest_t));

    //
    // Finally, issue the open request
    //
    if (rmqsOpenRequest(Client, Client->HostName) != rmqsrOK)
    {
        return 0;
    }

    return 1;
}
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerPropertiesRequest(rmqsClient_t *Client, uint32_t PropertiesCount, rmqsProperty_t *Properties)
{
    rmqsEnvironment_t *Environment = Client->Environment;
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

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Environment->IsLittleEndianMachine);

    //
    // Encode the map
    //
    rmqsAddUInt32ToStream(Client->TxStream, MapSize, Environment->IsLittleEndianMachine);

    for (i = 0; i < PropertiesCount; i++)
    {
        Property = &Properties[i];

        rmqsAddStringToStream(Client->TxStream, Property->Key, Environment->IsLittleEndianMachine);
        rmqsAddStringToStream(Client->TxStream, Property->Value, Environment->IsLittleEndianMachine);
    }

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(Environment, Client->Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
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
rmqsResponseCode rmqsSaslHandshakeRequest(rmqsClient_t *Client, uint8_t *PlainAuthSupported)
{
    rmqsEnvironment_t *Environment = Client->Environment;
    rmqsKey_t Key = rmqscSaslHandshake;
    rmqsVersion_t Version = 1;
    rmqsResponseHandshakeRequest_t *Response;
    uint16_t MechanismNo;
    char_t *Data;
    uint16_t *StringLen;
    char_t *String;

    *PlainAuthSupported = 0; // By default assume that the PLAIN auth is not supported

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Environment->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(Environment, Client->Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
    {
        Response = (rmqsResponseHandshakeRequest_t *)Client->RxStream->Data;

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
            Data = (char_t *)Client->RxStream->Data + sizeof(rmqsResponseHandshakeRequest_t);

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
rmqsResponseCode rmqsSaslAuthenticateRequest(rmqsClient_t *Client, const char_t *Mechanism, const char_t *Username, const char_t *Password)
{
    rmqsEnvironment_t *Environment = Client->Environment;
    rmqsKey_t Key = rmqscSaslAuthenticate;
    rmqsVersion_t Version = 1;
    rmqsResponse_t *Response;
    char_t *OpaqueData;
    size_t UsernameLen, PasswordLen, OpaqueDataLen;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Environment->IsLittleEndianMachine);

    rmqsAddStringToStream(Client->TxStream, (char_t *)Mechanism, Environment->IsLittleEndianMachine);

    UsernameLen = strlen(Username);
    PasswordLen = strlen(Password);
    OpaqueDataLen = UsernameLen + PasswordLen + 2; // +1 because there must be NULL before username and password
    OpaqueData = rmqsAllocateMemory(OpaqueDataLen);
    memset(OpaqueData, 0, OpaqueDataLen);
    memcpy(OpaqueData + 1, Username, UsernameLen);
    memcpy(OpaqueData + UsernameLen + 2, Password, PasswordLen);

    rmqsAddBytesToStream(Client->TxStream, OpaqueData, OpaqueDataLen, Environment->IsLittleEndianMachine);

    rmqsFreeMemory(OpaqueData);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(Environment, Client->Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
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
rmqsResponseCode rmqsOpenRequest(rmqsClient_t *Client, const char_t *HostName)
{
    rmqsEnvironment_t *Environment = (rmqsEnvironment_t *)Client->Environment;
    rmqsKey_t Key = rmqscOpen;
    rmqsVersion_t Version = 1;
    rmqsResponse_t *Response;

    rmqsMemBufferClear(Client->TxStream, 0);

    rmqsAddUInt32ToStream(Client->TxStream, 0, Environment->IsLittleEndianMachine); // Size is zero for now
    rmqsAddUInt16ToStream(Client->TxStream, Key, Environment->IsLittleEndianMachine);
    rmqsAddUInt16ToStream(Client->TxStream, Version, Environment->IsLittleEndianMachine);
    rmqsAddUInt32ToStream(Client->TxStream, Client->CorrelationId++, Environment->IsLittleEndianMachine);
    rmqsAddStringToStream(Client->TxStream, HostName, Environment->IsLittleEndianMachine);

    //
    // Moves to the beginning of the stream and writes the total message body size
    //
    rmqsMemBufferMoveTo(Client->TxStream, 0);
    rmqsAddUInt32ToStream(Client->TxStream, Client->TxStream->Size - sizeof(rmqsSize_t), Environment->IsLittleEndianMachine);

    rmqsSendMessage(Environment, Client->Socket, (const char_t *)Client->TxStream->Data, Client->TxStream->Size);

    if (rmqsWaitMessage(Environment, Client->Socket, Client->RxSocketBuffer, sizeof(Client->RxSocketBuffer), Client->RxStream, Client->RxStreamTempBuffer, 1000))
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

