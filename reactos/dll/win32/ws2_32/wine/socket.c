/*
 * based on Windows Sockets 1.1 specs
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
 * Copyright (C) 2005 Marcus Meissner
 * Copyright (C) 2006-2008 Kai Blin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTE: If you make any changes to fix a particular app, make sure
 * they don't break something else like Netscape or telnet and ftp
 * clients and servers (www.winsite.com got a lot of those).
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <ws2tcpip.h>

static inline void FreeAddrInfoW_(struct addrinfoW *pAddrInfo)
{
    freeaddrinfo((struct addrinfo *)pAddrInfo);
}
#define FreeAddrInfoW FreeAddrInfoW_

static struct addrinfoW *addrinfo_AtoW(const struct addrinfo *ai)
{
    struct addrinfoW *ret;

    if (!(ret = HeapAlloc(GetProcessHeap(), 0, sizeof(struct addrinfoW)))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, ai->ai_canonname, -1, NULL, 0);
        if (!(ret->ai_canonname = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len);
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc(GetProcessHeap(), 0, ai->ai_addrlen)))
        {
            HeapFree(GetProcessHeap(), 0, ret->ai_canonname);
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        memcpy(ret->ai_addr, ai->ai_addr, ai->ai_addrlen);
    }
    return ret;
}

static struct addrinfoW *addrinfo_list_AtoW(const struct addrinfo *info)
{
    struct addrinfoW *ret, *infoW;

    if (!(ret = infoW = addrinfo_AtoW(info))) return NULL;
    while (info->ai_next)
    {
        if (!(infoW->ai_next = addrinfo_AtoW(info->ai_next)))
        {
            FreeAddrInfoW(ret);
            return NULL;
        }
        infoW = infoW->ai_next;
        info = info->ai_next;
    }
    return ret;
}

static struct addrinfo *addrinfo_WtoA(const struct addrinfoW *ai)
{
    struct addrinfo *ret;

    if (!(ret = HeapAlloc(GetProcessHeap(), 0, sizeof(struct addrinfo)))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ai->ai_canonname, -1, NULL, 0, NULL, NULL);
        if (!(ret->ai_canonname = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        WideCharToMultiByte(CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len, NULL, NULL);
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sockaddr))))
        {
            HeapFree(GetProcessHeap(), 0, ret->ai_canonname);
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        memcpy(ret->ai_addr, ai->ai_addr, sizeof(struct sockaddr));
    }
    return ret;
}

/***********************************************************************
 *		GetAddrInfoW		(WS2_32.@)
 */
int WINAPI GetAddrInfoW(LPCWSTR nodename, LPCWSTR servname, const ADDRINFOW *hints, PADDRINFOW *res)
{
    int ret, len;
    char *nodenameA = NULL, *servnameA = NULL;
    struct addrinfo *resA, *hintsA = NULL;

    *res = NULL;
    if (nodename)
    {
        len = WideCharToMultiByte(CP_ACP, 0, nodename, -1, NULL, 0, NULL, NULL);
        if (!(nodenameA = HeapAlloc(GetProcessHeap(), 0, len))) return EAI_MEMORY;
        WideCharToMultiByte(CP_ACP, 0, nodename, -1, nodenameA, len, NULL, NULL);
    }
    if (servname)
    {
        len = WideCharToMultiByte(CP_ACP, 0, servname, -1, NULL, 0, NULL, NULL);
        if (!(servnameA = HeapAlloc(GetProcessHeap(), 0, len)))
        {
            HeapFree(GetProcessHeap(), 0, nodenameA);
            return EAI_MEMORY;
        }
        WideCharToMultiByte(CP_ACP, 0, servname, -1, servnameA, len, NULL, NULL);
    }

    if (hints) hintsA = addrinfo_WtoA(hints);
    ret = getaddrinfo(nodenameA, servnameA, hintsA, &resA);
    freeaddrinfo(hintsA);

    if (!ret)
    {
        *res = addrinfo_list_AtoW(resA);
        freeaddrinfo(resA);
    }

    HeapFree(GetProcessHeap(), 0, nodenameA);
    HeapFree(GetProcessHeap(), 0, servnameA);
    return ret;
}

int WINAPI GetNameInfoW(const SOCKADDR *sa, socklen_t salen, PWCHAR host,
                        DWORD hostlen, PWCHAR serv, DWORD servlen, INT flags)
{
    int ret;
    char *hostA = NULL, *servA = NULL;

    if (host && (!(hostA = HeapAlloc(GetProcessHeap(), 0, hostlen)))) return EAI_MEMORY;
    if (serv && (!(servA = HeapAlloc(GetProcessHeap(), 0, servlen))))
    {
        HeapFree(GetProcessHeap(), 0, hostA);
        return EAI_MEMORY;
    }

    ret = getnameinfo(sa, salen, hostA, hostlen, servA, servlen, flags);
    if (!ret)
    {
        if (host) MultiByteToWideChar(CP_ACP, 0, hostA, -1, host, hostlen);
        if (serv) MultiByteToWideChar(CP_ACP, 0, servA, -1, serv, servlen);
    }

    HeapFree(GetProcessHeap(), 0, hostA);
    HeapFree(GetProcessHeap(), 0, servA);
    return ret;
}
