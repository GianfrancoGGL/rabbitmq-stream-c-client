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
typedef struct
{
    //
    // Total body size for ready messages a queue can contain before it starts to drop them from its head.
    //
    bool_t SpecifyMaxLengthBytes;
    size_t MaxLengthBytes;

    // Sets the data retention for stream queues in time units
    // (Y=Years, M=Months, D=Days, h=hours, m=minutes, s=seconds).
    // E.g. "1h" configures the stream to only keep the last 1 hour of received messages.
    bool_t SpecifyMaxAge;
    char_t MaxAge[16];

    //
    // Total segment size for stream segments on disk.
    //
    bool_t SpecifyStreamMaxSegmentSizeBytes;
    size_t StreamMaxSegmentSizeBytes;

    //
    // Set the rule by which the queue leader is located when declared on a cluster of nodes. Valid values are 'client-local' (default) and 'balanced'
    // Other values - to clarify: 'random' - 'least-leaders'

    //
    bool_t SpecifyQueueLeaderLocator;
    char_t QueueLeaderLocator[32];
}
rqmsCreateStreamParams_t;
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, void *ParentObject);
void rmqsClientDestroy(rmqsClient_t *Client);
//---------------------------------------------------------------------------
bool_t rqmsClientLogin(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost, rmqsProperty_t *Properties, const size_t PropertiesCount);
bool_t rqmsClientLogout(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason);
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerProperties(rmqsClient_t *Client, const rmqsSocket Socket, rmqsProperty_t *Properties, const size_t PropertiesCount);
rmqsResponseCode rmqsSaslHandshake(rmqsClient_t *Client, const rmqsSocket Socket, bool_t *PlainAuthSupported);
rmqsResponseCode rmqsSaslAuthenticate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Mechanism, const char_t *Username, const char_t *Password);
rmqsResponseCode rmqsOpen(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost);
rmqsResponseCode rmqsClose(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason);
rmqsResponseCode rmqsCreate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Stream, const rqmsCreateStreamParams_t *CreateStreamParams);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
