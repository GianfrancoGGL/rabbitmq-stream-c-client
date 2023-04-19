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
#ifndef rmqsClientH
#define rmqsClientH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
#include "rmqsClientConfiguration.h"
#include "rmqsNetwork.h"
#include "rmqsProtocol.h"
#include "rmqsBuffer.h"
//---------------------------------------------------------------------------
#define RMQS_CLIENT_RX_BUFFER_SIZE      (1024 * 1000)
#define RMQS_MAX_STREAM_NAME_LENGTH       256
//---------------------------------------------------------------------------
#define RMQS_RX_DEFAULT_TIMEOUT         10000
//---------------------------------------------------------------------------
typedef enum
{
    rmqsctPublisher,
    rmqsctConsumer
}
rmqsClientType_t;
//---------------------------------------------------------------------------
typedef enum
{
    rmqssllClientLocal,
    rmqssllBalanced,
    rmqssllRandom,
    rmqssllLeasrLeaders
}
rmqsStreamLeaderLocator_t;
//---------------------------------------------------------------------------
typedef struct
{
    //
    // Total body size for ready messages a queue can contain before it starts to drop them from its head.
    //
    bool_t SetMaxLengthBytes;
    size_t MaxLengthBytes;

    // Sets the data retention for stream queues in time units
    // (Y=Years, M=Months, D=Days, h=hours, m=minutes, s=seconds).
    // E.g. "1h" configures the stream to only keep the last 1 hour of received messages.
    bool_t SetMaxAge;
    char_t MaxAge[16];

    //
    // Total segment size for stream segments on disk.
    //
    bool_t SetStreamMaxSegmentSizeBytes;
    size_t StreamMaxSegmentSizeBytes;

    //
    // Set the rule by which the queue leader is located when declared on a cluster of nodes. Valid values are 'client-local' (default) and 'balanced'
    // Other values - to clarify: 'random' - 'least-leaders'
    //
    bool_t SetQueueLeaderLocator;
    rmqsStreamLeaderLocator_t LeaderLocator;

    //
    // Set the queue initial cluster size.
    //
    bool_t SetInitialClusterSize;
    size_t InitialClusterSize;
}
rmqsCreateStreamArgs_t;
//---------------------------------------------------------------------------
typedef struct
{
    uint16_t Reference;
    char_t Host[RQMS_MAX_HOSTNAME_LENGTH + 1];
    uint32_t Port;
}
rmqsBrokerMetadata_t;
//---------------------------------------------------------------------------
typedef struct
{
    char_t Stream[RMQS_MAX_STREAM_NAME_LENGTH + 1];
    uint16_t Code;
    uint16_t LeaderReference;
    uint32_t ReplicasReferenceCount;
    uint16_t *ReplicasReferences;
}
rmqsStreamMetadata_t;
//---------------------------------------------------------------------------
typedef struct
{
    uint32_t BrokerMetadataCount;
    rmqsBrokerMetadata_t *BrokerMetadataList;
    uint32_t StreamMetadataCount;
    rmqsStreamMetadata_t *StreamMetadataList;
}
rmqsMetadata_t;
//---------------------------------------------------------------------------
typedef void (*MetadataUpdateCallback_t)(uint16_t Code, char_t *Stream);
//---------------------------------------------------------------------------
typedef struct
{
    rmqsClientConfiguration_t *ClientConfiguration;
    uint32_t ClientMaxFrameSize;
    uint32_t ClientHeartbeat;
    rmqsClientType_t ClientType;
    void *ParentObject; // Publisher or consumer
    uint32_t CorrelationId;
    rmqsResponse_t Response;
    rmqsBuffer_t *TxQueue;
    rmqsBuffer_t *RxQueue;
    char_t RxSocketBuffer[RMQS_CLIENT_RX_BUFFER_SIZE];
    MetadataUpdateCallback_t MetadataUpdateCallback;
}
rmqsClient_t;
//---------------------------------------------------------------------------
rmqsClientFunc rmqsClient_t * rmqsClientCreate(rmqsClientConfiguration_t *ClientConfiguration, rmqsClientType_t ClientType, void *ParentObject, MetadataUpdateCallback_t MetadataUpdateCallback);
rmqsClientFunc void rmqsClientDestroy(rmqsClient_t *Client);
//---------------------------------------------------------------------------
rmqsClientFunc bool_t rmqsClientLogin(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *VirtualHost, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsClientLogout(rmqsClient_t *Client, rmqsSocket_t Socket, uint16_t ClosingCode, char_t *ClosingReason, rmqsResponseCode_t *ResponseCode);
//---------------------------------------------------------------------------
rmqsClientFunc bool_t rmqsPeerProperties(rmqsClient_t *Client, rmqsSocket_t Socket, rmqsProperty_t *Properties, size_t PropertyCount, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsSaslHandshake(rmqsClient_t *Client, rmqsSocket_t Socket, bool_t *PlainAuthSupported, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsSaslAuthenticate(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Mechanism, char_t *Username, char_t *Password, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsOpen(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *VirtualHost, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsClose(rmqsClient_t *Client, rmqsSocket_t Socket, uint16_t ClosingCode, char_t *ClosingReason, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsCreate(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Stream, rmqsCreateStreamArgs_t *CreateStreamArgs, bool_t *StreamAlreadyExists, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsDelete(rmqsClient_t *Client, rmqsSocket_t Socket, char_t *Stream, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc bool_t rmqsMetadata(rmqsClient_t *Client, rmqsSocket_t Socket, char_t **Streams, size_t StreamCount, rmqsMetadata_t *Metadata, rmqsResponseCode_t *ResponseCode);
rmqsClientFunc void rmqsHeartbeat(rmqsClient_t *Client, rmqsSocket_t Socket);
//---------------------------------------------------------------------------
rmqsClientFunc void rmqsHandleMetadataUpdate(rmqsClient_t *Client, rmqsBuffer_t *Buffer);
//---------------------------------------------------------------------------
rmqsClientFunc rmqsMetadata_t *rmqsMetadataCreate(void);
rmqsClientFunc void rmqsMetadataClear(rmqsMetadata_t *Metadata);
rmqsClientFunc void rmqsMetadataDestroy(rmqsMetadata_t *Metadata);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
