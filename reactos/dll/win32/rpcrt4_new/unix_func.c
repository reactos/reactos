#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include "unix_func.h"


int
poll(struct pollfd *fds,
     unsigned long nfds,
     int timo)
{
    TIMEVAL timeout, *toptr;
    FD_SET ifds, ofds, efds, *ip, *op;
    int i, rc, n = -1;

    ip = op = NULL;

    FD_ZERO(&ifds);
    FD_ZERO(&ofds);
    FD_ZERO(&efds);

    for (i = 0; i < nfds; ++i)
    {
        fds[i].revents = 0;

        if (fds[i].fd < 0)
            continue;

        if (fds[i].fd > n)
            n = fds[i].fd;

        if (fds[i].events & (POLLIN|POLLPRI))
        {
            ip = &ifds;
            FD_SET(fds[i].fd, ip);
        }

        if (fds[i].events & POLLOUT)
        {
            op = &ofds;
            FD_SET(fds[i].fd, op);
        }

        FD_SET(fds[i].fd, &efds);
    }

    if (timo < 0)
        toptr = 0;
    else
    {
        toptr = &timeout;
        timeout.tv_sec = timo / 1000;
        timeout.tv_usec = (timo - timeout.tv_sec * 1000) * 1000;
    }

    rc = select(++n, ip, op, &efds, toptr);
    
    if (rc <= 0)
        return rc;

    for (i = 0, n = 0; i < nfds; ++i)
    {
        if (fds[i].fd < 0) continue;

        if (fds[i].events & (POLLIN|POLLPRI) && FD_ISSET(i, &ifds))
            fds[i].revents |= POLLIN;

        if (fds[i].events & POLLOUT && FD_ISSET(i, &ofds))
            fds[i].revents |= POLLOUT;

        if (FD_ISSET(i, &efds))
            fds[i].revents |= POLLHUP;
    }

    return rc;
}


int socketpair(int af,
               int type,
               int protocol,
               SOCKET socks[2])
{
    struct sockaddr_in addr;
    SOCKET listener;
    int e;
    int addrlen = sizeof(addr);
    DWORD flags = 0; //(make_overlapped ? WSA_FLAG_OVERLAPPED : 0);

    if (socks == 0)
    {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    socks[0] = socks[1] = INVALID_SOCKET;
    if ((listener = socket(af, type, 0)) == INVALID_SOCKET)
        return SOCKET_ERROR;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_port = 0;

    e = bind(listener, (const struct sockaddr*) &addr, sizeof(addr));
    if (e == SOCKET_ERROR)
    {
        e = WSAGetLastError();
        closesocket(listener);
        WSASetLastError(e);
        return SOCKET_ERROR;
    }
    e = getsockname(listener, (struct sockaddr*) &addr, &addrlen);
    if (e == SOCKET_ERROR)
    {
        e = WSAGetLastError();
        closesocket(listener);
        WSASetLastError(e);
        return SOCKET_ERROR;
    }

    do
    {
        if (listen(listener, 1) == SOCKET_ERROR)
            break;
        if ((socks[0] = WSASocket(af, type, 0, NULL, 0, flags)) == INVALID_SOCKET)
            break;
        if (connect(socks[0], (const struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR)
            break;
        if ((socks[1] = accept(listener, NULL, NULL)) == INVALID_SOCKET)
            break;

        closesocket(listener);
        return 0;

    } while (0);

    e = WSAGetLastError();
    closesocket(listener);
    closesocket(socks[0]);
    closesocket(socks[1]);
    WSASetLastError(e);
    return SOCKET_ERROR;
}


const char *
inet_ntop (int af,
           const void *src,
           char *dst,
           size_t cnt)
{
    struct in_addr in;
    char *text_addr;

    if (af == AF_INET)
    {
        memcpy(&in.s_addr, src, sizeof(in.s_addr));
        text_addr = inet_ntoa(in);
        if (text_addr && dst)
        {
            strncpy(dst, text_addr, cnt);
            return dst;
        }
    }

    return 0;
}
