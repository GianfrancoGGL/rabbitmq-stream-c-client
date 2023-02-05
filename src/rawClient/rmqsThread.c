//--------------------------------------------------------------------------
#include <memory.h>
#if ! _WIN32 || _WIN64
#include <unistd.h>
#endif
//--------------------------------------------------------------------------
#include "rmqsThread.h"
#include "rmqsMemory.h"
//--------------------------------------------------------------------------
#if _WIN32 || _WIN64
uint32_t rmqsThreadRoutine(const uint32_t *ThreadData);
#else
void * rqmsThreadRoutine(void *ThreadData);
#endif
//--------------------------------------------------------------------------
rmqsThread_t * rmqsThreadCreate(ThreadRoutineCallback_t ThreadRoutineCallback, CancelIORoutineCallback_t CancelIORoutineCallback, void *Parameters)
{
    rmqsThread_t *Thread = (rmqsThread_t *)rmqsAllocateMemory(sizeof(rmqsThread_t));

    memset(Thread, 0, sizeof(rmqsThread_t));

    Thread->ThreadRoutineCallback = ThreadRoutineCallback;
    Thread->CancelIORoutineCallback = CancelIORoutineCallback;
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
    #if _WIN32 || _WIN64
    DWORD ThreadId;
    #endif

    if (Thread->ThreadHandle != 0)
    {
        return;
    }

    Thread->TerminateRequest = false;
    Thread->Terminated = false;

    #if _WIN32 || _WIN64
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

    Thread->TerminateRequest = true;

    if (Thread->CancelIORoutineCallback != 0)
    {
        Thread->CancelIORoutineCallback(Thread->Parameters);
    }

    while (! Thread->Terminated)
    {
        rmqsThreadSleep(50);
    }

    #if _WIN32 || _WIN64
    TerminateThread(Thread->ThreadHandle, 0x00);
    CloseHandle(Thread->ThreadHandle);
    #endif //

    Thread->TerminateRequest = false;
    Thread->Terminated = true;
}
//--------------------------------------------------------------------------
void rmqsThreadSleep(const uint32_t Milliseconds)
{
    #if _WIN32 || _WIN64
    Sleep(Milliseconds);
    #else
    usleep(Milliseconds * 1000);
    #endif
}
//--------------------------------------------------------------------------
void rmqsThreadSleepEx(const uint32_t Milliseconds, const size_t HowManyTimes, const bool_t *Abort)
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
#if _WIN32 || _WIN64
uint32_t rmqsThreadRoutine(const uint32_t *ThreadPointer)
#else
void * rqmsThreadRoutine(void *ThreadPointer)
#endif
{
    rmqsThread_t *Thread = (rmqsThread_t *)ThreadPointer;

    Thread->ThreadRoutineCallback(Thread->Parameters, &Thread->TerminateRequest);
    Thread->Terminated = true;

    #if _WIN32 || _WIN64
    return 0;
    #else
    pthread_exit(0);

    return 0;
    #endif
}
//--------------------------------------------------------------------------
