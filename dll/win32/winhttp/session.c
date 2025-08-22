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

#ifdef __REACTOS__
#include "wine/config.h"
#else
#include "config.h"
#endif
#include <stdarg.h>
#include <stdlib.h>

#ifdef HAVE_CORESERVICES_CORESERVICES_H
#define GetCurrentThread MacGetCurrentThread
#define LoadResource MacLoadResource
#include <CoreServices/CoreServices.h>
#undef GetCurrentThread
#undef LoadResource
#endif

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#include "ws2tcpip.h"
#include "winhttp.h"
#include "winreg.h"
#include "wine/winternl.h"
#define COBJMACROS
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define DEFAULT_RESOLVE_TIMEOUT             0
#define DEFAULT_CONNECT_TIMEOUT             20000
#define DEFAULT_SEND_TIMEOUT                30000
#define DEFAULT_RECEIVE_TIMEOUT             30000
#define DEFAULT_RECEIVE_RESPONSE_TIMEOUT    ~0u

void send_callback( struct object_header *hdr, DWORD status, void *info, DWORD buflen )
{
    if (hdr->callback && (hdr->notify_mask & status))
    {
        TRACE("%p, 0x%08x, %p, %u\n", hdr, status, info, buflen);
        hdr->callback( hdr->handle, hdr->context, status, info, buflen );
        TRACE("returning from 0x%08x callback\n", status);
    }
}

/***********************************************************************
 *          WinHttpCheckPlatform (winhttp.@)
 */
BOOL WINAPI WinHttpCheckPlatform( void )
{
    TRACE("\n");
    return TRUE;
}

/***********************************************************************
 *          session_destroy (internal)
 */
static void session_destroy( struct object_header *hdr )
{
    struct session *session = (struct session *)hdr;

    TRACE("%p\n", session);

    if (session->unload_event) SetEvent( session->unload_event );
    destroy_cookies( session );

    session->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &session->cs );
    heap_free( session->agent );
    heap_free( session->proxy_server );
    heap_free( session->proxy_bypass );
    heap_free( session->proxy_username );
    heap_free( session->proxy_password );
    heap_free( session );
}

static BOOL session_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct session *session = (struct session *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        if (!buffer || *buflen < sizeof(DWORD))
        {
            *buflen = sizeof(DWORD);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD *)buffer = hdr->redirect_policy;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = session->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        *(DWORD *)buffer = session->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static BOOL session_set_option( struct object_header *hdr, DWORD option, void *buffer, DWORD buflen )
{
    struct session *session = (struct session *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PROXY:
    {
        WINHTTP_PROXY_INFO *pi = buffer;

        FIXME("%u %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass));
        return TRUE;
    }
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(policy))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_SECURE_PROTOCOLS:
    {
        if (buflen != sizeof(session->secure_protocols))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        EnterCriticalSection( &session->cs );
        session->secure_protocols = *(DWORD *)buffer;
        LeaveCriticalSection( &session->cs );
        TRACE("0x%x\n", session->secure_protocols);
        return TRUE;
    }
    case WINHTTP_OPTION_DISABLE_FEATURE:
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;

    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        session->resolve_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        session->connect_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        session->send_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        session->receive_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        session->receive_response_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH:
        session->passport_flags = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT:
        TRACE("WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT: %p\n", *(HANDLE *)buffer);
        session->unload_event = *(HANDLE *)buffer;
        return TRUE;

    case WINHTTP_OPTION_MAX_CONNS_PER_SERVER:
        FIXME("WINHTTP_OPTION_MAX_CONNS_PER_SERVER: %d\n", *(DWORD *)buffer);
        return TRUE;

    case WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER:
        FIXME("WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER: %d\n", *(DWORD *)buffer);
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        SetLastError( ERROR_WINHTTP_INVALID_OPTION );
        return FALSE;
    }
}

static const struct object_vtbl session_vtbl =
{
    session_destroy,
    session_query_option,
    session_set_option
};

#ifdef __REACTOS__
void winsock_init(void);
#endif

/***********************************************************************
 *          WinHttpOpen (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpen( LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags )
{
    struct session *session;
    HINTERNET handle = NULL;

    TRACE("%s, %u, %s, %s, 0x%08x\n", debugstr_w(agent), access, debugstr_w(proxy), debugstr_w(bypass), flags);

    if (!(session = heap_alloc_zero( sizeof(struct session) ))) return NULL;

    session->hdr.type = WINHTTP_HANDLE_TYPE_SESSION;
    session->hdr.vtbl = &session_vtbl;
    session->hdr.flags = flags;
    session->hdr.refs = 1;
    session->hdr.redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    list_init( &session->hdr.children );
    session->resolve_timeout = DEFAULT_RESOLVE_TIMEOUT;
    session->connect_timeout = DEFAULT_CONNECT_TIMEOUT;
    session->send_timeout = DEFAULT_SEND_TIMEOUT;
    session->receive_timeout = DEFAULT_RECEIVE_TIMEOUT;
    session->receive_response_timeout = DEFAULT_RECEIVE_RESPONSE_TIMEOUT;
    list_init( &session->cookie_cache );
    InitializeCriticalSection( &session->cs );
    session->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": session.cs");

    if (agent && !(session->agent = strdupW( agent ))) goto end;
    if (access == WINHTTP_ACCESS_TYPE_DEFAULT_PROXY)
    {
        WINHTTP_PROXY_INFO info;

        WinHttpGetDefaultProxyConfiguration( &info );
        session->access = info.dwAccessType;
        if (info.lpszProxy && !(session->proxy_server = strdupW( info.lpszProxy )))
        {
            GlobalFree( (LPWSTR)info.lpszProxy );
            GlobalFree( (LPWSTR)info.lpszProxyBypass );
            goto end;
        }
        if (info.lpszProxyBypass && !(session->proxy_bypass = strdupW( info.lpszProxyBypass )))
        {
            GlobalFree( (LPWSTR)info.lpszProxy );
            GlobalFree( (LPWSTR)info.lpszProxyBypass );
            goto end;
        }
    }
    else if (access == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
    {
        session->access = access;
        if (proxy && !(session->proxy_server = strdupW( proxy ))) goto end;
        if (bypass && !(session->proxy_bypass = strdupW( bypass ))) goto end;
    }

    if (!(handle = alloc_handle( &session->hdr ))) goto end;
    session->hdr.handle = handle;

#ifdef __REACTOS__
    winsock_init();
#endif

end:
    release_object( &session->hdr );
    TRACE("returning %p\n", handle);
    if (handle) SetLastError( ERROR_SUCCESS );
    return handle;
}

/***********************************************************************
 *          connect_destroy (internal)
 */
static void connect_destroy( struct object_header *hdr )
{
    struct connect *connect = (struct connect *)hdr;

    TRACE("%p\n", connect);

    release_object( &connect->session->hdr );

    heap_free( connect->hostname );
    heap_free( connect->servername );
    heap_free( connect->username );
    heap_free( connect->password );
    heap_free( connect );
}

static BOOL connect_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct connect *connect = (struct connect *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PARENT_HANDLE:
    {
        if (!buffer || *buflen < sizeof(HINTERNET))
        {
            *buflen = sizeof(HINTERNET);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(HINTERNET *)buffer = ((struct object_header *)connect->session)->handle;
        *buflen = sizeof(HINTERNET);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = connect->session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = connect->session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = connect->session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = connect->session->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        *(DWORD *)buffer = connect->session->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static const struct object_vtbl connect_vtbl =
{
    connect_destroy,
    connect_query_option,
    NULL
};

static BOOL domain_matches(LPCWSTR server, LPCWSTR domain)
{
    static const WCHAR localW[] = { '<','l','o','c','a','l','>',0 };
    BOOL ret = FALSE;

    if (!strcmpiW( domain, localW ) && !strchrW( server, '.' ))
        ret = TRUE;
    else if (*domain == '*')
    {
        if (domain[1] == '.')
        {
            LPCWSTR dot;

            /* For a hostname to match a wildcard, the last domain must match
             * the wildcard exactly.  E.g. if the wildcard is *.a.b, and the
             * hostname is www.foo.a.b, it matches, but a.b does not.
             */
            dot = strchrW( server, '.' );
            if (dot)
            {
                int len = strlenW( dot + 1 );

                if (len > strlenW( domain + 2 ))
                {
                    LPCWSTR ptr;

                    /* The server's domain is longer than the wildcard, so it
                     * could be a subdomain.  Compare the last portion of the
                     * server's domain.
                     */
                    ptr = dot + len + 1 - strlenW( domain + 2 );
                    if (!strcmpiW( ptr, domain + 2 ))
                    {
                        /* This is only a match if the preceding character is
                         * a '.', i.e. that it is a matching domain.  E.g.
                         * if domain is '*.b.c' and server is 'www.ab.c' they
                         * do not match.
                         */
                        ret = *(ptr - 1) == '.';
                    }
                }
                else
                    ret = !strcmpiW( dot + 1, domain + 2 );
            }
        }
    }
    else
        ret = !strcmpiW( server, domain );
    return ret;
}

/* Matches INTERNET_MAX_HOST_NAME_LENGTH in wininet.h, also RFC 1035 */
#define MAX_HOST_NAME_LENGTH 256

static BOOL should_bypass_proxy(struct session *session, LPCWSTR server)
{
    LPCWSTR ptr;
    BOOL ret = FALSE;

    if (!session->proxy_bypass) return FALSE;
    ptr = session->proxy_bypass;
    do {
        LPCWSTR tmp = ptr;

        ptr = strchrW( ptr, ';' );
        if (!ptr)
            ptr = strchrW( tmp, ' ' );
        if (ptr)
        {
            if (ptr - tmp < MAX_HOST_NAME_LENGTH)
            {
                WCHAR domain[MAX_HOST_NAME_LENGTH];

                memcpy( domain, tmp, (ptr - tmp) * sizeof(WCHAR) );
                domain[ptr - tmp] = 0;
                ret = domain_matches( server, domain );
            }
            ptr += 1;
        }
        else if (*tmp)
            ret = domain_matches( server, tmp );
    } while (ptr && !ret);
    return ret;
}

BOOL set_server_for_hostname( struct connect *connect, const WCHAR *server, INTERNET_PORT port )
{
    struct session *session = connect->session;
    BOOL ret = TRUE;

    if (session->proxy_server && !should_bypass_proxy(session, server))
    {
        LPCWSTR colon;

        if ((colon = strchrW( session->proxy_server, ':' )))
        {
            if (!connect->servername || strncmpiW( connect->servername,
                session->proxy_server, colon - session->proxy_server - 1 ))
            {
                heap_free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = heap_alloc(
                    (colon - session->proxy_server + 1) * sizeof(WCHAR) )))
                {
                    ret = FALSE;
                    goto end;
                }
                memcpy( connect->servername, session->proxy_server,
                    (colon - session->proxy_server) * sizeof(WCHAR) );
                connect->servername[colon - session->proxy_server] = 0;
                if (*(colon + 1))
                    connect->serverport = atoiW( colon + 1 );
                else
                    connect->serverport = INTERNET_DEFAULT_PORT;
            }
        }
        else
        {
            if (!connect->servername || strcmpiW( connect->servername,
                session->proxy_server ))
            {
                heap_free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = strdupW( session->proxy_server )))
                {
                    ret = FALSE;
                    goto end;
                }
                connect->serverport = INTERNET_DEFAULT_PORT;
            }
        }
    }
    else if (server)
    {
        heap_free( connect->servername );
        connect->resolved = FALSE;
        if (!(connect->servername = strdupW( server )))
        {
            ret = FALSE;
            goto end;
        }
        connect->serverport = port;
    }
end:
    return ret;
}

/***********************************************************************
 *          WinHttpConnect (winhttp.@)
 */
HINTERNET WINAPI WinHttpConnect( HINTERNET hsession, LPCWSTR server, INTERNET_PORT port, DWORD reserved )
{
    struct connect *connect;
    struct session *session;
    HINTERNET hconnect = NULL;

    TRACE("%p, %s, %u, %x\n", hsession, debugstr_w(server), port, reserved);

    if (!server)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(session = (struct session *)grab_object( hsession )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(connect = heap_alloc_zero( sizeof(struct connect) )))
    {
        release_object( &session->hdr );
        return NULL;
    }
    connect->hdr.type = WINHTTP_HANDLE_TYPE_CONNECT;
    connect->hdr.vtbl = &connect_vtbl;
    connect->hdr.refs = 1;
    connect->hdr.flags = session->hdr.flags;
    connect->hdr.callback = session->hdr.callback;
    connect->hdr.notify_mask = session->hdr.notify_mask;
    connect->hdr.context = session->hdr.context;
    connect->hdr.redirect_policy = session->hdr.redirect_policy;
    list_init( &connect->hdr.children );

    addref_object( &session->hdr );
    connect->session = session;
    list_add_head( &session->hdr.children, &connect->hdr.entry );

    if (!(connect->hostname = strdupW( server ))) goto end;
    connect->hostport = port;
    if (!set_server_for_hostname( connect, server, port )) goto end;

    if (!(hconnect = alloc_handle( &connect->hdr ))) goto end;
    connect->hdr.handle = hconnect;

    send_callback( &session->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hconnect, sizeof(hconnect) );

end:
    release_object( &connect->hdr );
    release_object( &session->hdr );
    TRACE("returning %p\n", hconnect);
    if (hconnect) SetLastError( ERROR_SUCCESS );
    return hconnect;
}

/***********************************************************************
 *          request_destroy (internal)
 */
static void request_destroy( struct object_header *hdr )
{
    struct request *request = (struct request *)hdr;
    unsigned int i, j;

    TRACE("%p\n", request);

#ifdef __REACTOS__
    if (request->task_thread)
#else
    if (request->task_proc_running)
#endif
    {
        /* Signal to the task proc to quit. It will call this again when it does. */
#ifdef __REACTOS__
        HANDLE thread = request->task_thread;
        request->task_thread = 0;
        SetEvent( request->task_cancel );
        CloseHandle( thread );
#else
        request->task_proc_running = FALSE;
        SetEvent( request->task_cancel );
#endif
        return;
    }
    release_object( &request->connect->hdr );

    if (request->cred_handle_initialized) FreeCredentialsHandle( &request->cred_handle );
    CertFreeCertificateContext( request->server_cert );
    CertFreeCertificateContext( request->client_cert );

    destroy_authinfo( request->authinfo );
    destroy_authinfo( request->proxy_authinfo );

    heap_free( request->verb );
    heap_free( request->path );
    heap_free( request->version );
    heap_free( request->raw_headers );
    heap_free( request->status_text );
    for (i = 0; i < request->num_headers; i++)
    {
        heap_free( request->headers[i].field );
        heap_free( request->headers[i].value );
    }
    heap_free( request->headers );
    for (i = 0; i < TARGET_MAX; i++)
    {
        for (j = 0; j < SCHEME_MAX; j++)
        {
            heap_free( request->creds[i][j].username );
            heap_free( request->creds[i][j].password );
        }
    }
    heap_free( request );
}

static void str_to_buffer( WCHAR *buffer, const WCHAR *str, LPDWORD buflen )
{
    int len = 0;
    if (str) len = strlenW( str );
    if (buffer && *buflen > len)
    {
        if (str) memcpy( buffer, str, len * sizeof(WCHAR) );
        buffer[len] = 0;
    }
    *buflen = len * sizeof(WCHAR);
}

static WCHAR *blob_to_str( DWORD encoding, CERT_NAME_BLOB *blob )
{
    WCHAR *ret;
    DWORD size, format = CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG;

    size = CertNameToStrW( encoding, blob, format, NULL, 0 );
    if ((ret = LocalAlloc( 0, size * sizeof(WCHAR) )))
        CertNameToStrW( encoding, blob, format, ret, size );

    return ret;
}

static BOOL copy_sockaddr( const struct sockaddr *addr, SOCKADDR_STORAGE *addr_storage )
{
    switch (addr->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr_storage;

        memcpy( addr_in, addr, sizeof(*addr_in) );
        memset( addr_in + 1, 0, sizeof(*addr_storage) - sizeof(*addr_in) );
        return TRUE;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr_storage;

        memcpy( addr_in6, addr, sizeof(*addr_in6) );
        memset( addr_in6 + 1, 0, sizeof(*addr_storage) - sizeof(*addr_in6) );
        return TRUE;
    }
    default:
        ERR("unhandled family %u\n", addr->sa_family);
        return FALSE;
    }
}

static BOOL request_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct request *request = (struct request *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;
        int bits;

        if (!buffer || *buflen < sizeof(flags))
        {
            *buflen = sizeof(flags);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        flags = request->security_flags;
        if (request->netconn)
        {
            bits = netconn_get_cipher_strength( request->netconn );
            if (bits >= 128)
                flags |= SECURITY_FLAG_STRENGTH_STRONG;
            else if (bits >= 56)
                flags |= SECURITY_FLAG_STRENGTH_MEDIUM;
            else
                flags |= SECURITY_FLAG_STRENGTH_WEAK;
        }
        *(DWORD *)buffer = flags;
        *buflen = sizeof(flags);
        return TRUE;
    }
    case WINHTTP_OPTION_SERVER_CERT_CONTEXT:
    {
        const CERT_CONTEXT *cert;

        if (!buffer || *buflen < sizeof(cert))
        {
            *buflen = sizeof(cert);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        if (!(cert = CertDuplicateCertificateContext( request->server_cert ))) return FALSE;
        *(CERT_CONTEXT **)buffer = (CERT_CONTEXT *)cert;
        *buflen = sizeof(cert);
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT:
    {
        const CERT_CONTEXT *cert = request->server_cert;
        const CRYPT_OID_INFO *oidInfo;
        WINHTTP_CERTIFICATE_INFO *ci = buffer;

        FIXME("partial stub\n");

        if (!buffer || *buflen < sizeof(*ci))
        {
            *buflen = sizeof(*ci);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (!cert) return FALSE;

        ci->ftExpiry = cert->pCertInfo->NotAfter;
        ci->ftStart  = cert->pCertInfo->NotBefore;
        ci->lpszSubjectInfo = blob_to_str( cert->dwCertEncodingType, &cert->pCertInfo->Subject );
        ci->lpszIssuerInfo  = blob_to_str( cert->dwCertEncodingType, &cert->pCertInfo->Issuer );
        ci->lpszProtocolName      = NULL;
        oidInfo = CryptFindOIDInfo( CRYPT_OID_INFO_OID_KEY, cert->pCertInfo->SignatureAlgorithm.pszObjId, 0 );
        if (oidInfo)
            ci->lpszSignatureAlgName = (LPWSTR)oidInfo->pwszName;
        else
            ci->lpszSignatureAlgName  = NULL;
        ci->lpszEncryptionAlgName = NULL;
        ci->dwKeySize = request->netconn ? netconn_get_cipher_strength( request->netconn ) : 0;

        *buflen = sizeof(*ci);
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_KEY_BITNESS:
    {
        if (!buffer || *buflen < sizeof(DWORD))
        {
            *buflen = sizeof(DWORD);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD *)buffer = request->netconn ? netconn_get_cipher_strength( request->netconn ) : 0;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_CONNECTION_INFO:
    {
        WINHTTP_CONNECTION_INFO *info = buffer;
        struct sockaddr local;
        socklen_t len = sizeof(local);
        const struct sockaddr *remote = (const struct sockaddr *)&request->connect->sockaddr;

        if (!buffer || *buflen < sizeof(*info))
        {
            *buflen = sizeof(*info);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (!request->netconn)
        {
            SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_STATE );
            return FALSE;
        }
        if (getsockname( request->netconn->socket, &local, &len )) return FALSE;
        if (!copy_sockaddr( &local, &info->LocalAddress )) return FALSE;
        if (!copy_sockaddr( remote, &info->RemoteAddress )) return FALSE;
        info->cbSize = sizeof(*info);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *(DWORD *)buffer = request->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *(DWORD *)buffer = request->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        *(DWORD *)buffer = request->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *(DWORD *)buffer = request->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        *(DWORD *)buffer = request->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_USERNAME:
        str_to_buffer( buffer, request->connect->username, buflen );
        return TRUE;

    case WINHTTP_OPTION_PASSWORD:
        str_to_buffer( buffer, request->connect->password, buflen );
        return TRUE;

    case WINHTTP_OPTION_PROXY_USERNAME:
        str_to_buffer( buffer, request->connect->session->proxy_username, buflen );
        return TRUE;

    case WINHTTP_OPTION_PROXY_PASSWORD:
        str_to_buffer( buffer, request->connect->session->proxy_password, buflen );
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static WCHAR *buffer_to_str( WCHAR *buffer, DWORD buflen )
{
    WCHAR *ret;
    if ((ret = heap_alloc( (buflen + 1) * sizeof(WCHAR))))
    {
        memcpy( ret, buffer, buflen * sizeof(WCHAR) );
        ret[buflen] = 0;
        return ret;
    }
    SetLastError( ERROR_OUTOFMEMORY );
    return NULL;
}

static BOOL request_set_option( struct object_header *hdr, DWORD option, void *buffer, DWORD buflen )
{
    struct request *request = (struct request *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PROXY:
    {
        WINHTTP_PROXY_INFO *pi = buffer;

        FIXME("%u %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass));
        return TRUE;
    }
    case WINHTTP_OPTION_DISABLE_FEATURE:
    {
        DWORD disable;

        if (buflen != sizeof(DWORD))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        disable = *(DWORD *)buffer;
        TRACE("0x%x\n", disable);
        hdr->disable_flags |= disable;
        return TRUE;
    }
    case WINHTTP_OPTION_AUTOLOGON_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(DWORD))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->logon_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        DWORD policy;

        if (buflen != sizeof(DWORD))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        policy = *(DWORD *)buffer;
        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;
        static const DWORD accepted = SECURITY_FLAG_IGNORE_CERT_CN_INVALID   |
                                      SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                                      SECURITY_FLAG_IGNORE_UNKNOWN_CA        |
                                      SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

        if (buflen < sizeof(DWORD))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        flags = *(DWORD *)buffer;
        TRACE("0x%x\n", flags);
        if (flags && (flags & ~accepted))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        request->security_flags = flags;
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        request->resolve_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        request->connect_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        request->send_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        request->receive_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        request->receive_response_timeout = *(DWORD *)buffer;
        return TRUE;

    case WINHTTP_OPTION_USERNAME:
    {
        struct connect *connect = request->connect;

        heap_free( connect->username );
        if (!(connect->username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PASSWORD:
    {
        struct connect *connect = request->connect;

        heap_free( connect->password );
        if (!(connect->password = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_USERNAME:
    {
        struct session *session = request->connect->session;

        heap_free( session->proxy_username );
        if (!(session->proxy_username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_PASSWORD:
    {
        struct session *session = request->connect->session;

        heap_free( session->proxy_password );
        if (!(session->proxy_password = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_CLIENT_CERT_CONTEXT:
    {
        const CERT_CONTEXT *cert;

        if (!(hdr->flags & WINHTTP_FLAG_SECURE))
        {
            SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_STATE );
            return FALSE;
        }
        if (!buffer)
        {
            CertFreeCertificateContext( request->client_cert );
            request->client_cert = NULL;
        }
        else if (buflen >= sizeof(cert))
        {
            if (!(cert = CertDuplicateCertificateContext( buffer ))) return FALSE;
            CertFreeCertificateContext( request->client_cert );
            request->client_cert = cert;
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if (request->cred_handle_initialized)
        {
            FreeCredentialsHandle( &request->cred_handle );
            request->cred_handle_initialized = FALSE;
        }

        return TRUE;
    }
    case WINHTTP_OPTION_ENABLE_FEATURE:
        if(buflen == sizeof( DWORD ) && *(DWORD *)buffer == WINHTTP_ENABLE_SSL_REVOCATION)
        {
            request->check_revocation = TRUE;
            SetLastError( NO_ERROR );
            return TRUE;
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

    case WINHTTP_OPTION_CONNECT_RETRIES:
        FIXME("WINHTTP_OPTION_CONNECT_RETRIES\n");
        return TRUE;

    default:
        FIXME("unimplemented option %u\n", option);
        SetLastError( ERROR_WINHTTP_INVALID_OPTION );
        return FALSE;
    }
}

static const struct object_vtbl request_vtbl =
{
    request_destroy,
    request_query_option,
    request_set_option
};

static BOOL add_accept_types_header( struct request *request, const WCHAR **types )
{
    static const WCHAR acceptW[] = {'A','c','c','e','p','t',0};
    static const DWORD flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA;

    if (!types) return TRUE;
    while (*types)
    {
        if (!process_header( request, acceptW, *types, flags, TRUE )) return FALSE;
        types++;
    }
    return TRUE;
}

static WCHAR *get_request_path( const WCHAR *object )
{
    int len = object ? strlenW(object) : 0;
    WCHAR *p, *ret;

    if (!object || object[0] != '/') len++;
    if (!(p = ret = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
    if (!object || object[0] != '/') *p++ = '/';
    if (object) strcpyW( p, object );
    ret[len] = 0;
    return ret;
}

/***********************************************************************
 *          WinHttpOpenRequest (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpenRequest( HINTERNET hconnect, LPCWSTR verb, LPCWSTR object, LPCWSTR version,
                                     LPCWSTR referrer, LPCWSTR *types, DWORD flags )
{
    struct request *request;
    struct connect *connect;
    HINTERNET hrequest = NULL;

    TRACE("%p, %s, %s, %s, %s, %p, 0x%08x\n", hconnect, debugstr_w(verb), debugstr_w(object),
          debugstr_w(version), debugstr_w(referrer), types, flags);

    if (types && TRACE_ON(winhttp))
    {
        const WCHAR **iter;
        TRACE("accept types:\n");
        for (iter = types; *iter; iter++) TRACE("    %s\n", debugstr_w(*iter));
    }

    if (!(connect = (struct connect *)grab_object( hconnect )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (connect->hdr.type != WINHTTP_HANDLE_TYPE_CONNECT)
    {
        release_object( &connect->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(request = heap_alloc_zero( sizeof(struct request) )))
    {
        release_object( &connect->hdr );
        return NULL;
    }
    request->hdr.type = WINHTTP_HANDLE_TYPE_REQUEST;
    request->hdr.vtbl = &request_vtbl;
    request->hdr.refs = 1;
    request->hdr.flags = flags;
    request->hdr.callback = connect->hdr.callback;
    request->hdr.notify_mask = connect->hdr.notify_mask;
    request->hdr.context = connect->hdr.context;
    request->hdr.redirect_policy = connect->hdr.redirect_policy;
    list_init( &request->hdr.children );
    list_init( &request->task_queue );

    addref_object( &connect->hdr );
    request->connect = connect;
    list_add_head( &connect->hdr.children, &request->hdr.entry );

    request->resolve_timeout = connect->session->resolve_timeout;
    request->connect_timeout = connect->session->connect_timeout;
    request->send_timeout = connect->session->send_timeout;
    request->receive_timeout = connect->session->receive_timeout;
    request->receive_response_timeout = connect->session->receive_response_timeout;

    if (!verb || !verb[0]) verb = getW;
    if (!(request->verb = strdupW( verb ))) goto end;
    if (!(request->path = get_request_path( object ))) goto end;

    if (!version || !version[0]) version = http1_1;
    if (!(request->version = strdupW( version ))) goto end;
    if (!(add_accept_types_header( request, types ))) goto end;

    if (!(hrequest = alloc_handle( &request->hdr ))) goto end;
    request->hdr.handle = hrequest;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hrequest, sizeof(hrequest) );

end:
    release_object( &request->hdr );
    release_object( &connect->hdr );
    TRACE("returning %p\n", hrequest);
    if (hrequest) SetLastError( ERROR_SUCCESS );
    return hrequest;
}

/***********************************************************************
 *          WinHttpCloseHandle (winhttp.@)
 */
BOOL WINAPI WinHttpCloseHandle( HINTERNET handle )
{
    struct object_header *hdr;

    TRACE("%p\n", handle);

    if (!(hdr = grab_object( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    release_object( hdr );
    free_handle( handle );
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

static BOOL query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    BOOL ret = FALSE;

    if (!buflen)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        if (!buffer || *buflen < sizeof(DWORD_PTR))
        {
            *buflen = sizeof(DWORD_PTR);
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(DWORD_PTR *)buffer = hdr->context;
        *buflen = sizeof(DWORD_PTR);
        return TRUE;
    }
    default:
        if (hdr->vtbl->query_option) ret = hdr->vtbl->query_option( hdr, option, buffer, buflen );
        else
        {
            FIXME("unimplemented option %u\n", option);
            SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
            return FALSE;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryOption (winhttp.@)
 */
BOOL WINAPI WinHttpQueryOption( HINTERNET handle, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    BOOL ret = FALSE;
    struct object_header *hdr;

    TRACE("%p, %u, %p, %p\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = query_option( hdr, option, buffer, buflen );

    release_object( hdr );
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

static BOOL set_option( struct object_header *hdr, DWORD option, void *buffer, DWORD buflen )
{
    BOOL ret = TRUE;

    if (!buffer && buflen)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        if (buflen != sizeof(DWORD_PTR))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        hdr->context = *(DWORD_PTR *)buffer;
        return TRUE;
    }
    default:
        if (hdr->vtbl->set_option) ret = hdr->vtbl->set_option( hdr, option, buffer, buflen );
        else
        {
            FIXME("unimplemented option %u\n", option);
            SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
            return FALSE;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpSetOption (winhttp.@)
 */
BOOL WINAPI WinHttpSetOption( HINTERNET handle, DWORD option, LPVOID buffer, DWORD buflen )
{
    BOOL ret = FALSE;
    struct object_header *hdr;

    TRACE("%p, %u, %p, %u\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = set_option( hdr, option, buffer, buflen );

    release_object( hdr );
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

static char *get_computer_name( COMPUTER_NAME_FORMAT format )
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( format, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = heap_alloc( size ))) return NULL;
    if (!GetComputerNameExA( format, ret, &size ))
    {
        heap_free( ret );
        return NULL;
    }
    return ret;
}

static BOOL is_domain_suffix( const char *domain, const char *suffix )
{
    int len_domain = strlen( domain ), len_suffix = strlen( suffix );

    if (len_suffix > len_domain) return FALSE;
    if (!_strnicmp( domain + len_domain - len_suffix, suffix, -1 )) return TRUE;
    return FALSE;
}

static int reverse_lookup( const struct addrinfo *ai, char *hostname, size_t len )
{
    return getnameinfo( ai->ai_addr, ai->ai_addrlen, hostname, len, NULL, 0, 0 );
}

static WCHAR *build_wpad_url( const char *hostname, const struct addrinfo *ai )
{
    static const WCHAR httpW[] = {'h','t','t','p',':','/','/',0};
    static const WCHAR wpadW[] = {'/','w','p','a','d','.','d','a','t',0};
    char name[NI_MAXHOST];
    WCHAR *ret, *p;
    int len;

    while (ai && ai->ai_family != AF_INET && ai->ai_family != AF_INET6) ai = ai->ai_next;
    if (!ai) return NULL;

    if (!reverse_lookup( ai, name, sizeof(name) )) hostname = name;

    len = strlenW( httpW ) + strlen( hostname ) + strlenW( wpadW );
    if (!(ret = p = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
    strcpyW( p, httpW );
    p += strlenW( httpW );
    while (*hostname) { *p++ = *hostname++; }
    strcpyW( p, wpadW );
    return ret;
}

static BOOL get_system_proxy_autoconfig_url( char *buf, DWORD buflen )
{
#if defined(MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
    CFDictionaryRef settings = CFNetworkCopySystemProxySettings();
    const void *ref;
    BOOL ret = FALSE;

    if (!settings) return FALSE;

    if (!(ref = CFDictionaryGetValue( settings, kCFNetworkProxiesProxyAutoConfigURLString )))
    {
        CFRelease( settings );
        return FALSE;
    }
    if (CFStringGetCString( ref, buf, buflen, kCFStringEncodingASCII ))
    {
        TRACE( "returning %s\n", debugstr_a(buf) );
        ret = TRUE;
    }
    CFRelease( settings );
    return ret;
#else
    static BOOL first = TRUE;
    if (first)
    {
        FIXME( "no support on this platform\n" );
        first = FALSE;
    }
    else
        TRACE( "no support on this platform\n" );
    return FALSE;
#endif
}

#define INTERNET_MAX_URL_LENGTH 2084

/***********************************************************************
 *          WinHttpDetectAutoProxyConfigUrl (winhttp.@)
 */
BOOL WINAPI WinHttpDetectAutoProxyConfigUrl( DWORD flags, LPWSTR *url )
{
    BOOL ret = FALSE;
    char system_url[INTERNET_MAX_URL_LENGTH + 1];

    TRACE("0x%08x, %p\n", flags, url);

    if (!flags || !url)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (get_system_proxy_autoconfig_url( system_url, sizeof(system_url) ))
    {
        WCHAR *urlW;

        if (!(urlW = strdupAW( system_url ))) return FALSE;
        *url = urlW;
        SetLastError( ERROR_SUCCESS );
        return TRUE;
    }
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DHCP)
    {
        static int fixme_shown;
        if (!fixme_shown++) FIXME("discovery via DHCP not supported\n");
    }
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DNS_A)
    {
        char *fqdn, *domain, *p;

        if (!(fqdn = get_computer_name( ComputerNamePhysicalDnsFullyQualified ))) return FALSE;
        if (!(domain = get_computer_name( ComputerNamePhysicalDnsDomain )))
        {
            heap_free( fqdn );
            return FALSE;
        }
        p = fqdn;
        while ((p = strchr( p, '.' )) && is_domain_suffix( p + 1, domain ))
        {
            struct addrinfo *ai;
            char *name;
            int res;

            if (!(name = heap_alloc( sizeof("wpad") + strlen(p) )))
            {
                heap_free( fqdn );
                heap_free( domain );
                return FALSE;
            }
            strcpy( name, "wpad" );
            strcat( name, p );
            res = getaddrinfo( name, NULL, NULL, &ai );
            if (!res)
            {
                *url = build_wpad_url( name, ai );
                freeaddrinfo( ai );
                if (*url)
                {
                    TRACE("returning %s\n", debugstr_w(*url));
                    heap_free( name );
                    ret = TRUE;
                    break;
                }
            }
            heap_free( name );
            p++;
        }
        heap_free( domain );
        heap_free( fqdn );
    }
    if (!ret)
    {
        SetLastError( ERROR_WINHTTP_AUTODETECTION_FAILED );
        *url = NULL;
    }
    else SetLastError( ERROR_SUCCESS );
    return ret;
}

static const WCHAR Connections[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
    'C','o','n','n','e','c','t','i','o','n','s',0 };
static const WCHAR WinHttpSettings[] = {
    'W','i','n','H','t','t','p','S','e','t','t','i','n','g','s',0 };
static const DWORD WINHTTP_SETTINGS_MAGIC = 0x18;
static const DWORD WININET_SETTINGS_MAGIC = 0x46;
static const DWORD PROXY_TYPE_DIRECT         = 1;
static const DWORD PROXY_TYPE_PROXY          = 2;
static const DWORD PROXY_USE_PAC_SCRIPT      = 4;
static const DWORD PROXY_AUTODETECT_SETTINGS = 8;

struct connection_settings_header
{
    DWORD magic;
    DWORD unknown; /* always zero? */
    DWORD flags;   /* one or more of PROXY_* */
};

static inline void copy_char_to_wchar_sz(const BYTE *src, DWORD len, WCHAR *dst)
{
    const BYTE *begin;

    for (begin = src; src - begin < len; src++, dst++)
        *dst = *src;
    *dst = 0;
}

/***********************************************************************
 *          WinHttpGetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpGetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    LONG l;
    HKEY key;
    BOOL got_from_reg = FALSE, direct = TRUE;
    char *envproxy;

    TRACE("%p\n", info);

    l = RegOpenKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, KEY_READ, &key );
    if (!l)
    {
        DWORD type, size = 0;

        l = RegQueryValueExW( key, WinHttpSettings, NULL, &type, NULL, &size );
        if (!l && type == REG_BINARY &&
            size >= sizeof(struct connection_settings_header) + 2 * sizeof(DWORD))
        {
            BYTE *buf = heap_alloc( size );

            if (buf)
            {
                struct connection_settings_header *hdr =
                    (struct connection_settings_header *)buf;
                DWORD *len = (DWORD *)(hdr + 1);

                l = RegQueryValueExW( key, WinHttpSettings, NULL, NULL, buf,
                    &size );
                if (!l && hdr->magic == WINHTTP_SETTINGS_MAGIC &&
                    hdr->unknown == 0)
                {
                    if (hdr->flags & PROXY_TYPE_PROXY)
                    {
                       BOOL sane = FALSE;
                       LPWSTR proxy = NULL;
                       LPWSTR proxy_bypass = NULL;

                        /* Sanity-check length of proxy string */
                        if ((BYTE *)len - buf + *len <= size)
                        {
                            sane = TRUE;
                            proxy = GlobalAlloc( 0, (*len + 1) * sizeof(WCHAR) );
                            if (proxy)
                                copy_char_to_wchar_sz( (BYTE *)(len + 1), *len, proxy );
                            len = (DWORD *)((BYTE *)(len + 1) + *len);
                        }
                        if (sane)
                        {
                            /* Sanity-check length of proxy bypass string */
                            if ((BYTE *)len - buf + *len <= size)
                            {
                                proxy_bypass = GlobalAlloc( 0, (*len + 1) * sizeof(WCHAR) );
                                if (proxy_bypass)
                                    copy_char_to_wchar_sz( (BYTE *)(len + 1), *len, proxy_bypass );
                            }
                            else
                            {
                                sane = FALSE;
                                GlobalFree( proxy );
                                proxy = NULL;
                            }
                        }
                        info->lpszProxy = proxy;
                        info->lpszProxyBypass = proxy_bypass;
                        if (sane)
                        {
                            got_from_reg = TRUE;
                            direct = FALSE;
                            info->dwAccessType =
                                WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                            TRACE("http proxy (from registry) = %s, bypass = %s\n",
                                debugstr_w(info->lpszProxy),
                                debugstr_w(info->lpszProxyBypass));
                        }
                    }
                }
                heap_free( buf );
            }
        }
        RegCloseKey( key );
    }
    if (!got_from_reg && (envproxy = getenv( "http_proxy" )))
    {
        char *colon, *http_proxy = NULL;

        if (!(colon = strchr( envproxy, ':' ))) http_proxy = envproxy;
        else
        {
            if (*(colon + 1) == '/' && *(colon + 2) == '/')
            {
                /* It's a scheme, check that it's http */
                if (!strncmp( envproxy, "http://", 7 )) http_proxy = envproxy + 7;
                else WARN("unsupported scheme in $http_proxy: %s\n", envproxy);
            }
            else http_proxy = envproxy;
        }

        if (http_proxy && http_proxy[0])
        {
            WCHAR *http_proxyW;
            int len;

            len = MultiByteToWideChar( CP_UNIXCP, 0, http_proxy, -1, NULL, 0 );
            if ((http_proxyW = GlobalAlloc( 0, len * sizeof(WCHAR))))
            {
                MultiByteToWideChar( CP_UNIXCP, 0, http_proxy, -1, http_proxyW, len );
                direct = FALSE;
                info->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                info->lpszProxy = http_proxyW;
                info->lpszProxyBypass = NULL;
                TRACE("http proxy (from environment) = %s\n", debugstr_w(info->lpszProxy));
            }
        }
    }
    if (direct)
    {
        info->dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
        info->lpszProxy       = NULL;
        info->lpszProxyBypass = NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

/***********************************************************************
 *          WinHttpGetIEProxyConfigForCurrentUser (winhttp.@)
 */
BOOL WINAPI WinHttpGetIEProxyConfigForCurrentUser( WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *config )
{
    static const WCHAR settingsW[] =
        {'D','e','f','a','u','l','t','C','o','n','n','e','c','t','i','o','n','S','e','t','t','i','n','g','s',0};
    HKEY hkey = NULL;
    struct connection_settings_header *hdr = NULL;
    DWORD type, offset, len, size = 0;
    BOOL ret = FALSE;

    TRACE("%p\n", config);

    if (!config)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    memset( config, 0, sizeof(*config) );
    config->fAutoDetect = TRUE;

    if (RegOpenKeyExW( HKEY_CURRENT_USER, Connections, 0, KEY_READ, &hkey ) ||
        RegQueryValueExW( hkey, settingsW, NULL, &type, NULL, &size ) ||
        type != REG_BINARY || size < sizeof(struct connection_settings_header))
    {
        ret = TRUE;
        goto done;
    }
    if (!(hdr = heap_alloc( size ))) goto done;
    if (RegQueryValueExW( hkey, settingsW, NULL, &type, (BYTE *)hdr, &size ) ||
        hdr->magic != WININET_SETTINGS_MAGIC)
    {
        ret = TRUE;
        goto done;
    }

    config->fAutoDetect = (hdr->flags & PROXY_AUTODETECT_SETTINGS) != 0;
    offset = sizeof(*hdr);
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && hdr->flags & PROXY_TYPE_PROXY)
    {
        if (!(config->lpszProxy = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszProxy );
    }
    offset += len;
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && (hdr->flags & PROXY_TYPE_PROXY))
    {
        if (!(config->lpszProxyBypass = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszProxyBypass );
    }
    offset += len;
    if (offset + sizeof(DWORD) > size) goto done;
    len = *(DWORD *)((char *)hdr + offset);
    offset += sizeof(DWORD);
    if (len && (hdr->flags & PROXY_USE_PAC_SCRIPT))
    {
        if (!(config->lpszAutoConfigUrl = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        copy_char_to_wchar_sz( (const BYTE *)hdr + offset , len, config->lpszAutoConfigUrl );
    }
    ret = TRUE;

done:
    RegCloseKey( hkey );
    heap_free( hdr );
    if (!ret)
    {
        GlobalFree( config->lpszAutoConfigUrl );
        config->lpszAutoConfigUrl = NULL;
        GlobalFree( config->lpszProxy );
        config->lpszProxy = NULL;
        GlobalFree( config->lpszProxyBypass );
        config->lpszProxyBypass = NULL;
    }
    else SetLastError( ERROR_SUCCESS );
    return ret;
}

static BOOL parse_script_result( const char *result, WINHTTP_PROXY_INFO *info )
{
    const char *p;
    WCHAR *q;
    int len;

    info->dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
    info->lpszProxy       = NULL;
    info->lpszProxyBypass = NULL;

    TRACE("%s\n", debugstr_a( result ));

    p = result;
    while (*p == ' ') p++;
    len = strlen( p );
    if (len >= 5 && !_strnicmp( p, "PROXY", sizeof("PROXY") - 1 ))
    {
        p += 5;
        while (*p == ' ') p++;
        if (!*p || *p == ';') return TRUE;
        if (!(info->lpszProxy = q = strdupAW( p ))) return FALSE;
        info->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        for (; *q; q++)
        {
            if (*q == ' ' || *q == ';')
            {
                *q = 0;
                break;
            }
        }
    }
    return TRUE;
}

static char *download_script( const WCHAR *url, DWORD *out_size )
{
    static const WCHAR typeW[] = {'*','/','*',0};
    static const WCHAR *acceptW[] = {typeW, NULL};
    HINTERNET ses, con = NULL, req = NULL;
    WCHAR *hostname;
    URL_COMPONENTSW uc;
    DWORD status, size = sizeof(status), offset, to_read, bytes_read, flags = 0;
    char *tmp, *buffer = NULL;

    *out_size = 0;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = -1;
    uc.dwUrlPathLength = -1;
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return NULL;
    if (!(hostname = heap_alloc( (uc.dwHostNameLength + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    if (!(ses = WinHttpOpen( NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 ))) goto done;
    if (!(con = WinHttpConnect( ses, hostname, uc.nPort, 0 ))) goto done;
    if (uc.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;
    if (!(req = WinHttpOpenRequest( con, NULL, uc.lpszUrlPath, NULL, NULL, acceptW, flags ))) goto done;
    if (!WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 )) goto done;

    if (!(WinHttpReceiveResponse( req, 0 ))) goto done;
    if (!WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status,
        &size, NULL ) || status != HTTP_STATUS_OK) goto done;

    size = 4096;
    if (!(buffer = heap_alloc( size ))) goto done;
    to_read = size;
    offset = 0;
    for (;;)
    {
        if (!WinHttpReadData( req, buffer + offset, to_read, &bytes_read )) goto done;
        if (!bytes_read) break;
        to_read -= bytes_read;
        offset += bytes_read;
        *out_size += bytes_read;
        if (!to_read)
        {
            to_read = size;
            size *= 2;
            if (!(tmp = heap_realloc( buffer, size ))) goto done;
            buffer = tmp;
        }
    }

done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
    heap_free( hostname );
    if (!buffer) SetLastError( ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT );
    return buffer;
}

struct AUTO_PROXY_SCRIPT_BUFFER
{
    DWORD dwStructSize;
    LPSTR lpszScriptBuffer;
    DWORD dwScriptBufferSize;
};

BOOL WINAPI InternetDeInitializeAutoProxyDll(LPSTR, DWORD);
BOOL WINAPI InternetGetProxyInfo(LPCSTR, DWORD, LPSTR, DWORD, LPSTR *, LPDWORD);
BOOL WINAPI InternetInitializeAutoProxyDll(DWORD, LPSTR, LPSTR, void *, struct AUTO_PROXY_SCRIPT_BUFFER *);

static BOOL run_script( char *script, DWORD size, const WCHAR *url, WINHTTP_PROXY_INFO *info )
{
    BOOL ret;
    char *result, *urlA;
    DWORD len_result;
    struct AUTO_PROXY_SCRIPT_BUFFER buffer;
    URL_COMPONENTSW uc;

    buffer.dwStructSize = sizeof(buffer);
    buffer.lpszScriptBuffer = script;
    buffer.dwScriptBufferSize = size;

    if (!(urlA = strdupWA( url ))) return FALSE;
    if (!(ret = InternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buffer )))
    {
        heap_free( urlA );
        return FALSE;
    }

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = -1;

    if (WinHttpCrackUrl( url, 0, 0, &uc ))
    {
        char *hostnameA = strdupWA_sized( uc.lpszHostName, uc.dwHostNameLength );

        if ((ret = InternetGetProxyInfo( urlA, strlen(urlA),
                        hostnameA, strlen(hostnameA), &result, &len_result )))
        {
            ret = parse_script_result( result, info );
            heap_free( result );
        }

        heap_free( hostnameA );
    }
    heap_free( urlA );
    return InternetDeInitializeAutoProxyDll( NULL, 0 );
}

/***********************************************************************
 *          WinHttpGetProxyForUrl (winhttp.@)
 */
BOOL WINAPI WinHttpGetProxyForUrl( HINTERNET hsession, LPCWSTR url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                   WINHTTP_PROXY_INFO *info )
{
    WCHAR *detected_pac_url = NULL;
    const WCHAR *pac_url;
    struct session *session;
    char *script;
    DWORD size;
    BOOL ret = FALSE;

    TRACE("%p, %s, %p, %p\n", hsession, debugstr_w(url), options, info);

    if (!(session = (struct session *)grab_object( hsession )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }
    if (!url || !options || !info ||
        !(options->dwFlags & (WINHTTP_AUTOPROXY_AUTO_DETECT|WINHTTP_AUTOPROXY_CONFIG_URL)) ||
        ((options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) && !options->dwAutoDetectFlags) ||
        ((options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT) &&
         (options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL)))
    {
        release_object( &session->hdr );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT &&
        !WinHttpDetectAutoProxyConfigUrl( options->dwAutoDetectFlags, &detected_pac_url ))
        goto done;

    if (options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL) pac_url = options->lpszAutoConfigUrl;
    else pac_url = detected_pac_url;

    if ((script = download_script( pac_url, &size )))
    {
        ret = run_script( script, size, url, info );
        heap_free( script );
    }

done:
    GlobalFree( detected_pac_url );
    release_object( &session->hdr );
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

/***********************************************************************
 *          WinHttpSetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpSetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    LONG l;
    HKEY key;
    BOOL ret = FALSE;
    const WCHAR *src;

    TRACE("%p\n", info);

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (info->dwAccessType)
    {
    case WINHTTP_ACCESS_TYPE_NO_PROXY:
        break;
    case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
        if (!info->lpszProxy)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        /* Only ASCII characters are allowed */
        for (src = info->lpszProxy; *src; src++)
            if (*src > 0x7f)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        if (info->lpszProxyBypass)
        {
            for (src = info->lpszProxyBypass; *src; src++)
                if (*src > 0x7f)
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
        }
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    l = RegCreateKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, NULL, 0,
        KEY_WRITE, NULL, &key, NULL );
    if (!l)
    {
        DWORD size = sizeof(struct connection_settings_header) + 2 * sizeof(DWORD);
        BYTE *buf;

        if (info->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
        {
            size += strlenW( info->lpszProxy );
            if (info->lpszProxyBypass)
                size += strlenW( info->lpszProxyBypass );
        }
        buf = heap_alloc( size );
        if (buf)
        {
            struct connection_settings_header *hdr =
                (struct connection_settings_header *)buf;
            DWORD *len = (DWORD *)(hdr + 1);

            hdr->magic = WINHTTP_SETTINGS_MAGIC;
            hdr->unknown = 0;
            if (info->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
            {
                BYTE *dst;

                hdr->flags = PROXY_TYPE_PROXY;
                *len++ = strlenW( info->lpszProxy );
                for (dst = (BYTE *)len, src = info->lpszProxy; *src;
                    src++, dst++)
                    *dst = *src;
                len = (DWORD *)dst;
                if (info->lpszProxyBypass)
                {
                    *len++ = strlenW( info->lpszProxyBypass );
                    for (dst = (BYTE *)len, src = info->lpszProxyBypass; *src;
                        src++, dst++)
                        *dst = *src;
                }
                else
                    *len++ = 0;
            }
            else
            {
                hdr->flags = PROXY_TYPE_DIRECT;
                *len++ = 0;
                *len++ = 0;
            }
            l = RegSetValueExW( key, WinHttpSettings, 0, REG_BINARY, buf, size );
            if (!l)
                ret = TRUE;
            heap_free( buf );
        }
        RegCloseKey( key );
    }
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

/***********************************************************************
 *          WinHttpSetStatusCallback (winhttp.@)
 */
WINHTTP_STATUS_CALLBACK WINAPI WinHttpSetStatusCallback( HINTERNET handle, WINHTTP_STATUS_CALLBACK callback,
                                                         DWORD flags, DWORD_PTR reserved )
{
    struct object_header *hdr;
    WINHTTP_STATUS_CALLBACK ret;

    TRACE("%p, %p, 0x%08x, 0x%lx\n", handle, callback, flags, reserved);

    if (!(hdr = grab_object( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return WINHTTP_INVALID_STATUS_CALLBACK;
    }
    ret = hdr->callback;
    hdr->callback = callback;
    hdr->notify_mask = flags;

    release_object( hdr );
    SetLastError( ERROR_SUCCESS );
    return ret;
}

/***********************************************************************
 *          WinHttpSetTimeouts (winhttp.@)
 */
BOOL WINAPI WinHttpSetTimeouts( HINTERNET handle, int resolve, int connect, int send, int receive )
{
    BOOL ret = TRUE;
    struct object_header *hdr;

    TRACE("%p, %d, %d, %d, %d\n", handle, resolve, connect, send, receive);

    if (resolve < -1 || connect < -1 || send < -1 || receive < -1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(hdr = grab_object( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    switch(hdr->type)
    {
    case WINHTTP_HANDLE_TYPE_REQUEST:
    {
        struct request *request = (struct request *)hdr;
        request->connect_timeout = connect;

        if (resolve < 0) resolve = 0;
        request->resolve_timeout = resolve;

        if (send < 0) send = 0;
        request->send_timeout = send;

        if (receive < 0) receive = 0;
        request->receive_timeout = receive;

        if (request->netconn)
        {
            if (netconn_set_timeout( request->netconn, TRUE, send )) ret = FALSE;
            if (netconn_set_timeout( request->netconn, FALSE, receive )) ret = FALSE;
        }
        break;
    }
    case WINHTTP_HANDLE_TYPE_SESSION:
    {
        struct session *session = (struct session *)hdr;
        session->connect_timeout = connect;

        if (resolve < 0) resolve = 0;
        session->resolve_timeout = resolve;

        if (send < 0) send = 0;
        session->send_timeout = send;

        if (receive < 0) receive = 0;
        session->receive_timeout = receive;
        break;
    }
    default:
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        ret = FALSE;
    }
    release_object( hdr );
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

static const WCHAR wkday[7][4] =
    {{'S','u','n', 0}, {'M','o','n', 0}, {'T','u','e', 0}, {'W','e','d', 0},
     {'T','h','u', 0}, {'F','r','i', 0}, {'S','a','t', 0}};
static const WCHAR month[12][4] =
    {{'J','a','n', 0}, {'F','e','b', 0}, {'M','a','r', 0}, {'A','p','r', 0},
     {'M','a','y', 0}, {'J','u','n', 0}, {'J','u','l', 0}, {'A','u','g', 0},
     {'S','e','p', 0}, {'O','c','t', 0}, {'N','o','v', 0}, {'D','e','c', 0}};

/***********************************************************************
 *           WinHttpTimeFromSystemTime (WININET.@)
 */
BOOL WINAPI WinHttpTimeFromSystemTime( const SYSTEMTIME *time, LPWSTR string )
{
    static const WCHAR format[] =
        {'%','s',',',' ','%','0','2','d',' ','%','s',' ','%','4','d',' ','%','0',
         '2','d',':','%','0','2','d',':','%','0','2','d',' ','G','M','T', 0};

    TRACE("%p, %p\n", time, string);

    if (!time || !string)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    sprintfW( string, format,
              wkday[time->wDayOfWeek],
              time->wDay,
              month[time->wMonth - 1],
              time->wYear,
              time->wHour,
              time->wMinute,
              time->wSecond );

    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

/***********************************************************************
 *           WinHttpTimeToSystemTime (WININET.@)
 */
BOOL WINAPI WinHttpTimeToSystemTime( LPCWSTR string, SYSTEMTIME *time )
{
    unsigned int i;
    const WCHAR *s = string;
    WCHAR *end;

    TRACE("%s, %p\n", debugstr_w(string), time);

    if (!string || !time)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Windows does this too */
    GetSystemTime( time );

    /*  Convert an RFC1123 time such as 'Fri, 07 Jan 2005 12:06:35 GMT' into
     *  a SYSTEMTIME structure.
     */

    SetLastError( ERROR_SUCCESS );

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wDayOfWeek = 7;

    for (i = 0; i < 7; i++)
    {
        if (toupperW( wkday[i][0] ) == toupperW( s[0] ) &&
            toupperW( wkday[i][1] ) == toupperW( s[1] ) &&
            toupperW( wkday[i][2] ) == toupperW( s[2] ) )
        {
            time->wDayOfWeek = i;
            break;
        }
    }

    if (time->wDayOfWeek > 6) return TRUE;
    while (*s && !isdigitW( *s )) s++;
    time->wDay = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wMonth = 0;

    for (i = 0; i < 12; i++)
    {
        if (toupperW( month[i][0]) == toupperW( s[0] ) &&
            toupperW( month[i][1]) == toupperW( s[1] ) &&
            toupperW( month[i][2]) == toupperW( s[2] ) )
        {
            time->wMonth = i + 1;
            break;
        }
    }
    if (time->wMonth == 0) return TRUE;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wYear = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wHour = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wMinute = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wSecond = strtolW( s, &end, 10 );

    time->wMilliseconds = 0;
    return TRUE;
}
