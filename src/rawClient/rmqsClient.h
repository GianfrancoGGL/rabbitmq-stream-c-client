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
//---------------------------------------------------------------------------
#define RMQS_CLIENT_HOSTNAME_MAX_SIZE    256
#define RMQS_CLIENT_RX_BUFFER_SIZE      1024
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClientConfiguration_t *ClientConfiguration;
    uint32_t ClientMaxFrameSize;
    uint32_t ClientHeartbeat;
    void *ParentObject; // Producer or consumer
    uint32_t CorrelationId;
    rmqsMemBuffer_t *TxStream;
    rmqsMemBuffer_t *RxStream;
    rmqsMemBuffer_t *RxStreamTempBuffer;
    char_t RxSocketBuffer[RMQS_CLIENT_RX_BUFFER_SIZE];
}
rmqsClient_t;
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, void *ParentObject);
void rmqsClientDestroy(rmqsClient_t *Client);
//---------------------------------------------------------------------------
bool_t rqmsClientLogin(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost, rmqsProperty_t *Properties, const size_t PropertiesCount);
rmqsResponseCode rmqsPeerPropertiesRequest(rmqsClient_t *Client, const rmqsSocket Socket, rmqsProperty_t *Properties, const size_t PropertiesCount);
rmqsResponseCode rmqsSaslHandshakeRequest(rmqsClient_t *Client, const rmqsSocket Socket, bool_t *PlainAuthSupported);
rmqsResponseCode rmqsSaslAuthenticateRequest(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Mechanism, const char_t *Username, const char_t *Password);
rmqsResponseCode rmqsOpenRequest(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
