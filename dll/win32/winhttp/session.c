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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#include "ws2tcpip.h"
#include "winhttp.h"
#include "winreg.h"
#include "winternl.h"
#include "iphlpapi.h"
#include "dhcpcsdk.h"
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
        TRACE( "%p, %#lx, %p, %lu, %lu\n", hdr, status, info, buflen, hdr->recursion_count );
        InterlockedIncrement( &hdr->recursion_count );
        hdr->callback( hdr->handle, hdr->context, status, info, buflen );
        InterlockedDecrement( &hdr->recursion_count );
        TRACE("returning from %#lx callback\n", status);
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

static void session_destroy( struct object_header *hdr )
{
    struct session *session = (struct session *)hdr;

    TRACE("%p\n", session);

    if (session->unload_event) SetEvent( session->unload_event );
    destroy_cookies( session );

    session->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &session->cs );
    free( session->agent );
    free( session->proxy_server );
    free( session->proxy_bypass );
    free( session->proxy_username );
    free( session->proxy_password );
    free( session );
}

static BOOL validate_buffer( void *buffer, DWORD *buflen, DWORD required )
{
    if (!buffer || *buflen < required)
    {
        *buflen = required;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    return TRUE;
}

static BOOL session_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct session *session = (struct session *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = hdr->redirect_policy;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_WEB_SOCKET_RECEIVE_BUFFER_SIZE:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->websocket_receive_buffer_size;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = session->websocket_send_buffer_size;
        *buflen = sizeof(DWORD);
        return TRUE;

    default:
        FIXME( "unimplemented option %lu\n", option );
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

        FIXME( "%lu %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass) );
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
        TRACE( "%#lx\n", policy );
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
        TRACE( "%#lx\n", session->secure_protocols );
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
        FIXME( "WINHTTP_OPTION_MAX_CONNS_PER_SERVER: %lu\n", *(DWORD *)buffer );
        return TRUE;

    case WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER:
        FIXME( "WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER: %lu\n", *(DWORD *)buffer );
        return TRUE;

    case WINHTTP_OPTION_WEB_SOCKET_RECEIVE_BUFFER_SIZE:
    {
        DWORD buffer_size;

        if (buflen != sizeof(buffer_size))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        buffer_size = *(DWORD *)buffer;
        TRACE( "%#lx\n", buffer_size );
        session->websocket_receive_buffer_size = buffer_size;
        return TRUE;
    }

    case WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE:
    {
        DWORD buffer_size;

        if (buflen != sizeof(buffer_size))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        buffer_size = *(DWORD *)buffer;
        TRACE( "%#lx\n", buffer_size );
        session->websocket_send_buffer_size = buffer_size;
        return TRUE;
    }

    default:
        FIXME( "unimplemented option %lu\n", option );
        SetLastError( ERROR_WINHTTP_INVALID_OPTION );
        return FALSE;
    }
}

static const struct object_vtbl session_vtbl =
{
    NULL,
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

    TRACE( "%s, %lu, %s, %s, %#lx\n", debugstr_w(agent), access, debugstr_w(proxy), debugstr_w(bypass), flags );

    if (!(session = calloc( 1, sizeof(*session) ))) return NULL;

    session->hdr.type = WINHTTP_HANDLE_TYPE_SESSION;
    session->hdr.vtbl = &session_vtbl;
    session->hdr.flags = flags;
    session->hdr.refs = 1;
    session->hdr.redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    session->resolve_timeout = DEFAULT_RESOLVE_TIMEOUT;
    session->connect_timeout = DEFAULT_CONNECT_TIMEOUT;
    session->send_timeout = DEFAULT_SEND_TIMEOUT;
    session->receive_timeout = DEFAULT_RECEIVE_TIMEOUT;
    session->receive_response_timeout = DEFAULT_RECEIVE_RESPONSE_TIMEOUT;
    session->websocket_receive_buffer_size = 32768;
    session->websocket_send_buffer_size = 32768;
    list_init( &session->cookie_cache );
    InitializeCriticalSectionEx( &session->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    session->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": session.cs");

    if (agent && !(session->agent = wcsdup( agent ))) goto end;
    if (access == WINHTTP_ACCESS_TYPE_DEFAULT_PROXY)
    {
        WINHTTP_PROXY_INFO info;

        WinHttpGetDefaultProxyConfiguration( &info );
        session->access = info.dwAccessType;
        if (info.lpszProxy && !(session->proxy_server = wcsdup( info.lpszProxy )))
        {
            GlobalFree( info.lpszProxy );
            GlobalFree( info.lpszProxyBypass );
            goto end;
        }
        if (info.lpszProxyBypass && !(session->proxy_bypass = wcsdup( info.lpszProxyBypass )))
        {
            GlobalFree( info.lpszProxy );
            GlobalFree( info.lpszProxyBypass );
            goto end;
        }
    }
    else if (access == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
    {
        session->access = access;
        if (proxy && !(session->proxy_server = wcsdup( proxy ))) goto end;
        if (bypass && !(session->proxy_bypass = wcsdup( bypass ))) goto end;
    }

    handle = alloc_handle( &session->hdr );

#ifdef __REACTOS__
    winsock_init();
#endif

end:
    release_object( &session->hdr );
    TRACE("returning %p\n", handle);
    if (handle) SetLastError( ERROR_SUCCESS );
    return handle;
}

static void connect_destroy( struct object_header *hdr )
{
    struct connect *connect = (struct connect *)hdr;

    TRACE("%p\n", connect);

    release_object( &connect->session->hdr );

    free( connect->hostname );
    free( connect->servername );
    free( connect->username );
    free( connect->password );
    free( connect );
}

static BOOL connect_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct connect *connect = (struct connect *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PARENT_HANDLE:
    {
        if (!validate_buffer( buffer, buflen, sizeof(HINTERNET) )) return FALSE;

        *(HINTERNET *)buffer = connect->session->hdr.handle;
        *buflen = sizeof(HINTERNET);
        return TRUE;
    }
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = connect->session->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = connect->session->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = connect->session->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = connect->session->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = connect->session->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    default:
        FIXME( "unimplemented option %lu\n", option );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static const struct object_vtbl connect_vtbl =
{
    NULL,
    connect_destroy,
    connect_query_option,
    NULL
};

static BOOL domain_matches(LPCWSTR server, LPCWSTR domain)
{
    BOOL ret = FALSE;

    if (!wcsicmp( domain, L"<local>" ) && !wcschr( server, '.' ))
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
            dot = wcschr( server, '.' );
            if (dot)
            {
                int len = lstrlenW( dot + 1 );

                if (len > lstrlenW( domain + 2 ))
                {
                    LPCWSTR ptr;

                    /* The server's domain is longer than the wildcard, so it
                     * could be a subdomain.  Compare the last portion of the
                     * server's domain.
                     */
                    ptr = dot + len + 1 - lstrlenW( domain + 2 );
                    if (!wcsicmp( ptr, domain + 2 ))
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
                    ret = !wcsicmp( dot + 1, domain + 2 );
            }
        }
    }
    else
        ret = !wcsicmp( server, domain );
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

        ptr = wcschr( ptr, ';' );
        if (!ptr)
            ptr = wcschr( tmp, ' ' );
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

        if ((colon = wcschr( session->proxy_server, ':' )))
        {
            if (!connect->servername || wcsnicmp( connect->servername,
                session->proxy_server, colon - session->proxy_server - 1 ))
            {
                free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = malloc( (colon - session->proxy_server + 1) * sizeof(WCHAR) )))
                {
                    ret = FALSE;
                    goto end;
                }
                memcpy( connect->servername, session->proxy_server, (colon - session->proxy_server) * sizeof(WCHAR) );
                connect->servername[colon - session->proxy_server] = 0;
                if (*(colon + 1))
                    connect->serverport = wcstol( colon + 1, NULL, 10 );
                else
                    connect->serverport = INTERNET_DEFAULT_PORT;
            }
        }
        else
        {
            if (!connect->servername || wcsicmp( connect->servername, session->proxy_server ))
            {
                free( connect->servername );
                connect->resolved = FALSE;
                if (!(connect->servername = wcsdup( session->proxy_server )))
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
        free( connect->servername );
        connect->resolved = FALSE;
        if (!(connect->servername = wcsdup( server )))
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
HINTERNET WINAPI WinHttpConnect( HINTERNET hsession, const WCHAR *server, INTERNET_PORT port, DWORD reserved )
{
    struct connect *connect;
    struct session *session;
    HINTERNET hconnect = NULL;

    TRACE( "%p, %s, %u, %#lx\n", hsession, debugstr_w(server), port, reserved );

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
    if (!(connect = calloc( 1, sizeof(*connect) )))
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

    addref_object( &session->hdr );
    connect->session = session;

    if (!(connect->hostname = wcsdup( server ))) goto end;
    connect->hostport = port;
    if (!set_server_for_hostname( connect, server, port )) goto end;

    if ((hconnect = alloc_handle( &connect->hdr )))
    {
        send_callback( &session->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hconnect, sizeof(hconnect) );
    }

end:
    release_object( &connect->hdr );
    release_object( &session->hdr );
    TRACE("returning %p\n", hconnect);
    if (hconnect) SetLastError( ERROR_SUCCESS );
    return hconnect;
}

static void request_destroy( struct object_header *hdr )
{
    struct request *request = (struct request *)hdr;
    unsigned int i, j;

    TRACE("%p\n", request);

    stop_queue( &request->queue );
    release_object( &request->connect->hdr );

    if (request->cred_handle_initialized) FreeCredentialsHandle( &request->cred_handle );
    CertFreeCertificateContext( request->server_cert );
    CertFreeCertificateContext( request->client_cert );

    destroy_authinfo( request->authinfo );
    destroy_authinfo( request->proxy_authinfo );

    free( request->verb );
    free( request->path );
    free( request->version );
    free( request->raw_headers );
    free( request->status_text );
    for (i = 0; i < request->num_headers; i++)
    {
        free( request->headers[i].field );
        free( request->headers[i].value );
    }
    free( request->headers );
    for (i = 0; i < TARGET_MAX; i++)
    {
        for (j = 0; j < SCHEME_MAX; j++)
        {
            free( request->creds[i][j].username );
            free( request->creds[i][j].password );
        }
    }

    free( request );
}

static BOOL return_string_option( WCHAR *buffer, const WCHAR *str, LPDWORD buflen )
{
    int len = sizeof(WCHAR);
    if (str) len += lstrlenW( str ) * sizeof(WCHAR);
    if (buffer && *buflen >= len)
    {
        if (str) memcpy( buffer, str, len );
        len -= sizeof(WCHAR);
        buffer[len / sizeof(WCHAR)] = 0;
        *buflen = len;
        return TRUE;
    }
    else
    {
        *buflen = len;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
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

static WCHAR *build_url( struct request *request )
{
    URL_COMPONENTS uc;
    DWORD len = 0;
    WCHAR *ret;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.nScheme = (request->hdr.flags & WINHTTP_FLAG_SECURE) ? INTERNET_SCHEME_HTTPS : INTERNET_SCHEME_HTTP;
    uc.lpszHostName = request->connect->hostname;
    uc.dwHostNameLength = wcslen( uc.lpszHostName );
    uc.nPort = request->connect->hostport;
    uc.lpszUserName = request->connect->username;
    uc.dwUserNameLength = request->connect->username ? wcslen( request->connect->username ) : 0;
    uc.lpszPassword = request->connect->password;
    uc.dwPasswordLength = request->connect->password ? wcslen( request->connect->password ) : 0;
    uc.lpszUrlPath = request->path;
    uc.dwUrlPathLength = wcslen( uc.lpszUrlPath );

    WinHttpCreateUrl( &uc, 0, NULL, &len );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !(ret = malloc( len * sizeof(WCHAR) ))) return NULL;

    if (WinHttpCreateUrl( &uc, 0, ret, &len )) return ret;
    free( ret );
    return NULL;
}

static BOOL request_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    struct request *request = (struct request *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_PARENT_HANDLE:
    {
        if (!validate_buffer( buffer, buflen, sizeof(HINTERNET) )) return FALSE;

        *(HINTERNET *)buffer = request->connect->hdr.handle;
        *buflen = sizeof(HINTERNET);
        return TRUE;
    }
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;
        int bits;

        if (!validate_buffer( buffer, buflen, sizeof(flags) )) return FALSE;

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

        if (!validate_buffer( buffer, buflen, sizeof(cert) )) return FALSE;

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

        if (!validate_buffer( buffer, buflen, sizeof(*ci) ) || !cert) return FALSE;

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
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

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

        if (!validate_buffer( buffer, buflen, sizeof(*info) )) return FALSE;

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
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->resolve_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->connect_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->send_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->receive_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->receive_response_timeout;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_USERNAME:
        return return_string_option( buffer, request->connect->username, buflen );

    case WINHTTP_OPTION_PASSWORD:
        return return_string_option( buffer, request->connect->password, buflen );

    case WINHTTP_OPTION_PROXY_USERNAME:
        return return_string_option( buffer, request->connect->session->proxy_username, buflen );

    case WINHTTP_OPTION_PROXY_PASSWORD:
        return return_string_option( buffer, request->connect->session->proxy_password, buflen );

    case WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->max_redirects;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_HTTP_PROTOCOL_USED:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        FIXME("WINHTTP_OPTION_HTTP_PROTOCOL_USED\n");
        *(DWORD *)buffer = 0;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_WEB_SOCKET_RECEIVE_BUFFER_SIZE:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->websocket_receive_buffer_size;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE:
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = request->websocket_set_send_buffer_size;
        *buflen = sizeof(DWORD);
        return TRUE;

    case WINHTTP_OPTION_URL:
    {
        WCHAR *url;
        BOOL ret;

        if (!(url = build_url( request ))) return FALSE;
        ret = return_string_option( buffer, url, buflen );
        free( url );
        return ret;
    }

    default:
        FIXME( "unimplemented option %lu\n", option );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

static WCHAR *buffer_to_str( WCHAR *buffer, DWORD buflen )
{
    WCHAR *ret;
    if ((ret = malloc( (buflen + 1) * sizeof(WCHAR))))
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

        FIXME( "%lu %s %s\n", pi->dwAccessType, debugstr_w(pi->lpszProxy), debugstr_w(pi->lpszProxyBypass) );
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
        TRACE( "%#lx\n", disable );
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
        TRACE( "%#lx\n", policy );
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
        TRACE( "%#lx\n", policy );
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
        TRACE( "%#lx\n", flags );
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

        free( connect->username );
        if (!(connect->username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PASSWORD:
    {
        struct connect *connect = request->connect;

        free( connect->password );
        if (!(connect->password = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_USERNAME:
    {
        struct session *session = request->connect->session;

        free( session->proxy_username );
        if (!(session->proxy_username = buffer_to_str( buffer, buflen ))) return FALSE;
        return TRUE;
    }
    case WINHTTP_OPTION_PROXY_PASSWORD:
    {
        struct session *session = request->connect->session;

        free( session->proxy_password );
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
        else if (buflen >= sizeof(*cert))
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

    case WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET:
        request->flags |= REQUEST_FLAG_WEBSOCKET_UPGRADE;
        return TRUE;

    case WINHTTP_OPTION_CONNECT_RETRIES:
        FIXME("WINHTTP_OPTION_CONNECT_RETRIES\n");
        return TRUE;

    case WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS:
        if (buflen == sizeof(DWORD))
        {
            request->max_redirects = *(DWORD *)buffer;
            SetLastError(NO_ERROR);
            return TRUE;
        }

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE:
        FIXME("WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE\n");
        return TRUE;

    case WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE:
        FIXME("WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE\n");
        return TRUE;

    case WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL:
        if (buflen == sizeof(DWORD))
        {
            FIXME( "WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL %#lx\n", *(DWORD *)buffer );
            return TRUE;
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case WINHTTP_OPTION_WEB_SOCKET_RECEIVE_BUFFER_SIZE:
    {
        DWORD buffer_size;

        if (buflen != sizeof(buffer_size))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        buffer_size = *(DWORD *)buffer;
        WARN( "Setting websocket receive buffer size currently has not effct, size %lu\n", buffer_size );
        request->websocket_receive_buffer_size = buffer_size;
        return TRUE;
    }

    case WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE:
    {
        DWORD buffer_size;

        if (buflen != sizeof(buffer_size))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        buffer_size = *(DWORD *)buffer;
        request->websocket_set_send_buffer_size = buffer_size;
        TRACE( "Websocket send buffer size %lu.\n", buffer_size);
        return TRUE;
    }

    default:
        FIXME( "unimplemented option %lu\n", option );
        SetLastError( ERROR_WINHTTP_INVALID_OPTION );
        return FALSE;
    }
}

static const struct object_vtbl request_vtbl =
{
    NULL,
    request_destroy,
    request_query_option,
    request_set_option
};

static BOOL add_accept_types_header( struct request *request, const WCHAR **types )
{
    static const DWORD flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA;

    if (!types) return TRUE;
    while (*types)
    {
        if (process_header( request, L"Accept", *types, flags, TRUE )) return FALSE;
        types++;
    }
    return TRUE;
}

static WCHAR *get_request_path( const WCHAR *object )
{
    int len = object ? lstrlenW(object) : 0;
    WCHAR *p, *ret;

    if (!object || object[0] != '/') len++;
    if (!(p = ret = malloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
    if (!object || object[0] != '/') *p++ = '/';
    if (object) lstrcpyW( p, object );
    ret[len] = 0;
    return ret;
}

/***********************************************************************
 *          WinHttpOpenRequest (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpenRequest( HINTERNET hconnect, const WCHAR *verb, const WCHAR *object, const WCHAR *version,
                                     const WCHAR *referrer, const WCHAR **types, DWORD flags )
{
    struct request *request;
    struct connect *connect;
    HINTERNET hrequest = NULL;

    TRACE( "%p, %s, %s, %s, %s, %p, %#lx\n", hconnect, debugstr_w(verb), debugstr_w(object),
          debugstr_w(version), debugstr_w(referrer), types, flags );

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
    if (!(request = calloc( 1, sizeof(*request) )))
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
    init_queue( &request->queue );

    addref_object( &connect->hdr );
    request->connect = connect;

    request->resolve_timeout = connect->session->resolve_timeout;
    request->connect_timeout = connect->session->connect_timeout;
    request->send_timeout = connect->session->send_timeout;
    request->receive_timeout = connect->session->receive_timeout;
    request->receive_response_timeout = connect->session->receive_response_timeout;
    request->max_redirects = 10;
    request->websocket_receive_buffer_size = connect->session->websocket_receive_buffer_size;
    request->websocket_send_buffer_size = connect->session->websocket_send_buffer_size;
    request->websocket_set_send_buffer_size = request->websocket_send_buffer_size;
    request->read_reply_status = ERROR_WINHTTP_INCORRECT_HANDLE_STATE;

    if (!verb || !verb[0]) verb = L"GET";
    if (!(request->verb = wcsdup( verb ))) goto end;
    if (!(request->path = get_request_path( object ))) goto end;

    if (!version || !version[0]) version = L"HTTP/1.1";
    if (!(request->version = wcsdup( version ))) goto end;
    if (!(add_accept_types_header( request, types ))) goto end;

    if ((hrequest = alloc_handle( &request->hdr )))
    {
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hrequest, sizeof(hrequest) );
    }

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
    case WINHTTP_OPTION_WORKER_THREAD_COUNT:
    {
        FIXME( "WINHTTP_OPTION_WORKER_THREAD_COUNT semi-stub.\n" );
        if (!validate_buffer( buffer, buflen, sizeof(DWORD) )) return FALSE;

        *(DWORD *)buffer = 0;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        if (!validate_buffer( buffer, buflen, sizeof(DWORD_PTR) )) return FALSE;

        *(DWORD_PTR *)buffer = hdr->context;
        *buflen = sizeof(DWORD_PTR);
        return TRUE;
    }
    default:
        if (hdr->vtbl->query_option) ret = hdr->vtbl->query_option( hdr, option, buffer, buflen );
        else
        {
            FIXME( "unimplemented option %lu\n", option );
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
BOOL WINAPI WinHttpQueryOption( HINTERNET handle, DWORD option, void *buffer, DWORD *buflen )
{
    BOOL ret = FALSE;
    struct object_header *hdr;

    TRACE( "%p, %lu, %p, %p\n", handle, option, buffer, buflen );

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
            FIXME( "unimplemented option %lu\n", option );
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
BOOL WINAPI WinHttpSetOption( HINTERNET handle, DWORD option,  void *buffer, DWORD buflen )
{
    BOOL ret = FALSE;
    struct object_header *hdr;

    TRACE( "%p, %lu, %p, %lu\n", handle, option, buffer, buflen );

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

static IP_ADAPTER_ADDRESSES *get_adapters(void)
{
    ULONG err, size = 1024, flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                    GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static WCHAR *detect_autoproxyconfig_url_dhcp(void)
{
    IP_ADAPTER_ADDRESSES *adapters, *ptr;
    DHCPCAPI_PARAMS_ARRAY send_params, recv_params;
    DHCPCAPI_PARAMS param;
    WCHAR name[MAX_ADAPTER_NAME_LENGTH + 1], *ret = NULL;
    DWORD err, size;
    BYTE *tmp, *buf = NULL;

    if (!(adapters = get_adapters())) return NULL;

    memset( &send_params, 0, sizeof(send_params) );
    memset( &param, 0, sizeof(param) );
    param.OptionId = OPTION_MSFT_IE_PROXY;
    recv_params.nParams = 1;
    recv_params.Params  = &param;

    for (ptr = adapters; ptr; ptr = ptr->Next)
    {
        MultiByteToWideChar( CP_ACP, 0, ptr->AdapterName, -1, name, ARRAY_SIZE(name) );
        TRACE( "adapter '%s' type %lu dhcpv4 enabled %d\n", wine_dbgstr_w(name), ptr->IfType, ptr->Dhcpv4Enabled );

        if (ptr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        /* FIXME: also skip adapters where DHCP is disabled */

        size = 256;
        if (!(buf = malloc( size ))) goto done;
        err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params,
                                 buf, &size, NULL );
        while (err == ERROR_MORE_DATA)
        {
            if (!(tmp = realloc( buf, size ))) goto done;
            buf = tmp;
            err = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, name, NULL, send_params, recv_params,
                                     buf, &size, NULL );
        }
        if (err == ERROR_SUCCESS && param.nBytesData)
        {
            int len = MultiByteToWideChar( CP_ACP, 0, (const char *)param.Data, param.nBytesData, NULL, 0 );
            if ((ret = malloc( (len + 1) * sizeof(WCHAR) )))
            {
                MultiByteToWideChar( CP_ACP, 0,  (const char *)param.Data, param.nBytesData, ret, len );
                ret[len] = 0;
            }
            TRACE("returning %s\n", debugstr_w(ret));
            break;
        }
    }

done:
    free( buf );
    free( adapters );
    return ret;
}

static char *get_computer_name( COMPUTER_NAME_FORMAT format )
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( format, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = malloc( size ))) return NULL;
    if (!GetComputerNameExA( format, ret, &size ))
    {
        free( ret );
        return NULL;
    }
    return ret;
}

static BOOL is_domain_suffix( const char *domain, const char *suffix )
{
    int len_domain = strlen( domain ), len_suffix = strlen( suffix );

    if (len_suffix > len_domain) return FALSE;
    if (!stricmp( domain + len_domain - len_suffix, suffix )) return TRUE;
    return FALSE;
}

static int reverse_lookup( const struct addrinfo *ai, char *hostname, size_t len )
{
    return getnameinfo( ai->ai_addr, ai->ai_addrlen, hostname, len, NULL, 0, 0 );
}

static WCHAR *build_wpad_url( const char *hostname, const struct addrinfo *ai )
{
    char name[NI_MAXHOST];
    WCHAR *ret, *p;
    int len;

    while (ai && ai->ai_family != AF_INET && ai->ai_family != AF_INET6) ai = ai->ai_next;
    if (!ai) return NULL;

    if (!reverse_lookup( ai, name, sizeof(name) )) hostname = name;

    len = lstrlenW( L"http://" ) + strlen( hostname ) + lstrlenW( L"/wpad.dat" );
    if (!(ret = p = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
    lstrcpyW( p, L"http://" );
    p += lstrlenW( L"http://" );
    while (*hostname) { *p++ = *hostname++; }
    lstrcpyW( p, L"/wpad.dat" );
    return ret;
}

static WCHAR *detect_autoproxyconfig_url_dns(void)
{
    char *fqdn, *domain, *p;
    WCHAR *ret = NULL;

    if (!(fqdn = get_computer_name( ComputerNamePhysicalDnsFullyQualified ))) return NULL;
    if (!(domain = get_computer_name( ComputerNamePhysicalDnsDomain )))
    {
        free( fqdn );
        return NULL;
    }
    p = fqdn;
    while ((p = strchr( p, '.' )) && is_domain_suffix( p + 1, domain ))
    {
        char *name;
        struct addrinfo *ai, hints;
        int res;

        if (!(name = malloc( sizeof("wpad") + strlen(p) )))
        {
            free( fqdn );
            free( domain );
            return NULL;
        }
        strcpy( name, "wpad" );
        strcat( name, p );
        memset( &hints, 0, sizeof(hints) );
        hints.ai_flags  = AI_ALL | AI_DNS_ONLY;
        hints.ai_family = AF_UNSPEC;
        res = getaddrinfo( name, NULL, &hints, &ai );
        if (!res)
        {
            ret = build_wpad_url( name, ai );
            freeaddrinfo( ai );
            if (ret)
            {
                TRACE("returning %s\n", debugstr_w(ret));
                free( name );
                break;
            }
        }
       free( name );
       p++;
    }
    free( domain );
    free( fqdn );
    return ret;
}

/***********************************************************************
 *          WinHttpDetectAutoProxyConfigUrl (winhttp.@)
 */
BOOL WINAPI WinHttpDetectAutoProxyConfigUrl( DWORD flags, WCHAR **url )
{
    TRACE( "%#lx, %p\n", flags, url );

    if (!flags || !url)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *url = NULL;
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DHCP)
    {
        *url = detect_autoproxyconfig_url_dhcp();
    }
    if (flags & WINHTTP_AUTO_DETECT_TYPE_DNS_A)
    {
        if (!*url) *url = detect_autoproxyconfig_url_dns();
    }
    if (!*url)
    {
        SetLastError( ERROR_WINHTTP_AUTODETECTION_FAILED );
        return FALSE;
    }
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

static const WCHAR path_connections[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections";

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
    WCHAR *envproxy;

    TRACE("%p\n", info);

    l = RegOpenKeyExW( HKEY_LOCAL_MACHINE, path_connections, 0, KEY_READ, &key );
    if (!l)
    {
        DWORD type, size = 0;

        l = RegQueryValueExW( key, L"WinHttpSettings", NULL, &type, NULL, &size );
        if (!l && type == REG_BINARY &&
            size >= sizeof(struct connection_settings_header) + 2 * sizeof(DWORD))
        {
            BYTE *buf = malloc( size );

            if (buf)
            {
                struct connection_settings_header *hdr =
                    (struct connection_settings_header *)buf;
                DWORD *len = (DWORD *)(hdr + 1);

                l = RegQueryValueExW( key, L"WinHttpSettings", NULL, NULL, buf,
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
                free( buf );
            }
        }
        RegCloseKey( key );
    }
    if (!got_from_reg && (envproxy = _wgetenv( L"http_proxy" )))
    {
        WCHAR *colon, *http_proxy = NULL;

        if (!(colon = wcschr( envproxy, ':' ))) http_proxy = envproxy;
        else
        {
            if (*(colon + 1) == '/' && *(colon + 2) == '/')
            {
                /* It's a scheme, check that it's http */
                if (!wcsncmp( envproxy, L"http://", 7 )) http_proxy = envproxy + 7;
                else WARN("unsupported scheme in $http_proxy: %s\n", debugstr_w(envproxy));
            }
            else http_proxy = envproxy;
        }

        if (http_proxy && http_proxy[0])
        {
            direct = FALSE;
            info->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            info->lpszProxy = GlobalAlloc( 0, (lstrlenW(http_proxy) + 1) * sizeof(WCHAR) );
            wcscpy( info->lpszProxy, http_proxy );
            info->lpszProxyBypass = NULL;
            TRACE("http proxy (from environment) = %s\n", debugstr_w(info->lpszProxy));
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

    if (RegOpenKeyExW( HKEY_CURRENT_USER, path_connections, 0, KEY_READ, &hkey ) ||
        RegQueryValueExW( hkey, L"DefaultConnectionSettings", NULL, &type, NULL, &size ) ||
        type != REG_BINARY || size < sizeof(struct connection_settings_header))
    {
        ret = TRUE;
        goto done;
    }
    if (!(hdr = malloc( size ))) goto done;
    if (RegQueryValueExW( hkey, L"DefaultConnectionSettings", NULL, &type, (BYTE *)hdr, &size ) ||
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
    free( hdr );
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
        if (!(q = strdupAW( p ))) return FALSE;
        len = wcslen( q );
        info->lpszProxy = GlobalAlloc( 0, (len + 1) * sizeof(WCHAR) );
        if (!info->lpszProxy)
        {
            free( q );
            return FALSE;
        }
        memcpy( info->lpszProxy, q, (len + 1) * sizeof(WCHAR) );
        free( q );
        q = info->lpszProxy;
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

static SRWLOCK cache_lock = SRWLOCK_INIT;
static DWORD cached_script_size;
static ULONGLONG cache_update_time;
static char *cached_script;
static WCHAR *cached_url;

static BOOL get_cached_script( const WCHAR *url, char **buffer, DWORD *out_size )
{
    BOOL ret = FALSE;

    *buffer = NULL;
    *out_size = 0;

    AcquireSRWLockExclusive( &cache_lock );
    if (cached_url && !wcscmp( cached_url, url ) && GetTickCount64() - cache_update_time < 60000)
    {
        ret = TRUE;
        if (cached_script && (*buffer = malloc( cached_script_size )))
        {
            memcpy( *buffer, cached_script, cached_script_size );
            *out_size = cached_script_size;
        }
    }
    ReleaseSRWLockExclusive( &cache_lock );
    return ret;
}

static void cache_script( const WCHAR *url, char *buffer, DWORD size )
{
    AcquireSRWLockExclusive( &cache_lock );
    free( cached_url );
    free( cached_script );
    cached_script_size = 0;
    cached_script = NULL;

    if ((cached_url = wcsdup( url )) && buffer && (cached_script = malloc( size )))
    {
        memcpy( cached_script, buffer, size );
        cached_script_size = size;
    }
    cache_update_time = GetTickCount64();
    ReleaseSRWLockExclusive( &cache_lock );
}

static char *download_script( const WCHAR *url, DWORD *out_size )
{
    static const WCHAR *acceptW[] = {L"*/*", NULL};
    HINTERNET ses, con = NULL, req = NULL;
    WCHAR *hostname;
    URL_COMPONENTSW uc;
    DWORD status, size = sizeof(status), offset, to_read, bytes_read, flags = 0;
    char *tmp, *buffer;

    if (get_cached_script( url, &buffer, out_size ))
    {
        TRACE( "Returning cached result.\n" );
        if (!buffer) SetLastError( ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT );
        return buffer;
    }

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = -1;
    uc.dwUrlPathLength = -1;
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return NULL;
    if (!(hostname = malloc( (uc.dwHostNameLength + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    if (!(ses = WinHttpOpen( NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 ))) goto done;
    WinHttpSetTimeouts( ses, 5000, 60000, 30000, 30000 );
    if (!(con = WinHttpConnect( ses, hostname, uc.nPort, 0 ))) goto done;
    if (uc.nScheme == INTERNET_SCHEME_HTTPS) flags |= WINHTTP_FLAG_SECURE;
    if (!(req = WinHttpOpenRequest( con, NULL, uc.lpszUrlPath, NULL, NULL, acceptW, flags ))) goto done;
    if (!WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 )) goto done;

    if (!WinHttpReceiveResponse( req, 0 )) goto done;
    if (!WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status,
        &size, NULL ) || status != HTTP_STATUS_OK) goto done;

    size = 4096;
    if (!(buffer = malloc( size ))) goto done;
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
            if (!(tmp = realloc( buffer, size ))) goto done;
            buffer = tmp;
        }
    }

done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
    free( hostname );
    cache_script( url, buffer, *out_size );
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

#define MAX_SCHEME_LENGTH 32
static BOOL run_script( char *script, DWORD size, const WCHAR *url, WINHTTP_PROXY_INFO *info, DWORD flags )
{
    WCHAR scheme[MAX_SCHEME_LENGTH + 1], buf[MAX_HOST_NAME_LENGTH + 1], *hostname;
    BOOL ret;
    char *result, *urlA, *hostnameA;
    DWORD len, len_scheme, len_hostname;
    struct AUTO_PROXY_SCRIPT_BUFFER buffer;
    URL_COMPONENTSW uc;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = -1;
    uc.dwHostNameLength = -1;

    if (!WinHttpCrackUrl( url, 0, 0, &uc ))
        return FALSE;

    memcpy( scheme, uc.lpszScheme, uc.dwSchemeLength * sizeof(WCHAR) );
    scheme[uc.dwSchemeLength] = 0;
    wcslwr( scheme );
    len_scheme = WideCharToMultiByte( CP_ACP, 0, scheme, uc.dwSchemeLength, NULL, 0, NULL, NULL );

    if (flags & WINHTTP_AUTOPROXY_HOST_LOWERCASE && !(flags & WINHTTP_AUTOPROXY_HOST_KEEPCASE))
    {
        memcpy( buf, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
        buf[uc.dwHostNameLength] = 0;
        wcslwr( buf );
        hostname = buf;
    }
    else
    {
        hostname = uc.lpszHostName;
    }
    len_hostname = WideCharToMultiByte( CP_ACP, 0, hostname, uc.dwHostNameLength, NULL, 0, NULL, NULL );

    len = WideCharToMultiByte( CP_ACP, 0, uc.lpszHostName + uc.dwHostNameLength, -1, NULL, 0, NULL, NULL );
    if (!(urlA = malloc( len + len_scheme + len_hostname + 3 ))) return FALSE;
    WideCharToMultiByte( CP_ACP, 0, scheme, uc.dwSchemeLength, urlA, len_scheme, NULL, NULL );
    urlA[len_scheme++] = ':';
    urlA[len_scheme++] = '/';
    urlA[len_scheme++] = '/';
    WideCharToMultiByte( CP_ACP, 0, hostname, uc.dwHostNameLength, urlA + len_scheme, len_hostname, NULL, NULL );
    hostnameA = urlA + len_scheme;
    WideCharToMultiByte( CP_ACP, 0, uc.lpszHostName + uc.dwHostNameLength, -1,
            urlA + len_scheme + len_hostname, len, NULL, NULL );

    buffer.dwStructSize = sizeof(buffer);
    buffer.lpszScriptBuffer = script;
    buffer.dwScriptBufferSize = size;

    if (!InternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buffer ))
    {
        free( urlA );
        return FALSE;
    }

    if ((ret = InternetGetProxyInfo( urlA, strlen(urlA), hostnameA, len_hostname, &result, &len )))
    {
        ret = parse_script_result( result, info );
        free( result );
    }

    free( urlA );
    InternetDeInitializeAutoProxyDll( NULL, 0 );
    return ret;
}

/***********************************************************************
 *          WinHttpGetProxyForUrl (winhttp.@)
 */
BOOL WINAPI WinHttpGetProxyForUrl( HINTERNET hsession, LPCWSTR url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                   WINHTTP_PROXY_INFO *info )
{
    WCHAR *pac_url;
    struct session *session;
    char *script = NULL;
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
        (options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL && !options->lpszAutoConfigUrl))
    {
        release_object( &session->hdr );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (options->dwFlags & WINHTTP_AUTOPROXY_AUTO_DETECT &&
        WinHttpDetectAutoProxyConfigUrl( options->dwAutoDetectFlags, &pac_url ))
    {
        script = download_script( pac_url, &size );
        GlobalFree( pac_url );
    }

    if (!script && options->dwFlags & WINHTTP_AUTOPROXY_CONFIG_URL)
        script = download_script( options->lpszAutoConfigUrl, &size );

    if (script)
    {
        ret = run_script( script, size, url, info, options->dwFlags );
        free( script );
    }

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

    l = RegCreateKeyExW( HKEY_LOCAL_MACHINE, path_connections, 0, NULL, 0,
        KEY_WRITE, NULL, &key, NULL );
    if (!l)
    {
        DWORD size = sizeof(struct connection_settings_header) + 2 * sizeof(DWORD);
        BYTE *buf;

        if (info->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
        {
            size += lstrlenW( info->lpszProxy );
            if (info->lpszProxyBypass)
                size += lstrlenW( info->lpszProxyBypass );
        }
        if ((buf = malloc( size )))
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
                *len++ = lstrlenW( info->lpszProxy );
                for (dst = (BYTE *)len, src = info->lpszProxy; *src;
                    src++, dst++)
                    *dst = *src;
                len = (DWORD *)dst;
                if (info->lpszProxyBypass)
                {
                    *len++ = lstrlenW( info->lpszProxyBypass );
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
            l = RegSetValueExW( key, L"WinHttpSettings", 0, REG_BINARY, buf, size );
            if (!l)
                ret = TRUE;
            free( buf );
        }
        RegCloseKey( key );
    }
    if (ret) SetLastError( ERROR_SUCCESS );
    return ret;
}

/***********************************************************************
 *          WinHttpCreateProxyResolver (winhttp.@)
 */
DWORD WINAPI WinHttpCreateProxyResolver( HINTERNET hsession, HINTERNET *hresolver )
{
    FIXME("%p, %p\n", hsession, hresolver);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpFreeProxyResult (winhttp.@)
 */
void WINAPI WinHttpFreeProxyResult( WINHTTP_PROXY_RESULT *result )
{
    FIXME("%p\n", result);
}

/***********************************************************************
 *          WinHttpFreeProxyResultEx (winhttp.@)
 */
void WINAPI WinHttpFreeProxyResultEx( WINHTTP_PROXY_RESULT_EX *result )
{
    FIXME("%p\n", result);
}

/***********************************************************************
 *          WinHttpFreeProxySettings (winhttp.@)
 */
void WINAPI WinHttpFreeProxySettings( WINHTTP_PROXY_SETTINGS *settings )
{
    FIXME("%p\n", settings);
}

/***********************************************************************
 *          WinHttpGetProxyForUrlEx (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyForUrlEx( HINTERNET hresolver, const WCHAR *url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                      DWORD_PTR ctx )
{
    FIXME( "%p, %s, %p, %Ix\n", hresolver, debugstr_w(url), options, ctx );
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpGetProxyForUrlEx2 (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyForUrlEx2( HINTERNET hresolver, const WCHAR *url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                       DWORD selection_len, BYTE *selection, DWORD_PTR ctx )
{
    FIXME( "%p, %s, %p, %lu, %p, %Ix\n", hresolver, debugstr_w(url), options, selection_len, selection, ctx );
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpGetProxyResult (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyResult( HINTERNET hresolver, WINHTTP_PROXY_RESULT *result )
{
    FIXME("%p, %p\n", hresolver, result);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpGetProxyResultEx (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyResultEx( HINTERNET hresolver, WINHTTP_PROXY_RESULT_EX *result )
{
    FIXME("%p, %p\n", hresolver, result);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpGetProxySettingsVersion (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxySettingsVersion( HINTERNET hsession, DWORD *version )
{
    FIXME("%p, %p\n", hsession, version);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpReadProxySettings (winhttp.@)
 */
DWORD WINAPI WinHttpReadProxySettings( HINTERNET hsession, const WCHAR *connection, BOOL use_defaults,
                                       BOOL set_autodiscover, DWORD *version, BOOL *defaults_returned,
                                       WINHTTP_PROXY_SETTINGS *settings)
{
    FIXME("%p, %s, %d, %d, %p, %p, %p\n", hsession, debugstr_w(connection), use_defaults, set_autodiscover,
          version, defaults_returned, settings);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpResetAutoProxy (winhttp.@)
 */
DWORD WINAPI WinHttpResetAutoProxy( HINTERNET hsession, DWORD flags )
{
    FIXME( "%p, %#lx\n", hsession, flags );
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

DWORD WINAPI WinHttpWriteProxySettings( HINTERNET hsession, BOOL force, WINHTTP_PROXY_SETTINGS *settings )
{
    FIXME("%p, %d, %p\n", hsession, force, settings);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpSetStatusCallback (winhttp.@)
 */
WINHTTP_STATUS_CALLBACK WINAPI WinHttpSetStatusCallback( HINTERNET handle, WINHTTP_STATUS_CALLBACK callback,
                                                         DWORD flags, DWORD_PTR reserved )
{
    struct object_header *hdr;
    WINHTTP_STATUS_CALLBACK ret;

    TRACE( "%p, %p, %#lx, %Ix\n", handle, callback, flags, reserved );

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
    {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
static const WCHAR month[12][4] =
    {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};

/***********************************************************************
 *           WinHttpTimeFromSystemTime (WININET.@)
 */
BOOL WINAPI WinHttpTimeFromSystemTime( const SYSTEMTIME *time, LPWSTR string )
{
    TRACE("%p, %p\n", time, string);

    if (!time || !string)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    swprintf( string, WINHTTP_TIME_FORMAT_BUFSIZE / sizeof(WCHAR),
              L"%s, %02d %s %4d %02d:%02d:%02d GMT",
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

    while (*s && !iswalpha( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wDayOfWeek = 7;

    for (i = 0; i < 7; i++)
    {
        if (towupper( wkday[i][0] ) == towupper( s[0] ) &&
            towupper( wkday[i][1] ) == towupper( s[1] ) &&
            towupper( wkday[i][2] ) == towupper( s[2] ) )
        {
            time->wDayOfWeek = i;
            break;
        }
    }

    if (time->wDayOfWeek > 6) return TRUE;
    while (*s && !iswdigit( *s )) s++;
    time->wDay = wcstol( s, &end, 10 );
    s = end;

    while (*s && !iswalpha( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wMonth = 0;

    for (i = 0; i < 12; i++)
    {
        if (towupper( month[i][0]) == towupper( s[0] ) &&
            towupper( month[i][1]) == towupper( s[1] ) &&
            towupper( month[i][2]) == towupper( s[2] ) )
        {
            time->wMonth = i + 1;
            break;
        }
    }
    if (time->wMonth == 0) return TRUE;

    while (*s && !iswdigit( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wYear = wcstol( s, &end, 10 );
    s = end;

    while (*s && !iswdigit( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wHour = wcstol( s, &end, 10 );
    s = end;

    while (*s && !iswdigit( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wMinute = wcstol( s, &end, 10 );
    s = end;

    while (*s && !iswdigit( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wSecond = wcstol( s, &end, 10 );

    time->wMilliseconds = 0;
    return TRUE;
}
