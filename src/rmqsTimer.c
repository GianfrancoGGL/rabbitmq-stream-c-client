//---------------------------------------------------------------------------
#include <time.h>
#include <stdio.h>
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

    TheTick = (TS.tv_nsec / 1000000) + (TS.tv_sec * 1000);

    return TheTick;
    #endif
}
//---------------------------------------------------------------------------
void rmqsGetCurrentDateTime(uint16_t *Year, uint8_t *Month, uint8_t *Day, uint8_t *Hour, uint8_t *Minute, uint8_t *Second)
{
    #ifdef __WIN32__
    SYSTEMTIME SystemTime;

    GetLocalTime(&SystemTime);

    *Year = SystemTime.wYear;
    *Month = (uint8_t)SystemTime.wMonth;
    *Day = (uint8_t)SystemTime.wDay;
    *Hour = (uint8_t)SystemTime.wHour;
    *Minute = (uint8_t)SystemTime.wMinute;
    *Second = (uint8_t)SystemTime.wSecond;
    #else
    time_t Time;
    struct tm TM;

    time(&Time);
    gmtime_r((const time_t *)&time, &TM);

    *Year = (uint16_t)(TM.tm_year + 1900);
    *Month = (uint8_t)(TM.tm_mon + 1);
    *Day = (uint8_t)TM.tm_mday;
    *Hour = (uint8_t)TM.tm_hour;
    *Minute = (uint8_t)TM.tm_min;
    *Second = (uint8_t)TM.tm_sec;
    #endif
}
//---------------------------------------------------------------------------
void rmqsGetCurrentDateTimeString(char *String, size_t Size)
{
    uint16_t Year;
    uint8_t Month, Day, Hour, Minute, Second;

    if (Size < 20)
    {
        *String = 0;
        return;
    }

    rmqsGetCurrentDateTime(&Year, &Month, &Day, &Hour, &Minute, &Second);

    sprintf(String, "%04d-%02d-%02d %02d:%02d:%02d", (int32_t)Year, (int32_t)Month, (int32_t)Day, (int32_t)Hour, (int32_t)Minute, (int32_t)Second);
}
//---------------------------------------------------------------------------
