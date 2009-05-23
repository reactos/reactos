/*
 * Wininet - networking layer. Uses unix sockets or OpenSSL.
 *
 * Copyright 2002 TransGaming Technologies Inc.
 *
 * David Hammerton
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

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include <sys/types.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <time.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_OPENSSL_SSL_H
# include <openssl/ssl.h>
#undef FAR
#undef DSA
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"
#include "wincrypt.h"

#include "wine/debug.h"
#include "internet.h"

/* To avoid conflicts with the Unix socket headers. we only need it for
 * the error codes anyway. */
#define USE_WS_PREFIX
#include "winsock2.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */
#define sock_get_error(x) WSAGetLastError()

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME!!!!!!
 *    This should use winsock - To use winsock the functions will have to change a bit
 *        as they are designed for unix sockets.
 *    SSL stuff should use crypt32.dll
 */

#ifdef SONAME_LIBSSL

#include <openssl/err.h>

static void *OpenSSL_ssl_handle;
static void *OpenSSL_crypto_handle;

static SSL_METHOD *meth;
static SSL_CTX *ctx;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL functions that we use */
MAKE_FUNCPTR(SSL_library_init);
MAKE_FUNCPTR(SSL_load_error_strings);
MAKE_FUNCPTR(SSLv23_method);
MAKE_FUNCPTR(SSL_CTX_new);
MAKE_FUNCPTR(SSL_new);
MAKE_FUNCPTR(SSL_free);
MAKE_FUNCPTR(SSL_set_fd);
MAKE_FUNCPTR(SSL_connect);
MAKE_FUNCPTR(SSL_shutdown);
MAKE_FUNCPTR(SSL_write);
MAKE_FUNCPTR(SSL_read);
MAKE_FUNCPTR(SSL_pending);
MAKE_FUNCPTR(SSL_get_verify_result);
MAKE_FUNCPTR(SSL_get_peer_certificate);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);
MAKE_FUNCPTR(SSL_CTX_set_default_verify_paths);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_fp);
MAKE_FUNCPTR(ERR_get_error);
MAKE_FUNCPTR(ERR_error_string);
MAKE_FUNCPTR(i2d_X509);
#undef MAKE_FUNCPTR

#endif

BOOL NETCON_init(WININET_NETCONNECTION *connection, BOOL useSSL)
{
    connection->useSSL = FALSE;
    connection->socketFD = -1;
    if (useSSL)
    {
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
        TRACE("using SSL connection\n");
	if (OpenSSL_ssl_handle) /* already initialized everything */
            return TRUE;
	OpenSSL_ssl_handle = wine_dlopen(SONAME_LIBSSL, RTLD_NOW, NULL, 0);
	if (!OpenSSL_ssl_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBSSL);
            INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
            return FALSE;
	}
	OpenSSL_crypto_handle = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
	if (!OpenSSL_crypto_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBCRYPTO);
            INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
            return FALSE;
	}

        /* mmm nice ugly macroness */
#define DYNSSL(x) \
    p##x = wine_dlsym(OpenSSL_ssl_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR); \
        return FALSE; \
    }

	DYNSSL(SSL_library_init);
	DYNSSL(SSL_load_error_strings);
	DYNSSL(SSLv23_method);
	DYNSSL(SSL_CTX_new);
	DYNSSL(SSL_new);
	DYNSSL(SSL_free);
	DYNSSL(SSL_set_fd);
	DYNSSL(SSL_connect);
	DYNSSL(SSL_shutdown);
	DYNSSL(SSL_write);
	DYNSSL(SSL_read);
	DYNSSL(SSL_pending);
	DYNSSL(SSL_get_verify_result);
	DYNSSL(SSL_get_peer_certificate);
	DYNSSL(SSL_CTX_get_timeout);
	DYNSSL(SSL_CTX_set_timeout);
	DYNSSL(SSL_CTX_set_default_verify_paths);
#undef DYNSSL

#define DYNCRYPTO(x) \
    p##x = wine_dlsym(OpenSSL_crypto_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR); \
        return FALSE; \
    }
	DYNCRYPTO(BIO_new_fp);
	DYNCRYPTO(ERR_get_error);
	DYNCRYPTO(ERR_error_string);
	DYNCRYPTO(i2d_X509);
#undef DYNCRYPTO

	pSSL_library_init();
	pSSL_load_error_strings();
	pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

	meth = pSSLv23_method();
#else
	FIXME("can't use SSL, not compiled in.\n");
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
        return FALSE;
#endif
    }
    return TRUE;
}

BOOL NETCON_connected(WININET_NETCONNECTION *connection)
{
    if (connection->socketFD == -1)
        return FALSE;
    else
        return TRUE;
}

#if 0
/* translate a unix error code into a winsock one */
static int sock_get_error( int err )
{
#if !defined(__MINGW32__) && !defined (_MSC_VER)
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
    default: errno=err; perror("sock_set_error"); return WSAEFAULT;
    }
#endif
    return err;
}
#endif

/******************************************************************************
 * NETCON_create
 * Basically calls 'socket()'
 */
BOOL NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol)
{
#ifdef SONAME_LIBSSL
    if (connection->useSSL)
        return FALSE;
#endif

    connection->socketFD = socket(domain, type, protocol);
    if (connection->socketFD == -1)
    {
        INTERNET_SetLastError(sock_get_error(errno));
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * NETCON_close
 * Basically calls 'close()' unless we should use SSL
 */
BOOL NETCON_close(WININET_NETCONNECTION *connection)
{
    int result;

    if (!NETCON_connected(connection)) return FALSE;

#ifdef SONAME_LIBSSL
    if (connection->useSSL)
    {
        pSSL_shutdown(connection->ssl_s);
        pSSL_free(connection->ssl_s);
        connection->ssl_s = NULL;

        connection->useSSL = FALSE;
    }
#endif

    result = closesocket(connection->socketFD);
    connection->socketFD = -1;

    if (result == -1)
    {
        INTERNET_SetLastError(sock_get_error(errno));
        return FALSE;
    }
    return TRUE;
}
#ifdef SONAME_LIBSSL
static BOOL check_hostname(X509 *cert, char *hostname)
{
    /* FIXME: implement */
    return TRUE;
}
#endif
/******************************************************************************
 * NETCON_secure_connect
 * Initiates a secure connection over an existing plaintext connection.
 */
BOOL NETCON_secure_connect(WININET_NETCONNECTION *connection, LPCWSTR hostname)
{
#ifdef SONAME_LIBSSL
    long verify_res;
    X509 *cert;
    int len;
    char *hostname_unix;

    /* can't connect if we are already connected */
    if (connection->useSSL)
    {
        ERR("already connected\n");
        return FALSE;
    }

    ctx = pSSL_CTX_new(meth);
    if (!pSSL_CTX_set_default_verify_paths(ctx))
    {
        ERR("SSL_CTX_set_default_verify_paths failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    connection->ssl_s = pSSL_new(ctx);
    if (!connection->ssl_s)
    {
        ERR("SSL_new failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        goto fail;
    }

    if (!pSSL_set_fd(connection->ssl_s, connection->socketFD))
    {
        ERR("SSL_set_fd failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
        goto fail;
    }

    if (pSSL_connect(connection->ssl_s) <= 0)
    {
        ERR("SSL_connect failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
        goto fail;
    }
    cert = pSSL_get_peer_certificate(connection->ssl_s);
    if (!cert)
    {
        ERR("no certificate for server %s\n", debugstr_w(hostname));
        /* FIXME: is this the best error? */
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_CA);
        goto fail;
    }
    verify_res = pSSL_get_verify_result(connection->ssl_s);
    if (verify_res != X509_V_OK)
    {
        ERR("couldn't verify the security of the connection, %ld\n", verify_res);
        /* FIXME: we should set an error and return, but we only warn at
         * the moment */
    }

    len = WideCharToMultiByte(CP_UNIXCP, 0, hostname, -1, NULL, 0, NULL, NULL);
    hostname_unix = HeapAlloc(GetProcessHeap(), 0, len);
    if (!hostname_unix)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        goto fail;
    }
    WideCharToMultiByte(CP_UNIXCP, 0, hostname, -1, hostname_unix, len, NULL, NULL);

    if (!check_hostname(cert, hostname_unix))
    {
        HeapFree(GetProcessHeap(), 0, hostname_unix);
        INTERNET_SetLastError(ERROR_INTERNET_SEC_CERT_CN_INVALID);
        goto fail;
    }

    HeapFree(GetProcessHeap(), 0, hostname_unix);
    connection->useSSL = TRUE;
    return TRUE;

fail:
    if (connection->ssl_s)
    {
        pSSL_shutdown(connection->ssl_s);
        pSSL_free(connection->ssl_s);
        connection->ssl_s = NULL;
    }
#endif
    return FALSE;
}

/******************************************************************************
 * NETCON_connect
 * Connects to the specified address.
 */
BOOL NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen)
{
    int result;

    if (!NETCON_connected(connection)) return FALSE;

    result = connect(connection->socketFD, serv_addr, addrlen);
    if (result == -1)
    {
        WARN("Unable to connect to host (%s)\n", strerror(errno));
        INTERNET_SetLastError(sock_get_error(errno));

        closesocket(connection->socketFD);
        connection->socketFD = -1;
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * NETCON_send
 * Basically calls 'send()' unless we should use SSL
 * number of chars send is put in *sent
 */
BOOL NETCON_send(WININET_NETCONNECTION *connection, const void *msg, size_t len, int flags,
		int *sent /* out */)
{
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
	*sent = send(connection->socketFD, msg, len, flags);
	if (*sent == -1)
	{
	    INTERNET_SetLastError(sock_get_error(errno));
	    return FALSE;
	}
        return TRUE;
    }
    else
    {
#ifdef SONAME_LIBSSL
	if (flags)
            FIXME("SSL_write doesn't support any flags (%08x)\n", flags);
	*sent = pSSL_write(connection->ssl_s, msg, len);
	if (*sent < 1 && len)
	    return FALSE;
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_recv
 * Basically calls 'recv()' unless we should use SSL
 * number of chars received is put in *recvd
 */
BOOL NETCON_recv(WININET_NETCONNECTION *connection, void *buf, size_t len, int flags,
		int *recvd /* out */)
{
    *recvd = 0;
    if (!NETCON_connected(connection)) return FALSE;
    if (!len)
        return TRUE;
    if (!connection->useSSL)
    {
	*recvd = recv(connection->socketFD, buf, len, flags);
	if (*recvd == -1)
	{
	    INTERNET_SetLastError(sock_get_error(errno));
	    return FALSE;
	}
        return TRUE;
    }
    else
    {
#ifdef SONAME_LIBSSL
	*recvd = pSSL_read(connection->ssl_s, buf, len);
	return *recvd > 0 || !len;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_query_data_available
 * Returns the number of bytes of peeked data plus the number of bytes of
 * queued, but unread data.
 */
BOOL NETCON_query_data_available(WININET_NETCONNECTION *connection, DWORD *available)
{
    *available = 0;
    if (!NETCON_connected(connection))
        return FALSE;

    if (!connection->useSSL)
    {
#ifdef FIONREAD
        int unread;
        int retval = ioctlsocket(connection->socketFD, FIONREAD, &unread);
        if (!retval)
        {
            TRACE("%d bytes of queued, but unread data\n", unread);
            *available += unread;
        }
#endif
    }
    else
    {
#ifdef SONAME_LIBSSL
        *available = pSSL_pending(connection->ssl_s);
#endif
    }
    return TRUE;
}

LPCVOID NETCON_GetCert(WININET_NETCONNECTION *connection)
{
#ifdef SONAME_LIBSSL
    X509* cert;
    unsigned char* buffer,*p;
    INT len;
    BOOL malloced = FALSE;
    LPCVOID r = NULL;

    if (!connection->useSSL)
        return NULL;

    cert = pSSL_get_peer_certificate(connection->ssl_s);
    p = NULL;
    len = pi2d_X509(cert,&p);
    /*
     * SSL 0.9.7 and above malloc the buffer if it is null. 
     * however earlier version do not and so we would need to alloc the buffer.
     *
     * see the i2d_X509 man page for more details.
     */
    if (!p)
    {
        buffer = HeapAlloc(GetProcessHeap(),0,len);
        p = buffer;
        len = pi2d_X509(cert,&p);
    }
    else
    {
        buffer = p;
        malloced = TRUE;
    }

    r = CertCreateCertificateContext(X509_ASN_ENCODING,buffer,len);

    if (malloced)
        free(buffer);
    else
        HeapFree(GetProcessHeap(),0,buffer);

    return r;
#else
    return NULL;
#endif
}

DWORD NETCON_set_timeout(WININET_NETCONNECTION *connection, BOOL send, int value)
{
    int result;
    struct timeval tv;

    /* FIXME: we should probably store the timeout in the connection to set
     * when we do connect */
    if (!NETCON_connected(connection))
        return ERROR_SUCCESS;

    /* value is in milliseconds, convert to struct timeval */
    tv.tv_sec = value / 1000;
    tv.tv_usec = (value % 1000) * 1000;

    result = setsockopt(connection->socketFD, SOL_SOCKET,
                        send ? SO_SNDTIMEO : SO_RCVTIMEO, (void*)&tv,
                        sizeof(tv));

    if (result == -1)
    {
        WARN("setsockopt failed (%s)\n", strerror(errno));
        return sock_get_error(errno);
    }

    return ERROR_SUCCESS;
}
