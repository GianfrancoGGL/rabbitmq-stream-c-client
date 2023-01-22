//---------------------------------------------------------------------------
#include <memory.h>
#if ! _WIN32 || _WIN64
#include <pthread.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsMutex.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsMutex_t * rmqsMutexCreate(void)
{
    rmqsMutex_t *Mutex = (rmqsMutex_t *)rmqsAllocateMemory(sizeof(rmqsMutex_t));

    #if _WIN32 || _WIN64
    InitializeCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_init(&Mutex->mutex, 0);
    #endif

    return Mutex;
}
//---------------------------------------------------------------------------
void rmqsMutexDestroy(rmqsMutex_t *Mutex)
{
    #if _WIN32 || _WIN64
    DeleteCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_destroy(&Mutex->mutex);
    #endif

    rmqsFreeMemory((void *)Mutex);
}
//---------------------------------------------------------------------------
void rmqsMutexLock(rmqsMutex_t *Mutex)
{
    #if _WIN32 || _WIN64
    EnterCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_lock(&Mutex->mutex);
    #endif
}
//---------------------------------------------------------------------------
void rmqsMutexUnlock(rmqsMutex_t *Mutex)
{
    #if _WIN32 || _WIN64
    LeaveCriticalSection((LPCRITICAL_SECTION)&Mutex->CS);
    #else
    pthread_mutex_unlock(&Mutex->mutex);
    #endif
}
//---------------------------------------------------------------------------
bool_t rmqsMutexTryLock(rmqsMutex_t *Mutex)
{
    #if _WIN32 || _WIN64
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