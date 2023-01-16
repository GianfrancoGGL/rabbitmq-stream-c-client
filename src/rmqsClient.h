//---------------------------------------------------------------------------
#ifndef rmqsClientH
#define rmqsClientH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsNetwork.h"
#include "rmqsProtocol.h"
#include "rmqsMemBuffer.h"
#include "rmqsThread.h"
//---------------------------------------------------------------------------
#define RMQS_CLIENT_HOSTNAME_MAX_SIZE    256
#define RMQS_CLIENT_RX_BUFFER_SIZE      1024
//---------------------------------------------------------------------------
typedef enum
{
    rmqsceDisconnected = 0,
    rmqsceConnected,
    rmqsceReady
}
rqmsClientEvent;
//---------------------------------------------------------------------------
typedef enum
{
    rmqscsDisconnected = 0,
    rmqscsConnected,
    rmqscsReady
}
rmqsClientStatus;
//---------------------------------------------------------------------------
typedef struct
{
    void *Environment; // This pointer is void because of a circular dependency of environment and client structs
    char_t HostName[RMQS_CLIENT_HOSTNAME_MAX_SIZE + 1];
    rmqsClientStatus Status;
    uint32_t FrameMax;
    uint32_t Heartbeat;
    void (*EventsCB)(rqmsClientEvent, void *);
    void *OwnerObject;
    void (*HandlerCB)(void *);
    rmqsSocket Socket;
    rmqsCorrelationId_t CorrelationId;
    rmqsMemBuffer_t *TxStream;
    rmqsMemBuffer_t *RxStream;
    rmqsMemBuffer_t *RxStreamTempBuffer;
    char_t RxSocketBuffer[RMQS_CLIENT_RX_BUFFER_SIZE];
    rmqsThread_t *ClientThread;
}
rmqsClient_t;
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(void *Environment, const char_t *HostName, void (*EventsCB)(rqmsClientEvent, void *), void *OwnerObject, void (*HandlerCB)(void *OwnerObject));
void rmqsClientDestroy(rmqsClient_t *Client);
void rmqsClientThreadRoutine(void *Parameters, uint8_t *TerminateRequest);
//---------------------------------------------------------------------------
uint8_t rqmsClientLogin(rmqsClient_t *Client, rmqsProperty_t *Properties);
rmqsResponseCode rmqsPeerPropertiesRequest(rmqsClient_t *Client, uint32_t PropertiesCount, rmqsProperty_t *Properties);
rmqsResponseCode rmqsSaslHandshakeRequest(rmqsClient_t *Client, uint8_t *PlainAuthSupported);
rmqsResponseCode rmqsSaslAuthenticateRequest(rmqsClient_t *Client, const char_t *Mechanism, const char_t *Username, const char_t *Password);
rmqsResponseCode rmqsOpenRequest(rmqsClient_t *Client, const char_t *HostName);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
