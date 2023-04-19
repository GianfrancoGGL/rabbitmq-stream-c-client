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
#ifndef rmqsProtocolH
#define rmqsProtocolH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsNetwork.h"
#include "rmqsBuffer.h"
//---------------------------------------------------------------------------
#define SwapUInt16(x) ((uint16_t)(x >> 8) & (uint16_t)0x00FF) | ((uint16_t)(x << 8) & (uint16_t)0xFF00)
#define SwapUInt32(x) ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | ((x << 24) & 0xFF000000)
#define SwapUInt64(x) ((x >> 56) & 0x00000000000000FFULL) | \
                      ((x >> 40) & 0x000000000000FF00ULL) | \
                      ((x >> 24) & 0x0000000000FF0000ULL) | \
                      ((x >>  8) & 0x00000000FF000000ULL) | \
                      ((x <<  8) & 0x000000FF00000000ULL) | \
                      ((x << 24) & 0x0000FF0000000000ULL) | \
                      ((x << 40) & 0x00FF000000000000ULL) | \
                      ((x << 56) & 0xFF00000000000000ULL)
//---------------------------------------------------------------------------
#define RMQS_NULL_STRING_LENGTH    -1
#define RMQS_EMPTY_DATA_LENGTH     -1
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
rmqsCommand_t;
//---------------------------------------------------------------------------
#define rmqsCommandHasCorrelationId(x) (x != rmqscTune && x != rmqscPublishConfirm && x != rmqscPublishError && x != rmqscDeliver)
//---------------------------------------------------------------------------
typedef enum
{
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
    rmqsrConnectionError = 0xFF
}
rmqsResponseCode_t;
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
    uint32_t Size;
    uint16_t Key;
    uint16_t Version;
}
rmqsMsgHeader_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsMsgHeader_t Header;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
}
rmqsResponse_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsMsgHeader_t Header;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
    uint16_t Unknown;
    uint16_t NoOfMechanisms;
}
rmqsSaslHandshakeResponse_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsMsgHeader_t Header;
    uint32_t FrameMax;
    uint32_t Heartbeat;
}
rmqsTuneRequest_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsMsgHeader_t Header;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
    uint64_t Sequence;
}
rmqsQueryPublisherResponse_t;
//---------------------------------------------------------------------------
typedef struct
{
    rmqsMsgHeader_t Header;
    uint32_t CorrelationId;
    uint16_t ResponseCode;
    uint64_t Offset;
}
rmqsQueryOffsetResponse_t;
//---------------------------------------------------------------------------
#pragma pack(pop)
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsIsLittleEndianMachine(void);
//---------------------------------------------------------------------------
rmqsProtoFunc bool_t rmqsSendMessage(void *Client, rmqsSocket_t Socket, char_t *Data, size_t DataSize);
rmqsProtoFunc bool_t rmqsWaitMessage(void *Client, rmqsSocket_t Socket, uint32_t RxTimeout, bool_t *ConnectionError);
rmqsProtoFunc bool_t rmqsWaitResponse(void *Client, rmqsSocket_t Socket, uint32_t CorrelationId, rmqsResponse_t *Response, uint32_t RxTimeout, bool_t *ConnectionError);
rmqsProtoFunc void rmqsDequeueMessageFromBuffer(rmqsBuffer_t *Buffer, size_t MessageSize);
rmqsProtoFunc char_t * rmqsGetCommandDescription(uint16_t Key);
rmqsProtoFunc char_t * rmqsGetResponseCodeDescription(uint16_t ResponseCode);
//---------------------------------------------------------------------------
rmqsProtoFunc size_t rmqsAddInt8ToBuffer(rmqsBuffer_t *Buffer, int8_t Value);
rmqsProtoFunc size_t rmqsAddUInt8ToBuffer(rmqsBuffer_t *Buffer, uint8_t Value);
rmqsProtoFunc size_t rmqsAddInt16ToBuffer(rmqsBuffer_t *Buffer, int16_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddUInt16ToBuffer(rmqsBuffer_t *Buffer, uint16_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddInt32ToBuffer(rmqsBuffer_t *Buffer, int32_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddUInt32ToBuffer(rmqsBuffer_t *Buffer, uint32_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddInt64ToBuffer(rmqsBuffer_t *Buffer, int64_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddUInt64ToBuffer(rmqsBuffer_t *Buffer, uint64_t Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddStringToBuffer(rmqsBuffer_t *Buffer, char_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc size_t rmqsAddBytesToBuffer(rmqsBuffer_t *Buffer, void *Value, size_t ValueLength, bool_t DeclareLength, bool_t IsLittleEndianMachine);
//---------------------------------------------------------------------------
rmqsProtoFunc void rmqsGetInt8FromMemory(char_t **Pointer, int8_t *Value);
rmqsProtoFunc void rmqsGetUInt8FromMemory(char_t **Pointer, uint8_t *Value);
rmqsProtoFunc void rmqsGetInt16FromMemory(char_t **Pointer, int16_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc void rmqsGetUInt16FromMemory(char_t **Pointer, uint16_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc void rmqsGetInt32FromMemory(char_t **Pointer, int32_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc void rmqsGetUInt32FromMemory(char_t **Pointer, uint32_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc void rmqsGetInt64FromMemory(char_t **Pointer, int64_t *Value, bool_t IsLittleEndianMachine);
rmqsProtoFunc void rmqsGetUInt64FromMemory(char_t **Pointer, uint64_t *Value, bool_t IsLittleEndianMachine);
void rmqsGetStringFromMemory(char_t **Pointer, char_t *Value, size_t MaxValueLength, bool_t IsLittleEndianMachine);//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
