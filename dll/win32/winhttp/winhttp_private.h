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

#include <wine/heap.h>

#include "ole2.h"
#include "sspi.h"
#include "wincrypt.h"

#ifdef __REACTOS__
#include <string.h>
#include <winnls.h>

#define wcsicmp _wcsicmp
#define wcslwr _wcslwr
#endif

#include "wine/list.h"

#define WINHTTP_HANDLE_TYPE_SOCKET 4

struct object_header;
struct object_vtbl
{
    void (*handle_closing) ( struct object_header * );
    void (*destroy)( struct object_header * );
    BOOL (*query_option)( struct object_header *, DWORD, void *, DWORD * );
    BOOL (*set_option)( struct object_header *, DWORD, void *, DWORD );
};

struct object_header
{
    DWORD type;
    HINTERNET handle;
    const struct object_vtbl *vtbl;
    DWORD flags;
    DWORD disable_flags;
    DWORD logon_policy;
    DWORD redirect_policy;
    DWORD error;
    DWORD_PTR context;
    LONG refs;
    WINHTTP_STATUS_CALLBACK callback;
    DWORD notify_mask;
    LONG recursion_count;
    struct list entry;
    volatile LONG pending_sends;
    volatile LONG pending_receives;
};

struct hostdata
{
    struct list entry;
    LONG ref;
    WCHAR *hostname;
    INTERNET_PORT port;
    BOOL secure;
    struct list connections;
};

struct session
{
    struct object_header hdr;
    CRITICAL_SECTION cs;
    WCHAR *agent;
    DWORD access;
    int resolve_timeout;
    int connect_timeout;
    int send_timeout;
    int receive_timeout;
    int receive_response_timeout;
    WCHAR *proxy_server;
    WCHAR *proxy_bypass;
    WCHAR *proxy_username;
    WCHAR *proxy_password;
    struct list cookie_cache;
    HANDLE unload_event;
    DWORD secure_protocols;
    DWORD passport_flags;
    unsigned int websocket_receive_buffer_size;
    unsigned int websocket_send_buffer_size;
};

struct connect
{
    struct object_header hdr;
    struct session *session;
    WCHAR *hostname;    /* final destination of the request */
    WCHAR *servername;  /* name of the server we directly connect to */
    WCHAR *username;
    WCHAR *password;
    INTERNET_PORT hostport;
    INTERNET_PORT serverport;
    struct sockaddr_storage sockaddr;
    BOOL resolved;
};

struct netconn
{
    struct list entry;
    LONG refs;
    int socket;
    struct sockaddr_storage sockaddr;
    BOOL secure; /* SSL active on connection? */
    struct hostdata *host;
    ULONGLONG keep_until;
    CtxtHandle ssl_ctx;
    SecPkgContext_StreamSizes ssl_sizes;
    char *ssl_read_buf, *ssl_write_buf;
    char *extra_buf;
    size_t extra_len;
    char *peek_msg;
    char *peek_msg_mem;
    size_t peek_len;
    HANDLE port;
};

struct header
{
    WCHAR *field;
    WCHAR *value;
    BOOL is_request; /* part of request headers? */
};

enum auth_target
{
    TARGET_INVALID = -1,
    TARGET_SERVER,
    TARGET_PROXY,
    TARGET_MAX
};

enum auth_scheme
{
    SCHEME_INVALID = -1,
    SCHEME_BASIC,
    SCHEME_NTLM,
    SCHEME_PASSPORT,
    SCHEME_DIGEST,
    SCHEME_NEGOTIATE,
    SCHEME_MAX
};

struct authinfo
{
    enum auth_scheme scheme;
    CredHandle cred;
    CtxtHandle ctx;
    TimeStamp exp;
    ULONG attr;
    ULONG max_token;
    char *data;
    unsigned int data_len;
    BOOL finished; /* finished authenticating */
};

struct queue
{
    SRWLOCK lock;
    struct list queued_tasks;
    BOOL callback_running;
};

enum request_flags
{
    REQUEST_FLAG_WEBSOCKET_UPGRADE = 0x01,
};

enum request_response_state
{
    REQUEST_RESPONSE_STATE_NONE,
    REQUEST_RESPONSE_STATE_SENDING_REQUEST,
    REQUEST_RESPONSE_STATE_READ_RESPONSE_QUEUED,
    REQUEST_RESPONSE_STATE_REQUEST_SENT,
    REQUEST_RESPONSE_STATE_READ_RESPONSE_QUEUED_REQUEST_SENT,
    REQUEST_RESPONSE_STATE_REPLY_RECEIVED,
    REQUEST_RESPONSE_STATE_READ_RESPONSE_QUEUED_REPLY_RECEIVED,
    REQUEST_RESPONSE_RECURSIVE_REQUEST,
    REQUEST_RESPONSE_STATE_RESPONSE_RECEIVED,
};

struct request
{
    struct object_header hdr;
    struct connect *connect;
    enum request_flags flags;
    WCHAR *verb;
    WCHAR *path;
    WCHAR *version;
    WCHAR *raw_headers;
    void *optional;
    DWORD optional_len;
    struct netconn *netconn;
    DWORD security_flags;
    BOOL check_revocation;
    const CERT_CONTEXT *server_cert;
    const CERT_CONTEXT *client_cert;
    CredHandle cred_handle;
    BOOL cred_handle_initialized;
    int resolve_timeout;
    int connect_timeout;
    int send_timeout;
    int receive_timeout;
    int receive_response_timeout;
    DWORD max_redirects;
    DWORD redirect_count; /* total number of redirects during this request */
    WCHAR *status_text;
    DWORD content_length; /* total number of bytes to be read */
    DWORD content_read;   /* bytes read so far */
    BOOL  read_chunked;   /* are we reading in chunked mode? */
    BOOL  read_chunked_eof;  /* end of stream in chunked mode */
    BOOL  read_chunked_size; /* chunk size remaining */
    DWORD read_pos;       /* current read position in read_buf */
    DWORD read_size;      /* valid data size in read_buf */
    char  read_buf[8192]; /* buffer for already read but not returned data */
    struct header *headers;
    DWORD num_headers;
    struct authinfo *authinfo;
    struct authinfo *proxy_authinfo;
    HANDLE task_wait;
    HANDLE task_cancel;
    struct queue queue;
#ifdef __REACTOS__
    HANDLE task_thread;
#endif
    struct
    {
        WCHAR *username;
        WCHAR *password;
    } creds[TARGET_MAX][SCHEME_MAX];
    unsigned int websocket_receive_buffer_size;
    unsigned int websocket_send_buffer_size, websocket_set_send_buffer_size;
    int read_reply_len;
    DWORD read_reply_status;
    enum request_response_state state;
};

enum socket_state
{
    SOCKET_STATE_OPEN     = 0,
    SOCKET_STATE_SHUTDOWN = 1,
    SOCKET_STATE_CLOSED   = 2,
};

/* rfc6455 */
enum socket_opcode
{
    SOCKET_OPCODE_CONTINUE  = 0x00,
    SOCKET_OPCODE_TEXT      = 0x01,
    SOCKET_OPCODE_BINARY    = 0x02,
    SOCKET_OPCODE_RESERVED3 = 0x03,
    SOCKET_OPCODE_RESERVED4 = 0x04,
    SOCKET_OPCODE_RESERVED5 = 0x05,
    SOCKET_OPCODE_RESERVED6 = 0x06,
    SOCKET_OPCODE_RESERVED7 = 0x07,
    SOCKET_OPCODE_CLOSE     = 0x08,
    SOCKET_OPCODE_PING      = 0x09,
    SOCKET_OPCODE_PONG      = 0x0a,
    SOCKET_OPCODE_INVALID   = 0xff,
};

enum fragment_type
{
    SOCKET_FRAGMENT_NONE,
    SOCKET_FRAGMENT_BINARY,
    SOCKET_FRAGMENT_UTF8,
};

struct socket
{
    struct object_header hdr;
    struct netconn *netconn;
    int keepalive_interval;
    unsigned int send_buffer_size;
    enum socket_state state;
    struct queue send_q;
    struct queue recv_q;
    enum socket_opcode opcode;
    DWORD read_size;
    char mask[4];
    unsigned int mask_index;
    BOOL close_frame_received;
    DWORD close_frame_receive_err;
    USHORT status;
    char reason[123];
    DWORD reason_len;
    char *send_frame_buffer;
    unsigned int send_frame_buffer_size;
    unsigned int send_remaining_size;
    unsigned int bytes_in_send_frame_buffer;
    unsigned int client_buffer_offset;
    char *read_buffer;
    unsigned int bytes_in_read_buffer;
    SRWLOCK send_lock;
    volatile LONG pending_noncontrol_send;
    enum fragment_type sending_fragment_type;
    enum fragment_type receiving_fragment_type;
    BOOL last_receive_final;
};

typedef void (*TASK_CALLBACK)( void *ctx, BOOL abort );

struct task_header
{
    struct list entry;
    TASK_CALLBACK callback;
    struct object_header *obj;
    volatile LONG refs;
    volatile LONG completion_sent;
};

struct send_callback
{
    struct task_header task_hdr;
    DWORD status;
    void *info;
    DWORD buflen;
    union
    {
        WINHTTP_ASYNC_RESULT result;
        DWORD count;
    };
};

struct send_request
{
    struct task_header task_hdr;
    WCHAR *headers;
    DWORD headers_len;
    void *optional;
    DWORD optional_len;
    DWORD total_len;
    DWORD_PTR context;
};

struct receive_response
{
    struct task_header task_hdr;
};

struct query_data
{
    struct task_header task_hdr;
    DWORD *available;
};

struct read_data
{
    struct task_header task_hdr;
    void *buffer;
    DWORD to_read;
    DWORD *read;
};

struct write_data
{
    struct task_header task_hdr;
    const void *buffer;
    DWORD to_write;
    DWORD *written;
};

struct socket_send
{
    struct task_header task_hdr;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    const void *buf;
    DWORD len;
    WSAOVERLAPPED ovr;
    BOOL complete_async;
};

struct socket_receive
{
    struct task_header task_hdr;
    void *buf;
    DWORD len;
};

struct socket_shutdown
{
    struct task_header task_hdr;
    USHORT status;
    char reason[123];
    DWORD len;
    BOOL send_callback;
    WSAOVERLAPPED ovr;
    BOOL complete_async;
};

struct object_header *addref_object( struct object_header * );
struct object_header *grab_object( HINTERNET );
void release_object( struct object_header * );
HINTERNET alloc_handle( struct object_header * );
BOOL free_handle( HINTERNET );

void send_callback( struct object_header *, DWORD, LPVOID, DWORD );
void close_connection( struct request * );
void init_queue( struct queue *queue );
void stop_queue( struct queue * );

void netconn_addref( struct netconn * );
void netconn_release( struct netconn * );
DWORD netconn_create( struct hostdata *, const struct sockaddr_storage *, int, struct netconn ** );
void netconn_unload( void );
ULONG netconn_query_data_available( struct netconn * );
DWORD netconn_recv( struct netconn *, void *, size_t, int, int * );
DWORD netconn_resolve( WCHAR *, INTERNET_PORT, struct sockaddr_storage *, int );
DWORD netconn_secure_connect( struct netconn *, WCHAR *, DWORD, CredHandle *, BOOL );
DWORD netconn_send( struct netconn *, const void *, size_t, int *, WSAOVERLAPPED * );
BOOL netconn_wait_overlapped_result( struct netconn *conn, WSAOVERLAPPED *ovr, DWORD *len );
void netconn_cancel_io( struct netconn *conn );
DWORD netconn_set_timeout( struct netconn *, BOOL, int );
BOOL netconn_is_alive( struct netconn * );
const void *netconn_get_certificate( struct netconn * );
int netconn_get_cipher_strength( struct netconn * );

BOOL set_cookies( struct request *, const WCHAR * );
DWORD add_cookie_headers( struct request * );
DWORD add_request_headers( struct request *, const WCHAR *, DWORD, DWORD );
void destroy_cookies( struct session * );
BOOL set_server_for_hostname( struct connect *, const WCHAR *, INTERNET_PORT );
void destroy_authinfo( struct authinfo * );

void release_host( struct hostdata * );
DWORD process_header( struct request *, const WCHAR *, const WCHAR *, DWORD, BOOL );

extern HRESULT WinHttpRequest_create( void ** );
void release_typelib( void );

static inline WCHAR *strdupAW( const char *src )
{
    WCHAR *dst = NULL;
    if (src)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
        if ((dst = malloc( len * sizeof(WCHAR) )))
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
        if ((dst = malloc( len )))
            WideCharToMultiByte( CP_ACP, 0, src, -1, dst, len, NULL, NULL );
    }
    return dst;
}

extern HINSTANCE winhttp_instance;

#define MIN_WEBSOCKET_SEND_BUFFER_SIZE 16

#endif /* _WINE_WINHTTP_PRIVATE_H_ */
