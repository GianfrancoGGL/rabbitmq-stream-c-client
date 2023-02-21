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
