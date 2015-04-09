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
#include <winhttp.h>

#include "wine/test.h"

static const WCHAR user_agent[] = {'w','i','n','e','t','e','s','t',0};
static const WCHAR test_winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};

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
    BOOL todo;
    BOOL ignore;
    BOOL skipped_for_proxy;
};

struct info
{
    enum api function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
    HANDLE wait;
    unsigned int line;
};

static BOOL proxy_active(void)
{
    WINHTTP_PROXY_INFO proxy_info;
    BOOL active = FALSE;

    if (WinHttpGetDefaultProxyConfiguration(&proxy_info))
    {
        active = (proxy_info.lpszProxy != NULL);
        if (active)
            GlobalFree(proxy_info.lpszProxy);
        if (proxy_info.lpszProxyBypass != NULL)
            GlobalFree(proxy_info.lpszProxyBypass);
    }
    else
       active = FALSE;

    return active;
}

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    BOOL status_ok, function_ok;
    struct info *info = (struct info *)context;
    unsigned int i = info->index;

    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        WinHttpQueryOption( handle, WINHTTP_OPTION_CONTEXT_VALUE, &info, &size );
    }
    ok(i < info->count, "%u: unexpected notification 0x%08x\n", info->line, status);
    if (i >= info->count) return;

    status_ok   = (info->test[i].status == status);
    function_ok = (info->test[i].function == info->function);
    if (!info->test[i].ignore && !info->test[i].todo)
    {
        ok(status_ok, "%u: expected status 0x%08x got 0x%08x\n", info->line, info->test[i].status, status);
        ok(function_ok, "%u: expected function %u got %u\n", info->line, info->test[i].function, info->function);
    }
    else if (!info->test[i].ignore)
    {
        todo_wine ok(status_ok, "%u: expected status 0x%08x got 0x%08x\n", info->line, info->test[i].status, status);
        if (status_ok)
        {
            todo_wine ok(function_ok, "%u: expected function %u got %u\n", info->line, info->test[i].function, info->function);
        }
    }
    if (status_ok && function_ok) info->index++;
    if (proxy_active())
    {
        while (info->test[info->index].skipped_for_proxy)
            info->index++;
    }

    if (status & (WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING))
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
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, FALSE, TRUE },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE }
};

static void setup_test( struct info *info, enum api function, unsigned int line )
{
    info->function = function;
    info->line = line;
}

static void test_connection_cache( void )
{
    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = cache_test;
    info.count = sizeof(cache_test) / sizeof(cache_test[0]);
    info.index = 0;
    info.wait = NULL;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
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
    WinHttpCloseHandle( req );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
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
    WinHttpCloseHandle( req );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );

    Sleep(2000); /* make sure connection is evicted from cache */

    info.index = 0;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
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
    WinHttpCloseHandle( req );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
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

    Sleep(2000); /* make sure connection is evicted from cache */
}

static const struct notification redirect_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, FALSE, TRUE },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, FALSE, TRUE },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, FALSE, TRUE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, FALSE, TRUE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, FALSE, FALSE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, FALSE, FALSE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE }
};

static void test_redirect( void )
{
    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = redirect_test;
    info.count = sizeof(redirect_test) / sizeof(redirect_test[0]);
    info.index = 0;
    info.wait = NULL;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
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
}

static const struct notification async_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, FALSE, TRUE },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, FALSE, TRUE },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, FALSE, TRUE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, FALSE, TRUE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, FALSE, FALSE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, FALSE, FALSE, TRUE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE },
    { winhttp_query_data,       WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, FALSE, TRUE },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, FALSE, TRUE },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, FALSE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, TRUE, TRUE }
};

static void test_async( void )
{
    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;
    struct info info, *context = &info;
    char buffer[1024];

    info.test  = async_test;
    info.count = sizeof(async_test) / sizeof(async_test[0]);
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
    {
        skip("connection failed, skipping\n");
        WinHttpCloseHandle( req );
        WinHttpCloseHandle( con );
        WinHttpCloseHandle( ses );
        CloseHandle( info.wait );
        return;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    setup_test( &info, winhttp_query_data, __LINE__ );
    ret = WinHttpQueryDataAvailable( req, NULL );
    ok(ret, "failed to query data available %u\n", GetLastError());

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_read_data, __LINE__ );
    ret = WinHttpReadData( req, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to query data available %u\n", GetLastError());

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );

    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );
}

START_TEST (notification)
{
    test_connection_cache();
    test_redirect();
    Sleep(2000); /* make sure previous connection is evicted from cache */
    test_async();
}
