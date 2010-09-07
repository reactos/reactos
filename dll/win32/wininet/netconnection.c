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

#define NONAMELESSUNION

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


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME!!!!!!
 *    This should use winsock - To use winsock the functions will have to change a bit
 *        as they are designed for unix sockets.
 *    SSL stuff should use crypt32.dll
 */

#ifdef SONAME_LIBSSL

#include <openssl/err.h>

static CRITICAL_SECTION init_ssl_cs;
static CRITICAL_SECTION_DEBUG init_ssl_cs_debug =
{
    0, 0, &init_ssl_cs,
    { &init_ssl_cs_debug.ProcessLocksList,
      &init_ssl_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_ssl_cs") }
};
static CRITICAL_SECTION init_ssl_cs = { &init_ssl_cs_debug, -1, 0, 0, 0, 0 };

static void *OpenSSL_ssl_handle;
static void *OpenSSL_crypto_handle;

static SSL_METHOD *meth;
static SSL_CTX *ctx;
static int hostname_idx;
static int error_idx;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL functions that we use */
MAKE_FUNCPTR(SSL_library_init);
MAKE_FUNCPTR(SSL_load_error_strings);
MAKE_FUNCPTR(SSLv23_method);
MAKE_FUNCPTR(SSL_CTX_free);
MAKE_FUNCPTR(SSL_CTX_new);
MAKE_FUNCPTR(SSL_new);
MAKE_FUNCPTR(SSL_free);
MAKE_FUNCPTR(SSL_set_fd);
MAKE_FUNCPTR(SSL_connect);
MAKE_FUNCPTR(SSL_shutdown);
MAKE_FUNCPTR(SSL_write);
MAKE_FUNCPTR(SSL_read);
MAKE_FUNCPTR(SSL_pending);
MAKE_FUNCPTR(SSL_get_error);
MAKE_FUNCPTR(SSL_get_ex_new_index);
MAKE_FUNCPTR(SSL_get_ex_data);
MAKE_FUNCPTR(SSL_set_ex_data);
MAKE_FUNCPTR(SSL_get_ex_data_X509_STORE_CTX_idx);
MAKE_FUNCPTR(SSL_get_verify_result);
MAKE_FUNCPTR(SSL_get_peer_certificate);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);
MAKE_FUNCPTR(SSL_CTX_set_default_verify_paths);
MAKE_FUNCPTR(SSL_CTX_set_verify);
MAKE_FUNCPTR(X509_STORE_CTX_get_ex_data);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_fp);
MAKE_FUNCPTR(CRYPTO_num_locks);
MAKE_FUNCPTR(CRYPTO_set_id_callback);
MAKE_FUNCPTR(CRYPTO_set_locking_callback);
MAKE_FUNCPTR(ERR_free_strings);
MAKE_FUNCPTR(ERR_get_error);
MAKE_FUNCPTR(ERR_error_string);
MAKE_FUNCPTR(i2d_X509);
MAKE_FUNCPTR(sk_num);
MAKE_FUNCPTR(sk_value);
#undef MAKE_FUNCPTR

static CRITICAL_SECTION *ssl_locks;
static unsigned int num_ssl_locks;

static unsigned long ssl_thread_id(void)
{
    return GetCurrentThreadId();
}

static void ssl_lock_callback(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        EnterCriticalSection(&ssl_locks[type]);
    else
        LeaveCriticalSection(&ssl_locks[type]);
}

static PCCERT_CONTEXT X509_to_cert_context(X509 *cert)
{
    unsigned char* buffer,*p;
    INT len;
    BOOL malloced = FALSE;
    PCCERT_CONTEXT ret;

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

    ret = CertCreateCertificateContext(X509_ASN_ENCODING,buffer,len);

    if (malloced)
        free(buffer);
    else
        HeapFree(GetProcessHeap(),0,buffer);

    return ret;
}

static DWORD netconn_verify_cert(PCCERT_CONTEXT cert, HCERTSTORE store,
    WCHAR *server)
{
    BOOL ret;
    CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
    PCCERT_CHAIN_CONTEXT chain;
    char oid_server_auth[] = szOID_PKIX_KP_SERVER_AUTH;
    char *server_auth[] = { oid_server_auth };
    DWORD err = ERROR_SUCCESS;

    TRACE("verifying %s\n", debugstr_w(server));
    chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = server_auth;
    if ((ret = CertGetCertificateChain(NULL, cert, NULL, store, &chainPara, 0,
        NULL, &chain)))
    {
        if (chain->TrustStatus.dwErrorStatus)
        {
            if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_NOT_TIME_VALID)
                err = ERROR_INTERNET_SEC_CERT_DATE_INVALID;
            else if (chain->TrustStatus.dwErrorStatus &
                     CERT_TRUST_IS_UNTRUSTED_ROOT)
                err = ERROR_INTERNET_INVALID_CA;
            else if ((chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_IS_OFFLINE_REVOCATION) ||
                     (chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_REVOCATION_STATUS_UNKNOWN))
                err = ERROR_INTERNET_SEC_CERT_NO_REV;
            else if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_REVOKED)
                err = ERROR_INTERNET_SEC_CERT_REVOKED;
            else if (chain->TrustStatus.dwErrorStatus &
                CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
                err = ERROR_INTERNET_SEC_INVALID_CERT;
            else
                err = ERROR_INTERNET_SEC_INVALID_CERT;
        }
        else
        {
            CERT_CHAIN_POLICY_PARA policyPara;
            SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslExtraPolicyPara;
            CERT_CHAIN_POLICY_STATUS policyStatus;

            sslExtraPolicyPara.u.cbSize = sizeof(sslExtraPolicyPara);
            sslExtraPolicyPara.dwAuthType = AUTHTYPE_SERVER;
            sslExtraPolicyPara.pwszServerName = server;
            policyPara.cbSize = sizeof(policyPara);
            policyPara.dwFlags = 0;
            policyPara.pvExtraPolicyPara = &sslExtraPolicyPara;
            ret = CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                chain, &policyPara, &policyStatus);
            /* Any error in the policy status indicates that the
             * policy couldn't be verified.
             */
            if (ret && policyStatus.dwError)
            {
                if (policyStatus.dwError == CERT_E_CN_NO_MATCH)
                    err = ERROR_INTERNET_SEC_CERT_CN_INVALID;
                else
                    err = ERROR_INTERNET_SEC_INVALID_CERT;
            }
        }
        CertFreeCertificateChain(chain);
    }
    TRACE("returning %08x\n", err);
    return err;
}

static int netconn_secure_verify(int preverify_ok, X509_STORE_CTX *ctx)
{
    SSL *ssl;
    WCHAR *server;
    BOOL ret = FALSE;

    ssl = pX509_STORE_CTX_get_ex_data(ctx,
        pSSL_get_ex_data_X509_STORE_CTX_idx());
    server = pSSL_get_ex_data(ssl, hostname_idx);
    if (preverify_ok)
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
            CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            X509 *cert;
            int i;
            PCCERT_CONTEXT endCert = NULL;

            ret = TRUE;
            for (i = 0; ret && i < psk_num((struct stack_st *)ctx->chain); i++)
            {
                PCCERT_CONTEXT context;

                cert = (X509 *)psk_value((struct stack_st *)ctx->chain, i);
                if ((context = X509_to_cert_context(cert)))
                {
                    if (i == 0)
                        ret = CertAddCertificateContextToStore(store, context,
                            CERT_STORE_ADD_ALWAYS, &endCert);
                    else
                        ret = CertAddCertificateContextToStore(store, context,
                            CERT_STORE_ADD_ALWAYS, NULL);
                    CertFreeCertificateContext(context);
                }
            }
            if (!endCert) ret = FALSE;
            if (ret)
            {
                DWORD_PTR err = netconn_verify_cert(endCert, store, server);

                if (err)
                {
                    pSSL_set_ex_data(ssl, error_idx, (void *)err);
                    ret = FALSE;
                }
            }
            CertFreeCertificateContext(endCert);
            CertCloseStore(store, 0);
        }
    } else
        pSSL_set_ex_data(ssl, error_idx, (void *)ERROR_INTERNET_SEC_CERT_ERRORS);

    return ret;
}

#endif

DWORD NETCON_init(WININET_NETCONNECTION *connection, BOOL useSSL)
{
    connection->useSSL = FALSE;
    connection->socketFD = -1;
    if (useSSL)
    {
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
        int i;

        TRACE("using SSL connection\n");
        EnterCriticalSection(&init_ssl_cs);
	if (OpenSSL_ssl_handle) /* already initialized everything */
        {
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_SUCCESS;
        }
	OpenSSL_ssl_handle = wine_dlopen(SONAME_LIBSSL, RTLD_NOW, NULL, 0);
	if (!OpenSSL_ssl_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBSSL);
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
	}
	OpenSSL_crypto_handle = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
	if (!OpenSSL_crypto_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBCRYPTO);
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
	}

        /* mmm nice ugly macroness */
#define DYNSSL(x) \
    p##x = wine_dlsym(OpenSSL_ssl_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        LeaveCriticalSection(&init_ssl_cs); \
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR; \
    }

	DYNSSL(SSL_library_init);
	DYNSSL(SSL_load_error_strings);
	DYNSSL(SSLv23_method);
	DYNSSL(SSL_CTX_free);
	DYNSSL(SSL_CTX_new);
	DYNSSL(SSL_new);
	DYNSSL(SSL_free);
	DYNSSL(SSL_set_fd);
	DYNSSL(SSL_connect);
	DYNSSL(SSL_shutdown);
	DYNSSL(SSL_write);
	DYNSSL(SSL_read);
	DYNSSL(SSL_pending);
        DYNSSL(SSL_get_error);
	DYNSSL(SSL_get_ex_new_index);
	DYNSSL(SSL_get_ex_data);
	DYNSSL(SSL_set_ex_data);
	DYNSSL(SSL_get_ex_data_X509_STORE_CTX_idx);
	DYNSSL(SSL_get_verify_result);
	DYNSSL(SSL_get_peer_certificate);
	DYNSSL(SSL_CTX_get_timeout);
	DYNSSL(SSL_CTX_set_timeout);
	DYNSSL(SSL_CTX_set_default_verify_paths);
	DYNSSL(SSL_CTX_set_verify);
	DYNSSL(X509_STORE_CTX_get_ex_data);
#undef DYNSSL

#define DYNCRYPTO(x) \
    p##x = wine_dlsym(OpenSSL_crypto_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        LeaveCriticalSection(&init_ssl_cs); \
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR; \
    }
	DYNCRYPTO(BIO_new_fp);
	DYNCRYPTO(CRYPTO_num_locks);
	DYNCRYPTO(CRYPTO_set_id_callback);
	DYNCRYPTO(CRYPTO_set_locking_callback);
	DYNCRYPTO(ERR_free_strings);
	DYNCRYPTO(ERR_get_error);
	DYNCRYPTO(ERR_error_string);
	DYNCRYPTO(i2d_X509);
	DYNCRYPTO(sk_num);
	DYNCRYPTO(sk_value);
#undef DYNCRYPTO

	pSSL_library_init();
	pSSL_load_error_strings();
	pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

	meth = pSSLv23_method();
        ctx = pSSL_CTX_new(meth);
        if (!pSSL_CTX_set_default_verify_paths(ctx))
        {
            ERR("SSL_CTX_set_default_verify_paths failed: %s\n",
                pERR_error_string(pERR_get_error(), 0));
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_OUTOFMEMORY;
        }
        hostname_idx = pSSL_get_ex_new_index(0, (void *)"hostname index",
                NULL, NULL, NULL);
        if (hostname_idx == -1)
        {
            ERR("SSL_get_ex_new_index failed; %s\n",
                pERR_error_string(pERR_get_error(), 0));
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_OUTOFMEMORY;
        }
        error_idx = pSSL_get_ex_new_index(0, (void *)"error index",
                NULL, NULL, NULL);
        if (error_idx == -1)
        {
            ERR("SSL_get_ex_new_index failed; %s\n",
                pERR_error_string(pERR_get_error(), 0));
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_OUTOFMEMORY;
        }
        pSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, netconn_secure_verify);

        pCRYPTO_set_id_callback(ssl_thread_id);
        num_ssl_locks = pCRYPTO_num_locks();
        ssl_locks = HeapAlloc(GetProcessHeap(), 0, num_ssl_locks * sizeof(CRITICAL_SECTION));
        if (!ssl_locks)
        {
            LeaveCriticalSection(&init_ssl_cs);
            return ERROR_OUTOFMEMORY;
        }
        for (i = 0; i < num_ssl_locks; i++)
            InitializeCriticalSection(&ssl_locks[i]);
        pCRYPTO_set_locking_callback(ssl_lock_callback);
        LeaveCriticalSection(&init_ssl_cs);
#else
	FIXME("can't use SSL, not compiled in.\n");
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
#endif
    }
    return ERROR_SUCCESS;
}

void NETCON_unload(void)
{
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    if (OpenSSL_crypto_handle)
    {
        pERR_free_strings();
        wine_dlclose(OpenSSL_crypto_handle, NULL, 0);
    }
    if (OpenSSL_ssl_handle)
    {
        if (ctx)
            pSSL_CTX_free(ctx);
        wine_dlclose(OpenSSL_ssl_handle, NULL, 0);
    }
    if (ssl_locks)
    {
        int i;
        for (i = 0; i < num_ssl_locks; i++) DeleteCriticalSection(&ssl_locks[i]);
        HeapFree(GetProcessHeap(), 0, ssl_locks);
    }
#endif
}

BOOL NETCON_connected(WININET_NETCONNECTION *connection)
{
    if (connection->socketFD == -1)
        return FALSE;
    else
        return TRUE;
}

/* translate a unix error code into a winsock one */
int sock_get_error( int err )
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

/******************************************************************************
 * NETCON_create
 * Basically calls 'socket()'
 */
DWORD NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol)
{
#ifdef SONAME_LIBSSL
    if (connection->useSSL)
        return ERROR_NOT_SUPPORTED;
#endif

    connection->socketFD = socket(domain, type, protocol);
    if (connection->socketFD == -1)
        return sock_get_error(errno);

    return ERROR_SUCCESS;
}

/******************************************************************************
 * NETCON_close
 * Basically calls 'close()' unless we should use SSL
 */
DWORD NETCON_close(WININET_NETCONNECTION *connection)
{
    int result;

    if (!NETCON_connected(connection)) return ERROR_SUCCESS;

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
        return sock_get_error(errno);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * NETCON_secure_connect
 * Initiates a secure connection over an existing plaintext connection.
 */
DWORD NETCON_secure_connect(WININET_NETCONNECTION *connection, LPWSTR hostname)
{
    DWORD res = ERROR_NOT_SUPPORTED;
#ifdef SONAME_LIBSSL
    long verify_res;
    X509 *cert;

    /* can't connect if we are already connected */
    if (connection->useSSL)
    {
        ERR("already connected\n");
        return ERROR_INTERNET_CANNOT_CONNECT;
    }

    connection->ssl_s = pSSL_new(ctx);
    if (!connection->ssl_s)
    {
        ERR("SSL_new failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        res = ERROR_OUTOFMEMORY;
        goto fail;
    }

    if (!pSSL_set_fd(connection->ssl_s, connection->socketFD))
    {
        ERR("SSL_set_fd failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        goto fail;
    }

    if (pSSL_connect(connection->ssl_s) <= 0)
    {
        res = (DWORD_PTR)pSSL_get_ex_data(connection->ssl_s, error_idx);
        if (!res)
            res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        ERR("SSL_connect failed: %d\n", res);
        goto fail;
    }
    pSSL_set_ex_data(connection->ssl_s, hostname_idx, hostname);
    cert = pSSL_get_peer_certificate(connection->ssl_s);
    if (!cert)
    {
        ERR("no certificate for server %s\n", debugstr_w(hostname));
        /* FIXME: is this the best error? */
        res = ERROR_INTERNET_INVALID_CA;
        goto fail;
    }
    verify_res = pSSL_get_verify_result(connection->ssl_s);
    if (verify_res != X509_V_OK)
    {
        ERR("couldn't verify the security of the connection, %ld\n", verify_res);
        /* FIXME: we should set an error and return, but we only warn at
         * the moment */
    }

    connection->useSSL = TRUE;
    return ERROR_SUCCESS;

fail:
    if (connection->ssl_s)
    {
        pSSL_shutdown(connection->ssl_s);
        pSSL_free(connection->ssl_s);
        connection->ssl_s = NULL;
    }
#endif
    return res;
}

/******************************************************************************
 * NETCON_connect
 * Connects to the specified address.
 */
DWORD NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen)
{
    int result;

    result = connect(connection->socketFD, serv_addr, addrlen);
    if (result == -1)
    {
        WARN("Unable to connect to host (%s)\n", strerror(errno));

        closesocket(connection->socketFD);
        connection->socketFD = -1;
        return sock_get_error(errno);
    }

    return ERROR_SUCCESS;
}

/******************************************************************************
 * NETCON_send
 * Basically calls 'send()' unless we should use SSL
 * number of chars send is put in *sent
 */
DWORD NETCON_send(WININET_NETCONNECTION *connection, const void *msg, size_t len, int flags,
		int *sent /* out */)
{
    if (!NETCON_connected(connection)) return ERROR_INTERNET_CONNECTION_ABORTED;
    if (!connection->useSSL)
    {
	*sent = send(connection->socketFD, msg, len, flags);
	if (*sent == -1)
	    return sock_get_error(errno);
        return ERROR_SUCCESS;
    }
    else
    {
#ifdef SONAME_LIBSSL
	if (flags)
            FIXME("SSL_write doesn't support any flags (%08x)\n", flags);
	*sent = pSSL_write(connection->ssl_s, msg, len);
	if (*sent < 1 && len)
	    return ERROR_INTERNET_CONNECTION_ABORTED;
        return ERROR_SUCCESS;
#else
	return ERROR_NOT_SUPPORTED;
#endif
    }
}

/******************************************************************************
 * NETCON_recv
 * Basically calls 'recv()' unless we should use SSL
 * number of chars received is put in *recvd
 */
DWORD NETCON_recv(WININET_NETCONNECTION *connection, void *buf, size_t len, int flags,
		int *recvd /* out */)
{
    *recvd = 0;
    if (!NETCON_connected(connection)) return ERROR_INTERNET_CONNECTION_ABORTED;
    if (!len)
        return ERROR_SUCCESS;
    if (!connection->useSSL)
    {
	*recvd = recv(connection->socketFD, buf, len, flags);
	return *recvd == -1 ? sock_get_error(errno) :  ERROR_SUCCESS;
    }
    else
    {
#ifdef SONAME_LIBSSL
	*recvd = pSSL_read(connection->ssl_s, buf, len);

        /* Check if EOF was received */
        if(!*recvd && (pSSL_get_error(connection->ssl_s, *recvd)==SSL_ERROR_ZERO_RETURN
                    || pSSL_get_error(connection->ssl_s, *recvd)==SSL_ERROR_SYSCALL))
            return ERROR_SUCCESS;

        return *recvd > 0 ? ERROR_SUCCESS : ERROR_INTERNET_CONNECTION_ABORTED;
#else
	return ERROR_NOT_SUPPORTED;
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
    LPCVOID r = NULL;

    if (!connection->useSSL)
        return NULL;

    cert = pSSL_get_peer_certificate(connection->ssl_s);
    r = X509_to_cert_context(cert);
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
