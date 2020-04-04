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

static const WCHAR user_agent[] = {'w','i','n','e','t','e','s','t',0};
static const WCHAR test_winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR tests_hello_html[] = {'/','t','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};
static const WCHAR tests_redirect[] = {'/','t','e','s','t','s','/','r','e','d','i','r','e','c','t',0};
static const WCHAR localhostW[] = {'l','o','c','a','l','h','o','s','t',0};

enum api
{
    winhttp_connect = 1,
    winhttp_open_request,
    winhttp_send_request,
    winhttp_receive_response,
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

struct info
{
    enum api function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
    HANDLE wait;
    unsigned int line;
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

    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        WinHttpQueryOption( handle, WINHTTP_OPTION_CONTEXT_VALUE, &info, &size );
    }
    while (info->index < info->count && info->test[info->index].status != status && (info->test[info->index].flags & NF_ALLOW))
        info->index++;
    while (info->index < info->count && (info->test[info->index].flags & NF_WINE_ALLOW))
    {
        todo_wine ok(info->test[info->index].status != status, "unexpected %x notification\n", status);
        if (info->test[info->index].status == status) break;
        info->index++;
    }
    ok(info->index < info->count, "%u: unexpected notification 0x%08x\n", info->line, status);
    if (info->index >= info->count) return;

    status_ok   = (info->test[info->index].status == status);
    function_ok = (info->test[info->index].function == info->function);
    ok(status_ok, "%u: expected status 0x%08x got 0x%08x\n", info->line, info->test[info->index].status, status);
    ok(function_ok, "%u: expected function %u got %u\n", info->line, info->test[info->index].function, info->function);

    if (status_ok && function_ok && info->test[info->index++].flags & NF_SIGNAL)
    {
        SetEvent( info->wait );
    }
}

static const struct notification cache_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
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
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
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
}

static void end_test( struct info *info, unsigned int line )
{
    ok_(__FILE__,line)(info->index == info->count, "some notifications were missing: %x\n",
                       info->test[info->index].status);
}

static void test_connection_cache( void )
{
    HANDLE ses, con, req, event;
    DWORD size, status, err;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;

    info.test  = cache_test;
    info.count = ARRAY_SIZE( cache_test );
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip("Unload event not supported\n");
        unload = FALSE;
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
    }


    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    if (unload)
    {
        ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
        ok(ret, "failed to set unload option\n");
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    setup_test( &info, winhttp_close_handle, __LINE__ );
done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );
    end_test( &info, __LINE__ );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
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
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void test_redirect( void )
{
    HANDLE ses, con, req;
    DWORD size, status, err;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = redirect_test;
    info.count = ARRAY_SIZE( redirect_test );
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_redirect, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

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
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL },
    { winhttp_query_data,       WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, NF_SIGNAL },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
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

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

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
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    SetLastError( 0xdeadbeef );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    err = GetLastError();
    ok(ret, "failed to set context value %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_connect, __LINE__ );
    SetLastError( 0xdeadbeef );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    err = GetLastError();
    ok(con != NULL, "failed to open a connection %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == WSAEINVAL) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_open_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    err = GetLastError();
    ok(req != NULL, "failed to open a request %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

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
    ok(ret, "failed to send request %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpReceiveResponse( req, NULL );
    err = GetLastError();
    ok(ret, "failed to receive response %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    err = GetLastError();
    ok(ret, "failed unexpectedly %u\n", err);
    ok(status == 200, "request failed unexpectedly %u\n", status);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_query_data, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryDataAvailable( req, NULL );
    err = GetLastError();
    ok(ret, "failed to query data available %u\n", err);
    ok(err == ERROR_SUCCESS || err == ERROR_IO_PENDING || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_read_data, __LINE__ );
    ret = WinHttpReadData( req, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to read data %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    end_test( &info, __LINE__ );

    if (unload)
    {
        status = WaitForSingleObject( event, 2000 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
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
static HANDLE server_socket_available, server_socket_done;

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
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, verb, path, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    count = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = WinHttpReadData(req, buffer, sizeof buffer, &count);
    ok(ret, "failed to read data %u\n", GetLastError());
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

    req->session = WinHttpOpen( user_agent, 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok(req->session != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetOption( req->session, WINHTTP_OPTION_CONTEXT_VALUE, &info, sizeof(struct info *) );
    WinHttpSetStatusCallback( req->session, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    setup_test( info, winhttp_connect, __LINE__ );
    req->connection = WinHttpConnect( req->session, localhostW, port, 0 );
    ok(req->connection != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( info, winhttp_open_request, __LINE__ );
    req->request = WinHttpOpenRequest( req->connection, NULL, path, NULL, NULL, NULL, 0 );
    ok(req->request != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req->request, NULL, 0, NULL, 0, 0, 0 );
    ok(ret, "failed to send request %u\n", GetLastError());
}

static void open_socket_request(int port, struct test_request *req, struct info *info)
{
    static const WCHAR socketW[] = {'/','s','o','c','k','e','t',0};

    ResetEvent( server_socket_done );
    open_async_request( port, req, info, socketW, FALSE );
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
    ok(ret, "failed to receive response %u\n", GetLastError());

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
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_ALLOW },
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
    ok(ret, "WinHttpCloseHandle failed: %u\n", GetLastError());
    ret = WinHttpCloseHandle( req->connection );
    ok(ret, "WinHttpCloseHandle failed: %u\n", GetLastError());
    ret = WinHttpCloseHandle( req->session );
    ok(ret, "WinHttpCloseHandle failed: %u\n", GetLastError());

    WaitForSingleObject( info->wait, INFINITE );
    end_test( info, __LINE__ );
}

static const struct notification read_test[] =
{
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL }
};

static const struct notification read_allow_close_test[] =
{
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL }
};

#define read_request_data(a,b,c,d) _read_request_data(a,b,c,d,__LINE__)
static void _read_request_data(struct test_request *req, struct info *info, const char *expected_data, BOOL closing_connection, unsigned line)
{
    char buffer[1024];
    DWORD len;
    BOOL ret;

    if (closing_connection)
    {
        info->test = read_allow_close_test;
        info->count = ARRAY_SIZE( read_allow_close_test );
    }
    else
    {
        info->test = read_test;
        info->count = ARRAY_SIZE( read_test );
    }
    info->index = 0;

    setup_test( info, winhttp_read_data, line );
    memset(buffer, '?', sizeof(buffer));
    ret = WinHttpReadData( req->request, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to read data %u\n", GetLastError());

    WaitForSingleObject( info->wait, INFINITE );

    len = strlen(expected_data);
    ok(!memcmp(buffer, expected_data, len), "unexpected data\n");
}

static void test_persistent_connection(int port)
{
    struct test_request req;
    struct info info;

    static const WCHAR testW[] = {'/','t','e','s','t',0};

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
    read_request_data( &req, &info, "X", FALSE );
    close_request( &req, &info, FALSE );

    /* chunked connection test */
    open_async_request( port, &req, &info, testW, TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "Connection: keep-alive\r\n"
                       "\r\n"
                       "9\r\n123456789\r\n"
                       "0\r\n\r\n" );
    read_request_data( &req, &info, "123456789", FALSE );
    close_request( &req, &info, FALSE );

    /* HTTP/1.1 connections are persistent by default, no additional header is needed */
    open_async_request( port, &req, &info, testW, TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 2\r\n"
                       "\r\n"
                       "xx" );
    read_request_data( &req, &info, "xx", FALSE );
    close_request( &req, &info, FALSE );

    open_async_request( port, &req, &info, testW, TRUE );
    server_read_data( "GET /test HTTP/1.1\r\n" );
    server_send_reply( &req, &info,
                       "HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 2\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "yy" );
    close_request( &req, &info, TRUE );

    SetEvent( server_socket_done );
    CloseHandle( info.wait );
}

START_TEST (notification)
{
    static const WCHAR quitW[] = {'/','q','u','i','t',0};
    struct server_info si;
    HANDLE thread;
    DWORD ret;

    test_connection_cache();
    test_redirect();
    test_async();

    si.event = CreateEventW( NULL, 0, 0, NULL );
    si.port = 7533;

    thread = CreateThread( NULL, 0, server_thread, &si, 0, NULL );
    ok(thread != NULL, "failed to create thread %u\n", GetLastError());

    server_socket_available = CreateEventW( NULL, 0, 0, NULL );
    server_socket_done = CreateEventW( NULL, 0, 0, NULL );

    ret = WaitForSingleObject( si.event, 10000 );
    ok(ret == WAIT_OBJECT_0, "failed to start winhttp test server %u\n", GetLastError());
    if (ret != WAIT_OBJECT_0)
    {
        CloseHandle(thread);
        return;
    }

    test_persistent_connection( si.port );

    /* send the basic request again to shutdown the server thread */
    test_basic_request( si.port, NULL, quitW );

    WaitForSingleObject( thread, 3000 );
    CloseHandle( thread );
    CloseHandle( server_socket_available );
    CloseHandle( server_socket_done );
}
