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
    double PCFrequency;
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
