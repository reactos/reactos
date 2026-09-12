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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winsock2.h>
#include <windows.h>
#include <winternl.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <mswsock.h>
#include <mstcpip.h>
#include <stdio.h>
#include "wine/test.h"

#define MAX_CLIENTS 4      /* Max number of clients */
#define FIRST_CHAR 'A'     /* First character in transferred pattern */
#define BIND_SLEEP 10      /* seconds to wait between attempts to bind() */
#define BIND_TRIES 6       /* Number of bind() attempts */
#define TEST_TIMEOUT 30    /* seconds to wait before killing child threads
                              after server initialization, if something hangs */

#define NUM_UDP_PEERS 3    /* Number of UDP sockets to create and test > 1 */

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
static int   (WINAPI *pWSAPoll)(WSAPOLLFD *,ULONG,INT);

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

/**************** Static variables ***************/

static DWORD      tls;              /* Thread local storage index */
static HANDLE     thread[1+MAX_CLIENTS];
static DWORD      thread_id[1+MAX_CLIENTS];
static HANDLE     server_ready;
static HANDLE     client_ready[MAX_CLIENTS];
static int        client_id;
static GUID       WSARecvMsg_GUID = WSAID_WSARECVMSG;
#ifdef __REACTOS__
static BOOL       HasIPV6;
#endif

/**************** General utility functions ***************/

static SOCKET setup_server_socket(struct sockaddr_in *addr, int *len);
static SOCKET setup_connector_socket(const struct sockaddr_in *addr, int len, BOOL nonblock);
static int sync_recv(SOCKET s, void *buffer, int len, DWORD flags);

#if defined(__REACTOS__) && DLL_EXPORT_VERSION < 0x600
/* inet_pton is NT6+ */
int WINAPI inet_pton(int af, const char *src, void *dst) {
    SOCKADDR_STORAGE ss;
    int len = sizeof(ss);
    if (WSAStringToAddressA((LPSTR)src, af, NULL, (LPSOCKADDR)&ss, &len) != 0)
        return 0;
    if (af == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
        memcpy(dst, &sin->sin_addr, sizeof(struct in_addr));
        return 1;
    } else if (af == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
        memcpy(dst, &sin6->sin6_addr, sizeof(struct in6_addr));
        return 1;
    }
    return 0;
}
#endif
static void tcp_socketpair_flags(SOCKET *src, SOCKET *dst, DWORD flags)
{
    SOCKET server = INVALID_SOCKET;
    struct sockaddr_in addr;
    int len, ret;

    *src = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
    ok(*src != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    server = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
    ok(server != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind socket, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr *)&addr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(server, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    ret = connect(*src, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    *dst = accept(server, (struct sockaddr *)&addr, &len);
    ok(*dst != INVALID_SOCKET, "failed to accept socket, error %u\n", WSAGetLastError());

    closesocket(server);
}

static void tcp_socketpair(SOCKET *src, SOCKET *dst)
{
    tcp_socketpair_flags(src, dst, WSA_FLAG_OVERLAPPED);
}

static void WINAPI apc_func(ULONG_PTR apc_count)
{
    ++*(unsigned int *)apc_count;
}

/* Set the linger timeout to zero and close the socket. This will trigger an
 * RST on the connection on Windows as well as on Unix systems. */
static void close_with_rst(SOCKET s)
{
    static const struct linger linger = {.l_onoff = 1};
    int ret;

    SetLastError(0xdeadbeef);
    ret = setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(linger));
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());

    closesocket(s);
}

#define check_poll(a, b) check_poll_(__LINE__, a, POLLRDNORM | POLLRDBAND | POLLWRNORM, b, FALSE)
#define check_poll_todo(a, b) check_poll_(__LINE__, a, POLLRDNORM | POLLRDBAND | POLLWRNORM, b, TRUE)
#define check_poll_mask(a, b, c) check_poll_(__LINE__, a, b, c, FALSE)
#define check_poll_mask_todo(a, b, c) check_poll_(__LINE__, a, b, c, TRUE)
static void check_poll_(int line, SOCKET s, short mask, short expect, BOOL todo)
{
    WSAPOLLFD pollfd;
    int ret;

    pollfd.fd = s;
    pollfd.events = mask;
    pollfd.revents = 0xdead;
    ret = pWSAPoll(&pollfd, 1, 1000);
    ok_(__FILE__, line)(ret == (pollfd.revents ? 1 : 0), "WSAPoll() returned %d\n", ret);
    todo_wine_if (todo) ok_(__FILE__, line)(pollfd.revents == expect, "got wrong events %#x\n", pollfd.revents);
}

static DWORD WINAPI poll_async_thread(void *arg)
{
    WSAPOLLFD *pollfd = arg;
    int ret;

    ret = pWSAPoll(pollfd, 1, 500);
    ok(ret == (pollfd->revents ? 1 : 0), "WSAPoll() returned %d\n", ret);

    return 0;
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
    wsa_ok ( n, 0 <=, "do_synchronous_send (%lx): error %d\n" );
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
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%lx): error %d:\n" );
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
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%lx): error %d:\n" );
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
    wsa_ok ( closesocket ( mem->s ), 0 ==, "closesocket error (%lx): %d\n" );
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

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, TRUE ), 0 ==, "simple_server (%lx): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "simple_server (%lx): listen failed: %d\n");

    SetEvent ( server_ready ); /* notify clients */

    for ( i = 0; i < min ( gen->n_clients, MAX_CLIENTS ); i++ )
    {
        /* accept a single connection */
        tmp = sizeof ( mem->sock[0].peer );
        mem->sock[0].s = accept ( mem->s, (struct sockaddr*) &mem->sock[0].peer, &tmp );
        wsa_ok ( mem->sock[0].s, INVALID_SOCKET !=, "simple_server (%lx): accept failed: %d\n" );

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
        wsa_ok ( closesocket ( mem->sock[0].s ),  0 ==, "simple_server (%lx): closesocket error: %d\n" );
        mem->sock[0].s = INVALID_SOCKET;
    }

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

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, TRUE ), 0 ==, "oob_server (%lx): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "oob_server (%lx): listen failed: %d\n");

    SetEvent ( server_ready ); /* notify clients */

    /* accept a single connection */
    tmp = sizeof ( mem->sock[0].peer );
    mem->sock[0].s = accept ( mem->s, (struct sockaddr*) &mem->sock[0].peer, &tmp );
    wsa_ok ( mem->sock[0].s, INVALID_SOCKET !=, "oob_server (%lx): accept failed: %d\n" );

    ok ( mem->sock[0].peer.sin_addr.s_addr == inet_addr ( gen->inet_addr ),
         "oob_server (%x): strange peer address\n", id );

    /* check initial atmark state */
    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    ok ( atmark == 1, "oob_server (%x): unexpectedly at the OOB mark: %li\n", id, atmark );

    /* Receive normal data */
    n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );
    ok ( n_recvd == n_expected,
         "oob_server (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
    pos = test_buffer ( mem->sock[0].buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "oob_server (%x): test pattern error: %d\n", id, pos);

    /* check atmark state */
    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    ok ( atmark == 1, "oob_server (%x): unexpectedly at the OOB mark: %li\n", id, atmark );

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

    /* Receive the rest of the out-of-band data and check atmark state */
    do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, 0, par->buflen );

    ioctlsocket ( mem->sock[0].s, SIOCATMARK, &atmark );
    todo_wine ok ( atmark == 0, "oob_server (%x): not at the OOB mark: %li\n", id, atmark );

    /* cleanup */
    wsa_ok ( closesocket ( mem->sock[0].s ),  0 ==, "oob_server (%lx): closesocket error: %d\n" );
    mem->sock[0].s = INVALID_SOCKET;

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

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, FALSE ), 0 ==, "select_server (%lx): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "select_server (%lx): listen failed: %d\n");

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
            "select_server (%lx): select() failed: %d\n" );

        /* check for incoming requests */
        if ( FD_ISSET ( mem->s, &fds_recv ) ) {
            n_set += 1;

            /* accept a single connection */
            tmp = sizeof ( mem->sock[n_connections].peer );
            mem->sock[n_connections].s = accept ( mem->s, (struct sockaddr*) &mem->sock[n_connections].peer, &tmp );
            wsa_ok ( mem->sock[n_connections].s, INVALID_SOCKET !=, "select_server (%lx): accept() failed: %d\n" );

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
        wsa_ok ( closesocket ( mem->sock[i].s ),  0 ==, "select_server (%lx): closesocket error: %d\n" );
        mem->sock[i].s = INVALID_SOCKET;
    }

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
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "simple_client (%lx): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "simple_client (%x): failed to set blocking mode\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%lx): shutdown failed: %d\n" );

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recv ( mem->s, mem->recv_buf, n_expected, 0, par->buflen );
    ok ( n_recvd == n_expected,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );

    /* check data */
    pos = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( pos == -1, "simple_client (%x): test pattern error: %d\n", id, pos);

    /* cleanup */
    read_zero_bytes ( mem->s );
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
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "oob_client (%lx): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "oob_client (%x): failed to set blocking mode\n", id );

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
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%lx): shutdown failed: %d\n" );

    /* cleanup */
    read_zero_bytes ( mem->s );
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
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "simple_client (%lx): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "simple_client (%x): failed to set blocking mode\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, 0, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%lx): shutdown failed: %d\n" );

    /* this shouldn't change, since lpFrom, is not updated on
       connection oriented sockets - exposed by bug 11640
    */
    ((struct sockaddr_in*)&test)->sin_addr.s_addr = inet_addr("0.0.0.0");

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recvfrom ( mem->s, mem->recv_buf, n_expected, 0, &test, &fromLen,
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

    client_start ( par );

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
            wsa_ok ( n, 0 <=, "event_client (%lx): recv error: %d\n" );

            while ( n >= 0 ) {
                recv_p += n;
                if ( recv_p == recv_last )
                {
                    mask &= ~FD_READ;
                    WSAEventSelect ( mem->s, event, mask );
                    break;
                }
                n = recv ( mem->s, recv_p, min ( recv_last - recv_p, par->buflen ), 0 );
                ok(n >= 0 || WSAGetLastError() == WSAEWOULDBLOCK,
                        "event_client (%x): got error %u\n", id, WSAGetLastError());
                
            }
        }   
        if ( wsa_events.lNetworkEvents & FD_CLOSE )
        {
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
    client_stop ();
}

/* Tests for WSAStartup */
static void test_WithoutWSAStartup(void)
{
    DWORD err;

    WSASetLastError(0xdeadbeef);
    ok(WSASocketA(0, 0, 0, NULL, 0, 0) == INVALID_SOCKET, "WSASocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSANOTINITIALISED, "Expected 10093, received %ld\n", err);

    WSASetLastError(0xdeadbeef);
    ok(gethostbyname("localhost") == NULL, "gethostbyname() succeeded unexpectedly\n");
    err = WSAGetLastError();
    ok(err == WSANOTINITIALISED, "Expected 10093, received %ld\n", err);
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
    for (socks = 0; socks < ARRAY_SIZE(pairs); socks++)
    {
        WSAPROTOCOL_INFOA info;
        tcp_socketpair(&pairs[socks].src, &pairs[socks].dst);

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
    ok(error == WSAENOTSOCK, "expected 10038, got %ld\n", error);

    SetLastError(0xdeadbeef);
    res = send(pairs[0].dst, "TEST", 4, 0);
    error = WSAGetLastError();
    ok(res == SOCKET_ERROR, "send should have failed\n");
    ok(error == WSAENOTSOCK, "expected 10038, got %ld\n", error);

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
            if (res == SOCKET_ERROR)
                ok(error == WSAENOTSOCK, "Test[%d]: expected 10038, got %ld\n", i, error);
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
            "WSACleanup returned %d WSAGetLastError is %ld\n", res, error);
}

/**************** Main program utility functions ***************/

static void Init (void)
{
    WORD ver = MAKEWORD (2, 2);
    WSADATA data;
    HMODULE hws2_32 = GetModuleHandleA("ws2_32.dll"), ntdll;

    pWSAPoll = (void *)GetProcAddress(hws2_32, "WSAPoll");

    ntdll = LoadLibraryA("ntdll.dll");
    if (ntdll)
        pNtClose = (void *)GetProcAddress(ntdll, "NtClose");

    ok ( WSAStartup ( ver, &data ) == 0, "WSAStartup failed\n" );
    tls = TlsAlloc();
#ifdef __REACTOS__
    /* Determine if we have IPV6 support */
    SOCKET ipv6_test;
    ipv6_test = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    HasIPV6 = !(ipv6_test == INVALID_SOCKET);
    if (!HasIPV6)
        skip("IPV6 support not detected. Several tests will be skipped.\n");
    else
        closesocket(ipv6_test);
#endif
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
    ok(!wait, "wait failed, error %lu\n", wait);

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
    static struct
    {
        int af;
        int type;
        int level;
        int optname;
        BOOL accepts_short_len;
        unsigned int sizes[3];
        DWORD values[3];
        BOOL accepts_large_value;
        BOOL bool_value;
        BOOL allow_noprotoopt; /* for old windows only, must work on wine */
#ifdef __REACTOS__
        DWORD expected_value;
        BOOL skip_test;
#endif
    }
    test_optsize[] =
    {
        {AF_INET, SOCK_DGRAM, SOL_SOCKET, SO_BROADCAST,  TRUE, {1, 1, 4}, {0, 0xdead0001, 0}, TRUE, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_DONTLINGER, TRUE, {1, 1, 4}, {0, 0xdead0001, 0}, TRUE, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_LINGER, FALSE, {1, 2, 4}, {0xdeadbe00, 0xdead0000}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_OOBINLINE, TRUE, {1, 1, 4}, {0, 0xdead0001, 0}, TRUE, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_RCVBUF, FALSE, {1, 2, 4}, {0xdeadbe00, 0xdead0000}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_KEEPALIVE, TRUE, {1, 1, 1}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_DONTROUTE, TRUE, {1, 1, 1}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_RCVTIMEO, FALSE, {1, 2, 4}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_REUSEADDR, TRUE, {1, 1, 4}, {0, 0xdead0001, 0}, TRUE, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, TRUE, {1, 1, 4}, {0, 0xdead0001, 0}, TRUE, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_SNDBUF, FALSE, {1, 2, 4}, {0xdeadbe00, 0xdead0000}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_SNDTIMEO, FALSE, {1, 2, 4}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, SOL_SOCKET, SO_OPENTYPE, FALSE, {1, 2, 4}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, IPPROTO_TCP, TCP_NODELAY, TRUE, {1, 1, 1}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, IPPROTO_TCP, TCP_KEEPALIVE, FALSE, {0, 0, 4}, {0}, TRUE},
        {AF_INET, SOCK_STREAM, IPPROTO_TCP, TCP_KEEPCNT, FALSE, {0, 0, 4}, {0}, FALSE, FALSE, TRUE}, /* win10+ */
        {AF_INET, SOCK_STREAM, IPPROTO_TCP, TCP_KEEPINTVL, FALSE, {0, 0, 4}, {0}, TRUE, FALSE, TRUE}, /* win10+ */
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_MULTICAST_LOOP, TRUE, {1, 1, 4}, {0}, TRUE, TRUE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_MULTICAST_TTL, TRUE, {1, 1, 4}, {0}, FALSE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_PKTINFO, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_RECVTOS, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_RECVTTL, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_TOS, TRUE, {1, 1, 4}, {0}, FALSE},
        {AF_INET, SOCK_DGRAM, IPPROTO_IP, IP_TTL, TRUE, {1, 1, 4}, {0}, FALSE},
        {AF_INET6, SOCK_STREAM, IPPROTO_IPV6, IPV6_DONTFRAG, TRUE, {1, 1, 4}, {0}, TRUE, TRUE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_HOPLIMIT, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, TRUE, {1, 1, 4}, {0}, FALSE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, TRUE, {1, 1, 4}, {0}, TRUE, TRUE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_PKTINFO, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_RECVTCLASS, FALSE, {0, 0, 4}, {0}, TRUE, TRUE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_UNICAST_HOPS, TRUE, {1, 1, 4}, {0}, FALSE},
        {AF_INET6, SOCK_DGRAM, IPPROTO_IPV6, IPV6_V6ONLY, TRUE, {1, 1, 1}, {0}, TRUE},
    };
    SOCKET s, s2, src, dst;
    int i, j, err, lasterr;
    int timeout;
    LINGER lingval;
    int size;
    WSAPROTOCOL_INFOA infoA;
    WSAPROTOCOL_INFOW infoW;
    char providername[WSAPROTOCOL_LEN + 1];
    DWORD expected_last_error, expected_value;
    int expected_err, expected_size;
    DWORD value, save_value;
    UINT64 value64;
    char buffer[4096];

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

#ifdef __REACTOS__
    /* WS03 needs some patches for test_optsize test */
    if (GetNTVersion() <= _WIN32_WINNT_WS03 && !is_reactos()) {
        test_optsize[5].sizes[2] = 4;
        test_optsize[5].values[1] = 0xdead0001;
        test_optsize[5].expected_value = 0x1;
        test_optsize[6].sizes[2] = 4;
        test_optsize[6].values[1] = 0xdead0001;
        test_optsize[6].expected_value = 0x1;
        test_optsize[13].skip_test = TRUE;
        test_optsize[14].skip_test = TRUE;
        test_optsize[15].skip_test = TRUE;
        test_optsize[16].skip_test = TRUE;
        test_optsize[17].skip_test = TRUE;
        test_optsize[18].skip_test = TRUE;
        test_optsize[19].skip_test = TRUE;
        test_optsize[20].skip_test = TRUE;
        test_optsize[21].skip_test = TRUE;
        test_optsize[22].skip_test = TRUE;
        test_optsize[23].skip_test = TRUE;
    }
#endif
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
    ok( value == 4096, "expected 4096, got %lu\n", value );

    /* SO_RCVBUF */
    value = 4096;
    size = sizeof(value);
    err = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, size);
    ok( !err, "setsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    value = 0xdeadbeef;
    err = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, &size);
    ok( !err, "getsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    ok( value == 4096, "expected 4096, got %lu\n", value );

    value = 0;
    size = sizeof(value);
    err = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, size);
    ok( !err, "setsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    value = 0xdeadbeef;
    err = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&value, &size);
    ok( !err, "getsockopt(SO_RCVBUF) failed error: %u\n", WSAGetLastError() );
    ok( value == 0, "expected 0, got %lu\n", value );

    /* Test non-blocking receive with too short SO_RCVBUF. */
    tcp_socketpair(&src, &dst);
    set_blocking(src, FALSE);
    set_blocking(dst, FALSE);

    value = 0;
    size = sizeof(value);
    err = setsockopt(src, SOL_SOCKET, SO_SNDBUF, (char *)&value, size);
    ok( !err, "got %d, error %u.\n", err, WSAGetLastError() );

    value = 0xdeadbeef;
    err = getsockopt(dst, SOL_SOCKET, SO_RCVBUF, (char *)&value, &size);
    ok( !err, "got %d, error %u.\n", err, WSAGetLastError() );
    if (value >= sizeof(buffer) * 3)
    {
        value = 1024;
        size = sizeof(value);
        err = setsockopt(dst, SOL_SOCKET, SO_RCVBUF, (char *)&value, size);
        ok( !err, "got %d, error %u.\n", err, WSAGetLastError() );
        value = 0xdeadbeef;
        err = getsockopt(dst, SOL_SOCKET, SO_RCVBUF, (char *)&value, &size);
        ok( !err, "got %d, error %u.\n", err, WSAGetLastError() );
        ok( value == 1024, "expected 0, got %lu\n", value );

        err = send(src, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d\n", err);
        err = send(src, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d\n", err);
        err = send(src, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d\n", err);

        err = sync_recv(dst, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d, error %u\n", err, WSAGetLastError());
        err = sync_recv(dst, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d, error %u\n", err, WSAGetLastError());
        err = sync_recv(dst, buffer, sizeof(buffer), 0);
        ok(err == sizeof(buffer), "got %d, error %u\n", err, WSAGetLastError());
    }
    else
    {
        skip("Default SO_RCVBUF %lu is too small, skipping test.\n", value);
    }

    closesocket(src);
    closesocket(dst);

    /* SO_LINGER */
    for( i = 0; i < ARRAY_SIZE(linger_testvals);i++) {
        size =  sizeof(lingval);
        lingval = linger_testvals[i];
        err = setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&lingval, size);
        ok(!err, "Test %u: failed to set SO_LINGER, error %u\n", i, WSAGetLastError());
        err = getsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&lingval, &size);
        ok(!err, "Test %u: failed to get SO_LINGER, error %u\n", i, WSAGetLastError());
        ok(!lingval.l_onoff == !linger_testvals[i].l_onoff, "Test %u: expected %d, got %d\n",
                i, linger_testvals[i].l_onoff, lingval.l_onoff);
        if (lingval.l_onoff)
            ok(lingval.l_linger == linger_testvals[i].l_linger, "Test %u: expected %d, got %d\n",
                    i, linger_testvals[i].l_linger, lingval.l_linger);
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

    /* TCP_NODELAY: optlen doesn't matter on windows, it should work with any positive value */
    size = sizeof(value);

    value = 1;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, 1);
    ok (!err, "setsockopt TCP_NODELAY failed with optlen == 1\n");
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
    ok(value == 1, "TCP_NODELAY should be 1\n");
    value = 0;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));
    ok(!err, "Failed to reset TCP_NODELAY to 0\n");

    value = 1;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, 4);
    ok (!err, "setsockopt TCP_NODELAY failed with optlen == 4\n");
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
    ok(value == 1, "TCP_NODELAY should be 1\n");
    value = 0;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));
    ok(!err, "Failed to reset TCP_NODELAY to 0\n");

    value = 1;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, 42);
    ok (!err, "setsockopt TCP_NODELAY failed with optlen == 42\n");
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
    ok(value == 1, "TCP_NODELAY should be 1\n");
    value = 0;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));
    ok(!err, "Failed to reset TCP_NODELAY to 0\n");

    value = 1;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, 0);
    ok(err == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n", err, WSAGetLastError());
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
    ok(!value, "TCP_NODELAY should be 0\n");

    value = 1;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, -1);
    /* On win 10 pro, this sets the error to WSAENOBUFS instead of WSAEFAULT */
#ifdef __REACTOS__
    ok((err == SOCKET_ERROR && (WSAGetLastError() == WSAEFAULT || WSAGetLastError() == WSAENOBUFS)) || broken(err == 0 && WSAGetLastError() == 0) /* WS03 */,
       "got %d with %d (expected SOCKET_ERROR with either WSAEFAULT or WSAENOBUFS)\n", err, WSAGetLastError());
#else
    ok(err == SOCKET_ERROR && (WSAGetLastError() == WSAEFAULT || WSAGetLastError() == WSAENOBUFS),
       "got %d with %d (expected SOCKET_ERROR with either WSAEFAULT or WSAENOBUFS)\n", err, WSAGetLastError());
#endif
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
#ifdef __REACTOS__
    ok(!value || broken(value == 1) /* WS03 */, "TCP_NODELAY should be 0 or 1, got %ld\n", value);
#else
    ok(!value, "TCP_NODELAY should be 0\n");
#endif

    value = 0x100;
    err = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, 4);
    ok (!err, "setsockopt TCP_NODELAY failed with optlen == 4 and optvalue = 0x100\n");
    value = 0xff;
    err = getsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size);
    ok(!err, "getsockopt TCP_NODELAY failed\n");
#ifdef __REACTOS__
    ok(!value || broken(value == 1) /* WS03 */, "TCP_NODELAY should be 0 or 1, got %ld\n", value);
#else
    ok(!value, "TCP_NODELAY should be 0\n");
#endif

    size = sizeof(DWORD);
    value = 3600;
    err = setsockopt(s, IPPROTO_TCP, TCP_KEEPALIVE, (char*)&value, 4);
#ifdef __REACTOS__
    if (err == -1 && WSAGetLastError() == WSAENOPROTOOPT) {
        skip("No support for TCP_KEEPALIVE on this operating system.\n");
    } else {
#endif
    ok(!err, "setsockopt TCP_KEEPALIVE failed\n");
    value = 0;
    err = getsockopt(s, IPPROTO_TCP, TCP_KEEPALIVE, (char*)&value, &size);
    ok(!err, "getsockopt TCP_KEEPALIVE failed\n");
    ok(value == 3600, "TCP_KEEPALIVE should be 3600, is %ld\n", value);
#ifdef __REACTOS__
    }
#endif

    /* TCP_KEEPCNT and TCP_KEEPINTVL are supported on win10 and later */
    value = 5;
    err = setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, (char*)&value, 4);
    ok(!err || broken(WSAGetLastError() == WSAENOPROTOOPT),
        "setsockopt TCP_KEEPCNT failed: %d\n", WSAGetLastError());

#ifdef __REACTOS__
    /* TCP_KEEPCNT (16) appears to map to something else on WS03 and succeed above. It's not valid for the tests below though. */
    if (!err && GetNTVersion() >= _WIN32_WINNT_VISTA)
#else
    if (!err)
#endif
    {
        value = 0;
        err = getsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, (char*)&value, &size);
        ok(!err, "getsockopt TCP_KEEPCNT failed\n");
        ok(value == 5, "TCP_KEEPCNT should be 5, is %ld\n", value);

        err = setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&value, 4);
        ok(!err, "setsockopt TCP_KEEPINTVL failed\n");
        value = 0;
        err = getsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&value, &size);
        ok(!err, "getsockopt TCP_KEEPINTVL failed\n");
        ok(value == 5, "TCP_KEEPINTVL should be 5, is %ld\n", value);
    }

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
    ok( !err && !WSAGetLastError(),
        "got %d with %d (expected 0 with 0)\n",
        err, WSAGetLastError());

    SetLastError(0xdeadbeef);
    i = 4321;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
    ok( !err && !WSAGetLastError(),
        "got %d with %d (expected 0 with 0)\n",
        err, WSAGetLastError());
    todo_wine
    ok (i == 1234, "got %d (expected 1234)\n", i);

    /* Test invalid optlen */
    SetLastError(0xdeadbeef);
    size = 1;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
    ok( (err == SOCKET_ERROR) && (WSAGetLastError() == WSAEFAULT),
        "got %d with %d (expected SOCKET_ERROR with WSAEFAULT)\n",
        err, WSAGetLastError());

    closesocket(s);

    /* Test option length. */
    for (i = 0; i < ARRAY_SIZE(test_optsize); ++i)
    {
#ifdef __REACTOS__
        if (test_optsize[i].skip_test || (!HasIPV6 && test_optsize[i].af == AF_INET6))
            continue;
#endif
        winetest_push_context("i %u, level %d, optname %d",
                i, test_optsize[i].level, test_optsize[i].optname);

        s2 = socket( test_optsize[i].af, test_optsize[i].type, 0 );
        ok(s2 != INVALID_SOCKET, "socket() failed error %d\n", WSAGetLastError());

        size = sizeof(save_value);
        err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&save_value, &size);
        ok(!err || broken(test_optsize[i].allow_noprotoopt && WSAGetLastError() == WSAENOPROTOOPT),
            "Unexpected getsockopt result %d.\n", err);

        if (err)
        {
            closesocket(s2);
            winetest_pop_context();
            continue;
        }

        value64 = 0xffffffff00000001;
        err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char *)&value64, sizeof(value64));
        ok(!err, "Unexpected setsockopt result %d.\n", err);
        ok(!WSAGetLastError(), "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

        size = sizeof(value64);
        err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value64, &size);
        ok(!err, "Unexpected getsockopt result %d.\n", err);
        ok(size == test_optsize[i].sizes[2], "Got unexpected size %d.\n", size);
        /* The behaviour regarding filling the high dword is different between options without the obvious
         * pattern, it is either left untouched (more often) or zeroed. Wine doesn't touch the high dword. */

        if (test_optsize[i].sizes[2] == 1 || test_optsize[i].level != SOL_SOCKET)
        {
            expected_err = -1;
            expected_last_error = WSAENOBUFS;
        }
        else
        {
            expected_err = 0;
            expected_last_error = 0;
        }

        value = 1;
        err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char *)&value, -1);
        ok(err == expected_err, "Unexpected setsockopt result %d.\n", err);
        /* Broken between Win7 and Win10 21H1. */
        ok(WSAGetLastError() == expected_last_error || broken(expected_last_error && WSAGetLastError() == WSAEFAULT),
                "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

        size = -1;
        value = 0xdeadbeef;
        err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, &size);
        if (test_optsize[i].optname == SO_OPENTYPE)
        {
            ok(!err, "Unexpected getsockopt result %d.\n", err);
            ok(!WSAGetLastError(), "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());
        }
        else
        {
            ok(err == -1, "Unexpected getsockopt result %d.\n", err);
            ok(WSAGetLastError() == WSAEFAULT, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());
        }
        ok(size == (test_optsize[i].optname == SO_OPENTYPE ? 4 : -1), "Got unexpected size %d.\n", size);

        if (test_optsize[i].level == SOL_SOCKET && test_optsize[i].bool_value)
        {
            expected_err = 0;
            expected_last_error = 0;
        }
        else
        {
            expected_err = -1;
            expected_last_error = WSAEFAULT;
        }
        value = 1;
        SetLastError(0xdeadbeef);
        err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, 0);
        ok(err == expected_err, "Unexpected setsockopt result %d.\n", err);
        ok(WSAGetLastError() == expected_last_error, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

        size = 0;
        err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, &size);
        ok(err == -1, "Unexpected getsockopt result %d.\n", err);
        ok(WSAGetLastError() == WSAEFAULT, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

        expected_size = test_optsize[i].sizes[2];
        if (expected_size == 1)
            expected_value = 0xdeadbe00;
        else
            expected_value = test_optsize[i].bool_value ? 0x1 : 0x100;
        if (test_optsize[i].accepts_large_value)
        {
            expected_err = 0;
            expected_last_error = 0;
        }
        else
        {
            expected_err = -1;
            expected_last_error = WSAEINVAL;
        }
#ifdef __REACTOS__
        if (test_optsize[i].expected_value)
            expected_value = test_optsize[i].expected_value;
#endif

        value = 0x100;
        SetLastError(0xdeadbeef);
        err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, 4);
        ok(err == expected_err, "Unexpected setsockopt result %d.\n", err);
        ok(WSAGetLastError() == expected_last_error, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

        if (test_optsize[i].accepts_large_value)
        {
            value = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            size = 4;
            err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, &size);
            ok(err == expected_err, "Unexpected getsockopt result %d.\n", err);
            ok(WSAGetLastError() == expected_last_error, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());
            todo_wine_if(test_optsize[i].optname == SO_DONTROUTE || test_optsize[i].optname == SO_LINGER)
            ok(value == expected_value, "Got unexpected value %#lx, expected %#lx.\n", value, expected_value);
            ok(size == expected_size, "Got unexpected size %u, expected %u.\n", size, expected_size);
        }

        winetest_pop_context();

        for (j = 0; j < ARRAY_SIZE(test_optsize[i].sizes); ++j)
        {
            size = 1 << j;
            winetest_push_context("i %u, level %d, optname %d, len %u",
                    i, test_optsize[i].level, test_optsize[i].optname, size);

            value = 1;
            if (test_optsize[i].values[j])
                expected_value = test_optsize[i].values[j];
            else
                expected_value = 0xdeadbeef;

            if (test_optsize[i].accepts_short_len || size == 4)
            {
                expected_err = 0;
                expected_last_error = 0;
                expected_size = test_optsize[i].sizes[j];

                if (!test_optsize[i].values[j])
                    memcpy(&expected_value, &value, expected_size);
            }
            else
            {
                expected_err = -1;
                expected_last_error = WSAEFAULT;
                expected_size = test_optsize[i].sizes[j];
            }

            SetLastError(0xdeadbeef);
            err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, size);
            ok(err == expected_err, "Unexpected setsockopt result %d.\n", err);
            ok(WSAGetLastError() == expected_last_error, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());

            value = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            err = getsockopt(s2, test_optsize[i].level, test_optsize[i].optname, (char*)&value, &size);
            ok(err == expected_err, "Unexpected getsockopt result %d.\n", err);
            ok(WSAGetLastError() == expected_last_error, "Unexpected WSAGetLastError() %u.\n", WSAGetLastError());
            ok(value == expected_value, "Got unexpected value %#lx, expected %#lx.\n", value, expected_value);
            ok(size == expected_size, "Got unexpected size %d, expected %d.\n", size, expected_size);

            winetest_pop_context();
        }

        err = setsockopt(s2, test_optsize[i].level, test_optsize[i].optname,
                (char*)&save_value, sizeof(save_value));
        ok(!err, "Unexpected getsockopt result %d.\n", err);
        closesocket(s2);
    }

    /* Test with the closed socket */
    SetLastError(0xdeadbeef);
    size = sizeof(i);
    i = 1234;
    err = getsockopt(s, SOL_SOCKET, SO_ERROR, (char *) &i, &size);
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
    for (i = 0; i < ARRAY_SIZE(prottest); i++)
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

        ok(infoA.szProtocol[0], "WSAPROTOCOL_INFOA was not filled\n");
        ok(infoW.szProtocol[0], "WSAPROTOCOL_INFOW was not filled\n");

        WideCharToMultiByte(CP_ACP, 0, infoW.szProtocol, -1,
                            providername, sizeof(providername), NULL, NULL);
        ok(!strcmp(infoA.szProtocol,providername),
           "different provider names '%s' != '%s'\n", infoA.szProtocol, providername);

        ok(!memcmp(&infoA, &infoW, FIELD_OFFSET(WSAPROTOCOL_INFOA, szProtocol)),
           "SO_PROTOCOL_INFO[A/W] comparison failed\n");

        /* Remove IF when WSAEnumProtocols support IPV6 data */
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
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %ld\n", GetLastError());
            k = 99;
            SetLastError(0xdeadbeef);
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            ok(err == -1, "Expected -1, got %d\n", err);
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %ld\n", GetLastError());
            ok(k == 99, "Expected 99, got %d\n", k);

            size = sizeof(k);
            k = 0;
            SetLastError(0xdeadbeef);
            err = setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, size);
            ok(err == -1, "Expected -1, got %d\n", err);
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %ld\n", GetLastError());
            k = 99;
            SetLastError(0xdeadbeef);
            err = getsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *) &k, &size);
            ok(err == -1, "Expected -1, got %d\n", err);
            ok(GetLastError() == WSAEINVAL, "Expected 10022, got %ld\n", GetLastError());
            ok(k == 99, "Expected 99, got %d\n", k);
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
                ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %ld\n", GetLastError());
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
                ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %ld\n", GetLastError());
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
        ok(GetLastError() == WSAEFAULT, "Expected 10014, got %ld\n", GetLastError());

        /* At least for IPv4 the size is exactly 56 bytes */
        size = sizeof(*csinfoA.cs.LocalAddr.lpSockaddr) * 2 + sizeof(csinfoA.cs);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(!err, "Expected 0, got %d\n", err);
        size--;
        SetLastError(0xdeadbeef);
        err = getsockopt(s, SOL_SOCKET, SO_BSP_STATE, (char *) &csinfoA, &size);
        ok(err, "Expected non-zero\n");
        ok(GetLastError() == WSAEFAULT, "Expected 10014, got %ld\n", GetLastError());
    }
    else
        ok(GetLastError() == WSAENOPROTOOPT, "Expected 10042, got %ld\n", GetLastError());

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
        ok(s != INVALID_SOCKET, "socket failed with error %ld\n", GetLastError());

        size = sizeof(value);
        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 0, "Expected 0, got %ld\n", value);

        size = sizeof(value);
        value = 1;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());

        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 1, "Expected 1, got %ld\n", value);

        size = sizeof(value);
        value = 0xdead;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());

        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 1, "Expected 1, got %ld\n", value);

        closesocket(s);

        s = socket(family, SOCK_STREAM, 0);
        ok(s != INVALID_SOCKET, "socket failed with error %ld\n", GetLastError());

        size = sizeof(value);
        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 1 || broken(value == 0) /* < vista */, "Expected 1, got %ld\n", value);

        size = sizeof(value);
        value = 0;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());

        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 0, "Expected 0, got %ld\n", value);

        closesocket(s);

        s = socket(family, SOCK_RAW, 0);
        if (s == INVALID_SOCKET)
        {
            if (WSAGetLastError() == WSAEACCES) skip("SOCK_RAW is not available\n");
            else if (i) skip("IPv6 is not supported\n");
            break;
        }
        ok(s != INVALID_SOCKET, "socket failed with error %ld\n", GetLastError());

        size = sizeof(value);
        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 0, "Expected 0, got %ld\n", value);

        size = sizeof(value);
        value = 1;
        err = setsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());

        value = 0xdead;
        err = getsockopt(s, level, IP_DONTFRAGMENT, (char *) &value, &size);
        ok(!err, "Expected 0, got %d with error %ld\n", err, GetLastError());
        ok(value == 1, "Expected 1, got %ld\n", value);

        closesocket(s);
    }
}

static void test_reuseaddr(void)
{
    static struct sockaddr_in6 saddr_in6_any, saddr_in6_loopback;
    static struct sockaddr_in6 saddr_in6_any_v4mapped, saddr_in6_loopback_v4mapped;
    static struct sockaddr_in saddr_in_any, saddr_in_loopback;

    static const struct
    {
        int domain;
        struct sockaddr *addr_any;
        struct sockaddr *addr_loopback;
        socklen_t addrlen;
    }
    tests[] =
    {
        { AF_INET, (struct sockaddr *)&saddr_in_any, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_any) },
        { AF_INET6, (struct sockaddr *)&saddr_in6_any, (struct sockaddr *)&saddr_in6_loopback, sizeof(saddr_in6_any) },
    };
    static const struct
    {
        struct
        {
            int domain;
            struct sockaddr *addr;
            socklen_t addrlen;
            BOOL exclusive;
        }
        s[2];
        int error;
    }
    tests_exclusive[] =
    {
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, }},
            WSAEACCES,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_any, sizeof(saddr_in6_any), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, }},
            WSAEACCES,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, }},
            NOERROR,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, }},
            WSAEACCES,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, }},
            NOERROR,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, },
            { AF_INET6, (struct sockaddr *)&saddr_in6_any, sizeof(saddr_in6_any), TRUE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_loopback, sizeof(saddr_in6_loopback), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, }},
            NOERROR,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_loopback, sizeof(saddr_in6_loopback), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, }},
            NOERROR,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_loopback_v4mapped, sizeof(saddr_in6_loopback_v4mapped), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_any, sizeof(saddr_in6_any), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, }},
            WSAEACCES,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_any, sizeof(saddr_in6_any), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, }},
            NOERROR,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), TRUE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_any, sizeof(saddr_in6_any), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, }},
            NOERROR,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_any_v4mapped, sizeof(saddr_in6_any_v4mapped), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_any, sizeof(saddr_in_any), FALSE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_loopback_v4mapped, sizeof(saddr_in6_loopback_v4mapped), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, },
            { AF_INET6, (struct sockaddr *)&saddr_in6_loopback_v4mapped, sizeof(saddr_in6_loopback_v4mapped), FALSE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET6, (struct sockaddr *)&saddr_in6_loopback, sizeof(saddr_in6_loopback), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, }},
            NOERROR,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, }},
            WSAEADDRINUSE,
        },
        {
            {{ AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), FALSE, },
            { AF_INET, (struct sockaddr *)&saddr_in_loopback, sizeof(saddr_in_loopback), TRUE, }},
            WSAEADDRINUSE,
        },
    };

    unsigned int rc, reuse, value;
    struct sockaddr_storage saddr;
    SOCKET s1, s2, s3, s4, s5, s6;
    unsigned int i, j;
    int size;

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03 && !is_reactos()) {
        skip("These tests hang on Windows Server 2003.\n");
        return;
    }
#endif
    saddr_in_any.sin_family = AF_INET;
    saddr_in_any.sin_port = htons(SERVERPORT + 1);
    saddr_in_any.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr_in_loopback = saddr_in_any;
    saddr_in_loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    saddr_in6_any.sin6_family = AF_INET6;
    saddr_in6_any.sin6_port = htons(SERVERPORT + 1);
    memset( &saddr_in6_any.sin6_addr, 0, sizeof(saddr_in6_any.sin6_addr) );
    saddr_in6_loopback = saddr_in6_any;
    inet_pton(AF_INET6, "::1", &saddr_in6_loopback.sin6_addr);

    saddr_in6_loopback_v4mapped = saddr_in6_any;
    rc = inet_pton(AF_INET6, "::ffff:127.0.0.1", &saddr_in6_loopback_v4mapped.sin6_addr);
    ok(rc, "got error %d.\n", WSAGetLastError());

    saddr_in6_any_v4mapped = saddr_in6_any;
    rc = inet_pton(AF_INET6, "::ffff:0.0.0.0", &saddr_in6_any_v4mapped.sin6_addr);
    ok(rc, "got error %d.\n", WSAGetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("test %u", i);

        /* Test with SO_REUSEADDR on second socket only. */
        s1=socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc || (tests[i].domain == AF_INET6 && WSAGetLastError() == WSAEADDRNOTAVAIL), "got error %d.\n", WSAGetLastError());
        if (tests[i].domain == AF_INET6 && WSAGetLastError() == WSAEADDRNOTAVAIL)
        {
            skip("IPv6 not supported, skipping test\n");
            closesocket(s1);
            winetest_pop_context();
            continue;
        }

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        reuse = 1;
        rc = setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_loopback, tests[i].addrlen);
        ok(rc == SOCKET_ERROR, "got rc %d.\n", rc);
        ok(WSAGetLastError() == WSAEACCES, "got error %d.\n", WSAGetLastError());

        closesocket(s1);
        closesocket(s2);

        /* Test with SO_REUSEADDR on both sockets. */
        s1 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        reuse = 1;
        rc = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        reuse = 0x1234;
        size = sizeof(reuse);
        rc = getsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, &size);
        ok(!rc && !reuse,"got rc %d, reuse %d.\n", rc, reuse);

        rc = bind(s2, tests[i].addr_loopback, tests[i].addrlen);
        ok(rc == SOCKET_ERROR, "got rc %d, error %d.\n", rc, WSAGetLastError());

        reuse = 1;
        rc = setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s3 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s3 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        /* Test if we can really connect to one of them. */
        rc = listen(s1, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());
        rc = listen(s2, 1);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());
        rc = connect(s3, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        /* The connection is delivered to the first socket. */
        size = tests[i].addrlen;
        s4 = accept(s1, (struct sockaddr *)&saddr, &size);
        ok(s4 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s1);
        closesocket(s2);
        closesocket(s3);
        closesocket(s4);

        /* Test binding and listening on any addr together with loopback, any addr first. */
        s1 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_any, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = listen(s1, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_loopback, tests[i].addrlen);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = listen(s2, 1);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());

        s3 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s3 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = connect(s3, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        size = tests[i].addrlen;
        s4 = accept(s2, (struct sockaddr *)&saddr, &size);
        todo_wine ok(s4 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s1);
        closesocket(s2);
        closesocket(s3);
        closesocket(s4);

        /* Test binding and listening on any addr together with loopback, loopback addr first. */

        s1 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = listen(s1, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_any, tests[i].addrlen);
        todo_wine ok(!rc, "got rc %d, error %d.\n", rc, WSAGetLastError());

        rc = listen(s2, 1);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());

        s3 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s3 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = connect(s3, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());
        size = tests[i].addrlen;
        s4 = accept(s1, (struct sockaddr *)&saddr, &size);

        ok(s4 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s1);
        closesocket(s2);
        closesocket(s3);
        closesocket(s4);

        /* Test binding to INADDR_ANY on two sockets. */
        s1 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_any, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_any, tests[i].addrlen);
        ok(rc == SOCKET_ERROR && WSAGetLastError() == WSAEADDRINUSE, "got rc %d, error %d.\n", rc, WSAGetLastError());

        closesocket(s1);
        closesocket(s2);

        /* Test successive binds and bind-after-listen */
        reuse = 1;
        s1 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s2 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s2 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s3 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s3 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = setsockopt(s3, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s4 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s4 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = setsockopt(s4, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = bind(s1, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        rc = bind(s2, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s5 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s5 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        rc = listen(s1, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());
        rc = listen(s2, 1);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());
        rc = connect(s5, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        /* The connection is delivered to the first socket. */
        size = tests[i].addrlen;
        s6 = accept(s1, (struct sockaddr *)&saddr, &size);
        ok(s6 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s1);
        closesocket(s5);
        closesocket(s6);

        rc = bind(s3, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s5 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s5 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = connect(s5, tests[i].addr_loopback, tests[i].addrlen);
        todo_wine ok(!rc, "got error %d.\n", WSAGetLastError());

        /* The connection is delivered to the second socket. */
        size = tests[i].addrlen;
        s6 = accept(s2, (struct sockaddr *)&saddr, &size);
        todo_wine ok(s6 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s2);
        closesocket(s5);
        closesocket(s6);

        rc = bind(s4, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());
        rc = listen(s3, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s5 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s5 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = connect(s5, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        /* The connection is delivered to the third socket. */
        size = tests[i].addrlen;
        s6 = accept(s3, (struct sockaddr *)&saddr, &size);
        ok(s6 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s3);
        closesocket(s5);
        closesocket(s6);

        rc = listen(s4, 1);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        s5 = socket(tests[i].domain, SOCK_STREAM, 0);
        ok(s5 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());
        rc = connect(s5, tests[i].addr_loopback, tests[i].addrlen);
        ok(!rc, "got error %d.\n", WSAGetLastError());

        /* The connection is delivered to the fourth socket. */
        size = tests[i].addrlen;
        s6 = accept(s4, (struct sockaddr *)&saddr, &size);
        ok(s6 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

        closesocket(s4);
        closesocket(s5);
        closesocket(s6);

        winetest_pop_context();
    }

    /* SO_REUSEADDR and SO_EXCLUSIVEADDRUSE are mutually exclusive. */
    s1 = socket(AF_INET, SOCK_STREAM, 0);
    ok(s1 != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

    value = 1;
    rc = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
    ok(!rc, "got error %d.\n", WSAGetLastError());

    value = 1;
    rc = setsockopt(s1, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&value, sizeof(value));
    ok(rc == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL, "got rc %d, error %d.\n", rc, WSAGetLastError());

    value = 0;
    rc = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));

    value = 1;
    rc = setsockopt(s1, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&value, sizeof(value));
    ok(!rc, "got error %d.\n", WSAGetLastError());

    value = 1;
    rc = setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
    ok(rc == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL, "got rc %d, error %d.\n", rc, WSAGetLastError());

    closesocket(s1);

    /* Test SO_EXCLUSIVEADDRUSE. */
    for (i = 0; i < ARRAY_SIZE(tests_exclusive); ++i)
    {
        SOCKET s[2];

        winetest_push_context("test %u", i);

        for (j = 0; j < 2; ++j)
        {
            s[j] = socket(tests_exclusive[i].s[j].domain, SOCK_STREAM, 0);
            ok(s[j] != INVALID_SOCKET, "got error %d.\n", WSAGetLastError());

            if (tests_exclusive[i].s[j].exclusive)
            {
                value = 1;
                rc = setsockopt(s[j], SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&value, sizeof(value));
                ok(!rc, "got error %d.\n", WSAGetLastError());
            }
            if (tests_exclusive[i].s[j].domain == AF_INET6)
            {
                value = 0;
                rc = setsockopt(s[j], IPPROTO_IPV6, IPV6_V6ONLY, (char*)&value, sizeof(value));
                ok(!rc, "got error %d.\n", WSAGetLastError());
            }
        }
        rc = bind(s[0], tests_exclusive[i].s[0].addr, tests_exclusive[i].s[0].addrlen);
        ok(!rc || (tests_exclusive[i].s[0].domain == AF_INET6 && WSAGetLastError() == WSAEADDRNOTAVAIL), "got error %d.\n", WSAGetLastError());

        rc = bind(s[1], tests_exclusive[i].s[1].addr, tests_exclusive[i].s[1].addrlen);

        if (tests_exclusive[i].error)
            ok(rc == SOCKET_ERROR && WSAGetLastError() == tests_exclusive[i].error,
                    "got rc %d, error %d, expected error %d.\n", rc, WSAGetLastError(), tests_exclusive[i].error);
        else
            ok(!rc, "got error %d.\n", WSAGetLastError());

        closesocket(s[0]);
        closesocket(s[1]);
        winetest_pop_context();
    }
}

#define IP_PKTINFO_LEN (sizeof(WSACMSGHDR) + WSA_CMSG_ALIGN(sizeof(struct in_pktinfo)))

static unsigned int got_ip_pktinfo_apc;

static void WINAPI ip_pktinfo_apc(DWORD error, DWORD size, OVERLAPPED *overlapped, DWORD flags)
{
    ok(error == WSAEMSGSIZE, "got error %lu\n", error);
    ok(size == 6, "got size %lu\n", size);
    ok(!flags, "got flags %#lx\n", flags);
    ++got_ip_pktinfo_apc;
}

static void test_ip_pktinfo(void)
{
    ULONG addresses[2] = {inet_addr("127.0.0.1"), htonl(INADDR_ANY)};
    char recvbuf[10], pktbuf[512], msg[] = "HELLO";
    struct sockaddr_in s1addr, s2addr, s3addr;
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

    for (i=0;i<ARRAY_SIZE(addresses);i++)
    {
        s1addr.sin_addr.s_addr = addresses[i];

        /* Build "server" side socket */
        s1=socket(AF_INET, SOCK_DGRAM, 0);
        ok(s1 != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

        /* Obtain the WSARecvMsg function */
        rc = WSAIoctl(s1, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSARecvMsg_GUID, sizeof(WSARecvMsg_GUID),
                 &pWSARecvMsg, sizeof(pWSARecvMsg), &dwBytes, NULL, NULL);
        ok(!rc, "failed to get WSARecvMsg, error %u\n", WSAGetLastError());
#ifdef __REACTOS__
        if (WSAGetLastError() == WSAEOPNOTSUPP) {
            skip("Got WSAEOPNOTSUPP\n");
            closesocket(s1);
            continue;
        }
#endif

        /* Setup the server side socket */
        rc=bind(s1, (struct sockaddr*)&s1addr, sizeof(s1addr));
        ok(rc != SOCKET_ERROR, "bind() failed error: %d\n", WSAGetLastError());

        /* Build "client" side socket */
        addrlen = sizeof(s2addr);
        rc = getsockname(s1, (struct sockaddr *) &s2addr, &addrlen);
        ok(!rc, "failed to get address, error %u\n", WSAGetLastError());
        s2addr.sin_addr.s_addr = addresses[0]; /* Always target the local adapter address */
        s2=socket(AF_INET, SOCK_DGRAM, 0);
        ok(s2 != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

        /* Test an empty message header */
        rc=pWSARecvMsg(s1, NULL, NULL, NULL, NULL);
        err=WSAGetLastError();
        ok(rc == SOCKET_ERROR && err == WSAEFAULT, "WSARecvMsg() failed error: %d (ret = %d)\n", err, rc);

        /* Test that when no control data arrives, a 0-length NULL-valued control buffer should succeed */
        SetLastError(0xdeadbeef);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
        hdr.Control.buf = NULL;
        hdr.Control.len = 0;
        rc=pWSARecvMsg(s1, &hdr, &dwSize, NULL, NULL);
        ok(rc == 0, "WSARecvMsg() failed error: %d\n", WSAGetLastError());
        hdr.Control.buf = pktbuf;

        /* Now start IP_PKTINFO for future tests */
        rc=setsockopt(s1, IPPROTO_IP, IP_PKTINFO, (const char*)&yes, sizeof(yes));
        ok(rc == 0, "failed to set IPPROTO_IP flag IP_PKTINFO!\n");

        /*
         * Send a packet from the client to the server and test for specifying
         * a short control header.
         */
        SetLastError(0xdeadbeef);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
        hdr.Control.len = 1;
        dwSize = 0xdeadbeef;
        rc = pWSARecvMsg(s1, &hdr, &dwSize, NULL, NULL);
        ok(rc == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEMSGSIZE, "got error %u\n", WSAGetLastError());
        todo_wine ok(dwSize == sizeof(msg), "got size %lu\n", dwSize);
        ok(hdr.dwFlags == MSG_CTRUNC, "got flags %#lx\n", hdr.dwFlags);
        hdr.dwFlags = 0; /* Reset flags */

        /* Perform another short control header test, this time with an overlapped receive */
        hdr.Control.len = 1;
        ov.Internal = 0xdead1;
        ov.InternalHigh = 0xdead2;
        ov.Offset = 0xdead3;
        ov.OffsetHigh = 0xdead4;
        rc=pWSARecvMsg(s1, &hdr, NULL, &ov, NULL);
        err=WSAGetLastError();
        ok(rc != 0 && err == WSA_IO_PENDING, "WSARecvMsg() failed error: %d\n", err);
        SetLastError(0xdeadbeef);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
        ok(!WaitForSingleObject(ov.hEvent, 100), "wait failed\n");
        ok((NTSTATUS)ov.Internal == STATUS_BUFFER_OVERFLOW, "got status %#lx\n", (NTSTATUS)ov.Internal);
        ok(ov.InternalHigh == sizeof(msg), "got size %Iu\n", ov.InternalHigh);
        ok(ov.Offset == 0xdead3, "got Offset %lu\n", ov.Offset);
        ok(ov.OffsetHigh == 0xdead4, "got OffsetHigh %lu\n", ov.OffsetHigh);
        dwFlags = 0xdeadbeef;
        rc = WSAGetOverlappedResult(s1, &ov, &dwSize, FALSE, &dwFlags);
        ok(!rc, "expected failure\n");
        ok(WSAGetLastError() == WSAEMSGSIZE, "got error %u\n", WSAGetLastError());
        ok(dwSize == sizeof(msg), "got size %lu\n", dwSize);
        todo_wine ok(dwFlags == 0xdeadbeef, "got flags %#lx\n", dwFlags);
        ok(hdr.dwFlags == MSG_CTRUNC,
           "WSARecvMsg() overlapped operation set unexpected flags %ld.\n", hdr.dwFlags);
        hdr.dwFlags = 0; /* Reset flags */

        /* And with an APC. */

        SetLastError(0xdeadbeef);
        rc = sendto(s2, msg, sizeof(msg), 0, (struct sockaddr *)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
        hdr.Control.len = 1;

        ov.Internal = 0xdead1;
        ov.InternalHigh = 0xdead2;
        ov.Offset = 0xdead3;
        ov.OffsetHigh = 0xdead4;
        dwSize = 0xdeadbeef;
        rc = pWSARecvMsg(s1, &hdr, NULL, &ov, ip_pktinfo_apc);
        ok(rc == -1, "expected failure\n");
        todo_wine ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

        rc = SleepEx(1000, TRUE);
        ok(rc == WAIT_IO_COMPLETION, "got %d\n", rc);
        ok(got_ip_pktinfo_apc == 1, "apc was called %u times\n", got_ip_pktinfo_apc);
        ok(hdr.dwFlags == MSG_CTRUNC, "got flags %#lx\n", hdr.dwFlags);
        got_ip_pktinfo_apc = 0;

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
           "WSARecvMsg() control length mismatch (%ld != sizeof pktbuf).\n", hdr.Control.len);
        rc=sendto(s2, msg, sizeof(msg), 0, (struct sockaddr*)&s2addr, sizeof(s2addr));
        ok(rc == sizeof(msg), "sendto() failed error: %d\n", WSAGetLastError());
        ok(!WaitForSingleObject(ov.hEvent, 100), "wait failed\n");
        dwSize = 0;
        WSAGetOverlappedResult(s1, &ov, &dwSize, FALSE, NULL);
        ok(dwSize == sizeof(msg),
           "WSARecvMsg() buffer length does not match transmitted data!\n");
        ok(strncmp(iovec[0].buf, msg, sizeof(msg)) == 0,
           "WSARecvMsg() buffer does not match transmitted data!\n");
        ok(hdr.Control.len == IP_PKTINFO_LEN,
           "WSARecvMsg() control length mismatch (%ld).\n", hdr.Control.len);

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

    CloseHandle(ov.hEvent);
}

static void test_ipv4_cmsg(void)
{
    static const DWORD off = 0;
    static const DWORD on = 1;
    SOCKADDR_IN localhost = {0};
    SOCKET client, server;
    char payload[] = "HELLO";
    char control[100];
    WSABUF payload_buf = {sizeof(payload), payload};
    WSAMSG msg = {NULL, 0, &payload_buf, 1, {sizeof(control), control}, 0};
    WSACMSGHDR *header = (WSACMSGHDR *)control;
    LPFN_WSARECVMSG pWSARecvMsg;
    INT *int_data = (INT *)WSA_CMSG_DATA(header);
    IN_PKTINFO *pkt_info = (IN_PKTINFO *)WSA_CMSG_DATA(header);
    DWORD count, state;
    int rc;

    localhost.sin_family = AF_INET;
    localhost.sin_port = htons(SERVERPORT);
    inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(client != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(server != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    rc = bind(server, (SOCKADDR *)&localhost, sizeof(localhost));
    ok(rc != SOCKET_ERROR, "bind failed, error %u\n", WSAGetLastError());
    rc = connect(client, (SOCKADDR *)&localhost, sizeof(localhost));
    ok(rc != SOCKET_ERROR, "connect failed, error %u\n", WSAGetLastError());

    rc = WSAIoctl(server, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSARecvMsg_GUID, sizeof(WSARecvMsg_GUID),
                  &pWSARecvMsg, sizeof(pWSARecvMsg), &count, NULL, NULL);
    ok(!rc, "failed to get WSARecvMsg, error %u\n", WSAGetLastError());

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "ReactOS crashes on the tests below.\n");
        goto cleanup;
    }
#endif
    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IP, IP_RECVTTL, (const char *)&on, sizeof(on));
#ifdef __REACTOS__
    if (rc && WSAGetLastError() == WSAENOPROTOOPT) {
        skip("IP_RECVTTL is not supported on this operating system.\n");
    } else {
#endif
    ok(!rc, "failed to set IP_RECVTTL, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IP, IP_RECVTTL, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IP_RECVTTL, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IP, "expected IPPROTO_IP, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IP_TTL || broken(header->cmsg_type == IP_HOPLIMIT) /* <= win10 v1607 */,
       "expected IP_TTL, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(INT),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(INT), header->cmsg_len);
    ok(*int_data >= 32, "expected at least 32, got %i\n", *int_data);
    setsockopt(server, IPPROTO_IP, IP_RECVTTL, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IP_RECVTTL, error %u\n", WSAGetLastError());
#ifdef __REACTOS__
    }
#endif

    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IP, IP_PKTINFO, (const char *)&on, sizeof(on));
    ok(!rc, "failed to set IP_PKTINFO, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IP, IP_PKTINFO, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IP_PKTINFO, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IP, "expected IPPROTO_IP, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IP_PKTINFO, "expected IP_PKTINFO, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(IN_PKTINFO),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(IN_PKTINFO), header->cmsg_len);
    ok(!memcmp(&pkt_info->ipi_addr, &localhost.sin_addr, sizeof(IN_ADDR)), "expected 127.0.0.1\n");
    rc = setsockopt(server, IPPROTO_IP, IP_PKTINFO, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IP_PKTINFO, error %u\n", WSAGetLastError());

    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IP, IP_RECVTOS, (const char *)&on, sizeof(on));
#ifdef __REACTOS__
    if (rc && WSAGetLastError() == WSAENOPROTOOPT) {
        skip("IP_RECVTOS is not supported on this operating system.\n");
    } else {
#endif
    ok(!rc, "failed to set IP_RECVTOS, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IP, IP_RECVTOS, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IP_RECVTOS, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IP, "expected IPPROTO_IP, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IP_TOS || broken(header->cmsg_type == IP_TCLASS) /* <= win10 v1607 */,
       "expected IP_TOS, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(INT),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(INT), header->cmsg_len);
    ok(*int_data == 0, "expected 0, got %i\n", *int_data);
    rc = setsockopt(server, IPPROTO_IP, IP_RECVTOS, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IP_RECVTOS, error %u\n", WSAGetLastError());
#ifdef __REACTOS__
    }
cleanup:
#endif

    closesocket(server);
    closesocket(client);
}

static void test_ipv6_cmsg(void)
{
    static const DWORD off = 0;
    static const DWORD on = 1;
    SOCKADDR_IN6 localhost = {0};
    SOCKET client, server;
    char payload[] = "HELLO";
    char control[100];
    WSABUF payload_buf = {sizeof(payload), payload};
    WSAMSG msg = {NULL, 0, &payload_buf, 1, {sizeof(control), control}, 0};
    WSACMSGHDR *header = (WSACMSGHDR *)control;
    LPFN_WSARECVMSG pWSARecvMsg;
    INT *int_data = (INT *)WSA_CMSG_DATA(header);
    IN6_PKTINFO *pkt_info = (IN6_PKTINFO *)WSA_CMSG_DATA(header);
    DWORD count, state;
    int rc;

#ifdef __REACTOS__
    if (!HasIPV6) {
        skip("test_ipv6_cmsg() is invalid on operating systems that don't support IPV6!\n");
        return;
    }
#endif
    localhost.sin6_family = AF_INET6;
    localhost.sin6_port = htons(SERVERPORT);
    inet_pton(AF_INET6, "::1", &localhost.sin6_addr);

    client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    ok(client != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    server = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    ok(server != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    rc = bind(server, (SOCKADDR *)&localhost, sizeof(localhost));
    ok(rc != SOCKET_ERROR || WSAGetLastError() == WSAEADDRNOTAVAIL, "bind failed, error %u\n", WSAGetLastError());
    if (WSAGetLastError() == WSAEADDRNOTAVAIL)
    {
        skip("IPv6 not supported, skipping test\n");
        goto cleanup;
    }
    rc = connect(client, (SOCKADDR *)&localhost, sizeof(localhost));
    ok(rc != SOCKET_ERROR, "connect failed, error %u\n", WSAGetLastError());

    rc = WSAIoctl(server, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSARecvMsg_GUID, sizeof(WSARecvMsg_GUID),
                  &pWSARecvMsg, sizeof(pWSARecvMsg), &count, NULL, NULL);
    ok(!rc, "failed to get WSARecvMsg, error %u\n", WSAGetLastError());

    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IPV6, IPV6_HOPLIMIT, (const char *)&on, sizeof(on));
    ok(!rc, "failed to set IPV6_HOPLIMIT, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IPV6, IPV6_HOPLIMIT, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IPV6_HOPLIMIT, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IPV6, "expected IPPROTO_IPV6, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IPV6_HOPLIMIT, "expected IPV6_HOPLIMIT, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(INT),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(INT), header->cmsg_len);
    ok(*int_data >= 32, "expected at least 32, got %i\n", *int_data);
    setsockopt(server, IPPROTO_IPV6, IPV6_HOPLIMIT, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IPV6_HOPLIMIT, error %u\n", WSAGetLastError());

    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IPV6, IPV6_PKTINFO, (const char *)&on, sizeof(on));
    ok(!rc, "failed to set IPV6_PKTINFO, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IPV6, IPV6_PKTINFO, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IPV6_PKTINFO, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IPV6, "expected IPPROTO_IPV6, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IPV6_PKTINFO, "expected IPV6_PKTINFO, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(IN6_PKTINFO),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(IN6_PKTINFO), header->cmsg_len);
    ok(!memcmp(&pkt_info->ipi6_addr, &localhost.sin6_addr, sizeof(IN6_ADDR)), "expected ::1\n");
    rc = setsockopt(server, IPPROTO_IPV6, IPV6_PKTINFO, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IPV6_PKTINFO, error %u\n", WSAGetLastError());

    memset(control, 0, sizeof(control));
    msg.Control.len = sizeof(control);
    rc = setsockopt(server, IPPROTO_IPV6, IPV6_RECVTCLASS, (const char *)&on, sizeof(on));
    ok(!rc, "failed to set IPV6_RECVTCLASS, error %u\n", WSAGetLastError());
    state = 0;
    count = sizeof(state);
    rc = getsockopt(server, IPPROTO_IPV6, IPV6_RECVTCLASS, (char *)&state, (INT *)&count);
    ok(!rc, "failed to get IPV6_RECVTCLASS, error %u\n", WSAGetLastError());
    ok(state == 1, "expected 1, got %lu\n", state);
    rc = send(client, payload, sizeof(payload), 0);
    ok(rc == sizeof(payload), "send failed, error %u\n", WSAGetLastError());
    rc = pWSARecvMsg(server, &msg, &count, NULL, NULL);
    ok(!rc, "WSARecvMsg failed, error %u\n", WSAGetLastError());
    ok(count == sizeof(payload), "expected length %Iu, got %lu\n", sizeof(payload), count);
    ok(header->cmsg_level == IPPROTO_IPV6, "expected IPPROTO_IPV6, got %i\n", header->cmsg_level);
    ok(header->cmsg_type == IPV6_TCLASS, "expected IPV6_TCLASS, got %i\n", header->cmsg_type);
    ok(header->cmsg_len == sizeof(*header) + sizeof(INT),
       "expected length %Iu, got %Iu\n", sizeof(*header) + sizeof(INT), header->cmsg_len);
    ok(*int_data == 0, "expected 0, got %i\n", *int_data);
    rc = setsockopt(server, IPPROTO_IPV6, IPV6_RECVTCLASS, (const char *)&off, sizeof(off));
    ok(!rc, "failed to clear IPV6_RECVTCLASS, error %u\n", WSAGetLastError());

cleanup:
    closesocket(server);
    closesocket(client);
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

struct send_udp_thread_param
{
    int sock;
    HANDLE start_event;
};

static DWORD WINAPI send_udp_thread( void *param )
{
    struct send_udp_thread_param *p = param;
    static const TIMEVAL timeout_zero = {0};
    static char buf[256];
    fd_set writefds;
    unsigned int i;
    int ret;

    WaitForSingleObject( p->start_event, INFINITE );
    for (i = 0; i < 256; ++i)
    {
        FD_ZERO(&writefds);
        FD_SET(p->sock, &writefds);
        ret = select( 1, NULL, &writefds, NULL, &timeout_zero );
        ok( ret == 1, "got %d, i %u.\n", ret, i );
        ret = send( p->sock, buf, sizeof(buf), 0 );
        ok( ret == sizeof(buf), "got %d, error %u, i %u.\n", ret, WSAGetLastError(), i );
#ifdef __REACTOS__
        /* This greatly improves test reliability on WS03 */
        Sleep(1);
#endif
    }

    return 0;
}

static void test_UDP(void)
{
    /* This function tests UDP sendto() and recvfrom(). UDP is unreliable, so it is
       possible that this test fails due to dropped packets. */

    /* peer 0 receives data from all other peers */
    static const TIMEVAL timeout_zero = {0};
    struct sock_info peer[NUM_UDP_PEERS];
    char buf[16], sockaddr_buf[1024];
    int ss, i, n_recv, n_sent, ret;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;
    int sock;
    struct send_udp_thread_param udp_thread_param;
    HANDLE thread;
    fd_set writefds;


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

    /* Test that specifying a too small fromlen for recvfrom() shouldn't write unnecessary data */
    n_sent = sendto ( peer[1].s, buf, sizeof(buf), 0, (struct sockaddr *)&peer[0].addr, sizeof(peer[0].addr) );
    ok ( n_sent == sizeof(buf), "UDP: sendto() sent wrong amount of data or socket error: %d\n", n_sent );

    sockaddr_buf[0] = 'A';
    ss = 1;
    n_recv = recvfrom ( peer[0].s, buf, sizeof(buf), 0, (struct sockaddr *)sockaddr_buf, &ss );
    todo_wine
    ok ( n_recv == SOCKET_ERROR, "UDP: recvfrom() succeeded\n" );
    ok ( sockaddr_buf[0] == 'A', "UDP: marker got overwritten\n" );
    if ( n_recv == SOCKET_ERROR )
    {
        ss = sizeof ( peer[0].addr );
        n_recv = recvfrom ( peer[0].s, buf, sizeof(buf), 0, (struct sockaddr *)sockaddr_buf, &ss );
        ok ( n_recv == sizeof(buf), "UDP: recvfrom() failed\n" );
    }

    /* Test that specifying a large fromlen for recvfrom() shouldn't write unnecessary data besides the socket address */
    n_sent = sendto ( peer[1].s, buf, sizeof(buf), 0, (struct sockaddr *)&peer[0].addr, sizeof(peer[0].addr) );
    ok ( n_sent == sizeof(buf), "UDP: sendto() sent wrong amount of data or socket error: %d\n", n_sent );

    sockaddr_buf[1023] = 'B';
    ss = sizeof(sockaddr_buf);
    n_recv = recvfrom ( peer[0].s, buf, sizeof(buf), 0, (struct sockaddr *)sockaddr_buf, &ss );
    ok ( n_recv == sizeof(buf), "UDP: recvfrom() received wrong amount of data or socket error: %d\n", n_recv );
    ok ( sockaddr_buf[1023] == 'B', "UDP: marker got overwritten\n" );

    /* test getsockname() */
    ok ( peer[0].addr.sin_port == htons ( SERVERPORT ), "UDP: getsockname returned incorrect peer port\n" );

    for ( i = 1; i < NUM_UDP_PEERS; i++ ) {
        /* send client's port */
        memcpy( buf, &peer[i].addr.sin_port, sizeof(peer[i].addr.sin_port) );
        n_sent = sendto ( peer[i].s, buf, sizeof(buf), 0, (struct sockaddr*) &peer[0].addr, sizeof(peer[0].addr) );
        ok ( n_sent == sizeof(buf), "UDP: sendto() sent wrong amount of data or socket error: %d\n", n_sent );
    }

    for ( i = 1; i < NUM_UDP_PEERS; i++ ) {
        n_recv = recvfrom ( peer[0].s, buf, sizeof(buf), 0,(struct sockaddr *) &peer[0].peer, &ss );
        ok ( n_recv == sizeof(buf), "UDP: recvfrom() received wrong amount of data or socket error: %d\n", n_recv );
        ok ( memcmp ( &peer[0].peer.sin_port, buf, sizeof(peer[0].addr.sin_port) ) == 0, "UDP: port numbers do not match\n" );
    }

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok( sock != INVALID_SOCKET, "got error %u.\n", WSAGetLastError() );

    memset( &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(255);

    ret = connect( sock, (struct sockaddr *)&addr, sizeof(addr) );
    ok( !ret, "got error %u.\n", WSAGetLastError() );

    /* Send to UDP socket succeeds even if the packets are not received and the network is replying with
     * "destination port unreachable" ICMP messages. */
    for (i = 0; i < 10; ++i)
    {
        ret = send( sock, buf, sizeof(buf), 0 );
        ok( ret == sizeof(buf), "got %d, error %u.\n", ret, WSAGetLastError() );
    }

    /* Test sending packets in parallel (mostly a regression test for Wine async handling race conditions). */
    set_blocking( sock, FALSE );

    udp_thread_param.sock = sock;
    udp_thread_param.start_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    thread = CreateThread( NULL, 0, send_udp_thread, &udp_thread_param, 0, NULL );
    SetEvent( udp_thread_param.start_event );
    for (i = 0; i < 256; ++i)
    {
        ret = send( sock, buf, sizeof(buf), 0 );
        ok( ret == sizeof(buf), "got %d, error %u, i %u.\n", ret, WSAGetLastError(), i );
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);
        ret = select( 1, NULL, &writefds, NULL, &timeout_zero );
        ok( ret == 1, "got %d, i %u.\n", ret, i );
    }
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( udp_thread_param.start_event );

    closesocket(sock);

    /* Test sending to port 0. */
    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    ok( sock != INVALID_SOCKET, "got error %u.\n", WSAGetLastError() );
    memset( &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    ret = sendto( sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr, sizeof(addr) );
    ok( ret == sizeof(buf), "got ret %d, error %u.\n", ret, WSAGetLastError() );
    closesocket(sock);

#ifdef __REACTOS__
    if (HasIPV6) {
#endif
    sock = socket( AF_INET6, SOCK_DGRAM, 0 );
    ok( sock != INVALID_SOCKET, "got error %u.\n", WSAGetLastError() );
    memset( &addr6, 0, sizeof(addr6) );
    addr6.sin6_family = AF_INET6;
    ret = inet_pton( AF_INET6, "::1", &addr6.sin6_addr );
    ok( ret, "got error %u.\n", WSAGetLastError() );
    ret = sendto( sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr6, sizeof(addr6) );
    ok( ret == sizeof(buf), "got ret %d, error %u.\n", ret, WSAGetLastError() );
    closesocket(sock);
#ifdef __REACTOS__
    }
#endif
}

static void test_WSASocket(void)
{
    SOCKET sock = INVALID_SOCKET;
    WSAPROTOCOL_INFOA *pi;
    int wsaproviders[] = {IPPROTO_TCP, IPPROTO_IP};
    int autoprotocols[] = {IPPROTO_TCP, IPPROTO_UDP};
    int items, err, size, socktype, i, j;
    DWORD pi_size;

    static const struct
    {
        int family, type, protocol;
        DWORD error;
        int ret_family, ret_type, ret_protocol;
        int ret_family_alt;
    }
    tests[] =
    {
        /* 0 */
        {0xdead,    SOCK_STREAM, IPPROTO_TCP, WSAEAFNOSUPPORT},
        {-1,        SOCK_STREAM, IPPROTO_TCP, WSAEAFNOSUPPORT},
        {AF_INET,   0xdead,      IPPROTO_TCP, WSAESOCKTNOSUPPORT},
        {AF_INET,   -1,          IPPROTO_TCP, WSAESOCKTNOSUPPORT},
        {AF_INET,   SOCK_STREAM, 0xdead,      WSAEPROTONOSUPPORT},
        {AF_INET,   SOCK_STREAM, -1,          WSAEPROTONOSUPPORT},
        {0xdead,    0xdead,      IPPROTO_TCP, WSAESOCKTNOSUPPORT},
        {0xdead,    SOCK_STREAM, 0xdead,      WSAEAFNOSUPPORT},
        {AF_INET,   0xdead,      0xdead,      WSAESOCKTNOSUPPORT},
        {0xdead,    SOCK_STREAM, IPPROTO_UDP, WSAEAFNOSUPPORT},

        /* 10 */
        {AF_INET,   SOCK_STREAM, 0,           0, AF_INET, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET,   SOCK_DGRAM,  0,           0, AF_INET, SOCK_DGRAM,  IPPROTO_UDP},
        {AF_INET,   0xdead,      0,           WSAESOCKTNOSUPPORT},
        {AF_INET,   0,           IPPROTO_TCP, 0, AF_INET, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET,   0,           IPPROTO_UDP, 0, AF_INET, SOCK_DGRAM,  IPPROTO_UDP},
        {AF_INET,   0,           0xdead,      WSAEPROTONOSUPPORT},
        {AF_INET,   0,           0,           0, AF_INET, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET,   SOCK_STREAM, IPPROTO_UDP, WSAEPROTONOSUPPORT},
        {AF_INET,   SOCK_DGRAM,  IPPROTO_TCP, WSAEPROTONOSUPPORT},

        /* 19 */
        {AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, AF_INET, SOCK_STREAM, IPPROTO_TCP, AF_INET6 /* win11 */},
        {AF_UNSPEC, SOCK_STREAM, 0xdead,      WSAEPROTONOSUPPORT},
        {AF_UNSPEC, 0xdead,      IPPROTO_UDP, WSAESOCKTNOSUPPORT},
        {AF_UNSPEC, SOCK_STREAM, 0,           WSAEINVAL},
        {AF_UNSPEC, SOCK_DGRAM,  0,           WSAEINVAL},
        {AF_UNSPEC, 0xdead,      0,           WSAEINVAL},
        {AF_UNSPEC, 0,           IPPROTO_TCP, 0, AF_INET, SOCK_STREAM, IPPROTO_TCP, AF_INET6 /* win11 */},
        {AF_UNSPEC, 0,           IPPROTO_UDP, 0, AF_INET, SOCK_DGRAM,  IPPROTO_UDP, AF_INET6 /* win11 */},
        {AF_UNSPEC, 0,           0xdead,      WSAEPROTONOSUPPORT},
        {AF_UNSPEC, 0,           0,           WSAEINVAL},
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        SetLastError( 0xdeadbeef );
        sock = WSASocketA( tests[i].family, tests[i].type, tests[i].protocol, NULL, 0, 0 );
        todo_wine_if (i == 7)
            ok(WSAGetLastError() == tests[i].error, "Test %u: got wrong error %u\n", i, WSAGetLastError());
        if (tests[i].error)
        {
            ok(sock == INVALID_SOCKET, "Test %u: expected failure\n", i);
        }
        else
        {
            WSAPROTOCOL_INFOA info;

            ok(sock != INVALID_SOCKET, "Text %u: expected success\n", i);

            size = sizeof(info);
            err = getsockopt( sock, SOL_SOCKET, SO_PROTOCOL_INFOA, (char *)&info, &size );
            ok(!err, "Test %u: getsockopt failed, error %u\n", i, WSAGetLastError());
            ok(info.iAddressFamily == tests[i].ret_family ||
               (tests[i].ret_family_alt && info.iAddressFamily == tests[i].ret_family_alt),
               "Test %u: got wrong family %d\n", i, info.iAddressFamily);
            ok(info.iSocketType == tests[i].ret_type, "Test %u: got wrong type %d\n", i, info.iSocketType);
            ok(info.iProtocol == tests[i].ret_protocol, "Test %u: got wrong protocol %d\n", i, info.iProtocol);

            closesocket( sock );
        }
    }

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

    pi = malloc(pi_size);
    ok(pi != NULL, "Failed to allocate memory\n");

    items = WSAEnumProtocolsA(wsaproviders, pi, &pi_size);
    ok(items != SOCKET_ERROR, "WSAEnumProtocolsA failed, last error is %d\n",
            WSAGetLastError());

    if (items == 0) {
        skip("No protocols enumerated.\n");
        free(pi);
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

    socktype = SOCK_STREAM;
    WSASetLastError(0xdeadbeef);
    err = setsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&socktype, sizeof(socktype));
    ok(err == -1, "expected failure\n");
#ifdef __REACTOS__
    todo_wine ok(WSAGetLastError() == WSAEINVAL || broken(WSAGetLastError() == WSAENOPROTOOPT) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
#endif

    socktype = SOCK_DGRAM;
    WSASetLastError(0xdeadbeef);
    err = setsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&socktype, sizeof(socktype));
    ok(err == -1, "expected failure\n");
#ifdef __REACTOS__
    todo_wine ok(WSAGetLastError() == WSAEINVAL || broken(WSAGetLastError() == WSAENOPROTOOPT) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
#endif

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

    socktype = SOCK_STREAM;
    WSASetLastError(0xdeadbeef);
    err = setsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&socktype, sizeof(socktype));
    ok(err == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

    socktype = SOCK_DGRAM;
    WSASetLastError(0xdeadbeef);
    err = setsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&socktype, sizeof(socktype));
    ok(err == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

    closesocket(sock);

    free(pi);

    pi_size = 0;
    items = WSAEnumProtocolsA(NULL, NULL, &pi_size);
    ok(items == SOCKET_ERROR, "WSAEnumProtocolsA(NULL, NULL, 0) returned %d\n",
            items);
    err = WSAGetLastError();
    ok(err == WSAENOBUFS, "WSAEnumProtocolsA error is %d, not WSAENOBUFS(%d)\n",
            err, WSAENOBUFS);

    pi = malloc(pi_size);
    ok(pi != NULL, "Failed to allocate memory\n");

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
    for (i = 0; i < ARRAY_SIZE(autoprotocols); i++)
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
                ok(pi[j].iSocketType == socktype, "expected %d, got %d\n", socktype, pi[j].iSocketType);
                err = 0;
                break;
            }
        }
        ok(!err, "Protocol %d not found in WSAEnumProtocols\n", autoprotocols[i]);

        closesocket(sock);
    }

    free(pi);

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
        WSAPROTOCOL_INFOW info;

        size = sizeof(socktype);
        socktype = 0xdead;
        err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
        ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
        ok(socktype == SOCK_RAW, "Wrong socket type, expected %d received %d\n",
           SOCK_RAW, socktype);

        size = sizeof(info);
        err = getsockopt(sock, SOL_SOCKET, SO_PROTOCOL_INFOW, (char *) &info, &size);
        ok(!err,"got error %d\n", WSAGetLastError());
        /* Protocol name in info.szProtocol is not entirely consistent across Windows versions and
         * locales, so not testing it. */
        ok(info.iAddressFamily == AF_INET, "got iAddressFamily %d.\n", info.iAddressFamily);
        ok(info.iSocketType == SOCK_RAW, "got iSocketType %d.\n", info.iSocketType);
        ok(info.iMaxSockAddr == 0x10, "got iMaxSockAddr %d.\n", info.iMaxSockAddr);
        ok(info.iMinSockAddr == 0x10, "got iMinSockAddr %d.\n", info.iMinSockAddr);
        todo_wine ok(!info.iProtocol, "got iProtocol %d.\n", info.iProtocol);
        ok(info.iProtocolMaxOffset == 255, "got iProtocol %d.\n", info.iProtocolMaxOffset);
        ok(info.dwProviderFlags == (PFL_MATCHES_PROTOCOL_ZERO | PFL_HIDDEN), "got dwProviderFlags %#lx.\n",
                info.dwProviderFlags);
        ok(info.dwServiceFlags1 == (XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST | XP1_SUPPORT_MULTIPOINT
                | XP1_MESSAGE_ORIENTED | XP1_CONNECTIONLESS), "got dwServiceFlags1 %#lx.\n",
                info.dwServiceFlags1);

        closesocket(sock);

        sock = WSASocketA(0, 0, IPPROTO_RAW, NULL, 0, 0);
        if (sock != INVALID_SOCKET)
        {
            size = sizeof(socktype);
            socktype = 0xdead;
            err = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *) &socktype, &size);
            ok(!err, "getsockopt failed with %d\n", WSAGetLastError());
            ok(socktype == SOCK_RAW, "Wrong socket type, expected %d received %d\n",
               SOCK_RAW, socktype);
            closesocket(sock);

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
        else if (WSAGetLastError() == WSAEACCES)
            skip("SOCK_RAW is not available\n");
        else
            ok(0, "Failed to create socket: %d\n", WSAGetLastError());

    }

    /* IPX socket tests */

    SetLastError(0xdeadbeef);
    sock = WSASocketA(AF_IPX, SOCK_DGRAM, NSPROTO_IPX, NULL, 0, 0);
    if (sock == INVALID_SOCKET)
    {
        ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());
        skip("IPX is not supported\n");
    }
    else
    {
        WSAPROTOCOL_INFOA info;
        closesocket(sock);

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
    ok(err == WSAENOTSOCK, "expected 10038, received %ld\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, 0, NULL),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, received %ld\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, ~0, &info),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEINVAL, "expected 10022, received %ld\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(0, GetCurrentProcessId(), &info),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAENOTSOCK, "expected 10038, received %ld\n", err);

    SetLastError(0xdeadbeef);
    ok(WSADuplicateSocketA(source, GetCurrentProcessId(), NULL),
       "WSADuplicateSocketA should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, received %ld\n", err);

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
    ok(err == WSAEINVAL, "expected 10022, received %ld\n", err);
    }

    closesocket(dupsock);
    closesocket(source);
}

static void test_WSAConnectByName(void)
{
#if defined(__REACTOS__) && DLL_EXPORT_VERSION < 0x600
    skip("test_WSAConnectByName requires DLL_EXPORT_VERSION >= 0x600\n");
#else
    SOCKET s;
    SOCKADDR_IN local_addr = {0}, remote_addr = {0},
                sock_addr = {0}, peer_addr = {0};
    DWORD local_len, remote_len, conn_ctx;
    int ret, err, sock_len, peer_len;
    WSAOVERLAPPED overlap;
    struct addrinfo *first_addrinfo, first_hints;

    conn_ctx = TRUE;

    /* First call of getaddrinfo fails on w8adm */
    first_addrinfo = NULL;
    memset(&first_hints, 0, sizeof(struct addrinfo));
    first_hints.ai_socktype = SOCK_STREAM;
    first_hints.ai_family = AF_INET;
    first_hints.ai_protocol = IPPROTO_TCP;
    getaddrinfo("winehq.org", "http", &first_hints, &first_addrinfo);
    if (first_addrinfo)
        freeaddrinfo(first_addrinfo);
    SetLastError(0xdeadbeef);

    /* Fill all fields */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    local_len = remote_len = sizeof(SOCKADDR_IN);
    ret = WSAConnectByNameA(s, "winehq.org", "http", &local_len, (struct sockaddr *)&local_addr,
                             &remote_len, (struct sockaddr *)&remote_addr, NULL, NULL);
    ok(ret, "WSAConnectByNameA should have succeeded, error %u\n", WSAGetLastError());
    setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, (char *)&conn_ctx, sizeof(DWORD));
    sock_len = peer_len = sizeof(SOCKADDR_IN);
    ret = getsockname(s, (struct sockaddr *)&sock_addr, &sock_len);
    ok(!ret, "getsockname should have succeeded, error %u\n", WSAGetLastError());
    ret = getpeername(s, (struct sockaddr *)&peer_addr, &peer_len);
    ok(!ret, "getpeername should have succeeded, error %u\n", WSAGetLastError());
    ok(sock_len == sizeof(SOCKADDR_IN), "got sockname size of %d\n", sock_len);
    ok(peer_len == sizeof(SOCKADDR_IN), "got peername size of %d\n", peer_len);
    ok(local_len == sizeof(SOCKADDR_IN), "got local size of %lu\n", local_len);
    ok(remote_len == sizeof(SOCKADDR_IN), "got remote size of %lu\n", remote_len);
    ok(!local_addr.sin_port, "local_addr has non-zero sin_port: %hu.\n", local_addr.sin_port);
    ok(!memcmp(&sock_addr.sin_addr, &local_addr.sin_addr, sizeof(struct in_addr)),
       "local_addr did not receive data.\n");
    ok(!memcmp(&peer_addr, &remote_addr, sizeof(SOCKADDR_IN)), "remote_addr did not receive data.\n");
    closesocket(s);

    /* Passing NULL length but a pointer to a sockaddr */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    local_len = remote_len = sizeof(SOCKADDR_IN);
    memset(&local_addr, 0, sizeof(SOCKADDR_IN));
    memset(&remote_addr, 0, sizeof(SOCKADDR_IN));
    memset(&sock_addr, 0, sizeof(SOCKADDR_IN));
    memset(&peer_addr, 0, sizeof(SOCKADDR_IN));
    ret = WSAConnectByNameA(s, "winehq.org", "http", NULL, (struct sockaddr *)&local_addr,
                             NULL, (struct sockaddr *)&remote_addr, NULL, NULL);
    ok(ret, "WSAConnectByNameA should have succeeded, error %u\n", WSAGetLastError());
    setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, (char *)&conn_ctx, sizeof(DWORD));
    sock_len = peer_len = sizeof(SOCKADDR_IN);
    ret = getsockname(s, (struct sockaddr *)&sock_addr, &sock_len);
    ok(!ret, "getsockname should have succeeded, error %u\n", WSAGetLastError());
    ret = getpeername(s, (struct sockaddr *)&peer_addr, &peer_len);
    ok(!ret, "getpeername should have succeeded, error %u\n", WSAGetLastError());
    ok(sock_len == sizeof(SOCKADDR_IN), "got sockname size of %d\n", sock_len);
    ok(peer_len == sizeof(SOCKADDR_IN), "got peername size of %d\n", peer_len);
    ok(!local_addr.sin_family, "local_addr received data.\n");
    ok(!remote_addr.sin_family, "remote_addr received data.\n");
    closesocket(s);

    /* Passing NULLs for node or service */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, NULL, "http", NULL, NULL, NULL, NULL, NULL, NULL);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSAEINVAL, "expected error %u (WSAEINVAL), got %u\n", WSAEINVAL, err);
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    closesocket(s);
    ret = WSAConnectByNameA(s, "winehq.org", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSAEINVAL, "expected error %u (WSAEINVAL), got %u\n", WSAEINVAL, err);
    closesocket(s);

    /* Passing NULL for the addresses and address lengths */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "http", NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret, "WSAConnectByNameA should have succeeded, error %u\n", WSAGetLastError());
    closesocket(s);

    /* Passing NULL for the addresses and passing correct lengths */
    local_len = remote_len = sizeof(SOCKADDR_IN);
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "http", &local_len, NULL,
                            &remote_len, NULL, NULL, NULL);
    ok(ret, "WSAConnectByNameA should have succeeded, error %u\n", WSAGetLastError());
    ok(local_len == sizeof(SOCKADDR_IN), "local_len should have been %Iu, got %ld\n", sizeof(SOCKADDR_IN),
       local_len);
    ok(remote_len == sizeof(SOCKADDR_IN), "remote_len should have been %Iu, got %ld\n", sizeof(SOCKADDR_IN),
       remote_len);
    closesocket(s);

    /* Passing addresses and passing short lengths */
    local_len = remote_len = 3;
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "http", &local_len, (struct sockaddr *)&local_addr,
                            &remote_len, (struct sockaddr *)&remote_addr, NULL, NULL);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSAEFAULT, "expected error %u (WSAEFAULT), got %u\n", WSAEFAULT, err);
    ok(local_len == 3, "local_len should have been 3, got %ld\n", local_len);
    ok(remote_len == 3, "remote_len should have been 3, got %ld\n", remote_len);
    closesocket(s);

    /* Passing addresses and passing long lengths */
    local_len = remote_len = 50;
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "http", &local_len, (struct sockaddr *)&local_addr,
                            &remote_len, (struct sockaddr *)&remote_addr, NULL, NULL);
    ok(ret, "WSAConnectByNameA should have succeeded, error %u\n", WSAGetLastError());
    ok(local_len == sizeof(SOCKADDR_IN), "local_len should have been %Iu, got %ld\n", sizeof(SOCKADDR_IN),
       local_len);
    ok(remote_len == sizeof(SOCKADDR_IN), "remote_len should have been %Iu, got %ld\n", sizeof(SOCKADDR_IN),
       remote_len);
    closesocket(s);

    /* Unknown service */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "nonexistentservice", NULL, NULL, NULL, NULL, NULL, NULL);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSATYPE_NOT_FOUND, "expected error %u (WSATYPE_NOT_FOUND), got %u\n",
       WSATYPE_NOT_FOUND, err);
    closesocket(s);

    /* Connecting with a UDP socket */
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ret = WSAConnectByNameA(s, "winehq.org", "https", NULL, NULL, NULL, NULL, NULL, NULL);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSAEINVAL || err == WSAEFAULT, "expected error %u (WSAEINVAL) or %u (WSAEFAULT), got %u\n",
       WSAEINVAL, WSAEFAULT, err); /* WSAEFAULT win10 >= 1809 */
    closesocket(s);

    /* Passing non-null as the reserved parameter */
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAConnectByNameA(s, "winehq.org", "http", NULL, NULL, NULL, NULL, NULL, &overlap);
    err = WSAGetLastError();
    ok(!ret, "WSAConnectByNameA should have failed\n");
    ok(err == WSAEINVAL, "expected error %u (WSAEINVAL), got %u\n", WSAEINVAL, err);
    closesocket(s);
#endif
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
        for (i = 0; i < ARRAY_SIZE(sock_type); i++)
        {
            if (i == 2)
                tcp_socketpair(&s, &s2);
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
                    ok (net_events.lNetworkEvents == FD_WRITE, "Test[%d]: expected 2, got %ld\n",
                        i, net_events.lNetworkEvents);
                }
                else
                {
                    ok (net_events.lNetworkEvents == 0, "Test[%d]: expected 0, got %ld\n",
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

static DWORD WINAPI SelectReadThread(void *param)
{
    select_thread_params *par = param;
    fd_set readfds;
    int ret;
    struct sockaddr_in addr;
    struct timeval select_timeout;

    FD_ZERO(&readfds);
    FD_SET(par->s, &readfds);
    select_timeout.tv_sec=5;
    select_timeout.tv_usec=0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVERIP);
    addr.sin_port = htons(SERVERPORT);

    do_bind(par->s, (struct sockaddr *)&addr, sizeof(addr));
    wsa_ok(listen(par->s, SOMAXCONN ), 0 ==, "SelectReadThread (%lx): listen failed: %d\n");

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

    /* udp test */
    SetLastError(0xdeadbeef);
    fdA = socket(AF_INET, SOCK_DGRAM, 0);
    ok ((fdA != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );

    ok ((listen(fdA, 1) == SOCKET_ERROR), "listen did not fail\n");
    ret = WSAGetLastError();
    ok (ret == WSAEOPNOTSUPP, "expected 10045, received %d\n", ret);

    ret = closesocket(fdA);
    ok (ret == 0, "closesocket failed unexpectedly: %d\n", ret);

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

    acceptc = 1;
    WSASetLastError(0xdeadbeef);
    ret = setsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char *)&acceptc, sizeof(acceptc));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

    acceptc = 0;
    WSASetLastError(0xdeadbeef);
    ret = setsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char *)&acceptc, sizeof(acceptc));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

    ok (!listen(fdA, 0), "listen failed\n");
    ok (!listen(fdA, SOMAXCONN), "double listen failed\n");

    acceptc = 0xdead;
    ret = getsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char*)&acceptc, &olen);
    ok (!ret, "getsockopt failed\n");
    ok (acceptc == 1, "SO_ACCEPTCONN should be 1, received %d\n", acceptc);

    acceptc = 1;
    WSASetLastError(0xdeadbeef);
    ret = setsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char *)&acceptc, sizeof(acceptc));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

    acceptc = 0;
    WSASetLastError(0xdeadbeef);
    ret = setsockopt(fdA, SOL_SOCKET, SO_ACCEPTCONN, (char *)&acceptc, sizeof(acceptc));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOPROTOOPT, "got error %u\n", WSAGetLastError());

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

    fd_set readfds, writefds, exceptfds, *alloc_fds;
    SOCKET fdListen, fdRead, fdWrite, sockets[200];
    int ret, len;
    char buffer;
    struct timeval select_timeout;
    struct sockaddr_in address;
    select_thread_params thread_params;
    HANDLE thread_handle;
    DWORD ticks, id, old_protect;
    unsigned int apc_count;
    unsigned int maxfd, i;
    char *page_pair;

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
    ok(ticks < 100, "select was blocking for %lu ms\n", ticks);
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
    ok ( (thread_handle != NULL), "CreateThread failed unexpectedly: %ld\n", GetLastError());

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

    tcp_socketpair(&fdRead, &fdWrite);
    maxfd = fdRead;
    if(fdWrite > maxfd) maxfd = fdWrite;

    FD_ZERO(&readfds);
    FD_SET(fdRead, &readfds);
    apc_count = 0;
    ret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
    ok(ret, "QueueUserAPC returned %d\n", ret);
    ret = select(fdRead+1, &readfds, NULL, NULL, &select_timeout);
    ok(!ret, "select returned %d\n", ret);
    ok(apc_count == 1, "got apc_count %d.\n", apc_count);

    FD_ZERO(&writefds);
    FD_SET(fdWrite, &writefds);
    apc_count = 0;
    ret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
    ok(ret, "QueueUserAPC returned %d\n", ret);
    ret = select(fdWrite+1, NULL, &writefds, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(FD_ISSET(fdWrite, &writefds), "fdWrite socket is not in the set\n");
    ok(!apc_count, "APC was called\n");
    SleepEx(0, TRUE);
    ok(apc_count == 1, "got apc_count %d.\n", apc_count);

    /* select the same socket twice */
    writefds.fd_count = 2;
    writefds.fd_array[0] = fdWrite;
    writefds.fd_array[1] = fdWrite;
    ret = select(0, NULL, &writefds, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    ok(writefds.fd_count == 1, "got count %u\n", writefds.fd_count);
    ok(writefds.fd_array[0] == fdWrite, "got fd %#Ix\n", writefds.fd_array[0]);
    ok(writefds.fd_array[1] == fdWrite, "got fd %#Ix\n", writefds.fd_array[1]);

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

#ifdef __REACTOS__
    /* ReactOS infinitely loops here */
    for(i = 0; i < 10000; i++) {
#else
    while(1) {
#endif
        FD_ZERO(&writefds);
        FD_SET(fdWrite, &writefds);
        ret = select(fdWrite+1, NULL, &writefds, NULL, &select_timeout);
        if(!ret) break;
        ok(send(fdWrite, tmp_buf, sizeof(tmp_buf), 0) > 0, "failed to send data\n");
    }
#ifdef __REACTOS__
    ok(i < 10000, "select() never had time limit expire.\n");
#endif
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

    page_pair = VirtualAlloc(NULL, 0x1000 * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    VirtualProtect(page_pair + 0x1000, 0x1000, PAGE_NOACCESS, &old_protect);
    alloc_fds = (fd_set *)((page_pair + 0x1000) - offsetof(fd_set, fd_array[1]));
    alloc_fds->fd_count = 1;
    alloc_fds->fd_array[0] = fdRead;
    ret = select(fdRead+1, alloc_fds, NULL, NULL, &select_timeout);
    ok(ret == 1, "select returned %d\n", ret);
    VirtualFree(page_pair, 0, MEM_RELEASE);

    closesocket(fdRead);
    closesocket(fdWrite);

    alloc_fds = malloc(offsetof(fd_set, fd_array[ARRAY_SIZE(sockets)]));
    alloc_fds->fd_count = ARRAY_SIZE(sockets);
    for (i = 0; i < ARRAY_SIZE(sockets); i += 2)
    {
        tcp_socketpair(&sockets[i], &sockets[i + 1]);
        alloc_fds->fd_array[i] = sockets[i];
        alloc_fds->fd_array[i + 1] = sockets[i + 1];
    }
    ret = select(0, NULL, alloc_fds, NULL, &select_timeout);
    ok(ret == ARRAY_SIZE(sockets), "got %d\n", ret);
    for (i = 0; i < ARRAY_SIZE(sockets); ++i)
    {
        ok(alloc_fds->fd_array[i] == sockets[i], "got socket %#Ix at index %u\n", alloc_fds->fd_array[i], i);
        closesocket(sockets[i]);
    }
    free(alloc_fds);

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
    ok(id == 0, "expected 0, got %ld\n", id);
    set_blocking(fdRead, FALSE);

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
    ok(!FD_ISSET(fdRead, &exceptfds), "fdRead socket is in the set\n");
    FD_ZERO_ALL();
    FD_SET_ALL(fdRead);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 2, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdRead, &readfds), "fdRead socket is not in the set\n");
    ok(FD_ISSET(fdRead, &writefds), "fdRead socket is not in the set\n");
    ok(!FD_ISSET(fdRead, &exceptfds), "fdRead socket is in the set\n");
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
    ok(GetLastError() == WSAEINVAL, "expected 10022, got %ld\n", GetLastError());
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(tmp_buf[0] == 'A', "expected 'A', got 0x%02X\n", tmp_buf[0]);

    /* Linux has some odd behaviour (probably a bug) where receiving OOB,
     * setting SO_OOBINLINE, and then calling recv() again will cause the same
     * data to be received twice. Avoid that messing with further tests by
     * calling recv() here. */
    ret = recv(fdRead, tmp_buf, sizeof(tmp_buf), 0);
    todo_wine ok(ret == -1, "got %d\n", ret);
    todo_wine ok(GetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

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
    ret = closesocket(fdWrite);
    ok(ret == 0, "expected 0, got %d\n", ret);

    /* w10pro64 sometimes takes over 2 seconds for an error to be reported. */
    if (winetest_interactive)
    {
        const struct sockaddr_in invalid_addr =
        {
            .sin_family = AF_INET,
            .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
            .sin_port = 255,
        };
        SOCKET client2, server2;

        fdWrite = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        set_blocking(fdWrite, FALSE);

        ret = connect(fdWrite, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 10;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "expected 1, got %d\n", ret);
        ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");
        ok(select_timeout.tv_usec == 250000, "select timeout should not have changed\n");

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(id == WSAECONNREFUSED, "got error %lu\n", id);

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(id == WSAECONNREFUSED, "got error %lu\n", id);

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 10;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "got %d\n", ret);
        ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");

        /* Calling connect() doesn't reset the socket error, but a successful
         * connection does. This is kind of tricky to test, because while
         * Windows takes a couple seconds to actually fail the connection,
         * Linux will fail the connection almost immediately. */

        ret = connect(fdWrite, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(id == WSAECONNREFUSED, "got error %lu\n", id);

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 10;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "got %d\n", ret);
        ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");

        len = sizeof(address);
        ret = getsockname(fdListen, (struct sockaddr *)&address, &len);
        ok(!ret, "got error %u\n", WSAGetLastError());
        ret = connect(fdWrite, (const struct sockaddr *)&address, sizeof(address));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 1;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "expected 1, got %d\n", ret);
        ok(FD_ISSET(fdWrite, &writefds), "fdWrite socket is not in the set\n");

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(!id, "got error %lu\n", id);

        closesocket(fdWrite);

        /* Test listening after a failed connection. */

        fdWrite = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        set_blocking(fdWrite, FALSE);

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        ret = bind(fdWrite, (struct sockaddr *)&address, sizeof(address));
        ok(!ret, "got %d\n", ret);

        ret = connect(fdWrite, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        FD_ZERO(&exceptfds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 10;
        ret = select(0, NULL, NULL, &exceptfds, &select_timeout);
        ok(ret == 1, "expected 1, got %d\n", ret);

        len = sizeof(address);
        ret = getsockname(fdWrite, (struct sockaddr *)&address, &len);
        ok(!ret, "got error %lu\n", GetLastError());

        /* Linux seems to forbid this. We'd need to replace the underlying fd. */
        ret = listen(fdWrite, 1);
        todo_wine ok(!ret, "got error %lu\n", GetLastError());

        if (!ret)
        {
            client2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            ret = connect(client2, (struct sockaddr *)&address, sizeof(address));
            ok(!ret, "got error %lu\n", GetLastError());

            server2 = accept(fdWrite, NULL, NULL);
            ok(server2 != INVALID_SOCKET, "got %d\n", ret);

            closesocket(server2);
            closesocket(client2);
        }

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(id == WSAECONNREFUSED, "got error %lu\n", id);

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 0;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "got %d\n", ret);
        ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");

        closesocket(fdWrite);

        /* test polling after a (synchronous) failure */

        fdWrite = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ret = connect(fdWrite, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAECONNREFUSED, "got error %u\n", WSAGetLastError());

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        todo_wine ok(!id, "got error %lu\n", id);

        FD_ZERO_ALL();
        FD_SET(fdWrite, &readfds);
        FD_SET(fdWrite, &writefds);
        FD_SET(fdWrite, &exceptfds);
        select_timeout.tv_sec = 0;
        ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
        ok(ret == 1, "expected 1, got %d\n", ret);
        ok(FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is not in the set\n");

        len = sizeof(id);
        id = 0xdeadbeef;
        ret = getsockopt(fdWrite, SOL_SOCKET, SO_ERROR, (char *)&id, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        todo_wine ok(!id, "got error %lu\n", id);

        closesocket(fdWrite);
    }

    ret = closesocket(fdListen);
    ok(ret == 0, "expected 0, got %d\n", ret);

    select_timeout.tv_sec = 1;
    select_timeout.tv_usec = 250000;

    /* Try select() on a closed socket after connection */
    tcp_socketpair(&fdRead, &fdWrite);
    closesocket(fdRead);
    FD_ZERO_ALL();
    FD_SET_ALL(fdWrite);
    FD_SET_ALL(fdRead);
    SetLastError(0xdeadbeef);
    ret = select(0, &readfds, NULL, &exceptfds, &select_timeout);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(GetLastError() == WSAENOTSOCK, "got %ld\n", GetLastError());
    /* descriptor sets are unchanged */
    ok(readfds.fd_count == 2, "expected 2, got %d\n", readfds.fd_count);
    ok(exceptfds.fd_count == 2, "expected 2, got %d\n", exceptfds.fd_count);
    closesocket(fdWrite);

    /* Close the socket currently being selected in a thread - bug 38399 */
    tcp_socketpair(&fdRead, &fdWrite);
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %ld\n", GetLastError());
    FD_ZERO_ALL();
    FD_SET_ALL(fdWrite);
    ret = select(0, &readfds, NULL, &exceptfds, &select_timeout);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok(FD_ISSET(fdWrite, &readfds), "fdWrite socket is not in the set\n");
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);
    /* test again with only the except descriptor */
    tcp_socketpair(&fdRead, &fdWrite);
    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &fdWrite, 0, &id);
    ok(thread_handle != NULL, "CreateThread failed unexpectedly: %ld\n", GetLastError());
    FD_ZERO_ALL();
    FD_SET(fdWrite, &exceptfds);
    SetLastError(0xdeadbeef);
    ret = select(0, NULL, NULL, &exceptfds, &select_timeout);
    ok(ret == SOCKET_ERROR, "expected -1, got %d\n", ret);
    ok(GetLastError() == WSAENOTSOCK, "got %ld\n", GetLastError());
    ok(!FD_ISSET(fdWrite, &exceptfds), "fdWrite socket is in the set\n");
    WaitForSingleObject (thread_handle, 1000);
    closesocket(fdRead);

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
    ok(server_socket != INVALID_SOCKET, "failed to bind socket, error %u\n", WSAGetLastError());

    val = 1;
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));
    ok(!ret, "failed to set SO_REUSEADDR, error %u\n", WSAGetLastError());

    ret = bind(server_socket, (struct sockaddr *)addr, *len);
    ok(!ret, "failed to bind socket, error %u\n", WSAGetLastError());

    ret = getsockname(server_socket, (struct sockaddr *)addr, len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(server_socket, 5);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    return server_socket;
}

static SOCKET setup_connector_socket(const struct sockaddr_in *addr, int len, BOOL nonblock)
{
    int ret;
    SOCKET connector;

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create connector socket %d\n", WSAGetLastError());

    if (nonblock)
        set_blocking(connector, !nonblock);

    ret = connect(connector, (const struct sockaddr *)addr, len);
    if (!nonblock)
        ok(!ret, "connecting to accepting socket failed %d\n", WSAGetLastError());
    else if (ret == SOCKET_ERROR)
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

    return connector;
}

struct connect_apc_func_param
{
    HANDLE event;
    struct sockaddr_in addr;
    SOCKET connector;
    unsigned int apc_count;
};

static DWORD WINAPI test_accept_connect_thread(void *param)
{
    struct connect_apc_func_param *p = (struct connect_apc_func_param *)param;

    WaitForSingleObject(p->event, INFINITE);
    p->connector = setup_connector_socket(&p->addr, sizeof(p->addr), FALSE);
    ok(p->connector != INVALID_SOCKET, "failed connecting from APC func.\n");
    return 0;
}

static void WINAPI connect_apc_func(ULONG_PTR param)
{
    struct connect_apc_func_param *p = (struct connect_apc_func_param *)param;

    ++p->apc_count;
    SetEvent(p->event);
}

static void test_accept(void)
{
    int ret;
    SOCKET server_socket, accepted = INVALID_SOCKET, connector;
    struct connect_apc_func_param apc_param;
    struct sockaddr_in address;
    SOCKADDR_STORAGE ss, ss_empty;
    int socklen;
    select_thread_params thread_params;
    HANDLE thread_handle = NULL;
    DWORD id;

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This test hangs on ReactOS!\n");
        return;
    }
#endif
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;

    socklen = sizeof(address);
    server_socket = setup_server_socket(&address, &socklen);

    memset(&apc_param, 0, sizeof(apc_param));
    apc_param.event = CreateEventW(NULL, FALSE, FALSE, NULL);
    apc_param.addr = address;
    /* Connecting directly from APC function randomly crashes on Windows for some reason,
     * so do it from a thread and only signal it from the APC when we are in accept() call. */
    thread_handle = CreateThread(NULL, 0, test_accept_connect_thread, &apc_param, 0, NULL);
    ret = QueueUserAPC(connect_apc_func, GetCurrentThread(), (ULONG_PTR)&apc_param);
    ok(ret, "QueueUserAPC returned %d\n", ret);
    accepted = accept(server_socket, NULL, NULL);
    ok(accepted != INVALID_SOCKET, "Failed to accept connection, %d\n", WSAGetLastError());
    ok(apc_param.apc_count == 1, "APC was called %u times\n", apc_param.apc_count);
    closesocket(accepted);
    closesocket(apc_param.connector);
    WaitForSingleObject(thread_handle, INFINITE);
    CloseHandle(thread_handle);
    CloseHandle(apc_param.event);

    connector = setup_connector_socket(&address, socklen, FALSE);
    if (connector == INVALID_SOCKET) goto done;

    accepted = WSAAccept(server_socket, NULL, NULL, AlwaysDeferConditionFunc, 0);
    ok(accepted == INVALID_SOCKET && WSAGetLastError() == WSATRY_AGAIN, "Failed to defer connection, %d\n", WSAGetLastError());

    accepted = accept(server_socket, NULL, 0);
    ok(accepted != INVALID_SOCKET, "Failed to accept deferred connection, error %d\n", WSAGetLastError());

    server_ready = CreateEventA(NULL, TRUE, FALSE, NULL);

    thread_params.s = server_socket;
    thread_params.ReadKilled = FALSE;
    thread_handle = CreateThread(NULL, 0, AcceptKillThread, &thread_params, 0, &id);

    WaitForSingleObject(server_ready, INFINITE);
    Sleep(200);
    ret = closesocket(server_socket);
    ok(!ret, "failed to close socket, error %u\n", WSAGetLastError());

    WaitForSingleObject(thread_handle, 1000);
    ok(thread_params.ReadKilled, "closesocket did not wake up accept\n");

    closesocket(accepted);
    closesocket(connector);
    accepted = connector = INVALID_SOCKET;

    socklen = sizeof(address);
    server_socket = setup_server_socket(&address, &socklen);

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

/* Test what socket state is inherited from the listening socket by accept(). */
static void test_accept_inheritance(void)
{
    struct sockaddr_in addr, destaddr;
    SOCKET listener, server, client;
    struct linger linger;
    int ret, len, value;
    unsigned int i;

    static const struct
    {
        int optname;
        int optval;
        int value;
    }
    int_tests[] =
    {
        {SOL_SOCKET, SO_REUSEADDR, 1},
        {SOL_SOCKET, SO_KEEPALIVE, 1},
        {SOL_SOCKET, SO_OOBINLINE, 1},
        {SOL_SOCKET, SO_SNDBUF, 0x123},
        {SOL_SOCKET, SO_RCVBUF, 0x123},
        {SOL_SOCKET, SO_SNDTIMEO, 0x123},
        {SOL_SOCKET, SO_RCVTIMEO, 0x123},
        {IPPROTO_TCP, TCP_NODELAY, 1},
    };

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());

    for (i = 0; i < ARRAY_SIZE(int_tests); ++i)
    {
        ret = setsockopt(listener, int_tests[i].optname, int_tests[i].optval,
                (char *)&int_tests[i].value, sizeof(int_tests[i].value));
        ok(!ret, "test %u: got error %u\n", i, WSAGetLastError());
    }

    linger.l_onoff = 1;
    linger.l_linger = 555;
    ret = setsockopt(listener, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
    ok(!ret, "got error %u\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(listener, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    for (i = 0; i < ARRAY_SIZE(int_tests); ++i)
    {
        value = 0;
        len = sizeof(value);
        ret = getsockopt(server, int_tests[i].optname, int_tests[i].optval, (char *)&value, &len);
        ok(!ret, "test %u: got error %u\n", i, WSAGetLastError());
        ok(value == int_tests[i].value, "test %u: got value %#x\n", i, value);
    }

    len = sizeof(linger);
    memset(&linger, 0, sizeof(linger));
    ret = getsockopt(server, SOL_SOCKET, SO_LINGER, (char *)&linger, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(linger.l_onoff == 1, "got on/off %u\n", linger.l_onoff);
    ok(linger.l_linger == 555, "got linger %u\n", linger.l_onoff);

    closesocket(server);
    closesocket(client);
    closesocket(listener);
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

    ret = WSAStartup(MAKEWORD(2,0), &wsa);
    ok(!ret, "failed to startup, error %u\n", WSAGetLastError());

    memset(&sa, 0, sa_len);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    ret = bind(sock, (struct sockaddr *) &sa, sa_len);
    ok(!ret, "failed to bind socket, error %u\n", WSAGetLastError());

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

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    ret = bind(sock, (struct sockaddr *) &sa, sa_len);
    ok(!ret, "failed to bind socket, error %u\n", WSAGetLastError());

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

    ret = WSAStartup(MAKEWORD(2,0), &wsa);
    ok(!ret, "failed to startup, error %u\n", WSAGetLastError());

    memset(&sa_set, 0, sa_set_len);

    sa_set.sin_family = AF_INET;
    sa_set.sin_port = htons(0);
    sa_set.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    sa_get = sa_set;
    WSASetLastError(0xdeadbeef);
    ret = getsockname(sock, (struct sockaddr *)&sa_get, &sa_get_len);
    ok(ret == SOCKET_ERROR, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&sa_get, &sa_set, sizeof(sa_get)), "address should not be changed\n");

    ret = bind(sock, (struct sockaddr *) &sa_set, sa_set_len);
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    memset(&sa_get, 0, sizeof(sa_get));
    ret = getsockname(sock, (struct sockaddr *) &sa_get, &sa_get_len);
    ok(!ret, "got %d\n", ret);
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());
    ok(sa_get.sin_family == AF_INET, "got family %#x\n", sa_get.sin_family);
    ok(sa_get.sin_port != 0, "got zero port\n");
    ok(sa_get.sin_addr.s_addr == INADDR_ANY, "got addr %08lx\n", sa_get.sin_addr.s_addr);

    ret = memcmp(sa_get.sin_zero, null_padding, 8);
    ok(ret == 0, "getsockname did not zero the sockaddr_in structure\n");

    sa_get_len = sizeof(sa_get) - 1;
    WSASetLastError(0xdeadbeef);
    ret = getsockname(sock, (struct sockaddr *)&sa_get, &sa_get_len);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(sa_get_len == sizeof(sa_get) - 1, "got size %d\n", sa_get_len);

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
            ok(sock != INVALID_SOCKET, "socket failed with %ld\n", GetLastError());

            memset(&sa_set, 0, sizeof(sa_set));
            sa_set.sin_family = AF_INET;
            sa_set.sin_addr.s_addr = ip.s_addr;
            /* The same address we bind must be the same address we get */
            ret = bind(sock, (struct sockaddr*)&sa_set, sizeof(sa_set));
            ok(ret == 0, "bind failed with %ld\n", GetLastError());
            sa_get_len = sizeof(sa_get);
            ret = getsockname(sock, (struct sockaddr*)&sa_get, &sa_get_len);
            ok(ret == 0, "getsockname failed with %ld\n", GetLastError());
            strcpy(ipstr, inet_ntoa(sa_get.sin_addr));
            ok(sa_get.sin_addr.s_addr == sa_set.sin_addr.s_addr,
               "address does not match: %s != %s\n", ipstr, inet_ntoa(sa_set.sin_addr));

            closesocket(sock);
        }
    }

    WSACleanup();
}

static DWORD apc_error, apc_size;
static OVERLAPPED *apc_overlapped;
static unsigned int apc_count;

static void WINAPI socket_apc(DWORD error, DWORD size, OVERLAPPED *overlapped, DWORD flags)
{
    ok(!flags, "got flags %#lx\n", flags);
    ++apc_count;
    apc_error = error;
    apc_size = size;
    apc_overlapped = overlapped;
}

#define check_fionread_siocatmark(a, b, c) check_fionread_siocatmark_(__LINE__, a, b, c, FALSE, FALSE)
#define check_fionread_siocatmark_todo(a, b, c) check_fionread_siocatmark_(__LINE__, a, b, c, TRUE, TRUE)
#define check_fionread_siocatmark_todo_oob(a, b, c) check_fionread_siocatmark_(__LINE__, a, b, c, FALSE, TRUE)
static void check_fionread_siocatmark_(int line, SOCKET s, unsigned int normal, unsigned int oob,
        BOOL todo_normal, BOOL todo_oob)
{
    int ret, value;
    DWORD size;

    value = 0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, FIONREAD, NULL, 0, &value, sizeof(value), &size, NULL, NULL);
    ok_(__FILE__, line)(!ret, "expected success\n");
    ok_(__FILE__, line)(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    todo_wine_if (todo_normal) ok_(__FILE__, line)(value == normal, "FIONBIO returned %u\n", value);

    value = 0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, SIOCATMARK, NULL, 0, &value, sizeof(value), &size, NULL, NULL);
    ok_(__FILE__, line)(!ret, "expected success\n");
    ok_(__FILE__, line)(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    todo_wine_if (todo_oob) ok_(__FILE__, line)(value == oob, "SIOCATMARK returned %u\n", value);
}

static void test_fionread_siocatmark(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    SOCKET client, server;
    char buffer[5];
    int ret, value;
    ULONG_PTR key;
    HANDLE port;
    DWORD size;

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    WSASetLastError(0xdeadbeef);
    ret = ioctlsocket(client, FIONREAD, (u_long *)1);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = ioctlsocket(client, SIOCATMARK, (u_long *)1);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(client, FIONREAD, NULL, 0, &value, sizeof(value), NULL, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(client, FIONREAD, NULL, 0, &value, sizeof(value) - 1, &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(client, SIOCATMARK, NULL, 0, &value, sizeof(value), NULL, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(client, SIOCATMARK, NULL, 0, &value, sizeof(value) - 1, &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    check_fionread_siocatmark(client, 0, TRUE);

    port = CreateIoCompletionPort((HANDLE)client, NULL, 123, 0);

    ret = WSAIoctl(client, FIONREAD, NULL, 0, &value, sizeof(value), NULL, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(client, SIOCATMARK, NULL, 0, &value, sizeof(value), NULL, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    value = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(client, FIONREAD, NULL, 0, &value, sizeof(value), &size, &overlapped, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(!value, "got %u\n", value);
    ok(size == sizeof(value), "got size %lu\n", size);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(key == 123, "got key %Iu\n", key);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    value = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(client, SIOCATMARK, NULL, 0, &value, sizeof(value), &size, &overlapped, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(value == TRUE, "got %u\n", value);
    ok(size == sizeof(value), "got size %lu\n", size);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(key == 123, "got key %Iu\n", key);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    /* wait for the data to be available */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask(client, POLLRDNORM, POLLRDNORM);

    check_fionread_siocatmark(client, 5, TRUE);

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    /* wait for the data to be available */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask(client, POLLRDBAND, POLLRDBAND);

#ifdef __REACTOS__
    check_fionread_siocatmark(client, 5, FALSE);
#else
    check_fionread_siocatmark_todo_oob(client, 5, FALSE);
#endif

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_fionread_siocatmark_todo(client, 5, FALSE);

    ret = recv(client, buffer, 3, 0);
    ok(ret == 3, "got %d\n", ret);

    check_fionread_siocatmark_todo(client, 2, FALSE);

    ret = recv(client, buffer, 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    /* wait for the data to be available */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask_todo(client, POLLRDBAND, POLLRDBAND);

    check_fionread_siocatmark_todo(client, 2, FALSE);

    ret = recv(client, buffer, 5, 0);
    todo_wine ok(ret == 2, "got %d\n", ret);

    check_fionread_siocatmark(client, 0, FALSE);

    ret = recv(client, buffer, 1, MSG_OOB);
    todo_wine ok(ret == 1, "got %d\n", ret);

    check_fionread_siocatmark_todo_oob(client, 0, TRUE);

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    /* wait for the data to be available */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask(client, POLLRDBAND, POLLRDBAND);

    ret = 1;
    ret = setsockopt(client, SOL_SOCKET, SO_OOBINLINE, (char *)&ret, sizeof(ret));
    ok(!ret, "got error %u\n", WSAGetLastError());

#ifdef __REACTOS__
    if (GetNTVersion() >= _WIN32_WINNT_VISTA) /* Not consistent on WS03 */
#endif
    check_fionread_siocatmark_todo_oob(client, 1, FALSE);

    ret = recv(client, buffer, 1, 0);
    ok(ret == 1, "got %d\n", ret);

    check_fionread_siocatmark(client, 0, TRUE);

    ret = send(server, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    /* wait for the data to be available */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask(client, POLLRDNORM, POLLRDNORM);

    check_fionread_siocatmark(client, 1, TRUE);

    closesocket(client);
    closesocket(server);

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    check_fionread_siocatmark(server, 0, TRUE);

    ret = bind(server, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_fionread_siocatmark(server, 0, TRUE);

    closesocket(server);
    CloseHandle(overlapped.hEvent);

    /* test with APCs */

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = WSAIoctl(server, FIONREAD, NULL, 0, &value, sizeof(value), NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(server, SIOCATMARK, NULL, 0, &value, sizeof(value), NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(server, FIONREAD, NULL, 0, &value, sizeof(value), &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
    ok(size == sizeof(value), "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(server, SIOCATMARK, NULL, 0, &value, sizeof(value), &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
    ok(size == sizeof(value), "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    closesocket(server);
}

static void test_fionbio(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    u_long one = 1, zero = 0;
    HANDLE port, event;
    ULONG_PTR key;
    void *output;
    DWORD size;
    SOCKET s;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

    WSASetLastError(0xdeadbeef);
    ret = ioctlsocket(s, FIONBIO, (u_long *)1);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, NULL, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one) - 1, NULL, 0, &size, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    size = 0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, &size, NULL, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(!size, "got size %lu\n", size);

    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one) + 1, NULL, 0, &size, NULL, NULL);
    ok(!ret, "got error %u\n", WSAGetLastError());

    output = VirtualAlloc(NULL, 4, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one) + 1, output, 4, &size, NULL, NULL);
    ok(!ret, "got error %u\n", WSAGetLastError());
    VirtualFree(output, 0, MEM_FREE);

    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, &size, &overlapped, NULL);
    ok(!ret, "expected success\n");
    ok(!size, "got size %lu\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, NULL, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    ret = WSAEventSelect(s, event, FD_READ);
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, &size, NULL, NULL);
    ok(!ret, "got error %u\n", WSAGetLastError());

    size = 0xdeadbeef;
    ret = WSAIoctl(s, FIONBIO, &zero, sizeof(zero), NULL, 0, &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    todo_wine ok(!size, "got size %lu\n", size);

    CloseHandle(port);
    closesocket(s);
    CloseHandle(event);

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, FIONBIO, &one, sizeof(one), NULL, 0, &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
    ok(!size, "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    closesocket(s);
}

static void test_keepalive_vals(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    struct tcp_keepalive kalive;
    ULONG_PTR key;
    HANDLE port;
    SOCKET sock;
    DWORD size;
    int ret;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "Creating the socket failed: %d\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)sock, NULL, 123, 0);

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, 0, NULL, 0, &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
#ifdef __REACTOS__
    ok(WSAGetLastError() == WSAEFAULT || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
    ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "got size %lu\n", size);
#else
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(!size, "got size %lu\n", size);
#endif

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, NULL, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "got size %lu\n", size);
#else
    ok(!size, "got size %lu\n", size);
#endif

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    make_keepalive(kalive, 0, 0, 0);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "got size %lu\n", size);
#else
    ok(!size, "got size %lu\n", size);
#endif

    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, NULL, &overlapped, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive) - 1, NULL, 0, &size, &overlapped, NULL);
    ok(ret == SOCKET_ERROR, "WSAIoctl succeeded unexpectedly\n");
#ifdef __REACTOS__
    ok(WSAGetLastError() == WSAEFAULT || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
#endif
    ok(size == 0xdeadbeef, "got size %lu\n", size);
#ifdef __REACTOS__
    todo_wine ok(overlapped.Internal == STATUS_PENDING || broken(overlapped.Internal == 0xdeadbeef) /* WS03 */, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
#else
    todo_wine ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
#endif
    ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, &overlapped, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    todo_wine ok(size == 0xdeadbeef, "got size %lu\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    make_keepalive(kalive, 1, 0, 0);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 1000, 1000);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 10000, 10000);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 1, 100, 100);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    make_keepalive(kalive, 0, 100, 100);
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, NULL, NULL);
    ok(ret == 0, "WSAIoctl failed unexpectedly\n");

    CloseHandle(port);
    closesocket(sock);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &kalive, sizeof(kalive), NULL, 0, &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
#ifdef __REACTOS__
    ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "got size %lu\n", size);
#else
    ok(!size, "got size %lu\n", size);
#endif

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    closesocket(sock);
}

static void test_unsupported_ioctls(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    unsigned int i;
    ULONG_PTR key;
    HANDLE port;
    DWORD size;
    SOCKET s;
    int ret;

    static const DWORD codes[] = {0xdeadbeef, FIOASYNC, 0x667e, SIO_FLUSH};

    for (i = 0; i < ARRAY_SIZE(codes); ++i)
    {
        winetest_push_context("ioctl %#lx", codes[i]);
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

        WSASetLastError(0xdeadbeef);
        ret = WSAIoctl(s, codes[i], NULL, 0, NULL, 0, NULL, &overlapped, NULL);
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

        WSASetLastError(0xdeadbeef);
        size = 0xdeadbeef;
        ret = WSAIoctl(s, codes[i], NULL, 0, NULL, 0, &size, NULL, NULL);
        ok(ret == -1, "expected failure\n");
#ifdef __REACTOS__
        ok(WSAGetLastError() == WSAEOPNOTSUPP || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
        ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "got size %lu\n", size);
#else
        ok(WSAGetLastError() == WSAEOPNOTSUPP, "got error %u\n", WSAGetLastError());
        ok(!size, "got size %lu\n", size);
#endif

        WSASetLastError(0xdeadbeef);
        size = 0xdeadbeef;
        overlapped.Internal = 0xdeadbeef;
        overlapped.InternalHigh = 0xdeadbeef;
        ret = WSAIoctl(s, codes[i], NULL, 0, NULL, 0, &size, &overlapped, NULL);
        ok(ret == -1, "expected failure\n");
#ifdef __REACTOS__
        ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
        ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
#endif
        ok(size == 0xdeadbeef, "got size %lu\n", size);

        ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
        ok(!ret, "expected failure\n");
#ifdef __REACTOS__
        if (GetLastError() != WAIT_TIMEOUT) {
#endif
        ok(GetLastError() == ERROR_NOT_SUPPORTED, "got error %lu\n", GetLastError());
        ok(!size, "got size %lu\n", size);
        ok(key == 123, "got key %Iu\n", key);
        ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
        ok((NTSTATUS)overlapped.Internal == STATUS_NOT_SUPPORTED,
                "got status %#lx\n", (NTSTATUS)overlapped.Internal);
        ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);
#ifdef __REACTOS__
        }
#endif

        CloseHandle(port);
        closesocket(s);

        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ret = WSAIoctl(s, codes[i], NULL, 0, NULL, 0, NULL, &overlapped, socket_apc);
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

        apc_count = 0;
        size = 0xdeadbeef;
        ret = WSAIoctl(s, codes[i], NULL, 0, NULL, 0, &size, &overlapped, socket_apc);
        ok(ret == -1, "expected failure\n");
#ifdef __REACTOS__
        ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
        ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
#endif
        ok(size == 0xdeadbeef, "got size %lu\n", size);

        ret = SleepEx(0, TRUE);
#ifdef __REACTOS__
        if (ret) { /* > WS03 */
#endif
        ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
        ok(apc_count == 1, "APC was called %u times\n", apc_count);
        ok(apc_error == WSAEOPNOTSUPP, "got APC error %lu\n", apc_error);
        ok(!apc_size, "got APC size %lu\n", apc_size);
        ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);
#ifdef __REACTOS__
        }
#endif

        closesocket(s);
        winetest_pop_context();
    }
}

static void test_get_extension_func(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    GUID acceptex_guid = WSAID_ACCEPTEX;
    GUID bogus_guid = {0xdeadbeef};
    ULONG_PTR key;
    HANDLE port;
    DWORD size;
    void *func;
    SOCKET s;
    int ret;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(GUID),
            &func, sizeof(func), NULL, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(GUID),
            &func, sizeof(func), &size, NULL, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(size == sizeof(func), "got size %lu\n", size);

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(GUID),
            &func, sizeof(func), &size, &overlapped, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(size == sizeof(func), "got size %lu\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(key == 123, "got key %Iu\n", key);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &bogus_guid, sizeof(GUID),
            &func, sizeof(func), &size, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);
    ok(overlapped.Internal == 0xdeadbeef, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %u\n", WSAGetLastError());

    CloseHandle(port);
    closesocket(s);

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(GUID),
            &func, sizeof(func), NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(GUID),
            &func, sizeof(func), &size, &overlapped, socket_apc);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(size == sizeof(func), "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    closesocket(s);
}

static void test_backlog_query(void)
{
    const struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    GUID acceptex_guid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX pAcceptEx;
    struct sockaddr_in destaddr;
    DWORD size;
    SOCKET s, listener;
    int len, ret;
    ULONG backlog = 0;

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());

    ret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
            &pAcceptEx, sizeof(pAcceptEx), &size, NULL, NULL);
    ok(!ret, "failed to get AcceptEx, error %u\n", WSAGetLastError());

    ret = bind(listener, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 2);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY, NULL, 0, &backlog, sizeof(backlog), &size, NULL, NULL);
#ifdef __REACTOS__
    ok(ret == SOCKET_ERROR && (WSAGetLastError() == WSAENOTCONN || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */),
       "WSAIoctl() failed: %d/%d\n", ret, WSAGetLastError());
#else
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAENOTCONN,
       "WSAIoctl() failed: %d/%d\n", ret, WSAGetLastError());
#endif

    ret = connect(s, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    ret = WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY, NULL, 0, &backlog, sizeof(backlog), &size, NULL, NULL);
#ifdef __REACTOS__
    ok(!ret || broken(ret == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL) /* WS03 */, "WSAIoctl() failed: %d\n", WSAGetLastError());
    if (!ret)
        ok(backlog == 0x10000, "got %08lx\n", backlog);
#else
    ok(!ret, "WSAIoctl() failed: %d\n", WSAGetLastError());
    ok(backlog == 0x10000, "got %08lx\n", backlog);
#endif

    closesocket(listener);
    closesocket(s);

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    backlog = 0;
    ret = WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY, NULL, 0, &backlog, sizeof(backlog), &size, NULL, NULL);
#ifdef __REACTOS__
    ok(ret == SOCKET_ERROR && (WSAGetLastError() == WSAEOPNOTSUPP || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */),
       "WSAIoctl() failed: %d/%d\n", ret, WSAGetLastError());
#else
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEOPNOTSUPP,
       "WSAIoctl() failed: %d/%d\n", ret, WSAGetLastError());
#endif
    closesocket(s);
}

static void test_base_handle(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    unsigned int i;
    SOCKET s, base;
    ULONG_PTR key;
    HANDLE port;
    DWORD size;
    int ret;

    static const struct
    {
        int family, type, protocol;
    }
    tests[] =
    {
        {AF_INET, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET, SOCK_DGRAM, IPPROTO_UDP},
        {AF_INET6, SOCK_STREAM, IPPROTO_TCP},
        {AF_INET6, SOCK_DGRAM, IPPROTO_UDP},
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        s = socket(tests[i].family, tests[i].type, tests[i].protocol);
        if (s == INVALID_SOCKET) continue;
        port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

        WSASetLastError(0xdeadbeef);
        ret = WSAIoctl(s, SIO_BASE_HANDLE, NULL, 0, &base, sizeof(base), NULL, &overlapped, NULL);
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

        WSASetLastError(0xdeadbeef);
        size = 0xdeadbeef;
        base = 0xdeadbeef;
        ret = WSAIoctl(s, SIO_BASE_HANDLE, NULL, 0, &base, sizeof(base), &size, NULL, NULL);
#ifdef __REACTOS__
        if (!ret) { /* > WS03 */
#endif
        ok(!ret, "expected success\n");
        ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
        ok(size == sizeof(base), "got size %lu\n", size);
        ok(base == s, "expected %#Ix, got %#Ix\n", s, base);
#ifdef __REACTOS__
        }
#endif

        WSASetLastError(0xdeadbeef);
        size = 0xdeadbeef;
        base = 0xdeadbeef;
        overlapped.Internal = 0xdeadbeef;
        overlapped.InternalHigh = 0xdeadbeef;
        ret = WSAIoctl(s, SIO_BASE_HANDLE, NULL, 0, &base, sizeof(base), &size, &overlapped, NULL);
        ok(ret == -1, "expected failure\n");
#ifdef __REACTOS__
        ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
        ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
#endif
        ok(size == 0xdeadbeef, "got size %lu\n", size);

        ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
        ok(!ret, "expected failure\n");
#ifdef __REACTOS__
        if (GetLastError() != WAIT_TIMEOUT) { /* > WS03 */
#endif
        ok(GetLastError() == ERROR_NOT_SUPPORTED, "got error %lu\n", GetLastError());
        ok(!size, "got size %lu\n", size);
        ok(key == 123, "got key %Iu\n", key);
        ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
        ok((NTSTATUS)overlapped.Internal == STATUS_NOT_SUPPORTED, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
        ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);
        ok(base == 0xdeadbeef, "expected %#Ix, got %#Ix\n", s, base);
#ifdef __REACTOS__
        }
#endif

        CloseHandle(port);
        closesocket(s);

        s = socket(tests[i].family, tests[i].type, tests[i].protocol);

        ret = WSAIoctl(s, SIO_BASE_HANDLE, NULL, 0, &base, sizeof(base), NULL, &overlapped, socket_apc);
        ok(ret == -1, "expected failure\n");
        ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

        apc_count = 0;
        size = 0xdeadbeef;
        base = 0xdeadbeef;
        ret = WSAIoctl(s, SIO_BASE_HANDLE, NULL, 0, &base, sizeof(base), &size, &overlapped, socket_apc);
        ok(ret == -1, "expected failure\n");
#ifdef __REACTOS__
        ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEINVAL) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
        ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
#endif
        ok(size == 0xdeadbeef, "got size %lu\n", size);

        ret = SleepEx(0, TRUE);
#ifdef __REACTOS__
        if (ret) { /* > WS03 */
#endif
        ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
        ok(apc_count == 1, "APC was called %u times\n", apc_count);
        ok(apc_error == WSAEOPNOTSUPP, "got APC error %lu\n", apc_error);
        ok(!apc_size, "got APC size %lu\n", apc_size);
        ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);
        ok(base == 0xdeadbeef, "expected %#Ix, got %#Ix\n", s, base);
#ifdef __REACTOS__
        }
#endif

        closesocket(s);
    }
}

static void test_circular_queueing(void)
{
        SOCKET s;
        DWORD size;
        int ret;

        s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        ret = WSAIoctl(s, SIO_ENABLE_CIRCULAR_QUEUEING, NULL, 0, NULL, 0, &size, NULL, NULL);
        ok(!ret, "expected 0, got %d\n", ret);

        closesocket(s);
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

    memset(&ov, 0, sizeof(ov));

    tcp_socketpair(&src, &dst);

    set_blocking(dst, FALSE);
    /* force disable buffering so we can get a pending overlapped request */
    ret = setsockopt(dst, SOL_SOCKET, SO_SNDBUF, (char *) &zero, sizeof(zero));
    ok(!ret, "setsockopt SO_SNDBUF failed: %d - %ld\n", ret, GetLastError());

    hThread = CreateThread(NULL, 0, drain_socket_thread, &dst, 0, &id);

    buffer = malloc(buflen);

    /* fill the buffer with some nonsense */
    for (i = 0; i < buflen; ++i)
    {
        buffer[i] = (char) i;
    }

    ret = send(src, buffer, buflen, 0);
    ok(ret == buflen, "send should have sent %d bytes, but it only sent %d\n", buflen, ret);

    buf.buf = buffer;
    buf.len = buflen;

    ov.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %ld\n", GetLastError());
    if (!ov.hEvent)
        goto end;

    bytes_sent = 0;
    WSASetLastError(12345);
    ret = WSASend(dst, &buf, 1, &bytes_sent, 0, &ov, NULL);
    ok(ret == SOCKET_ERROR, "expected failure\n");
    ok(WSAGetLastError() == ERROR_IO_PENDING, "wrong error %u\n", WSAGetLastError());

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

        ok(ret == 1, "Failed to receive data %d - %ld (got %d/%d)\n", ret, GetLastError(), i, buflen);
        if (ret != 1)
            break;

        ok(buffer[0] == (char) i, "Received bad data at position %d\n", i);
    }

    dwRet = WaitForSingleObject(ov.hEvent, 1000);
    ok(dwRet == WAIT_OBJECT_0, "Failed to wait for recv message: %ld - %ld\n", dwRet, GetLastError());
    if (dwRet == WAIT_OBJECT_0)
    {
        bret = GetOverlappedResult((HANDLE)dst, &ov, &bytes_sent, FALSE);
        ok(bret && bytes_sent == buflen,
           "Got %ld instead of %d (%d - %ld)\n", bytes_sent, buflen, bret, GetLastError());
    }

    WSASetLastError(12345);
    ret = WSASend(INVALID_SOCKET, &buf, 1, NULL, 0, &ov, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK,
       "WSASend failed %d - %d\n", ret, WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASend(dst, &buf, 1, NULL, 0, &ov, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == ERROR_IO_PENDING,
       "Failed to start overlapped send %d - %d\n", ret, WSAGetLastError());

end:
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (dst != INVALID_SOCKET)
        closesocket(dst);
    if (hThread != NULL)
    {
        dwRet = WaitForSingleObject(hThread, 500);
        ok(dwRet == WAIT_OBJECT_0, "failed to wait for thread termination: %ld\n", GetLastError());
        CloseHandle(hThread);
    }
    if (ov.hEvent)
        CloseHandle(ov.hEvent);
    free(buffer);
}

#define WM_SOCKET (WM_USER+100)

struct event_test_ctx
{
    int is_message;
    SOCKET socket;
    HANDLE event;
    HWND window;
};

static void select_events(struct event_test_ctx *ctx, SOCKET socket, LONG events)
{
    int ret;

    if (ctx->is_message)
        ret = WSAAsyncSelect(socket, ctx->window, WM_USER, events);
    else
        ret = WSAEventSelect(socket, ctx->event, events);
    ok(!ret, "failed to select, error %u\n", WSAGetLastError());
    ctx->socket = socket;
}

#define check_events(a, b, c, d) check_events_(__LINE__, a, b, c, d, FALSE, FALSE)
#define check_events_todo(a, b, c, d) check_events_(__LINE__, a, b, c, d, TRUE, TRUE)
#define check_events_todo_event(a, b, c, d) check_events_(__LINE__, a, b, c, d, TRUE, FALSE)
#define check_events_todo_msg(a, b, c, d) check_events_(__LINE__, a, b, c, d, FALSE, TRUE)
static void check_events_(int line, struct event_test_ctx *ctx,
        LONG flag1, LONG flag2, DWORD timeout, BOOL todo_event, BOOL todo_msg)
{
    int ret;

    if (ctx->is_message)
    {
        BOOL any_fail = FALSE;
        MSG msg;

        if (flag1)
        {
            ret = PeekMessageA(&msg, ctx->window, WM_USER, WM_USER, PM_REMOVE);
            while (!ret && !MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_POSTMESSAGE))
                ret = PeekMessageA(&msg, ctx->window, WM_USER, WM_USER, PM_REMOVE);
            todo_wine_if (todo_msg && !ret) ok_(__FILE__, line)(ret, "expected a message\n");
            if (ret)
            {
                ok_(__FILE__, line)(msg.wParam == ctx->socket,
                        "expected wparam %#Ix, got %#Ix\n", ctx->socket, msg.wParam);
                todo_wine_if (todo_msg && msg.lParam != flag1)
                    ok_(__FILE__, line)(msg.lParam == flag1, "got first event %#Ix\n", msg.lParam);
                if (msg.lParam != flag1) any_fail = TRUE;
            }
            else
                any_fail = TRUE;
        }
        if (flag2)
        {
            ret = PeekMessageA(&msg, ctx->window, WM_USER, WM_USER, PM_REMOVE);
            while (!ret && !MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_POSTMESSAGE))
                ret = PeekMessageA(&msg, ctx->window, WM_USER, WM_USER, PM_REMOVE);
            ok_(__FILE__, line)(ret, "expected a message\n");
            ok_(__FILE__, line)(msg.wParam == ctx->socket, "got wparam %#Ix\n", msg.wParam);
            todo_wine_if (todo_msg) ok_(__FILE__, line)(msg.lParam == flag2, "got second event %#Ix\n", msg.lParam);
        }
        ret = PeekMessageA(&msg, ctx->window, WM_USER, WM_USER, PM_REMOVE);
        todo_wine_if (todo_msg && ret) ok_(__FILE__, line)(!ret, "got unexpected event %#Ix\n", msg.lParam);
        if (ret) any_fail = TRUE;

        /* catch tests which succeed */
        todo_wine_if (todo_msg) ok_(__FILE__, line)(!any_fail, "event series matches\n");
    }
    else
    {
        WSANETWORKEVENTS events;
        unsigned int i;

        memset(&events, 0xcc, sizeof(events));
        ret = WaitForSingleObject(ctx->event, timeout);
        if (flag1 | flag2)
            todo_wine_if (todo_event && ret) ok_(__FILE__, line)(!ret, "event wait timed out\n");
        else
            todo_wine_if (todo_event) ok_(__FILE__, line)(ret == WAIT_TIMEOUT, "expected timeout\n");
        ret = WSAEnumNetworkEvents(ctx->socket, ctx->event, &events);
        ok_(__FILE__, line)(!ret, "failed to get events, error %u\n", WSAGetLastError());
        todo_wine_if (todo_event)
            ok_(__FILE__, line)(events.lNetworkEvents == LOWORD(flag1 | flag2), "got events %#lx\n", events.lNetworkEvents);
        for (i = 0; i < ARRAY_SIZE(events.iErrorCode); ++i)
        {
            if ((1u << i) == LOWORD(flag1) && (events.lNetworkEvents & LOWORD(flag1)))
                ok_(__FILE__, line)(events.iErrorCode[i] == HIWORD(flag1),
                        "got error code %d for event %#x\n", events.iErrorCode[i], 1u << i);
            if ((1u << i) == LOWORD(flag2) && (events.lNetworkEvents & LOWORD(flag2)))
                ok_(__FILE__, line)(events.iErrorCode[i] == HIWORD(flag2),
                        "got error code %d for event %#x\n", events.iErrorCode[i], 1u << i);
        }
    }
}

static void test_accept_events(struct event_test_ctx *ctx)
{
    const struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    SOCKET listener, server, client, client2;
    GUID acceptex_guid = WSAID_ACCEPTEX;
    struct sockaddr_in destaddr;
    OVERLAPPED overlapped = {0};
    LPFN_ACCEPTEX pAcceptEx;
    char buffer[32];
    int len, ret;
    DWORD size;

    overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());

    ret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
            &pAcceptEx, sizeof(pAcceptEx), &size, NULL, NULL);
    ok(!ret, "failed to get AcceptEx, error %u\n", WSAGetLastError());

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);

    ret = bind(listener, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 2);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    check_events(ctx, 0, 0, 0);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    check_events(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);
    if (ctx->is_message)
        check_events(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, listener, 0);
    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);
    if (ctx->is_message)
        check_events(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);

    client2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client2 != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client2, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    if (!ctx->is_message)
        check_events_todo(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);

    check_events(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);

    check_events(ctx, 0, 0, 0);

    closesocket(client2);
    closesocket(client);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    check_events(ctx, FD_ACCEPT, 0, 200);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    check_events(ctx, 0, 0, 200);

    closesocket(listener);

    /* Connect and then select. */

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = bind(listener, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 2);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);

    check_events(ctx, FD_ACCEPT, 0, 200);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    /* As above, but select on a subset containing FD_ACCEPT first. */

    if (!ctx->is_message)
    {
        select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);

        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
        ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
        ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

        ret = WaitForSingleObject(ctx->event, 200);
        ok(!ret, "wait timed out\n");

        select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB);
        ret = WaitForSingleObject(ctx->event, 0);
        ok(!ret, "wait timed out\n");

        ResetEvent(ctx->event);

        select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB);
        ret = WaitForSingleObject(ctx->event, 0);
        ok(ret == WAIT_TIMEOUT, "expected timeout\n");

        select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);
        ret = WaitForSingleObject(ctx->event, 0);
        ok(!ret, "wait timed out\n");
        check_events(ctx, FD_ACCEPT, 0, 0);

        server = accept(listener, NULL, NULL);
        ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
        closesocket(server);
        closesocket(client);
    }

    /* As above, but select on a subset not containing FD_ACCEPT first. */

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);
    check_events(ctx, FD_ACCEPT, 0, 200);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    /* As above, but call accept() before selecting. */

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    Sleep(200);
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);
    check_events(ctx, 0, 0, 200);

    closesocket(server);
    closesocket(client);

    closesocket(listener);

    /* The socket returned from accept() inherits the same parameters. */

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = bind(listener, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 2);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT | FD_WRITE);
    check_events(ctx, FD_ACCEPT, 0, 200);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    ctx->socket = server;
    check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(server);
    closesocket(client);

    /* Connect while there is a pending AcceptEx(). */

    select_events(ctx, listener, FD_CONNECT | FD_READ | FD_OOB | FD_ACCEPT);

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = pAcceptEx(listener, server, buffer, 0, 0, sizeof(buffer), NULL, &overlapped);
    ok(!ret, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(!ret, "got %d\n", ret);
    ret = GetOverlappedResult((HANDLE)listener, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);

    check_events(ctx, 0, 0, 0);

    closesocket(server);
    closesocket(client);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_events(ctx, FD_ACCEPT, 0, 200);
    check_events(ctx, 0, 0, 0);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(server);
    closesocket(client);

    closesocket(listener);
    CloseHandle(overlapped.hEvent);
}

static void test_connect_events(struct event_test_ctx *ctx)
{
    const struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    SOCKET listener, server, client;
    struct sockaddr_in destaddr;
    int len, ret;

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = bind(listener, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 2);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());

    select_events(ctx, client, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    check_events(ctx, 0, 0, 0);

    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret || WSAGetLastError() == WSAEWOULDBLOCK, "failed to connect, error %u\n", WSAGetLastError());

    check_events(ctx, FD_CONNECT, FD_WRITE, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, client, 0);
    select_events(ctx, client, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    select_events(ctx, server, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    check_events(ctx, FD_WRITE, 0, 200);

    closesocket(client);
    closesocket(server);

    /* Connect and then select. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    select_events(ctx, client, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    else
        check_events(ctx, FD_CONNECT, FD_WRITE, 200);

    closesocket(client);
    closesocket(server);

    /* As above, but select on a subset not containing FD_CONNECT first. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());

    select_events(ctx, client, FD_ACCEPT | FD_CLOSE | FD_OOB | FD_READ | FD_WRITE);

    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret || WSAGetLastError() == WSAEWOULDBLOCK, "failed to connect, error %u\n", WSAGetLastError());

    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    check_events(ctx, FD_WRITE, 0, 200);

    select_events(ctx, client, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);

    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    else
        check_events(ctx, FD_CONNECT, 0, 200);

    closesocket(client);
    closesocket(server);

    /* Test with UDP sockets. */

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    select_events(ctx, client, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    check_events_todo_event(ctx, 0, 0, 0);

    ret = bind(server, (const struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    len = sizeof(destaddr);
    ret = getsockname(server, (struct sockaddr *)&destaddr, &len);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());

    if (ctx->is_message)
        check_events_todo(ctx, FD_WRITE, 0, 200);
    else
        check_events_todo(ctx, FD_CONNECT, FD_WRITE, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(client);
    closesocket(server);

    closesocket(listener);
}

/* perform a blocking recv() even on a nonblocking socket */
static int sync_recv(SOCKET s, void *buffer, int len, DWORD flags)
{
    OVERLAPPED overlapped = {0};
    WSABUF wsabuf;
    DWORD ret_len;
    int ret;

    overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    wsabuf.buf = buffer;
    wsabuf.len = len;
    ret = WSARecv(s, &wsabuf, 1, &ret_len, &flags, &overlapped, NULL);
    if (ret == -1 && WSAGetLastError() == ERROR_IO_PENDING)
    {
        ret = WaitForSingleObject(overlapped.hEvent, 1000);
        ok(!ret, "wait timed out\n");
        ret = WSAGetOverlappedResult(s, &overlapped, &ret_len, FALSE, &flags);
        ret = (ret ? 0 : -1);
    }
    CloseHandle(overlapped.hEvent);
    if (!ret) return ret_len;
    return -1;
}

static void test_write_events(struct event_test_ctx *ctx)
{
    static const int buffer_size = 1024 * 1024;
    SOCKET server, client;
    char *buffer;
    int ret;

    buffer = malloc(buffer_size);

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    if (ctx->is_message)
        check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_events(ctx, 0, 0, 0);

    ret = sync_recv(client, buffer, buffer_size, 0);
    ok(ret == 5, "got %d\n", ret);

    check_events(ctx, 0, 0, 0);

    if (!broken(1))
    {
        /* Windows will never send less than buffer_size bytes here. */
        while ((ret = send(server, buffer, buffer_size, 0)) > 0)
            ok(ret == buffer_size, "got %d.\n", ret);
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        while (recv(client, buffer, buffer_size, 0) > 0);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        /* Broken on Windows versions older than win10v1607 (though sometimes
         * works regardless, for unclear reasons. */
        check_events(ctx, FD_WRITE, 0, 200);
        check_events(ctx, 0, 0, 0);
        select_events(ctx, server, 0);
        select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
        if (ctx->is_message)
            check_events(ctx, FD_WRITE, 0, 200);
        check_events(ctx, 0, 0, 0);
    }

    closesocket(server);
    closesocket(client);

    /* Select on a subset not containing FD_WRITE first. */

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    select_events(ctx, client, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ);
    if (!ctx->is_message)
        check_events(ctx, FD_CONNECT, 0, 200);
    check_events(ctx, 0, 0, 0);

    select_events(ctx, client, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    check_events(ctx, FD_WRITE, 0, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(client);
    closesocket(server);

    /* Despite the documentation, and unlike FD_ACCEPT and FD_RECV, calling
     * send() doesn't clear the FD_WRITE bit. */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_events(ctx, FD_WRITE, 0, 200);

    closesocket(server);
    closesocket(client);

    free(buffer);
}

static void test_read_events(struct event_test_ctx *ctx)
{
    OVERLAPPED overlapped = {0};
    SOCKET server, client;
    DWORD size, flags = 0;
    WSAPOLLFD pollfd;
    unsigned int i;
    char buffer[8];
    WSABUF wsabuf;
    HANDLE thread;
    int ret;

    overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    check_events(ctx, 0, 0, 0);

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_events(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    if (!ctx->is_message)
        check_events_todo(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = recv(server, buffer, 2, 0);
    ok(ret == 2, "got %d\n", ret);

    check_events(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = recv(server, buffer, -1, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT || WSAGetLastError() == WSAENOBUFS /* < Windows 7 */,
             "got error %u\n", WSAGetLastError());

    if (ctx->is_message)
        check_events_todo_msg(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);

    for (i = 0; i < 8; ++i)
    {
        ret = sync_recv(server, buffer, 1, 0);
        ok(ret == 1, "got %d\n", ret);

        if (i < 7)
            check_events(ctx, FD_READ, 0, 200);
        check_events(ctx, 0, 0, 0);
    }

    /* Send data while we're not selecting. */

    select_events(ctx, server, 0);
    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    check_events(ctx, FD_READ, 0, 200);

    ret = recv(server, buffer, 5, 0);
    ok(ret == 5, "got %d\n", ret);

    select_events(ctx, server, 0);
    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);
    ret = sync_recv(server, buffer, 5, 0);
    ok(ret == 5, "got %d\n", ret);
    select_events(ctx, server, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ);

    check_events(ctx, 0, 0, 200);

    /* Send data while we're polling for data but not selecting for FD_READ. */

    pollfd.fd = server;
    pollfd.events = POLLIN;
    thread = CreateThread(NULL, 0, poll_async_thread, &pollfd, 0, NULL);

    select_events(ctx, server, 0);
    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(thread, 1000);
    ok(!ret, "wait timed out\n");
    CloseHandle(thread);

    /* And check events, to show that WSAEnumNetworkEvents() should not clear
     * events we are not currently selecting for. */
    check_events(ctx, 0, 0, 0);

    select_events(ctx, server, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ | FD_WRITE);
    check_events(ctx, FD_READ, FD_WRITE, 200);
    check_events(ctx, 0, 0, 0);

    ret = sync_recv(server, buffer, 5, 0);
    ok(ret == 5, "got %d\n", ret);

    /* Send data while there is a pending WSARecv(). */

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    wsabuf.buf = buffer;
    wsabuf.len = 1;
    ret = WSARecv(server, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = send(client, "a", 1, 0);
    ok(ret == 1, "got %d\n", ret);

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(!ret, "got %d\n", ret);
    ret = GetOverlappedResult((HANDLE)server, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 1, "got size %lu\n", size);

    check_events(ctx, 0, 0, 0);

    ret = send(client, "a", 1, 0);
    ok(ret == 1, "got %d\n", ret);

    check_events(ctx, FD_READ, 0, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(server);
    closesocket(client);
    CloseHandle(overlapped.hEvent);
}

static void test_oob_events(struct event_test_ctx *ctx)
{
    SOCKET server, client;
    char buffer[1];
    int ret;

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    check_events(ctx, 0, 0, 0);

    ret = send(client, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_events(ctx, FD_OOB, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_OOB, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_OOB, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = send(client, "b", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    if (!ctx->is_message)
        check_events_todo_event(ctx, FD_OOB, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = recv(server, buffer, 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_events_todo(ctx, FD_OOB, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = recv(server, buffer, 1, MSG_OOB);
    todo_wine ok(ret == 1, "got %d\n", ret);

    check_events(ctx, 0, 0, 0);

    /* Send data while we're not selecting. */

    select_events(ctx, server, 0);
    ret = send(client, "a", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    check_events(ctx, FD_OOB, 0, 200);

    ret = recv(server, buffer, 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    closesocket(server);
    closesocket(client);
}

static void test_close_events(struct event_test_ctx *ctx)
{
    SOCKET server, client;
    char buffer[5];
    int ret;

    /* Test closesocket(). */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    closesocket(client);

    check_events(ctx, FD_CLOSE, 0, 1000);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_CLOSE, 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events(ctx, FD_CLOSE, 0, 200);
    check_events(ctx, 0, 0, 0);

    ret = recv(server, buffer, 5, 0);
    ok(!ret, "got %d\n", ret);

    check_events(ctx, 0, 0, 0);

    closesocket(server);

    /* Test shutdown(remote end, SD_SEND). */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    shutdown(client, SD_SEND);

    check_events(ctx, FD_CLOSE, 0, 1000);
    check_events(ctx, 0, 0, 0);

    closesocket(client);

    check_events(ctx, 0, 0, 0);

    closesocket(server);

    /* No other shutdown() call generates an event. */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    shutdown(client, SD_RECEIVE);
    shutdown(server, SD_BOTH);

    check_events(ctx, 0, 0, 200);

    shutdown(client, SD_SEND);

    check_events_todo(ctx, FD_CLOSE, 0, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(server);
    closesocket(client);

    /* Test sending data before calling closesocket(). */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_events(ctx, FD_READ, 0, 200);

    closesocket(client);

    check_events_todo(ctx, FD_CLOSE, 0, 200);

    ret = recv(server, buffer, 3, 0);
    ok(ret == 3, "got %d\n", ret);

    check_events(ctx, FD_READ, 0, 200);

    ret = recv(server, buffer, 5, 0);
    ok(ret == 2, "got %d\n", ret);

    check_events_todo(ctx, 0, 0, !strcmp(winetest_platform, "wine") ? 200 : 0);

    closesocket(server);

    /* Close and then select. */

    tcp_socketpair(&client, &server);
    closesocket(client);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    check_events(ctx, FD_CLOSE, 0, 200);

    closesocket(server);

    /* As above, but select on a subset not containing FD_CLOSE first. */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CONNECT | FD_OOB | FD_READ);

    closesocket(client);

    check_events(ctx, 0, 0, 200);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    check_events(ctx, FD_CLOSE, 0, 200);

    closesocket(server);

    /* Trigger RST. */

    tcp_socketpair(&client, &server);

    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);

    close_with_rst(client);

    check_events(ctx, MAKELONG(FD_CLOSE, WSAECONNABORTED), 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events_todo(ctx, MAKELONG(FD_CLOSE, WSAECONNABORTED), 0, 200);
    check_events(ctx, 0, 0, 0);
    select_events(ctx, server, 0);
    select_events(ctx, server, FD_ACCEPT | FD_CLOSE | FD_CONNECT | FD_OOB | FD_READ);
    if (ctx->is_message)
        check_events_todo(ctx, MAKELONG(FD_CLOSE, WSAECONNABORTED), 0, 200);
    check_events(ctx, 0, 0, 0);

    closesocket(server);
}

static void test_events(void)
{
    struct event_test_ctx ctx;

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03) {
        skip("Event tests crash on Windows Server 2003.\n");
        return;
    }
#endif
    ctx.is_message = FALSE;
    ctx.event = CreateEventW(NULL, TRUE, FALSE, NULL);

    test_accept_events(&ctx);
    test_connect_events(&ctx);
    test_write_events(&ctx);
    test_read_events(&ctx);
    test_close_events(&ctx);
    test_oob_events(&ctx);

    CloseHandle(ctx.event);

    ctx.is_message = TRUE;
    ctx.window = CreateWindowA("Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    test_accept_events(&ctx);
    test_connect_events(&ctx);
    test_write_events(&ctx);
    test_read_events(&ctx);
    test_close_events(&ctx);
    test_oob_events(&ctx);

    DestroyWindow(ctx.window);
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
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);

    ret = bind(v6, (struct sockaddr*)&sin6, sizeof(sin6));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    v4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(v4 != INVALID_SOCKET, "Could not create IPv4 socket (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 0;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(!ret, "setsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(!enabled, "expected 0, got %d\n", enabled);
}

    enabled = 1;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(!ret, "setsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());

    /* bind on IPv4 socket should succeed - IPV6_V6ONLY is enabled by default */
    ret = bind(v4, (struct sockaddr*)&sin4, sizeof(sin4));
    ok(!ret, "Could not bind IPv4 address (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 2;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 0;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(ret, "setsockopt(IPV6_V6ONLY) succeeded (LastError: %d)\n", WSAGetLastError());

todo_wine {
    enabled = 0;
    ret = getsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, &len);
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
    ok(enabled == 1, "expected 1, got %d\n", enabled);
}

    enabled = 1;
    len = sizeof(enabled);
    ret = setsockopt(v4, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&enabled, len);
    ok(ret, "setsockopt(IPV6_V6ONLY) succeeded (LastError: %d)\n", WSAGetLastError());

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
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
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
    ok(!ret, "getsockopt(IPV6_V6ONLY) failed (LastError: %d)\n", WSAGetLastError());
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
    ok(err == WSAENOTSOCK, "expected 10038, got %ld instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, NULL, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %ld instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, NULL, 0, &bytesSent, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %ld instead\n", err);

    WSASetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "WSASendMsg should have failed\n");
    err = WSAGetLastError();
    ok(err == WSAEFAULT, "expected 10014, got %ld instead\n", err);

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
    ok(err == WSAEINVAL, "expected 10022, got %ld instead\n", err);

    set_blocking(sock, TRUE);

    bytesSent = 0;
    SetLastError(0xdeadbeef);
    ret = pWSASendMsg(sock, &msg, 0, &bytesSent, NULL, NULL);
    ok(!ret, "WSASendMsg should have worked\n");
    ok(GetLastError() == 0 || broken(GetLastError() == 0xdeadbeef) /* Win <= 2008 */,
       "Expected 0, got %ld\n", GetLastError());
    ok(bytesSent == iovec[0].len, "incorrect bytes sent, expected %ld, sent %ld\n",
       iovec[0].len, bytesSent);

    /* receive data */
    addrlen = sizeof(sockaddr);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(0xdeadbeef);
    ret = recvfrom(dst, buffer, sizeof(buffer), 0, (struct sockaddr *) &sockaddr, &addrlen);
    ok(ret == bytesSent, "got %d, expected %ld\n",
       ret, bytesSent);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

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
    ok(bytesSent == iovec[0].len + iovec[1].len, "incorrect bytes sent, expected %ld, sent %ld\n",
       iovec[0].len + iovec[1].len, bytesSent);
    ok(GetLastError() == 0 || broken(GetLastError() == 0xdeadbeef) /* Win <= 2008 */,
       "Expected 0, got %ld\n", GetLastError());

    /* receive data */
    addrlen = sizeof(sockaddr);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(0xdeadbeef);
    ret = recvfrom(dst, buffer, sizeof(buffer), 0, (struct sockaddr *) &sockaddr, &addrlen);
    ok(ret == bytesSent, "got %d, expected %ld\n",
       ret, bytesSent);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

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
    ok(err == WSAEINVAL, "expected 10022, got %ld instead\n", err);
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
    ok(err == WSAEINVAL, "expected 10014, got %ld instead\n", err);
    closesocket(sock);
}

static void test_WSASendTo(void)
{
    SOCKET s;
    struct sockaddr_in addr, ret_addr;
    char buf[12] = "hello world";
    WSABUF data_buf;
    DWORD bytesSent;
    int ret, len;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(139);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    data_buf.len = sizeof(buf);
    data_buf.buf = buf;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    ok(s != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASendTo(INVALID_SOCKET, &data_buf, 1, NULL, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK,
       "WSASendTo() failed: %d/%d\n", ret, WSAGetLastError());

    len = sizeof(ret_addr);
    ret = getsockname(s, (struct sockaddr *)&ret_addr, &len);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASendTo(s, &data_buf, 1, NULL, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL);
    ok(ret == SOCKET_ERROR && WSAGetLastError() == WSAEFAULT,
       "WSASendTo() failed: %d/%d\n", ret, WSAGetLastError());

    WSASetLastError(12345);
    ret = WSASendTo(s, &data_buf, 1, &bytesSent, 0, (struct sockaddr *)&addr, sizeof(addr), NULL, NULL);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());

    len = sizeof(ret_addr);
    ret = getsockname(s, (struct sockaddr *)&ret_addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(ret_addr.sin_family == AF_INET, "got family %u\n", ret_addr.sin_family);
    ok(ret_addr.sin_port, "expected nonzero port\n");
}

struct recv_thread_apc_param
{
    SOCKET sock;
    unsigned int apc_count;
};

static void WINAPI recv_thread_apc_func(ULONG_PTR param)
{
    struct recv_thread_apc_param *p = (struct recv_thread_apc_param *)param;
    int ret;

    ++p->apc_count;

    ret = send(p->sock, "test", 4, 0);
    ok(ret == 4, "got %d.\n", ret);
}

struct recv_thread_param
{
    SOCKET sock;
    BOOL overlapped;
};

static DWORD WINAPI recv_thread(LPVOID arg)
{
    struct recv_thread_param *p = arg;
    SOCKET sock = p->sock;
    char buffer[32];
    WSABUF wsa;
    WSAOVERLAPPED ov;
    DWORD flags = 0;
    DWORD len;
    int ret;

    wsa.buf = buffer;
    wsa.len = sizeof(buffer);
    if (p->overlapped)
    {
        ov.hEvent = WSACreateEvent();
        WSARecv(sock, &wsa, 1, NULL, &flags, &ov, NULL);

        WaitForSingleObject(ov.hEvent, 1000);
        WSACloseEvent(ov.hEvent);
    }
    else
    {
        SetLastError(0xdeadbeef);
        ret = WSARecv(sock, &wsa, 1, &len, &flags, NULL, NULL);
        ok(!ret, "got ret %d.\n", ret);
        ok(WSAGetLastError() == 0, "got error %d.\n", WSAGetLastError());
        ok(len == 4, "got len %lu.\n", len);
    }
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
    struct recv_thread_apc_param apc_param;
    struct recv_thread_param recv_param;
    char buf[20];
    WSABUF bufs[2];
    WSAOVERLAPPED ov;
    DWORD bytesReturned, flags, id;
    struct sockaddr_in addr;
    unsigned int apc_count;
    int iret, len;
    DWORD dwret;
    BOOL bret;
    HANDLE thread, event = NULL, io_port;

    tcp_socketpair(&src, &dest);

    memset(&ov, 0, sizeof(ov));
    flags = 0;
    bufs[0].len = 1;
    bufs[0].buf = buf;

    /* Send 2 bytes and receive in two calls of 1 */
    SetLastError(0xdeadbeef);
    iret = send(src, "ab", 2, 0);
    ok(iret == 2, "got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;

    /* Non-overlapped WSARecv() performs an alertable wait (tested below), but
     * not if it completes synchronously. Make sure it completes synchronously
     * by polling for input. */
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        skip("Skipping crashing test on Windows Server 2003.\n");
    else
#endif
    check_poll_mask(dest, POLLRDNORM, POLLRDNORM);

    apc_count = 0;
    dwret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
    ok(dwret, "QueueUserAPC returned %lu\n", dwret);

    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 1, "got %ld\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

    ok(!apc_count, "got apc_count %u.\n", apc_count);
    SleepEx(0, TRUE);
    ok(apc_count == 1, "got apc_count %u.\n", apc_count);

    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 1, "got %ld\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

    bufs[0].len = 4;
    SetLastError(0xdeadbeef);
    iret = send(src, "test", 4, 0);
    ok(iret == 4, "Expected 4, got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 4, "Expected 4, got %ld\n", bytesReturned);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

    /* Test 2 buffers */
    bufs[0].len = 4;
    bufs[1].len = 5;
    bufs[1].buf = buf + 10;
    SetLastError(0xdeadbeef);
    iret = send(src, "deadbeefs", 9, 0);
    ok(iret == 9, "Expected 9, got %d\n", iret);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    bytesReturned = 0xdeadbeef;
    iret = WSARecv(dest, bufs, 2, &bytesReturned, &flags, NULL, NULL);
    ok(!iret, "Expected 0, got %d\n", iret);
    ok(bytesReturned == 9, "Expected 9, got %ld\n", bytesReturned);
    bufs[0].buf[4] = '\0';
    bufs[1].buf[5] = '\0';
    ok(!strcmp(bufs[0].buf, "dead"), "buf[0] doesn't match: %s != dead\n", bufs[0].buf);
    ok(!strcmp(bufs[1].buf, "beefs"), "buf[1] doesn't match: %s != beefs\n", bufs[1].buf);
    ok(GetLastError() == ERROR_SUCCESS, "Expected 0, got %ld\n", GetLastError());

    bufs[0].len = sizeof(buf);
    ov.hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %ld\n", GetLastError());
    if (!event)
        goto end;

    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %ld\n", iret, GetLastError());

    iret = WSARecv(dest, bufs, 1, &bytesReturned, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %ld\n", iret, GetLastError());

    close_with_rst(src);

    dwret = WaitForSingleObject(ov.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for disconnect event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)dest, &ov, &bytesReturned, FALSE);
    ok(!bret, "expected failure\n");
    ok(GetLastError() == ERROR_NETNAME_DELETED, "got error %lu\n", GetLastError());
    ok(bytesReturned == 0, "Bytes received is %ld\n", bytesReturned);
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
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    iret = getsockname(server, (struct sockaddr *)&addr, &len);
    ok(!iret, "failed to get address, error %u\n", WSAGetLastError());

    iret = listen(server, 1);
    ok(!iret, "failed to listen, error %u\n", WSAGetLastError());

    iret = connect(src, (struct sockaddr *)&addr, sizeof(addr));
    ok(!iret, "failed to connect, error %u\n", WSAGetLastError());

    len = sizeof(addr);
    dest = accept(server, (struct sockaddr *)&addr, &len);
    ok(dest != INVALID_SOCKET, "failed to create socket %d\n", WSAGetLastError());
    if (dest == INVALID_SOCKET) goto end;

    send(src, "test message", sizeof("test message"), 0);
    recv_param.sock = dest;
    recv_param.overlapped = TRUE;
    thread = CreateThread(NULL, 0, recv_thread, &recv_param, 0, &id);
    WaitForSingleObject(thread, 3000);
    CloseHandle(thread);

    recv_param.overlapped = FALSE;
    thread = CreateThread(NULL, 0, recv_thread, &recv_param, 0, &id);
    apc_param.apc_count = 0;
    apc_param.sock = src;
    dwret = QueueUserAPC(recv_thread_apc_func, thread, (ULONG_PTR)&apc_param);
    ok(dwret, "QueueUserAPC returned %lu\n", dwret);
    WaitForSingleObject(thread, 3000);
    ok(apc_param.apc_count == 1, "got apc_count %u.\n", apc_param.apc_count);

    CloseHandle(thread);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = event;
    ResetEvent(event);
    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, io_completion);
    ok(iret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING, "WSARecv failed - %d error %ld\n", iret, GetLastError());
    send(src, "test message", sizeof("test message"), 0);

    completion_called = 0;
    dwret = SleepEx(1000, TRUE);
    ok(dwret == WAIT_IO_COMPLETION, "got %lu\n", dwret);
    ok(completion_called == 1, "completion not called\n");

    dwret = WaitForSingleObject(event, 1);
    ok(dwret == WAIT_TIMEOUT, "got %lu\n", dwret);

    io_port = CreateIoCompletionPort( (HANDLE)dest, NULL, 0, 0 );
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    /* Using completion function on socket associated with completion port is not allowed. */
    memset(&ov, 0, sizeof(ov));
    completion_called = 0;
    iret = WSARecv(dest, bufs, 1, NULL, &flags, &ov, io_completion);
    ok(iret == SOCKET_ERROR && GetLastError() == WSAEINVAL, "WSARecv failed - %d error %ld\n", iret, GetLastError());
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
        ok( !ret, "WSARecv failed %lu\n", GetLastError() );
        ok( bytes == strlen(args->expect) + 1, "wrong len %ld\n", bytes );
        ok( !strcmp( args->base, args->expect ), "wrong data\n" );
        break;
    case 3:
        buf[0].len = args->size;
        buf[0].buf = args->base;
        ret = WSARecvFrom( args->dest, buf, 1, &bytes, &flags, &addr, &addr_len, NULL, NULL );
        ok( !ret, "WSARecvFrom failed %lu\n", GetLastError() );
        ok( bytes == strlen(args->expect) + 1, "wrong len %ld\n", bytes );
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

    /* Windows 11 no longer triggers write watches anymore. */

    tcp_socketpair(&src, &dest);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ov.hEvent != NULL, "could not create event object, errno = %ld\n", GetLastError());

    flags = 0;

    size = 0x10000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
#ifdef __REACTOS__
    if (base == NULL) {
        skip("Failed to allocate base needed for rest of this test.\n");
        WSACloseEvent(event);
        return;
    }
#endif

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

    bufs[0].len = 5;
    bufs[0].buf = base;
    bufs[1].len = 0x8000;
    bufs[1].buf = base + 0x4000;

    ret = WSARecv( dest, bufs, 2, NULL, &flags, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %ld\n", ret, GetLastError());

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 9 || !count /* Win 11 */, "wrong count %Iu\n", count );
    ok( !base[0], "data set\n" );

    send(src, "test message", sizeof("test message"), 0);

    ret = GetOverlappedResult( (HANDLE)dest, &ov, &bytesReturned, TRUE );
    ok( ret, "GetOverlappedResult failed %lu\n", GetLastError() );
    ok( bytesReturned == sizeof("test message"), "wrong size %lu\n", bytesReturned );
    ok( !memcmp( base, "test ", 5 ), "wrong data %s\n", base );
    ok( !memcmp( base + 0x4000, "message", 8 ), "wrong data %s\n", base + 0x4000 );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

    bufs[1].len = 0x4000;
    bufs[1].buf = base + 0x2000;
    ret = WSARecvFrom( dest, bufs, 2, NULL, &flags, &addr, &addr_len, &ov, NULL);
    ok(ret == SOCKET_ERROR && GetLastError() == ERROR_IO_PENDING,
       "WSARecv failed - %d error %ld\n", ret, GetLastError());

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 5 || !count /* Win 11 */, "wrong count %Iu\n", count );
    ok( !base[0], "data set\n" );

    send(src, "test message", sizeof("test message"), 0);

    ret = GetOverlappedResult( (HANDLE)dest, &ov, &bytesReturned, TRUE );
    ok( ret, "GetOverlappedResult failed %lu\n", GetLastError() );
    ok( bytesReturned == sizeof("test message"), "wrong size %lu\n", bytesReturned );
    ok( !memcmp( base, "test ", 5 ), "wrong data %s\n", base );
    ok( !memcmp( base + 0x2000, "message", 8 ), "wrong data %s\n", base + 0x2000 );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    memset( base, 0, size );
    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

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
        ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
        ok( count == 8 || !count /* Win 11 */, "wrong count %Iu\n", count );

        send(src, "test message", sizeof("test message"), 0);
        WaitForSingleObject( thread, 10000 );
        CloseHandle( thread );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
        ok( count == 0, "wrong count %Iu\n", count );
    }
    WSACloseEvent( event );
    closesocket( dest );
    closesocket( src );
    VirtualFree( base, 0, MEM_FREE );
}

static void test_WSAPoll(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    int ret, err, len;
    SOCKET listener, server, client;
    struct sockaddr_in address;
    WSAPOLLFD fds[16];
    HANDLE thread_handle;
    unsigned int i;
    char buffer[6];

    static const short invalid_flags[] =
            {POLLERR, POLLHUP, POLLNVAL, 0x8, POLLWRBAND, 0x40, 0x80, POLLPRI, 0x800, 0x1000, 0x2000, 0x4000, 0x8000};

    if (!pWSAPoll) /* >= Vista */
    {
        win_skip("WSAPoll is unsupported, some tests will be skipped.\n");
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

    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;
    len = sizeof(address);
    listener = setup_server_socket(&address, &len);

    for (i = 0; i < ARRAY_SIZE(invalid_flags); ++i)
    {
        fds[0].fd = listener;
        fds[0].events = invalid_flags[i];
        fds[0].revents = 0xdead;
        WSASetLastError(0xdeadbeef);
        ret = pWSAPoll(fds, 1, 0);
        todo_wine ok(ret == -1, "got %d\n", ret);
        todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    }

    /* When no events are pending poll returns 0 with no error */
    fds[0].fd = listener;
    fds[0].events = POLLRDNORM | POLLRDBAND | POLLWRNORM;
    fds[0].revents = 0xdead;
    ret = pWSAPoll(fds, 1, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(!fds[0].revents, "got events %#x\n", fds[0].revents);

    fds[0].fd = -1;
    fds[0].events = POLLERR;
    fds[0].revents = 0xdead;
    fds[1].fd = listener;
    fds[1].events = POLLIN;
    fds[1].revents = 0xdead;
    WSASetLastError(0xdeadbeef);
    ret = pWSAPoll(fds, 2, 0);
    ok(!ret, "got %d\n", ret);
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(fds[0].revents == POLLNVAL, "got events %#x\n", fds[0].revents);
    ok(!fds[1].revents, "got events %#x\n", fds[1].revents);

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    fds[0].revents = 0xdead;
    fds[1].fd = 0xabacab;
    fds[1].events = POLLIN;
    fds[1].revents = 0xdead;
    WSASetLastError(0xdeadbeef);
    ret = pWSAPoll(fds, 2, 0);
    ok(!ret, "got %d\n", ret);
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(!fds[0].revents, "got events %#x\n", fds[0].revents);
    ok(fds[1].revents == POLLNVAL, "got events %#x\n", fds[1].revents);

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    fds[0].revents = 0xdead;
    fds[1].fd = 0xabacab;
    fds[1].events = POLLERR;
    fds[1].revents = 0xdead;
    WSASetLastError(0xdeadbeef);
    ret = pWSAPoll(fds, 2, 0);
    todo_wine ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(!fds[0].revents, "got events %#x\n", fds[0].revents);
    todo_wine ok(!fds[1].revents, "got events %#x\n", fds[1].revents);

    fds[0].fd = -1;
    fds[0].events = POLLERR;
    fds[0].revents = 0xdead;
    fds[1].fd = 0xabacab;
    fds[1].events = POLLERR;
    fds[1].revents = 0xdead;
    WSASetLastError(0xdeadbeef);
    ret = pWSAPoll(fds, 2, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAENOTSOCK, "got error %u\n", WSAGetLastError());
    ok(fds[0].revents == POLLNVAL, "got events %#x\n", fds[0].revents);
    ok(fds[1].revents == POLLNVAL, "got events %#x\n", fds[1].revents);

    /* Test listening socket connection attempt notifications */
    client = setup_connector_socket(&address, len, TRUE);

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    fds[0].revents = 0xdead;
    ret = pWSAPoll(fds, 1, 100);
    ok(ret == 1, "got %d\n", ret);
    ok(fds[0].revents == POLLRDNORM, "got events %#x\n", fds[0].revents);

    fds[0].revents = 0xdead;
    ret = pWSAPoll(fds, 1, 0);
    ok(ret == 1, "got %d\n", ret);
    ok(fds[0].revents == POLLRDNORM, "got events %#x\n", fds[0].revents);

    fds[0].events = POLLRDBAND | POLLWRNORM;
    fds[0].revents = 0xdead;
    ret = pWSAPoll(fds, 1, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(!fds[0].revents, "got events %#x\n", fds[0].revents);

    server = accept(listener, NULL, NULL);
    ok(server != INVALID_SOCKET, "failed to accept, error %u\n", WSAGetLastError());
    set_blocking(client, FALSE);
    set_blocking(server, FALSE);

    for (i = 0; i < ARRAY_SIZE(invalid_flags); ++i)
    {
        fds[0].fd = server;
        fds[0].events = invalid_flags[i];
        fds[0].revents = 0xdead;
        WSASetLastError(0xdeadbeef);
        ret = pWSAPoll(fds, 1, 0);
        todo_wine ok(ret == -1, "got %d\n", ret);
        todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    }

    /* Test flags exposed by connected sockets. */

    fds[0].fd = listener;
    fds[0].events = POLLRDNORM | POLLRDBAND | POLLWRNORM;
    fds[0].revents = 0xdead;
    fds[1].fd = server;
    fds[1].events = POLLRDNORM | POLLRDBAND | POLLWRNORM;
    fds[1].revents = 0xdead;
    fds[2].fd = client;
    fds[2].events = POLLRDNORM | POLLRDBAND | POLLWRNORM;
    fds[2].revents = 0xdead;
    ret = pWSAPoll(fds, 3, 0);
    ok(ret == 2, "got %d\n", ret);
    ok(!fds[0].revents, "got events %#x\n", fds[0].revents);
    ok(fds[1].revents == POLLWRNORM, "got events %#x\n", fds[1].revents);
    ok(fds[2].revents == POLLWRNORM, "got events %#x\n", fds[2].revents);

    /* Test data receiving notifications */

    ret = send(server, "1234", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    check_poll_mask(client, POLLRDNORM | POLLRDBAND, POLLRDNORM);
    check_poll(client, POLLRDNORM | POLLWRNORM);
    check_poll(server, POLLWRNORM);

    ret = sync_recv(client, buffer, sizeof(buffer), 0);
    ok(ret == 4, "got %d\n", ret);

    check_poll(client, POLLWRNORM);
    check_poll(server, POLLWRNORM);

    /* Because the kernel asynchronously buffers data, this test is not reliable. */

    if (0)
    {
        static const int large_buffer_size = 1024 * 1024;
        char *large_buffer = malloc(large_buffer_size);

        while (send(server, large_buffer, large_buffer_size, 0) == large_buffer_size);

        check_poll(client, POLLWRNORM | POLLRDNORM);
        check_poll(server, 0);

        while (recv(client, large_buffer, large_buffer_size, 0) > 0);

        check_poll(client, POLLWRNORM);
        check_poll(server, POLLWRNORM);

        free(large_buffer);
    }

    /* Test OOB data notifications */

    ret = send(client, "A", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_poll(client, POLLWRNORM);
    check_poll_mask(server, POLLRDNORM | POLLRDBAND, POLLRDBAND);
    check_poll(server, POLLWRNORM | POLLRDBAND);

    buffer[0] = 0xcc;
    ret = recv(server, buffer, 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);
    ok(buffer[0] == 'A', "got %#x\n", buffer[0]);

    check_poll(client, POLLWRNORM);
    check_poll(server, POLLWRNORM);

    /* If the socket is OOBINLINED the notification is like normal data */

    ret = 1;
    ret = setsockopt(server, SOL_SOCKET, SO_OOBINLINE, (char *)&ret, sizeof(ret));
    ok(!ret, "got error %u\n", WSAGetLastError());
    ret = send(client, "A", 1, MSG_OOB);
    ok(ret == 1, "got %d\n", ret);

    check_poll(client, POLLWRNORM);
    check_poll_mask(server, POLLRDNORM | POLLRDBAND, POLLRDNORM);
    check_poll(server, POLLWRNORM | POLLRDNORM);

    buffer[0] = 0xcc;
    ret = recv(server, buffer, 1, 0);
    ok(ret == 1, "got %d\n", ret);
    ok(buffer[0] == 'A', "got %#x\n", buffer[0]);

    check_poll(client, POLLWRNORM);
    check_poll_todo(server, POLLWRNORM);

    /* Test shutdown. */

    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, POLLWRNORM);
    check_poll_todo(server, POLLWRNORM);

    ret = shutdown(client, SD_SEND);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, POLLWRNORM);
    check_poll_mask_todo(server, 0, POLLHUP);
    check_poll_todo(server, POLLWRNORM | POLLHUP);

    closesocket(client);
    closesocket(server);

    /* Test shutdown via closesocket(). */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&address, sizeof(address));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    closesocket(client);

    check_poll_mask(server, 0, POLLHUP);
    check_poll(server, POLLWRNORM | POLLHUP);

    closesocket(server);

    /* Test shutdown with data in the pipe. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ret = connect(client, (struct sockaddr *)&address, sizeof(address));
    ok(!ret, "got error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "got error %u\n", WSAGetLastError());

    ret = send(client, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    check_poll(client, POLLWRNORM);
    check_poll_mask(server, POLLRDNORM | POLLRDBAND, POLLRDNORM);
    check_poll(server, POLLWRNORM | POLLRDNORM);

    ret = shutdown(client, SD_SEND);

    check_poll(client, POLLWRNORM);
    check_poll_mask_todo(server, 0, POLLHUP);
    check_poll_todo(server, POLLWRNORM | POLLRDNORM | POLLHUP);

    closesocket(client);
    closesocket(server);

    /* Test closing a socket while selecting on it. */

    tcp_socketpair(&client, &server);

    thread_handle = CreateThread(NULL, 0, SelectCloseThread, &client, 0, NULL);
    fds[0].fd = client;
    fds[0].events = POLLRDNORM | POLLRDBAND;
    fds[0].revents = 0xdead;
    apc_count = 0;
    ret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
    ok(ret, "QueueUserAPC returned %d\n", ret);
    ret = pWSAPoll(fds, 1, 2000);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(ret == 1, "got %d\n", ret);
    ok(fds[0].revents == POLLNVAL, "got events %#x\n", fds[0].revents);
    ret = WaitForSingleObject(thread_handle, 1000);
    ok(!ret, "wait failed\n");
    CloseHandle(thread_handle);

    closesocket(server);

    /* Test a failed connection.
     *
     * The following WSAPoll() call times out on versions older than w10pro64,
     * but even on w10pro64 it takes over 2 seconds for an error to be reported,
     * so make the test interactive-only. */
    if (winetest_interactive)
    {
        const struct sockaddr_in invalid_addr =
        {
            .sin_family = AF_INET,
            .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
            .sin_port = 255,
        };
        SOCKET client;

        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        set_blocking(client, FALSE);

        ret = connect(client, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

        fds[0].fd = client;
        fds[0].events = POLLRDNORM | POLLRDBAND | POLLWRNORM;
        fds[0].revents = 0xdead;
        ret = pWSAPoll(fds, 1, 10000);
        ok(ret == 1, "got %d\n", ret);
        todo_wine ok(fds[0].revents == (POLLWRNORM | POLLHUP | POLLERR), "got events %#x\n", fds[0].revents);

        len = sizeof(err);
        err = 0xdeadbeef;
        ret = getsockopt(client, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(err == WSAECONNREFUSED, "got error %u\n", err);

        len = sizeof(err);
        err = 0xdeadbeef;
        ret = getsockopt(client, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        ok(err == WSAECONNREFUSED, "got error %u\n", err);

        check_poll_todo(client, POLLWRNORM | POLLHUP | POLLERR);

        closesocket(client);

        /* test polling after a (synchronous) failure */

        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        ret = connect(client, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSAECONNREFUSED, "got error %u\n", WSAGetLastError());

        check_poll_todo(client, POLLWRNORM | POLLHUP | POLLERR);

        len = sizeof(err);
        err = 0xdeadbeef;
        ret = getsockopt(client, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        ok(!ret, "getsockopt failed with %d\n", WSAGetLastError());
        todo_wine ok(!err, "got error %u\n", err);

        closesocket(client);
    }

    closesocket(listener);

    /* Test UDP sockets. */

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    check_poll(client, POLLWRNORM);
    check_poll(server, POLLWRNORM);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %u\n", WSAGetLastError());
    len = sizeof(address);
    ret = getsockname(client, (struct sockaddr *)&address, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    check_poll(client, POLLWRNORM);
    check_poll(server, POLLWRNORM);

    ret = sendto(server, "data", 5, 0, (struct sockaddr *)&address, sizeof(address));
    ok(ret == 5, "got %d\n", ret);

    check_poll_mask(client, POLLRDNORM | POLLRDBAND, POLLRDNORM);
    check_poll(client, POLLWRNORM | POLLRDNORM);
    check_poll(server, POLLWRNORM);

    closesocket(client);
    closesocket(server);
}

static void test_connect(void)
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
    ok(listener != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(listener, (struct sockaddr*)&address, sizeof(address));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    addrlen = sizeof(address);
    iret = getsockname(listener, (struct sockaddr*)&address, &addrlen);
    ok(!iret, "failed to get address, error %u\n", WSAGetLastError());

    iret = listen(listener, 1);
    ok(!iret, "failed to listen, error %u\n", WSAGetLastError());

    iret = set_blocking(listener, TRUE);
    ok(!iret, "failed to set nonblocking, error %u\n", WSAGetLastError());

    bytesReturned = 0xdeadbeef;
    iret = WSAIoctl(connector, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectExGuid, sizeof(connectExGuid),
        &pConnectEx, sizeof(pConnectEx), &bytesReturned, NULL, NULL);
    ok(!iret, "failed to get ConnectEx, error %u\n", WSAGetLastError());

    ok(bytesReturned == sizeof(pConnectEx), "expected sizeof(pConnectEx), got %lu\n", bytesReturned);

    WSASetLastError(0xdeadbeef);
    iret = connect(listener, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1, "got %d\n", iret);
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    iret = pConnectEx(listener, (struct sockaddr *)&address, sizeof(address), NULL, 0, &bytesReturned, &overlapped);
    ok(!iret, "got %d\n", iret);
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    bret = pConnectEx(INVALID_SOCKET, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "ConnectEx on invalid socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAEINVAL, "ConnectEx on a unbound socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, NULL);
    ok(bret == FALSE && WSAGetLastError() == ERROR_INVALID_PARAMETER, "ConnectEx on a NULL overlapped "
        "returned %d + errno %d\n", bret, WSAGetLastError());

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "ConnectEx failed: "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret, "Connecting failed, error %ld\n", GetLastError());
    ok(bytesReturned == 0, "Bytes sent is %ld\n", bytesReturned);

    closesocket(connector);
    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "failed to accept socket, error %u\n", WSAGetLastError());
#ifdef __REACTOS__
    }
#endif

    buffer[0] = '1';
    buffer[1] = '2';
    buffer[2] = '3';
    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, buffer, 3, &bytesReturned, &overlapped);
    memset(buffer, 0, 3);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "ConnectEx failed: "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret, "Connecting failed, error %ld\n", GetLastError());
    ok(bytesReturned == 3, "Bytes sent is %ld\n", bytesReturned);

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "could not accept socket error %d\n", WSAGetLastError());
#ifdef __REACTOS__
    }
#endif

    bytesReturned = recv(acceptor, buffer, 3, 0);
    buffer[4] = 0;
    ok(bytesReturned == 3, "Didn't get all sent data, got only %ld\n", bytesReturned);
    ok(buffer[0] == '1' && buffer[1] == '2' && buffer[2] == '3',
       "Failed to get the right data, expected '123', got '%s'\n", buffer);

    WSASetLastError(0xdeadbeef);
    iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1, "got %d\n", iret);
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    iret = connect(acceptor, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1, "got %d\n", iret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    bret = pConnectEx(connector, (struct sockaddr *)&address, sizeof(address), NULL, 0, &bytesReturned, &overlapped);
    ok(!bret, "got %d\n", bret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    bret = pConnectEx(acceptor, (struct sockaddr *)&address, sizeof(address), NULL, 0, &bytesReturned, &overlapped);
    ok(!bret, "got %d\n", bret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    closesocket(connector);
    closesocket(acceptor);

    tcp_socketpair(&connector, &acceptor);

    WSASetLastError(0xdeadbeef);
    iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1, "got %d\n", iret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    iret = connect(acceptor, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1, "got %d\n", iret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    bret = pConnectEx(connector, (struct sockaddr *)&address, sizeof(address), NULL, 0, &bytesReturned, &overlapped);
    ok(!bret, "got %d\n", bret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    bret = pConnectEx(acceptor, (struct sockaddr *)&address, sizeof(address), NULL, 0, &bytesReturned, &overlapped);
    ok(!bret, "got %d\n", bret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    closesocket(connector);
    closesocket(acceptor);

    /* Connect with error */

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    /* ConnectEx needs a bound socket */
    memset(&conaddress, 0, sizeof(conaddress));
    conaddress.sin_family = AF_INET;
    conaddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(connector, (struct sockaddr*)&conaddress, sizeof(conaddress));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    address.sin_port = htons(1);

    bret = pConnectEx(connector, (struct sockaddr*)&address, addrlen, NULL, 0, &bytesReturned, &overlapped);
    ok(bret == FALSE && GetLastError() == ERROR_IO_PENDING, "ConnectEx to bad destination failed: "
        "returned %d + errno %ld\n", bret, GetLastError());
    dwret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for connect event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)connector, &overlapped, &bytesReturned, FALSE);
    ok(bret == FALSE && GetLastError() == ERROR_CONNECTION_REFUSED,
       "Connecting to a disconnected host returned error %d - %d\n", bret, WSAGetLastError());

    WSACloseEvent(overlapped.hEvent);
    closesocket(connector);

    if (0)
    {
        /* Wait in connect() is alertable. This may take a very long time before connection fails,
         * so disable the test. Testing with localhost is unreliable as that may avoid waiting in
         * accept(). */
        connector = socket(AF_INET, SOCK_STREAM, 0);
        ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
        address.sin_addr.s_addr = inet_addr("8.8.8.8");
        address.sin_port = htons(255);

        apc_count = 0;
        SleepEx(0, TRUE);
        ok(apc_count == 0, "got apc_count %d.\n", apc_count);
        bret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
        ok(bret, "QueueUserAPC returned %d\n", bret);
        iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
        ok(apc_count == 1, "got apc_count %d.\n", apc_count);
        ok(iret == -1 && (WSAGetLastError() == WSAECONNREFUSED || WSAGetLastError() == WSAETIMEDOUT),
                "unexpected iret %d, error %d.\n", iret, WSAGetLastError());
        closesocket(connector);
    }

    /* Test connect after previous connect attempt failure. */
    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    conaddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    conaddress.sin_port = htons(255);
    iret = connect(connector, (struct sockaddr *)&conaddress, sizeof(conaddress));
    ok(iret == -1, "connection succeeded.\n");

    ok(WSAGetLastError() == WSAECONNREFUSED, "got error %u\n", WSAGetLastError());
    set_blocking( connector, FALSE );
    iret = getsockname(listener, (struct sockaddr*)&address, &addrlen);
    ok(!iret, "failed to get address, error %u\n", WSAGetLastError());

    iret = connect(connector, (struct sockaddr *)&address, sizeof(address));
    ok(iret == -1 && WSAGetLastError() == WSAEWOULDBLOCK, "unexpected iret %d, error %d.\n",
            iret, WSAGetLastError());
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != INVALID_SOCKET, "could not accept socket error %d\n", WSAGetLastError());

    closesocket(acceptor);
    closesocket(connector);
    closesocket(listener);
}

static void test_AcceptEx(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    SOCKET listener, acceptor, acceptor2, connector, connector2;
    struct sockaddr_in bindAddress, peerAddress, *readBindAddress, *readRemoteAddress;
    int socklen, iret, localSize = sizeof(struct sockaddr_in), remoteSize = localSize;
    GUID acceptExGuid = WSAID_ACCEPTEX, getAcceptExGuid = WSAID_GETACCEPTEXSOCKADDRS;
    GUID connectex_guid = WSAID_CONNECTEX;
    LPFN_ACCEPTEX pAcceptEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS pGetAcceptExSockaddrs = NULL;
    LPFN_CONNECTEX pConnectEx = NULL;
    fd_set fds_accept, fds_send;
    static const struct timeval timeout = {1, 0};
    char buffer[1024], ipbuffer[32];
    OVERLAPPED overlapped = {0}, overlapped2 = {0};
    DWORD bytesReturned, dwret;
    BOOL bret;

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    overlapped2.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    ok(listener != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(listener, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    socklen = sizeof(bindAddress);
    iret = getsockname(listener, (struct sockaddr*)&bindAddress, &socklen);
    ok(!iret, "failed to get address, error %u\n", WSAGetLastError());

    iret = set_blocking(listener, FALSE);
    ok(!iret, "Failed to set nonblocking, error %u\n", WSAGetLastError());

    iret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid),
        &pAcceptEx, sizeof(pAcceptEx), &bytesReturned, NULL, NULL);
    ok(!iret, "Failed to get AcceptEx, error %u\n", WSAGetLastError());

    iret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptExGuid, sizeof(getAcceptExGuid),
        &pGetAcceptExSockaddrs, sizeof(pGetAcceptExSockaddrs), &bytesReturned, NULL, NULL);
    ok(!iret, "Failed to get GetAcceptExSockaddrs, error %u\n", WSAGetLastError());

    iret = WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
            &pConnectEx, sizeof(pConnectEx), &bytesReturned, NULL, NULL);
    ok(!iret, "Failed to get ConnectEx, error %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(INVALID_SOCKET, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "AcceptEx on invalid listening socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    todo_wine
    ok(bret == FALSE && WSAGetLastError() == WSAEINVAL, "AcceptEx on a non-listening socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);
    if (!bret && WSAGetLastError() == ERROR_IO_PENDING)
        CancelIo((HANDLE)listener);

    iret = listen(listener, 5);
    ok(!iret, "failed to listen, error %lu\n", GetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, INVALID_SOCKET, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAENOTSOCK, "AcceptEx on invalid accepting socket "
        "returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, NULL, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    todo_wine ok(bret == FALSE && WSAGetLastError() == WSAEFAULT,
        "AcceptEx on NULL buffer returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING,
        "AcceptEx on too small local address size returned %d + errno %d\n",
        bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    iret = connect(connector, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to connect, error %u\n", WSAGetLastError());
    iret = getsockname(connector, (struct sockaddr *)&peerAddress, &remoteSize);
    ok(!iret, "getsockname failed, error %u\n", WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!dwret, "wait failed\n");
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "got error %lu\n", GetLastError());
    ok(!(NTSTATUS)overlapped.Internal, "got %#Ix\n", overlapped.Internal);
    ok(!bytesReturned, "got size %lu\n", bytesReturned);

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "FIXME: This crashes on ReactOS!\n");
    } else {
#endif
    readBindAddress = readRemoteAddress = (struct sockaddr_in *)0xdeadbeef;
    localSize = remoteSize = 0xdeadbeef;
    pGetAcceptExSockaddrs(buffer, 0, 0, sizeof(struct sockaddr_in) + 16,
            (struct sockaddr **)&readBindAddress, &localSize, (struct sockaddr **)&readRemoteAddress, &remoteSize);
    todo_wine ok(readBindAddress == (struct sockaddr_in *)0xdeadbeef, "got local addr %p\n", readBindAddress);
    ok(!memcmp(readRemoteAddress, &peerAddress, sizeof(peerAddress)), "remote addr didn't match\n");
    todo_wine ok(localSize == 0xdeadbeef, "got local size %u\n", localSize);
    ok(remoteSize == sizeof(struct sockaddr_in), "got remote size %u\n", remoteSize);
#ifdef __REACTOS__
    }
#endif

    closesocket(connector);
    closesocket(acceptor);

    /* A UDP socket cannot be accepted into. */

    acceptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    if (WSAGetLastError() == ERROR_IO_PENDING)
        CancelIo((HANDLE)listener);

    closesocket(acceptor);

    /* A bound socket cannot be accepted into. */

    acceptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    iret = bind(acceptor, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!iret, "got error %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    if (WSAGetLastError() == ERROR_IO_PENDING)
        CancelIo((HANDLE)listener);

    closesocket(acceptor);

    /* A connected socket cannot be accepted into. */

    tcp_socketpair(&acceptor, &acceptor2);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, 0, sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    if (WSAGetLastError() == ERROR_IO_PENDING)
        CancelIo((HANDLE)listener);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor2, buffer, 0, 0, sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    if (WSAGetLastError() == ERROR_IO_PENDING)
        CancelIo((HANDLE)listener);

    closesocket(acceptor);
    closesocket(acceptor2);

    /* Pass an insufficient local address size. */

    acceptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(acceptor != -1, "failed to create socket, error %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, 3,
            sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret && WSAGetLastError() == ERROR_IO_PENDING, "got %d, error %u\n", bret, WSAGetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "got %#Ix\n", overlapped.Internal);

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    iret = connect(connector, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to connect, error %u\n", WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!dwret, "wait failed\n");
    bytesReturned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(!bret, "expected failure\n");
#ifdef __REACTOS__
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER || broken(GetLastError() == ERROR_MORE_DATA) /* WS03 */, "got error %lu\n", GetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_TOO_SMALL || broken((NTSTATUS)overlapped.Internal ==  STATUS_BUFFER_OVERFLOW) /* WS03 */, "got %#Ix\n", overlapped.Internal);
    ok(!bytesReturned || broken(bytesReturned == 3) /* WS03 */, "got size %lu\n", bytesReturned);
#else
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_TOO_SMALL, "got %#Ix\n", overlapped.Internal);
    ok(!bytesReturned, "got size %lu\n", bytesReturned);
#endif

    closesocket(acceptor);

    /* The above connection request is not accepted. */
    acceptor = accept(listener, NULL, NULL);
#ifdef __REACTOS__
    todo_wine ok(acceptor != INVALID_SOCKET || broken(acceptor == INVALID_SOCKET && WSAGetLastError() == WSAEWOULDBLOCK) /* WS03 */, "failed to accept, error %u\n", WSAGetLastError());
#else
    todo_wine ok(acceptor != INVALID_SOCKET, "failed to accept, error %u\n", WSAGetLastError());
#endif
    closesocket(acceptor);

    closesocket(connector);

    acceptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(acceptor != -1, "failed to create socket, error %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 4,
            sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret && WSAGetLastError() == ERROR_IO_PENDING, "got %d, error %u\n", bret, WSAGetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "got %#Ix\n", overlapped.Internal);

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    iret = connect(connector, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to connect, error %u\n", WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!dwret, "wait failed\n");
    bytesReturned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    todo_wine ok(!bret, "expected failure\n");
#ifdef __REACTOS__
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER || broken(GetLastError() == ERROR_MORE_DATA) /* WS03 */, "got error %lu\n", GetLastError());
    todo_wine ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_TOO_SMALL || broken((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW) /* WS03 */, "got %#Ix\n", overlapped.Internal);
    ok(!bytesReturned || broken(bytesReturned == 20) /* WS03 */, "got size %lu\n", bytesReturned);
#else
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    todo_wine ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_TOO_SMALL, "got %#Ix\n", overlapped.Internal);
    ok(!bytesReturned, "got size %lu\n", bytesReturned);
#endif

    closesocket(acceptor);

    /* The above connection request is not accepted. */
    acceptor = accept(listener, NULL, NULL);
#ifdef __REACTOS__
    todo_wine ok(acceptor != INVALID_SOCKET || broken(acceptor == INVALID_SOCKET && WSAGetLastError() == WSAEWOULDBLOCK) /* WS03 */, "failed to accept, error %u\n", WSAGetLastError());
#else
    todo_wine ok(acceptor != INVALID_SOCKET, "failed to accept, error %u\n", WSAGetLastError());
#endif
    closesocket(acceptor);

    closesocket(connector);

    acceptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(acceptor != -1, "failed to create socket, error %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 15,
        sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx on too small local address "
        "size returned %d + errno %d\n",
        bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);
    bret = CancelIo((HANDLE) listener);
    ok(bret, "Failed to cancel pending accept socket\n");

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 16, 0,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAEFAULT,
        "AcceptEx on too small remote address size returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 16,
        sizeof(struct sockaddr_in) + 15, &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING,
        "AcceptEx on too small remote address size returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);
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

    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    /* try to accept into the same socket twice */
    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 0,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == WSAEINVAL,
       "AcceptEx on already pending socket returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    /* try to connect a socket that's being accepted into */
    iret = connect(acceptor,  (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL,
       "connecting to acceptex acceptor succeeded? return %d + errno %d\n", iret, WSAGetLastError());

    bret = pConnectEx(acceptor, (struct sockaddr *)&bindAddress, sizeof(bindAddress),
            NULL, 0, &bytesReturned, &overlapped2);
    ok(!bret, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    overlapped.Internal = 0xdeadbeef;
    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

#ifdef __REACTOS__
    /* ReactOS will hang infinitely here otherwise */
    dwret = WaitForSingleObject(overlapped.hEvent, 5000);
#else
    dwret = WaitForSingleObject(overlapped.hEvent, INFINITE);
#endif
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());
    ok(overlapped.Internal == STATUS_SUCCESS, "got %08lx\n", (ULONG)overlapped.Internal);

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %ld\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %ld\n", bytesReturned);

    /* Try to call getsockname on the acceptor socket.
     *
     * On Windows, this requires setting SO_UPDATE_ACCEPT_CONTEXT. */
    iret = setsockopt(acceptor, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listener, sizeof(SOCKET));
    ok(!iret, "Failed to set accept context %ld\n", GetLastError());
    iret = getsockname(acceptor, (struct sockaddr *)&peerAddress, &remoteSize);
    ok(!iret, "getsockname failed.\n");
    ok(remoteSize == sizeof(struct sockaddr_in), "got remote size %u\n", remoteSize);

    closesocket(connector);
    connector = INVALID_SOCKET;
    closesocket(acceptor);

    /* Test short reads */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    overlapped.Internal = 0xdeadbeef;
    bret = pAcceptEx(listener, acceptor, buffer, 2,
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    /* AcceptEx() still won't complete until we send data */
    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 0);
    ok(dwret == WAIT_TIMEOUT, "Waiting for accept event timeout failed with %ld + errno %ld\n", dwret, GetLastError());
    ok(overlapped.Internal == STATUS_PENDING, "got %08lx\n", (ULONG)overlapped.Internal);

    iret = getsockname( connector, (struct sockaddr *)&peerAddress, &remoteSize);
    ok( !iret, "getsockname failed.\n");

    /* AcceptEx() could complete any time now */
    iret = send(connector, buffer, 1, 0);
    ok(iret == 1, "could not send 1 byte: send %d errno %d\n", iret, WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());
    ok(overlapped.Internal == STATUS_SUCCESS, "got %08lx\n", (ULONG)overlapped.Internal);

    /* Check if the buffer from AcceptEx is decoded correctly */
    pGetAcceptExSockaddrs(buffer, 2, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                          (struct sockaddr **)&readBindAddress, &localSize,
                          (struct sockaddr **)&readRemoteAddress, &remoteSize);
#ifdef __REACTOS__
    if (readBindAddress == (void*)0x1D) {
        ok(FALSE, "readBindAddress is an invalid pointer!\n");
    } else {
#endif
    strcpy( ipbuffer, inet_ntoa(readBindAddress->sin_addr));
    ok( readBindAddress->sin_addr.s_addr == bindAddress.sin_addr.s_addr,
            "Local socket address is different %s != %s\n",
            ipbuffer, inet_ntoa(bindAddress.sin_addr));
    ok( readBindAddress->sin_port == bindAddress.sin_port,
            "Local socket port is different: %d != %d\n",
            readBindAddress->sin_port, bindAddress.sin_port);
#ifdef __REACTOS__
    } if (readRemoteAddress == (void*)-1) {
        ok(FALSE, "readRemoteAddress is an invalid pointer!\n");
    } else {
#endif
    strcpy( ipbuffer, inet_ntoa(readRemoteAddress->sin_addr));
    ok( readRemoteAddress->sin_addr.s_addr == peerAddress.sin_addr.s_addr,
            "Remote socket address is different %s != %s\n",
            ipbuffer, inet_ntoa(peerAddress.sin_addr));
    ok( readRemoteAddress->sin_port == peerAddress.sin_port,
            "Remote socket port is different: %d != %d\n",
            readRemoteAddress->sin_port, peerAddress.sin_port);
#ifdef __REACTOS__
    }
#endif

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %ld\n", GetLastError());
    ok(bytesReturned == 1, "bytesReturned isn't supposed to be %ld\n", bytesReturned);

    closesocket(connector);
    connector = INVALID_SOCKET;
    closesocket(acceptor);

    /* Test CF_DEFER & AcceptEx interaction */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    connector2 = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector2 != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());

    iret = set_blocking(connector, FALSE);
    ok(!iret, "failed to set nonblocking, error %lu\n", GetLastError());
    iret = set_blocking(connector2, FALSE);
    ok(!iret, "failed to set nonblocking, error %lu\n", GetLastError());

    /* Connect socket #1 */
    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    buffer[0] = '0';

    FD_ZERO(&fds_accept);
    FD_SET(listener, &fds_accept);
    iret = select(0, &fds_accept, NULL, NULL, &timeout);
    ok(iret == 1, "wait timed out\n");

    acceptor2 = WSAAccept(listener, NULL, NULL, AlwaysDeferConditionFunc, 0);
    ok(acceptor2 == INVALID_SOCKET, "expected failure\n");
    ok(WSAGetLastError() == WSATRY_AGAIN, "got error %u\n", WSAGetLastError());
    bret = pAcceptEx(listener, acceptor, buffer, 0, sizeof(struct sockaddr_in) + 16,
            sizeof(struct sockaddr_in) + 16, &bytesReturned, &overlapped);
    ok(!bret, "expected failure\n");
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    FD_ZERO(&fds_send);
    FD_SET(connector, &fds_send);
    iret = select(0, NULL, &fds_send, NULL, &timeout);
    ok(iret == 1, "wait timed out\n");

    iret = send(connector, "1", 1, 0);
    ok(iret == 1, "got ret %d, error %u\n", iret, WSAGetLastError());

    iret = connect(connector2, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    ok(iret == SOCKET_ERROR, "expected failure\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

    iret = select(0, &fds_accept, NULL, NULL, &timeout);
    ok(iret == 1, "wait timed out\n");

    acceptor2 = accept(listener, NULL, NULL);
    ok(acceptor2 != INVALID_SOCKET, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor2);

    FD_ZERO(&fds_send);
    FD_SET(connector2, &fds_send);
    iret = select(0, NULL, &fds_send, NULL, &timeout);
    ok(iret == 1, "wait timed out\n");

    iret = send(connector2, "2", 1, 0);
    ok(iret == 1, "got ret %d, error %u\n", iret, WSAGetLastError());

    dwret = WaitForSingleObject(overlapped.hEvent, 0);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %ld\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %ld\n", bytesReturned);

    set_blocking(acceptor, TRUE);
    iret = recv( acceptor, buffer, 2, 0);
    ok(iret == 1, "Failed to get data, %d, errno: %d\n", iret, WSAGetLastError());
    ok(buffer[0] == '1', "The wrong first client was accepted by acceptex: %c != 1\n", buffer[0]);

    closesocket(connector);
    closesocket(connector2);
    closesocket(acceptor);

    /* clean up in case of failures */
    while ((acceptor = accept(listener, NULL, NULL)) != INVALID_SOCKET)
        closesocket(acceptor);

    /* Disconnect during receive? */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %d\n", WSAGetLastError());

    closesocket(connector);
    connector = INVALID_SOCKET;

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());

    bytesReturned = 123456;
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(bret, "GetOverlappedResult failed, error %ld\n", GetLastError());
    ok(bytesReturned == 0, "bytesReturned isn't supposed to be %ld\n", bytesReturned);

    closesocket(acceptor);

    /* Test closing with pending requests */

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    closesocket(acceptor);

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0,
       "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());
    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(!bret && GetLastError() == ERROR_OPERATION_ABORTED, "GetOverlappedResult failed, error %ld\n", GetLastError());

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    CancelIo((HANDLE) acceptor);

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_TIMEOUT, "Waiting for timeout failed with %ld + errno %ld\n", dwret, GetLastError());

    closesocket(acceptor);

    acceptor = socket(AF_INET, SOCK_STREAM, 0);
    ok(acceptor != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    bret = pAcceptEx(listener, acceptor, buffer, sizeof(buffer) - 2*(sizeof(struct sockaddr_in) + 16),
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
        &bytesReturned, &overlapped);
    ok(bret == FALSE && WSAGetLastError() == ERROR_IO_PENDING, "AcceptEx returned %d + errno %d\n", bret, WSAGetLastError());

    closesocket(listener);

    dwret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(dwret == WAIT_OBJECT_0, "Waiting for accept event failed with %ld + errno %ld\n", dwret, GetLastError());

    bret = GetOverlappedResult((HANDLE)listener, &overlapped, &bytesReturned, FALSE);
    ok(!bret && GetLastError() == ERROR_OPERATION_ABORTED, "GetOverlappedResult failed, error %ld\n", GetLastError());

    CloseHandle(overlapped.hEvent);
    CloseHandle(overlapped2.hEvent);
    closesocket(acceptor);
    closesocket(connector2);
}

static void test_shutdown(void)
{
    struct sockaddr_in addr, server_addr, client_addr;
    SOCKET listener, client, server;
    OVERLAPPED overlapped = {0};
    DWORD size, flags = 0;
    int ret, addrlen;
    char buffer[5];
    WSABUF wsabuf;

    /* __REACTOS__ NOTE: BUGCHECKS on ReactOS */
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != INVALID_SOCKET, "failed to create listener socket, error %d\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    addrlen = sizeof(server_addr);
    ret = getsockname(listener, (struct sockaddr *)&server_addr, &addrlen);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(listener, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_RECEIVE);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTCONN, "got error %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    set_blocking(client, FALSE);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(client, "test", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(!ret, "got %d\n", ret);
    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(!ret, "got %d\n", ret);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(server, SD_RECEIVE);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = send(server, "test", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = sync_recv(client, buffer, sizeof(buffer), 0);
    ok(ret == 5, "got %d\n", ret);
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_an(buffer, ret));

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(server, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(server, "test", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    addrlen = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &server_addr, sizeof(server_addr)), "address didn't match\n");

    addrlen = sizeof(client_addr);
    ret = getsockname(client, (struct sockaddr *)&client_addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    addrlen = sizeof(addr);
    ret = getpeername(server, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &client_addr, sizeof(addr)), "address didn't match\n");

    WSASetLastError(0xdeadbeef);
    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, 0xdeadbeef);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    closesocket(client);
    closesocket(server);

    /* Test SD_BOTH. */

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "FIXME: This test bugchecks on ReactOS!\n");
    } else {
#endif
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_BOTH);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(client, "test", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(!ret, "got %d\n", ret);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(server, SD_BOTH);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(server, "test", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    addrlen = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &server_addr, sizeof(server_addr)), "address didn't match\n");

    addrlen = sizeof(client_addr);
    ret = getsockname(client, (struct sockaddr *)&client_addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    addrlen = sizeof(addr);
    ret = getpeername(server, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &client_addr, sizeof(addr)), "address didn't match\n");

    closesocket(client);
    closesocket(server);
#ifdef __REACTOS__
    }
#endif

    /* Send data to a peer which is closed. */

    tcp_socketpair(&client, &server);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());
    closesocket(client);

    ret = send(server, "test", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    WSASetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
#ifdef __REACTOS__
    ok(ret == -1 || broken(!ret) /* WS03 */, "got %d\n", ret);
    if (ret)
        todo_wine ok(WSAGetLastError() == WSAECONNABORTED, "got error %u\n", WSAGetLastError());
#else
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAECONNABORTED, "got error %u\n", WSAGetLastError());
#endif

    closesocket(server);

    /* Test shutting down with async I/O pending. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());
    set_blocking(client, FALSE);

    wsabuf.buf = buffer;
    wsabuf.len = sizeof(buffer);
    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = shutdown(client, SD_RECEIVE);
    ok(!ret, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, NULL, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = send(server, "test", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait timed out\n");
    size = 0xdeadbeef;
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
#ifdef __REACTOS__
    ok(ret || broken(!ret && GetLastError() == ERROR_NETNAME_DELETED) /* WS03 */, "got error %lu\n", GetLastError());
    ok(size == 5 || broken(!ret && size == 0), "got size %lu\n", size);
#else
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 5, "got size %lu\n", size);
#endif
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_an(buffer, size));

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
#ifdef __REACTOS__
    ok(WSAGetLastError() == WSAESHUTDOWN || broken(WSAGetLastError() == WSAECONNABORTED) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());
#endif

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(server, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
#ifdef __REACTOS__
    ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAECONNRESET) /* WS03 */, "got error %u\n", WSAGetLastError());
#else
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
#endif

    ret = shutdown(client, SD_SEND);
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
#ifdef __REACTOS__
    if (ret) {
        /* Happens on WS03 */
        skip("Wait timed out, skipping tests\n");
    } else {
#endif
    ok(!ret, "wait timed out\n");
    size = 0xdeadbeef;
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
#ifdef __REACTOS__
    }
#endif

    closesocket(client);
    closesocket(server);

    /* Test shutting down a listening socket. */

    WSASetLastError(0xdeadbeef);
    ret = shutdown(listener, SD_SEND);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(listener, SD_RECEIVE);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTCONN, "got error %u\n", WSAGetLastError());

    closesocket(listener);

    /* Test shutting down UDP sockets. */

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    addrlen = sizeof(server_addr);
    ret = getsockname(server, (struct sockaddr *)&server_addr, &addrlen);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    set_blocking(server, FALSE);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(server, SD_RECEIVE);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = recvfrom(server, buffer, sizeof(buffer), 0, NULL, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = sendto(client, "test", 5, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(ret == 5, "got %d\n", ret);

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = shutdown(client, SD_SEND);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = sendto(client, "test", 5, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    closesocket(client);
    closesocket(server);

    CloseHandle(overlapped.hEvent);
}

static void test_DisconnectEx(void)
{
    struct sockaddr_in server_addr, client_addr, addr;
    GUID disconnectex_guid = WSAID_DISCONNECTEX;
    SOCKET listener, server, client;
    LPFN_DISCONNECTEX pDisconnectEx;
    OVERLAPPED overlapped = {0};
    int addrlen, ret;
    char buffer[5];
    DWORD size;

    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    client = socket(AF_INET, SOCK_STREAM, 0);
    ok(client != INVALID_SOCKET, "failed to create connector socket, error %u\n", WSAGetLastError());

    ret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, &disconnectex_guid, sizeof(disconnectex_guid),
            &pDisconnectEx, sizeof(pDisconnectEx), &size, NULL, NULL);
    if (ret)
    {
        win_skip("WSAIoctl failed to get DisconnectEx, error %d\n", WSAGetLastError());
        closesocket(client);
        return;
    }

    listener = socket(AF_INET, SOCK_STREAM, 0);
    ok(listener != INVALID_SOCKET, "failed to create listener socket, error %d\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    addrlen = sizeof(server_addr);
    ret = getsockname(listener, (struct sockaddr *)&server_addr, &addrlen);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());
    ret = listen(listener, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pDisconnectEx(INVALID_SOCKET, &overlapped, 0, 0);
    ok(!ret, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTSOCK, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, &overlapped, 0, 0);
    ok(!ret, "expected failure\n");
    ok(WSAGetLastError() == WSAENOTCONN, "got error %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != INVALID_SOCKET, "failed to accept, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, &overlapped, 0, 0);
    ok(!ret, "expected failure\n");
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait timed out\n");
    size = 0xdeadbeef;
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);

    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(client, "test", 5, 0);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(!ret, "got %d\n", ret);

    ret = send(server, "test", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == 5, "got %d\n", ret);
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_an(buffer, ret));

    addrlen = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &server_addr, sizeof(server_addr)), "address didn't match\n");

    addrlen = sizeof(client_addr);
    ret = getsockname(client, (struct sockaddr *)&client_addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    addrlen = sizeof(addr);
    ret = getpeername(server, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &client_addr, sizeof(addr)), "address didn't match\n");

    closesocket(client);
    closesocket(server);

    /* Test the synchronous case. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, NULL, 0, 0);
    ok(ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, NULL, 0, 0);
    ok(ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* < 7 */, "got error %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEISCONN, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(client, "test", 5, 0);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAESHUTDOWN, "got error %u\n", WSAGetLastError());

    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(!ret, "got %d\n", ret);

    ret = send(server, "test", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == 5, "got %d\n", ret);
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_an(buffer, ret));

    addrlen = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &server_addr, sizeof(server_addr)), "address didn't match\n");

    addrlen = sizeof(client_addr);
    ret = getsockname(client, (struct sockaddr *)&client_addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    addrlen = sizeof(addr);
    ret = getpeername(server, (struct sockaddr *)&addr, &addrlen);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!memcmp(&addr, &client_addr, sizeof(addr)), "address didn't match\n");

    closesocket(client);
    closesocket(server);

    closesocket(listener);
    CloseHandle(overlapped.hEvent);
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
        ok_(file,line)(n1 == n2, "Block %d size mismatch (%ld != %ld)\n", i, n1, n2);
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

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "ReactOS crashes on most of these TransmitFile tests!\n");
        return;
    }
#endif
    /* Setup sockets for testing TransmitFile */
    client = socket(AF_INET, SOCK_STREAM, 0);
    ok(client != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    server = socket(AF_INET, SOCK_STREAM, 0);
    ok(server != INVALID_SOCKET, "failed to create socket, error %lu\n", GetLastError());
    iret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, &transmitFileGuid, sizeof(transmitFileGuid),
                    &pTransmitFile, sizeof(pTransmitFile), &num_bytes, NULL, NULL);
    ok(!iret, "failed to get TransmitFile, error %lu\n", GetLastError());
    GetSystemWindowsDirectoryA(system_ini_path, MAX_PATH );
    strcat(system_ini_path, "\\system.ini");
    file = CreateFileA(system_ini_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0x0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open file, error %lu\n", GetLastError());
    file_size = GetFileSize(file, NULL);

    /* Test TransmitFile with an invalid socket */
    bret = pTransmitFile(INVALID_SOCKET, file, 0, 0, NULL, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTSOCK, "TransmitFile triggered unexpected errno (%ld != %d)\n", err, WSAENOTSOCK);

    /* Test a bogus TransmitFile without a connected socket */
    bret = pTransmitFile(client, NULL, 0, 0, NULL, NULL, TF_REUSE_SOCKET);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTCONN, "TransmitFile triggered unexpected errno (%ld != %d)\n", err, WSAENOTCONN);

    /* Setup a properly connected socket for transfers */
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = htons(SERVERPORT+1);
    bindAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(server, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to bind socket, error %lu\n", GetLastError());
    iret = listen(server, 1);
    ok(!iret, "failed to listen, error %lu\n", GetLastError());
    iret = connect(client, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(!iret, "failed to connect, error %lu\n", GetLastError());
    len = sizeof(bindAddress);
    dest = accept(server, (struct sockaddr*)&bindAddress, &len);
    ok(dest != INVALID_SOCKET, "failed to accept, error %lu\n", GetLastError());
    iret = set_blocking(dest, FALSE);
    ok(!iret, "failed to set nonblocking, error %lu\n", GetLastError());

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
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    bret = pTransmitFile(client, file, 0, 0, &ov, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%ld != %d)\n",
       err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == file_size,
       "Overlapped TransmitFile sent an unexpected number of bytes (%ld != %ld).\n",
       total_sent, file_size);
    compare_file(file, dest, 0);

    /* Test overlapped TransmitFile w/ start offset */
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ov.Offset = 10;
    bret = pTransmitFile(client, file, 0, 0, &ov, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%ld != %d)\n", err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == (file_size - ov.Offset),
       "Overlapped TransmitFile sent an unexpected number of bytes (%ld != %ld).\n",
       total_sent, file_size - ov.Offset);
    compare_file(file, dest, ov.Offset);

    /* Test overlapped TransmitFile w/ file and buffer data */
    ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    buffers.Head = &header_msg[0];
    buffers.HeadLength = sizeof(header_msg);
    buffers.Tail = &footer_msg[0];
    buffers.TailLength = sizeof(footer_msg);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ov.Offset = 0;
    bret = pTransmitFile(client, file, 0, 0, &ov, &buffers, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == ERROR_IO_PENDING, "TransmitFile triggered unexpected errno (%ld != %d)\n", err, ERROR_IO_PENDING);
    iret = WaitForSingleObject(ov.hEvent, 2000);
    ok(iret == WAIT_OBJECT_0, "Overlapped TransmitFile failed.\n");
    WSAGetOverlappedResult(client, &ov, &total_sent, FALSE, NULL);
    ok(total_sent == (file_size + buffers.HeadLength + buffers.TailLength),
       "Overlapped TransmitFile sent an unexpected number of bytes (%ld != %ld).\n",
       total_sent, file_size  + buffers.HeadLength + buffers.TailLength);
    iret = recv(dest, buf, sizeof(header_msg), 0);
    ok(memcmp(buf, &header_msg[0], sizeof(header_msg)) == 0,
       "TransmitFile header buffer did not match!\n");
    compare_file(file, dest, 0);
    iret = recv(dest, buf, sizeof(footer_msg), 0);
    ok(memcmp(buf, &footer_msg[0], sizeof(footer_msg)) == 0,
       "TransmitFile footer buffer did not match!\n");

    /* Test TransmitFile with a UDP datagram socket */
    closesocket(client);
    client = socket(AF_INET, SOCK_DGRAM, 0);
    bret = pTransmitFile(client, NULL, 0, 0, NULL, NULL, 0);
    err = WSAGetLastError();
    ok(!bret, "TransmitFile succeeded unexpectedly.\n");
    ok(err == WSAENOTCONN, "TransmitFile triggered unexpected errno (%ld != %d)\n", err, WSAENOTCONN);

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
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    struct sockaddr_in in = {0}, out = {0};
    ULONG_PTR key;
    HANDLE port;
    SOCKET sock;
    DWORD size;
    int ret;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(sock != INVALID_SOCKET, "Expected socket to return a valid socket\n");
    port = CreateIoCompletionPort((HANDLE)sock, NULL, 123, 0);

    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), NULL, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in) - 1, &out, sizeof(out), &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, NULL, sizeof(in), &out, sizeof(out), &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    in.sin_family = AF_INET;
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), &size, NULL, NULL);
    todo_wine ok(ret == -1, "expected failure\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());
    todo_wine ok(size == 0xdeadbeef, "got size %lu\n", size);

    in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), &size, NULL, NULL);
    ok(!ret, "expected failure\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(size == sizeof(out), "got size %lu\n", size);
    /* We expect the source address to be INADDR_LOOPBACK as well, but
     * there's no guarantee that a route to the loopback address exists,
     * so rather than introduce spurious test failures we do not test the
     * source address.
     */

    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out) - 1, &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    todo_wine ok(size == sizeof(out), "got size %lu\n", size);

    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), NULL, sizeof(out), &size, NULL, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), NULL, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in), &out, sizeof(out), &size, &overlapped, NULL);
    ok(!ret, "expected failure\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(size == sizeof(out), "got size %lu\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);
    ok(!overlapped.Internal, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "got size %Iu\n", overlapped.InternalHigh);

    CloseHandle(port);
    closesocket(sock);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in),
            &out, sizeof(out), NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, &in, sizeof(in),
            &out, sizeof(out), &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
    ok(size == sizeof(out), "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

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
    ok(!!h, "failed to get interface list, error %u\n", WSAGetLastError());
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
    ok (!ret, "bind() failed with error %ld\n", GetLastError());
    set_blocking(sock, FALSE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %ld\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%lx\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = socket(AF_INET, 0, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %ld\n", GetLastError());
    set_blocking(sock, TRUE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %ld\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%lx\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %ld\n", GetLastError());
    set_blocking(sock, FALSE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %ld\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%lx\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %ld\n", GetLastError());
    set_blocking(sock, TRUE);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %ld\n", error);
    ok (error == ERROR_IO_PENDING, "expected 0x3e5, got 0x%lx\n", error);

    CloseHandle(overlapped.hEvent);
    closesocket(sock);

    /* When the socket is overlapped non-blocking and the list change is requested without
     * an overlapped structure the error will be different. */
    sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(sock != INVALID_SOCKET, "socket() failed\n");

    SetLastError(0xdeadbeef);
    ret = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok (!ret, "bind() failed with error %ld\n", GetLastError());
    set_blocking(sock, FALSE);

    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, NULL, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %ld\n", error);
    ok (error == WSAEWOULDBLOCK, "expected 10035, got %ld\n", error);

    io_port = CreateIoCompletionPort( (HANDLE)sock, NULL, 0, 0 );
    ok (io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    set_blocking(sock, FALSE);
    memset(&overlapped, 0, sizeof(overlapped));
    SetLastError(0xdeadbeef);
    ret = WSAIoctl(sock, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &num_bytes, &overlapped, NULL);
    error = GetLastError();
    ok (ret == SOCKET_ERROR, "WSAIoctl(SIO_ADDRESS_LIST_CHANGE) failed with error %lu\n", error);
    ok (error == ERROR_IO_PENDING, "expected ERROR_IO_PENDING got %lu\n", error);

    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 0 );
    ok(!bret, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(sock);

    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 0 );
    ok(!bret, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %lu\n", GetLastError());
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

    trace("Spent %ld ms waiting.\n", GetTickCount() - tick);

    WSACloseEvent(event2);
    WSACloseEvent(event3);

    closesocket(sock);
    closesocket(sock2);
    closesocket(sock3);
}

/*
 * Provide consistent initialization for the AcceptEx IOCP tests.
 */
static SOCKET setup_iocp_src(struct sockaddr_in *bindAddress)
{
    SOCKET src;
    int iret, socklen;

    src = socket(AF_INET, SOCK_STREAM, 0);
    ok(src != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    memset(bindAddress, 0, sizeof(*bindAddress));
    bindAddress->sin_family = AF_INET;
    bindAddress->sin_addr.s_addr = inet_addr("127.0.0.1");
    iret = bind(src, (struct sockaddr*)bindAddress, sizeof(*bindAddress));
    ok(!iret, "failed to bind, error %u\n", WSAGetLastError());

    socklen = sizeof(*bindAddress);
    iret = getsockname(src, (struct sockaddr*)bindAddress, &socklen);
    ok(!iret, "failed to get address, error %u\n", WSAGetLastError());

    iret = set_blocking(src, FALSE);
    ok(!iret, "failed to make socket non-blocking, error %u\n", WSAGetLastError());

    iret = listen(src, 5);
    ok(!iret, "failed to listen, error %u\n", WSAGetLastError());

    return src;
}

static void test_completion_port(void)
{
    HANDLE io_port;
    WSAOVERLAPPED ov, *olp;
    SOCKET src, dest, dup, connector = INVALID_SOCKET;
    WSAPROTOCOL_INFOA info;
    char buf[1024];
    WSABUF bufs;
    DWORD num_bytes, flags;
    int iret;
    BOOL bret;
    ULONG_PTR key;
    struct sockaddr_in bindAddress;
    GUID acceptExGuid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX pAcceptEx = NULL;
    fd_set fds_recv;

    memset(buf, 0, sizeof(buf));
    io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    ok( io_port != NULL, "Failed to create completion port %lu\n", GetLastError());

    memset(&ov, 0, sizeof(ov));

    tcp_socketpair(&src, &dest);

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;

    io_port = CreateIoCompletionPort( (HANDLE)dest, io_port, 125, 0 );
    ok(io_port != NULL, "Failed to create completion port %lu\n", GetLastError());

    SetLastError(0xdeadbeef);

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSARecv returned %d\n", iret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    Sleep(100);

    close_with_rst(src);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %d\n", bret);
    ok(GetLastError() == ERROR_NETNAME_DELETED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes received is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %d\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);

    memset(&ov, 0, sizeof(ov));

    tcp_socketpair(&src, &dest);

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    set_blocking(dest, FALSE);

    close_with_rst(src);

    Sleep(100);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);

    iret = WSASend(dest, &bufs, 1, &num_bytes, 0, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSASend failed - %d\n", iret);
    ok(GetLastError() == WSAECONNRESET, "Last error was %ld\n", GetLastError());
    ok(num_bytes == 0xdeadbeef, "Managed to send %ld\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);

    /* Test IOCP response on successful immediate read. */
    tcp_socketpair(&src, &dest);

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;
    SetLastError(0xdeadbeef);

    iret = WSASend(src, &bufs, 1, &num_bytes, 0, &ov, NULL);
    ok(!iret, "WSASend failed - %d, last error %lu\n", iret, GetLastError());
    ok(num_bytes == sizeof(buf), "Managed to send %ld\n", num_bytes);

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());
    set_blocking(dest, FALSE);

    FD_ZERO(&fds_recv);
    FD_SET(dest, &fds_recv);
    select(dest + 1, &fds_recv, NULL, NULL, NULL);

    num_bytes = 0xdeadbeef;
    flags = 0;

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(!iret, "WSARecv failed - %d, last error %lu\n", iret, GetLastError());
    ok(num_bytes == sizeof(buf), "Managed to read %ld\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == sizeof(buf), "Number of bytes transferred is %lu\n", num_bytes);
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
    ok(!iret, "WSARecv failed - %d, last error %lu\n", iret, GetLastError());
    ok(!num_bytes, "Managed to read %ld\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(!num_bytes, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    src = INVALID_SOCKET;
    closesocket(dest);
    dest = INVALID_SOCKET;

    /* Test IOCP response on hard shutdown. This was the condition that triggered
     * a crash in an actual app (bug 38980). */
    tcp_socketpair(&src, &dest);

    bufs.len = sizeof(buf);
    bufs.buf = buf;
    flags = 0;
    memset(&ov, 0, sizeof(ov));

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());
    set_blocking(dest, FALSE);

    close_with_rst(src);

    FD_ZERO(&fds_recv);
    FD_SET(dest, &fds_recv);
    select(dest + 1, &fds_recv, NULL, NULL, NULL);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);

    /* Somehow a hard shutdown doesn't work on my Linux box. It seems SO_LINGER is ignored. */
    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSARecv failed - %d\n", iret);
    ok(GetLastError() == WSAECONNRESET, "Last error was %ld\n", GetLastError());
    ok(num_bytes == 0xdeadbeef, "Managed to read %ld\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(dest);

    /* Test reading from a non-connected socket, mostly because the above test is marked todo. */
    dest = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(dest != INVALID_SOCKET, "socket() failed\n");

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());
    set_blocking(dest, FALSE);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    memset(&ov, 0, sizeof(ov));

    iret = WSARecv(dest, &bufs, 1, &num_bytes, &flags, &ov, NULL);
    ok(iret == SOCKET_ERROR, "WSARecv failed - %d\n", iret);
    ok(GetLastError() == WSAENOTCONN, "Last error was %ld\n", GetLastError());
    ok(num_bytes == 0xdeadbeef, "Managed to read %ld\n", num_bytes);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "GetQueuedCompletionStatus returned %u\n", bret );
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    num_bytes = 0xdeadbeef;
    closesocket(dest);

    dest = socket(AF_INET, SOCK_STREAM, 0);
    ok(dest != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    iret = WSAIoctl(dest, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid),
            &pAcceptEx, sizeof(pAcceptEx), &num_bytes, NULL, NULL);
    ok(!iret, "failed to get AcceptEx, error %u\n", WSAGetLastError());

    /* Test IOCP response on socket close (IOCP created after AcceptEx) */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP response on socket close (IOCP created before AcceptEx) */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
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
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && olp->Internal == (ULONG)STATUS_CANCELLED, "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle (closing duplicated handle) */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    closesocket(dup);
    dup = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    src = INVALID_SOCKET;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP with duplicated handle (closing original handle) */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    WSADuplicateSocketA( src, GetCurrentProcessId(), &info );
    dup = WSASocketA(AF_INET, SOCK_STREAM, 0, &info, 0, WSA_FLAG_OVERLAPPED);
    ok(dup != INVALID_SOCKET, "failed to duplicate socket!\n");

    bret = pAcceptEx(dup, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(dup);
    dup = INVALID_SOCKET;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_CANCELLED), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* Test IOCP without AcceptEx */

    src = setup_iocp_src(&bindAddress);

    SetLastError(0xdeadbeef);

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    closesocket(src);
    src = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    /* */

    src = setup_iocp_src(&bindAddress);

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %ld\n", GetLastError());

    closesocket(connector);
    connector = INVALID_SOCKET;

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == TRUE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == 0xdeadbeef, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_SUCCESS), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (dest != INVALID_SOCKET)
        closesocket(dest);
    if (src != INVALID_SOCKET)
        closesocket(dest);

    /* */

    src = setup_iocp_src(&bindAddress);

    dest = socket(AF_INET, SOCK_STREAM, 0);
    ok(dest != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %ld\n", GetLastError());

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
    ok(GetLastError() == 0xdeadbeef, "Last error was %ld\n", GetLastError());
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 1, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
    ok(olp && (olp->Internal == (ULONG)STATUS_SUCCESS), "Internal status is %Ix\n", olp ? olp->Internal : 0);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    if (src != INVALID_SOCKET)
        closesocket(src);
    if (connector != INVALID_SOCKET)
        closesocket(connector);

    /* */

    src = setup_iocp_src(&bindAddress);

    dest = socket(AF_INET, SOCK_STREAM, 0);
    ok(dest != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    connector = socket(AF_INET, SOCK_STREAM, 0);
    ok(connector != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    io_port = CreateIoCompletionPort((HANDLE)src, io_port, 125, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    io_port = CreateIoCompletionPort((HANDLE)dest, io_port, 236, 0);
    ok(io_port != NULL, "failed to create completion port %lu\n", GetLastError());

    bret = pAcceptEx(src, dest, buf, sizeof(buf) - 2*(sizeof(struct sockaddr_in) + 16),
            sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
            &num_bytes, &ov);
    ok(bret == FALSE, "AcceptEx returned %d\n", bret);
    ok(GetLastError() == ERROR_IO_PENDING, "Last error was %ld\n", GetLastError());

    iret = connect(connector, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
    ok(iret == 0, "connecting to accepting socket failed, error %ld\n", GetLastError());

    closesocket(dest);

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;

    bret = GetQueuedCompletionStatus(io_port, &num_bytes, &key, &olp, 100);
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
#ifdef __REACTOS__
    ok(GetLastError() == ERROR_OPERATION_ABORTED || broken(GetLastError() == ERROR_NETNAME_DELETED) /* WS03 */
            || GetLastError() == ERROR_CONNECTION_ABORTED, "got error %lu\n", GetLastError());
#else
    ok(GetLastError() == ERROR_OPERATION_ABORTED
            || GetLastError() == ERROR_CONNECTION_ABORTED, "got error %lu\n", GetLastError());
#endif
    ok(key == 125, "Key is %Iu\n", key);
    ok(num_bytes == 0, "Number of bytes transferred is %lu\n", num_bytes);
    ok(olp == &ov, "Overlapped structure is at %p\n", olp);
#ifdef __REACTOS__
    ok(olp && ((NTSTATUS)olp->Internal == STATUS_CANCELLED || broken((NTSTATUS)olp->Internal == STATUS_LOCAL_DISCONNECT) /* WS03 */
            || (NTSTATUS)olp->Internal == STATUS_CONNECTION_ABORTED), "olp is %s, got status %#Ix\n", olp ? "not null" : "NULL", olp ? olp->Internal : 0);
#else
    ok((NTSTATUS)olp->Internal == STATUS_CANCELLED
            || (NTSTATUS)olp->Internal == STATUS_CONNECTION_ABORTED, "got status %#Ix\n", olp->Internal);
#endif

    SetLastError(0xdeadbeef);
    key = 0xdeadbeef;
    num_bytes = 0xdeadbeef;
    olp = (WSAOVERLAPPED *)0xdeadbeef;
    bret = GetQueuedCompletionStatus( io_port, &num_bytes, &key, &olp, 200 );
    ok(bret == FALSE, "failed to get completion status %u\n", bret);
    ok(GetLastError() == WAIT_TIMEOUT, "Last error was %ld\n", GetLastError());
    ok(key == 0xdeadbeef, "Key is %Iu\n", key);
    ok(num_bytes == 0xdeadbeef, "Number of bytes transferred is %lu\n", num_bytes);
    ok(!olp, "Overlapped structure is at %p\n", olp);

    closesocket(src);
    closesocket(connector);
    CloseHandle(io_port);
}

static void test_connect_completion_port(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    GUID connectex_guid = WSAID_CONNECTEX;
    SOCKET connector, listener, acceptor;
    struct sockaddr_in addr, destaddr;
    LPFN_CONNECTEX pConnectEx;
    int ret, addrlen;
    ULONG_PTR key;
    HANDLE port;
    DWORD size;

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    addrlen = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &addrlen);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(listener, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());

    ret = WSAIoctl(connector, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
            &pConnectEx, sizeof(pConnectEx), &size, NULL, NULL);
    ok(!ret, "Failed to get ConnectEx, error %u\n", WSAGetLastError());

    /* connect() does not queue completion. */

    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());

    ret = connect(connector, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(connector);
    CloseHandle(port);

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    set_blocking(connector, FALSE);

    ret = connect(connector, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(connector);
    CloseHandle(port);

    /* ConnectEx() queues completion. */

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = bind(connector, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    ret = pConnectEx(connector, (struct sockaddr *)&destaddr, sizeof(destaddr),
            NULL, 0, &size, &overlapped);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)connector, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);
#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);
#ifdef __REACTOS__
    }
#endif

    size = 0xdeadbeef;
    key = 0xdeadbeef;
    overlapped_ptr = NULL;
    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!key, "got key %#Ix\n", key);
    ok(!size, "got %lu bytes\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    closesocket(connector);
    CloseHandle(port);

    /* Test ConnectEx() with a non-empty buffer. */

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = bind(connector, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    ret = pConnectEx(connector, (struct sockaddr *)&destaddr, sizeof(destaddr),
            (void *)"one", 3, &size, &overlapped);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)connector, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 3, "got %lu bytes\n", size);
#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);
#ifdef __REACTOS__
    }
#endif

    size = 0xdeadbeef;
    key = 0xdeadbeef;
    overlapped_ptr = NULL;
    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!key, "got key %#Ix\n", key);
    ok(size == 3, "got %lu bytes\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    closesocket(connector);
    CloseHandle(port);

    /* Suppress completion by setting the low bit. */

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = bind(connector, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    overlapped.hEvent = (HANDLE)((ULONG_PTR)overlapped.hEvent | 1);

    ret = pConnectEx(connector, (struct sockaddr *)&destaddr, sizeof(destaddr),
            NULL, 0, &size, &overlapped);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)connector, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);
#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);
#ifdef __REACTOS__
    }
#endif

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(connector);
    CloseHandle(port);

    overlapped.hEvent = (HANDLE)((ULONG_PTR)overlapped.hEvent & ~1);

    /* Skip completion on success. */

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = SetFileCompletionNotificationModes((HANDLE)connector, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
    ok(ret, "got error %lu\n", GetLastError());
    ret = bind(connector, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    ret = pConnectEx(connector, (struct sockaddr *)&destaddr, sizeof(destaddr),
            NULL, 0, &size, &overlapped);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)connector, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);
#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This hangs on ReactOS!\n");
    } else {
#endif
    acceptor = accept(listener, NULL, NULL);
    ok(acceptor != -1, "failed to accept, error %u\n", WSAGetLastError());
    closesocket(acceptor);
#ifdef __REACTOS__
    }
#endif

    size = 0xdeadbeef;
    key = 0xdeadbeef;
    overlapped_ptr = NULL;
    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!key, "got key %#Ix\n", key);
    ok(!size, "got %lu bytes\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    closesocket(connector);
    CloseHandle(port);

    closesocket(listener);

    /* Connect to an invalid address. */

    connector = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(connector != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)connector, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = SetFileCompletionNotificationModes((HANDLE)connector, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
    ok(ret, "got error %lu\n", GetLastError());
    ret = bind(connector, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());

    ret = pConnectEx(connector, (struct sockaddr *)&destaddr, sizeof(destaddr),
            NULL, 0, &size, &overlapped);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ret = WaitForSingleObject(overlapped.hEvent, 15000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)connector, &overlapped, &size, FALSE);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_CONNECTION_REFUSED, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);

    size = 0xdeadbeef;
    key = 0xdeadbeef;
    overlapped_ptr = NULL;
    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_CONNECTION_REFUSED, "got error %lu\n", GetLastError());
    ok(!key, "got key %#Ix\n", key);
    ok(!size, "got %lu bytes\n", size);
    ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    closesocket(connector);
    CloseHandle(port);
}

static void test_shutdown_completion_port(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    GUID disconnectex_guid = WSAID_DISCONNECTEX;
    struct sockaddr_in addr, destaddr;
    LPFN_DISCONNECTEX pDisconnectEx;
    SOCKET listener, server, client;
    int ret, addrlen;
    ULONG_PTR key;
    HANDLE port;
    DWORD size;

    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(listener != -1, "failed to create socket, error %u\n", WSAGetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "failed to bind, error %u\n", WSAGetLastError());
    addrlen = sizeof(destaddr);
    ret = getsockname(listener, (struct sockaddr *)&destaddr, &addrlen);
    ok(!ret, "failed to get address, error %u\n", WSAGetLastError());

    ret = listen(listener, 1);
    ok(!ret, "failed to listen, error %u\n", WSAGetLastError());

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());

    ret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, &disconnectex_guid, sizeof(disconnectex_guid),
            &pDisconnectEx, sizeof(pDisconnectEx), &size, NULL, NULL);
    ok(!ret, "Failed to get ConnectEx, error %u\n", WSAGetLastError());

    /* shutdown() does not queue completion. */

    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    ret = shutdown(client, SD_BOTH);
    ok(!ret, "failed to shutdown, error %u\n", WSAGetLastError());

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(server);
    closesocket(client);
    CloseHandle(port);

    /* WSASendDisconnect() and WSARecvDisconnect() do not queue completion. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    ret = WSASendDisconnect(client, NULL);
    ok(!ret, "failed to shutdown, error %u\n", WSAGetLastError());

    ret = WSARecvDisconnect(client, NULL);
    ok(!ret, "failed to shutdown, error %u\n", WSAGetLastError());

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(server);
    closesocket(client);
    CloseHandle(port);

    /* DisconnectEx() queues completion. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = SetFileCompletionNotificationModes((HANDLE)client, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
    ok(ret, "got error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    SetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, &overlapped, 0, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");

    size = 0xdeadbeef;
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, TRUE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);

    size = 0xdeadbeef;
    key = 0xdeadbeef;
    overlapped_ptr = NULL;
    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());
    todo_wine ok(!key, "got key %#Ix\n", key);
    todo_wine ok(!size, "got %lu bytes\n", size);
    todo_wine ok(overlapped_ptr == &overlapped, "got overlapped %p\n", overlapped_ptr);

    closesocket(server);
    closesocket(client);
    CloseHandle(port);

    /* Test passing a NULL overlapped structure to DisconnectEx(). */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    SetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, NULL, 0, 0);
    ok(ret, "expected success\n");
    ok(!GetLastError() || GetLastError() == 0xdeadbeef /* < 7 */, "got error %lu\n", GetLastError());

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(server);
    closesocket(client);
    CloseHandle(port);

    /* Suppress completion by setting the low bit. */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)client, NULL, 0, 0);
    ok(!!port, "failed to create port, error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&destaddr, sizeof(destaddr));
    ok(!ret, "failed to connect, error %u\n", WSAGetLastError());
    server = accept(listener, NULL, NULL);
    ok(server != -1, "failed to accept, error %u\n", WSAGetLastError());

    overlapped.hEvent = (HANDLE)((ULONG_PTR)overlapped.hEvent | 1);

    SetLastError(0xdeadbeef);
    ret = pDisconnectEx(client, &overlapped, 0, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");

    size = 0xdeadbeef;
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, TRUE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got %lu bytes\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == WAIT_TIMEOUT, "got error %lu\n", GetLastError());

    closesocket(server);
    closesocket(client);
    CloseHandle(port);

    overlapped.hEvent = (HANDLE)((ULONG_PTR)overlapped.hEvent & ~1);

    CloseHandle(overlapped.hEvent);
}

static void test_address_list_query(void)
{
    char buffer[1024];
    SOCKET_ADDRESS_LIST *address_list = (SOCKET_ADDRESS_LIST *)buffer;
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    DWORD size, expect_size;
    unsigned int i;
    ULONG_PTR key;
    HANDLE port;
    SOCKET s;
    int ret;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s != INVALID_SOCKET, "Failed to create socket, error %d.\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

    size = 0;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, 0, &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size >= FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]), "Got unexpected size %lu.\n", size);
    expect_size = size;

    size = 0;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, sizeof(buffer), &size, NULL, NULL);
    ok(!ret, "Got unexpected ret %d.\n", ret);
    ok(!WSAGetLastError(), "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);

    expect_size = FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[address_list->iAddressCount]);
    for (i = 0; i < address_list->iAddressCount; ++i)
    {
        expect_size += address_list->Address[i].iSockaddrLength;
    }
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);

    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, sizeof(buffer), NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());

    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, sizeof(buffer), &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);

    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, 0, &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);

    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, 1, &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "Got unexpected error %d.\n", WSAGetLastError());
    ok(!size, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    memset(buffer, 0xcc, sizeof(buffer));
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer,
            FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]), &size, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);
    ok(address_list->iAddressCount == 0xcccccccc, "Got %u addresses.\n", address_list->iAddressCount);

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, 0, &size, &overlapped, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);
    ok(overlapped.Internal == 0xdeadbeef, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "Got size %Iu.\n", overlapped.InternalHigh);

    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, 1, &size, &overlapped, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "Got unexpected error %d.\n", WSAGetLastError());
    ok(!size, "Expected size %lu, got %lu.\n", expect_size, size);
    ok(overlapped.Internal == 0xdeadbeef, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "Got size %Iu.\n", overlapped.InternalHigh);

    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer,
            FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[0]), &size, &overlapped, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);
    ok(overlapped.Internal == 0xdeadbeef, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "Got size %Iu.\n", overlapped.InternalHigh);
    ok(address_list->iAddressCount == 0xcccccccc, "Got %u addresses.\n", address_list->iAddressCount);

    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, sizeof(buffer), &size, &overlapped, NULL);
    ok(!ret, "Got unexpected ret %d.\n", ret);
    ok(!WSAGetLastError(), "Got unexpected error %d.\n", WSAGetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(!size, "Got size %lu.\n", size);
    ok(overlapped_ptr == &overlapped, "Got overlapped %p.\n", overlapped_ptr);
    ok(!overlapped.Internal, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "Got size %Iu.\n", overlapped.InternalHigh);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    closesocket(s);
    CloseHandle(port);

    /* Test with an APC. */

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, sizeof(buffer), NULL, &overlapped, socket_apc);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    apc_count = 0;
    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, sizeof(buffer), &size, &overlapped, socket_apc);
    ok(!ret, "expected success\n");
    ok(size == expect_size, "got size %lu\n", size);

    ret = SleepEx(0, TRUE);
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(!apc_error, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);

    closesocket(s);
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
    ok(port != 0, "CreateIoCompletionPort error %lu\n", GetLastError());

    buf.len = sizeof(data);
    buf.buf = data;
    bytes = 0xdeadbeef;
    flags = 0;
    SetLastError(0xdeadbeef);
    ret = WSARecv(src, &buf, 1, &bytes, &flags, &ovl, NULL);
    ok(ret == SOCKET_ERROR, "got %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %#Ix\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    ret = send(dst, "Hello World!", 12, 0);
    ok(ret == 12, "send returned %d\n", ret);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = NULL;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(ret, "got %d\n", ret);
    ok(bytes == 12, "got bytes %lu\n", bytes);
    ok(key == 0x12345678, "got key %#Ix\n", key);
    ok(ovl_iocp == &ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(ovl_iocp->InternalHigh == 12, "got %#Ix\n", ovl_iocp->InternalHigh);
        ok(!ovl_iocp->Internal , "got %#Ix\n", ovl_iocp->Internal);
        ok(!memcmp(data, "Hello World!", 12), "got %lu bytes (%*s)\n", bytes, (int)bytes, data);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %#Ix\n", key);
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
    ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
    ok(msg.lParam == 2, "got %08Ix\n", msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&ovl, 0, sizeof(ovl));

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %lu\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    buf.len = sizeof(data);
    buf.buf = data;
    bytes = 0xdeadbeef;
    flags = 0;
    SetLastError(0xdeadbeef);
    ret = WSARecv(src, &buf, 1, &bytes, &flags, &ovl, NULL);
    ok(ret == SOCKET_ERROR, "got %d\n", ret);
    ok(GetLastError() == ERROR_IO_PENDING, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %#Ix\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

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
        ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);
        break;
    case 1:
    case 2:
todo_wine
{
        ok(ret, "got %d\n", ret);
        ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
        ok(msg.lParam == 0x20, "got %08Ix\n", msg.lParam);
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
    ok(GetLastError() == ERROR_CONNECTION_ABORTED || GetLastError() == ERROR_NETNAME_DELETED /* XP */, "got %lu\n", GetLastError());
    ok(!bytes, "got bytes %lu\n", bytes);
    ok(key == 0x12345678, "got key %#Ix\n", key);
    ok(ovl_iocp == &ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(!ovl_iocp->InternalHigh, "got %#Ix\n", ovl_iocp->InternalHigh);
    todo_wine
        ok(ovl_iocp->Internal == (ULONG)STATUS_CONNECTION_ABORTED || ovl_iocp->Internal == (ULONG)STATUS_LOCAL_DISCONNECT /* XP */, "got %#Ix\n", ovl_iocp->Internal);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %#Ix\n", key);
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
    ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
    ok(msg.lParam == 2, "got %08Ix\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %lu\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %Iu\n", key);
    ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    closesocket(src);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %Iu\n", key);
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
    ok(GetLastError() == ERROR_IO_PENDING, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);

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
    ok(thread != 0, "CreateThread error %lu\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
    ok(msg.lParam == 2, "got %08Ix\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %lu\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&recv_info.ovl, 0, sizeof(recv_info.ovl));
    recv_info.sock = src;
    recv_info.wsa_buf.len = sizeof(data);
    recv_info.wsa_buf.buf = data;
    thread = CreateThread(NULL, 0, wsa_recv_thread, &recv_info, 0, &tid);
    ok(thread != 0, "CreateThread error %lu\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT || broken(GetLastError() == ERROR_OPERATION_ABORTED) /* XP */,
       "got %lu\n", GetLastError());
    if (GetLastError() == WAIT_TIMEOUT)
    {
        ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
        ok(key == 0xdeadbeef, "got key %Ix\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }
    else /* document XP behaviour */
    {
        ok(!bytes, "got bytes %lu\n", bytes);
        ok(key == 0x12345678, "got key %#Ix\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(!ovl_iocp->InternalHigh, "got %#Ix\n", ovl_iocp->InternalHigh);
            ok(ovl_iocp->Internal == STATUS_CANCELLED, "got %#Ix\n", ovl_iocp->Internal);
        }

        closesocket(src);
        goto xp_is_broken;
    }

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    closesocket(src);

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = NULL;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    todo_wine
    ok(GetLastError() == ERROR_CONNECTION_ABORTED || GetLastError() == ERROR_NETNAME_DELETED /* XP */, "got %lu\n", GetLastError());
    ok(!bytes, "got bytes %lu\n", bytes);
    ok(key == 0x12345678, "got key %#Ix\n", key);
    ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
    if (ovl_iocp)
    {
        ok(!ovl_iocp->InternalHigh, "got %#Ix\n", ovl_iocp->InternalHigh);
    todo_wine
        ok(ovl_iocp->Internal == (ULONG)STATUS_CONNECTION_ABORTED || ovl_iocp->Internal == (ULONG)STATUS_LOCAL_DISCONNECT /* XP */, "got %#Ix\n", ovl_iocp->Internal);
    }

xp_is_broken:
    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT, "got %lu\n", GetLastError());
    ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
    ok(key == 0xdeadbeef, "got key %Iu\n", key);
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
    ok(thread != 0, "CreateThread error %lu\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(ret, "got %d\n", ret);
    ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
    ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
    ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
    ok(msg.lParam == 2, "got %08Ix\n", msg.lParam);

    port = CreateIoCompletionPort((HANDLE)src, 0, 0x12345678, 0);
    ok(port != 0, "CreateIoCompletionPort error %lu\n", GetLastError());

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    memset(data, 0, sizeof(data));
    memset(&recv_info.ovl, 0, sizeof(recv_info.ovl));
    recv_info.sock = src;
    recv_info.wsa_buf.len = sizeof(data);
    recv_info.wsa_buf.buf = data;
    thread = CreateThread(NULL, 0, wsa_recv_thread, &recv_info, 0, &tid);
    ok(thread != 0, "CreateThread error %lu\n", GetLastError());
    ret = WaitForSingleObject(thread, 10000);
    ok(ret == WAIT_OBJECT_0, "thread failed to terminate\n");

    Sleep(100);
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == WAIT_TIMEOUT || broken(GetLastError() == ERROR_OPERATION_ABORTED) /* XP */, "got %lu\n", GetLastError());
    if (GetLastError() == WAIT_TIMEOUT)
    {
        ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
        ok(key == 0xdeadbeef, "got key %Iu\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }
    else /* document XP behaviour */
    {
        ok(bytes == 0, "got bytes %lu\n", bytes);
        ok(key == 0x12345678, "got key %#Ix\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(!ovl_iocp->InternalHigh, "got %#Ix\n", ovl_iocp->InternalHigh);
            ok(ovl_iocp->Internal == STATUS_CANCELLED, "got %#Ix\n", ovl_iocp->Internal);
        }
    }

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret || broken(msg.hwnd == hwnd) /* XP */, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);
    if (ret) /* document XP behaviour */
    {
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
        ok(msg.lParam == 1, "got %08Ix\n", msg.lParam);
    }

    ret = send(dst, "Hello World!", 12, 0);
    ok(ret == 12, "send returned %d\n", ret);

    Sleep(100);
    memset(&msg, 0, sizeof(msg));
    ret = PeekMessageA(&msg, hwnd, WM_SOCKET, WM_SOCKET, PM_REMOVE);
    ok(!ret || broken(msg.hwnd == hwnd) /* XP */, "got %04x,%08Ix,%08Ix\n", msg.message, msg.wParam, msg.lParam);
    if (ret) /* document XP behaviour */
    {
        ok(msg.hwnd == hwnd, "got %p\n", msg.hwnd);
        ok(msg.message == WM_SOCKET, "got %04x\n", msg.message);
        ok(msg.wParam == src, "got %08Ix\n", msg.wParam);
        ok(msg.lParam == 1, "got %08Ix\n", msg.lParam);
    }

    bytes = 0xdeadbeef;
    key = 0xdeadbeef;
    ovl_iocp = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetQueuedCompletionStatus(port, &bytes, &key, &ovl_iocp, 100);
    ok(ret || broken(GetLastError() == WAIT_TIMEOUT) /* XP */, "got %lu\n", GetLastError());
    if (ret)
    {
        ok(bytes == 12, "got bytes %lu\n", bytes);
        ok(key == 0x12345678, "got key %#Ix\n", key);
        ok(ovl_iocp == &recv_info.ovl, "got ovl %p\n", ovl_iocp);
        if (ovl_iocp)
        {
            ok(ovl_iocp->InternalHigh == 12, "got %#Ix\n", ovl_iocp->InternalHigh);
            ok(!ovl_iocp->Internal , "got %#Ix\n", ovl_iocp->Internal);
            ok(!memcmp(data, "Hello World!", 12), "got %lu bytes (%*s)\n", bytes, (int)bytes, data);
        }
    }
    else /* document XP behaviour */
    {
        ok(bytes == 0xdeadbeef, "got bytes %lu\n", bytes);
        ok(key == 0xdeadbeef, "got key %Iu\n", key);
        ok(!ovl_iocp, "got ovl %p\n", ovl_iocp);
    }

    CloseHandle(port);

    DestroyWindow(hwnd);
}

static void test_iocp(void)
{
    SOCKET src, dst;
    int i;

    tcp_socketpair(&src, &dst);
    sync_read(src, dst);
    iocp_async_read(src, dst);
    closesocket(src);
    closesocket(dst);

    tcp_socketpair(&src, &dst);
    iocp_async_read_thread(src, dst);
    closesocket(src);
    closesocket(dst);

    for (i = 0; i <= 2; i++)
    {
        tcp_socketpair(&src, &dst);
        iocp_async_read_closesocket(src, i);
        closesocket(dst);
    }

    tcp_socketpair(&src, &dst);
    iocp_async_closesocket(src);
    closesocket(dst);

    tcp_socketpair(&src, &dst);
    iocp_async_read_thread_closesocket(src);
    closesocket(dst);
}

static void test_get_interface_list(void)
{
    OVERLAPPED overlapped = {0}, *overlapped_ptr;
    DWORD size, expect_size;
    unsigned int i, count;
    INTERFACE_INFO *info;
    BOOL loopback_found;
    char buffer[4096];
    ULONG_PTR key;
    HANDLE port;
    SOCKET s;
    int ret;

    s = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(s != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    port = CreateIoCompletionPort((HANDLE)s, NULL, 123, 0);

    size = 0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(buffer), &size, NULL, NULL);
    ok(!ret, "Got unexpected ret %d.\n", ret);
    ok(!WSAGetLastError(), "Got error %u.\n", WSAGetLastError());
    ok(size && size != 0xdeadbeef && !(size % sizeof(INTERFACE_INFO)), "Got unexpected size %lu.\n", size);
#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03)
        expect_size = 0;
    else
#endif
    expect_size = size;

    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(buffer), &size, &overlapped, NULL);
#ifdef __REACTOS__
    if (ret) { /* > WS03  */
#endif
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "Got error %u.\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);
#ifdef __REACTOS__
    }
#endif

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 100);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(size == expect_size, "Expected size %lu, got %lu.\n", expect_size, size);
    ok(key == 123, "Got key %Iu.\n", key);
    ok(overlapped_ptr == &overlapped, "Got overlapped %p.\n", overlapped_ptr);
    ok(!overlapped.Internal, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == expect_size, "Expected size %lu, got %Iu.\n", expect_size, overlapped.InternalHigh);

    info = (INTERFACE_INFO *)buffer;
    count = size / sizeof(INTERFACE_INFO);
    loopback_found = FALSE;
    for (i = 0; i < count; ++i)
    {
        if (info[i].iiFlags & IFF_LOOPBACK)
            loopback_found = TRUE;

        ok(info[i].iiAddress.AddressIn.sin_family == AF_INET, "Got unexpected sin_family %#x.\n",
                info[i].iiAddress.AddressIn.sin_family);
        ok(info[i].iiNetmask.AddressIn.sin_family == AF_INET, "Got unexpected sin_family %#x.\n",
                info[i].iiNetmask.AddressIn.sin_family);
        ok(info[i].iiBroadcastAddress.AddressIn.sin_family
                == (info[i].iiFlags & IFF_BROADCAST) ? AF_INET : 0, "Got unexpected sin_family %#x.\n",
                info[i].iiBroadcastAddress.AddressIn.sin_family);
        ok(info[i].iiAddress.AddressIn.sin_addr.S_un.S_addr, "Got zero iiAddress.\n");
        ok(info[i].iiNetmask.AddressIn.sin_addr.S_un.S_addr, "Got zero iiNetmask.\n");
        ok((info[i].iiFlags & IFF_BROADCAST) ? info[i].iiBroadcastAddress.AddressIn.sin_addr.S_un.S_addr
                : !info[i].iiBroadcastAddress.AddressIn.sin_addr.S_un.S_addr,
                "Got unexpected iiBroadcastAddress %s.\n", inet_ntoa(info[i].iiBroadcastAddress.AddressIn.sin_addr));
    }

#ifdef __REACTOS__
    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
#endif
    ok(loopback_found, "Loopback interface not found.\n");

    size = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(INTERFACE_INFO) - 1, &size, NULL, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());
#ifdef __REACTOS__
    ok(!size || broken(size == 0xdeadbeef) /* WS03 */, "Got unexpected size %lu.\n", size);
#else
    ok(!size, "Got unexpected size %lu.\n", size);
#endif

    size = 0xdeadbeef;
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(INTERFACE_INFO) - 1, &size, &overlapped, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
#ifdef __REACTOS__
    ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEFAULT) /* WS03 */, "Got error %u.\n", WSAGetLastError());
#else
    ok(WSAGetLastError() == ERROR_IO_PENDING, "Got error %u.\n", WSAGetLastError());
#endif
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);

    ret = GetQueuedCompletionStatus(port, &size, &key, &overlapped_ptr, 100);
    ok(!ret, "Expected failure.\n");
#ifdef __REACTOS__
    if (GetLastError() != WAIT_TIMEOUT) { /* > WS03 */
#endif
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got error %lu.\n", GetLastError());
    ok(!size, "Got size %lu.\n", size);
    ok(key == 123, "Got key %Iu.\n", key);
    ok(overlapped_ptr == &overlapped, "Got overlapped %p.\n", overlapped_ptr);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_TOO_SMALL, "Got status %#lx.\n", (NTSTATUS)overlapped.Internal);
    ok(!overlapped.InternalHigh, "Got size %Iu.\n", overlapped.InternalHigh);
#ifdef __REACTOS__
    }
#endif

    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, sizeof(buffer), NULL, NULL, NULL);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "Got unexpected error %d.\n", WSAGetLastError());

    CloseHandle(port);
    closesocket(s);

    /* Test with an APC. */

    s = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ok(s != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());

    size = 0xdeadbeef;
    apc_count = 0;
    ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, buffer,
            sizeof(INTERFACE_INFO) - 1, &size, &overlapped, socket_apc);
    ok(ret == -1, "Got unexpected ret %d.\n", ret);
#ifdef __REACTOS__
    ok(WSAGetLastError() == ERROR_IO_PENDING || broken(WSAGetLastError() == WSAEFAULT) /* WS03 */, "Got error %u.\n", WSAGetLastError());
#else
    ok(WSAGetLastError() == ERROR_IO_PENDING, "Got error %u.\n", WSAGetLastError());
#endif
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);

    ret = SleepEx(100, TRUE);
#ifdef __REACTOS__
    if (ret) {
#endif
    ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
    ok(apc_count == 1, "APC was called %u times\n", apc_count);
    ok(apc_error == WSAEFAULT, "got APC error %lu\n", apc_error);
    ok(!apc_size, "got APC size %lu\n", apc_size);
    ok(apc_overlapped == &overlapped, "got APC overlapped %p\n", apc_overlapped);
#ifdef __REACTOS__
    }
#endif

    closesocket(s);
}

static IP_ADAPTER_ADDRESSES *get_adapters(void)
{
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( AF_UNSPEC, 0, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, 0, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static void test_bind(void)
{
    const struct sockaddr_in invalid_addr = {.sin_family = AF_INET, .sin_addr.s_addr = inet_addr("192.0.2.0")};
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    IP_ADAPTER_ADDRESSES *adapters, *adapter;
    struct sockaddr addr;
    SOCKET s, s2;
    int ret, len;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    WSASetLastError(0xdeadbeef);
    ret = bind(s, NULL, 0);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, NULL, sizeof(addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    addr.sa_family = AF_INET;
    WSASetLastError(0xdeadbeef);
    ret = bind(s, &addr, 0);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    addr.sa_family = 0xdead;
    WSASetLastError(0xdeadbeef);
    ret = bind(s, &addr, sizeof(addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&bind_addr, sizeof(bind_addr) - 1);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEADDRNOTAVAIL, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* win <7 */, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    len = sizeof(addr);
    ret = getsockname(s, &addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    s2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    WSASetLastError(0xdeadbeef);
    ret = bind(s2, &addr, sizeof(addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEADDRINUSE, "got error %u\n", WSAGetLastError());

    closesocket(s2);
    closesocket(s);

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEADDRNOTAVAIL, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = bind(s, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError() || WSAGetLastError() == 0xdeadbeef /* win <7 */, "got error %u\n", WSAGetLastError());

    closesocket(s);

    adapters = get_adapters();
    ok(adapters != NULL, "can't get adapters\n");

    for (adapter = adapters; adapter != NULL; adapter = adapter->Next)
    {
        const IP_ADAPTER_UNICAST_ADDRESS *unicast_addr;

        if (adapter->OperStatus != IfOperStatusUp) continue;

        for (unicast_addr = adapter->FirstUnicastAddress; unicast_addr != NULL; unicast_addr = unicast_addr->Next)
        {
            short family = unicast_addr->Address.lpSockaddr->sa_family;

            s = socket(family, SOCK_STREAM, IPPROTO_TCP);
            ok(s != -1, "failed to create socket, error %u\n", WSAGetLastError());

            ret = bind(s, unicast_addr->Address.lpSockaddr, unicast_addr->Address.iSockaddrLength);
            ok(!ret, "got error %u\n", WSAGetLastError());

            closesocket(s);

            if (family == AF_INET6)
            {
                struct sockaddr_in6 addr6, ret_addr6;

                memcpy(&addr6, unicast_addr->Address.lpSockaddr, sizeof(addr6));

                ok(unicast_addr->Address.iSockaddrLength == sizeof(struct sockaddr_in6),
                        "got unexpected length %u\n", unicast_addr->Address.iSockaddrLength);

                s = socket(family, SOCK_STREAM, IPPROTO_TCP);
                ok(s != -1, "failed to create socket, error %u\n", WSAGetLastError());

                ret = bind(s, unicast_addr->Address.lpSockaddr, sizeof(struct sockaddr_in6_old));
                ok(ret == -1, "expected failure\n");
                ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

                addr6.sin6_scope_id = 0xabacab;
                ret = bind(s, (struct sockaddr *)&addr6, sizeof(addr6));
                todo_wine_if (!((const struct sockaddr_in6 *)unicast_addr->Address.lpSockaddr)->sin6_scope_id)
                {
                    ok(ret == -1, "expected failure\n");
                    ok(WSAGetLastError() == WSAEADDRNOTAVAIL, "got error %u\n", WSAGetLastError());
                }

                addr6.sin6_scope_id = 0;
                ret = bind(s, (struct sockaddr *)&addr6, sizeof(addr6));
                todo_wine_if (!((const struct sockaddr_in6 *)unicast_addr->Address.lpSockaddr)->sin6_scope_id)
                    ok(!ret, "got error %u\n", WSAGetLastError());

                memcpy(&addr6, unicast_addr->Address.lpSockaddr, sizeof(addr6));

                len = sizeof(struct sockaddr_in6_old);
                ret = getsockname(s, (struct sockaddr *)&ret_addr6, &len);
                ok(ret == -1, "expected failure\n");
                ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

                len = sizeof(ret_addr6);
                memset(&ret_addr6, 0, sizeof(ret_addr6));
                ret = getsockname(s, (struct sockaddr *)&ret_addr6, &len);
                ok(!ret, "got error %u\n", WSAGetLastError());
                ok(ret_addr6.sin6_family == AF_INET6, "got family %u\n", ret_addr6.sin6_family);
                ok(ret_addr6.sin6_port != 0, "expected nonzero port\n");
                ok(!memcmp(&ret_addr6.sin6_addr, &addr6.sin6_addr, sizeof(addr6.sin6_addr)), "address didn't match\n");
                ok(ret_addr6.sin6_scope_id == addr6.sin6_scope_id, "got scope %lu\n", ret_addr6.sin6_scope_id);

                closesocket(s);
            }
        }
    }

    free(adapters);
}

/* Test calling methods on a socket which is currently connecting. */
static void test_connecting_socket(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_ANY)};
    const struct sockaddr_in invalid_addr =
    {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr("192.0.2.0"),
        .sin_port = 255
    };
    OVERLAPPED overlapped = {0}, overlapped2 = {0};
    GUID connectex_guid = WSAID_CONNECTEX;
    LPFN_CONNECTEX pConnectEx;
    struct sockaddr_in addr;
    char buffer[4];
    SOCKET client;
    int ret, len;
    DWORD size;

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    set_blocking(client, FALSE);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got %u\n", WSAGetLastError());

    /* Mortal Kombat 11 connects to the same address twice and expects the
     * second to return WSAEALREADY. */
    ret = connect(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEALREADY, "got %u\n", WSAGetLastError());

    ret = WSAIoctl(client, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
            &pConnectEx, sizeof(pConnectEx), &size, NULL, NULL);
    ok(!ret, "failed to get ConnectEx, error %u\n", WSAGetLastError());
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = pConnectEx(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr), NULL, 0, &size, &overlapped);
    ok(!ret, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "got %u\n", WSAGetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    len = sizeof(addr);
    ret = getsockname(client, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(addr.sin_family == AF_INET, "got family %u\n", addr.sin_family);
    ok(addr.sin_port, "expected nonzero port\n");

    len = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAENOTCONN, "got %u\n", WSAGetLastError());

    ret = send(client, "data", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAENOTCONN, "got %u\n", WSAGetLastError());

    closesocket(client);

    /* Test with ConnectEx(). */

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(client != -1, "failed to create socket, error %u\n", WSAGetLastError());
    set_blocking(client, FALSE);

    ret = bind(client, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got %u\n", WSAGetLastError());

    ret = pConnectEx(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr), NULL, 0, &size, &overlapped2);
    ok(!ret, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = connect(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr));
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "got %u\n", WSAGetLastError());

    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = pConnectEx(client, (struct sockaddr *)&invalid_addr, sizeof(invalid_addr), NULL, 0, &size, &overlapped);
    ok(!ret, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEINVAL, "got %u\n", WSAGetLastError());
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    todo_wine ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    len = sizeof(addr);
    ret = getsockname(client, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(addr.sin_family == AF_INET, "got family %u\n", addr.sin_family);
    ok(addr.sin_port, "expected nonzero port\n");

    len = sizeof(addr);
    ret = getpeername(client, (struct sockaddr *)&addr, &len);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAENOTCONN, "got %u\n", WSAGetLastError());

    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAENOTCONN, "got %u\n", WSAGetLastError());

    ret = send(client, "data", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(WSAGetLastError() == WSAENOTCONN, "got %u\n", WSAGetLastError());

    closesocket(client);
}

static DWORD map_status( NTSTATUS status )
{
    static const struct
    {
        NTSTATUS status;
        DWORD error;
    }
    errors[] =
    {
        {STATUS_PENDING,                    ERROR_IO_INCOMPLETE},

        {STATUS_BUFFER_OVERFLOW,            WSAEMSGSIZE},

        {STATUS_NOT_IMPLEMENTED,            WSAEOPNOTSUPP},
        {STATUS_ACCESS_VIOLATION,           WSAEFAULT},
        {STATUS_PAGEFILE_QUOTA,             WSAENOBUFS},
        {STATUS_INVALID_HANDLE,             WSAENOTSOCK},
        {STATUS_NO_SUCH_DEVICE,             WSAENETDOWN},
        {STATUS_NO_SUCH_FILE,               WSAENETDOWN},
        {STATUS_NO_MEMORY,                  WSAENOBUFS},
        {STATUS_CONFLICTING_ADDRESSES,      WSAENOBUFS},
        {STATUS_ACCESS_DENIED,              WSAEACCES},
        {STATUS_BUFFER_TOO_SMALL,           WSAEFAULT},
        {STATUS_OBJECT_TYPE_MISMATCH,       WSAENOTSOCK},
        {STATUS_OBJECT_NAME_NOT_FOUND,      WSAENETDOWN},
        {STATUS_OBJECT_PATH_NOT_FOUND,      WSAENETDOWN},
        {STATUS_SHARING_VIOLATION,          WSAEADDRINUSE},
        {STATUS_QUOTA_EXCEEDED,             WSAENOBUFS},
        {STATUS_TOO_MANY_PAGING_FILES,      WSAENOBUFS},
        {STATUS_INSUFFICIENT_RESOURCES,     WSAENOBUFS},
        {STATUS_WORKING_SET_QUOTA,          WSAENOBUFS},
        {STATUS_DEVICE_NOT_READY,           WSAEWOULDBLOCK},
        {STATUS_PIPE_DISCONNECTED,          WSAESHUTDOWN},
        {STATUS_IO_TIMEOUT,                 WSAETIMEDOUT},
        {STATUS_NOT_SUPPORTED,              WSAEOPNOTSUPP},
        {STATUS_REMOTE_NOT_LISTENING,       WSAECONNREFUSED},
        {STATUS_BAD_NETWORK_PATH,           WSAENETUNREACH},
        {STATUS_NETWORK_BUSY,               WSAENETDOWN},
        {STATUS_INVALID_NETWORK_RESPONSE,   WSAENETDOWN},
        {STATUS_UNEXPECTED_NETWORK_ERROR,   WSAENETDOWN},
        {STATUS_REQUEST_NOT_ACCEPTED,       WSAEWOULDBLOCK},
        {STATUS_CANCELLED,                  ERROR_OPERATION_ABORTED},
        {STATUS_COMMITMENT_LIMIT,           WSAENOBUFS},
        {STATUS_LOCAL_DISCONNECT,           WSAECONNABORTED},
        {STATUS_REMOTE_DISCONNECT,          WSAECONNRESET},
        {STATUS_REMOTE_RESOURCES,           WSAENOBUFS},
        {STATUS_LINK_FAILED,                WSAECONNRESET},
        {STATUS_LINK_TIMEOUT,               WSAETIMEDOUT},
        {STATUS_INVALID_CONNECTION,         WSAENOTCONN},
        {STATUS_INVALID_ADDRESS,            WSAEADDRNOTAVAIL},
        {STATUS_INVALID_BUFFER_SIZE,        WSAEMSGSIZE},
        {STATUS_INVALID_ADDRESS_COMPONENT,  WSAEADDRNOTAVAIL},
        {STATUS_TOO_MANY_ADDRESSES,         WSAENOBUFS},
        {STATUS_ADDRESS_ALREADY_EXISTS,     WSAEADDRINUSE},
        {STATUS_CONNECTION_DISCONNECTED,    WSAECONNRESET},
        {STATUS_CONNECTION_RESET,           WSAECONNRESET},
        {STATUS_TRANSACTION_ABORTED,        WSAECONNABORTED},
        {STATUS_CONNECTION_REFUSED,         WSAECONNREFUSED},
        {STATUS_GRACEFUL_DISCONNECT,        WSAEDISCON},
        {STATUS_CONNECTION_ACTIVE,          WSAEISCONN},
        {STATUS_NETWORK_UNREACHABLE,        WSAENETUNREACH},
        {STATUS_HOST_UNREACHABLE,           WSAEHOSTUNREACH},
        {STATUS_PROTOCOL_UNREACHABLE,       WSAENETUNREACH},
        {STATUS_PORT_UNREACHABLE,           WSAECONNRESET},
        {STATUS_REQUEST_ABORTED,            WSAEINTR},
        {STATUS_CONNECTION_ABORTED,         WSAECONNABORTED},
        {STATUS_DATATYPE_MISALIGNMENT_ERROR,WSAEFAULT},
        {STATUS_HOST_DOWN,                  WSAEHOSTDOWN},
        {0x80070000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
        {0xc0010000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
        {0xc0070000 | ERROR_IO_INCOMPLETE,  ERROR_IO_INCOMPLETE},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(errors); ++i)
    {
        if (errors[i].status == status)
            return errors[i].error;
    }

    return NT_SUCCESS(status) ? RtlNtStatusToDosErrorNoTeb(status) : WSAEINVAL;
}

static void test_WSAGetOverlappedResult(void)
{
    OVERLAPPED overlapped = {0};
    DWORD size, flags;
    NTSTATUS status;
    unsigned int i;
    SOCKET s;
    HANDLE h;
    BOOL ret;

    static const NTSTATUS ranges[][2] =
    {
        {0x0, 0x10000},
#ifndef __REACTOS__ // These tests crash on Windows
        {0x40000000, 0x40001000},
        {0x80000000, 0x80001000},
        {0x80070000, 0x80080000},
        {0xc0000000, 0xc0001000},
        {0xc0070000, 0xc0080000},
        {0xd0000000, 0xd0001000},
        {0xd0070000, 0xd0080000},
#endif
    };

    WSASetLastError(0xdeadbeef);
    ret = WSAGetOverlappedResult(0xdeadbeef, &overlapped, &size, FALSE, &flags);
    ok(!ret, "got %d.\n", ret);
    ok(WSAGetLastError() == WSAENOTSOCK, "got %u.\n", WSAGetLastError());

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = DuplicateHandle(GetCurrentProcess(), (HANDLE)s, GetCurrentProcess(), &h, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "got %d.\n", ret);
    ret = WSAGetOverlappedResult((SOCKET)h, &overlapped, &size, FALSE, &flags);
    ok(!ret, "got %d.\n", ret);
    ok(WSAGetLastError() == WSAENOTSOCK, "got %u.\n", WSAGetLastError());
    CloseHandle(h);

    for (i = 0; i < ARRAY_SIZE(ranges); ++i)
    {
        for (status = ranges[i][0]; status < ranges[i][1]; ++status)
        {
            BOOL expect_ret = NT_SUCCESS(status) && status != STATUS_PENDING;
            DWORD expect = map_status(status);

            overlapped.Internal = status;
            WSASetLastError(0xdeadbeef);
            ret = WSAGetOverlappedResult(s, &overlapped, &size, FALSE, &flags);
            ok(ret == expect_ret, "status %#lx: expected %d, got %d\n", status, expect_ret, ret);

            if (ret)
            {
                ok(WSAGetLastError() == expect /* >= win10 1809 */
                        || !WSAGetLastError() /* < win10 1809 */
                        || WSAGetLastError() == 0xdeadbeef, /* < win7 */
                        "status %#lx: expected error %lu, got %u\n", status, expect, WSAGetLastError());
            }
            else
            {
                ok(WSAGetLastError() == expect
                        || (status == (0xc0070000 | ERROR_IO_INCOMPLETE) && WSAGetLastError() == WSAEINVAL), /* < win8 */
                        "status %#lx: expected error %lu, got %u\n", status, expect, WSAGetLastError());
            }
        }
    }

    overlapped.Internal = STATUS_PENDING;
    overlapped.hEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

    apc_count = 0;
    ret = QueueUserAPC(apc_func, GetCurrentThread(), (ULONG_PTR)&apc_count);
    ok(ret, "QueueUserAPC returned %d\n", ret);
    ret = WSAGetOverlappedResult(s, &overlapped, &size, TRUE, &flags);
    ok(ret && (GetLastError() == ERROR_IO_PENDING || !WSAGetLastError()),
            "Got ret %d, err %lu.\n", ret, GetLastError());
    ok(!apc_count, "got apc_count %d.\n", apc_count);
    SleepEx(0, TRUE);
    ok(apc_count == 1, "got apc_count %d.\n", apc_count);

    CloseHandle(overlapped.hEvent);
    closesocket(s);
}

struct nonblocking_async_recv_params
{
    SOCKET client;
    HANDLE event;
};

static DWORD CALLBACK nonblocking_async_recv_thread(void *arg)
{
    const struct nonblocking_async_recv_params *params = arg;
    OVERLAPPED overlapped = {0};
    DWORD flags = 0, size;
    char buffer[5];
    WSABUF wsabuf;
    int ret;

    overlapped.hEvent = params->event;
    wsabuf.buf = buffer;
    wsabuf.len = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    ret = WSARecv(params->client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
#ifdef __REACTOS__
    ok(!ret || broken(ret == SOCKET_ERROR) /* WS03 */, "got %d\n", ret);
#else
    ok(!ret, "got %d\n", ret);
#endif
    ret = GetOverlappedResult((HANDLE)params->client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 4, "got size %lu\n", size);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, size));

    return 0;
}

static void test_nonblocking_async_recv(void)
{
    struct nonblocking_async_recv_params params;
    OVERLAPPED overlapped = {0};
    SOCKET client, server;
    DWORD flags = 0, size;
    HANDLE thread, event;
    char buffer[5];
    WSABUF wsabuf;
    int ret;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    wsabuf.buf = buffer;
    wsabuf.len = sizeof(buffer);

    tcp_socketpair(&client, &server);
    set_blocking(client, FALSE);
    set_blocking(server, FALSE);

    WSASetLastError(0xdeadbeef);
    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, NULL, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == 0xdeadbeef, "got status %#lx\n", (NTSTATUS)overlapped.Internal);

    /* Overlapped, with a NULL event. */

    overlapped.hEvent = NULL;

    memset(buffer, 0, sizeof(buffer));
    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    overlapped.InternalHigh = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
    ret = WaitForSingleObject((HANDLE)client, 0);
    ok(ret == WAIT_TIMEOUT, "expected timeout\n");
    ok(overlapped.Internal == STATUS_PENDING, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "got size %Iu\n", overlapped.InternalHigh);

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = WaitForSingleObject((HANDLE)client, 1000);
    ok(!ret, "wait timed out\n");
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 4, "got size %lu\n", size);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, size));

    /* Overlapped, with a non-NULL event. */

    overlapped.hEvent = event;

    memset(buffer, 0, sizeof(buffer));
    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "expected timeout\n");

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = WaitForSingleObject(event, 1000);
    ok(!ret, "wait timed out\n");
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 4, "got size %lu\n", size);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, size));

    /* With data already in the pipe; usually this does return 0 (but not
     * reliably). */

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = WSARecv(client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(!ret || WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
    ret = WaitForSingleObject(event, 1000);
    ok(!ret, "wait timed out\n");
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 4, "got size %lu\n", size);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, size));

    closesocket(client);
    closesocket(server);

    /* With a non-overlapped socket, WSARecv() always blocks when passed an
     * overlapped structure, but returns WSAEWOULDBLOCK otherwise. */

    tcp_socketpair_flags(&client, &server, 0);
    set_blocking(client, FALSE);
    set_blocking(server, FALSE);

    WSASetLastError(0xdeadbeef);
    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    overlapped.Internal = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, NULL, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u\n", WSAGetLastError());
    ok(overlapped.Internal == 0xdeadbeef, "got status %#lx\n", (NTSTATUS)overlapped.Internal);

    /* Overlapped, with a NULL event. */

    params.client = client;
    params.event = NULL;
    thread = CreateThread(NULL, 0, nonblocking_async_recv_thread, &params, 0, NULL);

    ret = WaitForSingleObject(thread, 200);
    ok(ret == WAIT_TIMEOUT, "expected timeout\n");

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = WaitForSingleObject(thread, 200);
    ok(!ret, "wait timed out\n");
    CloseHandle(thread);

    /* Overlapped, with a non-NULL event. */

    params.client = client;
    params.event = event;
    thread = CreateThread(NULL, 0, nonblocking_async_recv_thread, &params, 0, NULL);

    ret = WaitForSingleObject(thread, 200);
    ok(ret == WAIT_TIMEOUT, "expected timeout\n");

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = WaitForSingleObject(thread, 200);
    ok(!ret, "wait timed out\n");
    CloseHandle(thread);

    /* With data already in the pipe. */

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    memset(buffer, 0, sizeof(buffer));
    ret = WSARecv(client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
#ifdef __REACTOS__
    ok(!ret || broken(ret == SOCKET_ERROR) /* WS03 */, "got %d\n", ret);
#else
    ok(!ret, "got %d\n", ret);
#endif
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 4, "got size %lu\n", size);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, size));

    closesocket(client);
    closesocket(server);

    CloseHandle(overlapped.hEvent);
}

static void test_simultaneous_async_recv(void)
{
    SOCKET client, server;
    OVERLAPPED overlappeds[2] = {{0}};
    HANDLE events[2];
    WSABUF wsabufs[2];
    DWORD flags[2] = {0};
    size_t num_io = 2, stride = 16, i;
    char resbuf[32] = "";
    static const char msgstr[32] = "-- Lorem ipsum dolor sit amet -";
    int ret;

    for (i = 0; i < num_io; i++) events[i] = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    for (i = 0; i < num_io; i++)
    {
        wsabufs[i].buf = resbuf + i * stride;
        wsabufs[i].len = stride;
        overlappeds[i].hEvent = events[i];
        ret = WSARecv(client, &wsabufs[i], 1, NULL, &flags[i], &overlappeds[i], NULL);
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());
    }

    ret = send(server, msgstr, sizeof(msgstr), 0);
    ok(ret == sizeof(msgstr), "got %d\n", ret);

    for (i = 0; i < num_io; i++)
    {
        const void *expect = msgstr + i * stride;
        const void *actual = resbuf + i * stride;
        DWORD size;

        ret = WaitForSingleObject(events[i], 1000);
        ok(!ret, "wait timed out\n");

        size = 0;
        ret = GetOverlappedResult((HANDLE)client, &overlappeds[i], &size, FALSE);
        ok(ret, "got error %lu\n", GetLastError());
        ok(size == stride, "got size %lu\n", size);
        ok(!memcmp(expect, actual, stride), "expected %s, got %s\n", debugstr_an(expect, stride), debugstr_an(actual, stride));
    }

    closesocket(client);
    closesocket(server);

    for (i = 0; i < num_io; i++) CloseHandle(events[i]);
}

static void test_empty_recv(void)
{
    OVERLAPPED overlapped = {0};
    SOCKET client, server;
    DWORD size, flags = 0;
    char buffer[5];
    WSABUF wsabuf;
    int ret;

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This test bugchecks on ReactOS!\n");
        return;
    }
#endif
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    tcp_socketpair(&client, &server);

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, NULL, 0, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    wsabuf.buf = buffer;
    wsabuf.len = 0;
    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 0, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == WSAEINVAL, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, NULL, &flags, &overlapped, NULL);
    ok(ret == -1, "expected failure\n");
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = send(server, "data", 5, 0);
    ok(ret == 5, "got %d\n", ret);

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);

    WSASetLastError(0xdeadbeef);
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!size, "got size %lu\n", size);

    ret = recv(client, NULL, 0, 0);
    ok(!ret, "got %d\n", ret);

    ret = recv(client, buffer, sizeof(buffer), 0);
    ok(ret == 5, "got %d\n", ret);
    ok(!strcmp(buffer, "data"), "got %s\n", debugstr_an(buffer, ret));

    closesocket(client);
    closesocket(server);
    CloseHandle(overlapped.hEvent);
}

static void test_timeout(void)
{
    DWORD timeout, flags = 0, size;
    OVERLAPPED overlapped = {0};
    SOCKET client, server;
    WSABUF wsabuf;
    int ret, len;
    char buffer;

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "Most of this test either hangs or bugchecks on ReactOS!\n");
        return;
    }
#endif
    tcp_socketpair(&client, &server);
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    timeout = 0xdeadbeef;
    len = sizeof(timeout);
    WSASetLastError(0xdeadbeef);
    ret = getsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, &len);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(len == sizeof(timeout), "got size %u\n", len);
    ok(!timeout, "got timeout %lu\n", timeout);

    timeout = 100;
    WSASetLastError(0xdeadbeef);
    ret = setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());

    timeout = 0xdeadbeef;
    len = sizeof(timeout);
    WSASetLastError(0xdeadbeef);
    ret = getsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, &len);
    ok(!ret, "expected success\n");
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(timeout == 100, "got timeout %lu\n", timeout);

    WSASetLastError(0xdeadbeef);
    ret = recv(client, &buffer, 1, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAETIMEDOUT, "got error %u\n", WSAGetLastError());

    wsabuf.buf = &buffer;
    wsabuf.len = 1;
    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, NULL, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAETIMEDOUT, "got error %u\n", WSAGetLastError());
    ok(size == 0xdeadbeef, "got size %lu\n", size);

    wsabuf.buf = &buffer;
    wsabuf.len = 1;
    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(ret == WAIT_TIMEOUT, "got %d\n", ret);

    ret = send(server, "a", 1, 0);
    ok(ret == 1, "got %d\n", ret);

    ret = WaitForSingleObject(overlapped.hEvent, 200);
    ok(!ret, "got %d\n", ret);
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(ret, "got error %lu\n", GetLastError());
    ok(size == 1, "got size %lu\n", size);

    closesocket(client);
    closesocket(server);
    CloseHandle(overlapped.hEvent);
}

static void test_so_debug(void)
{
    int ret, len;
    DWORD debug;
    SOCKET s;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    len = sizeof(debug);
    WSASetLastError(0xdeadbeef);
    debug = 0xdeadbeef;
    ret = getsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&debug, &len);
    ok(!ret, "got %d\n", ret);
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(len == sizeof(debug), "got len %u\n", len);
    ok(!debug, "got debug %lu\n", debug);

    WSASetLastError(0xdeadbeef);
    debug = 2;
    ret = setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&debug, sizeof(debug));
    ok(!ret, "got %d\n", ret);
    todo_wine ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());

    len = sizeof(debug);
    WSASetLastError(0xdeadbeef);
    debug = 0xdeadbeef;
    ret = getsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&debug, &len);
    ok(!ret, "got %d\n", ret);
    ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
    ok(len == sizeof(debug), "got len %u\n", len);
    todo_wine ok(debug == 1, "got debug %lu\n", debug);

    closesocket(s);
}

struct sockopt_validity_test
{
    int opt;
    int get_error;
    int set_error;
    BOOL todo;
};

static void do_sockopt_validity_tests(const char *type, SOCKET sock, int level,
                                      const struct sockopt_validity_test *tests)
{
    char value[256];
    int count, rc, expected_rc, i;

    for (i = 0; tests[i].opt; i++)
    {
        winetest_push_context("%s option %i", type, tests[i].opt);
        memset(value, 0, sizeof(value));
        count = sizeof(value);

        WSASetLastError(0);
        rc = getsockopt(sock, level, tests[i].opt, value, &count);
        expected_rc = tests[i].get_error ? SOCKET_ERROR : 0;
        todo_wine_if(!tests[i].get_error && tests[i].todo)
        ok(rc == expected_rc || broken(rc == SOCKET_ERROR && WSAGetLastError() == WSAENOPROTOOPT),
           "expected getsockopt to return %i, got %i\n", expected_rc, rc);
        todo_wine_if(tests[i].todo)
        ok(WSAGetLastError() == tests[i].get_error || broken(rc == SOCKET_ERROR && WSAGetLastError() == WSAENOPROTOOPT),
           "expected getsockopt to set error %i, got %i\n", tests[i].get_error, WSAGetLastError());

        if (tests[i].get_error)
        {
            winetest_pop_context();
            continue;
        }

        WSASetLastError(0);
        rc = setsockopt(sock, level, tests[i].opt, value, count);
        expected_rc = tests[i].set_error ? SOCKET_ERROR : 0;
        todo_wine_if(!tests[i].set_error && tests[i].todo)
        ok(rc == expected_rc || broken(rc == SOCKET_ERROR && WSAGetLastError() == WSAENOPROTOOPT),
           "expected setsockopt to return %i, got %i\n", expected_rc, rc);
        todo_wine_if(tests[i].todo)
        ok(WSAGetLastError() == tests[i].set_error || broken(rc == SOCKET_ERROR && WSAGetLastError() == WSAENOPROTOOPT),
           "expected setsockopt to set error %i, got %i\n", tests[i].set_error, WSAGetLastError());

        winetest_pop_context();
    }
}

static void test_sockopt_validity(void)
{
#ifdef __REACTOS__
    static struct sockopt_validity_test ipv4_tcp_tests[] =
#else
    static const struct sockopt_validity_test ipv4_tcp_tests[] =
#endif
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IP_OPTIONS                                                    },
        { IP_HDRINCL,                 WSAEINVAL                         },
        { IP_TOS                                                        },
        { IP_TTL                                                        },
        { IP_MULTICAST_IF,            WSAEINVAL                         },
        { IP_MULTICAST_TTL,           WSAEINVAL                         },
        { IP_MULTICAST_LOOP,          WSAEINVAL                         },
        { IP_ADD_MEMBERSHIP,          WSAENOPROTOOPT                    },
        { IP_DROP_MEMBERSHIP,         WSAENOPROTOOPT                    },
        { IP_DONTFRAGMENT                                               },
        { IP_PKTINFO,                 WSAEINVAL                         },
        { IP_RECVTTL,                 WSAEINVAL                         },
        { IP_RECEIVE_BROADCAST,       WSAEINVAL,       0,          TRUE },
        { IP_RECVIF,                  WSAEINVAL,       0,          TRUE },
        { IP_RECVDSTADDR,             WSAEINVAL,       0,          TRUE },
        { IP_IFLIST,                  0,               0,          TRUE },
        { IP_UNICAST_IF                                                 },
        { IP_RTHDR,                   0,               0,          TRUE },
        { IP_GET_IFLIST,              WSAEINVAL,       0,          TRUE },
        { IP_RECVRTHDR,               WSAEINVAL,       0,          TRUE },
        { IP_RECVTOS,                 WSAEINVAL                         },
        { IP_ORIGINAL_ARRIVAL_IF,     WSAEINVAL,       0,          TRUE },
        { IP_ECN,                     WSAEINVAL,       0,          TRUE },
        { IP_PKTINFO_EX,              WSAEINVAL,       0,          TRUE },
        { IP_WFP_REDIRECT_RECORDS,    WSAEINVAL,       0,          TRUE },
        { IP_WFP_REDIRECT_CONTEXT,    WSAEINVAL,       0,          TRUE },
        { IP_MTU_DISCOVER,            0,               WSAEINVAL,  TRUE },
        { IP_MTU,                     WSAENOTCONN,     0,          TRUE },
        { IP_RECVERR,                 WSAEINVAL,       0,          TRUE },
        { IP_USER_MTU,                0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
#ifdef __REACTOS__
    static struct sockopt_validity_test ipv4_udp_tests[] =
#else
    static const struct sockopt_validity_test ipv4_udp_tests[] =
#endif
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IP_OPTIONS                                                    },
        { IP_HDRINCL,                 WSAEINVAL                         },
        { IP_TOS                                                        },
        { IP_TTL                                                        },
        { IP_MULTICAST_IF                                               },
        { IP_MULTICAST_TTL                                              },
        { IP_MULTICAST_LOOP                                             },
        { IP_ADD_MEMBERSHIP,          WSAENOPROTOOPT                    },
        { IP_DROP_MEMBERSHIP,         WSAENOPROTOOPT                    },
        { IP_DONTFRAGMENT                                               },
        { IP_PKTINFO                                                    },
        { IP_RECVTTL                                                    },
        { IP_RECEIVE_BROADCAST,       0,               0,          TRUE },
        { IP_RECVIF,                  0,               0,          TRUE },
        { IP_RECVDSTADDR,             0,               0,          TRUE },
        { IP_IFLIST,                  0,               0,          TRUE },
        { IP_UNICAST_IF                                                 },
        { IP_RTHDR,                   0,               0,          TRUE },
        { IP_GET_IFLIST,              WSAEINVAL,       0,          TRUE },
        { IP_RECVRTHDR,               0,               0,          TRUE },
        { IP_RECVTOS                                                    },
        { IP_ORIGINAL_ARRIVAL_IF,     0,               0,          TRUE },
        { IP_ECN,                     0,               0,          TRUE },
        { IP_PKTINFO_EX,              0,               0,          TRUE },
        { IP_WFP_REDIRECT_RECORDS,    0,               0,          TRUE },
        { IP_WFP_REDIRECT_CONTEXT,    0,               0,          TRUE },
        { IP_MTU_DISCOVER,            0,               WSAEINVAL,  TRUE },
        { IP_MTU,                     WSAENOTCONN,     0,          TRUE },
        { IP_RECVERR,                 0,               0,          TRUE },
        { IP_USER_MTU,                0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
#ifdef __REACTOS__
    static struct sockopt_validity_test ipv4_raw_tests[] =
#else
    static const struct sockopt_validity_test ipv4_raw_tests[] =
#endif
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IP_OPTIONS                                                    },
        { IP_HDRINCL,                                                   },
        { IP_TOS                                                        },
        { IP_TTL                                                        },
        { IP_MULTICAST_IF                                               },
        { IP_MULTICAST_TTL                                              },
        { IP_MULTICAST_LOOP                                             },
        { IP_ADD_MEMBERSHIP,          WSAENOPROTOOPT                    },
        { IP_DROP_MEMBERSHIP,         WSAENOPROTOOPT                    },
        { IP_DONTFRAGMENT                                               },
        { IP_PKTINFO                                                    },
        { IP_RECVTTL                                                    },
        { IP_RECEIVE_BROADCAST,       0,               0,          TRUE },
        { IP_RECVIF,                  0,               0,          TRUE },
        { IP_RECVDSTADDR,             0,               0,          TRUE },
        { IP_IFLIST,                  0,               0,          TRUE },
        { IP_UNICAST_IF                                                 },
        { IP_RTHDR,                   0,               0,          TRUE },
        { IP_GET_IFLIST,              WSAEINVAL,       0,          TRUE },
        { IP_RECVRTHDR,               0,               0,          TRUE },
        { IP_RECVTOS                                                    },
        { IP_ORIGINAL_ARRIVAL_IF,     0,               0,          TRUE },
        { IP_ECN,                     0,               0,          TRUE },
        { IP_PKTINFO_EX,              0,               0,          TRUE },
        { IP_WFP_REDIRECT_RECORDS,    0,               0,          TRUE },
        { IP_WFP_REDIRECT_CONTEXT,    0,               0,          TRUE },
        { IP_MTU_DISCOVER,            0,               WSAEINVAL,  TRUE },
        { IP_MTU,                     WSAENOTCONN,     0,          TRUE },
        { IP_RECVERR,                 WSAEINVAL,       0,          TRUE },
        { IP_USER_MTU,                0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
    static const struct sockopt_validity_test ipv6_tcp_tests[] =
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IPV6_HOPOPTS,               0,               0,          TRUE },
        { IPV6_HDRINCL,               WSAEINVAL,       0,          TRUE },
        { IPV6_UNICAST_HOPS                                             },
        { IPV6_MULTICAST_IF,          WSAEINVAL                         },
        { IPV6_MULTICAST_HOPS,        WSAEINVAL                         },
        { IPV6_MULTICAST_LOOP,        WSAEINVAL                         },
        { IPV6_ADD_MEMBERSHIP,        WSAENOPROTOOPT                    },
        { IPV6_DROP_MEMBERSHIP,       WSAENOPROTOOPT                    },
        { IPV6_DONTFRAG                                                 },
        { IPV6_PKTINFO,               WSAEINVAL                         },
        { IPV6_HOPLIMIT,              WSAEINVAL                         },
        { IPV6_PROTECTION_LEVEL                                         },
        { IPV6_RECVIF,                WSAEINVAL,       0,          TRUE },
        { IPV6_RECVDSTADDR,           WSAEINVAL,       0,          TRUE },
        { IPV6_V6ONLY                                                   },
        { IPV6_IFLIST,                0,               0,          TRUE },
        { IPV6_UNICAST_IF                                               },
        { IPV6_RTHDR,                 0,               0,          TRUE },
        { IPV6_GET_IFLIST,            WSAEINVAL,       0,          TRUE },
        { IPV6_RECVRTHDR,             WSAEINVAL,       0,          TRUE },
        { IPV6_RECVTCLASS,            WSAEINVAL                         },
        { IP_ORIGINAL_ARRIVAL_IF,     WSAEINVAL,       0,          TRUE },
        { IPV6_ECN,                   WSAEINVAL,       0,          TRUE },
        { IPV6_PKTINFO_EX,            WSAEINVAL,       0,          TRUE },
        { IPV6_WFP_REDIRECT_RECORDS,  WSAEINVAL,       0,          TRUE },
        { IPV6_WFP_REDIRECT_CONTEXT,  WSAEINVAL,       0,          TRUE },
        { IPV6_MTU_DISCOVER,          0,               WSAEINVAL,  TRUE },
        { IPV6_MTU,                   WSAENOTCONN,     0,          TRUE },
        { IPV6_RECVERR,               WSAEINVAL,       0,          TRUE },
        { IPV6_USER_MTU,              0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
    static const struct sockopt_validity_test ipv6_udp_tests[] =
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IPV6_HOPOPTS,               0,               0,          TRUE },
        { IPV6_HDRINCL,               WSAEINVAL,       0,          TRUE },
        { IPV6_UNICAST_HOPS                                             },
        { IPV6_MULTICAST_IF                                             },
        { IPV6_MULTICAST_HOPS                                           },
        { IPV6_MULTICAST_LOOP                                           },
        { IPV6_ADD_MEMBERSHIP,        WSAENOPROTOOPT                    },
        { IPV6_DROP_MEMBERSHIP,       WSAENOPROTOOPT                    },
        { IPV6_DONTFRAG                                                 },
        { IPV6_PKTINFO                                                  },
        { IPV6_HOPLIMIT                                                 },
        { IPV6_PROTECTION_LEVEL                                         },
        { IPV6_RECVIF,                0,               0,          TRUE },
        { IPV6_RECVDSTADDR,           0,               0,          TRUE },
        { IPV6_V6ONLY                                                   },
        { IPV6_IFLIST,                0,               0,          TRUE },
        { IPV6_UNICAST_IF                                               },
        { IPV6_RTHDR,                 0,               0,          TRUE },
        { IPV6_GET_IFLIST,            WSAEINVAL,       0,          TRUE },
        { IPV6_RECVRTHDR,             0,               0,          TRUE },
        { IPV6_RECVTCLASS                                               },
        { IP_ORIGINAL_ARRIVAL_IF,     0,               0,          TRUE },
        { IPV6_ECN,                   0,               0,          TRUE },
        { IPV6_PKTINFO_EX,            0,               0,          TRUE },
        { IPV6_WFP_REDIRECT_RECORDS,  0,               0,          TRUE },
        { IPV6_WFP_REDIRECT_CONTEXT,  0,               0,          TRUE },
        { IPV6_MTU_DISCOVER,          0,               WSAEINVAL,  TRUE },
        { IPV6_MTU,                   WSAENOTCONN,     0,          TRUE },
        { IPV6_RECVERR,               0,               0,          TRUE },
        { IPV6_USER_MTU,              0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
    static const struct sockopt_validity_test ipv6_raw_tests[] =
    {
        { -1,                         WSAENOPROTOOPT                    },
        { IPV6_HOPOPTS,               0,               0,          TRUE },
        { IPV6_HDRINCL,               0,               0,          TRUE },
        { IPV6_UNICAST_HOPS                                             },
        { IPV6_MULTICAST_IF                                             },
        { IPV6_MULTICAST_HOPS                                           },
        { IPV6_MULTICAST_LOOP                                           },
        { IPV6_ADD_MEMBERSHIP,        WSAENOPROTOOPT                    },
        { IPV6_DROP_MEMBERSHIP,       WSAENOPROTOOPT                    },
        { IPV6_DONTFRAG                                                 },
        { IPV6_PKTINFO                                                  },
        { IPV6_HOPLIMIT                                                 },
        { IPV6_PROTECTION_LEVEL                                         },
        { IPV6_RECVIF,                0,               0,          TRUE },
        { IPV6_RECVDSTADDR,           0,               0,          TRUE },
        { IPV6_V6ONLY                                                   },
        { IPV6_IFLIST,                0,               0,          TRUE },
        { IPV6_UNICAST_IF                                               },
        { IPV6_RTHDR,                 0,               0,          TRUE },
        { IPV6_GET_IFLIST,            WSAEINVAL,       0,          TRUE },
        { IPV6_RECVRTHDR,             0,               0,          TRUE },
        { IPV6_RECVTCLASS                                               },
        { IP_ORIGINAL_ARRIVAL_IF,     0,               0,          TRUE },
        { IPV6_ECN,                   0,               0,          TRUE },
        { IPV6_PKTINFO_EX,            0,               0,          TRUE },
        { IPV6_WFP_REDIRECT_RECORDS,  0,               0,          TRUE },
        { IPV6_WFP_REDIRECT_CONTEXT,  0,               0,          TRUE },
        { IPV6_MTU_DISCOVER,          0,               WSAEINVAL,  TRUE },
        { IPV6_MTU,                   WSAENOTCONN,     0,          TRUE },
        { IPV6_RECVERR,               WSAEINVAL,       0,          TRUE },
        { IPV6_USER_MTU,              0,               0,          TRUE },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
    static const struct sockopt_validity_test file_handle_tests[] =
    {
        { -1,                         WSAENOTSOCK                       },
        { SO_TYPE,                    WSAENOTSOCK                       },
        { SO_OPENTYPE                                                   },
#if defined(__REACTOS__) && defined(_MSC_VER)
        { 0 }
#else
        {}
#endif
    };
    char path[MAX_PATH];
    HANDLE file;
    SOCKET sock;

#ifdef __REACTOS__
    /* WS03 needs some patches to the ipv4 tests */
    if (GetNTVersion() == _WIN32_WINNT_WS03 && !is_reactos()) {
        ipv4_tcp_tests[1].set_error = WSAEFAULT;
        ipv4_udp_tests[1].set_error = WSAEFAULT;
        ipv4_udp_tests[2].get_error = 0;
        ipv4_raw_tests[1].set_error = WSAEFAULT;
    }
#endif
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    do_sockopt_validity_tests("IPv4 TCP", sock, IPPROTO_IP, ipv4_tcp_tests);
    closesocket(sock);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    do_sockopt_validity_tests("IPv4 UDP", sock, IPPROTO_IP, ipv4_udp_tests);
    closesocket(sock);

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock == INVALID_SOCKET && WSAGetLastError() == WSAEACCES)
    {
        skip("Raw IPv4 sockets are not available\n");
    }
    else
    {
        ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
        do_sockopt_validity_tests("IPv4 raw", sock, IPPROTO_IP, ipv4_raw_tests);
        closesocket(sock);
    }

#ifdef __REACTOS__
    if (HasIPV6) {
#endif
    sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    do_sockopt_validity_tests("IPv6 TCP", sock, IPPROTO_IPV6, ipv6_tcp_tests);
    closesocket(sock);

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
    do_sockopt_validity_tests("IPv6 UDP", sock, IPPROTO_IPV6, ipv6_udp_tests);
    closesocket(sock);

    sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (sock == INVALID_SOCKET && WSAGetLastError() == WSAEACCES)
    {
        skip("Raw IPv6 sockets are not available\n");
    }
    else
    {
        ok(sock != INVALID_SOCKET, "failed to create socket, error %u\n", WSAGetLastError());
        do_sockopt_validity_tests("IPv6 raw", sock, IPPROTO_IPV6, ipv6_raw_tests);
        closesocket(sock);
    }
#ifdef __REACTOS__
    }
#endif

    GetSystemWindowsDirectoryA(path, ARRAY_SIZE(path));
    strcat(path, "\\system.ini");
    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0x0, NULL);
    do_sockopt_validity_tests("file", (SOCKET)file, SOL_SOCKET, file_handle_tests);
    CloseHandle(file);
}

static void test_tcp_reset(void)
{
    static const struct timeval select_timeout;
    fd_set readfds, writefds, exceptfds;
    OVERLAPPED overlapped = {0};
    SOCKET client, server;
    DWORD size, flags = 0;
    int ret, len, error;
    char buffer[10];
    WSABUF wsabuf;

#ifdef __REACTOS__
    if (is_reactos()) {
        ok(FALSE, "This test bugchecks on ReactOS!\n");
        return;
    }
#endif
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    tcp_socketpair(&client, &server);

    wsabuf.buf = buffer;
    wsabuf.len = sizeof(buffer);
    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == ERROR_IO_PENDING, "got error %u\n", WSAGetLastError());

    close_with_rst(server);

    ret = WaitForSingleObject(overlapped.hEvent, 1000);
    ok(!ret, "wait failed\n");
    ret = GetOverlappedResult((HANDLE)client, &overlapped, &size, FALSE);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NETNAME_DELETED, "got error %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
#ifdef __REACTOS__
    ok((NTSTATUS)overlapped.Internal == STATUS_CONNECTION_RESET || broken((NTSTATUS)overlapped.Internal == STATUS_REMOTE_DISCONNECT) /* WS03 */, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
#else
    ok((NTSTATUS)overlapped.Internal == STATUS_CONNECTION_RESET, "got status %#lx\n", (NTSTATUS)overlapped.Internal);
#endif

    len = sizeof(error);
    ret = getsockopt(client, SOL_SOCKET, SO_ERROR, (char *)&error, &len);
    ok(!ret, "got error %u\n", WSAGetLastError());
    ok(!error, "got error %u\n", error);

    wsabuf.buf = buffer;
    wsabuf.len = sizeof(buffer);
    WSASetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = WSARecv(client, &wsabuf, 1, &size, &flags, &overlapped, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAECONNRESET, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = send(client, "data", 5, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAECONNRESET, "got error %u\n", WSAGetLastError());

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03 && !is_reactos())
        skip("This test crashes on Windows Server 2003.\n");
    else
#endif
    check_poll(client, POLLERR | POLLHUP | POLLWRNORM);

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(client, &readfds);
    FD_SET(client, &writefds);
    FD_SET(client, &exceptfds);
    ret = select(0, &readfds, &writefds, &exceptfds, &select_timeout);
    ok(ret == 2, "got %d\n", ret);
    ok(FD_ISSET(client, &readfds), "FD should be set\n");
    ok(FD_ISSET(client, &writefds), "FD should be set\n");
    ok(!FD_ISSET(client, &exceptfds), "FD should be set\n");

    FD_ZERO(&exceptfds);
    FD_SET(client, &exceptfds);
    ret = select(0, NULL, NULL, &exceptfds, &select_timeout);
    ok(!ret, "got %d\n", ret);
    ok(!FD_ISSET(client, &exceptfds), "FD should be set\n");

    closesocket(server);
    CloseHandle(overlapped.hEvent);
}

struct icmp_hdr
{
    BYTE type;
    BYTE code;
    UINT16 checksum;
    union
    {
        struct
        {
            UINT16 id;
            UINT16 sequence;
        } echo;
    } un;
};

struct ip_hdr
{
    BYTE v_hl; /* version << 4 | hdr_len */
    BYTE tos;
    UINT16 tot_len;
    UINT16 id;
    UINT16 frag_off;
    BYTE ttl;
    BYTE protocol;
    UINT16 checksum;
    ULONG saddr;
    ULONG daddr;
};

/* rfc 1071 checksum */
static unsigned short chksum(BYTE *data, unsigned int count)
{
    unsigned int sum = 0, carry = 0;
    unsigned short check, s;

    while (count > 1)
    {
        s = *(unsigned short *)data;
        data += 2;
        sum += carry;
        sum += s;
        carry = s > sum;
        count -= 2;
    }
    sum += carry; /* This won't produce another carry */
    sum = (sum & 0xffff) + (sum >> 16);

    if (count) sum += *data; /* LE-only */

    sum = (sum & 0xffff) + (sum >> 16);
    /* fold in any carry */
    sum = (sum & 0xffff) + (sum >> 16);

    check = ~sum;
    return check;
}

static void test_icmp(void)
{
    static const unsigned int ping_data = 0xdeadbeef;
    struct icmp_hdr *icmp_h;
    BYTE send_buf[sizeof(struct icmp_hdr) + sizeof(ping_data)];
    UINT16 recv_checksum, checksum;
    unsigned int reply_data;
    struct sockaddr_in sa;
    struct ip_hdr *ip_h;
    struct in_addr addr;
    BYTE recv_buf[256];
    SOCKET s;
    int ret;

    s = WSASocketA(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0);
    if (s == INVALID_SOCKET)
    {
        ret = WSAGetLastError();
        ok(ret == WSAEACCES, "Expected 10013, received %d\n", ret);
        skip("SOCK_RAW is not supported\n");
        return;
    }

    icmp_h = (struct icmp_hdr *)send_buf;
    icmp_h->type = ICMP4_ECHO_REQUEST;
    icmp_h->code = 0;
    icmp_h->checksum = 0;
    icmp_h->un.echo.id = 0xbeaf; /* will be overwritten for linux ping socks */
    icmp_h->un.echo.sequence = 1;
    *(unsigned int *)(icmp_h + 1) = ping_data;
    icmp_h->checksum = chksum(send_buf, sizeof(send_buf));

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = sendto(s, (char *)send_buf, sizeof(send_buf), 0, (struct sockaddr*)&sa, sizeof(sa));
    ok(ret == sizeof(send_buf), "got %d, error %d.\n", ret, WSAGetLastError());

    ret = recv(s, (char *)recv_buf, sizeof(struct ip_hdr) + sizeof(send_buf) - 1, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEMSGSIZE, "got %d\n", WSAGetLastError());

    icmp_h->un.echo.sequence = 2;
    icmp_h->checksum = 0;
    icmp_h->checksum = chksum(send_buf, sizeof(send_buf));

    ret = sendto(s, (char *)send_buf, sizeof(send_buf), 0, (struct sockaddr*)&sa, sizeof(sa));
    ok(ret != SOCKET_ERROR, "got error %d.\n", WSAGetLastError());

    memset(recv_buf, 0xcc, sizeof(recv_buf));
    ret = recv(s, (char *)recv_buf, sizeof(recv_buf), 0);
    ok(ret == sizeof(struct ip_hdr) + sizeof(send_buf), "got %d\n", ret);

    ip_h = (struct ip_hdr *)recv_buf;
    icmp_h = (struct icmp_hdr *)(ip_h + 1);
    reply_data = *(unsigned int *)(icmp_h + 1);

    ok(ip_h->v_hl == ((4 << 4) | (sizeof(*ip_h) >> 2)), "got v_hl %#x.\n", ip_h->v_hl);
    ok(ntohs(ip_h->tot_len) == sizeof(struct ip_hdr) + sizeof(send_buf),
            "got tot_len %#x.\n", ntohs(ip_h->tot_len));

    recv_checksum = ip_h->checksum;
    ip_h->checksum = 0;
    checksum = chksum((BYTE *)ip_h, sizeof(*ip_h));
    /* Checksum is 0 for localhost ping on Windows but not for remote host ping. */
    ok(recv_checksum == checksum || !recv_checksum, "got checksum %#x, expected %#x.\n", recv_checksum, checksum);

    ok(!ip_h->frag_off, "got id %#x.\n", ip_h->frag_off);
    addr.s_addr = ip_h->saddr;
    ok(ip_h->saddr == sa.sin_addr.s_addr, "got saddr %s.\n", inet_ntoa(addr));
    addr.s_addr = ip_h->daddr;
    ok(!!ip_h->daddr, "got daddr %s.\n", inet_ntoa(addr));

    ok(ip_h->protocol == 1, "got protocol %#x.\n", ip_h->protocol);

    ok(icmp_h->type == ICMP4_ECHO_REPLY, "got type %#x.\n", icmp_h->type);
    ok(!icmp_h->code, "got code %#x.\n", icmp_h->code);
    ok(icmp_h->un.echo.id == 0xbeaf, "got echo id %#x.\n", icmp_h->un.echo.id);
    ok(icmp_h->un.echo.sequence == 2, "got echo sequence %#x.\n", icmp_h->un.echo.sequence);

    recv_checksum = icmp_h->checksum;
    icmp_h->checksum = 0;
    checksum = chksum((BYTE *)icmp_h, sizeof(send_buf));
    ok(recv_checksum == checksum, "got checksum %#x, expected %#x.\n", recv_checksum, checksum);

    ok(reply_data == ping_data, "got reply_data %#x.\n", reply_data);

    closesocket(s);
}

static void test_connect_time(void)
{
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    SOCKET client, server;
    unsigned int time;
    int ret, len;

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    len = sizeof(time);
    SetLastError(0xdeadbeef);
    ret = getsockopt(client, SOL_SOCKET, SO_CONNECT_TIME, (char *)&time, &len);
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(len == sizeof(time), "got len %d\n", len);
    ok(time == ~0u, "got time %u\n", time);

    closesocket(client);

    tcp_socketpair(&client, &server);

    len = sizeof(time);
    SetLastError(0xdeadbeef);
    ret = getsockopt(client, SOL_SOCKET, SO_CONNECT_TIME, (char *)&time, &len);
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(len == sizeof(time), "got len %d\n", len);
    ok(time == 0, "got time %u\n", time);

    len = sizeof(time);
    SetLastError(0xdeadbeef);
    ret = getsockopt(server, SOL_SOCKET, SO_CONNECT_TIME, (char *)&time, &len);
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(len == sizeof(time), "got len %d\n", len);
    ok(time == 0, "got time %u\n", time);

    closesocket(client);
    closesocket(server);

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());
    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %lu\n", GetLastError());
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());

    len = sizeof(time);
    SetLastError(0xdeadbeef);
    ret = getsockopt(client, SOL_SOCKET, SO_CONNECT_TIME, (char *)&time, &len);
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(len == sizeof(time), "got len %d\n", len);
    ok(time == ~0u, "got time %u\n", time);

    len = sizeof(time);
    SetLastError(0xdeadbeef);
    ret = getsockopt(server, SOL_SOCKET, SO_CONNECT_TIME, (char *)&time, &len);
    ok(!ret, "got %d\n", ret);
    ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(len == sizeof(time), "got len %d\n", len);
    ok(time == ~0u, "got time %u\n", time);

    closesocket(server);
    closesocket(client);
}

static void test_connect_udp(void)
{
    const struct sockaddr_in bind_addr = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    struct sockaddr_in addr, ret_addr;
    SOCKET client, server;
    char buffer[5];
    int ret, len;

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_blocking(client, FALSE);
    set_blocking(server, FALSE);

    SetLastError(0xdeadbeef);
    ret = send(client, "data", 4, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAENOTCONN, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    todo_wine ok(GetLastError() == WSAEINVAL, "got error %lu\n", GetLastError());

    ret = bind(server, (const struct sockaddr *)&bind_addr, sizeof(bind_addr));
    ok(!ret, "got error %lu\n", GetLastError());
    len = sizeof(addr);
    ret = getsockname(server, (struct sockaddr *)&addr, &len);
    ok(!ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAEWOULDBLOCK, "got error %lu\n", GetLastError());

    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());
    ret = getpeername(client, (struct sockaddr *)&ret_addr, &len);
    ok(!ret, "got error %lu\n", GetLastError());
    ok(!memcmp(&ret_addr, &addr, sizeof(addr)), "addresses didn't match\n");

    ret = getsockname(client, (struct sockaddr *)&ret_addr, &len);
    ok(!ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = getpeername(server, (struct sockaddr *)&ret_addr, &len);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAENOTCONN, "got error %lu\n", GetLastError());

    ret = send(client, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    memset(buffer, 0xcc, sizeof(buffer));
    ret = recv(server, buffer, sizeof(buffer), 0);
#ifdef __REACTOS__
    ok(ret == 4 || broken(ret == SOCKET_ERROR) /* sometimes WS03 */, "got %d\n", ret);
    ok(!memcmp(buffer, "data", 4) || broken(!memcmp(buffer, "\xCC\xCC\xCC\xCC", 4)) /* sometimes WS03 */, "got %s\n", debugstr_an(buffer, ret));
#else
    ok(ret == 4, "got %d\n", ret);
    ok(!memcmp(buffer, "data", 4), "got %s\n", debugstr_an(buffer, ret));
#endif

    SetLastError(0xdeadbeef);
    ret = recv(server, buffer, sizeof(buffer), 0);
#ifdef __REACTOS__
    ok(ret == -1 || broken(ret == 4) /* sometimes WS03 */, "got %d\n", ret);
    ok(GetLastError() == WSAEWOULDBLOCK || broken(!GetLastError()) /* sometimes WS03 */, "got error %lu\n", GetLastError());
#else
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAEWOULDBLOCK, "got error %lu\n", GetLastError());
#endif

    SetLastError(0xdeadbeef);
    ret = send(server, "data", 4, 0);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAENOTCONN, "got error %lu\n", GetLastError());

    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());
    ++addr.sin_port;
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_UNSPEC;
    ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "got error %lu\n", GetLastError());

    ret = getpeername(client, (struct sockaddr *)&ret_addr, &len);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == WSAENOTCONN, "got error %lu\n", GetLastError());

    closesocket(server);
    closesocket(client);
}

static void test_tcp_sendto_recvfrom(void)
{
    SOCKET client, server = 0;
    SOCKADDR_IN addr = { AF_INET, SERVERPORT };
    SOCKADDR_IN bad_addr, bad_addr_copy;
    const char serverMsg[] = "ws2_32/TCP socket test";
    char clientBuf[sizeof(serverMsg)] = { 0 };
    int to_len = 0xc0ffee11;
    int ret;

    inet_pton(AF_INET, SERVERIP, &addr.sin_addr);

    tcp_socketpair(&client, &server);

    memset(&bad_addr, 0xfe, sizeof(bad_addr));
    memcpy(&bad_addr_copy, &bad_addr, sizeof(bad_addr_copy));

    ret = sendto(server, serverMsg, sizeof(serverMsg), 0, (SOCKADDR *)&bad_addr, sizeof(bad_addr));
    ok(ret == sizeof(serverMsg), "Incorrect return value from sendto: %d (%d)\n", ret, WSAGetLastError());
    ok(!memcmp(&bad_addr, &bad_addr_copy, sizeof(bad_addr)), "Provided address modified by sendto\n");
    ok(to_len == 0xc0ffee11, "Provided size modified by sendto\n");

    ret = recvfrom(client, clientBuf, sizeof(clientBuf), 0, (SOCKADDR *)&bad_addr, &to_len);
    ok(ret == sizeof(serverMsg), "Incorrect return value from recvfrom: %d (%d)\n", ret, WSAGetLastError());
    ok(!memcmp(&bad_addr, &bad_addr_copy, sizeof(bad_addr)), "Provided address modified by recvfrom\n");
    ok(to_len == 0xc0ffee11, "Provided size modified by recvfrom\n");

    ok(!memcmp(serverMsg, clientBuf, sizeof(serverMsg)), "Data mismatch over TCP socket\n");

    closesocket(client);
    closesocket(server);
}

/* Regression test for an internal bug affecting wget.exe. */
static void test_select_after_WSAEventSelect(void)
{
    SOCKET client, server;
    HANDLE event;
    int ret;

#ifdef __REACTOS__
    if (GetNTVersion() <= _WIN32_WINNT_WS03) {
        skip("This test crashes on Windows Server 2003 and ReactOS.\n");
        return;
    }
#endif
    tcp_socketpair(&client, &server);
    event = CreateEventA(NULL, FALSE, FALSE, NULL);

    ret = WSAEventSelect(client, event, FD_READ);
    ok(!ret, "got error %u\n", WSAGetLastError());

    ret = send(server, "data", 4, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = WaitForSingleObject(event, 1000);
    ok(!ret, "got %d\n", ret);

    /* Poll. This must not trigger any events to be signalled again. */
    check_poll(client, POLLRDNORM | POLLWRNORM);

    ret = WaitForSingleObject(event, 0);
    ok(ret == WAIT_TIMEOUT, "got %d\n", ret);

    CloseHandle(event);
    closesocket(server);
    closesocket(client);
}

static void test_broadcast(void)
{
    struct sockaddr_in bcast = {.sin_family = AF_INET, .sin_port = htons(12345), .sin_addr.s_addr = htonl(INADDR_BROADCAST)};
    struct sockaddr_in6 mcast6;
    int val, ret, len;
    SOCKET s;

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(s != INVALID_SOCKET, "got error %u.\n", WSAGetLastError());
    ret = sendto(s, "test", 4, 0, (struct sockaddr *)&bcast, sizeof(bcast));
    ok(ret == -1, "got %d, error %u.\n", ret, WSAGetLastError());
    val = 1;
    ret = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(val));
    ok(!ret, "got %d, error %u.\n", ret, WSAGetLastError());
    ret = sendto(s, "test", 4, 0, (struct sockaddr *)&bcast, sizeof(bcast));
    ok(ret == 4, "got %d, error %u.\n", ret, WSAGetLastError());

    val = 0;
    ret = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(val));
    ok(!ret, "got %d, error %u.\n", ret, WSAGetLastError());
    ret = sendto(s, "test", 4, 0, (struct sockaddr *)&bcast, sizeof(bcast));
    ok(ret == -1, "got %d, error %u.\n", ret, WSAGetLastError());

    ret = connect(s, (struct sockaddr *)&bcast, sizeof(bcast));
    ok(!ret, "got error %u.\n", WSAGetLastError());
    val = 1;
    len = sizeof(val);
    ret = getsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&val, &len);
    ok(!ret, "got %d, error %u.\n", ret, WSAGetLastError());
    ok(!val, "got %d.\n", val);
    ret = sendto(s, "test", 4, 0, (struct sockaddr *)&bcast, sizeof(bcast));
    ok(ret == -1, "got %d, error %u.\n", ret, WSAGetLastError());
    ret = send(s, "test", 4, 0);
    ok(ret == 4, "got %d, error %u.\n", ret, WSAGetLastError());
    closesocket(s);

#ifdef __REACTOS__
    if (HasIPV6) {
#endif
    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    ok(s != INVALID_SOCKET, "got error %u.\n", WSAGetLastError());
    memset(&mcast6, 0, sizeof(mcast6));
    ret = inet_pton(AF_INET6, "ff01::1", &mcast6.sin6_addr);
    ok(ret, "got error %u.\n", WSAGetLastError());
    mcast6.sin6_family = AF_INET6;
    mcast6.sin6_port = htons(12345);
    ret = sendto(s, "test", 4, 0, (struct sockaddr *)&mcast6, sizeof(mcast6));
    ok(ret == 4, "got %d, error %u.\n", ret, WSAGetLastError());
    closesocket(s);
#ifdef __REACTOS__
    }
#endif
}

struct test_send_buffering_data
{
    int buffer_size;
    int sent_size;
    SOCKET server;
    char *buffer;
};

static DWORD WINAPI test_send_buffering_thread(void *arg)
{
    struct test_send_buffering_data *d = arg;
    int ret;
#ifdef __REACTOS__
    int errCnt = 0;
#endif

    d->sent_size = 0;
    while ((ret = send(d->server, d->buffer, d->buffer_size, 0)) > 0)
    {
        ok(ret == d->buffer_size, "got %d.\n", ret);
        d->sent_size += ret;
#ifdef __REACTOS__
        if (ret != d->buffer_size)
            errCnt++;
        if (errCnt >= 25)
            break;
#endif
    }
#ifdef __REACTOS__
    ok(errCnt < 25, "error count too high in loop, would've infinitely looped.\n");
#endif
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u.\n", WSAGetLastError());
    ok(d->sent_size, "got 0.\n");
#if defined(__REACTOS__) && DLL_EXPORT_VERSION < 0x600
    ret = CancelIo((HANDLE)d->server);
#else
    ret = CancelIoEx((HANDLE)d->server, NULL);
#endif
#ifdef __REACTOS__
    ok((!ret && GetLastError() == ERROR_NOT_FOUND) || broken(ret == TRUE && GetLastError() == WSAEWOULDBLOCK) /* WS03 */, "got ret %d, error %lu.\n", ret, GetLastError());
#else
    ok(!ret && GetLastError() == ERROR_NOT_FOUND, "got ret %d, error %lu.\n", ret, GetLastError());
#endif
    ret = CancelIo((HANDLE)d->server);
    ok(ret, "got error %lu.\n", GetLastError());
    shutdown(d->server, SD_BOTH);
    closesocket(d->server);
    return 0;
}

static void test_send_buffering(void)
{
    static const char test_data[] = "abcdefg01234567";

    struct test_send_buffering_data d;
    int ret, recv_size, i;
    SOCKET client;
    HANDLE thread;
#ifdef __REACTOS__
    int errCnt = 0;
#endif

    d.buffer_size = 1024 * 1024 * 50;
    d.buffer = malloc(d.buffer_size);

    for (i = 0; i < d.buffer_size; ++i)
        d.buffer[i] = test_data[i % sizeof(test_data)];

    tcp_socketpair(&client, &d.server);
    set_blocking(client, FALSE);
    set_blocking(d.server, FALSE);

    d.sent_size = 0;
    while ((ret = send(d.server, d.buffer, d.buffer_size, 0)) > 0)
    {
        ok(ret == d.buffer_size, "got %d.\n", ret);
        d.sent_size += ret;
#ifdef __REACTOS__
        if (ret != d.buffer_size)
            errCnt++;
        if (errCnt >= 25)
            break;
#endif
    }
#ifdef __REACTOS__
    ok(errCnt < 25, "error count too high in loop, would've infinitely looped.\n");
#endif
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "got error %u.\n", WSAGetLastError());
    ok(d.sent_size, "got 0.\n");
    closesocket(d.server);

    recv_size = 0;
    while ((ret = recv(client, d.buffer, d.buffer_size, 0)) > 0 || WSAGetLastError() == WSAEWOULDBLOCK)
    {
        if (ret < 0)
            continue;
        for (i = 0; i < ret; ++i)
        {
            if (d.buffer[i] != test_data[(recv_size + i) % sizeof(test_data)])
                break;
        }
        ok(i == ret, "data mismatch.\n");
        recv_size += ret;
        ok(recv_size <= d.sent_size, "got ret %d, recv_size %d, sent_size %d.\n", ret, recv_size, d.sent_size);
    }
    ok(!ret && !WSAGetLastError(), "got ret %d, error %u.\n", ret, WSAGetLastError());
    ok(recv_size == d.sent_size, "got %d, expected %d.\n", recv_size, d.sent_size);
    closesocket(client);

    /* Test with the other thread which terminates before the data is actually sent. */
    for (i = 0; i < d.buffer_size; ++i)
        d.buffer[i] = test_data[i % sizeof(test_data)];

    tcp_socketpair(&client, &d.server);
    set_blocking(client, FALSE);
    set_blocking(d.server, FALSE);

    thread = CreateThread(NULL, 0, test_send_buffering_thread, &d, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    recv_size = 0;
    while ((ret = recv(client, d.buffer, d.buffer_size, 0)) > 0 || WSAGetLastError() == WSAEWOULDBLOCK)
    {
        if (ret < 0)
            continue;
        for (i = 0; i < ret; ++i)
        {
            if (d.buffer[i] != test_data[(recv_size + i) % sizeof(test_data)])
                break;
        }
        ok(i == ret, "data mismatch.\n");
        recv_size += ret;
        ok(recv_size <= d.sent_size, "got ret %d, recv_size %d, sent_size %d.\n", ret, recv_size, d.sent_size);
    }
    ok(!ret && !WSAGetLastError(), "got ret %d, error %u.\n", ret, WSAGetLastError());
    ok(recv_size == d.sent_size, "got %d, expected %d.\n", recv_size, d.sent_size);
    closesocket(client);
}

START_TEST( sock )
{
    int i;

/* Leave these tests at the beginning. They depend on WSAStartup not having been
 * called, which is done by Init() below. */
    test_WithoutWSAStartup();
    test_WithWSAStartup();

    Init();

    test_set_getsockopt();
    test_reuseaddr();
    test_ip_pktinfo();
    test_ipv4_cmsg();
    test_ipv6_cmsg();
    test_extendedSocketOptions();
    test_so_debug();
    test_sockopt_validity();
    test_connect_time();

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        do_test(&tests[i]);

    test_UDP();

    test_WSASocket();
    test_WSADuplicateSocket();
    test_WSAConnectByName();
    test_WSAEnumNetworkEvents();

    test_errors();
    test_listen();
    test_select();
    test_accept();
    test_accept_inheritance();
    test_getpeername();
    test_getsockname();

    test_address_list_query();
    test_fionbio();
    test_fionread_siocatmark();
    test_get_extension_func();
    test_backlog_query();
    test_get_interface_list();
    test_keepalive_vals();
    test_sioRoutingInterfaceQuery();
    test_sioAddressListChange();
    test_base_handle();
    test_circular_queueing();
    test_unsupported_ioctls();

    test_WSASendMsg();
    test_WSASendTo();
    test_WSARecv();
    test_WSAPoll();
    test_write_watch();

    test_events();
    test_select_after_WSAEventSelect();

    test_ipv6only();
    test_TransmitFile();
    test_AcceptEx();
    test_connect();
    test_shutdown();
    test_DisconnectEx();

    test_completion_port();
    test_connect_completion_port();
    test_shutdown_completion_port();
    test_bind();
    test_connecting_socket();
    test_WSAGetOverlappedResult();
    test_nonblocking_async_recv();
    test_simultaneous_async_recv();
    test_empty_recv();
    test_timeout();
    test_tcp_reset();
    test_icmp();
    test_connect_udp();
    test_tcp_sendto_recvfrom();
    test_broadcast();
    test_send_buffering();

    /* There is apparently an obscure interaction between this test and
     * test_WSAGetOverlappedResult().
     *
     * One thing this test does is to close socket handles through CloseHandle()
     * and NtClose(), to prove that that is sufficient to cancel I/O on the
     * socket. This has the obscure side effect that ws2_32.dll's internal
     * per-process list of sockets never has that socket removed.
     *
     * test_WSAGetOverlappedResult() effectively proves that the per-process
     * list of sockets exists, by calling DuplicateHandle() on a socket and then
     * passing it to a function which cares about socket handle validity, which
     * checks that handle against the internal list, finds it invalid, and
     * returns WSAENOTSOCK.
     *
     * The problem is that if we close an NT handle without removing it from the
     * ws2_32 list, then duplicate another handle, it *may* end up allocated to
     * the same handle value, and thus re-validate that handle right under the
     * nose of ws2_32. This causes the test_WSAGetOverlappedResult() test to
     * sometimes succeed where it's expected to fail with ENOTSOCK.
     *
     * In order to avoid this, make sure that this test—which is evidently
     * destructive to ws2_32 internal state in obscure ways—is executed last.
     */
    test_iocp();

    /* this is an io heavy test, do it at the end so the kernel doesn't start dropping packets */
    test_send();

    Exit();
}
