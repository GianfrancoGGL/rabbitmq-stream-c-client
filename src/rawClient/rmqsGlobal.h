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
#ifndef rmqsGlobalH
#define rmqsGlobalH
//---------------------------------------------------------------------------
typedef char char_t;
typedef unsigned char uchar_t;
typedef long long_t;
typedef unsigned long ulong_t;
typedef unsigned char bool_t;
typedef double double_t;
//---------------------------------------------------------------------------
#ifndef __cplusplus
#ifndef true
#define true   1
#endif
#ifndef false
#define false  0
#endif
#endif
#ifndef min
#define min(x, y) (x < y ? x : y)
#endif
//---------------------------------------------------------------------------
#define RQMS_MAX_HOSTNAME_LENGTH      256
//---------------------------------------------------------------------------
#define rmqsFieldSize(Type, Field)    sizeof(((Type *)0)->Field)
//---------------------------------------------------------------------------
#endif
