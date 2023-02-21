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
#ifndef rmqsBufferH
#define rmqsBufferH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
/** \brief Holds a memory stream in which information can be written in append mode and that
 *  dynamically allocates the memory necessary to contain it. Memory is allocated in chunks of customizable size
 *  to reduce the number of allocations needed
 *
 * \param ChunkSize The initial amount of memory that is allocated when the structure is created or when new memory blocks need
 * to be allocated
 * \param Size The size of the data written to the buffer
 * \param Position The current position within the buffer. If the buffer is used only in append mode, the position corresponds to
 * the last byte written, but it is possible to reposition itself in any point of the memory buffer and overwrite information already stored
 * \param CurrentMemorySize The amount of memory currently allocated. Should not be confused with the "Size" field, because
 * the memory is allocated in chunks, therefore it is possible to have only one character written in the buffer and therefore
 * Size=1 and the value of this field would be equal to ChunkSize
 * \param Data Pointer to the data written to the buffer
 * \param Tag1 Field in which it is possible to store additional information regarding the content of the data structure.
 * \param Tag2 Additional custom field
 * \param Tag3 Additional custom field
 */
typedef struct
{
    uint32_t ChunkSize;
    uint32_t Size;
    uint32_t Position;
    uint32_t CurrentMemorySize;
    void *Data;
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
