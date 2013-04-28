/*
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
 * Copyright 2006 Robert Shearman for CodeWeavers
 * Copyright 2011 Jacek Caban for CodeWeavers
 *
 * Ulrich Czekalla
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

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>
//#include "wine/port.h"

//#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
//#include <stdarg.h>
#include <stdio.h>
//#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
//#include <time.h>
#include <assert.h>
#ifdef HAVE_ZLIB
#  include <zlib.h>
#endif

#include <windef.h>
#include <winbase.h>
#include <wininet.h>
//#include "winerror.h"
#include <winternl.h>
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_GDI
#include <shlwapi.h>
#include <sspi.h>
//#include "wincrypt.h"
#include <winuser.h>
#include <cryptuiapi.h>

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include "internet.h"
#include <wine/debug.h>
#include <wine/exception.h>
//#include "wine/unicode.h"

// ReactOS
#include "inet_ntop.c"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

static const WCHAR g_szHttp1_0[] = {'H','T','T','P','/','1','.','0',0};
static const WCHAR g_szHttp1_1[] = {'H','T','T','P','/','1','.','1',0};
static const WCHAR szOK[] = {'O','K',0};
static const WCHAR szDefaultHeader[] = {'H','T','T','P','/','1','.','0',' ','2','0','0',' ','O','K',0};
static const WCHAR hostW[] = { 'H','o','s','t',0 };
static const WCHAR szAuthorization[] = { 'A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szProxy_Authorization[] = { 'P','r','o','x','y','-','A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szStatus[] = { 'S','t','a','t','u','s',0 };
static const WCHAR szKeepAlive[] = {'K','e','e','p','-','A','l','i','v','e',0};
static const WCHAR szGET[] = { 'G','E','T', 0 };
static const WCHAR szHEAD[] = { 'H','E','A','D', 0 };
static const WCHAR szCrLf[] = {'\r','\n', 0};

static const WCHAR szAccept[] = { 'A','c','c','e','p','t',0 };
static const WCHAR szAccept_Charset[] = { 'A','c','c','e','p','t','-','C','h','a','r','s','e','t', 0 };
static const WCHAR szAccept_Encoding[] = { 'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szAccept_Language[] = { 'A','c','c','e','p','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szAccept_Ranges[] = { 'A','c','c','e','p','t','-','R','a','n','g','e','s',0 };
static const WCHAR szAge[] = { 'A','g','e',0 };
static const WCHAR szAllow[] = { 'A','l','l','o','w',0 };
static const WCHAR szCache_Control[] = { 'C','a','c','h','e','-','C','o','n','t','r','o','l',0 };
static const WCHAR szConnection[] = { 'C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szContent_Base[] = { 'C','o','n','t','e','n','t','-','B','a','s','e',0 };
static const WCHAR szContent_Encoding[] = { 'C','o','n','t','e','n','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_ID[] = { 'C','o','n','t','e','n','t','-','I','D',0 };
static const WCHAR szContent_Language[] = { 'C','o','n','t','e','n','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szContent_Length[] = { 'C','o','n','t','e','n','t','-','L','e','n','g','t','h',0 };
static const WCHAR szContent_Location[] = { 'C','o','n','t','e','n','t','-','L','o','c','a','t','i','o','n',0 };
static const WCHAR szContent_MD5[] = { 'C','o','n','t','e','n','t','-','M','D','5',0 };
static const WCHAR szContent_Range[] = { 'C','o','n','t','e','n','t','-','R','a','n','g','e',0 };
static const WCHAR szContent_Transfer_Encoding[] = { 'C','o','n','t','e','n','t','-','T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_Type[] = { 'C','o','n','t','e','n','t','-','T','y','p','e',0 };
static const WCHAR szCookie[] = { 'C','o','o','k','i','e',0 };
static const WCHAR szDate[] = { 'D','a','t','e',0 };
static const WCHAR szFrom[] = { 'F','r','o','m',0 };
static const WCHAR szETag[] = { 'E','T','a','g',0 };
static const WCHAR szExpect[] = { 'E','x','p','e','c','t',0 };
static const WCHAR szExpires[] = { 'E','x','p','i','r','e','s',0 };
static const WCHAR szIf_Match[] = { 'I','f','-','M','a','t','c','h',0 };
static const WCHAR szIf_Modified_Since[] = { 'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szIf_None_Match[] = { 'I','f','-','N','o','n','e','-','M','a','t','c','h',0 };
static const WCHAR szIf_Range[] = { 'I','f','-','R','a','n','g','e',0 };
static const WCHAR szIf_Unmodified_Since[] = { 'I','f','-','U','n','m','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szLast_Modified[] = { 'L','a','s','t','-','M','o','d','i','f','i','e','d',0 };
static const WCHAR szLocation[] = { 'L','o','c','a','t','i','o','n',0 };
static const WCHAR szMax_Forwards[] = { 'M','a','x','-','F','o','r','w','a','r','d','s',0 };
static const WCHAR szMime_Version[] = { 'M','i','m','e','-','V','e','r','s','i','o','n',0 };
static const WCHAR szPragma[] = { 'P','r','a','g','m','a',0 };
static const WCHAR szProxy_Authenticate[] = { 'P','r','o','x','y','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };
static const WCHAR szProxy_Connection[] = { 'P','r','o','x','y','-','C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szPublic[] = { 'P','u','b','l','i','c',0 };
static const WCHAR szRange[] = { 'R','a','n','g','e',0 };
static const WCHAR szReferer[] = { 'R','e','f','e','r','e','r',0 };
static const WCHAR szRetry_After[] = { 'R','e','t','r','y','-','A','f','t','e','r',0 };
static const WCHAR szServer[] = { 'S','e','r','v','e','r',0 };
static const WCHAR szSet_Cookie[] = { 'S','e','t','-','C','o','o','k','i','e',0 };
static const WCHAR szTransfer_Encoding[] = { 'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szUnless_Modified_Since[] = { 'U','n','l','e','s','s','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szUpgrade[] = { 'U','p','g','r','a','d','e',0 };
static const WCHAR szURI[] = { 'U','R','I',0 };
static const WCHAR szUser_Agent[] = { 'U','s','e','r','-','A','g','e','n','t',0 };
static const WCHAR szVary[] = { 'V','a','r','y',0 };
static const WCHAR szVia[] = { 'V','i','a',0 };
static const WCHAR szWarning[] = { 'W','a','r','n','i','n','g',0 };
static const WCHAR szWWW_Authenticate[] = { 'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };

#define HTTP_REFERER    szReferer
#define HTTP_ACCEPT     szAccept
#define HTTP_USERAGENT  szUser_Agent

#define HTTP_ADDHDR_FLAG_ADD				0x20000000
#define HTTP_ADDHDR_FLAG_ADD_IF_NEW			0x10000000
#define HTTP_ADDHDR_FLAG_COALESCE			0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA		0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON	0x01000000
#define HTTP_ADDHDR_FLAG_REPLACE			0x80000000
#define HTTP_ADDHDR_FLAG_REQ				0x02000000

#define COLLECT_TIME 60000

#define ARRAYSIZE(array) (sizeof(array)/sizeof((array)[0]))

struct HttpAuthInfo
{
    LPWSTR scheme;
    CredHandle cred;
    CtxtHandle ctx;
    TimeStamp exp;
    ULONG attr;
    ULONG max_token;
    void *auth_data;
    unsigned int auth_data_len;
    BOOL finished; /* finished authenticating */
};


typedef struct _basicAuthorizationData
{
    struct list entry;

    LPWSTR host;
    LPWSTR realm;
    LPSTR  authorization;
    UINT   authorizationLen;
} basicAuthorizationData;

typedef struct _authorizationData
{
    struct list entry;

    LPWSTR host;
    LPWSTR scheme;
    LPWSTR domain;
    UINT   domain_len;
    LPWSTR user;
    UINT   user_len;
    LPWSTR password;
    UINT   password_len;
} authorizationData;

static struct list basicAuthorizationCache = LIST_INIT(basicAuthorizationCache);
static struct list authorizationCache = LIST_INIT(authorizationCache);

static CRITICAL_SECTION authcache_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &authcache_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": authcache_cs") }
};
static CRITICAL_SECTION authcache_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static BOOL HTTP_GetResponseHeaders(http_request_t *req, BOOL clear);
static DWORD HTTP_ProcessHeader(http_request_t *req, LPCWSTR field, LPCWSTR value, DWORD dwModifier);
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer);
static DWORD HTTP_InsertCustomHeader(http_request_t *req, LPHTTPHEADERW lpHdr);
static INT HTTP_GetCustomHeaderIndex(http_request_t *req, LPCWSTR lpszField, INT index, BOOL Request);
static BOOL HTTP_DeleteCustomHeader(http_request_t *req, DWORD index);
static LPWSTR HTTP_build_req( LPCWSTR *list, int len );
static DWORD HTTP_HttpQueryInfoW(http_request_t*, DWORD, LPVOID, LPDWORD, LPDWORD);
static LPWSTR HTTP_GetRedirectURL(http_request_t *req, LPCWSTR lpszUrl);
static UINT HTTP_DecodeBase64(LPCWSTR base64, LPSTR bin);
static BOOL HTTP_VerifyValidHeader(http_request_t *req, LPCWSTR field);

static CRITICAL_SECTION connection_pool_cs;
static CRITICAL_SECTION_DEBUG connection_pool_debug =
{
    0, 0, &connection_pool_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": connection_pool_cs") }
};
static CRITICAL_SECTION connection_pool_cs = { &connection_pool_debug, -1, 0, 0, 0, 0 };

static struct list connection_pool = LIST_INIT(connection_pool);
static BOOL collector_running;

void server_addref(server_t *server)
{
    InterlockedIncrement(&server->ref);
}

void server_release(server_t *server)
{
    if(InterlockedDecrement(&server->ref))
        return;

#ifndef __REACTOS__
    if(!server->ref)
        server->keep_until = (DWORD64)GetTickCount() + COLLECT_TIME;
#else
    EnterCriticalSection(&connection_pool_cs);
    list_remove(&server->entry);
    LeaveCriticalSection(&connection_pool_cs);
    
    heap_free(server->name);
    heap_free(server);
#endif
}

static server_t *get_server(const WCHAR *name, INTERNET_PORT port)
{
    server_t *iter, *server = NULL;

    EnterCriticalSection(&connection_pool_cs);

    LIST_FOR_EACH_ENTRY(iter, &connection_pool, server_t, entry) {
        if(iter->port == port && !strcmpW(iter->name, name)) {
            server = iter;
            server_addref(server);
            break;
        }
    }

    if(!server) {
        server = heap_alloc(sizeof(*server));
        if(server) {
            server->addr_len = 0;
            server->ref = 1;
            server->port = port;
            list_init(&server->conn_pool);
            server->name = heap_strdupW(name);
            if(server->name) {
                list_add_head(&connection_pool, &server->entry);
            }else {
                heap_free(server);
                server = NULL;
            }
        }
    }

    LeaveCriticalSection(&connection_pool_cs);

    return server;
}

BOOL collect_connections(BOOL collect_all)
{
    netconn_t *netconn, *netconn_safe;
    server_t *server, *server_safe;
    BOOL remaining = FALSE;
    DWORD64 now;

    now = GetTickCount();

    LIST_FOR_EACH_ENTRY_SAFE(server, server_safe, &connection_pool, server_t, entry) {
        LIST_FOR_EACH_ENTRY_SAFE(netconn, netconn_safe, &server->conn_pool, netconn_t, pool_entry) {
            if(collect_all || netconn->keep_until < now) {
                TRACE("freeing %p\n", netconn);
                list_remove(&netconn->pool_entry);
                free_netconn(netconn);
            }else {
                remaining = TRUE;
            }
        }

        if(!server->ref) {
            if(collect_all || server->keep_until < now) {
                list_remove(&server->entry);

                heap_free(server->name);
                heap_free(server);
            }else {
                remaining = TRUE;
            }
        }
    }

    return remaining;
}

static DWORD WINAPI collect_connections_proc(void *arg)
{
    BOOL remaining_conns;

    do {
        /* FIXME: Use more sophisticated method */
        Sleep(5000);

        EnterCriticalSection(&connection_pool_cs);

        remaining_conns = collect_connections(FALSE);
        if(!remaining_conns)
            collector_running = FALSE;

        LeaveCriticalSection(&connection_pool_cs);
    }while(remaining_conns);

    FreeLibraryAndExitThread(WININET_hModule, 0);
}

static LPHTTPHEADERW HTTP_GetHeader(http_request_t *req, LPCWSTR head)
{
    int HeaderIndex = 0;
    HeaderIndex = HTTP_GetCustomHeaderIndex(req, head, 0, TRUE);
    if (HeaderIndex == -1)
        return NULL;
    else
        return &req->custHeaders[HeaderIndex];
}

typedef enum {
    READMODE_SYNC,
    READMODE_ASYNC,
    READMODE_NOBLOCK
} read_mode_t;

struct data_stream_vtbl_t {
    DWORD (*get_avail_data)(data_stream_t*,http_request_t*);
    BOOL (*end_of_data)(data_stream_t*,http_request_t*);
    DWORD (*read)(data_stream_t*,http_request_t*,BYTE*,DWORD,DWORD*,read_mode_t);
    BOOL (*drain_content)(data_stream_t*,http_request_t*);
    void (*destroy)(data_stream_t*);
};

typedef struct {
    data_stream_t data_stream;

    BYTE buf[READ_BUFFER_SIZE];
    DWORD buf_size;
    DWORD buf_pos;
    DWORD chunk_size;
} chunked_stream_t;

static inline void destroy_data_stream(data_stream_t *stream)
{
    stream->vtbl->destroy(stream);
}

static void reset_data_stream(http_request_t *req)
{
    destroy_data_stream(req->data_stream);
    req->data_stream = &req->netconn_stream.data_stream;
    req->read_pos = req->read_size = req->netconn_stream.content_read = 0;
    req->read_chunked = req->read_gzip = FALSE;
}

#ifdef HAVE_ZLIB

typedef struct {
    data_stream_t stream;
    data_stream_t *parent_stream;
    z_stream zstream;
    BYTE buf[READ_BUFFER_SIZE];
    DWORD buf_size;
    DWORD buf_pos;
    BOOL end_of_data;
} gzip_stream_t;

static DWORD gzip_get_avail_data(data_stream_t *stream, http_request_t *req)
{
    /* Allow reading only from read buffer */
    return 0;
}

static BOOL gzip_end_of_data(data_stream_t *stream, http_request_t *req)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    return gzip_stream->end_of_data;
}

static DWORD gzip_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, read_mode_t read_mode)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    z_stream *zstream = &gzip_stream->zstream;
    DWORD current_read, ret_read = 0;
    BOOL end;
    int zres;
    DWORD res = ERROR_SUCCESS;

    while(size && !gzip_stream->end_of_data) {
        end = gzip_stream->parent_stream->vtbl->end_of_data(gzip_stream->parent_stream, req);

        if(gzip_stream->buf_size <= 64 && !end) {
            if(gzip_stream->buf_pos) {
                if(gzip_stream->buf_size)
                    memmove(gzip_stream->buf, gzip_stream->buf+gzip_stream->buf_pos, gzip_stream->buf_size);
                gzip_stream->buf_pos = 0;
            }
            res = gzip_stream->parent_stream->vtbl->read(gzip_stream->parent_stream, req, gzip_stream->buf+gzip_stream->buf_size,
                    sizeof(gzip_stream->buf)-gzip_stream->buf_size, &current_read, read_mode);
            gzip_stream->buf_size += current_read;
            if(res != ERROR_SUCCESS)
                break;
            end = gzip_stream->parent_stream->vtbl->end_of_data(gzip_stream->parent_stream, req);
            if(!current_read && !end) {
                if(read_mode != READMODE_NOBLOCK) {
                    WARN("unexpected end of data\n");
                    gzip_stream->end_of_data = TRUE;
                }
                break;
            }
            if(gzip_stream->buf_size <= 64 && !end)
                continue;
        }

        zstream->next_in = gzip_stream->buf+gzip_stream->buf_pos;
        zstream->avail_in = gzip_stream->buf_size-(end ? 0 : 64);
        zstream->next_out = buf+ret_read;
        zstream->avail_out = size;
        zres = inflate(&gzip_stream->zstream, 0);
        current_read = size - zstream->avail_out;
        size -= current_read;
        ret_read += current_read;
        gzip_stream->buf_size -= zstream->next_in - (gzip_stream->buf+gzip_stream->buf_pos);
        gzip_stream->buf_pos = zstream->next_in-gzip_stream->buf;
        if(zres == Z_STREAM_END) {
            TRACE("end of data\n");
            gzip_stream->end_of_data = TRUE;
            inflateEnd(zstream);
        }else if(zres != Z_OK) {
            WARN("inflate failed %d: %s\n", zres, debugstr_a(zstream->msg));
            if(!ret_read)
                res = ERROR_INTERNET_DECODING_FAILED;
            break;
        }

        if(ret_read && read_mode == READMODE_ASYNC)
            read_mode = READMODE_NOBLOCK;
    }

    TRACE("read %u bytes\n", ret_read);
    *read = ret_read;
    return res;
}

static BOOL gzip_drain_content(data_stream_t *stream, http_request_t *req)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;
    return gzip_stream->parent_stream->vtbl->drain_content(gzip_stream->parent_stream, req);
}

static void gzip_destroy(data_stream_t *stream)
{
    gzip_stream_t *gzip_stream = (gzip_stream_t*)stream;

    destroy_data_stream(gzip_stream->parent_stream);

    if(!gzip_stream->end_of_data)
        inflateEnd(&gzip_stream->zstream);
    heap_free(gzip_stream);
}

static const data_stream_vtbl_t gzip_stream_vtbl = {
    gzip_get_avail_data,
    gzip_end_of_data,
    gzip_read,
    gzip_drain_content,
    gzip_destroy
};

static voidpf wininet_zalloc(voidpf opaque, uInt items, uInt size)
{
    return heap_alloc(items*size);
}

static void wininet_zfree(voidpf opaque, voidpf address)
{
    heap_free(address);
}

static DWORD init_gzip_stream(http_request_t *req)
{
    gzip_stream_t *gzip_stream;
    int index, zres;

    gzip_stream = heap_alloc_zero(sizeof(gzip_stream_t));
    if(!gzip_stream)
        return ERROR_OUTOFMEMORY;

    gzip_stream->stream.vtbl = &gzip_stream_vtbl;
    gzip_stream->zstream.zalloc = wininet_zalloc;
    gzip_stream->zstream.zfree = wininet_zfree;

    zres = inflateInit2(&gzip_stream->zstream, 0x1f);
    if(zres != Z_OK) {
        ERR("inflateInit failed: %d\n", zres);
        heap_free(gzip_stream);
        return ERROR_OUTOFMEMORY;
    }

    index = HTTP_GetCustomHeaderIndex(req, szContent_Length, 0, FALSE);
    if(index != -1)
        HTTP_DeleteCustomHeader(req, index);

    if(req->read_size) {
        memcpy(gzip_stream->buf, req->read_buf+req->read_pos, req->read_size);
        gzip_stream->buf_size = req->read_size;
        req->read_pos = req->read_size = 0;
    }

    req->read_gzip = TRUE;
    gzip_stream->parent_stream = req->data_stream;
    req->data_stream = &gzip_stream->stream;
    return ERROR_SUCCESS;
}

#else

static DWORD init_gzip_stream(http_request_t *req)
{
    ERR("gzip stream not supported, missing zlib.\n");
    return ERROR_SUCCESS;
}

#endif

/***********************************************************************
 *           HTTP_Tokenize (internal)
 *
 *  Tokenize a string, allocating memory for the tokens.
 */
static LPWSTR * HTTP_Tokenize(LPCWSTR string, LPCWSTR token_string)
{
    LPWSTR * token_array;
    int tokens = 0;
    int i;
    LPCWSTR next_token;

    if (string)
    {
        /* empty string has no tokens */
        if (*string)
            tokens++;
        /* count tokens */
        for (i = 0; string[i]; i++)
        {
            if (!strncmpW(string+i, token_string, strlenW(token_string)))
            {
                DWORD j;
                tokens++;
                /* we want to skip over separators, but not the null terminator */
                for (j = 0; j < strlenW(token_string) - 1; j++)
                    if (!string[i+j])
                        break;
                i += j;
            }
        }
    }

    /* add 1 for terminating NULL */
    token_array = heap_alloc((tokens+1) * sizeof(*token_array));
    token_array[tokens] = NULL;
    if (!tokens)
        return token_array;
    for (i = 0; i < tokens; i++)
    {
        int len;
        next_token = strstrW(string, token_string);
        if (!next_token) next_token = string+strlenW(string);
        len = next_token - string;
        token_array[i] = heap_alloc((len+1)*sizeof(WCHAR));
        memcpy(token_array[i], string, len*sizeof(WCHAR));
        token_array[i][len] = '\0';
        string = next_token+strlenW(token_string);
    }
    return token_array;
}

/***********************************************************************
 *           HTTP_FreeTokens (internal)
 *
 *  Frees memory returned from HTTP_Tokenize.
 */
static void HTTP_FreeTokens(LPWSTR * token_array)
{
    int i;
    for (i = 0; token_array[i]; i++) heap_free(token_array[i]);
    heap_free(token_array);
}

static void HTTP_FixURL(http_request_t *request)
{
    static const WCHAR szSlash[] = { '/',0 };
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/', 0 };

    /* If we don't have a path we set it to root */
    if (NULL == request->path)
        request->path = heap_strdupW(szSlash);
    else /* remove \r and \n*/
    {
        int nLen = strlenW(request->path);
        while ((nLen >0 ) && ((request->path[nLen-1] == '\r')||(request->path[nLen-1] == '\n')))
        {
            nLen--;
            request->path[nLen]='\0';
        }
        /* Replace '\' with '/' */
        while (nLen>0) {
            nLen--;
            if (request->path[nLen] == '\\') request->path[nLen]='/';
        }
    }

    if(CSTR_EQUAL != CompareStringW( LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       request->path, strlenW(request->path), szHttp, strlenW(szHttp) )
       && request->path[0] != '/') /* not an absolute path ?? --> fix it !! */
    {
        WCHAR *fixurl = heap_alloc((strlenW(request->path) + 2)*sizeof(WCHAR));
        *fixurl = '/';
        strcpyW(fixurl + 1, request->path);
        heap_free( request->path );
        request->path = fixurl;
    }
}

static LPWSTR HTTP_BuildHeaderRequestString( http_request_t *request, LPCWSTR verb, LPCWSTR path, LPCWSTR version )
{
    LPWSTR requestString;
    DWORD len, n;
    LPCWSTR *req;
    UINT i;
    LPWSTR p;

    static const WCHAR szSpace[] = { ' ',0 };
    static const WCHAR szColon[] = { ':',' ',0 };
    static const WCHAR sztwocrlf[] = {'\r','\n','\r','\n', 0};

    /* allocate space for an array of all the string pointers to be added */
    len = (request->nCustHeaders)*4 + 10;
    req = heap_alloc(len*sizeof(LPCWSTR));

    /* add the verb, path and HTTP version string */
    n = 0;
    req[n++] = verb;
    req[n++] = szSpace;
    req[n++] = path;
    req[n++] = szSpace;
    req[n++] = version;

    /* Append custom request headers */
    for (i = 0; i < request->nCustHeaders; i++)
    {
        if (request->custHeaders[i].wFlags & HDR_ISREQUEST)
        {
            req[n++] = szCrLf;
            req[n++] = request->custHeaders[i].lpszField;
            req[n++] = szColon;
            req[n++] = request->custHeaders[i].lpszValue;

            TRACE("Adding custom header %s (%s)\n",
                   debugstr_w(request->custHeaders[i].lpszField),
                   debugstr_w(request->custHeaders[i].lpszValue));
        }
    }

    if( n >= len )
        ERR("oops. buffer overrun\n");

    req[n] = NULL;
    requestString = HTTP_build_req( req, 4 );
    heap_free( req );

    /*
     * Set (header) termination string for request
     * Make sure there's exactly two new lines at the end of the request
     */
    p = &requestString[strlenW(requestString)-1];
    while ( (*p == '\n') || (*p == '\r') )
       p--;
    strcpyW( p+1, sztwocrlf );
    
    return requestString;
}

static void HTTP_ProcessCookies( http_request_t *request )
{
    int HeaderIndex;
    int numCookies = 0;
    LPHTTPHEADERW setCookieHeader;

    if(request->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES)
        return;

    while((HeaderIndex = HTTP_GetCustomHeaderIndex(request, szSet_Cookie, numCookies++, FALSE)) != -1)
    {
        HTTPHEADERW *host;
        const WCHAR *data;
        WCHAR *name;

        setCookieHeader = &request->custHeaders[HeaderIndex];

        if (!setCookieHeader->lpszValue)
            continue;

        host = HTTP_GetHeader(request, hostW);
        if(!host)
            continue;

        data = strchrW(setCookieHeader->lpszValue, '=');
        if(!data)
            continue;

        name = heap_strndupW(setCookieHeader->lpszValue, data-setCookieHeader->lpszValue);
        if(!name)
            continue;

        data++;
        set_cookie(host->lpszValue, request->path, name, data);
        heap_free(name);
    }
}

static void strip_spaces(LPWSTR start)
{
    LPWSTR str = start;
    LPWSTR end;

    while (*str == ' ' && *str != '\0')
        str++;

    if (str != start)
        memmove(start, str, sizeof(WCHAR) * (strlenW(str) + 1));

    end = start + strlenW(start) - 1;
    while (end >= start && *end == ' ')
    {
        *end = '\0';
        end--;
    }
}

static inline BOOL is_basic_auth_value( LPCWSTR pszAuthValue, LPWSTR *pszRealm )
{
    static const WCHAR szBasic[] = {'B','a','s','i','c'}; /* Note: not nul-terminated */
    static const WCHAR szRealm[] = {'r','e','a','l','m'}; /* Note: not nul-terminated */
    BOOL is_basic;
    is_basic = !strncmpiW(pszAuthValue, szBasic, ARRAYSIZE(szBasic)) &&
        ((pszAuthValue[ARRAYSIZE(szBasic)] == ' ') || !pszAuthValue[ARRAYSIZE(szBasic)]);
    if (is_basic && pszRealm)
    {
        LPCWSTR token;
        LPCWSTR ptr = &pszAuthValue[ARRAYSIZE(szBasic)];
        LPCWSTR realm;
        ptr++;
        *pszRealm=NULL;
        token = strchrW(ptr,'=');
        if (!token)
            return TRUE;
        realm = ptr;
        while (*realm == ' ' && *realm != '\0')
            realm++;
        if(!strncmpiW(realm, szRealm, ARRAYSIZE(szRealm)) &&
            (realm[ARRAYSIZE(szRealm)] == ' ' || realm[ARRAYSIZE(szRealm)] == '='))
        {
            token++;
            while (*token == ' ' && *token != '\0')
                token++;
            if (*token == '\0')
                return TRUE;
            *pszRealm = heap_strdupW(token);
            strip_spaces(*pszRealm);
        }
    }

    return is_basic;
}

static void destroy_authinfo( struct HttpAuthInfo *authinfo )
{
    if (!authinfo) return;

    if (SecIsValidHandle(&authinfo->ctx))
        DeleteSecurityContext(&authinfo->ctx);
    if (SecIsValidHandle(&authinfo->cred))
        FreeCredentialsHandle(&authinfo->cred);

    heap_free(authinfo->auth_data);
    heap_free(authinfo->scheme);
    heap_free(authinfo);
}

static UINT retrieve_cached_basic_authorization(LPWSTR host, LPWSTR realm, LPSTR *auth_data)
{
    basicAuthorizationData *ad;
    UINT rc = 0;

    TRACE("Looking for authorization for %s:%s\n",debugstr_w(host),debugstr_w(realm));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &basicAuthorizationCache, basicAuthorizationData, entry)
    {
        if (!strcmpiW(host,ad->host) && !strcmpW(realm,ad->realm))
        {
            TRACE("Authorization found in cache\n");
            *auth_data = heap_alloc(ad->authorizationLen);
            memcpy(*auth_data,ad->authorization,ad->authorizationLen);
            rc = ad->authorizationLen;
            break;
        }
    }
    LeaveCriticalSection(&authcache_cs);
    return rc;
}

static void cache_basic_authorization(LPWSTR host, LPWSTR realm, LPSTR auth_data, UINT auth_data_len)
{
    struct list *cursor;
    basicAuthorizationData* ad = NULL;

    TRACE("caching authorization for %s:%s = %s\n",debugstr_w(host),debugstr_w(realm),debugstr_an(auth_data,auth_data_len));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH(cursor, &basicAuthorizationCache)
    {
        basicAuthorizationData *check = LIST_ENTRY(cursor,basicAuthorizationData,entry);
        if (!strcmpiW(host,check->host) && !strcmpW(realm,check->realm))
        {
            ad = check;
            break;
        }
    }

    if (ad)
    {
        TRACE("Found match in cache, replacing\n");
        heap_free(ad->authorization);
        ad->authorization = heap_alloc(auth_data_len);
        memcpy(ad->authorization, auth_data, auth_data_len);
        ad->authorizationLen = auth_data_len;
    }
    else
    {
        ad = heap_alloc(sizeof(basicAuthorizationData));
        ad->host = heap_strdupW(host);
        ad->realm = heap_strdupW(realm);
        ad->authorization = heap_alloc(auth_data_len);
        memcpy(ad->authorization, auth_data, auth_data_len);
        ad->authorizationLen = auth_data_len;
        list_add_head(&basicAuthorizationCache,&ad->entry);
        TRACE("authorization cached\n");
    }
    LeaveCriticalSection(&authcache_cs);
}

static BOOL retrieve_cached_authorization(LPWSTR host, LPWSTR scheme,
        SEC_WINNT_AUTH_IDENTITY_W *nt_auth_identity)
{
    authorizationData *ad;

    TRACE("Looking for authorization for %s:%s\n", debugstr_w(host), debugstr_w(scheme));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &authorizationCache, authorizationData, entry) {
        if(!strcmpiW(host, ad->host) && !strcmpiW(scheme, ad->scheme)) {
            TRACE("Authorization found in cache\n");

            nt_auth_identity->User = heap_strdupW(ad->user);
            nt_auth_identity->Password = heap_strdupW(ad->password);
            nt_auth_identity->Domain = heap_alloc(sizeof(WCHAR)*ad->domain_len);
            if(!nt_auth_identity->User || !nt_auth_identity->Password ||
                    (!nt_auth_identity->Domain && ad->domain_len)) {
                heap_free(nt_auth_identity->User);
                heap_free(nt_auth_identity->Password);
                heap_free(nt_auth_identity->Domain);
                break;
            }

            nt_auth_identity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            nt_auth_identity->UserLength = ad->user_len;
            nt_auth_identity->PasswordLength = ad->password_len;
            memcpy(nt_auth_identity->Domain, ad->domain, sizeof(WCHAR)*ad->domain_len);
            nt_auth_identity->DomainLength = ad->domain_len;
            LeaveCriticalSection(&authcache_cs);
            return TRUE;
        }
    }
    LeaveCriticalSection(&authcache_cs);

    return FALSE;
}

static void cache_authorization(LPWSTR host, LPWSTR scheme,
        SEC_WINNT_AUTH_IDENTITY_W *nt_auth_identity)
{
    authorizationData *ad;
    BOOL found = FALSE;

    TRACE("Caching authorization for %s:%s\n", debugstr_w(host), debugstr_w(scheme));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &authorizationCache, authorizationData, entry)
        if(!strcmpiW(host, ad->host) && !strcmpiW(scheme, ad->scheme)) {
            found = TRUE;
            break;
        }

    if(found) {
        heap_free(ad->user);
        heap_free(ad->password);
        heap_free(ad->domain);
    } else {
        ad = heap_alloc(sizeof(authorizationData));
        if(!ad) {
            LeaveCriticalSection(&authcache_cs);
            return;
        }

        ad->host = heap_strdupW(host);
        ad->scheme = heap_strdupW(scheme);
        list_add_head(&authorizationCache, &ad->entry);
    }

    ad->user = heap_strndupW(nt_auth_identity->User, nt_auth_identity->UserLength);
    ad->password = heap_strndupW(nt_auth_identity->Password, nt_auth_identity->PasswordLength);
    ad->domain = heap_strndupW(nt_auth_identity->Domain, nt_auth_identity->DomainLength);
    ad->user_len = nt_auth_identity->UserLength;
    ad->password_len = nt_auth_identity->PasswordLength;
    ad->domain_len = nt_auth_identity->DomainLength;

    if(!ad->host || !ad->scheme || !ad->user || !ad->password
            || (nt_auth_identity->Domain && !ad->domain)) {
        heap_free(ad->host);
        heap_free(ad->scheme);
        heap_free(ad->user);
        heap_free(ad->password);
        heap_free(ad->domain);
        list_remove(&ad->entry);
        heap_free(ad);
    }

    LeaveCriticalSection(&authcache_cs);
}

static BOOL HTTP_DoAuthorization( http_request_t *request, LPCWSTR pszAuthValue,
                                  struct HttpAuthInfo **ppAuthInfo,
                                  LPWSTR domain_and_username, LPWSTR password,
                                  LPWSTR host )
{
    SECURITY_STATUS sec_status;
    struct HttpAuthInfo *pAuthInfo = *ppAuthInfo;
    BOOL first = FALSE;
    LPWSTR szRealm = NULL;

    TRACE("%s\n", debugstr_w(pszAuthValue));

    if (!pAuthInfo)
    {
        TimeStamp exp;

        first = TRUE;
        pAuthInfo = heap_alloc(sizeof(*pAuthInfo));
        if (!pAuthInfo)
            return FALSE;

        SecInvalidateHandle(&pAuthInfo->cred);
        SecInvalidateHandle(&pAuthInfo->ctx);
        memset(&pAuthInfo->exp, 0, sizeof(pAuthInfo->exp));
        pAuthInfo->attr = 0;
        pAuthInfo->auth_data = NULL;
        pAuthInfo->auth_data_len = 0;
        pAuthInfo->finished = FALSE;

        if (is_basic_auth_value(pszAuthValue,NULL))
        {
            static const WCHAR szBasic[] = {'B','a','s','i','c',0};
            pAuthInfo->scheme = heap_strdupW(szBasic);
            if (!pAuthInfo->scheme)
            {
                heap_free(pAuthInfo);
                return FALSE;
            }
        }
        else
        {
            PVOID pAuthData;
            SEC_WINNT_AUTH_IDENTITY_W nt_auth_identity;

            pAuthInfo->scheme = heap_strdupW(pszAuthValue);
            if (!pAuthInfo->scheme)
            {
                heap_free(pAuthInfo);
                return FALSE;
            }

            if (domain_and_username)
            {
                WCHAR *user = strchrW(domain_and_username, '\\');
                WCHAR *domain = domain_and_username;

                /* FIXME: make sure scheme accepts SEC_WINNT_AUTH_IDENTITY before calling AcquireCredentialsHandle */

                pAuthData = &nt_auth_identity;

                if (user) user++;
                else
                {
                    user = domain_and_username;
                    domain = NULL;
                }

                nt_auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                nt_auth_identity.User = user;
                nt_auth_identity.UserLength = strlenW(nt_auth_identity.User);
                nt_auth_identity.Domain = domain;
                nt_auth_identity.DomainLength = domain ? user - domain - 1 : 0;
                nt_auth_identity.Password = password;
                nt_auth_identity.PasswordLength = strlenW(nt_auth_identity.Password);

                cache_authorization(host, pAuthInfo->scheme, &nt_auth_identity);
            }
            else if(retrieve_cached_authorization(host, pAuthInfo->scheme, &nt_auth_identity))
                pAuthData = &nt_auth_identity;
            else
                /* use default credentials */
                pAuthData = NULL;

            sec_status = AcquireCredentialsHandleW(NULL, pAuthInfo->scheme,
                                                   SECPKG_CRED_OUTBOUND, NULL,
                                                   pAuthData, NULL,
                                                   NULL, &pAuthInfo->cred,
                                                   &exp);

            if(pAuthData && !domain_and_username) {
                heap_free(nt_auth_identity.User);
                heap_free(nt_auth_identity.Domain);
                heap_free(nt_auth_identity.Password);
            }

            if (sec_status == SEC_E_OK)
            {
                PSecPkgInfoW sec_pkg_info;
                sec_status = QuerySecurityPackageInfoW(pAuthInfo->scheme, &sec_pkg_info);
                if (sec_status == SEC_E_OK)
                {
                    pAuthInfo->max_token = sec_pkg_info->cbMaxToken;
                    FreeContextBuffer(sec_pkg_info);
                }
            }
            if (sec_status != SEC_E_OK)
            {
                WARN("AcquireCredentialsHandleW for scheme %s failed with error 0x%08x\n",
                     debugstr_w(pAuthInfo->scheme), sec_status);
                heap_free(pAuthInfo->scheme);
                heap_free(pAuthInfo);
                return FALSE;
            }
        }
        *ppAuthInfo = pAuthInfo;
    }
    else if (pAuthInfo->finished)
        return FALSE;

    if ((strlenW(pszAuthValue) < strlenW(pAuthInfo->scheme)) ||
        strncmpiW(pszAuthValue, pAuthInfo->scheme, strlenW(pAuthInfo->scheme)))
    {
        ERR("authentication scheme changed from %s to %s\n",
            debugstr_w(pAuthInfo->scheme), debugstr_w(pszAuthValue));
        return FALSE;
    }

    if (is_basic_auth_value(pszAuthValue,&szRealm))
    {
        int userlen;
        int passlen;
        char *auth_data = NULL;
        UINT auth_data_len = 0;

        TRACE("basic authentication realm %s\n",debugstr_w(szRealm));

        if (!domain_and_username)
        {
            if (host && szRealm)
                auth_data_len = retrieve_cached_basic_authorization(host, szRealm,&auth_data);
            if (auth_data_len == 0)
            {
                heap_free(szRealm);
                return FALSE;
            }
        }
        else
        {
            userlen = WideCharToMultiByte(CP_UTF8, 0, domain_and_username, lstrlenW(domain_and_username), NULL, 0, NULL, NULL);
            passlen = WideCharToMultiByte(CP_UTF8, 0, password, lstrlenW(password), NULL, 0, NULL, NULL);

            /* length includes a nul terminator, which will be re-used for the ':' */
            auth_data = heap_alloc(userlen + 1 + passlen);
            if (!auth_data)
            {
                heap_free(szRealm);
                return FALSE;
            }

            WideCharToMultiByte(CP_UTF8, 0, domain_and_username, -1, auth_data, userlen, NULL, NULL);
            auth_data[userlen] = ':';
            WideCharToMultiByte(CP_UTF8, 0, password, -1, &auth_data[userlen+1], passlen, NULL, NULL);
            auth_data_len = userlen + 1 + passlen;
            if (host && szRealm)
                cache_basic_authorization(host, szRealm, auth_data, auth_data_len);
        }

        pAuthInfo->auth_data = auth_data;
        pAuthInfo->auth_data_len = auth_data_len;
        pAuthInfo->finished = TRUE;
        heap_free(szRealm);
        return TRUE;
    }
    else
    {
        LPCWSTR pszAuthData;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        unsigned char *buffer;
        ULONG context_req = ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE |
            ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;

        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer = 0;
        in.pvBuffer = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers = 1;
        in_desc.pBuffers = &in;

        pszAuthData = pszAuthValue + strlenW(pAuthInfo->scheme);
        if (*pszAuthData == ' ')
        {
            pszAuthData++;
            in.cbBuffer = HTTP_DecodeBase64(pszAuthData, NULL);
            in.pvBuffer = heap_alloc(in.cbBuffer);
            HTTP_DecodeBase64(pszAuthData, in.pvBuffer);
        }

        buffer = heap_alloc(pAuthInfo->max_token);

        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer = pAuthInfo->max_token;
        out.pvBuffer = buffer;

        out_desc.ulVersion = 0;
        out_desc.cBuffers = 1;
        out_desc.pBuffers = &out;

        sec_status = InitializeSecurityContextW(first ? &pAuthInfo->cred : NULL,
                                                first ? NULL : &pAuthInfo->ctx,
                                                first ? request->session->serverName : NULL,
                                                context_req, 0, SECURITY_NETWORK_DREP,
                                                in.pvBuffer ? &in_desc : NULL,
                                                0, &pAuthInfo->ctx, &out_desc,
                                                &pAuthInfo->attr, &pAuthInfo->exp);
        if (sec_status == SEC_E_OK)
        {
            pAuthInfo->finished = TRUE;
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending last auth packet\n");
        }
        else if (sec_status == SEC_I_CONTINUE_NEEDED)
        {
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
        }
        else
        {
            ERR("InitializeSecurityContextW returned error 0x%08x\n", sec_status);
            heap_free(out.pvBuffer);
            destroy_authinfo(pAuthInfo);
            *ppAuthInfo = NULL;
            return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpAddRequestHeadersW (internal)
 */
static DWORD HTTP_HttpAddRequestHeadersW(http_request_t *request,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPWSTR lpszStart;
    LPWSTR lpszEnd;
    LPWSTR buffer;
    DWORD len, res = ERROR_HTTP_INVALID_HEADER;

    TRACE("copying header: %s\n", debugstr_wn(lpszHeader, dwHeaderLength));

    if( dwHeaderLength == ~0U )
        len = strlenW(lpszHeader);
    else
        len = dwHeaderLength;
    buffer = heap_alloc(sizeof(WCHAR)*(len+1));
    lstrcpynW( buffer, lpszHeader, len + 1);

    lpszStart = buffer;

    do
    {
        LPWSTR * pFieldAndValue;

        lpszEnd = lpszStart;

        while (*lpszEnd != '\0')
        {
            if (*lpszEnd == '\r' || *lpszEnd == '\n')
                 break;
            lpszEnd++;
        }

        if (*lpszStart == '\0')
	    break;

        if (*lpszEnd == '\r' || *lpszEnd == '\n')
        {
            *lpszEnd = '\0';
            lpszEnd++; /* Jump over newline */
        }
        TRACE("interpreting header %s\n", debugstr_w(lpszStart));
        if (*lpszStart == '\0')
        {
            /* Skip 0-length headers */
            lpszStart = lpszEnd;
            res = ERROR_SUCCESS;
            continue;
        }
        pFieldAndValue = HTTP_InterpretHttpHeader(lpszStart);
        if (pFieldAndValue)
        {
            res = HTTP_VerifyValidHeader(request, pFieldAndValue[0]);
            if (res == ERROR_SUCCESS)
                res = HTTP_ProcessHeader(request, pFieldAndValue[0],
                    pFieldAndValue[1], dwModifier | HTTP_ADDHDR_FLAG_REQ);
            HTTP_FreeTokens(pFieldAndValue);
        }

        lpszStart = lpszEnd;
    } while (res == ERROR_SUCCESS);

    heap_free(buffer);
    return res;
}

/***********************************************************************
 *           HttpAddRequestHeadersW (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * NOTE
 * On Windows if dwHeaderLength includes the trailing '\0', then
 * HttpAddRequestHeadersW() adds it too. However this results in an
 * invalid Http header which is rejected by some servers so we probably
 * don't need to match Windows on that point.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersW(HINTERNET hHttpRequest,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    http_request_t *request;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("%p, %s, %i, %i\n", hHttpRequest, debugstr_wn(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    if (!lpszHeader) 
      return TRUE;

    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (request && request->hdr.htype == WH_HHTTPREQ)
        res = HTTP_HttpAddRequestHeadersW( request, lpszHeader, dwHeaderLength, dwModifier );
    if( request )
        WININET_Release( &request->hdr );

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest,
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    DWORD len;
    LPWSTR hdr;
    BOOL r;

    TRACE("%p, %s, %i, %i\n", hHttpRequest, debugstr_an(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    len = MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, NULL, 0 );
    hdr = heap_alloc(len*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, hdr, len );
    if( dwHeaderLength != ~0U )
        dwHeaderLength = len;

    r = HttpAddRequestHeadersW( hHttpRequest, hdr, dwHeaderLength, dwModifier );

    heap_free( hdr );
    return r;
}

static void free_accept_types( WCHAR **accept_types )
{
    WCHAR *ptr, **types = accept_types;

    if (!types) return;
    while ((ptr = *types))
    {
        heap_free( ptr );
        types++;
    }
    heap_free( accept_types );
}

static WCHAR **convert_accept_types( const char **accept_types )
{
    unsigned int count;
    const char **types = accept_types;
    WCHAR **typesW;
    BOOL invalid_pointer = FALSE;

    if (!types) return NULL;
    count = 0;
    while (*types)
    {
        __TRY
        {
            /* find out how many there are */
            if (*types && **types)
            {
                TRACE("accept type: %s\n", debugstr_a(*types));
                count++;
            }
        }
        __EXCEPT_PAGE_FAULT
        {
            WARN("invalid accept type pointer\n");
            invalid_pointer = TRUE;
        }
        __ENDTRY;
        types++;
    }
    if (invalid_pointer) return NULL;
    if (!(typesW = heap_alloc( sizeof(WCHAR *) * (count + 1) ))) return NULL;
    count = 0;
    types = accept_types;
    while (*types)
    {
        if (*types && **types) typesW[count++] = heap_strdupAtoW( *types );
        types++;
    }
    typesW[count] = NULL;
    return typesW;
}

/***********************************************************************
 *           HttpOpenRequestA (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWSTR szVerb = NULL, szObjectName = NULL;
    LPWSTR szVersion = NULL, szReferrer = NULL, *szAcceptTypes = NULL;
    HINTERNET rc = FALSE;

    TRACE("(%p, %s, %s, %s, %s, %p, %08x, %08lx)\n", hHttpSession,
          debugstr_a(lpszVerb), debugstr_a(lpszObjectName),
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        szVerb = heap_strdupAtoW(lpszVerb);
        if ( !szVerb )
            goto end;
    }

    if (lpszObjectName)
    {
        szObjectName = heap_strdupAtoW(lpszObjectName);
        if ( !szObjectName )
            goto end;
    }

    if (lpszVersion)
    {
        szVersion = heap_strdupAtoW(lpszVersion);
        if ( !szVersion )
            goto end;
    }

    if (lpszReferrer)
    {
        szReferrer = heap_strdupAtoW(lpszReferrer);
        if ( !szReferrer )
            goto end;
    }

    szAcceptTypes = convert_accept_types( lpszAcceptTypes );
    rc = HttpOpenRequestW(hHttpSession, szVerb, szObjectName, szVersion, szReferrer,
                          (const WCHAR **)szAcceptTypes, dwFlags, dwContext);

end:
    free_accept_types(szAcceptTypes);
    heap_free(szReferrer);
    heap_free(szVersion);
    heap_free(szObjectName);
    heap_free(szVerb);
    return rc;
}

/***********************************************************************
 *  HTTP_EncodeBase64
 */
static UINT HTTP_EncodeBase64( LPCSTR bin, unsigned int len, LPWSTR base64 )
{
    UINT n = 0, x;
    static const CHAR HTTP_Base64Enc[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while( len > 0 )
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = HTTP_Base64Enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if( len == 1 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[1]&0xf0) >> 4 ) ];
        x = ( bin[1] & 0x0f ) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if( len == 2 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[2]&0xc0 ) >> 6 ) ];

        /* last 6 bits, all from bin [2] */
        base64[n++] = HTTP_Base64Enc[ bin[2] & 0x3f ];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

#define CH(x) (((x) >= 'A' && (x) <= 'Z') ? (x) - 'A' : \
               ((x) >= 'a' && (x) <= 'z') ? (x) - 'a' + 26 : \
               ((x) >= '0' && (x) <= '9') ? (x) - '0' + 52 : \
               ((x) == '+') ? 62 : ((x) == '/') ? 63 : -1)
static const signed char HTTP_Base64Dec[256] =
{
    CH( 0),CH( 1),CH( 2),CH( 3),CH( 4),CH( 5),CH( 6),CH( 7),CH( 8),CH( 9),
    CH(10),CH(11),CH(12),CH(13),CH(14),CH(15),CH(16),CH(17),CH(18),CH(19),
    CH(20),CH(21),CH(22),CH(23),CH(24),CH(25),CH(26),CH(27),CH(28),CH(29),
    CH(30),CH(31),CH(32),CH(33),CH(34),CH(35),CH(36),CH(37),CH(38),CH(39),
    CH(40),CH(41),CH(42),CH(43),CH(44),CH(45),CH(46),CH(47),CH(48),CH(49),
    CH(50),CH(51),CH(52),CH(53),CH(54),CH(55),CH(56),CH(57),CH(58),CH(59),
    CH(60),CH(61),CH(62),CH(63),CH(64),CH(65),CH(66),CH(67),CH(68),CH(69),
    CH(70),CH(71),CH(72),CH(73),CH(74),CH(75),CH(76),CH(77),CH(78),CH(79),
    CH(80),CH(81),CH(82),CH(83),CH(84),CH(85),CH(86),CH(87),CH(88),CH(89),
    CH(90),CH(91),CH(92),CH(93),CH(94),CH(95),CH(96),CH(97),CH(98),CH(99),
    CH(100),CH(101),CH(102),CH(103),CH(104),CH(105),CH(106),CH(107),CH(108),CH(109),
    CH(110),CH(111),CH(112),CH(113),CH(114),CH(115),CH(116),CH(117),CH(118),CH(119),
    CH(120),CH(121),CH(122),CH(123),CH(124),CH(125),CH(126),CH(127),CH(128),CH(129),
    CH(130),CH(131),CH(132),CH(133),CH(134),CH(135),CH(136),CH(137),CH(138),CH(139),
    CH(140),CH(141),CH(142),CH(143),CH(144),CH(145),CH(146),CH(147),CH(148),CH(149),
    CH(150),CH(151),CH(152),CH(153),CH(154),CH(155),CH(156),CH(157),CH(158),CH(159),
    CH(160),CH(161),CH(162),CH(163),CH(164),CH(165),CH(166),CH(167),CH(168),CH(169),
    CH(170),CH(171),CH(172),CH(173),CH(174),CH(175),CH(176),CH(177),CH(178),CH(179),
    CH(180),CH(181),CH(182),CH(183),CH(184),CH(185),CH(186),CH(187),CH(188),CH(189),
    CH(190),CH(191),CH(192),CH(193),CH(194),CH(195),CH(196),CH(197),CH(198),CH(199),
    CH(200),CH(201),CH(202),CH(203),CH(204),CH(205),CH(206),CH(207),CH(208),CH(209),
    CH(210),CH(211),CH(212),CH(213),CH(214),CH(215),CH(216),CH(217),CH(218),CH(219),
    CH(220),CH(221),CH(222),CH(223),CH(224),CH(225),CH(226),CH(227),CH(228),CH(229),
    CH(230),CH(231),CH(232),CH(233),CH(234),CH(235),CH(236),CH(237),CH(238),CH(239),
    CH(240),CH(241),CH(242),CH(243),CH(244),CH(245),CH(246),CH(247),CH(248), CH(249),
    CH(250),CH(251),CH(252),CH(253),CH(254),CH(255),
};
#undef CH

/***********************************************************************
 *  HTTP_DecodeBase64
 */
static UINT HTTP_DecodeBase64( LPCWSTR base64, LPSTR bin )
{
    unsigned int n = 0;

    while(*base64)
    {
        signed char in[4];

        if (base64[0] >= ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[0] = HTTP_Base64Dec[base64[0]]) == -1) ||
            base64[1] >= ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[1] = HTTP_Base64Dec[base64[1]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(base64));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[0] << 2 | in[1] >> 4);
        n++;

        if ((base64[2] == '=') && (base64[3] == '='))
            break;
        if (base64[2] > ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[2] = HTTP_Base64Dec[base64[2]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[2]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[1] << 4 | in[2] >> 2);
        n++;

        if (base64[3] == '=')
            break;
        if (base64[3] > ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[3] = HTTP_Base64Dec[base64[3]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[3]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (((in[2] << 6) & 0xc0) | in[3]);
        n++;

        base64 += 4;
    }

    return n;
}

/***********************************************************************
 *  HTTP_InsertAuthorization
 *
 *   Insert or delete the authorization field in the request header.
 */
static BOOL HTTP_InsertAuthorization( http_request_t *request, struct HttpAuthInfo *pAuthInfo, LPCWSTR header )
{
    if (pAuthInfo)
    {
        static const WCHAR wszSpace[] = {' ',0};
        static const WCHAR wszBasic[] = {'B','a','s','i','c',0};
        unsigned int len;
        WCHAR *authorization = NULL;

        if (pAuthInfo->auth_data_len)
        {
            /* scheme + space + base64 encoded data (3/2/1 bytes data -> 4 bytes of characters) */
            len = strlenW(pAuthInfo->scheme)+1+((pAuthInfo->auth_data_len+2)*4)/3;
            authorization = heap_alloc((len+1)*sizeof(WCHAR));
            if (!authorization)
                return FALSE;

            strcpyW(authorization, pAuthInfo->scheme);
            strcatW(authorization, wszSpace);
            HTTP_EncodeBase64(pAuthInfo->auth_data,
                              pAuthInfo->auth_data_len,
                              authorization+strlenW(authorization));

            /* clear the data as it isn't valid now that it has been sent to the
             * server, unless it's Basic authentication which doesn't do
             * connection tracking */
            if (strcmpiW(pAuthInfo->scheme, wszBasic))
            {
                heap_free(pAuthInfo->auth_data);
                pAuthInfo->auth_data = NULL;
                pAuthInfo->auth_data_len = 0;
            }
        }

        TRACE("Inserting authorization: %s\n", debugstr_w(authorization));

        HTTP_ProcessHeader(request, header, authorization, HTTP_ADDHDR_FLAG_REQ | HTTP_ADDHDR_FLAG_REPLACE);
        heap_free(authorization);
    }
    return TRUE;
}

static WCHAR *HTTP_BuildProxyRequestUrl(http_request_t *req)
{
    static const WCHAR slash[] = { '/',0 };
    static const WCHAR format[] = { 'h','t','t','p',':','/','/','%','s',':','%','u',0 };
    static const WCHAR formatSSL[] = { 'h','t','t','p','s',':','/','/','%','s',':','%','u',0 };
    http_session_t *session = req->session;
    WCHAR new_location[INTERNET_MAX_URL_LENGTH], *url;
    DWORD size;

    size = sizeof(new_location);
    if (HTTP_HttpQueryInfoW(req, HTTP_QUERY_LOCATION, new_location, &size, NULL) == ERROR_SUCCESS)
    {
        URL_COMPONENTSW UrlComponents;

        if (!(url = heap_alloc(size + sizeof(WCHAR)))) return NULL;
        strcpyW( url, new_location );

        ZeroMemory(&UrlComponents,sizeof(URL_COMPONENTSW));
        if(InternetCrackUrlW(url, 0, 0, &UrlComponents)) goto done;
        heap_free(url);
    }

    size = 16; /* "https://" + sizeof(port#) + ":/\0" */
    size += strlenW( session->hostName ) + strlenW( req->path );

    if (!(url = heap_alloc(size * sizeof(WCHAR)))) return NULL;

    if (req->hdr.dwFlags & INTERNET_FLAG_SECURE)
        sprintfW( url, formatSSL, session->hostName, session->hostPort );
    else
        sprintfW( url, format, session->hostName, session->hostPort );
    if (req->path[0] != '/') strcatW( url, slash );
    strcatW( url, req->path );

done:
    TRACE("url=%s\n", debugstr_w(url));
    return url;
}

/***********************************************************************
 *           HTTP_DealWithProxy
 */
static BOOL HTTP_DealWithProxy(appinfo_t *hIC, http_session_t *session, http_request_t *request)
{
    WCHAR buf[INTERNET_MAX_HOST_NAME_LENGTH];
    WCHAR protoProxy[INTERNET_MAX_URL_LENGTH];
    DWORD protoProxyLen = INTERNET_MAX_URL_LENGTH;
    WCHAR proxy[INTERNET_MAX_URL_LENGTH];
    static WCHAR szNul[] = { 0 };
    URL_COMPONENTSW UrlComponents;
    static const WCHAR protoHttp[] = { 'h','t','t','p',0 };
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/',0 };
    static const WCHAR szFormat[] = { 'h','t','t','p',':','/','/','%','s',0 };

    memset( &UrlComponents, 0, sizeof UrlComponents );
    UrlComponents.dwStructSize = sizeof UrlComponents;
    UrlComponents.lpszHostName = buf;
    UrlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

    if (!INTERNET_FindProxyForProtocol(hIC->proxy, protoHttp, protoProxy, &protoProxyLen))
        return FALSE;
    if( CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                 protoProxy,strlenW(szHttp),szHttp,strlenW(szHttp)) )
        sprintfW(proxy, szFormat, protoProxy);
    else
	strcpyW(proxy, protoProxy);
    if( !InternetCrackUrlW(proxy, 0, 0, &UrlComponents) )
        return FALSE;
    if( UrlComponents.dwHostNameLength == 0 )
        return FALSE;

    if( !request->path )
        request->path = szNul;

    if(UrlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        UrlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

    heap_free(session->serverName);
    session->serverName = heap_strdupW(UrlComponents.lpszHostName);
    session->serverPort = UrlComponents.nPort;

    TRACE("proxy server=%s port=%d\n", debugstr_w(session->serverName), session->serverPort);
    return TRUE;
}

static DWORD HTTP_ResolveName(http_request_t *request, server_t *server)
{
    socklen_t addr_len;
    const void *addr;

    if(server->addr_len)
        return ERROR_SUCCESS;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_RESOLVING_NAME,
                          server->name,
                          (strlenW(server->name)+1) * sizeof(WCHAR));

    addr_len = sizeof(server->addr);
    if (!GetAddress(server->name, server->port, (struct sockaddr *)&server->addr, &addr_len))
        return ERROR_INTERNET_NAME_NOT_RESOLVED;

    switch(server->addr.ss_family) {
    case AF_INET:
        addr = &((struct sockaddr_in *)&server->addr)->sin_addr;
        break;
    case AF_INET6:
        addr = &((struct sockaddr_in6 *)&server->addr)->sin6_addr;
        break;
    default:
        WARN("unsupported family %d\n", server->addr.ss_family);
        return ERROR_INTERNET_NAME_NOT_RESOLVED;
    }

    server->addr_len = addr_len;
    inet_ntop(server->addr.ss_family, addr, server->addr_str, sizeof(server->addr_str));
    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_NAME_RESOLVED,
                          server->addr_str, strlen(server->addr_str)+1);

    TRACE("resolved %s to %s\n", debugstr_w(server->name), server->addr_str);
    return ERROR_SUCCESS;
}

static BOOL HTTP_GetRequestURL(http_request_t *req, LPWSTR buf)
{
    static const WCHAR http[] = { 'h','t','t','p',':','/','/',0 };
    static const WCHAR https[] = { 'h','t','t','p','s',':','/','/',0 };
    static const WCHAR slash[] = { '/',0 };
    LPHTTPHEADERW host_header;
    LPCWSTR scheme;

    host_header = HTTP_GetHeader(req, hostW);
    if(!host_header)
        return FALSE;

    if (req->hdr.dwFlags & INTERNET_FLAG_SECURE)
        scheme = https;
    else
        scheme = http;
    strcpyW(buf, scheme);
    strcatW(buf, host_header->lpszValue);
    if (req->path[0] != '/')
        strcatW(buf, slash);
    strcatW(buf, req->path);
    return TRUE;
}


/***********************************************************************
 *           HTTPREQ_Destroy (internal)
 *
 * Deallocate request handle
 *
 */
static void HTTPREQ_Destroy(object_header_t *hdr)
{
    http_request_t *request = (http_request_t*) hdr;
    DWORD i;

    TRACE("\n");

    if(request->hCacheFile) {
        WCHAR url[INTERNET_MAX_URL_LENGTH];

        CloseHandle(request->hCacheFile);

        if(HTTP_GetRequestURL(request, url)) {
            DWORD headersLen;

            headersLen = request->rawHeaders ? strlenW(request->rawHeaders) : 0;
            CommitUrlCacheEntryW(url, request->cacheFile, request->expires,
                    request->last_modified, NORMAL_CACHE_ENTRY,
                    request->rawHeaders, headersLen, NULL, 0);
        }
    }
    heap_free(request->cacheFile);

    request->read_section.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &request->read_section );
    WININET_Release(&request->session->hdr);

    destroy_authinfo(request->authInfo);
    destroy_authinfo(request->proxyAuthInfo);

    heap_free(request->path);
    heap_free(request->verb);
    heap_free(request->rawHeaders);
    heap_free(request->version);
    heap_free(request->statusText);

    for (i = 0; i < request->nCustHeaders; i++)
    {
        heap_free(request->custHeaders[i].lpszField);
        heap_free(request->custHeaders[i].lpszValue);
    }
    destroy_data_stream(request->data_stream);
    heap_free(request->custHeaders);
}

static void http_release_netconn(http_request_t *req, BOOL reuse)
{
    TRACE("%p %p\n",req, req->netconn);

    if(!req->netconn)
        return;

#ifndef __REACTOS__
    if(reuse && req->netconn->keep_alive) {
        BOOL run_collector;

        EnterCriticalSection(&connection_pool_cs);

        list_add_head(&req->netconn->server->conn_pool, &req->netconn->pool_entry);
        req->netconn->keep_until = (DWORD64)GetTickCount() + COLLECT_TIME;
        req->netconn = NULL;

        run_collector = !collector_running;
        collector_running = TRUE;

        LeaveCriticalSection(&connection_pool_cs);

        if(run_collector) {
            HANDLE thread = NULL;
            HMODULE module;

            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const WCHAR*)WININET_hModule, &module);
            if(module)
                thread = CreateThread(NULL, 0, collect_connections_proc, NULL, 0, NULL);
            if(!thread) {
                EnterCriticalSection(&connection_pool_cs);
                collector_running = FALSE;
                LeaveCriticalSection(&connection_pool_cs);

                if(module)
                    FreeLibrary(module);
            }
            else
                CloseHandle(thread);
        }
        return;
    }
#else
    // silence unused function warning
    (void)collect_connections_proc;
#endif

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_CLOSING_CONNECTION, 0, 0);

    free_netconn(req->netconn);
    req->netconn = NULL;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_CONNECTION_CLOSED, 0, 0);
}

static void drain_content(http_request_t *req)
{
    BOOL try_reuse;

    if (!req->netconn) return;

    if (req->contentLength == -1)
        try_reuse = FALSE;
    else if(!strcmpW(req->verb, szHEAD))
        try_reuse = TRUE;
    else
        try_reuse = req->data_stream->vtbl->drain_content(req->data_stream, req);

    http_release_netconn(req, try_reuse);
}

static BOOL HTTP_KeepAlive(http_request_t *request)
{
    WCHAR szVersion[10];
    WCHAR szConnectionResponse[20];
    DWORD dwBufferSize = sizeof(szVersion);
    BOOL keepalive = FALSE;

    /* as per RFC 2068, S8.1.2.1, if the client is HTTP/1.1 then assume that
     * the connection is keep-alive by default */
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_VERSION, szVersion, &dwBufferSize, NULL) == ERROR_SUCCESS
        && !strcmpiW(szVersion, g_szHttp1_1))
    {
        keepalive = TRUE;
    }

    dwBufferSize = sizeof(szConnectionResponse);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_PROXY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) == ERROR_SUCCESS
        || HTTP_HttpQueryInfoW(request, HTTP_QUERY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) == ERROR_SUCCESS)
    {
        keepalive = !strcmpiW(szConnectionResponse, szKeepAlive);
    }

    return keepalive;
}

static void HTTPREQ_CloseConnection(object_header_t *hdr)
{
    http_request_t *req = (http_request_t*)hdr;

    drain_content(req);
}

static DWORD HTTPREQ_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO:
    {
        http_session_t *session = req->session;
        INTERNET_DIAGNOSTIC_SOCKET_INFO *info = buffer;

        FIXME("INTERNET_DIAGNOSTIC_SOCKET_INFO stub\n");

        if (*size < sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO))
            return ERROR_INSUFFICIENT_BUFFER;
        *size = sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO);
        /* FIXME: can't get a SOCKET from our connection since we don't use
         * winsock
         */
        info->Socket = 0;
        /* FIXME: get source port from req->netConnection */
        info->SourcePort = 0;
        info->DestPort = session->hostPort;
        info->Flags = 0;
        if (HTTP_KeepAlive(req))
            info->Flags |= IDSI_FLAG_KEEP_ALIVE;
        if (session->appInfo->proxy && session->appInfo->proxy[0] != 0)
            info->Flags |= IDSI_FLAG_PROXY;
        if (req->netconn->useSSL)
            info->Flags |= IDSI_FLAG_SECURE;

        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        flags = 0;
        if (req->hdr.dwFlags & INTERNET_FLAG_SECURE)
            flags |= SECURITY_FLAG_SECURE;
        flags |= req->security_flags;
        if(req->netconn) {
            int bits = NETCON_GetCipherStrength(req->netconn);
            if (bits >= 128)
                flags |= SECURITY_FLAG_STRENGTH_STRONG;
            else if (bits >= 56)
                flags |= SECURITY_FLAG_STRENGTH_MEDIUM;
            else
                flags |= SECURITY_FLAG_STRENGTH_WEAK;
        }
        *(DWORD *)buffer = flags;
        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_HTTP_REQUEST;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_URL: {
        WCHAR url[INTERNET_MAX_URL_LENGTH];
        HTTPHEADERW *host;
        DWORD len;
        WCHAR *pch;

        static const WCHAR httpW[] = {'h','t','t','p',':','/','/',0};

        TRACE("INTERNET_OPTION_URL\n");

        host = HTTP_GetHeader(req, hostW);
        strcpyW(url, httpW);
        strcatW(url, host->lpszValue);
        if (NULL != (pch = strchrW(url + strlenW(httpW), ':')))
            *pch = 0;
        strcatW(url, req->path);

        TRACE("INTERNET_OPTION_URL: %s\n",debugstr_w(url));

        if(unicode) {
            len = (strlenW(url)+1) * sizeof(WCHAR);
            if(*size < len)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = len;
            strcpyW(buffer, url);
            return ERROR_SUCCESS;
        }else {
            len = WideCharToMultiByte(CP_ACP, 0, url, -1, buffer, *size, NULL, NULL);
            if(len > *size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = len;
            return ERROR_SUCCESS;
        }
    }

    case INTERNET_OPTION_CACHE_TIMESTAMPS: {
        INTERNET_CACHE_ENTRY_INFOW *info;
        INTERNET_CACHE_TIMESTAMPS *ts = buffer;
        WCHAR url[INTERNET_MAX_URL_LENGTH];
        DWORD nbytes, error;
        BOOL ret;

        TRACE("INTERNET_OPTION_CACHE_TIMESTAMPS\n");

        if (*size < sizeof(*ts))
        {
            *size = sizeof(*ts);
            return ERROR_INSUFFICIENT_BUFFER;
        }
        nbytes = 0;
        HTTP_GetRequestURL(req, url);
        ret = GetUrlCacheEntryInfoW(url, NULL, &nbytes);
        error = GetLastError();
        if (!ret && error == ERROR_INSUFFICIENT_BUFFER)
        {
            if (!(info = heap_alloc(nbytes)))
                return ERROR_OUTOFMEMORY;

            GetUrlCacheEntryInfoW(url, info, &nbytes);

            ts->ftExpires = info->ExpireTime;
            ts->ftLastModified = info->LastModifiedTime;

            heap_free(info);
            *size = sizeof(*ts);
            return ERROR_SUCCESS;
        }
        return error;
    }

    case INTERNET_OPTION_DATAFILE_NAME: {
        DWORD req_size;

        TRACE("INTERNET_OPTION_DATAFILE_NAME\n");

        if(!req->cacheFile) {
            *size = 0;
            return ERROR_INTERNET_ITEM_NOT_FOUND;
        }

        if(unicode) {
            req_size = (lstrlenW(req->cacheFile)+1) * sizeof(WCHAR);
            if(*size < req_size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = req_size;
            memcpy(buffer, req->cacheFile, *size);
            return ERROR_SUCCESS;
        }else {
            req_size = WideCharToMultiByte(CP_ACP, 0, req->cacheFile, -1, NULL, 0, NULL, NULL);
            if (req_size > *size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = WideCharToMultiByte(CP_ACP, 0, req->cacheFile,
                    -1, buffer, *size, NULL, NULL);
            return ERROR_SUCCESS;
        }
    }

    case INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT: {
        PCCERT_CONTEXT context;

        if(*size < sizeof(INTERNET_CERTIFICATE_INFOA)) {
            *size = sizeof(INTERNET_CERTIFICATE_INFOA);
            return ERROR_INSUFFICIENT_BUFFER;
        }

        context = (PCCERT_CONTEXT)NETCON_GetCert(req->netconn);
        if(context) {
            INTERNET_CERTIFICATE_INFOA *info = (INTERNET_CERTIFICATE_INFOA*)buffer;
            DWORD len;

            memset(info, 0, sizeof(*info));
            info->ftExpiry = context->pCertInfo->NotAfter;
            info->ftStart = context->pCertInfo->NotBefore;
            len = CertNameToStrA(context->dwCertEncodingType,
                     &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR, NULL, 0);
            info->lpszSubjectInfo = LocalAlloc(0, len);
            if(info->lpszSubjectInfo)
                CertNameToStrA(context->dwCertEncodingType,
                         &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR,
                         info->lpszSubjectInfo, len);
            len = CertNameToStrA(context->dwCertEncodingType,
                     &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR, NULL, 0);
            info->lpszIssuerInfo = LocalAlloc(0, len);
            if(info->lpszIssuerInfo)
                CertNameToStrA(context->dwCertEncodingType,
                         &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR,
                         info->lpszIssuerInfo, len);
            info->dwKeySize = NETCON_GetCipherStrength(req->netconn);
            CertFreeCertificateContext(context);
            return ERROR_SUCCESS;
        }
        return ERROR_NOT_SUPPORTED;
    }
    case INTERNET_OPTION_CONNECT_TIMEOUT:
        if (*size < sizeof(DWORD))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD *)buffer = req->connect_timeout;
        return ERROR_SUCCESS;
    }

    return INET_QueryOption(hdr, option, buffer, size, unicode);
}

static DWORD HTTPREQ_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_SECURITY_FLAGS:
    {
        DWORD flags;

        if (!buffer || size != sizeof(DWORD))
            return ERROR_INVALID_PARAMETER;
        flags = *(DWORD *)buffer;
        TRACE("%08x\n", flags);
        req->security_flags = flags;
        if(req->netconn)
            req->netconn->security_flags = flags;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_CONNECT_TIMEOUT:
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        req->connect_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_SEND_TIMEOUT:
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        req->send_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_RECEIVE_TIMEOUT:
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        req->receive_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_USERNAME:
        heap_free(req->session->userName);
        if (!(req->session->userName = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_PASSWORD:
        heap_free(req->session->password);
        if (!(req->session->password = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    case INTERNET_OPTION_HTTP_DECODING:
        if(size != sizeof(BOOL))
            return ERROR_INVALID_PARAMETER;
        req->decoding = *(BOOL*)buffer;
        return ERROR_SUCCESS;
    }

    return INET_SetOption(hdr, option, buffer, size);
}

/* read some more data into the read buffer (the read section must be held) */
static DWORD read_more_data( http_request_t *req, int maxlen )
{
    DWORD res;
    int len;

    if (req->read_pos)
    {
        /* move existing data to the start of the buffer */
        if(req->read_size)
            memmove( req->read_buf, req->read_buf + req->read_pos, req->read_size );
        req->read_pos = 0;
    }

    if (maxlen == -1) maxlen = sizeof(req->read_buf);

    res = NETCON_recv( req->netconn, req->read_buf + req->read_size,
                       maxlen - req->read_size, 0, &len );
    if(res == ERROR_SUCCESS)
        req->read_size += len;

    return res;
}

/* remove some amount of data from the read buffer (the read section must be held) */
static void remove_data( http_request_t *req, int count )
{
    if (!(req->read_size -= count)) req->read_pos = 0;
    else req->read_pos += count;
}

static BOOL read_line( http_request_t *req, LPSTR buffer, DWORD *len )
{
    int count, bytes_read, pos = 0;
    DWORD res;

    EnterCriticalSection( &req->read_section );
    for (;;)
    {
        BYTE *eol = memchr( req->read_buf + req->read_pos, '\n', req->read_size );

        if (eol)
        {
            count = eol - (req->read_buf + req->read_pos);
            bytes_read = count + 1;
        }
        else count = bytes_read = req->read_size;

        count = min( count, *len - pos );
        memcpy( buffer + pos, req->read_buf + req->read_pos, count );
        pos += count;
        remove_data( req, bytes_read );
        if (eol) break;

        if ((res = read_more_data( req, -1 )) != ERROR_SUCCESS || !req->read_size)
        {
            *len = 0;
            TRACE( "returning empty string %u\n", res);
            LeaveCriticalSection( &req->read_section );
            INTERNET_SetLastError(res);
            return FALSE;
        }
    }
    LeaveCriticalSection( &req->read_section );

    if (pos < *len)
    {
        if (pos && buffer[pos - 1] == '\r') pos--;
        *len = pos + 1;
    }
    buffer[*len - 1] = 0;
    TRACE( "returning %s\n", debugstr_a(buffer));
    return TRUE;
}

/* check if we have reached the end of the data to read (the read section must be held) */
static BOOL end_of_read_data( http_request_t *req )
{
    return !req->read_size && req->data_stream->vtbl->end_of_data(req->data_stream, req);
}

/* fetch some more data into the read buffer (the read section must be held) */
static DWORD refill_read_buffer(http_request_t *req, read_mode_t read_mode, DWORD *read_bytes)
{
    DWORD res, read=0, want;

    if(req->read_size == sizeof(req->read_buf))
        return ERROR_SUCCESS;

    if(req->read_pos) {
        if(req->read_size)
            memmove(req->read_buf, req->read_buf+req->read_pos, req->read_size);
        req->read_pos = 0;
    }

    want = sizeof(req->read_buf) - req->read_size;
    res = req->data_stream->vtbl->read(req->data_stream, req, req->read_buf+req->read_size,
            want, &read, read_mode);
    assert(read <= want);
    req->read_size += read;

    TRACE("read %u bytes, read_size %u\n", read, req->read_size);
    if(read_bytes)
        *read_bytes = read;
    return res;
}

/* return the size of data available to be read immediately (the read section must be held) */
static DWORD get_avail_data( http_request_t *req )
{
    return req->read_size + req->data_stream->vtbl->get_avail_data(req->data_stream, req);
}

static DWORD netconn_get_avail_data(data_stream_t *stream, http_request_t *req)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    DWORD avail = 0;

    if(req->netconn)
        NETCON_query_data_available(req->netconn, &avail);
    return netconn_stream->content_length == ~0u
        ? avail
        : min(avail, netconn_stream->content_length-netconn_stream->content_read);
}

static BOOL netconn_end_of_data(data_stream_t *stream, http_request_t *req)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    return netconn_stream->content_read == netconn_stream->content_length || !req->netconn;
}

static DWORD netconn_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, read_mode_t read_mode)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    int len = 0;

    size = min(size, netconn_stream->content_length-netconn_stream->content_read);

    if(read_mode == READMODE_NOBLOCK) {
        DWORD avail = netconn_get_avail_data(stream, req);
        if (size > avail)
            size = avail;
    }

    if(size && req->netconn) {
        if(NETCON_recv(req->netconn, buf, size, read_mode == READMODE_SYNC ? MSG_WAITALL : 0, &len) != ERROR_SUCCESS)
            len = 0;
        if(!len)
            netconn_stream->content_length = netconn_stream->content_read;
    }

    netconn_stream->content_read += *read = len;
    TRACE("read %u bytes\n", len);
    return ERROR_SUCCESS;
}

static BOOL netconn_drain_content(data_stream_t *stream, http_request_t *req)
{
    netconn_stream_t *netconn_stream = (netconn_stream_t*)stream;
    BYTE buf[1024];
    DWORD avail;
    int len;

    if(netconn_end_of_data(stream, req))
        return TRUE;

    do {
        avail = netconn_get_avail_data(stream, req);
        if(!avail)
            return FALSE;

        if(NETCON_recv(req->netconn, buf, min(avail, sizeof(buf)), 0, &len) != ERROR_SUCCESS)
            return FALSE;

        netconn_stream->content_read += len;
    }while(netconn_stream->content_read < netconn_stream->content_length);

    return TRUE;
}

static void netconn_destroy(data_stream_t *stream)
{
}

static const data_stream_vtbl_t netconn_stream_vtbl = {
    netconn_get_avail_data,
    netconn_end_of_data,
    netconn_read,
    netconn_drain_content,
    netconn_destroy
};

/* read some more data into the read buffer (the read section must be held) */
static DWORD read_more_chunked_data(chunked_stream_t *stream, http_request_t *req, int maxlen)
{
    DWORD res;
    int len;

    if (stream->buf_pos)
    {
        /* move existing data to the start of the buffer */
        if(stream->buf_size)
            memmove(stream->buf, stream->buf + stream->buf_pos, stream->buf_size);
        stream->buf_pos = 0;
    }

    if (maxlen == -1) maxlen = sizeof(stream->buf);

    res = NETCON_recv( req->netconn, stream->buf + stream->buf_size,
                       maxlen - stream->buf_size, 0, &len );
    if(res == ERROR_SUCCESS)
        stream->buf_size += len;

    return res;
}

/* remove some amount of data from the read buffer (the read section must be held) */
static void remove_chunked_data(chunked_stream_t *stream, int count)
{
    if (!(stream->buf_size -= count)) stream->buf_pos = 0;
    else stream->buf_pos += count;
}

/* discard data contents until we reach end of line (the read section must be held) */
static DWORD discard_chunked_eol(chunked_stream_t *stream, http_request_t *req)
{
    DWORD res;

    do
    {
        BYTE *eol = memchr(stream->buf + stream->buf_pos, '\n', stream->buf_size);
        if (eol)
        {
            remove_chunked_data(stream, (eol + 1) - (stream->buf + stream->buf_pos));
            break;
        }
        stream->buf_pos = stream->buf_size = 0;  /* discard everything */
        if ((res = read_more_chunked_data(stream, req, -1)) != ERROR_SUCCESS) return res;
    } while (stream->buf_size);
    return ERROR_SUCCESS;
}

/* read the size of the next chunk (the read section must be held) */
static DWORD start_next_chunk(chunked_stream_t *stream, http_request_t *req)
{
    /* TODOO */
    DWORD chunk_size = 0, res;

    if(stream->chunk_size != ~0u && (res = discard_chunked_eol(stream, req)) != ERROR_SUCCESS)
        return res;

    for (;;)
    {
        while (stream->buf_size)
        {
            char ch = stream->buf[stream->buf_pos];
            if (ch >= '0' && ch <= '9') chunk_size = chunk_size * 16 + ch - '0';
            else if (ch >= 'a' && ch <= 'f') chunk_size = chunk_size * 16 + ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') chunk_size = chunk_size * 16 + ch - 'A' + 10;
            else if (ch == ';' || ch == '\r' || ch == '\n')
            {
                TRACE( "reading %u byte chunk\n", chunk_size );
                stream->chunk_size = chunk_size;
                req->contentLength += chunk_size;
                return discard_chunked_eol(stream, req);
            }
            remove_chunked_data(stream, 1);
        }
        if ((res = read_more_chunked_data(stream, req, -1)) != ERROR_SUCCESS) return res;
        if (!stream->buf_size)
        {
            stream->chunk_size = 0;
            return ERROR_SUCCESS;
        }
    }
}

static DWORD chunked_get_avail_data(data_stream_t *stream, http_request_t *req)
{
    /* Allow reading only from read buffer */
    return 0;
}

static BOOL chunked_end_of_data(data_stream_t *stream, http_request_t *req)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    return !chunked_stream->chunk_size;
}

static DWORD chunked_read(data_stream_t *stream, http_request_t *req, BYTE *buf, DWORD size,
        DWORD *read, read_mode_t read_mode)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    DWORD read_bytes = 0, ret_read = 0, res = ERROR_SUCCESS;

    if(chunked_stream->chunk_size == ~0u) {
        res = start_next_chunk(chunked_stream, req);
        if(res != ERROR_SUCCESS)
            return res;
    }

    while(size && chunked_stream->chunk_size) {
        if(chunked_stream->buf_size) {
            read_bytes = min(size, min(chunked_stream->buf_size, chunked_stream->chunk_size));

            /* this could block */
            if(read_mode == READMODE_NOBLOCK && read_bytes == chunked_stream->chunk_size)
                break;

            memcpy(buf+ret_read, chunked_stream->buf+chunked_stream->buf_pos, read_bytes);
            remove_chunked_data(chunked_stream, read_bytes);
        }else {
            read_bytes = min(size, chunked_stream->chunk_size);

            if(read_mode == READMODE_NOBLOCK) {
                DWORD avail;

                if(!NETCON_query_data_available(req->netconn, &avail) || !avail)
                    break;
                if(read_bytes > avail)
                    read_bytes = avail;

                /* this could block */
                if(read_bytes == chunked_stream->chunk_size)
                    break;
            }

            res = NETCON_recv(req->netconn, (char *)buf+ret_read, read_bytes, 0, (int*)&read_bytes);
            if(res != ERROR_SUCCESS)
                break;
        }

        chunked_stream->chunk_size -= read_bytes;
        size -= read_bytes;
        ret_read += read_bytes;
        if(!chunked_stream->chunk_size) {
            assert(read_mode != READMODE_NOBLOCK);
            res = start_next_chunk(chunked_stream, req);
            if(res != ERROR_SUCCESS)
                break;
        }

        if(read_mode == READMODE_ASYNC)
            read_mode = READMODE_NOBLOCK;
    }

    TRACE("read %u bytes\n", ret_read);
    *read = ret_read;
    return res;
}

static BOOL chunked_drain_content(data_stream_t *stream, http_request_t *req)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;

    /* FIXME: we can do better */
    return !chunked_stream->chunk_size;
}

static void chunked_destroy(data_stream_t *stream)
{
    chunked_stream_t *chunked_stream = (chunked_stream_t*)stream;
    heap_free(chunked_stream);
}

static const data_stream_vtbl_t chunked_stream_vtbl = {
    chunked_get_avail_data,
    chunked_end_of_data,
    chunked_read,
    chunked_drain_content,
    chunked_destroy
};

/* set the request content length based on the headers */
static DWORD set_content_length(http_request_t *request)
{
    static const WCHAR szChunked[] = {'c','h','u','n','k','e','d',0};
    WCHAR encoding[20];
    DWORD size;

    if(request->status_code == HTTP_STATUS_NO_CONTENT) {
        request->contentLength = request->netconn_stream.content_length = 0;
        return ERROR_SUCCESS;
    }

    size = sizeof(request->contentLength);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,
                            &request->contentLength, &size, NULL) != ERROR_SUCCESS)
        request->contentLength = ~0u;
    request->netconn_stream.content_length = request->contentLength;
    request->netconn_stream.content_read = request->read_size;

    size = sizeof(encoding);
    if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_TRANSFER_ENCODING, encoding, &size, NULL) == ERROR_SUCCESS &&
        !strcmpiW(encoding, szChunked))
    {
        chunked_stream_t *chunked_stream;

        chunked_stream = heap_alloc(sizeof(*chunked_stream));
        if(!chunked_stream)
            return ERROR_OUTOFMEMORY;

        chunked_stream->data_stream.vtbl = &chunked_stream_vtbl;
        chunked_stream->buf_size = chunked_stream->buf_pos = 0;
        chunked_stream->chunk_size = ~0u;

        if(request->read_size) {
            memcpy(chunked_stream->buf, request->read_buf+request->read_pos, request->read_size);
            chunked_stream->buf_size = request->read_size;
            request->read_size = request->read_pos = 0;
        }

        request->data_stream = &chunked_stream->data_stream;
        request->contentLength = ~0u;
        request->read_chunked = TRUE;
    }

    if(request->decoding) {
        int encoding_idx;

        static const WCHAR gzipW[] = {'g','z','i','p',0};

        encoding_idx = HTTP_GetCustomHeaderIndex(request, szContent_Encoding, 0, FALSE);
        if(encoding_idx != -1 && !strcmpiW(request->custHeaders[encoding_idx].lpszValue, gzipW))
            return init_gzip_stream(request);
    }

    return ERROR_SUCCESS;
}

static void send_request_complete(http_request_t *req, DWORD_PTR result, DWORD error)
{
    INTERNET_ASYNC_RESULT iar;

    iar.dwResult = result;
    iar.dwError = error;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_REQUEST_COMPLETE, &iar,
            sizeof(INTERNET_ASYNC_RESULT));
}

static void HTTP_ReceiveRequestData(http_request_t *req, BOOL first_notif)
{
    DWORD res, read = 0, avail = 0;
    read_mode_t mode;

    TRACE("%p\n", req);

    EnterCriticalSection( &req->read_section );

    mode = first_notif && req->read_size ? READMODE_NOBLOCK : READMODE_ASYNC;
    res = refill_read_buffer(req, mode, &read);
    if(res == ERROR_SUCCESS && !first_notif)
        avail = get_avail_data(req);

    LeaveCriticalSection( &req->read_section );

    if(res != ERROR_SUCCESS || (mode != READMODE_NOBLOCK && !read)) {
        WARN("res %u read %u, closing connection\n", res, read);
        http_release_netconn(req, FALSE);
    }

    if(res == ERROR_SUCCESS)
        send_request_complete(req, req->session->hdr.dwInternalFlags & INET_OPENURL ? (DWORD_PTR)req->hdr.hInternet : 1, avail);
    else
        send_request_complete(req, 0, res);
}

/* read data from the http connection (the read section must be held) */
static DWORD HTTPREQ_Read(http_request_t *req, void *buffer, DWORD size, DWORD *read, BOOL sync)
{
    DWORD current_read = 0, ret_read = 0;
    read_mode_t read_mode;
    DWORD res = ERROR_SUCCESS;

    read_mode = req->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC ? READMODE_ASYNC : READMODE_SYNC;

    EnterCriticalSection( &req->read_section );

    if(req->read_size) {
        ret_read = min(size, req->read_size);
        memcpy(buffer, req->read_buf+req->read_pos, ret_read);
        req->read_size -= ret_read;
        req->read_pos += ret_read;
        if(read_mode == READMODE_ASYNC)
            read_mode = READMODE_NOBLOCK;
    }

    if(ret_read < size) {
        res = req->data_stream->vtbl->read(req->data_stream, req, (BYTE*)buffer+ret_read, size-ret_read, &current_read, read_mode);
        ret_read += current_read;
    }

    LeaveCriticalSection( &req->read_section );

    *read = ret_read;
    TRACE( "retrieved %u bytes (%u)\n", ret_read, req->contentLength );

    if(req->hCacheFile && res == ERROR_SUCCESS && ret_read) {
        BOOL res;
        DWORD written;

        res = WriteFile(req->hCacheFile, buffer, ret_read, &written, NULL);
        if(!res)
            WARN("WriteFile failed: %u\n", GetLastError());
    }

    if(size && !ret_read)
        http_release_netconn(req, res == ERROR_SUCCESS);

    return res;
}


static DWORD HTTPREQ_ReadFile(object_header_t *hdr, void *buffer, DWORD size, DWORD *read)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res;

    EnterCriticalSection( &req->read_section );
    if(hdr->dwError == INTERNET_HANDLE_IN_USE)
        hdr->dwError = ERROR_INTERNET_INTERNAL_ERROR;

    res = HTTPREQ_Read(req, buffer, size, read, TRUE);
    if(res == ERROR_SUCCESS)
        res = hdr->dwError;
    LeaveCriticalSection( &req->read_section );

    return res;
}

static void HTTPREQ_AsyncReadFileExAProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_INTERNETREADFILEEXA const *data = &workRequest->u.InternetReadFileExA;
    http_request_t *req = (http_request_t*)workRequest->hdr;
    DWORD res;

    TRACE("INTERNETREADFILEEXA %p\n", workRequest->hdr);

    res = HTTPREQ_Read(req, data->lpBuffersOut->lpvBuffer,
            data->lpBuffersOut->dwBufferLength, &data->lpBuffersOut->dwBufferLength, TRUE);

    send_request_complete(req, res == ERROR_SUCCESS, res);
}

static DWORD HTTPREQ_ReadFileExA(object_header_t *hdr, INTERNET_BUFFERSA *buffers,
        DWORD flags, DWORD_PTR context)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res, size, read, error = ERROR_SUCCESS;

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%x\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    if (buffers->dwStructSize != sizeof(*buffers))
        return ERROR_INVALID_PARAMETER;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    if (hdr->dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        if (TryEnterCriticalSection( &req->read_section ))
        {
            if (get_avail_data(req))
            {
                res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength,
                                   &buffers->dwBufferLength, FALSE);
                size = buffers->dwBufferLength;
                LeaveCriticalSection( &req->read_section );
                goto done;
            }
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncReadFileExAProc;
        workRequest.hdr = WININET_AddRef(&req->hdr);
        workRequest.u.InternetReadFileExA.lpBuffersOut = buffers;

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    read = 0;
    size = buffers->dwBufferLength;

    EnterCriticalSection( &req->read_section );
    if(hdr->dwError == ERROR_SUCCESS)
        hdr->dwError = INTERNET_HANDLE_IN_USE;
    else if(hdr->dwError == INTERNET_HANDLE_IN_USE)
        hdr->dwError = ERROR_INTERNET_INTERNAL_ERROR;

    while(1) {
        res = HTTPREQ_Read(req, (char*)buffers->lpvBuffer+read, size-read,
                &buffers->dwBufferLength, !(flags & IRF_NO_WAIT));
        if(res != ERROR_SUCCESS)
            break;

        read += buffers->dwBufferLength;
        if(read == size || end_of_read_data(req))
            break;

        LeaveCriticalSection( &req->read_section );

        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &buffers->dwBufferLength, sizeof(buffers->dwBufferLength));
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

        EnterCriticalSection( &req->read_section );
    }

    if(hdr->dwError == INTERNET_HANDLE_IN_USE)
        hdr->dwError = ERROR_SUCCESS;
    else
        error = hdr->dwError;

    LeaveCriticalSection( &req->read_section );
    size = buffers->dwBufferLength;
    buffers->dwBufferLength = read;

done:
    if (res == ERROR_SUCCESS) {
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &size, sizeof(size));
    }

    return res==ERROR_SUCCESS ? error : res;
}

static void HTTPREQ_AsyncReadFileExWProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_INTERNETREADFILEEXW const *data = &workRequest->u.InternetReadFileExW;
    http_request_t *req = (http_request_t*)workRequest->hdr;
    DWORD res;

    TRACE("INTERNETREADFILEEXW %p\n", workRequest->hdr);

    res = HTTPREQ_Read(req, data->lpBuffersOut->lpvBuffer,
            data->lpBuffersOut->dwBufferLength, &data->lpBuffersOut->dwBufferLength, TRUE);

    send_request_complete(req, res == ERROR_SUCCESS, res);
}

static DWORD HTTPREQ_ReadFileExW(object_header_t *hdr, INTERNET_BUFFERSW *buffers,
        DWORD flags, DWORD_PTR context)
{

    http_request_t *req = (http_request_t*)hdr;
    DWORD res, size, read, error = ERROR_SUCCESS;

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%x\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    if (buffers->dwStructSize != sizeof(*buffers))
        return ERROR_INVALID_PARAMETER;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    if (hdr->dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        if (TryEnterCriticalSection( &req->read_section ))
        {
            if (get_avail_data(req))
            {
                res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength,
                                   &buffers->dwBufferLength, FALSE);
                size = buffers->dwBufferLength;
                LeaveCriticalSection( &req->read_section );
                goto done;
            }
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncReadFileExWProc;
        workRequest.hdr = WININET_AddRef(&req->hdr);
        workRequest.u.InternetReadFileExW.lpBuffersOut = buffers;

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    read = 0;
    size = buffers->dwBufferLength;

    EnterCriticalSection( &req->read_section );
    if(hdr->dwError == ERROR_SUCCESS)
        hdr->dwError = INTERNET_HANDLE_IN_USE;
    else if(hdr->dwError == INTERNET_HANDLE_IN_USE)
        hdr->dwError = ERROR_INTERNET_INTERNAL_ERROR;

    while(1) {
        res = HTTPREQ_Read(req, (char*)buffers->lpvBuffer+read, size-read,
                &buffers->dwBufferLength, !(flags & IRF_NO_WAIT));
        if(res != ERROR_SUCCESS)
            break;

        read += buffers->dwBufferLength;
        if(read == size || end_of_read_data(req))
            break;

        LeaveCriticalSection( &req->read_section );

        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &buffers->dwBufferLength, sizeof(buffers->dwBufferLength));
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

        EnterCriticalSection( &req->read_section );
    }

    if(hdr->dwError == INTERNET_HANDLE_IN_USE)
        hdr->dwError = ERROR_SUCCESS;
    else
        error = hdr->dwError;

    LeaveCriticalSection( &req->read_section );
    size = buffers->dwBufferLength;
    buffers->dwBufferLength = read;

done:
    if (res == ERROR_SUCCESS) {
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &size, sizeof(size));
    }

    return res==ERROR_SUCCESS ? error : res;
}

static DWORD HTTPREQ_WriteFile(object_header_t *hdr, const void *buffer, DWORD size, DWORD *written)
{
    DWORD res;
    http_request_t *request = (http_request_t*)hdr;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

    *written = 0;
    res = NETCON_send(request->netconn, buffer, size, 0, (LPINT)written);
    if (res == ERROR_SUCCESS)
        request->bytesWritten += *written;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_REQUEST_SENT, written, sizeof(DWORD));
    return res;
}

static void HTTPREQ_AsyncQueryDataAvailableProc(WORKREQUEST *workRequest)
{
    http_request_t *req = (http_request_t*)workRequest->hdr;

    HTTP_ReceiveRequestData(req, FALSE);
}

static DWORD HTTPREQ_QueryDataAvailable(object_header_t *hdr, DWORD *available, DWORD flags, DWORD_PTR ctx)
{
    http_request_t *req = (http_request_t*)hdr;

    TRACE("(%p %p %x %lx)\n", req, available, flags, ctx);

    if (req->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        /* never wait, if we can't enter the section we queue an async request right away */
        if (TryEnterCriticalSection( &req->read_section ))
        {
            refill_read_buffer(req, READMODE_NOBLOCK, NULL);
            if ((*available = get_avail_data( req ))) goto done;
            if (end_of_read_data( req )) goto done;
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncQueryDataAvailableProc;
        workRequest.hdr = WININET_AddRef( &req->hdr );

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    EnterCriticalSection( &req->read_section );

    if (!(*available = get_avail_data( req )) && !end_of_read_data( req ))
    {
        refill_read_buffer( req, READMODE_ASYNC, NULL );
        *available = get_avail_data( req );
    }

done:
    LeaveCriticalSection( &req->read_section );

    TRACE( "returning %u\n", *available );
    return ERROR_SUCCESS;
}

static const object_vtbl_t HTTPREQVtbl = {
    HTTPREQ_Destroy,
    HTTPREQ_CloseConnection,
    HTTPREQ_QueryOption,
    HTTPREQ_SetOption,
    HTTPREQ_ReadFile,
    HTTPREQ_ReadFileExA,
    HTTPREQ_ReadFileExW,
    HTTPREQ_WriteFile,
    HTTPREQ_QueryDataAvailable,
    NULL
};

/***********************************************************************
 *           HTTP_HttpOpenRequestW (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
static DWORD HTTP_HttpOpenRequestW(http_session_t *session,
        LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
        LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
        DWORD dwFlags, DWORD_PTR dwContext, HINTERNET *ret)
{
    appinfo_t *hIC = session->appInfo;
    http_request_t *request;
    DWORD len, res = ERROR_SUCCESS;

    TRACE("-->\n");

    request = alloc_object(&session->hdr, &HTTPREQVtbl, sizeof(http_request_t));
    if(!request)
        return ERROR_OUTOFMEMORY;

    request->hdr.htype = WH_HHTTPREQ;
    request->hdr.dwFlags = dwFlags;
    request->hdr.dwContext = dwContext;
    request->contentLength = ~0u;

    request->netconn_stream.data_stream.vtbl = &netconn_stream_vtbl;
    request->data_stream = &request->netconn_stream.data_stream;
    request->connect_timeout = session->connect_timeout;
    request->send_timeout = session->send_timeout;
    request->receive_timeout = session->receive_timeout;

    InitializeCriticalSection( &request->read_section );
    request->read_section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": http_request_t.read_section");

    WININET_AddRef( &session->hdr );
    request->session = session;
    list_add_head( &session->hdr.children, &request->hdr.entry );

    if (dwFlags & INTERNET_FLAG_IGNORE_CERT_CN_INVALID)
        request->security_flags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    if (dwFlags & INTERNET_FLAG_IGNORE_CERT_DATE_INVALID)
        request->security_flags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (lpszObjectName && *lpszObjectName) {
        HRESULT rc;

        len = 0;
        rc = UrlEscapeW(lpszObjectName, NULL, &len, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            len = strlenW(lpszObjectName)+1;
        request->path = heap_alloc(len*sizeof(WCHAR));
        rc = UrlEscapeW(lpszObjectName, request->path, &len,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%d)\n",debugstr_w(lpszObjectName),rc);
            strcpyW(request->path,lpszObjectName);
        }
    }else {
        static const WCHAR slashW[] = {'/',0};

        request->path = heap_strdupW(slashW);
    }

    if (lpszReferrer && *lpszReferrer)
        HTTP_ProcessHeader(request, HTTP_REFERER, lpszReferrer, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);

    if (lpszAcceptTypes)
    {
        int i;
        for (i = 0; lpszAcceptTypes[i]; i++)
        {
            if (!*lpszAcceptTypes[i]) continue;
            HTTP_ProcessHeader(request, HTTP_ACCEPT, lpszAcceptTypes[i],
                               HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA |
                               HTTP_ADDHDR_FLAG_REQ |
                               (i == 0 ? HTTP_ADDHDR_FLAG_REPLACE : 0));
        }
    }

    request->verb = heap_strdupW(lpszVerb && *lpszVerb ? lpszVerb : szGET);
    request->version = heap_strdupW(lpszVersion ? lpszVersion : g_szHttp1_1);

    if (session->hostPort != INTERNET_INVALID_PORT_NUMBER &&
        session->hostPort != INTERNET_DEFAULT_HTTP_PORT &&
        session->hostPort != INTERNET_DEFAULT_HTTPS_PORT)
    {
        WCHAR *host_name;

        static const WCHAR host_formatW[] = {'%','s',':','%','u',0};

        host_name = heap_alloc((strlenW(session->hostName) + 7 /* length of ":65535" + 1 */) * sizeof(WCHAR));
        if (!host_name) {
            res = ERROR_OUTOFMEMORY;
            goto lend;
        }

        sprintfW(host_name, host_formatW, session->hostName, session->hostPort);
        HTTP_ProcessHeader(request, hostW, host_name, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);
        heap_free(host_name);
    }
    else
        HTTP_ProcessHeader(request, hostW, session->hostName,
                HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);

    if (session->serverPort == INTERNET_INVALID_PORT_NUMBER)
        session->serverPort = (dwFlags & INTERNET_FLAG_SECURE ?
                        INTERNET_DEFAULT_HTTPS_PORT :
                        INTERNET_DEFAULT_HTTP_PORT);

    if (session->hostPort == INTERNET_INVALID_PORT_NUMBER)
        session->hostPort = (dwFlags & INTERNET_FLAG_SECURE ?
                        INTERNET_DEFAULT_HTTPS_PORT :
                        INTERNET_DEFAULT_HTTP_PORT);

    if (hIC->proxy && hIC->proxy[0])
        HTTP_DealWithProxy( hIC, session, request );

    INTERNET_SendCallback(&session->hdr, dwContext,
                          INTERNET_STATUS_HANDLE_CREATED, &request->hdr.hInternet,
                          sizeof(HINTERNET));

lend:
    TRACE("<-- %u (%p)\n", res, request);

    if(res != ERROR_SUCCESS) {
        WININET_Release( &request->hdr );
        *ret = NULL;
        return res;
    }

    *ret = request->hdr.hInternet;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpOpenRequestW (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    http_session_t *session;
    HINTERNET handle = NULL;
    DWORD res;

    TRACE("(%p, %s, %s, %s, %s, %p, %08x, %08lx)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",debugstr_w(lpszAcceptTypes[i]));
    }

    session = (http_session_t*) get_handle_object( hHttpSession );
    if (NULL == session ||  session->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    /*
     * My tests seem to show that the windows version does not
     * become asynchronous until after this point. And anyhow
     * if this call was asynchronous then how would you get the
     * necessary HINTERNET pointer returned by this function.
     *
     */
    res = HTTP_HttpOpenRequestW(session, lpszVerb, lpszObjectName,
                                lpszVersion, lpszReferrer, lpszAcceptTypes,
                                dwFlags, dwContext, &handle);
lend:
    if( session )
        WININET_Release( &session->hdr );
    TRACE("returning %p\n", handle);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return handle;
}

static const LPCWSTR header_lookup[] = {
    szMime_Version,		/* HTTP_QUERY_MIME_VERSION = 0 */
    szContent_Type,		/* HTTP_QUERY_CONTENT_TYPE = 1 */
    szContent_Transfer_Encoding,/* HTTP_QUERY_CONTENT_TRANSFER_ENCODING = 2 */
    szContent_ID,		/* HTTP_QUERY_CONTENT_ID = 3 */
    NULL,			/* HTTP_QUERY_CONTENT_DESCRIPTION = 4 */
    szContent_Length,		/* HTTP_QUERY_CONTENT_LENGTH =  5 */
    szContent_Language,		/* HTTP_QUERY_CONTENT_LANGUAGE =  6 */
    szAllow,			/* HTTP_QUERY_ALLOW = 7 */
    szPublic,			/* HTTP_QUERY_PUBLIC = 8 */
    szDate,			/* HTTP_QUERY_DATE = 9 */
    szExpires,			/* HTTP_QUERY_EXPIRES = 10 */
    szLast_Modified,		/* HTTP_QUERY_LAST_MODIFIED = 11 */
    NULL,			/* HTTP_QUERY_MESSAGE_ID = 12 */
    szURI,			/* HTTP_QUERY_URI = 13 */
    szFrom,			/* HTTP_QUERY_DERIVED_FROM = 14 */
    NULL,			/* HTTP_QUERY_COST = 15 */
    NULL,			/* HTTP_QUERY_LINK = 16 */
    szPragma,			/* HTTP_QUERY_PRAGMA = 17 */
    NULL,			/* HTTP_QUERY_VERSION = 18 */
    szStatus,			/* HTTP_QUERY_STATUS_CODE = 19 */
    NULL,			/* HTTP_QUERY_STATUS_TEXT = 20 */
    NULL,			/* HTTP_QUERY_RAW_HEADERS = 21 */
    NULL,			/* HTTP_QUERY_RAW_HEADERS_CRLF = 22 */
    szConnection,		/* HTTP_QUERY_CONNECTION = 23 */
    szAccept,			/* HTTP_QUERY_ACCEPT = 24 */
    szAccept_Charset,		/* HTTP_QUERY_ACCEPT_CHARSET = 25 */
    szAccept_Encoding,		/* HTTP_QUERY_ACCEPT_ENCODING = 26 */
    szAccept_Language,		/* HTTP_QUERY_ACCEPT_LANGUAGE = 27 */
    szAuthorization,		/* HTTP_QUERY_AUTHORIZATION = 28 */
    szContent_Encoding,		/* HTTP_QUERY_CONTENT_ENCODING = 29 */
    NULL,			/* HTTP_QUERY_FORWARDED = 30 */
    NULL,			/* HTTP_QUERY_FROM = 31 */
    szIf_Modified_Since,	/* HTTP_QUERY_IF_MODIFIED_SINCE = 32 */
    szLocation,			/* HTTP_QUERY_LOCATION = 33 */
    NULL,			/* HTTP_QUERY_ORIG_URI = 34 */
    szReferer,			/* HTTP_QUERY_REFERER = 35 */
    szRetry_After,		/* HTTP_QUERY_RETRY_AFTER = 36 */
    szServer,			/* HTTP_QUERY_SERVER = 37 */
    NULL,			/* HTTP_TITLE = 38 */
    szUser_Agent,		/* HTTP_QUERY_USER_AGENT = 39 */
    szWWW_Authenticate,		/* HTTP_QUERY_WWW_AUTHENTICATE = 40 */
    szProxy_Authenticate,	/* HTTP_QUERY_PROXY_AUTHENTICATE = 41 */
    szAccept_Ranges,		/* HTTP_QUERY_ACCEPT_RANGES = 42 */
    szSet_Cookie,		/* HTTP_QUERY_SET_COOKIE = 43 */
    szCookie,			/* HTTP_QUERY_COOKIE = 44 */
    NULL,			/* HTTP_QUERY_REQUEST_METHOD = 45 */
    NULL,			/* HTTP_QUERY_REFRESH = 46 */
    NULL,			/* HTTP_QUERY_CONTENT_DISPOSITION = 47 */
    szAge,			/* HTTP_QUERY_AGE = 48 */
    szCache_Control,		/* HTTP_QUERY_CACHE_CONTROL = 49 */
    szContent_Base,		/* HTTP_QUERY_CONTENT_BASE = 50 */
    szContent_Location,		/* HTTP_QUERY_CONTENT_LOCATION = 51 */
    szContent_MD5,		/* HTTP_QUERY_CONTENT_MD5 = 52 */
    szContent_Range,		/* HTTP_QUERY_CONTENT_RANGE = 53 */
    szETag,			/* HTTP_QUERY_ETAG = 54 */
    hostW,			/* HTTP_QUERY_HOST = 55 */
    szIf_Match,			/* HTTP_QUERY_IF_MATCH = 56 */
    szIf_None_Match,		/* HTTP_QUERY_IF_NONE_MATCH = 57 */
    szIf_Range,			/* HTTP_QUERY_IF_RANGE = 58 */
    szIf_Unmodified_Since,	/* HTTP_QUERY_IF_UNMODIFIED_SINCE = 59 */
    szMax_Forwards,		/* HTTP_QUERY_MAX_FORWARDS = 60 */
    szProxy_Authorization,	/* HTTP_QUERY_PROXY_AUTHORIZATION = 61 */
    szRange,			/* HTTP_QUERY_RANGE = 62 */
    szTransfer_Encoding,	/* HTTP_QUERY_TRANSFER_ENCODING = 63 */
    szUpgrade,			/* HTTP_QUERY_UPGRADE = 64 */
    szVary,			/* HTTP_QUERY_VARY = 65 */
    szVia,			/* HTTP_QUERY_VIA = 66 */
    szWarning,			/* HTTP_QUERY_WARNING = 67 */
    szExpect,			/* HTTP_QUERY_EXPECT = 68 */
    szProxy_Connection,		/* HTTP_QUERY_PROXY_CONNECTION = 69 */
    szUnless_Modified_Since,	/* HTTP_QUERY_UNLESS_MODIFIED_SINCE = 70 */
};

#define LAST_TABLE_HEADER (sizeof(header_lookup)/sizeof(header_lookup[0]))

/***********************************************************************
 *           HTTP_HttpQueryInfoW (internal)
 */
static DWORD HTTP_HttpQueryInfoW(http_request_t *request, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL request_only = dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS;
    INT requested_index = lpdwIndex ? *lpdwIndex : 0;
    DWORD level = (dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK);
    INT index = -1;

    /* Find requested header structure */
    switch (level)
    {
    case HTTP_QUERY_CUSTOM:
        if (!lpBuffer) return ERROR_INVALID_PARAMETER;
        index = HTTP_GetCustomHeaderIndex(request, lpBuffer, requested_index, request_only);
        break;
    case HTTP_QUERY_RAW_HEADERS_CRLF:
        {
            LPWSTR headers;
            DWORD len = 0;
            DWORD res = ERROR_INVALID_PARAMETER;

            if (request_only)
                headers = HTTP_BuildHeaderRequestString(request, request->verb, request->path, request->version);
            else
                headers = request->rawHeaders;

            if (headers)
                len = strlenW(headers) * sizeof(WCHAR);

            if (len + sizeof(WCHAR) > *lpdwBufferLength)
            {
                len += sizeof(WCHAR);
                res = ERROR_INSUFFICIENT_BUFFER;
            }
            else if (lpBuffer)
            {
                if (headers)
                    memcpy(lpBuffer, headers, len + sizeof(WCHAR));
                else
                {
                    len = strlenW(szCrLf) * sizeof(WCHAR);
                    memcpy(lpBuffer, szCrLf, sizeof(szCrLf));
                }
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len / sizeof(WCHAR)));
                res = ERROR_SUCCESS;
            }
            *lpdwBufferLength = len;

            if (request_only) heap_free(headers);
            return res;
        }
    case HTTP_QUERY_RAW_HEADERS:
        {
            LPWSTR * ppszRawHeaderLines = HTTP_Tokenize(request->rawHeaders, szCrLf);
            DWORD i, size = 0;
            LPWSTR pszString = lpBuffer;

            for (i = 0; ppszRawHeaderLines[i]; i++)
                size += strlenW(ppszRawHeaderLines[i]) + 1;

            if (size + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                HTTP_FreeTokens(ppszRawHeaderLines);
                *lpdwBufferLength = (size + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (pszString)
            {
                for (i = 0; ppszRawHeaderLines[i]; i++)
                {
                    DWORD len = strlenW(ppszRawHeaderLines[i]);
                    memcpy(pszString, ppszRawHeaderLines[i], (len+1)*sizeof(WCHAR));
                    pszString += len+1;
                }
                *pszString = '\0';
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, size));
            }
            *lpdwBufferLength = size * sizeof(WCHAR);
            HTTP_FreeTokens(ppszRawHeaderLines);

            return ERROR_SUCCESS;
        }
    case HTTP_QUERY_STATUS_TEXT:
        if (request->statusText)
        {
            DWORD len = strlenW(request->statusText);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, request->statusText, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_VERSION:
        if (request->version)
        {
            DWORD len = strlenW(request->version);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, request->version, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_CONTENT_ENCODING:
        index = HTTP_GetCustomHeaderIndex(request, header_lookup[request->read_gzip ? HTTP_QUERY_CONTENT_TYPE : level],
                requested_index,request_only);
        break;
    case HTTP_QUERY_STATUS_CODE: {
        DWORD res = ERROR_SUCCESS;

        if(request_only || requested_index)
            break;

        if(dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) {
            if(*lpdwBufferLength >= sizeof(DWORD))
                *(DWORD*)lpBuffer = request->status_code;
            else
                res = ERROR_INSUFFICIENT_BUFFER;
            *lpdwBufferLength = sizeof(DWORD);
        }else {
            WCHAR buf[12];
            DWORD size;
            static const WCHAR formatW[] = {'%','u',0};

            size = (sprintfW(buf, formatW, request->status_code)+1) * sizeof(WCHAR);

            if(size <= *lpdwBufferLength)
                memcpy(lpBuffer, buf, size);
            else
                res = ERROR_INSUFFICIENT_BUFFER;

            *lpdwBufferLength = size;
        }
        return res;
    }
    default:
        assert (LAST_TABLE_HEADER == (HTTP_QUERY_UNLESS_MODIFIED_SINCE + 1));

        if (level < LAST_TABLE_HEADER && header_lookup[level])
            index = HTTP_GetCustomHeaderIndex(request, header_lookup[level],
                                              requested_index,request_only);
    }

    if (index >= 0)
        lphttpHdr = &request->custHeaders[index];

    /* Ensure header satisfies requested attributes */
    if (!lphttpHdr ||
        ((dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) &&
         (~lphttpHdr->wFlags & HDR_ISREQUEST)))
    {
        return ERROR_HTTP_HEADER_NOT_FOUND;
    }

    if (lpdwIndex) (*lpdwIndex)++;

    /* coalesce value to requested type */
    if (dwInfoLevel & HTTP_QUERY_FLAG_NUMBER && lpBuffer)
    {
        *(int *)lpBuffer = atoiW(lphttpHdr->lpszValue);
        TRACE(" returning number: %d\n", *(int *)lpBuffer);
     }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME && lpBuffer)
    {
        time_t tmpTime;
        struct tm tmpTM;
        SYSTEMTIME *STHook;

        tmpTime = ConvertTimeString(lphttpHdr->lpszValue);

        tmpTM = *gmtime(&tmpTime);
        STHook = (SYSTEMTIME *)lpBuffer;
        STHook->wDay = tmpTM.tm_mday;
        STHook->wHour = tmpTM.tm_hour;
        STHook->wMilliseconds = 0;
        STHook->wMinute = tmpTM.tm_min;
        STHook->wDayOfWeek = tmpTM.tm_wday;
        STHook->wMonth = tmpTM.tm_mon + 1;
        STHook->wSecond = tmpTM.tm_sec;
        STHook->wYear = tmpTM.tm_year;

        TRACE(" returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
              STHook->wYear, STHook->wMonth, STHook->wDay, STHook->wDayOfWeek,
              STHook->wHour, STHook->wMinute, STHook->wSecond, STHook->wMilliseconds);
    }
    else if (lphttpHdr->lpszValue)
    {
        DWORD len = (strlenW(lphttpHdr->lpszValue) + 1) * sizeof(WCHAR);

        if (len > *lpdwBufferLength)
        {
            *lpdwBufferLength = len;
            return ERROR_INSUFFICIENT_BUFFER;
        }
        if (lpBuffer)
        {
            memcpy(lpBuffer, lphttpHdr->lpszValue, len);
            TRACE("! returning string: %s\n", debugstr_w(lpBuffer));
        }
        *lpdwBufferLength = len - sizeof(WCHAR);
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoW (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoW(HINTERNET hHttpRequest, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    http_request_t *request;
    DWORD res;

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info query_flags[] = {
	    FE(HTTP_QUERY_MIME_VERSION),
	    FE(HTTP_QUERY_CONTENT_TYPE),
	    FE(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_CONTENT_ID),
	    FE(HTTP_QUERY_CONTENT_DESCRIPTION),
	    FE(HTTP_QUERY_CONTENT_LENGTH),
	    FE(HTTP_QUERY_CONTENT_LANGUAGE),
	    FE(HTTP_QUERY_ALLOW),
	    FE(HTTP_QUERY_PUBLIC),
	    FE(HTTP_QUERY_DATE),
	    FE(HTTP_QUERY_EXPIRES),
	    FE(HTTP_QUERY_LAST_MODIFIED),
	    FE(HTTP_QUERY_MESSAGE_ID),
	    FE(HTTP_QUERY_URI),
	    FE(HTTP_QUERY_DERIVED_FROM),
	    FE(HTTP_QUERY_COST),
	    FE(HTTP_QUERY_LINK),
	    FE(HTTP_QUERY_PRAGMA),
	    FE(HTTP_QUERY_VERSION),
	    FE(HTTP_QUERY_STATUS_CODE),
	    FE(HTTP_QUERY_STATUS_TEXT),
	    FE(HTTP_QUERY_RAW_HEADERS),
	    FE(HTTP_QUERY_RAW_HEADERS_CRLF),
	    FE(HTTP_QUERY_CONNECTION),
	    FE(HTTP_QUERY_ACCEPT),
	    FE(HTTP_QUERY_ACCEPT_CHARSET),
	    FE(HTTP_QUERY_ACCEPT_ENCODING),
	    FE(HTTP_QUERY_ACCEPT_LANGUAGE),
	    FE(HTTP_QUERY_AUTHORIZATION),
	    FE(HTTP_QUERY_CONTENT_ENCODING),
	    FE(HTTP_QUERY_FORWARDED),
	    FE(HTTP_QUERY_FROM),
	    FE(HTTP_QUERY_IF_MODIFIED_SINCE),
	    FE(HTTP_QUERY_LOCATION),
	    FE(HTTP_QUERY_ORIG_URI),
	    FE(HTTP_QUERY_REFERER),
	    FE(HTTP_QUERY_RETRY_AFTER),
	    FE(HTTP_QUERY_SERVER),
	    FE(HTTP_QUERY_TITLE),
	    FE(HTTP_QUERY_USER_AGENT),
	    FE(HTTP_QUERY_WWW_AUTHENTICATE),
	    FE(HTTP_QUERY_PROXY_AUTHENTICATE),
	    FE(HTTP_QUERY_ACCEPT_RANGES),
        FE(HTTP_QUERY_SET_COOKIE),
        FE(HTTP_QUERY_COOKIE),
	    FE(HTTP_QUERY_REQUEST_METHOD),
	    FE(HTTP_QUERY_REFRESH),
	    FE(HTTP_QUERY_CONTENT_DISPOSITION),
	    FE(HTTP_QUERY_AGE),
	    FE(HTTP_QUERY_CACHE_CONTROL),
	    FE(HTTP_QUERY_CONTENT_BASE),
	    FE(HTTP_QUERY_CONTENT_LOCATION),
	    FE(HTTP_QUERY_CONTENT_MD5),
	    FE(HTTP_QUERY_CONTENT_RANGE),
	    FE(HTTP_QUERY_ETAG),
	    FE(HTTP_QUERY_HOST),
	    FE(HTTP_QUERY_IF_MATCH),
	    FE(HTTP_QUERY_IF_NONE_MATCH),
	    FE(HTTP_QUERY_IF_RANGE),
	    FE(HTTP_QUERY_IF_UNMODIFIED_SINCE),
	    FE(HTTP_QUERY_MAX_FORWARDS),
	    FE(HTTP_QUERY_PROXY_AUTHORIZATION),
	    FE(HTTP_QUERY_RANGE),
	    FE(HTTP_QUERY_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_UPGRADE),
	    FE(HTTP_QUERY_VARY),
	    FE(HTTP_QUERY_VIA),
	    FE(HTTP_QUERY_WARNING),
	    FE(HTTP_QUERY_CUSTOM)
	};
	static const wininet_flag_info modifier_flags[] = {
	    FE(HTTP_QUERY_FLAG_REQUEST_HEADERS),
	    FE(HTTP_QUERY_FLAG_SYSTEMTIME),
	    FE(HTTP_QUERY_FLAG_NUMBER),
	    FE(HTTP_QUERY_FLAG_COALESCE)
	};
#undef FE
	DWORD info_mod = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
	DWORD info = dwInfoLevel & HTTP_QUERY_HEADER_MASK;
	DWORD i;

	TRACE("(%p, 0x%08x)--> %d\n", hHttpRequest, dwInfoLevel, info);
	TRACE("  Attribute:");
	for (i = 0; i < (sizeof(query_flags) / sizeof(query_flags[0])); i++) {
	    if (query_flags[i].val == info) {
		TRACE(" %s", query_flags[i].name);
		break;
	    }
	}
	if (i == (sizeof(query_flags) / sizeof(query_flags[0]))) {
	    TRACE(" Unknown (%08x)", info);
	}

	TRACE(" Modifier:");
	for (i = 0; i < (sizeof(modifier_flags) / sizeof(modifier_flags[0])); i++) {
	    if (modifier_flags[i].val & info_mod) {
		TRACE(" %s", modifier_flags[i].name);
		info_mod &= ~ modifier_flags[i].val;
	    }
	}
	
	if (info_mod) {
	    TRACE(" Unknown (%08x)", info_mod);
	}
	TRACE("\n");
    }
    
    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (NULL == request ||  request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (lpBuffer == NULL)
        *lpdwBufferLength = 0;
    res = HTTP_HttpQueryInfoW( request, dwInfoLevel,
                               lpBuffer, lpdwBufferLength, lpdwIndex);

lend:
    if( request )
         WININET_Release( &request->hdr );

    TRACE("%u <--\n", res);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoA (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoA(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL result;
    DWORD len;
    WCHAR* bufferW;

    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        return HttpQueryInfoW( hHttpRequest, dwInfoLevel, lpBuffer,
                               lpdwBufferLength, lpdwIndex );
    }

    if (lpBuffer)
    {
        DWORD alloclen;
        len = (*lpdwBufferLength)*sizeof(WCHAR);
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
        {
            alloclen = MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, NULL, 0 ) * sizeof(WCHAR);
            if (alloclen < len)
                alloclen = len;
        }
        else
            alloclen = len;
        bufferW = heap_alloc(alloclen);
        /* buffer is in/out because of HTTP_QUERY_CUSTOM */
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
            MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, bufferW, alloclen / sizeof(WCHAR) );
    } else
    {
        bufferW = NULL;
        len = 0;
    }

    result = HttpQueryInfoW( hHttpRequest, dwInfoLevel, bufferW,
                           &len, lpdwIndex );
    if( result )
    {
        len = WideCharToMultiByte( CP_ACP,0, bufferW, len / sizeof(WCHAR) + 1,
                                     lpBuffer, *lpdwBufferLength, NULL, NULL );
        *lpdwBufferLength = len - 1;

        TRACE("lpBuffer: %s\n", debugstr_a(lpBuffer));
    }
    else
        /* since the strings being returned from HttpQueryInfoW should be
         * only ASCII characters, it is reasonable to assume that all of
         * the Unicode characters can be reduced to a single byte */
        *lpdwBufferLength = len / sizeof(WCHAR);

    heap_free( bufferW );
    return result;
}

/***********************************************************************
 *           HTTP_GetRedirectURL (internal)
 */
static LPWSTR HTTP_GetRedirectURL(http_request_t *request, LPCWSTR lpszUrl)
{
    static WCHAR szHttp[] = {'h','t','t','p',0};
    static WCHAR szHttps[] = {'h','t','t','p','s',0};
    http_session_t *session = request->session;
    URL_COMPONENTSW urlComponents;
    DWORD url_length = 0;
    LPWSTR orig_url;
    LPWSTR combined_url;

    urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComponents.lpszScheme = (request->hdr.dwFlags & INTERNET_FLAG_SECURE) ? szHttps : szHttp;
    urlComponents.dwSchemeLength = 0;
    urlComponents.lpszHostName = session->hostName;
    urlComponents.dwHostNameLength = 0;
    urlComponents.nPort = session->hostPort;
    urlComponents.lpszUserName = session->userName;
    urlComponents.dwUserNameLength = 0;
    urlComponents.lpszPassword = NULL;
    urlComponents.dwPasswordLength = 0;
    urlComponents.lpszUrlPath = request->path;
    urlComponents.dwUrlPathLength = 0;
    urlComponents.lpszExtraInfo = NULL;
    urlComponents.dwExtraInfoLength = 0;

    if (!InternetCreateUrlW(&urlComponents, 0, NULL, &url_length) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return NULL;

    orig_url = heap_alloc(url_length);

    /* convert from bytes to characters */
    url_length = url_length / sizeof(WCHAR) - 1;
    if (!InternetCreateUrlW(&urlComponents, 0, orig_url, &url_length))
    {
        heap_free(orig_url);
        return NULL;
    }

    url_length = 0;
    if (!InternetCombineUrlW(orig_url, lpszUrl, NULL, &url_length, ICU_ENCODE_SPACES_ONLY) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        heap_free(orig_url);
        return NULL;
    }
    combined_url = heap_alloc(url_length * sizeof(WCHAR));

    if (!InternetCombineUrlW(orig_url, lpszUrl, combined_url, &url_length, ICU_ENCODE_SPACES_ONLY))
    {
        heap_free(orig_url);
        heap_free(combined_url);
        return NULL;
    }
    heap_free(orig_url);
    return combined_url;
}


/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static DWORD HTTP_HandleRedirect(http_request_t *request, LPCWSTR lpszUrl)
{
    http_session_t *session = request->session;
    appinfo_t *hIC = session->appInfo;
    BOOL using_proxy = hIC->proxy && hIC->proxy[0];
    WCHAR path[INTERNET_MAX_PATH_LENGTH];
    int index;

    if(lpszUrl[0]=='/')
    {
        /* if it's an absolute path, keep the same session info */
        lstrcpynW(path, lpszUrl, INTERNET_MAX_URL_LENGTH);
    }
    else
    {
        URL_COMPONENTSW urlComponents;
        WCHAR protocol[INTERNET_MAX_SCHEME_LENGTH];
        WCHAR hostName[INTERNET_MAX_HOST_NAME_LENGTH];
        WCHAR userName[INTERNET_MAX_USER_NAME_LENGTH];
        BOOL custom_port = FALSE;

        static WCHAR httpW[] = {'h','t','t','p',0};
        static WCHAR httpsW[] = {'h','t','t','p','s',0};

        userName[0] = 0;
        hostName[0] = 0;
        protocol[0] = 0;

        urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
        urlComponents.lpszScheme = protocol;
        urlComponents.dwSchemeLength = INTERNET_MAX_SCHEME_LENGTH;
        urlComponents.lpszHostName = hostName;
        urlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
        urlComponents.lpszUserName = userName;
        urlComponents.dwUserNameLength = INTERNET_MAX_USER_NAME_LENGTH;
        urlComponents.lpszPassword = NULL;
        urlComponents.dwPasswordLength = 0;
        urlComponents.lpszUrlPath = path;
        urlComponents.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
        urlComponents.lpszExtraInfo = NULL;
        urlComponents.dwExtraInfoLength = 0;
        if(!InternetCrackUrlW(lpszUrl, strlenW(lpszUrl), 0, &urlComponents))
            return INTERNET_GetLastError();

        if(!strcmpiW(protocol, httpW)) {
            if(request->hdr.dwFlags & INTERNET_FLAG_SECURE) {
                TRACE("redirect from secure page to non-secure page\n");
                /* FIXME: warn about from secure redirect to non-secure page */
                request->hdr.dwFlags &= ~INTERNET_FLAG_SECURE;
            }

            if(urlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
                urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;
            else if(urlComponents.nPort != INTERNET_DEFAULT_HTTP_PORT)
                custom_port = TRUE;
        }else if(!strcmpiW(protocol, httpsW)) {
            if(!(request->hdr.dwFlags & INTERNET_FLAG_SECURE)) {
                TRACE("redirect from non-secure page to secure page\n");
                /* FIXME: notify about redirect to secure page */
                request->hdr.dwFlags |= INTERNET_FLAG_SECURE;
            }

            if(urlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
                urlComponents.nPort = INTERNET_DEFAULT_HTTPS_PORT;
            else if(urlComponents.nPort != INTERNET_DEFAULT_HTTPS_PORT)
                custom_port = TRUE;
        }

        heap_free(session->hostName);

        if(custom_port) {
            int len;
            static const WCHAR fmt[] = {'%','s',':','%','u',0};
            len = lstrlenW(hostName);
            len += 7; /* 5 for strlen("65535") + 1 for ":" + 1 for '\0' */
            session->hostName = heap_alloc(len*sizeof(WCHAR));
            sprintfW(session->hostName, fmt, hostName, urlComponents.nPort);
        }
        else
            session->hostName = heap_strdupW(hostName);

        HTTP_ProcessHeader(request, hostW, session->hostName, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);

        heap_free(session->userName);
        session->userName = NULL;
        if (userName[0])
            session->userName = heap_strdupW(userName);

        reset_data_stream(request);

        if(!using_proxy) {
            if(strcmpiW(session->serverName, hostName)) {
                heap_free(session->serverName);
                session->serverName = heap_strdupW(hostName);
            }
            session->serverPort = urlComponents.nPort;
        }
    }
    heap_free(request->path);
    request->path=NULL;
    if (*path)
    {
        DWORD needed = 0;
        HRESULT rc;

        rc = UrlEscapeW(path, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlenW(path)+1;
        request->path = heap_alloc(needed*sizeof(WCHAR));
        rc = UrlEscapeW(path, request->path, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%d)\n",debugstr_w(path),rc);
            strcpyW(request->path,path);
        }
    }

    /* Remove custom content-type/length headers on redirects.  */
    index = HTTP_GetCustomHeaderIndex(request, szContent_Type, 0, TRUE);
    if (0 <= index)
        HTTP_DeleteCustomHeader(request, index);
    index = HTTP_GetCustomHeaderIndex(request, szContent_Length, 0, TRUE);
    if (0 <= index)
        HTTP_DeleteCustomHeader(request, index);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_build_req (internal)
 *
 *  concatenate all the strings in the request together
 */
static LPWSTR HTTP_build_req( LPCWSTR *list, int len )
{
    LPCWSTR *t;
    LPWSTR str;

    for( t = list; *t ; t++  )
        len += strlenW( *t );
    len++;

    str = heap_alloc(len*sizeof(WCHAR));
    *str = 0;

    for( t = list; *t ; t++ )
        strcatW( str, *t );

    return str;
}

static DWORD HTTP_SecureProxyConnect(http_request_t *request)
{
    LPWSTR lpszPath;
    LPWSTR requestString;
    INT len;
    INT cnt;
    INT responseLen;
    char *ascii_req;
    DWORD res;
    static const WCHAR szConnect[] = {'C','O','N','N','E','C','T',0};
    static const WCHAR szFormat[] = {'%','s',':','%','u',0};
    http_session_t *session = request->session;

    TRACE("\n");

    lpszPath = heap_alloc((lstrlenW( session->hostName ) + 13)*sizeof(WCHAR));
    sprintfW( lpszPath, szFormat, session->hostName, session->hostPort );
    requestString = HTTP_BuildHeaderRequestString( request, szConnect, lpszPath, g_szHttp1_1 );
    heap_free( lpszPath );

    len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                NULL, 0, NULL, NULL );
    len--; /* the nul terminator isn't needed */
    ascii_req = heap_alloc(len);
    WideCharToMultiByte( CP_ACP, 0, requestString, -1, ascii_req, len, NULL, NULL );
    heap_free( requestString );

    TRACE("full request -> %s\n", debugstr_an( ascii_req, len ) );

    NETCON_set_timeout( request->netconn, TRUE, request->send_timeout );
    res = NETCON_send( request->netconn, ascii_req, len, 0, &cnt );
    heap_free( ascii_req );
    if (res != ERROR_SUCCESS)
        return res;

    responseLen = HTTP_GetResponseHeaders( request, TRUE );
    if (!responseLen)
        return ERROR_HTTP_INVALID_HEADER;

    return ERROR_SUCCESS;
}

static void HTTP_InsertCookies(http_request_t *request)
{
    DWORD cookie_size, size, cnt = 0;
    HTTPHEADERW *host;
    WCHAR *cookies;

    static const WCHAR cookieW[] = {'C','o','o','k','i','e',':',' ',0};

    host = HTTP_GetHeader(request, hostW);
    if(!host)
        return;

    if(!get_cookie(host->lpszValue, request->path, NULL, &cookie_size))
        return;

    size = sizeof(cookieW) + cookie_size * sizeof(WCHAR) + sizeof(szCrLf);
    if(!(cookies = heap_alloc(size)))
        return;

    cnt += sprintfW(cookies, cookieW);
    get_cookie(host->lpszValue, request->path, cookies+cnt, &cookie_size);
    strcatW(cookies, szCrLf);

    HTTP_HttpAddRequestHeadersW(request, cookies, strlenW(cookies), HTTP_ADDREQ_FLAG_REPLACE);

    heap_free(cookies);
}

static WORD HTTP_ParseWkday(LPCWSTR day)
{
    static const WCHAR days[7][4] = {{ 's','u','n',0 },
                                     { 'm','o','n',0 },
                                     { 't','u','e',0 },
                                     { 'w','e','d',0 },
                                     { 't','h','u',0 },
                                     { 'f','r','i',0 },
                                     { 's','a','t',0 }};
    int i;
    for (i = 0; i < sizeof(days)/sizeof(*days); i++)
        if (!strcmpiW(day, days[i]))
            return i;

    /* Invalid */
    return 7;
}

static WORD HTTP_ParseMonth(LPCWSTR month)
{
    static const WCHAR jan[] = { 'j','a','n',0 };
    static const WCHAR feb[] = { 'f','e','b',0 };
    static const WCHAR mar[] = { 'm','a','r',0 };
    static const WCHAR apr[] = { 'a','p','r',0 };
    static const WCHAR may[] = { 'm','a','y',0 };
    static const WCHAR jun[] = { 'j','u','n',0 };
    static const WCHAR jul[] = { 'j','u','l',0 };
    static const WCHAR aug[] = { 'a','u','g',0 };
    static const WCHAR sep[] = { 's','e','p',0 };
    static const WCHAR oct[] = { 'o','c','t',0 };
    static const WCHAR nov[] = { 'n','o','v',0 };
    static const WCHAR dec[] = { 'd','e','c',0 };

    if (!strcmpiW(month, jan)) return 1;
    if (!strcmpiW(month, feb)) return 2;
    if (!strcmpiW(month, mar)) return 3;
    if (!strcmpiW(month, apr)) return 4;
    if (!strcmpiW(month, may)) return 5;
    if (!strcmpiW(month, jun)) return 6;
    if (!strcmpiW(month, jul)) return 7;
    if (!strcmpiW(month, aug)) return 8;
    if (!strcmpiW(month, sep)) return 9;
    if (!strcmpiW(month, oct)) return 10;
    if (!strcmpiW(month, nov)) return 11;
    if (!strcmpiW(month, dec)) return 12;
    /* Invalid */
    return 0;
}

/* Parses the string pointed to by *str, assumed to be a 24-hour time HH:MM:SS,
 * optionally preceded by whitespace.
 * Upon success, returns TRUE, sets the wHour, wMinute, and wSecond fields of
 * st, and sets *str to the first character after the time format.
 */
static BOOL HTTP_ParseTime(SYSTEMTIME *st, LPCWSTR *str)
{
    LPCWSTR ptr = *str;
    WCHAR *nextPtr;
    unsigned long num;

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || *nextPtr != ':')
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 23)
    {
        ERR("unexpected hour in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr + 1;
    st->wHour = (WORD)num;
    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || *nextPtr != ':')
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 59)
    {
        ERR("unexpected minute in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr + 1;
    st->wMinute = (WORD)num;
    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr)
    {
        ERR("unexpected time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    if (num > 59)
    {
        ERR("unexpected second in time format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr + 1;
    *str = ptr;
    st->wSecond = (WORD)num;
    return TRUE;
}

static BOOL HTTP_ParseDateAsAsctime(LPCWSTR value, FILETIME *ft)
{
    static const WCHAR gmt[]= { 'G','M','T',0 };
    WCHAR day[4], *dayPtr, month[4], *monthPtr, *nextPtr;
    LPCWSTR ptr;
    SYSTEMTIME st = { 0 };
    unsigned long num;

    for (ptr = value, dayPtr = day; *ptr && !isspaceW(*ptr) &&
         dayPtr - day < sizeof(day) / sizeof(day[0]) - 1; ptr++, dayPtr++)
        *dayPtr = *ptr;
    *dayPtr = 0;
    st.wDayOfWeek = HTTP_ParseWkday(day);
    if (st.wDayOfWeek >= 7)
    {
        ERR("unexpected weekday %s\n", debugstr_w(day));
        return FALSE;
    }

    while (isspaceW(*ptr))
        ptr++;

    for (monthPtr = month; !isspace(*ptr) &&
         monthPtr - month < sizeof(month) / sizeof(month[0]) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        ERR("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        ERR("unexpected day %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    while (isspaceW(*ptr))
        ptr++;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    while (isspaceW(*ptr))
        ptr++;

    /* asctime() doesn't report a timezone, but some web servers do, so accept
     * with or without GMT.
     */
    if (*ptr && strcmpW(ptr, gmt))
    {
        ERR("unexpected timezone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static BOOL HTTP_ParseRfc1123Date(LPCWSTR value, FILETIME *ft)
{
    static const WCHAR gmt[]= { 'G','M','T',0 };
    WCHAR *nextPtr, day[4], month[4], *monthPtr;
    LPCWSTR ptr;
    unsigned long num;
    SYSTEMTIME st = { 0 };

    ptr = strchrW(value, ',');
    if (!ptr)
        return FALSE;
    if (ptr - value != 3)
    {
        WARN("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    memcpy(day, value, (ptr - value) * sizeof(WCHAR));
    day[3] = 0;
    st.wDayOfWeek = HTTP_ParseWkday(day);
    if (st.wDayOfWeek > 6)
    {
        WARN("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    ptr++;

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        WARN("unexpected day %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    while (isspaceW(*ptr))
        ptr++;

    for (monthPtr = month; !isspace(*ptr) &&
         monthPtr - month < sizeof(month) / sizeof(month[0]) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        WARN("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (isspaceW(*ptr))
        ptr++;

    if (strcmpW(ptr, gmt))
    {
        ERR("unexpected time zone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static WORD HTTP_ParseWeekday(LPCWSTR day)
{
    static const WCHAR days[7][10] = {{ 's','u','n','d','a','y',0 },
                                     { 'm','o','n','d','a','y',0 },
                                     { 't','u','e','s','d','a','y',0 },
                                     { 'w','e','d','n','e','s','d','a','y',0 },
                                     { 't','h','u','r','s','d','a','y',0 },
                                     { 'f','r','i','d','a','y',0 },
                                     { 's','a','t','u','r','d','a','y',0 }};
    int i;
    for (i = 0; i < sizeof(days)/sizeof(*days); i++)
        if (!strcmpiW(day, days[i]))
            return i;

    /* Invalid */
    return 7;
}

static BOOL HTTP_ParseRfc850Date(LPCWSTR value, FILETIME *ft)
{
    static const WCHAR gmt[]= { 'G','M','T',0 };
    WCHAR *nextPtr, day[10], month[4], *monthPtr;
    LPCWSTR ptr;
    unsigned long num;
    SYSTEMTIME st = { 0 };

    ptr = strchrW(value, ',');
    if (!ptr)
        return FALSE;
    if (ptr - value == 3)
    {
        memcpy(day, value, (ptr - value) * sizeof(WCHAR));
        day[3] = 0;
        st.wDayOfWeek = HTTP_ParseWkday(day);
        if (st.wDayOfWeek > 6)
        {
            ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
            return FALSE;
        }
    }
    else if (ptr - value < sizeof(day) / sizeof(day[0]))
    {
        memcpy(day, value, (ptr - value) * sizeof(WCHAR));
        day[ptr - value + 1] = 0;
        st.wDayOfWeek = HTTP_ParseWeekday(day);
        if (st.wDayOfWeek > 6)
        {
            ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
            return FALSE;
        }
    }
    else
    {
        ERR("unexpected weekday %s\n", debugstr_wn(value, ptr - value));
        return FALSE;
    }
    ptr++;

    while (isspaceW(*ptr))
        ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || !num || num > 31)
    {
        ERR("unexpected day %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wDay = (WORD)num;

    if (*ptr != '-')
    {
        ERR("unexpected month format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr++;

    for (monthPtr = month; *ptr != '-' &&
         monthPtr - month < sizeof(month) / sizeof(month[0]) - 1;
         monthPtr++, ptr++)
        *monthPtr = *ptr;
    *monthPtr = 0;
    st.wMonth = HTTP_ParseMonth(month);
    if (!st.wMonth || st.wMonth > 12)
    {
        ERR("unexpected month %s\n", debugstr_w(month));
        return FALSE;
    }

    if (*ptr != '-')
    {
        ERR("unexpected year format %s\n", debugstr_w(ptr));
        return FALSE;
    }
    ptr++;

    num = strtoulW(ptr, &nextPtr, 10);
    if (!nextPtr || nextPtr <= ptr || num < 1601 || num > 30827)
    {
        ERR("unexpected year %s\n", debugstr_w(value));
        return FALSE;
    }
    ptr = nextPtr;
    st.wYear = (WORD)num;

    if (!HTTP_ParseTime(&st, &ptr))
        return FALSE;

    while (isspaceW(*ptr))
        ptr++;

    if (strcmpW(ptr, gmt))
    {
        ERR("unexpected time zone %s\n", debugstr_w(ptr));
        return FALSE;
    }
    return SystemTimeToFileTime(&st, ft);
}

static BOOL HTTP_ParseDate(LPCWSTR value, FILETIME *ft)
{
    static const WCHAR zero[] = { '0',0 };
    BOOL ret;

    if (!strcmpW(value, zero))
    {
        ft->dwLowDateTime = ft->dwHighDateTime = 0;
        ret = TRUE;
    }
    else if (strchrW(value, ','))
    {
        ret = HTTP_ParseRfc1123Date(value, ft);
        if (!ret)
        {
            ret = HTTP_ParseRfc850Date(value, ft);
            if (!ret)
                ERR("unexpected date format %s\n", debugstr_w(value));
        }
    }
    else
    {
        ret = HTTP_ParseDateAsAsctime(value, ft);
        if (!ret)
            ERR("unexpected date format %s\n", debugstr_w(value));
    }
    return ret;
}

static void HTTP_ProcessExpires(http_request_t *request)
{
    BOOL expirationFound = FALSE;
    int headerIndex;

    /* Look for a Cache-Control header with a max-age directive, as it takes
     * precedence over the Expires header.
     */
    headerIndex = HTTP_GetCustomHeaderIndex(request, szCache_Control, 0, FALSE);
    if (headerIndex != -1)
    {
        LPHTTPHEADERW ccHeader = &request->custHeaders[headerIndex];
        LPWSTR ptr;

        for (ptr = ccHeader->lpszValue; ptr && *ptr; )
        {
            LPWSTR comma = strchrW(ptr, ','), end, equal;

            if (comma)
                end = comma;
            else
                end = ptr + strlenW(ptr);
            for (equal = end - 1; equal > ptr && *equal != '='; equal--)
                ;
            if (*equal == '=')
            {
                static const WCHAR max_age[] = {
                    'm','a','x','-','a','g','e',0 };

                if (!strncmpiW(ptr, max_age, equal - ptr - 1))
                {
                    LPWSTR nextPtr;
                    unsigned long age;

                    age = strtoulW(equal + 1, &nextPtr, 10);
                    if (nextPtr > equal + 1)
                    {
                        LARGE_INTEGER ft;

                        NtQuerySystemTime( &ft );
                        /* Age is in seconds, FILETIME resolution is in
                         * 100 nanosecond intervals.
                         */
                        ft.QuadPart += age * (ULONGLONG)1000000;
                        request->expires.dwLowDateTime = ft.u.LowPart;
                        request->expires.dwHighDateTime = ft.u.HighPart;
                        expirationFound = TRUE;
                    }
                }
            }
            if (comma)
            {
                ptr = comma + 1;
                while (isspaceW(*ptr))
                    ptr++;
            }
            else
                ptr = NULL;
        }
    }
    if (!expirationFound)
    {
        headerIndex = HTTP_GetCustomHeaderIndex(request, szExpires, 0, FALSE);
        if (headerIndex != -1)
        {
            LPHTTPHEADERW expiresHeader = &request->custHeaders[headerIndex];
            FILETIME ft;

            if (HTTP_ParseDate(expiresHeader->lpszValue, &ft))
            {
                expirationFound = TRUE;
                request->expires = ft;
            }
        }
    }
    if (!expirationFound)
    {
        LARGE_INTEGER t;

        /* With no known age, default to 10 minutes until expiration. */
        NtQuerySystemTime( &t );
        t.QuadPart += 10 * 60 * (ULONGLONG)10000000;
        request->expires.dwLowDateTime = t.u.LowPart;
        request->expires.dwHighDateTime = t.u.HighPart;
    }
}

static void HTTP_ProcessLastModified(http_request_t *request)
{
    int headerIndex;

    headerIndex = HTTP_GetCustomHeaderIndex(request, szLast_Modified, 0, FALSE);
    if (headerIndex != -1)
    {
        LPHTTPHEADERW expiresHeader = &request->custHeaders[headerIndex];
        FILETIME ft;

        if (HTTP_ParseDate(expiresHeader->lpszValue, &ft))
            request->last_modified = ft;
    }
}

static void http_process_keep_alive(http_request_t *req)
{
    int index;

    index = HTTP_GetCustomHeaderIndex(req, szConnection, 0, FALSE);
    if(index != -1)
        req->netconn->keep_alive = !strcmpiW(req->custHeaders[index].lpszValue, szKeepAlive);
    else
        req->netconn->keep_alive = !strcmpiW(req->version, g_szHttp1_1);
}

static void HTTP_CacheRequest(http_request_t *request)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    WCHAR cacheFileName[MAX_PATH+1];
    BOOL b;

    b = HTTP_GetRequestURL(request, url);
    if(!b) {
        WARN("Could not get URL\n");
        return;
    }

    b = CreateUrlCacheEntryW(url, request->contentLength, NULL, cacheFileName, 0);
    if(b) {
        heap_free(request->cacheFile);
        CloseHandle(request->hCacheFile);

        request->cacheFile = heap_strdupW(cacheFileName);
        request->hCacheFile = CreateFileW(request->cacheFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(request->hCacheFile == INVALID_HANDLE_VALUE) {
            WARN("Could not create file: %u\n", GetLastError());
            request->hCacheFile = NULL;
        }
    }else {
        WARN("Could not create cache entry: %08x\n", GetLastError());
    }
}

static DWORD open_http_connection(http_request_t *request, BOOL *reusing)
{
    const BOOL is_https = (request->hdr.dwFlags & INTERNET_FLAG_SECURE) != 0;
    http_session_t *session = request->session;
    netconn_t *netconn = NULL;
    server_t *server;
    DWORD res;

    assert(!request->netconn);
    reset_data_stream(request);

    server = get_server(session->serverName, session->serverPort);
    if(!server)
        return ERROR_OUTOFMEMORY;

    res = HTTP_ResolveName(request, server);
    if(res != ERROR_SUCCESS) {
        server_release(server);
        return res;
    }

    EnterCriticalSection(&connection_pool_cs);

    while(!list_empty(&server->conn_pool)) {
        netconn = LIST_ENTRY(list_head(&server->conn_pool), netconn_t, pool_entry);
        list_remove(&netconn->pool_entry);

        if(NETCON_is_alive(netconn))
            break;

        TRACE("connection %p closed during idle\n", netconn);
        free_netconn(netconn);
        netconn = NULL;
    }

    LeaveCriticalSection(&connection_pool_cs);

    if(netconn) {
        TRACE("<-- reusing %p netconn\n", netconn);
        request->netconn = netconn;
        *reusing = TRUE;
        return ERROR_SUCCESS;
    }

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                          INTERNET_STATUS_CONNECTING_TO_SERVER,
                          server->addr_str,
                          strlen(server->addr_str)+1);

    res = create_netconn(is_https, server, request->security_flags, request->connect_timeout, &netconn);
    server_release(server);
    if(res != ERROR_SUCCESS) {
        ERR("create_netconn failed: %u\n", res);
        return res;
    }

    request->netconn = netconn;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
            INTERNET_STATUS_CONNECTED_TO_SERVER,
            server->addr_str, strlen(server->addr_str)+1);

    if(is_https) {
        /* Note: we differ from Microsoft's WinINet here. they seem to have
         * a bug that causes no status callbacks to be sent when starting
         * a tunnel to a proxy server using the CONNECT verb. i believe our
         * behaviour to be more correct and to not cause any incompatibilities
         * because using a secure connection through a proxy server is a rare
         * case that would be hard for anyone to depend on */
        if(session->appInfo->proxy)
            res = HTTP_SecureProxyConnect(request);
        if(res == ERROR_SUCCESS)
            res = NETCON_secure_connect(request->netconn);
        if(res != ERROR_SUCCESS)
        {
            WARN("Couldn't connect securely to host\n");

            if((request->hdr.ErrorMask&INTERNET_ERROR_MASK_COMBINED_SEC_CERT) && (
                    res == ERROR_INTERNET_SEC_CERT_DATE_INVALID
                    || res == ERROR_INTERNET_INVALID_CA
                    || res == ERROR_INTERNET_SEC_CERT_NO_REV
                    || res == ERROR_INTERNET_SEC_CERT_REV_FAILED
                    || res == ERROR_INTERNET_SEC_CERT_REVOKED
                    || res == ERROR_INTERNET_SEC_INVALID_CERT
                    || res == ERROR_INTERNET_SEC_CERT_CN_INVALID))
                res = ERROR_INTERNET_SEC_CERT_ERRORS;
        }
    }

    if(res != ERROR_SUCCESS) {
        http_release_netconn(request, FALSE);
        return res;
    }

    *reusing = FALSE;
    TRACE("Created connection to %s: %p\n", debugstr_w(server->name), netconn);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_HttpSendRequestW (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    ERROR_SUCCESS on success
 *    win32 error code on failure
 *
 */
static DWORD HTTP_HttpSendRequestW(http_request_t *request, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
	DWORD dwContentLength, BOOL bEndRequest)
{
    INT cnt;
    BOOL redirected = FALSE;
    LPWSTR requestString = NULL;
    INT responseLen;
    BOOL loop_next;
    static const WCHAR szPost[] = { 'P','O','S','T',0 };
    static const WCHAR szContentLength[] =
        { 'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','%','l','i','\r','\n',0 };
    WCHAR contentLengthStr[sizeof szContentLength/2 /* includes \r\n */ + 20 /* int */ ];
    DWORD res;

    TRACE("--> %p\n", request);

    assert(request->hdr.htype == WH_HHTTPREQ);

    /* if the verb is NULL default to GET */
    if (!request->verb)
        request->verb = heap_strdupW(szGET);

    if (dwContentLength || strcmpW(request->verb, szGET))
    {
        sprintfW(contentLengthStr, szContentLength, dwContentLength);
        HTTP_HttpAddRequestHeadersW(request, contentLengthStr, -1L, HTTP_ADDREQ_FLAG_REPLACE);
        request->bytesToWrite = dwContentLength;
    }
    if (request->session->appInfo->agent)
    {
        WCHAR *agent_header;
        static const WCHAR user_agent[] = {'U','s','e','r','-','A','g','e','n','t',':',' ','%','s','\r','\n',0};
        int len;

        len = strlenW(request->session->appInfo->agent) + strlenW(user_agent);
        agent_header = heap_alloc(len * sizeof(WCHAR));
        sprintfW(agent_header, user_agent, request->session->appInfo->agent);

        HTTP_HttpAddRequestHeadersW(request, agent_header, strlenW(agent_header), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
        heap_free(agent_header);
    }
    if (request->hdr.dwFlags & INTERNET_FLAG_PRAGMA_NOCACHE)
    {
        static const WCHAR pragma_nocache[] = {'P','r','a','g','m','a',':',' ','n','o','-','c','a','c','h','e','\r','\n',0};
        HTTP_HttpAddRequestHeadersW(request, pragma_nocache, strlenW(pragma_nocache), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }
    if ((request->hdr.dwFlags & INTERNET_FLAG_NO_CACHE_WRITE) && !strcmpW(request->verb, szPost))
    {
        static const WCHAR cache_control[] = {'C','a','c','h','e','-','C','o','n','t','r','o','l',':',
                                              ' ','n','o','-','c','a','c','h','e','\r','\n',0};
        HTTP_HttpAddRequestHeadersW(request, cache_control, strlenW(cache_control), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }

    /* add the headers the caller supplied */
    if( lpszHeaders && dwHeaderLength )
        HTTP_HttpAddRequestHeadersW(request, lpszHeaders, dwHeaderLength, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REPLACE);

    do
    {
        DWORD len;
        BOOL reusing_connection;
        char *ascii_req;

        loop_next = FALSE;

        /* like native, just in case the caller forgot to call InternetReadFile
         * for all the data */
        drain_content(request);
        if(redirected) {
            request->contentLength = ~0u;
            request->bytesToWrite = 0;
        }

        if (TRACE_ON(wininet))
        {
            LPHTTPHEADERW Host = HTTP_GetHeader(request, hostW);
            TRACE("Going to url %s %s\n", debugstr_w(Host->lpszValue), debugstr_w(request->path));
        }

        HTTP_FixURL(request);
        if (request->hdr.dwFlags & INTERNET_FLAG_KEEP_CONNECTION)
        {
            HTTP_ProcessHeader(request, szConnection, szKeepAlive, HTTP_ADDHDR_FLAG_REQ | HTTP_ADDHDR_FLAG_REPLACE);
        }
        HTTP_InsertAuthorization(request, request->authInfo, szAuthorization);
        HTTP_InsertAuthorization(request, request->proxyAuthInfo, szProxy_Authorization);

        if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES))
            HTTP_InsertCookies(request);

        if (request->session->appInfo->proxy && request->session->appInfo->proxy[0])
        {
            WCHAR *url = HTTP_BuildProxyRequestUrl(request);
            requestString = HTTP_BuildHeaderRequestString(request, request->verb, url, request->version);
            heap_free(url);
        }
        else
            requestString = HTTP_BuildHeaderRequestString(request, request->verb, request->path, request->version);

 
        TRACE("Request header -> %s\n", debugstr_w(requestString) );

        if ((res = open_http_connection(request, &reusing_connection)) != ERROR_SUCCESS)
            break;

        /* send the request as ASCII, tack on the optional data */
        if (!lpOptional || redirected)
            dwOptionalLength = 0;
        len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                   NULL, 0, NULL, NULL );
        ascii_req = heap_alloc(len + dwOptionalLength);
        WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                             ascii_req, len, NULL, NULL );
        if( lpOptional )
            memcpy( &ascii_req[len-1], lpOptional, dwOptionalLength );
        len = (len + dwOptionalLength - 1);
        ascii_req[len] = 0;
        TRACE("full request -> %s\n", debugstr_a(ascii_req) );

        INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                              INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        NETCON_set_timeout( request->netconn, TRUE, request->send_timeout );
        res = NETCON_send(request->netconn, ascii_req, len, 0, &cnt);
        heap_free( ascii_req );
        if(res != ERROR_SUCCESS) {
            TRACE("send failed: %u\n", res);
            if(!reusing_connection)
                break;
            http_release_netconn(request, FALSE);
            loop_next = TRUE;
            continue;
        }

        request->bytesWritten = dwOptionalLength;

        INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                              INTERNET_STATUS_REQUEST_SENT,
                              &len, sizeof(DWORD));

        if (bEndRequest)
        {
            DWORD dwBufferSize;

            INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);
    
            responseLen = HTTP_GetResponseHeaders(request, TRUE);
            /* FIXME: We should know that connection is closed before sending
             * headers. Otherwise wrong callbacks are executed */
            if(!responseLen && reusing_connection) {
                TRACE("Connection closed by server, reconnecting\n");
                http_release_netconn(request, FALSE);
                loop_next = TRUE;
                continue;
            }

            INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                                INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen,
                                sizeof(DWORD));

            http_process_keep_alive(request);
            HTTP_ProcessCookies(request);
            HTTP_ProcessExpires(request);
            HTTP_ProcessLastModified(request);

            res = set_content_length(request);
            if(res != ERROR_SUCCESS)
                goto lend;
            if(!request->contentLength)
                http_release_netconn(request, TRUE);

            if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && responseLen)
            {
                WCHAR *new_url, szNewLocation[INTERNET_MAX_URL_LENGTH];
                dwBufferSize=sizeof(szNewLocation);
                switch(request->status_code) {
                case HTTP_STATUS_REDIRECT:
                case HTTP_STATUS_MOVED:
                case HTTP_STATUS_REDIRECT_KEEP_VERB:
                case HTTP_STATUS_REDIRECT_METHOD:
                    if(HTTP_HttpQueryInfoW(request,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,NULL) != ERROR_SUCCESS)
                        break;

                    if (strcmpW(request->verb, szGET) && strcmpW(request->verb, szHEAD) &&
                        request->status_code != HTTP_STATUS_REDIRECT_KEEP_VERB)
                    {
                        heap_free(request->verb);
                        request->verb = heap_strdupW(szGET);
                    }
                    drain_content(request);
                    if ((new_url = HTTP_GetRedirectURL( request, szNewLocation )))
                    {
                        INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_REDIRECT,
                                              new_url, (strlenW(new_url) + 1) * sizeof(WCHAR));
                        res = HTTP_HandleRedirect(request, new_url);
                        if (res == ERROR_SUCCESS)
                        {
                            heap_free(requestString);
                            loop_next = TRUE;
                        }
                        heap_free( new_url );
                    }
                    redirected = TRUE;
                }
            }
            if (!(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTH) && res == ERROR_SUCCESS)
            {
                WCHAR szAuthValue[2048];
                dwBufferSize=2048;
                if (request->status_code == HTTP_STATUS_DENIED)
                {
                    LPHTTPHEADERW Host = HTTP_GetHeader(request, hostW);
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(request,HTTP_QUERY_WWW_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(request, szAuthValue,
                                                 &request->authInfo,
                                                 request->session->userName,
                                                 request->session->password,
                                                 Host->lpszValue))
                        {
                            heap_free(requestString);
                            loop_next = TRUE;
                            break;
                        }
                    }

                    if(!loop_next) {
                        TRACE("Cleaning wrong authorization data\n");
                        destroy_authinfo(request->authInfo);
                        request->authInfo = NULL;
                    }
                }
                if (request->status_code == HTTP_STATUS_PROXY_AUTH_REQ)
                {
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(request,HTTP_QUERY_PROXY_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(request, szAuthValue,
                                                 &request->proxyAuthInfo,
                                                 request->session->appInfo->proxyUsername,
                                                 request->session->appInfo->proxyPassword,
                                                 NULL))
                        {
                            loop_next = TRUE;
                            break;
                        }
                    }

                    if(!loop_next) {
                        TRACE("Cleaning wrong proxy authorization data\n");
                        destroy_authinfo(request->proxyAuthInfo);
                        request->proxyAuthInfo = NULL;
                    }
                }
            }
        }
        else
            res = ERROR_SUCCESS;
    }
    while (loop_next);

    if(res == ERROR_SUCCESS)
        HTTP_CacheRequest(request);

lend:
    heap_free(requestString);

    /* TODO: send notification for P3P header */

    if (request->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        if (res == ERROR_SUCCESS) {
            if(bEndRequest && request->contentLength && request->bytesWritten == request->bytesToWrite)
                HTTP_ReceiveRequestData(request, TRUE);
            else
                send_request_complete(request,
                        request->session->hdr.dwInternalFlags & INET_OPENURL ? (DWORD_PTR)request->hdr.hInternet : 1, 0);
        }else {
                send_request_complete(request, 0, res);
        }
    }

    TRACE("<--\n");
    return res;
}

/***********************************************************************
 *
 * Helper functions for the HttpSendRequest(Ex) functions
 *
 */
static void AsyncHttpSendRequestProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_HTTPSENDREQUESTW const *req = &workRequest->u.HttpSendRequestW;
    http_request_t *request = (http_request_t*) workRequest->hdr;

    TRACE("%p\n", request);

    HTTP_HttpSendRequestW(request, req->lpszHeader,
            req->dwHeaderLength, req->lpOptional, req->dwOptionalLength,
            req->dwContentLength, req->bEndRequest);

    heap_free(req->lpszHeader);
}


static DWORD HTTP_HttpEndRequestW(http_request_t *request, DWORD dwFlags, DWORD_PTR dwContext)
{
    DWORD dwBufferSize;
    INT responseLen;
    DWORD res = ERROR_SUCCESS;

    if(!request->netconn) {
        WARN("Not connected\n");
        send_request_complete(request, 0, ERROR_INTERNET_OPERATION_CANCELLED);
        return ERROR_INTERNET_OPERATION_CANCELLED;
    }

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                  INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    responseLen = HTTP_GetResponseHeaders(request, TRUE);
    if (!responseLen)
        res = ERROR_HTTP_HEADER_NOT_FOUND;

    INTERNET_SendCallback(&request->hdr, request->hdr.dwContext,
                  INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen, sizeof(DWORD));

    /* process cookies here. Is this right? */
    http_process_keep_alive(request);
    HTTP_ProcessCookies(request);
    HTTP_ProcessExpires(request);
    HTTP_ProcessLastModified(request);

    if ((res = set_content_length(request)) == ERROR_SUCCESS) {
        if(!request->contentLength)
            http_release_netconn(request, TRUE);
    }

    if (res == ERROR_SUCCESS && !(request->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT))
    {
        switch(request->status_code) {
        case HTTP_STATUS_REDIRECT:
        case HTTP_STATUS_MOVED:
        case HTTP_STATUS_REDIRECT_METHOD:
        case HTTP_STATUS_REDIRECT_KEEP_VERB: {
            WCHAR *new_url, szNewLocation[INTERNET_MAX_URL_LENGTH];
            dwBufferSize=sizeof(szNewLocation);
            if (HTTP_HttpQueryInfoW(request, HTTP_QUERY_LOCATION, szNewLocation, &dwBufferSize, NULL) != ERROR_SUCCESS)
                break;

            if (strcmpW(request->verb, szGET) && strcmpW(request->verb, szHEAD) &&
                request->status_code != HTTP_STATUS_REDIRECT_KEEP_VERB)
            {
                heap_free(request->verb);
                request->verb = heap_strdupW(szGET);
            }
            drain_content(request);
            if ((new_url = HTTP_GetRedirectURL( request, szNewLocation )))
            {
                INTERNET_SendCallback(&request->hdr, request->hdr.dwContext, INTERNET_STATUS_REDIRECT,
                                      new_url, (strlenW(new_url) + 1) * sizeof(WCHAR));
                res = HTTP_HandleRedirect(request, new_url);
                if (res == ERROR_SUCCESS)
                    res = HTTP_HttpSendRequestW(request, NULL, 0, NULL, 0, 0, TRUE);
                heap_free( new_url );
            }
        }
        }
    }

    if (res == ERROR_SUCCESS && request->contentLength)
        HTTP_ReceiveRequestData(request, TRUE);
    else
        send_request_complete(request, res == ERROR_SUCCESS, res);

    return res;
}

/***********************************************************************
 *           HttpEndRequestA (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestA(HINTERNET hRequest,
        LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    TRACE("(%p, %p, %08x, %08lx)\n", hRequest, lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return HttpEndRequestW(hRequest, NULL, dwFlags, dwContext);
}

static void AsyncHttpEndRequestProc(WORKREQUEST *work)
{
    struct WORKREQ_HTTPENDREQUESTW const *req = &work->u.HttpEndRequestW;
    http_request_t *request = (http_request_t*)work->hdr;

    TRACE("%p\n", request);

    HTTP_HttpEndRequestW(request, req->dwFlags, req->dwContext);
}

/***********************************************************************
 *           HttpEndRequestW (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestW(HINTERNET hRequest,
        LPINTERNET_BUFFERSW lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *request;
    DWORD res;

    TRACE("%p %p %x %lx -->\n", hRequest, lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    request = (http_request_t*) get_handle_object( hRequest );

    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        if (request)
            WININET_Release( &request->hdr );
        return FALSE;
    }
    request->hdr.dwFlags |= dwFlags;

    if (request->session->appInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST work;
        struct WORKREQ_HTTPENDREQUESTW *work_endrequest;

        work.asyncproc = AsyncHttpEndRequestProc;
        work.hdr = WININET_AddRef( &request->hdr );

        work_endrequest = &work.u.HttpEndRequestW;
        work_endrequest->dwFlags = dwFlags;
        work_endrequest->dwContext = dwContext;

        INTERNET_AsyncCall(&work);
        res = ERROR_IO_PENDING;
    }
    else
        res = HTTP_HttpEndRequestW(request, dwFlags, dwContext);

    WININET_Release( &request->hdr );
    TRACE("%u <--\n", res);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestExA (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExA(HINTERNET hRequest,
			       LPINTERNET_BUFFERSA lpBuffersIn,
			       LPINTERNET_BUFFERSA lpBuffersOut,
			       DWORD dwFlags, DWORD_PTR dwContext)
{
    INTERNET_BUFFERSW BuffersInW;
    BOOL rc = FALSE;
    DWORD headerlen;
    LPWSTR header = NULL;

    TRACE("(%p, %p, %p, %08x, %08lx)\n", hRequest, lpBuffersIn,
	    lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersIn)
    {
        BuffersInW.dwStructSize = sizeof(LPINTERNET_BUFFERSW);
        if (lpBuffersIn->lpcszHeader)
        {
            headerlen = MultiByteToWideChar(CP_ACP,0,lpBuffersIn->lpcszHeader,
                    lpBuffersIn->dwHeadersLength,0,0);
            header = heap_alloc(headerlen*sizeof(WCHAR));
            if (!(BuffersInW.lpcszHeader = header))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            BuffersInW.dwHeadersLength = MultiByteToWideChar(CP_ACP, 0,
                    lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                    header, headerlen);
        }
        else
            BuffersInW.lpcszHeader = NULL;
        BuffersInW.dwHeadersTotal = lpBuffersIn->dwHeadersTotal;
        BuffersInW.lpvBuffer = lpBuffersIn->lpvBuffer;
        BuffersInW.dwBufferLength = lpBuffersIn->dwBufferLength;
        BuffersInW.dwBufferTotal = lpBuffersIn->dwBufferTotal;
        BuffersInW.Next = NULL;
    }

    rc = HttpSendRequestExW(hRequest, lpBuffersIn ? &BuffersInW : NULL, NULL, dwFlags, dwContext);

    heap_free(header);
    return rc;
}

/***********************************************************************
 *           HttpSendRequestExW (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExW(HINTERNET hRequest,
                   LPINTERNET_BUFFERSW lpBuffersIn,
                   LPINTERNET_BUFFERSW lpBuffersOut,
                   DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *request;
    http_session_t *session;
    appinfo_t *hIC;
    DWORD res;

    TRACE("(%p, %p, %p, %08x, %08lx)\n", hRequest, lpBuffersIn,
            lpBuffersOut, dwFlags, dwContext);

    request = (http_request_t*) get_handle_object( hRequest );

    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    session = request->session;
    assert(session->hdr.htype == WH_HHTTPSESSION);
    hIC = session->appInfo;
    assert(hIC->hdr.htype == WH_HINIT);

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncproc = AsyncHttpSendRequestProc;
        workRequest.hdr = WININET_AddRef( &request->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpBuffersIn)
        {
            DWORD size = 0;

            if (lpBuffersIn->lpcszHeader)
            {
                if (lpBuffersIn->dwHeadersLength == ~0u)
                    size = (strlenW( lpBuffersIn->lpcszHeader ) + 1) * sizeof(WCHAR);
                else
                    size = lpBuffersIn->dwHeadersLength * sizeof(WCHAR);

                req->lpszHeader = heap_alloc(size);
                memcpy( req->lpszHeader, lpBuffersIn->lpcszHeader, size );
            }
            else req->lpszHeader = NULL;

            req->dwHeaderLength = size / sizeof(WCHAR);
            req->lpOptional = lpBuffersIn->lpvBuffer;
            req->dwOptionalLength = lpBuffersIn->dwBufferLength;
            req->dwContentLength = lpBuffersIn->dwBufferTotal;
        }
        else
        {
            req->lpszHeader = NULL;
            req->dwHeaderLength = 0;
            req->lpOptional = NULL;
            req->dwOptionalLength = 0;
            req->dwContentLength = 0;
        }

        req->bEndRequest = FALSE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        res = ERROR_IO_PENDING;
    }
    else
    {
        if (lpBuffersIn)
            res = HTTP_HttpSendRequestW(request, lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                                        lpBuffersIn->lpvBuffer, lpBuffersIn->dwBufferLength,
                                        lpBuffersIn->dwBufferTotal, FALSE);
        else
            res = HTTP_HttpSendRequestW(request, NULL, 0, NULL, 0, 0, FALSE);
    }

lend:
    if ( request )
        WININET_Release( &request->hdr );

    TRACE("<---\n");
    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestW (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    http_request_t *request;
    http_session_t *session = NULL;
    appinfo_t *hIC = NULL;
    DWORD res = ERROR_SUCCESS;

    TRACE("%p, %s, %i, %p, %i)\n", hHttpRequest,
            debugstr_wn(lpszHeaders, dwHeaderLength), dwHeaderLength, lpOptional, dwOptionalLength);

    request = (http_request_t*) get_handle_object( hHttpRequest );
    if (NULL == request || request->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    session = request->session;
    if (NULL == session ||  session->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    hIC = session->appInfo;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncproc = AsyncHttpSendRequestProc;
        workRequest.hdr = WININET_AddRef( &request->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpszHeaders)
        {
            DWORD size;

            if (dwHeaderLength == ~0u) size = (strlenW(lpszHeaders) + 1) * sizeof(WCHAR);
            else size = dwHeaderLength * sizeof(WCHAR);

            req->lpszHeader = heap_alloc(size);
            memcpy(req->lpszHeader, lpszHeaders, size);
        }
        else
            req->lpszHeader = 0;
        req->dwHeaderLength = dwHeaderLength;
        req->lpOptional = lpOptional;
        req->dwOptionalLength = dwOptionalLength;
        req->dwContentLength = dwOptionalLength;
        req->bEndRequest = TRUE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        res = ERROR_IO_PENDING;
    }
    else
    {
	res = HTTP_HttpSendRequestW(request, lpszHeaders,
		dwHeaderLength, lpOptional, dwOptionalLength,
		dwOptionalLength, TRUE);
    }
lend:
    if( request )
        WININET_Release( &request->hdr );

    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    LPWSTR szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        nLen=MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0);
        szHeaders = heap_alloc(nLen*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen);
    }
    result = HttpSendRequestW(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    heap_free(szHeaders);
    return result;
}

/***********************************************************************
 *           HTTPSESSION_Destroy (internal)
 *
 * Deallocate session handle
 *
 */
static void HTTPSESSION_Destroy(object_header_t *hdr)
{
    http_session_t *session = (http_session_t*) hdr;

    TRACE("%p\n", session);

    WININET_Release(&session->appInfo->hdr);

    heap_free(session->hostName);
    heap_free(session->serverName);
    heap_free(session->password);
    heap_free(session->userName);
}

static DWORD HTTPSESSION_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    http_session_t *ses = (http_session_t *)hdr;

    switch(option) {
    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_CONNECT_HTTP;
        return ERROR_SUCCESS;
    case INTERNET_OPTION_CONNECT_TIMEOUT:
        TRACE("INTERNET_OPTION_CONNECT_TIMEOUT\n");

        if (*size < sizeof(DWORD))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD *)buffer = ses->connect_timeout;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_SEND_TIMEOUT:
        TRACE("INTERNET_OPTION_SEND_TIMEOUT\n");

        if (*size < sizeof(DWORD))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD *)buffer = ses->send_timeout;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_RECEIVE_TIMEOUT:
        TRACE("INTERNET_OPTION_RECEIVE_TIMEOUT\n");

        if (*size < sizeof(DWORD))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD *)buffer = ses->receive_timeout;
        return ERROR_SUCCESS;
    }

    return INET_QueryOption(hdr, option, buffer, size, unicode);
}

static DWORD HTTPSESSION_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_session_t *ses = (http_session_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_USERNAME:
    {
        heap_free(ses->userName);
        if (!(ses->userName = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_PASSWORD:
    {
        heap_free(ses->password);
        if (!(ses->password = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_CONNECT_TIMEOUT:
    {
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        ses->connect_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_SEND_TIMEOUT:
    {
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        ses->send_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_RECEIVE_TIMEOUT:
    {
        if (!buffer || size != sizeof(DWORD)) return ERROR_INVALID_PARAMETER;
        ses->receive_timeout = *(DWORD *)buffer;
        return ERROR_SUCCESS;
    }
    default: break;
    }

    return INET_SetOption(hdr, option, buffer, size);
}

static const object_vtbl_t HTTPSESSIONVtbl = {
    HTTPSESSION_Destroy,
    NULL,
    HTTPSESSION_QueryOption,
    HTTPSESSION_SetOption,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


/***********************************************************************
 *           HTTP_Connect  (internal)
 *
 * Create http session handle
 *
 * RETURNS
 *   HINTERNET a session handle on success
 *   NULL on failure
 *
 */
DWORD HTTP_Connect(appinfo_t *hIC, LPCWSTR lpszServerName,
        INTERNET_PORT serverPort, LPCWSTR lpszUserName,
        LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
        DWORD dwInternalFlags, HINTERNET *ret)
{
    http_session_t *session = NULL;

    TRACE("-->\n");

    if (!lpszServerName || !lpszServerName[0])
        return ERROR_INVALID_PARAMETER;

    assert( hIC->hdr.htype == WH_HINIT );

    session = alloc_object(&hIC->hdr, &HTTPSESSIONVtbl, sizeof(http_session_t));
    if (!session)
        return ERROR_OUTOFMEMORY;

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    session->hdr.htype = WH_HHTTPSESSION;
    session->hdr.dwFlags = dwFlags;
    session->hdr.dwContext = dwContext;
    session->hdr.dwInternalFlags |= dwInternalFlags;

    WININET_AddRef( &hIC->hdr );
    session->appInfo = hIC;
    list_add_head( &hIC->hdr.children, &session->hdr.entry );

    if(hIC->proxy && hIC->accessType == INTERNET_OPEN_TYPE_PROXY) {
        if(hIC->proxyBypass)
            FIXME("Proxy bypass is ignored.\n");
    }
    session->serverName = heap_strdupW(lpszServerName);
    session->hostName = heap_strdupW(lpszServerName);
    if (lpszUserName && lpszUserName[0])
        session->userName = heap_strdupW(lpszUserName);
    if (lpszPassword && lpszPassword[0])
        session->password = heap_strdupW(lpszPassword);
    session->serverPort = serverPort;
    session->hostPort = serverPort;
    session->connect_timeout = INFINITE;
    session->send_timeout = INFINITE;
    session->receive_timeout = INFINITE;

    /* Don't send a handle created callback if this handle was created with InternetOpenUrl */
    if (!(session->hdr.dwInternalFlags & INET_OPENURL))
    {
        INTERNET_SendCallback(&hIC->hdr, dwContext,
                              INTERNET_STATUS_HANDLE_CREATED, &session->hdr.hInternet,
                              sizeof(HINTERNET));
    }

/*
 * an INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p --> %p\n", hIC, session);

    *ret = session->hdr.hInternet;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_clear_response_headers (internal)
 *
 * clear out any old response headers
 */
static void HTTP_clear_response_headers( http_request_t *request )
{
    DWORD i;

    for( i=0; i<request->nCustHeaders; i++)
    {
        if( !request->custHeaders[i].lpszField )
            continue;
        if( !request->custHeaders[i].lpszValue )
            continue;
        if ( request->custHeaders[i].wFlags & HDR_ISREQUEST )
            continue;
        HTTP_DeleteCustomHeader( request, i );
        i--;
    }
}

/***********************************************************************
 *           HTTP_GetResponseHeaders (internal)
 *
 * Read server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
static INT HTTP_GetResponseHeaders(http_request_t *request, BOOL clear)
{
    INT cbreaks = 0;
    WCHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    BOOL bSuccess = FALSE;
    INT  rc = 0;
    char bufferA[MAX_REPLY_LEN];
    LPWSTR status_code = NULL, status_text = NULL;
    DWORD cchMaxRawHeaders = 1024;
    LPWSTR lpszRawHeaders = NULL;
    LPWSTR temp;
    DWORD cchRawHeaders = 0;
    BOOL codeHundred = FALSE;

    TRACE("-->\n");

    if(!request->netconn)
        goto lend;

    NETCON_set_timeout( request->netconn, FALSE, request->receive_timeout );
    do {
        static const WCHAR szHundred[] = {'1','0','0',0};
        /*
         * We should first receive 'HTTP/1.x nnn OK' where nnn is the status code.
         */
        buflen = MAX_REPLY_LEN;
        if (!read_line(request, bufferA, &buflen))
            goto lend;

        /* clear old response headers (eg. from a redirect response) */
        if (clear) {
            HTTP_clear_response_headers( request );
            clear = FALSE;
        }

        rc += buflen;
        MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );
        /* check is this a status code line? */
        if (!strncmpW(buffer, g_szHttp1_0, 4))
        {
            /* split the version from the status code */
            status_code = strchrW( buffer, ' ' );
            if( !status_code )
                goto lend;
            *status_code++=0;

            /* split the status code from the status text */
            status_text = strchrW( status_code, ' ' );
            if( !status_text )
                goto lend;
            *status_text++=0;

            request->status_code = atoiW(status_code);

            TRACE("version [%s] status code [%s] status text [%s]\n",
               debugstr_w(buffer), debugstr_w(status_code), debugstr_w(status_text) );

            codeHundred = (!strcmpW(status_code, szHundred));
        }
        else if (!codeHundred)
        {
            WARN("No status line at head of response (%s)\n", debugstr_w(buffer));

            heap_free(request->version);
            heap_free(request->statusText);

            request->status_code = HTTP_STATUS_OK;
            request->version = heap_strdupW(g_szHttp1_0);
            request->statusText = heap_strdupW(szOK);

            heap_free(request->rawHeaders);
            request->rawHeaders = heap_strdupW(szDefaultHeader);

            bSuccess = TRUE;
            goto lend;
        }
    } while (codeHundred);

    /* Add status code */
    HTTP_ProcessHeader(request, szStatus, status_code,
            HTTP_ADDHDR_FLAG_REPLACE);

    heap_free(request->version);
    heap_free(request->statusText);

    request->version = heap_strdupW(buffer);
    request->statusText = heap_strdupW(status_text);

    /* Restore the spaces */
    *(status_code-1) = ' ';
    *(status_text-1) = ' ';

    /* regenerate raw headers */
    lpszRawHeaders = heap_alloc((cchMaxRawHeaders + 1) * sizeof(WCHAR));
    if (!lpszRawHeaders) goto lend;

    while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
        cchMaxRawHeaders *= 2;
    temp = heap_realloc(lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
    if (temp == NULL) goto lend;
    lpszRawHeaders = temp;
    memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
    cchRawHeaders += (buflen-1);
    memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
    cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
    lpszRawHeaders[cchRawHeaders] = '\0';

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
        if (read_line(request, bufferA, &buflen))
        {
            LPWSTR * pFieldAndValue;

            TRACE("got line %s, now interpreting\n", debugstr_a(bufferA));

            if (!bufferA[0]) break;
            MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

            pFieldAndValue = HTTP_InterpretHttpHeader(buffer);
            if (pFieldAndValue)
            {
                while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
                    cchMaxRawHeaders *= 2;
                temp = heap_realloc(lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
                if (temp == NULL) goto lend;
                lpszRawHeaders = temp;
                memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
                cchRawHeaders += (buflen-1);
                memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
                cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
                lpszRawHeaders[cchRawHeaders] = '\0';

                HTTP_ProcessHeader(request, pFieldAndValue[0], pFieldAndValue[1],
                                   HTTP_ADDREQ_FLAG_ADD );

                HTTP_FreeTokens(pFieldAndValue);
            }
        }
	else
	{
	    cbreaks++;
	    if (cbreaks >= 2)
	       break;
	}
    }while(1);

    /* make sure the response header is terminated with an empty line.  Some apps really
       truly care about that empty line being there for some reason.  Just add it to the
       header. */
    if (cchRawHeaders + strlenW(szCrLf) > cchMaxRawHeaders)
    {
        cchMaxRawHeaders = cchRawHeaders + strlenW(szCrLf);
        temp = heap_realloc(lpszRawHeaders, (cchMaxRawHeaders + 1) * sizeof(WCHAR));
        if (temp == NULL) goto lend;
        lpszRawHeaders = temp;
    }

    memcpy(&lpszRawHeaders[cchRawHeaders], szCrLf, sizeof(szCrLf));

    heap_free(request->rawHeaders);
    request->rawHeaders = lpszRawHeaders;
    TRACE("raw headers: %s\n", debugstr_w(lpszRawHeaders));
    bSuccess = TRUE;

lend:

    TRACE("<--\n");
    if (bSuccess)
        return rc;
    else
    {
        heap_free(lpszRawHeaders);
        return 0;
    }
}

/***********************************************************************
 *           HTTP_InterpretHttpHeader (internal)
 *
 * Parse server response
 *
 * RETURNS
 *
 *   Pointer to array of field, value, NULL on success.
 *   NULL on error.
 */
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer)
{
    LPWSTR * pTokenPair;
    LPWSTR pszColon;
    INT len;

    pTokenPair = heap_alloc_zero(sizeof(*pTokenPair)*3);

    pszColon = strchrW(buffer, ':');
    /* must have two tokens */
    if (!pszColon)
    {
        HTTP_FreeTokens(pTokenPair);
        if (buffer[0])
            TRACE("No ':' in line: %s\n", debugstr_w(buffer));
        return NULL;
    }

    pTokenPair[0] = heap_alloc((pszColon - buffer + 1) * sizeof(WCHAR));
    if (!pTokenPair[0])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[0], buffer, (pszColon - buffer) * sizeof(WCHAR));
    pTokenPair[0][pszColon - buffer] = '\0';

    /* skip colon */
    pszColon++;
    len = strlenW(pszColon);
    pTokenPair[1] = heap_alloc((len + 1) * sizeof(WCHAR));
    if (!pTokenPair[1])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[1], pszColon, (len + 1) * sizeof(WCHAR));

    strip_spaces(pTokenPair[0]);
    strip_spaces(pTokenPair[1]);

    TRACE("field(%s) Value(%s)\n", debugstr_w(pTokenPair[0]), debugstr_w(pTokenPair[1]));
    return pTokenPair;
}

/***********************************************************************
 *           HTTP_ProcessHeader (internal)
 *
 * Stuff header into header tables according to <dwModifier>
 *
 */

#define COALESCEFLAGS (HTTP_ADDHDR_FLAG_COALESCE|HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)

static DWORD HTTP_ProcessHeader(http_request_t *request, LPCWSTR field, LPCWSTR value, DWORD dwModifier)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    INT index = -1;
    BOOL request_only = dwModifier & HTTP_ADDHDR_FLAG_REQ;
    DWORD res = ERROR_HTTP_INVALID_HEADER;

    TRACE("--> %s: %s - 0x%08x\n", debugstr_w(field), debugstr_w(value), dwModifier);

    /* REPLACE wins out over ADD */
    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
        dwModifier &= ~HTTP_ADDHDR_FLAG_ADD;
    
    if (dwModifier & HTTP_ADDHDR_FLAG_ADD)
        index = -1;
    else
        index = HTTP_GetCustomHeaderIndex(request, field, 0, request_only);

    if (index >= 0)
    {
        if (dwModifier & HTTP_ADDHDR_FLAG_ADD_IF_NEW)
            return ERROR_HTTP_INVALID_HEADER;
        lphttpHdr = &request->custHeaders[index];
    }
    else if (value)
    {
        HTTPHEADERW hdr;

        hdr.lpszField = (LPWSTR)field;
        hdr.lpszValue = (LPWSTR)value;
        hdr.wFlags = hdr.wCount = 0;

        if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
            hdr.wFlags |= HDR_ISREQUEST;

        return HTTP_InsertCustomHeader(request, &hdr);
    }
    /* no value to delete */
    else return ERROR_SUCCESS;

    if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
	    lphttpHdr->wFlags |= HDR_ISREQUEST;
    else
        lphttpHdr->wFlags &= ~HDR_ISREQUEST;

    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
    {
        HTTP_DeleteCustomHeader( request, index );

        if (value)
        {
            HTTPHEADERW hdr;

            hdr.lpszField = (LPWSTR)field;
            hdr.lpszValue = (LPWSTR)value;
            hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

            return HTTP_InsertCustomHeader(request, &hdr);
        }

        return ERROR_SUCCESS;
    }
    else if (dwModifier & COALESCEFLAGS)
    {
        LPWSTR lpsztmp;
        WCHAR ch = 0;
        INT len = 0;
        INT origlen = strlenW(lphttpHdr->lpszValue);
        INT valuelen = strlenW(value);

        if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA)
        {
            ch = ',';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }
        else if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)
        {
            ch = ';';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }

        len = origlen + valuelen + ((ch > 0) ? 2 : 0);

        lpsztmp = heap_realloc(lphttpHdr->lpszValue, (len+1)*sizeof(WCHAR));
        if (lpsztmp)
        {
            lphttpHdr->lpszValue = lpsztmp;
    /* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
            if (ch > 0)
            {
                lphttpHdr->lpszValue[origlen] = ch;
                origlen++;
                lphttpHdr->lpszValue[origlen] = ' ';
                origlen++;
            }

            memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen*sizeof(WCHAR));
            lphttpHdr->lpszValue[len] = '\0';
            res = ERROR_SUCCESS;
        }
        else
        {
            WARN("heap_realloc (%d bytes) failed\n",len+1);
            res = ERROR_OUTOFMEMORY;
        }
    }
    TRACE("<-- %d\n", res);
    return res;
}

/***********************************************************************
 *           HTTP_GetCustomHeaderIndex (internal)
 *
 * Return index of custom header from header array
 *
 */
static INT HTTP_GetCustomHeaderIndex(http_request_t *request, LPCWSTR lpszField,
                                     int requested_index, BOOL request_only)
{
    DWORD index;

    TRACE("%s, %d, %d\n", debugstr_w(lpszField), requested_index, request_only);

    for (index = 0; index < request->nCustHeaders; index++)
    {
        if (strcmpiW(request->custHeaders[index].lpszField, lpszField))
            continue;

        if (request_only && !(request->custHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (!request_only && (request->custHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (requested_index == 0)
            break;
        requested_index --;
    }

    if (index >= request->nCustHeaders)
	index = -1;

    TRACE("Return: %d\n", index);
    return index;
}


/***********************************************************************
 *           HTTP_InsertCustomHeader (internal)
 *
 * Insert header into array
 *
 */
static DWORD HTTP_InsertCustomHeader(http_request_t *request, LPHTTPHEADERW lpHdr)
{
    INT count;
    LPHTTPHEADERW lph = NULL;

    TRACE("--> %s: %s\n", debugstr_w(lpHdr->lpszField), debugstr_w(lpHdr->lpszValue));
    count = request->nCustHeaders + 1;
    if (count > 1)
	lph = heap_realloc_zero(request->custHeaders, sizeof(HTTPHEADERW) * count);
    else
	lph = heap_alloc_zero(sizeof(HTTPHEADERW) * count);

    if (!lph)
        return ERROR_OUTOFMEMORY;

    request->custHeaders = lph;
    request->custHeaders[count-1].lpszField = heap_strdupW(lpHdr->lpszField);
    request->custHeaders[count-1].lpszValue = heap_strdupW(lpHdr->lpszValue);
    request->custHeaders[count-1].wFlags = lpHdr->wFlags;
    request->custHeaders[count-1].wCount= lpHdr->wCount;
    request->nCustHeaders++;

    return ERROR_SUCCESS;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 *  If this function is called, the indexs may change.
 */
static BOOL HTTP_DeleteCustomHeader(http_request_t *request, DWORD index)
{
    if( request->nCustHeaders <= 0 )
        return FALSE;
    if( index >= request->nCustHeaders )
        return FALSE;
    request->nCustHeaders--;

    heap_free(request->custHeaders[index].lpszField);
    heap_free(request->custHeaders[index].lpszValue);

    memmove( &request->custHeaders[index], &request->custHeaders[index+1],
             (request->nCustHeaders - index)* sizeof(HTTPHEADERW) );
    memset( &request->custHeaders[request->nCustHeaders], 0, sizeof(HTTPHEADERW) );

    return TRUE;
}


/***********************************************************************
 *           HTTP_VerifyValidHeader (internal)
 *
 * Verify the given header is not invalid for the given http request
 *
 */
static BOOL HTTP_VerifyValidHeader(http_request_t *request, LPCWSTR field)
{
    /* Accept-Encoding is stripped from HTTP/1.0 requests. It is invalid */
    if (!strcmpW(request->version, g_szHttp1_0) && !strcmpiW(field, szAccept_Encoding))
        return ERROR_HTTP_INVALID_HEADER;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          IsHostInProxyBypassList (@)
 *
 * Undocumented
 *
 */
BOOL WINAPI IsHostInProxyBypassList(DWORD flags, LPCSTR szHost, DWORD length)
{
   FIXME("STUB: flags=%d host=%s length=%d\n",flags,szHost,length);
   return FALSE;
}

/***********************************************************************
 *           InternetShowSecurityInfoByURLA (@)
 */
BOOL WINAPI InternetShowSecurityInfoByURLA(LPCSTR url, HWND window)
{
   FIXME("stub: %s %p\n", url, window);
   return FALSE;
}

/***********************************************************************
 *           InternetShowSecurityInfoByURLW (@)
 */
BOOL WINAPI InternetShowSecurityInfoByURLW(LPCWSTR url, HWND window)
{
   FIXME("stub: %s %p\n", debugstr_w(url), window);
   return FALSE;
}

/***********************************************************************
 *           ShowX509EncodedCertificate (@)
 */
DWORD WINAPI ShowX509EncodedCertificate(HWND parent, LPBYTE cert, DWORD len)
{
    PCCERT_CONTEXT certContext = CertCreateCertificateContext(X509_ASN_ENCODING,
        cert, len);
    DWORD ret;

    if (certContext)
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW view;

        memset(&view, 0, sizeof(view));
        view.hwndParent = parent;
        view.pCertContext = certContext;
        if (CryptUIDlgViewCertificateW(&view, NULL))
            ret = ERROR_SUCCESS;
        else
            ret = GetLastError();
        CertFreeCertificateContext(certContext);
    }
    else
        ret = GetLastError();
    return ret;
}
