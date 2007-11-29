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

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
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

/* To avoid conflicts with the Unix socket headers. we only need it for
 * the error codes anyway. */
#define USE_WS_PREFIX
#include "winsock2.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */
#define sock_get_error(x) WSAGetLastError()
#undef FIONREAD


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
MAKE_FUNCPTR(SSL_get_verify_result);
MAKE_FUNCPTR(SSL_get_peer_certificate);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);
MAKE_FUNCPTR(SSL_CTX_set_default_verify_paths);
MAKE_FUNCPTR(i2d_X509);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_fp);
MAKE_FUNCPTR(ERR_get_error);
MAKE_FUNCPTR(ERR_error_string);
#undef MAKE_FUNCPTR

#endif

BOOL NETCON_init(WININET_NETCONNECTION *connection, BOOL useSSL)
{
    connection->useSSL = FALSE;
    connection->socketFD = -1;
    if (useSSL)
    {
#ifdef SONAME_LIBSSL
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
	DYNSSL(SSL_get_verify_result);
	DYNSSL(SSL_get_peer_certificate);
	DYNSSL(SSL_CTX_get_timeout);
	DYNSSL(SSL_CTX_set_timeout);
	DYNSSL(SSL_CTX_set_default_verify_paths);
	DYNSSL(i2d_X509);
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
#undef DYNCRYPTO

	pSSL_library_init();
	pSSL_load_error_strings();
	pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

	meth = pSSLv23_method();
        connection->peek_msg = NULL;
        connection->peek_msg_mem = NULL;
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

#ifndef __REACTOS__
/* translate a unix error code into a winsock one */
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
    default: errno=err; perror("sock_set_error"); return WSAEFAULT;
    }
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
        HeapFree(GetProcessHeap(),0,connection->peek_msg_mem);
        connection->peek_msg = NULL;
        connection->peek_msg_mem = NULL;
        connection->peek_len = 0;

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
	if (flags & ~(MSG_PEEK|MSG_WAITALL))
	    FIXME("SSL_read does not support the following flag: %08x\n", flags);

        /* this ugly hack is all for MSG_PEEK. eww gross */
	if (flags & MSG_PEEK && !connection->peek_msg)
	{
	    connection->peek_msg = connection->peek_msg_mem = HeapAlloc(GetProcessHeap(), 0, (sizeof(char) * len) + 1);
	}
	else if (flags & MSG_PEEK && connection->peek_msg)
	{
	    if (len < connection->peek_len)
		FIXME("buffer isn't big enough. Do the expect us to wrap?\n");
	    *recvd = min(len, connection->peek_len);
	    memcpy(buf, connection->peek_msg, *recvd);
            return TRUE;
	}
	else if (connection->peek_msg)
	{
	    *recvd = min(len, connection->peek_len);
	    memcpy(buf, connection->peek_msg, *recvd);
	    connection->peek_len -= *recvd;
	    connection->peek_msg += *recvd;
	    if (connection->peek_len == 0)
	    {
		HeapFree(GetProcessHeap(), 0, connection->peek_msg_mem);
		connection->peek_msg_mem = NULL;
                connection->peek_msg = NULL;
	    }
	    /* check if we got enough data from the peek buffer */
	    if (!(flags & MSG_WAITALL) || (*recvd == len))
	        return TRUE;
	    /* otherwise, fall through */
	}
	*recvd += pSSL_read(connection->ssl_s, (char*)buf + *recvd, len - *recvd);
	if (flags & MSG_PEEK) /* must copy stuff into buffer */
	{
            connection->peek_len = *recvd;
	    if (!*recvd)
	    {
		HeapFree(GetProcessHeap(), 0, connection->peek_msg_mem);
		connection->peek_msg_mem = NULL;
		connection->peek_msg = NULL;
	    }
	    else
		memcpy(connection->peek_msg, buf, *recvd);
	}
	if (*recvd < 1 && len)
            return FALSE;
        return TRUE;
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

#ifdef SONAME_LIBSSL
    if (connection->peek_msg) *available = connection->peek_len;
#endif

#ifdef FIONREAD
    if (!connection->useSSL)
    {
        int unread;
        int retval = ioctl(connection->socketFD, FIONREAD, &unread);
        if (!retval)
        {
            TRACE("%d bytes of queued, but unread data\n", unread);
            *available += unread;
        }
    }
#endif
    return TRUE;
}

/******************************************************************************
 * NETCON_getNextLine
 */
BOOL NETCON_getNextLine(WININET_NETCONNECTION *connection, LPSTR lpszBuffer, LPDWORD dwBuffer)
{

    TRACE("\n");

    if (!NETCON_connected(connection)) return FALSE;

    if (!connection->useSSL)
    {
	struct timeval tv;
	fd_set infd;
	BOOL bSuccess = FALSE;
	DWORD nRecv = 0;

	FD_ZERO(&infd);
	FD_SET(connection->socketFD, &infd);
	tv.tv_sec=RESPONSE_TIMEOUT;
	tv.tv_usec=0;

	while (nRecv < *dwBuffer)
	{
	    if (select(connection->socketFD+1,&infd,NULL,NULL,&tv) > 0)
	    {
		if (recv(connection->socketFD, &lpszBuffer[nRecv], 1, 0) <= 0)
		{
		    INTERNET_SetLastError(sock_get_error(errno));
		    goto lend;
		}

		if (lpszBuffer[nRecv] == '\n')
		{
		    bSuccess = TRUE;
		    break;
		}
		if (lpszBuffer[nRecv] != '\r')
		    nRecv++;
	    }
	    else
	    {
		INTERNET_SetLastError(ERROR_INTERNET_TIMEOUT);
		goto lend;
	    }
	}

    lend:             /* FIXME: don't use labels */
	if (bSuccess)
	{
	    lpszBuffer[nRecv++] = '\0';
	    *dwBuffer = nRecv;
	    TRACE(":%u %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
	else
	{
	    return FALSE;
	}
    }
    else
    {
#ifdef SONAME_LIBSSL
	long prev_timeout;
	DWORD nRecv = 0;
        BOOL success = TRUE;

        prev_timeout = pSSL_CTX_get_timeout(ctx);
	pSSL_CTX_set_timeout(ctx, RESPONSE_TIMEOUT);

	while (nRecv < *dwBuffer)
	{
	    int recv = 1;
	    if (!NETCON_recv(connection, &lpszBuffer[nRecv], 1, 0, &recv))
	    {
                INTERNET_SetLastError(ERROR_CONNECTION_ABORTED);
		success = FALSE;
	    }

	    if (lpszBuffer[nRecv] == '\n')
	    {
		success = TRUE;
                break;
	    }
	    if (lpszBuffer[nRecv] != '\r')
		nRecv++;
	}

        pSSL_CTX_set_timeout(ctx, prev_timeout);
	if (success)
	{
	    lpszBuffer[nRecv++] = '\0';
	    *dwBuffer = nRecv;
	    TRACE("_SSL:%u %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
        return FALSE;
#else
	return FALSE;
#endif
    }
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

BOOL NETCON_set_timeout(WININET_NETCONNECTION *connection, BOOL send, int value)
{
    int result;
    struct timeval tv;

    /* FIXME: we should probably store the timeout in the connection to set
     * when we do connect */
    if (!NETCON_connected(connection))
        return TRUE;

    /* value is in milliseconds, convert to struct timeval */
    tv.tv_sec = value / 1000;
    tv.tv_usec = (value % 1000) * 1000;

    result = setsockopt(connection->socketFD, SOL_SOCKET,
                        send ? SO_SNDTIMEO : SO_RCVTIMEO, &tv,
                        sizeof(tv));

    if (result == -1)
    {
        WARN("setsockopt failed (%s)\n", strerror(errno));
        INTERNET_SetLastError(sock_get_error(errno));
        return FALSE;
    }

    return TRUE;
}
