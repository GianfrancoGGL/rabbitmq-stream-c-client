//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsMemBuffer.h"
#include "rmqsGlobal.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMemBuffer_t * rmqsMemBufferCreate(void)
{
    rmqsMemBuffer_t *Stream = (rmqsMemBuffer_t *)rmqsAllocateMemory(sizeof(rmqsMemBuffer_t));

    memset(Stream, 0, sizeof(rmqsMemBuffer_t));

    Stream->ChunkSize = 1024;

    return Stream;
}
//---------------------------------------------------------------------------
void rmqsMemBufferDestroy(rmqsMemBuffer_t *Stream)
{
    if (Stream->Data)
    {
        rmqsFreeMemory(Stream->Data);
    }

    rmqsFreeMemory((void *)Stream);
}
//---------------------------------------------------------------------------
void rmqsMemBufferClear(rmqsMemBuffer_t *Stream, const uint8_t ResetMemoryBuffer)
{
    Stream->Size = 0;
    Stream->Position = 0;

    if (ResetMemoryBuffer && Stream->Data)
    {
        rmqsFreeMemory(Stream->Data);

        Stream->Data = 0;
        Stream->CurrentMemorySize = 0;
    }
}
//---------------------------------------------------------------------------
size_t rmqsMemBufferRead(rmqsMemBuffer_t *Stream, void *ReadBuffer, size_t NrOfBytes)
{
    char_t *p;

    if (Stream->Position >= Stream->Size || NrOfBytes == 0)
    {
        return 0;
    }

    if (Stream->Position + NrOfBytes > Stream->Size)
    {
        NrOfBytes -= (Stream->Size - Stream->Position);
    }

    p = (char_t *)Stream->Data + Stream->Position;

    memcpy(ReadBuffer, (void *)p, NrOfBytes);

    Stream->Position += NrOfBytes;

    return NrOfBytes;
}
//---------------------------------------------------------------------------
size_t rmqsMemBufferWrite(rmqsMemBuffer_t *Stream, void *WriteBuffer, const size_t NrOfBytes)
{
    char_t *p;

    if (NrOfBytes == 0)
    {
        return 0;
    }

    rmqsMemBufferSetMemorySize(Stream, Stream->Position + NrOfBytes);

    p = (char_t *)Stream->Data + Stream->Position;

    memcpy(p, WriteBuffer, NrOfBytes);

    if (Stream->Position + NrOfBytes > Stream->Size)
    {
        Stream->Size = Stream->Position + NrOfBytes;
    }

    Stream->Position += NrOfBytes;

    return Stream->Position;
}
//---------------------------------------------------------------------------
void rmqsMemBufferMoveTo(rmqsMemBuffer_t *Stream, size_t Position)
{
    if (Position >= Stream->Size)
    {
        Position = Stream->Size;
    }

    Stream->Position = Position;
}
//---------------------------------------------------------------------------
void rmqsMemBufferSetMemorySize(rmqsMemBuffer_t *Stream, const size_t RequiredMemorySize)
{
    size_t MemoryToAllocateSize;

    if (RequiredMemorySize == 0 && Stream->Data != 0)
    {
        rmqsFreeMemory(Stream->Data);

        Stream->Data = 0;
        Stream->CurrentMemorySize = 0;

        return;
    }

    MemoryToAllocateSize = (((RequiredMemorySize - 1) / Stream->ChunkSize) + 1) * Stream->ChunkSize;

    if (Stream->Data != 0 && MemoryToAllocateSize <= Stream->CurrentMemorySize)
    {
        return;
    }

    if (! Stream->Data)
    {
        Stream->Data = rmqsAllocateMemory(MemoryToAllocateSize);
    }
    else
    {
        Stream->Data = rmqsRellocateMemory(Stream->Data, MemoryToAllocateSize);
    }

    Stream->CurrentMemorySize = MemoryToAllocateSize;
}
//---------------------------------------------------------------------------

