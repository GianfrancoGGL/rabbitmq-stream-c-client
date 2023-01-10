//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsStream.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsStream_t * rmqsStreamCreate(void)
{
    rmqsStream_t *Stream = (rmqsStream_t *)rmqsAllocateMemory(sizeof(rmqsStream_t));

    memset(Stream, 0, sizeof(rmqsStream_t));

    Stream->ChunkSize = 1024;

    return Stream;
}
//---------------------------------------------------------------------------
void rmqsStreamDestroy(rmqsStream_t *Stream)
{
    if (Stream->Data)
    {
        rmqsFreeMemory(Stream->Data);
    }

    rmqsFreeMemory((void *)Stream);
}
//---------------------------------------------------------------------------
void rmqsStreamClear(rmqsStream_t *Stream, const uint8_t ResetMemoryBuffer)
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
size_t rmqsStreamRead(rmqsStream_t *Stream, void *ReadBuffer, size_t NrOfBytes)
{
    char *p;

    if (Stream->Position >= Stream->Size || NrOfBytes == 0)
    {
        return 0;
    }

    if (Stream->Position + NrOfBytes > Stream->Size)
    {
        NrOfBytes -= (Stream->Size - Stream->Position);
    }

    p = (char *)Stream->Data + Stream->Position;

    memcpy(ReadBuffer, (void *)p, NrOfBytes);

    Stream->Position += NrOfBytes;

    return NrOfBytes;
}
//---------------------------------------------------------------------------
size_t rmqsStreamWrite(rmqsStream_t *Stream, void *WriteBuffer, const size_t NrOfBytes)
{
    char *p;

    if (NrOfBytes == 0)
    {
        return 0;
    }

    rmqsStreamSetMemorySize(Stream, Stream->Position + NrOfBytes);

    p = (char *)Stream->Data + Stream->Position;

    memcpy(p, WriteBuffer, NrOfBytes);

    if (Stream->Position + NrOfBytes > Stream->Size)
    {
        Stream->Size = Stream->Position + NrOfBytes;
    }

    Stream->Position += NrOfBytes;

    return Stream->Position;
}
//---------------------------------------------------------------------------
void rmqsStreamMoveTo(rmqsStream_t *Stream, size_t Position)
{
    if (Position >= Stream->Size)
    {
        Position = Stream->Size;
    }

    Stream->Position = Position;
}
//---------------------------------------------------------------------------
void rmqsStreamSetMemorySize(rmqsStream_t *Stream, const size_t RequiredMemorySize)
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

