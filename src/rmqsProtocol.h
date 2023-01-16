//---------------------------------------------------------------------------
#ifndef rmqsProtocolH
#define rmqsProtocolH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsMemBuffer.h"
//---------------------------------------------------------------------------
#define SwapUInt16(x) ((uint16_t)(x >> 8) & (uint16_t)0x00FF) | ((uint16_t)(x << 8) & (uint16_t)0xFF00)
#define SwapUInt32(x) ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | ((x << 24) & 0xFF000000)
//---------------------------------------------------------------------------
typedef enum
{
    rmqscDeclarePublisher = 0x01,
    rmqscPublish = 0x02,
    rmqscPublishConfirm = 0x03,
    rmqscPublishError = 0x04,
    rmqscQueryPublisherSequence = 0x05,
    rmqscDeletePublisher = 0x06,
    rmqscSubscribe = 0x07,
    rmqscDeliver = 0x08,
    rmqscCredit = 0x09,
    rmqscStoreOffset = 0x0A,
    rmqscQueryOffset = 0x0B,
    rmqscUnsubscribe = 0x0C,
    rmqscCreate = 0x0D,
    rmqscDelete = 0x0E,
    rmqscMetadata = 0x0F,
    rmqscMetadataUpdate = 0x10,
    rmqscPeerProperties = 0x11,
    rmqscSaslHandshake = 0x12,
    rmqscSaslAuthenticate = 0x13,
    rmqscTune = 0x14,
    rmqscOpen = 0x15,
    rmqscClose = 0x16,
    rmqscHeartbeat = 0x17,
    rmqscRoute = 0x18,
    rmqscPartitions = 0x19,
    rmqscConsumerUpdate = 0x1A,
    rmqscExchangeCommandVersions = 0x1B,
    rmqscStreamStats = 0x1C
}
rmqsCommand;
//---------------------------------------------------------------------------
typedef enum
{
    rmqsrNoReply = 0x00,
    rmqsrOK = 0x01,
    rmqsrStreamDoesNotExist = 0x02,
    rmqsrSubscriptionIDAlreadyExists = 0x03,
    rmqsrSubscriptionIDDoesNotExist = 0x04,
    rmqsrStreamAlreadyExists = 0x05,
    rmqsrStreamNotAvailable = 0x06,
    rmqsrSASLMechanismNotSupported = 0x07,
    rmqsrAuthenticationFailure = 0x08,
    rmqsrSASLError = 0x09,
    rmqsrSASLChallenge = 0x0A,
    rmqsrSASLAuthenticationFailureLoopback = 0x0B,
    rmqsrVirtualHostAccessFailure = 0x0C,
    rmqsrUnknownFrame = 0x0D,
    rmqsrFrameTooLarge = 0x0E,
    rmqsrInternalError = 0x0F,
    rmqsrAccessRefused = 0x10,
    rmqsrPreconditionFailed = 0x11,
    rmqsrPublisherDoesNotExist = 0x12,
    rmqsrNoOffset = 0x13,
    rmqsrWrongReply = 0xFF
}
rmqsResponseCode;
//---------------------------------------------------------------------------
typedef uint32_t rmqsSize_t;
typedef uint16_t rmqsKey_t;
typedef uint16_t rmqsVersion_t;
typedef uint32_t rmqsCorrelationId_t;
typedef uint8_t rmqsPublisherId_t;
typedef int16_t rmqsStringLen_t;
typedef int32_t rmqsDataLen_t;
//---------------------------------------------------------------------------
#define RMQS_MAX_KEY_SIZE        64
#define RMQS_MAX_VALUE_SIZE     128
#define RMQS_PLAIN_PROTOCOL  "PLAIN"
//---------------------------------------------------------------------------
typedef struct
{
    char_t Key[RMQS_MAX_KEY_SIZE + 1]; // + 1 for the null terminator
    char_t Value[RMQS_MAX_VALUE_SIZE + 1]; // + 1 for the null terminator
}
rmqsProperty_t;
//---------------------------------------------------------------------------
#pragma pack(push,1)
//---------------------------------------------------------------------------
typedef struct
{
    rmqsSize_t Size;
    rmqsKey_t Key;
    rmqsVersion_t Version;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
}
rmqsResponse_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsSize_t Size;
    rmqsKey_t Key;
    rmqsVersion_t Version;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
    uint16_t Unknown;
    uint16_t NoOfMechanisms;
}
rmqsResponseHandshakeRequest_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsSize_t Size;
    rmqsKey_t Key;
    rmqsVersion_t Version;
    uint32_t FrameMax;
    uint32_t Heartbeat;
}
rmqsTuneRequest_t;
//---------------------------------------------------------------------------
#pragma pack(pop)
//---------------------------------------------------------------------------
uint8_t rmqsIsLittleEndianMachine(void);
//---------------------------------------------------------------------------
void rmqsSendMessage(const void *Environment, const rmqsSocket Socket, const char_t *Data, size_t DataSize);
uint8_t rmqsWaitMessage(const void *Environment, const rmqsSocket Socket, char_t *RxBuffer, size_t RxBufferSize, rmqsMemBuffer_t *RxStream, rmqsMemBuffer_t *RxStreamTempBuffer, const uint32_t RxTimeout);
//---------------------------------------------------------------------------
size_t rmqsAddInt8ToStream(rmqsMemBuffer_t *Stream, int8_t Value);
size_t rmqsAddUInt8ToStream(rmqsMemBuffer_t *Stream, uint8_t Value);
size_t rmqsAddInt16ToStream(rmqsMemBuffer_t *Stream, int16_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddUInt16ToStream(rmqsMemBuffer_t *Stream, uint16_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddInt32ToStream(rmqsMemBuffer_t *Stream, int32_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddUInt32ToStream(rmqsMemBuffer_t *Stream, uint32_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddStringToStream(rmqsMemBuffer_t *Stream, const char_t *Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddBytesToStream(rmqsMemBuffer_t *Stream, void *Value, size_t ValueLength, uint8_t IsLittleEndianMachine);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
