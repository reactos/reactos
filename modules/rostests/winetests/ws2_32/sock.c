/*
 * Unit test suite for winsock functions
 *
 * Copyright 2002 Martin Wilck
 * Copyright 2005 Thomas Kho
 * Copyright 2008 Jeff Zaroyko
 * Copyright 2017 Dmitry Timoshkov
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
 */

#ifdef __REACTOS__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winsock2.h>
#include <windows.h>
#include <winternl.h>
#include <ws2tcpip.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <mswsock.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include "wine/test.h"

// ReactOS: Wine has this in mstcpip.h, but it doesn't belong there
#define WSA_CMSG_ALIGN(len)     (((len) + sizeof(SIZE_T) - 1) & ~(sizeof(SIZE_T) - 1))

#define MAX_CLIENTS 4      /* Max number of clients */
#define FIRST_CHAR 'A'     /* First character in transferred pattern */
#define BIND_SLEEP 10      /* seconds to wait between attempts to bind() */
#define BIND_TRIES 6       /* Number of bind() attempts */
#define TEST_TIMEOUT 30    /* seconds to wait before killing child threads
                              after server initialization, if something hangs */

#define NUM_UDP_PEERS 3    /* Number of UDP sockets to create and test > 1 */

#define NUM_THREADS 3      /* Number of threads to run getservbyname */
#define NUM_QUERIES 250    /* Number of getservbyname queries per thread */

#define SERVERIP "127.0.0.1"   /* IP to bind to */
#define SERVERPORT 9374        /* Port number to bind to */

#define wsa_ok(op, cond, msg) \
   do { \
        int tmp, err = 0; \
        tmp = op; \
        if ( !(cond tmp) ) err = WSAGetLastError(); \
        ok ( cond tmp, msg, GetCurrentThreadId(), err); \
   } while (0);

#define make_keepalive(k, enable, time, interval) \
   k.onoff = enable; \
   k.keepalivetime = time; \
   k.keepaliveinterval = interval;

/* Function pointers */
static void  (WINAPI *pfreeaddrinfo)(struct addrinfo *);
static int   (WINAPI *pgetaddrinfo)(LPCSTR,LPCSTR,const struct addrinfo *,struct addrinfo **);
static void  (WINAPI *pFreeAddrInfoW)(PADDRINFOW);
static void  (WINAPI *pFreeAddrInfoExW)(ADDRINFOEXW *ai);
static int   (WINAPI *pGetAddrInfoW)(LPCWSTR,LPCWSTR,const ADDRINFOW *,PADDRINFOW *);
static int   (WINAPI *pGetAddrInfoExW)(const WCHAR *name, const WCHAR *servname, DWORD namespace,
        GUID *namespace_id, const ADDRINFOEXW *hints, ADDRINFOEXW **result,
        struct timeval *timeout, OVERLAPPED *overlapped,
        LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine, HANDLE *handle);
static int   (WINAPI *pGetAddrInfoExOverlappedResult)(OVERLAPPED *overlapped);
static PCSTR (WINAPI *p_inet_ntop)(INT,LPVOID,LPSTR,ULONG);
static PCWSTR(WINAPI *pInetNtopW)(INT,LPVOID,LPWSTR,ULONG);
static int   (WINAPI *p_inet_pton)(INT,LPCSTR,LPVOID);
static int   (WINAPI *pInetPtonW)(INT,LPWSTR,LPVOID);
static int   (WINAPI *pWSALookupServiceBeginW)(LPWSAQUERYSETW,DWORD,LPHANDLE);
static int   (WINAPI *pWSALookupServiceEnd)(HANDLE);
static int   (WINAPI *pWSALookupServiceNextW)(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETW);
static int   (WINAPI *pWSAEnumNameSpaceProvidersA)(LPDWORD,LPWSANAMESPACE_INFOA);
static int   (WINAPI *pWSAEnumNameSpaceProvidersW)(LPDWORD,LPWSANAMESPACE_INFOW);
static int   (WINAPI *pWSAPoll)(WSAPOLLFD *,ULONG,INT);

/* Function pointers from ntdll */
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

/* Function pointers from iphlpapi */
static DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO,PULONG);
static DWORD (WINAPI *pGetIpForwardTable)(PMIB_IPFORWARDTABLE,PULONG,BOOL);

/* Function pointers from ntdll */
static DWORD (WINAPI *pNtClose)(HANDLE);

/**************** Structs and typedefs ***************/

typedef struct thread_info
{
    HANDLE thread;
    DWORD id;
} thread_info;

/* Information in the server about open client connections */
typedef struct sock_info
{
    SOCKET                 s;
    struct sockaddr_in     addr;
    struct sockaddr_in     peer;
    char                  *buf;
    int                    n_recvd;
    int                    n_sent;
} sock_info;

/* Test parameters for both server & client */
typedef struct test_params
{
    int          sock_type;
    int          sock_prot;
    const char  *inet_addr;
    short        inet_port;
    int          chunk_size;
    int          n_chunks;
    int          n_clients;
} test_params;

/* server-specific test parameters */
typedef struct server_params
{
    test_params   *general;
    DWORD          sock_flags;
    int            buflen;
} server_params;

/* client-specific test parameters */
typedef struct client_params
{
    test_params   *general;
    DWORD          sock_flags;
    int            buflen;
} client_params;

/* This type combines all information for setting up a test scenario */
typedef struct test_setup
{
    test_params              general;
    LPVOID                   srv;
    server_params            srv_params;
    LPVOID                   clt;
    client_params            clt_params;
} test_setup;

/* Thread local storage for server */
typedef struct server_memory
{
    SOCKET                  s;
    struct sockaddr_in      addr;
    sock_info               sock[MAX_CLIENTS];
} server_memory;

/* Thread local storage for client */
typedef struct client_memory
{
    SOCKET s;
    struct sockaddr_in      addr;
    char                   *send_buf;
    char                   *recv_buf;
} client_memory;

/* SelectReadThread thread parameters */
typedef struct select_thread_params
{
    SOCKET s;
    BOOL ReadKilled;
} select_thread_params;

/* Tests used in both getaddrinfo and GetAddrInfoW */
static const struct addr_hint_tests
{
    int family, socktype, protocol;
    DWORD error;
} hinttests[] = {
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0 },
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_UDP, 0 },
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_IPV6,0 },
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_TCP, 0 },
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_UDP, 0 },
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_IPV6,0 },
    {AF_INET,   SOCK_STREAM, IPPROTO_TCP, 0 },
    {AF_INET,   SOCK_STREAM, IPPROTO_UDP, 0 },
    {AF_INET,   SOCK_STREAM, IPPROTO_IPV6,0 },
    {AF_INET,   SOCK_DGRAM,  IPPROTO_TCP, 0 },
    {AF_INET,   SOCK_DGRAM,  IPPROTO_UDP, 0 },
    {AF_INET,   SOCK_DGRAM,  IPPROTO_IPV6,0 },
    {AF_UNSPEC, 0,           IPPROTO_TCP, 0 },
    {AF_UNSPEC, 0,           IPPROTO_UDP, 0 },
    {AF_UNSPEC, 0,           IPPROTO_IPV6,0 },
    {AF_UNSPEC, SOCK_STREAM, 0,           0 },
    {AF_UNSPEC, SOCK_DGRAM,  0,           0 },
    {AF_INET,   0,           IPPROTO_TCP, 0 },
    {AF_INET,   0,           IPPROTO_UDP, 0 },
    {AF_INET,   0,           IPPROTO_IPV6,0 },
    {AF_INET,   SOCK_STREAM, 0,           0 },
    {AF_INET,   SOCK_DGRAM,  0,           0 },
    {AF_UNSPEC, 999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT },
    {AF_UNSPEC, 999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT },
    {AF_UNSPEC, 999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT },
    {AF_INET,   999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT },
    {AF_INET,   999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT },
    {AF_INET,   999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT },
    {AF_UNSPEC, SOCK_STREAM, 999,         0 },
    {AF_UNSPEC, SOCK_STREAM, 999,         0 },
    {AF_INET,   SOCK_DGRAM,  999,         0 },
    {AF_INET,   SOCK_DGRAM,  999,         0 },
};

/**************** Static variables ***************/

static DWORD      tls;              /* Thread local storage index */
static HANDLE     thread[1+MAX_CLIENTS];
static DWORD      thread_id[1+MAX_CLIENTS];
static HANDLE     server_ready;
static HANDLE     client_ready[MAX_CLIENTS];
static int        client_id;

/**************** General utility functions ***************/

static SOCKET setup_server_socket(struct sockaddr_in *addr, int *len);
static SOCKET setup_connector_socket(struct sockaddr_in *addr, int len, BOOL nonblock);

static int tcp_socketpair(SOCKET *src, SOCKET *dst)
{
    SOCKET server = INVALID_SOCKET;
    struct sockaddr_in addr;
    int len;
    int ret;

    *src = INVALID_SOCKET;
    *dst = INVALID_SOCKET;

    *src = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*src == INVALID_SOCKET)
        goto end;

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET)
        goto end;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(server, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0)
        goto end;

    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr*)&addr, &len);
    if (ret != 0)
        goto end;

    ret = listen(server, 1);
    if (ret != 0)
        goto end;

    ret = connect(*src, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0)
        goto end;

    len = sizeof(addr);
    *dst = accept(server, (struct sockaddr*)&addr, &len);

end:
    if (server != INVALID_SOCKET)
        closesocket(server);
    if (*src != INVALID_SOCKET && *dst != INVALID_SOCKET)
        return 0;
    closesocket(*src);
    closesocket(*dst);
    return -1;
}

static int tcp_socketpair_ovl(SOCKET *src, SOCKET *dst)
{
    SOCKET server = INVALID_SOCKET;
    struct sockaddr_in addr;
    int len, ret;

    *src = INVALID_SOCKET;
    *dst = INVALID_SOCKET;

    *src = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (*src == INVALID_SOCKET)
        goto end;

    server = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (server == INVALID_SOCKET)
        goto end;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0)
        goto end;

    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr *)&addr, &len);
    if (ret != 0)
        goto end;

    ret = listen(server, 1);
    if (ret != 0)
        goto end;

    ret = connect(*src, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0)
        goto end;

    len = sizeof(addr);
    *dst = accept(server, (struct sockaddr *)&addr, &len);

end:
    if (server != INVALID_SOCKET)
        closesocket(server);
    if (*src != INVALID_SOCKET && *dst != INVALID_SOCKET)
        return 0;
    closesocket(*src);
    closesocket(*dst);
    return -1;
}

static void set_so_opentype ( BOOL overlapped )
{
    int optval = !overlapped, newval, len = sizeof (int);

    ok ( setsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
                      (LPVOID) &optval, sizeof (optval) ) == 0,
         "setting SO_OPENTYPE failed\n" );
    ok ( getsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
                      (LPVOID) &newval, &len ) == 0,
         "getting SO_OPENTYPE failed\n" );
    ok ( optval == newval, "failed to set SO_OPENTYPE\n" );
}

static int set_blocking ( SOCKET s, BOOL blocking )
{
    u_long val = !blocking;
    return ioctlsocket ( s, FIONBIO, &val );
}

static void fill_buffer ( char *buf, int chunk_size, int n_chunks )
{
    char c, *p;
    for ( c = FIRST_CHAR, p = buf; c < FIRST_CHAR + n_chunks; c++, p += chunk_size )
        memset ( p, c, chunk_size );
}

static int test_buffer ( char *buf, int chunk_size, int n_chunks )
{
    char c, *p;
    int i;
    for ( c = FIRST_CHAR, p = buf; c < FIRST_CHAR + n_chunks; c++, p += chunk_size )
    {
        for ( i = 0; i < chunk_size; i++ )
            if ( p[i] != c ) return i;
    }
    return -1;
}

/*
 * This routine is called when a client / server does not expect any more data,
 * but needs to acknowledge the closing of the connection (by reading 0 bytes).
 */
static void read_zero_bytes ( SOCKET s )
{
    char buf[256];
    int tmp, n = 0;
    while ( ( tmp = recv ( s, buf, 256, 0 ) ) > 0 )
        n += tmp;
    ok ( n <= 0, "garbage data received: %d bytes\n", n );
}

static int do_synchronous_send ( SOCKET s, char *buf, int buflen, int flags, int sendlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; )
    {
        n = send ( s, p, min ( sendlen, last - p ), flags );
        if (n > 0) p += n;
    }
    wsa_ok ( n, 0 <=, "do_synchronous_send (%x): error %d\n" );
    return p - buf;
}

static int do_synchronous_recv ( SOCKET s, char *buf, int buflen, int flags, int recvlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; )
    {
        n = recv ( s, p, min ( recvlen, last - p ), flags );
        if (n > 0) p += n;
    }
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%x): error %d:\n" );
    return p - buf;
}

static int do_synchronous_recvfrom ( SOCKET s, char *buf, int buflen, int flags, struct sockaddr *from, int *fromlen, int recvlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; )
    {
        n = recvfrom ( s, p, min ( recvlen, last - p ), flags, from, fromlen );
        if (n > 0) p += n;
    }
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%x): error %d:\n" );
    return p - buf;
}

/*
 *  Call this routine right after thread startup.
 *  SO_OPENTYPE must by 0, regardless what the server did.
 */
static void check_so_opentype (void)
{
    int tmp = 1, len;
    len = sizeof (tmp);
    getsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (LPVOID) &tmp, &len );
    ok ( tmp == 0, "check_so_opentype: wrong startup value of SO_OPENTYPE: %d\n", tmp );
}

static void compare_addrinfo (ADDRINFO *a, ADDRINFO *b)
{
    for (; a && b ; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %lu != %lu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!strcmp(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               a->ai_canonname, b->ai_canonname);
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

static void compare_addrinfow (ADDRINFOW *a, ADDRINFOW *b)
{
    for (; a && b ; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %lu != %lu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!lstrcmpW(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               wine_dbgstr_w(a->ai_canonname), wine_dbgstr_w(b->ai_canonname));
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

/**************** Server utility functions ***************/

/*
 *  Even if we have closed our server socket cleanly,
 *  the OS may mark the address "in use" for some time -
 *  this happens with native Linux apps, too.
 */
static void do_bind ( SOCKET s, struct sockaddr* addr, int addrlen )
{
    int err, wsaerr = 0, n_try = BIND_TRIES;

    while ( ( err = bind ( s, addr, addrlen ) ) != 0 &&
            ( wsaerr = WSAGetLastError () ) == WSAEADDRINUSE &&
            n_try-- >= 0)
    {
        trace ( "address in use, waiting ...\n" );
        Sleep ( 1000 * BIND_SLEEP );
    }
    ok ( err == 0, "failed to bind: %d\n", wsaerr );
}

static void server_start ( server_params *par )
{
    int i;
    test_params *gen = par->general;
    server_memory *mem = LocalAlloc ( LPTR, sizeof ( server_memory ) );

    TlsSetValue ( tls, mem );
    mem->s = WSASocketA ( AF_INET, gen->sock_type, gen->sock_prot,
                          NULL, 0, par->sock_flags );
    ok ( mem->s != INVALID_SOCKET, "Server: WSASocket failed\n" );

    mem->addr.sin_family = AF_INET;
    mem->addr.sin_addr.s_addr = inet_addr ( gen->inet_addr );
    mem->addr.sin_port = htons ( gen->inet_port );

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        mem->sock[i].s = INVALID_SOCKET;
        mem->sock[i].buf = LocalAlloc ( LPTR, gen->n_chunks * gen->chunk_size );
        mem->sock[i].n_recvd = 0;
        mem->sock[i].n_sent = 0;
    }

    if ( gen->sock_type == SOCK_STREAM )
        do_bind ( mem->s, (struct sockaddr*) &mem->addr, sizeof (mem->addr) );
}

static void server_stop (void)
{
    int i;
    server_memory *mem = TlsGetValue ( tls );

    for (i = 0; i < MAX_CLIENTS; i++ )
    {
        LocalFree ( mem->sock[i].buf );
        if ( mem->sock[i].s != INVALID_SOCKET )
            closesocket ( mem->sock[i].s );
    }
    ok ( closesocket ( mem->s ) == 0, "closesocket failed\n" );
    LocalFree ( mem );
    ExitThread ( GetCurrentThreadId () );
}

/**************** Client utilitiy functions ***************/

static void client_start ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem = LocalAlloc (LPTR, sizeof (client_memory));

    TlsSetValue ( tls, mem );

    WaitForSingleObject ( server_ready, INFINITE );

    mem->s = WSASocketA ( AF_INET, gen->sock_type, gen->sock_prot,
                          NULL, 0, par->sock_flags );

    mem->addr.sin_family = AF_INET;
    mem->addr.sin_addr.s_addr = inet_addr ( gen->inet_addr );
    mem->addr.sin_port = htons ( gen->inet_port );

    ok ( mem->s != INVALID_SOCKET, "Client: WSASocket failed\n" );

    mem->send_buf = LocalAlloc ( LPTR, 2 * gen->n_chunks * gen->chunk_size );
    mem->recv_buf = mem->send_buf + gen->n_chunks * gen->chunk_size;
    fill_buffer ( mem->send_buf, gen->chunk_size, gen->n_chunks );

    SetEvent ( client_ready[client_id] );
    /* Wait for the other clients to come up */
    WaitForMultipleObjects ( min ( gen->n_clients, MAX_CLIENTS ), client_ready, TRUE, INFINITE );
}

static void client_stop (void)
{
    client_memory *mem = TlsGetValue ( tls );
    wsa_ok ( closesocket ( mem->s ), 0 ==, "closesocket error (%x): %d\n" );
    LocalFree ( mem->send_buf );
    LocalFree ( mem );
    ExitThread(0);
}

/**************** Servers ***************/

/*
 * simple_server: A very basic server doing synchronous IO.
 */
static VOID WINAPI simple_server ( server_params *par )
{
    test_params *gen = par->general;
    server_memory *mem;
    int pos, n_recvd, n_sent, n_expected = gen->n_chunks * gen->chunk_size, tmp, i,
        id = GetCurrentThreadId();

    trace ( "simple_server (%x) starting\n", id );

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, TRUE ), 0 ==, "simple_server (%x): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "simple_server (%x): listen failed: %d\n");

    trace ( "simple_server (%x) ready\n", id );
    SetEvent ( server_ready ); /* notify clients */

    for ( i = 0; i < min ( gen->n_clients, MAX_CLIENTS ); i++ )
    {
        trace ( "simple_server (%x): waiting for client\n", id );

        /* accept a single connection */
        tmp = sizeof ( mem->sock[0].peer );
        mem->sock[0].s = accept ( mem->s, (struct sockaddr*) &mem->sock[0].peer, &tmp );
        wsa_ok ( mem->sock[0].s, INVALID_SOCKET !=, "simple_server (%x): accept failed: %d\n" );

        ok ( mem->sock[0].peer.sin_addr.s_addr == inet_addr ( gen->inet_addr ),
             "simple_server (%x): strange peer address\n", id );

        /* Receive data & check it */
        n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );
        ok ( n_recvd == n_expected,
             "simple_server (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
        pos = test_buffer ( mem->sock[0].buf, gen->chunk_size, gen->n_chunks );
        ok ( pos == -1, "simple_server (%x): test pattern error: %d\n", id, pos);

        /* Echo data back */
        n_sent = do_synchronous_send ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );
        ok ( n_sent == n_expected,
             "simple_server (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

        /* cleanup */
        read_zero_bytes ( mem->sock[0].s );
        wsa_ok ( closesocket ( mem->sock[0].s ),  0 ==, "simple_server (%x): closesocket error: %d\n" );
        mem->sock[0].s = INVALID_SOCKET;
    }

    trace ( "simple_server (%x) exiting\n", id );
    server_stop ();
}

/*
 * oob_server: A very basic server receiving out-of-band data.
 */
static VOID WINAPI oob_server ( server_params *par )
{
    test_params *gen = par->general;
    server_memory *mem;
    u_long atmark = 0;
    int pos, n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, tmp,
        id = GetCurrentThreadId();

    trace ( "oob_server (%x) starting\n", id );

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, TRUE ), 0 ==, "oob_server (%x): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "oob_server (%x): listen failed: %d\n");

    trace ( "oob_server (%x) ready\n", id );
    SetEvent ( server_ready ); /* notify clients */

    trace ( "oob_server (%x): waiting for client\n", id );

    /* accept a single connection */
    tmp = sizeof ( mem->sock[0].peer );
    mem->sock[0].s = accept ( mem->s, (struct sockaddr*) &mem->sock[0].peer, &tmp );
    wsa_ok ( mem->sock[0].s, INVALID_SOCKET !=, "oob_server (%x): accept failed: %d\n" );

    ok ( mem->sock[0].peer.sin_addr.s_addr == inet_addr ( gen->inet_addr ),
         "oob_server (%x): strange peer address\n", id );

    /* check initial atmark state */
    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    ok ( atmark == 1, "oob_server (%x): unexpectedly at the OOB mark: %i\n", id, atmark );

    /* Receive normal data */
    n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );
    ok ( n_recvd == n_expected,
         "oob_server (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
    pos = test_buffer ( mem->sock[0].buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "oob_server (%x): test pattern error: %d\n", id, pos);

    /* check atmark state */
    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    ok ( atmark == 1, "oob_server (%x): unexpectedly at the OOB mark: %i\n", id, atmark );

    /* Echo data back */
    n_sent = do_synchronous_send ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "oob_server (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* Receive a part of the out-of-band data and print atmark state */
    n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, 8, 0, par->buflen );
    ok ( n_recvd == 8,
         "oob_server (%x): received less data than expected: %d of %d\n", id, n_recvd, 8 );
    n_expected -= 8;

    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    trace( "oob_server (%x): %s the OOB mark: %i\n", id, atmark == 1 ? "not at" : "at", atmark );

    /* Receive the rest of the out-of-band data and check atmark state */
    do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );

    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    todo_wine ok ( atmark == 0, "oob_server (%x): not at the OOB mark: %i\n", id, atmark );

    /* cleanup */
    wsa_ok ( closesocket ( mem->sock[0].s ),  0 ==, "oob_server (%x): closesocket error: %d\n" );
    mem->sock[0].s = INVALID_SOCKET;

    trace ( "oob_server (%x) exiting\n", id );
    server_stop ();
}

/*
 * select_server: A non-blocking server.
 */
static VOID WINAPI select_server ( server_params *par )
{
    test_params *gen = par->general;
    server_memory *mem;
    int n_expected = gen->n_chunks * gen->chunk_size, tmp, i,
        id = GetCurrentThreadId(), n_connections = 0, n_sent, n_recvd,
        n_set, delta, n_ready;
    struct timeval timeout = {0,10}; /* wait for 10 milliseconds */
    fd_set fds_recv, fds_send, fds_openrecv, fds_opensend;

    trace ( "select_server (%x) starting\n", id );

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, FALSE ), 0 ==, "select_server (%x): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "select_server (%x): listen failed: %d\n");

    trace ( "select_server (%x) ready\n", id );
    SetEvent ( server_ready ); /* notify clients */

    FD_ZERO ( &fds_openrecv );
    FD_ZERO ( &fds_recv );
    FD_ZERO ( &fds_send );
    FD_ZERO ( &fds_opensend );

    FD_SET ( mem->s, &fds_openrecv );

    while(1)
    {
        fds_recv = fds_openrecv;
        fds_send = fds_opensend;

        n_set = 0;

        wsa_ok ( ( n_ready = select ( 0, &fds_recv, &fds_send, NULL, &timeout ) ), SOCKET_ERROR !=, 
            "select_server (%x): select() failed: %d\n" );

        /* check for incoming requests */
        if ( FD_ISSET ( mem->s, &fds_recv ) ) {
            n_set += 1;

            trace ( "select_server (%x): accepting client connection\n", id );

            /* accept a single connection */
            tmp = sizeof ( mem->sock[n_connections].peer );
            mem->sock[n_connections].s = accept ( mem->s, (struct sockaddr*) &mem->sock[n_connections].peer, &tmp );
            wsa_ok ( mem->sock[n_connections].s, INVALID_SOCKET !=, "select_server (%x): accept() failed: %d\n" );

            ok ( mem->sock[n_connections].peer.sin_addr.s_addr == inet_addr ( gen->inet_addr ),
                "select_server (%x): strange peer address\n", id );

            /* add to list of open connections */
            FD_SET ( mem->sock[n_connections].s, &fds_openrecv );
            FD_SET ( mem->sock[n_connections].s, &fds_opensend );

            n_connections++;
        }

        /* handle open requests */

        for ( i = 0; i < n_connections; i++ )
        {
            if ( FD_ISSET( mem->sock[i].s, &fds_recv ) ) {
                n_set += 1;

                if ( mem->sock[i].n_recvd < n_expected ) {
                    /* Receive data & check it */
                    n_recvd = recv ( mem->sock[i].s, mem->sock[i].buf + mem->sock[i].n_recvd, min ( n_expected - mem->sock[i].n_recvd, par->buflen ), 0 );
                    ok ( n_recvd != SOCKET_ERROR, "select_server (%x): error in recv(): %d\n", id, WSAGetLastError() );
                    mem->sock[i].n_recvd += n_recvd;

                    if ( mem->sock[i].n_recvd == n_expected ) {
                        int pos = test_buffer ( mem->sock[i].buf, gen->chunk_size, gen->n_chunks );
                        ok ( pos == -1, "select_server (%x): test pattern error: %d\n", id, pos );
                        FD_CLR ( mem->sock[i].s, &fds_openrecv );
                    }

                    ok ( mem->sock[i].n_recvd <= n_expected, "select_server (%x): received too many bytes: %d\n", id, mem->sock[i].n_recvd );
                }
            }

            /* only echo back what we've received */
            delta = mem->sock[i].n_recvd - mem->sock[i].n_sent;

            if ( FD_ISSET ( mem->sock[i].s, &fds_send ) ) {
                n_set += 1;

                if ( ( delta > 0 ) && ( mem->sock[i].n_sent < n_expected ) ) {
                    /* Echo data back */
                    n_sent = send ( mem->sock[i].s, mem->sock[i].buf + mem->sock[i].n_sent, min ( delta, par->buflen ), 0 );
                    ok ( n_sent != SOCKET_ERROR, "select_server (%x): error in send(): %d\n", id, WSAGetLastError() );
                    mem->sock[i].n_sent += n_sent;

                    if ( mem->sock[i].n_sent == n_expected ) {
                        FD_CLR ( mem->sock[i].s, &fds_opensend );
                    }

                    ok ( mem->sock[i].n_sent <= n_expected, "select_server (%x): sent too many bytes: %d\n", id, mem->sock[i].n_sent );
                }
            }
        }

        /* check that select returned the correct number of ready sockets */
        ok ( ( n_set == n_ready ), "select_server (%x): select() returns wrong number of ready sockets\n", id );

        /* check if all clients are done */
        if ( ( fds_opensend.fd_count == 0 ) 
            && ( fds_openrecv.fd_count == 1 ) /* initial socket that accepts clients */
            && ( n_connections  == min ( gen->n_clients, MAX_CLIENTS ) ) ) {
            break;
        }
    }

    for ( i = 0; i < min ( gen->n_clients, MAX_CLIENTS ); i++ )
    {
        /* cleanup */
        read_zero_bytes ( mem->sock[i].s );
        wsa_ok ( closesocket ( mem->sock[i].s ),  0 ==, "select_server (%x): closesocket error: %d\n" );
        mem->sock[i].s = INVALID_SOCKET;
    }

    trace ( "select_server (%x) exiting\n", id );
    server_stop ();
}

/**************** Clients ***************/

/*
 * simple_client: A very basic client doing synchronous IO.
 */
static VOID WINAPI simple_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int pos, n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, id;

    id = GetCurrentThreadId();
    trace ( "simple_client (%x): starting\n", id );
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );
    trace ( "simple_client (%x): server ready\n", id );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "simple_client (%x): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "simple_client (%x): failed to set blocking mode\n", id );
    trace ( "simple_client (%x) connected\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%x): shutdown failed: %d\n" );

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recv ( mem->s, mem->recv_buf, n_expected, 0, par->buflen );
    ok ( n_recvd == n_expected,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );

    /* check data */
    pos = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "simple_client (%x): test pattern error: %d\n", id, pos);

    /* cleanup */
    read_zero_bytes ( mem->s );
    trace ( "simple_client (%x) exiting\n", id );
    client_stop ();
}

/*
 * oob_client: A very basic client sending out-of-band data.
 */
static VOID WINAPI oob_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int pos, n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, id;

    id = GetCurrentThreadId();
    trace ( "oob_client (%x): starting\n", id );
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );
    trace ( "oob_client (%x): server ready\n", id );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "oob_client (%x): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "oob_client (%x): failed to set blocking mode\n", id );
    trace ( "oob_client (%x) connected\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "oob_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recv ( mem->s, mem->recv_buf, n_expected, 0, par->buflen );
    ok ( n_recvd == n_expected,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
    pos = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "simple_client (%x): test pattern error: %d\n", id, pos);

    /* send out-of-band data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, MSG_OOB, par->buflen );
    ok ( n_sent == n_expected,
         "oob_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%x): shutdown failed: %d\n" );

    /* cleanup */
    read_zero_bytes ( mem->s );
    trace ( "oob_client (%x) exiting\n", id );
    client_stop ();
}

/*
 * simple_mixed_client: mixing send and recvfrom
 */
static VOID WINAPI simple_mixed_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int pos, n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, id;
    int fromLen = sizeof(mem->addr);
    struct sockaddr test;

    id = GetCurrentThreadId();
    trace ( "simple_client (%x): starting\n", id );
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );
    trace ( "simple_client (%x): server ready\n", id );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "simple_client (%x): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "simple_client (%x): failed to set blocking mode\n", id );
    trace ( "simple_client (%x) connected\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%x): shutdown failed: %d\n" );

    /* this shouldn't change, since lpFrom, is not updated on
       connection oriented sockets - exposed by bug 11640
    */
    ((struct sockaddr_in*)&test)->sin_addr.s_addr = inet_addr("0.0.0.0");

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recvfrom ( mem->s,
					mem->recv_buf,
					n_expected,
					0,
					(struct sockaddr *)&test,
					&fromLen,
					par->buflen );
    ok ( n_recvd == n_expected,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );

    /* check that lpFrom was not updated */
    ok(0 ==
       strcmp(
	      inet_ntoa(((struct sockaddr_in*)&test)->sin_addr),
	      "0.0.0.0"), "lpFrom shouldn't be updated on connection oriented sockets\n");

    /* check data */
    pos = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "simple_client (%x): test pattern error: %d\n", id, pos);

    /* cleanup */
    read_zero_bytes ( mem->s );
    trace ( "simple_client (%x) exiting\n", id );
    client_stop ();
}

/*
 * event_client: An event-driven client
 */
static void WINAPI event_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int id = GetCurrentThreadId(), n_expected = gen->n_chunks * gen->chunk_size,
        tmp, err, n;
    HANDLE event;
    WSANETWORKEVENTS wsa_events;
    char *send_last, *recv_last, *send_p, *recv_p;
    LONG mask = FD_READ | FD_WRITE | FD_CLOSE;

    trace ( "event_client (%x): starting\n", id );
    client_start ( par );
    trace ( "event_client (%x): server ready\n", id );

    mem = TlsGetValue ( tls );

    /* Prepare event notification for connect, makes socket nonblocking */
    event = WSACreateEvent ();
    WSAEventSelect ( mem->s, event, FD_CONNECT );
    tmp = connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) );
    if ( tmp != 0 ) {
        err = WSAGetLastError ();
        ok ( err == WSAEWOULDBLOCK, "event_client (%x): connect error: %d\n", id, err );
        tmp = WaitForSingleObject ( event, INFINITE );
        ok ( tmp == WAIT_OBJECT_0, "event_client (%x): wait for connect event failed: %d\n", id, tmp );
        err = WSAEnumNetworkEvents ( mem->s, event, &wsa_events );
        ok ( err == 0, "event_client (%x): WSAEnumNetworkEvents error: %d\n", id, err );
        err = wsa_events.iErrorCode[ FD_CONNECT_BIT ];
        ok ( err == 0, "event_client (%x): connect error: %d\n", id, err );
        if ( err ) goto out;
    }

    trace ( "event_client (%x) connected\n", id );

    WSAEventSelect ( mem->s, event, mask );

    recv_p = mem->recv_buf;
    recv_last = mem->recv_buf + n_expected;
    send_p = mem->send_buf;
    send_last = mem->send_buf + n_expected;

    while ( TRUE )
    {
        err = WaitForSingleObject ( event, INFINITE );
        ok ( err == WAIT_OBJECT_0, "event_client (%x): wait failed\n", id );

        err = WSAEnumNetworkEvents ( mem->s, event, &wsa_events );
        ok( err == 0, "event_client (%x): WSAEnumNetworkEvents error: %d\n", id, err );

        if ( wsa_events.lNetworkEvents & FD_WRITE )
        {
            err = wsa_events.iErrorCode[ FD_WRITE_BIT ];
            ok ( err == 0, "event_client (%x): FD_WRITE error code: %d\n", id, err );

            if ( err== 0 )
                do
                {
                    n = send ( mem->s, send_p, min ( send_last - send_p, par->buflen ), 0 );
                    if ( n < 0 )
                    {
                        err = WSAGetLastError ();
                        ok ( err == WSAEWOULDBLOCK, "event_client (%x): send error: %d\n", id, err );
                    }
                    else
                        send_p += n;
                }
                while ( n >= 0 && send_p < send_last );

            if ( send_p == send_last )
            {
                trace ( "event_client (%x): all data sent - shutdown\n", id );
                shutdown ( mem->s, SD_SEND );
                mask &= ~FD_WRITE;
                WSAEventSelect ( mem->s, event, mask );
            }
        }
        if ( wsa_events.lNetworkEvents & FD_READ )
        {
            err = wsa_events.iErrorCode[ FD_READ_BIT ];
            ok ( err == 0, "event_client (%x): FD_READ error code: %d\n", id, err );
            if ( err != 0 ) break;
            
            /* First read must succeed */
            n = recv ( mem->s, recv_p, min ( recv_last - recv_p, par->buflen ), 0 );
            wsa_ok ( n, 0 <=, "event_client (%x): recv error: %d\n" );

            while ( n >= 0 ) {
                recv_p += n;
                if ( recv_p == recv_last )
                {
                    mask &= ~FD_READ;
                    trace ( "event_client (%x): all data received\n", id );
                    WSAEventSelect ( mem->s, event, mask );
                    break;
                }
                n = recv ( mem->s, recv_p, min ( recv_last - recv_p, par->buflen ), 0 );
                if ( n < 0 && ( err = WSAGetLastError()) != WSAEWOULDBLOCK )
                    ok ( 0, "event_client (%x): read error: %d\n", id, err );
                
            }
        }   
        if ( wsa_events.lNetworkEvents & FD_CLOSE )
        {
            trace ( "event_client (%x): close event\n", id );
            err = wsa_events.iErrorCode[ FD_CLOSE_BIT ];
            ok ( err == 0, "event_client (%x): FD_CLOSE error code: %d\n", id, err );
            break;
        }
    }

    n = send_p - mem->send_buf;
    ok ( send_p == send_last,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n, n_expected );
    n = recv_p - mem->recv_buf;
    ok ( recv_p == recv_last,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n, n_expected );
    n = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( n == -1, "event_client (%x): test pattern error: %d\n", id, n);

out:
    WSACloseEvent ( event );
    trace ( "event_client (%x) exiting\n", id );
    client_stop ();
}

/* Tests for WSAStartup */
static void test_WithoutWSAStartup(void)
{
    DWORD err;

    WSASetLastError(0xdeadbeef);
    ok(WSASocketA(0, 0, 0, NULL, 0, 0) == INVALID_SOCKET, "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSANOTINITIALISED, "Expected 10093, received %d\n", err);

    WSASetLastError(0xdeadbeef);
    ok(gethostbyname("localhost") == NULL, "gethostbyname() succeeded unexpectedly\n");
    err = WSAGetLastError();
    ok(err == WSANOTINITIALISED, "Expected 10093, received %d\n", err);
}

static void test_WithWSAStartup(void)
{
    WSADATA data;
    WORD version = MAKEWORD( 2, 2 );
    INT res, socks, i, j;
    SOCKET sock;
    LPVOID ptr;
    struct
    {
        SOCKET src, dst, dup_src, dup_dst;
    } pairs[32];
    DWORD error;

    res = WSAStartup( version, &data );
    ok(res == 0, "WSAStartup() failed unexpectedly: %d\n", res);

    ptr = gethostbyname("localhost");
    ok(ptr != NULL, "gethostbyname() failed unexpectedly: %d\n", WSAGetLastError());

    /* Alloc some sockets to check if they are destroyed on WSACleanup */
    for (socks = 0; socks < sizeof(pairs) / sizeof(pairs[0]); socks++)
    {
        WSAPROTOCOL_INFOA info;
        if (tcp_socketpair(&pairs[socks].src, &pairs[socks].dst)) break;

        memset(&info, 0, sizeof(info));
        ok(!WSADuplicateSocketA(pairs[socks].src, GetCurrentProcessId(), &info),
           "WSADuplicateSocketA should have worked\n");
        pairs[socks].dup_src = WSASocketA(0, 0, 0, &info, 0, 0);
        ok(pairs[socks].dup_src != SOCKET_ERROR, "expected != -1\n");

        memset(&info, 0, sizeof(info));
        ok(!WSADuplicateSocketA(pairs[socks].dst, GetCurrentProcessId(), &info),
           "WSADuplicateSocketA should have worked\n");
        pairs[socks].dup_dst = WSASocketA(0, 0, 0, &info, 0, 0);
        ok(pairs[socks].dup_dst != SOCKET_ERROR, "expected != -1\n");
    }

    res = send(pairs[0].src, "TEST", 4, 0);
    ok(res == 4, "send failed with error %d\n", WSAGetLastError());

    WSACleanup();

    res = WSAStartup( version, &data );
    ok(res == 0, "WSAStartup() failed unexpectedly: %d\n", res);

    /* show that sockets are destroyed automatically after WSACleanup */
    SetLastError(0xdeadbeef);
    res = send(pairs[0].src, "TEST", 4, 0);
    error = WSAGetLastError();
    ok(res == SOCKET_ERROR, "send should have failed\n");
    todo_wine ok(error == WSAENOTSOCK, "expected 10038, got %d\n", error);

    SetLastError(0xdeadbeef);
    res = send(pairs[0].dst, "TEST", 4, 0);
    error = WSAGetLastError();
    ok(res == SOCKET_ERROR, "send should have failed\n");
    todo_wine ok(error == WSAENOTSOCK, "expected 10038, got %d\n", error);

    /* Check that all sockets were destroyed */
    for (i = 0; i < socks; i++)
    {
        for (j = 0; j < 4; j++)
        {
            struct sockaddr_in saddr;
            int size = sizeof(saddr);
            switch(j)
            {
                case 0: sock = pairs[i].src; break;
                case 1: sock = pairs[i].dup_src; break;
                case 2: sock = pairs[i].dst; break;
                case 3: sock = pairs[i].dup_dst; break;
            }

            SetLastError(0xdeadbeef);
            res = getsockname(sock, (struct sockaddr *)&saddr, &size);
            error = WSAGetLastError();
            ok(res == SOCKET_ERROR, "Test[%d]: getsockname should have failed\n", i);
            todo_wine ok(error == WSAENOTSOCK, "Test[%d]: expected 10038, got %d\n", i, error);
        }
    }

    /* While wine is not fixed, close all sockets manually */
    for (i = 0; i < socks; i++)
    {
        closesocket(pairs[i].src);
        closesocket(pairs[i].dst);
        closesocket(pairs[i].dup_src);
        closesocket(pairs[i].dup_dst);
    }

    res = WSACleanup();
    ok(res == 0, "expected 0, got %d\n", res);
    WSASetLastError(0xdeadbeef);
    res = WSACleanup();
    error = WSAGetLastError();
    ok ( res == SOCKET_ERROR && error ==  WSANOTINITIALISED,
            "WSACleanup returned %d WSAGetLastError is %d\n", res, error);
}

/**************** Main program utility functions ***************/

static void Init (void)
{
    WORD ver = MAKEWORD (2, 2);
    WSADATA data;
    HMODULE hws2_32 = GetModuleHandleA("ws2_32.dll"), hiphlpapi, ntdll;

    pfreeaddrinfo = (void *)GetProcAddress(hws2_32, "freeaddrinfo");
    pgetaddrinfo = (void *)GetProcAddress(hws2_32, "getaddrinfo");
    pFreeAddrInfoW = (void *)GetProcAddress(hws2_32, "FreeAddrInfoW");
    pFreeAddrInfoExW = (void *)GetProcAddress(hws2_32, "FreeAddrInfoExW");
    pGetAddrInfoW = (void *)GetProcAddress(hws2_32, "GetAddrInfoW");
    pGetAddrInfoExW = (void *)GetProcAddress(hws2_32, "GetAddrInfoExW");
    pGetAddrInfoExOverlappedResult = (void *)GetProcAddress(hws2_32, "GetAddrInfoExOverlappedResult");
    p_inet_ntop = (void *)GetProcAddress(hws2_32, "inet_ntop");
    pInetNtopW = (void *)GetProcAddress(hws2_32, "InetNtopW");
    p_inet_pton = (void *)GetProcAddress(hws2_32, "inet_pton");
    pInetPtonW = (void *)GetProcAddress(hws2_32, "InetPtonW");
    pWSALookupServiceBeginW = (void *)GetProcAddress(hws2_32, "WSALookupServiceBeginW");
    pWSALookupServiceEnd = (void *)GetProcAddress(hws2_32, "WSALookupServiceEnd");
    pWSALookupServiceNextW = (void *)GetProcAddress(hws2_32, "WSALookupServiceNextW");
    pWSAEnumNameSpaceProvidersA = (void *)GetProcAddress(hws2_32, "WSAEnumNameSpaceProvidersA");
    pWSAEnumNameSpaceProvidersW = (void *)GetProcAddress(hws2_32, "WSAEnumNameSpaceProvidersW");
    pWSAPoll = (void *)GetProcAddress(hws2_32, "WSAPoll");

    hiphlpapi = LoadLibraryA("iphlpapi.dll");
    if (hiphlpapi)
    {
        pGetIpForwardTable = (void *)GetProcAddress(hiphlpapi, "GetIpForwardTable");
        pGetAdaptersInfo = (void *)GetProcAddress(hiphlpapi, "GetAdaptersInfo");
    }

    ntdll = LoadLibraryA("ntdll.dll");
    if (ntdll)
    {
        pNtClose = (void *)GetProcAddress(ntdll, "NtClose");
        pNtSetInformationFile = (void *)GetProcAddress(ntdll, "NtSetInformationFile");
        pNtQueryInformationFile = (void *)GetProcAddress(ntdll, "NtQueryInformationFile");
    }

    ok ( WSAStartup ( ver, &data ) == 0, "WSAStartup failed\n" );
    tls = TlsAlloc();
}

static void Exit (void)
{
    INT ret, err;
    TlsFree ( tls );
    ret = WSACleanup();
    err = WSAGetLastError();
    ok ( ret == 0, "WSACleanup failed ret = %d GetLastError is %d\n", ret, err);
}

static void StartServer (LPTHREAD_START_ROUTINE routine,
                         test_params *general, server_params *par)
{
    par->general = general;
    thread[0] = CreateThread ( NULL, 0, routine, par, 0, &thread_id[0] );
    ok ( thread[0] != NULL, "Failed to create server thread\n" );
}

static void StartClients (LPTHREAD_START_ROUTINE routine,
                          test_params *general, client_params *par)
{
    int i;
    par->general = general;
    for ( i = 1; i <= min ( general->n_clients, MAX_CLIENTS ); i++ )
    {
        client_id = i - 1;
        thread[i] = CreateThread ( NULL, 0, routine, par, 0, &thread_id[i] );
        ok ( thread[i] != NULL, "Failed to create client thread\n" );
        /* Make sure the client is up and running */
        WaitForSingleObject ( client_ready[client_id], INFINITE );
    };
}

static void do_test( test_setup *test )
{
    DWORD i, n = min (test->general.n_clients, MAX_CLIENTS);
    DWORD wait;

    server_ready = CreateEventA ( NULL, TRUE, FALSE, NULL );
    for (i = 0; i <= n; i++)
        client_ready[i] = CreateEventA ( NULL, TRUE, FALSE, NULL );

    StartServer ( test->srv, &test->general, &test->srv_params );
    StartClients ( test->clt, &test->general, &test->clt_params );
    WaitForSingleObject ( server_ready, INFINITE );

    wait = WaitForMultipleObjects ( 1 + n, thread, TRUE, 1000 * TEST_TIMEOUT );
    ok ( wait <= WAIT_OBJECT_0 + n ,
         "some threads have not completed: %x\n", wait );

    if ( ! ( wait <= WAIT_OBJECT_0 + n ) )
    {
        for (i = 0; i <= n; i++)
        {
            if ( WaitForSingleObject ( thread[i], 0 ) != WAIT_OBJECT_0 )
            {
                trace ("terminating thread %08x\n", thread_id[i]);
                TerminateThread ( thread [i], 0 );
            }
        }
    }
    CloseHandle ( server_ready );
    for (i = 0; i <= n; i++)
        CloseHandle ( client_ready[i] );
}

/********* some tests for getsockopt(setsockopt(X)) == X ***********/
/* optname = SO_LINGER */
static const LINGER linger_testvals[] = {
    {0,0},
    {0,73}, 
    {1,0},
    {5,189}
};

/* optname = SO_RCVTIMEO, SOSNDTIMEO */
#define SOCKTIMEOUT1 63000 /* 63 seconds. Do not test fractional part because of a
                        bug in the linux kernel (fixed in 2.6.8) */ 
#define SOCKTIMEOUT2 997000 /* 997 seconds */

static void test_set_getsockopt(void)
{
    SOCKET s, s2;
    int i, err, lasterr;
    int timeout;
    LINGER lingval;
    int size;
    WSAPROTOCOL_INFOA infoA;
    WSAPROTOCOL_INFOW infoW;
    char providername[WSAPROTOCOL_LEN + 1];
    DWORD value;
    struct _prottest
    {
        int family, type, proto;
    } prottest[] = {
        {AF_INET, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET, SOCK_DGRAM, IPPROTO_UDP},
        {AF_INET6, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET6, SOCK_DGRAM, IPPROTO_UDP}
    };
    union _csspace
    {
        CSADDR_INFO cs;
        char space[128];
    } csinfoA, csinfoB;

    s = socket(AF_INET, SOCK_STREAM, 0);
    ok(s!=INVALID_SOCKET, "socket() failed error: %d\n", WSAGetLastError());
    if( s == INVALID_SOCKET) return;
    /* SO_RCVTIMEO */
    timeout = SOCKTIMEOUT1;
    size = sizeof(timeout);
    err = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, size); 
    if( !err)
        err = getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, &size); 
    ok( !err, "get/setsockopt(SO_RCVTIMEO) failed error: %d\n", WSAGetLastError());
    ok( timeout == SOCKTIMEOUT1, "getsockopt(SO_RCVTIMEO) returned wrong value %d\n", timeout);

    timeout = 0;
    size = sizeof(timeout);
    err = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, size);
    if( !err)
        err = getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, &size);
    ok( !err, "get/setsockopt(SO_RCVTIMEO) failed error: %d\n", WSAGetLastError());
    ok( timeout == 0, "getsockopt(SO_RCVTIMEO) returned wrong value %d\n", timeout);

    /* SO_SNDTIMEO */
    timeout = SOCKTIMEOUT2; /* 997 seconds. See remark above */
    size = sizeof(timeout);
    err = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, size); 
    if( !err)
        err = getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, &size); 
    ok( !err, "get/setsockopt(SO_SNDTIMEO) failed error: %d\n", WSAGetLastError());
    ok( timeout == SOCKTIMEOUT2, "getsockopt(SO_SNDTIMEO) returned wrong value %d\n", timeout);

    /* SO_SNDBUF */
    value = 4096;
    size = sizeof(value);
    err = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&value, size);
    ok( !err, "setsockopt(SO_SNDBUF) failed error: %u\n", WSAGetLastError() );
    value = 0xdeadbeef;
    err = getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&value, &size);
    ok( !err, "getsockopt(SO_SNDBUF) failed error: %u\n", WSAGetLastError() );
    ok( value == 4096, "expected 4096, got %u\n", value );

    /* SO_RCVBUF */
    value = 4096;
    size = sizeof(value);
    err = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, size);
    ok( !err, "setsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    value = 0xdeadbeef;
    err = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, &size);
    ok( !err, "getsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    ok( value == 4096, "expected 4096, got %u\n", value );

    /* SO_LINGER */
    for( i = 0; i < sizeof(linger_testvals)/sizeof(LINGER);i++) {
        size =  sizeof(lingval);
        lingval = linger_testvals[i];
        err = setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &lingval, size); 
        if( !err)
            err = getsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &lingval, &size); 
        ok( !err, "get/setsockopt(SO_LINGER) failed error: %d\n", WSAGetLastError());
        ok( !lingval.l_onoff == !linger_testvals[i].l_onoff &&
                (lingval.l_linger == linger_testvals[i].l_linger ||
                 (!lingval.l_linger && !linger_testvals[i].l_onoff))
                , "getsockopt(SO_LINGER #%d) returned wrong value %d,%d not %d,%d\n", i, 
                 lingval.l_onoff, lingval.l_linger,
                 linger_testvals[i].l_onoff, linger_testvals[i].l_linger);
    }

    size =  sizeof(lingval);
    err = setsockopt(s, SOL_SOCKET, SO_LINGER, NULL, size);
    ok(err == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n", err, WSAGetLastError());
    err = setsockopt(s, SOL_SOCKET, SO_LINGER, NULL, 0);
    ok(err == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n", err, WSAGetLastError());

    size =  sizeof(BOOL);
    err = setsockopt(s, SOL_SOCKET, SO_DONTLINGER, NULL, size);
    ok(err == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n", err, WSAGetLastError());
    err = setsockopt(s, SOL_SOCKET, SO_DONTLINGER, NULL, 0);
    ok(err == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n", err, WSAGetLastError());

    /* Test for erroneously passing a value instead of a pointer as optval */
    size = sizeof(char);
    err = setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)1, size);
    ok(err == SOCKET_ERROR, "setsockopt with optval being a value passed "
                            "instead of failing.\n");
    lasterr = WSAGetLastError();
    ok(lasterr == WSAEFAULT, "setsockopt with optval being a value "
                             "returned 0x%08x, not WSAEFAULT(0x%08x)\n",
                             lasterr, WSAEFAULT);

    /* SO_RCVTIMEO with invalid values for level */
    size = sizeof(timeout);
    timeout = SOCKTIMEOUT1;
    SetLastError(0xdeadbeef);
    err = setsockopt(s, 0xffffffff, SO_RCVTIMEO, (char *) &timeout, size);
    ok( (err == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d (expected SOCKET_ERROR with WSAEINVAL)\n",
        err, WSAGetLastError());

    timeout = SOCKTIMEOUT1;
    SetLastError(0xdeadbeef);
    err = setsockopt(s, 0x00008000, SO_RCVTIMEO, (char *) &timeout, size);
    ok( (err == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d (expected SOCKET_ERROR with WSAEINVAL)\n",
        err, WSAGetLastError());

    /* Test SO_ERROR set/get */
    SetLastError(0xdeadbeef);
    i = 1234;
    err = setsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, size);
todo_wine
    ok( !err && !WSAGetLastError(),
        "got %d with %d (expected 0 with 0)\n",
        err, WSAGetLastError());

    SetLastError(0xdeadbeef);
    i = 4321;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
todo_wine
    ok( !err && !WSAGetLastError(),
        "got %d with %d (expected 0 with 0)\n",
        err, WSAGetLastError());
todo_wine
    ok (i == 1234, "got %d (expected 1234)\n", i);

    /* Test invalid optlen */
    SetLastError(0xdeadbeef);
    size = 1;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
todo_wine
    ok( (err == SOCKET_ERROR) && (WSAGetLastError() == WSAEFAULT),
        "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n",
        err, WSAGetLastError());

    closesocket(s);
    /* Test with the closed socket */
    SetLastError(0xdeadbeef);
    size = sizeof(i);
    i = 1234;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
todo_wine
    ok( (err == SOCKET_ERROR) && (WSAGetLastError() == WSAENOTSOCK),
        "got %d with %d (expected SOCKET_ERROR with WSAENOTSOCK)\n",
        err, WSAGetLastError());
    ok (i == 1234, "expected 1234, got %d\n", i);

    /* Test WS_IP_MULTICAST_TTL with 8, 16, 24 and 32 bits values */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    ok(s != INVALID_SOCKET, "Failed to create socket\n");
    size = sizeof(i);
    i = 0x0000000a;
    err = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &i, size);
    if (!err)
    {
        for (i = 0; i < 4; i++)
        {
            int k, j;
            const int tests[] = {0xffffff0a, 0xffff000b, 0xff00000c, 0x0000000d};
            err = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &tests[i], i + 1);
            ok(!err, "Test [%d] Expected 0, got %d\n", i, err);
            err = getsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &k, &size);
            ok(!err, "Test [%d] Expected 0, got %d\n", i, err);
            j = i != 3 ? tests[i] & ((1 << (i + 1) * 8) - 1) : tests[i];
            ok(k == j, "Test [%d] Expected 0x%x, got 0x%x\n", i, j, k);
        }
    }
    else
        win_skip("IP_MULTICAST_TTL is unsupported\n");
    closesocket(s);

    /* test SO_PROTOCOL_INFOA invalid parameters */
    ok(getsockopt(INVALID_SOCKET, SOL_SOCKET, SO_PROTOCOL_INFOA, NULL, NULL),
       "getsockopt should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, got %d instead\n", err);
    size = sizeof(WSAPROTOCOL_INFOA);
    ok(getsockopt(INVALID_SOCKET, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &infoA, &size),
       "getsockopt should have failed\n");
    ok(size == sizeof(WSAPROTOCOL_INFOA), "got size %d\n", size);
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, got %d instead\n", err);
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, NULL, NULL),
       "getsockopt should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);
    ok(getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &infoA, NULL),
       "getsockopt should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);
    ok(getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, NULL, &size),
       "getsockopt should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);
    size = sizeof(WSAPROTOCOL_INFOA) / 2;
    ok(getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &infoA, &size),
       "getsockopt should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);
    ok(size == sizeof(WSAPROTOCOL_INFOA), "got size %d\n", size);
    size = sizeof(WSAPROTOCOL_INFOA) * 2;
    err = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &infoA, &size);
    ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
    ok(size == sizeof(WSAPROTOCOL_INFOA) * 2, "got size %d\n", size);

    closesocket(s);

    /* test SO_PROTOCOL_INFO structure returned for different protocols */
    for (i = 0; i < sizeof(prottest) / sizeof(prottest[0]); i++)
    {
        int k;

        s = socket(prottest[i].family, prottest[i].type, prottest[i].proto);
        if (s == INVALID_SOCKET && prottest[i].family == AF_INET6) continue;

        ok(s != INVALID_SOCKET, "Failed to create socket: %d\n",
          WSAGetLastError());

        /* compare both A and W version */
        infoA.szProtocol[0] = 0;
        size = sizeof(WSAPROTOCOL_INFOA);
        err = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &infoA, &size);
        ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
        ok(size == sizeof(WSAPROTOCOL_INFOA), "got size %d\n", size);

        infoW.szProtocol[0] = 0;
        size = sizeof(WSAPROTOCOL_INFOW);
        err = getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOW, (char *) &infoW, &size);
        ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
        ok(size == sizeof(WSAPROTOCOL_INFOW), "got size %d\n", size);

        trace("provider name '%s', family %d, type %d, proto %d\n",
              infoA.szProtocol, prottest[i].family, prottest[i].type, prottest[i].proto);

        ok(infoA.szProtocol[0], "WSAPROTOCOL_INFOA was not filled\n");
        ok(infoW.szProtocol[0], "WSAPROTOCOL_INFOW was not filled\n");

        WideCharToMultiByte(CP_ACP, 0, infoW.szProtocol, -1,
                            providername, sizeof(providername), NULL, NULL);
        ok(!strcmp(infoA.szProtocol,providername),
           "different provider names '%s' != '%s'\n", infoA.szProtocol, providername);

        ok(!memcmp(&infoA, &infoW, FIELD_OFFSET(WSAPROTOCOL_INFOA, szProtocol)),
           "SO_PROTOCOL_INFO[A/W] comparison failed\n");

        /* Remove IF when WSAEnumProtocols support IPV6 data */
        todo_wine_if (prottest[i].family == AF_INET6)
            ok(infoA.iAddressFamily == prottest[i].family, "socket family invalid, expected %d received %d\n",
               prottest[i].family, infoA.iAddressFamily);
        ok(infoA.iSocketType == prottest[i].type, "socket type invalid, expected %d received %d\n",
           prottest[i].type, infoA.iSocketType);
        ok(infoA.iProtocol == prottest[i].proto, "socket protocol invalid, expected %d received %d\n",
           prottest[i].proto, infoA.iProtocol);

        /* IP_HDRINCL is supported only on SOCK_RAW but passed to SOCK_DGRAM by Impossible Creatures */
        size = sizeof(i);
        k = 1;
        SetLastError(0xdeadbeef);
        err = setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, size);
        if (err == -1) /* >= Vista */
        {
            todo_wine {
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %d\n", GetLastError());
            k = 99;
            SetLastError(0xdeadbeef);
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            ok(err == -1, "Expected -1, got %d\n", err);
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %d\n", GetLastError());
            ok(k == 99, "Expected 99, got %d\n", k);

            size = sizeof(k);
            k = 0;
            SetLastError(0xdeadbeef);
            err = setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, size);
            }
            ok(err == -1, "Expected -1, got %d\n", err);
            todo_wine {
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %d\n", GetLastError());
            k = 99;
            SetLastError(0xdeadbeef);
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            ok(err == -1, "Expected -1, got %d\n", err);
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %d\n", GetLastError());
            ok(k == 99, "Expected 99, got %d\n", k);
            }
        }
        else /* <= 2003 the tests differ between TCP and UDP, UDP silently accepts */
        {
            SetLastError(0xdeadbeef);
            k = 99;
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            if (prottest[i].type == SOCK_DGRAM)
            {
                ok(err == 0, "Expected 0, got %d\n", err);
                ok(k == 1, "Expected 1, got %d\n", k);
            }
            else
            {
                /* contratry to what we could expect the function returns error but k is changed */
                ok(err == -1, "Expected -1, got %d\n", err);
                ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %d\n", GetLastError());
                ok(k == 0, "Expected 0, got %d\n", k);
            }

            k = 0;
            err = setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, size);
            ok(err == 0, "Expected 0, got %d\n", err);

            k = 99;
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            if (prottest[i].type == SOCK_DGRAM)
            {
                ok(err == 0, "Expected 0, got %d\n", err);
                ok(k == 0, "Expected 0, got %d\n", k);
            }
            else
            {
                /* contratry to what we could expect the function returns error but k is changed */
                ok(err == -1, "Expected -1, got %d\n", err);
                ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %d\n", GetLastError());
                ok(k == 0, "Expected 0, got %d\n", k);
            }
        }

        closesocket(s);
    }

    /* Test SO_BSP_STATE - Present only in >= Win 2008 */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s != INVALID_SOCKET, "Failed to create socket\n");
    s2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s2 != INVALID_SOCKET, "Failed to create socket\n");

    SetLastError(0xdeadbeef);
    size = sizeof(csinfoA);
    err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
    if (!err)
    {
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        /* Socket is not bound, no information provided */
        ok(!csinfoA.cs.LocalAddr.iSockaddrLength, "Expected 0, got %d\n", csinfoA.cs.LocalAddr.iSockaddrLength);
        ok(csinfoA.cs.LocalAddr.lpSockaddr == NULL, "Expected NULL, got %p\n", csinfoA.cs.LocalAddr.lpSockaddr);
        /* Socket is not connected, no information provided */
        ok(!csinfoA.cs.RemoteAddr.iSockaddrLength, "Expected 0, got %d\n", csinfoA.cs.RemoteAddr.iSockaddrLength);
        ok(csinfoA.cs.RemoteAddr.lpSockaddr == NULL, "Expected NULL, got %p\n", csinfoA.cs.RemoteAddr.lpSockaddr);

        err = bind(s, (struct sockaddr*)&saddr, sizeof(saddr));
        ok(!err, "Expected 0, got %d\n", err);
        size = sizeof(csinfoA);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(!err, "Expected 0, got %d\n", err);

        /* Socket is bound */
        ok(csinfoA.cs.LocalAddr.iSockaddrLength, "Expected non-zero\n");
        ok(csinfoA.cs.LocalAddr.lpSockaddr != NULL, "Expected non-null\n");
        /* Socket is not connected, no information provided */
        ok(!csinfoA.cs.RemoteAddr.iSockaddrLength, "Expected 0, got %d\n", csinfoA.cs.RemoteAddr.iSockaddrLength);
        ok(csinfoA.cs.RemoteAddr.lpSockaddr == NULL, "Expected NULL, got %p\n", csinfoA.cs.RemoteAddr.lpSockaddr);

        err = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
        ok(!err, "Expected 0, got %d\n", err);
        err = getsockname(s2, (struct sockaddr *)&saddr, &size);
        ok(!err, "Expected 0, got %d\n", err);
        err = listen(s2, 1);
        ok(!err, "Expected 0, got %d\n", err);
        err = connect(s, (struct sockaddr*)&saddr, sizeof(saddr));
        ok(!err, "Expected 0, got %d\n", err);
        size = sizeof(saddr);
        err = accept(s2, (struct sockaddr*)&saddr, &size);
        ok(err != INVALID_SOCKET, "Failed to accept socket\n");
        closesocket(s2);
        s2 = err;

        size = sizeof(csinfoA);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(!err, "Expected 0, got %d\n", err);
        err = getsockopt(s2, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoB, &size);
        ok(!err, "Expected 0, got %d\n", err);
        ok(size == sizeof(csinfoA), "Got %d\n", size);
        size = sizeof(saddr);
        ok(size == csinfoA.cs.LocalAddr.iSockaddrLength, "Expected %d, got %d\n", size,
           csinfoA.cs.LocalAddr.iSockaddrLength);
        ok(size == csinfoA.cs.RemoteAddr.iSockaddrLength, "Expected %d, got %d\n", size,
           csinfoA.cs.RemoteAddr.iSockaddrLength);
        ok(!memcmp(csinfoA.cs.LocalAddr.lpSockaddr, csinfoB.cs.RemoteAddr.lpSockaddr, size),
           "Expected matching addresses\n");
        ok(!memcmp(csinfoB.cs.LocalAddr.lpSockaddr, csinfoA.cs.RemoteAddr.lpSockaddr, size),
           "Expected matching addresses\n");
        ok(csinfoA.cs.iSocketType == SOCK_STREAM, "Wrong socket type\n");
        ok(csinfoB.cs.iSocketType == SOCK_STREAM, "Wrong socket type\n");
        ok(csinfoA.cs.iProtocol == IPPROTO_TCP, "Wrong socket protocol\n");
        ok(csinfoB.cs.iProtocol == IPPROTO_TCP, "Wrong socket protocol\n");

        err = getpeername(s, (struct sockaddr *)&saddr, &size);
        ok(!err, "Expected 0, got %d\n", err);
        ok(!memcmp(&saddr, csinfoA.cs.RemoteAddr.lpSockaddr, size), "Expected matching addresses\n");
        ok(!memcmp(&saddr, csinfoB.cs.LocalAddr.lpSockaddr, size), "Expected matching addresses\n");
        err = getpeername(s2, (struct sockaddr *)&saddr, &size);
        ok(!err, "Expected 0, got %d\n", err);
        ok(!memcmp(&saddr, csinfoB.cs.RemoteAddr.lpSockaddr, size), "Expected matching addresses\n");
        ok(!memcmp(&saddr, csinfoA.cs.LocalAddr.lpSockaddr, size), "Expected matching addresses\n");
        err = getsockname(s, (struct sockaddr *)&saddr, &size);
        ok(!err, "Expected 0, got %d\n", err);
        ok(!memcmp(&saddr, csinfoA.cs.LocalAddr.lpSockaddr, size), "Expected matching addresses\n");
        ok(!memcmp(&saddr, csinfoB.cs.RemoteAddr.lpSockaddr, size), "Expected matching addresses\n");
        err = getsockname(s2, (struct sockaddr *)&saddr, &size);
        ok(!err, "Expected 0, got %d\n", err);
        ok(!memcmp(&saddr, csinfoB.cs.LocalAddr.lpSockaddr, size), "Expected matching addresses\n");
        ok(!memcmp(&saddr, csinfoA.cs.RemoteAddr.lpSockaddr, size), "Expected matching addresses\n");

        SetLastError(0xdeadbeef);
        size = sizeof(CSADDR_INFO);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(err, "Expected non-zero\n");
        ok(size == sizeof(CSADDR_INFO), "Got %d\n", size);
        ok(GetLastError() == WSAEFAULT, "Expected 10014, got %d\n", GetLastError());

        /* At least for IPv4 the size is exactly 56 bytes */
        size = sizeof(*csinfoA.cs.LocalAddr.lpSockaddr) * 2 + sizeof(csinfoA.cs);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(!err, "Expected 0, got %d\n", err);
        size--;
        SetLastError(0xdeadbeef);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(err, "Expected non-zero\n");
        ok(GetLastError() == WSAEFAULT, "Expected 10014, got %d\n", GetLastError());
    }
    else
        ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %d\n", GetLastError());

    closesocket(s);
    closesocket(s2);

    for (i = 0; i < 2; i++)
    {
        int family, level;

        if (i)
        {
            family = AF_INET6;
            level = IPPROTO_IPV6;
        }
        else
        {
            family = AF_INET;
            level = IPPROTO_IP;
        }

        s = socket(family, SOCK_DGRAM, 0);
        if (s == INVALID_SOCKET && i)
        {
            skip("IPv6 is not supported\n");
            break;
        }
        ok(s != INVALID_SOCKET, "socket failed with error %d\n", GetLastError());

        size = sizeof(value);
        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %d\n", err, GetLastError());
        ok(value == 0, "Expected 0, got %d\n", value);

        size = sizeof(value);
        value = 1;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %d\n", err, GetLastError());

        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %d\n", err, GetLastError());
        ok(value == 1, "Expected 1, got %d\n", value);

        size = sizeof(value);
        value = 0xdead;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %d\n", err, GetLastError());

        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %d\n", err, GetLastError());
        ok(value == 1, "Expected 1, got %d\n", value);

        closesocket(s);
    }
}

static void test_so_reuseaddr(void)
{
    struct sockaddr_in saddr;
    SOCKET s1,s2;
    unsigned int rc,reuse;
    int size;
    DWORD err;

    saddr.sin_family      = AF_INET;
    saddr.sin_port        = htons(SERVERPORT+1);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    s1=socket(AF_INET, SOCK_STREAM, 0);
    ok(s1!=INVALID_SOCKET, "socket() failed error: %d\n", WSAGetLastError());
    rc = bind(s1, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc!=SOCKET_ERROR, "bind(s1) failed error: %d\n", WSAGetLastError());

    s2=socket(AF_INET, SOCK_STREAM, 0);
    ok(s2!=INVALID_SOCKET, "socket() failed error: %d\n", WSAGetLastError());

    reuse=0x1234;
    size=sizeof(reuse);
    rc=getsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, &size );
    ok(rc==0 && reuse==0,"wrong result in getsockopt(SO_REUSEADDR): rc=%d reuse=%d\n",rc,reuse);

    rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc==SOCKET_ERROR, "bind() succeeded\n");

    reuse = 1;
    rc = setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    ok(rc==0, "setsockopt() failed error: %d\n", WSAGetLastError());

    /* On Win2k3 and above, all SO_REUSEADDR seems to do is to allow binding to
     * a port immediately after closing another socket on that port, so
     * basically following the BSD socket semantics here. */
    rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
    if(rc==0)
    {
        int s3=socket(AF_INET, SOCK_STREAM, 0), s4;
        trace("<= Win XP behavior of SO_REUSEADDR\n");

        /* If we could bind again in the same port this is Windows version <= XP.
         * Lets test if we can really connect to one of them. */
        set_blocking(s1, FALSE);
        set_blocking(s2, FALSE);
        rc = listen(s1, 1);
        ok(!rc, "listen() failed with error: %d\n", WSAGetLastError());
        rc = listen(s2, 1);
        ok(!rc, "listen() failed with error: %d\n", WSAGetLastError());
        rc = connect(s3, (struct sockaddr*)&saddr, sizeof(saddr));
        ok(!rc, "connecting to accepting socket failed %d\n", WSAGetLastError());

        /* the delivery of the connection is random so we need to try on both sockets */
        size = sizeof(saddr);
        s4 = accept(s1, (struct sockaddr*)&saddr, &size);
        if(s4 == INVALID_SOCKET)
            s4 = accept(s2, (struct sockaddr*)&saddr, &size);
        ok(s4 != INVALID_SOCKET, "none of the listening sockets could get the connection\n");

        closesocket(s1);
        closesocket(s3);
        closesocket(s4);
    }
    else
    {
        trace(">= Win 2003 behavior of SO_REUSEADDR\n");
        err = WSAGetLastError();
        ok(err==WSAEACCES, "expected 10013, got %d\n", err);

        closesocket(s1);
        rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
        ok(rc==0, "bind() failed error: %d\n", WSAGetLastError());
    }

    closesocket(s2);
}

#define IP_PKTINFO_LEN (sizeof(WSACMSGHDR) + WSA_CMSG_ALIGN(sizeof(struct in_pktinfo)))

static void test_ip_pktinfo(void)
{
    ULONG addresses[2] = {inet_addr("127.0.0.1"), htonl(INADDR_ANY)};
    char recvbuf[10], pktbuf[512], msg[] = "HELLO";
    struct sockaddr_in s1addr, s2addr, s3addr;
    GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;
    LPFN_WSARECVMSG pWSARecvMsg = NULL;
    unsigned int rc, yes = 1;
    BOOL foundhdr;
    DWORD dwBytes, dwSize, dwFlags;
    socklen_t addrlen;
    WSACMSGHDR *cmsg;
    WSAOVERLAPPED ov;
    WSABUF iovec[1];
    SOCKET s1, s2;
    WSAMSG hdr;
    int i, err;

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (ov.hEvent == INVALID_HANDLE_VALUE)
    {
        skip("Could not create event object, some tests will be skipped. errno = %d\n", GetLastError());
        return;
    }

    memset(&hdr, 0x00, sizeof(hdr));
    s1addr.sin_family = AF_INET;
    s1addr.sin_port   = htons(0);
    /* Note: s1addr.sin_addr is set below */
    iovec[0].buf      = recvbuf;
    iovec[0].len      = sizeof(recvbuf);
    hdr.name          = (struct sockaddr*)&s3addr;
    hdr.namelen       = sizeof(s3addr);
    hdr.lpBuffers     = &iovec[0];
    hdr.dwBufferCount = 1;
    hdr.Control.buf   = pktbuf;
    /* Note: hdr.Control.len is set below */
    hdr.dwFlags       = 0;

    for (i=0;i<sizeof(addresses)/sizeof(UINT32);i++)
    {
        s1addr.sin_addr.s_addr = addresses[i];

        /* Build "server" side socket */
        s1=socket(AF_INET, SOCK_DGRAM, 0);
        if (s1 == INVALID_SOCKET)
        {
            skip("socket() failed error, some tests skipped: %d\n", WSAGetLastError());
            goto cleanup;
        }

        /* Obtain the WSARecvMsg function */
        WSAIoctl(s1, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSARecvMsg_GUID, sizeof(WSARecvMsg_GUID),
                 &pWSARecvMsg, sizeof(pWSARecvMsg), &dwBytes, NULL, NULL);
        if (!pWSARecvMsg)
        {
            win_skip("WSARecvMsg is unsupported, some tests will be skipped.\n");
            closesocket(s1);
            goto cleanup;
        }

        /* Setup the server side socket */
        rc=bind(s1, (struct sockaddr*)&s1addr, sizeof(s1addr));
        ok(rc != SOCKET_ERROR, "bind() failed error: %d\n", WSAGetLastError());
        rc=setsockopt(s1, IPPROTO_IP, IP_PKTINFO, (const char*)&yes, sizeof(yes));
        ok(rc == 0, "failed to set IPPROTO_IP flag IP_PKTINFO!\n");

        /* Build "client" side socket */
        addrlen = sizeof(s2addr);
        if (getsockname(s1, (struct sockaddr *) &s2addr, &addrlen) != 0)
        {
            skip("Failed to call getsockname, some tests skipped: %d\n", WSAGetLastError());
            closesocket(s1);
            goto cleanup;
        }
        s2addr.sin_addr.s_addr = addresses[0]; /* Always target the local adapter address */
        s2=socket(AF_INET, SOCK_DGRAM, 0);
        if (s2 == INVALID_SOCKET)
        {
            skip("socket() failed error, some tests skipped: %d\n", WSAGetLastError());
            closesocket(s1);
            goto cleanup;
        }

        /* Test an empty message header */
        rc=pWSARecvMsg(s1, NULL, NULL, NULL, NULL);
        err=WSAGetLastError();
        ok(rc == SOCKET_ERROR && err == WSAEFAULT, "WSARecvMsg() failed error: %d (ret = %d)\n", err, rc);

        /*
         * Send a packet from the client to the server and test for specifying
         * a short control header.
         */
        SetLastError(0xdeadbeef);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
        hdr.Control.len = 1;
        rc=pWSARecvMsg(s1, &hdr, &dwSize, NULL, NULL);
        err=WSAGetLastError();
        ok(rc == SOCKET_ERROR && err == WSAEMSGSIZE && (hdr.dwFlags & MSG_CTRUNC),
           "WSARecvMsg() failed error: %d (ret: %d, flags: %d)\n", err, rc, hdr.dwFlags);
        hdr.dwFlags = 0; /* Reset flags */

        /* Perform another short control header test, this time with an overlapped receive */
        hdr.Control.len = 1;
        rc=pWSARecvMsg(s1, &hdr, NULL, &ov, NULL);
        err=WSAGetLastError();
        ok(rc != 0 && err == WSA_IO_PENDING, "WSARecvMsg() failed error: %d\n", err);
        SetLastError(0xdeadbeef);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
        if (WaitForSingleObject(ov.hEvent, 100) != WAIT_OBJECT_0)
        {
            skip("Server side did not receive packet, some tests skipped.\n");
            closesocket(s2);
            closesocket(s1);
            continue;
        }
        dwFlags = 0;
        WSAGetOverlappedResult(s1, &ov, NULL, FALSE, &dwFlags);
        ok(dwFlags == 0,
           "WSAGetOverlappedResult() returned unexpected flags %d!\n", dwFlags);
        ok(hdr.dwFlags == MSG_CTRUNC,
           "WSARecvMsg() overlapped operation set unexpected flags %d.\n", hdr.dwFlags);
        hdr.dwFlags = 0; /* Reset flags */

        /*
         * Setup an overlapped receive, send a packet, then wait for the packet to be retrieved
         * on the server end and check that the returned packet matches what was sent.
         */
        hdr.Control.len = sizeof(pktbuf);
        rc=pWSARecvMsg(s1, &hdr, NULL, &ov, NULL);
        err=WSAGetLastError();
        ok(rc != 0 && err == WSA_IO_PENDING, "WSARecvMsg() failed error: %d\n", err);
        ok(hdr.Control.len == sizeof(pktbuf),
           "WSARecvMsg() control length mismatch (%d != sizeof pktbuf).\n", hdr.Control.len);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        if (WaitForSingleObject(ov.hEvent, 100) != WAIT_OBJECT_0)
        {
            skip("Server side did not receive packet, some tests skipped.\n");
            closesocket(s2);
            closesocket(s1);
            continue;
        }
        dwSize = 0;
        WSAGetOverlappedResult(s1, &ov, &dwSize, FALSE, NULL);
        ok(dwSize == sizeof(msg),
           "WSARecvMsg() buffer length does not match transmitted data!\n");
        ok(strncmp(iovec[0].buf, msg, sizeof(msg)) == 0,
           "WSARecvMsg() buffer does not match transmitted data!\n");
        ok(hdr.Control.len == IP_PKTINFO_LEN,
           "WSARecvMsg() control length mismatch (%d).\n", hdr.Control.len);

        /* Test for the expected IP_PKTINFO return information. */
        foundhdr = FALSE;
        for (cmsg = WSA_CMSG_FIRSTHDR(&hdr); cmsg != NULL; cmsg = WSA_CMSG_NXTHDR(&hdr, cmsg))
        {
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
            {
                struct in_pktinfo *pi = (struct in_pktinfo *)WSA_CMSG_DATA(cmsg);

                ok(pi->ipi_addr.s_addr == s2addr.sin_addr.s_addr, "destination ip mismatch!\n");
                foundhdr = TRUE;
            }
        }
        ok(foundhdr, "IP_PKTINFO header information was not returned!\n");

        closesocket(s2);
        closesocket(s1);
    }

cleanup:
    CloseHandle(ov.hEvent);
}

/************* Array containing the tests to run **********/

#define STD_STREAM_SOCKET \
            SOCK_STREAM, \
            0, \
            SERVERIP, \
            SERVERPORT

static test_setup tests [] =
{
    /* Test 0: synchronous client and server */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        simple_server,
        {
            NULL,
            0,
            64
        },
        simple_client,
        {
            NULL,
            0,
            128
        }
    },
    /* Test 1: event-driven client, synchronous server */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        simple_server,
        {
            NULL,
            0,
            64
        },
        event_client,
        {
            NULL,
            WSA_FLAG_OVERLAPPED,
            128
        }
    },
    /* Test 2: synchronous client, non-blocking server via select() */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        select_server,
        {
            NULL,
            0,
            64
        },
        simple_client,
        {
            NULL,
            0,
            128
        }
    },
    /* Test 3: OOB client, OOB server */
    {
        {
            STD_STREAM_SOCKET,
            128,
            16,
            1
        },
        oob_server,
        {
            NULL,
            0,
            128
        },
        oob_client,
        {
            NULL,
            0,
            128
        }
    },
    /* Test 4: synchronous mixed client and server */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        simple_server,
        {
            NULL,
            0,
            64
        },
        simple_mixed_client,
        {
            NULL,
            0,
            128
        }
    }
};

static void test_UDP(void)
{
    /* This function tests UDP sendto() and recvfrom(). UDP is unreliable, so it is
       possible that this test fails due to dropped packets. */

    /* peer 0 receives data from all other peers */
    struct sock_info peer[NUM_UDP_PEERS];
    char buf[16];
    int ss, i, n_recv, n_sent;

    memset (buf,0,sizeof(buf));
    for ( i = NUM_UDP_PEERS - 1; i >= 0; i-- ) {
        ok ( ( peer[i].s = socket ( AF_INET, SOCK_DGRAM, 0 ) ) != INVALID_SOCKET, "UDP: socket failed\n" );

        peer[i].addr.sin_family         = AF_INET;
        peer[i].addr.sin_addr.s_addr    = inet_addr ( SERVERIP );

        if ( i == 0 ) {
            peer[i].addr.sin_port       = htons ( SERVERPORT );
        } else {
            peer[i].addr.sin_port       = htons ( 0 );
        }

        do_bind ( peer[i].s, (struct sockaddr *) &peer[i].addr, sizeof( peer[i].addr ) );

        /* test getsockname() to get peer's port */
        ss = sizeof ( peer[i].addr );
        ok ( getsockname ( peer[i].s, (struct sockaddr *) &peer[i].addr, &ss ) != SOCKET_ERROR, "UDP: could not getsockname()\n" );
        ok ( peer[i].addr.sin_port != htons ( 0 ), "UDP: bind() did not associate port\n" );
    }

    /* test getsockname() */
    ok ( peer[0].addr.sin_port == htons ( SERVERPORT ), "UDP: getsockname returned incorrect peer port\n" );

    for ( i = 1; i < NUM_UDP_PEERS; i++ ) {
        /* send client's ip */
        memcpy( buf, &peer[i].addr.sin_port, sizeof(peer[i].addr.sin_port) );
        n_sent = sendto ( peer[i].s, buf, sizeof(buf), 0, (struct sockaddr*) &peer[0].addr, sizeof(peer[0].addr) );
        ok ( n_sent == sizeof(buf), "UDP: sendto() sent wrong amount of data or socket error: %d\n", n_sent );
    }

    for ( i = 1; i < NUM_UDP_PEERS; i++ ) {
        n_recv = recvfrom ( peer[0].s, buf, sizeof(buf), 0,(struct sockaddr *) &peer[0].peer, &ss );
        ok ( n_recv == sizeof(buf), "UDP: recvfrom() received wrong amount of data or socket error: %d\n", n_recv );
        ok ( memcmp ( &peer[0].peer.sin_port, buf, sizeof(peer[0].addr.sin_port) ) == 0, "UDP: port numbers do not match\n" );
    }
}

static DWORD WINAPI do_getservbyname( void *param )
{
    struct {
        const char *name;
        const char *proto;
        int port;
    } serv[2] = { {"domain", "udp", 53}, {"telnet", "tcp", 23} };

    HANDLE *starttest = param;
    int i, j;
    struct servent *pserv[2];

    ok ( WaitForSingleObject ( *starttest, TEST_TIMEOUT * 1000 ) != WAIT_TIMEOUT,
         "test_getservbyname: timeout waiting for start signal\n" );

    /* ensure that necessary buffer resizes are completed */
    for ( j = 0; j < 2; j++) {
        pserv[j] = getservbyname ( serv[j].name, serv[j].proto );
    }

    for ( i = 0; i < NUM_QUERIES / 2; i++ ) {
        for ( j = 0; j < 2; j++ ) {
            pserv[j] = getservbyname ( serv[j].name, serv[j].proto );
            ok ( pserv[j] != NULL || broken(pserv[j] == NULL) /* win8, fixed in win81 */,
                 "getservbyname could not retrieve information for %s: %d\n", serv[j].name, WSAGetLastError() );
            if ( !pserv[j] ) continue;
            ok ( pserv[j]->s_port == htons(serv[j].port),
                 "getservbyname returned the wrong port for %s: %d\n", serv[j].name, ntohs(pserv[j]->s_port) );
            ok ( !strcmp ( pserv[j]->s_proto, serv[j].proto ),
                 "getservbyname returned the wrong protocol for %s: %s\n", serv[j].name, pserv[j]->s_proto );
            ok ( !strcmp ( pserv[j]->s_name, serv[j].name ),
                 "getservbyname returned the wrong name for %s: %s\n", serv[j].name, pserv[j]->s_name );
        }

        ok ( pserv[0] == pserv[1] || broken(pserv[0] != pserv[1]) /* win8, fixed in win81 */,
             "getservbyname: winsock resized servent buffer when not necessary\n" );
    }

    return 0;
}

static void test_getservbyname(void)
{
    int i;
    HANDLE starttest, thread[NUM_THREADS];
    DWORD thread_id[NUM_THREADS];

    starttest = CreateEventA ( NULL, 1, 0, "test_getservbyname_starttest" );

    /* create threads */
    for ( i = 0; i < NUM_THREADS; i++ ) {
        thread[i] = CreateThread ( NULL, 0, do_getservbyname, &starttest, 0, &thread_id[i] );
    }

    /* signal threads to start */
    SetEvent ( starttest );

    for ( i = 0; i < NUM_THREADS; i++) {
        WaitForSingleObject ( thread[i], TEST_TIMEOUT * 1000 );
    }
}

static void test_WSASocket(void)
{
    SOCKET sock = INVALID_SOCKET;
    WSAPROTOCOL_INFOA *pi;
    int wsaproviders[] = {IPPROTO_TCP, IPPROTO_IP};
    int autoprotocols[] = {IPPROTO_TCP, IPPROTO_UDP};
    int items, err, size, socktype, i, j;
    UINT pi_size;

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, 0, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "Expected 10022, received %d\n", err);

    sock = WSASocketA(AF_INET, 0, 0, NULL, 0, 0);
    ok(sock != INVALID_SOCKET, "WSASocketA should have succeeded\n");
    closesocket(sock);

    sock = WSASocketA(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
    ok(sock != INVALID_SOCKET, "WSASocketA should have succeeded\n");
    closesocket(sock);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, SOCK_STREAM, -1, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, -1, IPPROTO_UDP, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAESOCKTNOSUPPORT, "Expected 10044, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, -1, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "Expected 10022, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(AF_INET, -1, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAESOCKTNOSUPPORT, "Expected 10044, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(AF_INET, 0, -1, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, -1, -1, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAESOCKTNOSUPPORT, "Expected 10044, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(-1, SOCK_STREAM, IPPROTO_UDP, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEAFNOSUPPORT, "Expected 10047, received %d\n", err);

    sock = WSASocketA(AF_INET, 0, IPPROTO_TCP, NULL, 0, 0);
    ok(sock != INVALID_SOCKET, "WSASocketA should have succeeded\n");
    closesocket(sock);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, SOCK_STREAM, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "Expected 10022, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, 0, 0xdead, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(AF_INET, 0xdead, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAESOCKTNOSUPPORT, "Expected 10044, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSASocketA(0, 0xdead, 0, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "Expected 10022, received %d\n", err);

    sock = WSASocketA(0, 0, IPPROTO_TCP, NULL, 0, 0);
    ok(sock != INVALID_SOCKET, "WSASocketA should have succeeded\n");
    closesocket(sock);

    /* SOCK_STREAM does not support IPPROTO_UDP */
    SetLastError(0xdeadbeef);
    ok(WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_UDP, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

    /* SOCK_DGRAM does not support IPPROTO_TCP */
    SetLastError(0xdeadbeef);
    ok(WSASocketA(AF_INET, SOCK_DGRAM, IPPROTO_TCP, NULL, 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

    /* Set pi_size explicitly to a value below 2*sizeof(WSAPROTOCOL_INFOA)
     * to avoid a crash on win98.
     */
    pi_size = 0;
    items = WSAEnumProtocolsA(wsaproviders, NULL, &pi_size);
    ok(items == SOCKET_ERROR, "WSAEnumProtocolsA({6,0}, NULL, 0) returned %d\n",
            items);
    err = WSAGetLastError();
    ok(err == WSAENOBUFS, "WSAEnumProtocolsA error is %d, not WSAENOBUFS(%d)\n",
            err, WSAENOBUFS);

    pi = HeapAlloc(GetProcessHeap(), 0, pi_size);
    ok(pi != NULL, "Failed to allocate memory\n");
    if (pi == NULL) {
        skip("Can't continue without memory.\n");
        return;
    }

    items = WSAEnumProtocolsA(wsaproviders, pi, &pi_size);
    ok(items != SOCKET_ERROR, "WSAEnumProtocolsA failed, last error is %d\n",
            WSAGetLastError());

    if (items == 0) {
        skip("No protocols enumerated.\n");
        HeapFree(GetProcessHeap(), 0, pi);
        return;
    }

    sock = WSASocketA(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                      FROM_PROTOCOL_INFO, &pi[0], 0, 0);
    ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
            WSAGetLastError());
    closesocket(sock);

    /* find what parameters are used first: plain parameters or protocol info struct */
    pi[0].iProtocol = -1;
    pi[0].iSocketType = -1;
    pi[0].iAddressFamily = -1;
    ok(WSASocketA(0, 0, IPPROTO_UDP, &pi[0], 0, 0) == INVALID_SOCKET,
       "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEAFNOSUPPORT, "Expected 10047, received %d\n", err);

    pi[0].iProtocol = 0;
    pi[0].iSocketType = 0;
    pi[0].iAddressFamily = 0;
    sock = WSASocketA(0, 0, IPPROTO_UDP, &pi[0], 0, 0);
    if(sock != INVALID_SOCKET)
    {
      win_skip("must work only in OS <= 2003\n");
      closesocket(sock);
    }
    else
    {
      err = WSAGetLastError();
      ok(err == WSAEAFNOSUPPORT, "Expected 10047, received %d\n", err);
    }

    pi[0].iProtocol = IPPROTO_UDP;
    pi[0].iSocketType = SOCK_DGRAM;
    pi[0].iAddressFamily = AF_INET;
    sock = WSASocketA(0, 0, 0, &pi[0], 0, 0);
    ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
            WSAGetLastError());
    size = sizeof(socktype);
    socktype = 0xdead;
    err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
    ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
    ok(socktype == SOCK_DGRAM, "Wrong socket type, expected %d received %d\n",
       SOCK_DGRAM, socktype);
    closesocket(sock);

    sock = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, &pi[0], 0, 0);
    ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
            WSAGetLastError());
    size = sizeof(socktype);
    socktype = 0xdead;
    err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
    ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
    ok(socktype == SOCK_STREAM, "Wrong socket type, expected %d received %d\n",
       SOCK_STREAM, socktype);
    closesocket(sock);

    HeapFree(GetProcessHeap(), 0, pi);

    pi_size = 0;
    items = WSAEnumProtocolsA(NULL, NULL, &pi_size);
    ok(items == SOCKET_ERROR, "WSAEnumProtocolsA(NULL, NULL, 0) returned %d\n",
            items);
    err = WSAGetLastError();
    ok(err == WSAENOBUFS, "WSAEnumProtocolsA error is %d, not WSAENOBUFS(%d)\n",
            err, WSAENOBUFS);

    pi = HeapAlloc(GetProcessHeap(), 0, pi_size);
    ok(pi != NULL, "Failed to allocate memory\n");
    if (pi == NULL) {
        skip("Can't continue without memory.\n");
        return;
    }

    items = WSAEnumProtocolsA(NULL, pi, &pi_size);
    ok(items != SOCKET_ERROR, "WSAEnumProtocolsA failed, last error is %d\n",
            WSAGetLastError());

    /* when no protocol and socket type are specified the first entry
     * from WSAEnumProtocols that has the flag PFL_MATCHES_PROTOCOL_ZERO
     * is returned */
    sock = WSASocketA(AF_INET, 0, 0, NULL, 0, 0);
    ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
            WSAGetLastError());

    size = sizeof(socktype);
    socktype = 0xdead;
    err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
    ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
    for(i = 0; i < items; i++)
    {
        if(pi[i].dwProviderFlags & PFL_MATCHES_PROTOCOL_ZERO)
        {
            ok(socktype == pi[i].iSocketType, "Wrong socket type, expected %d received %d\n",
               pi[i].iSocketType, socktype);
             break;
        }
    }
    ok(i != items, "Creating a socket without protocol and socket type didn't work\n");
    closesocket(sock);

    /* when no socket type is specified the first entry from WSAEnumProtocols
     * that matches the protocol is returned */
    for (i = 0; i < sizeof(autoprotocols) / sizeof(autoprotocols[0]); i++)
    {
        sock = WSASocketA(0, 0, autoprotocols[i], NULL, 0, 0);
        ok(sock != INVALID_SOCKET, "Failed to create socket for protocol %d, received %d\n",
                autoprotocols[i], WSAGetLastError());

        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err, "getsockopt failed with %d\n", WSAGetLastError());

        for (err = 1, j = 0; j < items; j++)
        {
            if (pi[j].iProtocol == autoprotocols[i])
            {
                if (socktype == pi[j].iSocketType)
                    err = 0;
                else
                    ok(0, "Wrong socket type, expected %d received %d\n",
                       pi[j].iSocketType, socktype);
                break;
            }
        }
        ok(!err, "Protocol %d not found in WSAEnumProtocols\n", autoprotocols[i]);

        closesocket(sock);
    }

    HeapFree(GetProcessHeap(), 0, pi);

    SetLastError(0xdeadbeef);
    /* starting on vista the socket function returns error during the socket
       creation and no longer in the socket operations (sendto, readfrom) */
    sock = WSASocketA(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
    if (sock == INVALID_SOCKET)
    {
        err = WSAGetLastError();
        ok(err == WSAEACCES, "Expected 10013, received %d\n", err);
        skip("SOCK_RAW is not supported\n");
    }
    else
    {
        trace("SOCK_RAW is supported\n");

        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
        ok(socktype == SOCK_RAW, "Wrong socket type, expected %d received %d\n",
           SOCK_RAW, socktype);
        closesocket(sock);

        todo_wine {
        sock = WSASocketA(0, 0, IPPROTO_RAW, NULL, 0, 0);
        ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
           WSAGetLastError());
        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
        ok(socktype == SOCK_RAW, "Wrong socket type, expected %d received %d\n",
           SOCK_RAW, socktype);
        closesocket(sock);
        }

        sock = WSASocketA(AF_INET, SOCK_RAW, IPPROTO_TCP, NULL, 0, 0);
        ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
           WSAGetLastError());
        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
        ok(socktype == SOCK_RAW, "Wrong socket type, expected %d received %d\n",
           SOCK_RAW, socktype);
        closesocket(sock);
    }

    /* IPX socket tests */

    SetLastError(0xdeadbeef);
    sock = WSASocketA(AF_IPX, SOCK_DGRAM, NSPROTO_IPX, NULL, 0, 0);
    if (sock == INVALID_SOCKET)
    {
        err = WSAGetLastError();
        todo_wine_if (err == WSAEPROTONOSUPPORT)
        ok(err == WSAEAFNOSUPPORT || broken(err == WSAEPROTONOSUPPORT), "Expected 10047, received %d\n", err);
        skip("IPX is not supported\n");
    }
    else
    {
        WSAPROTOCOL_INFOA info;
        closesocket(sock);

        trace("IPX is supported\n");

        sock = WSASocketA(0, 0, NSPROTO_IPX, NULL, 0, 0);
        ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
                WSAGetLastError());

        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
        ok(socktype == SOCK_DGRAM, "Wrong socket type, expected %d received %d\n",
           SOCK_DGRAM, socktype);

        /* check socket family, type and protocol */
        size = sizeof(WSAPROTOCOL_INFOA);
        err = getsockopt(sock, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) &info, &size);
        ok(!err,"getsockopt failed with %d\n", WSAGetLastError());
        ok(info.iProtocol == NSPROTO_IPX, "expected protocol %d, received %d\n",
           NSPROTO_IPX, info.iProtocol);
        ok(info.iAddressFamily == AF_IPX, "expected family %d, received %d\n",
           AF_IPX, info.iProtocol);
        ok(info.iSocketType == SOCK_DGRAM, "expected type %d, received %d\n",
           SOCK_DGRAM, info.iSocketType);
        closesocket(sock);

        /* SOCK_STREAM does not support NSPROTO_IPX */
        SetLastError(0xdeadbeef);
        ok(WSASocketA(AF_IPX, SOCK_STREAM, NSPROTO_IPX, NULL, 0, 0) == INVALID_SOCKET,
           "WSASocketA should have failed\n");
        err = WSAGetLastError();
        ok(err == WSAEPROTONOSUPPORT, "Expected 10043, received %d\n", err);

        /* test extended IPX support - that is adding any number between 0 and 255
         * to the IPX protocol value will make it be used as IPX packet type */
        for(i = 0;i <= 255;i += 17)
        {
          SetLastError(0xdeadbeef);
          sock = WSASocketA(0, 0, NSPROTO_IPX + i, NULL, 0, 0);
          ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
                  WSAGetLastError());

          size = sizeof(int);
          socktype = -1;
          err = getsockopt(sock, NSPROTO_IPX, IPX_PTYPE, (char *) &socktype, &size);
          ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
          ok(socktype == i, "Wrong IPX packet type, expected %d received %d\n",
             i, socktype);

          closesocket(sock);
        }
    }
}

static void test_WSADuplicateSocket(void)
{
    SOCKET source, dupsock;
    WSAPROTOCOL_INFOA info;
    DWORD err;
    struct sockaddr_in addr;
    int socktype, size, addrsize, ret;
    char teststr[] = "TEST", buffer[16];

    source = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    ok(source != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    /* test invalid parameters */
    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(0, 0, NULL), "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, 0, NULL),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, ~0, &info),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(0, GetCurrentProcessId(), &info),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, received %d\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, GetCurrentProcessId(), NULL),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, received %d\n", err);

    /* test returned structure */
    memset(&info, 0, sizeof(info));
    ok(!WSADuplicateSocketA(source, GetCurrentProcessId(), &info),
       "WSADuplicateSocketA should have worked\n");

    ok(info.iProtocol == IPPROTO_TCP, "expected protocol %d, received %d\n",
       IPPROTO_TCP, info.iProtocol);
    ok(info.iAddressFamily == AF_INET, "expected family %d, received %d\n",
       AF_INET, info.iProtocol);
    ok(info.iSocketType == SOCK_STREAM, "expected type %d, received %d\n",
       SOCK_STREAM, info.iSocketType);

    dupsock = WSASocketA(0, 0, 0, &info, 0, 0);
    ok(dupsock != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    closesocket(dupsock);
    closesocket(source);

    /* create a socket, bind it, duplicate it then send data on source and
     * receive in the duplicated socket */
    source = WSASocketA(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
    ok(source != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    memset(&info, 0, sizeof(info));
    ok(!WSADuplicateSocketA(source, GetCurrentProcessId(), &info),
       "WSADuplicateSocketA should have worked\n");

    ok(info.iProtocol == IPPROTO_UDP, "expected protocol %d, received %d\n",
       IPPROTO_UDP, info.iProtocol);
    ok(info.iAddressFamily == AF_INET, "expected family %d, received %d\n",
       AF_INET, info.iProtocol);
    ok(info.iSocketType == SOCK_DGRAM, "expected type %d, received %d\n",
       SOCK_DGRAM, info.iSocketType);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ok(!bind(source, (struct sockaddr*)&addr, sizeof(addr)),
       "bind should have worked\n");

    /* read address to find out the port number to be used in sendto */
    memset(&addr, 0, sizeof(addr));
    addrsize = sizeof(addr);
    ok(!getsockname(source, (struct sockaddr *) &addr, &addrsize),
       "getsockname should have worked\n");
    ok(addr.sin_port, "socket port should be != 0\n");

    dupsock = WSASocketA(0, 0, 0, &info, 0, 0);
    ok(dupsock != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    size = sizeof(int);
    ret = getsockopt(dupsock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
    ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
    ok(socktype == SOCK_DGRAM, "Wrong socket type, expected %d received %d\n",
       SOCK_DGRAM, socktype);

    set_blocking(source, TRUE);

    /* send data on source socket */
    addrsize = sizeof(addr);
    size = sendto(source, teststr, sizeof(teststr), 0, (struct sockaddr *) &addr, addrsize);
    ok(size == sizeof(teststr), "got %d (err %d)\n", size, WSAGetLastError());

    /* receive on duplicated socket */
    addrsize = sizeof(addr);
    memset(buffer, 0, sizeof(buffer));
    size = recvfrom(dupsock, buffer, sizeof(teststr), 0, (struct sockaddr *) &addr, &addrsize);
    ok(size == sizeof(teststr), "got %d (err %d)\n", size, WSAGetLastError());
    buffer[sizeof(teststr) - 1] = 0;
    ok(!strcmp(buffer, teststr), "expected '%s', received '%s'\n", teststr, buffer);

    closesocket(dupsock);
    closesocket(source);

    /* show that the source socket need to be bound before the duplicated
     * socket is created */
    source = WSASocketA(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
    ok(source != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    memset(&info, 0, sizeof(info));
    ok(!WSADuplicateSocketA(source, GetCurrentProcessId(), &info),
       "WSADuplicateSocketA should have worked\n");

    dupsock = WSASocketA(0, 0, 0, &info, 0, 0);
    ok(dupsock != INVALID_SOCKET, "WSASocketA should have succeeded\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ok(!bind(source, (struct sockaddr*)&addr, sizeof(addr)),
       "bind should have worked\n");

    /* read address to find out the port number to be used in sendto */
    memset(&addr, 0, sizeof(addr));
    addrsize = sizeof(addr);
    ok(!getsockname(source, (struct sockaddr *) &addr, &addrsize),
       "getsockname should have worked\n");
    ok(addr.sin_port, "socket port should be != 0\n");

    set_blocking(source, TRUE);

    addrsize = sizeof(addr);
    size = sendto(source, teststr, sizeof(teststr), 0, (struct sockaddr *) &addr, addrsize);
    ok(size == sizeof(teststr), "got %d (err %d)\n", size, WSAGetLastError());

    SetLastError(0xdeadbeef);
    addrsize = sizeof(addr);
    memset(buffer, 0, sizeof(buffer));
    todo_wine {
    ok(recvfrom(dupsock, buffer, sizeof(teststr), 0, (struct sockaddr *) &addr, &addrsize) == -1,
       "recvfrom should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, received %d\n", err);
    }

    closesocket(dupsock);
    closesocket(source);
}

static void test_WSAEnumNetworkEvents(void)
{
    SOCKET s, s2;
    int sock_type[] = {SOCK_STREAM, SOCK_DGRAM, SOCK_STREAM}, i, j, k, l;
    struct sockaddr_in address;
    HANDLE event;
    WSANETWORKEVENTS net_events;

    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;

    /* This test follows the steps from bugs 10204 and 24946 */
    for (l = 0; l < 2; l++)
    {
        for (i = 0; i < sizeof(sock_type) / sizeof(sock_type[0]); i++)
        {
            if (i == 2)
                ok(!tcp_socketpair(&s, &s2), "Test[%d]: creating socket pair failed\n", i);
            else
            {
                s = socket(AF_INET, sock_type[i], 0);
                ok (s != SOCKET_ERROR, "Test[%d]: failed to create socket\n", i);
                ok (!bind(s, (struct sockaddr*) &address, sizeof(address)), "Test[%d]: bind failed\n", i);
            }
            event = WSACreateEvent();
            ok (event != NULL, "Test[%d]: failed to create event\n", i);
            for (j = 0; j < 5; j++) /* Repeat sometimes and the result must be the same */
            {
                /* When the TCP socket is not connected NO events will be returned.
                 * When connected and no data pending it will get the write event.
                 * UDP sockets don't have connections so as soon as they are bound
                 * they can read/write data. Since nobody is sendind us data only
                 * the write event will be returned and ONLY once.
                 */
                ok (!WSAEventSelect(s, event, FD_READ | FD_WRITE), "Test[%d]: WSAEventSelect failed\n", i);
                memset(&net_events, 0xAB, sizeof(net_events));
                ok (!WSAEnumNetworkEvents(s, l == 0 ? event : NULL, &net_events),
                    "Test[%d]: WSAEnumNetworkEvents failed\n", i);
                if (i >= 1 && j == 0) /* FD_WRITE is SET on first try for UDP and connected TCP */
                {
                    todo_wine_if (i == 0 || net_events.lNetworkEvents == 0)
                        ok (net_events.lNetworkEvents == FD_WRITE, "Test[%d]: expected 2, got %d\n",
                            i, net_events.lNetworkEvents);
                }
                else
                {
                    todo_wine_if (i != 0 && net_events.lNetworkEvents != 0)
                        ok (net_events.lNetworkEvents == 0, "Test[%d]: expected 0, got %d\n",
                            i, net_events.lNetworkEvents);
                }
                for (k = 0; k < FD_MAX_EVENTS; k++)
                {
                    if (net_events.lNetworkEvents & (1 << k))
                    {
                        ok (net_events.iErrorCode[k] == 0x0, "Test[%d][%d]: expected 0x0, got 0x%x\n",
                            i, k, net_events.iErrorCode[k]);
                    }
                    else
                    {
                        /* Bits that are not set in lNetworkEvents MUST not be changed */
                        ok (net_events.iErrorCode[k] == 0xABABABAB, "Test[%d][%d]: expected 0xABABABAB, got 0x%x\n",
                            i, k, net_events.iErrorCode[k]);
                    }
                }
            }
            closesocket(s);
            WSACloseEvent(event);
            if (i == 2) closesocket(s2);
        }
    }
}

static void test_WSAAddressToStringA(void)
{
    SOCKET v6 = INVALID_SOCKET;
    INT ret;
    DWORD len;
    int GLE;
    SOCKADDR_IN sockaddr;
    CHAR address[22]; /* 12 digits + 3 dots + ':' + 5 digits + '\0' */

    CHAR expect1[] = "0.0.0.0";
    CHAR expect2[] = "255.255.255.255";
    CHAR expect3[] = "0.0.0.0:65535";
    CHAR expect4[] = "255.255.255.255:65535";

    SOCKADDR_IN6 sockaddr6;
    CHAR address6[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */

    CHAR addr6_1[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    CHAR addr6_2[] = {0x20,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    CHAR addr6_3[] = {0x20,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x01};

    CHAR expect6_1[] = "::1";
    CHAR expect6_2[] = "20ab::1";
    CHAR expect6_3[] = "[20ab::2001]:33274";
    CHAR expect6_3_2[] = "[20ab::2001%4660]:33274";
    CHAR expect6_3_3[] = "20ab::2001%4660";

    len = 0;

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEFAULT) || (ret == 0), 
        "WSAAddressToStringA() failed unexpectedly: WSAGetLastError()=%d, ret=%d\n",
        GLE, ret );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !strcmp( address, expect1 ), "Expected: %s, got: %s\n", expect1, address );
    ok( len == sizeof( expect1 ), "Got size %d\n", len);

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0xffffffff;

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !strcmp( address, expect2 ), "Expected: %s, got: %s\n", expect2, address );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0xffff;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !strcmp( address, expect3 ), "Expected: %s, got: %s\n", expect3, address );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0xffff;
    sockaddr.sin_addr.s_addr = 0xffffffff;

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !strcmp( address, expect4 ), "Expected: %s, got: %s\n", expect4, address );
    ok( len == sizeof( expect4 ), "Got size %d\n", len);

    /*check to see it IPv6 is available */
    v6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (v6 == INVALID_SOCKET) {
        skip("Could not create IPv6 socket (LastError: %d; %d expected if IPv6 not available).\n",
            WSAGetLastError(), WSAEAFNOSUPPORT);
        goto end;
    }
    /* Test a short IPv6 address */
    len = sizeof(address6);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_1, sizeof(addr6_1));

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !strcmp( address6, expect6_1 ), "Expected: %s, got: %s\n", expect6_1, address6 );
    ok( len == sizeof(expect6_1), "Got size %d\n", len);

    /* Test a longer IPv6 address */
    len = sizeof(address6);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_2, sizeof(addr6_2));

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !strcmp( address6, expect6_2 ), "Expected: %s, got: %s\n", expect6_2, address6 );
    ok( len == sizeof(expect6_2), "Got size %d\n", len);

    /* Test IPv6 address and port number */
    len = sizeof(address6);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0xfa81;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !strcmp( address6, expect6_3 ), "Expected: %s, got: %s\n", expect6_3, address6 );
    ok( len == sizeof(expect6_3), "Got size %d\n", len );

    /* Test IPv6 address, port number and scope_id */
    len = sizeof(address6);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0xfa81;
    sockaddr6.sin6_scope_id = 0x1234;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !strcmp( address6, expect6_3_2 ), "Expected: %s, got: %s\n", expect6_3_2, address6 );
    ok( len == sizeof(expect6_3_2), "Got size %d\n", len );

    /* Test IPv6 address and scope_id */
    len = sizeof(address6);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0x1234;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringA( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringA() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !strcmp( address6, expect6_3_3 ), "Expected: %s, got: %s\n", expect6_3_3, address6 );
    ok( len == sizeof(expect6_3_3), "Got size %d\n", len );

end:
    if (v6 != INVALID_SOCKET)
        closesocket(v6);
}

static void test_WSAAddressToStringW(void)
{
    SOCKET v6 = INVALID_SOCKET;
    INT ret;
    DWORD len;
    int GLE;
    SOCKADDR_IN sockaddr;
    WCHAR address[22]; /* 12 digits + 3 dots + ':' + 5 digits + '\0' */

    WCHAR expect1[] = { '0','.','0','.','0','.','0', 0 };
    WCHAR expect2[] = { '2','5','5','.','2','5','5','.','2','5','5','.','2','5','5', 0 };
    WCHAR expect3[] = { '0','.','0','.','0','.','0', ':', '6', '5', '5', '3', '5', 0 };
    WCHAR expect4[] = { '2','5','5','.','2','5','5','.','2','5','5','.','2','5','5', ':',
                        '6', '5', '5', '3', '5', 0 };

    SOCKADDR_IN6 sockaddr6;
    WCHAR address6[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */

    CHAR addr6_1[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    CHAR addr6_2[] = {0x20,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    CHAR addr6_3[] = {0x20,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x01};

    WCHAR expect6_1[] = {':',':','1',0};
    WCHAR expect6_2[] = {'2','0','a','b',':',':','1',0};
    WCHAR expect6_3[] = {'[','2','0','a','b',':',':','2','0','0','1',']',':','3','3','2','7','4',0};
    WCHAR expect6_3_2[] = {'[','2','0','a','b',':',':','2','0','0','1','%','4','6','6','0',']',':','3','3','2','7','4',0};
    WCHAR expect6_3_3[] = {'2','0','a','b',':',':','2','0','0','1','%','6','5','5','3','4',0};

    len = 0;

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEFAULT) || (ret == 0), 
        "WSAAddressToStringW() failed unexpectedly: WSAGetLastError()=%d, ret=%d\n",
        GLE, ret );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !lstrcmpW( address, expect1 ), "Expected different address string\n" );
    ok( len == sizeof( expect1 )/sizeof( WCHAR ), "Got size %d\n", len);

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0xffffffff;

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !lstrcmpW( address, expect2 ), "Expected different address string\n" );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0xffff;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !lstrcmpW( address, expect3 ), "Expected different address string\n" );

    len = sizeof(address);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0xffff;
    sockaddr.sin_addr.s_addr = 0xffffffff;

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr, sizeof(sockaddr), NULL, address, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !lstrcmpW( address, expect4 ), "Expected different address string\n" );
    ok( len == sizeof( expect4 )/sizeof( WCHAR ), "Got %d\n", len);

    /*check to see it IPv6 is available */
    v6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (v6 == INVALID_SOCKET) {
        skip("Could not create IPv6 socket (LastError: %d; %d expected if IPv6 not available).\n",
            WSAGetLastError(), WSAEAFNOSUPPORT);
        goto end;
    }

    /* Test a short IPv6 address */
    len = sizeof(address6)/sizeof(WCHAR);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_1, sizeof(addr6_1));

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !lstrcmpW( address6, expect6_1 ), "Wrong string returned\n" );
    ok( len == sizeof(expect6_1)/sizeof(WCHAR), "Got %d\n", len);

    /* Test a longer IPv6 address */
    len = sizeof(address6)/sizeof(WCHAR);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_2, sizeof(addr6_2));

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );

    ok( !lstrcmpW( address6, expect6_2 ), "Wrong string returned\n" );
    ok( len == sizeof(expect6_2)/sizeof(WCHAR), "Got %d\n", len);

    /* Test IPv6 address and port number */
    len = sizeof(address6)/sizeof(WCHAR);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0xfa81;
    sockaddr6.sin6_scope_id = 0;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !lstrcmpW( address6, expect6_3 ),
        "Expected: %s, got: %s\n", wine_dbgstr_w(expect6_3), wine_dbgstr_w(address6) );
    ok( len == sizeof(expect6_3)/sizeof(WCHAR), "Got %d\n", len );

    /* Test IPv6 address, port number and scope_id */
    len = sizeof(address6)/sizeof(WCHAR);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0xfa81;
    sockaddr6.sin6_scope_id = 0x1234;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !lstrcmpW( address6, expect6_3_2 ),
        "Expected: %s, got: %s\n", wine_dbgstr_w(expect6_3_2), wine_dbgstr_w(address6) );
    ok( len == sizeof(expect6_3_2)/sizeof(WCHAR), "Got %d\n", len );

    /* Test IPv6 address and scope_id */
    len = sizeof(address6)/sizeof(WCHAR);

    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = 0x0000;
    sockaddr6.sin6_scope_id = 0xfffe;
    memcpy (sockaddr6.sin6_addr.s6_addr, addr6_3, sizeof(addr6_3));

    ret = WSAAddressToStringW( (SOCKADDR*)&sockaddr6, sizeof(sockaddr6), NULL, address6, &len );
    ok( !ret, "WSAAddressToStringW() failed unexpectedly: %d\n", WSAGetLastError() );
    ok( !lstrcmpW( address6, expect6_3_3 ),
        "Expected: %s, got: %s\n", wine_dbgstr_w(expect6_3_3), wine_dbgstr_w(address6) );
    ok( len == sizeof(expect6_3_3)/sizeof(WCHAR), "Got %d\n", len );

end:
    if (v6 != INVALID_SOCKET)
        closesocket(v6);
}

static void test_WSAStringToAddressA(void)
{
    INT ret, len;
    SOCKADDR_IN sockaddr;
    SOCKADDR_IN6 sockaddr6;
    int GLE;

    CHAR address1[] = "0.0.0.0";
    CHAR address2[] = "127.127.127.127";
    CHAR address3[] = "255.255.255.255";
    CHAR address4[] = "127.127.127.127:65535";
    CHAR address5[] = "255.255.255.255:65535";
    CHAR address6[] = "::1";
    CHAR address7[] = "[::1]";
    CHAR address8[] = "[::1]:65535";
    CHAR address9[] = "2001::1";

    len = 0;
    sockaddr.sin_family = AF_INET;

    ret = WSAStringToAddressA( address1, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( ret == SOCKET_ERROR, "WSAStringToAddressA() succeeded unexpectedly: %d\n",
        WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressA( address1, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0,
        "WSAStringToAddressA() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressA( address2, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0x7f7f7f7f,
        "WSAStringToAddressA() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressA( address3, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == 0 && sockaddr.sin_addr.s_addr == 0xffffffff) || 
        (ret == SOCKET_ERROR && (GLE == ERROR_INVALID_PARAMETER || GLE == WSAEINVAL)),
        "WSAStringToAddressA() failed unexpectedly: %d\n", GLE );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressA( address4, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0x7f7f7f7f && sockaddr.sin_port == 0xffff,
        "WSAStringToAddressA() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressA( address5, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == 0 && sockaddr.sin_addr.s_addr == 0xffffffff && sockaddr.sin_port == 0xffff) || 
        (ret == SOCKET_ERROR && (GLE == ERROR_INVALID_PARAMETER || GLE == WSAEINVAL)),
        "WSAStringToAddressA() failed unexpectedly: %d\n", GLE );

    len = sizeof(sockaddr);

    ret = WSAStringToAddressA( address9, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressA() should have failed with %d\n", GLE );

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address6, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    if (ret == SOCKET_ERROR)
    {
        win_skip("IPv6 not supported\n");
        return;
    }

    GLE = WSAGetLastError();
    ok( ret == 0, "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address7, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0, "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address8, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 && sockaddr6.sin6_port == 0xffff,
        "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);

    ret = WSAStringToAddressA( address7 + 1, AF_INET6, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() should have failed with %d\n", GLE );

    len = sizeof(sockaddr6);

    ret = WSAStringToAddressA( address8 + 1, AF_INET6, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() should have failed with %d\n", GLE );
}

static void test_WSAStringToAddressW(void)
{
    INT ret, len;
    SOCKADDR_IN sockaddr, *sin;
    SOCKADDR_IN6 sockaddr6;
    SOCKADDR_STORAGE sockaddr_storage;
    int GLE;

    WCHAR address1[] = { '0','.','0','.','0','.','0', 0 };
    WCHAR address2[] = { '1','2','7','.','1','2','7','.','1','2','7','.','1','2','7', 0 };
    WCHAR address3[] = { '2','5','5','.','2','5','5','.','2','5','5','.','2','5','5', 0 };
    WCHAR address4[] = { '1','2','7','.','1','2','7','.','1','2','7','.','1','2','7',
                         ':', '6', '5', '5', '3', '5', 0 };
    WCHAR address5[] = { '2','5','5','.','2','5','5','.','2','5','5','.','2','5','5', ':',
                         '6', '5', '5', '3', '5', 0 };
    WCHAR address6[] = {':',':','1','\0'};
    WCHAR address7[] = {'[',':',':','1',']','\0'};
    WCHAR address8[] = {'[',':',':','1',']',':','6','5','5','3','5','\0'};
    WCHAR address9[] = {'2','0','0','1',':',':','1','\0'};

    len = 0;
    sockaddr.sin_family = AF_INET;

    ret = WSAStringToAddressW( address1, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( ret == SOCKET_ERROR, "WSAStringToAddressW() failed unexpectedly: %d\n",
        WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address1, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0,
        "WSAStringToAddressW() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address2, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0x7f7f7f7f,
        "WSAStringToAddressW() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address3, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == 0 && sockaddr.sin_addr.s_addr == 0xffffffff) || 
        (ret == SOCKET_ERROR && (GLE == ERROR_INVALID_PARAMETER || GLE == WSAEINVAL)),
        "WSAStringToAddressW() failed unexpectedly: %d\n", GLE );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address4, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( !ret && sockaddr.sin_addr.s_addr == 0x7f7f7f7f && sockaddr.sin_port == 0xffff,
        "WSAStringToAddressW() failed unexpectedly: %d\n", WSAGetLastError() );

    len = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address5, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    ok( (ret == 0 && sockaddr.sin_addr.s_addr == 0xffffffff && sockaddr.sin_port == 0xffff) || 
        (ret == SOCKET_ERROR && (GLE == ERROR_INVALID_PARAMETER || GLE == WSAEINVAL)),
        "WSAStringToAddressW() failed unexpectedly: %d\n", GLE );

    /* Test with a larger buffer than necessary */
    len = sizeof(sockaddr_storage);
    sin = (SOCKADDR_IN *)&sockaddr_storage;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = 0;

    ret = WSAStringToAddressW( address5, AF_INET, NULL, (SOCKADDR*)sin, &len );
    ok( (ret == 0 && sin->sin_addr.s_addr == 0xffffffff && sin->sin_port == 0xffff) ||
        (ret == SOCKET_ERROR && (GLE == ERROR_INVALID_PARAMETER || GLE == WSAEINVAL)),
        "WSAStringToAddressW() failed unexpectedly: %d\n", GLE );
    ok( len == sizeof(SOCKADDR_IN), "unexpected length %d\n", len );

    len = sizeof(sockaddr);

    ret = WSAStringToAddressW( address9, AF_INET, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() should have failed with %d\n", GLE );

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address6, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    if (ret == SOCKET_ERROR)
    {
        win_skip("IPv6 not supported\n");
        return;
    }

    GLE = WSAGetLastError();
    ok( ret == 0, "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address7, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0, "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address8, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 && sockaddr6.sin6_port == 0xffff,
        "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);

    ret = WSAStringToAddressW( address7 + 1, AF_INET6, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() should have failed with %d\n", GLE );

    len = sizeof(sockaddr6);

    ret = WSAStringToAddressW( address8 + 1, AF_INET6, NULL, (SOCKADDR*)&sockaddr, &len );
    GLE = WSAGetLastError();
    ok( (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() should have failed with %d\n", GLE );
}

static DWORD WINAPI SelectReadThread(void *param)
{
    select_thread_params *par = param;
    fd_set readfds;
    int ret;
    struct sockaddr_in addr;
    struct timeval select_timeout;

    memset(&readfds, 0, sizeof(readfds));
    FD_ZERO(&readfds);
    FD_SET(par->s, &readfds);
    select_timeout.tv_sec=5;
    select_timeout.tv_usec=0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVERIP);
    addr.sin_port = htons(SERVERPORT);

    do_bind(par->s, (struct sockaddr *)&addr, sizeof(addr));
    wsa_ok(listen(par->s, SOMAXCONN ), 0 ==, "SelectReadThread (%x): listen failed: %d\n");

    SetEvent(server_ready);
    ret = select(par->s+1, &readfds, NULL, NULL, &select_timeout);
    par->ReadKilled = (ret == 1);

    return 0;
}

static DWORD WINAPI SelectCloseThread(void *param)
{
    SOCKET s = *(SOCKET*)param;
    Sleep(500);
    closesocket(s);
    return 0;
}

static void test_errors(void)
{
    SOCKET sock;
    SOCKADDR_IN  SockAddr;
    int ret, err;

    WSASetLastError(NO_ERROR);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    ok( (sock != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );
    memset(&SockAddr, 0, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(6924);
    SockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = connect(sock, (PSOCKADDR)&SockAddr, sizeof(SockAddr));
    ok( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got: %d\n", ret );
    if (ret == SOCKET_ERROR)
    {
        err = WSAGetLastError();
        ok( (err == WSAECONNREFUSED), "expected WSAECONNREFUSED, got: %d\n", err );
    }

    {
        TIMEVAL timeval;
        fd_set set = {1, {sock}};

        timeval.tv_sec = 0;
        timeval.tv_usec = 50000;

        ret = select(1, NULL, &set, NULL, &timeval);
        ok( (ret == 0), "expected 0 (timeout), got: %d\n", ret );
    }

    ret = closesocket(sock);
    ok ( (ret == 0), "closesocket failed unexpectedly: %d\n", WSAGetLastError());
}

static void test_listen(void)
{
    SOCKET fdA, fdB;
    int ret, acceptc, olen = sizeof(acceptc);
    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVERPORT);

    /* invalid socket tests */
    SetLastError(0xdeadbeef);
    ok ((listen(0, 0) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAENOTSOCK, "expected 10038, received %d\n", ret);

    SetLastError(0xdeadbeef);
    ok ((listen(0xdeadbeef, 0) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAENOTSOCK, "expected 10038, received %d\n", ret);

    /* tcp tests */
    fdA = socket(AF_INET, SOCK_STREAM, 0);
    ok ((fdA != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );

    fdB = socket(AF_INET, SOCK_STREAM, 0);
    ok ((fdB != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );

    SetLastError(0xdeadbeef);
    ok ((listen(fdA, -2) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAEINVAL, "expected 10022, received %d\n", ret);

    SetLastError(0xdeadbeef);
    ok ((listen(fdA, 1) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAEINVAL, "expected 10022, received %d\n", ret);

    SetLastError(0xdeadbeef);
    ok ((listen(fdA, SOMAXCONN) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAEINVAL, "expected 10022, received %d\n", ret);

    ok (!bind(fdA, (struct sockaddr*) &address, sizeof(address)), "bind failed\n");

    SetLastError(0xdeadbeef);
    ok (bind(fdB, (struct sockaddr*) &address, sizeof(address)), "bind should have failed\n");
    ok (ret == WSAEINVAL, "expected 10022, received %d\n", ret);

    acceptc = 0xdead;
    ret = getsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char*)&acceptc, &olen);
    ok (!ret, "getsockopt failed\n");
    ok (acceptc == 0, "SO_ACCEPTCONN should be 0, received %d\n", acceptc);

    ok (!listen(fdA, 0), "listen failed\n");
    ok (!listen(fdA, SOMAXCONN), "double listen failed\n");

    acceptc = 0xdead;
    ret = getsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char*)&acceptc, &olen);
    ok (!ret, "getsockopt failed\n");
    ok (acceptc == 1, "SO_ACCEPTCONN should be 1, received %d\n", acceptc);

    SetLastError(0xdeadbeef);
    ok ((listen(fdB, SOMAXCONN) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAEINVAL, "expected 10022, received %d\n", ret);

    ret = closesocket(fdB);
    ok (ret == 0, "closesocket failed unexpectedly: %d\n", ret);

    fdB = socket(AF_INET, SOCK_STREAM, 0);
    ok ((fdB != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );

    SetLastError(0xdeadbeef);
    ok (bind(fdB, (struct sockaddr*) &address, sizeof(address)), "bind should have failed\n");
    ret = WSAGetLastError();
    ok (ret == WSAEADDRINUSE, "expected 10048, received %d\n", ret);

    ret = closesocket(fdA);
    ok (ret == 0, "closesocket failed unexpectedly: %d\n", ret);
    ret = closesocket(fdB);
    ok (ret == 0, "closesocket failed unexpectedly: %d\n", ret);
}

#define FD_ZERO_ALL() { FD_ZERO(&readfds); FD_ZERO(&writefds); FD_ZERO(&exceptfds); }
#define FD_SET_ALL(s) { FD_SET(s, &readfds); FD_SET(s, &writefds); FD_SET(s, &exceptfds); }
static void test_select(void)
{
    static char tmp_buf[1024];

    SOCKET fdListen, fdRead, fdWrite;
    fd_set readfds, writefds, exceptfds;
    unsigned int maxfd;
    int ret, len;
    char buffer;
    struct timeval select_timeout;
    struct sockaddr_in address;
    select_thread_params thread_params;
    HANDLE thread_handle;
    DWORD ticks, id;

    fdRead = socket(AF_INET, SOCK_STREAM, 0);
    ok( (fdRead != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );
    fdWrite = socket(AF_INET, SOCK_STREAM, 0);
    ok( (fdWrite != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );

    maxfd = fdRead;
    if (fdWrite > maxfd)
        maxfd = fdWrite;

    FD_ZERO_ALL();
    FD_SET_ALL(fdRead);
    FD_SET_ALL(fdWrite);
    select_timeout.tv_sec=0;
    select_timeout.tv_usec=0;

    ticks = GetTickCount();
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ticks = GetTickCount() - ticks;
    ok(ret == 0, "select should not return any socket handles\n");
    ok(ticks < 10, "select was blocking for %u ms, expected < 10 ms\n", ticks);
    ok(!FD_ISSET(fdRead, &readfds), "FD should not be set\n");
    ok(!FD_ISSET(fdWrite, &writefds), "FD should not be set\n");
    ok(!FD_ISSET(fdRead, &exceptfds), "FD should not be set\n");
    ok(!FD_ISSET(fdWrite, &exceptfds), "FD should not be set\n");
 
    FD_ZERO_ALL();
    FD_SET_ALL(fdRead);
    FD_SET_ALL(fdWrite);
    select_timeout.tv_sec=0;
    select_timeout.tv_usec=500;

    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 0, "select should not return any socket handles\n");
    ok(!FD_ISSET(fdRead, &readfds), "FD should not be set\n");
    ok(!FD_ISSET(fdWrite, &writefds), "FD should not be set\n");
    ok(!FD_ISSET(fdRead, &exceptfds), "FD should not be set\n");
    ok(!FD_ISSET(fdWrite, &exceptfds), "FD should not be set\n");

    ok ((listen(fdWrite, SOMAXCONN) == SOCKET_ERROR), "listen did not fail\n");
    ret = closesocket(fdWrite);
    ok ( (ret == 0), "closesocket failed unexpectedly: %d\n", ret);

    thread_params.s = fdRead;
    thread_params.ReadKilled = FALSE;
    server_ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    thread_handle = CreateThread (NULL, 0, SelectReadThread, &thread_params, 0, &id );
    ok ( (thread_handle != NULL), "CreateThread failed unexpectedly: %d\n", GetLastError());

    WaitForSingleObject (server_ready, INFINITE);
    Sleep(200);
    ret = closesocket(fdRead);
    ok ( (ret == 0), "closesocket failed unexpectedly: %d\n", ret);

    WaitForSingleObject (thread_handle, 1000);
    ok ( thread_params.ReadKilled, "closesocket did not wake up select\n");
    ret = recv(fdRead, &buffer, 1, MSG_PEEK);
    ok( (ret == -1), "peek at closed socket expected -1 got %d\n", ret);

    /* Test selecting invalid handles */
    FD_ZERO_ALL();

    SetLastError(0);
    ret = select(maxfd+1, 0, 0, 0, &select_timeout);
    ok ( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got %i\n", ret);
    ok ( WSAGetLastError() == WSAEINVAL, "expected WSAEINVAL, got %i\n", WSAGetLastError());

    SetLastError(0);
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok ( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got %i\n", ret);
    ok ( WSAGetLastError() == WSAEINVAL, "expected WSAEINVAL, got %i\n", WSAGetLastError());

    FD_SET(INVALID_SOCKET, &readfds);
    SetLastError(0);
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok ( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got %i\n", ret);
    ok ( WSAGetLastError() == WSAENOTSOCK, "expected WSAENOTSOCK, got %i\n", WSAGetLastError());
    ok ( !FD_ISSET(fdRead, &readfds), "FD should not be set\n");

    FD_ZERO(&readfds);
    FD_SET(INVALID_SOCKET, &writefds);
    SetLastError(0);
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok ( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got %i\n", ret);
    ok ( WSAGetLastError() == WSAENOTSOCK, "expected WSAENOTSOCK, got %i\n", WSAGetLastError());
    ok ( !FD_ISSET(fdRead, &writefds), "FD should not be set\n");

    FD_ZERO(&writefds);
    FD_SET(INVALID_SOCKET, &exceptfds);
    SetLastError(0);
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok ( (ret == SOCKET_ERROR), "expected SOCKET_ERROR, got %i\n", ret);
    ok ( WSAGetLastError() == WSAENOTSOCK, "expected WSAENOTSOCK, got %i\n", WSAGetLastError());
    ok ( !FD_ISSET(fdRead, &exceptfds), "FD should not be set\n");

    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    maxfd = fdRead;
    if(fdWrite > maxfd) maxfd = fdWrite;

    FD_ZERO(&readfds);
    FD_SET(fdRead, &readfds);
    ret = select(fdRead+1, &readfds, NULL, NULL, &select_timeout);
    ok(!ret, "select returned %d\n", ret);

    FD_ZERO(&writefds);
    FD_SET(fdWrite, &writefds);
    ret = select(fdWrite+1, NULL, &writefds, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &writefds), "fdWrite socket is not in the set\n");

    /* tests for overlapping fd_set pointers */
    FD_ZERO(&readfds);
    FD_SET(fdWrite, &readfds);
    ret = select(fdWrite+1, &readfds, &readfds, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");

    FD_ZERO(&readfds);
    FD_SET(fdWrite, &readfds);
    FD_SET(fdRead, &readfds);
    ret = select(maxfd+1, &readfds, &readfds, NULL, &select_timeout);
    ok(ret == 2, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");

    ok(send(fdWrite, "test", 4, 0) == 4, "failed to send data\n");
    FD_ZERO(&readfds);
    FD_SET(fdRead, &readfds);
    ret = select(fdRead+1, &readfds, NULL, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");

    FD_ZERO(&readfds);
    FD_SET(fdWrite, &readfds);
    FD_SET(fdRead, &readfds);
    ret = select(maxfd+1, &readfds, &readfds, NULL, &select_timeout);
    ok(ret == 2, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");

#if ROSTESTS_233_IS_FIXED
    while(1) {
        FD_ZERO(&writefds);
        FD_SET(fdWrite, &writefds);
        ret = select(fdWrite+1, NULL, &writefds, NULL, &select_timeout);
        if(!ret) break;
        ok(send(fdWrite, tmp_buf, sizeof(tmp_buf), 0) > 0, "failed to send data\n");
    }
#endif /* ROSTESTS_233_IS_FIXED */

    FD_ZERO(&readfds);
    FD_SET(fdWrite, &readfds);
    FD_SET(fdRead, &readfds);
    ret = select(maxfd+1, &readfds, &readfds, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(!FD_ISSET(fdWrite, &readfds), "fdWrite socket is in the set\n");
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");

    ok(send(fdRead, "test", 4, 0) == 4, "failed to send data\n");
    Sleep(100);
    FD_ZERO(&readfds);
    FD_SET(fdWrite, &readfds);
    FD_SET(fdRead, &readfds);
    ret = select(maxfd+1, &readfds, &readfds, NULL, &select_timeout);
    ok(ret == 2, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");

    closesocket(fdRead);
    closesocket(fdWrite);

    /* select() works in 3 distinct states:
     * - to check if a connection attempt ended with success or error;
     * - to check if a pending connection is waiting for acceptance;
     * - to check for data to read, availability for write and OOB data
     *
     * The tests below ensure that all conditions are tested.
     */
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;
    len = sizeof(address);
    fdListen = setup_server_socket(&address, &len);
    select_timeout.tv_sec = 1;
    select_timeout.tv_usec = 250000;

    /* When no events are pending select returns 0 with no error */
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 0, "expected 0, got %d\n", ret);

    /* When a socket is attempting to connect the listening socket receives the read descriptor */
    fdWrite = setup_connector_socket(&address, len, TRUE);
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdListen, &readfds), "fdListen socket is not in the set\n");
    len = sizeof(address);
    fdRead = accept(fdListen, (struct sockaddr*) &address, &len);
    ok(fdRead != INVALID_SOCKET, "expected a valid socket\n");

    /* The connector is signaled through the write descriptor */
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    FD_SET_ALL(fdRead);
    FD_SET_ALL(fdWrite);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 2, "expected 2, got %d\n", ret);
    ok(FD_ISSET(fdWrite, &writefds), "fdWrite socket is not in the set\n");
    ok(FD_ISSET(fdRead, &writefds), "fdRead socket is not in the set\n");
    len = sizeof(id);
    id = 0xdeadbeef;
    ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char*)&id, &len);
    ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
    ok(id == 0, "expected 0, got %d\n", id);

    /* When data is received the receiver gets the read descriptor */
    ret = send(fdWrite, "1234", 4, 0);
    ok(ret == 4, "expected 4, got %d\n", ret);
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    FD_SET(fdRead, &readfds);
    FD_SET(fdRead, &exceptfds);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 4, "expected 4, got %d\n", ret);
    ok(!strcmp(tmp_buf, "1234"), "data received differs from sent\n");

    /* When OOB data is received the socket is set in the except descriptor */
    ret = send(fdWrite, "A", 1, MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    FD_SET(fdRead, &readfds);
    FD_SET(fdRead, &exceptfds);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdRead, &exceptfds), "fdRead socket is not in the set\n");
    tmp_buf[0] = 0xAF;
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(tmp_buf[0] == 'A', "expected 'A', got 0x%02X\n", tmp_buf[0]);

    /* If the socket is OOBINLINED it will not receive the OOB in except fds */
    ret = 1;
    ret = setsockopt(fdRead, SOL_SOCKET, SO_OOBINLINE, (char*) &ret, sizeof(ret));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ret = send(fdWrite, "A", 1, MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    FD_SET(fdRead, &readfds);
    FD_SET(fdRead, &exceptfds);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");
    tmp_buf[0] = 0xAF;
    SetLastError(0xdeadbeef);
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), MSG_OOB);
    ok(ret == SOCKET_ERROR, "expected SOCKET_ERROR, got %d\n", ret);
    ok(GetLastError() == WSAEINVAL, "expected 10022, got %d\n", GetLastError());
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(tmp_buf[0] == 'A', "expected 'A', got 0x%02X\n", tmp_buf[0]);

    /* When the connection is closed the socket is set in the read descriptor */
    ret = closesocket(fdRead);
    ok(ret == 0, "expected 0, got %d\n", ret);
    FD_ZERO_ALL();
    FD_SET_ALL(fdListen);
    FD_SET(fdWrite, &readfds);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    ret = recv(fdWrite, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 0, "expected 0, got %d\n", ret);

    /* When a connection is attempted to a non-listening socket it will get to the except descriptor */
    ret = closesocket(fdWrite);
    ok(ret == 0, "expected 0, got %d\n", ret);
    ret = closesocket(fdListen);
    ok(ret == 0, "expected 0, got %d\n", ret);
    len = sizeof(address);
    fdWrite = setup_connector_socket(&address, len, TRUE);
    FD_ZERO_ALL();
    FD_SET(fdWrite, &writefds);
    FD_SET(fdWrite, &exceptfds);
    select_timeout.tv_sec = 2; /* requires more time to realize it will not connect */
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    len = sizeof(id);
    id = 0xdeadbeef;
    ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char*)&id, &len);
    ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
    ok(id == WSAECONNREFUSED, "expected 10061, got %d\n", id);
    ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");
    ok(select_timeout.tv_usec == 250000, "select timeout should not have changed\n");
    closesocket(fdWrite);

    /* Try select() on a closed socket after connection */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    closesocket(fdRead);
    FD_ZERO_ALL();
    FD_SET_ALL(fdWrite);
    FD_SET_ALL(fdRead);
    SetLastError(0xdeadbeef);
    ret = select(0, &readfds, NULL, &exceptfds, &select_timeout);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
todo_wine
    ok(GetLastError() == WSAENOTSOCK, "expected 10038, got %d\n", GetLastError());
    /* descriptor sets are unchanged */
    ok(readfds.fd_count == 2, "expected 2, got %d\n", readfds.fd_count);
    ok(exceptfds.fd_count == 2, "expected 2, got %d\n", exceptfds.fd_count);
    closesocket(fdWrite);

#if ROSTESTS_233_IS_FIXED

    /* Close the socket currently being selected in a thread - bug 38399 */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %d\n", GetLastError());
    FD_ZERO_ALL();
    FD_SET_ALL(fdWrite);
    ret = select(0, &readfds, NULL, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);
    /* test again with only the except descriptor */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %d\n", GetLastError());
    FD_ZERO_ALL();
    FD_SET(fdWrite, &exceptfds);
    SetLastError(0xdeadbeef);
    ret = select(0, NULL, NULL, &exceptfds, &select_timeout);
todo_wine
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
todo_wine
    ok(GetLastError() == WSAENOTSOCK, "expected 10038, got %d\n", GetLastError());
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);

#endif /* ROSTESTS_233_IS_FIXED */

    /* test UDP behavior of unbound sockets */
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 250000;
    fdWrite = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(fdWrite != INVALID_SOCKET, "socket call failed\n");
    FD_ZERO_ALL();
    FD_SET_ALL(fdWrite);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdWrite, &writefds), "fdWrite socket is not in the set\n");
    closesocket(fdWrite);
}
#undef FD_SET_ALL
#undef FD_ZERO_ALL

static DWORD WINAPI AcceptKillThread(void *param)
{
    select_thread_params *par = param;
    struct sockaddr_in address;
    int len = sizeof(address);
    SOCKET client_socket;

    SetEvent(server_ready);
    client_socket = accept(par->s, (struct sockaddr*) &address, &len);
    if (client_socket != INVALID_SOCKET)
        closesocket(client_socket);
    par->ReadKilled = (client_socket == INVALID_SOCKET);
    return 0;
}


static int CALLBACK AlwaysDeferConditionFunc(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS pQos,
                                             LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData,
                                             GROUP *g, DWORD_PTR dwCallbackData)
{
    return CF_DEFER;
}

static SOCKET setup_server_socket(struct sockaddr_in *addr, int *len)
{
    int ret, val;
    SOCKET server_socket;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        trace("error creating server socket: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    val = 1;
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));
    if (ret)
    {
        trace("error setting SO_REUSEADDR: %d\n", WSAGetLastError());
        closesocket(server_socket);
        return INVALID_SOCKET;
    }

    ret = bind(server_socket, (struct sockaddr *)addr, *len);
    if (ret)
    {
        trace("error binding server socket: %d\n", WSAGetLastError());
    }

    ret = getsockname(server_socket, (struct sockaddr *)addr, len);
    if (ret)
    {
        skip("failed to lookup bind address: %d\n", WSAGetLastError());
        closesocket(server_socket);
        return INVALID_SOCKET;
    }

    ret = listen(server_socket, 5);
    if (ret)
    {
        trace("error making server socket listen: %d\n", WSAGetLastError());
        closesocket(server_socket);
        return INVALID_SOCKET;
    }

    return server_socket;
}

static SOCKET setup_connector_socket(struct sockaddr_in *addr, int len, BOOL nonblock)
{
    int ret;
    SOCKET connector;

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create connector socket %d\n", WSAGetLastError());

    if (nonblock)
        set_blocking(connector, !nonblock);

    ret = connect(connector, (struct sockaddr *)addr, len);
    if (!nonblock)
        ok(!ret, "connecting to accepting socket failed %d\n", WSAGetLastError());
    else if (ret == SOCKET_ERROR)
    {
        DWORD error = WSAGetLastError();
        ok(error == WSAEWOULDBLOCK || error == WSAEINPROGRESS,
           "expected 10035 or 10036, got %d\n", error);
    }

    return connector;
}

static void test_accept(void)
{
    int ret;
    SOCKET server_socket, accepted = INVALID_SOCKET, connector;
    struct sockaddr_in address;
    SOCKADDR_STORAGE ss, ss_empty;
    int socklen;
    select_thread_params thread_params;
    HANDLE thread_handle = NULL;
    DWORD id;

    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;

    socklen = sizeof(address);
    server_socket = setup_server_socket(&address, &socklen);
    if (server_socket == INVALID_SOCKET)
    {
        trace("error creating server socket: %d\n", WSAGetLastError());
        return;
    }

    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    trace("Blocking accept next\n");

    accepted = WSAAccept(server_socket, NULL, NULL, AlwaysDeferConditionFunc, 0);
    ok(accepted == INVALID_SOCKET && WSAGetLastError() == WSATRY_AGAIN, "Failed to defer connection, %d\n", WSAGetLastError());

    accepted = accept(server_socket, NULL, 0);
    ok(accepted != INVALID_SOCKET, "Failed to accept deferred connection, error %d\n", WSAGetLastError());

    server_ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (server_ready == INVALID_HANDLE_VALUE)
    {
        trace("error creating event: %d\n", GetLastError());
        goto done;
    }

    thread_params.s = server_socket;
    thread_params.ReadKilled = FALSE;
    thread_handle = CreateThread(NULL, 0, AcceptKillThread, &thread_params, 0, &id);
    if (thread_handle == NULL)
    {
        trace("error creating thread: %d\n", GetLastError());
        goto done;
    }

    WaitForSingleObject(server_ready, INFINITE);
    Sleep(200);
    ret = closesocket(server_socket);
    if (ret != 0)
    {
        trace("closesocket failed: %d\n", WSAGetLastError());
        goto done;
    }

    WaitForSingleObject(thread_handle, 1000);
    ok(thread_params.ReadKilled, "closesocket did not wake up accept\n");

    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    server_socket = setup_server_socket(&address, &socklen);
    if (server_socket == INVALID_SOCKET) goto done;

    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    socklen = 0;
    accepted = WSAAccept(server_socket, (struct sockaddr *)&ss, &socklen, NULL, 0);
    ok(accepted == INVALID_SOCKET && WSAGetLastError() == WSAEFAULT, "got %d\n", WSAGetLastError());
    ok(!socklen, "got %d\n", socklen);
    closesocket(connector);
    connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    accepted = WSAAccept(server_socket, NULL, NULL, NULL, 0);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    socklen = sizeof(ss);
    memset(&ss, 0, sizeof(ss));
    accepted = WSAAccept(server_socket, (struct sockaddr *)&ss, &socklen, NULL, 0);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    ok(socklen != sizeof(ss), "unexpected length\n");
    ok(ss.ss_family, "family not set\n");
    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    socklen = 0;
    accepted = accept(server_socket, (struct sockaddr *)&ss, &socklen);
    ok(accepted == INVALID_SOCKET && WSAGetLastError() == WSAEFAULT, "got %d\n", WSAGetLastError());
    ok(!socklen, "got %d\n", socklen);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    accepted = accept(server_socket, NULL, NULL);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    socklen = sizeof(ss);
    memset(&ss, 0, sizeof(ss));
    accepted = accept(server_socket, (struct sockaddr *)&ss, &socklen);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    ok(socklen != sizeof(ss), "unexpected length\n");
    ok(ss.ss_family, "family not set\n");
    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    memset(&ss, 0, sizeof(ss));
    memset(&ss_empty, 0, sizeof(ss_empty));
    accepted = accept(server_socket, (struct sockaddr *)&ss, NULL);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    ok(!memcmp(&ss, &ss_empty, sizeof(ss)), "structure is different\n");

done:
    if (accepted != INVALID_SOCKET)
        closesocket(accepted);
    if (connector != INVALID_SOCKET)
        closesocket(connector);
    if (thread_handle != NULL)
        CloseHandle(thread_handle);
    if (server_ready != INVALID_HANDLE_VALUE)
        CloseHandle(server_ready);
    if (server_socket != INVALID_SOCKET)
        closesocket(server_socket);
}

static void test_extendedSocketOptions(void)
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in sa;
    int sa_len = sizeof(struct sockaddr_in);
    int optval, optlen = sizeof(int), ret;
    BOOL bool_opt_val;
    LINGER linger_val;

    if(WSAStartup(MAKEWORD(2,0), &wsa)){
        trace("Winsock failed: %d. Aborting test\n", WSAGetLastError());
        return;
    }

    memset(&sa, 0, sa_len);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) == INVALID_SOCKET) {
        trace("Creating the socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    if(bind(sock, (struct sockaddr *) &sa, sa_len) < 0){
        trace("Failed to bind socket: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    ret = getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);

    ok(ret == 0, "getsockopt failed to query SO_MAX_MSG_SIZE, return value is 0x%08x\n", ret);
    ok((optval == 65507) || (optval == 65527),
            "SO_MAX_MSG_SIZE reported %d, expected 65507 or 65527\n", optval);

    /* IE 3 use 0xffffffff instead of SOL_SOCKET (0xffff) */
    SetLastError(0xdeadbeef);
    optval = 0xdeadbeef;
    optlen = sizeof(int);
    ret = getsockopt(sock, 0xffffffff, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d and optval: 0x%x/%d (expected SOCKET_ERROR with WSAEINVAL)\n",
        ret, WSAGetLastError(), optval, optval);

    /* more invalid values for level */
    SetLastError(0xdeadbeef);
    optval = 0xdeadbeef;
    optlen = sizeof(int);
    ret = getsockopt(sock, 0x1234ffff, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d and optval: 0x%x/%d (expected SOCKET_ERROR with WSAEINVAL)\n",
        ret, WSAGetLastError(), optval, optval);

    SetLastError(0xdeadbeef);
    optval = 0xdeadbeef;
    optlen = sizeof(int);
    ret = getsockopt(sock, 0x8000ffff, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d and optval: 0x%x/%d (expected SOCKET_ERROR with WSAEINVAL)\n",
        ret, WSAGetLastError(), optval, optval);

    SetLastError(0xdeadbeef);
    optval = 0xdeadbeef;
    optlen = sizeof(int);
    ret = getsockopt(sock, 0x00008000, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d and optval: 0x%x/%d (expected SOCKET_ERROR with WSAEINVAL)\n",
        ret, WSAGetLastError(), optval, optval);

    SetLastError(0xdeadbeef);
    optval = 0xdeadbeef;
    optlen = sizeof(int);
    ret = getsockopt(sock, 0x00000800, SO_MAX_MSG_SIZE, (char *)&optval, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAEINVAL),
        "got %d with %d and optval: 0x%x/%d (expected SOCKET_ERROR with WSAEINVAL)\n",
        ret, WSAGetLastError(), optval, optval);

    SetLastError(0xdeadbeef);
    optlen = sizeof(LINGER);
    ret = getsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger_val, &optlen);
    ok( (ret == SOCKET_ERROR) && (WSAGetLastError() == WSAENOPROTOOPT), 
        "getsockopt should fail for UDP sockets setting last error to WSAENOPROTOOPT, got %d with %d\n", 
        ret, WSAGetLastError());
    closesocket(sock);

    if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET) {
        trace("Creating the socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    if(bind(sock, (struct sockaddr *) &sa, sa_len) < 0){
        trace("Failed to bind socket: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    ret = getsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger_val, &optlen);
    ok(ret == 0, "getsockopt failed to query SO_LINGER, return value is 0x%08x\n", ret);

    optlen = sizeof(BOOL);
    ret = getsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (char *)&bool_opt_val, &optlen);
    ok(ret == 0, "getsockopt failed to query SO_DONTLINGER, return value is 0x%08x\n", ret);
    ok((linger_val.l_onoff && !bool_opt_val) || (!linger_val.l_onoff && bool_opt_val),
            "Return value of SO_DONTLINGER is %d, but SO_LINGER returned l_onoff == %d.\n",
            bool_opt_val, linger_val.l_onoff);

    closesocket(sock);
    WSACleanup();
}

static void test_getsockname(void)
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in sa_set, sa_get;
    int sa_set_len = sizeof(struct sockaddr_in);
    int sa_get_len = sa_set_len;
    static const unsigned char null_padding[] = {0,0,0,0,0,0,0,0};
    int ret;
    struct hostent *h;

    if(WSAStartup(MAKEWORD(2,0), &wsa)){
        trace("Winsock failed: %d. Aborting test\n", WSAGetLastError());
        return;
    }

    memset(&sa_set, 0, sa_set_len);

    sa_set.sin_family = AF_INET;
    sa_set.sin_port = htons(0);
    sa_set.sin_addr.s_addr = htonl(INADDR_ANY);

    if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET) {
        trace("Creating the socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    sa_get = sa_set;
    if (getsockname(sock, (struct sockaddr*) &sa_get, &sa_get_len) == 0)
        ok(0, "getsockname on unbound socket should fail\n");
    else {
        ok(WSAGetLastError() == WSAEINVAL, "getsockname on unbound socket "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEINVAL);
        ok(memcmp(&sa_get, &sa_set, sizeof(sa_get)) == 0,
            "failed getsockname modified sockaddr when it shouldn't\n");
    }

    if(bind(sock, (struct sockaddr *) &sa_set, sa_set_len) < 0){
        trace("Failed to bind socket: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    if(getsockname(sock, (struct sockaddr *) &sa_get, &sa_get_len) != 0){
        trace("Failed to call getsockname: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    ret = memcmp(sa_get.sin_zero, null_padding, 8);
    ok(ret == 0, "getsockname did not zero the sockaddr_in structure\n");

    closesocket(sock);

    h = gethostbyname("");
    if (h && h->h_length == 4) /* this test is only meaningful in IPv4 */
    {
        int i;
        for (i = 0; h->h_addr_list[i]; i++)
        {
            char ipstr[32];
            struct in_addr ip;
            ip.s_addr = *(ULONG *) h->h_addr_list[i];

            sock = socket(AF_INET, SOCK_DGRAM, 0);
            ok(sock != INVALID_SOCKET, "socket failed with %d\n", GetLastError());

            memset(&sa_set, 0, sizeof(sa_set));
            sa_set.sin_family = AF_INET;
            sa_set.sin_addr.s_addr = ip.s_addr;
            /* The same address we bind must be the same address we get */
            ret = bind(sock, (struct sockaddr*)&sa_set, sizeof(sa_set));
            ok(ret == 0, "bind failed with %d\n", GetLastError());
            sa_get_len = sizeof(sa_get);
            ret = getsockname(sock, (struct sockaddr*)&sa_get, &sa_get_len);
            ok(ret == 0, "getsockname failed with %d\n", GetLastError());
            strcpy(ipstr, inet_ntoa(sa_get.sin_addr));
            trace("testing bind on interface %s\n", ipstr);
            ok(sa_get.sin_addr.s_addr == sa_set.sin_addr.s_addr,
               "address does not match: %s != %s\n", ipstr, inet_ntoa(sa_set.sin_addr));

            closesocket(sock);
        }
    }

    WSACleanup();
}

static void test_dns(void)
{
    struct hostent *h;
    union memaddress
    {
        char *chr;
        void *mem;
    } addr;
    char **ptr;
    int acount;

    h = gethostbyname("");
    ok(h != NULL, "gethostbyname(\"\") failed with %d\n", h_errno);

    /* Use an address with valid alias names if possible */
    h = gethostbyname("source.winehq.org");
    if(!h)
    {
        skip("Can't test the hostent structure because gethostbyname failed\n");
        return;
    }

    /* The returned struct must be allocated in a very strict way. First we need to
     * count how many aliases there are because they must be located right after
     * the struct hostent size. Knowing the amount of aliases we know the exact
     * location of the first IP returned. Rule valid for >= XP, for older OS's
     * it's somewhat the opposite. */
    addr.mem = h + 1;
    if(h->h_addr_list == addr.mem) /* <= W2K */
    {
        win_skip("Skipping hostent tests since this OS is unsupported\n");
        return;
    }

    ok(h->h_aliases == addr.mem,
       "hostent->h_aliases should be in %p, it is in %p\n", addr.mem, h->h_aliases);

    for(ptr = h->h_aliases, acount = 1; *ptr; ptr++) acount++;
    addr.chr += sizeof(*ptr) * acount;
    ok(h->h_addr_list == addr.mem,
       "hostent->h_addr_list should be in %p, it is in %p\n", addr.mem, h->h_addr_list);

    for(ptr = h->h_addr_list, acount = 1; *ptr; ptr++) acount++;

    addr.chr += sizeof(*ptr) * acount;
    ok(h->h_addr_list[0] == addr.mem,
       "hostent->h_addr_list[0] should be in %p, it is in %p\n", addr.mem, h->h_addr_list[0]);
}

#ifndef __REACTOS__
/* Our winsock headers don't define gethostname because it conflicts with the
 * definition in unistd.h. Define it here to get rid of the warning. */

int WINAPI gethostname(char *name, int namelen);
#endif

static void test_gethostbyname(void)
{
    struct hostent *he;
    struct in_addr **addr_list;
    char name[256], first_ip[16];
    int ret, i, count;
    PMIB_IPFORWARDTABLE routes = NULL;
    PIP_ADAPTER_INFO adapters = NULL, k;
    DWORD adap_size = 0, route_size = 0;
    BOOL found_default = FALSE;
    BOOL local_ip = FALSE;

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
#ifdef __REACTOS__ /* ROSTESTS-233 */
    count = 0;
    if (he != NULL)
    {
#endif
        addr_list = (struct in_addr **)he->h_addr_list;
        strcpy(first_ip, inet_ntoa(*addr_list[0]));

        trace("List of local IPs:\n");
        for(count = 0; addr_list[count] != NULL; count++)
        {
            char *ip = inet_ntoa(*addr_list[count]);
            if (!strcmp(ip, "127.0.0.1"))
                local_ip = TRUE;
            trace("%s\n", ip);
        }
#ifdef __REACTOS__ /* ROSTESTS-233 */
    }
#endif

    if (local_ip)
    {
        ok (count == 1, "expected 127.0.0.1 to be the only IP returned\n");
        skip("Only the loopback address is present, skipping tests\n");
        return;
    }

    if (!pGetAdaptersInfo || !pGetIpForwardTable)
    {
        win_skip("GetAdaptersInfo and/or GetIpForwardTable not found, skipping tests\n");
        return;
    }

    ret = pGetAdaptersInfo(NULL, &adap_size);
    ok (ret  == ERROR_BUFFER_OVERFLOW, "GetAdaptersInfo failed with a different error: %d\n", ret);
    ret = pGetIpForwardTable(NULL, &route_size, FALSE);
    ok (ret == ERROR_INSUFFICIENT_BUFFER, "GetIpForwardTable failed with a different error: %d\n", ret);

    adapters = HeapAlloc(GetProcessHeap(), 0, adap_size);
    routes = HeapAlloc(GetProcessHeap(), 0, route_size);

    ret = pGetAdaptersInfo(adapters, &adap_size);
    ok (ret  == NO_ERROR, "GetAdaptersInfo failed, error: %d\n", ret);
    ret = pGetIpForwardTable(routes, &route_size, FALSE);
    ok (ret == NO_ERROR, "GetIpForwardTable failed, error: %d\n", ret);

    /* This test only has meaning if there is more than one IP configured */
    if (adapters->Next == NULL && count == 1)
    {
        skip("Only one IP is present, skipping tests\n");
        goto cleanup;
    }

    for (i = 0; !found_default && i < routes->dwNumEntries; i++)
    {
        /* default route (ip 0.0.0.0) ? */
        if (routes->table[i].dwForwardDest) continue;

        for (k = adapters; k != NULL; k = k->Next)
        {
            char *ip;

            if (k->Index != routes->table[i].dwForwardIfIndex) continue;

            /* the first IP returned from gethostbyname must be a default route */
            ip = k->IpAddressList.IpAddress.String;
            if (!strcmp(first_ip, ip))
            {
                found_default = TRUE;
                break;
            }
        }
    }
    ok (found_default, "failed to find the first IP from gethostbyname!\n");

cleanup:
    HeapFree(GetProcessHeap(), 0, adapters);
    HeapFree(GetProcessHeap(), 0, routes);
}

static void test_gethostbyname_hack(void)
{
    struct hostent *he;
    char name[256];
    static BYTE loopback[] = {127, 0, 0, 1};
    static BYTE magic_loopback[] = {127, 12, 34, 56};
    int ret;

    ret = gethostname(name, 256);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname("localhost");
    ok(he != NULL, "gethostbyname(\"localhost\") failed: %d\n", h_errno);
    if(he)
    {
        if(he->h_length != 4)
        {
            skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
            return;
        }

        ok(memcmp(he->h_addr_list[0], loopback, he->h_length) == 0,
           "gethostbyname(\"localhost\") returned %u.%u.%u.%u\n",
           he->h_addr_list[0][0], he->h_addr_list[0][1], he->h_addr_list[0][2],
           he->h_addr_list[0][3]);
    }

    if(strcmp(name, "localhost") == 0)
    {
        skip("hostname seems to be \"localhost\", skipping test.\n");
        return;
    }

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, h_errno);
    if(he)
    {
        if(he->h_length != 4)
        {
            skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
            return;
        }

        if (he->h_addr_list[0][0] == 127)
        {
            ok(memcmp(he->h_addr_list[0], magic_loopback, he->h_length) == 0,
               "gethostbyname(\"%s\") returned %u.%u.%u.%u not 127.12.34.56\n",
               name, he->h_addr_list[0][0], he->h_addr_list[0][1],
               he->h_addr_list[0][2], he->h_addr_list[0][3]);
        }
    }

    gethostbyname("nonexistent.winehq.org");
    /* Don't check for the return value, as some braindead ISPs will kindly
     * resolve nonexistent host names to addresses of the ISP's spam pages. */
}

static void test_gethostname(void)
{
    struct hostent *he;
    char name[256];
    int ret, len;

    WSASetLastError(0xdeadbeef);
    ret = gethostname(NULL, 256);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with null buffer "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());

    len = strlen(name);
    WSASetLastError(0xdeadbeef);
    strcpy(name, "deadbeef");
    ret = gethostname(name, len);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(!strcmp(name, "deadbeef"), "name changed unexpected!\n");
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with insufficient length "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    len++;
    ret = gethostname(name, len);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
}

static void test_inet_addr(void)
{
    u_long addr;

    addr = inet_addr(NULL);
    ok(addr == INADDR_NONE, "inet_addr succeeded unexpectedly\n");
}

static void test_addr_to_print(void)
{
    char dst[16];
    char dst6[64];
    const char * pdst;
    struct in_addr in;
    struct in6_addr in6;

    u_long addr0_Num = 0x00000000;
    PCSTR addr0_Str = "0.0.0.0";
    u_long addr1_Num = 0x20201015;
    PCSTR addr1_Str = "21.16.32.32";
    u_char addr2_Num[16] = {0,0,0,0,0,0,0,0,0,0,0xff,0xfe,0xcC,0x98,0xbd,0x74};
    PCSTR addr2_Str = "::fffe:cc98:bd74";
    u_char addr3_Num[16] = {0x20,0x30,0xa4,0xb1};
    PCSTR addr3_Str = "2030:a4b1::";
    u_char addr4_Num[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0xcC,0x98,0xbd,0x74};
    PCSTR addr4_Str = "::204.152.189.116";

    /* Test IPv4 addresses */
    in.s_addr = addr0_Num;

    pdst = inet_ntoa(*((struct in_addr*)&in.s_addr));
    ok(pdst != NULL, "inet_ntoa failed %s\n", dst);
    ok(!strcmp(pdst, addr0_Str),"Address %s != %s\n", pdst, addr0_Str);

    /* Test that inet_ntoa and inet_ntop return the same value */
    in.S_un.S_addr = addr1_Num;
    pdst = inet_ntoa(*((struct in_addr*)&in.s_addr));
    ok(pdst != NULL, "inet_ntoa failed %s\n", dst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* inet_ntop became available in Vista and Win2008 */
    if (!p_inet_ntop)
    {
        win_skip("InetNtop not present, not executing tests\n");
        return;
    }

    /* Second part of test */
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, sizeof(dst));
    ok(pdst != NULL, "InetNtop failed %s\n", dst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* Test invalid parm conditions */
    pdst = p_inet_ntop(1, (void *)&in.s_addr, dst, sizeof(dst));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "Should be WSAEAFNOSUPPORT\n");

    /* Test Null destination */
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, NULL, sizeof(dst));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test zero length passed */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, 0);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test length one shorter than the address length */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, 6);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test longer length is ok */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, sizeof(dst)+1);
    ok(pdst != NULL, "The pointer should  be returned (%p)\n", pdst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* Test the IPv6 addresses */

    /* Test an zero prefixed IPV6 address */
    memcpy(in6.u.Byte, addr2_Num, sizeof(addr2_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr2_Str),"Address %s != %s\n", pdst, addr2_Str);

    /* Test an zero suffixed IPV6 address */
    memcpy(in6.s6_addr, addr3_Num, sizeof(addr3_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr3_Str),"Address %s != %s\n", pdst, addr3_Str);

    /* Test the IPv6 address contains the IPv4 address in IPv4 notation */
    memcpy(in6.s6_addr, addr4_Num, sizeof(addr4_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr4_Str),"Address %s != %s\n", pdst, addr4_Str);

    /* Test invalid parm conditions */
    memcpy(in6.u.Byte, addr2_Num, sizeof(addr2_Num));

    /* Test Null destination */
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, NULL, sizeof(dst6));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test zero length passed */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 0);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test length one shorter than the address length */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 16);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test longer length is ok */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 18);
    ok(pdst != NULL, "The pointer should be returned (%p)\n", pdst);
}
static void test_inet_pton(void)
{
    static const struct
    {
        char input[32];
        int ret;
        DWORD addr;
    }
    ipv4_tests[] =
    {
        {"",                       0, 0xdeadbeef},
        {" ",                      0, 0xdeadbeef},
        {"1.1.1.1",                1, 0x01010101},
        {"0.0.0.0",                1, 0x00000000},
        {"127.127.127.255",        1, 0xff7f7f7f},
        {"127.127.127.255:123",    0, 0xff7f7f7f},
        {"127.127.127.256",        0, 0xdeadbeef},
        {"a",                      0, 0xdeadbeef},
        {"1.1.1.0xaA",             0, 0xdeadbeef},
        {"1.1.1.0x",               0, 0xdeadbeef},
        {"1.1.1.010",              0, 0xdeadbeef},
        {"1.1.1.00",               0, 0xdeadbeef},
        {"1.1.1.0a",               0, 0x00010101},
        {"1.1.1.0o10",             0, 0x00010101},
        {"1.1.1.0b10",             0, 0x00010101},
        {"1.1.1.-2",               0, 0xdeadbeef},
        {"1",                      0, 0xdeadbeef},
        {"1.2",                    0, 0xdeadbeef},
        {"1.2.3",                  0, 0xdeadbeef},
        {"203569230",              0, 0xdeadbeef},
        {"3.4.5.6.7",              0, 0xdeadbeef},
        {" 3.4.5.6",               0, 0xdeadbeef},
        {"\t3.4.5.6",              0, 0xdeadbeef},
        {"3.4.5.6 ",               0, 0x06050403},
        {"3. 4.5.6",               0, 0xdeadbeef},
        {"[0.1.2.3]",              0, 0xdeadbeef},
        {"0x00010203",             0, 0xdeadbeef},
        {"0x2134",                 0, 0xdeadbeef},
        {"1234BEEF",               0, 0xdeadbeef},
        {"017700000001",           0, 0xdeadbeef},
        {"0777",                   0, 0xdeadbeef},
        {"2607:f0d0:1002:51::4",   0, 0xdeadbeef},
        {"::177.32.45.20",         0, 0xdeadbeef},
        {"::1/128",                0, 0xdeadbeef},
        {"::1",                    0, 0xdeadbeef},
        {":1",                     0, 0xdeadbeef},
    };

    static const struct
    {
        char input[64];
        int ret;
        unsigned short addr[8];
        int broken;
        int broken_ret;
    }
    ipv6_tests[] =
    {
        {"0000:0000:0000:0000:0000:0000:0000:0000",        1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0000:0000:0000:0000:0000:0000:0000:0001",        1, {0, 0, 0, 0, 0, 0, 0, 0x100}},
        {"0:0:0:0:0:0:0:0",                                1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:0:0:1",                                1, {0, 0, 0, 0, 0, 0, 0, 0x100}},
        {"0:0:0:0:0:0:0::",                                1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:0:13.1.68.3",                          1, {0, 0, 0, 0, 0, 0, 0x10d, 0x344}},
        {"0:0:0:0:0:0::",                                  1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0::",                                    1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:FFFF:129.144.52.38",                   1, {0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634}},
        {"0::",                                            1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:1:2:3:4:5:6:7",                                1, {0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700}},
        {"1080:0:0:0:8:800:200c:417a",                     1, {0x8010, 0, 0, 0, 0x800, 0x8, 0x0c20, 0x7a41}},
        {"0:a:b:c:d:e:f::",                                1, {0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00, 0}},
        {"1111:2222:3333:4444:5555:6666:123.123.123.123",  1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111:2222:3333:4444:5555:6666:7777:8888",        1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:0x5555:6666:7777:8888",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:x555:6666:7777:8888",        0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:0r5555:6666:7777:8888",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:r5555:6666:7777:8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777::",           1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0}},
        {"1111:2222:3333:4444:5555:6666::",                1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0}},
        {"1111:2222:3333:4444:5555:6666::8888",            1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x8888}},
        {"1111:2222:3333:4444:5555:6666::7777:8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x7777}},
        {"1111:2222:3333:4444:5555:6666:7777::8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0}},
        {"1111:2222:3333:4444:5555::",                     1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0}},
        {"1111:2222:3333:4444:5555::123.123.123.123",      1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7b7b, 0x7b7b}},
        {"1111:2222:3333:4444:5555::0x1.123.123.123",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x100}},
        {"1111:2222:3333:4444:5555::0x88",                 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0X88",                 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0X",                   0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0}},
        {"1111:2222:3333:4444:5555::0X88:7777",            0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0x8888",               0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888}},
        {"1111:2222:3333:4444:5555::0x80000000",           0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0xffff}},
        {"1111:2222:3333:4444::5555:0x012345678",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0x7856}},
        {"1111:2222:3333:4444::5555:0x123456789",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0xffff}},
        {"1111:2222:3333:4444:5555:6666:0x12345678",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777:0x80000000",  0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff}},
        {"1111:2222:3333:4444:5555:6666:7777:0x012345678", 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x7856}},
        {"1111:2222:3333:4444:5555:6666:7777:0x123456789", 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff}},
        {"111:222:333:444:555:666:777:0x123456789abcdef0", 0, {0x1101, 0x2202, 0x3303, 0x4404, 0x5505, 0x6606, 0x7707, 0xffff}},
        {"1111:2222:3333:4444:5555::08888",                0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::08888::",              0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777:fffff:",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xabab}},
        {"1111:2222:3333:4444:5555:6666::fffff:",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::fffff",                0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444::fffff",                     0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333::fffff",                          0, {0x1111, 0x2222, 0x3333, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::7777:8888",            1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555::8888",                 1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888}},
        {"1111::",                                         1, {0x1111, 0, 0, 0, 0, 0, 0, 0}},
        {"1111::123.123.123.123",                          1, {0x1111, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b}},
        {"1111::3333:4444:5555:6666:123.123.123.123",      1, {0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::3333:4444:5555:6666:7777:8888",            1, {0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::4444:5555:6666:123.123.123.123",           1, {0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::4444:5555:6666:7777:8888",                 1, {0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::5555:6666:123.123.123.123",                1, {0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::5555:6666:7777:8888",                      1, {0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::6666:123.123.123.123",                     1, {0x1111, 0, 0, 0, 0, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::6666:7777:8888",                           1, {0x1111, 0, 0, 0, 0, 0x6666, 0x7777, 0x8888}},
        {"1111::7777:8888",                                1, {0x1111, 0, 0, 0, 0, 0, 0x7777, 0x8888}},
        {"1111::8888",                                     1, {0x1111, 0, 0, 0, 0, 0, 0, 0x8888}},
        {"1:2:3:4:5:6:1.2.3.4",                            1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x201, 0x403}},
        {"1:2:3:4:5:6:7:8",                                1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800}},
        {"1:2:3:4:5:6::",                                  1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0}},
        {"1:2:3:4:5:6::8",                                 1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0x800}},
        {"2001:0000:1234:0000:0000:C1C0:ABCD:0876",        1, {0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608}},
        {"2001:0000:4136:e378:8000:63bf:3fff:fdd2",        1, {0x120, 0, 0x3641, 0x78e3, 0x80, 0xbf63, 0xff3f, 0xd2fd}},
        {"2001:0db8:0:0:0:0:1428:57ab",                    1, {0x120, 0xb80d, 0, 0, 0, 0, 0x2814, 0xab57}},
        {"2001:0db8:1234:ffff:ffff:ffff:ffff:ffff",        1, {0x120, 0xb80d, 0x3412, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},
        {"2001::CE49:7601:2CAD:DFFF:7C94:FFFE",            1, {0x120, 0, 0x49ce, 0x176, 0xad2c, 0xffdf, 0x947c, 0xfeff}},
        {"2001:db8:85a3::8a2e:370:7334",                   1, {0x120, 0xb80d, 0xa385, 0, 0, 0x2e8a, 0x7003, 0x3473}},
        {"3ffe:0b00:0000:0000:0001:0000:0000:000a",        1, {0xfe3f, 0xb, 0, 0, 0x100, 0, 0, 0xa00}},
        {"::",                                             1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::%16",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::/16",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::01234",                                        0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::0",                                            1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0",                                          1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0",                                        1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0",                                      1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0",                                    1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0:0",                                  1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0:0:0",                                1, {0, 0, 0, 0, 0, 0, 0, 0}, 1},
        {"::0:a:b:c:d:e:f",                                1, {0, 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00}, 1},
        {"::123.123.123.123",                              1, {0, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b}},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",        1, {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},
        {"':10.0.0.1",                                     0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"-1",                                             0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"02001:0000:1234:0000:0000:C1C0:ABCD:0876",       0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:00000:1234:0000:0000:C1C0:ABCD:0876",       0, {0x120, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000:01234:0000:0000:C1C0:ABCD:0876",       0, {0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000::01234.0",                             0, {0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0::b.0",                                    0, {0x120, 0, 0, 0, 0, 0, 0, 0xb00}},
        {"2001::0:b.0",                                    0, {0x120, 0, 0, 0, 0, 0, 0, 0xb00}},
        {"1.2.3.4",                                        0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1.2.3.4:1111::5555",                             0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1.2.3.4::5555",                                  0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"11112222:3333:4444:5555:6666:1.2.3.4",           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"11112222:3333:4444:5555:6666:7777:8888",         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111",                                           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"0x1111",                                         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:22223333:4444:5555:6666:1.2.3.4",           0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:22223333:4444:5555:6666:7777:8888",         0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:123456789:4444:5555:6666:7777:8888",        0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:1234567890abcdef0:4444:5555:6666:7777:888", 0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:",                                     0, {0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:1.2.3.4",                              0, {0x1111, 0x2222, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333",                                 0, {0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666::1.2.3.4",         0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x100}},
        {"1111:2222:3333:4444:5555:6666:7777:1.2.3.4",     0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x100}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:1.2.3.4",0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:9999",   0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:::",                                   0, {0x1111, 0x2222, 0, 0, 0, 0, 0, 0}},
        {"1111::5555:",                                    0, {0x1111, 0x5555, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111::3333:4444:5555:6666:7777::",               0, {0x1111, 0, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777}},
        {"1111:2222:::4444:5555:6666:1.2.3.4",             0, {0x1111, 0x2222, 0, 0, 0, 0, 0, 0}},
        {"1111::3333::5555:6666:1.2.3.4",                  0, {0x1111, 0, 0, 0, 0, 0, 0, 0x3333}},
        {"12345::6:7:8",                                   0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::001.2.3.4",                                   1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.002.3.4",                                   1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::0001.2.3.4",                                  0, {0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.0002.3.4",                                  0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.256.4",                                   0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.4294967296.4",                            0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.18446744073709551616.4",                  0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.256",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4294967296",                            0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.18446744073709551616",                  0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.300",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.300.",                                  0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2::1",                                      0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4::1",                                  0, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.",                                          0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2",                                         0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.",                                        0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3",                                       0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.",                                      0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4",                                     1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.2.3.900",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.300.4",                                   0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.256.3.4",                                   0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.256:3.4",                                   0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2a.3.4",                                    0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::256.2.3.4",                                   0, {0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1a.2.3.4",                                    0, {0x100, 0, 0, 0, 0, 0, 0, 0x1a00}},
        {"1::2::3",                                        0, {0x100, 0, 0, 0, 0, 0, 0, 0x200}},
        {"2001:0000:1234: 0000:0000:C1C0:ABCD:0876",       0, {0x120, 0, 0x3412, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000:1234:0000:0000:C1C0:ABCD:0876  0",     0, {0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608}},
        {"2001:1:1:1:1:1:255Z255X255Y255",                 0, {0x120, 0x100, 0x100, 0x100, 0x100, 0x100, 0xabab, 0xabab}},
        {"2001::FFD3::57ab",                               0, {0x120, 0, 0, 0, 0, 0, 0, 0xd3ff}},
        {":",                                              0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111:2222:3333:4444:5555:6666:1.2.3.4",         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111:2222:3333:4444:5555:6666:7777:8888",       0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111::",                                        0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::-1",                                           0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::12345678",                                     0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::123456789",                                    0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::1234567890abcdef0",                            0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::0x80000000",                                   0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::0x012345678",                                  0, {0, 0, 0, 0, 0, 0, 0, 0x7856}},
        {"::0x123456789",                                  0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::0x1234567890abcdef0",                          0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::.",                                            0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::..",                                           0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::...",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:1.2.3.4",          0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"[::]",                                           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
    };

    BYTE buffer[32];
    int i, ret;

    /* inet_ntop and inet_pton became available in Vista and Win2008 */
    if (!p_inet_ntop)
    {
        win_skip("inet_ntop is not available\n");
        return;
    }

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_INET, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pInetPtonW(AF_UNSPEC, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pInetPtonW(AF_INET, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, "127.0.0.1", buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, "2607:f0d0:1002:51::4", buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());

    for (i = 0; i < ARRAY_SIZE(ipv4_tests); ++i)
    {
        WCHAR inputW[32];
        DWORD addr;

        winetest_push_context( "Address %s", debugstr_a(ipv4_tests[i].input) );

        WSASetLastError(0xdeadbeef);
        addr = 0xdeadbeef;
        ret = p_inet_pton(AF_INET, ipv4_tests[i].input, &addr);
        ok(ret == ipv4_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());
        ok(addr == ipv4_tests[i].addr, "got addr %#08lx\n", addr);

        MultiByteToWideChar(CP_ACP, 0, ipv4_tests[i].input, -1, inputW, ARRAY_SIZE(inputW));
        WSASetLastError(0xdeadbeef);
        addr = 0xdeadbeef;
        ret = pInetPtonW(AF_INET, inputW, &addr);
        ok(ret == ipv4_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == (ret ? 0xdeadbeef : WSAEINVAL), "got error %u\n", WSAGetLastError());
        ok(addr == ipv4_tests[i].addr, "got addr %#08lx\n", addr);

        WSASetLastError(0xdeadbeef);
        addr = inet_addr(ipv4_tests[i].input);
        ok(addr == ipv4_tests[i].ret ? ipv4_tests[i].addr : INADDR_NONE, "got addr %#08lx\n", addr);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(ipv6_tests); ++i)
    {
        unsigned short addr[8];
        WCHAR inputW[64];

        winetest_push_context( "Address %s", debugstr_a(ipv6_tests[i].input) );

        WSASetLastError(0xdeadbeef);
        memset(addr, 0xab, sizeof(addr));
        ret = p_inet_pton(AF_INET6, ipv6_tests[i].input, addr);
        if (ipv6_tests[i].broken)
            ok(ret == ipv6_tests[i].ret || broken(ret == ipv6_tests[i].broken_ret), "got %d\n", ret);
        else
            ok(ret == ipv6_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());
        if (ipv6_tests[i].broken)
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)) || broken(memcmp(addr, ipv6_tests[i].addr, sizeof(addr))),
               "address didn't match\n");
        else
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)), "address didn't match\n");

        MultiByteToWideChar(CP_ACP, 0, ipv6_tests[i].input, -1, inputW, ARRAY_SIZE(inputW));
        WSASetLastError(0xdeadbeef);
        memset(addr, 0xab, sizeof(addr));
        ret = pInetPtonW(AF_INET6, inputW, addr);
        if (ipv6_tests[i].broken)
            ok(ret == ipv6_tests[i].ret || broken(ret == ipv6_tests[i].broken_ret), "got %d\n", ret);
        else
            ok(ret == ipv6_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == (ret ? 0xdeadbeef : WSAEINVAL), "got error %u\n", WSAGetLastError());
        if (ipv6_tests[i].broken)
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)) || broken(memcmp(addr, ipv6_tests[i].addr, sizeof(addr))),
               "address didn't match\n");
        else
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)), "address didn't match\n");

        winetest_pop_context();
    }
}

static void test_ioctlsocket(void)
{
    SOCKET sock, src, dst;
    struct tcp_keepalive kalive;
    struct sockaddr_in address;
    int ret, optval;
    static const LONG cmds[] = {FIONBIO, FIONREAD, SIOCATMARK};
    UINT i, bytes_rec;
    char data;
    WSABUF bufs;
    u_long arg = 0;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "Creating the socket failed: %d\n", WSAGetLastError());
    if(sock == INVALID_SOCKET)
    {
        skip("Can't continue without a socket.\n");
        return;
    }

    for(i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++)
    {
        /* broken apps like defcon pass the argp value directly instead of a pointer to it */
        ret = ioctlsocket(sock, cmds[i], (u_long *)1);
        ok(ret == SOCKET_ERROR, "ioctlsocket succeeded unexpectedly\n");
        ret = WSAGetLastError();
        ok(ret == WSAEFAULT, "expected WSAEFAULT, got %d instead\n", ret);
    }

    /* A fresh and not connected socket has no urgent data, this test shows
     * that normal(not urgent) data returns a non-zero value for SIOCATMARK. */

    ret = ioctlsocket(sock, SIOCATMARK, &arg);
    ok(ret != SOCKET_ERROR, "ioctlsocket failed unexpectedly\n");
    ok(arg, "SIOCATMARK expected a non-zero value\n");

    /* when SO_OOBINLINE is set SIOCATMARK must always return TRUE */
    optval = 1;
    ret = setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, (void *)&optval, sizeof(optval));
    ok(ret != SOCKET_ERROR, "setsockopt failed unexpectedly\n");
    arg = 0;
    ret = ioctlsocket(sock, SIOCATMARK, &arg);
    ok(ret != SOCKET_ERROR, "ioctlsocket failed unexpectedly\n");
    ok(arg, "SIOCATMARK expected a non-zero value\n");

    /* disable SO_OOBINLINE and get the same old behavior */
    optval = 0;
    ret = setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, (void *)&optval, sizeof(optval));
    ok(ret != SOCKET_ERROR, "setsockopt failed unexpectedly\n");
    arg = 0;
    ret = ioctlsocket(sock, SIOCATMARK, &arg);
    ok(ret != SOCKET_ERROR, "ioctlsocket failed unexpectedly\n");
    ok(arg, "SIOCATMARK expected a non-zero value\n");

    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &arg, 0, NULL, 0, &arg, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ret = WSAGetLastError();
    ok(ret == WSAEFAULT || broken(ret == WSAEINVAL), "expected WSAEFAULT, got %d instead\n", ret);

    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, NULL, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ret = WSAGetLastError();
    ok(ret == WSAEFAULT || broken(ret == WSAEINVAL), "expected WSAEFAULT, got %d instead\n", ret);

    make_keepalive(kalive, 0, 0, 0);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 0, 0);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 1000, 1000);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 10000, 10000);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 100, 100);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 0, 100, 100);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(struct tcp_keepalive), NULL, 0, &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    closesocket(sock);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "Creating the socket failed: %d\n", WSAGetLastError());
    if(sock == INVALID_SOCKET)
    {
        skip("Can't continue without a socket.\n");
        return;
    }

    /* test FIONREAD with a fresh and non-connected socket */
    arg = 0xdeadbeef;
    ret = ioctlsocket(sock, FIONREAD, &arg);
    ok(ret == 0, "ioctlsocket failed unexpectedly with error %d\n", WSAGetLastError());
    ok(arg == 0, "expected 0, got %u\n", arg);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr( SERVERIP );
    address.sin_port = htons( SERVERPORT );
    ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    ok(ret == 0, "bind failed unexpectedly with error %d\n", WSAGetLastError());

    ret = listen(sock, SOMAXCONN);
    ok(ret == 0, "listen failed unexpectedly with error %d\n", WSAGetLastError());

    /* test FIONREAD with listening socket */
    arg = 0xdeadbeef;
    ret = ioctlsocket(sock, FIONREAD, &arg);
    ok(ret == 0, "ioctlsocket failed unexpectedly with error %d\n", WSAGetLastError());
    ok(arg == 0, "expected 0, got %u\n", arg);

    closesocket(sock);

    if (tcp_socketpair(&src, &dst) != 0)
    {
        ok(0, "creating socket pair failed, skipping test\n");
        return;
    }

    /* test FIONREAD on TCP sockets */
    optval = 0xdeadbeef;
    ret = WSAIoctl(dst, FIONREAD, NULL, 0, &optval, sizeof(optval), &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly with error %d\n", WSAGetLastError());
    ok(optval == 0, "FIONREAD should have returned 0 bytes, got %d instead\n", optval);

    optval = 0xdeadbeef;
    ok(send(src, "TEST", 4, 0) == 4, "failed to send test data\n");
    Sleep(100);
    ret = WSAIoctl(dst, FIONREAD, NULL, 0, &optval, sizeof(optval), &arg, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly with error %d\n", WSAGetLastError());
    ok(optval == 4, "FIONREAD should have returned 4 bytes, got %d instead\n", optval);

    /* trying to read from an OOB inlined socket with MSG_OOB results in WSAEINVAL */
    set_blocking(dst, FALSE);
    i = MSG_OOB;
    SetLastError(0xdeadbeef);
    ret = recv(dst, &data, 1, i);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ret = GetLastError();
    ok(ret == WSAEWOULDBLOCK, "expected 10035, got %d\n", ret);
    bufs.len = sizeof(char);
    bufs.buf = &data;
    ret = WSARecv(dst, &bufs, 1, &bytes_rec, &i, NULL, NULL);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ret = GetLastError();
    ok(ret == WSAEWOULDBLOCK, "expected 10035, got %d\n", ret);
    optval = 1;
    ret = setsockopt(dst, SOL_SOCKET, SO_OOBINLINE, (void *)&optval, sizeof(optval));
    ok(ret != SOCKET_ERROR, "setsockopt failed unexpectedly\n");
    i = MSG_OOB;
    SetLastError(0xdeadbeef);
    ret = recv(dst, &data, 1, i);
    ok(ret == SOCKET_ERROR, "expected SOCKET_ERROR, got %d\n", ret);
    ret = GetLastError();
    ok(ret == WSAEINVAL, "expected 10022, got %d\n", ret);
    bufs.len = sizeof(char);
    bufs.buf = &data;
    ret = WSARecv(dst, &bufs, 1, &bytes_rec, &i, NULL, NULL);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ret = GetLastError();
    ok(ret == WSAEINVAL, "expected 10022, got %d\n", ret);

    closesocket(dst);
    optval = 0xdeadbeef;
    ret = WSAIoctl(dst, FIONREAD, NULL, 0, &optval, sizeof(optval), &arg, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(optval == 0xdeadbeef, "FIONREAD should not have changed last error, got %d instead\n", optval);
    closesocket(src);
}

static BOOL drain_pause = FALSE;
static DWORD WINAPI drain_socket_thread(LPVOID arg)
{
    char buffer[1024];
    SOCKET sock = *(SOCKET*)arg;
    int ret;

    while ((ret = recv(sock, buffer, sizeof(buffer), 0)) != 0)
    {
        if (ret < 0)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                fd_set readset;
                FD_ZERO(&readset);
                FD_SET(sock, &readset);
                select(sock+1, &readset, NULL, NULL, NULL);
                while (drain_pause)
                    Sleep(100);
            }
            else
                break;
        }
    }
    return 0;
}

static void test_send(void)
{
    SOCKET src = INVALID_SOCKET;
    SOCKET dst = INVALID_SOCKET;
    HANDLE hThread = NULL;
    const int buflen = 1024*1024;
    char *buffer = NULL;
    int ret, i, zero = 0;
    WSABUF buf;
    OVERLAPPED ov;
    BOOL bret;
    DWORD id, bytes_sent, dwRet;
    DWORD expected_time, connect_time;
    socklen_t optlen;

    memset(&ov, 0, sizeof(ov));

    if (tcp_socketpair(&src, &dst) != 0)
    {
        ok(0, "creating socket pair failed, skipping test\n");
        return;
    }
    expected_time = GetTickCount();

    set_blocking(dst, FALSE);
    /* force disable buffering so we can get a pending overlapped request */
    ret = setsockopt(dst, SOL_SOCKET, SO_SNDBUF, (char *) &zero, sizeof(zero));
    ok(!ret, "setsockopt SO_SNDBUF failed: %d - %d\n", ret, GetLastError());

    hThread = CreateThread(NULL, 0, drain_socket_thread, &dst, 0, &id);
    if (hThread == NULL)
    {
        ok(0, "CreateThread failed, error %d\n", GetLastError());
        goto end;
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, buflen);
    if (buffer == NULL)
    {
        ok(0, "HeapAlloc failed, error %d\n", GetLastError());
        goto end;
    }

    /* fill the buffer with some nonsense */
    for (i = 0; i < buflen; ++i)
    {
        buffer[i] = (char) i;
    }

    ret = send(src, buffer, buflen, 0);
    if (ret >= 0)
        ok(ret == buflen, "send should have sent %d bytes, but it only sent %d\n", buflen, ret);
    else
        ok(0, "send failed, error %d\n", WSAGetLastError());

    buf.buf = buffer;
    buf.len = buflen;

    ov.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %d\n", GetLastError());
    if (!ov.hEvent)
        goto end;

    bytes_sent = 0;
    WSASetLastError(12345);
    ret = WSASend(dst, &buf, 1, &bytes_sent, 0, &ov, NULL);
    ok((ret == SOCKET_ERROR && WSAGetLastError() == ERROR_IO_PENDING) || broken(bytes_sent == buflen),
       "Failed to start overlapped send %d - %d - %d/%d\n", ret, WSAGetLastError(), bytes_sent, buflen);

    /* don't check for completion yet, we may need to drain the buffer while still sending */
    set_blocking(src, FALSE);
    for (i = 0; i < buflen; ++i)
    {
        int j = 0;

        ret = recv(src, buffer, 1, 0);
        while (ret == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK && j < 100)
        {
            j++;
            Sleep(50);
            ret = recv(src, buffer, 1, 0);
        }

        ok(ret == 1, "Failed to receive data %d - %d (got %d/%d)\n", ret, GetLastError(), i, buflen);
        if (ret != 1)
            break;

        ok(buffer[0] == (char) i, "Received bad data at position %d\n", i);
    }

    dwRet = WaitForSingleObject(ov.hEvent, 1000);
    ok(dwRet == WAIT_OBJECT_0, "Failed to wait for recv message: %d - %d\n", dwRet, GetLastError());
    if (dwRet == WAIT_OBJECT_0)
    {
        bret = GetOverlappedResult((HANDLE)dst, &ov, &bytes_sent, FALSE);
        ok(bret && bytes_sent == buflen,
           "Got %d instead of %d (%d - %d)\n", bytes_sent, buflen, bret, GetLastError());
    }

    WSASetLastError(12345);
    ret = WSASend(INVALID_SOCKET, &buf, 1, NULL, 0, &ov, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK,
       "WSASend failed %d - %d\n", ret, WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASend(dst, &buf, 1, NULL, 0, &ov, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == ERROR_IO_PENDING,
       "Failed to start overlapped send %d - %d\n", ret, WSAGetLastError());

    expected_time = (GetTickCount() - expected_time) / 1000;

    connect_time = 0xdeadbeef;
    optlen = sizeof(connect_time);
    ret = getsockopt(dst, SOL_SOCKET, SO_CONNECT_TIME, (char *)&connect_time, &optlen);
    ok(!ret, "getsockopt failed %d\n", WSAGetLastError());
    ok(connect_time >= expected_time && connect_time <= expected_time + 1,
       "unexpected connect time %u, expected %u\n", connect_time, expected_time);

    connect_time = 0xdeadbeef;
    optlen = sizeof(connect_time);
    ret = getsockopt(src, SOL_SOCKET, SO_CONNECT_TIME, (char *)&connect_time, &optlen);
    ok(!ret, "getsockopt failed %d\n", WSAGetLastError());
    ok(connect_time >= expected_time && connect_time <= expected_time + 1,
       "unexpected connect time %u, expected %u\n", connect_time, expected_time);

end:
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (dst != INVALID_SOCKET)
        closesocket(dst);
    if (hThread != NULL)
    {
        dwRet = WaitForSingleObject(hThread, 500);
        ok(dwRet == WAIT_OBJECT_0, "failed to wait for thread termination: %d\n", GetLastError());
        CloseHandle(hThread);
    }
    if (ov.hEvent)
        CloseHandle(ov.hEvent);
    HeapFree(GetProcessHeap(), 0, buffer);
}

typedef struct async_message
{
    SOCKET socket;
    LPARAM lparam;
    struct async_message *next;
} async_message;

static struct async_message *messages_received;

#define WM_SOCKET (WM_USER+100)
static LRESULT CALLBACK ws2_test_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct async_message *message;

    switch (msg)
    {
    case WM_SOCKET:
        message = HeapAlloc(GetProcessHeap(), 0, sizeof(*message));
        message->socket = (SOCKET) wparam;
        message->lparam = lparam;
        message->next = NULL;

        if (messages_received)
        {
            struct async_message *last = messages_received;
            while (last->next) last = last->next;
            last->next = message;
        }
        else
            messages_received = message;
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void get_event_details(int event, int *bit, char *name)
{
    switch (event)
    {
        case FD_ACCEPT:
            if (bit) *bit = FD_ACCEPT_BIT;
            if (name) strcpy(name, "FD_ACCEPT");
            break;
        case FD_CONNECT:
            if (bit) *bit = FD_CONNECT_BIT;
            if (name) strcpy(name, "FD_CONNECT");
            break;
        case FD_READ:
            if (bit) *bit = FD_READ_BIT;
            if (name) strcpy(name, "FD_READ");
            break;
        case FD_OOB:
            if (bit) *bit = FD_OOB_BIT;
            if (name) strcpy(name, "FD_OOB");
            break;
        case FD_WRITE:
            if (bit) *bit = FD_WRITE_BIT;
            if (name) strcpy(name, "FD_WRITE");
            break;
        case FD_CLOSE:
            if (bit) *bit = FD_CLOSE_BIT;
            if (name) strcpy(name, "FD_CLOSE");
            break;
        default:
            if (bit) *bit = -1;
            if (name) sprintf(name, "bad%x", event);
    }
}

static const char *dbgstr_event_seq(const LPARAM *seq)
{
    static char message[1024];
    char name[12];
    int len = 1;

    message[0] = '[';
    message[1] = 0;
    while (*seq)
    {
        get_event_details(WSAGETSELECTEVENT(*seq), NULL, name);
        len += sprintf(message + len, "%s(%d) ", name, WSAGETSELECTERROR(*seq));
        seq++;
    }
    if (len > 1) len--;
    strcpy( message + len, "]" );
    return message;
}

static char *dbgstr_event_seq_result(SOCKET s, WSANETWORKEVENTS *netEvents)
{
    static char message[1024];
    struct async_message *curr = messages_received;
    int index, error, bit = 0;
    char name[12];
    int len = 1;

    message[0] = '[';
    message[1] = 0;
    while (1)
    {
        if (netEvents)
        {
            if (bit >= FD_MAX_EVENTS) break;
            if ( !(netEvents->lNetworkEvents & (1 << bit)) )
            {
                bit++;
                continue;
            }
            get_event_details(1 << bit, &index, name);
            error = netEvents->iErrorCode[index];
            bit++;
        }
        else
        {
            if (!curr) break;
            if (curr->socket != s)
            {
                curr = curr->next;
                continue;
            }
            get_event_details(WSAGETSELECTEVENT(curr->lparam), NULL, name);
            error = WSAGETSELECTERROR(curr->lparam);
            curr = curr->next;
        }

        len += sprintf(message + len, "%s(%d) ", name, error);
    }
    if (len > 1) len--;
    strcpy( message + len, "]" );
    return message;
}

static void flush_events(SOCKET s, HANDLE hEvent)
{
    WSANETWORKEVENTS netEvents;
    struct async_message *prev = NULL, *curr = messages_received;
    int ret;
    DWORD dwRet;

    if (hEvent != INVALID_HANDLE_VALUE)
    {
        dwRet = WaitForSingleObject(hEvent, 100);
        if (dwRet == WAIT_OBJECT_0)
        {
            ret = WSAEnumNetworkEvents(s, hEvent, &netEvents);
            if (ret)
                ok(0, "WSAEnumNetworkEvents failed, error %d\n", ret);
        }
    }
    else
    {
        while (curr)
        {
            if (curr->socket == s)
            {
                if (prev) prev->next = curr->next;
                else messages_received = curr->next;

                HeapFree(GetProcessHeap(), 0, curr);

                if (prev) curr = prev->next;
                else curr = messages_received;
            }
            else
            {
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

static int match_event_sequence(SOCKET s, WSANETWORKEVENTS *netEvents, const LPARAM *seq)
{
    int event, index, error, events;
    struct async_message *curr;

    if (netEvents)
    {
        events = netEvents->lNetworkEvents;
        while (*seq)
        {
            event = WSAGETSELECTEVENT(*seq);
            error = WSAGETSELECTERROR(*seq);
            get_event_details(event, &index, NULL);

            if (!(events & event) && index != -1)
                return 0;
            if (events & event && index != -1)
            {
                if (netEvents->iErrorCode[index] != error)
                    return 0;
            }
            events &= ~event;
            seq++;
        }
        if (events)
            return 0;
    }
    else
    {
        curr = messages_received;
        while (curr)
        {
            if (curr->socket == s)
            {
                if (!*seq) return 0;
                if (*seq != curr->lparam) return 0;
                seq++;
            }
            curr = curr->next;
        }
        if (*seq)
            return 0;
    }
    return 1;
}

/* checks for a sequence of events, (order only checked if window is used) */
static void ok_event_sequence(SOCKET s, HANDLE hEvent, const LPARAM *seq, const LPARAM **broken_seqs, int completelyBroken)
{
    MSG msg;
    WSANETWORKEVENTS events, *netEvents = NULL;
    int ret;
    DWORD dwRet;

    if (hEvent != INVALID_HANDLE_VALUE)
    {
        netEvents = &events;

        dwRet = WaitForSingleObject(hEvent, 200);
        if (dwRet == WAIT_OBJECT_0)
        {
            ret = WSAEnumNetworkEvents(s, hEvent, netEvents);
            if (ret)
            {
                winetest_ok(0, "WSAEnumNetworkEvents failed, error %d\n", ret);
                return;
            }
        }
        else
            memset(netEvents, 0, sizeof(*netEvents));
    }
    else
    {
        Sleep(200);
        /* Run the message loop a little */
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
        {
            DispatchMessageA(&msg);
        }
    }

    if (match_event_sequence(s, netEvents, seq))
    {
        winetest_ok(1, "Sequence matches expected: %s\n", dbgstr_event_seq(seq));
        flush_events(s, hEvent);
        return;
    }

    if (broken_seqs)
    {
        for (; *broken_seqs; broken_seqs++)
        {
            if (match_event_sequence(s, netEvents, *broken_seqs))
            {
                winetest_ok(broken(1), "Sequence matches broken: %s, expected %s\n", dbgstr_event_seq_result(s, netEvents), dbgstr_event_seq(seq));
                flush_events(s, hEvent);
                return;
            }
        }
    }

    winetest_ok(broken(completelyBroken), "Expected event sequence %s, got %s\n", dbgstr_event_seq(seq),
                dbgstr_event_seq_result(s, netEvents));
    flush_events(s, hEvent);
}

#define ok_event_seq (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : ok_event_sequence

static void test_events(int useMessages)
{
    SOCKET server = INVALID_SOCKET;
    SOCKET src = INVALID_SOCKET, src2 = INVALID_SOCKET;
    SOCKET dst = INVALID_SOCKET, dst2 = INVALID_SOCKET;
    struct sockaddr_in addr;
    HANDLE hThread = NULL;
    HANDLE hEvent = INVALID_HANDLE_VALUE, hEvent2 = INVALID_HANDLE_VALUE;
    WNDCLASSEXA wndclass;
    HWND hWnd = NULL;
    char *buffer = NULL;
    int bufferSize = 1024*1024;
    WSABUF bufs;
    OVERLAPPED ov, ov2;
    DWORD flags = 0;
    DWORD bytesReturned;
    DWORD id;
    int len;
    int ret;
    DWORD dwRet;
    BOOL bret;
    static char szClassName[] = "wstestclass";
    const LPARAM *broken_seq[3];
    static const LPARAM empty_seq[] = { 0 };
    static const LPARAM close_seq[] = { WSAMAKESELECTREPLY(FD_CLOSE, 0), 0 };
    static const LPARAM write_seq[] = { WSAMAKESELECTREPLY(FD_WRITE, 0), 0 };
    static const LPARAM read_seq[] = { WSAMAKESELECTREPLY(FD_READ, 0), 0 };
    static const LPARAM oob_seq[] = { WSAMAKESELECTREPLY(FD_OOB, 0), 0 };
    static const LPARAM connect_seq[] = { WSAMAKESELECTREPLY(FD_CONNECT, 0),
                                          WSAMAKESELECTREPLY(FD_WRITE, 0), 0 };
    static const LPARAM read_read_seq[] = { WSAMAKESELECTREPLY(FD_READ, 0),
                                            WSAMAKESELECTREPLY(FD_READ, 0), 0 };
    static const LPARAM read_write_seq[] = { WSAMAKESELECTREPLY(FD_READ, 0),
                                             WSAMAKESELECTREPLY(FD_WRITE, 0), 0 };
    static const LPARAM read_close_seq[] = { WSAMAKESELECTREPLY(FD_READ, 0),
                                             WSAMAKESELECTREPLY(FD_CLOSE, 0), 0 };

    memset(&ov, 0, sizeof(ov));
    memset(&ov2, 0, sizeof(ov2));

    /* don't use socketpair, we want connection event */
    src = socket(AF_INET, SOCK_STREAM, 0);
    if (src == INVALID_SOCKET)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    ret = set_blocking(src, TRUE);
    ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());

    src2 = socket(AF_INET, SOCK_STREAM, 0);
    if (src2 == INVALID_SOCKET)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    ret = set_blocking(src2, TRUE);
    ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());

    len = sizeof(BOOL);
    if (getsockopt(src, SOL_SOCKET, SO_OOBINLINE, (void *)&bret, &len) == SOCKET_ERROR)
    {
        ok(0, "failed to get oobinline status, %d\n", GetLastError());
        goto end;
    }
    ok(bret == FALSE, "OOB not inline\n");

    if (useMessages)
    {
        trace("Event test using messages\n");

        wndclass.cbSize         = sizeof(wndclass);
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = ws2_test_WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = GetModuleHandleA(NULL);
        wndclass.hIcon          = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
        wndclass.hIconSm        = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
        wndclass.hCursor        = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
        wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass.lpszClassName  = szClassName;
        wndclass.lpszMenuName   = NULL;
        RegisterClassExA(&wndclass);

        hWnd = CreateWindowA(szClassName, "WS2Test", WS_OVERLAPPEDWINDOW,
                            0, 0, 500, 500, NULL, NULL, GetModuleHandleA(NULL), NULL);
        if (!hWnd)
        {
            ok(0, "failed to create window: %d\n", GetLastError());
            return;
        }

        ret = WSAAsyncSelect(src, hWnd, WM_SOCKET, FD_CONNECT | FD_READ | FD_OOB | FD_WRITE | FD_CLOSE);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ok(set_blocking(src, TRUE) == SOCKET_ERROR, "set_blocking should failed!\n");
        ok(WSAGetLastError() == WSAEINVAL, "expect WSAEINVAL, returned %x\n", WSAGetLastError());

        ret = WSAAsyncSelect(src2, hWnd, WM_SOCKET, FD_CONNECT | FD_READ | FD_OOB | FD_WRITE | FD_CLOSE);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ok(set_blocking(src2, TRUE) == SOCKET_ERROR, "set_blocking should failed!\n");
        ok(WSAGetLastError() == WSAEINVAL, "expect WSAEINVAL, returned %x\n", WSAGetLastError());
    }
    else
    {
        trace("Event test using events\n");

        hEvent = WSACreateEvent();
        if (hEvent == INVALID_HANDLE_VALUE)
        {
            ok(0, "WSACreateEvent failed, error %d\n", GetLastError());
            goto end;
        }

        hEvent2 = WSACreateEvent();
        if (hEvent2 == INVALID_HANDLE_VALUE)
        {
            ok(0, "WSACreateEvent failed, error %d\n", GetLastError());
            goto end;
        }

        ret = WSAEventSelect(src, hEvent, FD_CONNECT | FD_READ | FD_OOB | FD_WRITE | FD_CLOSE);
        if (ret)
        {
            ok(0, "WSAEventSelect failed, error %d\n", ret);
            goto end;
        }

        ok(set_blocking(src, TRUE) == SOCKET_ERROR, "set_blocking should failed!\n");
        ok(WSAGetLastError() == WSAEINVAL, "expect WSAEINVAL, returned %x\n", WSAGetLastError());

        ret = WSAEventSelect(src2, hEvent2, FD_CONNECT | FD_READ | FD_OOB | FD_WRITE | FD_CLOSE);
        if (ret)
        {
            ok(0, "WSAEventSelect failed, error %d\n", ret);
            goto end;
        }

        ok(set_blocking(src2, TRUE) == SOCKET_ERROR, "set_blocking should failed!\n");
        ok(WSAGetLastError() == WSAEINVAL, "expect WSAEINVAL, returned %x\n", WSAGetLastError());
    }

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(server, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr*)&addr, &len);
    if (ret != 0)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    ret = listen(server, 2);
    if (ret != 0)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    SetLastError(0xdeadbeef);
    ret = connect(src, NULL, 0);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(GetLastError() == WSAEFAULT, "expected 10014, got %d\n", GetLastError());

    ret = connect(src, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    ret = connect(src2, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    len = sizeof(addr);
    dst = accept(server, (struct sockaddr*)&addr, &len);
    if (dst == INVALID_SOCKET)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    len = sizeof(addr);
    dst2 = accept(server, (struct sockaddr*)&addr, &len);
    if (dst2 == INVALID_SOCKET)
    {
        ok(0, "creating socket pair failed (%d), skipping test\n", GetLastError());
        goto end;
    }

    closesocket(server);
    server = INVALID_SOCKET;

    /* On Windows it seems when a non-blocking socket sends to a
       blocking socket on the same host, the send() is BLOCKING,
       so make both sockets non-blocking. src is already non-blocking
       from the async select */

    if (set_blocking(dst, FALSE))
    {
        ok(0, "ioctlsocket failed, error %d\n", WSAGetLastError());
        goto end;
    }

    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
    if (buffer == NULL)
    {
        ok(0, "could not allocate memory for test\n");
        goto end;
    }

    ov.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (ov.hEvent == NULL)
    {
        ok(0, "could not create event object, errno = %d\n", GetLastError());
        goto end;
    }

    ov2.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (ov2.hEvent == NULL)
    {
        ok(0, "could not create event object, errno = %d\n", GetLastError());
        goto end;
    }

    /* FD_WRITE should be set initially, and allow us to send at least 1 byte */
    ok_event_seq(src, hEvent, connect_seq, NULL, 1);
    ok_event_seq(src2, hEvent2, connect_seq, NULL, 1);
    /* broken on all windows - FD_CONNECT error is garbage */

    /* Test simple send/recv */
    SetLastError(0xdeadbeef);
    ret = send(dst, buffer, 100, 0);
    ok(ret == 100, "Failed to send buffer %d err %d\n", ret, GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    SetLastError(0xdeadbeef);
    ret = recv(src, buffer, 1, MSG_PEEK);
    ok(ret == 1, "Failed to peek at recv buffer %d err %d\n", ret, GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    SetLastError(0xdeadbeef);
    ret = recv(src, buffer, 50, 0);
    ok(ret == 50, "Failed to recv buffer %d err %d\n", ret, GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    ret = recv(src, buffer, 50, 0);
    ok(ret == 50, "Failed to recv buffer %d err %d\n", ret, GetLastError());
    ok_event_seq(src, hEvent, empty_seq, NULL, 0);

    /* fun fact - events are re-enabled even on failure, but only for messages */
    ret = send(dst, "1", 1, 0);
    ok(ret == 1, "Failed to send buffer %d err %d\n", ret, GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    ret = recv(src, buffer, -1, 0);
    ok(ret == SOCKET_ERROR && (GetLastError() == WSAEFAULT || GetLastError() == WSAENOBUFS),
       "Failed to recv buffer %d err %d\n", ret, GetLastError());
    if (useMessages)
    {
        broken_seq[0] = empty_seq; /* win9x */
        broken_seq[1] = NULL;
        todo_wine ok_event_seq(src, hEvent, read_seq, broken_seq, 0);
    }
    else
        ok_event_seq(src, hEvent, empty_seq, NULL, 0);

    SetLastError(0xdeadbeef);
    ret = recv(src, buffer, 1, 0);
    ok(ret == 1, "Failed to recv buffer %d err %d\n", ret, GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    ok_event_seq(src, hEvent, empty_seq, NULL, 0);

    /* Interaction with overlapped */
    bufs.len = sizeof(char);
    bufs.buf = buffer;
    ret = WSARecv(src, &bufs, 1, &bytesReturned, &flags, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %d\n", ret, GetLastError());

    bufs.len = sizeof(char);
    bufs.buf = buffer+1;
    ret = WSARecv(src, &bufs, 1, &bytesReturned, &flags, &ov2, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %d\n", ret, GetLastError());

    ret = send(dst, "12", 2, 0);
    ok(ret == 2, "Failed to send buffer %d err %d\n", ret, GetLastError());
    broken_seq[0] = read_read_seq; /* win9x */
    broken_seq[1] = NULL;
    ok_event_seq(src, hEvent, empty_seq, broken_seq, 0);

    dwRet = WaitForSingleObject(ov.hEvent, 100);
    ok(dwRet == WAIT_OBJECT_0, "Failed to wait for recv message: %d - %d\n", dwRet, GetLastError());
    if (dwRet == WAIT_OBJECT_0)
    {
        bret = GetOverlappedResult((HANDLE)src, &ov, &bytesReturned, FALSE);
        ok((bret && bytesReturned == 1) || broken(!bret && GetLastError() == ERROR_IO_INCOMPLETE) /* win9x */,
           "Got %d instead of 1 (%d - %d)\n", bytesReturned, bret, GetLastError());
        ok(buffer[0] == '1', "Got %c instead of 1\n", buffer[0]);
    }

    dwRet = WaitForSingleObject(ov2.hEvent, 100);
    ok(dwRet == WAIT_OBJECT_0, "Failed to wait for recv message: %d - %d\n", dwRet, GetLastError());
    if (dwRet == WAIT_OBJECT_0)
    {
        bret = GetOverlappedResult((HANDLE)src, &ov2, &bytesReturned, FALSE);
        ok((bret && bytesReturned == 1) || broken(!bret && GetLastError() == ERROR_IO_INCOMPLETE) /* win9x */,
           "Got %d instead of 1 (%d - %d)\n", bytesReturned, bret, GetLastError());
        ok(buffer[1] == '2', "Got %c instead of 2\n", buffer[1]);
    }

    SetLastError(0xdeadbeef);
    ret = send(dst, "1", 1, 0);
    ok(ret == 1, "Failed to send buffer %d err %d\n", ret, GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    ret = recv(src, buffer, 1, 0);
    ok(ret == 1, "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    ok_event_seq(src, hEvent, empty_seq, NULL, 0);

    /* Notifications are delivered as soon as possible, blocked only on
     * async requests on the same type */
    bufs.len = sizeof(char);
    bufs.buf = buffer;
    ret = WSARecv(src, &bufs, 1, &bytesReturned, &flags, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %d\n", ret, GetLastError());

    if (0) {
    ret = send(dst, "1", 1, MSG_OOB);
    ok(ret == 1, "Failed to send buffer %d err %d\n", ret, GetLastError());
    ok_event_seq(src, hEvent, oob_seq, NULL, 0);
    }

    dwRet = WaitForSingleObject(ov.hEvent, 100);
    ok(dwRet == WAIT_TIMEOUT, "OOB message activated read?: %d - %d\n", dwRet, GetLastError());

    ret = send(dst, "2", 1, 0);
    ok(ret == 1, "Failed to send buffer %d err %d\n", ret, GetLastError());
    broken_seq[0] = read_seq;  /* win98 */
    broken_seq[1] = NULL;
    ok_event_seq(src, hEvent, empty_seq, broken_seq, 0);

    dwRet = WaitForSingleObject(ov.hEvent, 100);
    ok(dwRet == WAIT_OBJECT_0 || broken(dwRet == WAIT_TIMEOUT),
       "Failed to wait for recv message: %d - %d\n", dwRet, GetLastError());
    if (dwRet == WAIT_OBJECT_0)
    {
        bret = GetOverlappedResult((HANDLE)src, &ov, &bytesReturned, FALSE);
        ok((bret && bytesReturned == 1) || broken(!bret && GetLastError() == ERROR_IO_INCOMPLETE) /* win9x */,
           "Got %d instead of 1 (%d - %d)\n", bytesReturned, bret, GetLastError());
        ok(buffer[0] == '2', "Got %c instead of 2\n", buffer[0]);
    }

    if (0) {
    ret = recv(src, buffer, 1, MSG_OOB);
    todo_wine ok(ret == 1, "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    /* We get OOB notification, but no data on wine */
    ok_event_seq(src, hEvent, empty_seq, NULL, 0);
    }

    /* Flood the send queue */
    hThread = CreateThread(NULL, 0, drain_socket_thread, &dst, 0, &id);
    if (hThread == NULL)
    {
        ok(0, "CreateThread failed, error %d\n", GetLastError());
        goto end;
    }

    /* Now FD_WRITE should not be set, because the socket send buffer isn't full yet */
    ok_event_seq(src, hEvent, empty_seq, NULL, 0);

    /* Now if we send a ton of data and the 'server' does not drain it fast
     * enough (set drain_pause to be sure), the socket send buffer will only
     * take some of it, and we will get a short write. This will trigger
     * another FD_WRITE event as soon as data is sent and more space becomes
     * available, but not any earlier. */
    drain_pause = TRUE;
    do
    {
        ret = send(src, buffer, bufferSize, 0);
    } while (ret == bufferSize);
    drain_pause = FALSE;
    if (ret >= 0 || WSAGetLastError() == WSAEWOULDBLOCK)
    {
        Sleep(400); /* win9x */
        broken_seq[0] = read_write_seq;
        broken_seq[1] = NULL;
        ok_event_seq(src, hEvent, write_seq, broken_seq, 0);
    }
    else
    {
        ok(0, "sending a lot of data failed with error %d\n", WSAGetLastError());
    }

    /* Test how FD_CLOSE is handled */
    ret = send(dst, "12", 2, 0);
    ok(ret == 2, "Failed to send buffer %d err %d\n", ret, GetLastError());

    /* Wait a little and let the send complete */
    Sleep(100);
    closesocket(dst);
    dst = INVALID_SOCKET;
    Sleep(100);

    /* We can never implement this in wine, best we can hope for is
       sending FD_CLOSE after the reads complete */
    broken_seq[0] = read_seq;  /* win9x */
    broken_seq[1] = NULL;
    todo_wine ok_event_seq(src, hEvent, read_close_seq, broken_seq, 0);

    ret = recv(src, buffer, 1, 0);
    ok(ret == 1, "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    ok_event_seq(src, hEvent, read_seq, NULL, 0);

    ret = recv(src, buffer, 1, 0);
    ok(ret == 1, "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    /* want it? it's here, but you can't have it */
    broken_seq[0] = close_seq;  /* win9x */
    broken_seq[1] = NULL;
    todo_wine ok_event_seq(src, hEvent, empty_seq, /* wine sends FD_CLOSE here */
                           broken_seq, 0);

    /* Test how FD_CLOSE is handled */
    ret = send(dst2, "12", 2, 0);
    ok(ret == 2, "Failed to send buffer %d err %d\n", ret, GetLastError());

    Sleep(200);
    shutdown(dst2, SD_SEND);
    Sleep(200);

    /* Some of the below are technically todo_wine, but our event sequence is still valid, so to prevent
       regressions, don't mark them as todo_wine, and mark windows as broken */
    broken_seq[0] = read_close_seq;
    broken_seq[1] = close_seq;
    broken_seq[2] = NULL;
    ok_event_seq(src2, hEvent2, read_seq, broken_seq, 0);

    ret = recv(src2, buffer, 1, 0);
    ok(ret == 1 || broken(!ret), "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    broken_seq[0] = close_seq;  /* win98 */
    broken_seq[1] = NULL;
    ok_event_seq(src2, hEvent2, read_seq, broken_seq, 0);

    ret = recv(src2, buffer, 1, 0);
    ok(ret == 1 || broken(!ret), "Failed to empty buffer: %d - %d\n", ret, GetLastError());
    broken_seq[0] = empty_seq;
    broken_seq[1] = NULL;
    ok_event_seq(src2, hEvent2, close_seq, broken_seq, 0);

    ret = send(src2, "1", 1, 0);
    ok(ret == 1, "Sending to half-closed socket failed %d err %d\n", ret, GetLastError());
    ok_event_seq(src2, hEvent2, empty_seq, NULL, 0);

    ret = send(src2, "1", 1, 0);
    ok(ret == 1, "Sending to half-closed socket failed %d err %d\n", ret, GetLastError());
    ok_event_seq(src2, hEvent2, empty_seq, NULL, 0);

    if (useMessages)
    {
        ret = WSAAsyncSelect(src, hWnd, WM_SOCKET, 0);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ret = set_blocking(src, TRUE);
        ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());

        ret = WSAAsyncSelect(src2, hWnd, WM_SOCKET, 0);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ret = set_blocking(src2, TRUE);
        ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());
    }
    else
    {
        ret = WSAEventSelect(src, hEvent2, 0);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ret = set_blocking(src, TRUE);
        ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());

        ret = WSAEventSelect(src2, hEvent2, 0);
        if (ret)
        {
            ok(0, "WSAAsyncSelect failed, error %d\n", ret);
            goto end;
        }

        ret = set_blocking(src2, TRUE);
        ok(!ret, "set_blocking failed, error %d\n", WSAGetLastError());
    }

end:
    if (src != INVALID_SOCKET)
    {
        flush_events(src, hEvent);
        closesocket(src);
    }
    if (src2 != INVALID_SOCKET)
    {
        flush_events(src2, hEvent2);
        closesocket(src2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    if (server != INVALID_SOCKET)
        closesocket(server);
    if (dst != INVALID_SOCKET)
        closesocket(dst);
    if (dst2 != INVALID_SOCKET)
        closesocket(dst2);
    if (hThread != NULL)
        CloseHandle(hThread);
    if (hWnd != NULL)
        DestroyWindow(hWnd);
    if (hEvent != NULL)
        CloseHandle(hEvent);
    if (hEvent2 != NULL)
        CloseHandle(hEvent2);
    if (ov.hEvent != NULL)
        CloseHandle(ov.hEvent);
    if (ov2.hEvent != NULL)
        CloseHandle(ov2.hEvent);
}

static void test_ipv6only(void)
{
    SOCKET v4 = INVALID_SOCKET, v6;
    struct sockaddr_in sin4;
    struct sockaddr_in6 sin6;
    int ret, enabled, len = sizeof(enabled);

    memset(&sin4, 0, sizeof(sin4));
    sin4.sin_family = AF_INET;
    sin4.sin_port = htons(SERVERPORT);

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(SERVERPORT);

    v6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (v6 == INVALID_SOCKET)
    {
        skip("Could not create IPv6 socket (LastError: %d)\n", WSAGetLastError());
        goto end;
    }

    enabled = 2;
    ret = getsockopt(v6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);

    ret = bind(v6, (struct sockaddr*)&sin6, sizeof(sin6));
    if (ret)
    {
        skip("Could not bind IPv6 address (LastError: %d)\n", WSAGetLastError());
        goto end;
    }

    v4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(v4 != INVALID_SOCKET, "Could not create IPv4 socket (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 0;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(!ret, "setsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(!enabled, "expected 0, got %d\n", enabled);
}

    enabled = 1;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(!ret, "setsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());

    /* bind on IPv4 socket should succeed - IPV6_V6ONLY is enabled by default */
    ret = bind(v4, (struct sockaddr*)&sin4, sizeof(sin4));
    ok(!ret, "Could not bind IPv4 address (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 0;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(ret, "setsockopt(IPV6_ONLY) succeeded (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 0;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 1;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(ret, "setsockopt(IPV6_ONLY) succeeded (LastError: %d)\n", WSAGetLastError());

    closesocket(v4);
    closesocket(v6);

    /* Test again, this time disabling IPV6_V6ONLY. */
    sin4.sin_port = htons(SERVERPORT+2);
    sin6.sin6_port = htons(SERVERPORT+2);

    v6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    ok(v6 != INVALID_SOCKET, "Could not create IPv6 socket (LastError: %d; %d expected if IPv6 not available).\n",
        WSAGetLastError(), WSAEAFNOSUPPORT);

    enabled = 0;
    ret = setsockopt(v6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(!ret, "Could not disable IPV6_V6ONLY (LastError: %d).\n", WSAGetLastError());

    enabled = 2;
    ret = getsockopt(v6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(!enabled, "expected 0, got %d\n", enabled);

    /*
        Observaition:
        On Windows, bind on both IPv4 and IPv6 with IPV6_V6ONLY disabled succeeds by default.
        Application must set SO_EXCLUSIVEADDRUSE on first socket to disallow another successful bind.
        In general, a standard application should not use SO_REUSEADDR.
        Setting both SO_EXCLUSIVEADDRUSE and SO_REUSEADDR on the same socket is not possible in
        either order, the later setsockopt call always fails.
    */
    enabled = 1;
    ret = setsockopt(v6, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&enabled, len);
    ok(!ret, "Could not set SO_EXCLUSIVEADDRUSE on IPv6 socket (LastError: %d)\n", WSAGetLastError());

    ret = bind(v6, (struct sockaddr*)&sin6, sizeof(sin6));
    ok(!ret, "Could not bind IPv6 address (LastError: %d)\n", WSAGetLastError());

    enabled = 2;
    len = sizeof(enabled);
    getsockopt(v6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(!enabled, "IPV6_V6ONLY is enabled after bind\n");

    v4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(v4 != INVALID_SOCKET, "Could not create IPv4 socket (LastError: %d)\n", WSAGetLastError());

    enabled = 1;
    ret = setsockopt(v4, SOL_SOCKET, SO_REUSEADDR, (char*)&enabled, len);
    ok(!ret, "Could not set SO_REUSEADDR on IPv4 socket (LastError: %d)\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(v4, (struct sockaddr*)&sin4, sizeof(sin4));
    ok(ret, "bind succeeded unexpectedly for the IPv4 socket\n");
    ok(WSAGetLastError() == WSAEACCES, "Expected 10013, got %d\n", WSAGetLastError());

end:
    if (v4 != INVALID_SOCKET)
        closesocket(v4);
    if (v6 != INVALID_SOCKET)
        closesocket(v6);
}

static void test_WSASendMsg(void)
{
    SOCKET sock, dst;
    struct sockaddr_in sendaddr, sockaddr;
    GUID WSASendMsg_GUID = WSAID_WSASENDMSG;
    LPFN_WSASENDMSG pWSASendMsg = NULL;
    char teststr[12] = "hello world", buffer[32];
    WSABUF iovec[2];
    WSAMSG msg;
    DWORD bytesSent, err;
    int ret, addrlen;

    /* FIXME: Missing OVERLAPPED and OVERLAPPED COMPLETION ROUTINE tests */

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    /* Obtain the WSASendMsg function */
    WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSASendMsg_GUID, sizeof(WSASendMsg_GUID),
             &pWSASendMsg, sizeof(pWSASendMsg), &err, NULL, NULL);
    if (!pWSASendMsg)
    {
        closesocket(sock);
        win_skip("WSASendMsg is unsupported, some tests will be skipped.\n");
        return;
    }

    /* fake address for now */
    sendaddr.sin_family = AF_INET;
    sendaddr.sin_port = htons(139);
    sendaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(&msg, 0, sizeof(msg));
    iovec[0].buf      = teststr;
    iovec[0].len      = sizeof(teststr);
    iovec[1].buf      = teststr;
    iovec[1].len      = sizeof(teststr) / 2;
    msg.name          = (struct sockaddr *) &sendaddr;
    msg.namelen       = sizeof(sendaddr);
    msg.lpBuffers     = iovec;
    msg.dwBufferCount = 1; /* send only one buffer for now */

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(INVALID_SOCKET, &msg, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, got %d instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, NULL, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, NULL, 0, &bytesSent, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %d instead\n", err);

    closesocket(sock);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    dst = socket(AF_INET, SOCK_DGRAM, 0);
    ok(dst != INVALID_SOCKET, "socket() failed\n");

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ok(!bind(dst, (struct sockaddr*)&sockaddr, sizeof(sockaddr)),
       "bind should have worked\n");

    /* read address to find out the port number to be used in send */
    memset(&sendaddr, 0, sizeof(sendaddr));
    addrlen = sizeof(sendaddr);
    ok(!getsockname(dst, (struct sockaddr *) &sendaddr, &addrlen),
       "getsockname should have worked\n");
    ok(sendaddr.sin_port, "socket port should be != 0\n");

    /* ensure the sending socket is not bound */
    WSASetLastError(0xdeadbeef);
    addrlen = sizeof(sockaddr);
    ret = getsockname(sock, (struct sockaddr*)&sockaddr, &addrlen);
    ok(ret == SOCKET_ERROR, "getsockname should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, got %d instead\n", err);

    set_blocking(sock, TRUE);

    bytesSent = 0;
    SetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, &bytesSent, NULL, NULL);
    ok(!ret, "WSASendMsg should have worked\n");
    ok(GetLastError() == 0 || broken(GetLastError() == 0xdeadbeef) /* Win <= 2008 */,
       "Expected 0, got %d\n", GetLastError());
    ok(bytesSent == iovec[0].len, "incorret bytes sent, expected %d, sent %d\n",
       iovec[0].len, bytesSent);

    /* receive data */
    addrlen = sizeof(sockaddr);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(0xdeadbeef);
    ret = recvfrom(dst, buffer, sizeof(buffer), 0, (struct sockaddr *) &sockaddr, &addrlen);
    ok(ret == bytesSent, "got %d, expected %d\n",
       ret, bytesSent);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());

    /* A successful call to WSASendMsg must have bound the socket */
    addrlen = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = getsockname(sock, (struct sockaddr*)&sockaddr, &addrlen);
    ok(!ret, "getsockname should have worked\n");
    ok(sockaddr.sin_addr.s_addr == htonl(INADDR_ANY), "expected 0.0.0.0, got %s\n",
       inet_ntoa(sockaddr.sin_addr));
    ok(sockaddr.sin_port, "sin_port should be != 0\n");

    msg.dwBufferCount = 2; /* send both buffers */

    bytesSent = 0;
    SetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, &bytesSent, NULL, NULL);
    ok(!ret, "WSASendMsg should have worked\n");
    ok(bytesSent == iovec[0].len + iovec[1].len, "incorret bytes sent, expected %d, sent %d\n",
       iovec[0].len + iovec[1].len, bytesSent);
    ok(GetLastError() == 0 || broken(GetLastError() == 0xdeadbeef) /* Win <= 2008 */,
       "Expected 0, got %d\n", GetLastError());

    /* receive data */
    addrlen = sizeof(sockaddr);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(0xdeadbeef);
    ret = recvfrom(dst, buffer, sizeof(buffer), 0, (struct sockaddr *) &sockaddr, &addrlen);
    ok(ret == bytesSent, "got %d, expected %d\n",
       ret, bytesSent);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());

    closesocket(sock);
    closesocket(dst);

    /* a bad call to WSASendMsg will also bind the socket */
    addrlen = sizeof(sockaddr);
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ok(sock != INVALID_SOCKET, "socket() failed\n");
    ok(pWSASendMsg(sock, &msg, 0, NULL, NULL, NULL) == SOCKET_ERROR, "WSASendMsg should have failed\n");
todo_wine {
    ok(!getsockname(sock, (struct sockaddr*)&sockaddr, &addrlen), "getsockname should have worked\n");
    ok(sockaddr.sin_addr.s_addr == htonl(INADDR_ANY), "expected 0.0.0.0, got %s\n",
       inet_ntoa(sockaddr.sin_addr));
    ok(sockaddr.sin_port, "sin_port should be > 0\n");
}
    closesocket(sock);

    /* a bad call without msg parameter will not trigger the auto-bind */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ok(sock != INVALID_SOCKET, "socket() failed\n");
    ok(pWSASendMsg(sock, NULL, 0, NULL, NULL, NULL) == SOCKET_ERROR, "WSASendMsg should have failed\n");
    ok(getsockname(sock, (struct sockaddr*)&sockaddr, &addrlen), "getsockname should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, got %d instead\n", err);
    closesocket(sock);

    /* SOCK_STREAM sockets are not supported */
    bytesSent = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    ok(sock != INVALID_SOCKET, "socket() failed\n");
    SetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, &bytesSent, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
todo_wine
    ok(err == WSAEINVAL, "expected 10014, got %d instead\n", err);
    closesocket(sock);
}

static void test_WSASendTo(void)
{
    SOCKET s;
    struct sockaddr_in addr;
    char buf[12] = "hello world";
    WSABUF data_buf;
    DWORD bytesSent;
    int ret;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(139);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    data_buf.len = sizeof(buf);
    data_buf.buf = buf;

    if( (s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        ok(0, "socket() failed error: %d\n", WSAGetLastError());
        return;
    }

    WSASetLastError(12345);
    ret = WSASendTo(INVALID_SOCKET, &data_buf, 1, NULL, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK,
       "WSASendTo() failed: %d/%d\n", ret, WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASendTo(s, &data_buf, 1, NULL, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "WSASendTo() failed: %d/%d\n", ret, WSAGetLastError());

    WSASetLastError(12345);
    if(WSASendTo(s, &data_buf, 1, &bytesSent, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL)) {
        ok(0, "WSASendTo() failed error: %d\n", WSAGetLastError());
        return;
    }
    ok(!WSAGetLastError(), "WSAGetLastError() should return zero after "
            "a successful call to WSASendTo()\n");
}

static DWORD WINAPI recv_thread(LPVOID arg)
{
    SOCKET sock = *(SOCKET *)arg;
    char buffer[32];
    WSABUF wsa;
    WSAOVERLAPPED ov;
    DWORD flags = 0;

    wsa.buf = buffer;
    wsa.len = sizeof(buffer);
    ov.hEvent = WSACreateEvent();
    WSARecv(sock, &wsa, 1, NULL, &flags, &ov, NULL);

    WaitForSingleObject(ov.hEvent, 1000);
    WSACloseEvent(ov.hEvent);
    return 0;
}

static int completion_called;

static void WINAPI io_completion(DWORD error, DWORD transferred, WSAOVERLAPPED *overlapped, DWORD flags)
{
    completion_called++;
}

static void test_WSARecv(void)
{
    SOCKET src, dest, server = INVALID_SOCKET;
    char buf[20];
    WSABUF bufs[2];
    WSAOVERLAPPED ov;
    DWORD bytesReturned, flags, id;
    struct linger ling;
    struct sockaddr_in addr;
    int iret, len;
    DWORD dwret;
    BOOL bret;
    HANDLE thread, event = NULL, io_port;

    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        goto end;
    }

    memset(&ov, 0, sizeof(ov));
    flags = 0;
    bufs[0].len = 2;
    bufs[0].buf = buf;

    /* Send 4 bytes and receive in two calls of 2 */
    SetLastError(0xdeadbeef);
    iret = send(src, "test", 4, 0);
    ok(iret == 4, "Expected 4, got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 2, "Expected 2, got %d\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 2, "Expected 2, got %d\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());

    bufs[0].len = 4;
    SetLastError(0xdeadbeef);
    iret = send(src, "test", 4, 0);
    ok(iret == 4, "Expected 4, got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 4, "Expected 4, got %d\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());

    /* Test 2 buffers */
    bufs[0].len = 4;
    bufs[1].len = 5;
    bufs[1].buf = buf + 10;
    SetLastError(0xdeadbeef);
    iret = send(src, "deadbeefs", 9, 0);
    ok(iret == 9, "Expected 9, got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 2, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 9, "Expected 9, got %d\n", bytesReturned);
    bufs[0].buf[4] = '\0';
    bufs[1].buf[5] = '\0';
    ok(!strcmp(bufs[0].buf, "dead"), "buf[0] doesn't match: %s != dead\n", bufs[0].buf);
    ok(!strcmp(bufs[1].buf, "beefs"), "buf[1] doesn't match: %s != beefs\n", bufs[1].buf);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %d\n", GetLastError());

    bufs[0].len = sizeof(buf);
    ov.hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %d\n", GetLastError());
    if (!event)
        goto end;

    ling.l_onoff = 1;
    ling.l_linger = 0;
    iret = setsockopt (src, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
    ok(!iret, "Failed to set linger %d\n", GetLastError());

    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %d\n", iret, GetLastError());

    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %d\n", iret, GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    dwret = WaitForSingleObject(ov.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for disconnect event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)dest, &ov, &bytesReturned, FALSE);
    todo_wine ok(!bret && (GetLastError() == ERROR_NETNAME_DELETED || broken(GetLastError() == ERROR_IO_INCOMPLETE) /* win9x */),
        "Did not get disconnect event: %d, error %d\n", bret, GetLastError());
    ok(bytesReturned == 0, "Bytes received is %d\n", bytesReturned);
    closesocket(dest);
    dest = INVALID_SOCKET;

    src = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    ok(src != INVALID_SOCKET, "failed to create socket %d\n", WSAGetLastError());
    if (src == INVALID_SOCKET) goto end;

    server = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(server != INVALID_SOCKET, "failed to create socket %d\n", WSAGetLastError());
    if (server == INVALID_SOCKET) goto end;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (iret) goto end;

    len = sizeof(addr);
    iret = getsockname(server, (struct sockaddr *)&addr, &len);
    if (iret) goto end;

    iret = listen(server, 1);
    if (iret) goto end;

    iret = connect(src, (struct sockaddr *)&addr, sizeof(addr));
    if (iret) goto end;

    len = sizeof(addr);
    dest = accept(server, (struct sockaddr *)&addr, &len);
    ok(dest != INVALID_SOCKET, "failed to create socket %d\n", WSAGetLastError());
    if (dest == INVALID_SOCKET) goto end;

    send(src, "test message", sizeof("test message"), 0);
    thread = CreateThread(NULL, 0, recv_thread, &dest, 0, &id);
    WaitForSingleObject(thread, 3000);
    CloseHandle(thread);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = event;
    ResetEvent(event);
    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, io_completion);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %d\n", iret, GetLastError());
    send(src, "test message", sizeof("test message"), 0);

    completion_called = 0;
    dwret = SleepEx(1000, TRUE);
    ok(dwret == WAIT_IO_COMPLETION, "got %u\n", dwret);
    ok(completion_called == 1, "completion not called\n");

    dwret = WaitForSingleObject(event, 1);
    ok(dwret == WAIT_TIMEOUT, "got %u\n", dwret);

    io_port = CreateIoCompletionPort( (HANDLE)dest, NULL, 0, 0 );
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    /* Using completion function on socket associated with completion port is not allowed. */
    memset(&ov, 0, sizeof(ov));
    completion_called = 0;
    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, io_completion);
    ok(iret == SOCKET_ERROR && GetLastError() == WSAEINVAL, "WSARecv failed - %d error %d\n", iret, GetLastError());
    ok(!completion_called, "completion called\n");

    CloseHandle(io_port);

end:
    if (server != INVALID_SOCKET)
        closesocket(server);
    if (dest != INVALID_SOCKET)
        closesocket(dest);
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (event)
        WSACloseEvent(event);
}

struct write_watch_thread_args
{
    int func;
    SOCKET dest;
    void *base;
    DWORD size;
    const char *expect;
};

static DWORD CALLBACK write_watch_thread( void *arg )
{
    struct write_watch_thread_args *args = arg;
    struct sockaddr addr;
    int addr_len = sizeof(addr), ret;
    DWORD bytes, flags = 0;
    WSABUF buf[1];

    switch (args->func)
    {
    case 0:
        ret = recv( args->dest, args->base, args->size, 0 );
        ok( ret == strlen(args->expect) + 1, "wrong len %d\n", ret );
        ok( !strcmp( args->base, args->expect ), "wrong data\n" );
        break;
    case 1:
        ret = recvfrom( args->dest, args->base, args->size, 0, &addr, &addr_len );
        ok( ret == strlen(args->expect) + 1, "wrong len %d\n", ret );
        ok( !strcmp( args->base, args->expect ), "wrong data\n" );
        break;
    case 2:
        buf[0].len = args->size;
        buf[0].buf = args->base;
        ret = WSARecv( args->dest, buf, 1, &bytes, &flags, NULL, NULL );
        ok( !ret, "WSARecv failed %u\n", GetLastError() );
        ok( bytes == strlen(args->expect) + 1, "wrong len %d\n", bytes );
        ok( !strcmp( args->base, args->expect ), "wrong data\n" );
        break;
    case 3:
        buf[0].len = args->size;
        buf[0].buf = args->base;
        ret = WSARecvFrom( args->dest, buf, 1, &bytes, &flags, &addr, &addr_len, NULL, NULL );
        ok( !ret, "WSARecvFrom failed %u\n", GetLastError() );
        ok( bytes == strlen(args->expect) + 1, "wrong len %d\n", bytes );
        ok( !strcmp( args->base, args->expect ), "wrong data\n" );
        break;
    }
    return 0;
}

static void test_write_watch(void)
{
    SOCKET src, dest;
    WSABUF bufs[2];
    WSAOVERLAPPED ov;
    struct write_watch_thread_args args;
    DWORD bytesReturned, flags, size;
    struct sockaddr addr;
    int addr_len, ret;
    HANDLE thread, event;
    char *base;
    void *results[64];
    ULONG_PTR count;
    ULONG pagesize;
    UINT (WINAPI *pGetWriteWatch)(DWORD,LPVOID,SIZE_T,LPVOID*,ULONG_PTR*,ULONG*);

    pGetWriteWatch = (void *)GetProcAddress( GetModuleHandleA("kernel32.dll"), "GetWriteWatch" );
    if (!pGetWriteWatch)
    {
        win_skip( "write watched not supported\n" );
        return;
    }

    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        return;
    }

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %d\n", GetLastError());

    flags = 0;

    size = 0x10000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );

#ifdef __REACTOS__
    if (!base)
    {
        skip("VirtualAlloc(MEM_WRITE_WATCH) is not supported yet on ReactOS\n");
        skip("Skipping tests due to hang. See ROSTESTS-385\n");
        return;
    }
#endif

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 16, "wrong count %lu\n", count );

    bufs[0].len = 5;
    bufs[0].buf = base;
    bufs[1].len = 0x8000;
    bufs[1].buf = base + 0x4000;

    ret = WSARecv( dest, bufs, 2, NULL, &flags, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %d\n", ret, GetLastError());

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 9, "wrong count %lu\n", count );
    ok( !base[0], "data set\n" );

    send(src, "test message", sizeof("test message"), 0);

    ret = GetOverlappedResult( (HANDLE)dest, &ov, &bytesReturned, TRUE );
    ok( ret, "GetOverlappedResult failed %u\n", GetLastError() );
    ok( bytesReturned == sizeof("test message"), "wrong size %u\n", bytesReturned );
    ok( !memcmp( base, "test ", 5 ), "wrong data %s\n", base );
    ok( !memcmp( base + 0x4000, "message", 8 ), "wrong data %s\n", base + 0x4000 );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 16, "wrong count %lu\n", count );

    bufs[1].len = 0x4000;
    bufs[1].buf = base + 0x2000;
    ret = WSARecvFrom( dest, bufs, 2, NULL, &flags, &addr, &addr_len, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %d\n", ret, GetLastError());

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 5, "wrong count %lu\n", count );
    ok( !base[0], "data set\n" );

    send(src, "test message", sizeof("test message"), 0);

    ret = GetOverlappedResult( (HANDLE)dest, &ov, &bytesReturned, TRUE );
    ok( ret, "GetOverlappedResult failed %u\n", GetLastError() );
    ok( bytesReturned == sizeof("test message"), "wrong size %u\n", bytesReturned );
    ok( !memcmp( base, "test ", 5 ), "wrong data %s\n", base );
    ok( !memcmp( base + 0x2000, "message", 8 ), "wrong data %s\n", base + 0x2000 );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 16, "wrong count %lu\n", count );

    args.dest = dest;
    args.base = base;
    args.size = 0x7002;
    args.expect = "test message";
    for (args.func = 0; args.func < 4; args.func++)
    {
        thread = CreateThread( NULL, 0, write_watch_thread, &args, 0, NULL );
        Sleep( 200 );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
        ok( count == 8, "wrong count %lu\n", count );

        send(src, "test message", sizeof("test message"), 0);
        WaitForSingleObject( thread, 10000 );
        CloseHandle( thread );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
        ok( count == 0, "wrong count %lu\n", count );
    }
    WSACloseEvent( event );
    closesocket( dest );
    closesocket( src );
    VirtualFree( base, 0, MEM_FREE );
}

#define POLL_CLEAR() ix = 0
#define POLL_SET(s, ev) {fds[ix].fd = s; fds[ix++].events = ev;}
#define POLL_ISSET(s, rev) poll_isset(fds, ix, s, rev)
static BOOL poll_isset(WSAPOLLFD *fds, int max, SOCKET s, int rev)
{
    int k;
    for (k = 0; k < max; k++)
        if (fds[k].fd == s && (fds[k].revents == rev)) return TRUE;
    return FALSE;
}

static void test_WSAPoll(void)
{
    int ix, ret, err, poll_timeout;
    SOCKET fdListen, fdRead, fdWrite;
    struct sockaddr_in address;
    socklen_t len;
    static char tmp_buf[1024];
    WSAPOLLFD fds[16];
    HANDLE thread_handle;
    DWORD id;

    if (!pWSAPoll) /* >= Vista */
    {
        skip("WSAPoll is unsupported, some tests will be skipped.\n");
        return;
    }

    /* Invalid parameters test */
    SetLastError(0xdeadbeef);
    ret = pWSAPoll(NULL, 0, 0);
    err = GetLastError();
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(err == WSAEINVAL, "expected 10022, got %d\n", err);
    SetLastError(0xdeadbeef);
    ret = pWSAPoll(NULL, 1, 0);
    err = GetLastError();
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(err == WSAEFAULT, "expected 10014, got %d\n", err);
    SetLastError(0xdeadbeef);
    ret = pWSAPoll(NULL, 0, 1);
    err = GetLastError();
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(err == WSAEINVAL, "expected 10022, got %d\n", err);
    SetLastError(0xdeadbeef);
    ret = pWSAPoll(NULL, 1, 1);
    err = GetLastError();
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(err == WSAEFAULT, "expected 10014, got %d\n", err);

    /* WSAPoll() tries to mime the unix poll() call. The following tests do:
     * - check if a connection attempt ended with success or error;
     * - check if a pending connection is waiting for acceptance;
     * - check for data to read, availability for write and OOB data
     */
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;
    len = sizeof(address);
    fdListen = setup_server_socket(&address, &len);
    poll_timeout = 100;

    /* When no events are pending poll returns 0 with no error */
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 0, "expected 0, got %d\n", ret);

    /* Test listening socket connection attempt notifications */
    fdWrite = setup_connector_socket(&address, len, TRUE);
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdListen, POLLRDNORM), "fdListen socket events incorrect\n");
    len = sizeof(address);
    fdRead = accept(fdListen, (struct sockaddr*) &address, &len);
    ok(fdRead != INVALID_SOCKET, "expected a valid socket\n");

    /* Test client side connection attempt notifications */
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    POLL_SET(fdRead, POLLIN | POLLOUT);
    POLL_SET(fdWrite, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 2, "expected 2, got %d\n", ret);
    ok(POLL_ISSET(fdWrite, POLLWRNORM), "fdWrite socket events incorrect\n");
    ok(POLL_ISSET(fdRead, POLLWRNORM), "fdRead socket events incorrect\n");
    len = sizeof(id);
    id = 0xdeadbeef;
    err = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char*)&id, &len);
    ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
    ok(id == 0, "expected 0, got %d\n", id);

    /* Test data receiving notifications */
    ret = send(fdWrite, "1234", 4, 0);
    ok(ret == 4, "expected 4, got %d\n", ret);
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    POLL_SET(fdRead, POLLIN);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdRead, POLLRDNORM), "fdRead socket events incorrect\n");
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 4, "expected 4, got %d\n", ret);
    ok(!strcmp(tmp_buf, "1234"), "data received differs from sent\n");

    /* Test OOB data notifications */
    ret = send(fdWrite, "A", 1, MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    POLL_SET(fdRead, POLLIN);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdRead, POLLRDBAND), "fdRead socket events incorrect\n");
    tmp_buf[0] = 0xAF;
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(tmp_buf[0] == 'A', "expected 'A', got 0x%02X\n", tmp_buf[0]);

    /* If the socket is OOBINLINED the notification is like normal data */
    ret = 1;
    ret = setsockopt(fdRead, SOL_SOCKET, SO_OOBINLINE, (char*) &ret, sizeof(ret));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ret = send(fdWrite, "A", 1, MSG_OOB);
    ok(ret == 1, "expected 1, got %d\n", ret);
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    POLL_SET(fdRead, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    tmp_buf[0] = 0xAF;
    SetLastError(0xdeadbeef);
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), MSG_OOB);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(GetLastError() == WSAEINVAL, "expected 10022, got %d\n", GetLastError());
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(tmp_buf[0] == 'A', "expected 'A', got 0x%02X\n", tmp_buf[0]);

    /* Test connection closed notifications */
    ret = closesocket(fdRead);
    ok(ret == 0, "expected 0, got %d\n", ret);
    POLL_CLEAR();
    POLL_SET(fdListen, POLLIN | POLLOUT);
    POLL_SET(fdWrite, POLLIN);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdWrite, POLLHUP), "fdWrite socket events incorrect\n");
    ret = recv(fdWrite, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 0, "expected 0, got %d\n", ret);

    /* When a connection is attempted to a non-listening socket due to a bug
     * in the MS code it will never be notified. This is a long standing issue
     * that will never be fixed for compatibility reasons so we have to deal
     * with it manually. */
    ret = closesocket(fdWrite);
    ok(ret == 0, "expected 0, got %d\n", ret);
    ret = closesocket(fdListen);
    ok(ret == 0, "expected 0, got %d\n", ret);
    len = sizeof(address);
    fdWrite = setup_connector_socket(&address, len, TRUE);
    POLL_CLEAR();
    POLL_SET(fdWrite, POLLIN | POLLOUT);
    poll_timeout = 2000;
    ret = pWSAPoll(fds, ix, poll_timeout);
todo_wine
    ok(ret == 0, "expected 0, got %d\n", ret);
    len = sizeof(id);
    id = 0xdeadbeef;
    err = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char*)&id, &len);
    ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
    ok(id == WSAECONNREFUSED, "expected 10061, got %d\n", id);
    closesocket(fdWrite);

    /* Try poll() on a closed socket after connection */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    closesocket(fdRead);
    POLL_CLEAR();
    POLL_SET(fdWrite, POLLIN | POLLOUT);
    POLL_SET(fdRead, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdRead, POLLNVAL), "fdRead socket events incorrect\n");
    POLL_CLEAR();
    POLL_SET(fdWrite, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
todo_wine
    ok(POLL_ISSET(fdWrite, POLLWRNORM | POLLHUP) || broken(POLL_ISSET(fdWrite, POLLWRNORM)) /* <= 2008 */,
       "fdWrite socket events incorrect\n");
    closesocket(fdWrite);

    /* Close the socket currently being polled in a thread */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %d\n", GetLastError());
    POLL_CLEAR();
    POLL_SET(fdWrite, POLLIN | POLLOUT);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdWrite, POLLWRNORM), "fdWrite socket events incorrect\n");
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);
    /* test again with less flags - behavior changes */
    ok(!tcp_socketpair(&fdRead, &fdWrite), "creating socket pair failed\n");
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %d\n", GetLastError());
    POLL_CLEAR();
    POLL_SET(fdWrite, POLLIN);
    ret = pWSAPoll(fds, ix, poll_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(POLL_ISSET(fdWrite, POLLNVAL), "fdWrite socket events incorrect\n");
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);
}
#undef POLL_SET
#undef POLL_ISSET
#undef POLL_CLEAR

static void test_GetAddrInfoW(void)
{
    static const WCHAR port[] = {'8','0',0};
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR nxdomain[] =
        {'n','x','d','o','m','a','i','n','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR zero[] = {'0',0};
    int i, ret;
    ADDRINFOW *result, *result2, *p, hint;
    WCHAR name[256];
    DWORD size = sizeof(name)/sizeof(WCHAR);
    /* te su to.winehq.org written in katakana */
    static const WCHAR idn_domain[] =
        {0x30C6,0x30B9,0x30C8,'.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR idn_punycode[] =
        {'x','n','-','-','z','c','k','z','a','h','.','w','i','n','e','h','q','.','o','r','g',0};

    if (!pGetAddrInfoW || !pFreeAddrInfoW)
    {
        win_skip("GetAddrInfoW and/or FreeAddrInfoW not present\n");
        return;
    }
    memset(&hint, 0, sizeof(ADDRINFOW));
    name[0] = 0;
    GetComputerNameExW( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoW(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoW(empty, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    ret = pGetAddrInfoW(NULL, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = pGetAddrInfoW(NULL, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    pFreeAddrInfoW(result);
    pFreeAddrInfoW(result2);

    result = NULL;
    ret = pGetAddrInfoW(empty, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = pGetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    pFreeAddrInfoW(result);
    pFreeAddrInfoW(result2);

    result = NULL;
    ret = pGetAddrInfoW(localhost, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    ret = pGetAddrInfoW(localhost, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    ret = pGetAddrInfoW(localhost, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    ret = pGetAddrInfoW(localhost, port, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    ret = pGetAddrInfoW(localhost, NULL, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    result = NULL;
    SetLastError(0xdeadbeef);
    ret = pGetAddrInfoW(localhost, port, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pFreeAddrInfoW(result);

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = pGetAddrInfoW(name, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = pGetAddrInfoW(empty, NULL, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result2 != NULL, "GetAddrInfoW failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif
    compare_addrinfow(result, result2);
    pFreeAddrInfoW(result);
    pFreeAddrInfoW(result2);

    ret = pGetAddrInfoW(name, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = pGetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result2 != NULL, "GetAddrInfoW failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif
    compare_addrinfow(result, result2);
    pFreeAddrInfoW(result);
    pFreeAddrInfoW(result2);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoW(NULL, NULL, NULL, &result);
    if(ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoW(nxdomain, NULL, NULL, &result);
    if(ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    for (i = 0;i < (sizeof(hinttests) / sizeof(hinttests[0]));i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = pGetAddrInfoW(localhost, NULL, &hint, &result);
        if (!ret)
        {
            if (hinttests[i].error)
                ok(0, "test %d: GetAddrInfoW succeeded unexpectedly\n", i);
            else
            {
                p = result;
                do
                {
                    /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                    if (hinttests[i].family == AF_UNSPEC)
                        ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                           "test %d: expected AF_INET or AF_INET6, got %d\n",
                           i, p->ai_family);
                    else
                        ok(p->ai_family == hinttests[i].family,
                           "test %d: expected family %d, got %d\n",
                           i, hinttests[i].family, p->ai_family);

                    ok(p->ai_socktype == hinttests[i].socktype,
                       "test %d: expected type %d, got %d\n",
                       i, hinttests[i].socktype, p->ai_socktype);
                    ok(p->ai_protocol == hinttests[i].protocol,
                       "test %d: expected protocol %d, got %d\n",
                       i, hinttests[i].protocol, p->ai_protocol);
                    p = p->ai_next;
                }
                while (p);
            }
            pFreeAddrInfoW(result);
        }
        else
        {
            DWORD err = WSAGetLastError();
            if (hinttests[i].error)
                ok(hinttests[i].error == err, "test %d: GetAddrInfoW failed with error %d, expected %d\n",
                   i, err, hinttests[i].error);
            else
                ok(0, "test %d: GetAddrInfoW failed with %d (err %d)\n", i, ret, err);
        }
    }

    /* Test IDN resolution (Internationalized Domain Names) present since Windows 8 */
    trace("Testing punycode IDN %s\n", wine_dbgstr_w(idn_punycode));
    result = NULL;
    ret = pGetAddrInfoW(idn_punycode, NULL, NULL, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);
    pFreeAddrInfoW(result);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result = NULL;
    ret = pGetAddrInfoW(idn_punycode, NULL, &hint, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);

    trace("Testing unicode IDN %s\n", wine_dbgstr_w(idn_domain));
    result2 = NULL;
    ret = pGetAddrInfoW(idn_domain, NULL, NULL, &result2);
    if (ret == WSAHOST_NOT_FOUND && broken(1))
    {
        pFreeAddrInfoW(result);
        win_skip("IDN resolution not supported in Win <= 7\n");
        return;
    }

    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    pFreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = pGetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);

    /* ensure manually resolved punycode and unicode hosts result in same data */
    compare_addrinfow(result, result2);

    pFreeAddrInfoW(result);
    pFreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = pGetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    pFreeAddrInfoW(result2);

    /* Disable IDN resolution and test again*/
    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = AI_DISABLE_IDN_ENCODING;

    SetLastError(0xdeadbeef);
    result2 = NULL;
    ret = pGetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result2 == NULL, "got %p\n", result2);
}

static void test_GetAddrInfoExW(void)
{
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    ADDRINFOEXW *result;
    OVERLAPPED overlapped;
    HANDLE event;
    int ret;

    if (!pGetAddrInfoExW || !pGetAddrInfoExOverlappedResult)
    {
        win_skip("GetAddrInfoExW and/or GetAddrInfoExOverlappedResult not present\n");
        return;
    }

    event = WSACreateEvent();

    result = (ADDRINFOEXW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(empty, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    result = NULL;
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    result = (void*)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    overlapped.hEvent = event;
    ResetEvent(event);
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    pFreeAddrInfoExW(result);

    result = (void*)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(winehq, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == ERROR_IO_PENDING, "expected 11001, got %d\n", WSAGetLastError());
    ret = overlapped.Internal;
    ok(ret == WSAEINPROGRESS || ret == ERROR_SUCCESS, "overlapped.Internal = %u\n", ret);
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == event, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == ERROR_SUCCESS, "overlapped.Internal = %lx\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(result != NULL, "result == NULL\n");
    if (result != NULL)
    {
        ok(!result->ai_blob, "ai_blob != NULL\n");
        ok(!result->ai_bloblen, "ai_bloblen != 0\n");
        ok(!result->ai_provider, "ai_provider = %s\n", wine_dbgstr_guid(result->ai_provider));
        pFreeAddrInfoExW(result);
    }

    result = (void*)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    todo_wine
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    todo_wine
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);
    ret = WaitForSingleObject(event, 0);
    todo_wine_if(ret != WAIT_TIMEOUT) /* Remove when abowe todo_wines are fixed */
    ok(ret == WAIT_TIMEOUT, "wait failed\n");

    WSACloseEvent(event);
}

static void verify_ipv6_addrinfo(ADDRINFOA *result, const char *expectedIp)
{
    SOCKADDR_IN6 *sockaddr6;
    char ipBuffer[256];
    const char *ret;

    ok(result->ai_family == AF_INET6, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in6), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");

    if (result->ai_addr != NULL)
    {
        sockaddr6 = (SOCKADDR_IN6 *)result->ai_addr;
        ok(sockaddr6->sin6_family == AF_INET6, "ai_addr->sin6_family == %d\n", sockaddr6->sin6_family);
        ok(sockaddr6->sin6_port == 0, "ai_addr->sin6_port == %d\n", sockaddr6->sin6_port);

        ZeroMemory(ipBuffer, sizeof(ipBuffer));
        ret = p_inet_ntop(AF_INET6, &sockaddr6->sin6_addr, ipBuffer, sizeof(ipBuffer));
        ok(ret != NULL, "inet_ntop failed (%d)\n", WSAGetLastError());
        ok(strcmp(ipBuffer, expectedIp) == 0, "ai_addr->sin6_addr == '%s' (expected '%s')\n", ipBuffer, expectedIp);
    }
}

static void test_getaddrinfo(void)
{
    int i, ret;
    ADDRINFOA *result, *result2, *p, hint;
    SOCKADDR_IN *sockaddr;
    CHAR name[256], *ip;
    DWORD size = sizeof(name);

    if (!pgetaddrinfo || !pfreeaddrinfo)
    {
        win_skip("getaddrinfo and/or freeaddrinfo not present\n");
        return;
    }
    memset(&hint, 0, sizeof(ADDRINFOA));
    GetComputerNameExA( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pgetaddrinfo(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pgetaddrinfo("", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    ret = pgetaddrinfo(NULL, "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = pgetaddrinfo(NULL, "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    pfreeaddrinfo(result);
    pfreeaddrinfo(result2);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pgetaddrinfo("", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = pgetaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    pfreeaddrinfo(result);
    pfreeaddrinfo(result2);

    result = NULL;
    ret = pgetaddrinfo("localhost", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    ret = pgetaddrinfo("localhost", "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    ret = pgetaddrinfo("localhost", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    ret = pgetaddrinfo("localhost", "80", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    ret = pgetaddrinfo("localhost", NULL, &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pgetaddrinfo("localhost", "80", &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pfreeaddrinfo(result);

    hint.ai_flags = AI_NUMERICHOST;
    result = (void*)0xdeadbeef;
    ret = pgetaddrinfo("localhost", "80", &hint, &result);
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected WSAHOST_NOT_FOUND, got %d\n", WSAGetLastError());
    ok(!result, "result = %p\n", result);
    hint.ai_flags = 0;

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = pgetaddrinfo(name, NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result != NULL, "getaddrinfo failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif

    ret = pgetaddrinfo("", NULL, NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result2 != NULL, "getaddrinfo failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif
    compare_addrinfo(result, result2);
    pfreeaddrinfo(result);
    pfreeaddrinfo(result2);

    ret = pgetaddrinfo(name, "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result != NULL, "getaddrinfo failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif

    ret = pgetaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(result2 != NULL, "getaddrinfo failed\n");
#else
    ok(result != NULL, "GetAddrInfoW failed\n");
#endif
    compare_addrinfo(result, result2);
    pfreeaddrinfo(result);
    pfreeaddrinfo(result2);

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pgetaddrinfo("nxdomain.codeweavers.com", NULL, NULL, &result);
    if(ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    /* Test IPv4 address conversion */
    result = NULL;
    ret = pgetaddrinfo("192.168.1.253", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", ret);
    ok(result->ai_family == AF_INET, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");
    sockaddr = (SOCKADDR_IN *)result->ai_addr;
    ok(sockaddr->sin_family == AF_INET, "ai_addr->sin_family == %d\n", sockaddr->sin_family);
    ok(sockaddr->sin_port == 0, "ai_addr->sin_port == %d\n", sockaddr->sin_port);

    ip = inet_ntoa(sockaddr->sin_addr);
    ok(strcmp(ip, "192.168.1.253") == 0, "sockaddr->ai_addr == '%s'\n", ip);
    pfreeaddrinfo(result);

    /* Test IPv4 address conversion with port */
    result = NULL;
    hint.ai_flags = AI_NUMERICHOST;
    ret = pgetaddrinfo("192.168.1.253:1024", NULL, &hint, &result);
    hint.ai_flags = 0;
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo returned unexpected result: %d\n", ret);
    ok(result == NULL, "expected NULL, got %p\n", result);

    /* Test IPv6 address conversion */
    result = NULL;
    SetLastError(0xdeadbeef);
    ret = pgetaddrinfo("2a00:2039:dead:beef:cafe::6666", NULL, NULL, &result);

    if (result != NULL)
    {
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "2a00:2039:dead:beef:cafe::6666");
        pfreeaddrinfo(result);

        /* Test IPv6 address conversion with brackets */
        result = NULL;
        ret = pgetaddrinfo("[beef::cafe]", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        pfreeaddrinfo(result);

        /* Test IPv6 address conversion with brackets and hints */
        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET6;
        result = NULL;
        ret = pgetaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        pfreeaddrinfo(result);

        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET;
        result = NULL;
        ret = pgetaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        /* Test IPv6 address conversion with brackets and port */
        result = NULL;
        ret = pgetaddrinfo("[beef::cafe]:10239", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        pfreeaddrinfo(result);

        /* Test IPv6 address conversion with unmatched brackets */
        result = NULL;
        hint.ai_flags = AI_NUMERICHOST;
        ret = pgetaddrinfo("[beef::cafe", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        ret = pgetaddrinfo("beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
    }
    else
    {
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
        win_skip("getaddrinfo does not support IPV6\n");
    }

    hint.ai_flags = 0;

    for (i = 0;i < (sizeof(hinttests) / sizeof(hinttests[0]));i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = pgetaddrinfo("localhost", NULL, &hint, &result);
        if(!ret)
        {
            if (hinttests[i].error)
                ok(0, "test %d: getaddrinfo succeeded unexpectedly\n", i);
            else
            {
                p = result;
                do
                {
                    /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                    if (hinttests[i].family == AF_UNSPEC)
                        ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                           "test %d: expected AF_INET or AF_INET6, got %d\n",
                           i, p->ai_family);
                    else
                        ok(p->ai_family == hinttests[i].family,
                           "test %d: expected family %d, got %d\n",
                           i, hinttests[i].family, p->ai_family);

                    ok(p->ai_socktype == hinttests[i].socktype,
                       "test %d: expected type %d, got %d\n",
                       i, hinttests[i].socktype, p->ai_socktype);
                    ok(p->ai_protocol == hinttests[i].protocol,
                       "test %d: expected protocol %d, got %d\n",
                       i, hinttests[i].protocol, p->ai_protocol);
                    p = p->ai_next;
                }
                while (p);
            }
            pfreeaddrinfo(result);
        }
        else
        {
            DWORD err = WSAGetLastError();
            if (hinttests[i].error)
                ok(hinttests[i].error == err, "test %d: getaddrinfo failed with error %d, expected %d\n",
                   i, err, hinttests[i].error);
            else
                ok(0, "test %d: getaddrinfo failed with %d (err %d)\n", i, ret, err);
        }
    }
}

static void test_ConnectEx(void)
{
    SOCKET listener = INVALID_SOCKET;
    SOCKET acceptor = INVALID_SOCKET;
    SOCKET connector = INVALID_SOCKET;
    struct sockaddr_in address, conaddress;
    int addrlen;
    OVERLAPPED overlapped;
    LPFN_CONNECTEX pConnectEx;
    GUID connectExGuid = WSAID_CONNECTEX;
    DWORD bytesReturned;
    char buffer[1024];
    BOOL bret;
    DWORD dwret;
    int iret;

    memset(&overlapped, 0, sizeof(overlapped));

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        skip("could not create listener socket, error %d\n", WSAGetLastError());
        goto end;
    }

    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(listener, (struct sockaddr*)&address, sizeof(address));
    if (iret != 0) {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    addrlen = sizeof(address);
    iret = getsockname(listener, (struct sockaddr*)&address, &addrlen);
    if (iret != 0) {
        skip("failed to lookup bind address, error %d\n", WSAGetLastError());
        goto end;
    }

    if (set_blocking(listener, TRUE)) {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto end;
    }

    bytesReturned = 0xdeadbeef;
    iret = WSAIoctl(connector, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectExGuid, sizeof(connectExGuid),
        &pConnectEx, sizeof(pConnectEx), &bytesReturned, NULL, NULL);
    if (iret) {
        win_skip("WSAIoctl failed to get ConnectEx with ret %d + errno %d\n", iret, WSAGetLastError());
        goto end;
    }

    ok(bytesReturned == sizeof(pConnectEx), "expected sizeof(pConnectEx), got %u\n", bytesReturned);

    bret = pConnectEx(INVALID_SOCKET, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "ConnectEx on invalid socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAEINVAL, "ConnectEx on a unbound socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    if (bret == TRUE || WSAGetLastError() != WSAEINVAL)
    {
        acceptor = accept(listener, NULL, NULL);
        if (acceptor != INVALID_SOCKET) {
            closesocket(acceptor);
            acceptor = INVALID_SOCKET;
        }

        closesocket(connector);
        connector = socket(AF_INET, SOCK_STREAM, 0);
        if (connector == INVALID_SOCKET) {
            skip("could not create connector socket, error %d\n", WSAGetLastError());
            goto end;
        }
    }

    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    if (iret != 0) {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, NULL);
    ok(bret == FALSE && WSAGetLastError() == ERROR_INVALID_PARAMETER, "ConnectEx on a NULL overlapped "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        skip("could not create event object, errno = %d\n", GetLastError());
        goto end;
    }

    iret = listen(listener, 1);
    if (iret != 0) {
        skip("listening failed, errno = %d\n", WSAGetLastError());
        goto end;
    }

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "ConnectEx failed: "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret, "Connecting failed, error %d\n", GetLastError());
    ok(bytesReturned == 0, "Bytes sent is %d\n", bytesReturned);

    closesocket(connector);
    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }
    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    if (iret != 0) {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    acceptor = accept(listener, NULL, NULL);
    if (acceptor != INVALID_SOCKET) {
        closesocket(acceptor);
    }

    buffer[0] = '1';
    buffer[1] = '2';
    buffer[2] = '3';
    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, buffer, 3, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "ConnectEx failed: "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret, "Connecting failed, error %d\n", GetLastError());
    ok(bytesReturned == 3, "Bytes sent is %d\n", bytesReturned);

    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "could not accept socket error %d\n", WSAGetLastError());

    bytesReturned = recv(acceptor, buffer, 3, 0);
    buffer[4] = 0;
    ok(bytesReturned == 3, "Didn't get all sent data, got only %d\n", bytesReturned);
    ok(buffer[0] == '1' && buffer[1] == '2' && buffer[2] == '3',
       "Failed to get the right data, expected '123', got '%s'\n", buffer);

    closesocket(connector);
    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }
    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    if (iret != 0) {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    if (acceptor != INVALID_SOCKET) {
        closesocket(acceptor);
        acceptor = INVALID_SOCKET;
    }

    /* Connect with error */
    closesocket(listener);
    listener = INVALID_SOCKET;

    address.sin_port = htons(1);

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && GetLastError() == ERROR_IO_PENDING, "ConnectEx to bad destination failed: "
        "returned %d + errno %d\n", bret, GetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret == FALSE && GetLastError() == ERROR_CONNECTION_REFUSED,
       "Connecting to a disconnected host returned error %d - %d\n", bret, WSAGetLastError());

end:
    if (overlapped.hEvent)
        WSACloseEvent(overlapped.hEvent);
    if (listener != INVALID_SOCKET)
        closesocket(listener);
    if (acceptor != INVALID_SOCKET)
        closesocket(acceptor);
    if (connector != INVALID_SOCKET)
        closesocket(connector);
}

static void test_AcceptEx(void)
{
    SOCKET listener = INVALID_SOCKET;
    SOCKET acceptor = INVALID_SOCKET;
    SOCKET connector = INVALID_SOCKET;
    SOCKET connector2 = INVALID_SOCKET;
    struct sockaddr_in bindAddress, peerAddress, *readBindAddress, *readRemoteAddress;
    int socklen, optlen;
    GUID acceptExGuid = WSAID_ACCEPTEX, getAcceptExGuid = WSAID_GETACCEPTEXSOCKADDRS;
    LPFN_ACCEPTEX pAcceptEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS pGetAcceptExSockaddrs = NULL;
    fd_set fds_accept, fds_send;
    struct timeval timeout = {0,10}; /* wait for 10 milliseconds */
    int got, conn1, i;
    DWORD bytesReturned, connect_time;
    char buffer[1024], ipbuffer[32];
    OVERLAPPED overlapped;
    int iret, localSize = sizeof(struct sockaddr_in), remoteSize = localSize;
    BOOL bret;
    DWORD dwret;

    memset(&overlapped, 0, sizeof(overlapped));

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        skip("could not create listener socket, error %d\n", WSAGetLastError());
        goto end;
    }

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }

    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(listener, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    if (iret != 0) {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    socklen = sizeof(bindAddress);
    iret = getsockname(listener, (struct sockaddr*)&bindAddress, &socklen);
    if (iret != 0) {
        skip("failed to lookup bind address, error %d\n", WSAGetLastError());
        goto end;
    }

    if (set_blocking(listener, FALSE)) {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto end;
    }

    iret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid),
        &pAcceptEx, sizeof(pAcceptEx), &bytesReturned, NULL, NULL);
    if (iret) {
        skip("WSAIoctl failed to get AcceptEx with ret %d + errno %d\n", iret, WSAGetLastError());
        goto end;
    }

    iret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptExGuid, sizeof(getAcceptExGuid),
        &pGetAcceptExSockaddrs, sizeof(pGetAcceptExSockaddrs), &bytesReturned, NULL, NULL);
    if (iret) {
        skip("WSAIoctl failed to get GetAcceptExSockaddrs with ret %d + errno %d\n", iret, WSAGetLastError());
        goto end;
    }

    bret = pAcceptEx(INVALID_SOCKET, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "AcceptEx on invalid listening socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
todo_wine
    ok(bret == FALSE && WSAGetLastError() == WSAEINVAL, "AcceptEx on a non-listening socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    iret = listen(listener, 5);
    if (iret != 0) {
        skip("listening failed, errno = %d\n", WSAGetLastError());
        goto end;
    }

    bret = pAcceptEx(listener, INVALID_SOCKET, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "AcceptEx on invalid accepting socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, NULL, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    todo_wine ok(bret == FALSE && WSAGetLastError() == WSAEFAULT,
        "AcceptEx on NULL buffer returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING,
        "AcceptEx on too small local address size returned %d + errno %d\n",
        bret, WSAGetLastError());
    bret = CancelIo((HANDLE) listener);
    ok(bret, "Failed to cancel pending accept socket\n");

    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 15,
        sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx on too small local address "
        "size returned %d + errno %d\n",
        bret, WSAGetLastError());
    bret = CancelIo((HANDLE) listener);
    ok(bret, "Failed to cancel pending accept socket\n");

    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 16, 0,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAEFAULT,
        "AcceptEx on too small remote address size returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 16,
        sizeof(struct sockaddr_in) + 15, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING,
        "AcceptEx on too small remote address size returned %d + errno %d\n", bret, WSAGetLastError());
    bret = CancelIo((HANDLE) listener);
    ok(bret, "Failed to cancel pending accept socket\n");

    bret = pAcceptEx(listener, acceptor, buffer, 0,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, NULL);
    ok(bret == FALSE && WSAGetLastError() == ERROR_INVALID_PARAMETER, "AcceptEx on a NULL overlapped "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, 0, &bytesReturned, NULL);
    ok(bret == FALSE && WSAGetLastError() == ERROR_INVALID_PARAMETER, "AcceptEx on a NULL overlapped "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        skip("could not create event object, errno = %d\n", GetLastError());
        goto end;
    }

    bret = pAcceptEx(listener, acceptor, buffer, 0,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pAcceptEx(listener, acceptor, buffer, 0,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    todo_wine ok(bret == FALSE && WSAGetLastError() == WSAEINVAL,
       "AcceptEx on already pending socket returned %d + errno %d\n", bret, WSAGetLastError());
    if (bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING) {
        /* We need to cancel this call, otherwise things fail */
        bret = CancelIo((HANDLE) listener);
        ok(bret, "Failed to cancel failed test. Bailing...\n");
        if (!bret) return;
        WaitForSingleObject(overlapped.hEvent, 0);

        bret = pAcceptEx(listener, acceptor, buffer, 0,
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &bytesReturned, &overlapped);
        ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());
    }

    iret = connect(acceptor,  (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    todo_wine ok(iret == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL,
       "connecting to acceptex acceptor succeeded? return %d + errno %d\n", iret, WSAGetLastError());
    if (!iret || (iret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)) {
        /* We need to cancel this call, otherwise things fail */
        closesocket(acceptor);
        acceptor = socket(AF_INET, SOCK_STREAM, 0);
        if (acceptor == INVALID_SOCKET) {
            skip("could not create acceptor socket, error %d\n", WSAGetLastError());
            goto end;
        }

        bret = CancelIo((HANDLE) listener);
        ok(bret, "Failed to cancel failed test. Bailing...\n");
        if (!bret) return;

        bret = pAcceptEx(listener, acceptor, buffer, 0,
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &bytesReturned, &overlapped);
        ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());
    }

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, INFINITE);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %d\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %d\n", bytesReturned);

    closesocket(connector);
    connector = INVALID_SOCKET;
    closesocket(acceptor);

    /* Test short reads */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }
    bret = pAcceptEx(listener, acceptor, buffer, 2,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    connect_time = 0xdeadbeef;
    optlen = sizeof(connect_time);
    iret = getsockopt(connector, SOL_SOCKET, SO_CONNECT_TIME, (char *)&connect_time, &optlen);
    ok(!iret, "getsockopt failed %d\n", WSAGetLastError());
    ok(connect_time == ~0u, "unexpected connect time %u\n", connect_time);

    /* AcceptEx() still won't complete until we send data */
    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    connect_time = 0xdeadbeef;
    optlen = sizeof(connect_time);
    iret = getsockopt(connector, SOL_SOCKET, SO_CONNECT_TIME, (char *)&connect_time, &optlen);
    ok(!iret, "getsockopt failed %d\n", WSAGetLastError());
    ok(connect_time < 0xdeadbeef, "unexpected connect time %u\n", connect_time);

    dwret = WaitForSingleObject(overlapped.hEvent, 0);
    ok(dwret == WAIT_TIMEOUT, "Waiting for accept event timeout failed with %d + errno %d\n", dwret, GetLastError());

    iret = getsockname( connector, (struct sockaddr *)&peerAddress, &remoteSize);
    ok( !iret, "getsockname failed.\n");

    /* AcceptEx() could complete any time now */
    iret = send(connector, buffer, 1, 0);
    ok(iret == 1, "could not send 1 byte: send %d errno %d\n", iret, WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    /* Check if the buffer from AcceptEx is decoded correctly */
    pGetAcceptExSockaddrs(buffer, 2, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                          (struct sockaddr **)&readBindAddress, &localSize,
                          (struct sockaddr **)&readRemoteAddress, &remoteSize);
    strcpy( ipbuffer, inet_ntoa(readBindAddress->sin_addr));
    ok( readBindAddress->sin_addr.s_addr == bindAddress.sin_addr.s_addr,
            "Local socket address is different %s != %s\n",
            ipbuffer, inet_ntoa(bindAddress.sin_addr));
    ok( readBindAddress->sin_port == bindAddress.sin_port,
            "Local socket port is different: %d != %d\n",
            readBindAddress->sin_port, bindAddress.sin_port);
    strcpy( ipbuffer, inet_ntoa(readRemoteAddress->sin_addr));
    ok( readRemoteAddress->sin_addr.s_addr == peerAddress.sin_addr.s_addr,
            "Remote socket address is different %s != %s\n",
            ipbuffer, inet_ntoa(peerAddress.sin_addr));
    ok( readRemoteAddress->sin_port == peerAddress.sin_port,
            "Remote socket port is different: %d != %d\n",
            readRemoteAddress->sin_port, peerAddress.sin_port);

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %d\n", GetLastError());
    ok(bytesReturned == 1, "bytesReturned isn't supposed to be %d\n", bytesReturned);

    closesocket(connector);
    connector = INVALID_SOCKET;
    closesocket(acceptor);

    /* Test CF_DEFER & AcceptEx interaction */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }
    connector2 = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    if (set_blocking(connector, FALSE)) {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto end;
    }

    if (set_blocking(connector2, FALSE)) {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto end;
    }

    /* Connect socket #1 */
    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    FD_ZERO ( &fds_accept );
    FD_ZERO ( &fds_send );

    FD_SET ( listener, &fds_accept );
    FD_SET ( connector, &fds_send );

    buffer[0] = '0';
    got = 0;
    conn1 = 0;

    for (i = 0; i < 4000; ++i)
    {
        fd_set fds_openaccept = fds_accept, fds_opensend = fds_send;

        wsa_ok ( ( select ( 0, &fds_openaccept, &fds_opensend, NULL, &timeout ) ), SOCKET_ERROR !=,
            "acceptex test(%d): could not select on socket, errno %d\n" );

        /* check for incoming requests */
        if ( FD_ISSET ( listener, &fds_openaccept ) ) {
            got++;
            if (got == 1) {
                SOCKET tmp = WSAAccept(listener, NULL, NULL, (LPCONDITIONPROC) AlwaysDeferConditionFunc, 0);
                ok(tmp == INVALID_SOCKET && WSAGetLastError() == WSATRY_AGAIN, "Failed to defer connection, %d\n", WSAGetLastError());
                bret = pAcceptEx(listener, acceptor, buffer, 0,
                                    sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                                    &bytesReturned, &overlapped);
                ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());
            }
            else if (got == 2) {
                /* this should be socket #2 */
                SOCKET tmp = accept(listener, NULL, NULL);
                ok(tmp != INVALID_SOCKET, "accept failed %d\n", WSAGetLastError());
                closesocket(tmp);
            }
            else {
                ok(FALSE, "Got more than 2 connections?\n");
            }
        }
        if ( conn1 && FD_ISSET ( connector2, &fds_opensend ) ) {
            /* Send data on second socket, and stop */
            send(connector2, "2", 1, 0);
            FD_CLR ( connector2, &fds_send );

            break;
        }
        if ( FD_ISSET ( connector, &fds_opensend ) ) {
            /* Once #1 is connected, allow #2 to connect */
            conn1 = 1;

            send(connector, "1", 1, 0);
            FD_CLR ( connector, &fds_send );

            iret = connect(connector2, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
            ok(iret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK, "connecting to accepting socket failed, error %d\n", WSAGetLastError());
            FD_SET ( connector2, &fds_send );
        }
    }

    ok (got == 2 || broken(got == 1) /* NT4 */,
            "Did not get both connections, got %d\n", got);

    dwret = WaitForSingleObject(overlapped.hEvent, 0);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %d\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %d\n", bytesReturned);

    set_blocking(acceptor, TRUE);
    iret = recv( acceptor, buffer, 2, 0);
    ok(iret == 1, "Failed to get data, %d, errno: %d\n", iret, WSAGetLastError());

    ok(buffer[0] == '1', "The wrong first client was accepted by acceptex: %c != 1\n", buffer[0]);

    closesocket(connector);
    connector = INVALID_SOCKET;
    closesocket(acceptor);

    /* clean up in case of failures */
    while ((acceptor = accept(listener, NULL, NULL)) != INVALID_SOCKET)
        closesocket(acceptor);

    /* Disconnect during receive? */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    closesocket(connector);
    connector = INVALID_SOCKET;

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    bytesReturned = 123456;
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %d\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %d\n", bytesReturned);

    closesocket(acceptor);

    /* Test closing with pending requests */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    closesocket(acceptor);

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    todo_wine ok(dwret == WAIT_OBJECT_0,
       "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    if (dwret != WAIT_TIMEOUT) {
        bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
        ok(!bret && GetLastError() == ERROR_OPERATION_ABORTED, "GetOverlappedResult failed, error %d\n", GetLastError());
    }
    else {
        bret = CancelIo((HANDLE) listener);
        ok(bret, "Failed to cancel failed test. Bailing...\n");
        if (!bret) return;
        WaitForSingleObject(overlapped.hEvent, 0);
    }

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    CancelIo((HANDLE) acceptor);

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_TIMEOUT, "Waiting for timeout failed with %d + errno %d\n", dwret, GetLastError());

    closesocket(acceptor);

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    if (acceptor == INVALID_SOCKET) {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    closesocket(listener);
    listener = INVALID_SOCKET;

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %d + errno %d\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(!bret && GetLastError() == ERROR_OPERATION_ABORTED, "GetOverlappedResult failed, error %d\n", GetLastError());

end:
    if (overlapped.hEvent)
        WSACloseEvent(overlapped.hEvent);
    if (listener != INVALID_SOCKET)
        closesocket(listener);
    if (acceptor != INVALID_SOCKET)
        closesocket(acceptor);
    if (connector != INVALID_SOCKET)
        closesocket(connector);
    if (connector2 != INVALID_SOCKET)
        closesocket(connector2);
}

static void test_DisconnectEx(void)
{
    SOCKET listener, acceptor, connector;
    LPFN_DISCONNECTEX pDisconnectEx;
    GUID disconnectExGuid = WSAID_DISCONNECTEX;
    struct sockaddr_in address;
    DWORD num_bytes, flags;
    OVERLAPPED overlapped;
    int addrlen, iret;
    BOOL bret;

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create connector socket, error %d\n", WSAGetLastError());

    iret = WSAIoctl(connector, SIO_GET_EXTENSION_FUNCTION_POINTER, &disconnectExGuid, sizeof(disconnectExGuid),
                    &pDisconnectEx, sizeof(pDisconnectEx), &num_bytes, NULL, NULL);
    if (iret)
    {
        win_skip("WSAIoctl failed to get DisconnectEx, error %d\n", WSAGetLastError());
        closesocket(connector);
        return;
    }

    listener = socket(AF_INET, SOCK_STREAM, 0);
    ok(listener != INVALID_SOCKET, "failed to create listener socket, error %d\n", WSAGetLastError());

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(listener, (struct sockaddr *)&address, sizeof(address));
    ok(iret == 0, "failed to bind, error %d\n", WSAGetLastError());

    addrlen = sizeof(address);
    iret = getsockname(listener, (struct sockaddr *)&address, &addrlen);
    ok(iret == 0, "failed to lookup bind address, error %d\n", WSAGetLastError());

    iret = listen(listener, 1);
    ok(iret == 0, "failed to listen, error %d\n", WSAGetLastError());

    set_blocking(listener, TRUE);

    memset(&overlapped, 0, sizeof(overlapped));
    bret = pDisconnectEx(INVALID_SOCKET, &overlapped, 0, 0);
    ok(bret == FALSE, "DisconnectEx unexpectedly succeeded\n");
    ok(WSAGetLastError() == WSAENOTSOCK, "expected WSAENOTSOCK, got %d\n", WSAGetLastError());

    memset(&overlapped, 0, sizeof(overlapped));
    bret = pDisconnectEx(connector, &overlapped, 0, 0);
    ok(bret == FALSE, "DisconnectEx unexpectedly succeeded\n");
    todo_wine ok(WSAGetLastError() == WSAENOTCONN, "expected WSAENOTCONN, got %d\n", WSAGetLastError());

    iret = connect(connector, (struct sockaddr *)&address, addrlen);
    ok(iret == 0, "failed to connect, error %d\n", WSAGetLastError());

    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "could not accept socket, error %d\n", WSAGetLastError());

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = WSACreateEvent();
    ok(overlapped.hEvent != WSA_INVALID_EVENT, "WSACreateEvent failed, error %d\n", WSAGetLastError());
    bret = pDisconnectEx(connector, &overlapped, 0, 0);
    if (bret)
        ok(overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %08lx\n", overlapped.Internal);
    else if (WSAGetLastError() == ERROR_IO_PENDING)
        bret = WSAGetOverlappedResult(connector, &overlapped, &num_bytes, TRUE, &flags);
    ok(bret, "DisconnectEx failed, error %d\n", WSAGetLastError());
    WSACloseEvent(overlapped.hEvent);

    iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
    ok(iret != 0, "connect unexpectedly succeeded\n");
    ok(WSAGetLastError() == WSAEISCONN, "expected WSAEISCONN, got %d\n", WSAGetLastError());

    closesocket(acceptor);
    closesocket(connector);

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create connector socket, error %d\n", WSAGetLastError());

    iret = connect(connector, (struct sockaddr *)&address, addrlen);
    ok(iret == 0, "failed to connect, error %d\n", WSAGetLastError());

    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "could not accept socket, error %d\n", WSAGetLastError());

    bret = pDisconnectEx(connector, NULL, 0, 0);
    ok(bret, "DisconnectEx failed, error %d\n", WSAGetLastError());

    iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
    ok(iret != 0, "connect unexpectedly succeeded\n");
    ok(WSAGetLastError() == WSAEISCONN, "expected WSAEISCONN, got %d\n", WSAGetLastError());

    closesocket(acceptor);
    closesocket(connector);
    closesocket(listener);
}

#define compare_file(h,s,o) compare_file2(h,s,o,__FILE__,__LINE__)

static void compare_file2(HANDLE handle, SOCKET sock, int offset, const char *file, int line)
{
    char buf1[256], buf2[256];
    BOOL success;
    int i = 0;

    SetFilePointer(handle, offset, NULL, FILE_BEGIN);
    while (1)
    {
        DWORD n1 = 0, n2 = 0;

        success = ReadFile(handle, buf1, sizeof(buf1), &n1, NULL);
        ok_(file,line)(success, "Failed to read from file.\n");
        if (success && n1 == 0)
            break;
        else if(!success)
            return;
        n2 = recv(sock, buf2, n1, 0);
        ok_(file,line)(n1 == n2, "Block %d size mismatch (%d != %d)\n", i, n1, n2);
        ok_(file,line)(memcmp(buf1, buf2, n2) == 0, "Block %d failed\n", i);
        i++;
    }
}

static void test_TransmitFile(void)
{
    DWORD num_bytes, err, file_size, total_sent;
    GUID transmitFileGuid = WSAID_TRANSMITFILE;
    LPFN_TRANSMITFILE pTransmitFile = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    char header_msg[] = "hello world";
    char footer_msg[] = "goodbye!!!";
    char system_ini_path[MAX_PATH];
    struct sockaddr_in bindAddress;
    TRANSMIT_FILE_BUFFERS buffers;
    SOCKET client, server, dest;
    WSAOVERLAPPED ov;
    char buf[256];
    int iret, len;
    BOOL bret;

    memset( &ov, 0, sizeof(ov) );

    /* Setup sockets for testing TransmitFile */
    client = socket(AF_INET, SOCK_STREAM, 0);
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (client == INVALID_SOCKET || server == INVALID_SOCKET)
    {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto cleanup;
    }
    iret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, &transmitFileGuid, sizeof(transmitFileGuid),
                    &pTransmitFile, sizeof(pTransmitFile), &num_bytes, NULL, NULL);
    if (iret)
    {
        skip("WSAIoctl failed to get TransmitFile with ret %d + errno %d\n", iret, WSAGetLastError());
        goto cleanup;
    }
    GetSystemWindowsDirectoryA(system_ini_path, MAX_PATH );
    strcat(system_ini_path, "\\system.ini");
    file = CreateFileA(system_ini_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0x0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("Unable to open a file to transmit.\n");
        goto cleanup;
    }
    file_size = GetFileSize(file, NULL);

    /* Test TransmitFile with an invalid socket */
    bret = pTransmitFile(INVALID_SOCKET, file, 0, 0, NULL, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTSOCK, "TransmitFile triggered unexpected errno (%d != %d)\n", err, WSAENOTSOCK);

    /* Test a bogus TransmitFile without a connected socket */
    bret = pTransmitFile(client, NULL, 0, 0, NULL, NULL, TF_REUSE_SOCKET);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTCONN, "TransmitFile triggered unexpected errno (%d != %d)\n", err, WSAENOTCONN);

    /* Setup a properly connected socket for transfers */
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = htons(SERVERPORT+1);
    bindAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(server, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    if (iret != 0)
    {
        skip("failed to bind(), error %d\n", WSAGetLastError());
        goto cleanup;
    }
    iret = listen(server, 1);
    if (iret != 0)
    {
        skip("failed to listen(), error %d\n", WSAGetLastError());
        goto cleanup;
    }
    iret = connect(client, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    if (iret != 0)
    {
        skip("failed to connect(), error %d\n", WSAGetLastError());
        goto cleanup;
    }
    len = sizeof(bindAddress);
    dest = accept(server, (struct sockaddr*)&bindAddress, &len);
    if (dest == INVALID_SOCKET)
    {
        skip("failed to accept(), error %d\n", WSAGetLastError());
        goto cleanup;
    }
    if (set_blocking(dest, FALSE))
    {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto cleanup;
    }

    /* Test TransmitFile with no possible buffer */
    bret = pTransmitFile(client, NULL, 0, 0, NULL, NULL, 0);
    ok(bret, "TransmitFile failed unexpectedly.\n");
    iret = recv(dest, buf, sizeof(buf), 0);
    ok(iret == -1, "Returned an unexpected buffer from TransmitFile (%d != -1).\n", iret);

    /* Test TransmitFile with only buffer data */
    buffers.Head = &header_msg[0];
    buffers.HeadLength = sizeof(header_msg);
    buffers.Tail = &footer_msg[0];
    buffers.TailLength = sizeof(footer_msg);
    bret = pTransmitFile(client, NULL, 0, 0, NULL, &buffers, 0);
    ok(bret, "TransmitFile failed unexpectedly.\n");
    iret = recv(dest, buf, sizeof(buf), 0);
    ok(iret == sizeof(header_msg)+sizeof(footer_msg),
       "Returned an unexpected buffer from TransmitFile: %d\n", iret );
    ok(memcmp(&buf[0], &header_msg[0], sizeof(header_msg)) == 0,
       "TransmitFile header buffer did not match!\n");
    ok(memcmp(&buf[sizeof(header_msg)], &footer_msg[0], sizeof(footer_msg)) == 0,
       "TransmitFile footer buffer did not match!\n");

    /* Test TransmitFile with only file data */
    bret = pTransmitFile(client, file, 0, 0, NULL, NULL, 0);
    ok(bret, "TransmitFile failed unexpectedly.\n");
    compare_file(file, dest, 0);

    /* Test TransmitFile with both file and buffer data */
    buffers.Head = &header_msg[0];
    buffers.HeadLength = sizeof(header_msg);
    buffers.Tail = &footer_msg[0];
    buffers.TailLength = sizeof(footer_msg);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    bret = pTransmitFile(client, file, 0, 0, NULL, &buffers, 0);
    ok(bret, "TransmitFile failed unexpectedly.\n");
    iret = recv(dest, buf, sizeof(header_msg), 0);
    ok(memcmp(buf, &header_msg[0], sizeof(header_msg)) == 0,
       "TransmitFile header buffer did not match!\n");
    compare_file(file, dest, 0);
    iret = recv(dest, buf, sizeof(footer_msg), 0);
    ok(memcmp(buf, &footer_msg[0], sizeof(footer_msg)) == 0,
       "TransmitFile footer buffer did not match!\n");

    /* Test overlapped TransmitFile */
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (ov.hEvent == INVALID_HANDLE_VALUE)
    {
        skip("Could not create event object, some tests will be skipped. errno = %d\n",
             GetLastError());
        goto cleanup;
    }
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    bret = pTransmitFile(client, file, 0, 0, &ov, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%d != %d)\n",
       err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == file_size,
       "Overlapped TransmitFile sent an unexpected number of bytes (%d != %d).\n",
       total_sent, file_size);
    compare_file(file, dest, 0);

    /* Test overlapped TransmitFile w/ start offset */
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (ov.hEvent == INVALID_HANDLE_VALUE)
    {
        skip("Could not create event object, some tests will be skipped. errno = %d\n", GetLastError());
        goto cleanup;
    }
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ov.Offset = 10;
    bret = pTransmitFile(client, file, 0, 0, &ov, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%d != %d)\n", err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == (file_size - ov.Offset),
       "Overlapped TransmitFile sent an unexpected number of bytes (%d != %d).\n",
       total_sent, file_size - ov.Offset);
    compare_file(file, dest, ov.Offset);

    /* Test overlapped TransmitFile w/ file and buffer data */
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (ov.hEvent == INVALID_HANDLE_VALUE)
    {
        skip("Could not create event object, some tests will be skipped. errno = %d\n", GetLastError());
        goto cleanup;
    }
    buffers.Head = &header_msg[0];
    buffers.HeadLength = sizeof(header_msg);
    buffers.Tail = &footer_msg[0];
    buffers.TailLength = sizeof(footer_msg);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ov.Offset = 0;
    bret = pTransmitFile(client, file, 0, 0, &ov, &buffers, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%d != %d)\n", err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == (file_size + buffers.HeadLength + buffers.TailLength),
       "Overlapped TransmitFile sent an unexpected number of bytes (%d != %d).\n",
       total_sent, file_size  + buffers.HeadLength + buffers.TailLength);
    iret = recv(dest, buf, sizeof(header_msg), 0);
    ok(memcmp(buf, &header_msg[0], sizeof(header_msg)) == 0,
       "TransmitFile header buffer did not match!\n");
    compare_file(file, dest, 0);
    iret = recv(dest, buf, sizeof(footer_msg), 0);
    ok(memcmp(buf, &footer_msg[0], sizeof(footer_msg)) == 0,
       "TransmitFile footer buffer did not match!\n");

    /* Test TransmitFile w/ TF_DISCONNECT */
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    bret = pTransmitFile(client, file, 0, 0, NULL, NULL, TF_DISCONNECT);
    ok(bret, "TransmitFile failed unexpectedly.\n");
    compare_file(file, dest, 0);
    closesocket(client);
    ok(send(client, "test", 4, 0) == -1, "send() after TF_DISCONNECT succeeded unexpectedly.\n");
    err = WSAGetLastError();
    todo_wine ok(err == WSAENOTSOCK, "send() after TF_DISCONNECT triggered unexpected errno (%d != %d)\n",
                 err, WSAENOTSOCK);

    /* Test TransmitFile with a UDP datagram socket */
    client = socket(AF_INET, SOCK_DGRAM, 0);
    bret = pTransmitFile(client, NULL, 0, 0, NULL, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTCONN, "TransmitFile triggered unexpected errno (%d != %d)\n", err, WSAENOTCONN);

cleanup:
    CloseHandle(file);
    CloseHandle(ov.hEvent);
    closesocket(client);
    closesocket(server);
}

static void test_getpeername(void)
{
    SOCKET sock;
    struct sockaddr_in sa, sa_out;
    SOCKADDR_STORAGE ss;
    int sa_len;
    const char buf[] = "hello world";
    int ret;

    /* Test the parameter validation order. */
    ret = getpeername(INVALID_SOCKET, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return SOCKET_ERROR, got %d\n", ret);
    ok(WSAGetLastError() == WSAENOTSOCK,
       "Expected WSAGetLastError() to return WSAENOTSOCK, got %d\n", WSAGetLastError());

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    ok(sock != INVALID_SOCKET, "Expected socket to return a valid socket\n");
    if (sock == INVALID_SOCKET)
    {
        skip("Socket creation failed with %d\n", WSAGetLastError());
        return;
    }

    ret = getpeername(sock, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return SOCKET_ERROR, got %d\n", ret);
    ok(WSAGetLastError() == WSAENOTCONN,
       "Expected WSAGetLastError() to return WSAENOTCONN, got %d\n", WSAGetLastError());

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(139);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* sendto does not change a socket's connection state. */
    ret = sendto(sock, buf, sizeof(buf), 0, (struct sockaddr*)&sa, sizeof(sa));
    ok(ret != SOCKET_ERROR,
       "Expected sendto to succeed, WSAGetLastError() = %d\n", WSAGetLastError());

    ret = getpeername(sock, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return SOCKET_ERROR, got %d\n", ret);
    ok(WSAGetLastError() == WSAENOTCONN,
       "Expected WSAGetLastError() to return WSAENOTCONN, got %d\n", WSAGetLastError());

    ret = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
    ok(ret == 0,
       "Expected connect to succeed, WSAGetLastError() = %d\n", WSAGetLastError());

    ret = getpeername(sock, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return SOCKET_ERROR, got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT,
       "Expected WSAGetLastError() to return WSAEFAULT, got %d\n", WSAGetLastError());

    /* Test crashes on Wine. */
    if (0)
    {
        ret = getpeername(sock, (void*)0xdeadbeef, (void*)0xcafebabe);
        ok(ret == SOCKET_ERROR, "Expected getpeername to return SOCKET_ERROR, got %d\n", ret);
        ok(WSAGetLastError() == WSAEFAULT,
           "Expected WSAGetLastError() to return WSAEFAULT, got %d\n", WSAGetLastError());
    }

    ret = getpeername(sock, (struct sockaddr*)&sa_out, NULL);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return 0, got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT,
       "Expected WSAGetLastError() to return WSAEFAULT, got %d\n", WSAGetLastError());

    sa_len = 0;
    ret = getpeername(sock, NULL, &sa_len);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return 0, got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT,
       "Expected WSAGetLastError() to return WSAEFAULT, got %d\n", WSAGetLastError());
    ok(!sa_len, "got %d\n", sa_len);

    sa_len = 0;
    ret = getpeername(sock, (struct sockaddr *)&ss, &sa_len);
    ok(ret == SOCKET_ERROR, "Expected getpeername to return 0, got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT,
       "Expected WSAGetLastError() to return WSAEFAULT, got %d\n", WSAGetLastError());
    ok(!sa_len, "got %d\n", sa_len);

    sa_len = sizeof(ss);
    ret = getpeername(sock, (struct sockaddr *)&ss, &sa_len);
    ok(ret == 0, "Expected getpeername to return 0, got %d\n", ret);
    ok(!memcmp(&sa, &ss, sizeof(sa)),
       "Expected the returned structure to be identical to the connect structure\n");
    ok(sa_len == sizeof(sa), "got %d\n", sa_len);

    closesocket(sock);
}

static void test_sioRoutingInterfaceQuery(void)
{
    int ret;
    SOCKET sock;
    SOCKADDR_IN sin = { 0 }, sout = { 0 };
    DWORD bytesReturned;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    ok(sock != INVALID_SOCKET, "Expected socket to return a valid socket\n");
    if (sock == INVALID_SOCKET)
    {
        skip("Socket creation failed with %d\n", WSAGetLastError());
        return;
    }
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, NULL, 0, NULL, 0, NULL,
                   NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "expected WSAEFAULT, got %d\n", WSAGetLastError());
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &sin, sizeof(sin),
                   NULL, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "expected WSAEFAULT, got %d\n", WSAGetLastError());
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &sin, sizeof(sin),
                   NULL, 0, &bytesReturned, NULL, NULL);
    todo_wine ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEAFNOSUPPORT,
       "expected WSAEAFNOSUPPORT, got %d\n", WSAGetLastError());
    sin.sin_family = AF_INET;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &sin, sizeof(sin),
                   NULL, 0, &bytesReturned, NULL, NULL);
    todo_wine ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL,
       "expected WSAEINVAL, got %d\n", WSAGetLastError());
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &sin, sizeof(sin),
                   NULL, 0, &bytesReturned, NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "expected WSAEFAULT, got %d\n", WSAGetLastError());
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &sin, sizeof(sin),
                   &sout, sizeof(sout), &bytesReturned, NULL, NULL);
    ok(!ret, "WSAIoctl failed: %d\n", WSAGetLastError());
    ok(sout.sin_family == AF_INET, "expected AF_INET, got %d\n", sout.sin_family);
    /* We expect the source address to be INADDR_LOOPBACK as well, but
     * there's no guarantee that a route to the loopback address exists,
     * so rather than introduce spurious test failures we do not test the
     * source address.
     */
    closesocket(sock);
}

static void test_sioAddressListChange(void)
{
    struct sockaddr_in bindAddress;
    struct in_addr net_address;
    WSAOVERLAPPED overlapped, *olp;
    struct hostent *h;
    DWORD num_bytes, error, tick;
    SOCKET sock, sock2, sock3;
    WSAEVENT event2, event3;
    HANDLE io_port;
    ULONG_PTR key;
    int acount;
    BOOL bret;
    int ret;

    /* Use gethostbyname to find the list of local network interfaces */
    h = gethostbyname("");
    if (!h)
    {
        skip("Cannot test SIO_ADDRESS_LIST_CHANGE, gethostbyname failed with %u\n",
             WSAGetLastError());
        return;
    }
    for (acount = 0; h->h_addr_list[acount]; acount++);
    if (acount == 0)
    {
        skip("Cannot test SIO_ADDRESS_LIST_CHANGE, test requires a network card.\n");
        return;
    }

    net_address.s_addr = *(ULONG *) h->h_addr_list[0];

    sock = socket(AF_INET, 0, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = net_address.s_addr;
    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %d\n", GetLastError());
    set_blocking(sock, FALSE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %d\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%x\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = socket(AF_INET, 0, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %d\n", GetLastError());
    set_blocking(sock, TRUE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %d\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%x\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %d\n", GetLastError());
    set_blocking(sock, FALSE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %d\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%x\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %d\n", GetLastError());
    set_blocking(sock, TRUE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %d\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%x\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    /* When the socket is overlapped non-blocking and the list change is requested without
     * an overlapped structure the error will be different. */
    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %d\n", GetLastError());
    set_blocking(sock, FALSE);

    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, NULL, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %d\n", error);
    ok (error == WSAEWOULDBLOCK, "expected 10035, got %d\n", error);

    io_port = CreateIoCompletionPort( (HANDLE)sock, NULL, 0, 0 );
    ok (io_port != NULL, "failed to create completion port %u\n", GetLastError());

    set_blocking(sock, FALSE);
    memset(&overlapped, 0, sizeof(overlapped));
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %u\n", error);
    ok (error == ERROR_IO_PENDING, "expected ERROR_IO_PENDING got %u\n", error);

    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 0 );
    ok(!bret, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(sock);

    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 0 );
    ok(!bret, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %u\n", GetLastError());
    ok(olp == &overlapped, "Overlapped structure is at %p\n", olp);

    CloseHandle(io_port);

    /* Misuse of the API by using a blocking socket and not using an overlapped structure,
     * this leads to a hang forever. */
    if (0)
    {
        sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

        SetLastError(0xdeadbeef);
        bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));

        set_blocking(sock, TRUE);
        WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, NULL, NULL);
        /* hang */

        closesocket(sock);
    }

    if (!winetest_interactive)
    {
        skip("Cannot test SIO_ADDRESS_LIST_CHANGE, interactive tests must be enabled\n");
        return;
    }

    /* Bind an overlapped socket to the first found network interface */
    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "Expected socket to return a valid socket\n");
    sock2 = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock2 != INVALID_SOCKET, "Expected socket to return a valid socket\n");
    sock3 = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock3 != INVALID_SOCKET, "Expected socket to return a valid socket\n");

    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!ret, "bind failed unexpectedly\n");
    ret = bind(sock2, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!ret, "bind failed unexpectedly\n");
    ret = bind(sock3, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!ret, "bind failed unexpectedly\n");

    set_blocking(sock2, FALSE);
    set_blocking(sock3, FALSE);

    /* Wait for address changes, request that the user connects/disconnects an interface */
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(WSAGetLastError() == WSA_IO_PENDING, "Expected pending last error, got %d\n", WSAGetLastError());

    ret = WSAIoctl(sock2, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "Expected would block last error, got %d\n", WSAGetLastError());

    event2 = WSACreateEvent();
    event3 = WSACreateEvent();
    ret = WSAEventSelect (sock2, event2, FD_ADDRESS_LIST_CHANGE);
    ok(!ret, "WSAEventSelect failed with %d\n", WSAGetLastError());
    /* sock3 did not request SIO_ADDRESS_LIST_CHANGE but it is trying to wait anyway */
    ret = WSAEventSelect (sock3, event3, FD_ADDRESS_LIST_CHANGE);
    ok(!ret, "WSAEventSelect failed with %d\n", WSAGetLastError());

    trace("Testing socket-based ipv4 address list change notification. Please connect/disconnect or"
          " change the ipv4 address of any of the local network interfaces (15 second timeout).\n");
    tick = GetTickCount();
    ret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(ret == WAIT_OBJECT_0, "failed to get overlapped event %u\n", ret);

    ret = WaitForSingleObject(event2, 500);
todo_wine
    ok(ret == WAIT_OBJECT_0, "failed to get change event %u\n", ret);

    ret = WaitForSingleObject(event3, 500);
    ok(ret == WAIT_TIMEOUT, "unexpected change event\n");

    trace("Spent %d ms waiting.\n", GetTickCount() - tick);

    WSACloseEvent(event2);
    WSACloseEvent(event3);

    closesocket(sock);
    closesocket(sock2);
    closesocket(sock3);
}

static void test_synchronous_WSAIoctl(void)
{
    HANDLE previous_port, io_port;
    WSAOVERLAPPED overlapped, *olp;
    SOCKET socket;
    ULONG on;
    ULONG_PTR key;
    DWORD num_bytes;
    BOOL ret;
    int res;

    previous_port = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
    ok( previous_port != NULL, "failed to create completion port %u\n", GetLastError() );

    socket = WSASocketW( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
    ok( socket != INVALID_SOCKET, "failed to create socket %d\n", WSAGetLastError() );

    io_port = CreateIoCompletionPort( (HANDLE)socket, previous_port, 0, 0 );
    ok( io_port != NULL, "failed to create completion port %u\n", GetLastError() );

    on = 1;
    memset( &overlapped, 0, sizeof(overlapped) );
    res = WSAIoctl( socket, FIONBIO, &on, sizeof(on), NULL, 0, &num_bytes, &overlapped, NULL );
    ok( !res, "WSAIoctl failed %d\n", WSAGetLastError() );

    ret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 10000 );
    ok( ret, "failed to get completion status %u\n", GetLastError() );

    CloseHandle( io_port );
    closesocket( socket );
    CloseHandle( previous_port );
}

#define WM_ASYNCCOMPLETE (WM_USER + 100)
static HWND create_async_message_window(void)
{
    static const char class_name[] = "ws2_32 async message window class";

    WNDCLASSEXA wndclass;
    HWND hWnd;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = DefWindowProcA;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = GetModuleHandleA(NULL);
    wndclass.hIcon          = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hIconSm        = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hCursor        = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszClassName  = class_name;
    wndclass.lpszMenuName   = NULL;

    RegisterClassExA(&wndclass);

    hWnd = CreateWindowA(class_name, "ws2_32 async message window", WS_OVERLAPPEDWINDOW,
                        0, 0, 500, 500, NULL, NULL, GetModuleHandleA(NULL), NULL);
    if (!hWnd)
    {
        ok(0, "failed to create window: %u\n", GetLastError());
        return NULL;
    }

    return hWnd;
}

static void wait_for_async_message(HWND hwnd, HANDLE handle)
{
    BOOL ret;
    MSG msg;

    while ((ret = GetMessageA(&msg, 0, 0, 0)) &&
           !(msg.hwnd == hwnd && msg.message == WM_ASYNCCOMPLETE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok(ret, "did not expect WM_QUIT message\n");
    ok(msg.wParam == (WPARAM)handle, "expected wParam = %p, got %lx\n", handle, msg.wParam);
}

static void test_WSAAsyncGetServByPort(void)
{
    HWND hwnd = create_async_message_window();
    HANDLE ret;
    char buffer[MAXGETHOSTSTRUCT];

    if (!hwnd)
        return;

    /* FIXME: The asynchronous window messages should be tested. */

    /* Parameters are not checked when initiating the asynchronous operation.  */
    ret = WSAAsyncGetServByPort(NULL, 0, 0, NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, 0, NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, htons(80), NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, htons(80), NULL, buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    DestroyWindow(hwnd);
}

static void test_WSAAsyncGetServByName(void)
{
    HWND hwnd = create_async_message_window();
    HANDLE ret;
    char buffer[MAXGETHOSTSTRUCT];

    if (!hwnd)
        return;

    /* FIXME: The asynchronous window messages should be tested. */

    /* Parameters are not checked when initiating the asynchronous operation.  */
    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "", NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "", "", buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "http", NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "http", "tcp", buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    DestroyWindow(hwnd);
}

/*
 * Provide consistent initialization for the AcceptEx IOCP tests.
 */
static SOCKET setup_iocp_src(struct sockaddr_in *bindAddress)
{
    SOCKET src, ret = INVALID_SOCKET;
    int iret, socklen;

    src = socket(AF_INET, SOCK_STREAM, 0);
    if (src == INVALID_SOCKET)
    {
        skip("could not create listener socket, error %d\n", WSAGetLastError());
        goto end;
    }

    memset(bindAddress, 0, sizeof(*bindAddress));
    bindAddress->sin_family = AF_INET;
    bindAddress->sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(src, (struct sockaddr*)bindAddress, sizeof(*bindAddress));
    if (iret != 0)
    {
        skip("failed to bind, error %d\n", WSAGetLastError());
        goto end;
    }

    socklen = sizeof(*bindAddress);
    iret = getsockname(src, (struct sockaddr*)bindAddress, &socklen);
    if (iret != 0) {
        skip("failed to lookup bind address, error %d\n", WSAGetLastError());
        goto end;
    }

    if (set_blocking(src, FALSE))
    {
        skip("couldn't make socket non-blocking, error %d\n", WSAGetLastError());
        goto end;
    }

    iret = listen(src, 5);
    if (iret != 0)
    {
        skip("listening failed, errno = %d\n", WSAGetLastError());
        goto end;
    }

    ret = src;
end:
    if (src != ret && ret == INVALID_SOCKET)
        closesocket(src);
    return ret;
}

static void test_completion_port(void)
{
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION io_info;
    HANDLE previous_port, io_port;
    WSAOVERLAPPED ov, *olp;
    SOCKET src, dest, dup, connector = INVALID_SOCKET;
    WSAPROTOCOL_INFOA info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    char buf[1024];
    WSABUF bufs;
    DWORD num_bytes, flags;
    struct linger ling;
    int iret;
    BOOL bret;
    ULONG_PTR key;
    struct sockaddr_in bindAddress;
    GUID acceptExGuid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX pAcceptEx = NULL;
    fd_set fds_recv;

    memset(buf, 0, sizeof(buf));
    previous_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    ok( previous_port != NULL, "Failed to create completion port %u\n", GetLastError());

    memset(&ov, 0, sizeof(ov));

    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        goto end;
    }

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;

    ling.l_onoff = 1;
    ling.l_linger = 0;
    iret = setsockopt (src, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
    ok(!iret, "Failed to set linger %d\n", GetLastError());

    io_port = CreateIoCompletionPort( (HANDLE)dest, previous_port, 125, 0 );
    ok(io_port != NULL, "Failed to create completion port %u\n", GetLastError());

    SetLastError(0xdeadbeef);

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSARecv returned %d\n", iret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    Sleep(100);

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    todo_wine ok(bret == FALSE, "GetQueuedCompletionStatus returned %d\n", bret);
    todo_wine ok(GetLastError() == ERROR_NETNAME_DELETED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes received is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %d\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);

    memset(&ov, 0, sizeof(ov));

    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        goto end;
    }

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;

    ling.l_onoff = 1;
    ling.l_linger = 0;
    iret = setsockopt (src, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
    ok(!iret, "Failed to set linger %d\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    set_blocking(dest, FALSE);

    closesocket(src);
    src = INVALID_SOCKET;

    Sleep(100);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);

    iret = WSASend(dest, &bufs, 1, &num_bytes, 0, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSASend failed - %d\n", iret);
    ok(GetLastError() == WSAECONNRESET, "Last error was %d\n", GetLastError());
    ok(num_bytes == 0xdeadbeef, "Managed to send %d\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);

    /* Test IOCP response on successful immediate read. */
    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        goto end;
    }

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;
    SetLastError(0xdeadbeef);

    iret = WSASend(src, &bufs, 1, &num_bytes, 0, &ov, NULL);
    ok(!iret, "WSASend failed - %d, last error %u\n", iret, GetLastError());
    ok(num_bytes == sizeof(buf), "Managed to send %d\n", num_bytes);

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());
    set_blocking(dest, FALSE);

    FD_ZERO(&fds_recv);
    FD_SET(dest, &fds_recv);
    select(dest + 1, &fds_recv, NULL, NULL, NULL);

    num_bytes = 0xdeadbeef;
    flags = 0;

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(!iret, "WSARecv failed - %d, last error %u\n", iret, GetLastError());
    ok(num_bytes == sizeof(buf), "Managed to read %d\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == sizeof(buf), "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);

    /* Test IOCP response on graceful shutdown. */
    closesocket(src);

    FD_ZERO(&fds_recv);
    FD_SET(dest, &fds_recv);
    select(dest + 1, &fds_recv, NULL, NULL, NULL);

    num_bytes = 0xdeadbeef;
    flags = 0;
    memset(&ov, 0, sizeof(ov));

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(!iret, "WSARecv failed - %d, last error %u\n", iret, GetLastError());
    ok(!num_bytes, "Managed to read %d\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(!num_bytes, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    src = INVALID_SOCKET;
    closesocket(dest);
    dest = INVALID_SOCKET;

    /* Test IOCP response on hard shutdown. This was the condition that triggered
     * a crash in an actual app (bug 38980). */
    tcp_socketpair(&src, &dest);
    if (src == INVALID_SOCKET || dest == INVALID_SOCKET)
    {
        skip("failed to create sockets\n");
        goto end;
    }

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;
    memset(&ov, 0, sizeof(ov));

    ling.l_onoff = 1;
    ling.l_linger = 0;
    iret = setsockopt (src, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling));
    ok(!iret, "Failed to set linger %d\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());
    set_blocking(dest, FALSE);

    closesocket(src);
    src = INVALID_SOCKET;

    FD_ZERO(&fds_recv);
    FD_SET(dest, &fds_recv);
    select(dest + 1, &fds_recv, NULL, NULL, NULL);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);

    /* Somehow a hard shutdown doesn't work on my Linux box. It seems SO_LINGER is ignored. */
    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    todo_wine ok(iret == SOCKET_ERROR, "WSARecv failed - %d\n", iret);
    todo_wine ok(GetLastError() == WSAECONNRESET, "Last error was %d\n", GetLastError());
    todo_wine ok(num_bytes == 0xdeadbeef, "Managed to read %d\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    todo_wine ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    todo_wine ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    todo_wine ok(key == 0xdeadbeef, "Key is %lu\n", key);
    todo_wine ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    todo_wine ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(dest);

    /* Test reading from a non-connected socket, mostly because the above test is marked todo. */
    dest = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(dest != INVALID_SOCKET, "socket() failed\n");

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());
    set_blocking(dest, FALSE);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    memset(&ov, 0, sizeof(ov));

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSARecv failed - %d\n", iret);
    ok(GetLastError() == WSAENOTCONN, "Last error was %d\n", GetLastError());
    ok(num_bytes == 0xdeadbeef, "Managed to read %d\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    num_bytes = 0xdeadbeef;
    closesocket(dest);

    dest = socket(AF_INET, SOCK_STREAM, 0);
    if (dest == INVALID_SOCKET)
    {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }

    iret = WSAIoctl(dest, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid),
            &pAcceptEx, sizeof(pAcceptEx), &num_bytes, NULL, NULL);
    if (iret)
    {
        skip("WSAIoctl failed to get AcceptEx with ret %d + errno %d\n", iret, WSAGetLastError());
        goto end;
    }

    /* Test IOCP response on socket close (IOCP created after AcceptEx) */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP response on socket close (IOCP created before AcceptEx) */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    src = INVALID_SOCKET;
    closesocket(dup);
    dup = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && olp->Internal == (ULONG)STATUS_CANCELLED, "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle (closing duplicated handle) */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    closesocket(dup);
    dup = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    src = INVALID_SOCKET;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle (closing original handle) */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(dup);
    dup = INVALID_SOCKET;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP without AcceptEx */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", GetLastError());

    closesocket(connector);
    connector = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_SUCCESS), "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);
    if (src != INVALID_SOCKET)
        closesocket(dest);

    /* */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    dest = socket(AF_INET, SOCK_STREAM, 0);
    if (dest == INVALID_SOCKET)
    {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }

    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    io_info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile((HANDLE)src, &io, &io_info, sizeof(io_info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_INVALID_INFO_CLASS) /* XP */,
       "expected STATUS_SUCCESS, got %08x\n", status);

    io_info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile((HANDLE)dest, &io, &io_info, sizeof(io_info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_INVALID_INFO_CLASS) /* XP */,
       "expected STATUS_SUCCESS, got %08x\n", status);

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", GetLastError());

    iret = send(connector, buf, 1, 0);
    ok(iret == 1, "could not send 1 byte: send %d errno %d\n", iret, WSAGetLastError());

    Sleep(100);

    closesocket(dest);
    dest = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 1, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_SUCCESS), "Internal status is %lx\n", olp ? olp->Internal : 0);

    io_info.Flags = 0;
    status = pNtQueryInformationFile((HANDLE)dest, &io, &io_info, sizeof(io_info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS || broken(status == STATUS_INVALID_INFO_CLASS) /* XP */,
       "expected STATUS_SUCCESS, got %08x\n", status);
    if (status == STATUS_SUCCESS)
        ok((io_info.Flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) != 0, "got %08x\n", io_info.Flags);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (src != INVALID_SOCKET)
        closesocket(src);
    if (connector != INVALID_SOCKET)
        closesocket(connector);

    /* */

    if ((src = setup_iocp_src(&bindAddress)) == INVALID_SOCKET)
        goto end;

    dest = socket(AF_INET, SOCK_STREAM, 0);
    if (dest == INVALID_SOCKET)
    {
        skip("could not create acceptor socket, error %d\n", WSAGetLastError());
        goto end;
    }

    connector = socket(AF_INET, SOCK_STREAM, 0);
    if (connector == INVALID_SOCKET) {
        skip("could not create connector socket, error %d\n", WSAGetLastError());
        goto end;
    }

    io_port = CreateIoCompletionPort((HANDLE)src, previous_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, previous_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %u\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %d\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", GetLastError());

    closesocket(dest);
    dest = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    todo_wine ok(GetLastError() == ERROR_NETNAME_DELETED ||
                 GetLastError() == ERROR_OPERATION_ABORTED ||
                 GetLastError() == ERROR_CONNECTION_ABORTED ||
                 GetLastError() == ERROR_PIPE_NOT_CONNECTED /* win 2000 */,
                 "Last error was %d\n", GetLastError());
    ok(key == 125, "Key is %lu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %u\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    todo_wine ok(olp && (olp->Internal == (ULONG)STATUS_LOCAL_DISCONNECT ||
                         olp->Internal == (ULONG)STATUS_CANCELLED ||
                         olp->Internal == (ULONG)STATUS_CONNECTION_ABORTED ||
                         olp->Internal == (ULONG)STATUS_PIPE_DISCONNECTED /* win 2000 */),
                         "Internal status is %lx\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %d\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %lu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %u\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);


    end:
    if (dest != INVALID_SOCKET)
        closesocket(dest);
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (connector != INVALID_SOCKET)
        closesocket(connector);
    CloseHandle(previous_port);
}

static void test_address_list_query(void)
{
    SOCKET_ADDRESS_LIST *address_list;
    DWORD bytes_returned, size;
    unsigned int i;
    SOCKET s;
    int ret;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s != INVALID_SOCKET, "Failed to create socket, error %d.\n", WSAGetLastError());

    bytes_returned = 0;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, 0, &bytes_returned, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(bytes_returned >= FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]),
            "Got unexpected bytes_returned %u.\n", bytes_returned);

    size = bytes_returned;
    bytes_returned = 0;
    address_list = HeapAlloc(GetProcessHeap(), 0, size * 2);
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, address_list, size * 2, &bytes_returned, NULL, NULL);
    ok(!ret, "Got unexpected ret %d, error %d.\n", ret, WSAGetLastError());
    ok(bytes_returned == size, "Got unexpected bytes_returned %u, expected %u.\n", bytes_returned, size);

    bytes_returned = FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[address_list->iAddressCount]);
    for (i = 0; i < address_list->iAddressCount; ++i)
    {
        bytes_returned += address_list->Address[i].iSockaddrLength;
    }
    ok(size == bytes_returned, "Got unexpected size %u, expected %u.\n", size, bytes_returned);

    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, address_list, size, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());

    bytes_returned = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, size, &bytes_returned, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(bytes_returned == size, "Got unexpected bytes_returned %u, expected %u.\n", bytes_returned, size);

    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, address_list, 1, &bytes_returned, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "Got unexpected error %d.\n", WSAGetLastError());
    ok(bytes_returned == 0, "Got unexpected bytes_returned %u.\n", bytes_returned);

    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, address_list,
            FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]), &bytes_returned, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(bytes_returned == size, "Got unexpected bytes_returned %u, expected %u.\n", bytes_returned, size);

    HeapFree(GetProcessHeap(), 0, address_list);
    closesocket(s);
}

static DWORD WINAPI inet_ntoa_thread_proc(void *param)
{
    ULONG addr;
    const char *str;
    HANDLE *event = param;

    addr = inet_addr("4.3.2.1");
    ok(addr == htonl(0x04030201), "expected 0x04030201, got %08x\n", addr);
    str = inet_ntoa(*(struct in_addr *)&addr);
    ok(!strcmp(str, "4.3.2.1"), "expected 4.3.2.1, got %s\n", str);

    SetEvent(event[0]);
    WaitForSingleObject(event[1], 3000);

    return 0;
}

static void test_inet_ntoa(void)
{
    ULONG addr;
    const char *str;
    HANDLE thread, event[2];
    DWORD tid;

    addr = inet_addr("1.2.3.4");
    ok(addr == htonl(0x01020304), "expected 0x01020304, got %08x\n", addr);
    str = inet_ntoa(*(struct in_addr *)&addr);
    ok(!strcmp(str, "1.2.3.4"), "expected 1.2.3.4, got %s\n", str);

    event[0] = CreateEventW(NULL, TRUE, FALSE, NULL);
    event[1] = CreateEventW(NULL, TRUE, FALSE, NULL);

    thread = CreateThread(NULL, 0, inet_ntoa_thread_proc, event, 0, &tid);
    WaitForSingleObject(event[0], 3000);

    ok(!strcmp(str, "1.2.3.4"), "expected 1.2.3.4, got %s\n", str);

    SetEvent(event[1]);
    WaitForSingleObject(thread, 3000);

    CloseHandle(event[0]);
    CloseHandle(event[1]);
    CloseHandle(thread);
}

static void test_WSALookupService(void)
{
    char buffer[4096], strbuff[128];
    WSAQUERYSETW *qs = NULL;
    HANDLE hnd;
    PNLA_BLOB netdata;
    int ret;
    DWORD error, offset, bsize;

    if (!pWSALookupServiceBeginW || !pWSALookupServiceEnd || !pWSALookupServiceNextW)
    {
        win_skip("WSALookupServiceBeginW or WSALookupServiceEnd or WSALookupServiceNextW not found\n");
        return;
    }

    qs = (WSAQUERYSETW *)buffer;
    memset(qs, 0, sizeof(*qs));

    /* invalid parameter tests */
    ret = pWSALookupServiceBeginW(NULL, 0, &hnd);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
todo_wine
    ok(error == WSAEFAULT, "expected 10014, got %d\n", error);

    ret = pWSALookupServiceBeginW(qs, 0, NULL);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
todo_wine
    ok(error == WSAEFAULT, "expected 10014, got %d\n", error);

    ret = pWSALookupServiceBeginW(qs, 0, &hnd);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
todo_wine
    ok(error == WSAEINVAL
       || broken(error == ERROR_INVALID_PARAMETER) /* == XP */
       || broken(error == WSAEFAULT) /* == NT */
       || broken(error == WSASERVICE_NOT_FOUND) /* == 2000 */,
       "expected 10022, got %d\n", error);

    ret = pWSALookupServiceEnd(NULL);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "WSALookupServiceEnd should have failed\n");
todo_wine
    ok(error == ERROR_INVALID_HANDLE, "expected 6, got %d\n", error);

    /* standard network list query */
    qs->dwSize = sizeof(*qs);
    hnd = (HANDLE)0xdeadbeef;
    ret = pWSALookupServiceBeginW(qs, LUP_RETURN_ALL | LUP_DEEP, &hnd);
    error = WSAGetLastError();
    if(ret && error == ERROR_INVALID_PARAMETER)
    {
        win_skip("the current WSALookupServiceBeginW test is not supported in win <= 2000\n");
        return;
    }

todo_wine
    ok(!ret, "WSALookupServiceBeginW failed unexpectedly with error %d\n", error);
todo_wine
    ok(hnd != (HANDLE)0xdeadbeef, "Handle was not filled\n");

    offset = 0;
    do
    {
        memset(qs, 0, sizeof(*qs));
        bsize = sizeof(buffer);

        if (pWSALookupServiceNextW(hnd, 0, &bsize, qs) == SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if (error == WSA_E_NO_MORE) break;
            ok(0, "Error %d happened while listing services\n", error);
            break;
        }

        WideCharToMultiByte(CP_ACP, 0, qs->lpszServiceInstanceName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
        trace("Network Name: %s\n", strbuff);

        /* network data is written in the blob field */
        if (qs->lpBlob)
        {
            /* each network may have multiple NLA_BLOB information structures */
            do
            {
                netdata = (PNLA_BLOB) &qs->lpBlob->pBlobData[offset];
                switch (netdata->header.type)
                {
                    case NLA_RAW_DATA:
                        trace("\tNLA Data Type: NLA_RAW_DATA\n");
                        break;
                    case NLA_INTERFACE:
                        trace("\tNLA Data Type: NLA_INTERFACE\n");
                        trace("\t\tType: %d\n", netdata->data.interfaceData.dwType);
                        trace("\t\tSpeed: %d\n", netdata->data.interfaceData.dwSpeed);
                        trace("\t\tAdapter Name: %s\n", netdata->data.interfaceData.adapterName);
                        break;
                    case NLA_802_1X_LOCATION:
                        trace("\tNLA Data Type: NLA_802_1X_LOCATION\n");
                        trace("\t\tInformation: %s\n", netdata->data.locationData.information);
                        break;
                    case NLA_CONNECTIVITY:
                        switch (netdata->data.connectivity.type)
                        {
                            case NLA_NETWORK_AD_HOC:
                                trace("\t\tNetwork Type: AD HOC\n");
                                break;
                            case NLA_NETWORK_MANAGED:
                                trace("\t\tNetwork Type: Managed\n");
                                break;
                            case NLA_NETWORK_UNMANAGED:
                                trace("\t\tNetwork Type: Unmanaged\n");
                                break;
                            case NLA_NETWORK_UNKNOWN:
                                trace("\t\tNetwork Type: Unknown\n");
                        }
                        switch (netdata->data.connectivity.internet)
                        {
                            case NLA_INTERNET_NO:
                                trace("\t\tInternet connectivity: No\n");
                                break;
                            case NLA_INTERNET_YES:
                                trace("\t\tInternet connectivity: Yes\n");
                                break;
                            case NLA_INTERNET_UNKNOWN:
                                trace("\t\tInternet connectivity: Unknown\n");
                                break;
                        }
                        break;
                    case NLA_ICS:
                        trace("\tNLA Data Type: NLA_ICS\n");
                        trace("\t\tSpeed: %d\n",
                               netdata->data.ICS.remote.speed);
                        trace("\t\tType: %d\n",
                               netdata->data.ICS.remote.type);
                        trace("\t\tState: %d\n",
                               netdata->data.ICS.remote.state);
                        WideCharToMultiByte(CP_ACP, 0, netdata->data.ICS.remote.machineName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
                        trace("\t\tMachine Name: %s\n", strbuff);
                        WideCharToMultiByte(CP_ACP, 0, netdata->data.ICS.remote.sharedAdapterName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
                        trace("\t\tShared Adapter Name: %s\n", strbuff);
                        break;
                    default:
                        trace("\tNLA Data Type: Unknown\n");
                        break;
                }
            }
            while (offset);
        }
    }
    while (1);

    ret = pWSALookupServiceEnd(hnd);
    ok(!ret, "WSALookupServiceEnd failed unexpectedly\n");
}

static void test_WSAEnumNameSpaceProvidersA(void)
{
    LPWSANAMESPACE_INFOA name = NULL;
    DWORD ret, error, blen = 0, i;
    if (!pWSAEnumNameSpaceProvidersA)
    {
        win_skip("WSAEnumNameSpaceProvidersA not found\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersA(&blen, name);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    /* Invalid parameter tests */
    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersA(NULL, name);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersA(NULL, NULL);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersA(&blen, NULL);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

#ifdef __REACTOS__ /* ROSTESTS-233 */
    if (!blen)
    {
        skip("Failed to get length needed for name space providers.\n");
        return;
    }
#endif

    name = HeapAlloc(GetProcessHeap(), 0, blen);
    if (!name)
    {
        skip("Failed to alloc memory\n");
        return;
    }

    ret = pWSAEnumNameSpaceProvidersA(&blen, name);
todo_wine
    ok(ret > 0, "Expected more than zero name space providers\n");

    for (i = 0;i < ret; i++)
    {
        trace("Name space Identifier (%p): %s\n", name[i].lpszIdentifier,
              name[i].lpszIdentifier);
        switch (name[i].dwNameSpace)
        {
            case NS_DNS:
                trace("\tName space ID: NS_DNS (%u)\n", name[i].dwNameSpace);
                break;
            case NS_NLA:
                trace("\tName space ID: NS_NLA (%u)\n", name[i].dwNameSpace);
                break;
            default:
                trace("\tName space ID: Unknown (%u)\n", name[i].dwNameSpace);
                break;
        }
        trace("\tActive:  %d\n", name[i].fActive);
        trace("\tVersion: %d\n", name[i].dwVersion);
    }

    HeapFree(GetProcessHeap(), 0, name);
}

static void test_WSAEnumNameSpaceProvidersW(void)
{
    LPWSANAMESPACE_INFOW name = NULL;
    DWORD ret, error, blen = 0, i;
    if (!pWSAEnumNameSpaceProvidersW)
    {
        win_skip("WSAEnumNameSpaceProvidersW not found\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersW(&blen, name);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    /* Invalid parameter tests */
    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersW(NULL, name);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersW(NULL, NULL);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pWSAEnumNameSpaceProvidersW(&blen, NULL);
    error = WSAGetLastError();
todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %u\n", ret);
todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %u\n", error);

#ifdef __REACTOS__ /* ROSTESTS-233 */
    if (!blen)
    {
        skip("Failed to get length needed for name space providers.\n");
        return;
    }
#endif

    name = HeapAlloc(GetProcessHeap(), 0, blen);
    if (!name)
    {
        skip("Failed to alloc memory\n");
        return;
    }

    ret = pWSAEnumNameSpaceProvidersW(&blen, name);
todo_wine
    ok(ret > 0, "Expected more than zero name space providers\n");

    for (i = 0;i < ret; i++)
    {
        trace("Name space Identifier (%p): %s\n", name[i].lpszIdentifier,
               wine_dbgstr_w(name[i].lpszIdentifier));
        switch (name[i].dwNameSpace)
        {
            case NS_DNS:
                trace("\tName space ID: NS_DNS (%u)\n", name[i].dwNameSpace);
                break;
            case NS_NLA:
                trace("\tName space ID: NS_NLA (%u)\n", name[i].dwNameSpace);
                break;
            default:
                trace("\tName space ID: Unknown (%u)\n", name[i].dwNameSpace);
                break;
        }
        trace("\tActive:  %d\n", name[i].fActive);
        trace("\tVersion: %d\n", name[i].dwVersion);
    }

    HeapFree(GetProcessHeap(), 0, name);
}

static void sync_read(SOCKET src, SOCKET dst)
{
    int ret;
    char data[512];

    ret = send(dst, "Hello World!", 12, 0);
    ok(ret == 12, "send returned %d\n", ret);

    memset(data, 0, sizeof(data));
    ret = recv(src, data, sizeof(data), 0);
    ok(ret == 12, "expected 12, got %d\n", ret);
    ok(!memcmp(data, "Hello World!", 12), "got %u bytes (%*s)\n", ret, ret, data);
}

static void iocp_async_read(SOCKET src, SOCKET dst)
{
    HANDLE port;
    WSAOVERLAPPED ovl, *ovl_iocp;
    WSABUF buf;
    int ret;
    char data[512];
    DWORD flags, bytes;
    ULONG_PTR key;

    memset(data, 0, sizeof(data));
    memset(&ovl, 0, sizeof(ovl));

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %u\n", GetLastError());

    buf.len = sizeof(data);
    buf.buf = data;
    bytes = 0xdeadbeef;
    flags = 0;
    SetLastError(0xdeadbeef);
    ret = WSARecv(src, &buf, 1, &bytes, &flags, &ovl, NULL);
    ok(ret == SOCKET_ERROR, "got %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %#lx\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    ret = send(dst, "Hello World!", 12, 0);
    ok(ret == 12, "send returned %d\n", ret);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = NULL;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(ret, "got %d\n", ret);
    ok(bytes == 12, "got bytes %u\n", bytes);
    ok(key == 0x12345678, "got key %#lx\n", key);
    ok(ovl_iocp == &ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(ovl_iocp->InternalHigh == 12, "got %#lx\n", ovl_iocp->InternalHigh);
        ok(!ovl_iocp->Internal , "got %#lx\n", ovl_iocp->Internal);
        ok(!memcmp(data, "Hello World!", 12), "got %u bytes (%*s)\n", bytes, bytes, data);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %#lx\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    CloseHandle(port);
}

static void iocp_async_read_closesocket(SOCKET src, int how_to_close)
{
    HANDLE port;
    WSAOVERLAPPED ovl, *ovl_iocp;
    WSABUF buf;
    int ret;
    char data[512];
    DWORD flags, bytes;
    ULONG_PTR key;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    ret = WSAAsyncSelect(src, hwnd, WM_SOCKET, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
    ok(!ret, "got %d\n", ret);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08lx\n", msg.wParam);
    ok(msg.lParam == 2, "got %08lx\n", msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&ovl, 0, sizeof(ovl));

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %u\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    buf.len = sizeof(data);
    buf.buf = data;
    bytes = 0xdeadbeef;
    flags = 0;
    SetLastError(0xdeadbeef);
    ret = WSARecv(src, &buf, 1, &bytes, &flags, &ovl, NULL);
    ok(ret == SOCKET_ERROR, "got %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %#lx\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    switch (how_to_close)
    {
    case 0:
        closesocket(src);
        break;
    case 1:
        CloseHandle((HANDLE)src);
        break;
    case 2:
        pNtClose((HANDLE)src);
        break;
    default:
        ok(0, "wrong value %d\n", how_to_close);
        break;
    }

    Sleep(200);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    switch (how_to_close)
    {
    case 0:
        ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);
        break;
    case 1:
    case 2:
todo_wine
{
        ok(ret, "got %d\n", ret);
        ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08lx\n", msg.wParam);
        ok(msg.lParam == 0x20, "got %08lx\n", msg.lParam);
}
        break;
    default:
        ok(0, "wrong value %d\n", how_to_close);
        break;
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = NULL;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
todo_wine
    ok(GetLastError() == ERROR_CONNECTION_ABORTED || GetLastError() == ERROR_NETNAME_DELETED /* XP */, "got %u\n", GetLastError());
    ok(!bytes, "got bytes %u\n", bytes);
    ok(key == 0x12345678, "got key %#lx\n", key);
    ok(ovl_iocp == &ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(!ovl_iocp->InternalHigh, "got %#lx\n", ovl_iocp->InternalHigh);
todo_wine
        ok(ovl_iocp->Internal == (ULONG)STATUS_CONNECTION_ABORTED || ovl_iocp->Internal == (ULONG)STATUS_LOCAL_DISCONNECT /* XP */, "got %#lx\n", ovl_iocp->Internal);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %#lx\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    CloseHandle(port);

    DestroyWindow(hwnd);
}

static void iocp_async_closesocket(SOCKET src)
{
    HANDLE port;
    WSAOVERLAPPED *ovl_iocp;
    int ret;
    DWORD bytes;
    ULONG_PTR key;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    ret = WSAAsyncSelect(src, hwnd, WM_SOCKET, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
    ok(!ret, "got %d\n", ret);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08lx\n", msg.wParam);
    ok(msg.lParam == 2, "got %08lx\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %u\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %lu\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    closesocket(src);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %lu\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    CloseHandle(port);

    DestroyWindow(hwnd);
}

struct wsa_async_select_info
{
    SOCKET sock;
    HWND hwnd;
};

static DWORD WINAPI wsa_async_select_thread(void *param)
{
    struct wsa_async_select_info *info = param;
    int ret;

    ret = WSAAsyncSelect(info->sock, info->hwnd, WM_SOCKET, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
    ok(!ret, "got %d\n", ret);

    return 0;
}

struct wsa_recv_info
{
    SOCKET sock;
    WSABUF wsa_buf;
    WSAOVERLAPPED ovl;
};

static DWORD WINAPI wsa_recv_thread(void *param)
{
    struct wsa_recv_info *info = param;
    int ret;
    DWORD flags, bytes;

    bytes = 0xdeadbeef;
    flags = 0;
    SetLastError(0xdeadbeef);
    ret = WSARecv(info->sock, &info->wsa_buf, 1, &bytes, &flags, &info->ovl, NULL);
    ok(ret == SOCKET_ERROR, "got %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);

    return 0;
}

static void iocp_async_read_thread_closesocket(SOCKET src)
{
    struct wsa_async_select_info select_info;
    struct wsa_recv_info recv_info;
    HANDLE port, thread;
    WSAOVERLAPPED *ovl_iocp;
    int ret;
    char data[512];
    DWORD bytes, tid;
    ULONG_PTR key;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    select_info.sock = src;
    select_info.hwnd = hwnd;
    thread = CreateThread(NULL, 0, wsa_async_select_thread, &select_info, 0, &tid);
    ok(thread != 0, "CreateThread error %u\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08lx\n", msg.wParam);
    ok(msg.lParam == 2, "got %08lx\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %u\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&recv_info.ovl, 0, sizeof(recv_info.ovl));
    recv_info.sock = src;
    recv_info.wsa_buf.len = sizeof(data);
    recv_info.wsa_buf.buf = data;
    thread = CreateThread(NULL, 0, wsa_recv_thread, &recv_info, 0, &tid);
    ok(thread != 0, "CreateThread error %u\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT || broken(GetLastError() == ERROR_OPERATION_ABORTED) /* XP */,
       "got %u\n", GetLastError());
    if (GetLastError() == WAIT_TIMEOUT)
    {
        ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
        ok(key == 0xdeadbeef, "got key %lx\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }
    else /* document XP behaviour */
    {
        ok(!bytes, "got bytes %u\n", bytes);
        ok(key == 0x12345678, "got key %#lx\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(!ovl_iocp->InternalHigh, "got %#lx\n", ovl_iocp->InternalHigh);
            ok(ovl_iocp->Internal == STATUS_CANCELLED, "got %#lx\n", ovl_iocp->Internal);
        }

        closesocket(src);
        goto xp_is_broken;
    }

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    closesocket(src);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = NULL;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
todo_wine
    ok(GetLastError() == ERROR_CONNECTION_ABORTED || GetLastError() == ERROR_NETNAME_DELETED /* XP */, "got %u\n", GetLastError());
    ok(!bytes, "got bytes %u\n", bytes);
    ok(key == 0x12345678, "got key %#lx\n", key);
    ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(!ovl_iocp->InternalHigh, "got %#lx\n", ovl_iocp->InternalHigh);
todo_wine
        ok(ovl_iocp->Internal == (ULONG)STATUS_CONNECTION_ABORTED || ovl_iocp->Internal == (ULONG)STATUS_LOCAL_DISCONNECT /* XP */, "got %#lx\n", ovl_iocp->Internal);
    }

xp_is_broken:
    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %u\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
    ok(key == 0xdeadbeef, "got key %lu\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    CloseHandle(port);

    DestroyWindow(hwnd);
}

static void iocp_async_read_thread(SOCKET src, SOCKET dst)
{
    struct wsa_async_select_info select_info;
    struct wsa_recv_info recv_info;
    HANDLE port, thread;
    WSAOVERLAPPED *ovl_iocp;
    int ret;
    char data[512];
    DWORD bytes, tid;
    ULONG_PTR key;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    select_info.sock = src;
    select_info.hwnd = hwnd;
    thread = CreateThread(NULL, 0, wsa_async_select_thread, &select_info, 0, &tid);
    ok(thread != 0, "CreateThread error %u\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08lx\n", msg.wParam);
    ok(msg.lParam == 2, "got %08lx\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %u\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&recv_info.ovl, 0, sizeof(recv_info.ovl));
    recv_info.sock = src;
    recv_info.wsa_buf.len = sizeof(data);
    recv_info.wsa_buf.buf = data;
    thread = CreateThread(NULL, 0, wsa_recv_thread, &recv_info, 0, &tid);
    ok(thread != 0, "CreateThread error %u\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT || broken(GetLastError() == ERROR_OPERATION_ABORTED) /* XP */, "got %u\n", GetLastError());
    if (GetLastError() == WAIT_TIMEOUT)
    {
        ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
        ok(key == 0xdeadbeef, "got key %lu\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }
    else /* document XP behaviour */
    {
        ok(bytes == 0, "got bytes %u\n", bytes);
        ok(key == 0x12345678, "got key %#lx\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(!ovl_iocp->InternalHigh, "got %#lx\n", ovl_iocp->InternalHigh);
            ok(ovl_iocp->Internal == STATUS_CANCELLED, "got %#lx\n", ovl_iocp->Internal);
        }
    }

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret || broken(msg.hwnd == hwnd) /* XP */, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);
    if (ret) /* document XP behaviour */
    {
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08lx\n", msg.wParam);
        ok(msg.lParam == 1, "got %08lx\n", msg.lParam);
    }

    ret = send(dst, "Hello World!", 12, 0);
    ok(ret == 12, "send returned %d\n", ret);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret || broken(msg.hwnd == hwnd) /* XP */, "got %04x,%08lx,%08lx\n", msg.message, msg.wParam, msg.lParam);
    if (ret) /* document XP behaviour */
    {
        ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08lx\n", msg.wParam);
        ok(msg.lParam == 1, "got %08lx\n", msg.lParam);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(ret || broken(GetLastError() == WAIT_TIMEOUT) /* XP */, "got %u\n", GetLastError());
    if (ret)
    {
        ok(bytes == 12, "got bytes %u\n", bytes);
        ok(key == 0x12345678, "got key %#lx\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(ovl_iocp->InternalHigh == 12, "got %#lx\n", ovl_iocp->InternalHigh);
            ok(!ovl_iocp->Internal , "got %#lx\n", ovl_iocp->Internal);
            ok(!memcmp(data, "Hello World!", 12), "got %u bytes (%*s)\n", bytes, bytes, data);
        }
    }
    else /* document XP behaviour */
    {
        ok(bytes == 0xdeadbeef, "got bytes %u\n", bytes);
        ok(key == 0xdeadbeef, "got key %lu\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }

    CloseHandle(port);

    DestroyWindow(hwnd);
}

static void test_iocp(void)
{
    SOCKET src, dst;
    int i, ret;

    ret = tcp_socketpair_ovl(&src, &dst);
    ok(!ret, "creating socket pair failed\n");
    sync_read(src, dst);
    iocp_async_read(src, dst);
    closesocket(src);
    closesocket(dst);

    ret = tcp_socketpair_ovl(&src, &dst);
    ok(!ret, "creating socket pair failed\n");
    iocp_async_read_thread(src, dst);
    closesocket(src);
    closesocket(dst);

    for (i = 0; i <= 2; i++)
    {
        ret = tcp_socketpair_ovl(&src, &dst);
        ok(!ret, "creating socket pair failed\n");
        iocp_async_read_closesocket(src, i);
        closesocket(dst);
    }

    ret = tcp_socketpair_ovl(&src, &dst);
    ok(!ret, "creating socket pair failed\n");
    iocp_async_closesocket(src);
    closesocket(dst);

    ret = tcp_socketpair_ovl(&src, &dst);
    ok(!ret, "creating socket pair failed\n");
    iocp_async_read_thread_closesocket(src);
    closesocket(dst);
}

START_TEST( sock )
{
    int i;

/* Leave these tests at the beginning. They depend on WSAStartup not having been
 * called, which is done by Init() below. */
    test_WithoutWSAStartup();
    test_WithWSAStartup();

    Init();

    test_inet_ntoa();
    test_inet_pton();
    test_set_getsockopt();
    test_so_reuseaddr();
    test_ip_pktinfo();
    test_extendedSocketOptions();

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        trace ( " **** STARTING TEST %d ****\n", i );
        do_test (  &tests[i] );
        trace ( " **** TEST %d COMPLETE ****\n", i );
    }

    test_UDP();

    test_getservbyname();
    test_WSASocket();
    test_WSADuplicateSocket();
    test_WSAEnumNetworkEvents();

    test_WSAAddressToStringA();
    test_WSAAddressToStringW();

    test_WSAStringToAddressA();
    test_WSAStringToAddressW();

    test_errors();
    test_listen();
    test_select();
    test_accept();
    test_getpeername();
    test_getsockname();
    test_inet_addr();
    test_addr_to_print();
    test_ioctlsocket();
    test_dns();
    test_gethostbyname();
    test_gethostbyname_hack();
    test_gethostname();

    test_WSASendMsg();
    test_WSASendTo();
    test_WSARecv();
    test_WSAPoll();
    test_write_watch();
    test_iocp();

    test_events(0);
    test_events(1);

    test_ipv6only();
    test_TransmitFile();
    test_GetAddrInfoW();
    test_GetAddrInfoExW();
    test_getaddrinfo();

#ifdef __REACTOS__
    if (!winetest_interactive)
    {
        skip("WSPAcceptEx(), WSPConnectEx() and WSPDisconnectEx() are UNIMPLEMENTED on ReactOS\n");
        skip("Skipping tests due to hang. See ROSTESTS-385\n");
    }
    else
    {
#endif
    test_AcceptEx();
    test_ConnectEx();
    test_DisconnectEx();
#ifdef __REACTOS__
    }
#endif

    test_sioRoutingInterfaceQuery();
    test_sioAddressListChange();

    test_WSALookupService();
    test_WSAEnumNameSpaceProvidersA();
    test_WSAEnumNameSpaceProvidersW();

    test_WSAAsyncGetServByPort();
    test_WSAAsyncGetServByName();

    test_completion_port();
    test_address_list_query();

    /* this is an io heavy test, do it at the end so the kernel doesn't start dropping packets */
    test_send();
    test_synchronous_WSAIoctl();

    Exit();
}
