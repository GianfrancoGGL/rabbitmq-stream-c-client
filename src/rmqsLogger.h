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
    char *FileName;
    uint8_t AppendToFile;
    FILE *File;
    rmqsMutex_t *Mutex;
    rmqsTimer_t *Timer;
}
rmqsLogger_t;
//---------------------------------------------------------------------------
rmqsLogger_t * rmqsLoggerCreate(char *FileName, uint8_t AppendToFile);
void rmqsLoggerDestroy(rmqsLogger_t *Logger);
void rmqsLoggerRegisterLog(rmqsLogger_t *Logger, char *Message, char *Comment);
void rmqsLoggerRegisterDump(rmqsLogger_t *Logger, void *Data, size_t DataLen, char *Comment1, char *Comment2);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
