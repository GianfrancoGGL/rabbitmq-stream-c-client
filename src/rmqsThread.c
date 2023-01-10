//--------------------------------------------------------------------------
#include <memory.h>
#ifndef __WIN32__
#include <unistd.h>
#endif
//--------------------------------------------------------------------------
#include "rmqsThread.h"
#include "rmqsMemory.h"
//--------------------------------------------------------------------------
#ifdef __WIN32__
uint32_t rmqsThreadRoutine(uint32_t *ThreadData);
#else
void * rqmsThreadRoutine(void *ThreadData);
#endif
//--------------------------------------------------------------------------
rmqsThread_t * rmqsThreadCreate(void (*ThreadRoutine)(void *, uint8_t *), void (*CancelIORoutine)(void *), void *Parameters)
{
    rmqsThread_t *Thread = (rmqsThread_t *)rmqsAllocateMemory(sizeof(rmqsThread_t));

    memset(Thread, 0, sizeof(rmqsThread_t));

    Thread->ThreadRoutine = ThreadRoutine;
    Thread->CancelIORoutine = CancelIORoutine;
    Thread->Parameters = Parameters;

    return Thread;
}
//--------------------------------------------------------------------------
void rmqsThreadDestroy(rmqsThread_t *Thread)
{
    rmqsFreeMemory((void *)Thread);
}
//--------------------------------------------------------------------------
void rmqsThreadStart(rmqsThread_t *Thread)
{
    #ifdef __WIN32__
    DWORD ThreadId;
    #endif

    if (Thread->ThreadHandle != 0)
    {
        return;
    }

    Thread->TerminateRequest = 0;
    Thread->Terminated = 0;

    #ifdef __WIN32__
    Thread->ThreadHandle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)rmqsThreadRoutine, (uint32_t *)Thread, 0, &ThreadId);
    #else
    pthread_create(&Thread->ThreadHandle, 0, rqmsThreadRoutine, Thread);
    #endif
}
//--------------------------------------------------------------------------
void rmqsThreadStop(rmqsThread_t *Thread)
{
    if (Thread->ThreadHandle == 0)
    {
        return;
    }

    Thread->TerminateRequest = 1;

    if (Thread->CancelIORoutine != 0)
    {
        Thread->CancelIORoutine(Thread->Parameters);
    }

    while (! Thread->Terminated)
    {
        rmqsThreadSleep(50);
    }

    #ifdef __WIN32__
    TerminateThread(Thread->ThreadHandle, 0x00);
    CloseHandle(Thread->ThreadHandle);
    #endif //

    Thread->TerminateRequest = 0;
    Thread->Terminated = 1;
}
//--------------------------------------------------------------------------
void rmqsThreadSleep(uint32_t Milliseconds)
{
    #ifdef __WIN32__
    Sleep(Milliseconds);
    #else
    usleep(Milliseconds * 1000);
    #endif
}
//--------------------------------------------------------------------------
void rmqsThreadSleepEx(uint32_t Milliseconds, size_t HowManyTimes, uint8_t *Abort)
{
    size_t i;

    for (i = 0; i < HowManyTimes; i++)
    {
        if (*Abort)
        {
            return;
        }

        rmqsThreadSleep(Milliseconds);
    }
}
//--------------------------------------------------------------------------
#ifdef __WIN32__
uint32_t rmqsThreadRoutine(uint32_t *ThreadData)
#else
void * rqmsThreadRoutine(void *ThreadData)
#endif
{
    rmqsThread_t *Thread = (rmqsThread_t *)ThreadData;

    Thread->ThreadRoutine(Thread->Parameters, &Thread->TerminateRequest);
    Thread->Terminated = 1;

    #ifdef __WIN32__
    return 0;
    #else
    pthread_exit(0);

    return 0;
    #endif
}
//--------------------------------------------------------------------------

