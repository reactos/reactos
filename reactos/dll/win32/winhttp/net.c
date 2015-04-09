/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
 * Copyright 2013 Jacek Caban for CodeWeavers
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

#include "winhttp_private.h"

#include <assert.h>
#include <schannel.h>

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

static int sock_send(int fd, const void *msg, size_t len, int flags)
{
    int ret;
    do
    {
        ret = send(fd, msg, len, flags);
    }
    while(ret == -1 && errno == EINTR);
    return ret;
}

static int sock_recv(int fd, void *msg, size_t len, int flags)
{
    int ret;
    do
    {
        ret = recv(fd, msg, len, flags);
    }
    while(ret == -1 && errno == EINTR);
    return ret;
}

static DWORD netconn_verify_cert( PCCERT_CONTEXT cert, WCHAR *server, DWORD security_flags )
{
    HCERTSTORE store = cert->hCertStore;
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

static SecHandle cred_handle;
static BOOL cred_handle_initialized;

static CRITICAL_SECTION init_sechandle_cs;
static CRITICAL_SECTION_DEBUG init_sechandle_cs_debug = {
    0, 0, &init_sechandle_cs,
    { &init_sechandle_cs_debug.ProcessLocksList,
      &init_sechandle_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_sechandle_cs") }
};
static CRITICAL_SECTION init_sechandle_cs = { &init_sechandle_cs_debug, -1, 0, 0, 0, 0 };

static BOOL ensure_cred_handle(void)
{
    BOOL ret = TRUE;

    EnterCriticalSection(&init_sechandle_cs);

    if(!cred_handle_initialized) {
        SECURITY_STATUS res;

        res = AcquireCredentialsHandleW(NULL, (WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL, NULL,
                NULL, NULL, &cred_handle, NULL);
        if(res == SEC_E_OK) {
            cred_handle_initialized = TRUE;
        }else {
            WARN("AcquireCredentialsHandleW failed: %u\n", res);
            ret = FALSE;
        }
    }

    LeaveCriticalSection(&init_sechandle_cs);
    return ret;
}

BOOL netconn_init( netconn_t *conn )
{
    memset(conn, 0, sizeof(*conn));
    conn->socket = -1;
    return TRUE;
}

void netconn_unload( void )
{
    if(cred_handle_initialized)
        FreeCredentialsHandle(&cred_handle);
    DeleteCriticalSection(&init_sechandle_cs);
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

    if (conn->secure)
    {
        heap_free( conn->peek_msg_mem );
        conn->peek_msg_mem = NULL;
        conn->peek_msg = NULL;
        conn->peek_len = 0;
        heap_free(conn->ssl_buf);
        conn->ssl_buf = NULL;
        heap_free(conn->extra_buf);
        conn->extra_buf = NULL;
        conn->extra_len = 0;
        DeleteSecurityContext(&conn->ssl_ctx);
        conn->secure = FALSE;
    }
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
    int res = 0;
    ULONG state;

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
            /* ReactOS: use select instead of poll */
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
    SecBuffer out_buf = {0, SECBUFFER_TOKEN, NULL}, in_bufs[2] = {{0, SECBUFFER_TOKEN}, {0, SECBUFFER_EMPTY}};
    SecBufferDesc out_desc = {SECBUFFER_VERSION, 1, &out_buf}, in_desc = {SECBUFFER_VERSION, 2, in_bufs};
    BYTE *read_buf;
    SIZE_T read_buf_size = 2048;
    ULONG attrs = 0;
    CtxtHandle ctx;
    SSIZE_T size;
    const CERT_CONTEXT *cert;
    SECURITY_STATUS status;
    DWORD res = ERROR_SUCCESS;

    const DWORD isc_req_flags = ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_USE_SESSION_KEY|ISC_REQ_CONFIDENTIALITY
        |ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_MANUAL_CRED_VALIDATION;

    if(!ensure_cred_handle())
        return FALSE;

    read_buf = heap_alloc(read_buf_size);
    if(!read_buf)
        return FALSE;

    status = InitializeSecurityContextW(&cred_handle, NULL, hostname, isc_req_flags, 0, 0, NULL, 0,
            &ctx, &out_desc, &attrs, NULL);

    assert(status != SEC_E_OK);

    while(status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE) {
        if(out_buf.cbBuffer) {
            assert(status == SEC_I_CONTINUE_NEEDED);

            TRACE("sending %u bytes\n", out_buf.cbBuffer);

            size = sock_send(conn->socket, out_buf.pvBuffer, out_buf.cbBuffer, 0);
            if(size != out_buf.cbBuffer) {
                ERR("send failed\n");
                res = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
                break;
            }

            FreeContextBuffer(out_buf.pvBuffer);
            out_buf.pvBuffer = NULL;
            out_buf.cbBuffer = 0;
        }

        if(status == SEC_I_CONTINUE_NEEDED) {
            assert(in_bufs[1].cbBuffer < read_buf_size);

            memmove(read_buf, (BYTE*)in_bufs[0].pvBuffer+in_bufs[0].cbBuffer-in_bufs[1].cbBuffer, in_bufs[1].cbBuffer);
            in_bufs[0].cbBuffer = in_bufs[1].cbBuffer;

            in_bufs[1].BufferType = SECBUFFER_EMPTY;
            in_bufs[1].cbBuffer = 0;
            in_bufs[1].pvBuffer = NULL;
        }

        assert(in_bufs[0].BufferType == SECBUFFER_TOKEN);
        assert(in_bufs[1].BufferType == SECBUFFER_EMPTY);

        if(in_bufs[0].cbBuffer + 1024 > read_buf_size) {
            BYTE *new_read_buf;

            new_read_buf = heap_realloc(read_buf, read_buf_size + 1024);
            if(!new_read_buf) {
                status = E_OUTOFMEMORY;
                break;
            }

            in_bufs[0].pvBuffer = read_buf = new_read_buf;
            read_buf_size += 1024;
        }

        size = sock_recv(conn->socket, read_buf+in_bufs[0].cbBuffer, read_buf_size-in_bufs[0].cbBuffer, 0);
        if(size < 1) {
            WARN("recv error\n");
            status = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
            break;
        }

        TRACE("recv %lu bytes\n", size);

        in_bufs[0].cbBuffer += size;
        in_bufs[0].pvBuffer = read_buf;
        status = InitializeSecurityContextW(&cred_handle, &ctx, hostname,  isc_req_flags, 0, 0, &in_desc,
                0, NULL, &out_desc, &attrs, NULL);
        TRACE("InitializeSecurityContext ret %08x\n", status);

        if(status == SEC_E_OK) {
            if(in_bufs[1].BufferType == SECBUFFER_EXTRA)
                FIXME("SECBUFFER_EXTRA not supported\n");

            status = QueryContextAttributesW(&ctx, SECPKG_ATTR_STREAM_SIZES, &conn->ssl_sizes);
            if(status != SEC_E_OK) {
                WARN("Could not get sizes\n");
                break;
            }

            status = QueryContextAttributesW(&ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
            if(status == SEC_E_OK) {
                res = netconn_verify_cert(cert, hostname, conn->security_flags);
                CertFreeCertificateContext(cert);
                if(res != ERROR_SUCCESS) {
                    WARN("cert verify failed: %u\n", res);
                    break;
                }
            }else {
                WARN("Could not get cert\n");
                break;
            }

            conn->ssl_buf = heap_alloc(conn->ssl_sizes.cbHeader + conn->ssl_sizes.cbMaximumMessage + conn->ssl_sizes.cbTrailer);
            if(!conn->ssl_buf) {
                res = GetLastError();
                break;
            }
        }
    }

    heap_free(read_buf);

    if(status != SEC_E_OK || res != ERROR_SUCCESS) {
        WARN("Failed to initialize security context failed: %08x\n", status);
        heap_free(conn->ssl_buf);
        conn->ssl_buf = NULL;
        DeleteSecurityContext(&ctx);
        set_last_error(res ? res : ERROR_WINHTTP_SECURE_CHANNEL_ERROR);
        return FALSE;
    }


    TRACE("established SSL connection\n");
    conn->secure = TRUE;
    conn->ssl_ctx = ctx;
    return TRUE;
}

static BOOL send_ssl_chunk(netconn_t *conn, const void *msg, size_t size)
{
    SecBuffer bufs[4] = {
        {conn->ssl_sizes.cbHeader, SECBUFFER_STREAM_HEADER, conn->ssl_buf},
        {size,  SECBUFFER_DATA, conn->ssl_buf+conn->ssl_sizes.cbHeader},
        {conn->ssl_sizes.cbTrailer, SECBUFFER_STREAM_TRAILER, conn->ssl_buf+conn->ssl_sizes.cbHeader+size},
        {0, SECBUFFER_EMPTY, NULL}
    };
    SecBufferDesc buf_desc = {SECBUFFER_VERSION, sizeof(bufs)/sizeof(*bufs), bufs};
    SECURITY_STATUS res;

    memcpy(bufs[1].pvBuffer, msg, size);
    res = EncryptMessage(&conn->ssl_ctx, 0, &buf_desc, 0);
    if(res != SEC_E_OK) {
        WARN("EncryptMessage failed\n");
        return FALSE;
    }

    if(sock_send(conn->socket, conn->ssl_buf, bufs[0].cbBuffer+bufs[1].cbBuffer+bufs[2].cbBuffer, 0) < 1) {
        WARN("send failed\n");
        return FALSE;
    }

    return TRUE;
}

BOOL netconn_send( netconn_t *conn, const void *msg, size_t len, int *sent )
{
    if (!netconn_connected( conn )) return FALSE;
    if (conn->secure)
    {
        const BYTE *ptr = msg;
        size_t chunk_size;

        *sent = 0;

        while(len) {
            chunk_size = min(len, conn->ssl_sizes.cbMaximumMessage);
            if(!send_ssl_chunk(conn, ptr, chunk_size))
                return FALSE;

            *sent += chunk_size;
            ptr += chunk_size;
            len -= chunk_size;
        }

        return TRUE;
    }
    if ((*sent = sock_send( conn->socket, msg, len, 0 )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

static BOOL read_ssl_chunk(netconn_t *conn, void *buf, SIZE_T buf_size, SIZE_T *ret_size, BOOL *eof)
{
    const SIZE_T ssl_buf_size = conn->ssl_sizes.cbHeader+conn->ssl_sizes.cbMaximumMessage+conn->ssl_sizes.cbTrailer;
    SecBuffer bufs[4];
    SecBufferDesc buf_desc = {SECBUFFER_VERSION, sizeof(bufs)/sizeof(*bufs), bufs};
    SSIZE_T size, buf_len;
    unsigned int i;
    SECURITY_STATUS res;

    assert(conn->extra_len < ssl_buf_size);

    if(conn->extra_len) {
        memcpy(conn->ssl_buf, conn->extra_buf, conn->extra_len);
        buf_len = conn->extra_len;
        conn->extra_len = 0;
        heap_free(conn->extra_buf);
        conn->extra_buf = NULL;
    }else {
        buf_len = sock_recv(conn->socket, conn->ssl_buf+conn->extra_len, ssl_buf_size-conn->extra_len, 0);
        if(buf_len < 0) {
            WARN("recv failed\n");
            return FALSE;
        }

        if(!buf_len) {
            *eof = TRUE;
            return TRUE;
        }
    }

    *ret_size = 0;
    *eof = FALSE;

    do {
        memset(bufs, 0, sizeof(bufs));
        bufs[0].BufferType = SECBUFFER_DATA;
        bufs[0].cbBuffer = buf_len;
        bufs[0].pvBuffer = conn->ssl_buf;

        res = DecryptMessage(&conn->ssl_ctx, &buf_desc, 0, NULL);
        switch(res) {
        case SEC_E_OK:
            break;
        case SEC_I_CONTEXT_EXPIRED:
            TRACE("context expired\n");
            *eof = TRUE;
            return TRUE;
        case SEC_E_INCOMPLETE_MESSAGE:
            assert(buf_len < ssl_buf_size);

            size = sock_recv(conn->socket, conn->ssl_buf+buf_len, ssl_buf_size-buf_len, 0);
            if(size < 1)
                return FALSE;

            buf_len += size;
            continue;
        default:
            WARN("failed: %08x\n", res);
            return FALSE;
        }
    } while(res != SEC_E_OK);

    for(i=0; i < sizeof(bufs)/sizeof(*bufs); i++) {
        if(bufs[i].BufferType == SECBUFFER_DATA) {
            size = min(buf_size, bufs[i].cbBuffer);
            memcpy(buf, bufs[i].pvBuffer, size);
            if(size < bufs[i].cbBuffer) {
                assert(!conn->peek_len);
                conn->peek_msg_mem = conn->peek_msg = heap_alloc(bufs[i].cbBuffer - size);
                if(!conn->peek_msg)
                    return FALSE;
                conn->peek_len = bufs[i].cbBuffer-size;
                memcpy(conn->peek_msg, (char*)bufs[i].pvBuffer+size, conn->peek_len);
            }

            *ret_size = size;
        }
    }

    for(i=0; i < sizeof(bufs)/sizeof(*bufs); i++) {
        if(bufs[i].BufferType == SECBUFFER_EXTRA) {
            conn->extra_buf = heap_alloc(bufs[i].cbBuffer);
            if(!conn->extra_buf)
                return FALSE;

            conn->extra_len = bufs[i].cbBuffer;
            memcpy(conn->extra_buf, bufs[i].pvBuffer, conn->extra_len);
        }
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
        SIZE_T size, cread;
        BOOL res, eof;

        if (conn->peek_msg)
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
            if (!(flags & MSG_WAITALL) || *recvd == len) return TRUE;
        }
        size = *recvd;

        do {
            res = read_ssl_chunk(conn, (BYTE*)buf+size, len-size, &cread, &eof);
            if(!res) {
                WARN("read_ssl_chunk failed\n");
                if(!size)
                    return FALSE;
                break;
            }

            if(eof) {
                TRACE("EOF\n");
                break;
            }

            size += cread;
        }while(!size || ((flags & MSG_WAITALL) && size < len));

        TRACE("received %ld bytes\n", size);
        *recvd = size;
        return TRUE;
    }
    if ((*recvd = sock_recv( conn->socket, buf, len, flags )) == -1)
    {
        set_last_error( sock_get_error( errno ) );
        return FALSE;
    }
    return TRUE;
}

ULONG netconn_query_data_available( netconn_t *conn )
{
    if(!netconn_connected(conn))
        return 0;

    if(conn->secure)
        return conn->peek_len;

    return 0;
}

DWORD netconn_set_timeout( netconn_t *netconn, BOOL send, int value )
{
    struct timeval tv;

    /* value is in milliseconds, convert to struct timeval */
    tv.tv_sec = value / 1000;
    tv.tv_usec = (value % 1000) * 1000;

    if (setsockopt( netconn->socket, SOL_SOCKET, send ? SO_SNDTIMEO : SO_RCVTIMEO, (void*)&tv, sizeof(tv) ) == -1)
    {
        WARN("setsockopt failed (%s)\n", strerror( errno ));
        return sock_get_error( errno );
    }
    return ERROR_SUCCESS;
}

static DWORD resolve_hostname( const WCHAR *hostnameW, INTERNET_PORT port, struct sockaddr *sa, socklen_t *sa_len )
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
    const WCHAR     *hostname;
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
    const CERT_CONTEXT *ret;
    SECURITY_STATUS res;

    if (!conn->secure) return NULL;
    res = QueryContextAttributesW(&conn->ssl_ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&ret);
    return res == SEC_E_OK ? ret : NULL;
}

int netconn_get_cipher_strength( netconn_t *conn )
{
    SecPkgContext_ConnectionInfo conn_info;
    SECURITY_STATUS res;

    if (!conn->secure) return 0;
    res = QueryContextAttributesW(&conn->ssl_ctx, SECPKG_ATTR_CONNECTION_INFO, (void*)&conn_info);
    if(res != SEC_E_OK)
        WARN("QueryContextAttributesW failed: %08x\n", res);
    return res == SEC_E_OK ? conn_info.dwCipherStrength : 0;
}
