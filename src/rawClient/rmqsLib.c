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
#include <ctype.h>
#include <sys/timeb.h>
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <sys/time.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsLib.h"
//---------------------------------------------------------------------------
int64_t rmqsGetTimeStamp()
{
    #if _WIN32 || _WIN64
    struct _timeb TimeBuffer;
    #else
    struct timeval TV;
    struct timezone TZ;
    #endif
    int64_t Result;

    #if _WIN32 || _WIN64
    _ftime(&TimeBuffer);
    Result = (int64_t)((TimeBuffer.time * 1000) + TimeBuffer.millitm);
    Result -= ((int64_t)TimeBuffer.timezone - (60 * TimeBuffer.dstflag)) * 60 * 1000; // UTC to local time
    #else
    gettimeofday(&TV, &TZ);

    Result = (int64_t)((uint64_t)TV.tv_sec * 1000 + (uint64_t)(TV.tv_usec) / 1000);
	Result -= (TZ.tz_minuteswest * 60) * 1000;
    #endif

    return Result;
}
//---------------------------------------------------------------------------
bool_t rmqsStringContainsSpace(const char_t *String)
{
    while (*String)
    {
        if (*String == ' ')
        {
            return true;
        }

        String++;
    }

    return false;
}
//---------------------------------------------------------------------------
bool_t rmqsStringContainsCtrlChar(const char_t *String)
{
    while (*String)
    {
        if (iscntrl(*String))
        {
            return true;
        }

        String++;
    }

    return false;
}
//---------------------------------------------------------------------------
void rmqsConvertToLower(char_t *String)
{
    while (*String)
    {
        *String = (char_t)tolower(*String);
        String++;
    }
}
//---------------------------------------------------------------------------


