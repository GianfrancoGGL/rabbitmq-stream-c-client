//---------------------------------------------------------------------------
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
void * rmqsAllocateMemory(size_t Size)
{
    void *Memory = malloc(Size);

    return Memory;
}
//---------------------------------------------------------------------------
void rmqsFreeMemory(void *Memory)
{
    free(Memory);
}
//---------------------------------------------------------------------------
void * rmqsRellocateMemory(void *Memory, size_t size)
{
    void *NewMemory = realloc(Memory, size);

    return NewMemory;
}
//---------------------------------------------------------------------------

