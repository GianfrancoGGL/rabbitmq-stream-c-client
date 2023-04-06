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
#ifndef rmqsTimerH
#define rmqsTimerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
typedef struct
{
    bool_t Running;

    uint32_t TimeStarted;

    #if _WIN32 || _WIN64
    double_t PCFrequency;
    int64_t CounterStart;
    #endif
}
rmqsTimer_t;
//---------------------------------------------------------------------------
rmqsTimer_t * rmqsTimerCreate(void);
void rmqsTimerDestroy(rmqsTimer_t *Timer);
void rmqsTimerStart(rmqsTimer_t *Timer);
void rmqsTimerStop(rmqsTimer_t *Timer);
uint32_t rmqsTimerGetTime(rmqsTimer_t *Timer);
uint32_t rmqsTimerGetSystemClock(rmqsTimer_t *Timer);
void rmqsGetCurrentDateTime(uint16_t *Year, uint8_t *Month, uint8_t *Day, uint8_t *Hour, uint8_t *Minute, uint8_t *Second);
void rmqsGetCurrentDateTimeString(char_t *String, size_t Size);
//---------------------------------------------------------------------------
#endif
