//---------------------------------------------------------------------------
#ifndef rmqsBufferH
#define rmqsBufferH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
typedef struct
{
    uint32_t ChunkSize;
    uint32_t Size;
    uint32_t Position;
    uint32_t CurrentMemorySize;
    void *Data;
    //
    // Extra custom properties of the object, freely usable by the application
    //
    uint32_t Tag1;
    uint32_t Tag2;
    uint32_t Tag3;
}
rmqsBuffer_t;
//---------------------------------------------------------------------------
rmqsBuffer_t * rmqsBufferCreate(void);
void rmqsBufferDestroy(rmqsBuffer_t *Buffer);
void rmqsBufferClear(rmqsBuffer_t *Buffer, const bool_t ResetMemoryBuffer);
void rmqsBufferClearTags(rmqsBuffer_t *Buffer);
size_t rmqsBufferRead(rmqsBuffer_t *Buffer, void *ReadBuffer, size_t NoOfBytes);
size_t rmqsBufferWrite(rmqsBuffer_t *Buffer, void *WriteBuffer, size_t NoOfBytes);
void rmqsBufferDelete(rmqsBuffer_t *Buffer, size_t NoOfBytes);
void rmqsBufferMoveTo(rmqsBuffer_t *Buffer, size_t Position);
void rmqsBufferSetMemorySize(rmqsBuffer_t *Buffer, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
