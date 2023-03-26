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
#ifndef rmqsNetworkH
#define rmqsNetworkH
//---------------------------------------------------------------------------
#include <stdint.h>
#if ! (_WIN32 || _WIN64)
#include <sys/socket.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_HOSTNAME_LENGTH     255
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
typedef uint32_t rmqsSocket;
typedef int32_t rmqsSocketLen;
#define rmqsInvalidSocket 0
#else
typedef int32_t rmqsSocket;
typedef socklen_t rmqsSocketLen;
#define rmqsInvalidSocket -1
#endif
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
int32_t rmqsInitWinsock(void);
void rmqsShutdownWinsock(void);
#endif
//---------------------------------------------------------------------------
rmqsSocket rmqsSocketCreate(void);
void rmqsSocketDestroy(rmqsSocket *Socket);
//---------------------------------------------------------------------------
bool_t rmqsSocketConnect(const char_t *Host, const uint16_t Port, const rmqsSocket Socket, const uint32_t TimeoutMs);
//---------------------------------------------------------------------------
void rmqsSetSocketReadTimeout(const rmqsSocket Socket, const uint32_t ReadTimeout);
void rmqsSetSocketWriteTimeout(const rmqsSocket Socket, const uint32_t WriteTimeout);
bool_t rmqsSetSocketTxRxBuffers(const rmqsSocket Socket, const uint32_t TxBufferSize, const uint32_t RxBufferSize);
bool_t rmqsSetSocketBlocking(const rmqsSocket Socket);
bool_t rmqsSetSocketNonBlocking(const rmqsSocket Socket);
bool_t rmqsSetKeepAlive(const rmqsSocket Socket);
bool_t rmqsSetTcpNoDelay(const rmqsSocket Socket);
//---------------------------------------------------------------------------
bool_t rmqsNetworkError(void);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
