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

#ifndef _WINE_WINHTTP_PRIVATE_H_
#define _WINE_WINHTTP_PRIVATE_H_

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "wine/list.h"
#include "wine/unicode.h"

#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#if defined(__MINGW32__) || defined (_MSC_VER)
# include <ws2tcpip.h>
#else
# define closesocket close
# define ioctlsocket ioctl
#endif

static const WCHAR getW[]    = {'G','E','T',0};
static const WCHAR postW[]   = {'P','O','S','T',0};
static const WCHAR slashW[]  = {'/',0};
static const WCHAR http1_0[] = {'H','T','T','P','/','1','.','0',0};
static const WCHAR http1_1[] = {'H','T','T','P','/','1','.','1',0};

typedef struct _object_header_t object_header_t;

typedef struct
{
    void (*destroy)( object_header_t * );
    BOOL (*query_option)( object_header_t *, DWORD, void *, DWORD * );
    BOOL (*set_option)( object_header_t *, DWORD, void *, DWORD );
} object_vtbl_t;

struct _object_header_t
{
    DWORD type;
    HINTERNET handle;
    const object_vtbl_t *vtbl;
    DWORD flags;
    DWORD disable_flags;
    DWORD logon_policy;
    DWORD redirect_policy;
    DWORD error;
    DWORD_PTR context;
    LONG refs;
    WINHTTP_STATUS_CALLBACK callback;
    DWORD notify_mask;
    struct list entry;
    struct list children;
};

typedef struct
{
    struct list entry;
    WCHAR *name;
    struct list cookies;
} domain_t;

typedef struct
{
    struct list entry;
    WCHAR *name;
    WCHAR *value;
    WCHAR *path;
} cookie_t;

typedef struct
{
    object_header_t hdr;
    LPWSTR agent;
    DWORD access;
    LPWSTR proxy_server;
    LPWSTR proxy_bypass;
    LPWSTR proxy_username;
    LPWSTR proxy_password;
    struct list cookie_cache;
} session_t;

typedef struct
{
    object_header_t hdr;
    session_t *session;
    LPWSTR hostname;    /* final destination of the request */
    LPWSTR servername;  /* name of the server we directly connect to */
    LPWSTR username;
    LPWSTR password;
    INTERNET_PORT hostport;
    INTERNET_PORT serverport;
    struct sockaddr_in sockaddr;
} connect_t;

typedef struct
{
    int socket;
    BOOL secure; /* SSL active on connection? */
    void *ssl_conn;
    char *peek_msg;
    char *peek_msg_mem;
    size_t peek_len;
} netconn_t;

typedef struct
{
    LPWSTR field;
    LPWSTR value;
    BOOL is_request; /* part of request headers? */
} header_t;

typedef struct
{
    object_header_t hdr;
    connect_t *connect;
    LPWSTR verb;
    LPWSTR path;
    LPWSTR version;
    LPWSTR raw_headers;
    netconn_t netconn;
    LPWSTR status_text;
    DWORD content_length; /* total number of bytes to be read (per chunk) */
    DWORD content_read;   /* bytes read so far */
    header_t *headers;
    DWORD num_headers;
} request_t;

typedef struct _task_header_t task_header_t;

struct _task_header_t
{
    request_t *request;
    void (*proc)( task_header_t * );
};

typedef struct
{
    task_header_t hdr;
    LPWSTR headers;
    DWORD headers_len;
    LPVOID optional;
    DWORD optional_len;
    DWORD total_len;
    DWORD_PTR context;
} send_request_t;

typedef struct
{
    task_header_t hdr;
} receive_response_t;

typedef struct
{
    task_header_t hdr;
    LPDWORD available;
} query_data_t;

typedef struct
{
    task_header_t hdr;
    LPVOID buffer;
    DWORD to_read;
    LPDWORD read;
} read_data_t;

typedef struct
{
    task_header_t hdr;
    LPCVOID buffer;
    DWORD to_write;
    LPDWORD written;
} write_data_t;

object_header_t *addref_object( object_header_t * );
object_header_t *grab_object( HINTERNET );
void release_object( object_header_t * );
HINTERNET alloc_handle( object_header_t * );
BOOL free_handle( HINTERNET );

void set_last_error( DWORD );
DWORD get_last_error( void );
void send_callback( object_header_t *, DWORD, LPVOID, DWORD );
void close_connection( request_t * );

BOOL netconn_close( netconn_t * );
BOOL netconn_connect( netconn_t *, const struct sockaddr *, unsigned int );
BOOL netconn_connected( netconn_t * );
BOOL netconn_create( netconn_t *, int, int, int );
BOOL netconn_get_next_line( netconn_t *, char *, DWORD * );
BOOL netconn_init( netconn_t *, BOOL );
BOOL netconn_query_data_available( netconn_t *, DWORD * );
BOOL netconn_recv( netconn_t *, void *, size_t, int, int * );
BOOL netconn_resolve( WCHAR *, INTERNET_PORT, struct sockaddr_in * );
BOOL netconn_secure_connect( netconn_t * );
BOOL netconn_send( netconn_t *, const void *, size_t, int, int * );
const void *netconn_get_certificate( netconn_t * );

BOOL set_cookies( request_t *, const WCHAR * );
BOOL add_cookie_headers( request_t * );
BOOL add_request_headers( request_t *, LPCWSTR, DWORD, DWORD );
void delete_domain( domain_t * );

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline void *heap_alloc_zero( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
}

static inline void *heap_realloc( LPVOID mem, SIZE_T size )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, size );
}

static inline void *heap_realloc_zero( LPVOID mem, SIZE_T size )
{
    return HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size );
}

static inline BOOL heap_free( LPVOID mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;

    if (!src) return NULL;
    dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) strcpyW( dst, src );
    return dst;
}

static inline WCHAR *strdupAW( const char *src )
{
    WCHAR *dst = NULL;
    if (src)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
        if ((dst = heap_alloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    }
    return dst;
}

static inline char *strdupWA( const WCHAR *src )
{
    char *dst = NULL;
    if (src)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
        if ((dst = heap_alloc( len )))
            WideCharToMultiByte( CP_ACP, 0, src, -1, dst, len, NULL, NULL );
    }
    return dst;
}

#endif /* _WINE_WINHTTP_PRIVATE_H_ */
