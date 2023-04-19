/****************************************************************************
MIT License

Copyright (c) 2023 Gianfranco Giugliano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
//---------------------------------------------------------------------------
/**
 * @file rmqsBuffer.h
 *
 * Memory streams handling
 */
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
    size_t ChunkSize;
    size_t Size;
    size_t Position;
    size_t CurrentMemorySize;
    void *Data;
    uint32_t Tag1;
    uint32_t Tag2;
    uint32_t Tag3;
}
rmqsBuffer_t;
//---------------------------------------------------------------------------
rmqsBufferingFunc rmqsBuffer_t * rmqsBufferCreate(void);
rmqsBufferingFunc void rmqsBufferDestroy(rmqsBuffer_t *Buffer);
rmqsBufferingFunc void rmqsBufferClear(rmqsBuffer_t *Buffer, bool_t ResetMemoryBuffer);
rmqsBufferingFunc void rmqsBufferClearTags(rmqsBuffer_t *Buffer);
rmqsBufferingFunc size_t rmqsBufferRead(rmqsBuffer_t *Buffer, void *ReadBuffer, size_t NoOfBytes);
rmqsBufferingFunc size_t rmqsBufferWrite(rmqsBuffer_t *Buffer, void *WriteBuffer, size_t NoOfBytes);
rmqsBufferingFunc void rmqsBufferDelete(rmqsBuffer_t *Buffer, size_t NoOfBytes);
rmqsBufferingFunc void rmqsBufferMoveTo(rmqsBuffer_t *Buffer, size_t Position);
rmqsBufferingFunc void rmqsBufferSetMemorySize(rmqsBuffer_t *Buffer, size_t RequiredMemorySize);
#endif
//--------------------------------------------------------------------------
