/* Async WINSOCK DNS services
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 1999 Marcus Meissner
 * Copyright (C) 2009 Alexandre Julliard
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
 *
 * FIXME:
 *	- Add WSACancel* and correct handle management. (works rather well for
 *	  now without it.)
 *	- Verify & Check all calls for correctness
 *	  (currently only WSAGetHostByName*, WSAGetServByPort* calls)
 *	- Check error returns.
 *	- mirc/mirc32 Finger @linux.kernel.org sometimes fails in threaded mode.
 *	  (not sure why)
 *	- This implementation did ignore the "NOTE:" section above (since the
 *	  whole stuff did not work anyway to other changes).
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <wine/config.h>

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winsock2.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winsock);


struct async_query_header
{
    HWND   hWnd;
    UINT   uMsg;
    void  *sbuf;
    INT    sbuflen;
    HANDLE handle;
};

struct async_query_gethostbyname
{
    struct async_query_header query;
    char *host_name;
};

struct async_query_gethostbyaddr
{
    struct async_query_header query;
    char *host_addr;
    int   host_len;
    int   host_type;
};

struct async_query_getprotobyname
{
    struct async_query_header query;
    char *proto_name;
};

struct async_query_getprotobynumber
{
    struct async_query_header query;
    int   proto_number;
};

struct async_query_getservbyname
{
    struct async_query_header query;
    char *serv_name;
    char *serv_proto;
};

struct async_query_getservbyport
{
    struct async_query_header query;
    char *serv_proto;
    int   serv_port;
};


/* ----------------------------------- helper functions - */

static int list_size(char** l, int item_size)
{
  int i,j = 0;
  if(l)
  { for(i=0;l[i];i++)
	j += (item_size) ? item_size : strlen(l[i]) + 1;
    j += (i + 1) * sizeof(char*); }
  return j;
}

static int list_dup(char** l_src, char* ref, int item_size)
{
   char*		p = ref;
   char**		l_to = (char**)ref;
   int			i,j,k;

   for(j=0;l_src[j];j++) ;
   p += (j + 1) * sizeof(char*);
   for(i=0;i<j;i++)
   { l_to[i] = p;
     k = ( item_size ) ? item_size : strlen(l_src[i]) + 1;
     memcpy(p, l_src[i], k); p += k; }
   l_to[i] = NULL;
   return (p - ref);
}

static DWORD finish_query( struct async_query_header *query, LPARAM lparam )
{
    PostMessageW( query->hWnd, query->uMsg, (WPARAM)query->handle, lparam );
    HeapFree( GetProcessHeap(), 0, query );
    return 0;
}

/* ----- hostent */

static LPARAM copy_he(void *base, int size, const struct hostent *he)
{
    char *p;
    int needed;
    struct hostent *to = base;

    if (!he) return MAKELPARAM( 0, GetLastError() );

    needed = sizeof(struct hostent) + strlen(he->h_name) + 1 +
                 list_size(he->h_aliases, 0) +
                 list_size(he->h_addr_list, he->h_length );
    if (size < needed) return MAKELPARAM( needed, WSAENOBUFS );

    to->h_addrtype = he->h_addrtype;
    to->h_length = he->h_length;
    p = (char *)(to + 1);
    to->h_name = p;
    strcpy(p, he->h_name); p += strlen(p) + 1;
    to->h_aliases = (char **)p;
    p += list_dup(he->h_aliases, p, 0);
    to->h_addr_list = (char **)p;
    list_dup(he->h_addr_list, p, he->h_length);
    return MAKELPARAM( needed, 0 );
}

static DWORD WINAPI async_gethostbyname(LPVOID arg)
{
    struct async_query_gethostbyname *aq = arg;
    struct hostent *he = gethostbyname( aq->host_name );

    return finish_query( &aq->query, copy_he( aq->query.sbuf, aq->query.sbuflen, he ));
}

static DWORD WINAPI async_gethostbyaddr(LPVOID arg)
{
    struct async_query_gethostbyaddr *aq = arg;
    struct hostent *he = gethostbyaddr( aq->host_addr, aq->host_len, aq->host_type );

    return finish_query( &aq->query, copy_he( aq->query.sbuf, aq->query.sbuflen, he ));
}

/* ----- protoent */

static LPARAM copy_pe(void *base, int size, const struct protoent* pe)
{
    char *p;
    int needed;
    struct protoent *to = base;

    if (!pe) return MAKELPARAM( 0, GetLastError() );

    needed = sizeof(struct protoent) + strlen(pe->p_name) + 1 + list_size(pe->p_aliases, 0);
    if (size < needed) return MAKELPARAM( needed, WSAENOBUFS );

    to->p_proto = pe->p_proto;
    p = (char *)(to + 1);
    to->p_name = p;
    strcpy(p, pe->p_name); p += strlen(p) + 1;
    to->p_aliases = (char **)p;
    list_dup(pe->p_aliases, p, 0);
    return MAKELPARAM( needed, 0 );
}

static DWORD WINAPI async_getprotobyname(LPVOID arg)
{
    struct async_query_getprotobyname *aq = arg;
    struct protoent *pe = getprotobyname( aq->proto_name );

    return finish_query( &aq->query, copy_pe( aq->query.sbuf, aq->query.sbuflen, pe ));
}

static DWORD WINAPI async_getprotobynumber(LPVOID arg)
{
    struct async_query_getprotobynumber *aq = arg;
    struct protoent *pe = getprotobynumber( aq->proto_number );

    return finish_query( &aq->query, copy_pe( aq->query.sbuf, aq->query.sbuflen, pe ));
}

/* ----- servent */

static LPARAM copy_se(void *base, int size, const struct servent* se)
{
    char *p;
    int needed;
    struct servent *to = base;

    if (!se) return MAKELPARAM( 0, GetLastError() );

    needed = sizeof(struct servent) + strlen(se->s_proto) + strlen(se->s_name) + 2 + list_size(se->s_aliases, 0);
    if (size < needed) return MAKELPARAM( needed, WSAENOBUFS );

    to->s_port = se->s_port;
    p = (char *)(to + 1);
    to->s_name = p;
    strcpy(p, se->s_name); p += strlen(p) + 1;
    to->s_proto = p;
    strcpy(p, se->s_proto); p += strlen(p) + 1;
    to->s_aliases = (char **)p;
    list_dup(se->s_aliases, p, 0);
    return MAKELPARAM( needed, 0 );
}

static DWORD WINAPI async_getservbyname(LPVOID arg)
{
    struct async_query_getservbyname *aq = arg;
    struct servent *se = getservbyname( aq->serv_name, aq->serv_proto );

    return finish_query( &aq->query, copy_se( aq->query.sbuf, aq->query.sbuflen, se ));
}

static DWORD WINAPI async_getservbyport(LPVOID arg)
{
    struct async_query_getservbyport *aq = arg;
    struct servent *se = getservbyport( aq->serv_port, aq->serv_proto );

    return finish_query( &aq->query, copy_se( aq->query.sbuf, aq->query.sbuflen, se ));
}


/****************************************************************************
 * The main async help function.
 *
 * It either starts a thread or just calls the function directly for platforms
 * with no thread support. This relies on the fact that PostMessage() does
 * not actually call the windowproc before the function returns.
 */
static HANDLE run_query( HWND hWnd, UINT uMsg, LPTHREAD_START_ROUTINE func,
                         struct async_query_header *query, void *sbuf, INT sbuflen )
{
    static LONG next_handle = 0xdead;
    HANDLE thread;
    ULONG handle;
    do
        handle = LOWORD( InterlockedIncrement( &next_handle ));
    while (!handle); /* avoid handle 0 */

    query->hWnd    = hWnd;
    query->uMsg    = uMsg;
    query->handle  = UlongToHandle( handle );
    query->sbuf    = sbuf;
    query->sbuflen = sbuflen;

    thread = CreateThread( NULL, 0, func, query, 0, NULL );
    if (!thread)
    {
        SetLastError( WSAEWOULDBLOCK );
        HeapFree( GetProcessHeap(), 0, query );
        return 0;
    }
    CloseHandle( thread );
    return UlongToHandle( handle );
}


/***********************************************************************
 *       WSAAsyncGetHostByAddr        (WS2_32.102)
 */
HANDLE WINAPI WSAAsyncGetHostByAddr(HWND hWnd, UINT uMsg, LPCSTR addr,
                               INT len, INT type, LPSTR sbuf, INT buflen)
{
    struct async_query_gethostbyaddr *aq;

    TRACE("hwnd %p, msg %04x, addr %p[%i]\n", hWnd, uMsg, addr, len );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_addr = (char *)(aq + 1);
    aq->host_len  = len;
    aq->host_type = type;
    memcpy( aq->host_addr, addr, len );
    return run_query( hWnd, uMsg, async_gethostbyaddr, &aq->query, sbuf, buflen );
}

/***********************************************************************
 *       WSAAsyncGetHostByName	(WS2_32.103)
 */
HANDLE WINAPI WSAAsyncGetHostByName(HWND hWnd, UINT uMsg, LPCSTR name,
					LPSTR sbuf, INT buflen)
{
    struct async_query_gethostbyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %p, msg %04x, host %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_name = (char *)(aq + 1);
    strcpy( aq->host_name, name );
    return run_query( hWnd, uMsg, async_gethostbyname, &aq->query, sbuf, buflen );
}

/***********************************************************************
 *       WSAAsyncGetProtoByName       (WS2_32.105)
 */
HANDLE WINAPI WSAAsyncGetProtoByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                         LPSTR sbuf, INT buflen)
{
    struct async_query_getprotobyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %p, msg %04x, proto %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_name = (char *)(aq + 1);
    strcpy( aq->proto_name, name );
    return run_query( hWnd, uMsg, async_getprotobyname, &aq->query, sbuf, buflen );
}


/***********************************************************************
 *       WSAAsyncGetProtoByNumber     (WS2_32.104)
 */
HANDLE WINAPI WSAAsyncGetProtoByNumber(HWND hWnd, UINT uMsg, INT number,
                                           LPSTR sbuf, INT buflen)
{
    struct async_query_getprotobynumber *aq;

    TRACE("hwnd %p, msg %04x, num %i\n", hWnd, uMsg, number );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_number = number;
    return run_query( hWnd, uMsg, async_getprotobynumber, &aq->query, sbuf, buflen );
}

/***********************************************************************
 *       WSAAsyncGetServByName        (WS2_32.107)
 */
HANDLE WINAPI WSAAsyncGetServByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
    struct async_query_getservbyname *aq;
    unsigned int len1 = strlen(name) + 1;
    unsigned int len2 = proto ? strlen(proto) + 1 : 0;

    TRACE("hwnd %p, msg %04x, name %s, proto %s\n", hWnd, uMsg, debugstr_a(name), debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len1 + len2 )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }

    aq->serv_name  = (char *)(aq + 1);
    strcpy( aq->serv_name, name );

    if (proto)
    {
        aq->serv_proto = aq->serv_name + len1;
        strcpy( aq->serv_proto, proto );
    }
    else
        aq->serv_proto = NULL;

    return run_query( hWnd, uMsg, async_getservbyname, &aq->query, sbuf, buflen );
}

/***********************************************************************
 *       WSAAsyncGetServByPort        (WS2_32.106)
 */
HANDLE WINAPI WSAAsyncGetServByPort(HWND hWnd, UINT uMsg, INT port,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
    struct async_query_getservbyport *aq;
    unsigned int len = proto ? strlen(proto) + 1 : 0;

    TRACE("hwnd %p, msg %04x, port %i, proto %s\n", hWnd, uMsg, port, debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }

    if (proto)
    {
        aq->serv_proto = (char *)(aq + 1);
        strcpy( aq->serv_proto, proto );
    }
    else
        aq->serv_proto = NULL;

    aq->serv_port = port;

    return run_query( hWnd, uMsg, async_getservbyport, &aq->query, sbuf, buflen );
}
