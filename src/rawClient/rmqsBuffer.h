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
/** \brief A memory I/O stream
 *
 * Holds a memory stream in which information can be written in append mode and that
 * dynamically allocates the memory necessary to contain it. Memory is allocated in chunks of customizable size
 * to reduce the number of allocations needed
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
/** \brief Create a new memory stream
 *
 * \return Pointer to the newly created memory stream
 *
 */
rmqsBuffer_t * rmqsBufferCreate(void);
//---------------------------------------------------------------------------
/** \brief Destroys a previously created memory stream
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream to delete
 * \return void
 *
 */
void rmqsBufferDestroy(rmqsBuffer_t *Buffer);
//---------------------------------------------------------------------------
/** \brief Clears the content of a memory stream
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream to clear
 * \param ResetMemoryBuffer bool_t If the allocated memory needs to be freed, set false to avoid a new allocation of memory on write
 * \return void
 *
 */
void rmqsBufferClear(rmqsBuffer_t *Buffer, const bool_t ResetMemoryBuffer);
//---------------------------------------------------------------------------
/** \brief Set the three 'Tag' variables of the stream to zero
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream
 * \return void
 *
 */
void rmqsBufferClearTags(rmqsBuffer_t *Buffer);
//---------------------------------------------------------------------------
/** \brief Reads a certain amount of bytes from the stream, starting at
 *         the current location
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream to read from
 * \param ReadBuffer void* Memory block in which the bytes must be copied
 * \param NoOfBytes size_t How many bytes must be read from the stream
 * \return The amount of bytes read from the stream
 *
 */
size_t rmqsBufferRead(rmqsBuffer_t *Buffer, void *ReadBuffer, size_t NoOfBytes);
//---------------------------------------------------------------------------
/** \brief Write a certain amount of bytes to the stream, starting from
 *         the current location
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream to write to
 * \param WriteBuffer void* Memory block from which the bytes are to be copied
 * \param NoOfBytes size_t How many bytes must be written to the stream
 * \return The amount of bytes written to the stream
 *
 */
size_t rmqsBufferWrite(rmqsBuffer_t *Buffer, void *WriteBuffer, size_t NoOfBytes);
//---------------------------------------------------------------------------
/** \brief Deletes a certain amount of bytes from the stream, starting from
 *         the beginning of the stream
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream
 * \param NoOfBytes size_t How many bytes must be deleted from the stream
 * \return void
 *
 */
void rmqsBufferDelete(rmqsBuffer_t *Buffer, size_t NoOfBytes);
//---------------------------------------------------------------------------
/** \brief Set the current position of the memory stream
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream
 * \param Position size_t New position within the memory stream
 * \return void
 *
 */
void rmqsBufferMoveTo(rmqsBuffer_t *Buffer, size_t Position);
//---------------------------------------------------------------------------
/** \brief Set the size of the allocated memory of the stream.
 *  This function should be called only from the other stream functions.
 *
 * \param Buffer rmqsBuffer_t* Pointer to the memory stream
 * \param RequiredMemorySize size_t Size of the allocated memory
 * \return void
 *
 */
void rmqsBufferSetMemorySize(rmqsBuffer_t *Buffer, const size_t RequiredMemorySize);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
