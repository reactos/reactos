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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME!!!!!!
 *    This should use winsock - To use winsock the functions will have to change a bit
 *        as they are designed for unix sockets.
 *    SSL stuff should use crypt32.dll
 */

#ifdef HAVE_OPENSSL_SSL_H

#ifndef SONAME_LIBSSL
#define SONAME_LIBSSL "libssl.so"
#endif
#ifndef SONAME_LIBCRYPTO
#define SONAME_LIBCRYPTO "libcrypto.so"
#endif

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
MAKE_FUNCPTR(SSL_set_bio);
MAKE_FUNCPTR(SSL_connect);
MAKE_FUNCPTR(SSL_write);
MAKE_FUNCPTR(SSL_read);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_socket);
MAKE_FUNCPTR(BIO_new_fp);
#undef MAKE_FUNCPTR

#endif

void NETCON_init(WININET_NETCONNECTION *connection, BOOL useSSL)
{
    connection->useSSL = useSSL;
    connection->socketFD = -1;
    if (connection->useSSL)
    {
#ifdef HAVE_OPENSSL_SSL_H
        TRACE("using SSL connection\n");
	connection->ssl_sock = -1;
	if (OpenSSL_ssl_handle) /* already initilzed everything */
            return;
	OpenSSL_ssl_handle = wine_dlopen(SONAME_LIBSSL, RTLD_NOW, NULL, 0);
	if (!OpenSSL_ssl_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBSSL);
            connection->useSSL = FALSE;
            return;
	}
	OpenSSL_crypto_handle = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
	if (!OpenSSL_crypto_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBCRYPTO);
            connection->useSSL = FALSE;
            return;
	}

        /* mmm nice ugly macroness */
#define DYNSSL(x) \
    p##x = wine_dlsym(OpenSSL_ssl_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        connection->useSSL = FALSE; \
        return; \
    }

	DYNSSL(SSL_library_init);
	DYNSSL(SSL_load_error_strings);
	DYNSSL(SSLv23_method);
	DYNSSL(SSL_CTX_new);
	DYNSSL(SSL_new);
	DYNSSL(SSL_set_bio);
	DYNSSL(SSL_connect);
	DYNSSL(SSL_write);
	DYNSSL(SSL_read);
	DYNSSL(SSL_CTX_get_timeout);
        DYNSSL(SSL_CTX_set_timeout);
#undef DYNSSL

#define DYNCRYPTO(x) \
    p##x = wine_dlsym(OpenSSL_crypto_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        connection->useSSL = FALSE; \
        return; \
    }
	DYNCRYPTO(BIO_new_fp);
	DYNCRYPTO(BIO_new_socket);
#undef DYNCRYPTO

	pSSL_library_init();
	pSSL_load_error_strings();
	pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

	meth = pSSLv23_method();
	/* FIXME: SECURITY PROBLEM! WE ARN'T VERIFYING THE HOSTS CERTIFICATES OR ANYTHING */
#else
	FIXME("can't use SSL, not compiled in.\n");
        connection->useSSL = FALSE;
#endif
    }
}

BOOL NETCON_connected(WININET_NETCONNECTION *connection)
{
    if (!connection->useSSL)
    {
	if (connection->socketFD == -1)
	    return FALSE;
	return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
	if (connection->ssl_sock == -1)
	    return FALSE;
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_create
 * Basically calls 'socket()' unless useSSL is supplised,
 *  in which case we do other things.
 */
BOOL NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol)
{
    if (!connection->useSSL)
    {
	connection->socketFD = socket(domain, type, protocol);
	if (connection->socketFD == -1)
	    return FALSE;
	return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
        connection->ssl_sock = socket(domain, type, protocol);
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_close
 * Basically calls 'close()' unless we should use SSL
 */
BOOL NETCON_close(WININET_NETCONNECTION *connection)
{
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
        int result;
	result = closesocket(connection->socketFD);
        connection->socketFD = -1;
	if (result == -1)
	    return FALSE;
        return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
	closesocket(connection->ssl_sock);
	connection->ssl_sock = -1;
	/* FIXME should we call SSL_shutdown here?? Probably on whatever is the
	 * opposite of NETCON_init.... */
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_connect
 * Basically calls 'connect()' unless we should use SSL
 */
BOOL NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen)
{
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
	int result;
	result = connect(connection->socketFD, serv_addr, addrlen);
	if (result == -1)
        {
            closesocket(connection->socketFD);
            connection->socketFD = -1;
	    return FALSE;
        }
        return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
        BIO *sbio;

        ctx = pSSL_CTX_new(meth);
	connection->ssl_s = pSSL_new(ctx);

	if (connect(connection->ssl_sock, serv_addr, addrlen) == -1)
	    return FALSE;

	sbio = pBIO_new_socket(connection->ssl_sock, BIO_NOCLOSE);
        pSSL_set_bio(connection->ssl_s, sbio, sbio);
	if (pSSL_connect(connection->ssl_s) <= 0)
	{
            ERR("ssl couldn't connect\n");
	    return FALSE;
	}
	return TRUE;
#else
	return FALSE;
#endif
    }
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
	    return FALSE;
        return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
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
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
	*recvd = recv(connection->socketFD, buf, len, flags);
	if (*recvd == -1)
	    return FALSE;
        return TRUE;
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
	static char *peek_msg = NULL;
	static char *peek_msg_mem = NULL;

	if (flags & (~MSG_PEEK))
	    FIXME("SSL_read does not support the following flag: %08x\n", flags);

        /* this ugly hack is all for MSG_PEEK. eww gross */
	if (flags & MSG_PEEK && !peek_msg)
	{
	    peek_msg = peek_msg_mem = HeapAlloc(GetProcessHeap(), 0, (sizeof(char) * len) + 1);
	}
	else if (flags & MSG_PEEK && peek_msg)
	{
	    size_t peek_msg_len = strlen(peek_msg);
	    if (len < peek_msg_len)
		FIXME("buffer isn't big enough. Do the expect us to wrap?\n");
	    memcpy(buf, peek_msg, min(len,peek_msg_len+1));
	    *recvd = min(len, peek_msg_len);
            return TRUE;
	}
	else if (peek_msg)
	{
	    size_t peek_msg_len = strlen(peek_msg);
	    memcpy(buf, peek_msg, min(len,peek_msg_len+1));
	    peek_msg += *recvd = min(len, peek_msg_len);
	    if (*peek_msg == '\0' || *(peek_msg - 1) == '\0')
	    {
		HeapFree(GetProcessHeap(), 0, peek_msg_mem);
		peek_msg_mem = NULL;
                peek_msg = NULL;
	    }
            return TRUE;
	}
	*recvd = pSSL_read(connection->ssl_s, buf, len);
	if (flags & MSG_PEEK) /* must copy stuff into buffer */
	{
	    if (!*recvd)
	    {
		HeapFree(GetProcessHeap(), 0, peek_msg_mem);
		peek_msg_mem = NULL;
                peek_msg = NULL;
	    }
	    else
	    {
		memcpy(peek_msg, buf, *recvd);
		peek_msg[*recvd] = '\0';
	    }
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
		    INTERNET_SetLastError(ERROR_CONNECTION_ABORTED); /* fixme: right error? */
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
	    TRACE(":%lu %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
	else
	{
	    return FALSE;
	}
    }
    else
    {
#ifdef HAVE_OPENSSL_SSL_H
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
	    TRACE("_SSL:%lu %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
        return FALSE;
#else
	return FALSE;
#endif
    }
}
