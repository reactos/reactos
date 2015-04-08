/*
 *  TCP networking functions
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_NET_C)

#include "polarssl/net.h"

#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)

#if defined(POLARSSL_HAVE_IPV6)
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
/* Enables getaddrinfo() & Co */
#define _WIN32_WINNT 0x0501
#include <ws2tcpip.h>
#endif

#include <winsock2.h>
#include <windows.h>

#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment( lib, "ws2.lib" )
#else
#pragma comment( lib, "ws2_32.lib" )
#endif
#endif /* _MSC_VER */

#define read(fd,buf,len)        recv(fd,(char*)buf,(int) len,0)
#define write(fd,buf,len)       send(fd,(char*)buf,(int) len,0)
#define close(fd)               closesocket(fd)

static int wsa_init_done = 0;

#else /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(POLARSSL_HAVE_TIME)
#include <sys/time.h>
#endif
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) ||  \
    defined(__DragonFly__)
#include <sys/endian.h>
#elif defined(__APPLE__) || defined(HAVE_MACHINE_ENDIAN_H) ||   \
      defined(EFIX64) || defined(EFI32)
#include <machine/endian.h>
#elif defined(sun)
#include <sys/isa_defs.h>
#elif defined(_AIX) || defined(HAVE_ARPA_NAMESER_COMPAT_H)
#include <arpa/nameser_compat.h>
#else
#include <endian.h>
#endif

#endif /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */

#include <stdlib.h>
#include <stdio.h>

#if defined(_MSC_VER) && !defined  snprintf && !defined(EFIX64) && \
    !defined(EFI32)
#define  snprintf  _snprintf
#endif

#if defined(POLARSSL_HAVE_TIME)
#include <time.h>
#endif

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

/*
 * htons() is not always available.
 * By default go for LITTLE_ENDIAN variant. Otherwise hope for _BYTE_ORDER and
 * __BIG_ENDIAN to help determine endianness.
 */
#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) &&                   \
    __BYTE_ORDER == __BIG_ENDIAN
#define POLARSSL_HTONS(n) (n)
#define POLARSSL_HTONL(n) (n)
#else
#define POLARSSL_HTONS(n) ((((unsigned short)(n) & 0xFF      ) << 8 ) | \
                           (((unsigned short)(n) & 0xFF00    ) >> 8 ))
#define POLARSSL_HTONL(n) ((((unsigned long )(n) & 0xFF      ) << 24) | \
                           (((unsigned long )(n) & 0xFF00    ) << 8 ) | \
                           (((unsigned long )(n) & 0xFF0000  ) >> 8 ) | \
                           (((unsigned long )(n) & 0xFF000000) >> 24))
#endif

unsigned short net_htons( unsigned short n );
unsigned long  net_htonl( unsigned long  n );
#define net_htons(n) POLARSSL_HTONS(n)
#define net_htonl(n) POLARSSL_HTONL(n)

/*
 * Prepare for using the sockets interface
 */
static int net_prepare( void )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
    WSADATA wsaData;

    if( wsa_init_done == 0 )
    {
        if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 )
            return( POLARSSL_ERR_NET_SOCKET_FAILED );

        wsa_init_done = 1;
    }
#else
#if !defined(EFIX64) && !defined(EFI32)
    signal( SIGPIPE, SIG_IGN );
#endif
#endif
    return( 0 );
}

/*
 * Initiate a TCP connection with host:port
 */
int net_connect( int *fd, const char *host, int port )
{
#if defined(POLARSSL_HAVE_IPV6)
    int ret;
    struct addrinfo hints, *addr_list, *cur;
    char port_str[6];

    if( ( ret = net_prepare() ) != 0 )
        return( ret );

    /* getaddrinfo expects port as a string */
    memset( port_str, 0, sizeof( port_str ) );
    snprintf( port_str, sizeof( port_str ), "%d", port );

    /* Do name resolution with both IPv6 and IPv4, but only TCP */
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if( getaddrinfo( host, port_str, &hints, &addr_list ) != 0 )
        return( POLARSSL_ERR_NET_UNKNOWN_HOST );

    /* Try the sockaddrs until a connection succeeds */
    ret = POLARSSL_ERR_NET_UNKNOWN_HOST;
    for( cur = addr_list; cur != NULL; cur = cur->ai_next )
    {
        *fd = (int) socket( cur->ai_family, cur->ai_socktype,
                            cur->ai_protocol );
        if( *fd < 0 )
        {
            ret = POLARSSL_ERR_NET_SOCKET_FAILED;
            continue;
        }

        if( connect( *fd, cur->ai_addr, cur->ai_addrlen ) == 0 )
        {
            ret = 0;
            break;
        }

        close( *fd );
        ret = POLARSSL_ERR_NET_CONNECT_FAILED;
    }

    freeaddrinfo( addr_list );

    return( ret );

#else
    /* Legacy IPv4-only version */

    int ret;
    struct sockaddr_in server_addr;
    struct hostent *server_host;

    if( ( ret = net_prepare() ) != 0 )
        return( ret );

    if( ( server_host = gethostbyname( host ) ) == NULL )
        return( POLARSSL_ERR_NET_UNKNOWN_HOST );

    if( ( *fd = (int) socket( AF_INET, SOCK_STREAM, IPPROTO_IP ) ) < 0 )
        return( POLARSSL_ERR_NET_SOCKET_FAILED );

    memcpy( (void *) &server_addr.sin_addr,
            (void *) server_host->h_addr,
                     server_host->h_length );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = net_htons( port );

    if( connect( *fd, (struct sockaddr *) &server_addr,
                 sizeof( server_addr ) ) < 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_CONNECT_FAILED );
    }

    return( 0 );
#endif /* POLARSSL_HAVE_IPV6 */
}

/*
 * Create a listening socket on bind_ip:port
 */
int net_bind( int *fd, const char *bind_ip, int port )
{
#if defined(POLARSSL_HAVE_IPV6)
    int n, ret;
    struct addrinfo hints, *addr_list, *cur;
    char port_str[6];

    if( ( ret = net_prepare() ) != 0 )
        return( ret );

    /* getaddrinfo expects port as a string */
    memset( port_str, 0, sizeof( port_str ) );
    snprintf( port_str, sizeof( port_str ), "%d", port );

    /* Bind to IPv6 and/or IPv4, but only in TCP */
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if( bind_ip == NULL )
        hints.ai_flags = AI_PASSIVE;

    if( getaddrinfo( bind_ip, port_str, &hints, &addr_list ) != 0 )
        return( POLARSSL_ERR_NET_UNKNOWN_HOST );

    /* Try the sockaddrs until a binding succeeds */
    ret = POLARSSL_ERR_NET_UNKNOWN_HOST;
    for( cur = addr_list; cur != NULL; cur = cur->ai_next )
    {
        *fd = (int) socket( cur->ai_family, cur->ai_socktype,
                            cur->ai_protocol );
        if( *fd < 0 )
        {
            ret = POLARSSL_ERR_NET_SOCKET_FAILED;
            continue;
        }

        n = 1;
        if( setsockopt( *fd, SOL_SOCKET, SO_REUSEADDR,
                        (const char *) &n, sizeof( n ) ) != 0 )
        {
            close( *fd );
            ret = POLARSSL_ERR_NET_SOCKET_FAILED;
            continue;
        }

        if( bind( *fd, cur->ai_addr, cur->ai_addrlen ) != 0 )
        {
            close( *fd );
            ret = POLARSSL_ERR_NET_BIND_FAILED;
            continue;
        }

        if( listen( *fd, POLARSSL_NET_LISTEN_BACKLOG ) != 0 )
        {
            close( *fd );
            ret = POLARSSL_ERR_NET_LISTEN_FAILED;
            continue;
        }

        /* I we ever get there, it's a success */
        ret = 0;
        break;
    }

    freeaddrinfo( addr_list );

    return( ret );

#else
    /* Legacy IPv4-only version */

    int ret, n, c[4];
    struct sockaddr_in server_addr;

    if( ( ret = net_prepare() ) != 0 )
        return( ret );

    if( ( *fd = (int) socket( AF_INET, SOCK_STREAM, IPPROTO_IP ) ) < 0 )
        return( POLARSSL_ERR_NET_SOCKET_FAILED );

    n = 1;
    setsockopt( *fd, SOL_SOCKET, SO_REUSEADDR,
                (const char *) &n, sizeof( n ) );

    server_addr.sin_addr.s_addr = net_htonl( INADDR_ANY );
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = net_htons( port );

    if( bind_ip != NULL )
    {
        memset( c, 0, sizeof( c ) );
        sscanf( bind_ip, "%d.%d.%d.%d", &c[0], &c[1], &c[2], &c[3] );

        for( n = 0; n < 4; n++ )
            if( c[n] < 0 || c[n] > 255 )
                break;

        if( n == 4 )
            server_addr.sin_addr.s_addr = net_htonl(
                ( (uint32_t) c[0] << 24 ) |
                ( (uint32_t) c[1] << 16 ) |
                ( (uint32_t) c[2] <<  8 ) |
                ( (uint32_t) c[3]       ) );
    }

    if( bind( *fd, (struct sockaddr *) &server_addr,
              sizeof( server_addr ) ) < 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_BIND_FAILED );
    }

    if( listen( *fd, POLARSSL_NET_LISTEN_BACKLOG ) != 0 )
    {
        close( *fd );
        return( POLARSSL_ERR_NET_LISTEN_FAILED );
    }

    return( 0 );
#endif /* POLARSSL_HAVE_IPV6 */
}

#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
/*
 * Check if the requested operation would be blocking on a non-blocking socket
 * and thus 'failed' with a negative return value.
 */
static int net_would_block( int fd )
{
    ((void) fd);
    return( WSAGetLastError() == WSAEWOULDBLOCK );
}
#else
/*
 * Check if the requested operation would be blocking on a non-blocking socket
 * and thus 'failed' with a negative return value.
 *
 * Note: on a blocking socket this function always returns 0!
 */
static int net_would_block( int fd )
{
    /*
     * Never return 'WOULD BLOCK' on a non-blocking socket
     */
    if( ( fcntl( fd, F_GETFL ) & O_NONBLOCK ) != O_NONBLOCK )
        return( 0 );

    switch( errno )
    {
#if defined EAGAIN
        case EAGAIN:
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return( 1 );
    }
    return( 0 );
}
#endif /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */

/*
 * Accept a connection from a remote client
 */
int net_accept( int bind_fd, int *client_fd, void *client_ip )
{
#if defined(POLARSSL_HAVE_IPV6)
    struct sockaddr_storage client_addr;
#else
    struct sockaddr_in client_addr;
#endif

#if defined(__socklen_t_defined) || defined(_SOCKLEN_T) ||  \
    defined(_SOCKLEN_T_DECLARED)
    socklen_t n = (socklen_t) sizeof( client_addr );
#else
    int n = (int) sizeof( client_addr );
#endif

    *client_fd = (int) accept( bind_fd, (struct sockaddr *)
                               &client_addr, &n );

    if( *client_fd < 0 )
    {
        if( net_would_block( bind_fd ) != 0 )
            return( POLARSSL_ERR_NET_WANT_READ );

        return( POLARSSL_ERR_NET_ACCEPT_FAILED );
    }

    if( client_ip != NULL )
    {
#if defined(POLARSSL_HAVE_IPV6)
        if( client_addr.ss_family == AF_INET )
        {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) &client_addr;
            memcpy( client_ip, &addr4->sin_addr.s_addr,
                        sizeof( addr4->sin_addr.s_addr ) );
        }
        else
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &client_addr;
            memcpy( client_ip, &addr6->sin6_addr.s6_addr,
                        sizeof( addr6->sin6_addr.s6_addr ) );
        }
#else
        memcpy( client_ip, &client_addr.sin_addr.s_addr,
                    sizeof( client_addr.sin_addr.s_addr ) );
#endif /* POLARSSL_HAVE_IPV6 */
    }

    return( 0 );
}

/*
 * Set the socket blocking or non-blocking
 */
int net_set_block( int fd )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
    u_long n = 0;
    return( ioctlsocket( fd, FIONBIO, &n ) );
#else
    return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) & ~O_NONBLOCK ) );
#endif
}

int net_set_nonblock( int fd )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
    u_long n = 1;
    return( ioctlsocket( fd, FIONBIO, &n ) );
#else
    return( fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK ) );
#endif
}

#if defined(POLARSSL_HAVE_TIME)
/*
 * Portable usleep helper
 */
void net_usleep( unsigned long usec )
{
    struct timeval tv;
    tv.tv_sec  = usec / 1000000;
#if !defined(_WIN32) && ( defined(__unix__) || defined(__unix) || \
    ( defined(__APPLE__) && defined(__MACH__) ) )
    tv.tv_usec = (suseconds_t) usec % 1000000;
#else
    tv.tv_usec = usec % 1000000;
#endif
    select( 0, NULL, NULL, NULL, &tv );
}
#endif /* POLARSSL_HAVE_TIME */

/*
 * Read at most 'len' characters
 */
int net_recv( void *ctx, unsigned char *buf, size_t len )
{
    int fd = *((int *) ctx);
    int ret = (int) read( fd, buf, len );

    if( ret < 0 )
    {
        if( net_would_block( fd ) != 0 )
            return( POLARSSL_ERR_NET_WANT_READ );

#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
        if( WSAGetLastError() == WSAECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );
#else
        if( errno == EPIPE || errno == ECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );

        if( errno == EINTR )
            return( POLARSSL_ERR_NET_WANT_READ );
#endif

        return( POLARSSL_ERR_NET_RECV_FAILED );
    }

    return( ret );
}

/*
 * Write at most 'len' characters
 */
int net_send( void *ctx, const unsigned char *buf, size_t len )
{
    int fd = *((int *) ctx);
    int ret = (int) write( fd, buf, len );

    if( ret < 0 )
    {
        if( net_would_block( fd ) != 0 )
            return( POLARSSL_ERR_NET_WANT_WRITE );

#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && \
    !defined(EFI32)
        if( WSAGetLastError() == WSAECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );
#else
        if( errno == EPIPE || errno == ECONNRESET )
            return( POLARSSL_ERR_NET_CONN_RESET );

        if( errno == EINTR )
            return( POLARSSL_ERR_NET_WANT_WRITE );
#endif

        return( POLARSSL_ERR_NET_SEND_FAILED );
    }

    return( ret );
}

/*
 * Gracefully close the connection
 */
void net_close( int fd )
{
    shutdown( fd, 2 );
    close( fd );
}

#endif /* POLARSSL_NET_C */
