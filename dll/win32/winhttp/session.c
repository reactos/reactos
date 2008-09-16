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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

void set_last_error( DWORD error )
{
    /* FIXME */
    SetLastError( error );
}

void send_callback( object_header_t *hdr, DWORD status, LPVOID info, DWORD buflen )
{
    TRACE("%p, 0x%08x, %p, %u\n", hdr, status, info, buflen);

    if (hdr->notify_mask & status) hdr->callback( hdr->handle, hdr->context, status, info, buflen );
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
static void session_destroy( object_header_t *hdr )
{
    session_t *session = (session_t *)hdr;

    TRACE("%p\n", session);

    heap_free( session->agent );
    heap_free( session->proxy_server );
    heap_free( session->proxy_bypass );
    heap_free( session->proxy_username );
    heap_free( session->proxy_password );
    heap_free( session );
}

static BOOL session_set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
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
        DWORD policy = *(DWORD *)buffer;

        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        return TRUE;
    }
}

static const object_vtbl_t session_vtbl =
{
    session_destroy,
    NULL,
    session_set_option
};

/***********************************************************************
 *          WinHttpOpen (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpen( LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags )
{
    session_t *session;
    HINTERNET handle = NULL;

    TRACE("%s, %u, %s, %s, 0x%08x\n", debugstr_w(agent), access, debugstr_w(proxy), debugstr_w(bypass), flags);

    if (!(session = heap_alloc_zero( sizeof(session_t) ))) return NULL;

    session->hdr.type = WINHTTP_HANDLE_TYPE_SESSION;
    session->hdr.vtbl = &session_vtbl;
    session->hdr.flags = flags;
    session->hdr.refs = 1;
    session->access = access;

    if (agent && !(session->agent = strdupW( agent ))) goto end;
    if (proxy && !(session->proxy_server = strdupW( proxy ))) goto end;
    if (bypass && !(session->proxy_bypass = strdupW( bypass ))) goto end;

    if (!(handle = alloc_handle( &session->hdr ))) goto end;
    session->hdr.handle = handle;

end:
    release_object( &session->hdr );
    TRACE("returning %p\n", handle);
    return handle;
}

/***********************************************************************
 *          connect_destroy (internal)
 */
static void connect_destroy( object_header_t *hdr )
{
    connect_t *connect = (connect_t *)hdr;

    TRACE("%p\n", connect);

    release_object( &connect->session->hdr );

    heap_free( connect->hostname );
    heap_free( connect->servername );
    heap_free( connect->username );
    heap_free( connect->password );
    heap_free( connect );
}

static const object_vtbl_t connect_vtbl =
{
    connect_destroy,
    NULL,
    NULL
};

/***********************************************************************
 *          WinHttpConnect (winhttp.@)
 */
HINTERNET WINAPI WinHttpConnect( HINTERNET hsession, LPCWSTR server, INTERNET_PORT port, DWORD reserved )
{
    connect_t *connect;
    session_t *session;
    HINTERNET hconnect = NULL;

    TRACE("%p, %s, %u, %x\n", hsession, debugstr_w(server), port, reserved);

    if (!server)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(session = (session_t *)grab_object( hsession )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(connect = heap_alloc_zero( sizeof(connect_t) )))
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

    addref_object( &session->hdr );
    connect->session = session;
    list_add_head( &session->hdr.children, &connect->hdr.entry );

    if (server && !(connect->hostname = strdupW( server ))) goto end;
    connect->hostport = port ? port : (connect->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    if (server && !(connect->servername = strdupW( server ))) goto end;
    connect->serverport = port ? port : (connect->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    if (!(hconnect = alloc_handle( &connect->hdr ))) goto end;
    connect->hdr.handle = hconnect;

    send_callback( &session->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hconnect, sizeof(hconnect) );

end:
    release_object( &connect->hdr );

    TRACE("returning %p\n", hconnect);
    return hconnect;
}

/***********************************************************************
 *          request_destroy (internal)
 */
static void request_destroy( object_header_t *hdr )
{
    request_t *request = (request_t *)hdr;
    int i;

    TRACE("%p\n", request);

    release_object( &request->connect->hdr );

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
    heap_free( request );
}

static BOOL request_query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    switch (option)
    {
    case WINHTTP_OPTION_SECURITY_FLAGS:
    {
        DWORD flags = 0;

        if (hdr->flags & WINHTTP_FLAG_SECURE) flags |= SECURITY_FLAG_SECURE;
        *(DWORD *)buffer = flags;
        *buflen = sizeof(DWORD);
        return TRUE;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        return FALSE;
    }
}

static BOOL request_set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
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
        DWORD disable = *(DWORD *)buffer;

        TRACE("0x%x\n", disable);
        hdr->disable_flags &= disable;
        return TRUE;
    }
    case WINHTTP_OPTION_AUTOLOGON_POLICY:
    {
        DWORD policy = *(DWORD *)buffer;

        TRACE("0x%x\n", policy);
        hdr->logon_policy = policy;
        return TRUE;
    }
    case WINHTTP_OPTION_REDIRECT_POLICY:
    {
        DWORD policy = *(DWORD *)buffer;

        TRACE("0x%x\n", policy);
        hdr->redirect_policy = policy;
        return TRUE;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        return TRUE;
    }
}

static const object_vtbl_t request_vtbl =
{
    request_destroy,
    request_query_option,
    request_set_option
};

/***********************************************************************
 *          WinHttpOpenRequest (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpenRequest( HINTERNET hconnect, LPCWSTR verb, LPCWSTR object, LPCWSTR version,
                                     LPCWSTR referrer, LPCWSTR *types, DWORD flags )
{
    request_t *request;
    connect_t *connect;
    HINTERNET hrequest = NULL;

    TRACE("%p, %s, %s, %s, %s, %p, 0x%08x\n", hconnect, debugstr_w(verb), debugstr_w(object),
          debugstr_w(version), debugstr_w(referrer), types, flags);

    if (!(connect = (connect_t *)grab_object( hconnect )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (connect->hdr.type != WINHTTP_HANDLE_TYPE_CONNECT)
    {
        release_object( &connect->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(request = heap_alloc_zero( sizeof(request_t) )))
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

    addref_object( &connect->hdr );
    request->connect = connect;
    list_add_head( &connect->hdr.children, &request->hdr.entry );

    if (!netconn_init( &request->netconn, request->hdr.flags & WINHTTP_FLAG_SECURE )) goto end;

    if (verb && !(request->verb = strdupW( verb ))) goto end;
    if (object && !(request->path = strdupW( object ))) goto end;
    if (version && !(request->version = strdupW( version ))) goto end;

    if (!(hrequest = alloc_handle( &request->hdr ))) goto end;
    request->hdr.handle = hrequest;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hrequest, sizeof(hrequest) );

end:
    release_object( &request->hdr );

    TRACE("returning %p\n", hrequest);
    return hrequest;
}

/***********************************************************************
 *          WinHttpCloseHandle (winhttp.@)
 */
BOOL WINAPI WinHttpCloseHandle( HINTERNET handle )
{
    object_header_t *hdr;

    TRACE("%p\n", handle);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    release_object( hdr );
    free_handle( handle );
    return TRUE;
}

static BOOL query_option( object_header_t *hdr, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    BOOL ret = FALSE;

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        *(DWORD_PTR *)buffer = hdr->context;
        *buflen = sizeof(DWORD_PTR);
        return TRUE;
    }
    default:
    {
        if (hdr->vtbl->query_option) ret = hdr->vtbl->query_option( hdr, option, buffer, buflen );
        else FIXME("unimplemented option %u\n", option);
    }
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryOption (winhttp.@)
 */
BOOL WINAPI WinHttpQueryOption( HINTERNET handle, DWORD option, LPVOID buffer, LPDWORD buflen )
{
    BOOL ret = FALSE;
    object_header_t *hdr;

    TRACE("%p, %u, %p, %p\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = query_option( hdr, option, buffer, buflen );

    release_object( hdr );
    return ret;
}

static BOOL set_option( object_header_t *hdr, DWORD option, LPVOID buffer, DWORD buflen )
{
    BOOL ret = TRUE;

    switch (option)
    {
    case WINHTTP_OPTION_CONTEXT_VALUE:
    {
        hdr->context = *(DWORD_PTR *)buffer;
        return TRUE;
    }
    default:
    {
        if (hdr->vtbl->set_option) ret = hdr->vtbl->set_option( hdr, option, buffer, buflen );
        else FIXME("unimplemented option %u\n", option);
    }
    }
    return ret;
}

/***********************************************************************
 *          WinHttpSetOption (winhttp.@)
 */
BOOL WINAPI WinHttpSetOption( HINTERNET handle, DWORD option, LPVOID buffer, DWORD buflen )
{
    BOOL ret = FALSE;
    object_header_t *hdr;

    TRACE("%p, %u, %p, %u\n", handle, option, buffer, buflen);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = set_option( hdr, option, buffer, buflen );

    release_object( hdr );
    return ret;
}

/***********************************************************************
 *          WinHttpDetectAutoProxyConfigUrl (winhttp.@)
 */
BOOL WINAPI WinHttpDetectAutoProxyConfigUrl( DWORD flags, LPWSTR *url )
{
    FIXME("0x%08x, %p\n", flags, url);

    set_last_error( ERROR_WINHTTP_AUTODETECTION_FAILED );
    return FALSE;
}

/***********************************************************************
 *          WinHttpGetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpGetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    FIXME("%p\n", info);

    info->dwAccessType    = WINHTTP_ACCESS_TYPE_NO_PROXY;
    info->lpszProxy       = NULL;
    info->lpszProxyBypass = NULL;

    return TRUE;
}

/***********************************************************************
 *          WinHttpGetIEProxyConfigForCurrentUser (winhttp.@)
 */
BOOL WINAPI WinHttpGetIEProxyConfigForCurrentUser( WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *config )
{
    TRACE("%p\n", config);

    if (!config)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* FIXME: read from HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings */

    FIXME("returning no proxy used\n");
    config->fAutoDetect       = FALSE;
    config->lpszAutoConfigUrl = NULL;
    config->lpszProxy         = NULL;
    config->lpszProxyBypass   = NULL;

    return TRUE;
}

/***********************************************************************
 *          WinHttpGetProxyForUrl (winhttp.@)
 */
BOOL WINAPI WinHttpGetProxyForUrl( HINTERNET hsession, LPCWSTR url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                   WINHTTP_PROXY_INFO *info )
{
    FIXME("%p, %s, %p, %p\n", hsession, debugstr_w(url), options, info);

    set_last_error( ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR );
    return FALSE;
}

/***********************************************************************
 *          WinHttpSetDefaultProxyConfiguration (winhttp.@)
 */
BOOL WINAPI WinHttpSetDefaultProxyConfiguration( WINHTTP_PROXY_INFO *info )
{
    FIXME("%p [%u, %s, %s]\n", info, info->dwAccessType, debugstr_w(info->lpszProxy),
          debugstr_w(info->lpszProxyBypass));
    return TRUE;
}

/***********************************************************************
 *          WinHttpSetStatusCallback (winhttp.@)
 */
WINHTTP_STATUS_CALLBACK WINAPI WinHttpSetStatusCallback( HINTERNET handle, WINHTTP_STATUS_CALLBACK callback,
                                                         DWORD flags, DWORD_PTR reserved )
{
    object_header_t *hdr;
    WINHTTP_STATUS_CALLBACK ret;

    TRACE("%p, %p, 0x%08x, 0x%lx\n", handle, callback, flags, reserved);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return WINHTTP_INVALID_STATUS_CALLBACK;
    }
    ret = hdr->callback;
    hdr->callback = callback;
    hdr->notify_mask = flags;

    release_object( hdr );
    return ret;
}

/***********************************************************************
 *          WinHttpSetTimeouts (winhttp.@)
 */
BOOL WINAPI WinHttpSetTimeouts( HINTERNET handle, int resolve, int connect, int send, int receive )
{
    FIXME("%p, %d, %d, %d, %d\n", handle, resolve, connect, send, receive);
    return TRUE;
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

    if (!time || !string) return FALSE;

    sprintfW( string, format,
              wkday[time->wDayOfWeek],
              time->wDay,
              month[time->wMonth - 1],
              time->wYear,
              time->wHour,
              time->wMinute,
              time->wSecond );

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

    if (!string || !time) return FALSE;

    /* Windows does this too */
    GetSystemTime( time );

    /*  Convert an RFC1123 time such as 'Fri, 07 Jan 2005 12:06:35 GMT' into
     *  a SYSTEMTIME structure.
     */

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
    s = end;

    time->wMilliseconds = 0;
    return TRUE;
}
