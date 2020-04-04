/*
 * Wininet - HTTP tests
 *
 * Copyright 2002 Aric Stewart
 * Copyright 2004 Mike McCormack
 * Copyright 2005 Hans Leidekker
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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winineti.h"
#include "winsock2.h"

#include "wine/test.h"

/* Undocumented security flags */
#define _SECURITY_FLAG_CERT_REV_FAILED    0x00800000
#define _SECURITY_FLAG_CERT_INVALID_CA    0x01000000
#define _SECURITY_FLAG_CERT_INVALID_CN    0x02000000
#define _SECURITY_FLAG_CERT_INVALID_DATE  0x04000000

#define TEST_URL "http://test.winehq.org/tests/hello.html"

static BOOL first_connection_to_test_url = TRUE;

/* Adapted from dlls/urlmon/tests/protocol.c */

#define SET_EXPECT2(status, num) \
    expect[status] = num

#define SET_EXPECT(status) \
    SET_EXPECT2(status, 1)

#define SET_OPTIONAL2(status, num) \
    optional[status] = num

#define SET_OPTIONAL(status) \
    SET_OPTIONAL2(status, 1)

/* SET_WINE_ALLOW's should be used with an appropriate
 * todo_wine CHECK_NOTIFIED at a later point in the code */
#define SET_WINE_ALLOW2(status, num) \
    wine_allow[status] = num

#define SET_WINE_ALLOW(status) \
    SET_WINE_ALLOW2(status, 1)

#define CHECK_EXPECT(status) \
    do { \
        if (!expect[status] && !optional[status] && wine_allow[status]) \
        { \
            todo_wine ok(expect[status], "unexpected status %d (%s)\n", status, \
                         status < MAX_INTERNET_STATUS && status_string[status] ? \
                         status_string[status] : "unknown");            \
            wine_allow[status]--; \
        } \
        else \
        { \
            ok(expect[status] || optional[status], "unexpected status %d (%s)\n", status,   \
               status < MAX_INTERNET_STATUS && status_string[status] ? \
               status_string[status] : "unknown");                      \
            if (expect[status]) expect[status]--; \
            else if(optional[status]) optional[status]--; \
        } \
        notified[status]++; \
    }while(0)

/* CLEAR_NOTIFIED used in cases when notification behavior
 * differs between Windows versions */
#define CLEAR_NOTIFIED(status) \
    expect[status] = optional[status] = wine_allow[status] = notified[status] = 0;

#define CHECK_NOTIFIED2(status, num) \
    do { \
        ok(notified[status] + optional[status] == (num), \
           "expected status %d (%s) %d times, received %d times\n", \
           status, status < MAX_INTERNET_STATUS && status_string[status] ? \
           status_string[status] : "unknown", (num), notified[status]); \
        CLEAR_NOTIFIED(status);                                         \
    }while(0)

#define CHECK_NOTIFIED(status) \
    CHECK_NOTIFIED2(status, 1)

#define CHECK_NOT_NOTIFIED(status) \
    CHECK_NOTIFIED2(status, 0)

#define MAX_INTERNET_STATUS (INTERNET_STATUS_COOKIE_HISTORY+1)
static int expect[MAX_INTERNET_STATUS], optional[MAX_INTERNET_STATUS],
    wine_allow[MAX_INTERNET_STATUS], notified[MAX_INTERNET_STATUS];
static const char *status_string[MAX_INTERNET_STATUS];

static HANDLE complete_event, conn_close_event, conn_wait_event, server_req_rec_event, request_sent_event;
static DWORD req_error;
static BOOL is_ie7plus = TRUE;

#define TESTF_REDIRECT      0x01
#define TESTF_COMPRESSED    0x02
#define TESTF_CHUNKED       0x04

typedef struct {
    const char *url;
    const char *redirected_url;
    const char *host;
    const char *path;
    const char *headers;
    DWORD flags;
    const char *post_data;
    const char *content;
} test_data_t;

static const test_data_t test_data[] = {
    {
        "http://test.winehq.org/tests/data.php",
        "http://test.winehq.org/tests/data.php",
        "test.winehq.org",
        "/tests/data.php",
        "",
        TESTF_CHUNKED
    },
    {
        "http://test.winehq.org/tests/redirect",
        "http://test.winehq.org/tests/hello.html",
        "test.winehq.org",
        "/tests/redirect",
        "",
        TESTF_REDIRECT
    },
    {
        "http://test.winehq.org/tests/gzip.php",
        "http://test.winehq.org/tests/gzip.php",
        "test.winehq.org",
        "/tests/gzip.php",
        "Accept-Encoding: gzip, deflate",
        TESTF_COMPRESSED
    },
    {
        "http://test.winehq.org/tests/post.php",
        "http://test.winehq.org/tests/post.php",
        "test.winehq.org",
        "/tests/post.php",
        "Content-Type: application/x-www-form-urlencoded",
        0,
        "mode=Test",
        "mode => Test\n"
    }
};

static INTERNET_STATUS_CALLBACK (WINAPI *pInternetSetStatusCallbackA)(HINTERNET ,INTERNET_STATUS_CALLBACK);
static INTERNET_STATUS_CALLBACK (WINAPI *pInternetSetStatusCallbackW)(HINTERNET ,INTERNET_STATUS_CALLBACK);
static BOOL (WINAPI *pInternetGetSecurityInfoByURLA)(LPSTR,PCCERT_CHAIN_CONTEXT*,DWORD*);

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, ARRAY_SIZE(buf));
    return lstrcmpW(strw, buf);
}

static BOOL proxy_active(void)
{
    HKEY internet_settings;
    DWORD proxy_enable;
    DWORD size;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_QUERY_VALUE, &internet_settings) != ERROR_SUCCESS)
        return FALSE;

    size = sizeof(DWORD);
    if (RegQueryValueExA(internet_settings, "ProxyEnable", NULL, NULL, (LPBYTE) &proxy_enable, &size) != ERROR_SUCCESS)
        proxy_enable = 0;

    RegCloseKey(internet_settings);

    return proxy_enable != 0;
}

static void init_events(void)
{
    complete_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    conn_close_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    conn_wait_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    server_req_rec_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    request_sent_event = CreateEventW(NULL, FALSE, FALSE, NULL);
}

static void free_events(void)
{
    CloseHandle(complete_event);
    CloseHandle(conn_close_event);
    CloseHandle(conn_wait_event);
    CloseHandle(server_req_rec_event);
    CloseHandle(request_sent_event);
}

static void reset_events(void)
{
    ResetEvent(complete_event);
    ResetEvent(conn_close_event);
    ResetEvent(conn_wait_event);
    ResetEvent(server_req_rec_event);
    ResetEvent(request_sent_event);
}

#define test_status_code(a,b) _test_status_code(__LINE__,a,b, FALSE)
#define test_status_code_todo(a,b) _test_status_code(__LINE__,a,b, TRUE)
static void _test_status_code(unsigned line, HINTERNET req, DWORD excode, BOOL is_todo)
{
    DWORD code, size, index;
    char exbuf[12], bufa[10];
    WCHAR bufw[10];
    BOOL res;

    code = 0xdeadbeef;
    size = sizeof(code);
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &code, &size, NULL);
    ok_(__FILE__,line)(res, "[1] HttpQueryInfoA(HTTP_QUERY_STATUS_CODE|number) failed: %u\n", GetLastError());
    todo_wine_if (is_todo)
        ok_(__FILE__,line)(code == excode, "code = %d, expected %d\n", code, excode);
    ok_(__FILE__,line)(size == sizeof(code), "size = %u\n", size);

    code = 0xdeadbeef;
    index = 0;
    size = sizeof(code);
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &code, &size, &index);
    ok_(__FILE__,line)(res, "[2] HttpQueryInfoA(HTTP_QUERY_STATUS_CODE|number index) failed: %u\n", GetLastError());
    ok_(__FILE__,line)(!index, "index = %d, expected 0\n", index);
    ok_(__FILE__,line)(size == sizeof(code), "size = %u\n", size);

    sprintf(exbuf, "%u", excode);

    size = sizeof(bufa);
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE, bufa, &size, NULL);
    ok_(__FILE__,line)(res, "[3] HttpQueryInfoA(HTTP_QUERY_STATUS_CODE) failed: %u\n", GetLastError());
    todo_wine_if (is_todo)
        ok_(__FILE__,line)(!strcmp(bufa, exbuf), "unexpected status code %s, expected %s\n", bufa, exbuf);
    ok_(__FILE__,line)(size == strlen(exbuf), "unexpected size %d for \"%s\"\n", size, exbuf);

    size = 0;
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE, NULL, &size, NULL);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                       "[4] HttpQueryInfoA(HTTP_QUERY_STATUS_CODE) failed: %u\n", GetLastError());
    ok_(__FILE__,line)(size == strlen(exbuf)+1, "unexpected size %d for \"%s\"\n", size, exbuf);

    size = sizeof(bufw);
    res = HttpQueryInfoW(req, HTTP_QUERY_STATUS_CODE, bufw, &size, NULL);
    ok_(__FILE__,line)(res, "[5] HttpQueryInfoW(HTTP_QUERY_STATUS_CODE) failed: %u\n", GetLastError());
    todo_wine_if (is_todo)
        ok_(__FILE__,line)(!strcmp_wa(bufw, exbuf), "unexpected status code %s, expected %s\n", bufa, exbuf);
    ok_(__FILE__,line)(size == strlen(exbuf)*sizeof(WCHAR), "unexpected size %d for \"%s\"\n", size, exbuf);

    size = 0;
    res = HttpQueryInfoW(req, HTTP_QUERY_STATUS_CODE, bufw, &size, NULL);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                       "[6] HttpQueryInfoW(HTTP_QUERY_STATUS_CODE) failed: %u\n", GetLastError());
    ok_(__FILE__,line)(size == (strlen(exbuf)+1)*sizeof(WCHAR), "unexpected size %d for \"%s\"\n", size, exbuf);

    if(0) {
    size = sizeof(bufw);
    res = HttpQueryInfoW(req, HTTP_QUERY_STATUS_CODE, NULL, &size, NULL);
    ok(!res && GetLastError() == ERROR_INVALID_PARAMETER, "HttpQueryInfo(HTTP_QUERY_STATUS_CODE) failed: %u\n", GetLastError());
    ok(size == sizeof(bufw), "unexpected size %d\n", size);
    }

    code = 0xdeadbeef;
    index = 1;
    size = sizeof(code);
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &code, &size, &index);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND,
                       "[7] HttpQueryInfoA failed: %x(%d)\n", res, GetLastError());

    code = 0xdeadbeef;
    size = sizeof(code);
    res = HttpQueryInfoA(req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_REQUEST_HEADERS, &code, &size, NULL);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_HTTP_INVALID_QUERY_REQUEST,
                       "[8] HttpQueryInfoA failed: %x(%d)\n", res, GetLastError());
}

#define test_request_flags(a,b) _test_request_flags(__LINE__,a,b,FALSE)
#define test_request_flags_todo(a,b) _test_request_flags(__LINE__,a,b,TRUE)
static void _test_request_flags(unsigned line, HINTERNET req, DWORD exflags, BOOL is_todo)
{
    DWORD flags, size;
    BOOL res;

    flags = 0xdeadbeef;
    size = sizeof(flags);
    res = InternetQueryOptionW(req, INTERNET_OPTION_REQUEST_FLAGS, &flags, &size);
    ok_(__FILE__,line)(res, "InternetQueryOptionW(INTERNET_OPTION_REQUEST_FLAGS) failed: %u\n", GetLastError());

    /* FIXME: Remove once we have INTERNET_REQFLAG_CACHE_WRITE_DISABLED implementation */
    flags &= ~INTERNET_REQFLAG_CACHE_WRITE_DISABLED;
    todo_wine_if (is_todo)
        ok_(__FILE__,line)(flags == exflags, "flags = %x, expected %x\n", flags, exflags);
}

#define test_request_url(a,b) _test_request_url(__LINE__,a,b)
static void _test_request_url(unsigned line, HINTERNET req, const char *expected_url)
{
    char buf[INTERNET_MAX_URL_LENGTH];
    DWORD size = sizeof(buf);
    BOOL res;

    res = InternetQueryOptionA(req, INTERNET_OPTION_URL, buf, &size);
    ok_(__FILE__,line)(res, "InternetQueryOptionA(INTERNET_OPTION_URL) failed: %u\n", GetLastError());
    ok_(__FILE__,line)(size == strlen(expected_url), "size = %u\n", size);
    ok_(__FILE__,line)(!strcmp(buf, expected_url), "unexpected URL %s, expected %s\n", buf, expected_url);
}

#define test_http_version(a) _test_http_version(__LINE__,a)
static void _test_http_version(unsigned line, HINTERNET req)
{
    HTTP_VERSION_INFO v = {0xdeadbeef, 0xdeadbeef};
    DWORD size;
    BOOL res;

    size = sizeof(v);
    res = InternetQueryOptionW(req, INTERNET_OPTION_HTTP_VERSION, &v, &size);
    ok_(__FILE__,line)(res, "InternetQueryOptionW(INTERNET_OPTION_HTTP_VERSION) failed: %u\n", GetLastError());
    ok_(__FILE__,line)(v.dwMajorVersion == 1, "dwMajorVersion = %d\n", v.dwMajorVersion);
    ok_(__FILE__,line)(v.dwMinorVersion == 1, "dwMinorVersion = %d\n", v.dwMinorVersion);
}

static int close_handle_cnt;

static VOID WINAPI callback(
     HINTERNET hInternet,
     DWORD_PTR dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength
)
{
    ros_skip_flaky
    CHECK_EXPECT(dwInternetStatus);
    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_RESOLVING_NAME:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RESOLVING_NAME \"%s\" %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_NAME_RESOLVED:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_NAME_RESOLVED \"%s\" %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTING_TO_SERVER \"%s\" %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            ok(dwStatusInformationLength == strlen(lpvStatusInformation)+1, "unexpected size %u\n",
               dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTED_TO_SERVER \"%s\" %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            ok(dwStatusInformationLength == strlen(lpvStatusInformation)+1, "unexpected size %u\n",
               dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_SENDING_REQUEST:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_SENDING_REQUEST %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_REQUEST_SENT:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %d\n",
                dwStatusInformationLength);
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REQUEST_SENT 0x%x %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RECEIVING_RESPONSE:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RECEIVING_RESPONSE %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RESPONSE_RECEIVED:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %d\n",
                dwStatusInformationLength);
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RESPONSE_RECEIVED 0x%x %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CTL_RESPONSE_RECEIVED %p %d\n",
                      GetCurrentThreadId(), hInternet,dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_PREFETCH:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_PREFETCH %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CLOSING_CONNECTION:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CLOSING_CONNECTION %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CONNECTION_CLOSED:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTION_CLOSED %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_HANDLE_CREATED:
            ok(dwStatusInformationLength == sizeof(HINTERNET),
                "info length should be sizeof(HINTERNET) instead of %d\n",
                dwStatusInformationLength);
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_HANDLE_CREATED %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      *(HINTERNET *)lpvStatusInformation,dwStatusInformationLength);
            CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
            SET_EXPECT(INTERNET_STATUS_DETECTING_PROXY);
            break;
        case INTERNET_STATUS_HANDLE_CLOSING:
            ok(dwStatusInformationLength == sizeof(HINTERNET),
                "info length should be sizeof(HINTERNET) instead of %d\n",
                dwStatusInformationLength);
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_HANDLE_CLOSING %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      *(HINTERNET *)lpvStatusInformation, dwStatusInformationLength);
            if(!InterlockedDecrement(&close_handle_cnt))
                SetEvent(complete_event);
            break;
        case INTERNET_STATUS_REQUEST_COMPLETE:
        {
            INTERNET_ASYNC_RESULT *iar = (INTERNET_ASYNC_RESULT *)lpvStatusInformation;
            ok(dwStatusInformationLength == sizeof(INTERNET_ASYNC_RESULT),
                "info length should be sizeof(INTERNET_ASYNC_RESULT) instead of %d\n",
                dwStatusInformationLength);
            ok(iar->dwResult == 1 || iar->dwResult == 0, "iar->dwResult = %ld\n", iar->dwResult);
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REQUEST_COMPLETE {%ld,%d} %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      iar->dwResult,iar->dwError,dwStatusInformationLength);
            req_error = iar->dwError;
            if(!close_handle_cnt)
                SetEvent(complete_event);
            break;
        }
        case INTERNET_STATUS_REDIRECT:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REDIRECT \"%s\" %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      (LPCSTR)lpvStatusInformation, dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
            SET_EXPECT(INTERNET_STATUS_DETECTING_PROXY);
            break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx INTERNET_STATUS_INTERMEDIATE_RESPONSE %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext,
                      lpvStatusInformation, dwStatusInformationLength);
            break;
        default:
            if(winetest_debug > 1)
                trace("%04x:Callback %p 0x%lx %d %p %d\n",
                      GetCurrentThreadId(), hInternet, dwContext, dwInternetStatus,
                      lpvStatusInformation, dwStatusInformationLength);
    }
}

typedef struct {
    HINTERNET session;
    HINTERNET connection;
    HINTERNET request;
} test_request_t;

#define open_simple_request(a,b,c,d,e) _open_simple_request(__LINE__,a,b,c,d,e)
static void _open_simple_request(unsigned line, test_request_t *req, const char *host,
        int port, const char *verb, const char *url)
{
    req->session = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok_(__FILE__,line)(req->session != NULL, "InternetOpenA failed: %u\n", GetLastError());

    req->connection = InternetConnectA(req->session, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok_(__FILE__,line)(req->connection != NULL, "InternetConnectA failed: %u\n", GetLastError());

    req->request = HttpOpenRequestA(req->connection, verb, url, NULL, NULL, NULL, 0, 0);
    ok_(__FILE__,line)(req->request != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
}

#define close_request(a) _close_request(__LINE__,a)
static void _close_request(unsigned line, test_request_t *req)
{
    BOOL ret;

    ret = InternetCloseHandle(req->request);
    ok_(__FILE__,line)(ret, "InternetCloseHandle(request) failed: %u\n", GetLastError());
    ret = InternetCloseHandle(req->connection);
    ok_(__FILE__,line)(ret, "InternetCloseHandle(connection) failed: %u\n", GetLastError());
    ret = InternetCloseHandle(req->session);
    ok_(__FILE__,line)(ret, "InternetCloseHandle(session) failed: %u\n", GetLastError());
}

#define receive_simple_request(a,b,c) _receive_simple_request(__LINE__,a,b,c)
static DWORD _receive_simple_request(unsigned line, HINTERNET req, char *buf, size_t buf_size)
{
    DWORD read = 0;
    BOOL ret;

    ret = InternetReadFile(req, buf, buf_size, &read);
    ok_(__FILE__,line)(ret, "InternetReadFile failed: %u\n", GetLastError());

    return read;
}

static void close_async_handle(HINTERNET handle, int handle_cnt)
{
    BOOL res;

    close_handle_cnt = handle_cnt;

    SET_EXPECT2(INTERNET_STATUS_HANDLE_CLOSING, handle_cnt);
    res = InternetCloseHandle(handle);
    ok(res, "InternetCloseHandle failed: %u\n", GetLastError());
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_NOTIFIED2(INTERNET_STATUS_HANDLE_CLOSING, handle_cnt);
}

static void InternetReadFile_test(int flags, const test_data_t *test)
{
    char *post_data = NULL;
    BOOL res, on_async = TRUE;
    CHAR buffer[4000];
    WCHAR wbuffer[4000];
    DWORD length, length2, index, exlen = 0, post_len = 0;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;

    trace("Starting InternetReadFile test with flags 0x%x on url %s\n",flags,test->url);
    reset_events();

    trace("InternetOpenA <--\n");
    hi = InternetOpenA((test->flags & TESTF_COMPRESSED) ? "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)" : "",
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, flags);
    ok((hi != 0x0),"InternetOpen failed with error %u\n", GetLastError());
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    pInternetSetStatusCallbackA(hi,&callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi, test->host, INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %u\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, test->post_data ? "POST" : "GET", test->path, NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RELOAD,
                           0xdeadbead);
    if (hor == 0x0 && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest failed with error %u\n", GetLastError());
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    test_request_flags(hor, INTERNET_REQFLAG_NO_HEADERS);
    test_request_url(hor, test->url);

    length = sizeof(buffer);
    res = HttpQueryInfoA(hor, HTTP_QUERY_RAW_HEADERS, buffer, &length, 0x0);
    ok(res, "HttpQueryInfoA(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(length == 0 || (length == 1 && !*buffer) /* win10 */, "HTTP_QUERY_RAW_HEADERS: expected length 0, but got %d\n", length);
    ok(!strcmp(buffer, ""), "HTTP_QUERY_RAW_HEADERS: expected string \"\", but got \"%s\"\n", buffer);

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    SET_OPTIONAL2(INTERNET_STATUS_COOKIE_SENT,2);
    SET_OPTIONAL2(INTERNET_STATUS_COOKIE_RECEIVED,2);
    if (first_connection_to_test_url)
    {
        SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
        SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
    }
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT2(INTERNET_STATUS_SENDING_REQUEST, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    SET_EXPECT2(INTERNET_STATUS_REQUEST_SENT, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    SET_EXPECT2(INTERNET_STATUS_RECEIVING_RESPONSE, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    SET_EXPECT2(INTERNET_STATUS_RESPONSE_RECEIVED, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    if(test->flags & TESTF_REDIRECT) {
        SET_OPTIONAL2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
        SET_OPTIONAL2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
    }
    SET_EXPECT(INTERNET_STATUS_REDIRECT);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER);
    if (flags & INTERNET_FLAG_ASYNC)
        SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    if(test->flags & TESTF_COMPRESSED) {
        BOOL b = TRUE;

        res = InternetSetOptionA(hor, INTERNET_OPTION_HTTP_DECODING, &b, sizeof(b));
        ok(res || broken(!res && GetLastError() == ERROR_INTERNET_INVALID_OPTION),
           "InternetSetOption failed: %u\n", GetLastError());
        if(!res)
            goto abort;
    }

    test_status_code(hor, 0);

    trace("HttpSendRequestA -->\n");
    if(test->post_data) {
        post_len = strlen(test->post_data);
        post_data = HeapAlloc(GetProcessHeap(), 0, post_len);
        memcpy(post_data, test->post_data, post_len);
    }
    SetLastError(0xdeadbeef);
    res = HttpSendRequestA(hor, test->headers, -1, post_data, post_len);
    if (flags & INTERNET_FLAG_ASYNC)
        ok(!res && (GetLastError() == ERROR_IO_PENDING),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    else
        ok(res || (GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED),
           "Synchronous HttpSendRequest returning 0, error %u\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    if (flags & INTERNET_FLAG_ASYNC) {
        WaitForSingleObject(complete_event, INFINITE);
        ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);
    }
    HeapFree(GetProcessHeap(), 0, post_data);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_RECEIVED);
    if (first_connection_to_test_url)
    {
        if (! proxy_active())
        {
            CHECK_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
            CHECK_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
        }
        else
        {
            CLEAR_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
            CLEAR_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
        }
    }
    else
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    CHECK_NOTIFIED2(INTERNET_STATUS_SENDING_REQUEST, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    CHECK_NOTIFIED2(INTERNET_STATUS_REQUEST_SENT, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    CHECK_NOTIFIED2(INTERNET_STATUS_RECEIVING_RESPONSE, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    CHECK_NOTIFIED2(INTERNET_STATUS_RESPONSE_RECEIVED, (test->flags & TESTF_REDIRECT) ? 2 : 1);
    if(test->flags & TESTF_REDIRECT)
        CHECK_NOTIFIED(INTERNET_STATUS_REDIRECT);
    if (flags & INTERNET_FLAG_ASYNC)
        CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    /* Sent on WinXP only if first_connection_to_test_url is TRUE, on Win98 always sent */
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);

    test_request_flags(hor, 0);

    length = 100;
    res = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    ok(res, "InternetQueryOptionA(INTERNET_OPTION_URL) failed with error %d\n", GetLastError());

    length = sizeof(buffer)-2;
    memset(buffer, 0x77, sizeof(buffer));
    SetLastError(0xdeadbeef);
    res = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length,0x0);
    ok(res, "HttpQueryInfoA(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(GetLastError() == 0 ||
                broken(GetLastError() == 0xdeadbeef /* XP/W2K3 */), "Last Error not reset %u\n", GetLastError());
    /* show that the function writes data past the length returned */
    ok(buffer[length-2], "Expected any header character, got 0x00\n");
    ok(!buffer[length-1], "Expected 0x00, got %02X\n", buffer[length-1]);
    ok(!buffer[length], "Expected 0x00, got %02X\n", buffer[length]);
    ok(buffer[length+1] == 0x77, "Expected 0x77, got %02X\n", buffer[length+1]);

    length2 = length;
    res = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length2,0x0);
    ok(!res, "Expected 0x00, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(length2 == length+1, "Expected %d, got %d\n", length+1, length2);
    /* the in length of the buffer must be +1 but the length returned does not count this */
    length2 = length+1;
    memset(buffer, 0x77, sizeof(buffer));
    res = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length2,0x0);
    ok(res, "HttpQueryInfoA(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(buffer[length2] == 0x00, "Expected 0x00, got %02X\n", buffer[length2]);
    ok(buffer[length2+1] == 0x77, "Expected 0x77, got %02X\n", buffer[length2+1]);
    ok(length2 == length, "Value should not have changed: %d != %d\n", length2, length);

    length = sizeof(wbuffer)-2*sizeof(WCHAR);
    memset(wbuffer, 0x77, sizeof(wbuffer));
    res = HttpQueryInfoW(hor, HTTP_QUERY_RAW_HEADERS, wbuffer, &length, 0x0);
    ok(res, "HttpQueryInfoW(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(length % sizeof(WCHAR) == 0, "Expected that length is a multiple of sizeof(WCHAR), got %d.\n", length);
    length /= sizeof(WCHAR);
    /* show that the function writes data past the length returned */
    ok(wbuffer[length-2], "Expected any header character, got 0x0000\n");
    ok(!wbuffer[length-1], "Expected 0x0000, got %04X\n", wbuffer[length-1]);
    ok(!wbuffer[length], "Expected 0x0000, got %04X\n", wbuffer[length]);
    ok(wbuffer[length+1] == 0x7777 || broken(wbuffer[length+1] != 0x7777),
       "Expected 0x7777, got %04X\n", wbuffer[length+1]);

    length2 = length*sizeof(WCHAR);
    res = HttpQueryInfoW(hor,HTTP_QUERY_RAW_HEADERS,wbuffer,&length2,0x0);
    ok(!res, "Expected 0x00, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(length2 % sizeof(WCHAR) == 0, "Expected that length is a multiple of sizeof(WCHAR), got %d.\n", length2);
    length2 /= sizeof(WCHAR);
    ok(length2 == length+1, "Expected %d, got %d\n", length+1, length2);
    /* the in length of the buffer must be +1 but the length returned does not count this */
    length2 = (length+1)*sizeof(WCHAR);
    memset(wbuffer, 0x77, sizeof(wbuffer));
    res = HttpQueryInfoW(hor,HTTP_QUERY_RAW_HEADERS,wbuffer,&length2,0x0);
    ok(res, "HttpQueryInfoW(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(length2 % sizeof(WCHAR) == 0, "Expected that length is a multiple of sizeof(WCHAR), got %d.\n", length2);
    length2 /= sizeof(WCHAR);
    ok(!wbuffer[length2], "Expected 0x0000, got %04X\n", wbuffer[length2]);
    ok(wbuffer[length2+1] == 0x7777, "Expected 0x7777, got %04X\n", wbuffer[length2+1]);
    ok(length2 == length, "Value should not have changed: %d != %d\n", length2, length);

    test_request_url(hor, test->redirected_url);

    index = 0;
    length = 0;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,NULL,&length,&index) == FALSE,"Query worked\n");
    if(test->flags & TESTF_COMPRESSED)
        ok(GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND,
           "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", GetLastError());
    ok(index == 0, "Index was incremented\n");

    index = 0;
    length = 16;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,&buffer,&length,&index);
    trace("Option HTTP_QUERY_CONTENT_LENGTH -> %i  %s  (%u)\n",res,buffer,GetLastError());
    if(test->flags & TESTF_COMPRESSED)
        ok(!res && GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND,
           "expected ERROR_HTTP_HEADER_NOT_FOUND, got %x (%u)\n", res, GetLastError());
    ok(!res || index == 1, "Index was not incremented although result is %x (index = %u)\n", res, index);

    length = 100;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_TYPE,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option HTTP_QUERY_CONTENT_TYPE -> %i  %s\n",res,buffer);

    length = 100;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_ENCODING,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option HTTP_QUERY_CONTENT_ENCODING -> %i  %s\n",res,buffer);

    SetLastError(0xdeadbeef);
    res = InternetReadFile(NULL, buffer, 100, &length);
    ok(!res, "InternetReadFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "InternetReadFile should have set last error to ERROR_INVALID_HANDLE instead of %u\n",
        GetLastError());

    length = 100;
    if(winetest_debug > 1)
        trace("Entering Query loop\n");

    while (TRUE)
    {
        if (flags & INTERNET_FLAG_ASYNC)
            SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

        /* IE11 calls those in InternetQueryDataAvailable call. */
        SET_OPTIONAL(INTERNET_STATUS_RECEIVING_RESPONSE);
        SET_OPTIONAL(INTERNET_STATUS_RESPONSE_RECEIVED);

        length = 0;
        res = InternetQueryDataAvailable(hor,&length,0x0,0x0);

        CLEAR_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);

        if (flags & INTERNET_FLAG_ASYNC)
        {
            if (res)
            {
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
                if(exlen) {
                    ok(length >= exlen, "length %u < exlen %u\n", length, exlen);
                    exlen = 0;
                }
            }
            else if (GetLastError() == ERROR_IO_PENDING)
            {
                if(winetest_debug > 1)
                    trace("pending\n");
                /* on some tests, InternetQueryDataAvailable returns non-zero length and ERROR_IO_PENDING */
                if(!(test->flags & TESTF_CHUNKED))
                    ok(!length, "InternetQueryDataAvailable returned ERROR_IO_PENDING and %u length\n", length);
                WaitForSingleObject(complete_event, INFINITE);
                exlen = length;
                ok(exlen, "length = 0\n");
                CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
                CLEAR_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
                ok(req_error, "req_error = 0\n");
                continue;
            }else {
                ok(0, "InternetQueryDataAvailable failed: %u\n", GetLastError());
            }
        }else {
            ok(res, "InternetQueryDataAvailable failed: %u\n", GetLastError());
        }
        CLEAR_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);

        if(winetest_debug > 1)
            trace("length %u\n", length);
        if(test->flags & TESTF_CHUNKED)
            ok(length <= 8192, "length = %d, expected <= 8192\n", length);
        if (length)
        {
            char *buffer;
            buffer = HeapAlloc(GetProcessHeap(),0,length+1);

            res = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            if(winetest_debug > 1)
                trace("ReadFile -> %s %i\n", res ? "TRUE" : "FALSE", length);

            if(test->content)
                ok(!strcmp(buffer, test->content), "buffer = '%s', expected '%s'\n", buffer, test->content);
            HeapFree(GetProcessHeap(),0,buffer);
        }else {
            ok(!on_async, "Returned zero size in response to request complete\n");
            break;
        }
        on_async = FALSE;
    }
    if(test->flags & TESTF_REDIRECT) {
        CHECK_NOTIFIED2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
        CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
    }
abort:
    if(winetest_debug > 1)
        trace("aborting\n");
    close_async_handle(hi, 2);
    first_connection_to_test_url = FALSE;
}

static void InternetReadFile_chunked_test(void)
{
    BOOL res;
    CHAR buffer[4000];
    DWORD length, got;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;

    trace("Starting InternetReadFile chunked test\n");

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok((hi != 0x0),"InternetOpen failed with error %u\n", GetLastError());
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi, "test.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %u\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/tests/chunked", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RELOAD,
                           0xdeadbead);
    if (hor == 0x0 && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest failed with error %u\n", GetLastError());
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    trace("HttpSendRequestA -->\n");
    SetLastError(0xdeadbeef);
    res = HttpSendRequestA(hor, "", -1, NULL, 0);
    ok(res || (GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED),
       "Synchronous HttpSendRequest returning 0, error %u\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    test_request_flags(hor, 0);

    length = 100;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_TYPE,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option CONTENT_TYPE -> %i  %s\n",res,buffer);

    SetLastError( 0xdeadbeef );
    length = 100;
    res = HttpQueryInfoA(hor,HTTP_QUERY_TRANSFER_ENCODING,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option TRANSFER_ENCODING -> %i  %s\n",res,buffer);
    ok( res || ( proxy_active() && GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND ),
        "Failed to get TRANSFER_ENCODING option, error %u\n", GetLastError() );
    ok( !strcmp( buffer, "chunked" ) || ( ! res && proxy_active() && GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND ),
        "Wrong transfer encoding '%s'\n", buffer );

    SetLastError( 0xdeadbeef );
    length = 16;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,&buffer,&length,0x0);
    ok( !res, "Found CONTENT_LENGTH option '%s'\n", buffer );
    ok( GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "Wrong error %u\n", GetLastError() );

    length = 100;
    trace("Entering Query loop\n");

    while (TRUE)
    {
        res = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok(!(!res && length != 0),"InternetQueryDataAvailable failed with non-zero length\n");
        ok(res, "InternetQueryDataAvailable failed, error %d\n", GetLastError());
        trace("got %u available\n",length);
        if (length)
        {
            char *buffer = HeapAlloc(GetProcessHeap(),0,length+1);

            SetLastError(0xdeadbeef);
            res = InternetReadFile(hor,buffer,length,&got);
            ok(GetLastError() == 0 ||
                broken(GetLastError() == 0xdeadbeef /* XP/W2K3 */), "Last Error not reset %u\n", GetLastError());

            buffer[got]=0;
            trace("ReadFile -> %i %i\n",res,got);
            ok( length == got, "only got %u of %u available\n", got, length );
            ok( buffer[got-1] == '\n', "received partial line '%s'\n", buffer );

            HeapFree(GetProcessHeap(),0,buffer);
            if (!got) break;
        }
        if (length == 0)
        {
            got = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            res = InternetReadFile( hor, buffer, 1, &got );
            ok( res, "InternetReadFile failed: %u\n", GetLastError() );
            ok(GetLastError() == 0 ||
                broken(GetLastError() == 0xdeadbeef /* XP/W2K3 */), "Last Error not reset %u\n", GetLastError());
            ok( !got, "got %u\n", got );
            break;
        }
    }
abort:
    trace("aborting\n");
    if (hor != 0x0) {
        res = InternetCloseHandle(hor);
        ok (res, "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
    }
    if (hi != 0x0) {
        res = InternetCloseHandle(hi);
        ok (res, "InternetCloseHandle of handle opened by InternetOpenA failed\n");
    }
}

static void InternetReadFileExA_test(int flags)
{
    DWORD rc;
    DWORD length;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;
    INTERNET_BUFFERSA inetbuffers;

    trace("Starting InternetReadFileExA test with flags 0x%x\n",flags);
    reset_events();

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, flags);
    ok((hi != 0x0),"InternetOpen failed with error %u\n", GetLastError());
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    pInternetSetStatusCallbackA(hi,&callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi, "test.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %u\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/tests/redirect", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RELOAD,
                           0xdeadbead);
    if (hor == 0x0 && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest failed with error %u\n", GetLastError());
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    if (first_connection_to_test_url)
    {
        SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
        SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
    }
    SET_OPTIONAL2(INTERNET_STATUS_COOKIE_SENT, 2);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT2(INTERNET_STATUS_SENDING_REQUEST, 2);
    SET_EXPECT2(INTERNET_STATUS_REQUEST_SENT, 2);
    SET_EXPECT2(INTERNET_STATUS_RECEIVING_RESPONSE, 2);
    SET_EXPECT2(INTERNET_STATUS_RESPONSE_RECEIVED, 2);
    SET_OPTIONAL2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
    SET_OPTIONAL2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
    SET_EXPECT(INTERNET_STATUS_REDIRECT);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER);
    if (flags & INTERNET_FLAG_ASYNC)
        SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    else
        SET_WINE_ALLOW(INTERNET_STATUS_REQUEST_COMPLETE);

    trace("HttpSendRequestA -->\n");
    SetLastError(0xdeadbeef);
    rc = HttpSendRequestA(hor, "", -1, NULL, 0);
    if (flags & INTERNET_FLAG_ASYNC)
        ok(((rc == 0)&&(GetLastError() == ERROR_IO_PENDING)),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    else
        ok((rc != 0) || GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED,
           "Synchronous HttpSendRequest returning 0, error %u\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    if (!rc && (GetLastError() == ERROR_IO_PENDING)) {
        WaitForSingleObject(complete_event, INFINITE);
        ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);
    }

    if (first_connection_to_test_url)
    {
        CHECK_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    else
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    CHECK_NOTIFIED2(INTERNET_STATUS_SENDING_REQUEST, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_REQUEST_SENT, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_RECEIVING_RESPONSE, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_RESPONSE_RECEIVED, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_REDIRECT);
    if (flags & INTERNET_FLAG_ASYNC)
        CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    else
        todo_wine CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    /* Sent on WinXP only if first_connection_to_test_url is TRUE, on Win98 always sent */
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);

    if(is_ie7plus) {
        rc = InternetReadFileExW(hor, NULL, 0, 0xdeadcafe);
        ok(!rc && (GetLastError() == ERROR_INVALID_PARAMETER),
           "InternetReadFileEx should have failed with ERROR_INVALID_PARAMETER instead of %s, %u\n",
           rc ? "TRUE" : "FALSE", GetLastError());
    }

    /* tests invalid dwStructSize */
    inetbuffers.dwStructSize = sizeof(inetbuffers)+1;
    inetbuffers.lpcszHeader = NULL;
    inetbuffers.dwHeadersLength = 0;
    inetbuffers.dwBufferLength = 10;
    inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, 10);
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    rc = InternetReadFileExA(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_PARAMETER),
        "InternetReadFileEx should have failed with ERROR_INVALID_PARAMETER instead of %s, %u\n",
        rc ? "TRUE" : "FALSE", GetLastError());
    HeapFree(GetProcessHeap(), 0, inetbuffers.lpvBuffer);

    test_request_flags(hor, 0);

    /* tests to see whether lpcszHeader is used - it isn't */
    inetbuffers.dwStructSize = sizeof(inetbuffers);
    inetbuffers.lpcszHeader = (LPCSTR)0xdeadbeef;
    inetbuffers.dwHeadersLength = 255;
    inetbuffers.dwBufferLength = 0;
    inetbuffers.lpvBuffer = NULL;
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    rc = InternetReadFileExA(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(rc, "InternetReadFileEx failed with error %u\n", GetLastError());
    trace("read %i bytes\n", inetbuffers.dwBufferLength);

    rc = InternetReadFileExA(NULL, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_HANDLE),
        "InternetReadFileEx should have failed with ERROR_INVALID_HANDLE instead of %s, %u\n",
        rc ? "TRUE" : "FALSE", GetLastError());

    length = 0;
    trace("Entering Query loop\n");

    while (TRUE)
    {
        inetbuffers.dwStructSize = sizeof(inetbuffers);
        inetbuffers.dwBufferLength = 1024;
        inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, inetbuffers.dwBufferLength+1);
        inetbuffers.dwOffsetHigh = 1234;
        inetbuffers.dwOffsetLow = 5678;

        SET_WINE_ALLOW(INTERNET_STATUS_RECEIVING_RESPONSE);
        SET_WINE_ALLOW(INTERNET_STATUS_RESPONSE_RECEIVED);
        SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
        rc = InternetReadFileExA(hor, &inetbuffers, IRF_ASYNC | IRF_USE_CONTEXT, 0xcafebabe);
        if (!rc)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                trace("InternetReadFileEx -> PENDING\n");
                ok(flags & INTERNET_FLAG_ASYNC,
                   "Should not get ERROR_IO_PENDING without INTERNET_FLAG_ASYNC\n");
                CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
                WaitForSingleObject(complete_event, INFINITE);
                CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
                CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
                ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);
            }
            else
            {
                trace("InternetReadFileEx -> FAILED %u\n", GetLastError());
                break;
            }
        }
        else
        {
            trace("InternetReadFileEx -> SUCCEEDED\n");
            CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
            if (inetbuffers.dwBufferLength)
            {
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
            }
            else
            {
                /* Win98 still sends these when 0 bytes are read, WinXP does not */
                CLEAR_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
                CLEAR_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
            }
        }

        trace("read %i bytes\n", inetbuffers.dwBufferLength);
        ((char *)inetbuffers.lpvBuffer)[inetbuffers.dwBufferLength] = '\0';

        ok(inetbuffers.dwOffsetHigh == 1234 && inetbuffers.dwOffsetLow == 5678,
            "InternetReadFileEx sets offsets to 0x%x%08x\n",
            inetbuffers.dwOffsetHigh, inetbuffers.dwOffsetLow);

        HeapFree(GetProcessHeap(), 0, inetbuffers.lpvBuffer);

        if (!inetbuffers.dwBufferLength)
            break;

        length += inetbuffers.dwBufferLength;
    }
    ok(length > 0, "failed to read any of the document\n");
    trace("Finished. Read %d bytes\n", length);

abort:
    close_async_handle(hi, 2);
    first_connection_to_test_url = FALSE;
}

static void InternetOpenUrlA_test(void)
{
  HINTERNET myhinternet, myhttp;
  char buffer[0x400];
  DWORD size, readbytes, totalbytes=0;
  BOOL ret;

  ret = DeleteUrlCacheEntryA(TEST_URL);
  ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND,
          "DeleteUrlCacheEntry returned %x, GetLastError() = %d\n", ret, GetLastError());

  myhinternet = InternetOpenA("Winetest",0,NULL,NULL,INTERNET_FLAG_NO_CACHE_WRITE);
  ok((myhinternet != 0), "InternetOpen failed, error %u\n",GetLastError());
  size = 0x400;
  ret = InternetCanonicalizeUrlA(TEST_URL, buffer, &size,ICU_BROWSER_MODE);
  ok( ret, "InternetCanonicalizeUrl failed, error %u\n",GetLastError());

  SetLastError(0);
  myhttp = InternetOpenUrlA(myhinternet, TEST_URL, 0, 0,
			   INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_TRANSFER_BINARY,0);
  if (GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    return; /* WinXP returns this when not connected to the net */
  ok((myhttp != 0),"InternetOpenUrl failed, error %u\n",GetLastError());
  ret = InternetReadFile(myhttp, buffer,0x400,&readbytes);
  ok( ret, "InternetReadFile failed, error %u\n",GetLastError());
  totalbytes += readbytes;
  while (readbytes && InternetReadFile(myhttp, buffer,0x400,&readbytes))
    totalbytes += readbytes;
  trace("read 0x%08x bytes\n",totalbytes);

  InternetCloseHandle(myhttp);
  InternetCloseHandle(myhinternet);

  ret = DeleteUrlCacheEntryA(TEST_URL);
  ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "INTERNET_FLAG_NO_CACHE_WRITE flag doesn't work\n");
}

static void HttpSendRequestEx_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;

    INTERNET_BUFFERSA BufferIn;
    DWORD dwBytesWritten, dwBytesRead, error;
    CHAR szBuffer[256];
    int i;
    BOOL ret;

    static char szPostData[] = "mode=Test";
    static const char szContentType[] = "Content-Type: application/x-www-form-urlencoded";

    hSession = InternetOpenA("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnectA(hSession, "test.winehq.org",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://test.winehq.org\n");
    hRequest = HttpOpenRequestA(hConnect, "POST", "/tests/post.php",
            NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        skip( "Network unreachable, skipping test\n" );
        goto done;
    }
    ok( hRequest != NULL, "Failed to open request handle err %u\n", GetLastError());

    test_request_flags(hRequest, INTERNET_REQFLAG_NO_HEADERS);

    BufferIn.dwStructSize = sizeof(BufferIn);
    BufferIn.Next = (INTERNET_BUFFERSA*)0xdeadcab;
    BufferIn.lpcszHeader = szContentType;
    BufferIn.dwHeadersLength = sizeof(szContentType)-1;
    BufferIn.dwHeadersTotal = sizeof(szContentType)-1;
    BufferIn.lpvBuffer = szPostData;
    BufferIn.dwBufferLength = 3;
    BufferIn.dwBufferTotal = sizeof(szPostData)-1;
    BufferIn.dwOffsetLow = 0;
    BufferIn.dwOffsetHigh = 0;

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestExA(hRequest, &BufferIn, NULL, 0 ,0);
    error = GetLastError();
    ok(ret, "HttpSendRequestEx Failed with error %u\n", error);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", error);

    test_request_flags(hRequest, INTERNET_REQFLAG_NO_HEADERS);

    for (i = 3; szPostData[i]; i++)
        ok(InternetWriteFile(hRequest, &szPostData[i], 1, &dwBytesWritten),
                "InternetWriteFile failed\n");

    test_request_flags(hRequest, INTERNET_REQFLAG_NO_HEADERS);

    ok(HttpEndRequestA(hRequest, NULL, 0, 0), "HttpEndRequest Failed\n");

    test_request_flags(hRequest, 0);

    ok(InternetReadFile(hRequest, szBuffer, 255, &dwBytesRead),
            "Unable to read response\n");
    szBuffer[dwBytesRead] = 0;

    ok(dwBytesRead == 13,"Read %u bytes instead of 13\n",dwBytesRead);
    ok(strncmp(szBuffer,"mode => Test\n",dwBytesRead)==0 || broken(proxy_active()),"Got string %s\n",szBuffer);

    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
done:
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}

static void InternetOpenRequest_test(void)
{
    HINTERNET session, connect, request;
    static const char *types[] = { "*", "", NULL };
    static const WCHAR slash[] = {'/', 0}, any[] = {'*', 0}, empty[] = {0};
    static const WCHAR *typesW[] = { any, empty, NULL };
    BOOL ret;

    session = InternetOpenA("Wine Regression Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(session != NULL ,"Unable to open Internet session\n");

    connect = InternetConnectA(session, NULL, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect == NULL, "InternetConnectA should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "InternetConnectA with NULL server named should have failed with ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    connect = InternetConnectA(session, "", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect == NULL, "InternetConnectA should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "InternetConnectA with blank server named should have failed with ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "Unable to connect to http://test.winehq.org with error %d\n", GetLastError());

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, types, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!request && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        skip( "Network unreachable, skipping test\n" );
        goto done;
    }
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    ret = HttpSendRequestW(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    ok(InternetCloseHandle(request), "Close request handle failed\n");

    request = HttpOpenRequestW(connect, NULL, slash, NULL, NULL, typesW, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    ok(InternetCloseHandle(request), "Close request handle failed\n");

done:
    ok(InternetCloseHandle(connect), "Close connect handle failed\n");
    ok(InternetCloseHandle(session), "Close session handle failed\n");
}

static void test_cache_read(void)
{
    HINTERNET session, connection, req;
    FILETIME now, tomorrow, yesterday;
    BYTE content[1000], buf[2000];
    char file_path[MAX_PATH];
    ULARGE_INTEGER li;
    HANDLE file;
    DWORD size;
    unsigned i;
    BOOL res;

    static const char cache_only_url[] = "http://test.winehq.org/tests/cache-only";
    BYTE cache_headers[] = "HTTP/1.1 200 OK\r\n\r\n";

    trace("Testing cache read...\n");
    reset_events();

    for(i = 0; i < sizeof(content); i++)
        content[i] = '0' + (i%10);

    GetSystemTimeAsFileTime(&now);
    li.u.HighPart = now.dwHighDateTime;
    li.u.LowPart = now.dwLowDateTime;
    li.QuadPart += (LONGLONG)10000000 * 3600 * 24;
    tomorrow.dwHighDateTime = li.u.HighPart;
    tomorrow.dwLowDateTime = li.u.LowPart;
    li.QuadPart -= (LONGLONG)10000000 * 3600 * 24 * 2;
    yesterday.dwHighDateTime = li.u.HighPart;
    yesterday.dwLowDateTime = li.u.LowPart;

    res = CreateUrlCacheEntryA(cache_only_url, sizeof(content), "", file_path, 0);
    ok(res, "CreateUrlCacheEntryA failed: %u\n", GetLastError());

    file = CreateFileA(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    WriteFile(file, content, sizeof(content), &size, NULL);
    CloseHandle(file);

    res = CommitUrlCacheEntryA(cache_only_url, file_path, tomorrow, yesterday, NORMAL_CACHE_ENTRY,
                               cache_headers, sizeof(cache_headers)-1, "", 0);
    ok(res, "CommitUrlCacheEntryA failed: %u\n", GetLastError());

    session = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(session, callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    connection = InternetConnectA(session, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(connection != NULL,"InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(connection, "GET", "/tests/cache-only", NULL, NULL, NULL, 0, 0xdeadbead);
    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_SENDING_REQUEST);
    SET_WINE_ALLOW(INTERNET_STATUS_REQUEST_SENT);
    SET_WINE_ALLOW(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_WINE_ALLOW(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_WINE_ALLOW(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, -1, NULL, 0);
    todo_wine
    ok(res, "HttpSendRequest failed: %u\n", GetLastError());

    if(res) {
        size = 0;
        res = InternetQueryDataAvailable(req, &size, 0, 0);
        ok(res, "InternetQueryDataAvailable failed: %u\n", GetLastError());
        ok(size  == sizeof(content), "size = %u\n", size);

        size = sizeof(buf);
        res = InternetReadFile(req, buf, sizeof(buf), &size);
        ok(res, "InternetReadFile failed: %u\n", GetLastError());
        ok(size == sizeof(content), "size = %u\n", size);
        ok(!memcmp(content, buf, sizeof(content)), "unexpected content\n");
    }

    close_async_handle(session, 2);

    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CLEAR_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CLEAR_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CLEAR_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    res = DeleteUrlCacheEntryA(cache_only_url);
    ok(res, "DeleteUrlCacheEntryA failed: %u\n", GetLastError());
}

static void test_http_cache(void)
{
    HINTERNET session, connect, request;
    char file_name[MAX_PATH], url[INTERNET_MAX_URL_LENGTH];
    DWORD size, file_size;
    BYTE buf[100];
    HANDLE file;
    BOOL ret;
    FILETIME filetime_zero = {0};

    static const char cached_content[] = "data read from cache";
    static const char *types[] = { "*", "", NULL };

    session = InternetOpenA("Wine Regression Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(session != NULL ,"Unable to open Internet session\n");

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "Unable to connect to http://test.winehq.org with error %d\n", GetLastError());

    request = HttpOpenRequestA(connect, NULL, "/tests/hello.html", NULL, NULL, types, INTERNET_FLAG_NEED_FILE, 0);
    if (!request && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        skip( "Network unreachable, skipping test\n" );

        ok(InternetCloseHandle(connect), "Close connect handle failed\n");
        ok(InternetCloseHandle(session), "Close session handle failed\n");

        return;
    }
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    size = sizeof(url);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_URL, url, &size);
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_URL) failed: %u\n", GetLastError());
    ok(!strcmp(url, "http://test.winehq.org/tests/hello.html"), "Wrong URL %s\n", url);

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(!ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_ITEM_NOT_FOUND, "GetLastError()=%u\n", GetLastError());
    ok(!size, "size = %d\n", size);

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) failed: %u\n", GetLastError());

    file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    file_size = GetFileSize(file, NULL);
    ok(file_size == 106, "file size = %u\n", file_size);

    size = sizeof(buf);
    ret = InternetReadFile(request, buf, sizeof(buf), &size);
    ok(ret, "InternetReadFile failed: %u\n", GetLastError());
    ok(size == 100, "size = %u\n", size);

    file_size = GetFileSize(file, NULL);
    ok(file_size == 106, "file size = %u\n", file_size);
    CloseHandle(file);

    ret = DeleteFileA(file_name);
    ok(!ret && GetLastError() == ERROR_SHARING_VIOLATION, "Deleting file returned %x(%u)\n", ret, GetLastError());

    ok(InternetCloseHandle(request), "Close request handle failed\n");

    file = CreateFileA(file_name, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    ret = WriteFile(file, cached_content, sizeof(cached_content), &size, NULL);
    ok(ret && size, "WriteFile failed: %d, %d\n", ret, size);
    ret = CommitUrlCacheEntryA(url, file_name, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, NULL, 0);
    ok(ret, "CommitUrlCacheEntry failed: %d\n", GetLastError());
    CloseHandle(file);

    /* Send the same request, requiring it to be retrieved from the cache */
    request = HttpOpenRequestA(connect, "GET", "/tests/hello.html", NULL, NULL, NULL, INTERNET_FLAG_FROM_CACHE, 0);

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    size = sizeof(buf);
    ret = InternetReadFile(request, buf, sizeof(buf), &size);
    ok(ret, "InternetReadFile failed: %u\n", GetLastError());
    ok(size == 100, "size = %u\n", size);
    buf[99] = 0;
    todo_wine ok(!strcmp((char*)buf, cached_content), "incorrect page data: %s\n", (char*)buf);

    ok(InternetCloseHandle(request), "Close request handle failed\n");

    DeleteUrlCacheEntryA(url);
    request = HttpOpenRequestA(connect, "GET", "/tests/hello.html", NULL, NULL, NULL, INTERNET_FLAG_FROM_CACHE, 0);
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    todo_wine ok(!ret, "HttpSendRequest succeeded\n");
    if(!ret)
        ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError() = %d\n", GetLastError());
    ok(InternetCloseHandle(request), "Close request handle failed\n");

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, types, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(!ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_ITEM_NOT_FOUND, "GetLastError()=%u\n", GetLastError());
    ok(!size, "size = %d\n", size);

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    size = sizeof(file_name);
    file_name[0] = 0;
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    if (ret)
    {
        file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
        CloseHandle(file);
    }
    else
    {
        /* < IE8 */
        ok(file_name[0] == 0, "Didn't expect a file name\n");
    }

    ok(InternetCloseHandle(request), "Close request handle failed\n");
    ok(InternetCloseHandle(connect), "Close connect handle failed\n");
    ok(InternetCloseHandle(session), "Close session handle failed\n");

    test_cache_read();
}

static void InternetLockRequestFile_test(void)
{
    char file_name[MAX_PATH];
    test_request_t req;
    HANDLE lock, lock2;
    DWORD size;
    BOOL ret;

    open_simple_request(&req, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, "/tests/hello.html");

    size = sizeof(file_name);
    ret = InternetQueryOptionA(req.request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(!ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_ITEM_NOT_FOUND, "GetLastError()=%u\n", GetLastError());
    ok(!size, "size = %d\n", size);

    lock = NULL;
    ret = InternetLockRequestFile(req.request, &lock);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "InternetLockRequestFile returned: %x(%u)\n", ret, GetLastError());

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(req.request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) failed: %u\n", GetLastError());

    ret = InternetLockRequestFile(req.request, &lock);
    ok(ret, "InternetLockRequestFile returned: %x(%u)\n", ret, GetLastError());
    ok(lock != NULL, "lock == NULL\n");

    ret = InternetLockRequestFile(req.request, &lock2);
    ok(ret, "InternetLockRequestFile returned: %x(%u)\n", ret, GetLastError());
    ok(lock == lock2, "lock != lock2\n");

    ret = InternetUnlockRequestFile(lock2);
    ok(ret, "InternetUnlockRequestFile failed: %u\n", GetLastError());

    ret = DeleteFileA(file_name);
    ok(!ret && GetLastError() == ERROR_SHARING_VIOLATION, "Deleting file returned %x(%u)\n", ret, GetLastError());

    ok(InternetCloseHandle(req.request), "Close request handle failed\n");

    ret = DeleteFileA(file_name);
    ok(!ret && GetLastError() == ERROR_SHARING_VIOLATION, "Deleting file returned %x(%u)\n", ret, GetLastError());

    ret = InternetUnlockRequestFile(lock);
    ok(ret, "InternetUnlockRequestFile failed: %u\n", GetLastError());

    ret = DeleteFileA(file_name);
    ok(ret, "Deleting file returned %x(%u)\n", ret, GetLastError());
}

static void HttpHeaders_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    CHAR      buffer[256];
    WCHAR     wbuffer[256];
    DWORD     len = 256;
    DWORD     oldlen;
    DWORD     index = 0;
    BOOL      ret;

    hSession = InternetOpenA("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnectA(hSession, "test.winehq.org",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://test.winehq.org\n");
    hRequest = HttpOpenRequestA(hConnect, "POST", "/tests/post.php",
            NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        skip( "Network unreachable, skipping test\n" );
        goto done;
    }
    ok( hRequest != NULL, "Failed to open request handle\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
               buffer,&len,&index)==0,"Warning hearder reported as Existing\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test1",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add new header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    ok(len == 5, "Invalid length (exp. 5, got %d)\n", len);
    ok((len < sizeof(buffer)) && (buffer[len] == 0), "Buffer not NULL-terminated\n"); /* len show only 5 characters but the buffer is NULL-terminated*/
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Second Index Should Not Exist\n");

    index = 0;
    len = 5; /* could store the string but not the NULL terminator */
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index) == FALSE,"Query succeeded on a too small buffer\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(index == 0, "Index was incremented\n");
    ok(strcmp(buffer,"Warning")==0, "incorrect string was returned(%s)\n",buffer); /* string not touched */
    ok(len == 6, "Invalid length (exp. 6, got %d)\n", len); /* unlike success, the length includes the NULL-terminator */

    /* a call with NULL will fail but will return the length */
    index = 0;
    len = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");

    /* even for a len that is too small */
    index = 0;
    len = 15;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");

    index = 0;
    len = 0;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");
    oldlen = len;   /* bytes; at least long enough to hold buffer & nul */


    /* a working query */
    index = 0;
    len = sizeof(buffer);
    memset(buffer, 'x', sizeof(buffer));
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(len + sizeof(CHAR) <= oldlen, "Result longer than advertised\n");
    ok((len < sizeof(buffer)-sizeof(CHAR)) && (buffer[len/sizeof(CHAR)] == 0),"No NUL at end\n");
    ok(len == strlen(buffer) * sizeof(CHAR), "Length wrong\n");
    /* what's in the middle differs between Wine and Windows so currently we check only the beginning and the end */
    ok(strncmp(buffer, "POST /tests/post.php HTTP/1", 25)==0, "Invalid beginning of headers string\n");
    ok(strcmp(buffer + strlen(buffer) - 4, "\r\n\r\n")==0, "Invalid end of headers string\n");
    ok(index == 0, "Index was incremented\n");

    /* Like above two tests, but for W version */

    index = 0;
    len = 0;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfoW(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 80, "Invalid length (exp. more than 80, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");
    oldlen = len;   /* bytes; at least long enough to hold buffer & nul */

    /* a working query */
    index = 0;
    len = sizeof(wbuffer);
    memset(wbuffer, 'x', sizeof(wbuffer));
    ok(HttpQueryInfoW(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                wbuffer,&len,&index),"Unable to query header\n");
    ok(len + sizeof(WCHAR) <= oldlen, "Result longer than advertised\n");
    ok(len == lstrlenW(wbuffer) * sizeof(WCHAR), "Length wrong\n");
    ok((len < sizeof(wbuffer)-sizeof(WCHAR)) && (wbuffer[len/sizeof(WCHAR)] == 0),"No NUL at end\n");
    ok(index == 0, "Index was incremented\n");

    /* end of W version tests */

    /* Without HTTP_QUERY_FLAG_REQUEST_HEADERS */
    index = 0;
    len = sizeof(buffer);
    memset(buffer, 'x', sizeof(buffer));
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,
                buffer,&len,&index) == TRUE,"Query failed\n");
    ok(len == 2 || len == 4 /* win10 */, "Expected 2 or 4, got %d\n", len);
    ok(memcmp(buffer, "\r\n\r\n", len) == 0, "Expected CRLF, got '%s'\n", buffer);
    ok(index == 0, "Index was incremented\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test2",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add duplicate header using HTTP_ADDREQ_FLAG_ADD\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test3",-1,HTTP_ADDREQ_FLAG_REPLACE), "Failed to replace header using HTTP_ADDREQ_FLAG_REPLACE\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test4",-1,HTTP_ADDREQ_FLAG_ADD_IF_NEW)==0, "HTTP_ADDREQ_FLAG_ADD_IF_NEW replaced existing header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test4",-1, HTTP_ADDREQ_FLAG_COALESCE), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4")==0, "incorrect string was returned(%s)\n", buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test5",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test6",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5; test6")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeadersA(hRequest,"Warning:test7",-1, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE), "HTTP_ADDREQ_FLAG_ADD with HTTP_ADDREQ_FLAG_REPALCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test7")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    /* Ensure that blank headers are ignored and don't cause a failure */
    ok(HttpAddRequestHeadersA(hRequest,"\r\nBlankTest:value\r\n\r\n",-1, HTTP_ADDREQ_FLAG_ADD_IF_NEW), "Failed to add header with blank entries in list\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"BlankTest");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"value")==0, "incorrect string was returned(%s)\n",buffer);

    /* Ensure that malformed header separators are ignored and don't cause a failure */
    ok(HttpAddRequestHeadersA(hRequest,"\r\rMalformedTest:value\n\nMalformedTestTwo: value2\rMalformedTestThree: value3\n\n\r\r\n",-1, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE),
        "Failed to add header with malformed entries in list\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"MalformedTest");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"value")==0, "incorrect string was returned(%s)\n",buffer);
    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"MalformedTestTwo");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"value2")==0, "incorrect string was returned(%s)\n",buffer);
    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"MalformedTestThree");
    ok(HttpQueryInfoA(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"value3")==0, "incorrect string was returned(%s)\n",buffer);

    ret = HttpAddRequestHeadersA(hRequest, "Authorization: Basic\r\n", -1, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "unable to add header %u\n", GetLastError());

    index = 0;
    buffer[0] = 0;
    len = sizeof(buffer);
    ret = HttpQueryInfoA(hRequest, HTTP_QUERY_AUTHORIZATION|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &len, &index);
    ok(ret, "unable to query header %u\n", GetLastError());
    ok(index == 1, "index was not incremented\n");
    ok(!strcmp(buffer, "Basic"), "incorrect string was returned (%s)\n", buffer);

    ret = HttpAddRequestHeadersA(hRequest, "Authorization:\r\n", -1, HTTP_ADDREQ_FLAG_REPLACE);
    ok(ret, "unable to remove header %u\n", GetLastError());

    index = 0;
    len = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ok(!HttpQueryInfoA(hRequest, HTTP_QUERY_AUTHORIZATION|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &len, &index),
       "header still present\n");
    ok(GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "got %u\n", GetLastError());

    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
done:
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}

static const char garbagemsg[] =
"Garbage: Header\r\n";

static const char contmsg[] =
"HTTP/1.1 100 Continue\r\n";

static const char expandcontmsg[] =
"HTTP/1.1 100 Continue\r\n"
"Server: winecontinue\r\n"
"Tag: something witty\r\n";

static const char okmsg[] =
"HTTP/1.1 200 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char okmsg201[] =
"HTTP/1.1 201 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char okmsg2[] =
"HTTP/1.1 200 OK\r\n"
"Date: Mon, 01 Dec 2008 13:44:34 GMT\r\n"
"Server: winetest\r\n"
"Content-Length: 0\r\n"
"Set-Cookie: one\r\n"
"Set-Cookie: two\r\n"
"\r\n";

static DWORD64 content_length;
static const char largemsg[] =
"HTTP/1.1 200 OK\r\n"
"Content-Length: %I64u\r\n"
"\r\n";

static const char okmsg_cookie_path[] =
"HTTP/1.1 200 OK\r\n"
"Date: Mon, 01 Dec 2008 13:44:34 GMT\r\n"
"Server: winetest\r\n"
"Content-Length: 0\r\n"
"Set-Cookie: subcookie2=data; path=/test_cookie_set_path\r\n"
"\r\n";

static const char okmsg_cookie[] =
"HTTP/1.1 200 OK\r\n"
"Date: Mon, 01 Dec 2008 13:44:34 GMT\r\n"
"Server: winetest\r\n"
"Content-Length: 0\r\n"
"Set-Cookie: testcookie=testvalue\r\n"
"\r\n";

static const char notokmsg[] =
"HTTP/1.1 400 Bad Request\r\n"
"Server: winetest\r\n"
"\r\n";

static const char noauthmsg[] =
"HTTP/1.1 401 Unauthorized\r\n"
"Server: winetest\r\n"
"Connection: close\r\n"
"WWW-Authenticate: Basic realm=\"placebo\"\r\n"
"\r\n";

static const char noauthmsg2[] =
"HTTP/1.0 401 Anonymous requests or requests on unsecure channel are not allowed\r\n"
"HTTP/1.0 401 Anonymous requests or requests on unsecure channel are not allowed"
"\0d`0|6\n"
"Server: winetest\r\n";

static const char proxymsg[] =
"HTTP/1.1 407 Proxy Authentication Required\r\n"
"Server: winetest\r\n"
"Proxy-Connection: close\r\n"
"Proxy-Authenticate: Basic realm=\"placebo\"\r\n"
"\r\n";

static const char page1[] =
"<HTML>\r\n"
"<HEAD><TITLE>wininet test page</TITLE></HEAD>\r\n"
"<BODY>The quick brown fox jumped over the lazy dog<P></BODY>\r\n"
"</HTML>\r\n\r\n";

static const char ok_with_length[] =
"HTTP/1.1 200 OK\r\n"
"Connection: Keep-Alive\r\n"
"Content-Length: 18\r\n\r\n"
"HTTP/1.1 211 OK\r\n\r\n";

static const char ok_with_length2[] =
"HTTP/1.1 210 OK\r\n"
"Connection: Keep-Alive\r\n"
"Content-Length: 19\r\n\r\n"
"HTTP/1.1 211 OK\r\n\r\n";

struct server_info {
    HANDLE hEvent;
    int port;
};

static int test_cache_gzip;
static const char *send_buffer;
static int server_socket;

static DWORD CALLBACK server_thread(LPVOID param)
{
    struct server_info *si = param;
    int r, c = -1, i, on, count = 0;
    SOCKET s;
    struct sockaddr_in sa;
    char *buffer;
    size_t buffer_size;
    WSADATA wsaData;
    int last_request = 0;
    char host_header[22];
    char host_header_override[30];
    static int test_no_cache = 0;

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

    r = bind(s, (struct sockaddr*) &sa, sizeof sa);
    if (r<0)
        return 1;

    listen(s, 0);

    SetEvent(si->hEvent);

    sprintf(host_header, "Host: localhost:%d", si->port);
    sprintf(host_header_override, "Host: test.local:%d\r\n", si->port);
    buffer = HeapAlloc(GetProcessHeap(), 0, buffer_size = 1000);

    do
    {
        if(c == -1)
            c = accept(s, NULL, NULL);

        memset(buffer, 0, buffer_size);
        for(i=0;; i++)
        {
            if(i == buffer_size)
                buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, buffer_size *= 2);

            r = recv(c, buffer+i, 1, 0);
            if (r != 1)
                break;
            if (i<4) continue;
            if (buffer[i-2] == '\n' && buffer[i] == '\n' &&
                buffer[i-3] == '\r' && buffer[i-1] == '\r')
                break;
        }
        if (strstr(buffer, "GET /test1"))
        {
            if (!strstr(buffer, "Content-Length: 0"))
            {
                send(c, okmsg, sizeof okmsg-1, 0);
                send(c, page1, sizeof page1-1, 0);
            }
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "CONNECT "))
        {
            if (!strstr(buffer, "Content-Length: 0"))
                send(c, notokmsg, sizeof notokmsg-1, 0);
            else
                send(c, proxymsg, sizeof proxymsg-1, 0);
        }
        if (strstr(buffer, "/test2"))
        {
            if (strstr(buffer, "Proxy-Authorization: Basic bWlrZToxMTAx"))
            {
                send(c, okmsg, sizeof okmsg-1, 0);
                send(c, page1, sizeof page1-1, 0);
            }
            else
                send(c, proxymsg, sizeof proxymsg-1, 0);
        }
        if (strstr(buffer, "/test3"))
        {
            if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        if (strstr(buffer, "/test4"))
        {
            if (strstr(buffer, "Connection: Close"))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "POST /test5") ||
            strstr(buffer, "RPC_IN_DATA /test5") ||
            strstr(buffer, "RPC_OUT_DATA /test5"))
        {
            if (strstr(buffer, "Content-Length: 0"))
            {
                send(c, okmsg, sizeof okmsg-1, 0);
                send(c, page1, sizeof page1-1, 0);
            }
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "GET /test6"))
        {
            send(c, contmsg, sizeof contmsg-1, 0);
            send(c, contmsg, sizeof contmsg-1, 0);
            send(c, okmsg, sizeof okmsg-1, 0);
            send(c, page1, sizeof page1-1, 0);
        }
        if (strstr(buffer, "POST /test7"))
        {
            if (strstr(buffer, "Content-Length: 100"))
            {
                if (strstr(buffer, "POST /test7b"))
                    recvfrom(c, buffer, buffer_size, 0, NULL, NULL);
                send(c, okmsg, sizeof okmsg-1, 0);
                send(c, page1, sizeof page1-1, 0);
            }
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test8"))
        {
            if (!strstr(buffer, "Connection: Close") &&
                 strstr(buffer, "Connection: Keep-Alive") &&
                !strstr(buffer, "Cache-Control: no-cache") &&
                !strstr(buffer, "Pragma: no-cache") &&
                 strstr(buffer, host_header))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test9"))
        {
            if (!strstr(buffer, "Connection: Close") &&
                !strstr(buffer, "Connection: Keep-Alive") &&
                !strstr(buffer, "Cache-Control: no-cache") &&
                !strstr(buffer, "Pragma: no-cache") &&
                 strstr(buffer, host_header))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/testA"))
        {
            if (!strstr(buffer, "Connection: Close") &&
                !strstr(buffer, "Connection: Keep-Alive") &&
                (strstr(buffer, "Cache-Control: no-cache") ||
                 strstr(buffer, "Pragma: no-cache")) &&
                 strstr(buffer, host_header))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/testC"))
        {
            if (strstr(buffer, "cookie=biscuit"))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/testD"))
        {
            send(c, okmsg2, sizeof okmsg2-1, 0);
        }
        if (strstr(buffer, "/testE"))
        {
            send(c, noauthmsg2, sizeof noauthmsg2-1, 0);
        }
        if (strstr(buffer, "GET /quit"))
        {
            send(c, okmsg, sizeof okmsg-1, 0);
            send(c, page1, sizeof page1-1, 0);
            last_request = 1;
        }
        if (strstr(buffer, "GET /testF"))
        {
            send(c, expandcontmsg, sizeof expandcontmsg-1, 0);
            send(c, garbagemsg, sizeof garbagemsg-1, 0);
            send(c, contmsg, sizeof contmsg-1, 0);
            send(c, garbagemsg, sizeof garbagemsg-1, 0);
            send(c, okmsg, sizeof okmsg-1, 0);
            send(c, page1, sizeof page1-1, 0);
        }
        if (strstr(buffer, "GET /testG"))
        {
            send(c, page1, sizeof page1-1, 0);
        }

        if (strstr(buffer, "GET /testJ"))
        {
            if (count == 0)
            {
                count++;
                send(c, ok_with_length, sizeof(ok_with_length)-1, 0);
            }
            else
            {
                send(c, ok_with_length2, sizeof(ok_with_length2)-1, 0);
                count = 0;
            }
        }
        if (strstr(buffer, "GET /testH"))
        {
            send(c, ok_with_length2, sizeof(ok_with_length2)-1, 0);
            recvfrom(c, buffer, buffer_size, 0, NULL, NULL);
            send(c, ok_with_length, sizeof(ok_with_length)-1, 0);
        }

        if (strstr(buffer, "GET /test_no_content"))
        {
            static const char nocontentmsg[] = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
            send(c, nocontentmsg, sizeof(nocontentmsg)-1, 0);
        }
        if (strstr(buffer, "GET /test_conn_close"))
        {
            static const char conn_close_response[] = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nsome content";
            send(c, conn_close_response, sizeof(conn_close_response)-1, 0);
            WaitForSingleObject(conn_close_event, INFINITE);
            trace("closing connection\n");
        }
        if (strstr(buffer, "GET /test_cache_control_no_cache"))
        {
            static const char no_cache_response[] = "HTTP/1.1 200 OK\r\nCache-Control: no-cache\r\n\r\nsome content";
            if(!test_no_cache++)
                send(c, no_cache_response, sizeof(no_cache_response)-1, 0);
            else
                send(c, okmsg, sizeof(okmsg)-1, 0);
        }
        if (strstr(buffer, "GET /test_cache_control_no_store"))
        {
            static const char no_cache_response[] = "HTTP/1.1 200 OK\r\nCache-Control: junk, \t No-StOrE\r\n\r\nsome content";
            send(c, no_cache_response, sizeof(no_cache_response)-1, 0);
        }
        if (strstr(buffer, "GET /test_cache_gzip"))
        {
            static const char gzip_response[] = "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\nContent-Type: text/html\r\n\r\n"
                "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03\x4b\xaf\xca\x2c\x50\x28"
                "\x49\x2d\x2e\xe1\x02\x00\x62\x92\xc7\x6c\x0a\x00\x00\x00";
            if(!test_cache_gzip++)
                send(c, gzip_response, sizeof(gzip_response), 0);
            else
                send(c, notokmsg, sizeof(notokmsg)-1, 0);
        }
        if (strstr(buffer, "HEAD /test_head")) {
            static const char head_response[] =
                "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n"
                "Content-Length: 100\r\n"
                "\r\n";

            send(c, head_response, sizeof(head_response), 0);
            continue;
        }
        if (strstr(buffer, "GET /send_from_buffer"))
            send(c, send_buffer, strlen(send_buffer), 0);
        if (strstr(buffer, "/test_cache_control_verb"))
        {
            if (!memcmp(buffer, "GET ", sizeof("GET ")-1) &&
                !strstr(buffer, "Cache-Control: no-cache\r\n")) send(c, okmsg, sizeof(okmsg)-1, 0);
            else if (strstr(buffer, "Cache-Control: no-cache\r\n")) send(c, okmsg, sizeof(okmsg)-1, 0);
            else send(c, notokmsg, sizeof(notokmsg)-1, 0);
        }
        if (strstr(buffer, "/test_request_content_length"))
        {
            static char msg[] = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\n\r\n";
            static int seen_content_length;

            if (!seen_content_length)
            {
                if (strstr(buffer, "Content-Length: 0"))
                {
                    seen_content_length = 1;
                    send(c, msg, sizeof msg-1, 0);
                }
                else send(c, notokmsg, sizeof notokmsg-1, 0);
                WaitForSingleObject(complete_event, 5000);
            }
            else
            {
                if (strstr(buffer, "Content-Length: 0")) send(c, msg, sizeof msg-1, 0);
                else send(c, notokmsg, sizeof notokmsg-1, 0);
                WaitForSingleObject(complete_event, 5000);
            }
        }
        if (strstr(buffer, "GET /test_premature_disconnect"))
            trace("closing connection\n");
        if (strstr(buffer, "HEAD /upload.txt"))
        {
            if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        if (strstr(buffer, "PUT /upload2.txt"))
        {
            if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "HEAD /upload3.txt"))
        {
            if (strstr(buffer, "Authorization: Basic dXNlcjE6cHdkMQ=="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        if (strstr(buffer, "HEAD /upload4.txt"))
        {
            if (strstr(buffer, "Authorization: Bearer dXNlcjE6cHdkMQ=="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else if (strstr(buffer, "Authorization: Basic dXNlcjpwd2Q="))
                send(c, okmsg201, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        if (strstr(buffer, "/test_cookie_path1"))
        {
            if (strstr(buffer, "subcookie=data"))
                 send(c, okmsg, sizeof okmsg-1, 0);
             else
                 send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test_cookie_path2"))
        {
            if (strstr(buffer, "subcookie2=data"))
                 send(c, okmsg, sizeof okmsg-1, 0);
             else
                 send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test_cookie_set_path"))
        {
            send(c, okmsg_cookie_path, sizeof okmsg_cookie_path-1, 0);
        }
        if (strstr(buffer, "/test_cookie_merge"))
        {
            if (strstr(buffer, "subcookie=data") &&
                !strstr(buffer, "manual_cookie=test"))
                 send(c, okmsg, sizeof okmsg-1, 0);
             else
                 send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test_cookie_set_host_override"))
        {
            send(c, okmsg_cookie, sizeof okmsg_cookie-1, 0);
        }
        if (strstr(buffer, "/test_cookie_check_host_override"))
        {
            if (strstr(buffer, "Cookie:") && strstr(buffer, "testcookie=testvalue"))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test_cookie_check_different_host"))
        {
            if (!strstr(buffer, "foo") &&
                strstr(buffer, "cookie=biscuit"))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/test_host_override"))
        {
            if (strstr(buffer, host_header_override))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }
        if (strstr(buffer, "/async_read"))
        {
            const char *page1_mid = page1 + (sizeof page1 - 1)/2;
            const char *page1_end = page1 + sizeof page1 - 1;
            send(c, okmsg, sizeof okmsg-1, 0);
            send(c, page1, page1_mid - page1, 0);
            WaitForSingleObject(conn_wait_event, INFINITE);
            send(c, page1_mid, page1_end - page1_mid, 0);
        }
        if (strstr(buffer, "/socket"))
        {
            server_socket = c;
            SetEvent(server_req_rec_event);
            WaitForSingleObject(conn_wait_event, INFINITE);
        }
        if (strstr(buffer, "/echo_request"))
        {
            send(c, okmsg, sizeof(okmsg)-1, 0);
            send(c, buffer, strlen(buffer), 0);
        }
        if (strstr(buffer, "GET /test_remove_dot_segments"))
        {
            send(c, okmsg, sizeof(okmsg)-1, 0);
        }

        if (strstr(buffer, "HEAD /test_large_content"))
        {
            char msg[sizeof(largemsg) + 16];
            sprintf(msg, largemsg, content_length);
            send(c, msg, strlen(msg), 0);
        }
        if (strstr(buffer, "HEAD /test_auth_host1"))
        {
            if (strstr(buffer, "Authorization: Basic dGVzdDE6cGFzcw=="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        if (strstr(buffer, "HEAD /test_auth_host2"))
        {
            if (strstr(buffer, "Authorization: Basic dGVzdDE6cGFzczI="))
                send(c, okmsg, sizeof okmsg-1, 0);
            else
                send(c, noauthmsg, sizeof noauthmsg-1, 0);
        }
        shutdown(c, 2);
        closesocket(c);
        c = -1;
    } while (!last_request);

    closesocket(s);
    HeapFree(GetProcessHeap(), 0, buffer);

    return 0;
}

static void test_basic_request(int port, const char *verb, const char *url)
{
    test_request_t req;
    DWORD r, count, error;
    char buffer[0x100];

    trace("basic request %s %s\n", verb, url);

    open_simple_request(&req, "localhost", port, verb, url);

    SetLastError(0xdeadbeef);
    r = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    error = GetLastError();
    ok(error == ERROR_SUCCESS || broken(error != ERROR_SUCCESS), "expected ERROR_SUCCESS, got %u\n", error);
    ok(r, "HttpSendRequest failed: %u\n", GetLastError());

    count = 0;
    memset(buffer, 0, sizeof buffer);
    SetLastError(0xdeadbeef);
    r = InternetReadFile(req.request, buffer, sizeof buffer, &count);
    ok(r, "InternetReadFile failed %u\n", GetLastError());
    ok(count == sizeof page1 - 1, "count was wrong\n");
    ok(!memcmp(buffer, page1, sizeof page1), "http data wrong, got: %s\n", buffer);

    close_request(&req);
}

static void test_proxy_indirect(int port)
{
    test_request_t req;
    DWORD r, sz;
    char buffer[0x40];

    open_simple_request(&req, "localhost", port, NULL, "/test2");

    r = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed %u\n", GetLastError());

    sz = sizeof buffer;
    r = HttpQueryInfoA(req.request, HTTP_QUERY_PROXY_AUTHENTICATE, buffer, &sz, NULL);
    ok(r || GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "HttpQueryInfo failed: %d\n", GetLastError());
    if (!r)
    {
        skip("missing proxy header, not testing remaining proxy headers\n");
        goto out;
    }
    ok(!strcmp(buffer, "Basic realm=\"placebo\""), "proxy auth info wrong\n");

    test_status_code(req.request, 407);
    test_request_flags(req.request, 0);

    sz = sizeof buffer;
    r = HttpQueryInfoA(req.request, HTTP_QUERY_STATUS_TEXT, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "Proxy Authentication Required"), "proxy text wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfoA(req.request, HTTP_QUERY_VERSION, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "HTTP/1.1"), "http version wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfoA(req.request, HTTP_QUERY_SERVER, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "winetest"), "http server wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfoA(req.request, HTTP_QUERY_CONTENT_ENCODING, buffer, &sz, NULL);
    ok(GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "HttpQueryInfo should fail\n");
    ok(r == FALSE, "HttpQueryInfo failed\n");

out:
    close_request(&req);
}

static void test_proxy_direct(int port)
{
    HINTERNET hi, hc, hr;
    DWORD r, sz, error;
    char buffer[0x40], *url;
    WCHAR bufferW[0x40];
    static const char url_fmt[] = "http://test.winehq.org:%u/test2";
    static CHAR username[] = "mike",
                password[] = "1101",
                useragent[] = "winetest";
    static const WCHAR usernameW[]  = {'m','i','k','e',0},
                       passwordW[]  = {'1','1','0','1',0},
                       useragentW[] = {'w','i','n','e','t','e','s','t',0};

    /* specify proxy type without the proxy and bypass */
    SetLastError(0xdeadbeef);
    hi = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PROXY, NULL, NULL, 0);
    error = GetLastError();
    ok(error == ERROR_INVALID_PARAMETER ||
        broken(error == ERROR_SUCCESS) /* WinXPProSP2 */, "got %u\n", error);
    ok(hi == NULL || broken(!!hi) /* WinXPProSP2 */, "open should have failed\n");

    sprintf(buffer, "localhost:%d\n", port);
    hi = InternetOpenA(NULL, INTERNET_OPEN_TYPE_PROXY, buffer, NULL, 0);
    ok(hi != NULL, "open failed\n");

    /* try connect without authorization */
    hc = InternetConnectA(hi, "test.winehq.org", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequestA(hc, NULL, "/test2", NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_PASSWORD, NULL, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == 1, "got %u\n", sz);

    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, NULL, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == 1, "got %u\n", sz);

    sz = sizeof(buffer);
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_PASSWORD, buffer, &sz);
    ok(r, "unexpected failure %u\n", GetLastError());
    ok(!sz, "got %u\n", sz);

    sz = sizeof(buffer);
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, buffer, &sz);
    ok(r, "unexpected failure %u\n", GetLastError());
    ok(!sz, "got %u\n", sz);

    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PASSWORD, NULL, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == 1, "got %u\n", sz);

    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_USERNAME, NULL, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == 1, "got %u\n", sz);

    sz = sizeof(buffer);
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PASSWORD, buffer, &sz);
    ok(r, "unexpected failure %u\n", GetLastError());
    ok(!sz, "got %u\n", sz);

    sz = sizeof(buffer);
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_USERNAME, buffer, &sz);
    ok(r, "unexpected failure %u\n", GetLastError());
    ok(!sz, "got %u\n", sz);

    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_URL, NULL, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == 34, "got %u\n", sz);

    sz = sizeof(buffer);
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_URL, buffer, &sz);
    ok(r, "unexpected failure %u\n", GetLastError());
    ok(sz == 33, "got %u\n", sz);

    r = HttpSendRequestW(hr, NULL, 0, NULL, 0);
    ok(r || broken(!r), "HttpSendRequest failed %u\n", GetLastError());
    if (!r)
    {
        win_skip("skipping proxy tests on broken wininet\n");
        goto done;
    }

    test_status_code(hr, 407);

    /* set the user + password then try again */
    r = InternetSetOptionA(hi, INTERNET_OPTION_PROXY_USERNAME, username, 4);
    ok(!r, "unexpected success\n");

    r = InternetSetOptionA(hc, INTERNET_OPTION_PROXY_USERNAME, username, 4);
    ok(r, "failed to set user\n");

    r = InternetSetOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, username, 4);
    ok(r, "failed to set user\n");

    buffer[0] = 0;
    sz = 3;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, buffer, &sz);
    ok(!r, "unexpected failure %u\n", GetLastError());
    ok(!buffer[0], "got %s\n", buffer);
    ok(sz == strlen(username) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(username) + 1, "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PROXY_USERNAME, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (lstrlenW(usernameW) + 1) * sizeof(WCHAR), "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, buffer, &sz);
    ok(r, "failed to get username\n");
    ok(!strcmp(buffer, username), "got %s\n", buffer);
    ok(sz == strlen(username), "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PROXY_USERNAME, bufferW, &sz);
    ok(r, "failed to get username\n");
    ok(!lstrcmpW(bufferW, usernameW), "wrong username\n");
    ok(sz == lstrlenW(usernameW), "got %u\n", sz);

    r = InternetSetOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, username, 1);
    ok(r, "failed to set user\n");

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_USERNAME, buffer, &sz);
    ok(r, "failed to get username\n");
    ok(!strcmp(buffer, username), "got %s\n", buffer);
    ok(sz == strlen(username), "got %u\n", sz);

    r = InternetSetOptionA(hi, INTERNET_OPTION_USER_AGENT, useragent, 1);
    ok(r, "failed to set useragent\n");

    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hi, INTERNET_OPTION_USER_AGENT, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(useragent) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hi, INTERNET_OPTION_USER_AGENT, buffer, &sz);
    ok(r, "failed to get user agent\n");
    ok(!strcmp(buffer, useragent), "got %s\n", buffer);
    ok(sz == strlen(useragent), "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hi, INTERNET_OPTION_USER_AGENT, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (lstrlenW(useragentW) + 1) * sizeof(WCHAR), "got %u\n", sz);

    bufferW[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hi, INTERNET_OPTION_USER_AGENT, bufferW, &sz);
    ok(r, "failed to get user agent\n");
    ok(!lstrcmpW(bufferW, useragentW), "wrong user agent\n");
    ok(sz == lstrlenW(useragentW), "got %u\n", sz);

    r = InternetSetOptionA(hr, INTERNET_OPTION_USERNAME, username, 1);
    ok(r, "failed to set user\n");

    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_USERNAME, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(username) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_USERNAME, buffer, &sz);
    ok(r, "failed to get user\n");
    ok(!strcmp(buffer, username), "got %s\n", buffer);
    ok(sz == strlen(username), "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_USERNAME, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (lstrlenW(usernameW) + 1) * sizeof(WCHAR), "got %u\n", sz);

    bufferW[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_USERNAME, bufferW, &sz);
    ok(r, "failed to get user\n");
    ok(!lstrcmpW(bufferW, usernameW), "wrong user\n");
    ok(sz == lstrlenW(usernameW), "got %u\n", sz);

    r = InternetSetOptionA(hr, INTERNET_OPTION_PASSWORD, password, 1);
    ok(r, "failed to set password\n");

    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PASSWORD, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(password) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PASSWORD, buffer, &sz);
    ok(r, "failed to get password\n");
    ok(!strcmp(buffer, password), "got %s\n", buffer);
    ok(sz == strlen(password), "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PASSWORD, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (lstrlenW(passwordW) + 1) * sizeof(WCHAR), "got %u\n", sz);

    bufferW[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PASSWORD, bufferW, &sz);
    ok(r, "failed to get password\n");
    ok(!lstrcmpW(bufferW, passwordW), "wrong password\n");
    ok(sz == lstrlenW(passwordW), "got %u\n", sz);

    url = HeapAlloc(GetProcessHeap(), 0, strlen(url_fmt) + 11);
    sprintf(url, url_fmt, port);
    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_URL, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(url) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_URL, buffer, &sz);
    ok(r, "failed to get url\n");
    ok(!strcmp(buffer, url), "got %s\n", buffer);
    ok(sz == strlen(url), "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_URL, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (strlen(url) + 1) * sizeof(WCHAR), "got %u\n", sz);

    bufferW[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_URL, bufferW, &sz);
    ok(r, "failed to get url\n");
    ok(!strcmp_wa(bufferW, url), "wrong url\n");
    ok(sz == strlen(url), "got %u\n", sz);
    HeapFree(GetProcessHeap(), 0, url);

    r = InternetSetOptionA(hr, INTERNET_OPTION_PROXY_PASSWORD, password, 4);
    ok(r, "failed to set password\n");

    buffer[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_PASSWORD, buffer, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == strlen(password) + 1, "got %u\n", sz);

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = InternetQueryOptionA(hr, INTERNET_OPTION_PROXY_PASSWORD, buffer, &sz);
    ok(r, "failed to get password\n");
    ok(!strcmp(buffer, password), "got %s\n", buffer);
    ok(sz == strlen(password), "got %u\n", sz);

    bufferW[0] = 0;
    sz = 0;
    SetLastError(0xdeadbeef);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PROXY_PASSWORD, bufferW, &sz);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %u\n", GetLastError());
    ok(!r, "unexpected success\n");
    ok(sz == (lstrlenW(passwordW) + 1) * sizeof(WCHAR), "got %u\n", sz);

    bufferW[0] = 0;
    sz = sizeof(bufferW);
    r = InternetQueryOptionW(hr, INTERNET_OPTION_PROXY_PASSWORD, bufferW, &sz);
    ok(r, "failed to get password\n");
    ok(!lstrcmpW(bufferW, passwordW), "wrong password\n");
    ok(sz == lstrlenW(passwordW), "got %u\n", sz);

    r = HttpSendRequestW(hr, NULL, 0, NULL, 0);
    if (!r)
    {
        win_skip("skipping proxy tests on broken wininet\n");
        goto done;
    }
    ok(r, "HttpSendRequest failed %u\n", GetLastError());
    sz = sizeof buffer;
    r = HttpQueryInfoA(hr, HTTP_QUERY_STATUS_CODE, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "200"), "proxy code wrong\n");

    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);

    sprintf(buffer, "localhost:%d\n", port);
    hi = InternetOpenA("winetest", INTERNET_OPEN_TYPE_PROXY, buffer, NULL, 0);
    ok(hi != NULL, "InternetOpen failed\n");

    hc = InternetConnectA(hi, "test.winehq.org", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "InternetConnect failed\n");

    hr = HttpOpenRequestA(hc, "POST", "/test2", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    r = HttpSendRequestA(hr, NULL, 0, (char *)"data", sizeof("data"));
    ok(r, "HttpSendRequest failed %u\n", GetLastError());

    test_status_code(hr, 407);

done:
    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static void test_header_handling_order(int port)
{
    static const char authorization[] = "Authorization: Basic dXNlcjpwd2Q=";
    static const char connection[]    = "Connection: Close";
    static const char *types[2] = { "*", NULL };
    char data[32];
    HINTERNET session, connect, request;
    DWORD size, status, data_len;
    BOOL ret;

    session = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, NULL, "/test3", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(request, authorization, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(request, 200);
    test_request_flags(request, 0);

    InternetCloseHandle(request);

    request = HttpOpenRequestA(connect, NULL, "/test4", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(request, connection, ~0u, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfoA( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200 || status == 400 /* IE6 */, "got status %u, expected 200 or 400\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);

    connect = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, "POST", "/test7", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(request, "Content-Length: 100\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(request, connection, ~0u, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfoA( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "got status %u, expected 200\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);

    connect = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, "POST", "/test7b", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(request, "Content-Length: 100\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders failed\n");

    data_len = sizeof(data);
    memset(data, 'a', sizeof(data));
    ret = HttpSendRequestA(request, NULL, 0, data, data_len);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfoA( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "got status %u, expected 200\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_connection_header(int port)
{
    HINTERNET ses, con, req;
    BOOL ret;

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, NULL, "/test8", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test9", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test9", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, "POST", "/testA", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_header_override(int port)
{
    char buffer[128], host_header_override[30], full_url[128];
    HINTERNET ses, con, req;
    DWORD size, count, err;
    BOOL ret;

    sprintf(host_header_override, "Host: test.local:%d\r\n", port);
    sprintf(full_url, "http://localhost:%d/test_host_override", port);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, NULL, "/test_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    size = sizeof(buffer) - 1;
    count = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = HttpQueryInfoA(req, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, &count);
    err = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "Expected error ERROR_HTTP_HEADER_NOT_FOUND, got %d\n", err);

    test_request_url(req, full_url);

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_COALESCE);
    ok(ret, "HttpAddRequestHeaders failed\n");

    size = sizeof(buffer) - 1;
    count = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = HttpQueryInfoA(req, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, &count);
    ok(ret, "HttpQueryInfo failed\n");

    test_request_url(req, full_url);

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_COALESCE);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_COALESCE);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 400);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_REPLACE);
    if(ret) { /* win10 returns success */
        trace("replacing host header is supported.\n");

        ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
        ok(ret, "HttpSendRequest failed\n");

        test_status_code(req, 200);
    }else {
        trace("replacing host header is not supported.\n");

        err = GetLastError();
        ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "Expected error ERROR_HTTP_HEADER_NOT_FOUND, got %d\n", err);

        ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
        ok(ret, "HttpSendRequest failed\n");

        test_status_code(req, 400);
    }

    InternetCloseHandle(req);
    InternetSetCookieA("http://localhost", "cookie", "biscuit");
    req = HttpOpenRequestA(con, NULL, "/testC", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_cookie_set_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_cookie_check_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_cookie_check_host_override", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetSetCookieA("http://test.local", "foo", "bar");
    req = HttpOpenRequestA(con, NULL, "/test_cookie_check_different_host", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    req = HttpOpenRequestA(con, NULL, "/test_cookie_check_different_host", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpenA failed\n");

    con = InternetConnectA(ses, "localhost", port, "test1", "pass", INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnectA failed %u\n", GetLastError());

    req = HttpOpenRequestA( con, "HEAD", "/test_auth_host1", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequestA failed %u\n", GetLastError());

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequestA failed %u\n", GetLastError());

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpenA failed\n");

    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnectA failed %u\n", GetLastError());

    req = HttpOpenRequestA(con, "HEAD", "/test_auth_host1", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequestA failed %u\n", GetLastError());

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpenA failed\n");

    con = InternetConnectA(ses, "localhost", port, "test1", "pass2", INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnectA failed %u\n", GetLastError());

    req = HttpOpenRequestA(con, "HEAD", "/test_auth_host2", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequestA failed %u\n", GetLastError());

    ret = HttpAddRequestHeadersA(req, host_header_override, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequestA failed %u\n", GetLastError());

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpenA failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnectA failed %u\n", GetLastError());

    req = HttpOpenRequestA(con, "HEAD", "/test_auth_host2", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequestA failed %u\n", GetLastError());

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequestA failed %u\n", GetLastError());

    test_status_code(req, 200);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_connection_closing(int port)
{
    HINTERNET session, connection, req;
    DWORD res;

    reset_events();

    session = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(session, callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    connection = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(connection != NULL,"InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(connection, "GET", "/testJ", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0xdeadbeaf);
    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    test_status_code(req, 200);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    test_status_code(req, 210);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    test_status_code(req, 200);

    SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);

    close_async_handle(session, 2);
}

static void test_successive_HttpSendRequest(int port)
{
    HINTERNET session, connection, req;
    DWORD res;

    reset_events();

    session = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(session, callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    connection = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(connection != NULL,"InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(connection, "GET", "/testH", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0xdeadbeaf);
    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    test_status_code(req, 210);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    test_status_code(req, 200);

    SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);

    close_async_handle(session, 2);
}

static void test_no_content(int port)
{
    HINTERNET session, connection, req;
    DWORD res;

    trace("Testing 204 no content response...\n");

    reset_events();

    session = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(session, callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    connection = InternetConnectA(session, "localhost", port,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(connection != NULL,"InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(connection, "GET", "/test_no_content", NULL, NULL, NULL,
            INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE, 0xdeadbead);
    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, -1, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    close_async_handle(session, 2);

    /*
     * The connection should be closed before closing handle. This is true for most
     * wininet versions (including Wine), but some old win2k versions fail to do that.
     */
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
}

static void test_conn_close(int port)
{
    HINTERNET session, connection, req;
    DWORD res, avail, size;
    BYTE buf[1024];

    trace("Testing connection close connection...\n");

    reset_events();

    session = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(session, callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    connection = InternetConnectA(session, "localhost", port,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(connection != NULL,"InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(connection, "GET", "/test_conn_close", NULL, NULL, NULL,
            INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE, 0xdeadbead);
    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    res = HttpSendRequestA(req, NULL, -1, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    avail = 0;
    res = InternetQueryDataAvailable(req, &avail, 0, 0);
    ok(res, "InternetQueryDataAvailable failed: %u\n", GetLastError());
    ok(avail != 0, "avail = 0\n");

    size = 0;
    res = InternetReadFile(req, buf, avail, &size);
    ok(res, "InternetReadFile failed: %u\n", GetLastError());

    /* IE11 calls those in InternetQueryDataAvailable call. */
    SET_OPTIONAL(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_OPTIONAL(INTERNET_STATUS_RESPONSE_RECEIVED);

    res = InternetQueryDataAvailable(req, &avail, 0, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    ok(!avail, "avail = %u, expected 0\n", avail);

    CLEAR_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SetEvent(conn_close_event);
    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %u\n", req_error);
    CLEAR_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);

    close_async_handle(session, 2);
}

static void test_no_cache(int port)
{
    static const char cache_control_no_cache[] = "/test_cache_control_no_cache";
    static const char cache_control_no_store[] = "/test_cache_control_no_store";
    static const char cache_url_fmt[] = "http://localhost:%d%s";

    char cache_url[256], buf[256];
    HINTERNET ses, con, req;
    DWORD read, size;
    BOOL ret;

    trace("Testing no-cache header\n");

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL,"InternetOpen failed with error %u\n", GetLastError());

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed with error %u\n", GetLastError());

    req = HttpOpenRequestA(con, NULL, cache_control_no_cache, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    sprintf(cache_url, cache_url_fmt, port, cache_control_no_cache);
    DeleteUrlCacheEntryA(cache_url);

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf, sizeof(buf), &read) && read)
        size += read;
    ok(size == 12, "read %d bytes of data\n", size);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, cache_control_no_cache, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf, sizeof(buf), &read) && read)
        size += read;
    ok(size == 0, "read %d bytes of data\n", size);
    InternetCloseHandle(req);
    DeleteUrlCacheEntryA(cache_url);

    req = HttpOpenRequestA(con, NULL, cache_control_no_store, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    sprintf(cache_url, cache_url_fmt, port, cache_control_no_store);
    DeleteUrlCacheEntryA(cache_url);

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf, sizeof(buf), &read) && read)
        size += read;
    ok(size == 12, "read %d bytes of data\n", size);
    InternetCloseHandle(req);

    ret = DeleteUrlCacheEntryA(cache_url);
    ok(!ret && GetLastError()==ERROR_FILE_NOT_FOUND, "cache entry should not exist\n");

    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_cache_read_gzipped(int port)
{
    static const char cache_url_fmt[] = "http://localhost:%d%s";
    static const char get_gzip[] = "/test_cache_gzip";
    static const char content[] = "gzip test\n";
    static const char text_html[] = "text/html";
    static const char raw_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

    HINTERNET ses, con, req;
    DWORD read, size;
    char cache_url[256], buf[256];
    BOOL ret;

    trace("Testing reading compressed content from cache\n");

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL,"InternetOpen failed with error %u\n", GetLastError());

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed with error %u\n", GetLastError());

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = TRUE;
    ret = InternetSetOptionA(req, INTERNET_OPTION_HTTP_DECODING, &ret, sizeof(ret));
    if(!ret && GetLastError()==ERROR_INTERNET_INVALID_OPTION) {
        win_skip("INTERNET_OPTION_HTTP_DECODING not supported\n");
        InternetCloseHandle(req);
        InternetCloseHandle(con);
        InternetCloseHandle(ses);
        return;
    }
    ok(ret, "InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %d\n", GetLastError());

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-size, &read) && read)
        size += read;
    ok(size == 10, "read %d bytes of data\n", size);
    buf[size] = 0;
    ok(!strncmp(buf, content, size), "incorrect page content: %s\n", buf);

    size = sizeof(buf)-1;
    ret = HttpQueryInfoA(req, HTTP_QUERY_CONTENT_TYPE, buf, &size, 0);
    ok(ret, "HttpQueryInfo(HTTP_QUERY_CONTENT_TYPE) failed: %d\n", GetLastError());
    buf[size] = 0;
    ok(!strncmp(text_html, buf, size), "buf = %s\n", buf);

    size = sizeof(buf)-1;
    ret = HttpQueryInfoA(req, HTTP_QUERY_RAW_HEADERS_CRLF, buf, &size, 0);
    ok(ret, "HttpQueryInfo(HTTP_QUERY_CONTENT_TYPE) failed: %d\n", GetLastError());
    buf[size] = 0;
    ok(!strncmp(raw_header, buf, size), "buf = %s\n", buf);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, INTERNET_FLAG_FROM_CACHE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = TRUE;
    ret = InternetSetOptionA(req, INTERNET_OPTION_HTTP_DECODING, &ret, sizeof(ret));
    ok(ret, "InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %d\n", GetLastError());

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-1-size, &read) && read)
        size += read;
    todo_wine ok(size == 10, "read %d bytes of data\n", size);
    buf[size] = 0;
    ok(!strncmp(buf, content, size), "incorrect page content: %s\n", buf);

    size = sizeof(buf);
    ret = HttpQueryInfoA(req, HTTP_QUERY_CONTENT_ENCODING, buf, &size, 0);
    ok(!ret && GetLastError()==ERROR_HTTP_HEADER_NOT_FOUND,
            "HttpQueryInfo(HTTP_QUERY_CONTENT_ENCODING) returned %d, %d\n",
            ret, GetLastError());

    size = sizeof(buf)-1;
    ret = HttpQueryInfoA(req, HTTP_QUERY_CONTENT_TYPE, buf, &size, 0);
    todo_wine ok(ret, "HttpQueryInfo(HTTP_QUERY_CONTENT_TYPE) failed: %d\n", GetLastError());
    buf[size] = 0;
    todo_wine ok(!strncmp(text_html, buf, size), "buf = %s\n", buf);
    InternetCloseHandle(req);

    /* Decompression doesn't work while reading from cache */
    test_cache_gzip = 0;
    sprintf(cache_url, cache_url_fmt, port, get_gzip);
    DeleteUrlCacheEntryA(cache_url);

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-1-size, &read) && read)
        size += read;
    ok(size == 31, "read %d bytes of data\n", size);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, INTERNET_FLAG_FROM_CACHE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = TRUE;
    ret = InternetSetOptionA(req, INTERNET_OPTION_HTTP_DECODING, &ret, sizeof(ret));
    ok(ret, "InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %d\n", GetLastError());

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-1-size, &read) && read)
        size += read;
    todo_wine ok(size == 31, "read %d bytes of data\n", size);

    size = sizeof(buf);
    ret = HttpQueryInfoA(req, HTTP_QUERY_CONTENT_ENCODING, buf, &size, 0);
    todo_wine ok(ret, "HttpQueryInfo(HTTP_QUERY_CONTENT_ENCODING) failed: %d\n", GetLastError());
    InternetCloseHandle(req);

    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    /* Decompression doesn't work while reading from cache */
    test_cache_gzip = 0;
    sprintf(cache_url, cache_url_fmt, port, get_gzip);
    DeleteUrlCacheEntryA(cache_url);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL,"InternetOpen failed with error %u\n", GetLastError());

    ret = TRUE;
    ret = InternetSetOptionA(ses, INTERNET_OPTION_HTTP_DECODING, &ret, sizeof(ret));
    ok(ret, "InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %d\n", GetLastError());

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed with error %u\n", GetLastError());

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-1-size, &read) && read)
        size += read;
    ok(size == 10, "read %d bytes of data\n", size);
    buf[size] = 0;
    ok(!strncmp(buf, content, size), "incorrect page content: %s\n", buf);
    InternetCloseHandle(req);

    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    /* Decompression doesn't work while reading from cache */
    test_cache_gzip = 0;
    sprintf(cache_url, cache_url_fmt, port, get_gzip);
    DeleteUrlCacheEntryA(cache_url);

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL,"InternetOpen failed with error %u\n", GetLastError());

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed with error %u\n", GetLastError());

    ret = TRUE;
    ret = InternetSetOptionA(con, INTERNET_OPTION_HTTP_DECODING, &ret, sizeof(ret));
    ok(ret, "InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %d\n", GetLastError());

    req = HttpOpenRequestA(con, NULL, get_gzip, NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", -1, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());
    size = 0;
    while(InternetReadFile(req, buf+size, sizeof(buf)-1-size, &read) && read)
        size += read;
    ok(size == 10, "read %d bytes of data\n", size);
    buf[size] = 0;
    ok(!strncmp(buf, content, size), "incorrect page content: %s\n", buf);
    InternetCloseHandle(req);

    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    DeleteUrlCacheEntryA(cache_url);
}

static void test_HttpSendRequestW(int port)
{
    static const WCHAR header[] = {'U','A','-','C','P','U',':',' ','x','8','6',0};
    HINTERNET ses, con, req;
    DWORD error;
    BOOL ret;

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, NULL, "/test1", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestW(req, header, ~0u, NULL, 0);
    error = GetLastError();
    ok(!ret, "HttpSendRequestW succeeded\n");
    ok(error == ERROR_IO_PENDING ||
       broken(error == ERROR_HTTP_HEADER_NOT_FOUND) ||  /* IE6 */
       broken(error == ERROR_INVALID_PARAMETER),        /* IE5 */
       "got %u expected ERROR_IO_PENDING\n", error);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_cookie_header(int port)
{
    HINTERNET ses, con, req;
    DWORD size, error;
    BOOL ret;
    char buffer[256];

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    InternetSetCookieA("http://localhost", "cookie", "biscuit");

    req = HttpOpenRequestA(con, NULL, "/testC", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    buffer[0] = 0;
    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = HttpQueryInfoA(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    error = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(error == ERROR_HTTP_HEADER_NOT_FOUND, "got %u expected ERROR_HTTP_HEADER_NOT_FOUND\n", error);

    ret = HttpAddRequestHeadersA(req, "Cookie: cookie=not biscuit\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed: %u\n", GetLastError());

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!!strstr(buffer, "cookie=not biscuit"), "got '%s' expected \'cookie=not biscuit\'\n", buffer);

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    test_status_code(req, 200);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!strstr(buffer, "cookie=not biscuit"), "'%s' should not contain \'cookie=not biscuit\'\n", buffer);
    ok(!!strstr(buffer, "cookie=biscuit"), "'%s' should contain \'cookie=biscuit\'\n", buffer);

    InternetCloseHandle(req);

    InternetSetCookieA("http://localhost/testCCCC", "subcookie", "data");

    req = HttpOpenRequestA(con, NULL, "/test_cookie_path1", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test_cookie_path1/abc", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test_cookie_set_path", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test_cookie_path2", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 400);
    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, NULL, "/test_cookie_merge", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, "Cookie: manual_cookie=test\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed: %u\n", GetLastError());

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    test_status_code(req, 200);
    InternetCloseHandle(req);

    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_basic_authentication(int port)
{
    HINTERNET session, connect, request;
    BOOL ret;

    session = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnectA(session, "localhost", port, "user", "pwd", INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, NULL, "/test3", NULL, NULL, NULL, 0, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());

    test_status_code(request, 200);
    test_request_flags(request, 0);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_premature_disconnect(int port)
{
    test_request_t req;
    DWORD err;
    BOOL ret;

    open_simple_request(&req, "localhost", port, NULL, "/premature_disconnect");

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    err = GetLastError();
    todo_wine ok(!ret, "HttpSendRequest succeeded\n");
    todo_wine ok(err == ERROR_HTTP_INVALID_SERVER_RESPONSE, "got %u\n", err);

    close_request(&req);
}

static void test_invalid_response_headers(int port)
{
    test_request_t req;
    DWORD size;
    BOOL ret;
    char buffer[256];

    open_simple_request(&req, "localhost", port, NULL, "/testE");

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());

    test_status_code(req.request, 401);
    test_request_flags(req.request, 0);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "HTTP/1.0 401 Anonymous requests or requests on unsecure channel are not allowed"),
       "headers wrong \"%s\"\n", buffer);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_SERVER, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "winetest"), "server wrong \"%s\"\n", buffer);

    close_request(&req);
}

static void test_response_without_headers(int port)
{
    test_request_t req;
    DWORD r, count, size;
    char buffer[1024];

    open_simple_request(&req, "localhost", port, NULL, "/testG");

    test_request_flags(req.request, INTERNET_REQFLAG_NO_HEADERS);

    r = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed %u\n", GetLastError());

    test_request_flags_todo(req.request, INTERNET_REQFLAG_NO_HEADERS);

    count = 0;
    memset(buffer, 0, sizeof buffer);
    r = InternetReadFile(req.request, buffer, sizeof buffer, &count);
    ok(r, "InternetReadFile failed %u\n", GetLastError());
    todo_wine ok(count == sizeof page1 - 1, "count was wrong\n");
    todo_wine ok(!memcmp(buffer, page1, sizeof page1), "http data wrong\n");

    test_status_code(req.request, 200);
    test_request_flags_todo(req.request, INTERNET_REQFLAG_NO_HEADERS);

    buffer[0] = 0;
    size = sizeof(buffer);
    r = HttpQueryInfoA(req.request, HTTP_QUERY_STATUS_TEXT, buffer, &size, NULL );
    ok(r, "HttpQueryInfo failed %u\n", GetLastError());
    ok(!strcmp(buffer, "OK"), "expected OK got: \"%s\"\n", buffer);

    buffer[0] = 0;
    size = sizeof(buffer);
    r = HttpQueryInfoA(req.request, HTTP_QUERY_VERSION, buffer, &size, NULL);
    ok(r, "HttpQueryInfo failed %u\n", GetLastError());
    ok(!strcmp(buffer, "HTTP/1.0"), "expected HTTP/1.0 got: \"%s\"\n", buffer);

    buffer[0] = 0;
    size = sizeof(buffer);
    r = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS, buffer, &size, NULL);
    ok(r, "HttpQueryInfo failed %u\n", GetLastError());
    ok(!strcmp(buffer, "HTTP/1.0 200 OK"), "raw headers wrong: \"%s\"\n", buffer);

    close_request(&req);
}

static void test_head_request(int port)
{
    DWORD len, content_length;
    test_request_t req;
    BYTE buf[100];
    BOOL ret;

    open_simple_request(&req, "localhost", port, "HEAD", "/test_head");

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    len = sizeof(content_length);
    content_length = -1;
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH, &content_length, &len, 0);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(len == sizeof(DWORD), "len = %u\n", len);
    ok(content_length == 100, "content_length = %u\n", content_length);

    len = -1;
    ret = InternetReadFile(req.request, buf, sizeof(buf), &len);
    ok(ret, "InternetReadFile failed: %u\n", GetLastError());

    len = -1;
    ret = InternetReadFile(req.request, buf, sizeof(buf), &len);
    ok(ret, "InternetReadFile failed: %u\n", GetLastError());

    close_request(&req);
}

static void test_HttpQueryInfo(int port)
{
    test_request_t req;
    DWORD size, index, error;
    char buffer[1024];
    BOOL ret;

    open_simple_request(&req, "localhost", port, NULL, "/testD");

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_STATUS_TEXT, buffer, &size, &index);
    error = GetLastError();
    ok(!ret || broken(ret), "HttpQueryInfo succeeded\n");
    if (!ret) ok(error == ERROR_HTTP_HEADER_NOT_FOUND, "got %u expected ERROR_HTTP_HEADER_NOT_FOUND\n", error);

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_DATE | HTTP_QUERY_FLAG_SYSTEMTIME, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    index = 0xdeadbeef; /* invalid start index */
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS, buffer, &size, &index);
    todo_wine ok(!ret, "HttpQueryInfo should have failed\n");
    todo_wine ok(GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND,
       "Expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", GetLastError());

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_STATUS_TEXT, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_VERSION, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    test_status_code(req.request, 200);

    index = 0xdeadbeef;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_FORWARDED, buffer, &size, &index);
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(index == 0xdeadbeef, "expected 0xdeadbeef got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_SERVER, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    strcpy(buffer, "Server");
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_CUSTOM, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_SET_COOKIE, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_SET_COOKIE, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 2, "expected 2 got %u\n", index);

    close_request(&req);
}

static void test_options(int port)
{
    INTERNET_DIAGNOSTIC_SOCKET_INFO idsi;
    HINTERNET ses, con, req;
    DWORD size, error;
    DWORD_PTR ctx;
    BOOL ret;

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, NULL, 0);
    error = GetLastError();
    ok(!ret, "InternetSetOption succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, NULL, sizeof(ctx));
    ok(!ret, "InternetSetOption succeeded\n");
    error = GetLastError();
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, &ctx, 0);
    ok(!ret, "InternetSetOption succeeded\n");
    error = GetLastError();
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    ctx = 1;
    ret = InternetSetOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, &ctx, sizeof(ctx));
    ok(ret, "InternetSetOption failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, NULL, NULL);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, &ctx, NULL);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, NULL, &size);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);

    size = sizeof(ctx);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(NULL, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE, got %u\n", error);

    ctx = 0xdeadbeef;
    size = sizeof(ctx);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());
    ok(ctx == 1, "expected 1 got %lu\n", ctx);

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    ctx = 0xdeadbeef;
    size = sizeof(ctx);
    ret = InternetQueryOptionA(con, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());
    ok(ctx == 0, "expected 0 got %lu\n", ctx);

    ctx = 2;
    ret = InternetSetOptionA(con, INTERNET_OPTION_CONTEXT_VALUE, &ctx, sizeof(ctx));
    ok(ret, "InternetSetOption failed %u\n", GetLastError());

    ctx = 0xdeadbeef;
    size = sizeof(ctx);
    ret = InternetQueryOptionA(con, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());
    ok(ctx == 2, "expected 2 got %lu\n", ctx);

    req = HttpOpenRequestA(con, NULL, "/test1", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ctx = 0xdeadbeef;
    size = sizeof(ctx);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());
    ok(ctx == 0, "expected 0 got %lu\n", ctx);

    ctx = 3;
    ret = InternetSetOptionA(req, INTERNET_OPTION_CONTEXT_VALUE, &ctx, sizeof(ctx));
    ok(ret, "InternetSetOption failed %u\n", GetLastError());

    ctx = 0xdeadbeef;
    size = sizeof(ctx);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_CONTEXT_VALUE, &ctx, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());
    ok(ctx == 3, "expected 3 got %lu\n", ctx);

    size = sizeof(idsi);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO, &idsi, &size);
    ok(ret, "InternetQueryOption failed %u\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, NULL, &size);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INTERNET_INVALID_OPERATION, "expected ERROR_INTERNET_INVALID_OPERATION, got %u\n", error);

    /* INTERNET_OPTION_PROXY */
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PROXY, NULL, NULL);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PROXY, &ctx, NULL);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PROXY, NULL, &size);
    error = GetLastError();
    ok(!ret, "InternetQueryOption succeeded\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);
    ok(size >= sizeof(INTERNET_PROXY_INFOA), "expected size to be greater or equal to the struct size\n");

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

typedef struct {
    const char *response_text;
    int status_code;
    const char *status_text;
    const char *raw_headers;
} http_status_test_t;

static const http_status_test_t http_status_tests[] = {
    {
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 1\r\n"
        "\r\nx",
        200,
        "OK"
    },
    {
        "HTTP/1.1 404 Fail\r\n"
        "Content-Length: 1\r\n"
        "\r\nx",
        404,
        "Fail"
    },
    {
        "HTTP/1.1 200\r\n"
        "Content-Length: 1\r\n"
        "\r\nx",
        200,
        ""
    },
    {
        "HTTP/1.1 410 \r\n"
        "Content-Length: 1\r\n"
        "\r\nx",
        410,
        ""
    }
};

static void test_http_status(int port)
{
    test_request_t req;
    char buf[1000];
    DWORD i, size;
    BOOL res;

    for(i = 0; i < ARRAY_SIZE(http_status_tests); i++) {
        send_buffer = http_status_tests[i].response_text;

        open_simple_request(&req, "localhost", port, NULL, "/send_from_buffer");

        res = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
        ok(res, "HttpSendRequest failed\n");

        test_status_code(req.request, http_status_tests[i].status_code);

        size = sizeof(buf);
        res = HttpQueryInfoA(req.request, HTTP_QUERY_STATUS_TEXT, buf, &size, NULL);
        ok(res, "HttpQueryInfo failed: %u\n", GetLastError());
        ok(!strcmp(buf, http_status_tests[i].status_text), "[%u] Unexpected status text \"%s\", expected \"%s\"\n",
           i, buf, http_status_tests[i].status_text);

        close_request(&req);
    }
}

static void test_cache_control_verb(int port)
{
    HINTERNET session, connect, request;
    BOOL ret;

    session = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnectA(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, "RPC_OUT_DATA", "/test_cache_control_verb", NULL, NULL, NULL,
                              INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(request, 200);
    InternetCloseHandle(request);

    request = HttpOpenRequestA(connect, "POST", "/test_cache_control_verb", NULL, NULL, NULL,
                              INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(request, 200);
    InternetCloseHandle(request);

    request = HttpOpenRequestA(connect, "HEAD", "/test_cache_control_verb", NULL, NULL, NULL,
                              INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(request, 200);
    InternetCloseHandle(request);

    request = HttpOpenRequestA(connect, "GET", "/test_cache_control_verb", NULL, NULL, NULL,
                              INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(request, 200);
    InternetCloseHandle(request);

    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_request_content_length(int port)
{
    char data[] = {'t','e','s','t'};
    test_request_t req;
    BOOL ret;

    reset_events();
    open_simple_request(&req, "localhost", port, "POST", "/test_request_content_length");

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(req.request, 200);

    SetEvent(complete_event);

    ret = HttpSendRequestA(req.request, NULL, 0, data, sizeof(data));
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());
    test_status_code(req.request, 200);

    SetEvent(complete_event);
    close_request(&req);
}

static void test_accept_encoding(int port)
{
    HINTERNET ses, con, req;
    char buf[1000];
    BOOL ret;

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, "GET", "/echo_request", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeadersA(req, "Accept-Encoding: gzip\r\n", ~0u, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequestA(req, NULL,  0, NULL, 0);
    ok(ret, "HttpSendRequestA failed\n");

    test_status_code(req, 200);
    receive_simple_request(req, buf, sizeof(buf));
    ok(strstr(buf, "Accept-Encoding: gzip") != NULL, "Accept-Encoding header not found in %s\n", buf);

    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, "GET", "/echo_request", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, "Accept-Encoding: gzip", ~0u, NULL, 0);
    ok(ret, "HttpSendRequestA failed\n");

    test_status_code(req, 200);
    receive_simple_request(req, buf, sizeof(buf));
    ok(strstr(buf, "Accept-Encoding: gzip") != NULL, "Accept-Encoding header not found in %s\n", buf);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_basic_auth_credentials_reuse(int port)
{
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;
    char buffer[0x40];

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, "user", "pwd",
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "user"), "got %s\n", buffer);
    ok(size == 4, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "pwd"), "got %s\n", buffer);
    ok(size == 3, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, NULL, NULL,
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "PUT", "/upload2.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "user"), "got %s\n", buffer);
    ok(size == 4, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "pwd"), "got %s\n", buffer);
    ok(size == 3, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
}

static void test_basic_auth_credentials_end_session(int port)
{
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;
    char buffer[0x40];

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, "user", "pwd",
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "user"), "got %s\n", buffer);
    ok(size == 4, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "pwd"), "got %s\n", buffer);
    ok(size == 3, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == HTTP_STATUS_OK, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    /* Clear the cached credentials */
    ret = InternetSetOptionA(ses,  INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
    ok(ret, "unexpected failure %u\n", GetLastError());

    con = InternetConnectA( ses, "localhost", port, NULL, NULL,
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "PUT", "/upload2.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, ""), "got %s\n", buffer);
    ok(size == 0, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, ""), "got %s\n", buffer);
    ok(size == 0, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == HTTP_STATUS_BAD_REQUEST, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
}

static void test_basic_auth_credentials_different(int port)
{
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;
    char buffer[0x40];

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, "user", "pwd",
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "user"), "got %s\n", buffer);
    ok(size == 4, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "pwd"), "got %s\n", buffer);
    ok(size == 3, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, "user1", "pwd1",
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload3.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_USERNAME, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "user1"), "got %s\n", buffer);
    ok(size == 5, "got %u\n", size);

    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_PASSWORD, buffer, &size);
    ok(ret, "unexpected failure %u\n", GetLastError());
    ok(!strcmp(buffer, "pwd1"), "got %s\n", buffer);
    ok(size == 4, "got %u\n", size);

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
}

/*
 * Manually set the Authorization for both calls.
 */
static void test_basic_auth_credentials_manual(int port)
{
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    /* Clear the cached credentials */
    ret = InternetSetOptionA(ses,  INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
    ok(ret, "unexpected failure %u\n", GetLastError());

    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    /* Set Authorization Header */
    ret = HttpAddRequestHeadersA(req, "Authorization: Basic dXNlcjpwd2Q=\r\n", ~0u,
                            HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders Failed\n");

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    /* Show manual headers are cached. */
    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 401, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload4.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    /* Set Authorization Header */
    ret = HttpAddRequestHeadersA(req, "Authorization: Bearer dXNlcjE6cHdkMQ==\r\n", ~0u,
                            HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders Failed\n");

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
}

/*
 * Manually set the Authorization for the bearer call, which shows the cached is used.
 */
static void test_basic_auth_credentials_cached_manual(int port)
{
    HINTERNET ses, con, req;
    DWORD status, size;
    BOOL ret;

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    /* Clear the cached credentials */
    ret = InternetSetOptionA(ses,  INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
    ok(ret, "unexpected failure %u\n", GetLastError());

     con = InternetConnectA( ses, "localhost", port, "user", "pwd",
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 200, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, "HEAD", "/upload4.txt", NULL, NULL, NULL, 0, 0 );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    /* Setting an Authorization Header doesn't override the cached one. */
    ret = HttpAddRequestHeadersA(req, "Authorization: Bearer dXNlcjE6cHdkMQ==\r\n", ~0u,
                            HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders Failed\n");

    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( ret, "HttpSendRequestA failed %u\n", GetLastError() );

    status = 0xdeadbeef;
    size = sizeof(status);
    ret = HttpQueryInfoA( req, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    ok( status == 201, "got %u\n", status );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
}

static void test_async_read(int port)
{
    HINTERNET ses, con, req;
    INTERNET_BUFFERSA ib;
    char buffer[0x100];
    DWORD pending_reads;
    DWORD res, count, bytes;
    BOOL ret;

    reset_events();

    /* test asynchronous InternetReadFileEx */
    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC );
    ok( ses != NULL, "InternetOpenA failed\n" );
    pInternetSetStatusCallbackA( ses, &callback );

    SET_EXPECT( INTERNET_STATUS_HANDLE_CREATED );
    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0xdeadbeef );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );
    CHECK_NOTIFIED( INTERNET_STATUS_HANDLE_CREATED );

    SET_EXPECT( INTERNET_STATUS_HANDLE_CREATED );
    req = HttpOpenRequestA( con, "GET", "/async_read", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0xdeadbeef );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );
    CHECK_NOTIFIED( INTERNET_STATUS_HANDLE_CREATED );

    SET_OPTIONAL( INTERNET_STATUS_COOKIE_SENT );
    SET_OPTIONAL( INTERNET_STATUS_DETECTING_PROXY );
    SET_EXPECT( INTERNET_STATUS_CONNECTING_TO_SERVER );
    SET_EXPECT( INTERNET_STATUS_CONNECTED_TO_SERVER );
    SET_EXPECT( INTERNET_STATUS_SENDING_REQUEST );
    SET_EXPECT( INTERNET_STATUS_REQUEST_SENT );
    SET_EXPECT( INTERNET_STATUS_RECEIVING_RESPONSE );
    SET_EXPECT( INTERNET_STATUS_RESPONSE_RECEIVED );
    SET_OPTIONAL( INTERNET_STATUS_CLOSING_CONNECTION );
    SET_OPTIONAL( INTERNET_STATUS_CONNECTION_CLOSED );
    SET_EXPECT( INTERNET_STATUS_REQUEST_COMPLETE );

    SetLastError( 0xdeadbeef );
    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( !ret, "HttpSendRequestA unexpectedly succeeded\n" );
    ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", GetLastError() );
    WaitForSingleObject( complete_event, INFINITE );
    ok( req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error );

    CLEAR_NOTIFIED( INTERNET_STATUS_COOKIE_SENT );
    CLEAR_NOTIFIED( INTERNET_STATUS_DETECTING_PROXY );
    CHECK_NOTIFIED( INTERNET_STATUS_CONNECTING_TO_SERVER );
    CHECK_NOTIFIED( INTERNET_STATUS_CONNECTED_TO_SERVER );
    CHECK_NOTIFIED( INTERNET_STATUS_SENDING_REQUEST );
    CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_SENT );
    CHECK_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
    CHECK_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
    CLEAR_NOTIFIED( INTERNET_STATUS_CLOSING_CONNECTION );
    CLEAR_NOTIFIED( INTERNET_STATUS_CONNECTION_CLOSED );
    CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_COMPLETE );

    pending_reads = 0;
    memset( &ib, 0, sizeof(ib) );
    memset( buffer, 0, sizeof(buffer) );
    ib.dwStructSize = sizeof(ib);
    for (count = 0; count < sizeof(buffer); count += ib.dwBufferLength)
    {
        ib.lpvBuffer = buffer + count;
        ib.dwBufferLength = min(16, sizeof(buffer) - count);

        SET_EXPECT( INTERNET_STATUS_RECEIVING_RESPONSE );
        SET_EXPECT( INTERNET_STATUS_RESPONSE_RECEIVED );

        ret = InternetReadFileExA( req, &ib, 0, 0xdeadbeef );
        if (!count) /* the first part should arrive immediately */
            ok( ret, "InternetReadFileExA failed %u\n", GetLastError() );
        ros_skip_flaky
        if (!ret)
        {
            ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", GetLastError() );
            ros_skip_flaky
            CHECK_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
            SET_EXPECT( INTERNET_STATUS_REQUEST_COMPLETE );
            if (!pending_reads++)
            {
                res = WaitForSingleObject( complete_event, 0 );
                ok( res == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %u\n", res );
                SetEvent( conn_wait_event );
            }
            res = WaitForSingleObject( complete_event, INFINITE );
            ok( res == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", res );
            ok( req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error );
            todo_wine_if( pending_reads > 1 )
            ok( ib.dwBufferLength != 0, "expected ib.dwBufferLength != 0\n" );
            CHECK_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
            CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_COMPLETE );
        }

        CLEAR_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
        CLEAR_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
        if (!ib.dwBufferLength) break;
    }

    ok( pending_reads == 1, "expected 1 pending read, got %u\n", pending_reads );
    ok( !strcmp(buffer, page1), "unexpected buffer content\n" );
    close_async_handle( ses, 2 );
    ResetEvent( conn_wait_event );

    /* test asynchronous InternetReadFile */
    ses = InternetOpenA( "winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC );
    ok( ses != NULL, "InternetOpenA failed\n" );
    pInternetSetStatusCallbackA( ses, &callback );

    SET_EXPECT( INTERNET_STATUS_HANDLE_CREATED );
    con = InternetConnectA( ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0xdeadbeef );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );
    CHECK_NOTIFIED( INTERNET_STATUS_HANDLE_CREATED );

    SET_EXPECT( INTERNET_STATUS_HANDLE_CREATED );
    req = HttpOpenRequestA( con, "GET", "/async_read", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0xdeadbeef );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );
    CHECK_NOTIFIED( INTERNET_STATUS_HANDLE_CREATED );

    SET_OPTIONAL( INTERNET_STATUS_COOKIE_SENT );
    SET_OPTIONAL( INTERNET_STATUS_DETECTING_PROXY );
    SET_EXPECT( INTERNET_STATUS_CONNECTING_TO_SERVER );
    SET_EXPECT( INTERNET_STATUS_CONNECTED_TO_SERVER );
    SET_EXPECT( INTERNET_STATUS_SENDING_REQUEST );
    SET_EXPECT( INTERNET_STATUS_REQUEST_SENT );
    SET_EXPECT( INTERNET_STATUS_RECEIVING_RESPONSE );
    SET_EXPECT( INTERNET_STATUS_RESPONSE_RECEIVED );
    SET_OPTIONAL( INTERNET_STATUS_CLOSING_CONNECTION );
    SET_OPTIONAL( INTERNET_STATUS_CONNECTION_CLOSED );
    SET_EXPECT( INTERNET_STATUS_REQUEST_COMPLETE );

    SetLastError( 0xdeadbeef );
    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    ok( !ret, "HttpSendRequestA unexpectedly succeeded\n" );
    ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", GetLastError() );
    WaitForSingleObject( complete_event, INFINITE );
    ok( req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error );

    CLEAR_NOTIFIED( INTERNET_STATUS_COOKIE_SENT );
    CLEAR_NOTIFIED( INTERNET_STATUS_DETECTING_PROXY );
    CHECK_NOTIFIED( INTERNET_STATUS_CONNECTING_TO_SERVER );
    CHECK_NOTIFIED( INTERNET_STATUS_CONNECTED_TO_SERVER );
    CHECK_NOTIFIED( INTERNET_STATUS_SENDING_REQUEST );
    CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_SENT );
    CHECK_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
    CHECK_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
    CLEAR_NOTIFIED( INTERNET_STATUS_CLOSING_CONNECTION );
    CLEAR_NOTIFIED( INTERNET_STATUS_CONNECTION_CLOSED );
    CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_COMPLETE );

    pending_reads = 0;
    memset( buffer, 0, sizeof(buffer) );
    for (count = 0; count < sizeof(buffer); count += bytes)
    {
        SET_EXPECT( INTERNET_STATUS_RECEIVING_RESPONSE );
        SET_EXPECT( INTERNET_STATUS_RESPONSE_RECEIVED );

        bytes = 0xdeadbeef;
        ret = InternetReadFile( req, buffer + count, min(16, sizeof(buffer) - count), &bytes );
        if (!count) /* the first part should arrive immediately */
            ok( ret, "InternetReadFile failed %u\n", GetLastError() );
        if (!ret)
        {
            ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", GetLastError() );
            ok( bytes == 0, "expected 0, got %u\n", bytes );
            ros_skip_flaky
            CHECK_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
            SET_EXPECT( INTERNET_STATUS_REQUEST_COMPLETE );
            if (!pending_reads++)
            {
                res = WaitForSingleObject( complete_event, 0 );
                ok( res == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %u\n", res );
                SetEvent( conn_wait_event );
            }
            res = WaitForSingleObject( complete_event, INFINITE );
            ok( res == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", res );
            ok( req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error );
            ros_skip_flaky {
            todo_wine_if( pending_reads > 1 )
            ok( bytes != 0, "expected bytes != 0\n" );
            }
            ros_skip_flaky
            CHECK_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
            ros_skip_flaky
            CHECK_NOTIFIED( INTERNET_STATUS_REQUEST_COMPLETE );
        }

        CLEAR_NOTIFIED( INTERNET_STATUS_RECEIVING_RESPONSE );
        CLEAR_NOTIFIED( INTERNET_STATUS_RESPONSE_RECEIVED );
        if (!bytes) break;
    }

    ros_skip_flaky
    ok( pending_reads == 1, "expected 1 pending read, got %u\n", pending_reads );
    ok( !strcmp(buffer, page1), "unexpected buffer content\n" );
    close_async_handle( ses, 2 );
}

static void server_send_string(const char *msg)
{
    send(server_socket, msg, strlen(msg), 0);
}

static size_t server_read_data(char *buf, size_t buf_size)
{
    return recv(server_socket, buf, buf_size, 0);
}

#define server_read_request(a) _server_read_request(__LINE__,a)
static void _server_read_request(unsigned line, const char *expected_request)
{
    char buf[4000], *p;
    size_t size;

    size = server_read_data(buf, sizeof(buf) - 1);
    buf[size] = 0;
    p = strstr(buf, "\r\n");
    if(p) *p = 0;
    ok_(__FILE__,line)(p && !strcmp(buf, expected_request), "unexpected request %s\n", buf);
}

static BOOL skip_receive_notification_tests;
static DWORD received_response_size;

static void WINAPI readex_callback(HINTERNET handle, DWORD_PTR context, DWORD status, void *info, DWORD info_size)
{
    switch(status) {
    case INTERNET_STATUS_RECEIVING_RESPONSE:
        if(!skip_receive_notification_tests)
            callback(handle, context, status, info, info_size);
        break;
    case INTERNET_STATUS_RESPONSE_RECEIVED:
        if(!skip_receive_notification_tests)
            callback(handle, context, status, info, info_size);
        received_response_size = *(DWORD*)info;
        break;
    case INTERNET_STATUS_REQUEST_SENT:
        callback(handle, context, status, info, info_size);
        SetEvent(request_sent_event);
        break;
    default:
        callback(handle, context, status, info, info_size);
    }
}

static void send_socket_request(test_request_t *req, BOOL new_connection)
{
    BOOL ret;

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    if(new_connection) {
        SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
        SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    }
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    if(!skip_receive_notification_tests)
        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestA(req->request, NULL, 0, NULL, 0);
    ok(!ret, "HttpSendRequestA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", GetLastError());

    if(new_connection)
        WaitForSingleObject(server_req_rec_event, INFINITE);
    WaitForSingleObject(request_sent_event, INFINITE);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    if(new_connection) {
        CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
        CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    }
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
}

static void open_socket_request(int port, test_request_t *req, const char *verb)
{
    /* We're connecting to new socket */
    if(!verb)
        reset_events();

    req->session = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(req->session != NULL, "InternetOpenA failed\n");
    pInternetSetStatusCallbackA(req->session, readex_callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req->connection = InternetConnectA(req->session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0xdeadbeef);
    ok(req->connection != NULL, "InternetConnectA failed %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED );

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req->request = HttpOpenRequestA(req->connection, "GET", verb ? verb : "/socket",
                                    NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0xdeadbeef);
    ok(req->request != NULL, "HttpOpenRequestA failed %u\n", GetLastError());
    CHECK_NOTIFIED( INTERNET_STATUS_HANDLE_CREATED );

    send_socket_request(req, !verb);
}

static void open_read_test_request(int port, test_request_t *req, const char *response)
{
    if(!skip_receive_notification_tests)
        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);

    open_socket_request(port, req, NULL);

    if(!skip_receive_notification_tests) {
        SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
        received_response_size = 0xdeadbeef;
    }
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    server_send_string(response);
    WaitForSingleObject(complete_event, INFINITE);

    if(!skip_receive_notification_tests) {
        CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
        CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
        todo_wine
        ok(received_response_size == strlen(response), "received_response_size = %u\n", received_response_size);
    }
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);
}

#define readex_expect_sync_data_len(a,b,c,d,e,f,g) _readex_expect_sync_data_len(__LINE__,a,b,c,d,e,f,g)
static void _readex_expect_sync_data_len(unsigned line, HINTERNET req, DWORD flags, INTERNET_BUFFERSW *buf,
                                         DWORD buf_size, const char *exdata, DWORD len, DWORD expect_receive)
{
    BOOL ret;

    if(!skip_receive_notification_tests && expect_receive) {
        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
        SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
        received_response_size = 0xdeadbeef;
    }

    memset(buf->lpvBuffer, 0xff, buf_size);
    buf->dwBufferLength = buf_size;
    ret = InternetReadFileExW(req, buf, flags, 0xdeadbeef);
    ok_(__FILE__,line)(ret, "InternetReadFileExW failed: %u\n", GetLastError());
    ok_(__FILE__,line)(buf->dwBufferLength == len, "dwBufferLength = %u, expected %u\n", buf->dwBufferLength, len);
    if(len && exdata)
        ok_(__FILE__,line)(!memcmp(buf->lpvBuffer, exdata, len), "Unexpected data\n");

    if(!skip_receive_notification_tests && expect_receive) {
        CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
        CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
        ok_(__FILE__,line)(received_response_size == len, "received_response_size = %u\n", received_response_size);
    }
}

#define readex_expect_sync_data(a,b,c,d,e,f) _readex_expect_sync_data(__LINE__,a,b,c,d,e,f)
static void _readex_expect_sync_data(unsigned line, HINTERNET req, DWORD flags, INTERNET_BUFFERSW *buf,
                                     DWORD buf_size, const char *exdata, DWORD expect_receive)
{
    _readex_expect_sync_data_len(line, req, flags, buf, buf_size, exdata, strlen(exdata), expect_receive);
}

#define read_expect_sync_data_len(a,b,c,d,e) _read_expect_sync_data_len(__LINE__,a,b,c,d,e)
static void _read_expect_sync_data_len(unsigned line, HINTERNET req, void *buf, DWORD buf_size,
        const char *exdata, DWORD len)
{
    DWORD ret_size = 0xdeadbeef;
    BOOL ret;

    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    received_response_size = 0xdeadbeef;

    memset(buf, 0xff, buf_size);
    ret = InternetReadFile(req, buf, buf_size, &ret_size);
    ok_(__FILE__,line)(ret, "InternetReadFileExW failed: %u\n", GetLastError());
    ok_(__FILE__,line)(ret_size == len, "dwBufferLength = %u, expected %u\n", ret_size, len);
    if(len && exdata)
        ok_(__FILE__,line)(!memcmp(buf, exdata, len), "Unexpected data\n");

    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CLEAR_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok_(__FILE__,line)(received_response_size == len, "received_response_size = %u\n", received_response_size);
    ok_(__FILE__,line)(!req_error, "req_error = %u\n", req_error);
}

#define read_expect_sync_data(a,b,c,d) _read_expect_sync_data(__LINE__,a,b,c,d)
static void _read_expect_sync_data(unsigned line, HINTERNET req, void *buf,
        DWORD buf_size, const char *exdata)
{
    _read_expect_sync_data_len(line, req, buf, buf_size, exdata, strlen(exdata));
}

static void close_connection(void)
{
    char c;
    SetEvent(conn_wait_event);
    recv(server_socket, &c, 1, 0);
}

#define send_response_and_wait(a,b,c,d,e,f,g,h) _send_response_and_wait(__LINE__,a,b,c,d,e,f,g,h)
static void _send_response_and_wait(unsigned line, const char *response, BOOL do_close_connection,
                                    void *buf, DWORD *ret_size, const char *exdata,
                                    DWORD expected_size, DWORD expected_req_error, DWORD expected_receive_size)
{
    if(!skip_receive_notification_tests)
        SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    if(response)
        server_send_string(response);

    if(do_close_connection)
        close_connection();

    WaitForSingleObject(complete_event, INFINITE);

    if(!skip_receive_notification_tests)
        CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    if(!skip_receive_notification_tests && expected_receive_size != -1)
        todo_wine_if(received_response_size != expected_receive_size) /* FIXME! remove when wine is fixed */
        ok_(__FILE__,line)(received_response_size == expected_receive_size,
                           "received_response_size = %u\n", received_response_size);
    ok_(__FILE__,line)(req_error == expected_req_error, "req_error = %u, expected %u\n", req_error, expected_req_error);

    /* If IRF_NO_WAIT is used, buffer is not changed. */
    ok_(__FILE__,line)(*ret_size == expected_size, "dwBufferLength = %u\n", *ret_size);
    if(exdata)
        ok_(__FILE__,line)(!memcmp(buf, exdata, strlen(exdata)), "unexpected buffer data\n");
    else if(buf)
        ok_(__FILE__,line)(!*(DWORD*)buf, "buffer data changed\n");
}

#define send_response_ex_and_wait(a,b,c,d,e,f) _send_response_ex_and_wait(__LINE__,a,b,c,d,e,f)
static void _send_response_ex_and_wait(unsigned line, const char *response, BOOL close_connection,
                                       INTERNET_BUFFERSW *buf, const char *exdata, DWORD expected_req_error,
                                       DWORD expected_receive_size)
{
    _send_response_and_wait(line, response, close_connection, buf->lpvBuffer, &buf->dwBufferLength,
                            exdata, exdata ? strlen(exdata) : buf->dwBufferLength, expected_req_error,
                            expected_receive_size);
}

static void send_response_len_and_wait(unsigned len, BOOL close_connection, INTERNET_BUFFERSW *buf)
{
    char *response;

    response = HeapAlloc(GetProcessHeap(), 0, len+1);
    memset(response, 'x', len);
    response[len] = 0;
    send_response_ex_and_wait(response, close_connection, buf, NULL, 0, -1);
    HeapFree(GetProcessHeap(), 0, response);
}

#define readex_expect_async(a,b,c,d,e) _readex_expect_async(__LINE__,a,b,c,d,e)
static void _readex_expect_async(unsigned line, HINTERNET req, DWORD flags, INTERNET_BUFFERSW *buf,
        DWORD buf_size, const char *exdata)
{
    unsigned len = 0;
    BOOL ret;

    if(!skip_receive_notification_tests)
        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);

    memset(buf->lpvBuffer, 0, max(buf_size, sizeof(DWORD)));
    buf->dwBufferLength = buf_size;
    ret = InternetReadFileExW(req, buf, flags, 0xdeadbeef);
    ok_(__FILE__,line)(!ret && GetLastError() == ERROR_IO_PENDING, "InternetReadFileExW returned %x (%u)\n", ret, GetLastError());
    ok_(__FILE__,line)(buf->dwBufferLength == buf_size, "dwBufferLength = %u, expected %u\n", buf->dwBufferLength, buf_size);
    if(exdata) {
        len = strlen(exdata);
        ok_(__FILE__,line)(!memcmp(buf->lpvBuffer, exdata, len), "unexpected buffer data\n");
    }else {
        ok_(__FILE__,line)(!*(DWORD*)buf->lpvBuffer, "buffer data changed\n");
    }

    if(!skip_receive_notification_tests)
        CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
}

static void read_expect_async(HINTERNET req, void *buf, DWORD buf_size, DWORD *ret_size, const char *exdata)
{
    unsigned len = 0;
    const char *p;
    BOOL ret;

    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);

    *ret_size = 0xdeadbeef;
    memset(buf, 0, buf_size);
    ret = InternetReadFile(req, buf, buf_size, ret_size);
    ok(!ret && GetLastError() == ERROR_IO_PENDING, "InternetReadFileExW returned %x (%u)\n", ret, GetLastError());
    ok(*ret_size == 0, "dwBufferLength = %u\n", *ret_size);
    if(exdata) {
        len = strlen(exdata);
        ok(!memcmp(buf, exdata, len), "unexpected buffer data\n");
    }
    for(p = (const char*)buf + len; p < (const char*)buf + buf_size; p++) {
        if(*p)
            ok(0, "buffer data changed\n");
    }

    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
}

#define expect_data_available(a,b) _expect_data_available(__LINE__,a,b)
static DWORD _expect_data_available(unsigned line, HINTERNET req, int exsize)
{
    DWORD size = 0;
    BOOL res;

    res = InternetQueryDataAvailable(req, &size, 0, 0);
    ok_(__FILE__,line)(res, "InternetQueryDataAvailable failed: %u\n", GetLastError());
    if(exsize != -1)
        ok_(__FILE__,line)(size  == exsize, "size = %u, expected %u\n", size, exsize);

    return size;
}

#define async_query_data_available(a,b) _async_query_data_available(__LINE__,a,b)
static void _async_query_data_available(unsigned line, HINTERNET req, DWORD *size)
{
    BOOL res;

    if(!skip_receive_notification_tests)
        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);

    *size = 0xdeadbeef;
    res = InternetQueryDataAvailable(req, size, 0, 0);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_IO_PENDING,
                       "InternetQueryDataAvailable returned: %x(%u)\n", res, GetLastError());
    ok_(__FILE__,line)(!*size, "size = %u\n", *size);

    if(!skip_receive_notification_tests)
        CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
}

static void test_http_read(int port)
{
    INTERNET_BUFFERSW ib;
    test_request_t req;
    DWORD read_size;
    char buf[24000];
    DWORD avail, i;

    if(!is_ie7plus)
        return;

    memset(&ib, 0, sizeof(ib));
    ib.dwStructSize = sizeof(ib);
    ib.lpvBuffer = buf;

    trace("Testing InternetReadFileExW with IRF_ASYNC flag...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "\r\n"
                           "xx");

    readex_expect_async(req.request, IRF_ASYNC, &ib, 4, "xx");

    send_response_ex_and_wait("yy1234567890", FALSE, &ib, "xxyy", 0, 2);
    readex_expect_sync_data(req.request, IRF_ASYNC, &ib, 4, "1234", 4);
    readex_expect_sync_data(req.request, IRF_ASYNC, &ib, 5, "56789", 5);

    readex_expect_async(req.request, IRF_ASYNC, &ib, sizeof(buf), "0");
    send_response_ex_and_wait("123", TRUE, &ib, "0123", 0, 4);

    close_async_handle(req.session, 2);

    trace("Testing InternetReadFileExW with no flags...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "\r\n"
                           "xx");

    readex_expect_async(req.request, 0, &ib, 4, "xx");

    send_response_ex_and_wait("yy1234567890", FALSE, &ib, "xxyy", 0, 2);
    readex_expect_sync_data(req.request, 0, &ib, 4, "1234", 4);
    readex_expect_sync_data(req.request, 0, &ib, 5, "56789", 5);

    readex_expect_async(req.request, 0, &ib, sizeof(buf), "0");
    send_response_ex_and_wait("123", TRUE, &ib, "0123", 0, 4);

    close_async_handle(req.session, 2);

    trace("Testing InternetReadFile...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "\r\n"
                           "xx");

    read_expect_async(req.request, buf, 4, &read_size, "xx");

    send_response_and_wait("yy1234567890", FALSE, buf, &read_size, "xxyy", 4, 0, 2);
    read_expect_sync_data(req.request, buf, 4, "1234");
    read_expect_sync_data(req.request, buf, 5, "56789");

    read_expect_async(req.request, buf, sizeof(buf), &read_size, "0");
    send_response_and_wait("123", TRUE, buf, &read_size, "0123", 4, 0, 4);

    close_async_handle(req.session, 2);

    trace("Testing InternetReadFileExW with IRF_NO_WAIT flag...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "\r\n"
                           "xx");

    SET_OPTIONAL(INTERNET_STATUS_RECEIVING_RESPONSE);

    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "xx", 0);

    if(notified[INTERNET_STATUS_RECEIVING_RESPONSE]) {
        win_skip("Skipping receive notification tests on too old Windows.\n");
        skip_receive_notification_tests = TRUE;
    }
    CLEAR_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);

    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);
    send_response_ex_and_wait("1234567890", FALSE, &ib, NULL, 0, 10);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, 5, "12345", 0);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "67890", 0);

    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);
    send_response_ex_and_wait("12345", TRUE, &ib, NULL, 0, 5);

    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "12345", 0);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "", TRUE);

    close_async_handle(req.session, 2);

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "\r\n"
                           "9\r\n123456789");
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "123456789", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_ex_and_wait("\r\n1\r\na\r\n1\r\nb\r", FALSE, &ib, NULL, 0, 13);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "ab", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_ex_and_wait("\n3\r\nab", FALSE, &ib, NULL, 0, 6);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "ab", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_ex_and_wait("c", FALSE, &ib, NULL, 0, 1);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "c", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_ex_and_wait("\r\n1\r\nx\r\n0\r\n\r\n", TRUE, &ib, NULL, 0, 13);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "x", 0);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "", 0);

    close_async_handle(req.session, 2);

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Transfer-Encoding: chunked\r\n"
                           "\r\n"
                           "3\r\n123\r\n");
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "123", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_ex_and_wait("0\r\n\r\n", TRUE, &ib, NULL, 0, 5);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "", 0);

    close_async_handle(req.session, 2);

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Connection: close\r\n"
                           "\r\n");
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);
    send_response_ex_and_wait("123", TRUE, &ib, NULL, 0, 3);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "123", 0);

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    close_async_handle(req.session, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);

    trace("Testing InternetQueryDataAvailable...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "\r\n"
                           "123");
    expect_data_available(req.request, 3);
    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "123", 0);
    readex_expect_async(req.request, IRF_NO_WAIT, &ib, sizeof(buf), NULL);

    send_response_len_and_wait(20000, TRUE, &ib);
    avail = expect_data_available(req.request, -1);
    ok(avail < 17000, "avail = %u\n", avail);

    SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);
    close_async_handle(req.session, 2);
    todo_wine CHECK_NOT_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    todo_wine CHECK_NOT_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "123");

    expect_data_available(req.request, 3);
    readex_expect_sync_data(req.request, 0, &ib, 3, "123", 0);

    async_query_data_available(req.request, &read_size);
    send_response_and_wait("1234567890", FALSE, NULL, &read_size, NULL, 10, 10, 10);

    readex_expect_sync_data(req.request, 0, &ib, 9, "123456789", 0);
    expect_data_available(req.request, 1);
    readex_expect_sync_data(req.request, 0, &ib, 1, "0", 0);

    async_query_data_available(req.request, &read_size);
    send_response_and_wait("1234567890", FALSE, NULL, &read_size, NULL, 10, 10, 10);
    expect_data_available(req.request, 10);
    for(i = 0; i < 10; i++)
        server_send_string("x");
    expect_data_available(req.request, 10);

    readex_expect_async(req.request, IRF_ASYNC, &ib, 21, "1234567890");
    send_response_ex_and_wait("X", FALSE, &ib, "1234567890xxxxxxxxxxX", 0, 11);
    async_query_data_available(req.request, &read_size);

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    send_response_and_wait(NULL, TRUE, NULL, &read_size, NULL, 0, 0, 0);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);

    close_async_handle(req.session, 2);

    skip_receive_notification_tests = FALSE;
}

static void test_connection_break(int port)
{
    INTERNET_BUFFERSW ib;
    test_request_t req;
    char buf[24000];

    if(!is_ie7plus)
        return;

    memset(&ib, 0, sizeof(ib));
    ib.dwStructSize = sizeof(ib);
    ib.lpvBuffer = buf;

    trace("Testing InternetReadFileExW on broken connection...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Content-Length: 10000\r\n"
                           "\r\n"
                           "xx");

    /* close connection and make sure that it's closed on handle release. */
    close_connection();
    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    close_async_handle(req.session, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
}

static void test_long_url(int port)
{
    char long_path[INTERNET_MAX_PATH_LENGTH*2] = "/echo_request?";
    char buf[sizeof(long_path)*2], url[sizeof(buf)];
    test_request_t req;
    DWORD size, len;
    BOOL ret;

    if(!is_ie7plus)
        return;

    memset(long_path+strlen(long_path), 'x', sizeof(long_path)-strlen(long_path));
    long_path[sizeof(long_path)-1] = 0;
    open_simple_request(&req, "localhost", port, NULL, long_path);

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    test_status_code(req.request, 200);

    receive_simple_request(req.request, buf, sizeof(buf));
    ok(strstr(buf, long_path) != NULL, "long pathnot found in %s\n", buf);

    sprintf(url, "http://localhost:%u%s", port, long_path);

    size = sizeof(buf);
    ret = InternetQueryOptionA(req.request, INTERNET_OPTION_URL, buf, &size);
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_URL) failed: %u\n", GetLastError());
    len = strlen(url);
    ok(size == len, "size = %u, expected %u\n", size, len);
    ok(!strcmp(buf, url), "Wrong URL %s, expected %s\n", buf, url);

    close_request(&req);
}

static void test_persistent_connection(int port)
{
    INTERNET_BUFFERSW ib;
    test_request_t req;
    char buf[24000];

    if(!is_ie7plus)
        return;

    memset(&ib, 0, sizeof(ib));
    ib.dwStructSize = sizeof(ib);
    ib.lpvBuffer = buf;

    skip_receive_notification_tests = TRUE;

    trace("Testing persistent connection...\n");

    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "xx");
    readex_expect_sync_data(req.request, IRF_ASYNC, &ib, 4, "xx", 0);
    close_async_handle(req.session, 2);

    open_socket_request(port, &req, "/test_simple_chunked");
    server_read_request("GET /test_simple_chunked HTTP/1.1");

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    server_send_string("HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "2\r\nab\r\n");
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "ab", 0);
    readex_expect_async(req.request, IRF_ASYNC, &ib, sizeof(buf), NULL);
    send_response_ex_and_wait("3\r\nabc\r\n0\r\n\r\n", FALSE, &ib, "abc", 0, 13);
    close_async_handle(req.session, 2);

    open_socket_request(port, &req, "/chunked");
    server_read_request("GET /chunked HTTP/1.1");

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    server_send_string("HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "2\r\nab\r\n");
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, 3, "ab", 0);
    readex_expect_async(req.request, IRF_ASYNC, &ib, 3, NULL);
    send_response_ex_and_wait("3\r\nabc\r\n", FALSE, &ib, "abc", 0, 13);

    /* send another request on the same request handle, it must drain remaining last chunk marker */
    server_send_string("0\r\n\r\n");

    send_socket_request(&req, FALSE);
    server_read_request("GET /chunked HTTP/1.1");

    ResetEvent(complete_event);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    server_send_string("HTTP/1.1 201 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: keep-alive\r\n"
                       "\r\n");
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    test_status_code(req.request, 201);
    close_async_handle(req.session, 2);

    /* the connection is still valid */
    open_socket_request(port, &req, "/another_chunked");
    server_read_request("GET /another_chunked HTTP/1.1");

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    server_send_string("HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "2\r\nab\r\n");
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    readex_expect_sync_data(req.request, IRF_NO_WAIT, &ib, sizeof(buf), "ab", 0);
    readex_expect_async(req.request, IRF_ASYNC, &ib, sizeof(buf), NULL);

    /* we're missing trailing '\n'; the connection can't be drained without blocking,
     * so it will be closed */
    send_response_ex_and_wait("3\r\nabc\r\n0\r\n\r", FALSE, &ib, "abc", 0, 13);

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    close_async_handle(req.session, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);

    close_connection();
    skip_receive_notification_tests = FALSE;
}

static void test_redirect(int port)
{
    char buf[4000], expect_url[INTERNET_MAX_URL_LENGTH];
    INTERNET_BUFFERSW ib;
    test_request_t req;

    if(!is_ie7plus)
        return;

    skip_receive_notification_tests = TRUE;

    memset(&ib, 0, sizeof(ib));
    ib.dwStructSize = sizeof(ib);
    ib.lpvBuffer = buf;

    trace("Testing redirection...\n");

    open_socket_request(port, &req, NULL);

    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_EXPECT(INTERNET_STATUS_REDIRECT);
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);

    server_send_string("HTTP/1.1 302 Found\r\n"
                       "Server: winetest\r\n"
                       "Location: test_redirection\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n");

    server_read_request("GET /test_redirection HTTP/1.1");

    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);

    sprintf(expect_url, "http://localhost:%u/test_redirection", port);
    test_request_url(req.request, expect_url);

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    server_send_string("HTTP/1.1 200 OK\r\n"
                       "Server: winetest\r\n"
                       "Content-Length: 3\r\n"
                       "\r\n"
                       "xxx");

    WaitForSingleObject(complete_event, INFINITE);

    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_REDIRECT);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    test_status_code(req.request, 200);

    close_connection();
    close_async_handle(req.session, 2);

    trace("Test redirect to non-http URL...\n");

    open_socket_request(port, &req, NULL);

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    server_send_string("HTTP/1.1 302 Found\r\n"
                       "Server: winetest\r\n"
                       "Location: test:non:http/url\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n");

    WaitForSingleObject(complete_event, INFINITE);

    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", req_error);

    sprintf(expect_url, "http://localhost:%u/socket", port);
    test_request_url(req.request, expect_url);
    test_status_code(req.request, 302);

    close_connection();
    close_async_handle(req.session, 2);

    trace("Test redirect to http URL with no host name...\n");

    open_socket_request(port, &req, NULL);

    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);

    server_send_string("HTTP/1.1 302 Found\r\n"
                       "Server: winetest\r\n"
                       "Location: http:///nohost\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n");

    WaitForSingleObject(complete_event, INFINITE);

    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    ok(req_error == ERROR_INTERNET_INVALID_URL, "expected ERROR_INTERNET_INVALID_URL, got %u\n", req_error);

    sprintf(expect_url, "http://localhost:%u/socket", port);
    test_request_url(req.request, expect_url);
    test_status_code(req.request, 302);

    close_connection();
    close_async_handle(req.session, 2);

    skip_receive_notification_tests = FALSE;
}

static void test_remove_dot_segments(int port)
{
    test_request_t req;
    BOOL ret;

    open_simple_request(&req, "localhost", port, NULL, "/A/../B/./C/../../test_remove_dot_segments");

    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    test_status_code(req.request, 200);

    close_request(&req);
}

struct large_test
{
    DWORD64 content_length;
    BOOL ret;
};

static void test_large_content(int port)
{
    struct large_test tests[] = {
        { 0,                   TRUE },
        { UINT_MAX-1,          TRUE },
        { UINT_MAX,            TRUE },
        { (DWORD64)UINT_MAX+1, FALSE },
        { ~0,                  FALSE },
    };
    test_request_t req;
    DWORD sizelen, len;
    DWORD read_size;
    DWORD64 len64;
    char buf[16];
    BOOL ret;
    size_t i;

    open_simple_request(&req, "localhost", port, "HEAD", "/test_large_content");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        content_length = tests[i].content_length;
        ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
        ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

        len = ~0;
        sizelen = sizeof(len);
        SetLastError(0xdeadbeef);
        ret = HttpQueryInfoA(req.request, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,
                             &len, &sizelen, 0);
        if (tests[i].ret)
        {
            ok(ret, "HttpQueryInfo should have succeeded\n");
            ok(GetLastError() == ERROR_SUCCESS ||
                broken(GetLastError() == 0xdeadbeef), /* xp, 2k8, vista */
                "expected ERROR_SUCCESS, got %x\n", GetLastError());
            ok(len == (DWORD)tests[i].content_length, "expected %u, got %u\n",
                (DWORD)tests[i].content_length, len);
        }
        else
        {
            ok(!ret, "HttpQueryInfo should have failed\n");
            ok(GetLastError() == ERROR_HTTP_INVALID_HEADER,
                "expected ERROR_HTTP_INVALID_HEADER, got %x\n", GetLastError());
            ok(len == ~0, "expected ~0, got %u\n", len);
        }
        ok(sizelen == sizeof(DWORD), "sizelen %u\n", sizelen);
    }

    /* test argument size */
    len64 = ~0;
    sizelen = sizeof(len64);
    SetLastError(0xdeadbeef);
    ret = HttpQueryInfoA(req.request, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,
                         &len64, &len, 0);
    ok(!ret, "HttpQueryInfo should have failed\n");
    ok(GetLastError() == ERROR_HTTP_INVALID_HEADER,
        "expected ERROR_HTTP_INVALID_HEADER, got %x\n", GetLastError());
    ok(sizelen == sizeof(DWORD64), "sizelen %u\n", sizelen);
    ok(len64 == ~0, "len64 %x%08x\n", (DWORD)(len64 >> 32), (DWORD)len64);

    close_request(&req);

    /* test internal use of HttpQueryInfo on large size */
    open_read_test_request(port, &req,
                           "HTTP/1.1 200 OK\r\n"
                           "Server: winetest\r\n"
                           "Content-Length: 4294967296\r\n"
                           "\r\n"
                           "xx");
    read_expect_async(req.request, buf, 4, &read_size, "xx");
    send_response_and_wait("yy1234567890", FALSE, buf, &read_size, "xxyy", 4, 0, 2);
    read_expect_sync_data(req.request, buf, 10, "1234567890");

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    close_async_handle(req.session, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    close_connection();
}

static void test_http_connection(void)
{
    struct server_info si;
    HANDLE hThread;
    DWORD id = 0, r;

    si.hEvent = CreateEventW(NULL, 0, 0, NULL);
    si.port = 7531;

    hThread = CreateThread(NULL, 0, server_thread, &si, 0, &id);
    ok( hThread != NULL, "create thread failed\n");

    r = WaitForSingleObject(si.hEvent, 10000);
    ok (r == WAIT_OBJECT_0, "failed to start wininet test server\n");
    if (r != WAIT_OBJECT_0)
    {
        CloseHandle(hThread);
        return;
    }

    test_basic_request(si.port, "GET", "/test1");
    test_proxy_indirect(si.port);
    test_proxy_direct(si.port);
    test_header_handling_order(si.port);
    test_basic_request(si.port, "POST", "/test5");
    test_basic_request(si.port, "RPC_IN_DATA", "/test5");
    test_basic_request(si.port, "RPC_OUT_DATA", "/test5");
    test_basic_request(si.port, "GET", "/test6");
    test_basic_request(si.port, "GET", "/testF");
    test_connection_header(si.port);
    test_header_override(si.port);
    test_cookie_header(si.port);
    test_basic_authentication(si.port);
    test_invalid_response_headers(si.port);
    test_response_without_headers(si.port);
    test_HttpQueryInfo(si.port);
    test_HttpSendRequestW(si.port);
    test_options(si.port);
    test_no_content(si.port);
    test_conn_close(si.port);
    test_no_cache(si.port);
    test_cache_read_gzipped(si.port);
    test_http_status(si.port);
    test_premature_disconnect(si.port);
    test_connection_closing(si.port);
    test_cache_control_verb(si.port);
    test_successive_HttpSendRequest(si.port);
    test_head_request(si.port);
    test_request_content_length(si.port);
    test_accept_encoding(si.port);
    test_basic_auth_credentials_reuse(si.port);
    test_basic_auth_credentials_end_session(si.port);
    test_basic_auth_credentials_different(si.port);
    test_basic_auth_credentials_manual(si.port);
    test_basic_auth_credentials_cached_manual(si.port);
    test_async_read(si.port);
    test_http_read(si.port);
    test_connection_break(si.port);
    test_long_url(si.port);
#ifdef __REACTOS__
if (!winetest_interactive)
{
    skip("Skipping test_redirect and test_persistent_connection due to hang. See ROSTESTS-294.\n");
}
else
{
#endif
    test_redirect(si.port);
    test_persistent_connection(si.port);
#ifdef __REACTOS__
}
#endif
    test_remove_dot_segments(si.port);
    test_large_content(si.port);

    /* send the basic request again to shutdown the server thread */
    test_basic_request(si.port, "GET", "/quit");

    r = WaitForSingleObject(hThread, 3000);
    ok( r == WAIT_OBJECT_0, "thread wait failed\n");
    CloseHandle(hThread);
}

static void release_cert_info(INTERNET_CERTIFICATE_INFOA *info)
{
    LocalFree(info->lpszSubjectInfo);
    LocalFree(info->lpszIssuerInfo);
    LocalFree(info->lpszProtocolName);
    LocalFree(info->lpszSignatureAlgName);
    LocalFree(info->lpszEncryptionAlgName);
}

typedef struct {
    const char *ex_subject;
    const char *ex_issuer;
} cert_struct_test_t;

static const cert_struct_test_t test_winehq_org_cert = {
    "US\r\n"
    "55114\r\n"
    "MN\r\n"
    "Saint Paul\r\n"
    "Ste 120\r\n"
    "700 Raymond Ave\r\n"
    "CodeWeavers\r\n"
    "IT\r\n"
    "Secure Link SSL Wildcard\r\n"
    "*.winehq.org",

    "US\r\n"
    "VA\r\n"
    "Herndon\r\n"
    "Network Solutions L.L.C.\r\n"
    "Network Solutions OV Server CA 2"
};

static const cert_struct_test_t test_winehq_com_cert = {
    "US\r\n"
    "Minnesota\r\n"
    "Saint Paul\r\n"
    "WineHQ\r\n"
    "IT\r\n"
    "test.winehq.com\r\n"
    "webmaster@winehq.org",

    "US\r\n"
    "Minnesota\r\n"
    "Saint Paul\r\n"
    "WineHQ\r\n"
    "IT\r\n"
    "test.winehq.com\r\n"
    "webmaster@winehq.org"
};

static void test_cert_struct(HINTERNET req, const cert_struct_test_t *test)
{
    INTERNET_CERTIFICATE_INFOA info;
    DWORD size;
    BOOL res;

    memset(&info, 0x5, sizeof(info));

    size = sizeof(info);
    res = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, &info, &size);
    if (!res)
    {
        win_skip("Querying INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT failed, skipping tests\n");
        return;
    }

    ok(res, "InternetQueryOption failed: %u\n", GetLastError());
    ok(size == sizeof(info), "size = %u\n", size);

    ok(!strcmp(info.lpszSubjectInfo, test->ex_subject), "lpszSubjectInfo = %s\n", info.lpszSubjectInfo);
    ok(!strcmp(info.lpszIssuerInfo, test->ex_issuer), "lpszIssuerInfo = %s\n", info.lpszIssuerInfo);
    ok(!info.lpszSignatureAlgName, "lpszSignatureAlgName = %s\n", info.lpszSignatureAlgName);
    ok(!info.lpszEncryptionAlgName, "lpszEncryptionAlgName = %s\n", info.lpszEncryptionAlgName);
    ok(!info.lpszProtocolName, "lpszProtocolName = %s\n", info.lpszProtocolName);
    ok(info.dwKeySize >= 128 && info.dwKeySize <= 256, "dwKeySize = %u\n", info.dwKeySize);

    release_cert_info(&info);
}

#define test_security_info(a,b,c) _test_security_info(__LINE__,a,b,c)
static void _test_security_info(unsigned line, const char *urlc, DWORD error, DWORD ex_flags)
{
    char url[INTERNET_MAX_URL_LENGTH];
    const CERT_CHAIN_CONTEXT *chain;
    DWORD flags;
    BOOL res;

    if(!pInternetGetSecurityInfoByURLA) {
        win_skip("pInternetGetSecurityInfoByURLA not available\n");
        return;
    }

    strcpy(url, urlc);
    chain = (void*)0xdeadbeef;
    flags = 0xdeadbeef;
    res = pInternetGetSecurityInfoByURLA(url, &chain, &flags);
    if(error == ERROR_SUCCESS) {
        ok_(__FILE__,line)(res, "InternetGetSecurityInfoByURLA failed: %u\n", GetLastError());
        ok_(__FILE__,line)(chain != NULL, "chain = NULL\n");
        ok_(__FILE__,line)(flags == ex_flags, "flags = %x\n", flags);
        CertFreeCertificateChain(chain);

        SetLastError(0xdeadbeef);
        res = pInternetGetSecurityInfoByURLA(url, NULL, NULL);
        ok_(__FILE__,line)(!res && GetLastError() == ERROR_INVALID_PARAMETER,
                           "InternetGetSecurityInfoByURLA returned: %x(%u)\n", res, GetLastError());

        res = pInternetGetSecurityInfoByURLA(url, &chain, NULL);
        ok_(__FILE__,line)(res, "InternetGetSecurityInfoByURLA failed: %u\n", GetLastError());
        CertFreeCertificateChain(chain);

        res = pInternetGetSecurityInfoByURLA(url, NULL, &flags);
        ok_(__FILE__,line)(res, "InternetGetSecurityInfoByURLA failed: %u\n", GetLastError());
    }else {
        ok_(__FILE__,line)(!res && GetLastError() == error,
                           "InternetGetSecurityInfoByURLA returned: %x(%u), expected %u\n", res, GetLastError(), error);
    }
}

#define test_secflags_option(a,b,c) _test_secflags_option(__LINE__,a,b,c)
static void _test_secflags_option(unsigned line, HINTERNET req, DWORD ex_flags, DWORD opt_flags)
{
    DWORD flags, size;
    BOOL res;

    flags = 0xdeadbeef;
    size = sizeof(flags);
    res = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_FLAGS, &flags, &size);
    ok_(__FILE__,line)(res, "InternetQueryOptionW(INTERNET_OPTION_SECURITY_FLAGS) failed: %u\n", GetLastError());
    ok_(__FILE__,line)((flags & ~opt_flags) == ex_flags, "INTERNET_OPTION_SECURITY_FLAGS flags = %x, expected %x\n",
                       flags, ex_flags);

    /* Option 98 is undocumented and seems to be the same as INTERNET_OPTION_SECURITY_FLAGS */
    flags = 0xdeadbeef;
    size = sizeof(flags);
    res = InternetQueryOptionW(req, 98, &flags, &size);
    ok_(__FILE__,line)(res, "InternetQueryOptionW(98) failed: %u\n", GetLastError());
    ok_(__FILE__,line)((flags & ~opt_flags) == ex_flags, "INTERNET_OPTION_SECURITY_FLAGS(98) flags = %x, expected %x\n",
                       flags, ex_flags);
}

#define set_secflags(a,b,c) _set_secflags(__LINE__,a,b,c)
static void _set_secflags(unsigned line, HINTERNET req, BOOL use_undoc, DWORD flags)
{
    BOOL res;

    res = InternetSetOptionW(req, use_undoc ? 99 : INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    ok_(__FILE__,line)(res, "InternetSetOption(INTERNET_OPTION_SECURITY_FLAGS) failed: %u\n", GetLastError());
}

static void test_security_flags(void)
{
    INTERNET_CERTIFICATE_INFOA *cert;
    HINTERNET ses, conn, req;
    DWORD size, flags;
    char buf[100];
    BOOL res;

    trace("Testing security flags...\n");
    reset_events();

    ses = InternetOpenA("WineTest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(ses != NULL, "InternetOpen failed\n");

    pInternetSetStatusCallbackA(ses, &callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    conn = InternetConnectA(ses, "test.winehq.com", INTERNET_DEFAULT_HTTPS_PORT,
                            NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0xdeadbeef);
    ok(conn != NULL, "InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(conn, "GET", "/tests/hello.html", NULL, NULL, NULL,
                          INTERNET_FLAG_SECURE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE,
                          0xdeadbeef);
    ok(req != NULL, "HttpOpenRequest failed\n");
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    flags = 0xdeadbeef;
    size = sizeof(flags);
    res = InternetQueryOptionW(req, 98, &flags, &size);
    if(!res && GetLastError() == ERROR_INVALID_PARAMETER) {
        win_skip("Incomplete security flags support, skipping\n");

        close_async_handle(ses, 2);
        return;
    }

    test_secflags_option(req, 0, 0);
    test_security_info("https://test.winehq.com/data/some_file.html?q", ERROR_INTERNET_ITEM_NOT_FOUND, 0);

    set_secflags(req, TRUE, SECURITY_FLAG_IGNORE_REVOCATION);
    test_secflags_option(req, SECURITY_FLAG_IGNORE_REVOCATION, 0);

    set_secflags(req, TRUE, SECURITY_FLAG_IGNORE_CERT_CN_INVALID);
    test_secflags_option(req, SECURITY_FLAG_IGNORE_REVOCATION|SECURITY_FLAG_IGNORE_CERT_CN_INVALID, 0);

    set_secflags(req, FALSE, SECURITY_FLAG_IGNORE_UNKNOWN_CA);
    test_secflags_option(req, SECURITY_FLAG_IGNORE_UNKNOWN_CA|SECURITY_FLAG_IGNORE_REVOCATION|SECURITY_FLAG_IGNORE_CERT_CN_INVALID, 0);

    flags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_SECURE;
    res = InternetSetOptionW(req, 99, &flags, sizeof(flags));
    ok(!res && GetLastError() == ERROR_INTERNET_OPTION_NOT_SETTABLE, "InternetSetOption(99) failed: %u\n", GetLastError());

    SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
    SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION); /* IE11 calls it, it probably reconnects. */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER); /* IE11 */
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);
    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "HttpSendRequest failed: %u\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %d\n", req_error);

    CHECK_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
    CHECK_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTING_TO_SERVER, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTED_TO_SERVER, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);

    test_request_flags(req, 0);
    test_secflags_option(req, SECURITY_FLAG_SECURE|SECURITY_FLAG_IGNORE_UNKNOWN_CA
            |SECURITY_FLAG_IGNORE_REVOCATION|SECURITY_FLAG_IGNORE_CERT_CN_INVALID|SECURITY_FLAG_STRENGTH_STRONG, 0);

    res = InternetReadFile(req, buf, sizeof(buf), &size);
    ok(res, "InternetReadFile failed: %u\n", GetLastError());
    ok(size, "size = 0\n");

    /* Collect all existing persistent connections */
    res = InternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(res, "InternetSetOption(INTERNET_OPTION_END_BROWSER_SESSION) failed: %u\n", GetLastError());

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(conn, "GET", "/tests/hello.html", NULL, NULL, NULL,
                          INTERNET_FLAG_SECURE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE,
                          0xdeadbeef);
    ok(req != NULL, "HttpOpenRequest failed\n");
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    flags = INTERNET_ERROR_MASK_COMBINED_SEC_CERT|INTERNET_ERROR_MASK_LOGIN_FAILURE_DISPLAY_ENTITY_BODY;
    res = InternetSetOptionA(req, INTERNET_OPTION_ERROR_MASK, (void*)&flags, sizeof(flags));
    ok(res, "InternetQueryOption(INTERNET_OPTION_ERROR_MASK failed: %u\n", GetLastError());

    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION); /* IE11 calls it, it probably reconnects. */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER); /* IE11 */
    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "HttpSendRequest failed: %u\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_INTERNET_SEC_CERT_ERRORS,
       "req_error = %d\n", req_error);

    size = 0;
    res = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, NULL, &size);
    ok(res || GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InternetQueryOption failed: %d\n", GetLastError());
    ok(size == sizeof(INTERNET_CERTIFICATE_INFOA), "size = %u\n", size);
    cert = HeapAlloc(GetProcessHeap(), 0, size);
    cert->lpszSubjectInfo = NULL;
    cert->lpszIssuerInfo = NULL;
    cert->lpszSignatureAlgName = (char *)0xdeadbeef;
    cert->lpszEncryptionAlgName = (char *)0xdeadbeef;
    cert->lpszProtocolName = (char *)0xdeadbeef;
    cert->dwKeySize = 0xdeadbeef;
    res = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, cert, &size);
    ok(res, "InternetQueryOption failed: %u\n", GetLastError());
    if (res)
    {
        ok(cert->lpszSubjectInfo && strlen(cert->lpszSubjectInfo) > 1, "expected a non-empty subject name\n");
        ok(cert->lpszIssuerInfo && strlen(cert->lpszIssuerInfo) > 1, "expected a non-empty issuer name\n");
        ok(!cert->lpszSignatureAlgName, "unexpected signature algorithm name\n");
        ok(!cert->lpszEncryptionAlgName, "unexpected encryption algorithm name\n");
        ok(!cert->lpszProtocolName, "unexpected protocol name\n");
        ok(cert->dwKeySize != 0xdeadbeef, "unexpected key size\n");

        LocalFree(cert->lpszSubjectInfo);
        LocalFree(cert->lpszIssuerInfo);
    }
    HeapFree(GetProcessHeap(), 0, cert);

    ros_skip_flaky
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTING_TO_SERVER, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTED_TO_SERVER, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);

    if(req_error != ERROR_INTERNET_SEC_CERT_ERRORS) {
        win_skip("Unexpected cert errors %u, skipping security flags tests\n", req_error);

        close_async_handle(ses, 3);
        return;
    }

    size = sizeof(buf);
    res = HttpQueryInfoA(req, HTTP_QUERY_CONTENT_ENCODING, buf, &size, 0);
    ok(!res && GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "HttpQueryInfoA(HTTP_QUERY_CONTENT_ENCODING) failed: %u\n", GetLastError());

    test_request_flags(req, 8);
    /* IE11 finds both rev failure and invalid CA. Previous versions required rev failure
       to be ignored before invalid CA was reported. */
    test_secflags_option(req, _SECURITY_FLAG_CERT_INVALID_CA, _SECURITY_FLAG_CERT_REV_FAILED);

    set_secflags(req, FALSE, SECURITY_FLAG_IGNORE_REVOCATION);
    test_secflags_option(req, _SECURITY_FLAG_CERT_INVALID_CA|SECURITY_FLAG_IGNORE_REVOCATION, _SECURITY_FLAG_CERT_REV_FAILED);

    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "HttpSendRequest failed: %u\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_INTERNET_INVALID_CA || req_error == ERROR_INTERNET_SEC_CERT_ERRORS, "req_error = %d\n", req_error);

    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);

    test_request_flags(req, INTERNET_REQFLAG_NO_HEADERS);
    test_secflags_option(req, SECURITY_FLAG_IGNORE_REVOCATION|_SECURITY_FLAG_CERT_INVALID_CA, 0);
    test_security_info("https://test.winehq.com/data/some_file.html?q", ERROR_INTERNET_ITEM_NOT_FOUND, 0);

    set_secflags(req, FALSE, SECURITY_FLAG_IGNORE_UNKNOWN_CA);
    test_secflags_option(req, _SECURITY_FLAG_CERT_INVALID_CA
            |SECURITY_FLAG_IGNORE_REVOCATION|SECURITY_FLAG_IGNORE_UNKNOWN_CA, 0);
    test_http_version(req);

    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION); /* IE11 calls it, it probably reconnects. */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER); /* IE11 */
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);
    SET_OPTIONAL(INTERNET_STATUS_DETECTING_PROXY);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "HttpSendRequest failed: %u\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %d\n", req_error);

    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTING_TO_SERVER, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTED_TO_SERVER, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);
    CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);

    test_request_flags(req, 0);
    test_secflags_option(req, SECURITY_FLAG_SECURE|SECURITY_FLAG_IGNORE_UNKNOWN_CA|SECURITY_FLAG_IGNORE_REVOCATION
            |SECURITY_FLAG_STRENGTH_STRONG|_SECURITY_FLAG_CERT_INVALID_CA, 0);

    test_cert_struct(req, &test_winehq_com_cert);
    test_security_info("https://test.winehq.com/data/some_file.html?q", 0,
            _SECURITY_FLAG_CERT_INVALID_CA);

    res = InternetReadFile(req, buf, sizeof(buf), &size);
    ok(res, "InternetReadFile failed: %u\n", GetLastError());
    ok(size, "size = 0\n");

    close_async_handle(ses, 3);

    /* Collect all existing persistent connections */
    res = InternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(res, "InternetSetOption(INTERNET_OPTION_END_BROWSER_SESSION) failed: %u\n", GetLastError());

    /* Make another request, without setting security flags */

    ses = InternetOpenA("WineTest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(ses != NULL, "InternetOpen failed\n");

    pInternetSetStatusCallbackA(ses, &callback);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    conn = InternetConnectA(ses, "test.winehq.com", INTERNET_DEFAULT_HTTPS_PORT,
                            NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0xdeadbeef);
    ok(conn != NULL, "InternetConnect failed with error %u\n", GetLastError());
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    req = HttpOpenRequestA(conn, "GET", "/tests/hello.html", NULL, NULL, NULL,
                          INTERNET_FLAG_SECURE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE,
                          0xdeadbeef);
    ok(req != NULL, "HttpOpenRequest failed\n");
    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);

    test_secflags_option(req, SECURITY_FLAG_SECURE|SECURITY_FLAG_IGNORE_UNKNOWN_CA|SECURITY_FLAG_STRENGTH_STRONG
            |SECURITY_FLAG_IGNORE_REVOCATION|_SECURITY_FLAG_CERT_INVALID_CA, 0);
    test_http_version(req);

    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_OPTIONAL(INTERNET_STATUS_CLOSING_CONNECTION); /* IE11 calls it, it probably reconnects. */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTION_CLOSED); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTING_TO_SERVER); /* IE11 */
    SET_OPTIONAL(INTERNET_STATUS_CONNECTED_TO_SERVER); /* IE11 */
    SET_EXPECT(INTERNET_STATUS_SENDING_REQUEST);
    SET_EXPECT(INTERNET_STATUS_REQUEST_SENT);
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
    SET_OPTIONAL(INTERNET_STATUS_COOKIE_SENT);

    res = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "HttpSendRequest failed: %u\n", GetLastError());

    WaitForSingleObject(complete_event, INFINITE);
    ok(req_error == ERROR_SUCCESS, "req_error = %d\n", req_error);

    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTING_TO_SERVER, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTED_TO_SERVER, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CHECK_NOTIFIED(INTERNET_STATUS_SENDING_REQUEST);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_SENT);
    CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
    CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    CLEAR_NOTIFIED(INTERNET_STATUS_COOKIE_SENT);

    test_request_flags(req, 0);
    test_secflags_option(req, SECURITY_FLAG_SECURE|SECURITY_FLAG_IGNORE_UNKNOWN_CA|SECURITY_FLAG_STRENGTH_STRONG
            |SECURITY_FLAG_IGNORE_REVOCATION|_SECURITY_FLAG_CERT_INVALID_CA, 0);

    res = InternetReadFile(req, buf, sizeof(buf), &size);
    ok(res, "InternetReadFile failed: %u\n", GetLastError());
    ok(size, "size = 0\n");

    close_async_handle(ses, 2);

    test_security_info("http://test.winehq.com/data/some_file.html?q", ERROR_INTERNET_ITEM_NOT_FOUND, 0);
    test_security_info("file:///c:/dir/file.txt", ERROR_INTERNET_ITEM_NOT_FOUND, 0);
    test_security_info("xxx:///c:/dir/file.txt", ERROR_INTERNET_ITEM_NOT_FOUND, 0);
}

static void test_secure_connection(void)
{
    static const WCHAR gizmo5[] = {'G','i','z','m','o','5',0};
    static const WCHAR testsite[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR get[] = {'G','E','T',0};
    static const WCHAR testpage[] = {'/','t','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};
    HINTERNET ses, con, req;
    DWORD size, flags;
    INTERNET_CERTIFICATE_INFOA *certificate_structA = NULL;
    INTERNET_CERTIFICATE_INFOW *certificate_structW = NULL;
    BOOL ret;

    ses = InternetOpenA("Gizmo5", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "test.winehq.org",
                          INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL,
                          INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, "GET", "/tests/hello.html", NULL, NULL, NULL,
                          INTERNET_FLAG_SECURE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret || broken(GetLastError() == ERROR_INTERNET_CANNOT_CONNECT),
                     "HttpSendRequest failed: %d\n", GetLastError());
    if (!ret)
    {
        win_skip("Cannot connect to https.\n");
        goto done;
    }

    size = sizeof(flags);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_FLAGS, &flags, &size);
    ok(ret, "InternetQueryOption failed: %d\n", GetLastError());
    ok(flags & SECURITY_FLAG_SECURE, "expected secure flag to be set\n");

    test_cert_struct(req, &test_winehq_org_cert);

    /* Querying the same option through InternetQueryOptionW still results in
     * ASCII strings being returned.
     */
    size = 0;
    ret = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                               NULL, &size);
    ok(ret || GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InternetQueryOption failed: %d\n", GetLastError());
    ok(size == sizeof(INTERNET_CERTIFICATE_INFOW), "size = %d\n", size);
    certificate_structW = HeapAlloc(GetProcessHeap(), 0, size);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                              certificate_structW, &size);
    certificate_structA = (INTERNET_CERTIFICATE_INFOA *)certificate_structW;
    ok(ret, "InternetQueryOption failed: %d\n", GetLastError());
    if (ret)
    {
        ok(certificate_structA->lpszSubjectInfo &&
           strlen(certificate_structA->lpszSubjectInfo) > 1,
           "expected a non-empty subject name\n");
        ok(certificate_structA->lpszIssuerInfo &&
           strlen(certificate_structA->lpszIssuerInfo) > 1,
           "expected a non-empty issuer name\n");
        ok(!certificate_structA->lpszSignatureAlgName,
           "unexpected signature algorithm name\n");
        ok(!certificate_structA->lpszEncryptionAlgName,
           "unexpected encryption algorithm name\n");
        ok(!certificate_structA->lpszProtocolName,
           "unexpected protocol name\n");
        ok(certificate_structA->dwKeySize, "expected a non-zero key size\n");
        release_cert_info(certificate_structA);
    }
    HeapFree(GetProcessHeap(), 0, certificate_structW);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);

    /* Repeating the tests with the W functions has the same result: */
    ses = InternetOpenW(gizmo5, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectW(ses, testsite,
                          INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL,
                          INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestW(con, get, testpage, NULL, NULL, NULL,
                          INTERNET_FLAG_SECURE|INTERNET_FLAG_RELOAD, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %d\n", GetLastError());

    size = sizeof(flags);
    ret = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_FLAGS, &flags, &size);
    ok(ret, "InternetQueryOption failed: %d\n", GetLastError());
    ok(flags & SECURITY_FLAG_SECURE, "expected secure flag to be set, got %x\n", flags);

    ret = InternetQueryOptionA(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                               NULL, &size);
    ok(ret || GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InternetQueryOption failed: %d\n", GetLastError());
    ok(size == sizeof(INTERNET_CERTIFICATE_INFOA), "size = %d\n", size);
    certificate_structA = HeapAlloc(GetProcessHeap(), 0, size);
    ret = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                               certificate_structA, &size);
    ok(ret, "InternetQueryOption failed: %d\n", GetLastError());
    if (ret)
    {
        ok(certificate_structA->lpszSubjectInfo &&
           strlen(certificate_structA->lpszSubjectInfo) > 1,
           "expected a non-empty subject name\n");
        ok(certificate_structA->lpszIssuerInfo &&
           strlen(certificate_structA->lpszIssuerInfo) > 1,
           "expected a non-empty issuer name\n");
        ok(!certificate_structA->lpszSignatureAlgName,
           "unexpected signature algorithm name\n");
        ok(!certificate_structA->lpszEncryptionAlgName,
           "unexpected encryption algorithm name\n");
        ok(!certificate_structA->lpszProtocolName,
           "unexpected protocol name\n");
        ok(certificate_structA->dwKeySize, "expected a non-zero key size\n");
        release_cert_info(certificate_structA);
    }
    HeapFree(GetProcessHeap(), 0, certificate_structA);

    /* Again, querying the same option through InternetQueryOptionW still
     * results in ASCII strings being returned.
     */
    size = 0;
    ret = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                               NULL, &size);
    ok(ret || GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InternetQueryOption failed: %d\n", GetLastError());
    ok(size == sizeof(INTERNET_CERTIFICATE_INFOW), "size = %d\n", size);
    certificate_structW = HeapAlloc(GetProcessHeap(), 0, size);
    ret = InternetQueryOptionW(req, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                               certificate_structW, &size);
    certificate_structA = (INTERNET_CERTIFICATE_INFOA *)certificate_structW;
    ok(ret, "InternetQueryOption failed: %d\n", GetLastError());
    if (ret)
    {
        ok(certificate_structA->lpszSubjectInfo &&
           strlen(certificate_structA->lpszSubjectInfo) > 1,
           "expected a non-empty subject name\n");
        ok(certificate_structA->lpszIssuerInfo &&
           strlen(certificate_structA->lpszIssuerInfo) > 1,
           "expected a non-empty issuer name\n");
        ok(!certificate_structA->lpszSignatureAlgName,
           "unexpected signature algorithm name\n");
        ok(!certificate_structA->lpszEncryptionAlgName,
           "unexpected encryption algorithm name\n");
        ok(!certificate_structA->lpszProtocolName,
           "unexpected protocol name\n");
        ok(certificate_structA->dwKeySize, "expected a non-zero key size\n");
        release_cert_info(certificate_structA);
    }
    HeapFree(GetProcessHeap(), 0, certificate_structW);

done:
    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_user_agent_header(void)
{
    HINTERNET ses, con, req;
    DWORD size, err;
    char buffer[64];
    BOOL ret;

    ses = InternetOpenA("Gizmo5", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "test.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, "GET", "/tests/hello.html", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_USER_AGENT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", err);

    ret = HttpAddRequestHeadersA(req, "User-Agent: Gizmo Project\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders succeeded\n");

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_USER_AGENT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(ret, "HttpQueryInfo failed\n");

    InternetCloseHandle(req);

    req = HttpOpenRequestA(con, "GET", "/", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", err);

    ret = HttpAddRequestHeadersA(req, "Accept: audio/*, image/*, text/*\r\nUser-Agent: Gizmo Project\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders failed\n");

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfoA(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!strcmp(buffer, "audio/*, image/*, text/*"), "got '%s' expected 'audio/*, image/*, text/*'\n", buffer);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_bogus_accept_types_array(void)
{
    HINTERNET ses, con, req;
    static const char *types[] = { (const char *)6240, "*/*", "%p", "", (const char *)0xffffffff, "*/*", NULL };
    DWORD size, error;
    char buffer[32];
    BOOL ret;

    ses = InternetOpenA("MERONG(0.9/;p)", INTERNET_OPEN_TYPE_DIRECT, "", "", 0);
    con = InternetConnectA(ses, "www.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    req = HttpOpenRequestA(con, "POST", "/post/post_action.php", "HTTP/1.0", "", types, INTERNET_FLAG_FORMS_SUBMIT, 0);

    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());

    buffer[0] = 0;
    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = HttpQueryInfoA(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    error = GetLastError();
    ok(!ret || broken(ret), "HttpQueryInfo succeeded\n");
    if (!ret) ok(error == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", error);
    ok(broken(!strcmp(buffer, ", */*, %p, , , */*")) /* IE6 */ ||
       broken(!strcmp(buffer, "*/*, %p, */*")) /* IE7/8 */ ||
       !strcmp(buffer, ""), "got '%s' expected ''\n", buffer);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

struct context
{
    HANDLE event;
    HINTERNET req;
};

static void WINAPI cb(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD size)
{
    INTERNET_ASYNC_RESULT *result = info;
    struct context *ctx = (struct context *)context;

    if(winetest_debug > 1)
        trace("%p 0x%08lx %u %p 0x%08x\n", handle, context, status, info, size);

    switch(status) {
    case INTERNET_STATUS_REQUEST_COMPLETE:
        trace("request handle: 0x%08lx\n", result->dwResult);
        ctx->req = (HINTERNET)result->dwResult;
        SetEvent(ctx->event);
        break;
    case INTERNET_STATUS_HANDLE_CLOSING: {
        DWORD type = INTERNET_HANDLE_TYPE_CONNECT_HTTP, size = sizeof(type);

        if (InternetQueryOptionA(handle, INTERNET_OPTION_HANDLE_TYPE, &type, &size))
            ok(type != INTERNET_HANDLE_TYPE_CONNECT_HTTP, "unexpected callback\n");
        SetEvent(ctx->event);
        break;
    }
    case INTERNET_STATUS_NAME_RESOLVED:
    case INTERNET_STATUS_CONNECTING_TO_SERVER:
    case INTERNET_STATUS_CONNECTED_TO_SERVER: {
        char *str = info;
        ok(str[0] && str[1], "Got string: %s\n", str);
        ok(size == strlen(str)+1, "unexpected size %u\n", size);
    }
    }
}

static void test_open_url_async(void)
{
    BOOL ret;
    HINTERNET ses, req;
    DWORD size, error;
    struct context ctx;
    ULONG type;

    /* Collect all existing persistent connections */
    ret = InternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(ret, "InternetSetOption(INTERNET_OPTION_END_BROWSER_SESSION) failed: %u\n", GetLastError());

    /*
     * Some versions of IE6 fail those tests. They pass some notification data as UNICODE string, while
     * other versions never do. They also hang of following tests. We disable it for everything older
     * than IE7.
     */
    if(!is_ie7plus)
        return;

    ctx.req = NULL;
    ctx.event = CreateEventA(NULL, TRUE, FALSE, "Z:_home_hans_jaman-installer.exe_ev1");

    ses = InternetOpenA("AdvancedInstaller", 0, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(ses != NULL, "InternetOpen failed\n");

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(NULL, INTERNET_OPTION_CALLBACK, &cb, sizeof(DWORD_PTR));
    error = GetLastError();
    ok(!ret, "InternetSetOptionA succeeded\n");
    ok(error == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "got %u expected ERROR_INTERNET_INCORRECT_HANDLE_TYPE\n", error);

    ret = InternetSetOptionA(ses, INTERNET_OPTION_CALLBACK, &cb, sizeof(DWORD_PTR));
    error = GetLastError();
    ok(!ret, "InternetSetOptionA failed\n");
    ok(error == ERROR_INTERNET_OPTION_NOT_SETTABLE, "got %u expected ERROR_INTERNET_OPTION_NOT_SETTABLE\n", error);

    pInternetSetStatusCallbackW(ses, cb);
    ResetEvent(ctx.event);

    req = InternetOpenUrlA(ses, "http://test.winehq.org", NULL, 0, 0, (DWORD_PTR)&ctx);
    ok(!req && GetLastError() == ERROR_IO_PENDING, "InternetOpenUrl failed\n");

    WaitForSingleObject(ctx.event, INFINITE);

    type = 0;
    size = sizeof(type);
    ret = InternetQueryOptionA(ctx.req, INTERNET_OPTION_HANDLE_TYPE, &type, &size);
    ok(ret, "InternetQueryOption failed: %u\n", GetLastError());
    ok(type == INTERNET_HANDLE_TYPE_HTTP_REQUEST,
       "expected INTERNET_HANDLE_TYPE_HTTP_REQUEST, got %u\n", type);

    size = 0;
    ret = HttpQueryInfoA(ctx.req, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &size, NULL);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "HttpQueryInfo failed\n");
    ok(size > 0, "expected size > 0\n");

    ResetEvent(ctx.event);
    InternetCloseHandle(ctx.req);
    WaitForSingleObject(ctx.event, INFINITE);

    InternetCloseHandle(ses);
    CloseHandle(ctx.event);
}

enum api
{
    internet_connect = 1,
    http_open_request,
    http_send_request_ex,
    internet_writefile,
    http_end_request,
    internet_close_handle
};

struct notification
{
    enum api     function; /* api responsible for notification */
    unsigned int status;   /* status received */
    BOOL         async;    /* delivered from another thread? */
    BOOL         todo;
    BOOL         optional;
};

struct info
{
    enum api     function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
    HANDLE       wait;
    DWORD        thread;
    unsigned int line;
    DWORD        expect_result;
    BOOL         is_aborted;
};

static CRITICAL_SECTION notification_cs;

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    BOOL status_ok, function_ok;
    struct info *info = (struct info *)context;
    unsigned int i;

    EnterCriticalSection( &notification_cs );

    if(info->is_aborted) {
        LeaveCriticalSection(&notification_cs);
        return;
    }

    if (status == INTERNET_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        HttpQueryInfoA( handle, INTERNET_OPTION_CONTEXT_VALUE, &info, &size, 0 );
    }else if(status == INTERNET_STATUS_REQUEST_COMPLETE) {
        INTERNET_ASYNC_RESULT *ar = (INTERNET_ASYNC_RESULT*)buffer;

        ok(buflen == sizeof(*ar), "unexpected buflen = %d\n", buflen);
        if(info->expect_result == ERROR_SUCCESS) {
            ok(ar->dwResult == 1, "ar->dwResult = %ld, expected 1\n", ar->dwResult);
        }else {
            ok(!ar->dwResult, "ar->dwResult = %ld, expected 1\n", ar->dwResult);
            ok(ar->dwError == info->expect_result, "ar->dwError = %d, expected %d\n", ar->dwError, info->expect_result);
        }
    }

    i = info->index;
    if (i >= info->count)
    {
        LeaveCriticalSection( &notification_cs );
        return;
    }

    while (info->test[i].status != status &&
        (info->test[i].optional || info->test[i].todo) &&
        i < info->count - 1 &&
        info->test[i].function == info->test[i + 1].function)
    {
        i++;
    }

    status_ok   = (info->test[i].status == status);
    function_ok = (info->test[i].function == info->function);

    if (!info->test[i].todo)
    {
        ok( status_ok, "%u: expected status %u got %u\n", info->line, info->test[i].status, status );
        ok( function_ok, "%u: expected function %u got %u\n", info->line, info->test[i].function, info->function );

        if (info->test[i].async)
            ok(info->thread != GetCurrentThreadId(), "%u: expected thread %u got %u\n",
               info->line, info->thread, GetCurrentThreadId());
    }
    else
    {
        todo_wine ok( status_ok, "%u: expected status %u got %u\n", info->line, info->test[i].status, status );
        if (status_ok)
            todo_wine ok( function_ok, "%u: expected function %u got %u\n", info->line, info->test[i].function, info->function );
    }
    if (i == info->count - 1 || info->test[i].function != info->test[i + 1].function) SetEvent( info->wait );
    info->index = i+1;

    LeaveCriticalSection( &notification_cs );
}

static void setup_test( struct info *info, enum api function, unsigned int line, DWORD expect_result )
{
    info->function = function;
    info->line = line;
    info->expect_result = expect_result;
}

struct notification_data
{
    const struct notification *test;
    const unsigned int count;
    const char *method;
    const char *host;
    const char *path;
    const char *data;
    BOOL expect_conn_failure;
};

static const struct notification async_send_request_ex_test[] =
{
    { internet_connect,      INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_open_request,     INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_send_request_ex,  INTERNET_STATUS_DETECTING_PROXY, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_COOKIE_SENT, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_RESOLVING_NAME, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_NAME_RESOLVED, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTING_TO_SERVER, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTED_TO_SERVER, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_SENDING_REQUEST, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_SENT, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { internet_writefile,    INTERNET_STATUS_SENDING_REQUEST, FALSE },
    { internet_writefile,    INTERNET_STATUS_REQUEST_SENT, FALSE },
    { http_end_request,      INTERNET_STATUS_RECEIVING_RESPONSE, TRUE },
    { http_end_request,      INTERNET_STATUS_RESPONSE_RECEIVED, TRUE },
    { http_end_request,      INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CLOSING_CONNECTION, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CONNECTION_CLOSED, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, }
};

static const struct notification async_send_request_ex_test2[] =
{
    { internet_connect,      INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_open_request,     INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_send_request_ex,  INTERNET_STATUS_DETECTING_PROXY, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_COOKIE_SENT, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_RESOLVING_NAME, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_NAME_RESOLVED, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTING_TO_SERVER, TRUE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTED_TO_SERVER, TRUE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_SENDING_REQUEST, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_SENT, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { http_end_request,      INTERNET_STATUS_RECEIVING_RESPONSE, TRUE },
    { http_end_request,      INTERNET_STATUS_RESPONSE_RECEIVED, TRUE },
    { http_end_request,      INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CLOSING_CONNECTION, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CONNECTION_CLOSED, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, }
};

static const struct notification async_send_request_ex_resolve_failure_test[] =
{
    { internet_connect,      INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_open_request,     INTERNET_STATUS_HANDLE_CREATED, FALSE },
    { http_send_request_ex,  INTERNET_STATUS_DETECTING_PROXY, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_RESOLVING_NAME, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_DETECTING_PROXY, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { http_end_request,      INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CLOSING_CONNECTION, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CONNECTION_CLOSED, FALSE, FALSE, TRUE },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, FALSE, }
};

static const struct notification async_send_request_ex_chunked_test[] =
{
    { internet_connect,      INTERNET_STATUS_HANDLE_CREATED },
    { http_open_request,     INTERNET_STATUS_HANDLE_CREATED },
    { http_send_request_ex,  INTERNET_STATUS_DETECTING_PROXY, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_COOKIE_SENT, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_RESOLVING_NAME, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_NAME_RESOLVED, TRUE, FALSE, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTING_TO_SERVER, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTED_TO_SERVER, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_SENDING_REQUEST, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_SENT, TRUE },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { http_end_request,      INTERNET_STATUS_RECEIVING_RESPONSE, TRUE },
    { http_end_request,      INTERNET_STATUS_RESPONSE_RECEIVED, TRUE },
    { http_end_request,      INTERNET_STATUS_REQUEST_COMPLETE, TRUE },
    { internet_close_handle, INTERNET_STATUS_CLOSING_CONNECTION },
    { internet_close_handle, INTERNET_STATUS_CONNECTION_CLOSED },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING }
};

static const struct notification_data notification_data[] = {
    {
        async_send_request_ex_chunked_test,
        ARRAY_SIZE(async_send_request_ex_chunked_test),
        "GET",
        "test.winehq.org",
        "tests/data.php"
    },
    {
        async_send_request_ex_test,
        ARRAY_SIZE(async_send_request_ex_test),
        "POST",
        "test.winehq.org",
        "tests/post.php",
        "Public ID=codeweavers"
    },
    {
        async_send_request_ex_test2,
        ARRAY_SIZE(async_send_request_ex_test2),
        "POST",
        "test.winehq.org",
        "tests/post.php"
    },
    {
        async_send_request_ex_resolve_failure_test,
        ARRAY_SIZE(async_send_request_ex_resolve_failure_test),
        "GET",
        "brokenhost",
        "index.html",
        NULL,
        TRUE
    }
};

static void test_async_HttpSendRequestEx(const struct notification_data *nd)
{
    BOOL ret;
    HINTERNET ses, req, con;
    struct info info;
    DWORD size, written, error;
    INTERNET_BUFFERSA b;
    static const char *accept[2] = {"*/*", NULL};
    char buffer[32];

    trace("Async HttpSendRequestEx test (%s %s)\n", nd->method, nd->host);

    InitializeCriticalSection( &notification_cs );

    info.test  = nd->test;
    info.count = nd->count;
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );
    info.thread = GetCurrentThreadId();
    info.is_aborted = FALSE;

    ses = InternetOpenA( "winetest", 0, NULL, NULL, INTERNET_FLAG_ASYNC );
    ok( ses != NULL, "InternetOpen failed\n" );

    pInternetSetStatusCallbackA( ses, check_notification );

    setup_test( &info, internet_connect, __LINE__, ERROR_SUCCESS );
    con = InternetConnectA( ses, nd->host, 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)&info );
    ok( con != NULL, "InternetConnect failed %u\n", GetLastError() );

    WaitForSingleObject( info.wait, 10000 );

    setup_test( &info, http_open_request, __LINE__, ERROR_SUCCESS );
    req = HttpOpenRequestA( con, nd->method, nd->path, NULL, NULL, accept, 0, (DWORD_PTR)&info );
    ok( req != NULL, "HttpOpenRequest failed %u\n", GetLastError() );

    WaitForSingleObject( info.wait, 10000 );

    if(nd->data) {
        memset( &b, 0, sizeof(INTERNET_BUFFERSA) );
        b.dwStructSize = sizeof(INTERNET_BUFFERSA);
        b.lpcszHeader = "Content-Type: application/x-www-form-urlencoded";
        b.dwHeadersLength = strlen( b.lpcszHeader );
        b.dwBufferTotal = nd->data ? strlen( nd->data ) : 0;
    }

    setup_test( &info, http_send_request_ex, __LINE__,
            nd->expect_conn_failure ? ERROR_INTERNET_NAME_NOT_RESOLVED : ERROR_SUCCESS );
    ret = HttpSendRequestExA( req, nd->data ? &b : NULL, NULL, 0x28, 0 );
    ok( !ret && GetLastError() == ERROR_IO_PENDING, "HttpSendRequestExA failed %d %u\n", ret, GetLastError() );

    error = WaitForSingleObject( info.wait, 10000 );
    if(error != WAIT_OBJECT_0) {
        skip("WaitForSingleObject returned %d, assuming DNS problem\n", error);
        info.is_aborted = TRUE;
        goto abort;
    }

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    ret = HttpQueryInfoA( req, HTTP_QUERY_CONTENT_ENCODING, buffer, &size, 0 );
    error = GetLastError();
    ok( !ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    if(nd->expect_conn_failure) {
        ok(error == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND got %u\n", error );
    }else {
        todo_wine
        ok(error == ERROR_INTERNET_INCORRECT_HANDLE_STATE,
            "expected ERROR_INTERNET_INCORRECT_HANDLE_STATE got %u\n", error );
    }

    if (nd->data)
    {
        written = 0;
        size = strlen( nd->data );
        setup_test( &info, internet_writefile, __LINE__, ERROR_SUCCESS );
        ret = InternetWriteFile( req, nd->data, size, &written );
        ok( ret, "InternetWriteFile failed %u\n", GetLastError() );
        ok( written == size, "expected %u got %u\n", written, size );

        WaitForSingleObject( info.wait, 10000 );

        SetLastError( 0xdeadbeef );
        ret = HttpEndRequestA( req, (void *)nd->data, 0x28, 0 );
        error = GetLastError();
        ok( !ret, "HttpEndRequestA succeeded\n" );
        ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error );
    }

    SetLastError( 0xdeadbeef );
    setup_test( &info, http_end_request, __LINE__,
            nd->expect_conn_failure ? ERROR_INTERNET_OPERATION_CANCELLED : ERROR_SUCCESS);
    ret = HttpEndRequestA( req, NULL, 0x28, 0 );
    error = GetLastError();
    ok( !ret, "HttpEndRequestA succeeded\n" );
    ok( error == ERROR_IO_PENDING, "expected ERROR_IO_PENDING got %u\n", error );

    WaitForSingleObject( info.wait, 10000 );

    setup_test( &info, internet_close_handle, __LINE__, ERROR_SUCCESS );
 abort:
    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    WaitForSingleObject( info.wait, 10000 );
    Sleep(100);
    CloseHandle( info.wait );
    DeleteCriticalSection( &notification_cs );
}

static HINTERNET closetest_session, closetest_req, closetest_conn;
static BOOL closetest_closed;

static void WINAPI closetest_callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
     LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    DWORD len, type;
    BOOL res;

    if(winetest_debug > 1)
        trace("closetest_callback %p: %d\n", hInternet, dwInternetStatus);

    ok(hInternet == closetest_session || hInternet == closetest_conn || hInternet == closetest_req,
       "Unexpected hInternet %p\n", hInternet);
    if(!closetest_closed)
        return;

    len = sizeof(type);
    res = InternetQueryOptionA(closetest_req, INTERNET_OPTION_HANDLE_TYPE, &type, &len);
    ok(!res && GetLastError() == ERROR_INVALID_HANDLE,
       "InternetQueryOptionA(%p INTERNET_OPTION_HANDLE_TYPE) failed: %x %u, expected TRUE ERROR_INVALID_HANDLE\n",
       closetest_req, res, GetLastError());
}

static void test_InternetCloseHandle(void)
{
    DWORD len, flags;
    BOOL res;

    closetest_session = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(closetest_session != NULL,"InternetOpen failed with error %u\n", GetLastError());

    pInternetSetStatusCallbackA(closetest_session, closetest_callback);

    closetest_conn = InternetConnectA(closetest_session, "source.winehq.org", INTERNET_INVALID_PORT_NUMBER,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok(closetest_conn != NULL,"InternetConnect failed with error %u\n", GetLastError());

    closetest_req = HttpOpenRequestA(closetest_conn, "GET", "winegecko.php", NULL, NULL, NULL,
            INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE, 0xdeadbead);

    res = HttpSendRequestA(closetest_req, NULL, -1, NULL, 0);
    ok(!res && (GetLastError() == ERROR_IO_PENDING),
       "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");

    test_request_flags(closetest_req, INTERNET_REQFLAG_NO_HEADERS);

    res = InternetCloseHandle(closetest_session);
    ok(res, "InternetCloseHandle failed: %u\n", GetLastError());
    closetest_closed = TRUE;
    trace("Closed session handle\n");

    res = InternetCloseHandle(closetest_conn);
    ok(!res && GetLastError() == ERROR_INVALID_HANDLE, "InternetCloseConnection(conn) failed: %x %u\n",
       res, GetLastError());

    res = InternetCloseHandle(closetest_req);
    ok(!res && GetLastError() == ERROR_INVALID_HANDLE, "InternetCloseConnection(req) failed: %x %u\n",
       res, GetLastError());

    len = sizeof(flags);
    res = InternetQueryOptionA(closetest_req, INTERNET_OPTION_REQUEST_FLAGS, &flags, &len);
    ok(!res && GetLastError() == ERROR_INVALID_HANDLE,
       "InternetQueryOptionA(%p INTERNET_OPTION_REQUEST_FLAGS) failed: %x %u, expected TRUE ERROR_INVALID_HANDLE\n",
       closetest_req, res, GetLastError());
}

static void test_connection_failure(void)
{
    test_request_t req;
    DWORD error;
    BOOL ret;

    open_simple_request(&req, "localhost", 1, NULL, "/");

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestA(req.request, NULL, 0, NULL, 0);
    error = GetLastError();
    ok(!ret, "unexpected success\n");
    ok(error == ERROR_INTERNET_CANNOT_CONNECT, "wrong error %u\n", error);

    close_request(&req);
}

static void test_default_service_port(void)
{
    HINTERNET session, connect, request;
    DWORD size, error;
    char buffer[128];
    BOOL ret;

    if(!is_ie7plus)
        return;

    session = InternetOpenA("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_INVALID_PORT_NUMBER, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    SetLastError(0xdeadbeef);
    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    error = GetLastError();
    ok(!ret, "HttpSendRequest succeeded\n");
    ok(error == ERROR_INTERNET_SECURITY_CHANNEL_ERROR || error == ERROR_INTERNET_CANNOT_CONNECT,
       "got %u\n", error);

    size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    ret = HttpQueryInfoA(request, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed with error %u\n", GetLastError());
    ok(!strcmp(buffer, "test.winehq.org:80"), "Expected test.winehg.org:80, got '%s'\n", buffer);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_INVALID_PORT_NUMBER, NULL, NULL,
                               INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());

    size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    ret = HttpQueryInfoA(request, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed with error %u\n", GetLastError());
    ok(!strcmp(buffer, "test.winehq.org"), "Expected test.winehg.org, got '%s'\n", buffer);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_INVALID_PORT_NUMBER, NULL, NULL,
                               INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, NULL, 0, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed with error %u\n", GetLastError());

    size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    ret = HttpQueryInfoA(request, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed with error %u\n", GetLastError());
    ok(!strcmp(buffer, "test.winehq.org:443"), "Expected test.winehg.org:443, got '%s'\n", buffer);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void init_status_tests(void)
{
    memset(expect, 0, sizeof(expect));
    memset(optional, 0, sizeof(optional));
    memset(wine_allow, 0, sizeof(wine_allow));
    memset(notified, 0, sizeof(notified));
    memset(status_string, 0, sizeof(status_string));

#define STATUS_STRING(status) status_string[status] = #status
    STATUS_STRING(INTERNET_STATUS_RESOLVING_NAME);
    STATUS_STRING(INTERNET_STATUS_NAME_RESOLVED);
    STATUS_STRING(INTERNET_STATUS_CONNECTING_TO_SERVER);
    STATUS_STRING(INTERNET_STATUS_CONNECTED_TO_SERVER);
    STATUS_STRING(INTERNET_STATUS_SENDING_REQUEST);
    STATUS_STRING(INTERNET_STATUS_REQUEST_SENT);
    STATUS_STRING(INTERNET_STATUS_RECEIVING_RESPONSE);
    STATUS_STRING(INTERNET_STATUS_RESPONSE_RECEIVED);
    STATUS_STRING(INTERNET_STATUS_CTL_RESPONSE_RECEIVED);
    STATUS_STRING(INTERNET_STATUS_PREFETCH);
    STATUS_STRING(INTERNET_STATUS_CLOSING_CONNECTION);
    STATUS_STRING(INTERNET_STATUS_CONNECTION_CLOSED);
    STATUS_STRING(INTERNET_STATUS_HANDLE_CREATED);
    STATUS_STRING(INTERNET_STATUS_HANDLE_CLOSING);
    STATUS_STRING(INTERNET_STATUS_DETECTING_PROXY);
    STATUS_STRING(INTERNET_STATUS_REQUEST_COMPLETE);
    STATUS_STRING(INTERNET_STATUS_REDIRECT);
    STATUS_STRING(INTERNET_STATUS_INTERMEDIATE_RESPONSE);
    STATUS_STRING(INTERNET_STATUS_USER_INPUT_REQUIRED);
    STATUS_STRING(INTERNET_STATUS_STATE_CHANGE);
    STATUS_STRING(INTERNET_STATUS_COOKIE_SENT);
    STATUS_STRING(INTERNET_STATUS_COOKIE_RECEIVED);
    STATUS_STRING(INTERNET_STATUS_PRIVACY_IMPACTED);
    STATUS_STRING(INTERNET_STATUS_P3P_HEADER);
    STATUS_STRING(INTERNET_STATUS_P3P_POLICYREF);
    STATUS_STRING(INTERNET_STATUS_COOKIE_HISTORY);
#undef STATUS_STRING
}

static void WINAPI header_cb( HINTERNET handle, DWORD_PTR ctx, DWORD status, LPVOID info, DWORD len )
{
    BOOL ret;
    DWORD index, size;
    char buf[256];

    if (status == INTERNET_STATUS_SENDING_REQUEST)
    {
        ret = HttpAddRequestHeadersA( handle, "winetest: winetest", ~0u, HTTP_ADDREQ_FLAG_ADD );
        ok( ret, "HttpAddRequestHeadersA failed %u\n", GetLastError() );
        SetEvent( (HANDLE)ctx );
    }
    else if (status == INTERNET_STATUS_REQUEST_SENT)
    {
        index = 0;
        size = sizeof(buf);
        ret = HttpQueryInfoA( handle, HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                              buf, &size, &index );
        ok( ret, "HttpQueryInfoA failed %u\n", GetLastError() );
        ok( strstr( buf, "winetest: winetest" ) != NULL, "header missing\n" );
        SetEvent( (HANDLE)ctx );
    }
}

static void test_concurrent_header_access(void)
{
    HINTERNET ses, con, req;
    DWORD err;
    BOOL ret;
    HANDLE wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = InternetOpenA( "winetest", 0, NULL, NULL, INTERNET_FLAG_ASYNC );
    ok( ses != NULL, "InternetOpenA failed\n" );

    con = InternetConnectA( ses, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                            INTERNET_SERVICE_HTTP, 0, 0 );
    ok( con != NULL, "InternetConnectA failed %u\n", GetLastError() );

    req = HttpOpenRequestA( con, NULL, "/", NULL, NULL, NULL, 0, (DWORD_PTR)wait );
    ok( req != NULL, "HttpOpenRequestA failed %u\n", GetLastError() );

    pInternetSetStatusCallbackA( req, header_cb );

    SetLastError( 0xdeadbeef );
    ret = HttpSendRequestA( req, NULL, 0, NULL, 0 );
    err = GetLastError();
    ok( !ret, "HttpSendRequestA succeeded\n" );
    ok( err == ERROR_IO_PENDING, "got %u\n", ERROR_IO_PENDING );

    WaitForSingleObject( wait, 5000 );
    WaitForSingleObject( wait, 5000 );

    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );
    CloseHandle( wait );
}

START_TEST(http)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");

    if(!GetProcAddress(hdll, "InternetGetCookieExW")) {
        win_skip("Too old IE (older than 6.0)\n");
        return;
    }

    pInternetSetStatusCallbackA = (void*)GetProcAddress(hdll, "InternetSetStatusCallbackA");
    pInternetSetStatusCallbackW = (void*)GetProcAddress(hdll, "InternetSetStatusCallbackW");
    pInternetGetSecurityInfoByURLA = (void*)GetProcAddress(hdll, "InternetGetSecurityInfoByURLA");

    if(!pInternetGetSecurityInfoByURLA) {
        is_ie7plus = FALSE;
        win_skip("IE6 found. It's too old for some tests.\n");
    }

    init_events();
    init_status_tests();
    test_InternetCloseHandle();
    InternetReadFile_test(INTERNET_FLAG_ASYNC, &test_data[0]);
    InternetReadFile_test(INTERNET_FLAG_ASYNC, &test_data[1]);
    InternetReadFile_test(0, &test_data[1]);
    InternetReadFile_test(INTERNET_FLAG_ASYNC, &test_data[2]);
    test_security_flags();
    InternetReadFile_test(0, &test_data[2]);
    InternetReadFileExA_test(INTERNET_FLAG_ASYNC);
    test_open_url_async();
    test_async_HttpSendRequestEx(&notification_data[0]);
    test_async_HttpSendRequestEx(&notification_data[1]);
    test_async_HttpSendRequestEx(&notification_data[2]);
    test_async_HttpSendRequestEx(&notification_data[3]);
    InternetOpenRequest_test();
    test_http_cache();
    InternetLockRequestFile_test();
    InternetOpenUrlA_test();
    HttpHeaders_test();
    test_http_connection();
    test_secure_connection();
    test_user_agent_header();
    test_bogus_accept_types_array();
    InternetReadFile_chunked_test();
    HttpSendRequestEx_test();
    InternetReadFile_test(INTERNET_FLAG_ASYNC, &test_data[3]);
    test_connection_failure();
    test_default_service_port();
    test_concurrent_header_access();
    free_events();
}
