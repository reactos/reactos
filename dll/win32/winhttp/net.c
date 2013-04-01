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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>
//#include "wine/port.h"

//#include <stdarg.h>
//#include <stdio.h>
//#include <errno.h>

//#include <sys/types.h>
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
# include <openssl/opensslv.h>
#undef FAR
#undef DSA
#endif

#define NONAMELESSUNION

#include <wine/debug.h>
#include <wine/library.h>

//#include "windef.h"
//#include "winbase.h"
#include <winhttp.h>
//#include "wincrypt.h"

#include "winhttp_private.h"

/* to avoid conflicts with the Unix socket headers */
#define USE_WS_PREFIX
//#include "winsock2.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

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

static CRITICAL_SECTION init_ssl_cs;
static CRITICAL_SECTION_DEBUG init_ssl_cs_debug =
{
    0, 0, &init_ssl_cs,
    { &init_ssl_cs_debug.ProcessLocksList,
      &init_ssl_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_ssl_cs") }
};
static CRITICAL_SECTION init_ssl_cs = { &init_ssl_cs_debug, -1, 0, 0, 0, 0 };

static void *libssl_handle;
static void *libcrypto_handle;

#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER > 0x10000000)
static const SSL_METHOD *method;
#else
static SSL_METHOD *method;
#endif
static SSL_CTX *ctx;
static int hostname_idx;
static int error_idx;
static int conn_idx;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

MAKE_FUNCPTR( SSL_library_init );
MAKE_FUNCPTR( SSL_load_error_strings );
MAKE_FUNCPTR( SSLv23_method );
MAKE_FUNCPTR( SSL_CTX_free );
MAKE_FUNCPTR( SSL_CTX_new );
MAKE_FUNCPTR( SSL_new );
MAKE_FUNCPTR( SSL_free );
MAKE_FUNCPTR( SSL_set_fd );
MAKE_FUNCPTR( SSL_connect );
MAKE_FUNCPTR( SSL_shutdown );
MAKE_FUNCPTR( SSL_write );
MAKE_FUNCPTR( SSL_read );
MAKE_FUNCPTR( SSL_pending );
MAKE_FUNCPTR( SSL_get_error );
MAKE_FUNCPTR( SSL_get_ex_new_index );
MAKE_FUNCPTR( SSL_get_ex_data );
MAKE_FUNCPTR( SSL_set_ex_data );
MAKE_FUNCPTR( SSL_get_ex_data_X509_STORE_CTX_idx );
MAKE_FUNCPTR( SSL_get_peer_certificate );
MAKE_FUNCPTR( SSL_CTX_set_default_verify_paths );
MAKE_FUNCPTR( SSL_CTX_set_verify );
MAKE_FUNCPTR( SSL_get_current_cipher );
MAKE_FUNCPTR( SSL_CIPHER_get_bits );

MAKE_FUNCPTR( CRYPTO_num_locks );
MAKE_FUNCPTR( CRYPTO_set_id_callback );
MAKE_FUNCPTR( CRYPTO_set_locking_callback );
MAKE_FUNCPTR( ERR_free_strings );
MAKE_FUNCPTR( ERR_get_error );
MAKE_FUNCPTR( ERR_error_string );
MAKE_FUNCPTR( X509_STORE_CTX_get_ex_data );
MAKE_FUNCPTR( X509_STORE_CTX_get_chain );
MAKE_FUNCPTR( i2d_X509 );
MAKE_FUNCPTR( sk_value );
MAKE_FUNCPTR( sk_num );
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
        EnterCriticalSection( &ssl_locks[type] );
    else
        LeaveCriticalSection( &ssl_locks[type] );
}

#endif

/* translate a unix error code into a winsock error code */
#ifndef __REACTOS__
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
    default: errno = err; perror( "sock_set_error" ); return WSAEFAULT;
    }
#endif
    return err;
}
#else
#define sock_get_error(x) WSAGetLastError()

static inline int unix_ioctl(int filedes, long request, void *arg)
{
    return ioctlsocket(filedes, request, arg);
}
#define ioctlsocket unix_ioctl
#endif

#ifdef SONAME_LIBSSL
static PCCERT_CONTEXT X509_to_cert_context(X509 *cert)
{
    unsigned char *buffer, *p;
    int len;
    BOOL malloc = FALSE;
    PCCERT_CONTEXT ret;

    p = NULL;
    if ((len = pi2d_X509( cert, &p )) < 0) return NULL;
    /*
     * SSL 0.9.7 and above malloc the buffer if it is null.
     * however earlier version do not and so we would need to alloc the buffer.
     *
     * see the i2d_X509 man page for more details.
     */
    if (!p)
    {
        if (!(buffer = heap_alloc( len ))) return NULL;
        p = buffer;
        len = pi2d_X509( cert, &p );
    }
    else
    {
        buffer = p;
        malloc = TRUE;
    }

    ret = CertCreateCertificateContext( X509_ASN_ENCODING, buffer, len );

    if (malloc) free( buffer );
    else heap_free( buffer );

    return ret;
}

static DWORD netconn_verify_cert( PCCERT_CONTEXT cert, HCERTSTORE store,
                                  WCHAR *server, DWORD security_flags )
{
    BOOL ret;
    CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
    PCCERT_CHAIN_CONTEXT chain;
    char oid_server_auth[] = szOID_PKIX_KP_SERVER_AUTH;
    char *server_auth[] = { oid_server_auth };
    DWORD err = ERROR_SUCCESS;

    TRACE("verifying %s\n", debugstr_w( server ));
    chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = server_auth;
    if ((ret = CertGetCertificateChain( NULL, cert, NULL, store, &chainPara,
                                        CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
                                        NULL, &chain )))
    {
        if (chain->TrustStatus.dwErrorStatus)
        {
            static const DWORD supportedErrors =
                CERT_TRUST_IS_NOT_TIME_VALID |
                CERT_TRUST_IS_UNTRUSTED_ROOT |
                CERT_TRUST_IS_NOT_VALID_FOR_USAGE;

            if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_NOT_TIME_VALID)
            {
                if (!(security_flags & SECURITY_FLAG_IGNORE_CERT_DATE_INVALID))
                    err = ERROR_WINHTTP_SECURE_CERT_DATE_INVALID;
            }
            else if (chain->TrustStatus.dwErrorStatus &
                     CERT_TRUST_IS_UNTRUSTED_ROOT)
            {
                if (!(security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA))
                    err = ERROR_WINHTTP_SECURE_INVALID_CA;
            }
            else if ((chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_IS_OFFLINE_REVOCATION) ||
                     (chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_REVOCATION_STATUS_UNKNOWN))
                err = ERROR_WINHTTP_SECURE_CERT_REV_FAILED;
            else if (chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_REVOKED)
                err = ERROR_WINHTTP_SECURE_CERT_REVOKED;
            else if (chain->TrustStatus.dwErrorStatus &
                CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
            {
                if (!(security_flags & SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE))
                    err = ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE;
            }
            else if (chain->TrustStatus.dwErrorStatus & ~supportedErrors)
                err = ERROR_WINHTTP_SECURE_INVALID_CERT;
        }
        if (!err)
        {
            CERT_CHAIN_POLICY_PARA policyPara;
            SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslExtraPolicyPara;
            CERT_CHAIN_POLICY_STATUS policyStatus;
            CERT_CHAIN_CONTEXT chainCopy;

            /* Clear chain->TrustStatus.dwErrorStatus so
             * CertVerifyCertificateChainPolicy will verify additional checks
             * rather than stopping with an existing, ignored error.
             */
            memcpy(&chainCopy, chain, sizeof(chainCopy));
            chainCopy.TrustStatus.dwErrorStatus = 0;
            sslExtraPolicyPara.u.cbSize = sizeof(sslExtraPolicyPara);
            sslExtraPolicyPara.dwAuthType = AUTHTYPE_SERVER;
            sslExtraPolicyPara.pwszServerName = server;
            sslExtraPolicyPara.fdwChecks = security_flags;
            policyPara.cbSize = sizeof(policyPara);
            policyPara.dwFlags = 0;
            policyPara.pvExtraPolicyPara = &sslExtraPolicyPara;
            ret = CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_SSL,
                                                    &chainCopy, &policyPara,
                                                    &policyStatus );
            /* Any error in the policy status indicates that the
             * policy couldn't be verified.
             */
            if (ret && policyStatus.dwError)
            {
                if (policyStatus.dwError == CERT_E_CN_NO_MATCH)
                    err = ERROR_WINHTTP_SECURE_CERT_CN_INVALID;
                else
                    err = ERROR_WINHTTP_SECURE_INVALID_CERT;
            }
        }
        CertFreeCertificateChain( chain );
    }
    else
        err = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
    TRACE("returning %08x\n", err);
    return err;
}

static int netconn_secure_verify( int preverify_ok, X509_STORE_CTX *ctx )
{
    SSL *ssl;
    WCHAR *server;
    BOOL ret = FALSE;
    netconn_t *conn;
    HCERTSTORE store = CertOpenStore( CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL );

    ssl = pX509_STORE_CTX_get_ex_data( ctx, pSSL_get_ex_data_X509_STORE_CTX_idx() );
    server = pSSL_get_ex_data( ssl, hostname_idx );
    conn = pSSL_get_ex_data( ssl, conn_idx );
    if (store)
    {
        X509 *cert;
        int i;
        PCCERT_CONTEXT endCert = NULL;
        struct stack_st *chain = (struct stack_st *)pX509_STORE_CTX_get_chain( ctx );

        ret = TRUE;
        for (i = 0; ret && i < psk_num(chain); i++)
        {
            PCCERT_CONTEXT context;

            cert = (X509 *)psk_value(chain, i);
            if ((context = X509_to_cert_context( cert )))
            {
                if (i == 0)
                    ret = CertAddCertificateContextToStore( store, context,
                        CERT_STORE_ADD_ALWAYS, &endCert );
                else
                    ret = CertAddCertificateContextToStore( store, context,
                        CERT_STORE_ADD_ALWAYS, NULL );
                CertFreeCertificateContext( context );
            }
        }
        if (!endCert) ret = FALSE;
        if (ret)
        {
            DWORD_PTR err = netconn_verify_cert( endCert, store, server,
                                                 conn->security_flags );

            if (err)
            {
                pSSL_set_ex_data( ssl, error_idx, (void *)err );
                ret = FALSE;
            }
        }
        CertFreeCertificateContext( endCert );
        CertCloseStore( store, 0 );
    }
    return ret;
}
#endif

BOOL netconn_init( netconn_t *conn, BOOL secure )
{
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    int i;
#endif

    conn->socket = -1;
    if (!secure) return TRUE;

#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    EnterCriticalSection( &init_ssl_cs );
    if (libssl_handle)
    {
        LeaveCriticalSection( &init_ssl_cs );
        return TRUE;
    }
    if (!(libssl_handle = wine_dlopen( SONAME_LIBSSL, RTLD_NOW, NULL, 0 )))
    {
        ERR("Trying to use SSL but couldn't load %s. Expect trouble.\n", SONAME_LIBSSL);
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    if (!(libcrypto_handle = wine_dlopen( SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0 )))
    {
        ERR("Trying to use SSL but couldn't load %s. Expect trouble.\n", SONAME_LIBCRYPTO);
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
#define LOAD_FUNCPTR(x) \
    if (!(p##x = wine_dlsym( libssl_handle, #x, NULL, 0 ))) \
    { \
        ERR("Failed to load symbol %s\n", #x); \
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR ); \
        LeaveCriticalSection( &init_ssl_cs ); \
        return FALSE; \
    }
    LOAD_FUNCPTR( SSL_library_init );
    LOAD_FUNCPTR( SSL_load_error_strings );
    LOAD_FUNCPTR( SSLv23_method );
    LOAD_FUNCPTR( SSL_CTX_free );
    LOAD_FUNCPTR( SSL_CTX_new );
    LOAD_FUNCPTR( SSL_new );
    LOAD_FUNCPTR( SSL_free );
    LOAD_FUNCPTR( SSL_set_fd );
    LOAD_FUNCPTR( SSL_connect );
    LOAD_FUNCPTR( SSL_shutdown );
    LOAD_FUNCPTR( SSL_write );
    LOAD_FUNCPTR( SSL_read );
    LOAD_FUNCPTR( SSL_pending );
    LOAD_FUNCPTR( SSL_get_error );
    LOAD_FUNCPTR( SSL_get_ex_new_index );
    LOAD_FUNCPTR( SSL_get_ex_data );
    LOAD_FUNCPTR( SSL_set_ex_data );
    LOAD_FUNCPTR( SSL_get_ex_data_X509_STORE_CTX_idx );
    LOAD_FUNCPTR( SSL_get_peer_certificate );
    LOAD_FUNCPTR( SSL_CTX_set_default_verify_paths );
    LOAD_FUNCPTR( SSL_CTX_set_verify );
    LOAD_FUNCPTR( SSL_get_current_cipher );
    LOAD_FUNCPTR( SSL_CIPHER_get_bits );
#undef LOAD_FUNCPTR

#define LOAD_FUNCPTR(x) \
    if (!(p##x = wine_dlsym( libcrypto_handle, #x, NULL, 0 ))) \
    { \
        ERR("Failed to load symbol %s\n", #x); \
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR ); \
        LeaveCriticalSection( &init_ssl_cs ); \
        return FALSE; \
    }
    LOAD_FUNCPTR( CRYPTO_num_locks );
    LOAD_FUNCPTR( CRYPTO_set_id_callback );
    LOAD_FUNCPTR( CRYPTO_set_locking_callback );
    LOAD_FUNCPTR( ERR_free_strings );
    LOAD_FUNCPTR( ERR_get_error );
    LOAD_FUNCPTR( ERR_error_string );
    LOAD_FUNCPTR( X509_STORE_CTX_get_ex_data );
    LOAD_FUNCPTR( X509_STORE_CTX_get_chain );
    LOAD_FUNCPTR( i2d_X509 );
    LOAD_FUNCPTR( sk_value );
    LOAD_FUNCPTR( sk_num );
#undef LOAD_FUNCPTR

    pSSL_library_init();
    pSSL_load_error_strings();

    method = pSSLv23_method();
    ctx = pSSL_CTX_new( method );
    if (!pSSL_CTX_set_default_verify_paths( ctx ))
    {
        ERR("SSL_CTX_set_default_verify_paths failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    hostname_idx = pSSL_get_ex_new_index( 0, (void *)"hostname index", NULL, NULL, NULL );
    if (hostname_idx == -1)
    {
        ERR("SSL_get_ex_new_index failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    error_idx = pSSL_get_ex_new_index( 0, (void *)"error index", NULL, NULL, NULL );
    if (error_idx == -1)
    {
        ERR("SSL_get_ex_new_index failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    conn_idx = pSSL_get_ex_new_index( 0, (void *)"netconn index", NULL, NULL, NULL );
    if (conn_idx == -1)
    {
        ERR("SSL_get_ex_new_index failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    pSSL_CTX_set_verify( ctx, SSL_VERIFY_PEER, netconn_secure_verify );

    pCRYPTO_set_id_callback(ssl_thread_id);
    num_ssl_locks = pCRYPTO_num_locks();
    ssl_locks = heap_alloc(num_ssl_locks * sizeof(CRITICAL_SECTION));
    if (!ssl_locks)
    {
        set_last_error( ERROR_OUTOFMEMORY );
        LeaveCriticalSection( &init_ssl_cs );
        return FALSE;
    }
    for (i = 0; i < num_ssl_locks; i++)
    {
        InitializeCriticalSection( &ssl_locks[i] );
        ssl_locks[i].DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ssl_locks");
    }
    pCRYPTO_set_locking_callback(ssl_lock_callback);

    LeaveCriticalSection( &init_ssl_cs );
#else
    WARN("SSL support not compiled in.\n");
    set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
    return FALSE;
#endif
    return TRUE;
}

void netconn_unload( void )
{
#if defined(SONAME_LIBSSL) && defined(SONAME_LIBCRYPTO)
    if (libcrypto_handle)
    {
        pERR_free_strings();
        wine_dlclose( libcrypto_handle, NULL, 0 );
    }
    if (libssl_handle)
    {
        if (ctx)
            pSSL_CTX_free( ctx );
        wine_dlclose( libssl_handle, NULL, 0 );
    }
    if (ssl_locks)
    {
        int i;
        for (i = 0; i < num_ssl_locks; i++)
        {
            ssl_locks[i].DebugInfo->Spare[0] = 0;
            DeleteCriticalSection( &ssl_locks[i] );
        }
        heap_free( ssl_locks );
    }
    DeleteCriticalSection(&init_ssl_cs);
#endif
#ifndef HAVE_GETADDRINFO
    DeleteCriticalSection(&cs_gethostbyname);
#endif
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

BOOL netconn_connect( netconn_t *conn, const struct sockaddr *sockaddr, unsigned int addr_len, int timeout )
{
    BOOL ret = FALSE;
    int res = 0, state;

    if (timeout > 0)
    {
        state = 1;
        ioctlsocket( conn->socket, FIONBIO, &state );
    }
    if (connect( conn->socket, sockaddr, addr_len ) < 0)
    {
        res = sock_get_error( errno );
        if (res == WSAEWOULDBLOCK || res == WSAEINPROGRESS)
        {
            // ReactOS: use select instead of poll
            fd_set outfd;
            struct timeval tv;

            FD_ZERO(&outfd);
            FD_SET(conn->socket, &outfd);

            tv.tv_sec = 0;
            tv.tv_usec = timeout * 1000;

            if (select( 0, NULL, &outfd, NULL, &tv ) > 0)
                ret = TRUE;
            else
                res = sock_get_error( errno );
        }
    }
    else
        ret = TRUE;
    if (timeout > 0)
    {
        state = 0;
        ioctlsocket( conn->socket, FIONBIO, &state );
    }
    if (!ret)
    {
        WARN("unable to connect to host (%d)\n", res);
        set_last_error( res );
    }
    return ret;
}

BOOL netconn_secure_connect( netconn_t *conn, WCHAR *hostname )
{
#ifdef SONAME_LIBSSL
    if (!(conn->ssl_conn = pSSL_new( ctx )))
    {
        ERR("SSL_new failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_OUTOFMEMORY );
        goto fail;
    }
    if (!pSSL_set_ex_data( conn->ssl_conn, hostname_idx, hostname ))
    {
        ERR("SSL_set_ex_data failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        goto fail;
    }
    if (!pSSL_set_ex_data( conn->ssl_conn, conn_idx, conn ))
    {
        ERR("SSL_set_ex_data failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        return FALSE;
    }
    if (!pSSL_set_fd( conn->ssl_conn, conn->socket ))
    {
        ERR("SSL_set_fd failed: %s\n", pERR_error_string( pERR_get_error(), 0 ));
        set_last_error( ERROR_WINHTTP_SECURE_CHANNEL_ERROR );
        goto fail;
    }
    if (pSSL_connect( conn->ssl_conn ) <= 0)
    {
        DWORD err;

        err = (DWORD_PTR)pSSL_get_ex_data( conn->ssl_conn, error_idx );
        if (!err) err = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
        ERR("couldn't verify server certificate (%d)\n", err);
        set_last_error( err );
        goto fail;
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
        int ret;

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
        ret = pSSL_read( conn->ssl_conn, (char *)buf + *recvd, len - *recvd );
        if (ret < 0)
            return FALSE;

        /* check if EOF was received */
        if (!ret && (pSSL_get_error( conn->ssl_conn, ret ) == SSL_ERROR_ZERO_RETURN ||
                     pSSL_get_error( conn->ssl_conn, ret ) == SSL_ERROR_SYSCALL ))
        {
            netconn_close( conn );
            return TRUE;
        }
        if (flags & MSG_PEEK) /* must copy into buffer */
        {
            conn->peek_len = ret;
            if (!ret)
            {
                heap_free( conn->peek_msg_mem );
                conn->peek_msg_mem = NULL;
                conn->peek_msg = NULL;
            }
            else memcpy( conn->peek_msg, buf, ret );
        }
        *recvd += ret;
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
        *available = pSSL_pending( conn->ssl_conn ) + conn->peek_len;
#endif
        return TRUE;
    }
#ifdef FIONREAD
    if (!(ret = ioctlsocket( conn->socket, FIONREAD, &unread ))) *available = unread;
#endif
    return TRUE;
}

BOOL netconn_get_next_line( netconn_t *conn, char *buffer, DWORD *buflen )
{
    // ReactOS: use select instead of poll
    fd_set infd;
    BOOL ret = FALSE;
    DWORD recvd = 0;

    if (!netconn_connected( conn )) return FALSE;

    if (conn->secure)
    {
#ifdef SONAME_LIBSSL
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

    while (recvd < *buflen)
    {
        int res;
        struct timeval tv, *ptv;
        socklen_t len = sizeof(tv);

        if ((res = getsockopt( conn->socket, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, &len ) != -1))
            ptv = &tv;
        else
            ptv = NULL;

        if (select( 0, &infd, NULL, NULL, ptv ) > 0)
        {
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

    if ((res = setsockopt( netconn->socket, SOL_SOCKET, send ? SO_SNDTIMEO : SO_RCVTIMEO, (void*)&tv, sizeof(tv) ) == -1))
    {
        WARN("setsockopt failed (%s)\n", strerror( errno ));
        return sock_get_error( errno );
    }
    return ERROR_SUCCESS;
}

static DWORD resolve_hostname( WCHAR *hostnameW, INTERNET_PORT port, struct sockaddr *sa, socklen_t *sa_len )
{
    char *hostname;
#ifdef HAVE_GETADDRINFO
    struct addrinfo *res, hints;
    int ret;
#else
    struct hostent *he;
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
#endif

    if (!(hostname = strdupWA( hostnameW ))) return ERROR_OUTOFMEMORY;

#ifdef HAVE_GETADDRINFO
    memset( &hints, 0, sizeof(struct addrinfo) );
    /* Prefer IPv4 to IPv6 addresses, since some web servers do not listen on
     * their IPv6 addresses even though they have IPv6 addresses in the DNS.
     */
    hints.ai_family = AF_INET;

    ret = getaddrinfo( hostname, NULL, &hints, &res );
    if (ret != 0)
    {
        TRACE("failed to get IPv4 address of %s (%s), retrying with IPv6\n", debugstr_w(hostnameW), gai_strerror(ret));
        hints.ai_family = AF_INET6;
        ret = getaddrinfo( hostname, NULL, &hints, &res );
        if (ret != 0)
        {
            TRACE("failed to get address of %s (%s)\n", debugstr_w(hostnameW), gai_strerror(ret));
            heap_free( hostname );
            return ERROR_WINHTTP_NAME_NOT_RESOLVED;
        }
    }
    heap_free( hostname );
    if (*sa_len < res->ai_addrlen)
    {
        WARN("address too small\n");
        freeaddrinfo( res );
        return ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }
    *sa_len = res->ai_addrlen;
    memcpy( sa, res->ai_addr, res->ai_addrlen );
    /* Copy port */
    switch (res->ai_family)
    {
    case AF_INET:
        ((struct sockaddr_in *)sa)->sin_port = htons( port );
        break;
    case AF_INET6:
        ((struct sockaddr_in6 *)sa)->sin6_port = htons( port );
        break;
    }

    freeaddrinfo( res );
    return ERROR_SUCCESS;
#else
    EnterCriticalSection( &cs_gethostbyname );

    he = gethostbyname( hostname );
    heap_free( hostname );
    if (!he)
    {
        TRACE("failed to get address of %s (%d)\n", debugstr_w(hostnameW), h_errno);
        LeaveCriticalSection( &cs_gethostbyname );
        return ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }
    if (*sa_len < sizeof(struct sockaddr_in))
    {
        WARN("address too small\n");
        LeaveCriticalSection( &cs_gethostbyname );
        return ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }
    *sa_len = sizeof(struct sockaddr_in);
    memset( sa, 0, sizeof(struct sockaddr_in) );
    memcpy( &sin->sin_addr, he->h_addr, he->h_length );
    sin->sin_family = he->h_addrtype;
    sin->sin_port = htons( port );

    LeaveCriticalSection( &cs_gethostbyname );
    return ERROR_SUCCESS;
#endif
}

struct resolve_args
{
    WCHAR           *hostname;
    INTERNET_PORT    port;
    struct sockaddr *sa;
    socklen_t       *sa_len;
};

static DWORD CALLBACK resolve_proc( LPVOID arg )
{
    struct resolve_args *ra = arg;
    return resolve_hostname( ra->hostname, ra->port, ra->sa, ra->sa_len );
}

BOOL netconn_resolve( WCHAR *hostname, INTERNET_PORT port, struct sockaddr *sa, socklen_t *sa_len, int timeout )
{
    DWORD ret;

    if (timeout)
    {
        DWORD status;
        HANDLE thread;
        struct resolve_args ra;

        ra.hostname = hostname;
        ra.port     = port;
        ra.sa       = sa;
        ra.sa_len   = sa_len;

        thread = CreateThread( NULL, 0, resolve_proc, &ra, 0, NULL );
        if (!thread) return FALSE;

        status = WaitForSingleObject( thread, timeout );
        if (status == WAIT_OBJECT_0) GetExitCodeThread( thread, &ret );
        else ret = ERROR_WINHTTP_TIMEOUT;
        CloseHandle( thread );
    }
    else ret = resolve_hostname( hostname, port, sa, sa_len );

    if (ret)
    {
        set_last_error( ret );
        return FALSE;
    }
    return TRUE;
}

const void *netconn_get_certificate( netconn_t *conn )
{
#ifdef SONAME_LIBSSL
    X509 *cert;
    const CERT_CONTEXT *ret;

    if (!conn->secure) return NULL;

    if (!(cert = pSSL_get_peer_certificate( conn->ssl_conn ))) return NULL;
    ret = X509_to_cert_context( cert );
    return ret;
#else
    return NULL;
#endif
}

int netconn_get_cipher_strength( netconn_t *conn )
{
#ifdef SONAME_LIBSSL
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090707f)
    const SSL_CIPHER *cipher;
#else
    SSL_CIPHER *cipher;
#endif
    int bits = 0;

    if (!conn->secure) return 0;
    if (!(cipher = pSSL_get_current_cipher( conn->ssl_conn ))) return 0;
    pSSL_CIPHER_get_bits( cipher, &bits );
    return bits;
#else
    return 0;
#endif
}
