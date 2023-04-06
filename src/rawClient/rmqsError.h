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
#ifndef rmqsErrorH
#define rmqsErrorH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
#define RMQS_ERR_IDX_BROKER_DEF_WITH_SPACE         0
#define RMQS_ERR_IDX_BROKER_DEF_WITH_CTRL_CHAR     1
#define RMQS_ERR_IDX_EMPTY_HOSTNAME                2
#define RMQS_ERR_IDX_UNSPECIFIED_PORT              3
#define RMQS_ERR_IDX_WRONG_BROKER_DEF              4
//---------------------------------------------------------------------------
#define RMQS_ERR_MAX_IDX                           RMQS_ERR_IDX_WRONG_BROKER_DEF
//---------------------------------------------------------------------------
#define RMQS_ERR_MAX_STRING_LENGTH               256
//---------------------------------------------------------------------------
void rmqsSetError(size_t ErrorIndex, char_t *ErrorString, size_t ErrorStringLength);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
