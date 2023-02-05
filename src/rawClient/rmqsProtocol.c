//---------------------------------------------------------------------------
#include <string.h>
//---------------------------------------------------------------------------
#include "rmqsClientConfiguration.h"
#include "rmqsMemory.h"
#include "rmqsProtocol.h"
//---------------------------------------------------------------------------
#define RMQS_NULL_STRING_LENGTH    -1
#define RMQS_EMPTY_DATA_LENGTH     -1
//---------------------------------------------------------------------------
bool_t rmqsIsLittleEndianMachine(void)
{
    union
    {
        uint32_t i;
        char_t c[4];
    }
    bint = {0x01020304};

    if (bint.c[0] == 1)
    {
       return 0;
    }
    else
    {
       return 1;
    }
}
//---------------------------------------------------------------------------
void rmqsSendMessage(const rmqsClientConfiguration_t *ClientConfiguration, const rmqsSocket Socket, const char_t *Data, size_t DataSize)
{
    if (ClientConfiguration->Logger)
    {
        rmqsLoggerRegisterDump(ClientConfiguration->Logger, (void *)Data, DataSize, "TX", 0);
    }

    send(Socket, (const char_t *)Data, DataSize, 0);
}
//---------------------------------------------------------------------------
bool_t rmqsWaitMessage(const rmqsClientConfiguration_t *ClientConfiguration, const rmqsSocket Socket, char_t *RxBuffer, const size_t RxBufferSize, rmqsMemBuffer_t *RxStream, rmqsMemBuffer_t *RxStreamTempBuffer, const uint32_t RxTimeout)
{
    bool_t MessageReceived = 0;
    int32_t RxBytes;
    uint32_t MessageSize;

    rmqsMemBufferClear(RxStream, 0);

    //
    // Tries to extract the message from the already received bytes, if not enough, read
    // again from the socket
    //
    while (1)
    {
        //
        // Unprocessed bytes saved in the temp stream are processed now
        //
        if (RxStreamTempBuffer->Size > 0)
        {
            rmqsMemBufferWrite(RxStream, RxStreamTempBuffer->Data, RxStreamTempBuffer->Size);
            rmqsMemBufferClear(RxStreamTempBuffer, 0);
        }

        //
        // Is the message length (4 bytes) arrived?
        //
        if (RxStream->Size >= sizeof(uint32_t))
        {
            //
            // Message length is once stored in the Tag1 field and eventually with the correct endianness
            //
            MessageSize = *(uint32_t *)RxStream->Data;

            if (ClientConfiguration->IsLittleEndianMachine)
            {
                MessageSize = SwapUInt32(MessageSize);
            }

            //
            // The number of bytes to wait is the one stored in the 4 bytes of the length + the length itself
            //
            MessageSize += sizeof(uint32_t);

            if (RxStream->Size >= MessageSize)
            {
                //
                // Message completed!
                //
                MessageReceived = 1;

                if (ClientConfiguration->Logger)
                {
                    rmqsLoggerRegisterDump(ClientConfiguration->Logger, (void *)RxStream->Data, RxStream->Size, "RX", 0);
                }

                //
                // Store the extra bytes in the rx buffer stream
                //
                rmqsMemBufferWrite(RxStreamTempBuffer, (void *)((char_t *)RxStream->Data + MessageSize), RxStream->Size - MessageSize);
            }
        }

        if (MessageReceived)
        {
            break;
        }

        rmqsSetSocketReadTimeouts(Socket, RxTimeout);

        RxBytes = recv(Socket, RxBuffer, RxBufferSize, 0);

        if (RxBytes <= 0)
        {
            break;
        }

        rmqsMemBufferWrite(RxStream, RxBuffer, RxBytes);

        //
        // New loop to extract the message from the receive stream
        //
    }

    return MessageReceived;
}
//---------------------------------------------------------------------------
size_t rmqsAddInt8ToStream(rmqsMemBuffer_t *Stream, int8_t Value)
{
    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt8ToStream(rmqsMemBuffer_t *Stream, uint8_t Value)
{
    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));
    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt16ToStream(rmqsMemBuffer_t *Stream, int16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt16ToStream(rmqsMemBuffer_t *Stream, uint16_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt16(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt32ToStream(rmqsMemBuffer_t *Stream, int32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt32ToStream(rmqsMemBuffer_t *Stream, uint32_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt32(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddInt64ToStream(rmqsMemBuffer_t *Stream, int64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddUInt64ToStream(rmqsMemBuffer_t *Stream, uint64_t Value, bool_t IsLittleEndianMachine)
{
    if (IsLittleEndianMachine)
    {
        Value = SwapUInt64(Value);
    }

    rmqsMemBufferWrite(Stream, (void *)&Value, sizeof(Value));

    return sizeof(Value);
}
//---------------------------------------------------------------------------
size_t rmqsAddStringToStream(rmqsMemBuffer_t *Stream, const char_t *Value, bool_t IsLittleEndianMachine)
{
    int16_t StringLen;
    size_t BytesAdded;

    if (Value == 0 || *Value == 0)
    {
        StringLen = RMQS_NULL_STRING_LENGTH;
    }
    else
    {
        StringLen = (int16_t)strlen(Value);
    }

    rmqsAddInt16ToStream(Stream, StringLen, IsLittleEndianMachine);
    BytesAdded = sizeof(StringLen);

    if (StringLen != RMQS_NULL_STRING_LENGTH)
    {
        rmqsMemBufferWrite(Stream, (void *)Value, StringLen);
        BytesAdded += StringLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------
size_t rmqsAddBytesToStream(rmqsMemBuffer_t *Stream, void *Value, size_t ValueLength, bool_t IsLittleEndianMachine)
{
    int32_t DataLen;
    size_t BytesAdded;

    if (ValueLength == 0)
    {
        DataLen = RMQS_EMPTY_DATA_LENGTH;
    }
    else
    {
        DataLen = (int32_t)ValueLength;
    }

    rmqsAddInt32ToStream(Stream, DataLen, IsLittleEndianMachine);
    BytesAdded = sizeof(DataLen);

    if (DataLen != RMQS_EMPTY_DATA_LENGTH)
    {
        rmqsMemBufferWrite(Stream, (void *)Value, DataLen);
        BytesAdded += DataLen;
    }

    return BytesAdded;
}
//---------------------------------------------------------------------------

