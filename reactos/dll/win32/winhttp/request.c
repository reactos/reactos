/*
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2006 Rob Shearman for CodeWeavers
 * Copyright 2008, 2011 Hans Leidekker for CodeWeavers
 * Copyright 2009 Juan Lang
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

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include <assert.h>
#include <winuser.h>
#include <httprequest.h>
#include <httprequestid.h>

#include "inet_ntop.c"

static const WCHAR attr_accept[] = {'A','c','c','e','p','t',0};
static const WCHAR attr_accept_charset[] = {'A','c','c','e','p','t','-','C','h','a','r','s','e','t', 0};
static const WCHAR attr_accept_encoding[] = {'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',0};
static const WCHAR attr_accept_language[] = {'A','c','c','e','p','t','-','L','a','n','g','u','a','g','e',0};
static const WCHAR attr_accept_ranges[] = {'A','c','c','e','p','t','-','R','a','n','g','e','s',0};
static const WCHAR attr_age[] = {'A','g','e',0};
static const WCHAR attr_allow[] = {'A','l','l','o','w',0};
static const WCHAR attr_authorization[] = {'A','u','t','h','o','r','i','z','a','t','i','o','n',0};
static const WCHAR attr_cache_control[] = {'C','a','c','h','e','-','C','o','n','t','r','o','l',0};
static const WCHAR attr_connection[] = {'C','o','n','n','e','c','t','i','o','n',0};
static const WCHAR attr_content_base[] = {'C','o','n','t','e','n','t','-','B','a','s','e',0};
static const WCHAR attr_content_encoding[] = {'C','o','n','t','e','n','t','-','E','n','c','o','d','i','n','g',0};
static const WCHAR attr_content_id[] = {'C','o','n','t','e','n','t','-','I','D',0};
static const WCHAR attr_content_language[] = {'C','o','n','t','e','n','t','-','L','a','n','g','u','a','g','e',0};
static const WCHAR attr_content_length[] = {'C','o','n','t','e','n','t','-','L','e','n','g','t','h',0};
static const WCHAR attr_content_location[] = {'C','o','n','t','e','n','t','-','L','o','c','a','t','i','o','n',0};
static const WCHAR attr_content_md5[] = {'C','o','n','t','e','n','t','-','M','D','5',0};
static const WCHAR attr_content_range[] = {'C','o','n','t','e','n','t','-','R','a','n','g','e',0};
static const WCHAR attr_content_transfer_encoding[] = {'C','o','n','t','e','n','t','-','T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0};
static const WCHAR attr_content_type[] = {'C','o','n','t','e','n','t','-','T','y','p','e',0};
static const WCHAR attr_cookie[] = {'C','o','o','k','i','e',0};
static const WCHAR attr_date[] = {'D','a','t','e',0};
static const WCHAR attr_from[] = {'F','r','o','m',0};
static const WCHAR attr_etag[] = {'E','T','a','g',0};
static const WCHAR attr_expect[] = {'E','x','p','e','c','t',0};
static const WCHAR attr_expires[] = {'E','x','p','i','r','e','s',0};
static const WCHAR attr_host[] = {'H','o','s','t',0};
static const WCHAR attr_if_match[] = {'I','f','-','M','a','t','c','h',0};
static const WCHAR attr_if_modified_since[] = {'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0};
static const WCHAR attr_if_none_match[] = {'I','f','-','N','o','n','e','-','M','a','t','c','h',0};
static const WCHAR attr_if_range[] = {'I','f','-','R','a','n','g','e',0};
static const WCHAR attr_if_unmodified_since[] = {'I','f','-','U','n','m','o','d','i','f','i','e','d','-','S','i','n','c','e',0};
static const WCHAR attr_last_modified[] = {'L','a','s','t','-','M','o','d','i','f','i','e','d',0};
static const WCHAR attr_location[] = {'L','o','c','a','t','i','o','n',0};
static const WCHAR attr_max_forwards[] = {'M','a','x','-','F','o','r','w','a','r','d','s',0};
static const WCHAR attr_mime_version[] = {'M','i','m','e','-','V','e','r','s','i','o','n',0};
static const WCHAR attr_pragma[] = {'P','r','a','g','m','a',0};
static const WCHAR attr_proxy_authenticate[] = {'P','r','o','x','y','-','A','u','t','h','e','n','t','i','c','a','t','e',0};
static const WCHAR attr_proxy_authorization[] = {'P','r','o','x','y','-','A','u','t','h','o','r','i','z','a','t','i','o','n',0};
static const WCHAR attr_proxy_connection[] = {'P','r','o','x','y','-','C','o','n','n','e','c','t','i','o','n',0};
static const WCHAR attr_public[] = {'P','u','b','l','i','c',0};
static const WCHAR attr_range[] = {'R','a','n','g','e',0};
static const WCHAR attr_referer[] = {'R','e','f','e','r','e','r',0};
static const WCHAR attr_retry_after[] = {'R','e','t','r','y','-','A','f','t','e','r',0};
static const WCHAR attr_server[] = {'S','e','r','v','e','r',0};
static const WCHAR attr_set_cookie[] = {'S','e','t','-','C','o','o','k','i','e',0};
static const WCHAR attr_status[] = {'S','t','a','t','u','s',0};
static const WCHAR attr_transfer_encoding[] = {'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0};
static const WCHAR attr_unless_modified_since[] = {'U','n','l','e','s','s','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0};
static const WCHAR attr_upgrade[] = {'U','p','g','r','a','d','e',0};
static const WCHAR attr_uri[] = {'U','R','I',0};
static const WCHAR attr_user_agent[] = {'U','s','e','r','-','A','g','e','n','t',0};
static const WCHAR attr_vary[] = {'V','a','r','y',0};
static const WCHAR attr_via[] = {'V','i','a',0};
static const WCHAR attr_warning[] = {'W','a','r','n','i','n','g',0};
static const WCHAR attr_www_authenticate[] = {'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',0};

static const WCHAR *attribute_table[] =
{
    attr_mime_version,              /* WINHTTP_QUERY_MIME_VERSION               = 0  */
    attr_content_type,              /* WINHTTP_QUERY_CONTENT_TYPE               = 1  */
    attr_content_transfer_encoding, /* WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING  = 2  */
    attr_content_id,                /* WINHTTP_QUERY_CONTENT_ID                 = 3  */
    NULL,                           /* WINHTTP_QUERY_CONTENT_DESCRIPTION        = 4  */
    attr_content_length,            /* WINHTTP_QUERY_CONTENT_LENGTH             = 5  */
    attr_content_language,          /* WINHTTP_QUERY_CONTENT_LANGUAGE           = 6  */
    attr_allow,                     /* WINHTTP_QUERY_ALLOW                      = 7  */
    attr_public,                    /* WINHTTP_QUERY_PUBLIC                     = 8  */
    attr_date,                      /* WINHTTP_QUERY_DATE                       = 9  */
    attr_expires,                   /* WINHTTP_QUERY_EXPIRES                    = 10 */
    attr_last_modified,             /* WINHTTP_QUERY_LAST_MODIFIEDcw            = 11 */
    NULL,                           /* WINHTTP_QUERY_MESSAGE_ID                 = 12 */
    attr_uri,                       /* WINHTTP_QUERY_URI                        = 13 */
    attr_from,                      /* WINHTTP_QUERY_DERIVED_FROM               = 14 */
    NULL,                           /* WINHTTP_QUERY_COST                       = 15 */
    NULL,                           /* WINHTTP_QUERY_LINK                       = 16 */
    attr_pragma,                    /* WINHTTP_QUERY_PRAGMA                     = 17 */
    NULL,                           /* WINHTTP_QUERY_VERSION                    = 18 */
    attr_status,                    /* WINHTTP_QUERY_STATUS_CODE                = 19 */
    NULL,                           /* WINHTTP_QUERY_STATUS_TEXT                = 20 */
    NULL,                           /* WINHTTP_QUERY_RAW_HEADERS                = 21 */
    NULL,                           /* WINHTTP_QUERY_RAW_HEADERS_CRLF           = 22 */
    attr_connection,                /* WINHTTP_QUERY_CONNECTION                 = 23 */
    attr_accept,                    /* WINHTTP_QUERY_ACCEPT                     = 24 */
    attr_accept_charset,            /* WINHTTP_QUERY_ACCEPT_CHARSET             = 25 */
    attr_accept_encoding,           /* WINHTTP_QUERY_ACCEPT_ENCODING            = 26 */
    attr_accept_language,           /* WINHTTP_QUERY_ACCEPT_LANGUAGE            = 27 */
    attr_authorization,             /* WINHTTP_QUERY_AUTHORIZATION              = 28 */
    attr_content_encoding,          /* WINHTTP_QUERY_CONTENT_ENCODING           = 29 */
    NULL,                           /* WINHTTP_QUERY_FORWARDED                  = 30 */
    NULL,                           /* WINHTTP_QUERY_FROM                       = 31 */
    attr_if_modified_since,         /* WINHTTP_QUERY_IF_MODIFIED_SINCE          = 32 */
    attr_location,                  /* WINHTTP_QUERY_LOCATION                   = 33 */
    NULL,                           /* WINHTTP_QUERY_ORIG_URI                   = 34 */
    attr_referer,                   /* WINHTTP_QUERY_REFERER                    = 35 */
    attr_retry_after,               /* WINHTTP_QUERY_RETRY_AFTER                = 36 */
    attr_server,                    /* WINHTTP_QUERY_SERVER                     = 37 */
    NULL,                           /* WINHTTP_TITLE                            = 38 */
    attr_user_agent,                /* WINHTTP_QUERY_USER_AGENT                 = 39 */
    attr_www_authenticate,          /* WINHTTP_QUERY_WWW_AUTHENTICATE           = 40 */
    attr_proxy_authenticate,        /* WINHTTP_QUERY_PROXY_AUTHENTICATE         = 41 */
    attr_accept_ranges,             /* WINHTTP_QUERY_ACCEPT_RANGES              = 42 */
    attr_set_cookie,                /* WINHTTP_QUERY_SET_COOKIE                 = 43 */
    attr_cookie,                    /* WINHTTP_QUERY_COOKIE                     = 44 */
    NULL,                           /* WINHTTP_QUERY_REQUEST_METHOD             = 45 */
    NULL,                           /* WINHTTP_QUERY_REFRESH                    = 46 */
    NULL,                           /* WINHTTP_QUERY_CONTENT_DISPOSITION        = 47 */
    attr_age,                       /* WINHTTP_QUERY_AGE                        = 48 */
    attr_cache_control,             /* WINHTTP_QUERY_CACHE_CONTROL              = 49 */
    attr_content_base,              /* WINHTTP_QUERY_CONTENT_BASE               = 50 */
    attr_content_location,          /* WINHTTP_QUERY_CONTENT_LOCATION           = 51 */
    attr_content_md5,               /* WINHTTP_QUERY_CONTENT_MD5                = 52 */
    attr_content_range,             /* WINHTTP_QUERY_CONTENT_RANGE              = 53 */
    attr_etag,                      /* WINHTTP_QUERY_ETAG                       = 54 */
    attr_host,                      /* WINHTTP_QUERY_HOST                       = 55 */
    attr_if_match,                  /* WINHTTP_QUERY_IF_MATCH                   = 56 */
    attr_if_none_match,             /* WINHTTP_QUERY_IF_NONE_MATCH              = 57 */
    attr_if_range,                  /* WINHTTP_QUERY_IF_RANGE                   = 58 */
    attr_if_unmodified_since,       /* WINHTTP_QUERY_IF_UNMODIFIED_SINCE        = 59 */
    attr_max_forwards,              /* WINHTTP_QUERY_MAX_FORWARDS               = 60 */
    attr_proxy_authorization,       /* WINHTTP_QUERY_PROXY_AUTHORIZATION        = 61 */
    attr_range,                     /* WINHTTP_QUERY_RANGE                      = 62 */
    attr_transfer_encoding,         /* WINHTTP_QUERY_TRANSFER_ENCODING          = 63 */
    attr_upgrade,                   /* WINHTTP_QUERY_UPGRADE                    = 64 */
    attr_vary,                      /* WINHTTP_QUERY_VARY                       = 65 */
    attr_via,                       /* WINHTTP_QUERY_VIA                        = 66 */
    attr_warning,                   /* WINHTTP_QUERY_WARNING                    = 67 */
    attr_expect,                    /* WINHTTP_QUERY_EXPECT                     = 68 */
    attr_proxy_connection,          /* WINHTTP_QUERY_PROXY_CONNECTION           = 69 */
    attr_unless_modified_since,     /* WINHTTP_QUERY_UNLESS_MODIFIED_SINCE      = 70 */
    NULL,                           /* WINHTTP_QUERY_PROXY_SUPPORT              = 75 */
    NULL,                           /* WINHTTP_QUERY_AUTHENTICATION_INFO        = 76 */
    NULL,                           /* WINHTTP_QUERY_PASSPORT_URLS              = 77 */
    NULL                            /* WINHTTP_QUERY_PASSPORT_CONFIG            = 78 */
};

static task_header_t *dequeue_task( request_t *request )
{
    task_header_t *task;

    EnterCriticalSection( &request->task_cs );
    TRACE("%u tasks queued\n", list_count( &request->task_queue ));
    task = LIST_ENTRY( list_head( &request->task_queue ), task_header_t, entry );
    if (task) list_remove( &task->entry );
    LeaveCriticalSection( &request->task_cs );

    TRACE("returning task %p\n", task);
    return task;
}

static DWORD CALLBACK task_proc( LPVOID param )
{
    request_t *request = param;
    HANDLE handles[2];

    handles[0] = request->task_wait;
    handles[1] = request->task_cancel;
    for (;;)
    {
        DWORD err = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
        switch (err)
        {
        case WAIT_OBJECT_0:
        {
            task_header_t *task;
            while ((task = dequeue_task( request )))
            {
                task->proc( task );
                release_object( &task->request->hdr );
                heap_free( task );
            }
            break;
        }
        case WAIT_OBJECT_0 + 1:
            TRACE("exiting\n");
            CloseHandle( request->task_cancel );
            CloseHandle( request->task_wait );
            request->task_cs.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection( &request->task_cs );
            request->hdr.vtbl->destroy( &request->hdr );
            return 0;

        default:
            ERR("wait failed %u (%u)\n", err, GetLastError());
            break;
        }
    }
    return 0;
}

static BOOL queue_task( task_header_t *task )
{
    request_t *request = task->request;

    if (!request->task_thread)
    {
        if (!(request->task_wait = CreateEventW( NULL, FALSE, FALSE, NULL ))) return FALSE;
        if (!(request->task_cancel = CreateEventW( NULL, FALSE, FALSE, NULL )))
        {
            CloseHandle( request->task_wait );
            request->task_wait = NULL;
            return FALSE;
        }
        if (!(request->task_thread = CreateThread( NULL, 0, task_proc, request, 0, NULL )))
        {
            CloseHandle( request->task_wait );
            request->task_wait = NULL;
            CloseHandle( request->task_cancel );
            request->task_cancel = NULL;
            return FALSE;
        }
        InitializeCriticalSection( &request->task_cs );
        request->task_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": request.task_cs");
    }

    EnterCriticalSection( &request->task_cs );
    TRACE("queueing task %p\n", task );
    list_add_tail( &request->task_queue, &task->entry );
    LeaveCriticalSection( &request->task_cs );

    SetEvent( request->task_wait );
    return TRUE;
}

static void free_header( header_t *header )
{
    heap_free( header->field );
    heap_free( header->value );
    heap_free( header );
}

static BOOL valid_token_char( WCHAR c )
{
    if (c < 32 || c == 127) return FALSE;
    switch (c)
    {
    case '(': case ')':
    case '<': case '>':
    case '@': case ',':
    case ';': case ':':
    case '\\': case '\"':
    case '/': case '[':
    case ']': case '?':
    case '=': case '{':
    case '}': case ' ':
    case '\t':
        return FALSE;
    default:
        return TRUE;
    }
}

static header_t *parse_header( LPCWSTR string )
{
    const WCHAR *p, *q;
    header_t *header;
    int len;

    p = string;
    if (!(q = strchrW( p, ':' )))
    {
        WARN("no ':' in line %s\n", debugstr_w(string));
        return NULL;
    }
    if (q == string)
    {
        WARN("empty field name in line %s\n", debugstr_w(string));
        return NULL;
    }
    while (*p != ':')
    {
        if (!valid_token_char( *p ))
        {
            WARN("invalid character in field name %s\n", debugstr_w(string));
            return NULL;
        }
        p++;
    }
    len = q - string;
    if (!(header = heap_alloc_zero( sizeof(header_t) ))) return NULL;
    if (!(header->field = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( header );
        return NULL;
    }
    memcpy( header->field, string, len * sizeof(WCHAR) );
    header->field[len] = 0;

    q++; /* skip past colon */
    while (*q == ' ') q++;
    len = strlenW( q );

    if (!(header->value = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        free_header( header );
        return NULL;
    }
    memcpy( header->value, q, len * sizeof(WCHAR) );
    header->value[len] = 0;

    return header;
}

static int get_header_index( request_t *request, LPCWSTR field, int requested_index, BOOL request_only )
{
    int index;

    TRACE("%s\n", debugstr_w(field));

    for (index = 0; index < request->num_headers; index++)
    {
        if (strcmpiW( request->headers[index].field, field )) continue;
        if (request_only && !request->headers[index].is_request) continue;
        if (!request_only && request->headers[index].is_request) continue;

        if (!requested_index) break;
        requested_index--;
    }
    if (index >= request->num_headers) index = -1;
    TRACE("returning %d\n", index);
    return index;
}

static BOOL insert_header( request_t *request, header_t *header )
{
    DWORD count;
    header_t *hdrs;

    count = request->num_headers + 1;
    if (count > 1)
        hdrs = heap_realloc_zero( request->headers, sizeof(header_t) * count );
    else
        hdrs = heap_alloc_zero( sizeof(header_t) * count );

    if (hdrs)
    {
        request->headers = hdrs;
        request->headers[count - 1].field = strdupW( header->field );
        request->headers[count - 1].value = strdupW( header->value );
        request->headers[count - 1].is_request = header->is_request;
        request->num_headers++;
        return TRUE;
    }
    return FALSE;
}

static BOOL delete_header( request_t *request, DWORD index )
{
    if (!request->num_headers) return FALSE;
    if (index >= request->num_headers) return FALSE;
    request->num_headers--;

    heap_free( request->headers[index].field );
    heap_free( request->headers[index].value );

    memmove( &request->headers[index], &request->headers[index + 1], (request->num_headers - index) * sizeof(header_t) );
    memset( &request->headers[request->num_headers], 0, sizeof(header_t) );
    return TRUE;
}

BOOL process_header( request_t *request, LPCWSTR field, LPCWSTR value, DWORD flags, BOOL request_only )
{
    int index;
    header_t hdr;

    TRACE("%s: %s 0x%08x\n", debugstr_w(field), debugstr_w(value), flags);

    if ((index = get_header_index( request, field, 0, request_only )) >= 0)
    {
        if (flags & WINHTTP_ADDREQ_FLAG_ADD_IF_NEW) return FALSE;
    }

    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE)
    {
        if (index >= 0)
        {
            delete_header( request, index );
            if (!value || !value[0]) return TRUE;
        }
        else if (!(flags & WINHTTP_ADDREQ_FLAG_ADD))
        {
            set_last_error( ERROR_WINHTTP_HEADER_NOT_FOUND );
            return FALSE;
        }

        hdr.field = (LPWSTR)field;
        hdr.value = (LPWSTR)value;
        hdr.is_request = request_only;
        return insert_header( request, &hdr );
    }
    else if (value)
    {

        if ((flags & (WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON)) &&
            index >= 0)
        {
            WCHAR *tmp;
            int len, len_orig, len_value;
            header_t *header = &request->headers[index];

            len_orig = strlenW( header->value );
            len_value = strlenW( value );

            len = len_orig + len_value + 2;
            if (!(tmp = heap_realloc( header->value, (len + 1) * sizeof(WCHAR) ))) return FALSE;
            header->value = tmp;
            header->value[len_orig++] = (flags & WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA) ? ',' : ';';
            header->value[len_orig++] = ' ';

            memcpy( &header->value[len_orig], value, len_value * sizeof(WCHAR) );
            header->value[len] = 0;
            return TRUE;
        }
        else
        {
            hdr.field = (LPWSTR)field;
            hdr.value = (LPWSTR)value;
            hdr.is_request = request_only;
            return insert_header( request, &hdr );
        }
    }

    return TRUE;
}

BOOL add_request_headers( request_t *request, LPCWSTR headers, DWORD len, DWORD flags )
{
    BOOL ret = FALSE;
    WCHAR *buffer, *p, *q;
    header_t *header;

    if (len == ~0u) len = strlenW( headers );
    if (!len) return TRUE;
    if (!(buffer = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;
    memcpy( buffer, headers, len * sizeof(WCHAR) );
    buffer[len] = 0;

    p = buffer;
    do
    {
        q = p;
        while (*q)
        {
            if (q[0] == '\n' && q[1] == '\r')
            {
                q[0] = '\r';
                q[1] = '\n';
            }
            if (q[0] == '\r' && q[1] == '\n') break;
            q++;
        }
        if (!*p) break;
        if (*q == '\r')
        {
            *q = 0;
            q += 2; /* jump over \r\n */
        }
        if ((header = parse_header( p )))
        {
            ret = process_header( request, header->field, header->value, flags, TRUE );
            free_header( header );
        }
        p = q;
    } while (ret);

    heap_free( buffer );
    return ret;
}

/***********************************************************************
 *          WinHttpAddRequestHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpAddRequestHeaders( HINTERNET hrequest, LPCWSTR headers, DWORD len, DWORD flags )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %s, 0x%x, 0x%08x\n", hrequest, debugstr_w(headers), len, flags);

    if (!headers || !len)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = add_request_headers( request, headers, len, flags );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static WCHAR *build_request_path( request_t *request )
{
    WCHAR *ret;

    if (strcmpiW( request->connect->hostname, request->connect->servername ))
    {
        static const WCHAR http[] = { 'h','t','t','p',0 };
        static const WCHAR https[] = { 'h','t','t','p','s',0 };
        static const WCHAR fmt[] = { '%','s',':','/','/','%','s',0 };
        LPCWSTR scheme = request->netconn.secure ? https : http;
        int len;

        len = strlenW( scheme ) + strlenW( request->connect->hostname );
        /* 3 characters for '://', 1 for NUL. */
        len += 4;
        if (request->connect->hostport)
        {
            /* 1 for ':' between host and port, up to 5 for port */
            len += 6;
        }
        if (request->path)
            len += strlenW( request->path );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
        {
            sprintfW( ret, fmt, scheme, request->connect->hostname );
            if (request->connect->hostport)
            {
                static const WCHAR colonFmt[] = { ':','%','u',0 };

                sprintfW( ret + strlenW( ret ), colonFmt,
                    request->connect->hostport );
            }
            if (request->path)
                strcatW( ret, request->path );
        }
    }
    else
        ret = request->path;
    return ret;
}

static WCHAR *build_request_string( request_t *request )
{
    static const WCHAR space[]   = {' ',0};
    static const WCHAR crlf[]    = {'\r','\n',0};
    static const WCHAR colon[]   = {':',' ',0};
    static const WCHAR twocrlf[] = {'\r','\n','\r','\n',0};

    WCHAR *path, *ret;
    const WCHAR **headers, **p;
    unsigned int len, i = 0, j;

    /* allocate space for an array of all the string pointers to be added */
    len = request->num_headers * 4 + 7;
    if (!(headers = heap_alloc( len * sizeof(LPCWSTR) ))) return NULL;

    path = build_request_path( request );
    headers[i++] = request->verb;
    headers[i++] = space;
    headers[i++] = path;
    headers[i++] = space;
    headers[i++] = request->version;

    for (j = 0; j < request->num_headers; j++)
    {
        if (request->headers[j].is_request)
        {
            headers[i++] = crlf;
            headers[i++] = request->headers[j].field;
            headers[i++] = colon;
            headers[i++] = request->headers[j].value;

            TRACE("adding header %s (%s)\n", debugstr_w(request->headers[j].field),
                  debugstr_w(request->headers[j].value));
        }
    }
    headers[i++] = twocrlf;
    headers[i] = NULL;

    len = 0;
    for (p = headers; *p; p++) len += strlenW( *p );
    len++;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) )))
        goto out;
    *ret = 0;
    for (p = headers; *p; p++) strcatW( ret, *p );

out:
    if (path != request->path)
        heap_free( path );
    heap_free( headers );
    return ret;
}

#define QUERY_MODIFIER_MASK (WINHTTP_QUERY_FLAG_REQUEST_HEADERS | WINHTTP_QUERY_FLAG_SYSTEMTIME | WINHTTP_QUERY_FLAG_NUMBER)

static BOOL query_headers( request_t *request, DWORD level, LPCWSTR name, LPVOID buffer, LPDWORD buflen, LPDWORD index )
{
    header_t *header = NULL;
    BOOL request_only, ret = FALSE;
    int requested_index, header_index = -1;
    DWORD attr, len;

    request_only = level & WINHTTP_QUERY_FLAG_REQUEST_HEADERS;
    requested_index = index ? *index : 0;

    attr = level & ~QUERY_MODIFIER_MASK;
    switch (attr)
    {
    case WINHTTP_QUERY_CUSTOM:
    {
        header_index = get_header_index( request, name, requested_index, request_only );
        break;
    }
    case WINHTTP_QUERY_RAW_HEADERS:
    {
        WCHAR *headers, *p, *q;

        if (request_only)
            headers = build_request_string( request );
        else
            headers = request->raw_headers;

        if (!(p = headers)) return FALSE;
        for (len = 0; *p; p++) if (*p != '\r') len++;

        if (!buffer || len * sizeof(WCHAR) > *buflen)
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        else
        {
            for (p = headers, q = buffer; *p; p++, q++)
            {
                if (*p != '\r') *q = *p;
                else
                {
                    *q = 0;
                    p++; /* skip '\n' */
                }
            }
            TRACE("returning data: %s\n", debugstr_wn(buffer, len));
            if (len) len--;
            ret = TRUE;
        }
        *buflen = len * sizeof(WCHAR);
        if (request_only) heap_free( headers );
        return ret;
    }
    case WINHTTP_QUERY_RAW_HEADERS_CRLF:
    {
        WCHAR *headers;

        if (request_only)
            headers = build_request_string( request );
        else
            headers = request->raw_headers;

        if (!headers) return FALSE;
        len = strlenW( headers ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            memcpy( buffer, headers, len + sizeof(WCHAR) );
            TRACE("returning data: %s\n", debugstr_wn(buffer, len / sizeof(WCHAR)));
            ret = TRUE;
        }
        *buflen = len;
        if (request_only) heap_free( headers );
        return ret;
    }
    case WINHTTP_QUERY_VERSION:
        len = strlenW( request->version ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            strcpyW( buffer, request->version );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = TRUE;
        }
        *buflen = len;
        return ret;

    case WINHTTP_QUERY_STATUS_TEXT:
        len = strlenW( request->status_text ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            strcpyW( buffer, request->status_text );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = TRUE;
        }
        *buflen = len;
        return ret;

    default:
        if (attr >= sizeof(attribute_table)/sizeof(attribute_table[0]) || !attribute_table[attr])
        {
            FIXME("attribute %u not implemented\n", attr);
            return FALSE;
        }
        TRACE("attribute %s\n", debugstr_w(attribute_table[attr]));
        header_index = get_header_index( request, attribute_table[attr], requested_index, request_only );
        break;
    }

    if (header_index >= 0)
    {
        header = &request->headers[header_index];
    }
    if (!header || (request_only && !header->is_request))
    {
        set_last_error( ERROR_WINHTTP_HEADER_NOT_FOUND );
        return FALSE;
    }
    if (index) *index += 1;
    if (level & WINHTTP_QUERY_FLAG_NUMBER)
    {
        if (!buffer || sizeof(int) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            int *number = buffer;
            *number = atoiW( header->value );
            TRACE("returning number: %d\n", *number);
            ret = TRUE;
        }
        *buflen = sizeof(int);
    }
    else if (level & WINHTTP_QUERY_FLAG_SYSTEMTIME)
    {
        SYSTEMTIME *st = buffer;
        if (!buffer || sizeof(SYSTEMTIME) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if ((ret = WinHttpTimeToSystemTime( header->value, st )))
        {
            TRACE("returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
                  st->wYear, st->wMonth, st->wDay, st->wDayOfWeek,
                  st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
        }
        *buflen = sizeof(SYSTEMTIME);
    }
    else if (header->value)
    {
        len = strlenW( header->value ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            strcpyW( buffer, header->value );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = TRUE;
        }
        *buflen = len;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpQueryHeaders( HINTERNET hrequest, DWORD level, LPCWSTR name, LPVOID buffer, LPDWORD buflen, LPDWORD index )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, 0x%08x, %s, %p, %p, %p\n", hrequest, level, debugstr_w(name), buffer, buflen, index);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = query_headers( request, level, name, buffer, buflen, index );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static LPWSTR concatenate_string_list( LPCWSTR *list, int len )
{
    LPCWSTR *t;
    LPWSTR str;

    for( t = list; *t ; t++  )
        len += strlenW( *t );
    len++;

    str = heap_alloc( len * sizeof(WCHAR) );
    if (!str) return NULL;
    *str = 0;

    for( t = list; *t ; t++ )
        strcatW( str, *t );

    return str;
}

static LPWSTR build_header_request_string( request_t *request, LPCWSTR verb,
    LPCWSTR path, LPCWSTR version )
{
    static const WCHAR crlf[] = {'\r','\n',0};
    static const WCHAR space[] = { ' ',0 };
    static const WCHAR colon[] = { ':',' ',0 };
    static const WCHAR twocrlf[] = {'\r','\n','\r','\n', 0};
    LPWSTR requestString;
    DWORD len, n;
    LPCWSTR *req;
    UINT i;
    LPWSTR p;

    /* allocate space for an array of all the string pointers to be added */
    len = (request->num_headers) * 4 + 10;
    req = heap_alloc( len * sizeof(LPCWSTR) );
    if (!req) return NULL;

    /* add the verb, path and HTTP version string */
    n = 0;
    req[n++] = verb;
    req[n++] = space;
    req[n++] = path;
    req[n++] = space;
    req[n++] = version;

    /* Append custom request headers */
    for (i = 0; i < request->num_headers; i++)
    {
        if (request->headers[i].is_request)
        {
            req[n++] = crlf;
            req[n++] = request->headers[i].field;
            req[n++] = colon;
            req[n++] = request->headers[i].value;

            TRACE("Adding custom header %s (%s)\n",
                   debugstr_w(request->headers[i].field),
                   debugstr_w(request->headers[i].value));
        }
    }

    if( n >= len )
        ERR("oops. buffer overrun\n");

    req[n] = NULL;
    requestString = concatenate_string_list( req, 4 );
    heap_free( req );
    if (!requestString) return NULL;

    /*
     * Set (header) termination string for request
     * Make sure there are exactly two new lines at the end of the request
     */
    p = &requestString[strlenW(requestString)-1];
    while ( (*p == '\n') || (*p == '\r') )
       p--;
    strcpyW( p+1, twocrlf );

    return requestString;
}

static BOOL read_reply( request_t *request );

static BOOL secure_proxy_connect( request_t *request )
{
    static const WCHAR verbConnect[] = {'C','O','N','N','E','C','T',0};
    static const WCHAR fmt[] = {'%','s',':','%','u',0};
    BOOL ret = FALSE;
    LPWSTR path;
    connect_t *connect = request->connect;

    path = heap_alloc( (strlenW( connect->hostname ) + 13) * sizeof(WCHAR) );
    if (path)
    {
        LPWSTR requestString;

        sprintfW( path, fmt, connect->hostname, connect->hostport );
        requestString = build_header_request_string( request, verbConnect,
            path, http1_1 );
        heap_free( path );
        if (requestString)
        {
            LPSTR req_ascii = strdupWA( requestString );

            heap_free( requestString );
            if (req_ascii)
            {
                int len = strlen( req_ascii ), bytes_sent;

                ret = netconn_send( &request->netconn, req_ascii, len, &bytes_sent );
                heap_free( req_ascii );
                if (ret)
                    ret = read_reply( request );
            }
        }
    }
    return ret;
}

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static WCHAR *addr_to_str( struct sockaddr *addr )
{
    char buf[INET6_ADDRSTRLEN];
    void *src;

    switch (addr->sa_family)
    {
    case AF_INET:
        src = &((struct sockaddr_in *)addr)->sin_addr;
        break;
    case AF_INET6:
        src = &((struct sockaddr_in6 *)addr)->sin6_addr;
        break;
    default:
        WARN("unsupported address family %d\n", addr->sa_family);
        return NULL;
    }
    if (!inet_ntop( addr->sa_family, src, buf, sizeof(buf) )) return NULL;
    return strdupAW( buf );
}

static BOOL open_connection( request_t *request )
{
    connect_t *connect;
    WCHAR *addressW = NULL;
    INTERNET_PORT port;
    socklen_t slen;
    struct sockaddr *saddr;
    DWORD len;

    if (netconn_connected( &request->netconn )) goto done;

    connect = request->connect;
    port = connect->serverport ? connect->serverport : (request->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);
    saddr = (struct sockaddr *)&connect->sockaddr;
    slen = sizeof(struct sockaddr);

    if (!connect->resolved)
    {
        len = strlenW( connect->servername ) + 1;
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, connect->servername, len );

        if (!netconn_resolve( connect->servername, port, saddr, &slen, request->resolve_timeout )) return FALSE;
        connect->resolved = TRUE;

        if (!(addressW = addr_to_str( saddr ))) return FALSE;
        len = strlenW( addressW ) + 1;
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, addressW, len );
    }
    if (!addressW && !(addressW = addr_to_str( saddr ))) return FALSE;
    TRACE("connecting to %s:%u\n", debugstr_w(addressW), port);

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, addressW, 0 );

    if (!netconn_create( &request->netconn, saddr->sa_family, SOCK_STREAM, 0 ))
    {
        heap_free( addressW );
        return FALSE;
    }
    netconn_set_timeout( &request->netconn, TRUE, request->send_timeout );
    netconn_set_timeout( &request->netconn, FALSE, request->recv_timeout );
    if (!netconn_connect( &request->netconn, saddr, slen, request->connect_timeout ))
    {
        netconn_close( &request->netconn );
        heap_free( addressW );
        return FALSE;
    }
    if (request->hdr.flags & WINHTTP_FLAG_SECURE)
    {
        if (connect->session->proxy_server &&
            strcmpiW( connect->hostname, connect->servername ))
        {
            if (!secure_proxy_connect( request ))
            {
                heap_free( addressW );
                return FALSE;
            }
        }
        if (!netconn_secure_connect( &request->netconn, connect->hostname ))
        {
            netconn_close( &request->netconn );
            heap_free( addressW );
            return FALSE;
        }
    }

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, addressW, strlenW(addressW) + 1 );

done:
    request->read_pos = request->read_size = 0;
    request->read_chunked = FALSE;
    request->read_chunked_size = ~0u;
    request->read_chunked_eof = FALSE;
    heap_free( addressW );
    return TRUE;
}

void close_connection( request_t *request )
{
    if (!netconn_connected( &request->netconn )) return;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, 0, 0 );
    netconn_close( &request->netconn );
    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, 0, 0 );
}

static BOOL add_host_header( request_t *request, DWORD modifier )
{
    BOOL ret;
    DWORD len;
    WCHAR *host;
    static const WCHAR fmt[] = {'%','s',':','%','u',0};
    connect_t *connect = request->connect;
    INTERNET_PORT port;

    port = connect->hostport ? connect->hostport : (request->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    if (port == INTERNET_DEFAULT_HTTP_PORT || port == INTERNET_DEFAULT_HTTPS_PORT)
    {
        return process_header( request, attr_host, connect->hostname, modifier, TRUE );
    }
    len = strlenW( connect->hostname ) + 7; /* sizeof(":65335") */
    if (!(host = heap_alloc( len * sizeof(WCHAR) ))) return FALSE;
    sprintfW( host, fmt, connect->hostname, port );
    ret = process_header( request, attr_host, host, modifier, TRUE );
    heap_free( host );
    return ret;
}

static void clear_response_headers( request_t *request )
{
    unsigned int i;

    for (i = 0; i < request->num_headers; i++)
    {
        if (!request->headers[i].field) continue;
        if (!request->headers[i].value) continue;
        if (request->headers[i].is_request) continue;
        delete_header( request, i );
        i--;
    }
}

static BOOL send_request( request_t *request, LPCWSTR headers, DWORD headers_len, LPVOID optional,
                          DWORD optional_len, DWORD total_len, DWORD_PTR context, BOOL async )
{
    static const WCHAR keep_alive[] = {'K','e','e','p','-','A','l','i','v','e',0};
    static const WCHAR no_cache[]   = {'n','o','-','c','a','c','h','e',0};
    static const WCHAR length_fmt[] = {'%','l','d',0};

    BOOL ret = FALSE;
    connect_t *connect = request->connect;
    session_t *session = connect->session;
    WCHAR *req = NULL;
    char *req_ascii;
    int bytes_sent;
    DWORD len;

    clear_response_headers( request );

    if (session->agent)
        process_header( request, attr_user_agent, session->agent, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );

    if (connect->hostname)
        add_host_header( request, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW );

    if (total_len || (request->verb && !strcmpW( request->verb, postW )))
    {
        WCHAR length[21]; /* decimal long int + null */
        sprintfW( length, length_fmt, total_len );
        process_header( request, attr_content_length, length, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (!(request->hdr.disable_flags & WINHTTP_DISABLE_KEEP_ALIVE))
    {
        process_header( request, attr_connection, keep_alive, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (request->hdr.flags & WINHTTP_FLAG_REFRESH)
    {
        process_header( request, attr_pragma, no_cache, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
        process_header( request, attr_cache_control, no_cache, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (headers && !add_request_headers( request, headers, headers_len, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ))
    {
        TRACE("failed to add request headers\n");
        return FALSE;
    }
    if (!(request->hdr.disable_flags & WINHTTP_DISABLE_COOKIES) && !add_cookie_headers( request ))
    {
        WARN("failed to add cookie headers\n");
        return FALSE;
    }

    if (context) request->hdr.context = context;

    if (!(ret = open_connection( request ))) goto end;
    if (!(req = build_request_string( request ))) goto end;

    if (!(req_ascii = strdupWA( req ))) goto end;
    TRACE("full request: %s\n", debugstr_a(req_ascii));
    len = strlen(req_ascii);

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, NULL, 0 );

    ret = netconn_send( &request->netconn, req_ascii, len, &bytes_sent );
    heap_free( req_ascii );
    if (!ret) goto end;

    if (optional_len)
    {
        if (!netconn_send( &request->netconn, optional, optional_len, &bytes_sent )) goto end;
        request->optional = optional;
        request->optional_len = optional_len;
        len += optional_len;
    }
    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_SENT, &len, sizeof(len) );

end:
    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NULL, 0 );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_SEND_REQUEST;
            result.dwError  = get_last_error();
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    heap_free( req );
    return ret;
}

static void task_send_request( task_header_t *task )
{
    send_request_t *s = (send_request_t *)task;
    send_request( s->hdr.request, s->headers, s->headers_len, s->optional, s->optional_len, s->total_len, s->context, TRUE );
    heap_free( s->headers );
}

/***********************************************************************
 *          WinHttpSendRequest (winhttp.@)
 */
BOOL WINAPI WinHttpSendRequest( HINTERNET hrequest, LPCWSTR headers, DWORD headers_len,
                                LPVOID optional, DWORD optional_len, DWORD total_len, DWORD_PTR context )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %s, 0x%x, %u, %u, %lx\n",
          hrequest, debugstr_w(headers), headers_len, optional_len, total_len, context);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (headers && !headers_len) headers_len = strlenW( headers );

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        send_request_t *s;

        if (!(s = heap_alloc( sizeof(send_request_t) ))) return FALSE;
        s->hdr.request  = request;
        s->hdr.proc     = task_send_request;
        s->headers      = strdupW( headers );
        s->headers_len  = headers_len;
        s->optional     = optional;
        s->optional_len = optional_len;
        s->total_len    = total_len;
        s->context      = context;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)s );
    }
    else
        ret = send_request( request, headers, headers_len, optional, optional_len, total_len, context, FALSE );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

#undef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))

static const WCHAR basicW[]     = {'B','a','s','i','c',0};
static const WCHAR ntlmW[]      = {'N','T','L','M',0};
static const WCHAR passportW[]  = {'P','a','s','s','p','o','r','t',0};
static const WCHAR digestW[]    = {'D','i','g','e','s','t',0};
static const WCHAR negotiateW[] = {'N','e','g','o','t','i','a','t','e',0};

static const struct
{
    const WCHAR *str;
    unsigned int len;
    DWORD scheme;
}
auth_schemes[] =
{
    { basicW,     ARRAYSIZE(basicW) - 1,     WINHTTP_AUTH_SCHEME_BASIC },
    { ntlmW,      ARRAYSIZE(ntlmW) - 1,      WINHTTP_AUTH_SCHEME_NTLM },
    { passportW,  ARRAYSIZE(passportW) - 1,  WINHTTP_AUTH_SCHEME_PASSPORT },
    { digestW,    ARRAYSIZE(digestW) - 1,    WINHTTP_AUTH_SCHEME_DIGEST },
    { negotiateW, ARRAYSIZE(negotiateW) - 1, WINHTTP_AUTH_SCHEME_NEGOTIATE }
};
static const unsigned int num_auth_schemes = sizeof(auth_schemes)/sizeof(auth_schemes[0]);

static enum auth_scheme scheme_from_flag( DWORD flag )
{
    int i;

    for (i = 0; i < num_auth_schemes; i++) if (flag == auth_schemes[i].scheme) return i;
    return SCHEME_INVALID;
}

static DWORD auth_scheme_from_header( WCHAR *header )
{
    unsigned int i;

    for (i = 0; i < num_auth_schemes; i++)
    {
        if (!strncmpiW( header, auth_schemes[i].str, auth_schemes[i].len ) &&
            (header[auth_schemes[i].len] == ' ' || !header[auth_schemes[i].len])) return auth_schemes[i].scheme;
    }
    return 0;
}

static BOOL query_auth_schemes( request_t *request, DWORD level, LPDWORD supported, LPDWORD first )
{
    DWORD index = 0;
    BOOL ret = FALSE;

    for (;;)
    {
        WCHAR *buffer;
        DWORD size, scheme;

        size = 0;
        query_headers( request, level, NULL, NULL, &size, &index );
        if (get_last_error() != ERROR_INSUFFICIENT_BUFFER) break;

        index--;
        if (!(buffer = heap_alloc( size ))) return FALSE;
        if (!query_headers( request, level, NULL, buffer, &size, &index ))
        {
            heap_free( buffer );
            return FALSE;
        }
        scheme = auth_scheme_from_header( buffer );
        heap_free( buffer );
        if (!scheme) break;

        if (first && index == 1)
            *first = *supported = scheme;
        else
            *supported |= scheme;

        ret = TRUE;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryAuthSchemes (winhttp.@)
 */
BOOL WINAPI WinHttpQueryAuthSchemes( HINTERNET hrequest, LPDWORD supported, LPDWORD first, LPDWORD target )
{
    BOOL ret = FALSE;
    request_t *request;

    TRACE("%p, %p, %p, %p\n", hrequest, supported, first, target);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }
    if (!supported || !first || !target)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;

    }

    if (query_auth_schemes( request, WINHTTP_QUERY_WWW_AUTHENTICATE, supported, first ))
    {
        *target = WINHTTP_AUTH_TARGET_SERVER;
        ret = TRUE;
    }
    else if (query_auth_schemes( request, WINHTTP_QUERY_PROXY_AUTHENTICATE, supported, first ))
    {
        *target = WINHTTP_AUTH_TARGET_PROXY;
        ret = TRUE;
    }

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static UINT encode_base64( const char *bin, unsigned int len, WCHAR *base64 )
{
    UINT n = 0, x;
    static const char base64enc[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (len > 0)
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = base64enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if (len == 1)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[1] & 0xf0) >> 4)];
        x = (bin[1] & 0x0f) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if (len == 2)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[2] & 0xc0) >> 6)];

        /* last 6 bits, all from bin [2] */
        base64[n++] = base64enc[bin[2] & 0x3f];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

static inline char decode_char( WCHAR c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 64;
}

static unsigned int decode_base64( const WCHAR *base64, unsigned int len, char *buf )
{
    unsigned int i = 0;
    char c0, c1, c2, c3;
    const WCHAR *p = base64;

    while (len > 4)
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        len -= 4;
        i += 3;
        p += 4;
    }
    if (p[2] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;

        if (buf) buf[i] = (c0 << 2) | (c1 >> 4);
        i++;
    }
    else if (p[3] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
        }
        i += 2;
    }
    else
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        i += 3;
    }
    return i;
}

static struct authinfo *alloc_authinfo(void)
{
    struct authinfo *ret;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;

    SecInvalidateHandle( &ret->cred );
    SecInvalidateHandle( &ret->ctx );
    memset( &ret->exp, 0, sizeof(ret->exp) );
    ret->scheme    = 0;
    ret->attr      = 0;
    ret->max_token = 0;
    ret->data      = NULL;
    ret->data_len  = 0;
    ret->finished  = FALSE;
    return ret;
}

void destroy_authinfo( struct authinfo *authinfo )
{
    if (!authinfo) return;

    if (SecIsValidHandle( &authinfo->ctx ))
        DeleteSecurityContext( &authinfo->ctx );
    if (SecIsValidHandle( &authinfo->cred ))
        FreeCredentialsHandle( &authinfo->cred );

    heap_free( authinfo->data );
    heap_free( authinfo );
}

static BOOL get_authvalue( request_t *request, DWORD level, DWORD scheme, WCHAR *buffer, DWORD len )
{
    DWORD size, index = 0;
    for (;;)
    {
        size = len;
        if (!query_headers( request, level, NULL, buffer, &size, &index )) return FALSE;
        if (auth_scheme_from_header( buffer ) == scheme) break;
    }
    return TRUE;
}

static BOOL do_authorization( request_t *request, DWORD target, DWORD scheme_flag )
{
    struct authinfo *authinfo, **auth_ptr;
    enum auth_scheme scheme = scheme_from_flag( scheme_flag );
    const WCHAR *auth_target, *username, *password;
    WCHAR auth_value[2048], *auth_reply;
    DWORD len = sizeof(auth_value), len_scheme, flags;
    BOOL ret;

    if (scheme == SCHEME_INVALID) return FALSE;

    switch (target)
    {
    case WINHTTP_AUTH_TARGET_SERVER:
        if (!get_authvalue( request, WINHTTP_QUERY_WWW_AUTHENTICATE, scheme_flag, auth_value, len ))
            return FALSE;
        auth_ptr = &request->authinfo;
        auth_target = attr_authorization;
        if (request->creds[TARGET_SERVER][scheme].username)
        {
            username = request->creds[TARGET_SERVER][scheme].username;
            password = request->creds[TARGET_SERVER][scheme].password;
        }
        else
        {
            username = request->connect->username;
            password = request->connect->password;
        }
        break;

    case WINHTTP_AUTH_TARGET_PROXY:
        if (!get_authvalue( request, WINHTTP_QUERY_PROXY_AUTHENTICATE, scheme_flag, auth_value, len ))
            return FALSE;
        auth_ptr = &request->proxy_authinfo;
        auth_target = attr_proxy_authorization;
        if (request->creds[TARGET_PROXY][scheme].username)
        {
            username = request->creds[TARGET_PROXY][scheme].username;
            password = request->creds[TARGET_PROXY][scheme].password;
        }
        else
        {
            username = request->connect->session->proxy_username;
            password = request->connect->session->proxy_password;
        }
        break;

    default:
        WARN("unknown target %x\n", target);
        return FALSE;
    }
    authinfo = *auth_ptr;

    switch (scheme)
    {
    case SCHEME_BASIC:
    {
        int userlen, passlen;

        if (!username || !password) return FALSE;
        if ((!authinfo && !(authinfo = alloc_authinfo())) || authinfo->finished) return FALSE;

        userlen = WideCharToMultiByte( CP_UTF8, 0, username, strlenW( username ), NULL, 0, NULL, NULL );
        passlen = WideCharToMultiByte( CP_UTF8, 0, password, strlenW( password ), NULL, 0, NULL, NULL );

        authinfo->data_len = userlen + 1 + passlen;
        if (!(authinfo->data = heap_alloc( authinfo->data_len ))) return FALSE;

        WideCharToMultiByte( CP_UTF8, 0, username, -1, authinfo->data, userlen, NULL, NULL );
        authinfo->data[userlen] = ':';
        WideCharToMultiByte( CP_UTF8, 0, password, -1, authinfo->data + userlen + 1, passlen, NULL, NULL );

        authinfo->scheme   = SCHEME_BASIC;
        authinfo->finished = TRUE;
        break;
    }
    case SCHEME_NTLM:
    case SCHEME_NEGOTIATE:
    {
        SECURITY_STATUS status;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        ULONG flags = ISC_REQ_CONNECTION|ISC_REQ_USE_DCE_STYLE|ISC_REQ_MUTUAL_AUTH|ISC_REQ_DELEGATE;
        const WCHAR *p;
        BOOL first = FALSE;

        if (!authinfo)
        {
            TimeStamp exp;
            SEC_WINNT_AUTH_IDENTITY_W id;
            WCHAR *domain, *user;

            if (!username || !password || !(authinfo = alloc_authinfo())) return FALSE;

            first = TRUE;
            domain = (WCHAR *)username;
            user = strchrW( username, '\\' );

            if (user) user++;
            else
            {
                user = (WCHAR *)username;
                domain = NULL;
            }
            id.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            id.User           = user;
            id.UserLength     = strlenW( user );
            id.Domain         = domain;
            id.DomainLength   = domain ? user - domain - 1 : 0;
            id.Password       = (WCHAR *)password;
            id.PasswordLength = strlenW( password );

            status = AcquireCredentialsHandleW( NULL, (SEC_WCHAR *)auth_schemes[scheme].str,
                                                SECPKG_CRED_OUTBOUND, NULL, &id, NULL, NULL,
                                                &authinfo->cred, &exp );
            if (status == SEC_E_OK)
            {
                PSecPkgInfoW info;
                status = QuerySecurityPackageInfoW( (SEC_WCHAR *)auth_schemes[scheme].str, &info );
                if (status == SEC_E_OK)
                {
                    authinfo->max_token = info->cbMaxToken;
                    FreeContextBuffer( info );
                }
            }
            if (status != SEC_E_OK)
            {
                WARN("AcquireCredentialsHandleW for scheme %s failed with error 0x%08x\n",
                     debugstr_w(auth_schemes[scheme].str), status);
                heap_free( authinfo );
                return FALSE;
            }
            authinfo->scheme = scheme;
        }
        else if (authinfo->finished) return FALSE;

        if ((strlenW( auth_value ) < auth_schemes[authinfo->scheme].len ||
            strncmpiW( auth_value, auth_schemes[authinfo->scheme].str, auth_schemes[authinfo->scheme].len )))
        {
            ERR("authentication scheme changed from %s to %s\n",
                debugstr_w(auth_schemes[authinfo->scheme].str), debugstr_w(auth_value));
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer   = 0;
        in.pvBuffer   = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers  = 1;
        in_desc.pBuffers  = &in;

        p = auth_value + auth_schemes[scheme].len;
        if (*p == ' ')
        {
            int len = strlenW( ++p );
            in.cbBuffer = decode_base64( p, len, NULL );
            if (!(in.pvBuffer = heap_alloc( in.cbBuffer ))) {
                destroy_authinfo( authinfo );
                *auth_ptr = NULL;
                return FALSE;
            }
            decode_base64( p, len, in.pvBuffer );
        }
        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer   = authinfo->max_token;
        if (!(out.pvBuffer = heap_alloc( authinfo->max_token )))
        {
            heap_free( in.pvBuffer );
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        out_desc.ulVersion = 0;
        out_desc.cBuffers  = 1;
        out_desc.pBuffers  = &out;

        status = InitializeSecurityContextW( first ? &authinfo->cred : NULL, first ? NULL : &authinfo->ctx,
                                             first ? request->connect->servername : NULL, flags, 0,
                                             SECURITY_NETWORK_DREP, in.pvBuffer ? &in_desc : NULL, 0,
                                             &authinfo->ctx, &out_desc, &authinfo->attr, &authinfo->exp );
        heap_free( in.pvBuffer );
        if (status == SEC_E_OK)
        {
            heap_free( authinfo->data );
            authinfo->data     = out.pvBuffer;
            authinfo->data_len = out.cbBuffer;
            authinfo->finished = TRUE;
            TRACE("sending last auth packet\n");
        }
        else if (status == SEC_I_CONTINUE_NEEDED)
        {
            heap_free( authinfo->data );
            authinfo->data     = out.pvBuffer;
            authinfo->data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
        }
        else
        {
            ERR("InitializeSecurityContextW failed with error 0x%08x\n", status);
            heap_free( out.pvBuffer );
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        break;
    }
    default:
        ERR("invalid scheme %u\n", scheme);
        return FALSE;
    }
    *auth_ptr = authinfo;

    len_scheme = auth_schemes[authinfo->scheme].len;
    len = len_scheme + 1 + ((authinfo->data_len + 2) * 4) / 3;
    if (!(auth_reply = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;

    memcpy( auth_reply, auth_schemes[authinfo->scheme].str, len_scheme * sizeof(WCHAR) );
    auth_reply[len_scheme] = ' ';
    encode_base64( authinfo->data, authinfo->data_len, auth_reply + len_scheme + 1 );

    flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
    ret = process_header( request, auth_target, auth_reply, flags, TRUE );
    heap_free( auth_reply );
    return ret;
}

static BOOL set_credentials( request_t *request, DWORD target, DWORD scheme_flag, const WCHAR *username,
                             const WCHAR *password )
{
    enum auth_scheme scheme = scheme_from_flag( scheme_flag );

    if (scheme == SCHEME_INVALID || ((scheme == SCHEME_BASIC || scheme == SCHEME_DIGEST) && (!username || !password)))
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (target)
    {
    case WINHTTP_AUTH_TARGET_SERVER:
    {
        heap_free( request->creds[TARGET_SERVER][scheme].username );
        if (!username) request->creds[TARGET_SERVER][scheme].username = NULL;
        else if (!(request->creds[TARGET_SERVER][scheme].username = strdupW( username ))) return FALSE;

        heap_free( request->creds[TARGET_SERVER][scheme].password );
        if (!password) request->creds[TARGET_SERVER][scheme].password = NULL;
        else if (!(request->creds[TARGET_SERVER][scheme].password = strdupW( password ))) return FALSE;
        break;
    }
    case WINHTTP_AUTH_TARGET_PROXY:
    {
        heap_free( request->creds[TARGET_PROXY][scheme].username );
        if (!username) request->creds[TARGET_PROXY][scheme].username = NULL;
        else if (!(request->creds[TARGET_PROXY][scheme].username = strdupW( username ))) return FALSE;

        heap_free( request->creds[TARGET_PROXY][scheme].password );
        if (!password) request->creds[TARGET_PROXY][scheme].password = NULL;
        else if (!(request->creds[TARGET_PROXY][scheme].password = strdupW( password ))) return FALSE;
        break;
    }
    default:
        WARN("unknown target %u\n", target);
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *          WinHttpSetCredentials (winhttp.@)
 */
BOOL WINAPI WinHttpSetCredentials( HINTERNET hrequest, DWORD target, DWORD scheme, LPCWSTR username,
                                   LPCWSTR password, LPVOID params )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %x, 0x%08x, %s, %p, %p\n", hrequest, target, scheme, debugstr_w(username), password, params);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = set_credentials( request, target, scheme, username, password );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static BOOL handle_authorization( request_t *request, DWORD status )
{
    DWORD i, schemes, first, level, target;

    switch (status)
    {
    case HTTP_STATUS_DENIED:
        target = WINHTTP_AUTH_TARGET_SERVER;
        level  = WINHTTP_QUERY_WWW_AUTHENTICATE;
        break;

    case HTTP_STATUS_PROXY_AUTH_REQ:
        target = WINHTTP_AUTH_TARGET_PROXY;
        level  = WINHTTP_QUERY_PROXY_AUTHENTICATE;
        break;

    default:
        WARN("unhandled status %u\n", status);
        return FALSE;
    }

    if (!query_auth_schemes( request, level, &schemes, &first )) return FALSE;
    if (do_authorization( request, target, first )) return TRUE;

    schemes &= ~first;
    for (i = 0; i < num_auth_schemes; i++)
    {
        if (!(schemes & auth_schemes[i].scheme)) continue;
        if (do_authorization( request, target, auth_schemes[i].scheme )) return TRUE;
    }
    return FALSE;
}

/* set the request content length based on the headers */
static DWORD set_content_length( request_t *request, DWORD status )
{
    WCHAR encoding[20];
    DWORD buflen = sizeof(request->content_length);

    if (status == HTTP_STATUS_NO_CONTENT || status == HTTP_STATUS_NOT_MODIFIED || !strcmpW( request->verb, headW ))
        request->content_length = 0;
    else
    {
        if (!query_headers( request, WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, &request->content_length, &buflen, NULL ))
            request->content_length = ~0u;

        buflen = sizeof(encoding);
        if (query_headers( request, WINHTTP_QUERY_TRANSFER_ENCODING, NULL, encoding, &buflen, NULL ) &&
            !strcmpiW( encoding, chunkedW ))
        {
            request->content_length = ~0u;
            request->read_chunked = TRUE;
            request->read_chunked_size = ~0u;
            request->read_chunked_eof = FALSE;
        }
    }
    request->content_read = 0;
    return request->content_length;
}

/* read some more data into the read buffer */
static BOOL read_more_data( request_t *request, int maxlen, BOOL notify )
{
    int len;
    BOOL ret;

    if (request->read_chunked_eof) return FALSE;

    if (request->read_size && request->read_pos)
    {
        /* move existing data to the start of the buffer */
        memmove( request->read_buf, request->read_buf + request->read_pos, request->read_size );
        request->read_pos = 0;
    }
    if (maxlen == -1) maxlen = sizeof(request->read_buf);

    if (notify) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NULL, 0 );

    ret = netconn_recv( &request->netconn, request->read_buf + request->read_size,
                        maxlen - request->read_size, 0, &len );

    if (notify) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, &len, sizeof(len) );

    request->read_size += len;
    return ret;
}

/* remove some amount of data from the read buffer */
static void remove_data( request_t *request, int count )
{
    if (!(request->read_size -= count)) request->read_pos = 0;
    else request->read_pos += count;
}

static BOOL read_line( request_t *request, char *buffer, DWORD *len )
{
    int count, bytes_read, pos = 0;

    for (;;)
    {
        char *eol = memchr( request->read_buf + request->read_pos, '\n', request->read_size );
        if (eol)
        {
            count = eol - (request->read_buf + request->read_pos);
            bytes_read = count + 1;
        }
        else count = bytes_read = request->read_size;

        count = min( count, *len - pos );
        memcpy( buffer + pos, request->read_buf + request->read_pos, count );
        pos += count;
        remove_data( request, bytes_read );
        if (eol) break;

        if (!read_more_data( request, -1, TRUE )) return FALSE;
        if (!request->read_size)
        {
            *len = 0;
            TRACE("returning empty string\n");
            return FALSE;
        }
    }
    if (pos < *len)
    {
        if (pos && buffer[pos - 1] == '\r') pos--;
        *len = pos + 1;
    }
    buffer[*len - 1] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return TRUE;
}

/* discard data contents until we reach end of line */
static BOOL discard_eol( request_t *request, BOOL notify )
{
    do
    {
        char *eol = memchr( request->read_buf + request->read_pos, '\n', request->read_size );
        if (eol)
        {
            remove_data( request, (eol + 1) - (request->read_buf + request->read_pos) );
            break;
        }
        request->read_pos = request->read_size = 0;  /* discard everything */
        if (!read_more_data( request, -1, notify )) return FALSE;
    } while (request->read_size);
    return TRUE;
}

/* read the size of the next chunk */
static BOOL start_next_chunk( request_t *request, BOOL notify )
{
    DWORD chunk_size = 0;

    assert(!request->read_chunked_size || request->read_chunked_size == ~0u);

    if (request->read_chunked_eof) return FALSE;

    /* read terminator for the previous chunk */
    if (!request->read_chunked_size && !discard_eol( request, notify )) return FALSE;

    for (;;)
    {
        while (request->read_size)
        {
            char ch = request->read_buf[request->read_pos];
            if (ch >= '0' && ch <= '9') chunk_size = chunk_size * 16 + ch - '0';
            else if (ch >= 'a' && ch <= 'f') chunk_size = chunk_size * 16 + ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') chunk_size = chunk_size * 16 + ch - 'A' + 10;
            else if (ch == ';' || ch == '\r' || ch == '\n')
            {
                TRACE("reading %u byte chunk\n", chunk_size);

                if (request->content_length == ~0u) request->content_length = chunk_size;
                else request->content_length += chunk_size;

                request->read_chunked_size = chunk_size;
                if (!chunk_size) request->read_chunked_eof = TRUE;

                return discard_eol( request, notify );
            }
            remove_data( request, 1 );
        }
        if (!read_more_data( request, -1, notify )) return FALSE;
        if (!request->read_size)
        {
            request->content_length = request->content_read = 0;
            request->read_chunked_size = 0;
            return TRUE;
        }
    }
}

/* return the size of data available to be read immediately */
static DWORD get_available_data( request_t *request )
{
    if (request->read_chunked) return min( request->read_chunked_size, request->read_size );
    return request->read_size;
}

/* check if we have reached the end of the data to read */
static BOOL end_of_read_data( request_t *request )
{
    if (!request->content_length) return TRUE;
    if (request->read_chunked) return request->read_chunked_eof;
    if (request->content_length == ~0u) return FALSE;
    return (request->content_length == request->content_read);
}

static BOOL refill_buffer( request_t *request, BOOL notify )
{
    int len = sizeof(request->read_buf);

    if (request->read_chunked)
    {
        if (request->read_chunked_eof) return FALSE;
        if (request->read_chunked_size == ~0u || !request->read_chunked_size)
        {
            if (!start_next_chunk( request, notify )) return FALSE;
        }
        len = min( len, request->read_chunked_size );
    }
    else if (request->content_length != ~0u)
    {
        len = min( len, request->content_length - request->content_read );
    }

    if (len <= request->read_size) return TRUE;
    if (!read_more_data( request, len, notify )) return FALSE;
    if (!request->read_size) request->content_length = request->content_read = 0;
    return TRUE;
}

#define MAX_REPLY_LEN   1460
#define INITIAL_HEADER_BUFFER_LEN  512

static BOOL read_reply( request_t *request )
{
    static const WCHAR crlf[] = {'\r','\n',0};

    char buffer[MAX_REPLY_LEN];
    DWORD buflen, len, offset, crlf_len = 2; /* strlenW(crlf) */
    char *status_code, *status_text;
    WCHAR *versionW, *status_textW, *raw_headers;
    WCHAR status_codeW[4]; /* sizeof("nnn") */

    if (!netconn_connected( &request->netconn )) return FALSE;

    do
    {
        buflen = MAX_REPLY_LEN;
        if (!read_line( request, buffer, &buflen )) return FALSE;

        /* first line should look like 'HTTP/1.x nnn OK' where nnn is the status code */
        if (!(status_code = strchr( buffer, ' ' ))) return FALSE;
        status_code++;
        if (!(status_text = strchr( status_code, ' ' ))) return FALSE;
        if ((len = status_text - status_code) != sizeof("nnn") - 1) return FALSE;
        status_text++;

        TRACE("version [%s] status code [%s] status text [%s]\n",
              debugstr_an(buffer, status_code - buffer - 1),
              debugstr_an(status_code, len),
              debugstr_a(status_text));

    } while (!memcmp( status_code, "100", len )); /* ignore "100 Continue" responses */

    /*  we rely on the fact that the protocol is ascii */
    MultiByteToWideChar( CP_ACP, 0, status_code, len, status_codeW, len );
    status_codeW[len] = 0;
    if (!(process_header( request, attr_status, status_codeW,
                          WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE, FALSE )))
        return FALSE;

    len = status_code - buffer;
    if (!(versionW = heap_alloc( len * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_ACP, 0, buffer, len - 1, versionW, len -1 );
    versionW[len - 1] = 0;

    heap_free( request->version );
    request->version = versionW;

    len = buflen - (status_text - buffer);
    if (!(status_textW = heap_alloc( len * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_ACP, 0, status_text, len, status_textW, len );

    heap_free( request->status_text );
    request->status_text = status_textW;

    len = max( buflen + crlf_len, INITIAL_HEADER_BUFFER_LEN );
    if (!(raw_headers = heap_alloc( len * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_ACP, 0, buffer, buflen, raw_headers, buflen );
    memcpy( raw_headers + buflen - 1, crlf, sizeof(crlf) );

    heap_free( request->raw_headers );
    request->raw_headers = raw_headers;

    offset = buflen + crlf_len - 1;
    for (;;)
    {
        header_t *header;

        buflen = MAX_REPLY_LEN;
        if (!read_line( request, buffer, &buflen )) return TRUE;
        if (!*buffer) buflen = 1;

        while (len - offset < buflen + crlf_len)
        {
            WCHAR *tmp;
            len *= 2;
            if (!(tmp = heap_realloc( raw_headers, len * sizeof(WCHAR) ))) return FALSE;
            request->raw_headers = raw_headers = tmp;
        }
        if (!*buffer)
        {
            memcpy( raw_headers + offset, crlf, sizeof(crlf) );
            break;
        }
        MultiByteToWideChar( CP_ACP, 0, buffer, buflen, raw_headers + offset, buflen );

        if (!(header = parse_header( raw_headers + offset ))) break;
        if (!(process_header( request, header->field, header->value, WINHTTP_ADDREQ_FLAG_ADD, FALSE )))
        {
            free_header( header );
            break;
        }
        free_header( header );
        memcpy( raw_headers + offset + buflen - 1, crlf, sizeof(crlf) );
        offset += buflen + crlf_len - 1;
    }

    TRACE("raw headers: %s\n", debugstr_w(raw_headers));
    return TRUE;
}

static void finished_reading( request_t *request )
{
    static const WCHAR closeW[] = {'c','l','o','s','e',0};

    BOOL close = FALSE;
    WCHAR connection[20];
    DWORD size = sizeof(connection);

    if (request->hdr.disable_flags & WINHTTP_DISABLE_KEEP_ALIVE) close = TRUE;
    else if (query_headers( request, WINHTTP_QUERY_CONNECTION, NULL, connection, &size, NULL ) ||
             query_headers( request, WINHTTP_QUERY_PROXY_CONNECTION, NULL, connection, &size, NULL ))
    {
        if (!strcmpiW( connection, closeW )) close = TRUE;
    }
    else if (!strcmpW( request->version, http1_0 )) close = TRUE;
    if (close) close_connection( request );
}

static BOOL read_data( request_t *request, void *buffer, DWORD size, DWORD *read, BOOL async )
{
    int count, bytes_read = 0;

    if (end_of_read_data( request )) goto done;

    while (size)
    {
        if (!(count = get_available_data( request )))
        {
            if (!refill_buffer( request, async )) goto done;
            if (!(count = get_available_data( request ))) goto done;
        }
        count = min( count, size );
        memcpy( (char *)buffer + bytes_read, request->read_buf + request->read_pos, count );
        remove_data( request, count );
        if (request->read_chunked) request->read_chunked_size -= count;
        size -= count;
        bytes_read += count;
        request->content_read += count;
        if (end_of_read_data( request )) goto done;
    }
    if (request->read_chunked && !request->read_chunked_size) refill_buffer( request, async );

done:
    TRACE( "retrieved %u bytes (%u/%u)\n", bytes_read, request->content_read, request->content_length );

    if (async) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, buffer, bytes_read );
    if (read) *read = bytes_read;
    if (end_of_read_data( request )) finished_reading( request );
    return TRUE;
}

/* read any content returned by the server so that the connection can be reused */
static void drain_content( request_t *request )
{
    DWORD bytes_read;
    char buffer[2048];

    refill_buffer( request, FALSE );
    for (;;)
    {
        if (!read_data( request, buffer, sizeof(buffer), &bytes_read, FALSE ) || !bytes_read) return;
    }
}

static void record_cookies( request_t *request )
{
    unsigned int i;

    for (i = 0; i < request->num_headers; i++)
    {
        header_t *set_cookie = &request->headers[i];
        if (!strcmpiW( set_cookie->field, attr_set_cookie ) && !set_cookie->is_request)
        {
            set_cookies( request, set_cookie->value );
        }
    }
}

static WCHAR *get_redirect_url( request_t *request, DWORD *len )
{
    DWORD size;
    WCHAR *ret;

    query_headers( request, WINHTTP_QUERY_LOCATION, NULL, NULL, &size, NULL );
    if (get_last_error() != ERROR_INSUFFICIENT_BUFFER) return FALSE;
    if (!(ret = heap_alloc( size ))) return NULL;
    *len = size / sizeof(WCHAR);
    if (query_headers( request, WINHTTP_QUERY_LOCATION, NULL, ret, &size, NULL )) return ret;
    heap_free( ret );
    return NULL;
}

static BOOL handle_redirect( request_t *request, DWORD status )
{
    BOOL ret = FALSE;
    DWORD len, len_url;
    URL_COMPONENTS uc;
    connect_t *connect = request->connect;
    INTERNET_PORT port;
    WCHAR *hostname = NULL, *location;
    int index;

    if (!(location = get_redirect_url( request, &len_url ))) return FALSE;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = ~0u;

    if (!WinHttpCrackUrl( location, len_url, 0, &uc )) /* assume relative redirect */
    {
        WCHAR *path, *p;

        if (location[0] == '/')
        {
            len = strlenW( location );
            if (!(path = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
            strcpyW( path, location );
        }
        else
        {
            if ((p = strrchrW( request->path, '/' ))) *p = 0;
            len = strlenW( request->path ) + 1 + strlenW( location );
            if (!(path = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
            strcpyW( path, request->path );
            strcatW( path, slashW );
            strcatW( path, location );
        }
        heap_free( request->path );
        request->path = path;

        drain_content( request );
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REDIRECT, location, len_url + 1 );
    }
    else
    {
        if (uc.nScheme == INTERNET_SCHEME_HTTP && request->hdr.flags & WINHTTP_FLAG_SECURE)
        {
            if (request->hdr.redirect_policy == WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP) goto end;
            TRACE("redirect from secure page to non-secure page\n");
            request->hdr.flags &= ~WINHTTP_FLAG_SECURE;
        }
        else if (uc.nScheme == INTERNET_SCHEME_HTTPS && !(request->hdr.flags & WINHTTP_FLAG_SECURE))
        {
            TRACE("redirect from non-secure page to secure page\n");
            request->hdr.flags |= WINHTTP_FLAG_SECURE;
        }

        drain_content( request );
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REDIRECT, location, len_url + 1 );

        len = uc.dwHostNameLength;
        if (!(hostname = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
        memcpy( hostname, uc.lpszHostName, len * sizeof(WCHAR) );
        hostname[len] = 0;

        port = uc.nPort ? uc.nPort : (uc.nScheme == INTERNET_SCHEME_HTTPS ? 443 : 80);
        if (strcmpiW( connect->hostname, hostname ) || connect->serverport != port)
        {
            heap_free( connect->hostname );
            connect->hostname = hostname;
            connect->hostport = port;
            if (!(ret = set_server_for_hostname( connect, hostname, port ))) goto end;

            netconn_close( &request->netconn );
            if (!(ret = netconn_init( &request->netconn ))) goto end;
            request->read_pos = request->read_size = 0;
            request->read_chunked = FALSE;
            request->read_chunked_eof = FALSE;
        }
        else heap_free( hostname );

        if (!(ret = add_host_header( request, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ))) goto end;
        if (!(ret = open_connection( request ))) goto end;

        heap_free( request->path );
        request->path = NULL;
        if (uc.dwUrlPathLength)
        {
            len = uc.dwUrlPathLength + uc.dwExtraInfoLength;
            if (!(request->path = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
            strcpyW( request->path, uc.lpszUrlPath );
        }
        else request->path = strdupW( slashW );
    }

    /* remove content-type/length headers */
    if ((index = get_header_index( request, attr_content_type, 0, TRUE )) >= 0) delete_header( request, index );
    if ((index = get_header_index( request, attr_content_length, 0, TRUE )) >= 0 ) delete_header( request, index );

    if (status != HTTP_STATUS_REDIRECT_KEEP_VERB && !strcmpW( request->verb, postW ))
    {
        heap_free( request->verb );
        request->verb = strdupW( getW );
        request->optional = NULL;
        request->optional_len = 0;
    }
    ret = TRUE;

end:
    heap_free( location );
    return ret;
}

static BOOL receive_response( request_t *request, BOOL async )
{
    BOOL ret;
    DWORD size, query, status;

    for (;;)
    {
        if (!(ret = read_reply( request )))
        {
            set_last_error( ERROR_WINHTTP_INVALID_SERVER_RESPONSE );
            break;
        }
        size = sizeof(DWORD);
        query = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
        if (!(ret = query_headers( request, query, NULL, &status, &size, NULL ))) break;

        set_content_length( request, status );

        if (!(request->hdr.disable_flags & WINHTTP_DISABLE_COOKIES)) record_cookies( request );

        if (status == HTTP_STATUS_MOVED || status == HTTP_STATUS_REDIRECT || status == HTTP_STATUS_REDIRECT_KEEP_VERB)
        {
            if (request->hdr.disable_flags & WINHTTP_DISABLE_REDIRECTS ||
                request->hdr.redirect_policy == WINHTTP_OPTION_REDIRECT_POLICY_NEVER) break;

            if (!(ret = handle_redirect( request, status ))) break;

            /* recurse synchronously */
            if ((ret = send_request( request, NULL, 0, request->optional, request->optional_len, 0, 0, FALSE ))) continue;
        }
        else if (status == HTTP_STATUS_DENIED || status == HTTP_STATUS_PROXY_AUTH_REQ)
        {
            if (request->hdr.disable_flags & WINHTTP_DISABLE_AUTHENTICATION) break;

            if (!handle_authorization( request, status )) break;
            drain_content( request );

            /* recurse synchronously */
            if ((ret = send_request( request, NULL, 0, request->optional, request->optional_len, 0, 0, FALSE ))) continue;
        }
        break;
    }

    if (request->content_length) refill_buffer( request, FALSE );

    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NULL, 0 );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_RECEIVE_RESPONSE;
            result.dwError  = get_last_error();
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void task_receive_response( task_header_t *task )
{
    receive_response_t *r = (receive_response_t *)task;
    receive_response( r->hdr.request, TRUE );
}

/***********************************************************************
 *          WinHttpReceiveResponse (winhttp.@)
 */
BOOL WINAPI WinHttpReceiveResponse( HINTERNET hrequest, LPVOID reserved )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %p\n", hrequest, reserved);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        receive_response_t *r;

        if (!(r = heap_alloc( sizeof(receive_response_t) ))) return FALSE;
        r->hdr.request = request;
        r->hdr.proc    = task_receive_response;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)r );
    }
    else
        ret = receive_response( request, FALSE );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static BOOL query_data_available( request_t *request, DWORD *available, BOOL async )
{
    DWORD count = 0;

    if (end_of_read_data( request )) goto done;

    count = get_available_data( request );
    if (!request->read_chunked)
        count += netconn_query_data_available( &request->netconn );
    if (!count)
    {
        refill_buffer( request, async );
        count = get_available_data( request );
        if (!request->read_chunked)
            count += netconn_query_data_available( &request->netconn );
    }

done:
    if (async) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, &count, sizeof(count) );
    TRACE("%u bytes available\n", count);
    if (available) *available = count;
    return TRUE;
}

static void task_query_data_available( task_header_t *task )
{
    query_data_t *q = (query_data_t *)task;
    query_data_available( q->hdr.request, q->available, TRUE );
}

/***********************************************************************
 *          WinHttpQueryDataAvailable (winhttp.@)
 */
BOOL WINAPI WinHttpQueryDataAvailable( HINTERNET hrequest, LPDWORD available )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %p\n", hrequest, available);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        query_data_t *q;

        if (!(q = heap_alloc( sizeof(query_data_t) ))) return FALSE;
        q->hdr.request = request;
        q->hdr.proc    = task_query_data_available;
        q->available   = available;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)q );
    }
    else
        ret = query_data_available( request, available, FALSE );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static void task_read_data( task_header_t *task )
{
    read_data_t *r = (read_data_t *)task;
    read_data( r->hdr.request, r->buffer, r->to_read, r->read, TRUE );
}

/***********************************************************************
 *          WinHttpReadData (winhttp.@)
 */
BOOL WINAPI WinHttpReadData( HINTERNET hrequest, LPVOID buffer, DWORD to_read, LPDWORD read )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %p, %d, %p\n", hrequest, buffer, to_read, read);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        read_data_t *r;

        if (!(r = heap_alloc( sizeof(read_data_t) ))) return FALSE;
        r->hdr.request = request;
        r->hdr.proc    = task_read_data;
        r->buffer      = buffer;
        r->to_read     = to_read;
        r->read        = read;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)r );
    }
    else
        ret = read_data( request, buffer, to_read, read, FALSE );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

static BOOL write_data( request_t *request, LPCVOID buffer, DWORD to_write, LPDWORD written, BOOL async )
{
    BOOL ret;
    int num_bytes;

    ret = netconn_send( &request->netconn, buffer, to_write, &num_bytes );

    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, &num_bytes, sizeof(num_bytes) );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_WRITE_DATA;
            result.dwError  = get_last_error();
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    if (ret && written) *written = num_bytes;
    return ret;
}

static void task_write_data( task_header_t *task )
{
    write_data_t *w = (write_data_t *)task;
    write_data( w->hdr.request, w->buffer, w->to_write, w->written, TRUE );
}

/***********************************************************************
 *          WinHttpWriteData (winhttp.@)
 */
BOOL WINAPI WinHttpWriteData( HINTERNET hrequest, LPCVOID buffer, DWORD to_write, LPDWORD written )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %p, %d, %p\n", hrequest, buffer, to_write, written);

    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        write_data_t *w;

        if (!(w = heap_alloc( sizeof(write_data_t) ))) return FALSE;
        w->hdr.request = request;
        w->hdr.proc    = task_write_data;
        w->buffer      = buffer;
        w->to_write    = to_write;
        w->written     = written;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)w );
    }
    else
        ret = write_data( request, buffer, to_write, written, FALSE );

    release_object( &request->hdr );
    if (ret) set_last_error( ERROR_SUCCESS );
    return ret;
}

enum request_state
{
    REQUEST_STATE_UNINITIALIZED,
    REQUEST_STATE_INITIALIZED,
    REQUEST_STATE_CANCELLED,
    REQUEST_STATE_OPEN,
    REQUEST_STATE_SENT,
    REQUEST_STATE_RESPONSE_RECEIVED
};

struct winhttp_request
{
    IWinHttpRequest IWinHttpRequest_iface;
    LONG refs;
    CRITICAL_SECTION cs;
    enum request_state state;
    HINTERNET hsession;
    HINTERNET hconnect;
    HINTERNET hrequest;
    VARIANT data;
    WCHAR *verb;
    HANDLE thread;
    HANDLE wait;
    HANDLE cancel;
    char *buffer;
    DWORD offset;
    DWORD bytes_available;
    DWORD bytes_read;
    DWORD error;
    DWORD logon_policy;
    DWORD disable_feature;
    LONG resolve_timeout;
    LONG connect_timeout;
    LONG send_timeout;
    LONG receive_timeout;
    WINHTTP_PROXY_INFO proxy;
    BOOL async;
    UINT url_codepage;
};

static inline struct winhttp_request *impl_from_IWinHttpRequest( IWinHttpRequest *iface )
{
    return CONTAINING_RECORD( iface, struct winhttp_request, IWinHttpRequest_iface );
}

static ULONG WINAPI winhttp_request_AddRef(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    return InterlockedIncrement( &request->refs );
}

/* critical section must be held */
static void cancel_request( struct winhttp_request *request )
{
    if (request->state <= REQUEST_STATE_CANCELLED) return;

    SetEvent( request->cancel );
    LeaveCriticalSection( &request->cs );
    WaitForSingleObject( request->thread, INFINITE );
    EnterCriticalSection( &request->cs );

    request->state = REQUEST_STATE_CANCELLED;

    CloseHandle( request->thread );
    request->thread = NULL;
    CloseHandle( request->wait );
    request->wait = NULL;
    CloseHandle( request->cancel );
    request->cancel = NULL;
}

/* critical section must be held */
static void free_request( struct winhttp_request *request )
{
    if (request->state < REQUEST_STATE_INITIALIZED) return;
    WinHttpCloseHandle( request->hrequest );
    WinHttpCloseHandle( request->hconnect );
    WinHttpCloseHandle( request->hsession );
    CloseHandle( request->thread );
    CloseHandle( request->wait );
    CloseHandle( request->cancel );
    heap_free( (WCHAR *)request->proxy.lpszProxy );
    heap_free( (WCHAR *)request->proxy.lpszProxyBypass );
    heap_free( request->buffer );
    heap_free( request->verb );
    VariantClear( &request->data );
}

static ULONG WINAPI winhttp_request_Release(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    LONG refs = InterlockedDecrement( &request->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", request);

        EnterCriticalSection( &request->cs );
        cancel_request( request );
        free_request( request );
        LeaveCriticalSection( &request->cs );
        request->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &request->cs );
        heap_free( request );
    }
    return refs;
}

static HRESULT WINAPI winhttp_request_QueryInterface(
    IWinHttpRequest *iface,
    REFIID riid,
    void **obj )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %s, %p\n", request, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_IWinHttpRequest ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWinHttpRequest_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_GetTypeInfoCount(
    IWinHttpRequest *iface,
    UINT *count )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %p\n", request, count);
    *count = 1;
    return S_OK;
}

enum type_id
{
    IWinHttpRequest_tid,
    last_tid
};

static ITypeLib *winhttp_typelib;
static ITypeInfo *winhttp_typeinfo[last_tid];

static REFIID winhttp_tid_id[] =
{
    &IID_IWinHttpRequest
};

static HRESULT get_typeinfo( enum type_id tid, ITypeInfo **ret )
{
    HRESULT hr;

    if (!winhttp_typelib)
    {
        ITypeLib *typelib;

        hr = LoadRegTypeLib( &LIBID_WinHttp, 5, 1, LOCALE_SYSTEM_DEFAULT, &typelib );
        if (FAILED(hr))
        {
            ERR("LoadRegTypeLib failed: %08x\n", hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)&winhttp_typelib, typelib, NULL ))
            ITypeLib_Release( typelib );
    }
    if (!winhttp_typeinfo[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid( winhttp_typelib, winhttp_tid_id[tid], &typeinfo );
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(winhttp_tid_id[tid]), hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)(winhttp_typeinfo + tid), typeinfo, NULL ))
            ITypeInfo_Release( typeinfo );
    }
    *ret = winhttp_typeinfo[tid];
    ITypeInfo_AddRef(winhttp_typeinfo[tid]);
    return S_OK;
}

void release_typelib(void)
{
    unsigned i;

    for (i = 0; i < sizeof(winhttp_typeinfo)/sizeof(*winhttp_typeinfo); i++)
        if (winhttp_typeinfo[i])
            ITypeInfo_Release(winhttp_typeinfo[i]);

    if (winhttp_typelib)
        ITypeLib_Release(winhttp_typelib);
}

static HRESULT WINAPI winhttp_request_GetTypeInfo(
    IWinHttpRequest *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    TRACE("%p, %u, %u, %p\n", request, index, lcid, info);

    return get_typeinfo( IWinHttpRequest_tid, info );
}

static HRESULT WINAPI winhttp_request_GetIDsOfNames(
    IWinHttpRequest *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %u, %p\n", request, debugstr_guid(riid), names, count, lcid, dispid);

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( IWinHttpRequest_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI winhttp_request_Invoke(
    IWinHttpRequest *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %d, %s, %d, %d, %p, %p, %p, %p\n", request, member, debugstr_guid(riid),
          lcid, flags, params, result, excep_info, arg_err);

    if (!IsEqualIID( riid, &IID_NULL )) return DISP_E_UNKNOWNINTERFACE;

    if (member == DISPID_HTTPREQUEST_OPTION)
    {
        VARIANT ret_value, option;
        UINT err_pos;

        if (!result) result = &ret_value;
        if (!arg_err) arg_err = &err_pos;

        VariantInit( &option );
        VariantInit( result );

        if (!flags) return S_OK;

        if (flags == DISPATCH_PROPERTYPUT)
        {
            hr = DispGetParam( params, 0, VT_I4, &option, arg_err );
            if (FAILED(hr)) return hr;

            hr = IWinHttpRequest_put_Option( &request->IWinHttpRequest_iface, V_I4( &option ), params->rgvarg[0] );
            if (FAILED(hr))
                WARN("put_Option(%d) failed: %x\n", V_I4( &option ), hr);
            return hr;
        }
        else if (flags & (DISPATCH_PROPERTYGET | DISPATCH_METHOD))
        {
            hr = DispGetParam( params, 0, VT_I4, &option, arg_err );
            if (FAILED(hr)) return hr;

            hr = IWinHttpRequest_get_Option( &request->IWinHttpRequest_iface, V_I4( &option ), result );
            if (FAILED(hr))
                WARN("get_Option(%d) failed: %x\n", V_I4( &option ), hr);
            return hr;
        }

        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    /* fallback to standard implementation */

    hr = get_typeinfo( IWinHttpRequest_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &request->IWinHttpRequest_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI winhttp_request_SetProxy(
    IWinHttpRequest *iface,
    HTTPREQUEST_PROXY_SETTING proxy_setting,
    VARIANT proxy_server,
    VARIANT bypass_list )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS;

    TRACE("%p, %u, %s, %s\n", request, proxy_setting, debugstr_variant(&proxy_server),
          debugstr_variant(&bypass_list));

    EnterCriticalSection( &request->cs );
    switch (proxy_setting)
    {
    case HTTPREQUEST_PROXYSETTING_DEFAULT:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        heap_free( (WCHAR *)request->proxy.lpszProxy );
        heap_free( (WCHAR *)request->proxy.lpszProxyBypass );
        request->proxy.lpszProxy = NULL;
        request->proxy.lpszProxyBypass = NULL;
        break;

    case HTTPREQUEST_PROXYSETTING_DIRECT:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
        heap_free( (WCHAR *)request->proxy.lpszProxy );
        heap_free( (WCHAR *)request->proxy.lpszProxyBypass );
        request->proxy.lpszProxy = NULL;
        request->proxy.lpszProxyBypass = NULL;
        break;

    case HTTPREQUEST_PROXYSETTING_PROXY:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        if (V_VT( &proxy_server ) == VT_BSTR)
        {
            heap_free( (WCHAR *)request->proxy.lpszProxy );
            request->proxy.lpszProxy = strdupW( V_BSTR( &proxy_server ) );
        }
        if (V_VT( &bypass_list ) == VT_BSTR)
        {
            heap_free( (WCHAR *)request->proxy.lpszProxyBypass );
            request->proxy.lpszProxyBypass = strdupW( V_BSTR( &bypass_list ) );
        }
        break;

    default:
        err = ERROR_INVALID_PARAMETER;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_SetCredentials(
    IWinHttpRequest *iface,
    BSTR username,
    BSTR password,
    HTTPREQUEST_SETCREDENTIALS_FLAGS flags )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD target, scheme = WINHTTP_AUTH_SCHEME_BASIC; /* FIXME: query supported schemes */
    DWORD err = ERROR_SUCCESS;

    TRACE("%p, %s, %p, 0x%08x\n", request, debugstr_w(username), password, flags);

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN;
        goto done;
    }
    switch (flags)
    {
    case HTTPREQUEST_SETCREDENTIALS_FOR_SERVER:
        target = WINHTTP_AUTH_TARGET_SERVER;
        break;
    case HTTPREQUEST_SETCREDENTIALS_FOR_PROXY:
        target = WINHTTP_AUTH_TARGET_PROXY;
        break;
    default:
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    if (!WinHttpSetCredentials( request->hrequest, target, scheme, username, password, NULL ))
    {
        err = get_last_error();
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static void initialize_request( struct winhttp_request *request )
{
    request->hrequest = NULL;
    request->hconnect = NULL;
    request->hsession = NULL;
    request->thread   = NULL;
    request->wait     = NULL;
    request->cancel   = NULL;
    request->buffer   = NULL;
    request->verb     = NULL;
    request->offset = 0;
    request->bytes_available = 0;
    request->bytes_read = 0;
    request->error = ERROR_SUCCESS;
    request->async = FALSE;
    request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
    request->disable_feature = 0;
    request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    request->proxy.lpszProxy = NULL;
    request->proxy.lpszProxyBypass = NULL;
    request->resolve_timeout = 0;
    request->connect_timeout = 60000;
    request->send_timeout    = 30000;
    request->receive_timeout = 30000;
    request->url_codepage = CP_UTF8;
    VariantInit( &request->data );
    request->state = REQUEST_STATE_INITIALIZED;
}

static void reset_request( struct winhttp_request *request )
{
    cancel_request( request );
    WinHttpCloseHandle( request->hrequest );
    request->hrequest = NULL;
    WinHttpCloseHandle( request->hconnect );
    request->hconnect = NULL;
    heap_free( request->buffer );
    request->buffer   = NULL;
    heap_free( request->verb );
    request->verb     = NULL;
    request->offset   = 0;
    request->bytes_available = 0;
    request->bytes_read = 0;
    request->error    = ERROR_SUCCESS;
    request->async    = FALSE;
    request->url_codepage = CP_UTF8;
    VariantClear( &request->data );
    request->state = REQUEST_STATE_INITIALIZED;
}

static HRESULT WINAPI winhttp_request_Open(
    IWinHttpRequest *iface,
    BSTR method,
    BSTR url,
    VARIANT async )
{
    static const WCHAR typeW[] = {'*','/','*',0};
    static const WCHAR *acceptW[] = {typeW, NULL};
    static const WCHAR httpsW[] = {'h','t','t','p','s'};
    static const WCHAR user_agentW[] = {
        'M','o','z','i','l','l','a','/','4','.','0',' ','(','c','o','m','p','a','t','i','b','l','e',';',' ',
        'W','i','n','3','2',';',' ','W','i','n','H','t','t','p','.','W','i','n','H','t','t','p',
        'R','e','q','u','e','s','t','.','5',')',0};
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    URL_COMPONENTS uc;
    WCHAR *hostname, *path = NULL, *verb = NULL;
    DWORD err = ERROR_OUTOFMEMORY, len, flags = 0;

    TRACE("%p, %s, %s, %s\n", request, debugstr_w(method), debugstr_w(url),
          debugstr_variant(&async));

    if (!method || !url) return E_INVALIDARG;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength   = ~0u;
    uc.dwHostNameLength = ~0u;
    uc.dwUrlPathLength  = ~0u;
    uc.dwExtraInfoLength = ~0u;
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return HRESULT_FROM_WIN32( get_last_error() );

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_INITIALIZED) initialize_request( request );
    else reset_request( request );

    if (!(hostname = heap_alloc( (uc.dwHostNameLength + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    if (!(path = heap_alloc( (uc.dwUrlPathLength + uc.dwExtraInfoLength + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( path, uc.lpszUrlPath, (uc.dwUrlPathLength + uc.dwExtraInfoLength) * sizeof(WCHAR) );
    path[uc.dwUrlPathLength + uc.dwExtraInfoLength] = 0;

    if (!(verb = strdupW( method ))) goto error;
    if (SUCCEEDED( VariantChangeType( &async, &async, 0, VT_BOOL )) && V_BOOL( &async )) request->async = TRUE;
    else request->async = FALSE;

    if (!request->hsession)
    {
        if (!(request->hsession = WinHttpOpen( user_agentW, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL,
                                               WINHTTP_FLAG_ASYNC )))
        {
            err = get_last_error();
            goto error;
        }
        if (!(request->hconnect = WinHttpConnect( request->hsession, hostname, uc.nPort, 0 )))
        {
            WinHttpCloseHandle( request->hsession );
            request->hsession = NULL;
            err = get_last_error();
            goto error;
        }
    }
    else if (!(request->hconnect = WinHttpConnect( request->hsession, hostname, uc.nPort, 0 )))
    {
        err = get_last_error();
        goto error;
    }

    len = sizeof(httpsW) / sizeof(WCHAR);
    if (uc.dwSchemeLength == len && !memcmp( uc.lpszScheme, httpsW, len * sizeof(WCHAR) ))
    {
        flags |= WINHTTP_FLAG_SECURE;
    }
    if (!(request->hrequest = WinHttpOpenRequest( request->hconnect, method, path, NULL, NULL, acceptW, flags )))
    {
        err = get_last_error();
        goto error;
    }
    WinHttpSetOption( request->hrequest, WINHTTP_OPTION_CONTEXT_VALUE, &request, sizeof(request) );

    request->state = REQUEST_STATE_OPEN;
    request->verb = verb;
    heap_free( hostname );
    heap_free( path );
    LeaveCriticalSection( &request->cs );
    return S_OK;

error:
    WinHttpCloseHandle( request->hconnect );
    request->hconnect = NULL;
    heap_free( hostname );
    heap_free( path );
    heap_free( verb );
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_SetRequestHeader(
    IWinHttpRequest *iface,
    BSTR header,
    BSTR value )
{
    static const WCHAR fmtW[] = {'%','s',':',' ','%','s','\r','\n',0};
    static const WCHAR emptyW[] = {0};
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD len, err = ERROR_SUCCESS;
    WCHAR *str;

    TRACE("%p, %s, %s\n", request, debugstr_w(header), debugstr_w(value));

    if (!header) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN;
        goto done;
    }
    if (request->state >= REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND;
        goto done;
    }
    len = strlenW( header ) + 4;
    if (value) len += strlenW( value );
    if (!(str = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    sprintfW( str, fmtW, header, value ? value : emptyW );
    if (!WinHttpAddRequestHeaders( request->hrequest, str, len,
                                   WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ))
    {
        err = get_last_error();
    }
    heap_free( str );

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_GetResponseHeader(
    IWinHttpRequest *iface,
    BSTR header,
    BSTR *value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD size, err = ERROR_SUCCESS;

    TRACE("%p, %p\n", request, header);

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!header || !value)
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    size = 0;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CUSTOM, header, NULL, &size, NULL ))
    {
        err = get_last_error();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*value = SysAllocStringLen( NULL, size / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CUSTOM, header, *value, &size, NULL ))
    {
        err = get_last_error();
        SysFreeString( *value );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_GetAllResponseHeaders(
    IWinHttpRequest *iface,
    BSTR *headers )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD size, err = ERROR_SUCCESS;

    TRACE("%p, %p\n", request, headers);

    if (!headers) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    size = 0;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &size, NULL ))
    {
        err = get_last_error();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*headers = SysAllocStringLen( NULL, size / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, *headers, &size, NULL ))
    {
        err = get_last_error();
        SysFreeString( *headers );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static void CALLBACK wait_status_callback( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD size )
{
    struct winhttp_request *request = (struct winhttp_request *)context;

    switch (status)
    {
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        request->bytes_available = *(DWORD *)buffer;
        request->error = ERROR_SUCCESS;
        break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        request->bytes_read = size;
        request->error = ERROR_SUCCESS;
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    {
        WINHTTP_ASYNC_RESULT *result = (WINHTTP_ASYNC_RESULT *)buffer;
        request->error = result->dwError;
        break;
    }
    default: break;
    }
    SetEvent( request->wait );
}

static void wait_set_status_callback( struct winhttp_request *request, DWORD status )
{
    status |= WINHTTP_CALLBACK_STATUS_REQUEST_ERROR;
    WinHttpSetStatusCallback( request->hrequest, wait_status_callback, status, 0 );
}

static DWORD wait_for_completion( struct winhttp_request *request )
{
    HANDLE handles[2] = { request->wait, request->cancel };

    switch (WaitForMultipleObjects( 2, handles, FALSE, INFINITE ))
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_OBJECT_0 + 1:
        request->error = ERROR_CANCELLED;
        break;
    default:
        request->error = get_last_error();
        break;
    }
    return request->error;
}

static HRESULT request_receive( struct winhttp_request *request )
{
    DWORD err, size, buflen = 4096;

    wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE );
    if (!WinHttpReceiveResponse( request->hrequest, NULL ))
    {
        return HRESULT_FROM_WIN32( get_last_error() );
    }
    if ((err = wait_for_completion( request ))) return HRESULT_FROM_WIN32( err );
    if (!strcmpW( request->verb, headW ))
    {
        request->state = REQUEST_STATE_RESPONSE_RECEIVED;
        return S_OK;
    }
    if (!(request->buffer = heap_alloc( buflen ))) return E_OUTOFMEMORY;
    request->buffer[0] = 0;
    size = 0;
    do
    {
        wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE );
        if (!WinHttpQueryDataAvailable( request->hrequest, &request->bytes_available ))
        {
            err = get_last_error();
            goto error;
        }
        if ((err = wait_for_completion( request ))) goto error;
        if (!request->bytes_available) break;
        size += request->bytes_available;
        if (buflen < size)
        {
            char *tmp;
            while (buflen < size) buflen *= 2;
            if (!(tmp = heap_realloc( request->buffer, buflen )))
            {
                err = ERROR_OUTOFMEMORY;
                goto error;
            }
            request->buffer = tmp;
        }
        wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_READ_COMPLETE );
        if (!WinHttpReadData( request->hrequest, request->buffer + request->offset,
                              request->bytes_available, &request->bytes_read ))
        {
            err = get_last_error();
            goto error;
        }
        if ((err = wait_for_completion( request ))) goto error;
        request->offset += request->bytes_read;
    } while (request->bytes_read);

    request->state = REQUEST_STATE_RESPONSE_RECEIVED;
    return S_OK;

error:
    heap_free( request->buffer );
    request->buffer = NULL;
    return HRESULT_FROM_WIN32( err );
}

static DWORD request_set_parameters( struct winhttp_request *request )
{
    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_PROXY, &request->proxy,
                           sizeof(request->proxy) )) return get_last_error();

    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_AUTOLOGON_POLICY, &request->logon_policy,
                           sizeof(request->logon_policy) )) return get_last_error();

    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_DISABLE_FEATURE, &request->disable_feature,
                           sizeof(request->disable_feature) )) return get_last_error();

    if (!WinHttpSetTimeouts( request->hrequest,
                             request->resolve_timeout,
                             request->connect_timeout,
                             request->send_timeout,
                             request->receive_timeout )) return get_last_error();
    return ERROR_SUCCESS;
}

static void request_set_utf8_content_type( struct winhttp_request *request )
{
    static const WCHAR fmtW[] = {'%','s',':',' ','%','s',0};
    static const WCHAR text_plainW[] = {'t','e','x','t','/','p','l','a','i','n',0};
    static const WCHAR charset_utf8W[] = {'c','h','a','r','s','e','t','=','u','t','f','-','8',0};
    WCHAR headerW[64];
    int len;

    len = sprintfW( headerW, fmtW, attr_content_type, text_plainW );
    WinHttpAddRequestHeaders( request->hrequest, headerW, len, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW );
    len = sprintfW( headerW, fmtW, attr_content_type, charset_utf8W );
    WinHttpAddRequestHeaders( request->hrequest, headerW, len, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON );
}

static HRESULT request_send( struct winhttp_request *request )
{
    SAFEARRAY *sa = NULL;
    VARIANT data;
    char *ptr = NULL;
    LONG size = 0;
    HRESULT hr;
    DWORD err;

    if ((err = request_set_parameters( request ))) return HRESULT_FROM_WIN32( err );
    if (strcmpW( request->verb, getW ))
    {
        VariantInit( &data );
        if (V_VT( &request->data ) == VT_BSTR)
        {
            UINT cp = CP_ACP;
            const WCHAR *str = V_BSTR( &request->data );
            int i, len = strlenW( str );

            for (i = 0; i < len; i++)
            {
                if (str[i] > 127)
                {
                    cp = CP_UTF8;
                    break;
                }
            }
            size = WideCharToMultiByte( cp, 0, str, len, NULL, 0, NULL, NULL );
            if (!(ptr = heap_alloc( size ))) return E_OUTOFMEMORY;
            WideCharToMultiByte( cp, 0, str, len, ptr, size, NULL, NULL );
            if (cp == CP_UTF8) request_set_utf8_content_type( request );
        }
        else if (VariantChangeType( &data, &request->data, 0, VT_ARRAY|VT_UI1 ) == S_OK)
        {
            sa = V_ARRAY( &data );
            if ((hr = SafeArrayAccessData( sa, (void **)&ptr )) != S_OK) return hr;
            if ((hr = SafeArrayGetUBound( sa, 1, &size )) != S_OK)
            {
                SafeArrayUnaccessData( sa );
                return hr;
            }
            size++;
        }
    }
    wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_REQUEST_SENT );
    if (!WinHttpSendRequest( request->hrequest, NULL, 0, ptr, size, size, 0 ))
    {
        err = get_last_error();
        goto error;
    }
    if ((err = wait_for_completion( request ))) goto error;
    if (sa) SafeArrayUnaccessData( sa );
    else heap_free( ptr );
    request->state = REQUEST_STATE_SENT;
    return S_OK;

error:
    if (sa) SafeArrayUnaccessData( sa );
    else heap_free( ptr );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT request_send_and_receive( struct winhttp_request *request )
{
    HRESULT hr = request_send( request );
    if (hr == S_OK) hr = request_receive( request );
    return hr;
}

static DWORD CALLBACK send_and_receive_proc( void *arg )
{
    struct winhttp_request *request = (struct winhttp_request *)arg;
    return request_send_and_receive( request );
}

/* critical section must be held */
static DWORD request_wait( struct winhttp_request *request, DWORD timeout )
{
    HANDLE thread = request->thread;
    DWORD err, ret;

    LeaveCriticalSection( &request->cs );
    while ((err = MsgWaitForMultipleObjects( 1, &thread, FALSE, timeout, QS_ALLINPUT )) == WAIT_OBJECT_0 + 1)
    {
        MSG msg;
        while (PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    }
    switch (err)
    {
    case WAIT_OBJECT_0:
        ret = ERROR_SUCCESS;
        break;
    case WAIT_TIMEOUT:
        ret = ERROR_TIMEOUT;
        break;
    case WAIT_FAILED:
    default:
        ret = get_last_error();
        break;
    }
    EnterCriticalSection( &request->cs );
    return ret;
}

static HRESULT WINAPI winhttp_request_Send(
    IWinHttpRequest *iface,
    VARIANT body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr;

    TRACE("%p, %s\n", request, debugstr_variant(&body));

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        LeaveCriticalSection( &request->cs );
        return HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN );
    }
    if (request->state >= REQUEST_STATE_SENT)
    {
        LeaveCriticalSection( &request->cs );
        return S_OK;
    }
    VariantClear( &request->data );
    if ((hr = VariantCopyInd( &request->data, &body )) != S_OK)
    {
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    if (!(request->thread = CreateThread( NULL, 0, send_and_receive_proc, request, 0, NULL )))
    {
        LeaveCriticalSection( &request->cs );
        return HRESULT_FROM_WIN32( get_last_error() );
    }
    request->wait = CreateEventW( NULL, FALSE, FALSE, NULL );
    request->cancel = CreateEventW( NULL, FALSE, FALSE, NULL );
    if (!request->async)
    {
        hr = HRESULT_FROM_WIN32( request_wait( request, INFINITE ) );
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_get_Status(
    IWinHttpRequest *iface,
    LONG *status )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS, flags, status_code, len = sizeof(status_code), index = 0;

    TRACE("%p, %p\n", request, status);

    if (!status) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    flags = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
    if (!WinHttpQueryHeaders( request->hrequest, flags, NULL, &status_code, &len, &index ))
    {
        err = get_last_error();
        goto done;
    }
    *status = status_code;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_StatusText(
    IWinHttpRequest *iface,
    BSTR *status )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS, len = 0, index = 0;

    TRACE("%p, %p\n", request, status);

    if (!status) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_STATUS_TEXT, NULL, NULL, &len, &index ))
    {
        err = get_last_error();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*status = SysAllocStringLen( NULL, len / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    index = 0;
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_STATUS_TEXT, NULL, *status, &len, &index ))
    {
        err = get_last_error();
        SysFreeString( *status );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static DWORD request_get_codepage( struct winhttp_request *request, UINT *codepage )
{
    static const WCHAR utf8W[] = {'u','t','f','-','8',0};
    static const WCHAR charsetW[] = {'c','h','a','r','s','e','t',0};
    WCHAR *buffer, *p;
    DWORD size;

    *codepage = CP_ACP;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CONTENT_TYPE, NULL, NULL, &size, NULL ) &&
        get_last_error() == ERROR_INSUFFICIENT_BUFFER)
    {
        if (!(buffer = heap_alloc( size ))) return ERROR_OUTOFMEMORY;
        if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CONTENT_TYPE, NULL, buffer, &size, NULL ))
        {
            return get_last_error();
        }
        if ((p = strstrW( buffer, charsetW )))
        {
            p += strlenW( charsetW );
            while (*p == ' ') p++;
            if (*p++ == '=')
            {
                while (*p == ' ') p++;
                if (!strcmpiW( p, utf8W )) *codepage = CP_UTF8;
            }
        }
        heap_free( buffer );
    }
    return ERROR_SUCCESS;
}

static HRESULT WINAPI winhttp_request_get_ResponseText(
    IWinHttpRequest *iface,
    BSTR *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    UINT codepage;
    DWORD err = ERROR_SUCCESS;
    int len;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if ((err = request_get_codepage( request, &codepage ))) goto done;
    len = MultiByteToWideChar( codepage, 0, request->buffer, request->offset, NULL, 0 );
    if (!(*body = SysAllocStringLen( NULL, len )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    MultiByteToWideChar( codepage, 0, request->buffer, request->offset, *body, len );
    (*body)[len] = 0;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_ResponseBody(
    IWinHttpRequest *iface,
    VARIANT *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    SAFEARRAY *sa;
    HRESULT hr;
    DWORD err = ERROR_SUCCESS;
    char *ptr;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!(sa = SafeArrayCreateVector( VT_UI1, 0, request->offset )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    if ((hr = SafeArrayAccessData( sa, (void **)&ptr )) != S_OK)
    {
        SafeArrayDestroy( sa );
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    memcpy( ptr, request->buffer, request->offset );
    if ((hr = SafeArrayUnaccessData( sa )) != S_OK)
    {
        SafeArrayDestroy( sa );
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    V_VT( body ) =  VT_ARRAY|VT_UI1;
    V_ARRAY( body ) = sa;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

struct stream
{
    IStream IStream_iface;
    LONG    refs;
    char   *data;
    ULARGE_INTEGER pos, size;
};

static inline struct stream *impl_from_IStream( IStream *iface )
{
    return CONTAINING_RECORD( iface, struct stream, IStream_iface );
}

static HRESULT WINAPI stream_QueryInterface( IStream *iface, REFIID riid, void **obj )
{
    struct stream *stream = impl_from_IStream( iface );

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), obj);

    if (IsEqualGUID( riid, &IID_IStream ) || IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IStream_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI stream_AddRef( IStream *iface )
{
    struct stream *stream = impl_from_IStream( iface );
    return InterlockedIncrement( &stream->refs );
}

static ULONG WINAPI stream_Release( IStream *iface )
{
    struct stream *stream = impl_from_IStream( iface );
    LONG refs = InterlockedDecrement( &stream->refs );
    if (!refs)
    {
        heap_free( stream->data );
        heap_free( stream );
    }
    return refs;
}

static HRESULT WINAPI stream_Read( IStream *iface, void *buf, ULONG len, ULONG *read )
{
    struct stream *stream = impl_from_IStream( iface );
    ULONG size;

    if (stream->pos.QuadPart >= stream->size.QuadPart)
    {
        *read = 0;
        return S_FALSE;
    }

    size = min( stream->size.QuadPart - stream->pos.QuadPart, len );
    memcpy( buf, stream->data + stream->pos.QuadPart, size );
    stream->pos.QuadPart += size;
    *read = size;

    return S_OK;
}

static HRESULT WINAPI stream_Write( IStream *iface, const void *buf, ULONG len, ULONG *written )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Seek( IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *newpos )
{
    struct stream *stream = impl_from_IStream( iface );

    if (origin == STREAM_SEEK_SET)
        stream->pos.QuadPart = move.QuadPart;
    else if (origin == STREAM_SEEK_CUR)
        stream->pos.QuadPart += move.QuadPart;
    else if (origin == STREAM_SEEK_END)
        stream->pos.QuadPart = stream->size.QuadPart - move.QuadPart;

    if (newpos) newpos->QuadPart = stream->pos.QuadPart;
    return S_OK;
}

static HRESULT WINAPI stream_SetSize( IStream *iface, ULARGE_INTEGER newsize )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CopyTo( IStream *iface, IStream *stream, ULARGE_INTEGER len, ULARGE_INTEGER *read,
                                     ULARGE_INTEGER *written )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Commit( IStream *iface, DWORD flags )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Revert( IStream *iface )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_LockRegion( IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_UnlockRegion( IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Stat( IStream *iface, STATSTG *stg, DWORD flag )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Clone( IStream *iface, IStream **stream )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IStreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write,
    stream_Seek,
    stream_SetSize,
    stream_CopyTo,
    stream_Commit,
    stream_Revert,
    stream_LockRegion,
    stream_UnlockRegion,
    stream_Stat,
    stream_Clone
};

static HRESULT WINAPI winhttp_request_get_ResponseStream(
    IWinHttpRequest *iface,
    VARIANT *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS;
    struct stream *stream;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!(stream = heap_alloc( sizeof(*stream) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    stream->IStream_iface.lpVtbl = &stream_vtbl;
    stream->refs = 1;
    if (!(stream->data = heap_alloc( request->offset )))
    {
        heap_free( stream );
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    memcpy( stream->data, request->buffer, request->offset );
    stream->pos.QuadPart = 0;
    stream->size.QuadPart = request->offset;
    V_VT( body ) = VT_UNKNOWN;
    V_UNKNOWN( body ) = (IUnknown *)&stream->IStream_iface;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_Option(
    IWinHttpRequest *iface,
    WinHttpRequestOption option,
    VARIANT *value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p\n", request, option, value);

    EnterCriticalSection( &request->cs );
    switch (option)
    {
    case WinHttpRequestOption_URLCodePage:
        V_VT( value ) = VT_I4;
        V_I4( value ) = request->url_codepage;
        break;
    default:
        FIXME("unimplemented option %u\n", option);
        hr = E_NOTIMPL;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_put_Option(
    IWinHttpRequest *iface,
    WinHttpRequestOption option,
    VARIANT value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u, %s\n", request, option, debugstr_variant(&value));

    EnterCriticalSection( &request->cs );
    switch (option)
    {
    case WinHttpRequestOption_EnableRedirects:
    {
        if (V_BOOL( &value ))
            request->disable_feature &= ~WINHTTP_DISABLE_REDIRECTS;
        else
            request->disable_feature |= WINHTTP_DISABLE_REDIRECTS;
        break;
    }
    case WinHttpRequestOption_URLCodePage:
    {
        static const WCHAR utf8W[] = {'u','t','f','-','8',0};
        VARIANT cp;

        VariantInit( &cp );
        hr = VariantChangeType( &cp, &value, 0, VT_UI4 );
        if (SUCCEEDED( hr ))
        {
            request->url_codepage = V_UI4( &cp );
            TRACE("URL codepage: %u\n", request->url_codepage);
        }
        else if (V_VT( &value ) == VT_BSTR && !strcmpiW( V_BSTR( &value ), utf8W ))
        {
            TRACE("URL codepage: UTF-8\n");
            request->url_codepage = CP_UTF8;
            hr = S_OK;
        }
        else
            FIXME("URL codepage %s is not recognized\n", debugstr_variant( &value ));
        break;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        hr = E_NOTIMPL;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_WaitForResponse(
    IWinHttpRequest *iface,
    VARIANT timeout,
    VARIANT_BOOL *succeeded )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err, msecs = (V_I4(&timeout) == -1) ? INFINITE : V_I4(&timeout) * 1000;

    TRACE("%p, %s, %p\n", request, debugstr_variant(&timeout), succeeded);

    EnterCriticalSection( &request->cs );
    if (request->state >= REQUEST_STATE_RESPONSE_RECEIVED)
    {
        LeaveCriticalSection( &request->cs );
        return S_OK;
    }
    switch ((err = request_wait( request, msecs )))
    {
    case ERROR_TIMEOUT:
        if (succeeded) *succeeded = VARIANT_FALSE;
        err = ERROR_SUCCESS;
        break;

    case ERROR_SUCCESS:
        if (succeeded) *succeeded = VARIANT_TRUE;
        break;

    default: break;
    }
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_Abort(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p\n", request);

    EnterCriticalSection( &request->cs );
    cancel_request( request );
    LeaveCriticalSection( &request->cs );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_SetTimeouts(
    IWinHttpRequest *iface,
    LONG resolve_timeout,
    LONG connect_timeout,
    LONG send_timeout,
    LONG receive_timeout )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %d, %d, %d, %d\n", request, resolve_timeout, connect_timeout, send_timeout, receive_timeout);

    EnterCriticalSection( &request->cs );
    request->resolve_timeout = resolve_timeout;
    request->connect_timeout = connect_timeout;
    request->send_timeout    = send_timeout;
    request->receive_timeout = receive_timeout;
    LeaveCriticalSection( &request->cs );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_SetClientCertificate(
    IWinHttpRequest *iface,
    BSTR certificate )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI winhttp_request_SetAutoLogonPolicy(
    IWinHttpRequest *iface,
    WinHttpRequestAutoLogonPolicy policy )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u\n", request, policy );

    EnterCriticalSection( &request->cs );
    switch (policy)
    {
    case AutoLogonPolicy_Always:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
        break;
    case AutoLogonPolicy_OnlyIfBypassProxy:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
        break;
    case AutoLogonPolicy_Never:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
        break;
    default: hr = E_INVALIDARG;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static const struct IWinHttpRequestVtbl winhttp_request_vtbl =
{
    winhttp_request_QueryInterface,
    winhttp_request_AddRef,
    winhttp_request_Release,
    winhttp_request_GetTypeInfoCount,
    winhttp_request_GetTypeInfo,
    winhttp_request_GetIDsOfNames,
    winhttp_request_Invoke,
    winhttp_request_SetProxy,
    winhttp_request_SetCredentials,
    winhttp_request_Open,
    winhttp_request_SetRequestHeader,
    winhttp_request_GetResponseHeader,
    winhttp_request_GetAllResponseHeaders,
    winhttp_request_Send,
    winhttp_request_get_Status,
    winhttp_request_get_StatusText,
    winhttp_request_get_ResponseText,
    winhttp_request_get_ResponseBody,
    winhttp_request_get_ResponseStream,
    winhttp_request_get_Option,
    winhttp_request_put_Option,
    winhttp_request_WaitForResponse,
    winhttp_request_Abort,
    winhttp_request_SetTimeouts,
    winhttp_request_SetClientCertificate,
    winhttp_request_SetAutoLogonPolicy
};

HRESULT WinHttpRequest_create( void **obj )
{
    struct winhttp_request *request;

    TRACE("%p\n", obj);

    if (!(request = heap_alloc( sizeof(*request) ))) return E_OUTOFMEMORY;
    request->IWinHttpRequest_iface.lpVtbl = &winhttp_request_vtbl;
    request->refs = 1;
    request->state = REQUEST_STATE_UNINITIALIZED;
    request->proxy.lpszProxy = NULL;
    request->proxy.lpszProxyBypass = NULL;
    request->url_codepage = CP_UTF8;
    InitializeCriticalSection( &request->cs );
    request->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": winhttp_request.cs");

    *obj = &request->IWinHttpRequest_iface;
    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
