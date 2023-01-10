//---------------------------------------------------------------------------
#ifndef rmqsTimerH
#define rmqsTimerH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#ifdef __WIN32__
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
//---------------------------------------------------------------------------
typedef struct
{
    uint8_t Running;

    uint32_t TimeStarted;

    #ifdef __WIN32__
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
//---------------------------------------------------------------------------
#endif
