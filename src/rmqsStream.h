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
rmqsStream;
//---------------------------------------------------------------------------
rmqsStream * rmqsStreamCreate(void);
void rmqsStreamDestroy(rmqsStream *Stream);
void rmqsStreamClear(rmqsStream *Stream, const uint8_t ResetMemoryBuffer);
size_t rmqsStreamRead(rmqsStream *Stream, void *ReadBuffer, const size_t NrOfBytes);
size_t rmqsStreamWrite(rmqsStream *Stream, void *WriteBuffer, const size_t NrOfBytes);
void rmqsStreamMoveTo(rmqsStream *Stream, size_t Position);
void rmqsStreamSetMemorySize(rmqsStream *Stream, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
