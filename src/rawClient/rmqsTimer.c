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
    #if _WIN32 || _WIN64
    LARGE_INTEGER LI;

    QueryPerformanceFrequency(&LI);

    Timer->PCFrequency = (double_t)LI.QuadPart / 1000.0;

    QueryPerformanceCounter(&LI);

    Timer->CounterStart = LI.QuadPart;
    #endif

    Timer->Running = true;
    Timer->TimeStarted = rmqsTimerGetSystemClock(Timer);
}
//---------------------------------------------------------------------------
void rmqsTimerStop(rmqsTimer_t *Timer)
{
    Timer->Running = false;
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
    #if _WIN32 || _WIN64
    LARGE_INTEGER LI;

    if (! Timer->Running)
    {
        return 0;
    }

    QueryPerformanceCounter(&LI);

    return (uint32_t)(double_t)((LI.QuadPart - Timer->CounterStart) / Timer->PCFrequency);
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
    #if _WIN32 || _WIN64
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
    localtime_r((time_t *)&Time, &TM);

    *Year = (uint16_t)(TM.tm_year + 1900);
    *Month = (uint8_t)(TM.tm_mon + 1);
    *Day = (uint8_t)TM.tm_mday;
    *Hour = (uint8_t)TM.tm_hour;
    *Minute = (uint8_t)TM.tm_min;
    *Second = (uint8_t)TM.tm_sec;
    #endif
}
//---------------------------------------------------------------------------
void rmqsGetCurrentDateTimeString(char_t *String, size_t Size)
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
