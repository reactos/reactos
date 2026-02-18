/*
 * test status notifications
 *
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

#include "wine/test.h"

static DWORD (WINAPI *pWinHttpWebSocketClose)(HINTERNET,USHORT,void*,DWORD);
static HINTERNET (WINAPI *pWinHttpWebSocketCompleteUpgrade)(HINTERNET,DWORD_PTR);
static DWORD (WINAPI *pWinHttpWebSocketQueryCloseStatus)(HINTERNET,USHORT*,void*,DWORD,DWORD*);
static DWORD (WINAPI *pWinHttpWebSocketReceive)(HINTERNET,void*,DWORD,DWORD*,WINHTTP_WEB_SOCKET_BUFFER_TYPE*);
static DWORD (WINAPI *pWinHttpWebSocketSend)(HINTERNET,WINHTTP_WEB_SOCKET_BUFFER_TYPE,void*,DWORD);
static DWORD (WINAPI *pWinHttpWebSocketShutdown)(HINTERNET,USHORT,void*,DWORD);

enum api
{
    winhttp_connect = 1,
    winhttp_open_request,
    winhttp_send_request,
    winhttp_receive_response,
    winhttp_websocket_complete_upgrade,
    winhttp_websocket_send,
    winhttp_websocket_receive,
    winhttp_websocket_shutdown,
    winhttp_websocket_close,
    winhttp_query_data,
    winhttp_read_data,
    winhttp_write_data,
    winhttp_close_handle
};

struct notification
{
    enum api function;      /* api responsible for notification */
    unsigned int status;    /* status received */
    DWORD flags;            /* a combination of NF_* flags */
};

#define NF_ALLOW       0x0001  /* notification may or may not happen */
#define NF_WINE_ALLOW  0x0002  /* wine sends notification when it should not */
#define NF_SIGNAL      0x0004  /* signal wait handle when notified */
#define NF_MAIN_THREAD 0x0008  /* the operation completes synchronously and callback is called from the main thread */
#define NF_SAVE_BUFFER 0x0010  /* save buffer data when notified */
#define NF_OTHER_THREAD 0x0020 /* the operation completes asynchronously and callback is called from the other thread */

struct info
{
    enum api function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
    HANDLE wait;
    unsigned int line;
    DWORD main_thread_id;
    DWORD last_thread_id;
    DWORD last_status;
    char buffer[256];
    unsigned int buflen;
};

struct test_request
{
    HINTERNET session;
    HINTERNET connection;
    HINTERNET request;
};

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    BOOL status_ok, function_ok;
    struct info *info = (struct info *)context;

    info->last_status = status;
    info->last_thread_id = GetCurrentThreadId();

    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        WinHttpQueryOption( handle, WINHTTP_OPTION_CONTEXT_VALUE, &info, &size );
    }
    while (info->index < info->count && info->test[info->index].status != status && (info->test[info->index].flags & NF_ALLOW))
        info->index++;
    while (info->index < info->count && (info->test[info->index].flags & NF_WINE_ALLOW))
    {
        todo_wine ok( info->test[info->index].status != status, "unexpected %#lx notification\n", status );
        if (info->test[info->index].status == status) break;
        info->index++;
    }
    ok( info->index < info->count, "%u: unexpected notification %#lx\n", info->line, status );
    if (info->index >= info->count) return;

    status_ok   = (info->test[info->index].status == status);
    function_ok = (info->test[info->index].function == info->function);

    ok( status_ok, "%u: expected status %#x got %#lx\n", info->line, info->test[info->index].status, status );
    ok(function_ok, "%u: expected function %u got %u\n", info->line, info->test[info->index].function, info->function);

    if (info->test[info->index].flags & NF_MAIN_THREAD)
    {
        ok(GetCurrentThreadId() == info->main_thread_id, "%u: expected callback %#lx to be called from the same thread\n",
                info->line, status);
    }
    else if (info->test[info->index].flags & NF_OTHER_THREAD)
    {
        ok(GetCurrentThreadId() != info->main_thread_id, "%u: expected callback %#lx to be called from the other thread\n",
                info->line, status);
    }
    if (info->test[info->index].flags & NF_SAVE_BUFFER)
    {
        info->buflen = buflen;
        memcpy( info->buffer, buffer, min( buflen, sizeof(info->buffer) ));
    }

    if (status_ok && function_ok && info->test[info->index++].flags & NF_SIGNAL)
    {
        SetEvent( info->wait );
    }
}

static const struct notification cache_test_async[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static const struct notification cache_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void setup_test( struct info *info, enum api function, unsigned int line )
{
    if (info->wait) ResetEvent( info->wait );
    info->function = function;
    info->line = line;
    while (info->index < info->count && info->test[info->index].function != function
           && (info->test[info->index].flags & (NF_ALLOW | NF_WINE_ALLOW)))
        info->index++;
    ok_(__FILE__,line)(info->test[info->index].function == function,
                       "unexpected function %u, expected %u. probably some notifications were missing\n",
                       info->test[info->index].function, function);
    info->last_thread_id = 0xdeadbeef;
    info->last_status = 0xdeadbeef;
    info->main_thread_id = GetCurrentThreadId();
}

static void end_test( struct info *info, unsigned int line )
{
    ok_(__FILE__,line)(info->index == info->count, "some notifications were missing: %x\n",
                       info->test[info->index].status);
}

static void test_connection_cache( BOOL async )
{
    HANDLE ses, con, req, event;
    DWORD size, status, err;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;

    info.test  = async ? cache_test_async : cache_test;
    info.count = async ? ARRAY_SIZE( cache_test_async ) : ARRAY_SIZE ( cache_test );
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( L"winetest", 0, NULL, NULL, async ? WINHTTP_FLAG_ASYNC : 0 );
    ok( ses != NULL, "failed to open session %lu\n", GetLastError() );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip("Unload event not supported\n");
        unload = FALSE;
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, L"test.winehq.org", 0, 0 );
    ok( con != NULL, "failed to open a connection %lu\n", GetLastError() );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "failed to send request %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly %lu\n", status );

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "failed to send request %lu\n", GetLastError() );

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly %lu\n", status );

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok( status == WAIT_TIMEOUT, "got %#lx\n", status );
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok( status == WAIT_OBJECT_0, "got %#lx\n", status );
    }

    ses = WinHttpOpen( L"winetest", 0, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %lu\n", GetLastError() );

    if (unload)
    {
        ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
        ok(ret, "failed to set unload option\n");
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, L"test.winehq.org", 0, 0 );
    ok( con != NULL, "failed to open a connection %lu\n", GetLastError() );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "failed to send request %lu\n", GetLastError() );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly %lu\n", status );

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "failed to send request %lu\n", GetLastError() );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly %lu\n", status );

    setup_test( &info, winhttp_close_handle, __LINE__ );
done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok( status == WAIT_TIMEOUT, "got %#lx\n", status );
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );
    end_test( &info, __LINE__ );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok( status == WAIT_OBJECT_0, "got %#lx\n", status );
    }

    CloseHandle( event );
}

static const struct notification redirect_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, NF_MAIN_THREAD },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW | NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW | NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW | NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW | NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT, NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD},
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD | NF_SIGNAL},
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static const struct notification redirect_test_async[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL | NF_OTHER_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_OTHER_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_OTHER_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_OTHER_THREAD | NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void test_redirect( BOOL async )
{
    HANDLE ses, con, req;
    DWORD size, status, err;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = async ? redirect_test_async : redirect_test;
    info.count = async ? ARRAY_SIZE( redirect_test_async ) : ARRAY_SIZE( redirect_test );
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( L"winetest", 0, NULL, NULL, async ? WINHTTP_FLAG_ASYNC : 0 );
    ok( ses != NULL, "failed to open session %lu\n", GetLastError() );

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok( ret, "failed to set context value %lu\n", GetLastError() );

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, L"test.winehq.org", 0, 0 );
    ok( con != NULL, "failed to open a connection %lu\n", GetLastError() );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, L"/tests/redirect", NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "failed to send request %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    status = 0xdeadbeef;
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly %lu\n", status );

    setup_test( &info, winhttp_close_handle, __LINE__ );
done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );
    end_test( &info, __LINE__ );
}

static const struct notification async_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_MAIN_THREAD },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_SIGNAL | NF_MAIN_THREAD },
    { winhttp_query_data,       WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, NF_SIGNAL },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void test_async( void )
{
    HANDLE ses, con, req, event;
    DWORD size, status, err;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;
    char buffer[1024];

    info.test  = async_test;
    info.count = ARRAY_SIZE( async_test );
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( L"winetest", 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok( ses != NULL, "failed to open session %lu\n", GetLastError() );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip("Unload event not supported\n");
        unload = FALSE;
    }

    SetLastError( 0xdeadbeef );
    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
    err = GetLastError();
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    SetLastError( 0xdeadbeef );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    err = GetLastError();
    ok( ret, "failed to set context value %lu\n", err );
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_connect, __LINE__ );
    SetLastError( 0xdeadbeef );
    con = WinHttpConnect( ses, L"test.winehq.org", 0, 0 );
    err = GetLastError();
    ok( con != NULL, "failed to open a connection %lu\n", err );
    ok( err == ERROR_SUCCESS || broken(err == WSAEINVAL) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_open_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    req = WinHttpOpenRequest( con, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    err = GetLastError();
    ok( req != NULL, "failed to open a request %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    ret = WinHttpAddRequestHeaders( req , L"Connection: close", -1L, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD );
    err = GetLastError();
    ok(ret, "WinHttpAddRequestHeaders failed to add new header, got %d with error %lu\n", ret, err);

    setup_test( &info, winhttp_send_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        WinHttpCloseHandle( req );
        WinHttpCloseHandle( con );
        WinHttpCloseHandle( ses );
        CloseHandle( info.wait );
        return;
    }
    ok( ret, "failed to send request %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpReceiveResponse( req, NULL );
    err = GetLastError();
    ok( ret, "failed to receive response %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    SetLastError( 0xdeadbeef );
    ret = WinHttpReceiveResponse( req, NULL );
    err = GetLastError();
    ok( ret, "failed to receive response %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    err = GetLastError();
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 200, "request failed unexpectedly %lu\n", status );
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_query_data, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryDataAvailable( req, NULL );
    err = GetLastError();
    ok( ret, "failed to query data available %lu\n", err );
    ok( err == ERROR_SUCCESS || err == ERROR_IO_PENDING || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );
    ok( info.last_status == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, "got status %#lx\n", status );
    ok(( err == ERROR_SUCCESS && info.last_thread_id == GetCurrentThreadId())
         || (err == ERROR_IO_PENDING && info.last_thread_id != GetCurrentThreadId()),
         "got unexpected thread %#lx, err %#lx\n", info.last_thread_id, err );

    setup_test( &info, winhttp_read_data, __LINE__ );
    ret = WinHttpReadData( req, buffer, sizeof(buffer), NULL );
    ok( ret, "failed to read data %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );

    ok( info.last_status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE, "got status %#lx\n", status );
    ok( (err == ERROR_SUCCESS && info.last_thread_id == GetCurrentThreadId())
        || (err == ERROR_IO_PENDING && info.last_thread_id != GetCurrentThreadId()),
        "got unexpected thread %#lx, err %#lx\n", info.last_thread_id, err );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok( status == WAIT_TIMEOUT, "got %#lx\n", status );
    }
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    if (unload)
    {
        status = WaitForSingleObject( event, 2000 );
        ok( status == WAIT_OBJECT_0, "got %#lx\n", status );
    }
    CloseHandle( event );
    CloseHandle( info.wait );
    end_test( &info, __LINE__ );
}

static const struct notification websocket_test[] =
{
    { winhttp_connect,                    WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,               WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_SIGNAL },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_RESOLVING_NAME },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_NAME_RESOLVED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_websocket_send,             WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_send,             WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_send,             WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_send,             WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_shutdown,         WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE, NF_MAIN_THREAD | NF_SIGNAL },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SAVE_BUFFER | NF_SIGNAL },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SAVE_BUFFER | NF_SIGNAL },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE, NF_SIGNAL },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
};

static const struct notification websocket_test2[] =
{
    { winhttp_open_request,               WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL | NF_MAIN_THREAD},
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, NF_SIGNAL },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_MAIN_THREAD | NF_SAVE_BUFFER},
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE, NF_SIGNAL },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static const struct notification websocket_test3[] =
{
    { winhttp_open_request,               WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL | NF_MAIN_THREAD },
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, NF_SIGNAL },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },

    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_MAIN_THREAD },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_SAVE_BUFFER },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },

    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
};

static struct notification websocket_test4[] =
{
    { winhttp_open_request,               WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL | NF_MAIN_THREAD },
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, NF_SIGNAL },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },

    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, NF_SAVE_BUFFER },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },

    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
};

static const struct notification websocket_test5[] =
{
    { winhttp_open_request,               WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,               WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_MAIN_THREAD },
    { winhttp_receive_response,           WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL | NF_MAIN_THREAD },
    { winhttp_websocket_complete_upgrade, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, NF_SIGNAL | NF_MAIN_THREAD },
    { winhttp_websocket_receive,          WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },

    { winhttp_websocket_shutdown,         WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE, NF_MAIN_THREAD },
    { winhttp_websocket_shutdown,         WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SAVE_BUFFER | NF_SIGNAL },
    { winhttp_websocket_close,            WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE,
                                                                 NF_MAIN_THREAD| NF_SAVE_BUFFER | NF_SIGNAL },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,               WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

#define BIG_BUFFER_SIZE (16 * 1024)

static void test_websocket(BOOL secure)
{
    HANDLE session, connection, request, socket, event;
    WINHTTP_WEB_SOCKET_ASYNC_RESULT *result;
    WINHTTP_WEB_SOCKET_STATUS *ws_status;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    DWORD size, status, err, value;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;
    unsigned char *big_buffer;
    char buffer[1024];
    USHORT close_status;
    DWORD protocols, flags;
    unsigned int i, test_index, offset;

    if (!pWinHttpWebSocketCompleteUpgrade)
    {
        win_skip( "WinHttpWebSocketCompleteUpgrade not supported\n" );
        return;
    }

    info.test  = websocket_test;
    info.count = ARRAY_SIZE( websocket_test );
    info.index = 0;
    info.wait  = CreateEventW( NULL, FALSE, FALSE, NULL );

    session = WinHttpOpen( L"winetest", 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok( session != NULL, "got %lu\n", GetLastError() );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( session, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip( "Unload event not supported\n" );
        unload = FALSE;
    }

    if (secure)
    {
        protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
        ret = WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
        ok( ret, "failed to set protocols %lu\n", GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    WinHttpSetStatusCallback( session, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
    err = GetLastError();
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    SetLastError( 0xdeadbeef );
    ret = WinHttpSetOption( session, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context) );
    err = GetLastError();
    ok( ret, "got %lu\n", err );
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_connect, __LINE__ );
    SetLastError( 0xdeadbeef );
    connection = WinHttpConnect( session, L"ws.ifelse.io", 0, 0 );
    err = GetLastError();
    ok( connection != NULL, "got %lu\n", err );
    ok( err == ERROR_SUCCESS || broken(err == WSAEINVAL) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_open_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    request = WinHttpOpenRequest( connection, NULL, L"/", NULL, NULL, NULL, secure ? WINHTTP_FLAG_SECURE : 0);
    err = GetLastError();
    ok( request != NULL, "got %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    if (secure)
    {
        flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        ret = WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
        ok( ret, "failed to set security flags %lu\n", GetLastError() );
    }

    ret = WinHttpSetOption( request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );

    value = 15;
    ret = WinHttpSetOption(request, WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE, &value, sizeof(DWORD));
    ok(ret, "got %lu\n", GetLastError());
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    ok( ret, "got err %lu.\n", err );

    WaitForSingleObject( info.wait, INFINITE );

    value = 32768;
    ret = WinHttpSetOption(request, WINHTTP_OPTION_WEB_SOCKET_SEND_BUFFER_SIZE, &value, sizeof(DWORD));
    ok(ret, "got %lu\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip( "connection failed, skipping\n" );
        WinHttpCloseHandle( request );
        WinHttpCloseHandle( connection );
        WinHttpCloseHandle( session );
        CloseHandle( info.wait );
        return;
    }
    ok( ret, "got %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpReceiveResponse( request, NULL );
    err = GetLastError();
    ok( ret, "got %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    err = GetLastError();
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 101, "got %lu\n", status );
    ok( err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %lu\n", err );

    setup_test( &info, winhttp_websocket_complete_upgrade, __LINE__ );
    SetLastError( 0xdeadbeef );
    socket = pWinHttpWebSocketCompleteUpgrade( request, (DWORD_PTR)context );
    err = GetLastError();
    ok( socket != NULL, "got %lu\n", err );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    WinHttpCloseHandle( request );

    WaitForSingleObject( info.wait, INFINITE );

    /* The send is executed synchronously (even if sending a reasonably big buffer exceeding SSL buffer size).
     * It is possible to trigger queueing the send into another thread but that involves sending a considerable
     * amount of big enough buffers. */
    big_buffer = malloc( BIG_BUFFER_SIZE );
    for (i = 0; i < BIG_BUFFER_SIZE; ++i) big_buffer[i] = (i & 0xff) ^ 0xcc;

    setup_test( &info, winhttp_websocket_send, __LINE__ );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE, big_buffer, BIG_BUFFER_SIZE / 2 );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE,
                                 big_buffer + BIG_BUFFER_SIZE / 2, BIG_BUFFER_SIZE / 2 );
    ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                 big_buffer + BIG_BUFFER_SIZE / 2, BIG_BUFFER_SIZE / 2 );
    ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

    setup_test( &info, winhttp_websocket_send, __LINE__ );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE,
                                 big_buffer + BIG_BUFFER_SIZE / 2, BIG_BUFFER_SIZE / 2 );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_send, __LINE__ );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, NULL, 0 );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_send, __LINE__ );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void *)"hello", sizeof("hello") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_shutdown, __LINE__ );
    err = pWinHttpWebSocketShutdown( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void *)"hello", sizeof("hello") );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );

    err = pWinHttpWebSocketShutdown( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );
    err = pWinHttpWebSocketSend( socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void *)"hello", sizeof("hello") );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );

    setup_test( &info, winhttp_websocket_receive, __LINE__ );
    buffer[0] = 0;
    size = 0xdeadbeef;
    type = 0xdeadbeef;
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );
    ok( info.buflen == sizeof(*ws_status), "got %u\n", info.buflen );
    ws_status = (WINHTTP_WEB_SOCKET_STATUS *)info.buffer;
    ok( ws_status->eBufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        "got unexpected eBufferType %u\n", ws_status->eBufferType );
    ok( size == 0xdeadbeef, "got %lu\n", size );
    ok( type == 0xdeadbeef, "got %u\n", type );
    ok( buffer[0] == 'R', "unexpected data\n" );

    memset( big_buffer, 0, BIG_BUFFER_SIZE );
    offset = 0;
    test_index = info.index;
    do
    {
        info.index = test_index;
        setup_test( &info, winhttp_websocket_receive, __LINE__ );
        size = 0xdeadbeef;
        type = 0xdeadbeef;
        ws_status = (WINHTTP_WEB_SOCKET_STATUS *)info.buffer;
        ws_status->eBufferType = ~0u;
        err = pWinHttpWebSocketReceive( socket, big_buffer + offset, BIG_BUFFER_SIZE - offset, &size, &type );
        ok( err == ERROR_SUCCESS, "got %lu\n", err );
        WaitForSingleObject( info.wait, INFINITE );
        ok( info.buflen == sizeof(*ws_status), "got %u\n", info.buflen );
        ok( ws_status->eBufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE
            || ws_status->eBufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE, "got %u\n", ws_status->eBufferType );
        offset += ws_status->dwBytesTransferred;
        ok( offset <= BIG_BUFFER_SIZE, "got %lu\n", ws_status->dwBytesTransferred );
        ok( size == 0xdeadbeef, "got %lu\n", size );
        ok( type == 0xdeadbeef, "got %u\n", type );
    }
    while (ws_status->eBufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

    ok( offset == BIG_BUFFER_SIZE, "got %u\n", offset );

    for (i = 0; i < BIG_BUFFER_SIZE; ++i)
        if (big_buffer[i] != ((i & 0xff) ^ 0xcc)) break;
    ok( i == BIG_BUFFER_SIZE, "unexpected data %#x at %u\n", (unsigned char)big_buffer[i], i );

    free( big_buffer );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );
    ok( close_status == 0xdead, "got %u\n", close_status );
    ok( size == sizeof(buffer) + 1, "got %lu\n", size );

    setup_test( &info, winhttp_websocket_close, __LINE__ );
    err = pWinHttpWebSocketClose( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    ok( close_status == 1000, "got %u\n", close_status );
    ok( size <= sizeof(buffer), "got %lu\n", size );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( socket );

    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    /* Test socket close while receive is pending. */
    info.test  = websocket_test2;
    info.count = ARRAY_SIZE( websocket_test2 );
    info.index = 0;

    setup_test( &info, winhttp_open_request, __LINE__ );
    request = WinHttpOpenRequest( connection, NULL, L"/", NULL, NULL, NULL, secure ? WINHTTP_FLAG_SECURE : 0);
    ok( request != NULL, "got %lu\n", err );

    if (secure)
    {
        flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        ret = WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
        ok( ret, "failed to set security flags %lu\n", GetLastError() );
    }

    ret = WinHttpSetOption( request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "got %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( request, NULL );
    ok( ret, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 101, "got %lu\n", status );

    setup_test( &info, winhttp_websocket_complete_upgrade, __LINE__ );
    socket = pWinHttpWebSocketCompleteUpgrade( request, (DWORD_PTR)context );
    ok( socket != NULL, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_receive, __LINE__ );
    buffer[0] = 0;
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );
    ok( buffer[0] == 'R', "unexpected data\n" );

    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    setup_test( &info, winhttp_websocket_close, __LINE__ );
    err = pWinHttpWebSocketClose( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    ok( info.buflen == sizeof(*result), "got %u\n", info.buflen );
    result = (WINHTTP_WEB_SOCKET_ASYNC_RESULT *)info.buffer;
    ok( result->Operation == WINHTTP_WEB_SOCKET_RECEIVE_OPERATION, "got %u\n", result->Operation );
    ok( !result->AsyncResult.dwResult, "got %Iu\n", result->AsyncResult.dwResult );
    ok( result->AsyncResult.dwError == ERROR_WINHTTP_OPERATION_CANCELLED, "got %lu\n", result->AsyncResult.dwError );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );
    ok( close_status == 0xdead, "got %u\n", close_status );
    ok( size == sizeof(buffer) + 1, "got %lu\n", size );

    WaitForSingleObject( info.wait, INFINITE );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    ok( close_status == 1000, "got %u\n", close_status );
    ok( size <= sizeof(buffer), "got %lu\n", size );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( socket );
    WinHttpCloseHandle( request );

    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );


    /* Test socket handle close while web socket close is pending. */
    info.test  = websocket_test3;
    info.count = ARRAY_SIZE( websocket_test3 );
    info.index = 0;

    setup_test( &info, winhttp_open_request, __LINE__ );
    request = WinHttpOpenRequest( connection, NULL, L"/", NULL, NULL, NULL, secure ? WINHTTP_FLAG_SECURE : 0);
    ok( request != NULL, "got %lu\n", err );

    if (secure)
    {
        flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        ret = WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
        ok( ret, "failed to set security flags %lu\n", GetLastError() );
    }

    ret = WinHttpSetOption( request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "got %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( request, NULL );
    ok( ret, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 101, "got %lu\n", status );

    setup_test( &info, winhttp_websocket_complete_upgrade, __LINE__ );
    socket = pWinHttpWebSocketCompleteUpgrade( request, (DWORD_PTR)context );
    ok( socket != NULL, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_receive, __LINE__ );
    buffer[0] = 0;
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );
    ok( buffer[0] == 'R', "unexpected data\n" );

    setup_test( &info, winhttp_websocket_close, __LINE__ );

    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    err = pWinHttpWebSocketClose( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    info.buflen = 0xdeadbeef;
    WinHttpCloseHandle( socket );
    WaitForSingleObject( info.wait, INFINITE );

    ok( info.buflen == sizeof(*result), "got %u\n", info.buflen );
    result = (WINHTTP_WEB_SOCKET_ASYNC_RESULT *)info.buffer;
    ok( result->Operation == WINHTTP_WEB_SOCKET_CLOSE_OPERATION, "got %u\n", result->Operation );
    todo_wine ok( !result->AsyncResult.dwResult, "got %Iu\n", result->AsyncResult.dwResult );
    ok( result->AsyncResult.dwError == ERROR_WINHTTP_OPERATION_CANCELLED, "got %lu\n", result->AsyncResult.dwError );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( request );
    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    /* Test socket handle close while receive is pending. */
    info.test  = websocket_test4;
    info.count = ARRAY_SIZE( websocket_test4 );
    info.index = 0;

    setup_test( &info, winhttp_open_request, __LINE__ );
    request = WinHttpOpenRequest( connection, NULL, L"/", NULL, NULL, NULL, secure ? WINHTTP_FLAG_SECURE : 0);
    ok( request != NULL, "got %lu\n", err );

    if (secure)
    {
        flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        ret = WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
        ok( ret, "failed to set security flags %lu\n", GetLastError() );
    }

    ret = WinHttpSetOption( request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "got %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( request, NULL );
    ok( ret, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 101, "got %lu\n", status );

    setup_test( &info, winhttp_websocket_complete_upgrade, __LINE__ );
    socket = pWinHttpWebSocketCompleteUpgrade( request, (DWORD_PTR)context );
    ok( socket != NULL, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_receive, __LINE__ );
    buffer[0] = 0;
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );
    ok( buffer[0] == 'R', "unexpected data\n" );

    setup_test( &info, winhttp_websocket_close, __LINE__ );

    info.buflen = 0xdeadbeef;

    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    WinHttpCloseHandle( socket );
    WaitForSingleObject( info.wait, INFINITE );

    ok( info.buflen == sizeof(*result), "got %u\n", info.buflen );
    result = (WINHTTP_WEB_SOCKET_ASYNC_RESULT *)info.buffer;
    ok( result->Operation == WINHTTP_WEB_SOCKET_RECEIVE_OPERATION, "got %u\n", result->Operation );
    ok( !result->AsyncResult.dwResult, "got %Iu\n", result->AsyncResult.dwResult );
    ok( result->AsyncResult.dwError == ERROR_WINHTTP_OPERATION_CANCELLED, "got %lu\n", result->AsyncResult.dwError );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( request );
    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    /* Test socket shutdown while receive is pending. */
    info.test  = websocket_test5;
    info.count = ARRAY_SIZE( websocket_test5 );
    info.index = 0;

    setup_test( &info, winhttp_open_request, __LINE__ );
    request = WinHttpOpenRequest( connection, NULL, L"/", NULL, NULL, NULL, secure ? WINHTTP_FLAG_SECURE : 0);
    ok( request != NULL, "got %lu\n", err );

    if (secure)
    {
        flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        ret = WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
        ok( ret, "failed to set security flags %lu\n", GetLastError() );
    }

    ret = WinHttpSetOption( request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "got %lu\n", GetLastError() );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( request, NULL );
    ok( ret, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed unexpectedly %lu\n", err );
    ok( status == 101, "got %lu\n", status );

    setup_test( &info, winhttp_websocket_complete_upgrade, __LINE__ );
    socket = pWinHttpWebSocketCompleteUpgrade( request, (DWORD_PTR)context );
    ok( socket != NULL, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_websocket_receive, __LINE__ );
    buffer[0] = 0;
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    WaitForSingleObject( info.wait, INFINITE );
    ok( buffer[0] == 'R', "unexpected data\n" );

    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );

    setup_test( &info, winhttp_websocket_shutdown, __LINE__ );
    ws_status = (WINHTTP_WEB_SOCKET_STATUS *)info.buffer;
    ws_status->eBufferType = ~0u;
    err = pWinHttpWebSocketShutdown( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );
    ok( close_status == 0xdead, "got %u\n", close_status );
    ok( size == sizeof(buffer) + 1, "got %lu\n", size );

    WaitForSingleObject( info.wait, INFINITE );

    ok( info.buflen == sizeof(*ws_status), "got %u\n", info.buflen );
    ok( ws_status->eBufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE, "got %u\n", ws_status->eBufferType );
    ok( !ws_status->dwBytesTransferred, "got %lu\n", ws_status->dwBytesTransferred );

    close_status = 0xdead;
    size = sizeof(buffer) + 1;
    err = pWinHttpWebSocketQueryCloseStatus( socket, &close_status, buffer, sizeof(buffer), &size );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );
    ok( close_status == 1000, "got %u\n", close_status );
    ok( size <= sizeof(buffer), "got %lu\n", size );

    err = pWinHttpWebSocketReceive( socket, buffer, sizeof(buffer), &size, &type );
    ok( err == ERROR_INVALID_OPERATION, "got %lu\n", err );

    info.buflen = 0xdeadbeef;
    setup_test( &info, winhttp_websocket_close, __LINE__ );
    err = pWinHttpWebSocketClose( socket, 1000, (void *)"success", sizeof("success") );
    ok( err == ERROR_SUCCESS, "got %lu\n", err );

    WaitForSingleObject( info.wait, INFINITE );
    ok( !info.buflen, "got %u\n", info.buflen );

    setup_test( &info, winhttp_close_handle, __LINE__ );

    WinHttpCloseHandle( socket );
    WinHttpCloseHandle( request );
    WinHttpCloseHandle( connection );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok( status == WAIT_TIMEOUT, "got %#lx\n", status );
    }
    WinHttpCloseHandle( session );
    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    if (unload)
    {
        status = WaitForSingleObject( event, 2000 );
        ok( status == WAIT_OBJECT_0, "got %#lx\n", status );
    }
    CloseHandle( event );
    CloseHandle( info.wait );
    end_test( &info, __LINE__ );
}

static const char okmsg[] =
"HTTP/1.1 200 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char page1[] =
"<HTML>\r\n"
"<HEAD><TITLE>winhttp test page</TITLE></HEAD>\r\n"
"<BODY>The quick brown fox jumped over the lazy dog<P></BODY>\r\n"
"</HTML>\r\n\r\n";

struct server_info
{
    HANDLE event;
    int port;
};

static int server_socket;
static HANDLE server_socket_available, server_socket_closed, server_socket_done;

static DWORD CALLBACK server_thread(LPVOID param)
{
    struct server_info *si = param;
    int r, c = -1, i, on;
    SOCKET s;
    struct sockaddr_in sa;
    char buffer[0x100];
    WSADATA wsaData;
    int last_request = 0;

    WSAStartup(MAKEWORD(1,1), &wsaData);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        return 1;

    on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof on);

    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(si->port);
    sa.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    r = bind(s, (struct sockaddr *)&sa, sizeof(sa));
    if (r < 0)
        return 1;

    listen(s, 0);
    SetEvent(si->event);
    do
    {
        if (c == -1) c = accept(s, NULL, NULL);
        ResetEvent(server_socket_closed);
        memset(buffer, 0, sizeof buffer);
        for(i = 0; i < sizeof buffer - 1; i++)
        {
            r = recv(c, &buffer[i], 1, 0);
            if (r != 1)
                break;
            if (i < 4) continue;
            if (buffer[i - 2] == '\n' && buffer[i] == '\n' &&
                buffer[i - 3] == '\r' && buffer[i - 1] == '\r')
                break;
        }
        if (strstr(buffer, "GET /quit"))
        {
            send(c, okmsg, sizeof okmsg - 1, 0);
            send(c, page1, sizeof page1 - 1, 0);
            last_request = 1;
        }
        else if(strstr(buffer, "GET /socket"))
        {
            server_socket = c;
            SetEvent(server_socket_available);
            WaitForSingleObject(server_socket_done, INFINITE);
            ResetEvent(server_socket_available);
        }
        shutdown(c, 2);
        closesocket(c);
        SetEvent(server_socket_closed);
        c = -1;
    } while (!last_request);

    closesocket(s);
    return 0;
}

static void test_basic_request(int port, const WCHAR *verb, const WCHAR *path)
{
    HINTERNET ses, con, req;
    char buffer[0x100];
    DWORD count, status, size;
    BOOL ret;

    ses = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok( ses != NULL, "failed to open session %lu\n", GetLastError() );

    con = WinHttpConnect(ses, L"localhost", port, 0);
    ok( con != NULL, "failed to open a connection %lu\n", GetLastError() );

    req = WinHttpOpenRequest(con, verb, path, NULL, NULL, NULL, 0);
    ok( req != NULL, "failed to open a request %lu\n", GetLastError() );

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok( ret, "failed to send request %lu\n", GetLastError() );

    ret = WinHttpReceiveResponse(req, NULL);
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok( ret, "failed to query status code %lu\n", GetLastError());
    ok( status == HTTP_STATUS_OK, "request failed unexpectedly %lu\n", status );

    count = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = WinHttpReadData(req, buffer, sizeof buffer, &count);
    ok( ret, "failed to read data %lu\n", GetLastError() );
    ok(count == sizeof page1 - 1, "count was wrong\n");
    ok(!memcmp(buffer, page1, sizeof page1), "http data wrong\n");

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static const struct notification open_socket_request_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW }, /* some versions call it twice. why? */
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL }
};

static const struct notification reuse_socket_request_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
};

static void open_async_request(int port, struct test_request *req, struct info *info, const WCHAR *path, BOOL reuse_connection)
{
    BOOL ret;

    info->index = 0;
    if (reuse_connection)
    {
        info->test  = reuse_socket_request_test;
        info->count = ARRAY_SIZE( reuse_socket_request_test );
    }
    else
    {
        info->test  = open_socket_request_test;
        info->count = ARRAY_SIZE( open_socket_request_test );
    }

    req->session = WinHttpOpen( L"winetest", 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok( req->session != NULL, "failed to open session %lu\n", GetLastError() );

    WinHttpSetOption( req->session, WINHTTP_OPTION_CONTEXT_VALUE, &info, sizeof(struct info *) );
    WinHttpSetStatusCallback( req->session, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    setup_test( info, winhttp_connect, __LINE__ );
    req->connection = WinHttpConnect( req->session, L"localhost", port, 0 );
    ok( req->connection != NULL, "failed to open a connection %lu\n", GetLastError() );

    setup_test( info, winhttp_open_request, __LINE__ );
    req->request = WinHttpOpenRequest( req->connection, NULL, path, NULL, NULL, NULL, 0 );
    ok( req->request != NULL, "failed to open a request %lu\n", GetLastError() );

    setup_test( info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req->request, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %lu\n", GetLastError() );
}

static void open_socket_request(int port, struct test_request *req, struct info *info)
{
    ResetEvent( server_socket_done );
    open_async_request( port, req, info, L"/socket", FALSE );
    WaitForSingleObject( server_socket_available, INFINITE );
}

static const struct notification server_reply_test[] =
{
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL }
};

static void server_send_reply(struct test_request *req, struct info *info, const char *msg)
{
    BOOL ret;

    send( server_socket, msg, strlen( msg ), 0 );
    WaitForSingleObject( info->wait, INFINITE );

    info->test  = server_reply_test;
    info->count = ARRAY_SIZE( server_reply_test );
    info->index = 0;
    setup_test( info, winhttp_send_request, __LINE__ );
    ret = WinHttpReceiveResponse( req->request, NULL );
    ok( ret, "failed to receive response %lu\n", GetLastError() );

    WaitForSingleObject( info->wait, INFINITE );
    end_test( info, __LINE__ );
}

#define server_read_data(a) _server_read_data(a,__LINE__)
static void _server_read_data(const char *expect_prefix, unsigned int line)
{
    char buf[1024];
    DWORD size, len;

    size = recv( server_socket, buf, sizeof(buf), 0 );
    len = strlen( expect_prefix );
    ok_(__FILE__,line)(size > len, "data too short\n");
    if (size >= len)
    {
        buf[len] = 0;
        ok_(__FILE__,line)(!strcmp( buf, expect_prefix ), "unexpected data \"%s\"\n", buf);
    }
}

static const struct notification close_request_test[] =
{
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static const struct notification close_allow_connection_close_request_test[] =
{
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void close_request(struct test_request *req, struct info *info, BOOL allow_closing_connection)
{
    BOOL ret;

    if (allow_closing_connection)
    {
        info->test = close_allow_connection_close_request_test;
        info->count = ARRAY_SIZE( close_allow_connection_close_request_test );
    }
    else
    {
        info->test = close_request_test;
        info->count = ARRAY_SIZE( close_request_test );
    }
    info->index = 0;
    setup_test( info, winhttp_close_handle, __LINE__ );

    ret = WinHttpCloseHandle( req->request );
    ok( ret, "WinHttpCloseHandle failed: %lu\n", GetLastError() );
    ret = WinHttpCloseHandle( req->connection );
    ok( ret, "WinHttpCloseHandle failed: %lu\n", GetLastError() );
    ret = WinHttpCloseHandle( req->session );
    ok( ret, "WinHttpCloseHandle failed: %lu\n", GetLastError() );

    WaitForSingleObject( info->wait, INFINITE );
    end_test( info, __LINE__ );
}

static const struct notification read_test[] =
{
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL }
};

#define read_request_data(a,b,c) _read_request_data(a,b,c,__LINE__)
static void _read_request_data(struct test_request *req, struct info *info, const char *expected_data, unsigned line)
{
    char buffer[1024];
    DWORD len;
    BOOL ret;

    info->test = read_test;
    info->count = ARRAY_SIZE( read_test );
    info->index = 0;

    setup_test( info, winhttp_read_data, line );
    memset(buffer, '?', sizeof(buffer));
    ret = WinHttpReadData( req->request, buffer, sizeof(buffer), NULL );
    ok( ret, "failed to read data %lu\n", GetLastError() );

    WaitForSingleObject( info->wait, INFINITE );

    len = strlen(expected_data);
    ok(!memcmp(buffer, expected_data, len), "unexpected data\n");
}

static void test_persistent_connection(int port)
{
    struct test_request req;
    struct info info;

    trace("Testing persistent connection...\n");

    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    open_socket_request( port, &req, &info );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Length: 1\r\n"
                       "\r\n"
                       "X" );
    read_request_data( &req, &info, "X" );
    close_request( &req, &info, FALSE );

    /* chunked connection test */
    open_async_request( port, &req, &info, L"/test", TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "Connection: keep-alive\r\n"
                       "\r\n"
                       "9\r\n123456789\r\n"
                       "0\r\n\r\n" );
    read_request_data( &req, &info, "123456789" );
    close_request( &req, &info, FALSE );

    /* HTTP/1.1 connections are persistent by default, no additional header is needed */
    open_async_request( port, &req, &info, L"/test", TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 2\r\n"
                       "\r\n"
                       "xx" );
    read_request_data( &req, &info, "xx" );
    close_request( &req, &info, FALSE );

    open_async_request( port, &req, &info, L"/test", TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 2\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "yy" );
    read_request_data( &req, &info, "yy" );
    close_request( &req, &info, TRUE );

    SetEvent( server_socket_done );
    CloseHandle( info.wait );
    WaitForSingleObject( server_socket_closed, INFINITE );
}

struct test_recursion_context
{
    HANDLE request;
    HANDLE wait;
    LONG recursion_count, max_recursion_query, max_recursion_read;
    BOOL read_from_callback;
    BOOL have_sync_callback;
    DWORD call_receive_response_status;
    DWORD main_thread_id;
    DWORD receive_response_thread_id;
    BOOL headers_available;
    DWORD total_len;
    BYTE *send_buffer;
};

/* The limit is 128 before Win7 and 3 on newer Windows. */
#define TEST_RECURSION_LIMIT 128

static void CALLBACK test_recursion_callback( HINTERNET handle, DWORD_PTR context_ptr,
                                              DWORD status, void *buffer, DWORD buflen )
{
    struct test_recursion_context *context = (struct test_recursion_context *)context_ptr;
    DWORD err;
    BOOL ret;
    BYTE b;

    switch (status)
    {
        case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            if (status == context->call_receive_response_status)
            {
                if (context->total_len)
                {
                    ret = WinHttpWriteData( context->request, context->send_buffer, context->total_len, NULL );
                    ok(ret, "failed.\n");
                }
                else
                {
                    context->receive_response_thread_id = GetCurrentThreadId();
                    ret = WinHttpReceiveResponse( context->request, NULL );
                    ok( ret, "failed to receive response, GetLastError() %lu\n", GetLastError() );
                }
            }
            break;

        case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
            trace("WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE thread %04lx.\n", GetCurrentThreadId());
            context->receive_response_thread_id = GetCurrentThreadId();
            ret = WinHttpReceiveResponse( context->request, NULL );
            ok( ret, "failed to receive response, GetLastError() %lu\n", GetLastError() );
            break;

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            if (context->call_receive_response_status != WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
                ok( GetCurrentThreadId() != context->main_thread_id,
                    "expected callback to be called from the other thread, got main.\n" );
            context->headers_available = TRUE;
            SetEvent( context->wait );
            break;

        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            DWORD len;

            if (!context->read_from_callback)
            {
                SetEvent( context->wait );
                break;
            }

            if (!*(DWORD *)buffer)
            {
                SetEvent( context->wait );
                break;
            }

            ok( context->recursion_count < TEST_RECURSION_LIMIT,
                "got %lu, thread %#lx\n", context->recursion_count, GetCurrentThreadId() );
            context->max_recursion_query = max( context->max_recursion_query, context->recursion_count );
            InterlockedIncrement( &context->recursion_count );
            b = 0xff;
            len = 0xdeadbeef;
            ret = WinHttpReadData( context->request, &b, 1, &len );
            err = GetLastError();
            ok( ret, "failed to read data, GetLastError() %lu\n", err );
            ok( err == ERROR_SUCCESS || err == ERROR_IO_PENDING, "got %lu\n", err );
            ok( b != 0xff, "got %#x.\n", b );
            ok( len == 1, "got %lu.\n", len );
            if (err == ERROR_SUCCESS) context->have_sync_callback = TRUE;
            InterlockedDecrement( &context->recursion_count );
            break;
        }

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            static DWORD len;

            if (!buflen)
            {
                SetEvent( context->wait );
                break;
            }
            ok( context->recursion_count < TEST_RECURSION_LIMIT,
                "got %lu, thread %#lx\n", context->recursion_count, GetCurrentThreadId() );
            context->max_recursion_read = max( context->max_recursion_read, context->recursion_count );
            context->read_from_callback = TRUE;
            InterlockedIncrement( &context->recursion_count );
            len = 0xdeadbeef;
            /* Use static variable len here so write to it doesn't destroy the stack on old Windows which
             * doesn't set the value at once. */
            ret = WinHttpQueryDataAvailable( context->request, &len );
            err = GetLastError();
            ok( ret, "failed to query data available, GetLastError() %lu\n", err );
            ok( err == ERROR_SUCCESS || err == ERROR_IO_PENDING, "got %lu\n", err );
            ok( len != 0xdeadbeef || broken( len == 0xdeadbeef ) /* Win7 */, "got %lu.\n", len );
            if (err == ERROR_SUCCESS) context->have_sync_callback = TRUE;
            InterlockedDecrement( &context->recursion_count );
            break;
        }

        case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
            if (!context->headers_available
                && context->call_receive_response_status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
                ok( GetCurrentThreadId() == context->receive_response_thread_id,
                    "expected callback to be called from the same thread, got %lx.\n", GetCurrentThreadId() );
            break;
    }
}

static void test_recursion(void)
{
    static DWORD request_callback_status_tests[] =
    {
        WINHTTP_CALLBACK_STATUS_SENDING_REQUEST,
        WINHTTP_CALLBACK_STATUS_REQUEST_SENT,
    };
    struct test_recursion_context context;
    HANDLE session, connection, request;
    DWORD size, status, err;
    char buffer[1024];
    unsigned int i;
    BOOL ret;
    BYTE b;

    memset( &context, 0, sizeof(context) );

    context.wait = CreateEventW( NULL, FALSE, FALSE, NULL );
    context.main_thread_id = GetCurrentThreadId();

    session = WinHttpOpen( L"winetest", 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok( !!session, "failed to open session, GetLastError() %lu\n", GetLastError() );

    WinHttpSetStatusCallback( session, test_recursion_callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    connection = WinHttpConnect( session, L"test.winehq.org", 0, 0 );
    ok( !!connection, "failed to open a connection, GetLastError() %lu\n", GetLastError() );

    request = WinHttpOpenRequest( connection, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
    ok( !!request, "failed to open a request, GetLastError() %lu\n", GetLastError() );

    context.request = request;

    ret = WinHttpReceiveResponse( request, NULL );
    ok( ret, "failed to receive response, GetLastError() %lu\n", GetLastError() );

    context.call_receive_response_status = WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE;
    context.total_len = 1;
    context.send_buffer = &b;
    b = 0;
    ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, context.total_len, (DWORD_PTR)&context );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("Connection failed, skipping\n");
        WinHttpSetStatusCallback( session, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
        WinHttpCloseHandle( request );
        WinHttpCloseHandle( connection );
        WinHttpCloseHandle( session );
        CloseHandle( context.wait );
        return;
    }
    ok( ret, "failed to send request, GetLastError() %lu\n", GetLastError() );

    WaitForSingleObject( context.wait, INFINITE );
    context.total_len = 0;
    context.send_buffer = NULL;

    size = sizeof(status);
    ret = WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL,
                               &status, &size, NULL );
    ok( ret, "request failed, GetLastError() %lu\n", GetLastError() );
    ok( status == 200, "request failed unexpectedly, status %lu\n", status );

    ret = WinHttpQueryDataAvailable( request, NULL );
    ok( ret, "failed to query data available, GetLastError() %lu\n", GetLastError() );

    WaitForSingleObject( context.wait, INFINITE );

    ret = WinHttpReadData( request, &b, 1, NULL );
    ok( ret, "failed to read data, GetLastError() %lu\n", GetLastError() );

    WaitForSingleObject( context.wait, INFINITE );
    if (context.have_sync_callback)
    {
        ok( context.max_recursion_query >= 2, "got %lu\n", context.max_recursion_query );
        ok( context.max_recursion_read >= 2, "got %lu\n", context.max_recursion_read );
    }
    else skip( "no sync callbacks\n");

    WinHttpCloseHandle( request );

    for (i = 0; i < ARRAY_SIZE(request_callback_status_tests); ++i)
    {
        winetest_push_context( "i %u", i );

        request = WinHttpOpenRequest( connection, NULL, L"/tests/hello.html", NULL, NULL, NULL, 0 );
        ok( !!request, "failed to open a request, GetLastError() %lu\n", GetLastError() );

        context.request = request;
        context.call_receive_response_status = request_callback_status_tests[i];
        context.headers_available = FALSE;

        ret = WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, (DWORD_PTR)&context );
        err = GetLastError();
        if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
        {
            skip("Connection failed, skipping\n");
            WinHttpSetStatusCallback( session, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
            WinHttpCloseHandle( request );
            WinHttpCloseHandle( connection );
            WinHttpCloseHandle( session );
            CloseHandle( context.wait );
            winetest_pop_context();
            return;
        }

        WaitForSingleObject( context.wait, INFINITE );

        ret = WinHttpReadData( request, buffer, sizeof(buffer), NULL );
        ok( ret, "failed to read data, GetLastError() %lu\n", GetLastError() );

        WaitForSingleObject( context.wait, INFINITE );

        WinHttpCloseHandle( request );
        winetest_pop_context();
    }

    WinHttpSetStatusCallback( session, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
    WinHttpCloseHandle( connection );
    WinHttpCloseHandle( session );
    CloseHandle( context.wait );
}

START_TEST (notification)
{
    HMODULE mod = GetModuleHandleA( "winhttp.dll" );
    struct server_info si;
    HANDLE thread;
    DWORD ret;

    pWinHttpWebSocketClose = (void *)GetProcAddress( mod, "WinHttpWebSocketClose" );
    pWinHttpWebSocketCompleteUpgrade = (void *)GetProcAddress( mod, "WinHttpWebSocketCompleteUpgrade" );
    pWinHttpWebSocketQueryCloseStatus = (void *)GetProcAddress( mod, "WinHttpWebSocketQueryCloseStatus" );
    pWinHttpWebSocketReceive = (void *)GetProcAddress( mod, "WinHttpWebSocketReceive" );
    pWinHttpWebSocketSend = (void *)GetProcAddress( mod, "WinHttpWebSocketSend" );
    pWinHttpWebSocketShutdown = (void *)GetProcAddress( mod, "WinHttpWebSocketShutdown" );

    test_connection_cache( FALSE );
    test_redirect( FALSE );
    winetest_push_context( "async" );
    test_connection_cache( TRUE );
    test_redirect( TRUE );
    winetest_pop_context();
    test_async();
    test_websocket( FALSE );
    winetest_push_context( "secure" );
    test_websocket( TRUE );
    winetest_pop_context();
    test_recursion();

    si.event = CreateEventW( NULL, 0, 0, NULL );
    si.port = 7533;

    thread = CreateThread( NULL, 0, server_thread, &si, 0, NULL );
    ok( thread != NULL, "failed to create thread %lu\n", GetLastError() );

    server_socket_available = CreateEventW( NULL, 0, 0, NULL );
    server_socket_closed = CreateEventW( NULL, 0, 0, NULL );
    server_socket_done = CreateEventW( NULL, 0, 0, NULL );

    ret = WaitForSingleObject( si.event, 10000 );
    ok( ret == WAIT_OBJECT_0, "failed to start winhttp test server %lu\n", GetLastError() );
    if (ret != WAIT_OBJECT_0)
    {
        CloseHandle(thread);
        return;
    }

    test_persistent_connection( si.port );

    /* send the basic request again to shutdown the server thread */
    test_basic_request( si.port, NULL, L"/quit" );

    WaitForSingleObject( thread, 3000 );
    CloseHandle( thread );
    CloseHandle( server_socket_available );
    CloseHandle( server_socket_done );
    CloseHandle( server_socket_closed );
}
