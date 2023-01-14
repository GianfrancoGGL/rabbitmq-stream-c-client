//---------------------------------------------------------------------------
#ifndef rmqsNetworkH
#define rmqsNetworkH
//---------------------------------------------------------------------------
#include <stdint.h>
#ifndef __WIN32__
#include <sys/socket.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsGlobal.h"
//---------------------------------------------------------------------------
#define RMQS_MAX_HOSTNAME_LENGTH     255
//---------------------------------------------------------------------------
#ifdef __WIN32__
typedef uint32_t rmqsSocket;
typedef int32_t rmqsSocketLen;
#define rmqsInvalidSocket 0
#else
typedef int32_t rmqsSocket;
typedef socklen_t rmqsSocketLen;
#define rmqsInvalidSocket -1
#endif
//---------------------------------------------------------------------------
#ifdef __WIN32__
int32_t rmqsInitWinsock(void);
void rmqsShutdownWinsock(void);
#endif
//---------------------------------------------------------------------------
rmqsSocket rmqsSocketCreate(void);
void rmqsSocketDestroy(rmqsSocket *Socket);
//---------------------------------------------------------------------------
uint8_t rmqsSocketConnect(const char_t *Host, const uint16_t Port, const rmqsSocket Socket, const uint32_t TimeoutMs);
//---------------------------------------------------------------------------
void rmqsSetSocketReadTimeouts(const rmqsSocket Socket, const uint32_t ReadTimeout);
void rmqsSetSocketWriteTimeouts(const rmqsSocket Socket, const uint32_t WriteTimeout);
uint8_t rmqsSetSocketTxRxBuffers(const rmqsSocket Socket, const uint32_t TxBufferSize, const uint32_t RxBufferSize);
uint8_t rmqsSetSocketBlocking(const rmqsSocket Socket);
uint8_t rmqsSetSocketNonBlocking(const rmqsSocket Socket);
uint8_t rmqsSetKeepAlive(const rmqsSocket Socket);
uint8_t rmqsSetTcpNoDelay(const rmqsSocket Socket);
//---------------------------------------------------------------------------
uint8_t rmqsNetworkError(void);
//---------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------
