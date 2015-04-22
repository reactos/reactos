/*
 * Wininet - networking layer
 *
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2013 Jacek Caban for CodeWeavers
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

#include "internet.h"

#include <sys/types.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#include <errno.h>

static DWORD netconn_verify_cert(netconn_t *conn, PCCERT_CONTEXT cert, HCERTSTORE store)
{
    BOOL ret;
    CERT_CHAIN_PARA chainPara = { sizeof(chainPara), { 0 } };
    PCCERT_CHAIN_CONTEXT chain;
    char oid_server_auth[] = szOID_PKIX_KP_SERVER_AUTH;
    char *server_auth[] = { oid_server_auth };
    DWORD err = ERROR_SUCCESS, errors;

    static const DWORD supportedErrors =
        CERT_TRUST_IS_NOT_TIME_VALID |
        CERT_TRUST_IS_UNTRUSTED_ROOT |
        CERT_TRUST_IS_PARTIAL_CHAIN |
        CERT_TRUST_IS_NOT_SIGNATURE_VALID |
        CERT_TRUST_IS_NOT_VALID_FOR_USAGE;

    TRACE("verifying %s\n", debugstr_w(conn->server->name));

    chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = server_auth;
    if (!(ret = CertGetCertificateChain(NULL, cert, NULL, store, &chainPara, 0, NULL, &chain))) {
        TRACE("failed\n");
        return GetLastError();
    }

    errors = chain->TrustStatus.dwErrorStatus;

    do {
        /* This seems strange, but that's what tests show */
        if(errors & CERT_TRUST_IS_PARTIAL_CHAIN) {
            WARN("ERROR_INTERNET_SEC_CERT_REV_FAILED\n");
            err = ERROR_INTERNET_SEC_CERT_REV_FAILED;
            if(conn->mask_errors)
                conn->security_flags |= _SECURITY_FLAG_CERT_REV_FAILED;
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_REVOCATION))
                break;
        }

        if (chain->TrustStatus.dwErrorStatus & ~supportedErrors) {
            WARN("error status %x\n", chain->TrustStatus.dwErrorStatus & ~supportedErrors);
            err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
            errors &= supportedErrors;
            if(!conn->mask_errors)
                break;
            WARN("unknown error flags\n");
        }

        if(errors & CERT_TRUST_IS_NOT_TIME_VALID) {
            WARN("CERT_TRUST_IS_NOT_TIME_VALID\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_CERT_DATE_INVALID)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_CERT_DATE_INVALID;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_DATE;
            }
            errors &= ~CERT_TRUST_IS_NOT_TIME_VALID;
        }

        if(errors & CERT_TRUST_IS_UNTRUSTED_ROOT) {
            WARN("CERT_TRUST_IS_UNTRUSTED_ROOT\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_INVALID_CA;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CA;
            }
            errors &= ~CERT_TRUST_IS_UNTRUSTED_ROOT;
        }

        if(errors & CERT_TRUST_IS_PARTIAL_CHAIN) {
            WARN("CERT_TRUST_IS_PARTIAL_CHAIN\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_INVALID_CA;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CA;
            }
            errors &= ~CERT_TRUST_IS_PARTIAL_CHAIN;
        }

        if(errors & CERT_TRUST_IS_NOT_SIGNATURE_VALID) {
            WARN("CERT_TRUST_IS_NOT_SIGNATURE_VALID\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_UNKNOWN_CA)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_INVALID_CA;
                if(!conn->mask_errors)
                    break;
                conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CA;
            }
            errors &= ~CERT_TRUST_IS_NOT_SIGNATURE_VALID;
        }

        if(errors & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) {
            WARN("CERT_TRUST_IS_NOT_VALID_FOR_USAGE\n");
            if(!(conn->security_flags & SECURITY_FLAG_IGNORE_WRONG_USAGE)) {
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
                if(!conn->mask_errors)
                    break;
                WARN("CERT_TRUST_IS_NOT_VALID_FOR_USAGE, unknown error flags\n");
            }
            errors &= ~CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
        }

        if(err == ERROR_INTERNET_SEC_CERT_REV_FAILED) {
            assert(conn->security_flags & SECURITY_FLAG_IGNORE_REVOCATION);
            err = ERROR_SUCCESS;
        }
    }while(0);

    if(!err || conn->mask_errors) {
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
        sslExtraPolicyPara.pwszServerName = conn->server->name;
        sslExtraPolicyPara.fdwChecks = conn->security_flags;
        policyPara.cbSize = sizeof(policyPara);
        policyPara.dwFlags = 0;
        policyPara.pvExtraPolicyPara = &sslExtraPolicyPara;
        ret = CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                &chainCopy, &policyPara, &policyStatus);
        /* Any error in the policy status indicates that the
         * policy couldn't be verified.
         */
        if(ret) {
            if(policyStatus.dwError == CERT_E_CN_NO_MATCH) {
                WARN("CERT_E_CN_NO_MATCH\n");
                if(conn->mask_errors)
                    conn->security_flags |= _SECURITY_FLAG_CERT_INVALID_CN;
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_CERT_CN_INVALID;
            }else if(policyStatus.dwError) {
                WARN("policyStatus.dwError %x\n", policyStatus.dwError);
                if(conn->mask_errors)
                    WARN("unknown error flags for policy status %x\n", policyStatus.dwError);
                err = conn->mask_errors && err ? ERROR_INTERNET_SEC_CERT_ERRORS : ERROR_INTERNET_SEC_INVALID_CERT;
            }
        }else {
            err = GetLastError();
        }
    }

    if(err) {
        WARN("failed %u\n", err);
        CertFreeCertificateChain(chain);
        if(conn->server->cert_chain) {
            CertFreeCertificateChain(conn->server->cert_chain);
            conn->server->cert_chain = NULL;
        }
        if(conn->mask_errors)
            conn->server->security_flags |= conn->security_flags & _SECURITY_ERROR_FLAGS_MASK;
        return err;
    }

    /* FIXME: Reuse cached chain */
    if(conn->server->cert_chain)
        CertFreeCertificateChain(chain);
    else
        conn->server->cert_chain = chain;
    return ERROR_SUCCESS;
}

static SecHandle cred_handle, compat_cred_handle;
static BOOL cred_handle_initialized, have_compat_cred_handle;

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
    SECURITY_STATUS res = SEC_E_OK;

    EnterCriticalSection(&init_sechandle_cs);

    if(!cred_handle_initialized) {
        SCHANNEL_CRED cred = {SCHANNEL_CRED_VERSION};
        SecPkgCred_SupportedProtocols prots;

        res = AcquireCredentialsHandleW(NULL, (WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL, &cred,
                NULL, NULL, &cred_handle, NULL);
        if(res == SEC_E_OK) {
            res = QueryCredentialsAttributesA(&cred_handle, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &prots);
            if(res != SEC_E_OK || (prots.grbitProtocol & SP_PROT_TLS1_1PLUS_CLIENT)) {
                cred.grbitEnabledProtocols = prots.grbitProtocol & ~SP_PROT_TLS1_1PLUS_CLIENT;
                res = AcquireCredentialsHandleW(NULL, (WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL, &cred,
                       NULL, NULL, &compat_cred_handle, NULL);
                have_compat_cred_handle = res == SEC_E_OK;
            }
        }

        cred_handle_initialized = res == SEC_E_OK;
    }

    LeaveCriticalSection(&init_sechandle_cs);

    if(res != SEC_E_OK) {
        WARN("Failed: %08x\n", res);
        return FALSE;
    }

    return TRUE;
}

static BOOL winsock_loaded = FALSE;

static BOOL WINAPI winsock_startup(INIT_ONCE *once, void *param, void **context)
{
    WSADATA wsa_data;
    DWORD res;

    res = WSAStartup(MAKEWORD(1,1), &wsa_data);
    if(res == ERROR_SUCCESS)
        winsock_loaded = TRUE;
    else
        ERR("WSAStartup failed: %u\n", res);
    return TRUE;
}

void init_winsock(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, winsock_startup, NULL, NULL);
}

static void set_socket_blocking(int socket, blocking_mode_t mode)
{
    ULONG arg = mode == BLOCKING_DISALLOW;
    ioctlsocket(socket, FIONBIO, &arg);
}

static DWORD create_netconn_socket(server_t *server, netconn_t *netconn, DWORD timeout)
{
    int result;
    ULONG flag;
    DWORD res;

    init_winsock();

    assert(server->addr_len);
    result = netconn->socket = socket(server->addr.ss_family, SOCK_STREAM, 0);
    if(result != -1) {
        set_socket_blocking(netconn->socket, BLOCKING_DISALLOW);
        result = connect(netconn->socket, (struct sockaddr*)&server->addr, server->addr_len);
        if(result == -1)
        {
            res = sock_get_error();
            if (res == WSAEINPROGRESS || res == WSAEWOULDBLOCK) {
                FD_SET set;
                int res;
                socklen_t len = sizeof(res);
                TIMEVAL timeout_timeval = {0, timeout*1000};

                FD_ZERO(&set);
                FD_SET(netconn->socket, &set);
                res = select(netconn->socket+1, NULL, &set, NULL, &timeout_timeval);
                if(!res || res == SOCKET_ERROR) {
                    closesocket(netconn->socket);
                    netconn->socket = -1;
                    return ERROR_INTERNET_CANNOT_CONNECT;
                }
                if (!getsockopt(netconn->socket, SOL_SOCKET, SO_ERROR, (void *)&res, &len) && !res)
                    result = 0;
            }
        }
        if(result == -1)
        {
            closesocket(netconn->socket);
            netconn->socket = -1;
        }
        else {
            set_socket_blocking(netconn->socket, BLOCKING_ALLOW);
        }
    }
    if(result == -1)
        return ERROR_INTERNET_CANNOT_CONNECT;

    flag = 1;
    result = setsockopt(netconn->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
    if(result < 0)
        WARN("setsockopt(TCP_NODELAY) failed\n");

    return ERROR_SUCCESS;
}

DWORD create_netconn(BOOL useSSL, server_t *server, DWORD security_flags, BOOL mask_errors, DWORD timeout, netconn_t **ret)
{
    netconn_t *netconn;
    int result;

    netconn = heap_alloc_zero(sizeof(*netconn));
    if(!netconn)
        return ERROR_OUTOFMEMORY;

    netconn->socket = -1;
    netconn->security_flags = security_flags | server->security_flags;
    netconn->mask_errors = mask_errors;
    list_init(&netconn->pool_entry);
    SecInvalidateHandle(&netconn->ssl_ctx);

    result = create_netconn_socket(server, netconn, timeout);
    if (result != ERROR_SUCCESS) {
        heap_free(netconn);
        return result;
    }

    server_addref(server);
    netconn->server = server;
    *ret = netconn;
    return result;
}

BOOL is_valid_netconn(netconn_t *netconn)
{
    return netconn && netconn->socket != -1;
}

void close_netconn(netconn_t *netconn)
{
    closesocket(netconn->socket);
    netconn->socket = -1;
}

void free_netconn(netconn_t *netconn)
{
    server_release(netconn->server);

    if (netconn->secure) {
        heap_free(netconn->peek_msg_mem);
        netconn->peek_msg_mem = NULL;
        netconn->peek_msg = NULL;
        netconn->peek_len = 0;
        heap_free(netconn->ssl_buf);
        netconn->ssl_buf = NULL;
        heap_free(netconn->extra_buf);
        netconn->extra_buf = NULL;
        netconn->extra_len = 0;
        if (SecIsValidHandle(&netconn->ssl_ctx))
            DeleteSecurityContext(&netconn->ssl_ctx);
    }

    heap_free(netconn);
}

void NETCON_unload(void)
{
    if(cred_handle_initialized)
        FreeCredentialsHandle(&cred_handle);
    if(have_compat_cred_handle)
        FreeCredentialsHandle(&compat_cred_handle);
    DeleteCriticalSection(&init_sechandle_cs);
    WSACleanup();
}

/* translate a unix error code into a winsock one */
int sock_get_error(void)
{
    return WSAGetLastError();
}

int sock_send(int fd, const void *msg, size_t len, int flags)
{
    int ret;
    do
    {
        ret = send(fd, msg, len, flags);
    }
    while(ret == -1 && sock_get_error() == WSAEINTR);
    return ret;
}

int sock_recv(int fd, void *msg, size_t len, int flags)
{
    int ret;
    do
    {
        ret = recv(fd, msg, len, flags);
    }
    while(ret == -1 && sock_get_error() == WSAEINTR);
    return ret;
}

static DWORD netcon_secure_connect_setup(netconn_t *connection, BOOL compat_mode)
{
    SecBuffer out_buf = {0, SECBUFFER_TOKEN, NULL}, in_bufs[2] = {{0, SECBUFFER_TOKEN}, {0, SECBUFFER_EMPTY}};
    SecBufferDesc out_desc = {SECBUFFER_VERSION, 1, &out_buf}, in_desc = {SECBUFFER_VERSION, 2, in_bufs};
    SecHandle *cred = &cred_handle;
    BYTE *read_buf;
    SIZE_T read_buf_size = 2048;
    ULONG attrs = 0;
    CtxtHandle ctx;
    SSIZE_T size;
    int bits;
    const CERT_CONTEXT *cert;
    SECURITY_STATUS status;
    DWORD res = ERROR_SUCCESS;

    const DWORD isc_req_flags = ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_USE_SESSION_KEY|ISC_REQ_CONFIDENTIALITY
        |ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_MANUAL_CRED_VALIDATION;

    if(!ensure_cred_handle())
        return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;

    if(compat_mode) {
        if(!have_compat_cred_handle)
            return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        cred = &compat_cred_handle;
    }

    read_buf = heap_alloc(read_buf_size);
    if(!read_buf)
        return ERROR_OUTOFMEMORY;

    status = InitializeSecurityContextW(cred, NULL, connection->server->name, isc_req_flags, 0, 0, NULL, 0,
            &ctx, &out_desc, &attrs, NULL);

    assert(status != SEC_E_OK);

    while(status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE) {
        if(out_buf.cbBuffer) {
            assert(status == SEC_I_CONTINUE_NEEDED);

            TRACE("sending %u bytes\n", out_buf.cbBuffer);

            size = sock_send(connection->socket, out_buf.pvBuffer, out_buf.cbBuffer, 0);
            if(size != out_buf.cbBuffer) {
                ERR("send failed\n");
                status = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
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

        size = sock_recv(connection->socket, read_buf+in_bufs[0].cbBuffer, read_buf_size-in_bufs[0].cbBuffer, 0);
        if(size < 1) {
            WARN("recv error\n");
            res = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
            break;
        }

        TRACE("recv %lu bytes\n", size);

        in_bufs[0].cbBuffer += size;
        in_bufs[0].pvBuffer = read_buf;
        status = InitializeSecurityContextW(cred, &ctx, connection->server->name,  isc_req_flags, 0, 0, &in_desc,
                0, NULL, &out_desc, &attrs, NULL);
        TRACE("InitializeSecurityContext ret %08x\n", status);

        if(status == SEC_E_OK) {
            if(SecIsValidHandle(&connection->ssl_ctx))
                DeleteSecurityContext(&connection->ssl_ctx);
            connection->ssl_ctx = ctx;

            if(in_bufs[1].BufferType == SECBUFFER_EXTRA)
                FIXME("SECBUFFER_EXTRA not supported\n");

            status = QueryContextAttributesW(&ctx, SECPKG_ATTR_STREAM_SIZES, &connection->ssl_sizes);
            if(status != SEC_E_OK) {
                WARN("Could not get sizes\n");
                break;
            }

            status = QueryContextAttributesW(&ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
            if(status == SEC_E_OK) {
                res = netconn_verify_cert(connection, cert, cert->hCertStore);
                CertFreeCertificateContext(cert);
                if(res != ERROR_SUCCESS) {
                    WARN("cert verify failed: %u\n", res);
                    break;
                }
            }else {
                WARN("Could not get cert\n");
                break;
            }

            connection->ssl_buf = heap_alloc(connection->ssl_sizes.cbHeader + connection->ssl_sizes.cbMaximumMessage
                    + connection->ssl_sizes.cbTrailer);
            if(!connection->ssl_buf) {
                res = GetLastError();
                break;
            }
        }
    }

    heap_free(read_buf);

    if(status != SEC_E_OK || res != ERROR_SUCCESS) {
        WARN("Failed to establish SSL connection: %08x (%u)\n", status, res);
        heap_free(connection->ssl_buf);
        connection->ssl_buf = NULL;
        return res ? res : ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
    }

    TRACE("established SSL connection\n");
    connection->secure = TRUE;
    connection->security_flags |= SECURITY_FLAG_SECURE;

    bits = NETCON_GetCipherStrength(connection);
    if (bits >= 128)
        connection->security_flags |= SECURITY_FLAG_STRENGTH_STRONG;
    else if (bits >= 56)
        connection->security_flags |= SECURITY_FLAG_STRENGTH_MEDIUM;
    else
        connection->security_flags |= SECURITY_FLAG_STRENGTH_WEAK;

    if(connection->mask_errors)
        connection->server->security_flags = connection->security_flags;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * NETCON_secure_connect
 * Initiates a secure connection over an existing plaintext connection.
 */
DWORD NETCON_secure_connect(netconn_t *connection, server_t *server)
{
    DWORD res;

    /* can't connect if we are already connected */
    if(connection->secure) {
        ERR("already connected\n");
        return ERROR_INTERNET_CANNOT_CONNECT;
    }

    if(server != connection->server) {
        server_release(connection->server);
        server_addref(server);
        connection->server = server;
    }

    /* connect with given TLS options */
    res = netcon_secure_connect_setup(connection, FALSE);
    if (res == ERROR_SUCCESS)
        return res;

    /* FIXME: when got version alert and FIN from server */
    /* fallback to connect without TLSv1.1/TLSv1.2        */
    if (res == ERROR_INTERNET_SECURITY_CHANNEL_ERROR && have_compat_cred_handle)
    {
        closesocket(connection->socket);
        res = create_netconn_socket(connection->server, connection, 500);
        if (res != ERROR_SUCCESS)
            return res;
        res = netcon_secure_connect_setup(connection, TRUE);
    }
    return res;
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

/******************************************************************************
 * NETCON_send
 * Basically calls 'send()' unless we should use SSL
 * number of chars send is put in *sent
 */
DWORD NETCON_send(netconn_t *connection, const void *msg, size_t len, int flags,
		int *sent /* out */)
{
    if(!connection->secure)
    {
	*sent = sock_send(connection->socket, msg, len, flags);
        return *sent == -1 ? sock_get_error() : ERROR_SUCCESS;
    }
    else
    {
        const BYTE *ptr = msg;
        size_t chunk_size;

        *sent = 0;

        while(len) {
            chunk_size = min(len, connection->ssl_sizes.cbMaximumMessage);
            if(!send_ssl_chunk(connection, ptr, chunk_size))
                return ERROR_INTERNET_SECURITY_CHANNEL_ERROR;

            *sent += chunk_size;
            ptr += chunk_size;
            len -= chunk_size;
        }

        return ERROR_SUCCESS;
    }
}

static BOOL read_ssl_chunk(netconn_t *conn, void *buf, SIZE_T buf_size, blocking_mode_t mode, SIZE_T *ret_size, BOOL *eof)
{
    const SIZE_T ssl_buf_size = conn->ssl_sizes.cbHeader+conn->ssl_sizes.cbMaximumMessage+conn->ssl_sizes.cbTrailer;
    SecBuffer bufs[4];
    SecBufferDesc buf_desc = {SECBUFFER_VERSION, sizeof(bufs)/sizeof(*bufs), bufs};
    SSIZE_T size, buf_len = 0;
    blocking_mode_t tmp_mode;
    int i;
    SECURITY_STATUS res;

    assert(conn->extra_len < ssl_buf_size);

    /* BLOCKING_WAITALL is handled by caller */
    if(mode == BLOCKING_WAITALL)
        mode = BLOCKING_ALLOW;

    if(conn->extra_len) {
        memcpy(conn->ssl_buf, conn->extra_buf, conn->extra_len);
        buf_len = conn->extra_len;
        conn->extra_len = 0;
        heap_free(conn->extra_buf);
        conn->extra_buf = NULL;
    }

    tmp_mode = buf_len ? BLOCKING_DISALLOW : mode;
    set_socket_blocking(conn->socket, tmp_mode);
    size = sock_recv(conn->socket, conn->ssl_buf+buf_len, ssl_buf_size-buf_len, 0);
    if(size < 0) {
        if(!buf_len) {
            if(sock_get_error() == WSAEWOULDBLOCK) {
                TRACE("would block\n");
                return WSAEWOULDBLOCK;
            }
            WARN("recv failed\n");
            return ERROR_INTERNET_CONNECTION_ABORTED;
        }
    }else {
        buf_len += size;
    }

    *ret_size = buf_len;

    if(!buf_len) {
        *eof = TRUE;
        return ERROR_SUCCESS;
    }

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
            return ERROR_SUCCESS;
        case SEC_E_INCOMPLETE_MESSAGE:
            assert(buf_len < ssl_buf_size);

            set_socket_blocking(conn->socket, mode);
            size = sock_recv(conn->socket, conn->ssl_buf+buf_len, ssl_buf_size-buf_len, 0);
            if(size < 1) {
                if(size < 0 && sock_get_error() == WSAEWOULDBLOCK) {
                    TRACE("would block\n");

                    /* FIXME: Optimize extra_buf usage. */
                    conn->extra_buf = heap_alloc(buf_len);
                    if(!conn->extra_buf)
                        return ERROR_NOT_ENOUGH_MEMORY;

                    conn->extra_len = buf_len;
                    memcpy(conn->extra_buf, conn->ssl_buf, conn->extra_len);
                    return WSAEWOULDBLOCK;
                }

                return ERROR_INTERNET_CONNECTION_ABORTED;
            }

            buf_len += size;
            continue;
        default:
            WARN("failed: %08x\n", res);
            return ERROR_INTERNET_CONNECTION_ABORTED;
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
                    return ERROR_NOT_ENOUGH_MEMORY;
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
                return ERROR_NOT_ENOUGH_MEMORY;

            conn->extra_len = bufs[i].cbBuffer;
            memcpy(conn->extra_buf, bufs[i].pvBuffer, conn->extra_len);
        }
    }

    return ERROR_SUCCESS;
}

/******************************************************************************
 * NETCON_recv
 * Basically calls 'recv()' unless we should use SSL
 * number of chars received is put in *recvd
 */
DWORD NETCON_recv(netconn_t *connection, void *buf, size_t len, blocking_mode_t mode, int *recvd)
{
    *recvd = 0;
    if (!len)
        return ERROR_SUCCESS;

    if (!connection->secure)
    {
        int flags = 0;

        if(mode == BLOCKING_WAITALL)
            flags = MSG_WAITALL;

        set_socket_blocking(connection->socket, mode);
	*recvd = sock_recv(connection->socket, buf, len, flags);
	return *recvd == -1 ? sock_get_error() :  ERROR_SUCCESS;
    }
    else
    {
        SIZE_T size = 0, cread;
        BOOL eof;
        DWORD res;

        if(connection->peek_msg) {
            size = min(len, connection->peek_len);
            memcpy(buf, connection->peek_msg, size);
            connection->peek_len -= size;
            connection->peek_msg += size;

            if(!connection->peek_len) {
                heap_free(connection->peek_msg_mem);
                connection->peek_msg_mem = connection->peek_msg = NULL;
            }
            /* check if we have enough data from the peek buffer */
            if(mode != BLOCKING_WAITALL || size == len) {
                *recvd = size;
                return ERROR_SUCCESS;
            }

            mode = BLOCKING_DISALLOW;
        }

        do {
            res = read_ssl_chunk(connection, (BYTE*)buf+size, len-size, mode, &cread, &eof);
            if(res != ERROR_SUCCESS) {
                if(res == WSAEWOULDBLOCK) {
                    if(size)
                        res = ERROR_SUCCESS;
                }else {
                    WARN("read_ssl_chunk failed\n");
                }
                break;
            }

            if(eof) {
                TRACE("EOF\n");
                break;
            }

            size += cread;
        }while(!size || (mode == BLOCKING_WAITALL && size < len));

        TRACE("received %ld bytes\n", size);
        *recvd = size;
        return res;
    }
}

/******************************************************************************
 * NETCON_query_data_available
 * Returns the number of bytes of peeked data plus the number of bytes of
 * queued, but unread data.
 */
BOOL NETCON_query_data_available(netconn_t *connection, DWORD *available)
{
    *available = 0;

    if(!connection->secure)
    {
        ULONG unread;
        int retval = ioctlsocket(connection->socket, FIONREAD, &unread);
        if (!retval)
        {
            TRACE("%d bytes of queued, but unread data\n", unread);
            *available += unread;
        }
    }
    else
    {
        *available = connection->peek_len;
    }
    return TRUE;
}

BOOL NETCON_is_alive(netconn_t *netconn)
{
    int len;
    char b;

    set_socket_blocking(netconn->socket, BLOCKING_DISALLOW);
    len = sock_recv(netconn->socket, &b, 1, MSG_PEEK);
    set_socket_blocking(netconn->socket, BLOCKING_ALLOW);

    return len == 1 || (len == -1 && sock_get_error() == WSAEWOULDBLOCK);
}

LPCVOID NETCON_GetCert(netconn_t *connection)
{
    const CERT_CONTEXT *ret;
    SECURITY_STATUS res;

    res = QueryContextAttributesW(&connection->ssl_ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&ret);
    return res == SEC_E_OK ? ret : NULL;
}

int NETCON_GetCipherStrength(netconn_t *connection)
{
    SecPkgContext_ConnectionInfo conn_info;
    SECURITY_STATUS res;

    if (!connection->secure)
        return 0;

    res = QueryContextAttributesW(&connection->ssl_ctx, SECPKG_ATTR_CONNECTION_INFO, (void*)&conn_info);
    if(res != SEC_E_OK)
        WARN("QueryContextAttributesW failed: %08x\n", res);
    return res == SEC_E_OK ? conn_info.dwCipherStrength : 0;
}

DWORD NETCON_set_timeout(netconn_t *connection, BOOL send, DWORD value)
{
    int result;
    struct timeval tv;

    /* value is in milliseconds, convert to struct timeval */
    if (value == INFINITE)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    else
    {
        tv.tv_sec = value / 1000;
        tv.tv_usec = (value % 1000) * 1000;
    }
    result = setsockopt(connection->socket, SOL_SOCKET,
                        send ? SO_SNDTIMEO : SO_RCVTIMEO, (void*)&tv,
                        sizeof(tv));
    if (result == -1)
    {
        WARN("setsockopt failed\n");
        return sock_get_error();
    }
    return ERROR_SUCCESS;
}
