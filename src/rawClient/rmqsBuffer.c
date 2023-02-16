//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsBuffer.h"
#include "rmqsGlobal.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsBuffer_t * rmqsBufferCreate(void)
{
    rmqsBuffer_t *Buffer = (rmqsBuffer_t *)rmqsAllocateMemory(sizeof(rmqsBuffer_t));

    memset(Buffer, 0, sizeof(rmqsBuffer_t));

    Buffer->ChunkSize = 10240000;

    return Buffer;
}
//---------------------------------------------------------------------------
void rmqsBufferDestroy(rmqsBuffer_t *Buffer)
{
    if (Buffer->Data)
    {
        rmqsFreeMemory(Buffer->Data);
    }

    rmqsFreeMemory((void *)Buffer);
}
//---------------------------------------------------------------------------
void rmqsBufferClear(rmqsBuffer_t *Buffer, const bool_t ResetMemoryBuffer)
{
    Buffer->Size = 0;
    Buffer->Position = 0;

    if (ResetMemoryBuffer && Buffer->Data)
    {
        rmqsFreeMemory(Buffer->Data);

        Buffer->Data = 0;
        Buffer->CurrentMemorySize = 0;
    }
}
//---------------------------------------------------------------------------
void rmqsBufferClearTags(rmqsBuffer_t *Buffer)
{
    Buffer->Tag1 = 0;
    Buffer->Tag2 = 0;
    Buffer->Tag3 = 0;
}
//---------------------------------------------------------------------------
size_t rmqsBufferRead(rmqsBuffer_t *Buffer, void *ReadBuffer, size_t NoOfBytes)
{
    char_t *p;

    if (Buffer->Position >= Buffer->Size || NoOfBytes == 0)
    {
        return 0;
    }

    if (Buffer->Position + NoOfBytes > Buffer->Size)
    {
        NoOfBytes -= (Buffer->Size - Buffer->Position);
    }

    p = (char_t *)Buffer->Data + Buffer->Position;

    memcpy(ReadBuffer, (void *)p, NoOfBytes);

    Buffer->Position += NoOfBytes;

    return NoOfBytes;
}
//---------------------------------------------------------------------------
size_t rmqsBufferWrite(rmqsBuffer_t *Buffer, void *WriteBuffer, const size_t NoOfBytes)
{
    char_t *p;

    if (NoOfBytes == 0)
    {
        return 0;
    }

    rmqsBufferSetMemorySize(Buffer, Buffer->Position + NoOfBytes);

    p = (char_t *)Buffer->Data + Buffer->Position;

    memcpy(p, WriteBuffer, NoOfBytes);

    if (Buffer->Position + NoOfBytes > Buffer->Size)
    {
        Buffer->Size = Buffer->Position + NoOfBytes;
    }

    Buffer->Position += NoOfBytes;

    return Buffer->Position;
}
//---------------------------------------------------------------------------
void rmqsBufferDelete(rmqsBuffer_t *Buffer, size_t NoOfBytes)
{
    if (NoOfBytes > Buffer->Size)
    {
        NoOfBytes = Buffer->Size;
    }

    memmove(Buffer->Data, (char_t *)Buffer->Data + NoOfBytes, Buffer->Size - NoOfBytes);

    Buffer->Size -= NoOfBytes;

    if (Buffer->Position >= NoOfBytes)
    {
        Buffer->Position -= NoOfBytes;
    }
    else
    {
        Buffer->Position = 0;
    }
}
//---------------------------------------------------------------------------
void rmqsBufferMoveTo(rmqsBuffer_t *Buffer, size_t Position)
{
    if (Position >= Buffer->Size)
    {
        Position = Buffer->Size;
    }

    Buffer->Position = Position;
}
//---------------------------------------------------------------------------
void rmqsBufferSetMemorySize(rmqsBuffer_t *Buffer, const size_t RequiredMemorySize)
{
    size_t MemoryToAllocateSize;

    if (RequiredMemorySize == 0 && Buffer->Data != 0)
    {
        rmqsFreeMemory(Buffer->Data);

        Buffer->Data = 0;
        Buffer->CurrentMemorySize = 0;

        return;
    }

    MemoryToAllocateSize = (((RequiredMemorySize - 1) / Buffer->ChunkSize) + 1) * Buffer->ChunkSize;

    if (Buffer->Data != 0 && MemoryToAllocateSize <= Buffer->CurrentMemorySize)
    {
        return;
    }

    if (! Buffer->Data)
    {
        Buffer->Data = rmqsAllocateMemory(MemoryToAllocateSize);
    }
    else
    {
        Buffer->Data = rmqsRellocateMemory(Buffer->Data, MemoryToAllocateSize);
    }

    Buffer->CurrentMemorySize = MemoryToAllocateSize;
}
//---------------------------------------------------------------------------
