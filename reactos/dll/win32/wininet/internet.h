/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
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

#ifndef _WINE_INTERNET_H_
#define _WINE_INTERNET_H_

#include <wine/config.h>

#include <assert.h>
#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wininet.h>
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_GDI
#include <shlwapi.h>

#include <wine/list.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <sys/types.h>
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#else
#define closesocket close
#define ioctlsocket ioctl
#endif /* __MINGW32__ */

#include <winineti.h>

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

extern HMODULE WININET_hModule DECLSPEC_HIDDEN;

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

typedef struct {
    WCHAR *name;
    INTERNET_PORT port;
    BOOL is_https;
    struct sockaddr_storage addr;
    int addr_len;
    char addr_str[INET6_ADDRSTRLEN];

    WCHAR *scheme_host_port;
    const WCHAR *host_port;
    const WCHAR *canon_host_port;

    LONG ref;

    DWORD security_flags;
    const CERT_CHAIN_CONTEXT *cert_chain;

    struct list entry;
    struct list conn_pool;
} server_t;

void server_addref(server_t*) DECLSPEC_HIDDEN;
void server_release(server_t*) DECLSPEC_HIDDEN;

typedef enum {
    COLLECT_TIMEOUT,
    COLLECT_CONNECTIONS,
    COLLECT_CLEANUP
} collect_type_t;
BOOL collect_connections(collect_type_t) DECLSPEC_HIDDEN;

/* used for netconnection.c stuff */
typedef struct
{
    int socket;
    BOOL secure;
    CtxtHandle ssl_ctx;
    SecPkgContext_StreamSizes ssl_sizes;
    server_t *server;
    char *ssl_buf;
    char *extra_buf;
    size_t extra_len;
    char *peek_msg;
    char *peek_msg_mem;
    size_t peek_len;
    DWORD security_flags;
    BOOL mask_errors;

    BOOL keep_alive;
    DWORD64 keep_until;
    struct list pool_entry;
} netconn_t;

BOOL is_valid_netconn(netconn_t *) DECLSPEC_HIDDEN;
void close_netconn(netconn_t *) DECLSPEC_HIDDEN;

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline char *heap_strdupA(const char *str)
{
    char *ret = NULL;

    if(str) {
        DWORD size = strlen(str)+1;

        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strndupW(LPCWSTR str, UINT max_len)
{
    LPWSTR ret;
    UINT len;

    if(!str)
        return NULL;

    for(len=0; len<max_len; len++)
        if(str[len] == '\0')
            break;

    ret = heap_alloc(sizeof(WCHAR)*(len+1));
    if(ret) {
        memcpy(ret, str, sizeof(WCHAR)*len);
        ret[len] = '\0';
    }

    return ret;
}

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *heap_strdupWtoA(LPCWSTR str)
{
    char *ret = NULL;

    if(str) {
        DWORD size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(size);
        if(ret)
            WideCharToMultiByte(CP_ACP, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline void WININET_find_data_WtoA(LPWIN32_FIND_DATAW dataW, LPWIN32_FIND_DATAA dataA)
{
    dataA->dwFileAttributes = dataW->dwFileAttributes;
    dataA->ftCreationTime   = dataW->ftCreationTime;
    dataA->ftLastAccessTime = dataW->ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW->ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW->nFileSizeHigh;
    dataA->nFileSizeLow     = dataW->nFileSizeLow;
    dataA->dwReserved0      = dataW->dwReserved0;
    dataA->dwReserved1      = dataW->dwReserved1;
    WideCharToMultiByte(CP_ACP, 0, dataW->cFileName, -1, 
        dataA->cFileName, sizeof(dataA->cFileName),
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, dataW->cAlternateFileName, -1, 
        dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName),
        NULL, NULL);
}

typedef enum
{
    WH_HINIT = INTERNET_HANDLE_TYPE_INTERNET,
    WH_HFTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_FTP,
    WH_HGOPHERSESSION = INTERNET_HANDLE_TYPE_CONNECT_GOPHER,
    WH_HHTTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_HTTP,
    WH_HFILE = INTERNET_HANDLE_TYPE_FTP_FILE,
    WH_HFTPFINDNEXT = INTERNET_HANDLE_TYPE_FTP_FIND,
    WH_HHTTPREQ = INTERNET_HANDLE_TYPE_HTTP_REQUEST,
} WH_TYPE;

#define INET_OPENURL 0x0001
#define INET_CALLBACKW 0x0002

typedef struct
{
    LONG ref;
    HANDLE file_handle;
    WCHAR *file_name;
    BOOL is_committed;
} req_file_t;

typedef struct _object_header_t object_header_t;

typedef struct {
    void (*Destroy)(object_header_t*);
    void (*CloseConnection)(object_header_t*);
    DWORD (*QueryOption)(object_header_t*,DWORD,void*,DWORD*,BOOL);
    DWORD (*SetOption)(object_header_t*,DWORD,void*,DWORD);
    DWORD (*ReadFile)(object_header_t*,void*,DWORD,DWORD*);
    DWORD (*ReadFileEx)(object_header_t*,void*,DWORD,DWORD*,DWORD,DWORD_PTR);
    DWORD (*WriteFile)(object_header_t*,const void*,DWORD,DWORD*);
    DWORD (*QueryDataAvailable)(object_header_t*,DWORD*,DWORD,DWORD_PTR);
    DWORD (*FindNextFileW)(object_header_t*,void*);
    DWORD (*LockRequestFile)(object_header_t*,req_file_t**);
} object_vtbl_t;

#define INTERNET_HANDLE_IN_USE 1

struct _object_header_t
{
    WH_TYPE htype;
    const object_vtbl_t *vtbl;
    HINTERNET hInternet;
    BOOL valid_handle;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
    DWORD  dwError;
    ULONG  ErrorMask;
    DWORD  dwInternalFlags;
    LONG   refs;
    INTERNET_STATUS_CALLBACK lpfnStatusCB;
    struct list entry;
    struct list children;
};

typedef struct
{
    object_header_t hdr;
    LPWSTR  agent;
    LPWSTR  proxy;
    LPWSTR  proxyBypass;
    LPWSTR  proxyUsername;
    LPWSTR  proxyPassword;
    DWORD   accessType;
    DWORD   connect_timeout;
} appinfo_t;

typedef struct
{
    object_header_t hdr;
    appinfo_t *appInfo;
    LPWSTR  hostName; /* the final destination of the request */
    LPWSTR  userName;
    LPWSTR  password;
    INTERNET_PORT hostPort; /* the final destination port of the request */
    DWORD connect_timeout;
    DWORD send_timeout;
    DWORD receive_timeout;
} http_session_t;

#define HDR_ISREQUEST		0x0001
#define HDR_COMMADELIMITED	0x0002
#define HDR_SEMIDELIMITED	0x0004

typedef struct
{
    LPWSTR lpszField;
    LPWSTR lpszValue;
    WORD wFlags;
    WORD wCount;
} HTTPHEADERW, *LPHTTPHEADERW;


struct HttpAuthInfo;

typedef struct data_stream_vtbl_t data_stream_vtbl_t;

typedef struct {
    const data_stream_vtbl_t *vtbl;
}  data_stream_t;

typedef struct {
    data_stream_t data_stream;
    DWORD content_length;
    DWORD content_read;
} netconn_stream_t;

#define READ_BUFFER_SIZE 8192

typedef struct
{
    object_header_t hdr;
    http_session_t *session;
    server_t *server;
    server_t *proxy;
    LPWSTR path;
    LPWSTR verb;
    netconn_t *netconn;
    DWORD security_flags;
    DWORD connect_timeout;
    DWORD send_timeout;
    DWORD receive_timeout;
    LPWSTR version;
    DWORD status_code;
    LPWSTR statusText;
    DWORD bytesToWrite;
    DWORD bytesWritten;

    CRITICAL_SECTION headers_section;  /* section to protect the headers array */
    HTTPHEADERW *custHeaders;
    DWORD nCustHeaders;

    FILETIME last_modified;
    HANDLE hCacheFile;
    req_file_t *req_file;
    FILETIME expires;
    struct HttpAuthInfo *authInfo;
    struct HttpAuthInfo *proxyAuthInfo;

    CRITICAL_SECTION read_section;  /* section to protect the following fields */
    DWORD contentLength;  /* total number of bytes to be read */
    BOOL  read_chunked;   /* are we reading in chunked mode? */
    BOOL  read_gzip;      /* are we reading in gzip mode? */
    DWORD read_pos;       /* current read position in read_buf */
    DWORD read_size;      /* valid data size in read_buf */
    BYTE  read_buf[READ_BUFFER_SIZE]; /* buffer for already read but not returned data */

    BOOL decoding;
    data_stream_t *data_stream;
    netconn_stream_t netconn_stream;
} http_request_t;

typedef struct task_header_t task_header_t;
typedef void (*async_task_proc_t)(task_header_t*);

struct task_header_t
{
    async_task_proc_t proc;
    object_header_t *hdr;
};

void *alloc_async_task(object_header_t*,async_task_proc_t,size_t) DECLSPEC_HIDDEN;

void *alloc_object(object_header_t*,const object_vtbl_t*,size_t) DECLSPEC_HIDDEN;
object_header_t *get_handle_object( HINTERNET hinternet ) DECLSPEC_HIDDEN;
object_header_t *WININET_AddRef( object_header_t *info ) DECLSPEC_HIDDEN;
BOOL WININET_Release( object_header_t *info ) DECLSPEC_HIDDEN;

DWORD INET_QueryOption(object_header_t*,DWORD,void*,DWORD*,BOOL) DECLSPEC_HIDDEN;
DWORD INET_SetOption(object_header_t*,DWORD,void*,DWORD) DECLSPEC_HIDDEN;

time_t ConvertTimeString(LPCWSTR asctime) DECLSPEC_HIDDEN;

HINTERNET FTP_Connect(appinfo_t *hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
	DWORD dwInternalFlags) DECLSPEC_HIDDEN;

DWORD HTTP_Connect(appinfo_t*,LPCWSTR,
        INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
        LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
        DWORD dwInternalFlags, HINTERNET*) DECLSPEC_HIDDEN;

BOOL GetAddress(const WCHAR*,INTERNET_PORT,SOCKADDR*,int*,char*) DECLSPEC_HIDDEN;

DWORD get_cookie_header(const WCHAR*,const WCHAR*,WCHAR**) DECLSPEC_HIDDEN;
DWORD set_cookie(const WCHAR*,const WCHAR*,const WCHAR*,const WCHAR*,DWORD) DECLSPEC_HIDDEN;

void INTERNET_SetLastError(DWORD dwError) DECLSPEC_HIDDEN;
DWORD INTERNET_GetLastError(void) DECLSPEC_HIDDEN;
DWORD INTERNET_AsyncCall(task_header_t*) DECLSPEC_HIDDEN;
LPSTR INTERNET_GetResponseBuffer(void) DECLSPEC_HIDDEN;

VOID SendAsyncCallback(object_header_t *hdr, DWORD_PTR dwContext,
                       DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                       DWORD dwStatusInfoLength) DECLSPEC_HIDDEN;

VOID INTERNET_SendCallback(object_header_t *hdr, DWORD_PTR dwContext,
                           DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                           DWORD dwStatusInfoLength) DECLSPEC_HIDDEN;
BOOL INTERNET_FindProxyForProtocol(LPCWSTR szProxy, LPCWSTR proto, WCHAR *foundProxy, DWORD *foundProxyLen) DECLSPEC_HIDDEN;

typedef enum {
    BLOCKING_ALLOW,
    BLOCKING_DISALLOW,
    BLOCKING_WAITALL
} blocking_mode_t;

DWORD create_netconn(BOOL,server_t*,DWORD,BOOL,DWORD,netconn_t**) DECLSPEC_HIDDEN;
void free_netconn(netconn_t*) DECLSPEC_HIDDEN;
void NETCON_unload(void) DECLSPEC_HIDDEN;
DWORD NETCON_secure_connect(netconn_t*,server_t*) DECLSPEC_HIDDEN;
DWORD NETCON_send(netconn_t *connection, const void *msg, size_t len, int flags,
		int *sent /* out */) DECLSPEC_HIDDEN;
DWORD NETCON_recv(netconn_t*,void*,size_t,blocking_mode_t,int*) DECLSPEC_HIDDEN;
BOOL NETCON_query_data_available(netconn_t *connection, DWORD *available) DECLSPEC_HIDDEN;
BOOL NETCON_is_alive(netconn_t*) DECLSPEC_HIDDEN;
LPCVOID NETCON_GetCert(netconn_t *connection) DECLSPEC_HIDDEN;
int NETCON_GetCipherStrength(netconn_t*) DECLSPEC_HIDDEN;
DWORD NETCON_set_timeout(netconn_t *connection, BOOL send, DWORD value) DECLSPEC_HIDDEN;
int sock_get_error(void) DECLSPEC_HIDDEN;
int sock_send(int fd, const void *msg, size_t len, int flags) DECLSPEC_HIDDEN;
int sock_recv(int fd, void *msg, size_t len, int flags) DECLSPEC_HIDDEN;

server_t *get_server(const WCHAR*,INTERNET_PORT,BOOL,BOOL);

DWORD create_req_file(const WCHAR*,req_file_t**) DECLSPEC_HIDDEN;
void req_file_release(req_file_t*) DECLSPEC_HIDDEN;

static inline req_file_t *req_file_addref(req_file_t *req_file)
{
    InterlockedIncrement(&req_file->ref);
    return req_file;
}

BOOL init_urlcache(void) DECLSPEC_HIDDEN;
void free_urlcache(void) DECLSPEC_HIDDEN;
void free_cookie(void) DECLSPEC_HIDDEN;

void init_winsock(void) DECLSPEC_HIDDEN;

#define MAX_REPLY_LEN	 	0x5B4

/* Used for debugging - maybe need to be shared in the Wine debugging code ? */
typedef struct
{
    DWORD val;
    const char* name;
} wininet_flag_info;

/* Undocumented security flags */
#define _SECURITY_FLAG_CERT_REV_FAILED    0x00800000
#define _SECURITY_FLAG_CERT_INVALID_CA    0x01000000
#define _SECURITY_FLAG_CERT_INVALID_CN    0x02000000
#define _SECURITY_FLAG_CERT_INVALID_DATE  0x04000000

#define _SECURITY_ERROR_FLAGS_MASK              \
    (_SECURITY_FLAG_CERT_REV_FAILED             \
    |_SECURITY_FLAG_CERT_INVALID_CA             \
    |_SECURITY_FLAG_CERT_INVALID_CN             \
    |_SECURITY_FLAG_CERT_INVALID_DATE)

#endif /* _WINE_INTERNET_H_ */
