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
#include <stdio.h>
#if _WIN32 || _WIN64
#include <winsock2.h>
#else
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <strings.h>
#include <netdb.h>
#include <fcntl.h>
#endif
//---------------------------------------------------------------------------
#include "rmqsNetwork.h"
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)
typedef struct
{
    u_long onoff;
    u_long keepalivetime;
    u_long keepaliveinterval;
}
TcpKeepAlive;
#else
#if ! defined(SOL_TCP) && defined(IPPROTO_TCP)
#define SOL_TCP IPPROTO_TCP
#endif
#if ! defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif
#endif
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
int32_t rmqsInitWinsock(void)
{
    WSADATA wsaData;
    int32_t Result;

    Result = WSAStartup((WORD)((2 << 8) | 2), &wsaData);

    if (Result == 0)
    {
        return Result;
    }

    Result = WSAStartup((WORD)((2 << 8) | 1), &wsaData);

    if (Result == 0)
    {
        return Result;
    }

    Result = WSAStartup((WORD)((2 << 8) | 0), &wsaData);

    if (Result == 0)
    {
        return Result;
    }

    Result = WSAStartup((WORD)((1 << 8) | 1), &wsaData);

    if (Result == 0)
    {
        return Result;
    }

    Result = WSAStartup((WORD)((1 << 8) | 0), &wsaData);

    return Result;
}
#endif
//---------------------------------------------------------------------------
#if _WIN32 || _WIN64
void rmqsShutdownWinsock(void)
{
    WSACleanup();
}
#endif
//---------------------------------------------------------------------------
rmqsSocket rmqsSocketCreate(void)
{
    return (rmqsSocket)socket(AF_INET, SOCK_STREAM, 0);
}
//---------------------------------------------------------------------------
void rmqsSocketDestroy(rmqsSocket *Socket)
{
    #if _WIN32 || _WIN64
    shutdown(*Socket, SD_BOTH);
    closesocket(*Socket);
    #else
    shutdown(*Socket, SHUT_RDWR);
    close(*Socket);
    #endif

    *Socket = rmqsInvalidSocket;
}
//---------------------------------------------------------------------------
bool_t rmqsSocketConnect(const char_t *Host, const uint16_t Port, const rmqsSocket Socket, const uint32_t TimeoutMs)
{
    bool_t Connected = false;
    struct hostent *pHost;
    struct sockaddr_in ServerAddress;
    #if _WIN32 || _WIN64
    fd_set FDS;
    TIMEVAL Timeout;
    int32_t Result;
    #endif

    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(Port);

    #if _WIN32 || _WIN64
    memset(ServerAddress.sin_zero, 0, 8);
    #else
    bzero(&(ServerAddress.sin_zero), 8);
    #endif

    pHost = (struct hostent *)gethostbyname(Host);

    if (pHost)
    {
        ServerAddress.sin_addr = *(struct in_addr *)pHost->h_addr;
    }
    else
    {
        ServerAddress.sin_addr.s_addr = inet_addr(Host);
    }

    #if _WIN32 || _WIN64
    rmqsSetSocketNonBlocking(Socket); // Socket set as nonblocking to set the connect timeout

    if (connect(Socket, (struct sockaddr *)&ServerAddress, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        Result = WSAGetLastError();

        if (Result == WSAEWOULDBLOCK || Result == WSAEALREADY)
        {
            //
            // Set timeout...
            //
            FD_ZERO(&FDS);
            FD_SET(Socket, &FDS);

            Timeout.tv_sec = 0;
            Timeout.tv_usec = TimeoutMs * 1000; // Must be expressed in microseconds...

            if (select(0, 0, &FDS, 0, &Timeout) > 0)
            {
                Connected = 1;
            }
        }
    }
    else
    {
        Connected = 1;
    }

    rmqsSetSocketBlocking(Socket);
    #else
	setsockopt(Socket, IPPROTO_TCP , TCP_USER_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs));

    #ifdef __APPLE__
	setsockopt(Socket, IPPROTO_TCP , TCP_CONNECTIONTIMEOUT, &TimeoutMs, sizeof(TimeoutMs));
    #else
    setsockopt(Socket, IPPROTO_TCP , TCP_USER_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs));
    #endif

    if (connect(Socket, (struct sockaddr *)&ServerAddress, sizeof(struct sockaddr)) == 0)
    {
        Connected = 1;
    }
    #endif

    return Connected;
}
//---------------------------------------------------------------------------
void rmqsSetSocketReadTimeout(const rmqsSocket Socket, const uint32_t ReadTimeout)
{
    #if _WIN32 || _WIN64
    DWORD dwRead;
    dwRead = ReadTimeout;

    setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (const char_t *)&dwRead, sizeof(dwRead));
    #else
    struct timeval TVRead;
    TVRead.tv_sec = 0;
    TVRead.tv_usec = ReadTimeout * 1000;

    setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (const int8_t *)&TVRead, sizeof(TVRead));
    #endif
}
//--------------------------------------------------------------------------
void rmqsSetSocketWriteTimeout(const rmqsSocket Socket, const uint32_t WriteTimeout)
{
    #if ! _WIN32 || _WIN64
    struct timeval TVWrite;

    TVWrite.tv_sec = 0;
    TVWrite.tv_usec = WriteTimeout * 1000;

    setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (const char_t *)&TVWrite, (int32_t)sizeof(TVWrite));
    #else
    DWORD dwWrite;
    dwWrite = WriteTimeout;

    setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (const char_t *)&dwWrite, sizeof(dwWrite));
    #endif
}
//--------------------------------------------------------------------------
bool_t rmqsSetSocketTxRxBuffers(const rmqsSocket Socket, const uint32_t ulTxBufferSize, const uint32_t ulRxBufferSize)
{
    bool_t Result = true;

    if (ulTxBufferSize != 0) // 0 = default
    {
        if (setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (const char_t *)&ulTxBufferSize, sizeof(uint32_t)) == -1)
        {
            Result = false;
        }
    }

    if (ulRxBufferSize != 0) // 0 = default
    {
        if (setsockopt(Socket, SOL_SOCKET, SO_RCVBUF, (const char_t *)&ulRxBufferSize, sizeof(uint32_t)) == -1)
        {
            Result = false;
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
bool_t rmqsSetSocketBlocking(const rmqsSocket Socket)
{
    #if _WIN32 || _WIN64
    unsigned long ulNonBlocking = 0;

    if (ioctlsocket(Socket, FIONBIO, &ulNonBlocking) == -1)
    {
        return 0;
    }
    #else
    int32_t Flags;

    Flags = fcntl(Socket, F_GETFL, 0);

    if (fcntl(Socket, F_SETFL, Flags & (~O_NONBLOCK)) == -1)
    {
        return 0;
    }
    #endif

    return 1;
}
//---------------------------------------------------------------------------
bool_t rmqsSetSocketNonBlocking(const rmqsSocket Socket)
{
    #if _WIN32 || _WIN64
    unsigned long ulNonBlocking = 1;

    if (ioctlsocket(Socket, FIONBIO, &ulNonBlocking) == -1)
    {
        return 0;
    }
    #else
    int32_t Flags;

    Flags = fcntl(Socket, F_GETFL, 0);

    if (fcntl(Socket, F_SETFL, Flags | O_NONBLOCK) == -1)
    {
        return 0;
    }
    #endif

    return 1;
}
//--------------------------------------------------------------------------
bool_t rmqsSetKeepAlive(const rmqsSocket Socket)
{
    #if _WIN32 || _WIN64
    TcpKeepAlive Alive;
    DWORD dwBytesReturned;

    Alive.onoff = TRUE;
    Alive.keepalivetime = 5000;
    Alive.keepaliveinterval = 1000;

    if (WSAIoctl(Socket, SIO_KEEPALIVE_VALS, &Alive, sizeof(Alive), 0, 0, &dwBytesReturned, 0, 0) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    #else
    int32_t OptVal;
    socklen_t OptValLen = sizeof(OptVal);

    OptVal = 1;

    if (setsockopt(Socket, SOL_SOCKET, SO_KEEPALIVE, &OptVal, OptValLen) == -1)
    {
        return 0;
    }

    OptVal = 1;

    if (setsockopt(Socket, SOL_TCP, TCP_KEEPIDLE, &OptVal, OptValLen) == -1)
    {
        return 0;
    }

    OptVal = 3;

    if (setsockopt(Socket, SOL_TCP, TCP_KEEPCNT, &OptVal, OptValLen) == -1)
    {
        return 0;
    }

    OptVal = 2;

    if (setsockopt(Socket, SOL_TCP, TCP_KEEPINTVL, &OptVal, OptValLen) == -1)
    {
        return 0;
    }

    return 1;
    #endif
}
//---------------------------------------------------------------------------
bool_t rmqsSetTcpNoDelay(const rmqsSocket Socket)
{
    int32_t iNoTcpDelayOptVal = 1, iNoTcpDelayOptValLen = sizeof(iNoTcpDelayOptVal);

    if (setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (const char_t *)&iNoTcpDelayOptVal, (rmqsSocketLen)iNoTcpDelayOptValLen) == -1)
    {
        return 0;
    }

    return 1;
}
//---------------------------------------------------------------------------
bool_t rmqsNetworkError(void)
{
    bool_t Result;

    #if _WIN32 || _WIN64
    unsigned long Error = GetLastError();

    if (Error == WSAECONNRESET || Error == WSAENETDOWN || Error == WSAENETUNREACH || Error == WSAENETRESET)
    {
        Result = true;
    }
    else
    {
        Result = false;
    }
    #else
    if (errno == ECONNRESET)
    {
        Result = true;
    }
    else
    {
        Result = false;
    }
    #endif

    return Result;
}
//---------------------------------------------------------------------------
