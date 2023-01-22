//---------------------------------------------------------------------------
#ifndef rmqsLoggerH
#define rmqsLoggerH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
//---------------------------------------------------------------------------
#include "rmqsMutex.h"
#include "rmqsTimer.h"
//---------------------------------------------------------------------------
typedef struct
{
    char_t *FileName;
    bool_t AppendToFile;
    FILE *File;
    rmqsMutex_t *Mutex;
    rmqsTimer_t *Timer;
}
rmqsLogger_t;
//---------------------------------------------------------------------------
rmqsLogger_t * rmqsLoggerCreate(const char_t *FileName, bool_t AppendToFile);
void rmqsLoggerDestroy(rmqsLogger_t *Logger);
void rmqsLoggerRegisterLog(rmqsLogger_t *Logger, const char_t *Message, const char_t *Comment);
void rmqsLoggerRegisterDump(rmqsLogger_t *Logger, void *Data, size_t DataLen, const char_t *Comment1, const char_t *Comment2);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
