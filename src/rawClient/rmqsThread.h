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
//--------------------------------------------------------------------------
#ifndef rmqsThreadH
#define rmqsThreadH
//--------------------------------------------------------------------------
#include <stdint.h>
//--------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif
//--------------------------------------------------------------------------
#include "rmqsGlobal.h"
//--------------------------------------------------------------------------
typedef void (*ThreadRoutineCallback_t)(void *Parameters, bool_t *Terminate);
typedef void (*CancelIORoutineCallback_t)(void *Parameters);
//--------------------------------------------------------------------------
typedef struct
{
    #if _WIN32 || _WIN64
    HANDLE ThreadHandle;
    #else
    pthread_t ThreadHandle;
    #endif
    ThreadRoutineCallback_t ThreadRoutineCallback;
    CancelIORoutineCallback_t CancelIORoutineCallback;
    void *Parameters;
    bool_t TerminateRequest;
    bool_t Terminated;
}
rmqsThread_t;
//---------------------------------------------------------------------------
rmqsThread_t * rmqsThreadCreate(ThreadRoutineCallback_t ThreadRoutineCallback, CancelIORoutineCallback_t CancelIORoutineCallback, void *Parameters);
void rmqsThreadDestroy(rmqsThread_t *Thread);
void rmqsThreadStart(rmqsThread_t *Thread);
void rmqsThreadStop(rmqsThread_t *Thread);
void rmqsThreadSleep(uint32_t Milliseconds);
void rmqsThreadSleepEx(uint32_t Milliseconds, size_t HowManyTimes, bool_t *Abort);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
