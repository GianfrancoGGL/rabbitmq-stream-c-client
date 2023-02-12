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
typedef enum
{
    rmqsctProducer,
    rmqsctConsumer
}
rmqsClientType_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClientConfiguration_t *ClientConfiguration;
    uint32_t ClientMaxFrameSize;
    uint32_t ClientHeartbeat;
    rmqsClientType_t ClientType;
    void *ParentObject; // Producer or consumer
    uint32_t CorrelationId;
    rmqsResponse_t Response;
    rmqsMemBuffer_t *TxQueue;
    rmqsMemBuffer_t *RxQueue;
    char_t RxSocketBuffer[RMQS_CLIENT_RX_BUFFER_SIZE];
}
rmqsClient_t;
//---------------------------------------------------------------------------
typedef enum
{
    rmqssllClientLocal,
    rmqssllBalanced,
    rmqssllRandom,
    rmqssllLeasrLeaders
}
rqmsStreamLeaderLocator_t;
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
    rqmsStreamLeaderLocator_t LeaderLocator;

    //
    // Set the queue initial cluster size.
    //
    bool_t SpecifyInitialClusterSize;
    size_t InitialClusterSize;
}
rqmsCreateStreamArgs_t;
//---------------------------------------------------------------------------
rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, rmqsClientType_t ClientType, void *ParentObject);
void rmqsClientDestroy(rmqsClient_t *Client);
//---------------------------------------------------------------------------
bool_t rqmsClientLogin(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost, rmqsProperty_t *Properties, const size_t PropertiesCount);
bool_t rqmsClientLogout(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason);
//---------------------------------------------------------------------------
rmqsResponseCode_t rmqsPeerProperties(rmqsClient_t *Client, const rmqsSocket Socket, rmqsProperty_t *Properties, const size_t PropertiesCount);
rmqsResponseCode_t rmqsSaslHandshake(rmqsClient_t *Client, const rmqsSocket Socket, bool_t *PlainAuthSupported);
rmqsResponseCode_t rmqsSaslAuthenticate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *Mechanism, const char_t *Username, const char_t *Password);
rmqsResponseCode_t rmqsOpen(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *VirtualHost);
rmqsResponseCode_t rmqsClose(rmqsClient_t *Client, const rmqsSocket Socket, const uint16_t ClosingCode, const char_t *ClosingReason);
rmqsResponseCode_t rmqsCreate(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *StreamName, const rqmsCreateStreamArgs_t *CreateStreamArgs);
rmqsResponseCode_t rmqsDelete(rmqsClient_t *Client, const rmqsSocket Socket, const char_t *StreamName);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
