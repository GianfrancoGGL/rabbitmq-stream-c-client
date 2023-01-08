//---------------------------------------------------------------------------
#ifdef __WIN32__
#include <winsock2.h>
#else

#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <strings.h>
#include <netdb.h>
#include <fcntl.h>

#endif
//---------------------------------------------------------------------------
#include "rmqsNetwork.h"
//---------------------------------------------------------------------------
#ifdef __WIN32__
#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)
typedef struct
{
    u_long onoff;
    u_long keepalivetime;
    u_long keepaliveinterval;
}
TcpKeepAlive;
#endif
//---------------------------------------------------------------------------
#ifdef __WIN32__
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
#ifdef __WIN32__
void rmqsShutdownWinsock(void)
{
    WSACleanup();
}
#endif

//---------------------------------------------------------------------------
rmqsSocket rmqsSocketCreate(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

//---------------------------------------------------------------------------
void rmqsSocketDestroy(rmqsSocket *Socket) {
#ifndef __WIN32__
    shutdown(*Socket, SHUT_RDWR);
    close(*Socket);
#else
    shutdown(*Socket, SD_BOTH);
    closesocket(*Socket);
#endif

    *Socket = rmqsInvalidSocket;
}

//---------------------------------------------------------------------------
uint8_t rmqsConnect(const char *Host, const uint16_t Port, const rmqsSocket Socket, const uint32_t TimeoutMs) {
    uint8_t Connected = 0;
    struct hostent *pHost;
    struct sockaddr_in ServerAddress;
#ifdef __WIN32__
    fd_set FDS;
    TIMEVAL Timeout;
    int32_t Result;
#endif

    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(Port);

#ifndef __WIN32__
    bzero(&(ServerAddress.sin_zero), 8);
#else
    memset(ServerAddress.sin_zero, 0, 8);
#endif

    pHost = (struct hostent *) gethostbyname(Host);

    ServerAddress.sin_addr = *(struct in_addr *) pHost->h_addr;

#ifdef __WIN32__
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
#endif


#ifdef __LINUX__
    setsockopt(Socket, IPPROTO_TCP , TCP_USER_TIMEOUT, &TimeoutMs, sizeof(TimeoutMs));
#endif

#ifdef __APPLE__
    setsockopt(Socket, IPPROTO_TCP, TCP_CONNECTIONTIMEOUT, &TimeoutMs, sizeof(TimeoutMs));
#endif

#if defined(__LINUX__) || defined(__APPLE__)
    if (connect(Socket, (struct sockaddr *)&ServerAddress, sizeof(struct sockaddr)) == 0)
    {
        Connected = 1;
    }
#endif



    return Connected;
}

//---------------------------------------------------------------------------
void rmqsSetSocketTimeouts(const rmqsSocket Socket, const uint32_t ReadTimeout, const uint32_t WriteTimeout) {
#ifndef __WIN32__
    struct timeval TVRead, TVWrite;
    TVRead.tv_sec = 0;
    TVRead.tv_usec = ReadTimeout * 1000;

    TVWrite.tv_sec = 0;
    TVWrite.tv_usec = WriteTimeout * 1000;

    setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (const int8_t *) &TVRead, sizeof(TVRead));
    setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (const int8_t *) &TVWrite, sizeof(TVWrite));
#else
    DWORD dwRead, dwWrite;
    dwRead = ReadTimeout;
    dwWrite = WriteTimeout;

    setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&dwRead, sizeof(dwRead));
    setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&dwWrite, sizeof(dwWrite));
#endif
}

//--------------------------------------------------------------------------
uint8_t
rmqsSetSocketTxRxBuffers(const rmqsSocket Socket, const uint32_t ulTxBufferSize, const uint32_t ulRxBufferSize) {
    uint8_t Result = 1;

    if (setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (const char *) &ulTxBufferSize, sizeof(uint32_t)) == -1) {
        Result = 0;
    }

    if (setsockopt(Socket, SOL_SOCKET, SO_RCVBUF, (const char *) &ulRxBufferSize, sizeof(uint32_t)) == -1) {
        Result = 0;
    }

    return Result;
}

//---------------------------------------------------------------------------
uint8_t rmqsSetSocketBlocking(const rmqsSocket Socket) {
#ifndef __WIN32__
    int32_t Flags;

    Flags = fcntl(Socket, F_GETFL, 0);

    if (fcntl(Socket, F_SETFL, Flags & (~O_NONBLOCK)) == -1) {
        return 0;
    }
#else
    unsigned long ulNonBlocking = 0;

    if (ioctlsocket(Socket, FIONBIO, &ulNonBlocking) == -1)
    {
        return 0;
    }
#endif

    return 1;
}

//---------------------------------------------------------------------------
uint8_t rmqsSetSocketNonBlocking(const rmqsSocket Socket) {
#ifndef __WIN32__
    int32_t Flags;

    Flags = fcntl(Socket, F_GETFL, 0);

    if (fcntl(Socket, F_SETFL, Flags | O_NONBLOCK) == -1) {
        return 0;
    }
#else
    unsigned long ulNonBlocking = 1;

    if (ioctlsocket(Socket, FIONBIO, &ulNonBlocking) == -1)
    {
        return 0;
    }
#endif

    return 1;
}

//--------------------------------------------------------------------------
uint8_t rmqsSetKeepAlive(const rmqsSocket Socket) {
#ifndef __WIN32__
    int32_t OptVal;
    socklen_t OptValLen = sizeof(OptVal);

    OptVal = 1;

    if (setsockopt(Socket, SOL_SOCKET, SO_KEEPALIVE, &OptVal, OptValLen) == -1) {
        return 0;
    }

    OptVal = 1;


#ifdef __APPLE__
    if (setsockopt(Socket, SOL_SOCKET, TCP_KEEPALIVE, &OptVal, OptValLen) == -1) {
        return 0;
    }
#else
    if (setsockopt(Socket, SOL_TCP, TCP_KEEPIDLE, &OptVal, OptValLen) == -1) {
        return 0;
    }
#endif


    OptVal = 3;


#ifdef __APPLE__
    if (setsockopt(Socket, SOL_SOCKET, TCP_KEEPCNT, &OptVal, OptValLen) == -1) {
        return 0;
    }
#else
    if (setsockopt(Socket, SOL_TCP, TCP_KEEPCNT, &OptVal, OptValLen) == -1) {
        return 0;
    }
#endif


    OptVal = 2;

#ifdef __APPLE__
    if (setsockopt(Socket, SOL_SOCKET, TCP_KEEPINTVL, &OptVal, OptValLen) == -1) {
        return 0;
    }
#else
    if (setsockopt(Socket, SOL_TCP, TCP_KEEPINTVL, &OptVal, OptValLen) == -1) {
        return 0;
    }
#endif

    return 1;
#else
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
#endif
}

//---------------------------------------------------------------------------
uint8_t rmqsSetTcpNoDelay(const rmqsSocket Socket) {
    int32_t iNoTcpDelayOptVal = 1, iNoTcpDelayOptValLen = sizeof(iNoTcpDelayOptVal);

    if (setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (const char *) &iNoTcpDelayOptVal,
                   (rmqsSocketLen) iNoTcpDelayOptValLen) == -1) {
        return 0;
    }

    return 1;
}

//---------------------------------------------------------------------------
uint8_t rmqsNetworkError(void) {
    uint8_t Result = 0;

#ifdef __WIN32__
    unsigned long Error = GetLastError();

    if (Error == WSAECONNRESET || Error == WSAENETDOWN || Error == WSAENETUNREACH || Error == WSAENETRESET)
    {
        Result = 1;
    }
    else
    {
        Result = 0;
    }
#else
    if (errno == ECONNRESET) {
        Result = 1;
    } else {
        Result = 0;
    }
#endif

    return Result;
}
//---------------------------------------------------------------------------
