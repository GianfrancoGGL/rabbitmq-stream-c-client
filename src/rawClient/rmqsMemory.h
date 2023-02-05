//---------------------------------------------------------------------------
#ifndef rmqsMemoryH
#define rmqsMemoryH
//---------------------------------------------------------------------------
#include <stdlib.h>
#include <memory.h>
//---------------------------------------------------------------------------
void * rmqsAllocateMemory(const size_t Size);
void rmqsFreeMemory(void *Memory);
void * rmqsRellocateMemory(void *Memory, const size_t Size);
size_t rmqsGetUsedMemory(void);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
