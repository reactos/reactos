/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#ifdef HAVE_OPENSSL_SSL_H
# include <openssl/ssl.h>
#undef FAR
#undef DSA
#endif

#include "wine/debug.h"
#include "wine/library.h"

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

/* to avoid conflicts with the Unix socket headers */
#define USE_WS_PREFIX
#include "winsock2.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define DEFAULT_SEND_TIMEOUT        30
#define DEFAULT_RECEIVE_TIMEOUT     30
#define RESPONSE_TIMEOUT            30

#ifndef HAVE_GETADDRINFO

/* critical section to protect non-reentrant gethostbyname() */
static CRITICAL_SECTION cs_gethostbyname;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &cs_gethostbyname,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs_gethostbyname") }
};
static CRITICAL_SECTION cs_gethostbyname = { &critsect_debug, -1, 0, 0, 0, 0 };

#endif

#ifdef SONAME_LIBSSL

#include <openssl/err.h>

static void *libssl_handle;
static void *libcrypto_handle;

static SSL_METHOD *method;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

MAKE_FUNCPTR( SSL_library_init );
MAKE_FUNCPTR( SSL_load_error_strings );
MAKE_FUNCPTR( SSLv23_method );
MAKE_FUNCPTR( SSL_CTX_new );
MAKE_FUNCPTR( SSL_CTX_free );
MAKE_FUNCPTR( SSL_new );
MAKE_FUNCPTR( SSL_free );
MAKE_FUNCPTR( SSL_set_fd );
MAKE_FUNCPTR( SSL_connect );
MAKE_FUNCPTR( SSL_shutdown );
MAKE_FUNCPTR( SSL_write );
MAKE_FUNCPTR( SSL_read );
MAKE_FUNCPTR( SSL_get_verify_result );
MAKE_FUNCPTR( SSL_get_peer_certificate );
MAKE_FUNCPTR( SSL_CTX_get_timeout );
MAKE_FUNCPTR( SSL_CTX_set_timeout );
MAKE_FUNCPTR( SSL_CTX_set_default_verify_paths );

MAKE_FUNCPTR( BIO_new_fp );
MAKE_FUNCPTR( ERR_get_error );
MAKE_FUNCPTR( ERR_error_string );
#undef MAKE_FUNCPTR

#endif

#if 0
/* translate a unix error code into a winsock error code */
static int sock_get_error( int err )
{
    switch (err)
    {
        case EINTR:             return WSAEINTR;
        case EBADF:             return WSAEBADF;
        case EPERM:
        case EACCES:            return WSAEACCES;
        case EFAULT:            return WSAEFAULT;
        case EINVAL:            return WSAEINVAL;
        case EMFILE:            return WSAEMFILE;
        case EWOULDBLOCK:       return WSAEWOULDBLOCK;
        case EINPROGRESS:       return WSAEINPROGRESS;
        case EALREADY:          return WSAEALREADY;
        case ENOTSOCK:          return WSAENOTSOCK;
        case EDESTADDRREQ:      return WSAEDESTADDRREQ;
        case EMSGSIZE:          return WSAEMSGSIZE;
        case EPROTOTYPE:        return WSAEPROTOTYPE;
        case ENOPROTOOPT:       return WSAENOPROTOOPT;
        case EPROTONOSUPPORT:   return WSAEPROTONOSUPPORT;
        case ESOCKTNOSUPPORT:   return WSAESOCKTNOSUPPORT;
        case EOPNOTSUPP:        return WSAEOPNOTSUPP;
        case EPFNOSUPPORT:      return WSAEPFNOSUPPORT;
        case EAFNOSUPPORT:      return WSAEAFNOSUPPORT;
        case EADDRINUSE:        return WSAEADDRINUSE;
        case EADDRNOTAVAIL:     return WSAEADDRNOTAVAIL;
        case ENETDOWN:          return WSAENETDOWN;
        case ENETUNREACH:       return WSAENETUNREACH;
        case ENETRESET:         return WSAENETRESET;
        case ECONNABORTED:      return WSAECONNABORTED;
        case EPIPE:
        case ECONNRESET:        return WSAECONNRESET;
        case ENOBUFS:           return WSAENOBUFS;
        case EISCONN:           return WSAEISCONN;
        case ENOTCONN:          return WSAENOTCONN;
        case ESHUTDOWN:         return WSAESHUTDOWN;
        case ETOOMANYREFS:      return WSAETOOMANYREFS;
        case ETIMEDOUT:         return WSAETIMEDOUT;
        case ECONNREFUSED:      return WSAECONNREFUSED;
        case ELOOP:             return WSAELOOP;
        case ENAMETOOLONG:      return WSAENAMETOOLONG;
        case EHOSTDOWN:         return WSAEHOSTDOWN;
        case EHOSTUNREACH:      return WSAEHOSTUNREACH;
        case ENOTEMPTY:         return WSAENOTEMPTY;
#ifdef EPROCLIM
        case EPROCLIM:          return WSAEPROCLIM;
#endif
#ifdef EUSERS
        case EUSERS:            return WSAEUSERS;
#endif
#ifdef EDQUOT
        case EDQUOT:            return WSAEDQUOT;
#endif
#ifdef ESTALE
        case ESTALE:            return WSAESTALE;
#endif
#ifdef EREMOTE
        case EREMOTE:           return WSAEREMOTE;
#endif
    default: errno = err; perror( "sock_set_error" ); return WSAEFAULT;
    }
    return err;
}
#else
#define sock_get_error(x) WSAGetLastError()
#endif

BOOL netconn_init( netconn_t *conn, BOOL secure )
{
    conn->socket = -1;
    if (!secure) return TRUE;

#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    if (libssl_handle) return TRUE;
    if (!(libssl_handle = wine_dlopen( SONAME_LIBSSL, RTLD_NOW, NULL, 0 )))
    {
        ERR("Trying to use SSL but couldn't load %s. Expect trouble.\n", SONAME_LIBSSL);
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        return FALSE;
    }
    if (!(libcrypto_handle = wine_dlopen( SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0 )))
    {
        ERR("Trying to use SSL but couldn't load %s. Expect trouble.\n", SONAME_LIBCRYPTO);
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        return FALSE;
    }
#define LOAD_FUNCPTR(x) \
    if (!(p##x = wine_dlsym( libssl_handle, #x, NULL, 0 ))) \
    { \
        ERR("Failed to load symbol %s\n", #x); \
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR ); \
        return FALSE; \
    }
    LOAD_FUNCPTR( SSL_library_init );
    LOAD_FUNCPTR( SSL_load_error_strings );
    LOAD_FUNCPTR( SSLv23_method );
    LOAD_FUNCPTR( SSL_CTX_new );
    LOAD_FUNCPTR( SSL_CTX_free );
    LOAD_FUNCPTR( SSL_new );
    LOAD_FUNCPTR( SSL_free );
    LOAD_FUNCPTR( SSL_set_fd );
    LOAD_FUNCPTR( SSL_connect );
    LOAD_FUNCPTR( SSL_shutdown );
    LOAD_FUNCPTR( SSL_write );
    LOAD_FUNCPTR( SSL_read );
    LOAD_FUNCPTR( SSL_get_verify_result );
    LOAD_FUNCPTR( SSL_get_peer_certificate );
    LOAD_FUNCPTR( SSL_CTX_get_timeout );
    LOAD_FUNCPTR( SSL_CTX_set_timeout );
    LOAD_FUNCPTR( SSL_CTX_set_default_verify_paths );
#undef LOAD_FUNCPTR

#define LOAD_FUNCPTR(x) \
    if (!(p##x = wine_dlsym( libcrypto_handle, #x, NULL, 0 ))) \
    { \
        ERR("Failed to load symbol %s\n", #x); \
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR ); \
        return FALSE; \
    }
    LOAD_FUNCPTR( BIO_new_fp );
    LOAD_FUNCPTR( ERR_get_error );
    LOAD_FUNCPTR( ERR_error_string );
#undef LOAD_FUNCPTR

    pSSL_library_init();
    pSSL_load_error_strings();
    pBIO_new_fp( stderr, BIO_NOCLOSE );

    method = pSSLv23_method();
    conn->ssl_ctx = pSSL_CTX_new( method );

#else
    WARN("SSL support not compiled in.\n");
    set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
    return FALSE;
#endif
    return TRUE;
}

BOOL netconn_connected( netconn_t *conn )
{
    return (conn->socket != -1);
}

BOOL netconn_create( netconn_t *conn, int domain, int type, int protocol )
{
    if ((conn->socket = socket( domain, type, protocol )) == -1)
    {
        WARN("unable to create socket (%s)\n", strerror(errno));
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_close( netconn_t *conn )
{
    int res;

#ifdef SONAME_LIBSSL
    if (conn->secure)
    {
        heap_free( conn->peek_msg_mem );
        conn->peek_msg_mem = NULL;
        conn->peek_msg = NULL;
        conn->peek_len = 0;

        pSSL_shutdown( conn->ssl_conn );
        pSSL_free( conn->ssl_conn );

        conn->ssl_conn = NULL;
        conn->secure = FALSE;
    }
#endif
    res = closesocket( conn->socket );
    conn->socket = -1;
    if (res == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_connect( netconn_t *conn, const struct sockaddr *sockaddr, unsigned int addr_len )
{
    if (connect( conn->socket, sockaddr, addr_len ) == -1)
    {
        WARN("unable to connect to host (%s)\n", strerror(errno));
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_secure_connect( netconn_t *conn )
{
#ifdef SONAME_LIBSSL
    X509 *cert;
    long res;

    if (!pSSL_CTX_set_default_verify_paths( conn->ssl_ctx ))
    {
        ERR("SSL_CTX_set_default_verify_paths failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        return FALSE;
    }
    if (!(conn->ssl_conn = pSSL_new( conn->ssl_ctx )))
    {
        ERR("SSL_new failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        goto fail;
    }
    if (!pSSL_set_fd( conn->ssl_conn, conn->socket ))
    {
        ERR("SSL_set_fd failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        goto fail;
    }
    if (pSSL_connect( conn->ssl_conn ) <= 0)
    {
        ERR("SSL_connect failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        goto fail;
    }
    if (!(cert = pSSL_get_peer_certificate( conn->ssl_conn )))
    {
        ERR("No certificate for server: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        goto fail;
    }
    if ((res = pSSL_get_verify_result( conn->ssl_conn )) != X509_V_OK)
    {
        /* FIXME: we should set an error and return, but we only print an error at the moment */
        ERR("couldn't verify server certificate (%ld)\n", res);
    }
    TRACE("established SSL connection\n");
    conn->secure = TRUE;
    return TRUE;

fail:
    if (conn->ssl_conn)
    {
        pSSL_shutdown( conn->ssl_conn );
        pSSL_free( conn->ssl_conn );
        conn->ssl_conn = NULL;
    }
#endif
    return FALSE;
}

BOOL netconn_send( netconn_t *conn, const void *msg, size_t len, int flags, int *sent )
{
    if (!netconn_connected( conn )) return FALSE;
    if (conn->secure)
    {
#ifdef SONAME_LIBSSL
        if (flags) FIXME("SSL_write doesn't support any flags (%08x)\n", flags);
        *sent = pSSL_write( conn->ssl_conn, msg, len );
        if (*sent < 1 && len) return FALSE;
        return TRUE;
#else
        return FALSE;
#endif
    }
    if ((*sent = send( conn->socket, msg, len, flags )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_recv( netconn_t *conn, void *buf, size_t len, int flags, int *recvd )
{
    *recvd = 0;
    if (!netconn_connected( conn )) return FALSE;
    if (!len) return TRUE;

    if (conn->secure)
    {
#ifdef SONAME_LIBSSL
        if (flags & ~(MSG_PEEK | MSG_WAITALL))
            FIXME("SSL_read does not support the following flags: %08x\n", flags);

        /* this ugly hack is all for MSG_PEEK */
        if (flags & MSG_PEEK && !conn->peek_msg)
        {
            if (!(conn->peek_msg = conn->peek_msg_mem = heap_alloc( len + 1 ))) return FALSE;
        }
        else if (flags & MSG_PEEK && conn->peek_msg)
        {
            if (len < conn->peek_len) FIXME("buffer isn't big enough, should we wrap?\n");
            *recvd = min( len, conn->peek_len );
            memcpy( buf, conn->peek_msg, *recvd );
            return TRUE;
        }
        else if (conn->peek_msg)
        {
            *recvd = min( len, conn->peek_len );
            memcpy( buf, conn->peek_msg, *recvd );
            conn->peek_len -= *recvd;
            conn->peek_msg += *recvd;

            if (conn->peek_len == 0)
            {
                heap_free( conn->peek_msg_mem );
                conn->peek_msg_mem = NULL;
                conn->peek_msg = NULL;
            }
            /* check if we have enough data from the peek buffer */
            if (!(flags & MSG_WAITALL) || (*recvd == len)) return TRUE;
        }
        *recvd += pSSL_read( conn->ssl_conn, (char *)buf + *recvd, len - *recvd );
        if (flags & MSG_PEEK) /* must copy into buffer */
        {
            conn->peek_len = *recvd;
            if (!*recvd)
            {
                heap_free( conn->peek_msg_mem );
                conn->peek_msg_mem = NULL;
                conn->peek_msg = NULL;
            }
            else memcpy( conn->peek_msg, buf, *recvd );
        }
        if (*recvd < 1 && len) return FALSE;
        return TRUE;
#else
        return FALSE;
#endif
    }
    if ((*recvd = recv( conn->socket, buf, len, flags )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

BOOL netconn_query_data_available( netconn_t *conn, DWORD *available )
{
#ifdef FIONREAD
    int ret, unread;
#endif
    *available = 0;
    if (!netconn_connected( conn )) return FALSE;

    if (conn->secure)
    {
#ifdef SONAME_LIBSSL
        if (conn->peek_msg) *available = conn->peek_len;
#endif
        return TRUE;
    }
#ifdef FIONREAD
    if (!(ret = ioctlsocket( conn->socket, FIONREAD, &unread )))
    {
        TRACE("%d bytes of queued, but unread data\n", unread);
        *available += unread;
    }
#endif
    return TRUE;
}

BOOL netconn_get_next_line( netconn_t *conn, char *buffer, DWORD *buflen )
{
    struct timeval tv;
    fd_set infd;
    BOOL ret = FALSE;
    DWORD recvd = 0;

    if (!netconn_connected( conn )) return FALSE;

    if (conn->secure)
    {
#ifdef SONAME_LIBSSL
        long timeout;

        timeout = pSSL_CTX_get_timeout( conn->ssl_ctx );
        pSSL_CTX_set_timeout( conn->ssl_ctx, DEFAULT_RECEIVE_TIMEOUT );

        while (recvd < *buflen)
        {
            int dummy;
            if (!netconn_recv( conn, &buffer[recvd], 1, 0, &dummy ))
            {
                set_last_error( ERROR_CONNECTION_ABORTED );
                break;
            }
            if (buffer[recvd] == '\n')
            {
                ret = TRUE;
                break;
            }
            if (buffer[recvd] != '\r') recvd++;
        }
        pSSL_CTX_set_timeout( conn->ssl_ctx, timeout );
        if (ret)
        {
            buffer[recvd++] = 0;
            *buflen = recvd;
            TRACE("received line %s\n", debugstr_a(buffer));
        }
        return ret;
#else
        return FALSE;
#endif
    }

    FD_ZERO(&infd);
    FD_SET(conn->socket, &infd);
    tv.tv_sec=RESPONSE_TIMEOUT;
    tv.tv_usec=0;
    while (recvd < *buflen)
    {
        if (select(conn->socket+1,&infd,NULL,NULL,&tv) > 0)
        {
            int res;
            if ((res = recv( conn->socket, &buffer[recvd], 1, 0 )) <= 0)
            {
                if (res == -1) set_last_error( sock_get_error( errno ) );
                break;
            }
            if (buffer[recvd] == '\n')
            {
                ret = TRUE;
                break;
            }
            if (buffer[recvd] != '\r') recvd++;
        }
        else
        {
            set_last_error( ERROR_WINHTTP_TIMEOUT );
            break;
        }
    }
    if (ret)
    {
        buffer[recvd++] = 0;
        *buflen = recvd;
        TRACE("received line %s\n", debugstr_a(buffer));
    }
    return ret;
}

DWORD netconn_set_timeout( netconn_t *netconn, BOOL send, int value )
{
    int res;
    struct timeval tv;

    /* value is in milliseconds, convert to struct timeval */
    tv.tv_sec = value / 1000;
    tv.tv_usec = (value % 1000) * 1000;

    if ((res = setsockopt( netconn->socket, SOL_SOCKET, send ? SO_SNDTIMEO : SO_RCVTIMEO, &tv, sizeof(tv) ) == -1))
    {
        WARN("setsockopt failed (%s)\n", strerror( errno ));
        return sock_get_error( errno );
    }
    return ERROR_SUCCESS;
}

BOOL netconn_resolve( WCHAR *hostnameW, INTERNET_PORT port, struct sockaddr_in *sa )
{
    char *hostname;
#ifdef HAVE_GETADDRINFO
    struct addrinfo *res, hints;
    int ret;
#else
    struct hostent *he;
#endif

    if (!(hostname = strdupWA( hostnameW ))) return FALSE;

#ifdef HAVE_GETADDRINFO
    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_family = AF_INET;

    ret = getaddrinfo( hostname, NULL, &hints, &res );
    heap_free( hostname );
    if (ret != 0)
    {
        TRACE("failed to get address of %s (%s)\n", debugstr_a(hostname), gai_strerror(ret));
        return FALSE;
    }
    memset( sa, 0, sizeof(struct sockaddr_in) );
    memcpy( &sa->sin_addr, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sizeof(struct in_addr) );
    sa->sin_family = res->ai_family;
    sa->sin_port = htons( port );

    freeaddrinfo( res );
#else
    EnterCriticalSection( &cs_gethostbyname );

    he = gethostbyname( hostname );
    heap_free( hostname );
    if (!he)
    {
        TRACE("failed to get address of %s (%d)\n", debugstr_a(hostname), h_errno);
        LeaveCriticalSection( &cs_gethostbyname );
        return FALSE;
    }
    memset( sa, 0, sizeof(struct sockaddr_in) );
    memcpy( (char *)&sa->sin_addr, he->h_addr, he->h_length );
    sa->sin_family = he->h_addrtype;
    sa->sin_port = htons( port );

    LeaveCriticalSection( &cs_gethostbyname );
#endif
    return TRUE;
}
