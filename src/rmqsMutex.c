//---------------------------------------------------------------------------
#include <memory.h>
#ifndef __WIN32__
#include <pthread.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsMutex.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMutex * rmqsMutexCreate(void)
{
    rmqsMutex *Mutex = (rmqsMutex *)rmqsAllocateMemory(sizeof(rmqsMutex));

    #ifdef __WIN32__
    InitializeCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_init(&Mutex->mutex, 0);
    #endif

    return Mutex;
}
//---------------------------------------------------------------------------
void rmqsMutexDestroy(rmqsMutex *Mutex)
{
    #ifdef __WIN32__
    DeleteCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_destroy(&Mutex->mutex);
    #endif

    rmqsFreeMemory((void *)Mutex);
}
//---------------------------------------------------------------------------
void rmqsMutexLock(rmqsMutex *Mutex)
{
    #ifdef __WIN32__
    EnterCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_lock(&Mutex->mutex);
    #endif
}
//---------------------------------------------------------------------------
void rmqsMutexUnlock(rmqsMutex *Mutex)
{
    #ifdef __WIN32__
    LeaveCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_unlock(&Mutex->mutex);
    #endif
}
//---------------------------------------------------------------------------
uint8_t rmqsMutexTryLock(rmqsMutex *Mutex)
{
    #ifdef __WIN32__
    if (TryEnterCriticalSection((LPCRITICAL_SECTION)&Mutex->CS))
    {
        return 1;
    }
    else
    {
        return 0;
    }
    #else
    if (pthread_mutex_trylock(&Mutex->mutex) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    #endif
}
//---------------------------------------------------------------------------
