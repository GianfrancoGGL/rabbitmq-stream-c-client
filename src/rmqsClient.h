//---------------------------------------------------------------------------
#ifndef rmqsClientH
#define rmqsClientH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
#include "rmqsClientConfiguration.h"
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
    rmqsClientConfiguration_t *ClientConfiguration; // This pointer is void because of a circular dependency of environment and client structs
    rmqsClientStatus Status;
    uint32_t ClientMaxFrameSize;
    uint32_t ClientHeartbeat;
    void (*EventsCallback)(rqmsClientEvent, void *);
    void *ParentObject; // Producer or consumer
    void (*HandlerCallback)(void *);
    char_t Hostname[RQMS_MAX_HOSTNAME_LENGTH + 1];
    rmqsSocket Socket;
    uint32_t CorrelationId;
    rmqsMemBuffer_t *TxStream;
    rmqsMemBuffer_t *RxStream;
    rmqsMemBuffer_t *RxStreamTempBuffer;
    char_t RxSocketBuffer[RMQS_CLIENT_RX_BUFFER_SIZE];
    rmqsThread_t *ClientThread;
}
rmqsClient_t;
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, const char_t *Hostname, void (*EventsCallback)(rqmsClientEvent, void *), void *ParentObject, void (*HandlerCallback)(void *ParentObject));
void rmqsClientDestroy(rmqsClient_t *Client);
void rmqsClientThreadRoutine(void *Parameters, bool_t *TerminateRequest);
//---------------------------------------------------------------------------
bool_t rqmsClientLogin(rmqsClient_t *Client, rmqsProperty_t *Properties);
rmqsResponseCode rmqsPeerPropertiesRequest(rmqsClient_t *Client, uint32_t PropertiesCount, rmqsProperty_t *Properties);
rmqsResponseCode rmqsSaslHandshakeRequest(rmqsClient_t *Client, bool_t *PlainAuthSupported);
rmqsResponseCode rmqsSaslAuthenticateRequest(rmqsClient_t *Client, const char_t *Mechanism, const char_t *Username, const char_t *Password);
rmqsResponseCode rmqsOpenRequest(rmqsClient_t *Client, const char_t *Hostname);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
