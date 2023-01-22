//---------------------------------------------------------------------------
#ifndef rmqsMutexH
#define rmqsMutexH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
typedef struct
{
#if _WIN32 || _WIN64
    CRITICAL_SECTION CS;
#else
    pthread_mutex_t mutex;
#endif
}
rmqsMutex_t;
//---------------------------------------------------------------------------
rmqsMutex_t * rmqsMutexCreate(void);
void rmqsMutexDestroy(rmqsMutex_t *Mutex);
void rmqsMutexLock(rmqsMutex_t *Mutex);
void rmqsMutexUnlock(rmqsMutex_t *Mutex);
bool_t rmqsMutexTryLock(rmqsMutex_t *Mutex);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
