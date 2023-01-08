//--------------------------------------------------------------------------
#ifndef rmqsThreadH
#define rmqsThreadH
//--------------------------------------------------------------------------
#include <stdint.h>
//--------------------------------------------------------------------------
#ifdef __WIN32__
#include <windows.h>
#else
#include <pthread.h>
#endif
//--------------------------------------------------------------------------
typedef struct
{
    #ifdef __WIN32__
    HANDLE ThreadHandle;
    #else
    pthread_t ThreadHandle;
    #endif
    void (*ThreadRoutine)(void *, uint8_t *);
    void (*CancelIORoutine)(void *);
    void *Parameters;
    uint8_t TerminateRequest;
    uint8_t Terminated;
}
rmqsThread;
//---------------------------------------------------------------------------
rmqsThread * rmqsThreadCreate(void (*ThreadRoutine)(void *, uint8_t *), void (*CancelIORoutine)(void *), void *Parameters);
void rmqsThreadDestroy(rmqsThread *Thread);
void rmqsThreadStart(rmqsThread *Thread);
void rmqsThreadStop(rmqsThread *Thread);
void rmqsThreadSleep(uint32_t Milliseconds);
void rmqsThreadSleepEx(uint32_t Milliseconds, size_t HowManyTimes, uint8_t *Abort);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
