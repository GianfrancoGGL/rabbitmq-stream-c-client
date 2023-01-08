//---------------------------------------------------------------------------
#ifndef rmqsProtocolH
#define rmqsProtocolH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsStream.h"
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
    rmqsrNoOffset = 0x13
}
rmqsResponseCode;
//---------------------------------------------------------------------------
typedef uint32_t rmqsSize;
typedef uint16_t rmqsKey;
typedef uint16_t rmqsVersion;
typedef uint32_t rmqsCorrelationId;
typedef uint8_t rmqsPublisherId;
typedef int16_t rmqsStringLen;
//---------------------------------------------------------------------------
#define RMQS_MAX_KEY_SIZE     64
#define RMQS_MAX_VALUE_SIZE  128
//---------------------------------------------------------------------------
typedef struct
{
    char Key[RMQS_MAX_KEY_SIZE + 1]; // + 1 for the null terminator
    char Value[RMQS_MAX_VALUE_SIZE + 1]; // + 1 for the null terminator
}
rmqsProperty;
//---------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
    uint32_t Size;
    uint16_t Key;
    uint16_t Version;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
}
rmqsResponse;
#pragma pack(pop)
//---------------------------------------------------------------------------
uint8_t rmqsIsLittleEndianMachine(void);
//---------------------------------------------------------------------------
rmqsResponseCode rmqsPeerPropertiesRequest(const void *Producer, rmqsCorrelationId CorrelationId, uint32_t PropertiesCount, rmqsProperty *Properties);
size_t rmqsAddInt8ToStream(rmqsStream *Stream, const int8_t Value);
size_t rmqsAddUInt8ToStream(rmqsStream *Stream, const uint8_t Value);
size_t rmqsAddInt16ToStream(rmqsStream *Stream, const int16_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddUInt16ToStream(rmqsStream *Stream, const uint16_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddInt32ToStream(rmqsStream *Stream, const int32_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddUInt32ToStream(rmqsStream *Stream, const uint32_t Value, uint8_t IsLittleEndianMachine);
size_t rmqsAddStringToStream(rmqsStream *Stream, const char *Value, uint8_t IsLittleEndianMachine);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
