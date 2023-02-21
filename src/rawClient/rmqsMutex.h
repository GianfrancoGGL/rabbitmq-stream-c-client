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
#ifndef rmqsMutexH
#define rmqsMutexH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
typedef struct
{
#if _WIN32 || _WIN64
    CRITICAL_SECTION CS;
#else
    pthread_mutex_t mutex;
#endif
}
rmqsMutex_t;
//---------------------------------------------------------------------------
rmqsMutex_t * rmqsMutexCreate(void);
void rmqsMutexDestroy(rmqsMutex_t *Mutex);
void rmqsMutexLock(rmqsMutex_t *Mutex);
void rmqsMutexUnlock(rmqsMutex_t *Mutex);
bool_t rmqsMutexTryLock(rmqsMutex_t *Mutex);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
