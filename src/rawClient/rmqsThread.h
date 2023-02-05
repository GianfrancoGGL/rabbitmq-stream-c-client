//--------------------------------------------------------------------------
#ifndef rmqsThreadH
#define rmqsThreadH
//--------------------------------------------------------------------------
#include <stdint.h>
//--------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif
//--------------------------------------------------------------------------
#include "rmqsGlobal.h"
//--------------------------------------------------------------------------
typedef void (*ThreadRoutineCallback_t)(void *Parameters, bool_t *Terminate);
typedef void (*CancelIORoutineCallback_t)(void *Parameters);
//--------------------------------------------------------------------------
typedef struct
{
    #if _WIN32 || _WIN64
    HANDLE ThreadHandle;
    #else
    pthread_t ThreadHandle;
    #endif
    ThreadRoutineCallback_t ThreadRoutineCallback;
    CancelIORoutineCallback_t CancelIORoutineCallback;
    void *Parameters;
    bool_t TerminateRequest;
    bool_t Terminated;
}
rmqsThread_t;
//---------------------------------------------------------------------------
rmqsThread_t * rmqsThreadCreate(ThreadRoutineCallback_t ThreadRoutineCallback, CancelIORoutineCallback_t CancelIORoutineCallback, void *Parameters);
void rmqsThreadDestroy(rmqsThread_t *Thread);
void rmqsThreadStart(rmqsThread_t *Thread);
void rmqsThreadStop(rmqsThread_t *Thread);
void rmqsThreadSleep(const uint32_t Milliseconds);
void rmqsThreadSleepEx(const uint32_t Milliseconds, const size_t HowManyTimes, const bool_t *Abort);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
