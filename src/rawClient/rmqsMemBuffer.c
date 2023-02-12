//---------------------------------------------------------------------------
#include <memory.h>
//---------------------------------------------------------------------------
#include "rmqsMemBuffer.h"
#include "rmqsGlobal.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMemBuffer_t * rmqsMemBufferCreate(void)
{
    rmqsMemBuffer_t *MemBuffer = (rmqsMemBuffer_t *)rmqsAllocateMemory(sizeof(rmqsMemBuffer_t));

    memset(MemBuffer, 0, sizeof(rmqsMemBuffer_t));

    MemBuffer->ChunkSize = 10240000;

    return MemBuffer;
}
//---------------------------------------------------------------------------
void rmqsMemBufferDestroy(rmqsMemBuffer_t *MemBuffer)
{
    if (MemBuffer->Data)
    {
        rmqsFreeMemory(MemBuffer->Data);
    }

    rmqsFreeMemory((void *)MemBuffer);
}
//---------------------------------------------------------------------------
void rmqsMemBufferClear(rmqsMemBuffer_t *MemBuffer, const bool_t ResetMemoryBuffer)
{
    MemBuffer->Size = 0;
    MemBuffer->Position = 0;

    if (ResetMemoryBuffer && MemBuffer->Data)
    {
        rmqsFreeMemory(MemBuffer->Data);

        MemBuffer->Data = 0;
        MemBuffer->CurrentMemorySize = 0;
    }
}
//---------------------------------------------------------------------------
void rmqsMemBufferClearTags(rmqsMemBuffer_t *MemBuffer)
{
    MemBuffer->Tag1 = 0;
    MemBuffer->Tag2 = 0;
    MemBuffer->Tag3 = 0;
}
//---------------------------------------------------------------------------
size_t rmqsMemBufferRead(rmqsMemBuffer_t *MemBuffer, void *ReadBuffer, size_t NoOfBytes)
{
    char_t *p;

    if (MemBuffer->Position >= MemBuffer->Size || NoOfBytes == 0)
    {
        return 0;
    }

    if (MemBuffer->Position + NoOfBytes > MemBuffer->Size)
    {
        NoOfBytes -= (MemBuffer->Size - MemBuffer->Position);
    }

    p = (char_t *)MemBuffer->Data + MemBuffer->Position;

    memcpy(ReadBuffer, (void *)p, NoOfBytes);

    MemBuffer->Position += NoOfBytes;

    return NoOfBytes;
}
//---------------------------------------------------------------------------
size_t rmqsMemBufferWrite(rmqsMemBuffer_t *MemBuffer, void *WriteBuffer, const size_t NoOfBytes)
{
    char_t *p;

    if (NoOfBytes == 0)
    {
        return 0;
    }

    rmqsMemBufferSetMemorySize(MemBuffer, MemBuffer->Position + NoOfBytes);

    p = (char_t *)MemBuffer->Data + MemBuffer->Position;

    memcpy(p, WriteBuffer, NoOfBytes);

    if (MemBuffer->Position + NoOfBytes > MemBuffer->Size)
    {
        MemBuffer->Size = MemBuffer->Position + NoOfBytes;
    }

    MemBuffer->Position += NoOfBytes;

    return MemBuffer->Position;
}
//---------------------------------------------------------------------------
void rmqsMemBufferDelete(rmqsMemBuffer_t *MemBuffer, size_t NoOfBytes)
{
    if (NoOfBytes > MemBuffer->Size)
    {
        NoOfBytes = MemBuffer->Size;
    }

    memmove(MemBuffer->Data, (char_t *)MemBuffer->Data + NoOfBytes, MemBuffer->Size - NoOfBytes);

    MemBuffer->Size -= NoOfBytes;

    if (MemBuffer->Position >= NoOfBytes)
    {
        MemBuffer->Position -= NoOfBytes;
    }
    else
    {
        MemBuffer->Position = 0;
    }
}
//---------------------------------------------------------------------------
void rmqsMemBufferMoveTo(rmqsMemBuffer_t *MemBuffer, size_t Position)
{
    if (Position >= MemBuffer->Size)
    {
        Position = MemBuffer->Size;
    }

    MemBuffer->Position = Position;
}
//---------------------------------------------------------------------------
void rmqsMemBufferSetMemorySize(rmqsMemBuffer_t *MemBuffer, const size_t RequiredMemorySize)
{
    size_t MemoryToAllocateSize;

    if (RequiredMemorySize == 0 && MemBuffer->Data != 0)
    {
        rmqsFreeMemory(MemBuffer->Data);

        MemBuffer->Data = 0;
        MemBuffer->CurrentMemorySize = 0;

        return;
    }

    MemoryToAllocateSize = (((RequiredMemorySize - 1) / MemBuffer->ChunkSize) + 1) * MemBuffer->ChunkSize;

    if (MemBuffer->Data != 0 && MemoryToAllocateSize <= MemBuffer->CurrentMemorySize)
    {
        return;
    }

    if (! MemBuffer->Data)
    {
        MemBuffer->Data = rmqsAllocateMemory(MemoryToAllocateSize);
    }
    else
    {
        MemBuffer->Data = rmqsRellocateMemory(MemBuffer->Data, MemoryToAllocateSize);
    }

    MemBuffer->CurrentMemorySize = MemoryToAllocateSize;
}
//---------------------------------------------------------------------------
