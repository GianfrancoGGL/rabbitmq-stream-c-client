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
#include "rmqsMemory.h"
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
size_t TotalUsedMemory = 0;
//---------------------------------------------------------------------------
void * rmqsAllocateMemory(size_t Size)
{
    char_t *Memory = (char_t *)malloc(Size + sizeof(size_t)); // Allocate 4 extra bytes to store the block size

    *(size_t *)Memory = Size; // Store the size at the beginning of the memory block
    Memory += sizeof(size_t); // Move the pointer by 4 bytes to point to memory to use

    TotalUsedMemory += Size + sizeof(size_t); // Used memory is the specified size + 4 for the block size

    return (void *)Memory; // Memory pointer to be used
}
//---------------------------------------------------------------------------
void rmqsFreeMemory(void *Memory)
{
    char_t *MemoryToFree = (char_t *)Memory;

    MemoryToFree -= sizeof(size_t); // Move back the pointer by 4 bytes to point to
                                    // the effective allocated block start, where the size is stored

    TotalUsedMemory -= *(size_t *)MemoryToFree + sizeof(size_t); // Decrease the amount of allocated memory

    free((void *)MemoryToFree);
}
//---------------------------------------------------------------------------
void * rmqsRellocateMemory(void *Memory, size_t Size)
{
    char_t *OldMemory = (char_t *)Memory, *NewMemory;

    OldMemory -= sizeof(size_t); // Move back the pointer by 4 bytes to point to
                                 // the effective allocated block start, where the size is stored

    TotalUsedMemory -= *(size_t *)OldMemory + sizeof(size_t); // Decrease the amount of allocated size

    NewMemory = realloc((void *)OldMemory, Size + sizeof(size_t)); // Allocate a new memory block,
                                                                   // preserving the original memory content

    *(size_t *)NewMemory = Size; // Store the size at the beginning of the memory block
    NewMemory += sizeof(size_t); // Memory pointer to be used

    TotalUsedMemory += Size + sizeof(size_t); // Update the used memory size

    return (void *)NewMemory;
}
//---------------------------------------------------------------------------
size_t rmqsGetUsedMemory(void)
{
    return TotalUsedMemory;
}
//---------------------------------------------------------------------------

