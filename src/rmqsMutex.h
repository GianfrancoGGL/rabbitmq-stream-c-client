//---------------------------------------------------------------------------
#ifndef rmqsMutexH
#define rmqsMutexH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#ifdef __WIN32__
#include <windows.h>
#else
#include <pthread.h>
#endif
//---------------------------------------------------------------------------
typedef struct
{
#ifdef __WIN32__
    CRITICAL_SECTION CS;
#else
    pthread_mutex_t mutex;
#endif
}
rmqsMutex;
//---------------------------------------------------------------------------
rmqsMutex * rmqsMutexCreate(void);
void rmqsMutexDestroy(rmqsMutex *Mutex);
void rmqsMutexLock(rmqsMutex *Mutex);
void rmqsMutexUnlock(rmqsMutex *Mutex);
uint8_t rmqsMutexTryLock(rmqsMutex *Mutex);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
