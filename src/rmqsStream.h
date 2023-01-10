//---------------------------------------------------------------------------
#ifndef rmqsStreamH
#define rmqsStreamH
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
rmqsStream_t;
//---------------------------------------------------------------------------
rmqsStream_t * rmqsStreamCreate(void);
void rmqsStreamDestroy(rmqsStream_t *Stream);
void rmqsStreamClear(rmqsStream_t *Stream, const uint8_t ResetMemoryBuffer);
size_t rmqsStreamRead(rmqsStream_t *Stream, void *ReadBuffer, size_t NrOfBytes);
size_t rmqsStreamWrite(rmqsStream_t *Stream, void *WriteBuffer, size_t NrOfBytes);
void rmqsStreamMoveTo(rmqsStream_t *Stream, size_t Position);
void rmqsStreamSetMemorySize(rmqsStream_t *Stream, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
