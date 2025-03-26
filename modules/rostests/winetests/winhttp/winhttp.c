/*
 * WinHTTP - tests
 *
 * Copyright 2008 Google (Zac Brown)
 * Copyright 2015 Dmitry Timoshkov
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

#define COBJMACROS
#include <stdarg.h>
#include <windef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <winreg.h>
#include <stdio.h>
#include <initguid.h>
#include <httprequest.h>
#include <httprequestid.h>

#include "wine/test.h"
#include "wine/heap.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const WCHAR test_useragent[] =
    {'W','i','n','e',' ','R','e','g','r','e','s','s','i','o','n',' ','T','e','s','t',0};
static const WCHAR test_winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR test_winehq_https[] = {'h','t','t','p','s',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',':','4','4','3',0};
static const WCHAR localhostW[] = {'l','o','c','a','l','h','o','s','t',0};

static WCHAR *a2w(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    WCHAR *ret = heap_alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static int strcmp_wa(const WCHAR *str1, const char *stra)
{
    WCHAR *str2 = a2w(stra);
    int r = lstrcmpW(str1, str2);
    heap_free(str2);
    return r;
}

static BOOL proxy_active(void)
{
    WINHTTP_PROXY_INFO proxy_info;
    BOOL active = FALSE;

    SetLastError(0xdeadbeef);
    if (WinHttpGetDefaultProxyConfiguration(&proxy_info))
    {
        ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
           "got %u\n", GetLastError());
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

static void test_WinHttpQueryOption(void)
{
    BOOL ret;
    HINTERNET session, request, connection;
    DWORD feature, size;

    SetLastError(0xdeadbeef);
    session = WinHttpOpen(test_useragent, 0, 0, 0, 0);
    ok(session != NULL, "WinHttpOpen failed to open session, error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(session, WINHTTP_OPTION_REDIRECT_POLICY, NULL, NULL);
    ok(!ret, "should fail to set redirect policy %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(session, WINHTTP_OPTION_REDIRECT_POLICY, NULL, &size);
    ok(!ret, "should fail to query option\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
    ok(size == 4, "expected 4, got %u\n", size);

    feature = 0xdeadbeef;
    size = sizeof(feature) - 1;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, &size);
    ok(!ret, "should fail to query option\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
    ok(size == 4, "expected 4, got %u\n", size);

    feature = 0xdeadbeef;
    size = sizeof(feature) + 1;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, &size);
    ok(ret, "failed to query option %u\n", GetLastError());
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
       "got %u\n", GetLastError());
    ok(size == sizeof(feature), "WinHttpQueryOption should set the size: %u\n", size);
    ok(feature == WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP,
       "expected WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP, got %#x\n", feature);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, NULL, sizeof(feature));
    ok(!ret, "should fail to set redirect policy %u\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    feature = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, sizeof(feature) - 1);
    ok(!ret, "should fail to set redirect policy %u\n", GetLastError());
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    feature = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, sizeof(feature) + 1);
    ok(!ret, "should fail to set redirect policy %u\n", GetLastError());
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    feature = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, sizeof(feature));
    ok(ret, "failed to set redirect policy %u\n", GetLastError());

    feature = 0xdeadbeef;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &feature, &size);
    ok(ret, "failed to query option %u\n", GetLastError());
    ok(feature == WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS,
       "expected WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS, got %#x\n", feature);

    feature = WINHTTP_DISABLE_COOKIES;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(session, WINHTTP_OPTION_DISABLE_FEATURE, &feature, sizeof(feature));
    ok(!ret, "should fail to set disable feature for a session\n");
    ok(GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INCORRECT_HANDLE_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    connection = WinHttpConnect(session, test_winehq, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u\n", GetLastError());

    feature = WINHTTP_DISABLE_COOKIES;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(connection, WINHTTP_OPTION_DISABLE_FEATURE, &feature, sizeof(feature));
    ok(!ret, "should fail to set disable feature for a connection\n");
    ok(GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INCORRECT_HANDLE_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    request = WinHttpOpenRequest(connection, NULL, NULL, NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping the test\n");
        goto done;
    }

    feature = 0xdeadbeef;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &feature, &size);
    ok(!ret, "should fail to query disable feature for a request\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    feature = 0;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &feature, sizeof(feature));
    ok(ret, "failed to set feature %u\n", GetLastError());

    feature = 0xffffffff;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &feature, sizeof(feature));
    ok(ret, "failed to set feature %u\n", GetLastError());

    feature = WINHTTP_DISABLE_COOKIES;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &feature, sizeof(feature));
    ok(ret, "failed to set feature %u\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(request, WINHTTP_OPTION_DISABLE_FEATURE, NULL, &size);
    ok(!ret, "should fail to query disable feature for a request\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    feature = 0xdeadbeef;
    size = sizeof(feature);
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(request, WINHTTP_OPTION_ENABLE_FEATURE, &feature, &size);
    ok(!ret, "should fail to query enabled features for a request\n");
    ok(feature == 0xdeadbeef, "expect feature 0xdeadbeef, got %u\n", feature);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    feature = WINHTTP_ENABLE_SSL_REVOCATION;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_ENABLE_FEATURE, 0, sizeof(feature));
    ok(!ret, "should fail to enable WINHTTP_ENABLE_SSL_REVOCATION with invalid parameters\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_ENABLE_FEATURE, &feature, 0);
    ok(!ret, "should fail to enable WINHTTP_ENABLE_SSL_REVOCATION with invalid parameters\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_ENABLE_FEATURE, &feature, sizeof(feature));
    ok(ret, "failed to set feature\n");
    ok(GetLastError() == NO_ERROR || broken(GetLastError() == 0xdeadbeef), /* Doesn't set error code on Vista or older */
       "expected NO_ERROR, got %u\n", GetLastError());

    feature = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_ENABLE_FEATURE, &feature, sizeof(feature));
    ok(!ret, "should fail to enable WINHTTP_ENABLE_SSL_REVOCATION with invalid parameters\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    feature = 6;
    size = sizeof(feature);
    ret = WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_RETRIES, &feature, sizeof(feature));
    ok(ret, "failed to set WINHTTP_OPTION_CONNECT_RETRIES %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpCloseHandle(request);
    ok(ret, "WinHttpCloseHandle failed on closing request: %u\n", GetLastError());

done:
    SetLastError(0xdeadbeef);
    ret = WinHttpCloseHandle(connection);
    ok(ret, "WinHttpCloseHandle failed on closing connection: %u\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = WinHttpCloseHandle(session);
    ok(ret, "WinHttpCloseHandle failed on closing session: %u\n", GetLastError());
}

static void test_WinHttpOpenRequest (void)
{
    BOOL ret;
    HINTERNET session, request, connection;
    DWORD err;

    SetLastError(0xdeadbeef);
    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    err = GetLastError();
    ok(session != NULL, "WinHttpOpen failed to open session.\n");
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    /* Test with a bad server name */
    SetLastError(0xdeadbeef);
    connection = WinHttpConnect(session, NULL, INTERNET_DEFAULT_HTTP_PORT, 0);
    err = GetLastError();
    ok (connection == NULL, "WinHttpConnect succeeded in opening connection to NULL server argument.\n");
    ok(err == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u.\n", err);

    /* Test with a valid server name */
    SetLastError(0xdeadbeef);
    connection = WinHttpConnect (session, test_winehq, INTERNET_DEFAULT_HTTP_PORT, 0);
    err = GetLastError();
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u.\n", err);
    ok(err == ERROR_SUCCESS || broken(err == WSAEINVAL) /* < win7 */, "got %u\n", err);

    SetLastError(0xdeadbeef);
    request = WinHttpOpenRequest(connection, NULL, NULL, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    err = GetLastError();
    if (request == NULL && err == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    ok(request != NULL, "WinHttpOpenrequest failed to open a request, error: %u.\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("Connection failed, skipping.\n");
        goto done;
    }
    ok(ret, "WinHttpSendRequest failed: %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpCloseHandle(request);
    err = GetLastError();
    ok(ret, "WinHttpCloseHandle failed on closing request, got %u.\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

 done:
    ret = WinHttpCloseHandle(connection);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);

}

static void test_empty_headers_param(void)
{
    static const WCHAR empty[]  = {0};
    HINTERNET ses, con, req;
    DWORD err;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, test_winehq, 80, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, empty, 0, NULL, 0, 0, 0);
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

 done:
    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_WinHttpSendRequest (void)
{
    static const WCHAR content_type[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','a','p','p','l','i','c','a','t','i','o','n',
         '/','x','-','w','w','w','-','f','o','r','m','-','u','r','l','e','n','c','o','d','e','d',0};
    static const WCHAR test_file[] = {'t','e','s','t','s','/','p','o','s','t','.','p','h','p',0};
    static const WCHAR postW[] = {'P','O','S','T',0};
    static CHAR post_data[] = "mode=Test";
    static const char test_post[] = "mode => Test\0\n";
    HINTERNET session, request, connection;
    DWORD header_len, optional_len, total_len, bytes_rw, size, err, disable, len;
    DWORD_PTR context;
    BOOL ret;
    CHAR buffer[256];
    WCHAR method[8];
    int i;

    header_len = -1L;
    total_len = optional_len = sizeof(post_data);
    memset(buffer, 0xff, sizeof(buffer));

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    ok(session != NULL, "WinHttpOpen failed to open session.\n");

    connection = WinHttpConnect (session, test_winehq, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, postW, test_file, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    ok(request != NULL, "WinHttpOpenrequest failed to open a request, error: %u.\n", GetLastError());
    if (!request) goto done;

    method[0] = 0;
    len = sizeof(method);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_REQUEST_METHOD, NULL, method, &len, NULL);
    ok(ret, "got %u\n", GetLastError());
    ok(len == lstrlenW(postW) * sizeof(WCHAR), "got %u\n", len);
    ok(!lstrcmpW(method, postW), "got %s\n", wine_dbgstr_w(method));

    context = 0xdeadbeef;
    ret = WinHttpSetOption(request, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context));
    ok(ret, "WinHttpSetOption failed: %u\n", GetLastError());

    /* writing more data than promised by the content-length header causes an error when the connection
       is resued, so disable keep-alive */
    disable = WINHTTP_DISABLE_KEEP_ALIVE;
    ret = WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable));
    ok(ret, "WinHttpSetOption failed: %u\n", GetLastError());

    context++;
    ret = WinHttpSendRequest(request, content_type, header_len, post_data, optional_len, total_len, context);
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret == TRUE, "WinHttpSendRequest failed: %u\n", GetLastError());

    context = 0;
    size = sizeof(context);
    ret = WinHttpQueryOption(request, WINHTTP_OPTION_CONTEXT_VALUE, &context, &size);
    ok(ret, "WinHttpQueryOption failed: %u\n", GetLastError());
    ok(context == 0xdeadbef0, "expected 0xdeadbef0, got %lx\n", context);

    for (i = 3; post_data[i]; i++)
    {
        bytes_rw = -1;
        SetLastError(0xdeadbeef);
        ret = WinHttpWriteData(request, &post_data[i], 1, &bytes_rw);
        if (ret)
        {
          ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %u.\n", GetLastError());
          ok(bytes_rw == 1, "WinHttpWriteData failed, wrote %u bytes instead of 1 byte.\n", bytes_rw);
        }
        else /* Since we already passed all optional data in WinHttpSendRequest Win7 fails our WinHttpWriteData call */
        {
          ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER got %u.\n", GetLastError());
          ok(bytes_rw == -1, "Expected bytes_rw to remain unchanged.\n");
        }
    }

    SetLastError(0xdeadbeef);
    ret = WinHttpReceiveResponse(request, NULL);
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == ERROR_NO_TOKEN) /* < win7 */,
       "Expected ERROR_SUCCESS got %u.\n", GetLastError());
    ok(ret == TRUE, "WinHttpReceiveResponse failed: %u.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_ORIG_URI, NULL, NULL, &len, NULL);
    ok(!ret && GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_MAX + 1, NULL, NULL, &len, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %u\n", GetLastError());

    bytes_rw = -1;
    ret = WinHttpReadData(request, buffer, sizeof(buffer) - 1, &bytes_rw);
    ok(ret == TRUE, "WinHttpReadData failed: %u.\n", GetLastError());

    ok(bytes_rw == sizeof(test_post) - 1, "Read %u bytes\n", bytes_rw);
    ok(!memcmp(buffer, test_post, sizeof(test_post) - 1), "Data read did not match.\n");

 done:
    ret = WinHttpCloseHandle(request);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);
    ret = WinHttpCloseHandle(connection);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);
}

static void test_WinHttpTimeFromSystemTime(void)
{
    BOOL ret;
    static const SYSTEMTIME time = {2008, 7, 1, 28, 10, 5, 52, 0};
    static const WCHAR expected_string[] =
        {'M','o','n',',',' ','2','8',' ','J','u','l',' ','2','0','0','8',' ',
         '1','0',':','0','5',':','5','2',' ','G','M','T',0};
    WCHAR time_string[WINHTTP_TIME_FORMAT_BUFSIZE+1];
    DWORD err;

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeFromSystemTime(&time, NULL);
    err = GetLastError();
    ok(!ret, "WinHttpTimeFromSystemTime succeeded\n");
    ok(err == ERROR_INVALID_PARAMETER, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeFromSystemTime(NULL, time_string);
    err = GetLastError();
    ok(!ret, "WinHttpTimeFromSystemTime succeeded\n");
    ok(err == ERROR_INVALID_PARAMETER, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeFromSystemTime(&time, time_string);
    err = GetLastError();
    ok(ret, "WinHttpTimeFromSystemTime failed: %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);
    ok(memcmp(time_string, expected_string, sizeof(expected_string)) == 0,
        "Time string returned did not match expected time string.\n");
}

static void test_WinHttpTimeToSystemTime(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expected_time = {2008, 7, 1, 28, 10, 5, 52, 0};
    static const WCHAR time_string1[] =
        {'M','o','n',',',' ','2','8',' ','J','u','l',' ','2','0','0','8',' ',
         +          '1','0',':','0','5',':','5','2',' ','G','M','T','\n',0};
    static const WCHAR time_string2[] =
        {' ','m','o','n',' ','2','8',' ','j','u','l',' ','2','0','0','8',' ',
         '1','0',' ','0','5',' ','5','2','\n',0};
    DWORD err;

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeToSystemTime(time_string1, NULL);
    err = GetLastError();
    ok(!ret, "WinHttpTimeToSystemTime succeeded\n");
    ok(err == ERROR_INVALID_PARAMETER, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeToSystemTime(NULL, &time);
    err = GetLastError();
    ok(!ret, "WinHttpTimeToSystemTime succeeded\n");
    ok(err == ERROR_INVALID_PARAMETER, "got %u\n", err);

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeToSystemTime(time_string1, &time);
    err = GetLastError();
    ok(ret, "WinHttpTimeToSystemTime failed: %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);
    ok(memcmp(&time, &expected_time, sizeof(SYSTEMTIME)) == 0,
        "Returned SYSTEMTIME structure did not match expected SYSTEMTIME structure.\n");

    SetLastError(0xdeadbeef);
    ret = WinHttpTimeToSystemTime(time_string2, &time);
    err = GetLastError();
    ok(ret, "WinHttpTimeToSystemTime failed: %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);
    ok(memcmp(&time, &expected_time, sizeof(SYSTEMTIME)) == 0,
        "Returned SYSTEMTIME structure did not match expected SYSTEMTIME structure.\n");
}

static void test_WinHttpAddHeaders(void)
{
    HINTERNET session, request, connection;
    BOOL ret, reverse;
    WCHAR buffer[MAX_PATH];
    WCHAR check_buffer[MAX_PATH];
    DWORD err, index, len, oldlen;

    static const WCHAR test_file[] = {'/','p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR test_verb[] = {'P','O','S','T',0};
    static const WCHAR test_header_begin[] =
        {'P','O','S','T',' ','/','p','o','s','t','t','e','s','t','.','p','h','p',' ','H','T','T','P','/','1'};
    static const WCHAR full_path_test_header_begin[] =
        {'P','O','S','T',' ','h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',':','8','0',
         '/','p','o','s','t','t','e','s','t','.','p','h','p',' ','H','T','T','P','/','1'};
    static const WCHAR test_header_end[] = {'\r','\n','\r','\n',0};
    static const WCHAR test_header_name[] = {'W','a','r','n','i','n','g',0};
    static const WCHAR test_header_name2[] = {'n','a','m','e',0};
    static const WCHAR test_header_name3[] = {'a',0};
    static const WCHAR test_header_range[] = {'R','a','n','g','e',0};
    static const WCHAR test_header_range_bytes[] = {'R','a','n','g','e',':',' ','b','y','t','e','s','=','0','-','7','7','3','\r','\n',0};
    static const WCHAR test_header_bytes[] = {'b','y','t','e','s','=','0','-','7','7','3',0};

    static const WCHAR test_flag_coalesce[] = {'t','e','s','t','2',',',' ','t','e','s','t','4',0};
    static const WCHAR test_flag_coalesce_reverse[] = {'t','e','s','t','3',',',' ','t','e','s','t','4',0};
    static const WCHAR test_flag_coalesce_comma[] =
        {'t','e','s','t','2',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',0};
    static const WCHAR test_flag_coalesce_comma_reverse[] =
        {'t','e','s','t','3',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',0};
    static const WCHAR test_flag_coalesce_semicolon[] =
        {'t','e','s','t','2',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',';',' ','t','e','s','t','6',0};
    static const WCHAR test_flag_coalesce_semicolon_reverse[] =
        {'t','e','s','t','3',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',';',' ','t','e','s','t','6',0};

    static const WCHAR field[] = {'f','i','e','l','d',0};
    static const WCHAR value[] = {'v','a','l','u','e',' ',0};
    static const WCHAR value_nospace[] = {'v','a','l','u','e',0};
    static const WCHAR empty[] = {0};

    static const WCHAR test_headers[][14] =
        {
            {'W','a','r','n','i','n','g',':','t','e','s','t','1',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','2',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','3',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','4',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','5',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','6',0},
            {'W','a','r','n','i','n','g',':','t','e','s','t','7',0},
            {0},
            {':',0},
            {'a',':',0},
            {':','b',0},
            {'c','d',0},
            {' ','e',' ',':','f',0},
            {'f','i','e','l','d',':',' ','v','a','l','u','e',' ',0},
            {'n','a','m','e',':',' ','v','a','l','u','e',0},
            {'n','a','m','e',':',0}
        };
    static const WCHAR test_indices[][6] =
        {
            {'t','e','s','t','1',0},
            {'t','e','s','t','2',0},
            {'t','e','s','t','3',0},
            {'t','e','s','t','4',0}
        };

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    ok(session != NULL, "WinHttpOpen failed to open session.\n");

    connection = WinHttpConnect (session, test_winehq, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, test_verb, test_file, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    ok(request != NULL, "WinHttpOpenRequest failed to open a request, error: %u.\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded, found 'Warning' header.\n");
    SetLastError(0xdeadbeef);
    ret = WinHttpAddRequestHeaders(request, test_headers[0], -1L, WINHTTP_ADDREQ_FLAG_ADD);
    err = GetLastError();
    ok(ret, "WinHttpAddRequestHeaders failed to add new header, got %d with error %u.\n", ret, err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed: header index not incremented\n");
    ok(memcmp(buffer, test_indices[0], sizeof(test_indices[0])) == 0, "WinHttpQueryHeaders failed: incorrect string returned\n");
    ok(len == 5*sizeof(WCHAR), "WinHttpQueryHeaders failed: invalid length returned, expected 5, got %d\n", len);

    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded, second index should not exist.\n");

    /* Try to fetch the header info with a buffer that's big enough to fit the
     * string but not the NULL terminator.
     */
    index = 0;
    len = 5*sizeof(WCHAR);
    memset(check_buffer, 0xab, sizeof(check_buffer));
    memcpy(buffer, check_buffer, sizeof(buffer));
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded with a buffer that's too small.\n");
    ok(memcmp(buffer, check_buffer, sizeof(buffer)) == 0,
            "WinHttpQueryHeaders failed, modified the buffer when it should not have.\n");
    ok(len == 6*sizeof(WCHAR), "WinHttpQueryHeaders returned invalid length, expected 12, got %d\n", len);

    /* Try with a NULL buffer */
    index = 0;
    len = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, NULL, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
    ok(len > 40, "WinHttpQueryHeaders returned invalid length: expected greater than 40, got %d\n", len);
    ok(index == 0, "WinHttpQueryHeaders incorrectly incremented header index.\n");

    /* Try with a NULL buffer and a length that's too small */
    index = 0;
    len = 10;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, NULL, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "WinHttpQueryHeaders set incorrect error: expected ERROR_INSUFFICENT_BUFFER, got %u\n", GetLastError());
    ok(len > 40, "WinHttpQueryHeaders returned invalid length: expected greater than 40, got %d\n", len);
    ok(index == 0, "WinHttpQueryHeaders incorrectly incremented header index.\n");

    index = 0;
    len = 0;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, NULL, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "WinHttpQueryHeaders set incorrect error: expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
    ok(len > 40, "WinHttpQueryHeaders returned invalid length: expected greater than 40, got %d\n", len);
    ok(index == 0, "WinHttpQueryHeaders failed: index was incremented.\n");

    /* valid query */
    oldlen = len;
    index = 0;
    len = sizeof(buffer);
    memset(buffer, 0xff, sizeof(buffer));
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: got %d\n", ret);
    ok(len + sizeof(WCHAR) <= oldlen, "WinHttpQueryHeaders resulting length longer than advertized.\n");
    ok((len < sizeof(buffer) - sizeof(WCHAR)) && buffer[len / sizeof(WCHAR)] == 0, "WinHttpQueryHeaders did not append NULL terminator\n");
    ok(len == lstrlenW(buffer) * sizeof(WCHAR), "WinHttpQueryHeaders returned incorrect length.\n");
    ok(memcmp(buffer, test_header_begin, sizeof(test_header_begin)) == 0 ||
        memcmp(buffer, full_path_test_header_begin, sizeof(full_path_test_header_begin)) == 0,
        "WinHttpQueryHeaders returned invalid beginning of header string.\n");
    ok(memcmp(buffer + lstrlenW(buffer) - 4, test_header_end, sizeof(test_header_end)) == 0,
        "WinHttpQueryHeaders returned invalid end of header string.\n");
    ok(index == 0, "WinHttpQueryHeaders incremented header index.\n");

    index = 0;
    len = 0;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, NULL, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders unexpectedly succeeded.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "WinHttpQueryHeaders set incorrect error: expected ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
    ok(len > 40, "WinHttpQueryHeaders returned invalid length: expected greater than 40, got %d\n", len);
    ok(index == 0, "WinHttpQueryHeaders failed: index was incremented.\n");

    oldlen = len;
    index = 0;
    len = sizeof(buffer);
    memset(buffer, 0xff, sizeof(buffer));
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed %u\n", GetLastError());
    ok(len + sizeof(WCHAR) <= oldlen, "resulting length longer than advertized\n");
    ok((len < sizeof(buffer) - sizeof(WCHAR)) && !buffer[len / sizeof(WCHAR)] && !buffer[len / sizeof(WCHAR) - 1],
        "no double NULL terminator\n");
    ok(memcmp(buffer, test_header_begin, sizeof(test_header_begin)) == 0 ||
        memcmp(buffer, full_path_test_header_begin, sizeof(full_path_test_header_begin)) == 0,
        "invalid beginning of header string.\n");
    ok(index == 0, "header index was incremented\n");

    /* tests for more indices */
    ret = WinHttpAddRequestHeaders(request, test_headers[1], -1L, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed to add duplicate header: %d\n", ret);

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[0], sizeof(test_indices[0])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[1], sizeof(test_indices[1])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[2], -1L, WINHTTP_ADDREQ_FLAG_REPLACE);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed to add duplicate header.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    reverse = (memcmp(buffer, test_indices[1], sizeof(test_indices[1])) != 0); /* Win7 returns values in reverse order of adding */
    ok(memcmp(buffer, test_indices[reverse ? 2 : 1], sizeof(test_indices[reverse ? 2 : 1])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 2], sizeof(test_indices[reverse ? 1 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    /* add if new flag */
    ret = WinHttpAddRequestHeaders(request, test_headers[3], -1L, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret == FALSE, "WinHttpAddRequestHeaders incorrectly replaced existing header.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 2 : 1], sizeof(test_indices[reverse ? 2 : 1])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 2], sizeof(test_indices[reverse ? 1 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");

    /* coalesce flag */
    ret = WinHttpAddRequestHeaders(request, test_headers[3], -1L, WINHTTP_ADDREQ_FLAG_COALESCE);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed with flag WINHTTP_ADDREQ_FLAG_COALESCE.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, reverse ? test_flag_coalesce_reverse : test_flag_coalesce,
                      reverse ? sizeof(test_flag_coalesce_reverse) : sizeof(test_flag_coalesce)) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 2], sizeof(test_indices[reverse ? 1 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");

    /* coalesce with comma flag */
    ret = WinHttpAddRequestHeaders(request, test_headers[4], -1L, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed with flag WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, reverse ? test_flag_coalesce_comma_reverse : test_flag_coalesce_comma,
                      reverse ? sizeof(test_flag_coalesce_comma_reverse) : sizeof(test_flag_coalesce_comma)) == 0,
        "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 2], sizeof(test_indices[reverse ? 1 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");


    /* coalesce with semicolon flag */
    ret = WinHttpAddRequestHeaders(request, test_headers[5], -1L, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed with flag WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, reverse ? test_flag_coalesce_semicolon_reverse : test_flag_coalesce_semicolon,
                      reverse ? sizeof(test_flag_coalesce_semicolon_reverse) : sizeof(test_flag_coalesce_semicolon)) == 0,
            "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 2], sizeof(test_indices[reverse ? 1 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");

    /* add and replace flags */
    ret = WinHttpAddRequestHeaders(request, test_headers[3], -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    ok(ret == TRUE, "WinHttpAddRequestHeaders failed with flag WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 3 : 2], sizeof(test_indices[reverse ? 3 : 2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[reverse ? 1 : 3], sizeof(test_indices[reverse ? 1 : 3])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[8], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(!ret, "WinHttpAddRequestHeaders failed\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[9], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret, "WinHttpAddRequestHeaders failed\n");

    index = 0;
    memset(buffer, 0xff, sizeof(buffer));
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_name3, buffer, &len, &index);
    ok(ret, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(!memcmp(buffer, empty, sizeof(empty)), "unexpected result\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[10], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(!ret, "WinHttpAddRequestHeaders failed\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[11], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(!ret, "WinHttpAddRequestHeaders failed\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[12], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(!ret, "WinHttpAddRequestHeaders failed\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[13], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret, "WinHttpAddRequestHeaders failed\n");

    index = 0;
    buffer[0] = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        field, buffer, &len, &index);
    ok(ret, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(!memcmp(buffer, value, sizeof(value)) || ! memcmp(buffer, value_nospace, sizeof(value_nospace)), "unexpected result\n");

    SetLastError(0xdeadbeef);
    ret = WinHttpAddRequestHeaders(request, test_header_range_bytes, 0,
                                   WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    err = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(err == ERROR_INVALID_PARAMETER, "got %u\n", err);

    ret = WinHttpAddRequestHeaders(request, test_header_range_bytes, ~0u,
                                   WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    ok(ret, "failed to add header: %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_range, buffer, &len, &index);
    ok(ret, "failed to get range header %u\n", GetLastError());
    ok(!memcmp(buffer, test_header_bytes, sizeof(test_header_bytes)), "incorrect string returned\n");
    ok(len == lstrlenW(test_header_bytes) * sizeof(WCHAR), "wrong length %u\n", len);
    ok(index == 1, "wrong index %u\n", index);

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_name2, buffer, &len, &index);
    ok(!ret, "unexpected success\n");

    SetLastError(0xdeadbeef);
    ret = WinHttpAddRequestHeaders(request, test_headers[14], ~0u, WINHTTP_ADDREQ_FLAG_REPLACE);
    err = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(err == ERROR_WINHTTP_HEADER_NOT_FOUND, "got %u\n", err);

    ret = WinHttpAddRequestHeaders(request, test_headers[14], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret, "got %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_name2, buffer, &len, &index);
    ok(ret, "got %u\n", GetLastError());
    ok(index == 1, "wrong index %u\n", index);
    ok(!memcmp(buffer, value_nospace, sizeof(value_nospace)), "incorrect string\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[15], ~0u, WINHTTP_ADDREQ_FLAG_REPLACE);
    ok(ret, "got %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_name2, buffer, &len, &index);
    err = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(err == ERROR_WINHTTP_HEADER_NOT_FOUND, "got %u\n", err);

    ret = WinHttpAddRequestHeaders(request, test_headers[14], -1L, 0);
    ok(ret, "got %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              test_header_name2, buffer, &len, &index);
    ok(ret, "got %u\n", GetLastError());
    ok(index == 1, "wrong index %u\n", index);
    ok(!memcmp(buffer, value_nospace, sizeof(value_nospace)), "incorrect string\n");

    ret = WinHttpCloseHandle(request);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);
 done:
    ret = WinHttpCloseHandle(connection);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);

}

static void CALLBACK cert_error(HINTERNET handle, DWORD_PTR ctx, DWORD status, LPVOID buf, DWORD len)
{
    DWORD flags = *(DWORD *)buf;

    if (!flags)
    {
        trace("WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR\n");
        return;
    }
#define X(x) if (flags & x) trace("%s\n", #x);
    X(WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED)
    X(WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
    X(WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
    X(WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
    X(WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
    X(WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
    X(WINHTTP_CALLBACK_STATUS_FLAG_CERT_WRONG_USAGE)
#undef X
}

static void test_secure_connection(void)
{
    static const char data_start[] = "<!DOCTYPE html PUBLIC";
    HINTERNET ses, con, req;
    DWORD size, status, policy, bitness, read_size, err, available_size, protocols, flags;
    BOOL ret;
    CERT_CONTEXT *cert;
    WINHTTP_CERTIFICATE_INFO info;
    char buffer[32];

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    ok(ret, "failed to set redirect policy %u\n", GetLastError());

    protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    err = GetLastError();
    ok(ret || err == ERROR_INVALID_PARAMETER /* < win7 */, "failed to set protocols %u\n", err);

    con = WinHttpConnect(ses, test_winehq, 443, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    /* try without setting WINHTTP_FLAG_SECURE */
    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
    err = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(err == ERROR_WINHTTP_INCORRECT_HANDLE_STATE || broken(err == ERROR_INVALID_PARAMETER) /* winxp */,
       "setting client cert context returned %u\n", err);

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("Connection failed, skipping.\n");
        goto cleanup;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "header query failed %u\n", GetLastError());
    ok(status == HTTP_STATUS_BAD_REQUEST, "got %u\n", status);

    WinHttpCloseHandle(req);

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    flags = 0xdeadbeef;
    size = sizeof(flags);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, &size);
    ok(ret, "failed to query security flags %u\n", GetLastError());
    ok(!flags, "got %08x\n", flags);

    flags = SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok(ret, "failed to set security flags %u\n", GetLastError());

    flags = SECURITY_FLAG_SECURE;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok(!ret, "success\n");

    flags = SECURITY_FLAG_STRENGTH_STRONG;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok(!ret, "success\n");

    flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok(ret, "failed to set security flags %u\n", GetLastError());

    flags = 0;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok(ret, "failed to set security flags %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
    err = GetLastError();
    ok(ret || broken(!ret && err == ERROR_INVALID_PARAMETER) /* winxp */, "failed to set client cert context %u\n", err);

    WinHttpSetStatusCallback(req, cert_error, WINHTTP_CALLBACK_STATUS_SECURE_FAILURE, 0);

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_SECURE_FAILURE || err == ERROR_WINHTTP_CANNOT_CONNECT ||
                 err == ERROR_WINHTTP_TIMEOUT || err == SEC_E_ILLEGAL_MESSAGE))
    {
        skip("secure connection failed, skipping remaining secure tests\n");
        goto cleanup;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    size = sizeof(cert);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &cert, &size );
    ok(ret, "failed to retrieve certificate context %u\n", GetLastError());
    if (ret) CertFreeCertificateContext(cert);

    size = sizeof(bitness);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SECURITY_KEY_BITNESS, &bitness, &size );
    ok(ret, "failed to retrieve key bitness %u\n", GetLastError());

    size = sizeof(info);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT, &info, &size );
    ok(ret, "failed to retrieve certificate info %u\n", GetLastError());

    if (ret)
    {
        trace("lpszSubjectInfo %s\n", wine_dbgstr_w(info.lpszSubjectInfo));
        trace("lpszIssuerInfo %s\n", wine_dbgstr_w(info.lpszIssuerInfo));
        trace("lpszProtocolName %s\n", wine_dbgstr_w(info.lpszProtocolName));
        trace("lpszSignatureAlgName %s\n", wine_dbgstr_w(info.lpszSignatureAlgName));
        trace("lpszEncryptionAlgName %s\n", wine_dbgstr_w(info.lpszEncryptionAlgName));
        trace("dwKeySize %u\n", info.dwKeySize);
        LocalFree( info.lpszSubjectInfo );
        LocalFree( info.lpszIssuerInfo );
    }

    ret = WinHttpReceiveResponse(req, NULL);
    if (!ret && GetLastError() == ERROR_WINHTTP_CONNECTION_ERROR)
    {
        skip("connection error, skipping remaining secure tests\n");
        goto cleanup;
    }
    ok(ret, "failed to receive response %u\n", GetLastError());

    available_size = 0;
    ret = WinHttpQueryDataAvailable(req, &available_size);
    ok(ret, "failed to query available data %u\n", GetLastError());
    ok(available_size > 2014, "available_size = %u\n", available_size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    size = 0;
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &size, NULL);
    ok(!ret, "succeeded unexpectedly\n");

    read_size = 0;
    for (;;)
    {
        size = 0;
        ret = WinHttpReadData(req, buffer, sizeof(buffer), &size);
        ok(ret == TRUE, "WinHttpReadData failed: %u.\n", GetLastError());
        if (!size) break;
        read_size += size;

        if (read_size <= 32)
            ok(!memcmp(buffer, data_start, sizeof(data_start)-1), "not expected: %.32s\n", buffer);
    }
    ok(read_size >= available_size, "read_size = %u, available_size = %u\n", read_size, available_size);

    size = sizeof(cert);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &cert, &size);
    ok(ret, "failed to retrieve certificate context %u\n", GetLastError());
    if (ret) CertFreeCertificateContext(cert);

cleanup:
    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_request_parameter_defaults(void)
{
    static const WCHAR empty[] = {0};
    HINTERNET ses, con, req;
    DWORD size, status, error;
    WCHAR *version;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, test_winehq, 0, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    error = GetLastError();
    if (!ret && (error == ERROR_WINHTTP_CANNOT_CONNECT || error == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    WinHttpCloseHandle(req);

    req = WinHttpOpenRequest(con, empty, empty, empty, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    error = GetLastError();
    if (!ret && (error == ERROR_WINHTTP_CANNOT_CONNECT || error == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_VERSION, NULL, NULL, &size, NULL);
    error = GetLastError();
    ok(!ret, "succeeded unexpectedly\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);

    version = HeapAlloc(GetProcessHeap(), 0, size);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_VERSION, NULL, version, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(lstrlenW(version) == size / sizeof(WCHAR), "unexpected size %u\n", size);
    HeapFree(GetProcessHeap(), 0, version);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

done:
    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static const WCHAR Connections[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
    'C','o','n','n','e','c','t','i','o','n','s',0 };
static const WCHAR WinHttpSettings[] = {
    'W','i','n','H','t','t','p','S','e','t','t','i','n','g','s',0 };

static DWORD get_default_proxy_reg_value( BYTE *buf, DWORD len, DWORD *type )
{
    LONG l;
    HKEY key;
    DWORD ret = 0;

    l = RegOpenKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, KEY_READ, &key );
    if (!l)
    {
        DWORD size = 0;

        l = RegQueryValueExW( key, WinHttpSettings, NULL, type, NULL, &size );
        if (!l)
        {
            if (size <= len)
                l = RegQueryValueExW( key, WinHttpSettings, NULL, type, buf,
                    &size );
            if (!l)
                ret = size;
        }
        RegCloseKey( key );
    }
    return ret;
}

static void set_proxy( REGSAM access, BYTE *buf, DWORD len, DWORD type )
{
    HKEY hkey;
    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, Connections, 0, NULL, 0, access, NULL, &hkey, NULL ))
    {
        if (len) RegSetValueExW( hkey, WinHttpSettings, 0, type, buf, len );
        else RegDeleteValueW( hkey, WinHttpSettings );
        RegCloseKey( hkey );
    }
}

static void set_default_proxy_reg_value( BYTE *buf, DWORD len, DWORD type )
{
    BOOL wow64;
    IsWow64Process( GetCurrentProcess(), &wow64 );
    if (sizeof(void *) > sizeof(int) || wow64)
    {
        set_proxy( KEY_WRITE|KEY_WOW64_64KEY, buf, len, type );
        set_proxy( KEY_WRITE|KEY_WOW64_32KEY, buf, len, type );
    }
    else
        set_proxy( KEY_WRITE, buf, len, type );
}

static void test_set_default_proxy_config(void)
{
    static WCHAR wideString[] = { 0x226f, 0x575b, 0 };
    static WCHAR normalString[] = { 'f','o','o',0 };
    DWORD type, len;
    BYTE *saved_proxy_settings = NULL;
    WINHTTP_PROXY_INFO info;
    BOOL ret;

    /* FIXME: it would be simpler to read the current settings using
     * WinHttpGetDefaultProxyConfiguration and save them using
     * WinHttpSetDefaultProxyConfiguration, but they appear to have a bug.
     *
     * If a proxy is configured in the registry, e.g. via 'proxcfg -p "foo"',
     * the access type reported by WinHttpGetDefaultProxyConfiguration is 1,
     * WINHTTP_ACCESS_TYPE_NO_PROXY, whereas it should be
     * WINHTTP_ACCESS_TYPE_NAMED_PROXY.
     * If WinHttpSetDefaultProxyConfiguration is called with dwAccessType = 1,
     * the lpszProxy and lpszProxyBypass values are ignored.
     * Thus, if a proxy is set with proxycfg, then calling
     * WinHttpGetDefaultProxyConfiguration followed by
     * WinHttpSetDefaultProxyConfiguration results in the proxy settings
     * getting deleted from the registry.
     *
     * Instead I read the current registry value and restore it directly.
     */
    len = get_default_proxy_reg_value( NULL, 0, &type );
    if (len)
    {
        saved_proxy_settings = HeapAlloc( GetProcessHeap(), 0, len );
        len = get_default_proxy_reg_value( saved_proxy_settings, len, &type );
    }

    if (0)
    {
        /* Crashes on Vista and higher */
        SetLastError(0xdeadbeef);
        ret = WinHttpSetDefaultProxyConfiguration(NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
            "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* test with invalid access type */
    info.dwAccessType = 0xdeadbeef;
    info.lpszProxy = info.lpszProxyBypass = NULL;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetDefaultProxyConfiguration(&info);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* at a minimum, the proxy server must be set */
    info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    info.lpszProxy = info.lpszProxyBypass = NULL;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetDefaultProxyConfiguration(&info);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    info.lpszProxyBypass = normalString;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetDefaultProxyConfiguration(&info);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* the proxy server can't have wide characters */
    info.lpszProxy = wideString;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetDefaultProxyConfiguration(&info);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
        skip("couldn't set default proxy configuration: access denied\n");
    else
        ok((!ret && GetLastError() == ERROR_INVALID_PARAMETER) ||
           broken(ret), /* Earlier winhttp versions on W2K/XP */
           "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    info.lpszProxy = normalString;
    SetLastError(0xdeadbeef);
    ret = WinHttpSetDefaultProxyConfiguration(&info);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
        skip("couldn't set default proxy configuration: access denied\n");
    else
    {
        ok(ret, "WinHttpSetDefaultProxyConfiguration failed: %u\n", GetLastError());
        ok(GetLastError() == ERROR_SUCCESS ||  broken(GetLastError() == 0xdeadbeef) /* < win7 */,
           "got %u\n", GetLastError());
    }
    set_default_proxy_reg_value( saved_proxy_settings, len, type );
}

static void test_timeouts(void)
{
    BOOL ret;
    DWORD value, size;
    HINTERNET ses, req, con;

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, -2, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, 0, -2, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, 0, 0, -2, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, 0, 0, 0, -2);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, -1, -1, -1, -1);
    ok(ret, "%u\n", GetLastError());
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
       "expected ERROR_SUCCESS, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, 0, 0, 0, 0);
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(ses, 0x0123, 0x4567, 0x89ab, 0xcdef);
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x0123, "Expected 0x0123, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x4567, "Expected 0x4567, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x89ab, "Expected 0x89ab, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xcdef, "Expected 0xcdef, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    con = WinHttpConnect(ses, test_winehq, 0, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    /* Timeout values should match the last one set for session */
    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, -2, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, 0, -2, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, 0, 0, -2, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, 0, 0, 0, -2);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, -1, -1, -1, -1);
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(con, 0, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(con, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(con, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(con, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(con, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE,
       "expected ERROR_WINHTTP_INVALID_TYPE, got %u\n", GetLastError());

    /* Changing timeout values for session should affect the values for connection */
    SetLastError(0xdeadbeef);
    value = 0xdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdead;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    /* Timeout values should match the last one set for session */
    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, -2, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, 0, -2, 0, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, 0, 0, -2, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, 0, 0, 0, -2);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, -1, -1, -1, -1);
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, 0, 0, 0, 0);
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetTimeouts(req, 0xcdef, 0x89ab, 0x4567, 0x0123);
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xcdef, "Expected 0xcdef, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x89ab, "Expected 0x89ab, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x4567, "Expected 0x4567, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0x0123, "Expected 0x0123, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0, "Expected 0, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    /* Changing timeout values for session should not affect the values for a request,
     * neither should the other way around.
     */
    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeefdead;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xdead, "Expected 0xdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeef;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeef;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeef;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_SEND_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xbeef;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(DWORD);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 0xbeefdead, "Expected 0xbeefdead, got %u\n", value);

    /* response timeout */
    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == ~0u, "got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 30000;
    ret = WinHttpSetOption(req, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    todo_wine ok(value == 0xbeefdead, "got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(con, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == ~0u, "got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 30000;
    ret = WinHttpSetOption(con, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, sizeof(value));
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == ~0u, "got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 48878;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    todo_wine ok(value == 48879, "got %u\n", value);

    SetLastError(0xdeadbeef);
    value = 48880;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, sizeof(value));
    ok(ret, "%u\n", GetLastError());

    SetLastError(0xdeadbeef);
    value = 0xdeadbeef;
    size  = sizeof(value);
    ret = WinHttpQueryOption(ses, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &value, &size);
    ok(ret, "%u\n", GetLastError());
    ok(value == 48880, "got %u\n", value);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_resolve_timeout(void)
{
    static const WCHAR nxdomain[] =
        {'n','x','d','o','m','a','i','n','.','w','i','n','e','h','q','.','o','r','g',0};
    HINTERNET ses, con, req;
    DWORD timeout;
    BOOL ret;

    if (! proxy_active())
    {
        ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
        ok(ses != NULL, "failed to open session %u\n", GetLastError());

        timeout = 10000;
        ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &timeout, sizeof(timeout));
        ok(ret, "failed to set resolve timeout %u\n", GetLastError());

        con = WinHttpConnect(ses, nxdomain, 0, 0);
        ok(con != NULL, "failed to open a connection %u\n", GetLastError());

        req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
        ok(req != NULL, "failed to open a request %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
        if (ret)
        {
            skip("nxdomain returned success. Broken ISP redirects?\n");
            goto done;
        }
        ok(GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED,
           "expected ERROR_WINHTTP_NAME_NOT_RESOLVED got %u\n", GetLastError());

        ret = WinHttpReceiveResponse( req, NULL );
        ok( !ret && (GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_STATE ||
                     GetLastError() == ERROR_WINHTTP_OPERATION_CANCELLED /* < win7 */),
            "got %u\n", GetLastError() );

        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
    }
    else
       skip("Skipping host resolution tests, host resolution preformed by proxy\n");

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    timeout = 10000;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_RESOLVE_TIMEOUT, &timeout, sizeof(timeout));
    ok(ret, "failed to set resolve timeout %u\n", GetLastError());

    con = WinHttpConnect(ses, test_winehq, 0, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    if (!ret && GetLastError() == ERROR_WINHTTP_CANNOT_CONNECT)
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request\n");

 done:
    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static const char page1[] =
"<HTML>\r\n"
"<HEAD><TITLE>winhttp test page</TITLE></HEAD>\r\n"
"<BODY>The quick brown fox jumped over the lazy dog<P></BODY>\r\n"
"</HTML>\r\n\r\n";

static const char okmsg[] =
"HTTP/1.1 200 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char notokmsg[] =
"HTTP/1.1 400 Bad Request\r\n"
"\r\n";

static const char cookiemsg[] =
"HTTP/1.1 200 OK\r\n"
"Set-Cookie: name = value \r\n"
"Set-Cookie: NAME = value \r\n"
"\r\n";

static const char cookiemsg2[] =
"HTTP/1.1 200 OK\r\n"
"Set-Cookie: name2=value; Domain = localhost; Path=/cookie5;Expires=Wed, 13 Jan 2021 22:23:01 GMT; HttpOnly; \r\n"
"\r\n";

static const char nocontentmsg[] =
"HTTP/1.1 204 No Content\r\n"
"Server: winetest\r\n"
"\r\n";

static const char notmodified[] =
"HTTP/1.1 304 Not Modified\r\n"
"\r\n";

static const char noauthmsg[] =
"HTTP/1.1 401 Unauthorized\r\n"
"Server: winetest\r\n"
"Connection: close\r\n"
"WWW-Authenticate: Basic realm=\"placebo\"\r\n"
"Content-Length: 12\r\n"
"Content-Type: text/plain\r\n"
"\r\n";

static const char okauthmsg[] =
"HTTP/1.1 200 OK\r\n"
"Server: winetest\r\n"
"Connection: close\r\n"
"Content-Length: 11\r\n"
"Content-Type: text/plain\r\n"
"\r\n";

static const char headmsg[] =
"HTTP/1.1 200 OK\r\n"
"Content-Length: 100\r\n"
"\r\n";

static const char multiauth[] =
"HTTP/1.1 401 Unauthorized\r\n"
"Server: winetest\r\n"
"WWW-Authenticate: Bearer\r\n"
"WWW-Authenticate: Basic realm=\"placebo\"\r\n"
"WWW-Authenticate: NTLM\r\n"
"Content-Length: 10\r\n"
"Content-Type: text/plain\r\n"
"\r\n";

static const char largeauth[] =
"HTTP/1.1 401 Unauthorized\r\n"
"Server: winetest\r\n"
"WWW-Authenticate: Basic realm=\"placebo\"\r\n"
"WWW-Authenticate: NTLM\r\n"
"Content-Length: 10240\r\n"
"Content-Type: text/plain\r\n"
"\r\n";

static const char passportauth[] =
"HTTP/1.1 302 Found\r\n"
"Content-Length: 0\r\n"
"Location: /\r\n"
"WWW-Authenticate: Passport1.4\r\n"
"\r\n";

static const char unauthorized[] = "Unauthorized";
static const char hello_world[] = "Hello World";
static const char auth_unseen[] = "Auth Unseen";

struct server_info
{
    HANDLE event;
    int port;
};

#define BIG_BUFFER_LEN 0x2250

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
        if (strstr(buffer, "GET /basic"))
        {
            send(c, okmsg, sizeof okmsg - 1, 0);
            send(c, page1, sizeof page1 - 1, 0);
        }
        if (strstr(buffer, "/auth_with_creds"))
        {
            send(c, okauthmsg, sizeof okauthmsg - 1, 0);
            if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
                send(c, hello_world, sizeof hello_world - 1, 0);
            else
                send(c, auth_unseen, sizeof auth_unseen - 1, 0);
            continue;
        }
        if (strstr(buffer, "/auth"))
        {
            if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
            {
                send(c, okauthmsg, sizeof okauthmsg - 1, 0);
                send(c, hello_world, sizeof hello_world - 1, 0);
            }
            else
            {
                send(c, noauthmsg, sizeof noauthmsg - 1, 0);
                send(c, unauthorized, sizeof unauthorized - 1, 0);
            }
            continue;
        }
        if (strstr(buffer, "/big"))
        {
            char msg[BIG_BUFFER_LEN];
            memset(msg, 'm', sizeof(msg));
            send(c, okmsg, sizeof(okmsg) - 1, 0);
            send(c, msg, sizeof(msg), 0);
        }
        if (strstr(buffer, "/no_headers"))
        {
            send(c, page1, sizeof page1 - 1, 0);
        }
        if (strstr(buffer, "GET /no_content"))
        {
            send(c, nocontentmsg, sizeof nocontentmsg - 1, 0);
            continue;
        }
        if (strstr(buffer, "GET /not_modified"))
        {
            if (strstr(buffer, "If-Modified-Since:")) send(c, notmodified, sizeof notmodified - 1, 0);
            else send(c, notokmsg, sizeof(notokmsg) - 1, 0);
            continue;
        }
        if (strstr(buffer, "HEAD /head"))
        {
            send(c, headmsg, sizeof headmsg - 1, 0);
            continue;
        }
        if (strstr(buffer, "GET /multiauth"))
        {
            send(c, multiauth, sizeof multiauth - 1, 0);
        }
        if (strstr(buffer, "GET /largeauth"))
        {
            if (strstr(buffer, "Authorization: NTLM"))
                send(c, okmsg, sizeof(okmsg) - 1, 0);
            else
            {
                send(c, largeauth, sizeof largeauth - 1, 0);
#ifdef __REACTOS__
                memset(buffer, 'A', sizeof(buffer));
                for (i = 0; i < (10240 / sizeof(buffer)); i++) send(c, buffer, sizeof(buffer), 0);
#else
                for (i = 0; i < 10240; i++) send(c, "A", 1, 0);
#endif
                continue;
            }
        }
        if (strstr(buffer, "GET /cookie5"))
        {
            if (strstr(buffer, "Cookie: name2=value\r\n"))
                send(c, okmsg, sizeof(okmsg) - 1, 0);
            else
                send(c, notokmsg, sizeof(notokmsg) - 1, 0);
        }
        if (strstr(buffer, "GET /cookie4"))
        {
            send(c, cookiemsg2, sizeof(cookiemsg2) - 1, 0);
        }
        if (strstr(buffer, "GET /cookie3"))
        {
            if (strstr(buffer, "Cookie: name=value2; NAME=value; name=value\r\n") ||
                broken(strstr(buffer, "Cookie: name=value2; name=value; NAME=value\r\n") != NULL))
                send(c, okmsg, sizeof(okmsg) - 1, 0);
            else
                send(c, notokmsg, sizeof(notokmsg) - 1, 0);
        }
        if (strstr(buffer, "GET /cookie2"))
        {
            if (strstr(buffer, "Cookie: NAME=value; name=value\r\n") ||
                broken(strstr(buffer, "Cookie: name=value; NAME=value\r\n") != NULL))
                send(c, okmsg, sizeof(okmsg) - 1, 0);
            else
                send(c, notokmsg, sizeof(notokmsg) - 1, 0);
        }
        else if (strstr(buffer, "GET /cookie"))
        {
            if (!strstr(buffer, "Cookie: name=value\r\n")) send(c, cookiemsg, sizeof(cookiemsg) - 1, 0);
            else send(c, notokmsg, sizeof(notokmsg) - 1, 0);
        }
        else if (strstr(buffer, "GET /escape"))
        {
            static const char res[] = "%0D%0A%1F%7F%3C%20%one?%1F%7F%20!%22%23$%&'()*+,-./:;%3C=%3E?@%5B%5C%5D"
                                      "%5E_%60%7B%7C%7D~%0D%0A ";
            static const char res2[] = "%0D%0A%1F%7F%3C%20%25two?%1F%7F%20!%22%23$%25&'()*+,-./:;%3C=%3E?@%5B%5C%5D"
                                       "%5E_%60%7B%7C%7D~%0D%0A ";
            static const char res3[] = "\x1f\x7f<%20%three?\x1f\x7f%20!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ";
            static const char res4[] = "%0D%0A%1F%7F%3C%20%four?\x1f\x7f%20!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ";
            static const char res5[] = "&text=one%C2%80%7F~";
            static const char res6[] = "&text=two%C2%80\x7f~";
            static const char res7[] = "&text=%E5%90%9B%E3%81%AE%E5%90%8D%E3%81%AF";

            if (strstr(buffer + 11, res) || strstr(buffer + 11, res2) || strstr(buffer + 11, res3) ||
                strstr(buffer + 11, res4) || strstr(buffer + 11, res5) || strstr(buffer + 11, res6) ||
                strstr(buffer + 11, res7))
            {
                send(c, okmsg, sizeof(okmsg) - 1, 0);
            }
            else send(c, notokmsg, sizeof(notokmsg) - 1, 0);
        }
        else if (strstr(buffer, "GET /passport"))
        {
            send(c, passportauth, sizeof(passportauth) - 1, 0);
        }
        if (strstr(buffer, "GET /quit"))
        {
            send(c, okmsg, sizeof okmsg - 1, 0);
            send(c, page1, sizeof page1 - 1, 0);
            last_request = 1;
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
    static const WCHAR test_header_end_clrf[] = {'\r','\n','\r','\n',0};
    static const WCHAR test_header_end_raw[] = {0,0};
    HINTERNET ses, con, req;
    char buffer[0x100];
    WCHAR buffer2[0x100];
    DWORD count, status, size, error, supported, first, target;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(ses, 0, buffer, sizeof(buffer));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INVALID_OPTION, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(ses, 0, buffer, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(con, 0, buffer, sizeof(buffer));
    todo_wine ok(!ret && GetLastError() == ERROR_WINHTTP_INVALID_OPTION, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(con, 0, buffer, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %u\n", GetLastError());

    req = WinHttpOpenRequest(con, verb, path, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetOption(req, 0, buffer, sizeof(buffer));
    ok(!ret && GetLastError() == ERROR_WINHTTP_INVALID_OPTION, "got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryOption(req, 0, buffer, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    supported = first = target = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, &supported, &first, &target);
    error = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(error == ERROR_INVALID_OPERATION, "expected ERROR_INVALID_OPERATION, got %u\n", error);
    ok(supported == 0xdeadbeef, "got %x\n", supported);
    ok(first == 0xdeadbeef, "got %x\n", first);
    ok(target == 0xdeadbeef, "got %x\n", target);

    size = sizeof(buffer2);
    memset(buffer2, 0, sizeof(buffer2));
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, buffer2, &size, NULL);
    ok(ret, "failed to query for raw headers: %u\n", GetLastError());
    ok(!memcmp(buffer2 + lstrlenW(buffer2) - 4, test_header_end_clrf, sizeof(test_header_end_clrf)),
       "WinHttpQueryHeaders returned invalid end of header string\n");

    size = sizeof(buffer2);
    memset(buffer2, 0, sizeof(buffer2));
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS, NULL, buffer2, &size, NULL);
    ok(ret, "failed to query for raw headers: %u\n", GetLastError());
    ok(!memcmp(buffer2 + (size / sizeof(WCHAR)) - 1, test_header_end_raw, sizeof(test_header_end_raw)),
       "WinHttpQueryHeaders returned invalid end of header string\n");
    ok(buffer2[(size / sizeof(WCHAR)) - 2] != 0, "returned string has too many NULL characters\n");

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

static void test_basic_authentication(int port)
{
    static const WCHAR authW[] = {'/','a','u','t','h',0};
    static const WCHAR auth_with_credsW[] = {'/','a','u','t','h','_','w','i','t','h','_','c','r','e','d','s',0};
    static WCHAR userW[] = {'u','s','e','r',0};
    static WCHAR passW[] = {'p','w','d',0};
    static WCHAR pass2W[] = {'p','w','d','2',0};
    HINTERNET ses, con, req;
    DWORD status, size, error, supported, first, target;
    char buffer[32];
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, authW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(NULL, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER || error == ERROR_INVALID_OPERATION, "got %u\n", error);

    supported = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, &supported, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER || error == ERROR_INVALID_OPERATION, "got %u\n", error);
    ok(supported == 0xdeadbeef, "got %x\n", supported);

    supported = first = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, &supported, &first, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER || error == ERROR_INVALID_OPERATION, "got %u\n", error);
    ok(supported == 0xdeadbeef, "got %x\n", supported);
    ok(first == 0xdeadbeef, "got %x\n", first);

    supported = first = target = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, &supported, &first, &target);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_OPERATION, "expected ERROR_INVALID_OPERATION, got %u\n", error);
    ok(supported == 0xdeadbeef, "got %x\n", supported);
    ok(first == 0xdeadbeef, "got %x\n", first);
    ok(target == 0xdeadbeef, "got %x\n", target);

    supported = first = target = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(NULL, &supported, &first, &target);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %u\n", error);
    ok(supported == 0xdeadbeef, "got %x\n", supported);
    ok(first == 0xdeadbeef, "got %x\n", first);
    ok(target == 0xdeadbeef, "got %x\n", target);

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_DENIED, "request failed unexpectedly %u\n", status);

    size = 0;
    ret = WinHttpReadData(req, buffer, sizeof(buffer), &size);
    error = GetLastError();
    ok(ret || broken(error == ERROR_WINHTTP_SHUTDOWN || error == ERROR_WINHTTP_TIMEOUT) /* XP */, "failed to read data %u\n", GetLastError());
    if (ret)
    {
        ok(size == 12, "expected 12, got %u\n", size);
        ok(!memcmp(buffer, unauthorized, 12), "got %s\n", buffer);
    }

    supported = first = target = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryAuthSchemes(req, &supported, &first, &target);
    error = GetLastError();
    ok(ret, "failed to query authentication schemes %u\n", error);
    ok(error == ERROR_SUCCESS || broken(error == 0xdeadbeef) /* < win7 */, "expected ERROR_SUCCESS, got %u\n", error);
    ok(supported == WINHTTP_AUTH_SCHEME_BASIC, "got %x\n", supported);
    ok(first == WINHTTP_AUTH_SCHEME_BASIC, "got %x\n", first);
    ok(target == WINHTTP_AUTH_TARGET_SERVER, "got %x\n", target);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_NTLM, NULL, NULL, NULL);
    error = GetLastError();
    ok(ret, "failed to set credentials %u\n", error);
    ok(error == ERROR_SUCCESS || broken(error == 0xdeadbeef) /* < win7 */, "expected ERROR_SUCCESS, got %u\n", error);

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_PASSPORT, NULL, NULL, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_NEGOTIATE, NULL, NULL, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_DIGEST, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, NULL, passW, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, passW, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    size = 0;
    ret = WinHttpReadData(req, buffer, sizeof(buffer), &size);
    error = GetLastError();
    ok(ret || broken(error == ERROR_WINHTTP_SHUTDOWN || error == ERROR_WINHTTP_TIMEOUT) /* XP */, "failed to read data %u\n", GetLastError());
    if (ret)
    {
        ok(size == 11, "expected 11, got %u\n", size);
        ok(!memcmp(buffer, hello_world, 11), "got %s\n", buffer);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);

    /* now set the credentials first to show that they get sent with the first request */
    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, auth_with_credsW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, passW, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    size = 0;
    ret = WinHttpReadData(req, buffer, sizeof(buffer), &size);
    error = GetLastError();
    ok(ret || broken(error == ERROR_WINHTTP_SHUTDOWN || error == ERROR_WINHTTP_TIMEOUT) /* XP */, "failed to read data %u\n", GetLastError());
    if (ret)
    {
        ok(size == 11, "expected 11, got %u\n", size);
        ok(!memcmp(buffer, hello_world, 11), "got %s\n", buffer);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);

    /* credentials set with WinHttpSetCredentials take precedence over those set through options */

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, authW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, passW, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_USERNAME, userW, lstrlenW(userW));
    ok(ret, "failed to set username %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_PASSWORD, pass2W, lstrlenW(pass2W));
    ok(ret, "failed to set password %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, authW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_USERNAME, userW, lstrlenW(userW));
    ok(ret, "failed to set username %u\n", GetLastError());

    ret = WinHttpSetOption(req, WINHTTP_OPTION_PASSWORD, passW, lstrlenW(passW));
    ok(ret, "failed to set password %u\n", GetLastError());

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, pass2W, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed to query status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_DENIED, "request failed unexpectedly %u\n", status);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_multi_authentication(int port)
{
    static const WCHAR multiauthW[] = {'/','m','u','l','t','i','a','u','t','h',0};
    static const WCHAR www_authenticateW[] =
        {'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',0};
    static const WCHAR getW[] = {'G','E','T',0};
    HINTERNET ses, con, req;
    DWORD supported, first, target, size, index;
    WCHAR buf[512];
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, getW, multiauthW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                             WINHTTP_NO_REQUEST_DATA,0, 0, 0 );
    ok(ret, "expected success\n");

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "expected success\n");

    supported = first = target = 0xdeadbeef;
    ret = WinHttpQueryAuthSchemes(req, &supported, &first, &target);
    ok(ret, "expected success\n");
    ok(supported == (WINHTTP_AUTH_SCHEME_BASIC | WINHTTP_AUTH_SCHEME_NTLM), "got %x\n", supported);
    ok(target == WINHTTP_AUTH_TARGET_SERVER, "got %x\n", target);
    ok(first == WINHTTP_AUTH_SCHEME_BASIC, "got %x\n", first);

    index = 0;
    size = sizeof(buf);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_CUSTOM, www_authenticateW, buf, &size, &index);
    ok(ret, "expected success\n");
    ok(!strcmp_wa(buf, "Bearer"), "buf = %s\n", wine_dbgstr_w(buf));
    ok(size == lstrlenW(buf) * sizeof(WCHAR), "size = %u\n", size);
    ok(index == 1, "index = %u\n", index);

    index = 0;
    size = 0xdeadbeef;
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_CUSTOM, www_authenticateW, NULL, &size, &index);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "WinHttpQueryHeaders returned %x(%u)\n", ret, GetLastError());
    ok(size == (lstrlenW(buf) + 1) * sizeof(WCHAR), "size = %u\n", size);
    ok(index == 0, "index = %u\n", index);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_large_data_authentication(int port)
{
    static const WCHAR largeauthW[] = {'/','l','a','r','g','e','a','u','t','h',0};
    static const WCHAR getW[] = {'G','E','T',0};
    static WCHAR userW[] = {'u','s','e','r',0};
    static WCHAR passW[] = {'p','w','d',0};
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, getW, largeauthW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    ok(ret, "expected success\n");

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "expected success\n");

    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL,
                              &status, &size, NULL);
    ok(ret, "expected success\n");
    ok(status == HTTP_STATUS_DENIED, "got %d\n", status);

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_NTLM, userW, passW, NULL);
    ok(ret, "expected success\n");

    ret = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    ok(ret, "expected success %d\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "expected success\n");

    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL,
                              &status, &size, NULL);
    ok(ret, "expected success\n");
    ok(status == HTTP_STATUS_OK, "got %d\n", status);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_no_headers(int port)
{
    static const WCHAR no_headersW[] = {'/','n','o','_','h','e','a','d','e','r','s',0};
    HINTERNET ses, con, req;
    DWORD error;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, no_headersW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    if (!ret)
    {
        error = GetLastError();
        ok(error == ERROR_WINHTTP_INVALID_SERVER_RESPONSE, "got %u\n", error);
    }
    else
    {
        SetLastError(0xdeadbeef);
        ret = WinHttpReceiveResponse(req, NULL);
        error = GetLastError();
        ok(!ret, "expected failure\n");
        ok(error == ERROR_WINHTTP_INVALID_SERVER_RESPONSE, "got %u\n", error);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_no_content(int port)
{
    static const WCHAR no_contentW[] = {'/','n','o','_','c','o','n','t','e','n','t',0};
    HINTERNET ses, con, req;
    char buf[128];
    DWORD size, len = sizeof(buf), bytes_read, status;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, no_contentW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    size = 12345;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryDataAvailable(req, &size);
    todo_wine {
    ok(!ret, "expected error\n");
    ok(GetLastError() == ERROR_WINHTTP_INCORRECT_HANDLE_STATE,
       "expected ERROR_WINHTTP_INCORRECT_HANDLE_STATE, got 0x%08x\n", GetLastError());
    ok(size == 12345 || broken(size == 0) /* Win <= 2003 */,
       "expected 12345, got %u\n", size);
    }

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "expected success\n");

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "expected success\n");

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &status, &size, NULL);
    ok(ret, "expected success\n");
    ok(status == HTTP_STATUS_NO_CONTENT, "expected status 204, got %d\n", status);

    SetLastError(0xdeadbeef);
    size = sizeof(status);
    status = 12345;
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &status, &size, 0);
    ok(!ret, "expected no content-length header\n");
    ok(GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND, "wrong error %u\n", GetLastError());
    ok(status == 12345, "expected 0, got %d\n", status);

    SetLastError(0xdeadbeef);
    size = 12345;
    ret = WinHttpQueryDataAvailable(req, &size);
    ok(ret, "expected success\n");
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
       "wrong error %u\n", GetLastError());
    ok(!size, "expected 0, got %u\n", size);

    SetLastError(0xdeadbeef);
    ret = WinHttpReadData(req, buf, len, &bytes_read);
    ok(ret, "expected success\n");
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == 0xdeadbeef) /* < win7 */,
       "wrong error %u\n", GetLastError());
    ok(!bytes_read, "expected 0, got %u\n", bytes_read);

    size = 12345;
    ret = WinHttpQueryDataAvailable(req, &size);
    ok(ret, "expected success\n");
    ok(size == 0, "expected 0, got %d\n", size);

    WinHttpCloseHandle(req);

    size = 12345;
    SetLastError(0xdeadbeef);
    ret = WinHttpQueryDataAvailable(req, &size);
    ok(!ret, "expected error\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "expected ERROR_INVALID_HANDLE, got 0x%08x\n", GetLastError());
    ok(size == 12345, "expected 12345, got %u\n", size);

    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_head_request(int port)
{
    static const WCHAR verbW[] = {'H','E','A','D',0};
    static const WCHAR headW[] = {'/','h','e','a','d',0};
    HINTERNET ses, con, req;
    char buf[128];
    DWORD size, len, count, status;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, verbW, headW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &status, &size, NULL);
    ok(ret, "failed to get status code %u\n", GetLastError());
    ok(status == HTTP_STATUS_OK, "got %u\n", status);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &len, &size, 0);
    ok(ret, "failed to get content-length header %u\n", GetLastError());
    ok(len == HTTP_STATUS_CONTINUE, "got %u\n", len);

    count = 0xdeadbeef;
    ret = WinHttpQueryDataAvailable(req, &count);
    ok(ret, "failed to query data available %u\n", GetLastError());
    ok(!count, "got %u\n", count);

    len = sizeof(buf);
    count = 0xdeadbeef;
    ret = WinHttpReadData(req, buf, len, &count);
    ok(ret, "failed to read data %u\n", GetLastError());
    ok(!count, "got %u\n", count);

    count = 0xdeadbeef;
    ret = WinHttpQueryDataAvailable(req, &count);
    ok(ret, "failed to query data available %u\n", GetLastError());
    ok(!count, "got %u\n", count);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_not_modified(int port)
{
    static const WCHAR pathW[] = {'/','n','o','t','_','m','o','d','i','f','i','e','d',0};
    static const WCHAR ifmodifiedW[] = {'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',':',' '};
    static const WCHAR ifmodified2W[] = {'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0};
    BOOL ret;
    HINTERNET session, request, connection;
    DWORD index, len, status, size, start = GetTickCount();
    SYSTEMTIME st;
    WCHAR today[(sizeof(ifmodifiedW) + WINHTTP_TIME_FORMAT_BUFSIZE)/sizeof(WCHAR) + 3], buffer[32];

    memcpy(today, ifmodifiedW, sizeof(ifmodifiedW));
    GetSystemTime(&st);
    WinHttpTimeFromSystemTime(&st, &today[ARRAY_SIZE(ifmodifiedW)]);

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    ok(session != NULL, "WinHttpOpen failed: %u\n", GetLastError());

    connection = WinHttpConnect(session, localhostW, port, 0);
    ok(connection != NULL, "WinHttpConnect failed: %u\n", GetLastError());

    request = WinHttpOpenRequest(connection, NULL, pathW, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    ok(request != NULL, "WinHttpOpenrequest failed: %u\n", GetLastError());

    ret = WinHttpSendRequest(request, today, 0, NULL, 0, 0, 0);
    ok(ret, "WinHttpSendRequest failed: %u\n", GetLastError());

    ret = WinHttpReceiveResponse(request, NULL);
    ok(ret, "WinHttpReceiveResponse failed: %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                              ifmodified2W, buffer, &len, &index);
    ok(ret, "failed to get header %u\n", GetLastError());

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &status, &size, NULL);
    ok(ret, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(status == HTTP_STATUS_NOT_MODIFIED, "got %u\n", status);

    size = 0xdeadbeef;
    ret = WinHttpQueryDataAvailable(request, &size);
    ok(ret, "WinHttpQueryDataAvailable failed: %u\n", GetLastError());
    ok(!size, "got %u\n", size);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    start = GetTickCount() - start;
    ok(start <= 2000, "Expected less than 2 seconds for the test, got %u ms\n", start);
}

static void test_bad_header( int port )
{
    static const WCHAR bad_headerW[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ',
         't','e','x','t','/','h','t','m','l','\n','\r',0};
    static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};
    static const WCHAR content_typeW[] = {'C','o','n','t','e','n','t','-','T','y','p','e',0};
    WCHAR buffer[32];
    HINTERNET ses, con, req;
    DWORD index, len;
    BOOL ret;

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpAddRequestHeaders( req, bad_headerW, ~0u, WINHTTP_ADDREQ_FLAG_ADD );
    ok( ret, "failed to add header %u\n", GetLastError() );

    index = 0;
    buffer[0] = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_CUSTOM|WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                               content_typeW, buffer, &len, &index );
    ok( ret, "failed to query headers %u\n", GetLastError() );
    ok( !lstrcmpW( buffer, text_htmlW ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( index == 1, "index = %u\n", index );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static void test_multiple_reads(int port)
{
    static const WCHAR bigW[] = {'b','i','g',0};
    HINTERNET ses, con, req;
    DWORD total_len = 0;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, port, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, bigW, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    trace("waiting for response\n");
    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret == TRUE, "expected success\n");

    trace("finished waiting for response\n");

    for (;;)
    {
        DWORD len = 0xdeadbeef;
        ret = WinHttpQueryDataAvailable( req, &len );
        ok( ret, "WinHttpQueryDataAvailable failed with error %u\n", GetLastError() );
        if (ret) ok( len != 0xdeadbeef, "WinHttpQueryDataAvailable return wrong length\n" );
        if (len)
        {
            DWORD bytes_read;
            char *buf = HeapAlloc( GetProcessHeap(), 0, len + 1 );

            ret = WinHttpReadData( req, buf, len, &bytes_read );
            ok(ret, "WinHttpReadData failed: %u.\n", GetLastError());
            ok( len == bytes_read, "only got %u of %u available\n", bytes_read, len );

            HeapFree( GetProcessHeap(), 0, buf );
            if (!bytes_read) break;
            total_len += bytes_read;
            trace("read bytes %u, total_len: %u\n", bytes_read, total_len);
        }
        if (!len) break;
    }
    ok(total_len == BIG_BUFFER_LEN, "got wrong length: 0x%x\n", total_len);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_cookies( int port )
{
    static const WCHAR cookieW[] = {'/','c','o','o','k','i','e',0};
    static const WCHAR cookie2W[] = {'/','c','o','o','k','i','e','2',0};
    static const WCHAR cookie3W[] = {'/','c','o','o','k','i','e','3',0};
    static const WCHAR cookie4W[] = {'/','c','o','o','k','i','e','4',0};
    static const WCHAR cookie5W[] = {'/','c','o','o','k','i','e','5',0};
    static const WCHAR cookieheaderW[] =
        {'C','o','o','k','i','e',':',' ','n','a','m','e','=','v','a','l','u','e','2','\r','\n',0};
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, cookieW, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );

    req = WinHttpOpenRequest( con, NULL, cookie2W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, cookie2W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );

    req = WinHttpOpenRequest( con, NULL, cookie3W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, cookieheaderW, ~0u, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK || broken(status == HTTP_STATUS_BAD_REQUEST), "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, cookie2W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_BAD_REQUEST, "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, cookie4W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK, "request failed unexpectedly %u\n", status );
    WinHttpCloseHandle( req );

    req = WinHttpOpenRequest( con, NULL, cookie5W, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK || broken(status == HTTP_STATUS_BAD_REQUEST) /* < win7 */,
        "request failed unexpectedly %u\n", status );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static void do_request( HINTERNET con, const WCHAR *obj, DWORD flags )
{
    HINTERNET req;
    DWORD status, size;
    BOOL ret;

    req = WinHttpOpenRequest( con, NULL, obj, NULL, NULL, NULL, flags );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "failed to query status code %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK || broken(status == HTTP_STATUS_BAD_REQUEST) /* < win7 */,
        "request %s with flags %08x failed %u\n", wine_dbgstr_w(obj), flags, status );
    WinHttpCloseHandle( req );
}

static void test_request_path_escapes( int port )
{
    static const WCHAR objW[] =
        {'/','e','s','c','a','p','e','\r','\n',0x1f,0x7f,'<',' ','%','o','n','e','?',0x1f,0x7f,' ','!','"','#',
         '$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_','`',
         '{','|','}','~','\r','\n',0};
    static const WCHAR obj2W[] =
        {'/','e','s','c','a','p','e','\r','\n',0x1f,0x7f,'<',' ','%','t','w','o','?',0x1f,0x7f,' ','!','"','#',
         '$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_','`',
         '{','|','}','~','\r','\n',0};
    static const WCHAR obj3W[] =
        {'/','e','s','c','a','p','e','\r','\n',0x1f,0x7f,'<',' ','%','t','h','r','e','e','?',0x1f,0x7f,' ','!',
         '"','#','$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^',
         '_','`','{','|','}','~','\r','\n',0};
    static const WCHAR obj4W[] =
        {'/','e','s','c','a','p','e','\r','\n',0x1f,0x7f,'<',' ','%','f','o','u','r','?',0x1f,0x7f,' ','!','"',
         '#','$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_',
         '`','{','|','}','~','\r','\n',0};
    static const WCHAR obj5W[] =
        {'/','e','s','c','a','p','e','&','t','e','x','t','=','o','n','e',0x80,0x7f,0x7e,0};
    static const WCHAR obj6W[] =
        {'/','e','s','c','a','p','e','&','t','e','x','t','=','t','w','o',0x80,0x7f,0x7e,0};
    static const WCHAR obj7W[] =
        {'/','e','s','c','a','p','e','&','t','e','x','t','=',0x541b,0x306e,0x540d,0x306f,0};
    HINTERNET ses, con;

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    do_request( con, objW, 0 );
    do_request( con, obj2W, WINHTTP_FLAG_ESCAPE_PERCENT );
    do_request( con, obj3W, WINHTTP_FLAG_ESCAPE_DISABLE );
    do_request( con, obj4W, WINHTTP_FLAG_ESCAPE_DISABLE_QUERY );
    do_request( con, obj5W, 0 );
    do_request( con, obj6W, WINHTTP_FLAG_ESCAPE_DISABLE );
    do_request( con, obj7W, WINHTTP_FLAG_ESCAPE_DISABLE );

    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static void test_connection_info( int port )
{
    static const WCHAR basicW[] = {'/','b','a','s','i','c',0};
    HINTERNET ses, con, req;
    WINHTTP_CONNECTION_INFO info;
    DWORD size, error;
    BOOL ret;

    ses = WinHttpOpen( test_useragent, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0 );
    ok( ses != NULL, "failed to open session %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "failed to open a connection %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, basicW, NULL, NULL, NULL, 0 );
    ok( req != NULL, "failed to open a request %u\n", GetLastError() );

    size = sizeof(info);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryOption( req, WINHTTP_OPTION_CONNECTION_INFO, &info, &size );
    error = GetLastError();
    if (!ret && error == ERROR_INVALID_PARAMETER)
    {
        win_skip( "WINHTTP_OPTION_CONNECTION_INFO not supported\n" );
        return;
    }
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_WINHTTP_INCORRECT_HANDLE_STATE, "got %u\n", error );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "failed to send request %u\n", GetLastError() );

    size = 0;
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryOption( req, WINHTTP_OPTION_CONNECTION_INFO, &info, &size );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER, "got %u\n", error );

    size = sizeof(info);
    memset( &info, 0, sizeof(info) );
    ret = WinHttpQueryOption( req, WINHTTP_OPTION_CONNECTION_INFO, &info, &size );
    ok( ret, "failed to retrieve connection info %u\n", GetLastError() );
    ok( info.cbSize == sizeof(info) || info.cbSize == sizeof(info) - sizeof(info.cbSize) /* Win7 */, "wrong size %u\n", info.cbSize );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "failed to receive response %u\n", GetLastError() );

    size = sizeof(info);
    memset( &info, 0, sizeof(info) );
    ret = WinHttpQueryOption( req, WINHTTP_OPTION_CONNECTION_INFO, &info, &size );
    ok( ret, "failed to retrieve connection info %u\n", GetLastError() );
    ok( info.cbSize == sizeof(info) || info.cbSize == sizeof(info) - sizeof(info.cbSize) /* Win7 */, "wrong size %u\n", info.cbSize );

    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static void test_passport_auth( int port )
{
    static const WCHAR passportW[] =
        {'/','p','a','s','s','p','o','r','t',0};
    static const WCHAR foundW[] =
        {'F','o','u','n','d',0};
    static const WCHAR unauthorizedW[] =
        {'U','n','a','u','t','h','o','r','i','z','e','d',0};
    static const WCHAR headersW[] =
        {'H','T','T','P','/','1','.','1',' ','4','0','1',' ','F','o','u','n','d','\r','\n',
         'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','0','\r','\n',
         'L','o','c','a','t','i','o','n',':',' ','/','\r','\n',
         'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',':',' ',
         'P','a','s','s','p','o','r','t','1','.','4','\r','\n','\r','\n',0};
    HINTERNET ses, con, req;
    DWORD status, size, option;
    WCHAR buf[128];
    BOOL ret;

    ses = WinHttpOpen( test_useragent, 0, NULL, NULL, 0 );
    ok( ses != NULL, "got %u\n", GetLastError() );

    option = WINHTTP_ENABLE_PASSPORT_AUTH;
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH, &option, sizeof(option) );
    ok( ret, "got %u\n", GetLastError() );

    con = WinHttpConnect( ses, localhostW, port, 0 );
    ok( con != NULL, "got %u\n", GetLastError() );

    req = WinHttpOpenRequest( con, NULL, passportW, NULL, NULL, NULL, 0 );
    ok( req != NULL, "got %u\n", GetLastError() );

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok( ret, "got %u\n", GetLastError() );

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret || broken(!ret && GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE) /* winxp */, "got %u\n", GetLastError() );
    if (!ret && GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE)
    {
        win_skip("no support for Passport redirects\n");
        goto cleanup;
    }

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok( ret, "got %u\n", GetLastError() );
    ok( status == HTTP_STATUS_DENIED, "got %u\n", status );

    buf[0] = 0;
    size = sizeof(buf);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_TEXT, NULL, buf, &size, NULL );
    ok( ret, "got %u\n", GetLastError() );
    ok( !lstrcmpW(foundW, buf) || broken(!lstrcmpW(unauthorizedW, buf)) /* < win7 */, "got %s\n", wine_dbgstr_w(buf) );

    buf[0] = 0;
    size = sizeof(buf);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, buf, &size, NULL );
    ok( ret || broken(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER) /* < win7 */, "got %u\n", GetLastError() );
    if (ret)
    {
        ok( size == lstrlenW(headersW) * sizeof(WCHAR), "got %u\n", size );
        ok( !lstrcmpW(headersW, buf), "got %s\n", wine_dbgstr_w(buf) );
    }

cleanup:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static void test_credentials(void)
{
    static WCHAR userW[] = {'u','s','e','r',0};
    static WCHAR passW[] = {'p','a','s','s',0};
    static WCHAR proxy_userW[] = {'p','r','o','x','y','u','s','e','r',0};
    static WCHAR proxy_passW[] = {'p','r','o','x','y','p','a','s','s',0};
    HINTERNET ses, con, req;
    DWORD size, error;
    WCHAR buffer[32];
    BOOL ret;

    ses = WinHttpOpen(test_useragent, 0, proxy_userW, proxy_passW, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, localhostW, 0, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PROXY_USERNAME, &buffer, &size);
    ok(ret, "failed to query proxy username %u\n", GetLastError());
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PROXY_PASSWORD, &buffer, &size);
    ok(ret, "failed to query proxy password %u\n", GetLastError());
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);

    ret = WinHttpSetOption(req, WINHTTP_OPTION_PROXY_USERNAME, proxy_userW, lstrlenW(proxy_userW));
    ok(ret, "failed to set username %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PROXY_USERNAME, &buffer, &size);
    ok(ret, "failed to query proxy username %u\n", GetLastError());
    ok(!winetest_strcmpW(buffer, proxy_userW), "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(size == lstrlenW(proxy_userW) * sizeof(WCHAR), "unexpected result %u\n", size);

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_USERNAME, &buffer, &size);
    ok(ret, "failed to query username %u\n", GetLastError());
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PASSWORD, &buffer, &size);
    ok(ret, "failed to query password %u\n", GetLastError());
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);

    ret = WinHttpSetOption(req, WINHTTP_OPTION_PROXY_PASSWORD, proxy_passW, lstrlenW(proxy_passW));
    ok(ret, "failed to set proxy password %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PROXY_PASSWORD, &buffer, &size);
    ok(ret, "failed to query proxy password %u\n", GetLastError());
    ok(!winetest_strcmpW(buffer, proxy_passW), "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(size == lstrlenW(proxy_passW) * sizeof(WCHAR), "unexpected result %u\n", size);

    ret = WinHttpSetOption(req, WINHTTP_OPTION_USERNAME, userW, lstrlenW(userW));
    ok(ret, "failed to set username %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_USERNAME, &buffer, &size);
    ok(ret, "failed to query username %u\n", GetLastError());
    ok(!winetest_strcmpW(buffer, userW), "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(size == lstrlenW(userW) * sizeof(WCHAR), "unexpected result %u\n", size);

    ret = WinHttpSetOption(req, WINHTTP_OPTION_PASSWORD, passW, lstrlenW(passW));
    ok(ret, "failed to set password %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PASSWORD, &buffer, &size);
    ok(ret, "failed to query password %u\n", GetLastError());
    ok(!winetest_strcmpW(buffer, passW), "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(size == lstrlenW(passW) * sizeof(WCHAR), "unexpected result %u\n", size);

    WinHttpCloseHandle(req);

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, NULL, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, NULL, passW, NULL);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    ret = WinHttpSetCredentials(req, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, userW, passW, NULL);
    ok(ret, "failed to set credentials %u\n", GetLastError());

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_USERNAME, &buffer, &size);
    ok(ret, "failed to query username %u\n", GetLastError());
    todo_wine {
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);
    }

    size = ARRAY_SIZE(buffer);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_PASSWORD, &buffer, &size);
    ok(ret, "failed to query password %u\n", GetLastError());
    todo_wine {
    ok(!buffer[0], "unexpected result %s\n", wine_dbgstr_w(buffer));
    ok(!size, "expected 0, got %u\n", size);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_IWinHttpRequest(int port)
{
    static const WCHAR data_start[] = {'<','!','D','O','C','T','Y','P','E',' ','h','t','m','l',' ','P','U','B','L','I','C'};
    static const WCHAR usernameW[] = {'u','s','e','r','n','a','m','e',0};
    static const WCHAR passwordW[] = {'p','a','s','s','w','o','r','d',0};
    static const WCHAR url1W[] = {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR url2W[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR url3W[] = {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.',
                                  'o','r','g','/','t','e','s','t','s','/','p','o','s','t','.','p','h','p',0};
    static const WCHAR method1W[] = {'G','E','T',0};
    static const WCHAR method2W[] = {'I','N','V','A','L','I','D',0};
    static const WCHAR method3W[] = {'P','O','S','T',0};
    static const WCHAR proxy_serverW[] = {'p','r','o','x','y','s','e','r','v','e','r',0};
    static const WCHAR bypas_listW[] = {'b','y','p','a','s','s','l','i','s','t',0};
    static const WCHAR connectionW[] = {'C','o','n','n','e','c','t','i','o','n',0};
    static const WCHAR dateW[] = {'D','a','t','e',0};
    static const WCHAR test_dataW[] = {'t','e','s','t','d','a','t','a',128,0};
    static const WCHAR utf8W[] = {'u','t','f','-','8',0};
    static const WCHAR unauthW[] = {'U','n','a','u','t','h','o','r','i','z','e','d',0};
    HRESULT hr;
    IWinHttpRequest *req;
    BSTR method, url, username, password, response = NULL, status_text = NULL, headers = NULL;
    BSTR date, today, connection, value = NULL;
    VARIANT async, empty, timeout, body, body2, proxy_server, bypass_list, data, cp;
    VARIANT_BOOL succeeded;
    LONG status;
    WCHAR todayW[WINHTTP_TIME_FORMAT_BUFSIZE];
    SYSTEMTIME st;
    IStream *stream, *stream2;
    LARGE_INTEGER pos;
    char buf[128];
    WCHAR bufW[128];
    DWORD count;

    GetSystemTime( &st );
    WinHttpTimeFromSystemTime( &st, todayW );

    CoInitialize( NULL );
    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &empty ) = VT_ERROR;
    V_ERROR( &empty ) = 0xdeadbeef;

    V_VT( &async ) = VT_BOOL;
    V_BOOL( &async ) = VARIANT_FALSE;

    method = SysAllocString( method3W );
    url = SysAllocString( url3W );
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( method );
    SysFreeString( url );

    V_VT( &data ) = VT_BSTR;
    V_BSTR( &data ) = SysAllocString( test_dataW );
    hr = IWinHttpRequest_Send( req, data );
    ok( hr == S_OK || hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_SERVER_RESPONSE ), "got %08x\n", hr );
    SysFreeString( V_BSTR( &data ) );
    if (hr != S_OK) goto done;

    hr = IWinHttpRequest_Open( req, NULL, NULL, empty );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    method = SysAllocString( method1W );
    hr = IWinHttpRequest_Open( req, method, NULL, empty );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_Open( req, method, NULL, async );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    url = SysAllocString( url1W );
    hr = IWinHttpRequest_Open( req, NULL, url, empty );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Open( req, method, url, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    IWinHttpRequest_Release( req );

    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    SysFreeString( url );
    url = SysAllocString( url2W );
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_UNRECOGNIZED_SCHEME ), "got %08x\n", hr );

    SysFreeString( method );
    method = SysAllocString( method2W );
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_UNRECOGNIZED_SCHEME ), "got %08x\n", hr );

    SysFreeString( method );
    method = SysAllocString( method1W );
    SysFreeString( url );
    url = SysAllocString( url1W );
    V_VT( &async ) = VT_ERROR;
    V_ERROR( &async ) = DISP_E_PARAMNOTFOUND;
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &cp ) = VT_ERROR;
    V_ERROR( &cp ) = 0xdeadbeef;
    hr = IWinHttpRequest_get_Option( req, WinHttpRequestOption_URLCodePage, &cp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &cp ) == VT_I4, "got %08x\n", V_VT( &cp ) );
    ok( V_I4( &cp ) == CP_UTF8, "got %u\n", V_I4( &cp ) );

    V_VT( &cp ) = VT_UI4;
    V_UI4( &cp ) = CP_ACP;
    hr = IWinHttpRequest_put_Option( req, WinHttpRequestOption_URLCodePage, cp );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &cp ) = VT_ERROR;
    V_ERROR( &cp ) = 0xdeadbeef;
    hr = IWinHttpRequest_get_Option( req, WinHttpRequestOption_URLCodePage, &cp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &cp ) == VT_I4, "got %08x\n", V_VT( &cp ) );
    ok( V_I4( &cp ) == CP_ACP, "got %u\n", V_I4( &cp ) );

    value = SysAllocString( utf8W );
    V_VT( &cp ) = VT_BSTR;
    V_BSTR( &cp ) = value;
    hr = IWinHttpRequest_put_Option( req, WinHttpRequestOption_URLCodePage, cp );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( value );

    V_VT( &cp ) = VT_ERROR;
    V_ERROR( &cp ) = 0xdeadbeef;
    hr = IWinHttpRequest_get_Option( req, WinHttpRequestOption_URLCodePage, &cp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &cp ) == VT_I4, "got %08x\n", V_VT( &cp ) );
    ok( V_I4( &cp ) == CP_UTF8, "got %u\n", V_I4( &cp ) );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN ), "got %08x\n", hr );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    IWinHttpRequest_Release( req );

    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_Status( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_StatusText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_StatusText( req, &status_text );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseBody( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetTimeouts( req, 10000, 10000, 10000, 10000 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, NULL, NULL, 0xdeadbeef );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN ), "got %08x\n", hr );

    VariantInit( &proxy_server );
    V_VT( &proxy_server ) = VT_ERROR;
    VariantInit( &bypass_list );
    V_VT( &bypass_list ) = VT_ERROR;
    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_PROXY, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, &headers );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_GetResponseHeader( req, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    connection = SysAllocString( connectionW );
    hr = IWinHttpRequest_GetResponseHeader( req, connection, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_GetResponseHeader( req, connection, &value );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_SetRequestHeader( req, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    date = SysAllocString( dateW );
    hr = IWinHttpRequest_SetRequestHeader( req, date, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN ), "got %08x\n", hr );

    today = SysAllocString( todayW );
    hr = IWinHttpRequest_SetRequestHeader( req, date, today );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN ), "got %08x\n", hr );

    hr = IWinHttpRequest_SetAutoLogonPolicy( req, 0xdeadbeef );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetAutoLogonPolicy( req, AutoLogonPolicy_OnlyIfBypassProxy );
    ok( hr == S_OK, "got %08x\n", hr );

    SysFreeString( method );
    method = SysAllocString( method1W );
    SysFreeString( url );
    url = SysAllocString( url1W );
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_StatusText( req, &status_text );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseBody( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetTimeouts( req, 10000, 10000, 10000, 10000 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, NULL, NULL, 0xdeadbeef );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    username = SysAllocString( usernameW );
    hr = IWinHttpRequest_SetCredentials( req, username, NULL, 0xdeadbeef );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    password = SysAllocString( passwordW );
    hr = IWinHttpRequest_SetCredentials( req, NULL, password, 0xdeadbeef );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, username, password, 0xdeadbeef );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, NULL, password, HTTPREQUEST_SETCREDENTIALS_FOR_SERVER );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, username, password, HTTPREQUEST_SETCREDENTIALS_FOR_SERVER );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &proxy_server ) = VT_BSTR;
    V_BSTR( &proxy_server ) = SysAllocString( proxy_serverW );
    V_VT( &bypass_list ) = VT_BSTR;
    V_BSTR( &bypass_list ) = SysAllocString( bypas_listW );
    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_PROXY, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, 0xdeadbeef, proxy_server, bypass_list );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, &headers );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_GetResponseHeader( req, connection, &value );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_SetRequestHeader( req, date, today );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetRequestHeader( req, date, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetAutoLogonPolicy( req, AutoLogonPolicy_OnlyIfBypassProxy );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !memcmp(response, data_start, sizeof(data_start)), "got %s\n", wine_dbgstr_wn(response, 32) );
    SysFreeString( response );

    hr = IWinHttpRequest_get_Status( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    status = 0;
    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == S_OK, "got %08x\n", hr );
    trace("Status=%d\n", status);

    hr = IWinHttpRequest_get_StatusText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_StatusText( req, &status_text );
    ok( hr == S_OK, "got %08x\n", hr );
    trace("StatusText=%s\n", wine_dbgstr_w(status_text));
    SysFreeString( status_text );

    hr = IWinHttpRequest_get_ResponseBody( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_SetCredentials( req, username, password, HTTPREQUEST_SETCREDENTIALS_FOR_SERVER );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_PROXY, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, &headers );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( headers );

    hr = IWinHttpRequest_GetResponseHeader( req, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_GetResponseHeader( req, connection, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_GetResponseHeader( req, connection, &value );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( value );

    hr = IWinHttpRequest_SetRequestHeader( req, date, today );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_SetAutoLogonPolicy( req, AutoLogonPolicy_OnlyIfBypassProxy );
    ok( hr == S_OK, "got %08x\n", hr );

    VariantInit( &timeout );
    V_VT( &timeout ) = VT_I4;
    V_I4( &timeout ) = 10;
    hr = IWinHttpRequest_WaitForResponse( req, timeout, &succeeded );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_StatusText( req, &status_text );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( status_text );

    hr = IWinHttpRequest_SetCredentials( req, username, password, HTTPREQUEST_SETCREDENTIALS_FOR_SERVER );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_PROXY, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( response );

    hr = IWinHttpRequest_get_ResponseBody( req, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    VariantInit( &body );
    V_VT( &body ) = VT_ERROR;
    hr = IWinHttpRequest_get_ResponseBody( req, &body );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &body ) == (VT_ARRAY|VT_UI1), "got %08x\n", V_VT( &body ) );

    hr = VariantClear( &body );
    ok( hr == S_OK, "got %08x\n", hr );

    VariantInit( &body );
    V_VT( &body ) = VT_ERROR;
    hr = IWinHttpRequest_get_ResponseStream( req, &body );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &body ) == VT_UNKNOWN, "got %08x\n", V_VT( &body ) );

    hr = IUnknown_QueryInterface( V_UNKNOWN( &body ), &IID_IStream, (void **)&stream );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_UNKNOWN( &body ) == (IUnknown *)stream, "got different interface pointer\n" );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = IStream_Read( stream, buf, 128, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count != 0xdeadbeef, "count not set\n" );
    ok( buf[0], "no data\n" );

    VariantInit( &body2 );
    V_VT( &body2 ) = VT_ERROR;
    hr = IWinHttpRequest_get_ResponseStream( req, &body2 );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &body2 ) == VT_UNKNOWN, "got %08x\n", V_VT( &body2 ) );
    ok( V_UNKNOWN( &body ) != V_UNKNOWN( &body2 ), "got same interface pointer\n" );

    hr = IUnknown_QueryInterface( V_UNKNOWN( &body2 ), &IID_IStream, (void **)&stream2 );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_UNKNOWN( &body2 ) == (IUnknown *)stream2, "got different interface pointer\n" );
    IStream_Release( stream2 );

    hr = VariantClear( &body );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = VariantClear( &body2 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_PROXY, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_SetProxy( req, HTTPREQUEST_PROXYSETTING_DIRECT, proxy_server, bypass_list );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_GetAllResponseHeaders( req, &headers );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( headers );

    hr = IWinHttpRequest_GetResponseHeader( req, connection, &value );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( value );

    hr = IWinHttpRequest_SetRequestHeader( req, date, today );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND ), "got %08x\n", hr );

    hr = IWinHttpRequest_SetAutoLogonPolicy( req, AutoLogonPolicy_OnlyIfBypassProxy );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Abort( req );
    ok( hr == S_OK, "got %08x\n", hr );

    IWinHttpRequest_Release( req );

    pos.QuadPart = 0;
    hr = IStream_Seek( stream, pos, STREAM_SEEK_SET, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    buf[0] = 0;
    count = 0xdeadbeef;
    hr = IStream_Read( stream, buf, 128, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count != 0xdeadbeef, "count not set\n" );
    ok( buf[0], "no data\n" );
    IStream_Release( stream );

    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &async ) = VT_I4;
    V_I4( &async ) = 1;
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWinHttpRequest_WaitForResponse( req, timeout, &succeeded );
    ok( hr == S_OK, "got %08x\n", hr );

    IWinHttpRequest_Release( req );

    SysFreeString( method );
    SysFreeString( url );
    SysFreeString( username );
    SysFreeString( password );
    SysFreeString( connection );
    SysFreeString( date );
    SysFreeString( today );
    VariantClear( &proxy_server );
    VariantClear( &bypass_list );

    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    url = SysAllocString( test_winehq_https );
    method = SysAllocString( method3W );
    V_VT( &async ) = VT_BOOL;
    V_BOOL( &async ) = VARIANT_FALSE;
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( method );
    SysFreeString( url );

    hr = IWinHttpRequest_Send( req, empty );
    ok( hr == S_OK || hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_INVALID_SERVER_RESPONSE ) ||
        hr == SEC_E_ILLEGAL_MESSAGE /* winxp */, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == S_OK, "got %08x\n", hr );
#ifdef __REACTOS__
    ok( !memcmp(response, data_start, sizeof(data_start)), "got %s\n",
        wine_dbgstr_wn(response, min(SysStringLen(response), 32)) );
#else
    ok( !memcmp(response, data_start, sizeof(data_start)), "got %s\n", wine_dbgstr_wn(response, 32) );
#endif
    SysFreeString( response );

    IWinHttpRequest_Release( req );

    hr = CoCreateInstance( &CLSID_WinHttpRequest, NULL, CLSCTX_INPROC_SERVER, &IID_IWinHttpRequest, (void **)&req );
    ok( hr == S_OK, "got %08x\n", hr );

    sprintf( buf, "http://localhost:%d/auth", port );
    MultiByteToWideChar( CP_ACP, 0, buf, -1, bufW, ARRAY_SIZE( bufW ));
    url = SysAllocString( bufW );
    method = SysAllocString( method3W );
    V_VT( &async ) = VT_BOOL;
    V_BOOL( &async ) = VARIANT_FALSE;
    hr = IWinHttpRequest_Open( req, method, url, async );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( method );
    SysFreeString( url );

    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND ), "got %08x\n", hr );

    V_VT( &data ) = VT_BSTR;
    V_BSTR( &data ) = SysAllocString( test_dataW );
    hr = IWinHttpRequest_Send( req, data );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( V_BSTR( &data ) );

    hr = IWinHttpRequest_get_ResponseText( req, &response );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !memcmp( response, unauthW, sizeof(unauthW) ), "got %s\n", wine_dbgstr_w(response) );
    SysFreeString( response );

    status = 0xdeadbeef;
    hr = IWinHttpRequest_get_Status( req, &status );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( status == HTTP_STATUS_DENIED, "got %d\n", status );

done:
    IWinHttpRequest_Release( req );
    CoUninitialize();
}

static void request_get_property(IWinHttpRequest *request, int property, VARIANT *ret)
{
    DISPPARAMS params;
    VARIANT arg;
    HRESULT hr;

    memset(&params, 0, sizeof(params));
    params.cNamedArgs = 0;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 1;
    params.rgvarg = &arg;
    VariantInit(&arg);
    V_VT(&arg) = VT_I4;
    V_I4(&arg) = property;
    VariantInit(ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0,
                                DISPATCH_PROPERTYGET, &params, ret, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);
}

static void test_IWinHttpRequest_Invoke(void)
{
    static const WCHAR utf8W[] = {'U','T','F','-','8',0};
    static const WCHAR regid[] = {'W','i','n','H','t','t','p','.','W','i','n','H','t','t','p','R','e','q','u','e','s','t','.','5','.','1',0};
    WCHAR openW[] = {'O','p','e','n',0};
    WCHAR optionW[] = {'O','p','t','i','o','n',0};
    OLECHAR *open = openW, *option = optionW;
    BSTR utf8;
    CLSID clsid;
    IWinHttpRequest *request;
    IDispatch *dispatch;
    DISPID id;
    DISPPARAMS params;
    VARIANT arg[3], ret;
    UINT err;
    BOOL bret;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CLSIDFromProgID(regid, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID error %#x\n", hr);
    bret = IsEqualIID(&clsid, &CLSID_WinHttpRequest);
    ok(bret || broken(!bret) /* win2003 */, "not expected %s\n", wine_dbgstr_guid(&clsid));

    hr = CoCreateInstance(&CLSID_WinHttpRequest, 0, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&request);
    ok(hr == S_OK, "error %#x\n", hr);

    hr = IWinHttpRequest_QueryInterface(request, &IID_IDispatch, (void **)&dispatch);
    ok(hr == S_OK, "error %#x\n", hr);
    IDispatch_Release(dispatch);

    hr = IWinHttpRequest_GetIDsOfNames(request, &IID_NULL, &open, 1, 0x0409, &id);
    ok(hr == S_OK, "error %#x\n", hr);
    ok(id == DISPID_HTTPREQUEST_OPEN, "expected DISPID_HTTPREQUEST_OPEN, got %u\n", id);

    hr = IWinHttpRequest_GetIDsOfNames(request, &IID_NULL, &option, 1, 0x0409, &id);
    ok(hr == S_OK, "error %#x\n", hr);
    ok(id == DISPID_HTTPREQUEST_OPTION, "expected DISPID_HTTPREQUEST_OPTION, got %u\n", id);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    memset(&params, 0, sizeof(params));
    params.cArgs = 2;
    params.cNamedArgs = 0;
    params.rgvarg = arg;
    V_VT(&arg[0]) = VT_I4;
    V_I4(&arg[0]) = 1252;
    V_VT(&arg[1]) = VT_R8;
    V_R8(&arg[1]) = 2.0; /* WinHttpRequestOption_URLCodePage */
    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0,
                                DISPATCH_METHOD, &params, NULL, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    memset(&params, 0, sizeof(params));
    params.cArgs = 2;
    params.cNamedArgs = 0;
    params.rgvarg = arg;
    V_VT(&arg[0]) = VT_I4;
    V_I4(&arg[0]) = 1252;
    V_VT(&arg[1]) = VT_R8;
    V_R8(&arg[1]) = 2.0; /* WinHttpRequestOption_URLCodePage */
    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0,
                                DISPATCH_METHOD | DISPATCH_PROPERTYPUT, &params, NULL, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    memset(&params, 0, sizeof(params));
    params.cArgs = 2;
    params.cNamedArgs = 0;
    params.rgvarg = arg;
    V_VT(&arg[0]) = VT_I4;
    V_I4(&arg[0]) = 1252;
    V_VT(&arg[1]) = VT_R8;
    V_R8(&arg[1]) = 2.0; /* WinHttpRequestOption_URLCodePage */
    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0,
                                DISPATCH_PROPERTYPUT, &params, NULL, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 1252, "expected 1252, got %d\n", V_I4(&ret));

    memset(&params, 0, sizeof(params));
    params.cArgs = 2;
    params.cNamedArgs = 0;
    params.rgvarg = arg;
    V_VT(&arg[0]) = VT_BSTR;
    utf8 = SysAllocString(utf8W);
    V_BSTR(&arg[0]) = utf8;
    V_VT(&arg[1]) = VT_R8;
    V_R8(&arg[1]) = 2.0; /* WinHttpRequestOption_URLCodePage */
    hr = IWinHttpRequest_Invoke(request, id, &IID_NULL, 0, DISPATCH_METHOD, &params, NULL, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 1252, "expected 1252, got %d\n", V_I4(&ret));

    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, id, &IID_NULL, 0, DISPATCH_METHOD, &params, &ret, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 1252, "expected 1252, got %d\n", V_I4(&ret));

    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, id, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &params, &ret, NULL, &err);
    ok(hr == S_OK, "error %#x\n", hr);

    request_get_property(request, WinHttpRequestOption_URLCodePage, &ret);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);

    hr = IWinHttpRequest_Invoke(request, 255, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "error %#x\n", hr);

    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_IUnknown, 0, DISPATCH_PROPERTYPUT, &params, &ret, NULL, &err);
    ok(hr == DISP_E_UNKNOWNINTERFACE, "error %#x\n", hr);

    VariantInit(&ret);
    if (0) /* crashes */
        hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYPUT, NULL, &ret, NULL, &err);

    params.cArgs = 1;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &params, &ret, NULL, &err);
    ok(hr == DISP_E_TYPEMISMATCH, "error %#x\n", hr);

    VariantInit(&arg[2]);
    params.cArgs = 3;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &params, &ret, NULL, &err);
todo_wine
    ok(hr == S_OK, "error %#x\n", hr);

    VariantInit(&arg[0]);
    VariantInit(&arg[1]);
    VariantInit(&arg[2]);

    params.cArgs = 1;
    V_VT(&arg[0]) = VT_I4;
    V_I4(&arg[0]) = WinHttpRequestOption_URLCodePage;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYGET, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);

    V_VT(&ret) = 0xdead;
    V_I4(&ret) = 0xbeef;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_METHOD|DISPATCH_PROPERTYGET, &params, &ret, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    V_VT(&ret) = 0xdead;
    V_I4(&ret) = 0xbeef;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_METHOD, &params, &ret, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);
    ok(V_VT(&ret) == VT_I4, "expected VT_I4, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == CP_UTF8, "expected CP_UTF8, got %d\n", V_I4(&ret));

    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_METHOD|DISPATCH_PROPERTYGET, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);

    V_VT(&ret) = 0xdead;
    V_I4(&ret) = 0xbeef;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, 0, &params, &ret, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "expected VT_EMPTY, got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 0xbeef || V_I4(&ret) == 0 /* Win8 */, "expected 0xdead, got %d\n", V_I4(&ret));

    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, 0, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);

    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_IUnknown, 0, DISPATCH_PROPERTYGET, &params, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNINTERFACE, "error %#x\n", hr);

    params.cArgs = 2;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYGET, &params, NULL, NULL, NULL);
todo_wine
    ok(hr == S_OK, "error %#x\n", hr);

    params.cArgs = 0;
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_OPTION, &IID_NULL, 0, DISPATCH_PROPERTYGET, &params, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTFOUND, "error %#x\n", hr);

    SysFreeString(utf8);

    params.cArgs = 1;
    V_VT(&arg[0]) = VT_I4;
    V_I4(&arg[0]) = AutoLogonPolicy_Never;
    VariantInit(&ret);
    hr = IWinHttpRequest_Invoke(request, DISPID_HTTPREQUEST_SETAUTOLOGONPOLICY, &IID_NULL, 0,
                                DISPATCH_METHOD, &params, &ret, NULL, NULL);
    ok(hr == S_OK, "error %#x\n", hr);

    IWinHttpRequest_Release(request);

    CoUninitialize();
}

static void test_WinHttpDetectAutoProxyConfigUrl(void)
{
    BOOL ret;
    WCHAR *url;
    DWORD error;

    SetLastError(0xdeadbeef);
    ret = WinHttpDetectAutoProxyConfigUrl( 0, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    url = NULL;
    SetLastError(0xdeadbeef);
    ret = WinHttpDetectAutoProxyConfigUrl( 0, &url );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError(0xdeadbeef);
    ret = WinHttpDetectAutoProxyConfigUrl( WINHTTP_AUTO_DETECT_TYPE_DNS_A, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    url = (WCHAR *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpDetectAutoProxyConfigUrl( WINHTTP_AUTO_DETECT_TYPE_DNS_A, &url );
    error = GetLastError();
    if (!ret)
    {
        ok( error == ERROR_WINHTTP_AUTODETECTION_FAILED, "got %u\n", error );
        ok( !url || broken(url == (WCHAR *)0xdeadbeef), "got %p\n", url );
    }
    else
    {
        trace("%s\n", wine_dbgstr_w(url));
        GlobalFree( url );
    }

    url = (WCHAR *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WinHttpDetectAutoProxyConfigUrl( WINHTTP_AUTO_DETECT_TYPE_DHCP, &url );
    error = GetLastError();
    if (!ret)
    {
        ok( error == ERROR_WINHTTP_AUTODETECTION_FAILED, "got %u\n", error );
        ok( !url || broken(url == (WCHAR *)0xdeadbeef), "got %p\n", url );
    }
    else
    {
        ok( error == ERROR_SUCCESS, "got %u\n", error );
        trace("%s\n", wine_dbgstr_w(url));
        GlobalFree( url );
    }
}

static void test_WinHttpGetIEProxyConfigForCurrentUser(void)
{
    BOOL ret;
    DWORD error;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG cfg;

    memset( &cfg, 0, sizeof(cfg) );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetIEProxyConfigForCurrentUser( NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetIEProxyConfigForCurrentUser( &cfg );
    error = GetLastError();
    ok( ret, "expected success\n" );
    ok( error == ERROR_SUCCESS || broken(error == ERROR_NO_TOKEN) /* < win7 */, "got %u\n", error );

    trace("IEProxy.AutoDetect=%d\n", cfg.fAutoDetect);
    trace("IEProxy.AutoConfigUrl=%s\n", wine_dbgstr_w(cfg.lpszAutoConfigUrl));
    trace("IEProxy.Proxy=%s\n", wine_dbgstr_w(cfg.lpszProxy));
    trace("IEProxy.ProxyBypass=%s\n", wine_dbgstr_w(cfg.lpszProxyBypass));
    GlobalFree( cfg.lpszAutoConfigUrl );
    GlobalFree( cfg.lpszProxy );
    GlobalFree( cfg.lpszProxyBypass );
}

static void test_WinHttpGetProxyForUrl(void)
{
    static const WCHAR urlW[] = {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR wpadW[] = {'h','t','t','p',':','/','/','w','p','a','d','/','w','p','a','d','.','d','a','t',0};
    static const WCHAR emptyW[] = {0};
    BOOL ret;
    DWORD error;
    HINTERNET session;
    WINHTTP_AUTOPROXY_OPTIONS options;
    WINHTTP_PROXY_INFO info;

    memset( &options, 0, sizeof(options) );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( NULL, NULL, NULL, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_HANDLE, "got %u\n", error );

    session = WinHttpOpen( test_useragent, 0, NULL, NULL, 0 );
    ok( session != NULL, "failed to open session %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, NULL, NULL, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, emptyW, NULL, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, NULL, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, &options, &info );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
    options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, &options, NULL );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
    options.dwAutoDetectFlags = 0;

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, &options, &info );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL;
    options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;

    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, &options, &info );
    error = GetLastError();
    ok( !ret, "expected failure\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
    options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DNS_A;

    memset( &info, 0, sizeof(info) );
    SetLastError(0xdeadbeef);
    ret = WinHttpGetProxyForUrl( session, urlW, &options, &info );
    error = GetLastError();
    if (ret)
    {
        ok( error == ERROR_SUCCESS, "got %u\n", error );
        trace("Proxy.AccessType=%u\n", info.dwAccessType);
        trace("Proxy.Proxy=%s\n", wine_dbgstr_w(info.lpszProxy));
        trace("Proxy.ProxyBypass=%s\n", wine_dbgstr_w(info.lpszProxyBypass));
        GlobalFree( info.lpszProxy );
        GlobalFree( info.lpszProxyBypass );
    }

    options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
    options.dwAutoDetectFlags = 0;
    options.lpszAutoConfigUrl = wpadW;

    memset( &info, 0, sizeof(info) );
    ret = WinHttpGetProxyForUrl( session, urlW, &options, &info );
    if (ret)
    {
        trace("Proxy.AccessType=%u\n", info.dwAccessType);
        trace("Proxy.Proxy=%s\n", wine_dbgstr_w(info.lpszProxy));
        trace("Proxy.ProxyBypass=%s\n", wine_dbgstr_w(info.lpszProxyBypass));
        GlobalFree( info.lpszProxy );
        GlobalFree( info.lpszProxyBypass );
    }
    WinHttpCloseHandle( session );
}

static void test_chunked_read(void)
{
    static const WCHAR verb[] = {'/','t','e','s','t','s','/','c','h','u','n','k','e','d',0};
    static const WCHAR chunked[] = {'c','h','u','n','k','e','d',0};
    WCHAR header[32];
    DWORD len, err;
    HINTERNET ses, con = NULL, req = NULL;
    BOOL ret;

    trace( "starting chunked read test\n" );

    ses = WinHttpOpen( test_useragent, 0, NULL, NULL, 0 );
    ok( ses != NULL, "WinHttpOpen failed with error %u\n", GetLastError() );
    if (!ses) goto done;

    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok( con != NULL, "WinHttpConnect failed with error %u\n", GetLastError() );
    if (!con) goto done;

    req = WinHttpOpenRequest( con, NULL, verb, NULL, NULL, NULL, 0 );
    ok( req != NULL, "WinHttpOpenRequest failed with error %u\n", GetLastError() );
    if (!req) goto done;

    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok( ret, "WinHttpSendRequest failed with error %u\n", GetLastError() );
    if (!ret) goto done;

    ret = WinHttpReceiveResponse( req, NULL );
    ok( ret, "WinHttpReceiveResponse failed with error %u\n", GetLastError() );
    if (!ret) goto done;

    header[0] = 0;
    len = sizeof(header);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_TRANSFER_ENCODING, NULL, header, &len, 0 );
    ok( ret, "failed to get TRANSFER_ENCODING header (error %u)\n", GetLastError() );
    ok( !lstrcmpW( header, chunked ), "wrong transfer encoding %s\n", wine_dbgstr_w(header) );
    trace( "transfer encoding: %s\n", wine_dbgstr_w(header) );

    header[0] = 0;
    len = sizeof(header);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_CONTENT_LENGTH, NULL, &header, &len, 0 );
    ok( !ret, "unexpected CONTENT_LENGTH header %s\n", wine_dbgstr_w(header) );
    ok( GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND, "wrong error %u\n", GetLastError() );

    trace( "entering query loop\n" );
    for (;;)
    {
        len = 0xdeadbeef;
        ret = WinHttpQueryDataAvailable( req, &len );
        ok( ret, "WinHttpQueryDataAvailable failed with error %u\n", GetLastError() );
        if (ret) ok( len != 0xdeadbeef, "WinHttpQueryDataAvailable return wrong length\n" );
        trace( "got %u available\n", len );
        if (len)
        {
            DWORD bytes_read;
            char *buf = HeapAlloc( GetProcessHeap(), 0, len + 1 );

            ret = WinHttpReadData( req, buf, len, &bytes_read );
            ok(ret, "WinHttpReadData failed: %u.\n", GetLastError());

            buf[bytes_read] = 0;
            trace( "WinHttpReadData -> %d %u\n", ret, bytes_read );
            ok( len == bytes_read, "only got %u of %u available\n", bytes_read, len );
            ok( buf[bytes_read - 1] == '\n', "received partial line '%s'\n", buf );

            HeapFree( GetProcessHeap(), 0, buf );
            if (!bytes_read) break;
        }
        if (!len) break;
    }
    trace( "done\n" );

done:
    if (req) WinHttpCloseHandle( req );
    if (con) WinHttpCloseHandle( con );
    if (ses) WinHttpCloseHandle( ses );
}

START_TEST (winhttp)
{
    static const WCHAR basicW[] = {'/','b','a','s','i','c',0};
    static const WCHAR quitW[] = {'/','q','u','i','t',0};
    struct server_info si;
    HANDLE thread;
    DWORD ret;

    test_WinHttpOpenRequest();
    test_WinHttpSendRequest();
    test_WinHttpTimeFromSystemTime();
    test_WinHttpTimeToSystemTime();
    test_WinHttpAddHeaders();
    test_secure_connection();
    test_request_parameter_defaults();
    test_WinHttpQueryOption();
    test_set_default_proxy_config();
    test_empty_headers_param();
    test_timeouts();
    test_resolve_timeout();
    test_credentials();
    test_IWinHttpRequest_Invoke();
    test_WinHttpDetectAutoProxyConfigUrl();
    test_WinHttpGetIEProxyConfigForCurrentUser();
    test_WinHttpGetProxyForUrl();
    test_chunked_read();

    si.event = CreateEventW(NULL, 0, 0, NULL);
    si.port = 7532;

    thread = CreateThread(NULL, 0, server_thread, &si, 0, NULL);
    ok(thread != NULL, "failed to create thread %u\n", GetLastError());

    ret = WaitForSingleObject(si.event, 10000);
    ok(ret == WAIT_OBJECT_0, "failed to start winhttp test server %u\n", GetLastError());
    if (ret != WAIT_OBJECT_0)
    {
        CloseHandle(thread);
        return;
    }

    test_IWinHttpRequest(si.port);
    test_connection_info(si.port);
    test_basic_request(si.port, NULL, basicW);
    test_no_headers(si.port);
    test_no_content(si.port);
    test_head_request(si.port);
    test_not_modified(si.port);
    test_basic_authentication(si.port);
    test_multi_authentication(si.port);
    test_large_data_authentication(si.port);
    test_bad_header(si.port);
#ifdef __REACTOS__
    if (!winetest_interactive)
    {
        skip("Skipping tests due to hang. See ROSTESTS-350\n");
    }
    else
    {
        test_multiple_reads(si.port);
        test_cookies(si.port);
        test_request_path_escapes(si.port);
        test_passport_auth(si.port);

        /* send the basic request again to shutdown the server thread */
        test_basic_request(si.port, NULL, quitW);
    }
#else
    test_multiple_reads(si.port);
    test_cookies(si.port);
    test_request_path_escapes(si.port);
    test_passport_auth(si.port);

    /* send the basic request again to shutdown the server thread */
    test_basic_request(si.port, NULL, quitW);
#endif

    WaitForSingleObject(thread, 3000);
    CloseHandle(thread);
}
