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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ws2tcpip.h"
#include "winhttp.h"
#include "schannel.h"
#include "winternl.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static int sock_send(int fd, const void *msg, size_t len, WSAOVERLAPPED *ovr)
{
    WSABUF wsabuf;
    DWORD size;
    int err;

    wsabuf.len = len;
    wsabuf.buf = (void *)msg;

    if (!WSASend( (SOCKET)fd, &wsabuf, 1, &size, 0, ovr, NULL ))
    {
        assert( size == len );
        return size;
    }
    err = WSAGetLastError();
    if (!(ovr && err == WSA_IO_PENDING)) WARN( "send error %d\n", err );
    return -1;
}

BOOL netconn_wait_overlapped_result( struct netconn *conn, WSAOVERLAPPED *ovr, DWORD *len )
{
    OVERLAPPED *completion_ovr;
    ULONG_PTR key;

    while (1)
    {
        if (!GetQueuedCompletionStatus( conn->port, len, &key, &completion_ovr, INFINITE ))
        {
            WARN( "GetQueuedCompletionStatus failed, err %lu.\n", GetLastError() );
            return FALSE;
        }
        if (completion_ovr == (OVERLAPPED *)ovr && (key == conn->socket || conn->socket == -1))
            break;
        ERR( "Unexpected completion key %Ix, completion ovr %p, ovr %p.\n", key, completion_ovr, ovr );
    }
    return TRUE;
}

static int sock_recv(int fd, void *msg, size_t len, int flags)
{
    int ret;
    do
    {
        if ((ret = recv(fd, msg, len, flags)) == -1) WARN( "recv error %d\n", WSAGetLastError() );
    }
    while(ret == -1 && WSAGetLastError() == WSAEINTR);
    return ret;
}

static DWORD netconn_verify_cert( PCCERT_CONTEXT cert, WCHAR *server, DWORD security_flags, BOOL check_revocation )
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
    ret = CertGetCertificateChain( NULL, cert, NULL, store, &chainPara,
                                   check_revocation ? CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT : 0,
                                   NULL, &chain );
    if (ret)
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
            else if ((chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_IS_UNTRUSTED_ROOT) ||
                     (chain->TrustStatus.dwErrorStatus &
                      CERT_TRUST_IS_PARTIAL_CHAIN))
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
            sslExtraPolicyPara.cbSize = sizeof(sslExtraPolicyPara);
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
    TRACE( "returning %#lx\n", err );
    return err;
}

static BOOL winsock_loaded;

void netconn_unload( void )
{
    if (winsock_loaded) WSACleanup();
}

static BOOL WINAPI winsock_startup( INIT_ONCE *once, void *param, void **ctx )
{
    int ret;
    WSADATA data;
    if (!(ret = WSAStartup( MAKEWORD(1,1), &data ))) winsock_loaded = TRUE;
    else ERR( "WSAStartup failed: %d\n", ret );
    return TRUE;
}

#ifdef __REACTOS__
void winsock_init(void)
#else
static void winsock_init(void)
#endif
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce( &once, winsock_startup, NULL, NULL );
}

static void set_blocking( struct netconn *conn, BOOL blocking )
{
    ULONG state = !blocking;
    ioctlsocket( conn->socket, FIONBIO, &state );
}

DWORD netconn_create( struct hostdata *host, const struct sockaddr_storage *sockaddr, int timeout,
                      struct netconn **ret_conn )
{
    struct netconn *conn;
    unsigned int addr_len;
    DWORD ret;

#ifndef __REACTOS__
    winsock_init();
#endif

    if (!(conn = calloc( 1, sizeof(*conn) ))) return ERROR_OUTOFMEMORY;
    conn->refs = 1;
    conn->host = host;
    conn->sockaddr = *sockaddr;
    if ((conn->socket = WSASocketW( sockaddr->ss_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED )) == -1)
    {
        ret = WSAGetLastError();
        WARN( "unable to create socket (%lu)\n", ret );
        free( conn );
        return ret;
    }
    if (!SetFileCompletionNotificationModes( (HANDLE)(UINT_PTR)conn->socket, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS ))
        ERR( "SetFileCompletionNotificationModes failed.\n" );

    switch (conn->sockaddr.ss_family)
    {
    case AF_INET:
        addr_len = sizeof(struct sockaddr_in);
        break;
    case AF_INET6:
        addr_len = sizeof(struct sockaddr_in6);
        break;
    default:
        ERR( "unhandled family %u\n", conn->sockaddr.ss_family );
        free( conn );
        return ERROR_INVALID_PARAMETER;
    }

    if (timeout > 0) set_blocking( conn, FALSE );

    if (!connect( conn->socket, (const struct sockaddr *)&conn->sockaddr, addr_len )) ret = ERROR_SUCCESS;
    else
    {
        ret = WSAGetLastError();
        if (ret == WSAEWOULDBLOCK || ret == WSAEINPROGRESS)
        {
            TIMEVAL timeval = { timeout / 1000, (timeout % 1000) * 1000 };
            FD_SET set_read, set_error;
            int res;

            FD_ZERO( &set_read );
            FD_SET( conn->socket, &set_read );
            FD_ZERO( &set_error );
            FD_SET( conn->socket, &set_error );
            if ((res = select( conn->socket + 1, NULL, &set_read, &set_error, &timeval )) > 0)
            {
                if (FD_ISSET(conn->socket, &set_read)) ret = ERROR_SUCCESS;
                else                                   assert( FD_ISSET(conn->socket, &set_error) );
            }
            else if (!res) ret = ERROR_WINHTTP_TIMEOUT;
        }
    }

    if (timeout > 0) set_blocking( conn, TRUE );

    if (ret)
    {
        WARN( "unable to connect to host (%lu)\n", ret );
        closesocket( conn->socket );
        free( conn );
        return ret == ERROR_WINHTTP_TIMEOUT ? ERROR_WINHTTP_TIMEOUT : ERROR_WINHTTP_CANNOT_CONNECT;
    }

    *ret_conn = conn;
    return ERROR_SUCCESS;
}

void netconn_addref( struct netconn *conn )
{
    InterlockedIncrement( &conn->refs );
}

void netconn_release( struct netconn *conn )
{
    if (InterlockedDecrement( &conn->refs )) return;
    TRACE( "Closing connection %p.\n", conn );
    if (conn->secure)
    {
        free( conn->peek_msg_mem );
        free(conn->ssl_read_buf);
        free(conn->ssl_write_buf);
        free(conn->extra_buf);
        DeleteSecurityContext(&conn->ssl_ctx);
    }
    if (conn->socket != -1)
        closesocket( conn->socket );
    release_host( conn->host );
    if (conn->port)
        CloseHandle( conn->port );
    free(conn);
}

DWORD netconn_secure_connect( struct netconn *conn, WCHAR *hostname, DWORD security_flags, CredHandle *cred_handle,
                              BOOL check_revocation )
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

    if (!(read_buf = malloc( read_buf_size ))) return ERROR_OUTOFMEMORY;

    memset( &ctx, 0, sizeof(ctx) );
    status = InitializeSecurityContextW(cred_handle, NULL, hostname, isc_req_flags, 0, 0, NULL, 0,
            &ctx, &out_desc, &attrs, NULL);

    assert(status != SEC_E_OK);

    while(status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE) {
        if(out_buf.cbBuffer) {
            assert(status == SEC_I_CONTINUE_NEEDED);

            TRACE( "sending %lu bytes\n", out_buf.cbBuffer );

            size = sock_send(conn->socket, out_buf.pvBuffer, out_buf.cbBuffer, NULL);
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
        }

        assert(in_bufs[0].BufferType == SECBUFFER_TOKEN);
        in_bufs[1].BufferType = SECBUFFER_EMPTY;
        in_bufs[1].cbBuffer = 0;
        in_bufs[1].pvBuffer = NULL;

        if(in_bufs[0].cbBuffer + 1024 > read_buf_size) {
            BYTE *new_read_buf;

            new_read_buf = realloc(read_buf, read_buf_size + 1024);
            if(!new_read_buf) {
                status = E_OUTOFMEMORY;
                break;
            }

            in_bufs[0].pvBuffer = read_buf = new_read_buf;
            read_buf_size += 1024;
        }

        size = sock_recv(conn->socket, read_buf+in_bufs[0].cbBuffer, read_buf_size-in_bufs[0].cbBuffer, 0);
        if(size < 1) {
            status = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
            break;
        }

        TRACE( "recv %Iu bytes\n", size );

        in_bufs[0].cbBuffer += size;
        in_bufs[0].pvBuffer = read_buf;
        status = InitializeSecurityContextW(cred_handle, &ctx, hostname,  isc_req_flags, 0, 0, &in_desc,
                0, NULL, &out_desc, &attrs, NULL);
        TRACE( "InitializeSecurityContext ret %#lx\n", status );

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
                res = netconn_verify_cert(cert, hostname, security_flags, check_revocation);
                CertFreeCertificateContext(cert);
                if(res != ERROR_SUCCESS) {
                    WARN( "cert verify failed: %lu\n", res );
                    break;
                }
            }else {
                WARN("Could not get cert\n");
                break;
            }

            conn->ssl_read_buf = malloc(conn->ssl_sizes.cbHeader + conn->ssl_sizes.cbMaximumMessage + conn->ssl_sizes.cbTrailer);
            if(!conn->ssl_read_buf) {
                res = ERROR_OUTOFMEMORY;
                break;
            }
            conn->ssl_write_buf = malloc(conn->ssl_sizes.cbHeader + conn->ssl_sizes.cbMaximumMessage + conn->ssl_sizes.cbTrailer);
            if(!conn->ssl_write_buf) {
                res = ERROR_OUTOFMEMORY;
                break;
            }
        }
    }

    free(read_buf);

    if(status != SEC_E_OK || res != ERROR_SUCCESS) {
        WARN( "Failed to initialize security context: %#lx\n", status );
        free(conn->ssl_read_buf);
        conn->ssl_read_buf = NULL;
        free(conn->ssl_write_buf);
        conn->ssl_write_buf = NULL;
        DeleteSecurityContext(&ctx);
        return ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
    }


    TRACE("established SSL connection\n");
    conn->secure = TRUE;
    conn->ssl_ctx = ctx;
    return ERROR_SUCCESS;
}

static DWORD send_ssl_chunk( struct netconn *conn, const void *msg, size_t size, WSAOVERLAPPED *ovr )
{
    SecBuffer bufs[4] = {
        {conn->ssl_sizes.cbHeader, SECBUFFER_STREAM_HEADER, conn->ssl_write_buf},
        {size,  SECBUFFER_DATA, conn->ssl_write_buf+conn->ssl_sizes.cbHeader},
        {conn->ssl_sizes.cbTrailer, SECBUFFER_STREAM_TRAILER, conn->ssl_write_buf+conn->ssl_sizes.cbHeader+size},
        {0, SECBUFFER_EMPTY, NULL}
    };
    SecBufferDesc buf_desc = {SECBUFFER_VERSION, ARRAY_SIZE(bufs), bufs};
    SECURITY_STATUS res;

    memcpy( bufs[1].pvBuffer, msg, size );
    if ((res = EncryptMessage(&conn->ssl_ctx, 0, &buf_desc, 0)) != SEC_E_OK)
    {
        WARN( "EncryptMessage failed: %#lx\n", res );
        return res;
    }

    if (sock_send( conn->socket, conn->ssl_write_buf, bufs[0].cbBuffer + bufs[1].cbBuffer + bufs[2].cbBuffer, ovr ) < 1)
    {
        WARN("send failed\n");
        return WSAGetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD netconn_send( struct netconn *conn, const void *msg, size_t len, int *sent, WSAOVERLAPPED *ovr )
{
    DWORD err;

    if (ovr && !conn->port)
    {
        if (!(conn->port = CreateIoCompletionPort( (HANDLE)(SOCKET)conn->socket, NULL, (ULONG_PTR)conn->socket, 0 )))
            ERR( "Failed to create port.\n" );
    }

    if (conn->secure)
    {
        const BYTE *ptr = msg;
        size_t chunk_size;
        DWORD res;

        *sent = 0;
        while (len)
        {
            chunk_size = min( len, conn->ssl_sizes.cbMaximumMessage );
            if ((res = send_ssl_chunk( conn, ptr, chunk_size, ovr )))
            {
                if (res == WSA_IO_PENDING) *sent += chunk_size;
                return res;
            }
            *sent += chunk_size;
            ptr += chunk_size;
            len -= chunk_size;
        }

        return ERROR_SUCCESS;
    }

    if ((*sent = sock_send( conn->socket, msg, len, ovr )) < 0)
    {
        err = WSAGetLastError();
        *sent = (err == WSA_IO_PENDING) ? len : 0;
        return err;
    }
    return ERROR_SUCCESS;
}

static DWORD read_ssl_chunk( struct netconn *conn, void *buf, SIZE_T buf_size, SIZE_T *ret_size, BOOL *eof )
{
    const SIZE_T ssl_buf_size = conn->ssl_sizes.cbHeader+conn->ssl_sizes.cbMaximumMessage+conn->ssl_sizes.cbTrailer;
    SecBuffer bufs[4];
    SecBufferDesc buf_desc = {SECBUFFER_VERSION, ARRAY_SIZE(bufs), bufs};
    SSIZE_T size, buf_len;
    unsigned int i;
    SECURITY_STATUS res;

    assert(conn->extra_len < ssl_buf_size);

    if(conn->extra_len) {
        memcpy(conn->ssl_read_buf, conn->extra_buf, conn->extra_len);
        buf_len = conn->extra_len;
        conn->extra_len = 0;
        free(conn->extra_buf);
        conn->extra_buf = NULL;
    }else {
        if ((buf_len = sock_recv( conn->socket, conn->ssl_read_buf + conn->extra_len, ssl_buf_size - conn->extra_len, 0)) < 0)
            return WSAGetLastError();

        if (!buf_len)
        {
            *eof = TRUE;
            return ERROR_SUCCESS;
        }
    }

    *ret_size = 0;
    *eof = FALSE;

    do {
        memset(bufs, 0, sizeof(bufs));
        bufs[0].BufferType = SECBUFFER_DATA;
        bufs[0].cbBuffer = buf_len;
        bufs[0].pvBuffer = conn->ssl_read_buf;

        switch ((res = DecryptMessage( &conn->ssl_ctx, &buf_desc, 0, NULL )))
        {
        case SEC_E_OK:
            break;

        case SEC_I_RENEGOTIATE:
            TRACE("renegotiate\n");
            return ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED;

        case SEC_I_CONTEXT_EXPIRED:
            TRACE("context expired\n");
            *eof = TRUE;
            return ERROR_SUCCESS;

        case SEC_E_INCOMPLETE_MESSAGE:
            assert(buf_len < ssl_buf_size);

            if ((size = sock_recv( conn->socket, conn->ssl_read_buf + buf_len, ssl_buf_size - buf_len, 0 )) < 1)
                return SEC_E_INCOMPLETE_MESSAGE;

            buf_len += size;
            continue;

        default:
            WARN( "failed: %#lx\n", res );
            return res;
        }
    } while (res != SEC_E_OK);

    for(i = 0; i < ARRAY_SIZE(bufs); i++) {
        if(bufs[i].BufferType == SECBUFFER_DATA) {
            size = min(buf_size, bufs[i].cbBuffer);
            memcpy(buf, bufs[i].pvBuffer, size);
            if(size < bufs[i].cbBuffer) {
                assert(!conn->peek_len);
                conn->peek_msg_mem = conn->peek_msg = malloc(bufs[i].cbBuffer - size);
                if(!conn->peek_msg)
                    return ERROR_OUTOFMEMORY;
                conn->peek_len = bufs[i].cbBuffer-size;
                memcpy(conn->peek_msg, (char*)bufs[i].pvBuffer+size, conn->peek_len);
            }

            *ret_size = size;
        }
    }

    for(i = 0; i < ARRAY_SIZE(bufs); i++) {
        if(bufs[i].BufferType == SECBUFFER_EXTRA) {
            conn->extra_buf = malloc(bufs[i].cbBuffer);
            if(!conn->extra_buf)
                return ERROR_OUTOFMEMORY;

            conn->extra_len = bufs[i].cbBuffer;
            memcpy(conn->extra_buf, bufs[i].pvBuffer, conn->extra_len);
        }
    }

    return ERROR_SUCCESS;
}

DWORD netconn_recv( struct netconn *conn, void *buf, size_t len, int flags, int *recvd )
{
    *recvd = 0;
    if (!len) return ERROR_SUCCESS;

    if (conn->secure)
    {
        SIZE_T size;
        DWORD res;
        BOOL eof;

        if (conn->peek_msg)
        {
            *recvd = min( len, conn->peek_len );
            memcpy( buf, conn->peek_msg, *recvd );
            conn->peek_len -= *recvd;
            conn->peek_msg += *recvd;

            if (conn->peek_len == 0)
            {
                free( conn->peek_msg_mem );
                conn->peek_msg_mem = NULL;
                conn->peek_msg = NULL;
            }
            /* check if we have enough data from the peek buffer */
            if (!(flags & MSG_WAITALL) || *recvd == len) return ERROR_SUCCESS;
        }
        size = *recvd;

        do
        {
            SIZE_T cread = 0;
            if ((res = read_ssl_chunk( conn, (BYTE *)buf + size, len - size, &cread, &eof )))
            {
                WARN( "read_ssl_chunk failed: %lu\n", res );
                if (!size) return res;
                break;
            }
            if (eof)
            {
                TRACE("EOF\n");
                break;
            }
            size += cread;

        } while (!size || ((flags & MSG_WAITALL) && size < len));

        TRACE( "received %Iu bytes\n", size );
        *recvd = size;
        return ERROR_SUCCESS;
    }

    if ((*recvd = sock_recv( conn->socket, buf, len, flags )) < 0) return WSAGetLastError();
    return ERROR_SUCCESS;
}

void netconn_cancel_io( struct netconn *conn )
{
    SOCKET socket = InterlockedExchange( (LONG *)&conn->socket, -1 );

    closesocket( socket );
}

ULONG netconn_query_data_available( struct netconn *conn )
{
    return conn->secure ? conn->peek_len : 0;
}

DWORD netconn_set_timeout( struct netconn *netconn, BOOL send, int value )
{
    int opt = send ? SO_SNDTIMEO : SO_RCVTIMEO;
    if (setsockopt( netconn->socket, SOL_SOCKET, opt, (void *)&value, sizeof(value) ) == -1)
    {
        DWORD err = WSAGetLastError();
        WARN( "setsockopt failed (%lu)\n", err );
        return err;
    }
    return ERROR_SUCCESS;
}

BOOL netconn_is_alive( struct netconn *netconn )
{
    SIZE_T size;
    int len;
    char b;
    DWORD err;
    BOOL eof;

    set_blocking( netconn, FALSE );
    if (netconn->secure)
    {
        while (!netconn->peek_msg && !(err = read_ssl_chunk( netconn, NULL, 0, &size, &eof )) && !eof)
            ;

        TRACE( "checking secure connection, err %lu\n", err );

        if (netconn->peek_msg || err == WSAEWOULDBLOCK)
        {
            set_blocking( netconn, TRUE );
            return TRUE;
        }
        if (err != SEC_E_OK && err != SEC_E_INCOMPLETE_MESSAGE)
        {
            set_blocking( netconn, TRUE );
            return FALSE;
        }
    }
    len = sock_recv( netconn->socket, &b, 1, MSG_PEEK );
    err = WSAGetLastError();
    set_blocking( netconn, TRUE );

    return len == 1 || (len == -1 && err == WSAEWOULDBLOCK);
}

static DWORD resolve_hostname( const WCHAR *name, INTERNET_PORT port, struct sockaddr_storage *sa )
{
    ADDRINFOW *res, hints;
    int ret;

    memset( &hints, 0, sizeof(hints) );
    /* Prefer IPv4 to IPv6 addresses, since some web servers do not listen on
     * their IPv6 addresses even though they have IPv6 addresses in the DNS.
     */
    hints.ai_family = AF_INET;

    ret = GetAddrInfoW( name, NULL, &hints, &res );
    if (ret != 0)
    {
        TRACE("failed to get IPv4 address of %s, retrying with IPv6\n", debugstr_w(name));
        hints.ai_family = AF_INET6;
        ret = GetAddrInfoW( name, NULL, &hints, &res );
        if (ret != 0)
        {
            TRACE("failed to get address of %s\n", debugstr_w(name));
            return ERROR_WINHTTP_NAME_NOT_RESOLVED;
        }
    }
    memcpy( sa, res->ai_addr, res->ai_addrlen );
    switch (res->ai_family)
    {
    case AF_INET:
        ((struct sockaddr_in *)sa)->sin_port = htons( port );
        break;
    case AF_INET6:
        ((struct sockaddr_in6 *)sa)->sin6_port = htons( port );
        break;
    }

    FreeAddrInfoW( res );
    return ERROR_SUCCESS;
}

struct async_resolve
{
    LONG                     ref;
    WCHAR                   *hostname;
    INTERNET_PORT            port;
    struct sockaddr_storage  addr;
    DWORD                    result;
    HANDLE                   done;
};

static struct async_resolve *create_async_resolve( const WCHAR *hostname, INTERNET_PORT port )
{
    struct async_resolve *ret;

    if (!(ret = malloc(sizeof(*ret))))
    {
        ERR( "No memory.\n" );
        return NULL;
    }
    ret->ref = 1;
    ret->hostname = wcsdup( hostname );
    ret->port     = port;
    if (!(ret->done = CreateEventW( NULL, FALSE, FALSE, NULL )))
    {
        free( ret->hostname );
        free( ret );
        return NULL;
    }
    return ret;
}

static void async_resolve_release( struct async_resolve *async )
{
    if (InterlockedDecrement( &async->ref )) return;

    free( async->hostname );
    CloseHandle( async->done );
    free( async );
}

static void CALLBACK resolve_proc( TP_CALLBACK_INSTANCE *instance, void *ctx )
{
    struct async_resolve *async = ctx;

    async->result = resolve_hostname( async->hostname, async->port, &async->addr );
    SetEvent( async->done );
    async_resolve_release( async );
}

DWORD netconn_resolve( WCHAR *hostname, INTERNET_PORT port, struct sockaddr_storage *addr, int timeout )
{
    DWORD ret;

    if (!timeout) ret = resolve_hostname( hostname, port, addr );
    else
    {
        struct async_resolve *async;

        if (!(async = create_async_resolve( hostname, port )))
            return ERROR_OUTOFMEMORY;

        InterlockedIncrement( &async->ref );
        if (!TrySubmitThreadpoolCallback( resolve_proc, async, NULL ))
        {
            InterlockedDecrement( &async->ref );
            async_resolve_release( async );
            return GetLastError();
        }
        if (WaitForSingleObject( async->done, timeout ) != WAIT_OBJECT_0) ret = ERROR_WINHTTP_TIMEOUT;
        else
        {
            *addr = async->addr;
            ret = async->result;
        }
        async_resolve_release( async );
    }

    return ret;
}

const void *netconn_get_certificate( struct netconn *conn )
{
    const CERT_CONTEXT *ret;
    SECURITY_STATUS res;

    if (!conn->secure) return NULL;
    res = QueryContextAttributesW(&conn->ssl_ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&ret);
    return res == SEC_E_OK ? ret : NULL;
}

int netconn_get_cipher_strength( struct netconn *conn )
{
    SecPkgContext_ConnectionInfo conn_info;
    SECURITY_STATUS res;

    if (!conn->secure) return 0;
    res = QueryContextAttributesW(&conn->ssl_ctx, SECPKG_ATTR_CONNECTION_INFO, (void*)&conn_info);
    if(res != SEC_E_OK)
        WARN( "QueryContextAttributesW failed: %#lx\n", res );
    return res == SEC_E_OK ? conn_info.dwCipherStrength : 0;
}
