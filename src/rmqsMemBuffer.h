//---------------------------------------------------------------------------
#ifndef rmqsMemBufferH
#define rmqsMemBufferH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
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
void rmqsMemBufferDestroy(rmqsMemBuffer_t *Stream);
void rmqsMemBufferClear(rmqsMemBuffer_t *Stream, const uint8_t ResetMemoryBuffer);
size_t rmqsMemBufferRead(rmqsMemBuffer_t *Stream, void *ReadBuffer, size_t NrOfBytes);
size_t rmqsMemBufferWrite(rmqsMemBuffer_t *Stream, void *WriteBuffer, size_t NrOfBytes);
void rmqsMemBufferMoveTo(rmqsMemBuffer_t *Stream, size_t Position);
void rmqsMemBufferSetMemorySize(rmqsMemBuffer_t *Stream, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
