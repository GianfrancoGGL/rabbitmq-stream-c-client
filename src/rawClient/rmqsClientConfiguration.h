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
#ifndef rmqsClientConfigurationH
#define rmqsClientConfigurationH
//---------------------------------------------------------------------------
#include <stdint.h>
//---------------------------------------------------------------------------
#include "rmqsList.h"
#include "rmqsTimer.h"
#include "rmqsLogger.h"
//---------------------------------------------------------------------------
typedef struct
{
    bool_t IsLittleEndianMachine;
    rmqsList_t *BrokerList;
    bool_t UseTLS;
    rmqsTimer_t *WaitReplyTimer;
    rmqsLogger_t *Logger;
}
rmqsClientConfiguration_t;
//---------------------------------------------------------------------------
rmqsClientConfiguration_t * rmqsClientConfigurationCreate(const char_t *BrokersString,
                                                          const bool_t EnableLogging,
                                                          const char_t *LogFileName,
                                                          char_t *ErrorString,
                                                          const size_t ErrorStringLength);

void rmqsClientConfigurationDestroy(rmqsClientConfiguration_t *ClientConfiguration);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
