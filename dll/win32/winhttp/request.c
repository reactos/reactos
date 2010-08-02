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

#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "winhttp_private.h"

#include "inet_ntop.c"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

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

static DWORD CALLBACK task_thread( LPVOID param )
{
    task_header_t *task = param;

    task->proc( task );

    release_object( &task->request->hdr );
    heap_free( task );
    return ERROR_SUCCESS;
}

static BOOL queue_task( task_header_t *task )
{
    return QueueUserWorkItem( task_thread, task, WT_EXECUTELONGFUNCTION );
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
    if (!*q)
    {
        WARN("no value in line %s\n", debugstr_w(string));
        return header;
    }
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

static BOOL process_header( request_t *request, LPCWSTR field, LPCWSTR value, DWORD flags, BOOL request_only )
{
    int index;
    header_t *header;

    TRACE("%s: %s 0x%08x\n", debugstr_w(field), debugstr_w(value), flags);

    /* replace wins out over add */
    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE) flags &= ~WINHTTP_ADDREQ_FLAG_ADD;

    if (flags & WINHTTP_ADDREQ_FLAG_ADD) index = -1;
    else
        index = get_header_index( request, field, 0, request_only );

    if (index >= 0)
    {
        if (flags & WINHTTP_ADDREQ_FLAG_ADD_IF_NEW) return FALSE;
        header = &request->headers[index];
    }
    else if (value)
    {
        header_t hdr;

        hdr.field = (LPWSTR)field;
        hdr.value = (LPWSTR)value;
        hdr.is_request = request_only;

        return insert_header( request, &hdr );
    }
    /* no value to delete */
    else return TRUE;

    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE)
    {
        delete_header( request, index );
        if (value)
        {
            header_t hdr;

            hdr.field = (LPWSTR)field;
            hdr.value = (LPWSTR)value;
            hdr.is_request = request_only;

            return insert_header( request, &hdr );
        }
        return TRUE;
    }
    else if (flags & (WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON))
    {
        WCHAR sep, *tmp;
        int len, orig_len, value_len;

        orig_len = strlenW( header->value );
        value_len = strlenW( value );

        if (flags & WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA) sep = ',';
        else sep = ';';

        len = orig_len + value_len + 2;
        if ((tmp = heap_realloc( header->value, (len + 1) * sizeof(WCHAR) )))
        {
            header->value = tmp;

            header->value[orig_len] = sep;
            orig_len++;
            header->value[orig_len] = ' ';
            orig_len++;

            memcpy( &header->value[orig_len], value, value_len * sizeof(WCHAR) );
            header->value[len] = 0;
            return TRUE;
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
    if (!(buffer = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;
    strcpyW( buffer, headers );

    p = buffer;
    do
    {
        q = p;
        while (*q)
        {
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

    if (!headers)
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
    return ret;
}

static WCHAR *build_request_string( request_t *request )
{
    static const WCHAR space[]   = {' ',0};
    static const WCHAR crlf[]    = {'\r','\n',0};
    static const WCHAR colon[]   = {':',' ',0};
    static const WCHAR twocrlf[] = {'\r','\n','\r','\n',0};

    WCHAR *ret;
    const WCHAR **headers, **p;
    unsigned int len, i = 0, j;

    /* allocate space for an array of all the string pointers to be added */
    len = request->num_headers * 4 + 7;
    if (!(headers = heap_alloc( len * sizeof(LPCWSTR) ))) return NULL;

    headers[i++] = request->verb;
    headers[i++] = space;
    headers[i++] = request->path;
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
    {
        heap_free( headers );
        return NULL;
    }
    *ret = 0;
    for (p = headers; *p; p++) strcatW( ret, *p );

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

        if ((len + 1) * sizeof(WCHAR) > *buflen || !buffer)
        {
            len++;
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (buffer)
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
            *q = 0;
            TRACE("returning data: %s\n", debugstr_wn(buffer, len));
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
        if (len + sizeof(WCHAR) > *buflen || !buffer)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (buffer)
        {
            memcpy( buffer, headers, len + sizeof(WCHAR) );
            TRACE("returning data: %s\n", debugstr_wn(buffer, len / sizeof(WCHAR)));
            ret = TRUE;
        }
        *buflen = len;
        if (request_only) heap_free( headers );
        return ret;
    }
    default:
    {
        if (attr >= sizeof(attribute_table)/sizeof(attribute_table[0]) || !attribute_table[attr])
        {
            FIXME("attribute %u not implemented\n", attr);
            return FALSE;
        }
        TRACE("attribute %s\n", debugstr_w(attribute_table[attr]));
        header_index = get_header_index( request, attribute_table[attr], requested_index, request_only );
    }
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
        int *number = buffer;
        if (sizeof(int) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (number)
        {
            *number = atoiW( header->value );
            TRACE("returning number: %d\n", *number);
            ret = TRUE;
        }
        *buflen = sizeof(int);
    }
    else if (level & WINHTTP_QUERY_FLAG_SYSTEMTIME)
    {
        SYSTEMTIME *st = buffer;
        if (sizeof(SYSTEMTIME) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (st && (ret = WinHttpTimeToSystemTime( header->value, st )))
        {
            TRACE("returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
                  st->wYear, st->wMonth, st->wDay, st->wDayOfWeek,
                  st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
        }
        *buflen = sizeof(SYSTEMTIME);
    }
    else if (header->value)
    {
        WCHAR *string = buffer;
        DWORD len = (strlenW( header->value ) + 1) * sizeof(WCHAR);
        if (len > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            *buflen = len;
            return FALSE;
        }
        else if (string)
        {
            strcpyW( string, header->value );
            TRACE("returning string: %s\n", debugstr_w(string));
            ret = TRUE;
        }
        *buflen = len - sizeof(WCHAR);
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
    return ret;
}

static BOOL open_connection( request_t *request )
{
    connect_t *connect;
    char address[32];
    WCHAR *addressW;
    INTERNET_PORT port;

    if (netconn_connected( &request->netconn )) return TRUE;

    connect = request->connect;
    port = connect->hostport ? connect->hostport : (request->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, connect->servername, strlenW(connect->servername) + 1 );

    if (!netconn_resolve( connect->servername, port, &connect->sockaddr )) return FALSE;
    inet_ntop( connect->sockaddr.sin_family, &connect->sockaddr.sin_addr, address, sizeof(address) );
    addressW = strdupAW( address );

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, addressW, strlenW(addressW) + 1 );

    TRACE("connecting to %s:%u\n", address, ntohs(connect->sockaddr.sin_port));

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, addressW, 0 );

    if (!netconn_create( &request->netconn, connect->sockaddr.sin_family, SOCK_STREAM, 0 ))
    {
        heap_free( addressW );
        return FALSE;
    }
    if (!netconn_connect( &request->netconn, (struct sockaddr *)&connect->sockaddr, sizeof(struct sockaddr_in) ))
    {
        netconn_close( &request->netconn );
        heap_free( addressW );
        return FALSE;
    }
    if (request->hdr.flags & WINHTTP_FLAG_SECURE && !netconn_secure_connect( &request->netconn ))
    {
        netconn_close( &request->netconn );
        heap_free( addressW );
        return FALSE;
    }

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, addressW, strlenW(addressW) + 1 );

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

    if (!(ret = open_connection( request ))) goto end;
    if (!(req = build_request_string( request ))) goto end;

    if (!(req_ascii = strdupWA( req ))) goto end;
    TRACE("full request: %s\n", debugstr_a(req_ascii));
    len = strlen(req_ascii);

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, NULL, 0 );

    ret = netconn_send( &request->netconn, req_ascii, len, 0, &bytes_sent );
    heap_free( req_ascii );
    if (!ret) goto end;

    if (optional_len && !netconn_send( &request->netconn, optional, optional_len, 0, &bytes_sent )) goto end;
    len += optional_len;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_SENT, &len, sizeof(DWORD) );

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

#define MAX_REPLY_LEN   1460
#define INITIAL_HEADER_BUFFER_LEN  512

static BOOL read_reply( request_t *request )
{
    static const WCHAR crlf[] = {'\r','\n',0};

    char buffer[MAX_REPLY_LEN];
    DWORD buflen, len, offset, received_len, crlf_len = 2; /* strlenW(crlf) */
    char *status_code, *status_text;
    WCHAR *versionW, *status_textW, *raw_headers;
    WCHAR status_codeW[4]; /* sizeof("nnn") */

    if (!netconn_connected( &request->netconn )) return FALSE;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NULL, 0 );

    received_len = 0;
    do
    {
        buflen = MAX_REPLY_LEN;
        if (!netconn_get_next_line( &request->netconn, buffer, &buflen )) return FALSE;
        received_len += buflen;

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
    if (!(process_header( request, attr_status, status_codeW, WINHTTP_ADDREQ_FLAG_REPLACE, FALSE ))) return FALSE;

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
        if (!netconn_get_next_line( &request->netconn, buffer, &buflen )) goto end;
        received_len += buflen;
        if (!*buffer) break;

        while (len - offset < buflen + crlf_len)
        {
            WCHAR *tmp;
            len *= 2;
            if (!(tmp = heap_realloc( raw_headers, len * sizeof(WCHAR) ))) return FALSE;
            request->raw_headers = raw_headers = tmp;
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

end:
    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, &received_len, sizeof(DWORD) );
    return TRUE;
}

static BOOL handle_redirect( request_t *request )
{
    BOOL ret = FALSE;
    DWORD size, len;
    URL_COMPONENTS uc;
    connect_t *connect = request->connect;
    INTERNET_PORT port;
    WCHAR *hostname = NULL, *location = NULL;
    int index;

    size = 0;
    query_headers( request, WINHTTP_QUERY_LOCATION, NULL, NULL, &size, NULL );
    if (!(location = heap_alloc( size ))) return FALSE;
    if (!query_headers( request, WINHTTP_QUERY_LOCATION, NULL, location, &size, NULL )) goto end;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REDIRECT, location, size / sizeof(WCHAR) + 1 );

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = ~0u;

    if (!WinHttpCrackUrl( location, size / sizeof(WCHAR), 0, &uc )) /* assume relative redirect */
    {
        WCHAR *path, *p;

        len = strlenW( location ) + 1;
        if (location[0] != '/') len++;
        if (!(p = path = heap_alloc( len * sizeof(WCHAR) ))) goto end;

        if (location[0] != '/') *p++ = '/';
        strcpyW( p, location );

        heap_free( request->path );
        request->path = path;
    }
    else
    {
        if (uc.nScheme == INTERNET_SCHEME_HTTP && request->hdr.flags & WINHTTP_FLAG_SECURE)
        {
            TRACE("redirect from secure page to non-secure page\n");
            request->hdr.flags &= ~WINHTTP_FLAG_SECURE;
        }
        else if (uc.nScheme == INTERNET_SCHEME_HTTPS && !(request->hdr.flags & WINHTTP_FLAG_SECURE))
        {
            TRACE("redirect from non-secure page to secure page\n");
            request->hdr.flags |= WINHTTP_FLAG_SECURE;
        }

        len = uc.dwHostNameLength;
        if (!(hostname = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
        memcpy( hostname, uc.lpszHostName, len * sizeof(WCHAR) );
        hostname[len] = 0;

        port = uc.nPort ? uc.nPort : (uc.nScheme == INTERNET_SCHEME_HTTPS ? 443 : 80);
        if (strcmpiW( connect->servername, hostname ) || connect->serverport != port)
        {
            heap_free( connect->hostname );
            connect->hostname = hostname;
            heap_free( connect->servername );
            connect->servername = strdupW( connect->hostname );
            connect->serverport = connect->hostport = port;

            netconn_close( &request->netconn );
            if (!(ret = netconn_init( &request->netconn, request->hdr.flags & WINHTTP_FLAG_SECURE ))) goto end;
        }
        if (!(ret = add_host_header( request, WINHTTP_ADDREQ_FLAG_REPLACE ))) goto end;
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

    /* redirects are always GET requests */
    heap_free( request->verb );
    request->verb = strdupW( getW );
    ret = TRUE;

end:
    if (!ret) heap_free( hostname );
    heap_free( location );
    return ret;
}

static BOOL receive_data( request_t *request, void *buffer, DWORD size, DWORD *read, BOOL async )
{
    DWORD to_read;
    int bytes_read;

    to_read = min( size, request->content_length - request->content_read );
    if (!netconn_recv( &request->netconn, buffer, to_read, async ? 0 : MSG_WAITALL, &bytes_read ))
    {
        if (bytes_read != to_read)
        {
            ERR("not all data received %d/%d\n", bytes_read, to_read);
        }
        /* always return success, even if the network layer returns an error */
        *read = 0;
        return TRUE;
    }
    request->content_read += bytes_read;
    *read = bytes_read;
    return TRUE;
}

static DWORD get_chunk_size( const char *buffer )
{
    const char *p;
    DWORD size = 0;

    for (p = buffer; *p; p++)
    {
        if (*p >= '0' && *p <= '9') size = size * 16 + *p - '0';
        else if (*p >= 'a' && *p <= 'f') size = size * 16 + *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') size = size * 16 + *p - 'A' + 10;
        else if (*p == ';') break;
    }
    return size;
}

static BOOL receive_data_chunked( request_t *request, void *buffer, DWORD size, DWORD *read, BOOL async )
{
    char reply[MAX_REPLY_LEN], *p = buffer;
    DWORD buflen, to_read, to_write = size;
    int bytes_read;

    *read = 0;
    for (;;)
    {
        if (*read == size) break;

        if (request->content_length == ~0u) /* new chunk */
        {
            buflen = sizeof(reply);
            if (!netconn_get_next_line( &request->netconn, reply, &buflen )) break;

            if (!(request->content_length = get_chunk_size( reply )))
            {
                /* zero sized chunk marks end of transfer; read any trailing headers and return */
                read_reply( request );
                break;
            }
        }
        to_read = min( to_write, request->content_length - request->content_read );

        if (!netconn_recv( &request->netconn, p, to_read, async ? 0 : MSG_WAITALL, &bytes_read ))
        {
            if (bytes_read != to_read)
            {
                ERR("Not all data received %d/%d\n", bytes_read, to_read);
            }
            /* always return success, even if the network layer returns an error */
            *read = 0;
            break;
        }
        if (!bytes_read) break;

        request->content_read += bytes_read;
        to_write -= bytes_read;
        *read += bytes_read;
        p += bytes_read;

        if (request->content_read == request->content_length) /* chunk complete */
        {
            request->content_read = 0;
            request->content_length = ~0u;

            buflen = sizeof(reply);
            if (!netconn_get_next_line( &request->netconn, reply, &buflen ))
            {
                ERR("Malformed chunk\n");
                *read = 0;
                break;
            }
        }
    }
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
    request->content_length = ~0u;
    request->content_read = 0;
}

static BOOL read_data( request_t *request, void *buffer, DWORD to_read, DWORD *read, BOOL async )
{
    static const WCHAR chunked[] = {'c','h','u','n','k','e','d',0};

    BOOL ret;
    WCHAR encoding[20];
    DWORD num_bytes, buflen = sizeof(encoding);

    if (query_headers( request, WINHTTP_QUERY_TRANSFER_ENCODING, NULL, encoding, &buflen, NULL ) &&
        !strcmpiW( encoding, chunked ))
    {
        ret = receive_data_chunked( request, buffer, to_read, &num_bytes, async );
    }
    else
        ret = receive_data( request, buffer, to_read, &num_bytes, async );

    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, buffer, num_bytes );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_READ_DATA;
            result.dwError  = get_last_error();
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    if (ret)
    {
        if (read) *read = num_bytes;
        if (!num_bytes) finished_reading( request );
    }
    return ret;
}

/* read any content returned by the server so that the connection can be reused */
static void drain_content( request_t *request )
{
    DWORD bytes_read;
    char buffer[2048];

    if (!request->content_length) return;
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

static BOOL receive_response( request_t *request, BOOL async )
{
    BOOL ret;
    DWORD size, query, status;

    for (;;)
    {
        if (!(ret = read_reply( request ))) break;

        size = sizeof(DWORD);
        query = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
        if (!(ret = query_headers( request, query, NULL, &status, &size, NULL ))) break;

        size = sizeof(DWORD);
        query = WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER;
        if (!query_headers( request, query, NULL, &request->content_length, &size, NULL ))
            request->content_length = ~0u;

        if (!(request->hdr.disable_flags & WINHTTP_DISABLE_COOKIES)) record_cookies( request );

        if (status == 301 || status == 302)
        {
            if (request->hdr.disable_flags & WINHTTP_DISABLE_REDIRECTS) break;

            drain_content( request );
            if (!(ret = handle_redirect( request ))) break;

            clear_response_headers( request );
            ret = send_request( request, NULL, 0, NULL, 0, 0, 0, FALSE ); /* recurse synchronously */
            continue;
        }
        if (status == 401) FIXME("authentication not supported\n");
        break;
    }

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
    return ret;
}

static BOOL query_data( request_t *request, LPDWORD available, BOOL async )
{
    BOOL ret;
    DWORD num_bytes;

    if ((ret = netconn_query_data_available( &request->netconn, &num_bytes )))
    {
        if (request->content_read < request->content_length)
        {
            if (!num_bytes)
            {
                char buffer[4096];
                size_t to_read = min( sizeof(buffer), request->content_length - request->content_read );

                ret = netconn_recv( &request->netconn, buffer, to_read, MSG_PEEK, (int *)&num_bytes );
                if (ret && !num_bytes) WARN("expected more data to be available\n");
            }
        }
        else if (num_bytes)
        {
            WARN("extra data available %u\n", num_bytes);
            ret = FALSE;
        }
    }
    TRACE("%u bytes available\n", num_bytes);

    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, &num_bytes, sizeof(DWORD) );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_QUERY_DATA_AVAILABLE;
            result.dwError  = get_last_error();
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    if (ret && available) *available = num_bytes;
    return ret;
}

static void task_query_data( task_header_t *task )
{
    query_data_t *q = (query_data_t *)task;
    query_data( q->hdr.request, q->available, TRUE );
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
        q->hdr.proc    = task_query_data;
        q->available   = available;

        addref_object( &request->hdr );
        ret = queue_task( (task_header_t *)q );
    }
    else
        ret = query_data( request, available, FALSE );

    release_object( &request->hdr );
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
    return ret;
}

static BOOL write_data( request_t *request, LPCVOID buffer, DWORD to_write, LPDWORD written, BOOL async )
{
    BOOL ret;
    int num_bytes;

    ret = netconn_send( &request->netconn, buffer, to_write, 0, &num_bytes );

    if (async)
    {
        if (ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, &num_bytes, sizeof(DWORD) );
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
    return ret;
}

#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))

static DWORD auth_scheme_from_header( WCHAR *header )
{
    static const WCHAR basic[]     = {'B','a','s','i','c'};
    static const WCHAR ntlm[]      = {'N','T','L','M'};
    static const WCHAR passport[]  = {'P','a','s','s','p','o','r','t'};
    static const WCHAR digest[]    = {'D','i','g','e','s','t'};
    static const WCHAR negotiate[] = {'N','e','g','o','t','i','a','t','e'};

    if (!strncmpiW( header, basic, ARRAYSIZE(basic) ) &&
        (header[ARRAYSIZE(basic)] == ' ' || !header[ARRAYSIZE(basic)])) return WINHTTP_AUTH_SCHEME_BASIC;

    if (!strncmpiW( header, ntlm, ARRAYSIZE(ntlm) ) &&
        (header[ARRAYSIZE(ntlm)] == ' ' || !header[ARRAYSIZE(ntlm)])) return WINHTTP_AUTH_SCHEME_NTLM;

    if (!strncmpiW( header, passport, ARRAYSIZE(passport) ) &&
        (header[ARRAYSIZE(passport)] == ' ' || !header[ARRAYSIZE(passport)])) return WINHTTP_AUTH_SCHEME_PASSPORT;

    if (!strncmpiW( header, digest, ARRAYSIZE(digest) ) &&
        (header[ARRAYSIZE(digest)] == ' ' || !header[ARRAYSIZE(digest)])) return WINHTTP_AUTH_SCHEME_DIGEST;

    if (!strncmpiW( header, negotiate, ARRAYSIZE(negotiate) ) &&
        (header[ARRAYSIZE(negotiate)] == ' ' || !header[ARRAYSIZE(negotiate)])) return WINHTTP_AUTH_SCHEME_NEGOTIATE;

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
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;

        index--;
        if (!(buffer = heap_alloc( size ))) return FALSE;
        if (!query_headers( request, level, NULL, buffer, &size, &index ))
        {
            heap_free( buffer );
            return FALSE;
        }
        scheme = auth_scheme_from_header( buffer );
        if (index == 1) *first = scheme;
        *supported |= scheme;

        heap_free( buffer );
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

static BOOL set_credentials( request_t *request, DWORD target, DWORD scheme, LPCWSTR username, LPCWSTR password )
{
    static const WCHAR basic[] = {'B','a','s','i','c',' ',0};

    const WCHAR *auth_scheme, *auth_target;
    WCHAR *auth_header;
    DWORD len, auth_data_len;
    char *auth_data;
    BOOL ret;

    switch (target)
    {
    case WINHTTP_AUTH_TARGET_SERVER: auth_target = attr_authorization; break;
    case WINHTTP_AUTH_TARGET_PROXY:  auth_target = attr_proxy_authorization; break;
    default:
        WARN("unknown target %x\n", target);
        return FALSE;
    }
    switch (scheme)
    {
    case WINHTTP_AUTH_SCHEME_BASIC:
    {
        int userlen = WideCharToMultiByte( CP_UTF8, 0, username, strlenW( username ), NULL, 0, NULL, NULL );
        int passlen = WideCharToMultiByte( CP_UTF8, 0, password, strlenW( password ), NULL, 0, NULL, NULL );

        TRACE("basic authentication\n");

        auth_scheme = basic;
        auth_data_len = userlen + 1 + passlen;
        if (!(auth_data = heap_alloc( auth_data_len ))) return FALSE;

        WideCharToMultiByte( CP_UTF8, 0, username, -1, auth_data, userlen, NULL, NULL );
        auth_data[userlen] = ':';
        WideCharToMultiByte( CP_UTF8, 0, password, -1, auth_data + userlen + 1, passlen, NULL, NULL );
        break;
    }
    case WINHTTP_AUTH_SCHEME_NTLM:
    case WINHTTP_AUTH_SCHEME_PASSPORT:
    case WINHTTP_AUTH_SCHEME_DIGEST:
    case WINHTTP_AUTH_SCHEME_NEGOTIATE:
        FIXME("unimplemented authentication scheme %x\n", scheme);
        return FALSE;
    default:
        WARN("unknown authentication scheme %x\n", scheme);
        return FALSE;
    }

    len = strlenW( auth_scheme ) + ((auth_data_len + 2) * 4) / 3;
    if (!(auth_header = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( auth_data );
        return FALSE;
    }
    strcpyW( auth_header, auth_scheme );
    encode_base64( auth_data, auth_data_len, auth_header + strlenW( auth_header ) );

    ret = process_header( request, auth_target, auth_header, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE, TRUE );

    heap_free( auth_data );
    heap_free( auth_header );
    return ret;
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
    return ret;
}
