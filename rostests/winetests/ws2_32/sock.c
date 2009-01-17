/*
 * Unit test suite for winsock functions
 *
 * Copyright 2002 Martin Wilck
 * Copyright 2005 Thomas Kho
 * Copyright 2008 Jeff Zaroyko
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

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "wine/test.h"
#include <winnt.h>
#include <winerror.h>

#define MAX_CLIENTS 4      /* Max number of clients */
#define NUM_TESTS   4      /* Number of tests performed */
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

/**************** General utility functions ***************/

static int tcp_socketpair(SOCKET *src, SOCKET *dst)
{
    SOCKET server = INVALID_SOCKET;
    struct sockaddr_in addr;
    int len;
    int ret;

    *src = INVALID_SOCKET;
    *dst = INVALID_SOCKET;

    *src = socket(AF_INET, SOCK_STREAM, 0);
    if (*src == INVALID_SOCKET)
        goto end;

    server = socket(AF_INET, SOCK_STREAM, 0);
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

static int do_synchronous_send ( SOCKET s, char *buf, int buflen, int sendlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; p += n )
        n = send ( s, p, min ( sendlen, last - p ), 0 );
    wsa_ok ( n, 0 <=, "do_synchronous_send (%x): error %d\n" );
    return p - buf;
}

static int do_synchronous_recv ( SOCKET s, char *buf, int buflen, int recvlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; p += n )
        n = recv ( s, p, min ( recvlen, last - p ), 0 );
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%x): error %d:\n" );
    return p - buf;
}

static int do_synchronous_recvfrom ( SOCKET s, char *buf, int buflen,int flags,struct sockaddr *from, socklen_t *fromlen, int recvlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; p += n )
      n = recvfrom ( s, p, min ( recvlen, last - p ), 0, from, fromlen );
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
        n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, par->buflen );
        ok ( n_recvd == n_expected,
             "simple_server (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
        pos = test_buffer ( mem->sock[0].buf, gen->chunk_size, gen->n_chunks );
        ok ( pos == -1, "simple_server (%x): test pattern error: %d\n", id, pos);

        /* Echo data back */
        n_sent = do_synchronous_send ( mem->sock[0].s, mem->sock[0].buf, n_expected, par->buflen );
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
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%x): shutdown failed: %d\n" );

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recv ( mem->s, mem->recv_buf, n_expected, par->buflen );
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
 * simple_mixed_client: mixing send and recvfrom
 */
static VOID WINAPI simple_mixed_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int pos, n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, id;
    socklen_t fromLen = sizeof(mem->addr);
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
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, par->buflen );
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
    long mask = FD_READ | FD_WRITE | FD_CLOSE;

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
        wsa_ok ( err, 0 ==, "event_client (%x): WSAEnumNetworkEvents error: %d\n" );
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
        wsa_ok ( err, 0 ==, "event_client (%x): WSAEnumNetworkEvents error: %d\n" );

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

/**************** Main program utility functions ***************/

static void Init (void)
{
    WORD ver = MAKEWORD (2, 2);
    WSADATA data;

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
    ret = WSACleanup();
    err = WSAGetLastError();
    ok ( ret == SOCKET_ERROR && err ==  WSANOTINITIALISED,
            "WSACleanup returned %d GetLastError is %d\n", ret, err);
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
            trace ("terminating thread %08x\n", thread_id[i]);
            if ( WaitForSingleObject ( thread[i], 0 ) != WAIT_OBJECT_0 )
                TerminateThread ( thread [i], 0 );
        }
    }
    CloseHandle ( server_ready );
    for (i = 0; i <= n; i++)
        CloseHandle ( client_ready[i] );
}

/********* some tests for getsockopt(setsockopt(X)) == X ***********/
/* optname = SO_LINGER */
LINGER linger_testvals[] = {
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
    SOCKET s;
    int i, err, lasterr;
    int timeout;
    LINGER lingval;
    int size;

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
    /* SO_SNDTIMEO */
    timeout = SOCKTIMEOUT2; /* 54 seconds. See remark above */ 
    size = sizeof(timeout);
    err = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, size); 
    if( !err)
        err = getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, &size); 
    ok( !err, "get/setsockopt(SO_SNDTIMEO) failed error: %d\n", WSAGetLastError());
    ok( timeout == SOCKTIMEOUT2, "getsockopt(SO_SNDTIMEO) returned wrong value %d\n", timeout);
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
    /* Test for erroneously passing a value instead of a pointer as optval */
    size = sizeof(char);
    err = setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)1, size);
    ok(err == SOCKET_ERROR, "setsockopt with optval being a value passed "
                            "instead of failing.\n");
    lasterr = WSAGetLastError();
    ok(lasterr == WSAEFAULT, "setsockopt with optval being a value "
                             "returned 0x%08x, not WSAEFAULT(0x%08x)\n",
                             lasterr, WSAEFAULT);
    closesocket(s);
}

static void test_so_reuseaddr(void)
{
    struct sockaddr_in saddr;
    SOCKET s1,s2;
    unsigned int rc,reuse;
    int size;

    saddr.sin_family      = AF_INET;
    saddr.sin_port        = htons(9375);
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
    closesocket(s1);
    rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc==0, "bind() failed error: %d\n", WSAGetLastError());

    closesocket(s2);
}

/************* Array containing the tests to run **********/

#define STD_STREAM_SOCKET \
            SOCK_STREAM, \
            0, \
            SERVERIP, \
            SERVERPORT

static test_setup tests [NUM_TESTS] =
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
        /* Test 3: synchronous mixed client and server */
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

static void WINAPI do_getservbyname( HANDLE *starttest )
{
    struct {
        const char *name;
        const char *proto;
        int port;
    } serv[2] = { {"domain", "udp", 53}, {"telnet", "tcp", 23} };

    int i, j;
    struct servent *pserv[2];

    ok ( WaitForSingleObject ( *starttest, TEST_TIMEOUT * 1000 ) != WAIT_TIMEOUT, "test_getservbyname: timeout waiting for start signal\n");

    /* ensure that necessary buffer resizes are completed */
    for ( j = 0; j < 2; j++) {
        pserv[j] = getservbyname ( serv[j].name, serv[j].proto );
    }

    for ( i = 0; i < NUM_QUERIES / 2; i++ ) {
        for ( j = 0; j < 2; j++ ) {
            pserv[j] = getservbyname ( serv[j].name, serv[j].proto );
            ok ( pserv[j] != NULL, "getservbyname could not retrieve information for %s: %d\n", serv[j].name, WSAGetLastError() );
            ok ( pserv[j]->s_port == htons(serv[j].port), "getservbyname returned the wrong port for %s: %d\n", serv[j].name, ntohs(pserv[j]->s_port) );
            ok ( !strcmp ( pserv[j]->s_proto, serv[j].proto ), "getservbyname returned the wrong protocol for %s: %s\n", serv[j].name, pserv[j]->s_proto );
            ok ( !strcmp ( pserv[j]->s_name, serv[j].name ), "getservbyname returned the wrong name for %s: %s\n", serv[j].name, pserv[j]->s_name );
        }

        ok ( pserv[0] == pserv[1], "getservbyname: winsock resized servent buffer when not necessary\n" );
    }
}

static void test_getservbyname(void)
{
    int i;
    HANDLE starttest, thread[NUM_THREADS];
    DWORD thread_id[NUM_THREADS];

    starttest = CreateEvent ( NULL, 1, 0, "test_getservbyname_starttest" );

    /* create threads */
    for ( i = 0; i < NUM_THREADS; i++ ) {
        thread[i] = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) &do_getservbyname, &starttest, 0, &thread_id[i] );
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
    int providers[] = {6, 0};
    int ret, err;
    UINT pi_size;

    /* Set pi_size explicitly to a value below 2*sizeof(WSAPROTOCOL_INFOA)
     * to avoid a crash on win98.
     */
    pi_size = 0;
    ret = WSAEnumProtocolsA(providers, NULL, &pi_size);
    ok(ret == SOCKET_ERROR, "WSAEnumProtocolsA({6,0}, NULL, 0) returned %d\n",
            ret);
    err = WSAGetLastError();
    ok(err == WSAENOBUFS, "WSAEnumProtocolsA error is %d, not WSAENOBUFS(%d)\n",
            err, WSAENOBUFS);

    pi = HeapAlloc(GetProcessHeap(), 0, pi_size);
    ok(pi != NULL, "Failed to allocate memory\n");
    if (pi == NULL) {
        skip("Can't continue without memory.\n");
        return;
    }

    ret = WSAEnumProtocolsA(providers, pi, &pi_size);
    ok(ret != SOCKET_ERROR, "WSAEnumProtocolsA failed, last error is %d\n",
            WSAGetLastError());

    if (ret == 0) {
        skip("No protocols enumerated.\n");
        HeapFree(GetProcessHeap(), 0, pi);
        return;
    }

    sock = WSASocketA(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                      FROM_PROTOCOL_INFO, &pi[0], 0, 0);
    ok(sock != INVALID_SOCKET, "Failed to create socket: %d\n",
            WSAGetLastError());

    closesocket(sock);
    HeapFree(GetProcessHeap(), 0, pi);
}

static void test_WSAAddressToStringA(void)
{
    INT ret;
    DWORD len;
    int GLE;
    SOCKADDR_IN sockaddr;
    CHAR address[22]; /* 12 digits + 3 dots + ':' + 5 digits + '\0' */

    CHAR expect1[] = "0.0.0.0";
    CHAR expect2[] = "255.255.255.255";
    CHAR expect3[] = "0.0.0.0:65535";
    CHAR expect4[] = "255.255.255.255:65535";

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
}

static void test_WSAAddressToStringW(void)
{
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

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address6, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 || (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address7, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 || (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressA( address8, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( (ret == 0 && sockaddr6.sin6_port == 0xffff) ||
        (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressA() failed for IPv6 address: %d\n", GLE);

}

static void test_WSAStringToAddressW(void)
{
    INT ret, len;
    SOCKADDR_IN sockaddr;
    SOCKADDR_IN6 sockaddr6;
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

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address6, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 || (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address7, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( ret == 0 || (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, len);
    sockaddr6.sin6_family = AF_INET6;

    ret = WSAStringToAddressW( address8, AF_INET6, NULL, (SOCKADDR*)&sockaddr6,
            &len );
    GLE = WSAGetLastError();
    ok( (ret == 0 && sockaddr6.sin6_port == 0xffff) ||
        (ret == SOCKET_ERROR && GLE == WSAEINVAL),
        "WSAStringToAddressW() failed for IPv6 address: %d\n", GLE);

}

static VOID WINAPI SelectReadThread(select_thread_params *par)
{
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
    wsa_ok(listen(par->s, SOMAXCONN ), 0 ==, "SelectReadThread (%x): listen failed: %d\n");

    SetEvent(server_ready);
    ret = select(par->s+1, &readfds, NULL, NULL, &select_timeout);
    par->ReadKilled = (ret == 1);
}

static void test_select(void)
{
    SOCKET fdRead, fdWrite;
    fd_set readfds, writefds, exceptfds;
    unsigned int maxfd;
    int ret;
    struct timeval select_timeout;
    select_thread_params thread_params;
    HANDLE thread_handle;
    DWORD id;

    fdRead = socket(AF_INET, SOCK_STREAM, 0);
    ok( (fdRead != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );
    fdWrite = socket(AF_INET, SOCK_STREAM, 0);
    ok( (fdWrite != INVALID_SOCKET), "socket failed unexpectedly: %d\n", WSAGetLastError() );
 
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(fdRead, &readfds);
    FD_SET(fdWrite, &writefds);
    FD_SET(fdRead, &exceptfds);
    FD_SET(fdWrite, &exceptfds);
    select_timeout.tv_sec=0;
    select_timeout.tv_usec=500;

    maxfd = fdRead;
    if (fdWrite > maxfd)
        maxfd = fdWrite;
       
    todo_wine {
    ret = select(maxfd+1, &readfds, &writefds, &exceptfds, &select_timeout);
    ok ( (ret == 0), "select should not return any socket handles\n");
    ok ( !FD_ISSET(fdRead, &readfds), "FD should not be set\n");
    ok ( !FD_ISSET(fdWrite, &writefds), "FD should not be set\n");
    }

    ok ( !FD_ISSET(fdRead, &exceptfds), "FD should not be set\n");
    ok ( !FD_ISSET(fdWrite, &exceptfds), "FD should not be set\n");
 
    todo_wine {
    ok ((listen(fdWrite, SOMAXCONN) == SOCKET_ERROR), "listen did not fail\n");
    }
    ret = closesocket(fdWrite);
    ok ( (ret == 0), "closesocket failed unexpectedly: %d\n", ret);

    thread_params.s = fdRead;
    thread_params.ReadKilled = FALSE;
    server_ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    thread_handle = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) &SelectReadThread, &thread_params, 0, &id );
    ok ( (thread_handle != NULL), "CreateThread failed unexpectedly: %d\n", GetLastError());

    WaitForSingleObject (server_ready, INFINITE);
    Sleep(200);
    ret = closesocket(fdRead);
    ok ( (ret == 0), "closesocket failed unexpectedly: %d\n", ret);

    WaitForSingleObject (thread_handle, 1000);
    ok ( (thread_params.ReadKilled) ||
         broken(thread_params.ReadKilled == 0), /*Win98*/
            "closesocket did not wakeup select\n");

}

static DWORD WINAPI AcceptKillThread(select_thread_params *par)
{
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

static void test_accept(void)
{
    int ret;
    SOCKET server_socket = INVALID_SOCKET;
    struct sockaddr_in address;
    select_thread_params thread_params;
    HANDLE thread_handle = NULL;
    DWORD id;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        trace("error creating server socket: %d\n", WSAGetLastError());
        goto done;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    ret = bind(server_socket, (struct sockaddr*) &address, sizeof(address));
    if (ret != 0)
    {
        trace("error binding server socket: %d\n", WSAGetLastError());
        goto done;
    }

    ret = listen(server_socket, 1);
    if (ret != 0)
    {
        trace("error making server socket listen: %d\n", WSAGetLastError());
        goto done;
    }

    server_ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (server_ready == INVALID_HANDLE_VALUE)
    {
        trace("error creating event: %d\n", GetLastError());
        goto done;
    }

    thread_params.s = server_socket;
    thread_params.ReadKilled = FALSE;
    thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) AcceptKillThread,
        &thread_params, 0, &id);
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
    ok(thread_params.ReadKilled, "closesocket did not wakeup accept\n");

done:
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

    optlen = sizeof(LINGER);
    ret = getsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger_val, &optlen);
    todo_wine{
    ok(ret == SOCKET_ERROR, "getsockopt should fail for UDP sockets but return value is 0x%08x\n", ret);
    }

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

    memcpy(&sa_get, &sa_set, sizeof(sa_set));
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
    ok(ret == 0 || broken(ret != 0), /* NT4 */
            "getsockname did not zero the sockaddr_in structure\n");

    closesocket(sock);
    WSACleanup();
}

static void test_dns(void)
{
    struct hostent *h;

    h = gethostbyname("");
    ok(h != NULL, "gethostbyname(\"\") failed with %d\n", h_errno);
}

/* Our winsock headers don't define gethostname because it conflicts with the
 * definition in unistd.h. Define it here to get rid of the warning. */

int WINAPI gethostname(char *name, int namelen);

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
           "gethostbyname(\"localhost\") returned %d.%d.%d.%d\n",
           he->h_addr_list[0][0], he->h_addr_list[0][1], he->h_addr_list[0][2],
           he->h_addr_list[0][3]);
    }

    /* No reason to test further with NULL hostname */
    if(name == NULL)
        return;

    if(strcmp(name, "localhost") == 0)
    {
        skip("hostname seems to be \"localhost\", skipping test.\n");
        return;
    }

    he = NULL;
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
               "gethostbyname(\"%s\") returned %d.%d.%d.%d not 127.12.34.56\n",
               name, he->h_addr_list[0][0], he->h_addr_list[0][1],
               he->h_addr_list[0][2], he->h_addr_list[0][3]);
        }
    }

    he = NULL;
    he = gethostbyname("nonexistent.winehq.org");
    /* Don't check for the return value, as some braindead ISPs will kindly
     * resolve nonexistent host names to addresses of the ISP's spam pages. */
}

static void test_inet_addr(void)
{
    u_long addr;

    addr = inet_addr(NULL);
    ok(addr == INADDR_NONE, "inet_addr succeeded unexpectedly\n");
}

static void test_ioctlsocket(void)
{
    SOCKET sock;
    int ret;
    long cmds[] = {FIONBIO, FIONREAD, SIOCATMARK};
    int i;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(sock != INVALID_SOCKET, "Creating the socket failed: %d\n", WSAGetLastError());
    if(sock == INVALID_SOCKET)
    {
        skip("Can't continue without a socket.\n");
        return;
    }

    for(i = 0; i < sizeof(cmds)/sizeof(long); i++)
    {
        /* broken apps like defcon pass the argp value directly instead of a pointer to it */
        ret = ioctlsocket(sock, cmds[i], (u_long *)1);
        ok(ret == SOCKET_ERROR, "ioctlsocket succeeded unexpectedly\n");
        ret = WSAGetLastError();
        ok(ret == WSAEFAULT, "expected WSAEFAULT, got %d instead\n", ret);
    }
}

static int drain_pause=0;
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
                select(0, &readset, NULL, NULL, NULL);
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
    int ret;
    DWORD id;

    if (tcp_socketpair(&src, &dst) != 0)
    {
        ok(0, "creating socket pair failed, skipping test\n");
        return;
    }

    hThread = CreateThread(NULL, 0, drain_socket_thread, &dst, 0, &id);
    if (hThread == NULL)
    {
        ok(0, "CreateThread failed, error %d\n", GetLastError());
        goto end;
    }

    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    if (buffer == NULL)
    {
        ok(0, "HeapAlloc failed, error %d\n", GetLastError());
        goto end;
    }

    ret = send(src, buffer, buflen, 0);
    if (ret >= 0)
        ok(ret == buflen, "send should have sent %d bytes, but it only sent %d\n", buflen, ret);
    else
        ok(0, "send failed, error %d\n", WSAGetLastError());

end:
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (dst != INVALID_SOCKET)
        closesocket(dst);
    if (hThread != NULL)
        CloseHandle(hThread);
    HeapFree(GetProcessHeap(), 0, buffer);
}

static void test_write_events(void)
{
    SOCKET src = INVALID_SOCKET;
    SOCKET dst = INVALID_SOCKET;
    HANDLE hThread = NULL;
    HANDLE hEvent = INVALID_HANDLE_VALUE;
    char *buffer = NULL;
    int bufferSize = 1024*1024;
    u_long one = 1;
    int ret;
    DWORD id;
    WSANETWORKEVENTS netEvents;
    DWORD dwRet;

    if (tcp_socketpair(&src, &dst) != 0)
    {
        ok(0, "creating socket pair failed, skipping test\n");
        return;
    }

    /* On Windows it seems when a non-blocking socket sends to a
       blocking socket on the same host, the send() is BLOCKING,
       so make both sockets non-blocking */
    ret = ioctlsocket(src, FIONBIO, &one);
    if (ret)
    {
        ok(0, "ioctlsocket failed, error %d\n", WSAGetLastError());
        goto end;
    }
    ret = ioctlsocket(dst, FIONBIO, &one);
    if (ret)
    {
        ok(0, "ioctlsocket failed, error %d\n", WSAGetLastError());
        goto end;
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (buffer == NULL)
    {
        ok(0, "could not allocate memory for test\n");
        goto end;
    }

    hThread = CreateThread(NULL, 0, drain_socket_thread, &dst, 0, &id);
    if (hThread == NULL)
    {
        ok(0, "CreateThread failed, error %d\n", GetLastError());
        goto end;
    }

    hEvent = CreateEventA(NULL, FALSE, TRUE, NULL);
    if (hEvent == INVALID_HANDLE_VALUE)
    {
        ok(0, "CreateEventA failed, error %d\n", GetLastError());
        goto end;
    }

    ret = WSAEventSelect(src, hEvent, FD_WRITE | FD_CLOSE);
    if (ret)
    {
        ok(0, "WSAEventSelect failed, error %d\n", ret);
        goto end;
    }

    /* FD_WRITE should be set initially, and allow us to send at least 1 byte */
    dwRet = WaitForSingleObject(hEvent, 5000);
    if (dwRet != WAIT_OBJECT_0)
    {
        ok(0, "Initial WaitForSingleObject failed, error %d\n", dwRet);
        goto end;
    }
    ret = WSAEnumNetworkEvents(src, NULL, &netEvents);
    if (ret)
    {
        ok(0, "WSAEnumNetworkEvents failed, error %d\n", ret);
        goto end;
    }
    if (netEvents.lNetworkEvents & FD_WRITE)
    {
        ret = send(src, "a", 1, 0);
        ok(ret == 1, "sending 1 byte failed, error %d\n", WSAGetLastError());
        if (ret != 1)
            goto end;
    }
    else
    {
        ok(0, "FD_WRITE not among initial events\n");
        goto end;
    }

    /* Now FD_WRITE should not be set, because the socket send buffer isn't full yet */
    dwRet = WaitForSingleObject(hEvent, 2000);
    if (dwRet == WAIT_OBJECT_0)
    {
        ok(0, "WaitForSingleObject should have timed out, but succeeded!\n");
        goto end;
    }

    /* Now if we send a ton of data and the 'server' does not drain it fast
     * enough (set drain_pause to be sure), the socket send buffer will only
     * take some of it, and we will get a short write. This will trigger
     * another FD_WRITE event as soon as data is sent and more space becomes
     * available, but not any earlier. */
    drain_pause=1;
    do
    {
        ret = send(src, buffer, bufferSize, 0);
    } while (ret == bufferSize);
    drain_pause=0;
    if (ret >= 0 || WSAGetLastError() == WSAEWOULDBLOCK)
    {
        dwRet = WaitForSingleObject(hEvent, 5000);
        ok(dwRet == WAIT_OBJECT_0, "Waiting failed with %d\n", dwRet);
        if (dwRet == WAIT_OBJECT_0)
        {
            ret = WSAEnumNetworkEvents(src, NULL, &netEvents);
            ok(ret == 0, "WSAEnumNetworkEvents failed, error %d\n", ret);
            if (ret == 0)
                goto end;
            ok(netEvents.lNetworkEvents & FD_WRITE,
                "FD_WRITE event not set as expected, events are 0x%x\n", netEvents.lNetworkEvents);
        }
        else
            goto end;
    }
    else
    {
        ok(0, "sending a lot of data failed with error %d\n", WSAGetLastError());
        goto end;
    }

end:
    HeapFree(GetProcessHeap(), 0, buffer);
    if (src != INVALID_SOCKET)
        closesocket(src);
    if (dst != INVALID_SOCKET)
        closesocket(dst);
    if (hThread != NULL)
        CloseHandle(hThread);
    CloseHandle(hEvent);
}

static void test_ipv6only(void)
{
    SOCKET v4 = INVALID_SOCKET,
           v6 = INVALID_SOCKET;
    struct sockaddr_in sin4;
    struct sockaddr_in6 sin6;
    int ret;

    memset(&sin4, 0, sizeof(sin4));
    sin4.sin_family = AF_INET;
    sin4.sin_port = htons(SERVERPORT);

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(SERVERPORT);

    v6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (v6 == INVALID_SOCKET) {
        skip("Could not create IPv6 socket (LastError: %d; %d expected if IPv6 not available).\n",
            WSAGetLastError(), WSAEAFNOSUPPORT);
        goto end;
    }
    ret = bind(v6, (struct sockaddr*)&sin6, sizeof(sin6));
    if (ret) {
        skip("Could not bind IPv6 address (LastError: %d).\n",
            WSAGetLastError());
        goto end;
    }

    v4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (v4 == INVALID_SOCKET) {
        skip("Could not create IPv4 socket (LastError: %d).\n",
            WSAGetLastError());
        goto end;
    }
    ret = bind(v4, (struct sockaddr*)&sin4, sizeof(sin4));
    ok(!ret, "Could not bind IPv4 address (LastError: %d; %d expected if IPv6 binds to IPv4 as well).\n",
        WSAGetLastError(), WSAEADDRINUSE);

end:
    if (v4 != INVALID_SOCKET)
        closesocket(v4);
    if (v6 != INVALID_SOCKET)
        closesocket(v6);
}

static void test_WSASendTo(void)
{
    SOCKET s;
    struct sockaddr_in addr;
    char buf[12] = "hello world";
    WSABUF data_buf;
    DWORD bytesSent;

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
    if(WSASendTo(s, &data_buf, 1, &bytesSent, 0, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL)) {
        ok(0, "WSASendTo() failed error: %d\n", WSAGetLastError());
        return;
    }
    ok(!WSAGetLastError(), "WSAGetLastError() should return zero after "
            "a successful call to WSASendTo()\n");
}

/**************** Main program  ***************/

START_TEST( sock )
{
    int i;
    Init();

    test_set_getsockopt();
    test_so_reuseaddr();
    test_extendedSocketOptions();

    for (i = 0; i < NUM_TESTS; i++)
    {
        trace ( " **** STARTING TEST %d ****\n", i );
        do_test (  &tests[i] );
        trace ( " **** TEST %d COMPLETE ****\n", i );
    }

    test_UDP();

    test_getservbyname();
    test_WSASocket();

    test_WSAAddressToStringA();
    test_WSAAddressToStringW();

    test_WSAStringToAddressA();
    test_WSAStringToAddressW();

    test_select();
    test_accept();
    test_getsockname();
    test_inet_addr();
    test_ioctlsocket();
    test_dns();
    test_gethostbyname_hack();

    test_send();
    test_write_events();

    test_WSASendTo();

    test_ipv6only();

    Exit();
}
