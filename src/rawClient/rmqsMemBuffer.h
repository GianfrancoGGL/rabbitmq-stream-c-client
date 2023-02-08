//---------------------------------------------------------------------------
#ifndef rmqsMemBufferH
#define rmqsMemBufferH
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
}
rmqsMemBuffer_t;
//---------------------------------------------------------------------------
rmqsMemBuffer_t * rmqsMemBufferCreate(void);
void rmqsMemBufferDestroy(rmqsMemBuffer_t *MemBuffer);
void rmqsMemBufferClear(rmqsMemBuffer_t *MemBuffer, const bool_t ResetMemoryBuffer);
size_t rmqsMemBufferRead(rmqsMemBuffer_t *MemBuffer, void *ReadBuffer, size_t NoOfBytes);
size_t rmqsMemBufferWrite(rmqsMemBuffer_t *MemBuffer, void *WriteBuffer, size_t NoOfBytes);
void rmqsMemBufferDelete(rmqsMemBuffer_t *MemBuffer, size_t NoOfBytes);
void rmqsMemBufferMoveTo(rmqsMemBuffer_t *MemBuffer, size_t Position);
void rmqsMemBufferSetMemorySize(rmqsMemBuffer_t *MemBuffer, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
