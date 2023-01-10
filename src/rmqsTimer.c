//---------------------------------------------------------------------------
#include <time.h>
#include <limits.h>
//---------------------------------------------------------------------------
#include "rmqsTimer.h"
#include "rmqsMemory.h"
//---------------------------------------------------------------------------
rmqsTimer_t * rmqsTimerCreate(void)
{
    rmqsTimer_t *Timer = (rmqsTimer_t *)rmqsAllocateMemory(sizeof(rmqsTimer_t));

    memset(Timer, 0, sizeof(rmqsTimer_t));

    return Timer;
}
//---------------------------------------------------------------------------
void rmqsTimerDestroy(rmqsTimer_t *Timer)
{
    rmqsFreeMemory((void *)Timer);
}
//---------------------------------------------------------------------------
void rmqsTimerStart(rmqsTimer_t *Timer)
{
    #ifdef __WIN32__
    LARGE_INTEGER LI;

    QueryPerformanceFrequency(&LI);

    Timer->PCFrequency = (double)LI.QuadPart / 1000.0;

    QueryPerformanceCounter(&LI);

    Timer->CounterStart = LI.QuadPart;
    #endif

    Timer->Running = 1;
    Timer->TimeStarted = rmqsTimerGetSystemClock(Timer);
}
//---------------------------------------------------------------------------
void rmqsTimerStop(rmqsTimer_t *Timer)
{
    Timer->Running = 0;
}
//---------------------------------------------------------------------------
uint32_t rmqsTimerGetTime(rmqsTimer_t *Timer)
{
    if (! Timer->Running)
    {
        return 0;
    }

    return rmqsTimerGetSystemClock(Timer) - Timer->TimeStarted;
}
//---------------------------------------------------------------------------
uint32_t rmqsTimerGetSystemClock(rmqsTimer_t *Timer)
{
    #ifdef __WIN32__
    LARGE_INTEGER LI;

    if (! Timer->Running)
    {
        return 0;
    }

    QueryPerformanceCounter(&LI);

    if (Timer->PCFrequency != 0.0)
    {
        return (uint32_t) (double)((LI.QuadPart - Timer->CounterStart) / Timer->PCFrequency);
    }
    else
    {
        return 0;
    }
    #else
    struct timespec TS;
    uint32_t TheTick;

    clock_gettime(CLOCK_MONOTONIC, &TS);

    TheTick  = TS.tv_nsec / 1000000;
    TheTick += TS.tv_sec * 1000;

    return TheTick;
    #endif
}
//---------------------------------------------------------------------------

