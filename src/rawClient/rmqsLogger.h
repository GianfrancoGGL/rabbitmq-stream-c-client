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
rmqsLogger_t * rmqsLoggerCreate(char_t *FileName, bool_t AppendToFile);
void rmqsLoggerDestroy(rmqsLogger_t *Logger);
void rmqsLoggerRegisterLog(rmqsLogger_t *Logger, char_t *Message, char_t *Comment);
void rmqsLoggerRegisterDump(rmqsLogger_t *Logger, void *Data, size_t DataLen, char_t *Comment1, char_t *Comment2, char_t *Comment3);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
