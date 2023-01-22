//---------------------------------------------------------------------------
#ifndef rmqsNetworkH
#define rmqsNetworkH
//---------------------------------------------------------------------------
#include <stdint.h>
#if ! __WIN32__
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
void rmqsSetSocketReadTimeouts(const rmqsSocket Socket, const uint32_t ReadTimeout);
void rmqsSetSocketWriteTimeouts(const rmqsSocket Socket, const uint32_t WriteTimeout);
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
