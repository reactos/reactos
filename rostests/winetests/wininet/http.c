/*
 * Wininet - Http tests
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

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winsock.h"

#include "wine/test.h"

#define TEST_URL "http://test.winehq.org/hello.html"

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
                         status < MAX_INTERNET_STATUS && status_string[status][0] != 0 ? \
                         status_string[status] : "unknown");            \
            wine_allow[status]--; \
        } \
        else \
        { \
            ok(expect[status] || optional[status], "unexpected status %d (%s)\n", status,   \
               status < MAX_INTERNET_STATUS && status_string[status][0] != 0 ? \
               status_string[status] : "unknown");                      \
            if (expect[status]) expect[status]--; \
            else optional[status]--; \
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
           status, status < MAX_INTERNET_STATUS && status_string[status][0] != 0 ? \
           status_string[status] : "unknown", (num), notified[status]); \
        CLEAR_NOTIFIED(status);                                         \
    }while(0)

#define CHECK_NOTIFIED(status) \
    CHECK_NOTIFIED2(status, 1)

#define CHECK_NOT_NOTIFIED(status) \
    CHECK_NOTIFIED2(status, 0)

#define MAX_INTERNET_STATUS (INTERNET_STATUS_COOKIE_HISTORY+1)
#define MAX_STATUS_NAME 50
static int expect[MAX_INTERNET_STATUS], optional[MAX_INTERNET_STATUS],
    wine_allow[MAX_INTERNET_STATUS], notified[MAX_INTERNET_STATUS];
static CHAR status_string[MAX_INTERNET_STATUS][MAX_STATUS_NAME];

static HANDLE hCompleteEvent;

static INTERNET_STATUS_CALLBACK (WINAPI *pInternetSetStatusCallbackA)(HINTERNET ,INTERNET_STATUS_CALLBACK);


static VOID WINAPI callback(
     HINTERNET hInternet,
     DWORD_PTR dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength
)
{
    CHECK_EXPECT(dwInternetStatus);
    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_RESOLVING_NAME:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RESOLVING_NAME \"%s\" %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_NAME_RESOLVED:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_NAME_RESOLVED \"%s\" %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTING_TO_SERVER \"%s\" %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTED_TO_SERVER \"%s\" %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            break;
        case INTERNET_STATUS_SENDING_REQUEST:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_SENDING_REQUEST %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_REQUEST_SENT:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %d\n",
                dwStatusInformationLength);
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REQUEST_SENT 0x%x %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RECEIVING_RESPONSE:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RECEIVING_RESPONSE %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RESPONSE_RECEIVED:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %d\n",
                dwStatusInformationLength);
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_RESPONSE_RECEIVED 0x%x %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CTL_RESPONSE_RECEIVED %p %d\n",
                GetCurrentThreadId(), hInternet,dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_PREFETCH:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_PREFETCH %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CLOSING_CONNECTION:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CLOSING_CONNECTION %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CONNECTION_CLOSED:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_CONNECTION_CLOSED %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_HANDLE_CREATED:
            ok(dwStatusInformationLength == sizeof(HINTERNET),
                "info length should be sizeof(HINTERNET) instead of %d\n",
                dwStatusInformationLength);
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
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_HANDLE_CLOSING %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(HINTERNET *)lpvStatusInformation, dwStatusInformationLength);
            break;
        case INTERNET_STATUS_REQUEST_COMPLETE:
        {
            INTERNET_ASYNC_RESULT *iar = (INTERNET_ASYNC_RESULT *)lpvStatusInformation;
            ok(dwStatusInformationLength == sizeof(INTERNET_ASYNC_RESULT),
                "info length should be sizeof(INTERNET_ASYNC_RESULT) instead of %d\n",
                dwStatusInformationLength);
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REQUEST_COMPLETE {%ld,%d} %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                iar->dwResult,iar->dwError,dwStatusInformationLength);
            SetEvent(hCompleteEvent);
            break;
        }
        case INTERNET_STATUS_REDIRECT:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_REDIRECT \"%s\" %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation, dwStatusInformationLength);
            *(LPSTR)lpvStatusInformation = '\0';
            CLEAR_NOTIFIED(INTERNET_STATUS_DETECTING_PROXY);
            SET_EXPECT(INTERNET_STATUS_DETECTING_PROXY);
            break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            trace("%04x:Callback %p 0x%lx INTERNET_STATUS_INTERMEDIATE_RESPONSE %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation, dwStatusInformationLength);
            break;
        default:
            trace("%04x:Callback %p 0x%lx %d %p %d\n",
                GetCurrentThreadId(), hInternet, dwContext, dwInternetStatus,
                lpvStatusInformation, dwStatusInformationLength);
    }
}

static void InternetReadFile_test(int flags)
{
    BOOL res;
    CHAR buffer[4000];
    DWORD length;
    DWORD out;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;

    hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    trace("Starting InternetReadFile test with flags 0x%x\n",flags);

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
    hor = HttpOpenRequestA(hic, "GET", "/testredirect", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE,
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

    length = sizeof(buffer);
    res = InternetQueryOptionA(hor, INTERNET_OPTION_URL, buffer, &length);
    ok(res, "InternetQueryOptionA(INTERNET_OPTION_URL) failed: %u\n", GetLastError());
    ok(!strcmp(buffer, "http://test.winehq.org/testredirect"), "Wrong URL %s\n", buffer);

    length = sizeof(buffer);
    res = HttpQueryInfoA(hor, HTTP_QUERY_RAW_HEADERS, buffer, &length, 0x0);
    ok(res, "HttpQueryInfoA(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    ok(length == 0, "HTTP_QUERY_RAW_HEADERS: expected length 0, but got %d\n", length);
    ok(!strcmp(buffer, ""), "HTTP_QUERY_RAW_HEADERS: expected string \"\", but got \"%s\"\n", buffer);

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    if (first_connection_to_test_url)
    {
        SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
        SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
        SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
        SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);
    }
    else
    {
        SET_WINE_ALLOW2(INTERNET_STATUS_RESOLVING_NAME,2);
        SET_WINE_ALLOW2(INTERNET_STATUS_NAME_RESOLVED,2);
    }
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTED_TO_SERVER);
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
    res = HttpSendRequestA(hor, "", -1, NULL, 0);
    if (flags & INTERNET_FLAG_ASYNC)
        ok(!res && (GetLastError() == ERROR_IO_PENDING),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    else
        ok(res || (GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED),
           "Synchronous HttpSendRequest returning 0, error %u\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    if (flags & INTERNET_FLAG_ASYNC)
        WaitForSingleObject(hCompleteEvent, INFINITE);

    if (first_connection_to_test_url)
    {
        CHECK_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    else todo_wine
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    CHECK_NOTIFIED2(INTERNET_STATUS_SENDING_REQUEST, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_REQUEST_SENT, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_RECEIVING_RESPONSE, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_RESPONSE_RECEIVED, 2);
    CHECK_NOTIFIED(INTERNET_STATUS_REDIRECT);
    if (flags & INTERNET_FLAG_ASYNC)
        CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    else
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    /* Sent on WinXP only if first_connection_to_test_url is TRUE, on Win98 always sent */
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);

    length = 4;
    res = InternetQueryOptionA(hor,INTERNET_OPTION_REQUEST_FLAGS,&out,&length);
    ok(res, "InternetQueryOptionA(INTERNET_OPTION_REQUEST) failed with error %d\n", GetLastError());

    length = 100;
    res = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    ok(res, "InternetQueryOptionA(INTERNET_OPTION_URL) failed with error %d\n", GetLastError());

    length = sizeof(buffer);
    res = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length,0x0);
    ok(res, "HttpQueryInfoA(HTTP_QUERY_RAW_HEADERS) failed with error %d\n", GetLastError());
    buffer[length]=0;

    length = sizeof(buffer);
    res = InternetQueryOptionA(hor, INTERNET_OPTION_URL, buffer, &length);
    ok(res, "InternetQueryOptionA(INTERNET_OPTION_URL) failed: %u\n", GetLastError());
    ok(!strcmp(buffer, "http://test.winehq.org/hello.html"), "Wrong URL %s\n", buffer);

    length = 16;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,&buffer,&length,0x0);
    trace("Option 0x5 -> %i  %s  (%u)\n",res,buffer,GetLastError());

    length = 100;
    res = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_TYPE,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x1 -> %i  %s\n",res,buffer);

    SetLastError(0xdeadbeef);
    res = InternetReadFile(NULL, buffer, 100, &length);
    ok(!res, "InternetReadFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "InternetReadFile should have set last error to ERROR_INVALID_HANDLE instead of %u\n",
        GetLastError());

    length = 100;
    trace("Entering Query loop\n");

    while (TRUE)
    {
        if (flags & INTERNET_FLAG_ASYNC)
            SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
        res = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok(!(!res && length != 0),"InternetQueryDataAvailable failed with non-zero length\n");
        ok(res || ((flags & INTERNET_FLAG_ASYNC) && GetLastError() == ERROR_IO_PENDING),
           "InternetQueryDataAvailable failed, error %d\n", GetLastError());
        if (flags & INTERNET_FLAG_ASYNC)
        {
            if (res)
            {
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
            }
            else if (GetLastError() == ERROR_IO_PENDING)
            {
                WaitForSingleObject(hCompleteEvent, INFINITE);
                CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
                continue;
            }
        }
        if (length)
        {
            char *buffer;
            buffer = HeapAlloc(GetProcessHeap(),0,length+1);

            res = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            trace("ReadFile -> %s %i\n",res?"TRUE":"FALSE",length);

            HeapFree(GetProcessHeap(),0,buffer);
        }
        if (length == 0)
            break;
    }
    CHECK_NOTIFIED2(INTERNET_STATUS_CLOSING_CONNECTION, 2);
    CHECK_NOTIFIED2(INTERNET_STATUS_CONNECTION_CLOSED, 2);
abort:
    trace("aborting\n");
    SET_EXPECT2(INTERNET_STATUS_HANDLE_CLOSING, (hor != 0x0) + (hic != 0x0));
    if (hor != 0x0) {
        SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
        SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);
        SetLastError(0xdeadbeef);
        trace("closing\n");
        res = InternetCloseHandle(hor);
        ok (res, "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        SetLastError(0xdeadbeef);
        res = InternetCloseHandle(hor);
        ok (!res, "Double close of handle opened by HttpOpenRequestA succeeded\n");
        ok (GetLastError() == ERROR_INVALID_HANDLE,
            "Double close of handle should have set ERROR_INVALID_HANDLE instead of %u\n",
            GetLastError());
    }
    /* We intentionally do not close the handle opened by InternetConnectA as this
     * tickles bug #9479: native closes child internet handles when the parent handles
     * are closed. This is verified below by checking that the number of
     * INTERNET_STATUS_HANDLE_CLOSING notifications matches the number expected. */
    if (hi != 0x0) {
      SET_WINE_ALLOW(INTERNET_STATUS_HANDLE_CLOSING);
        trace("closing 2\n");
      res = InternetCloseHandle(hi);
      ok (res, "InternetCloseHandle of handle opened by InternetOpenA failed\n");
      if (flags & INTERNET_FLAG_ASYNC)
          Sleep(100);
    }
    CHECK_NOTIFIED2(INTERNET_STATUS_HANDLE_CLOSING, (hor != 0x0) + (hic != 0x0));
    if (hor != 0x0) todo_wine
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    }
    else
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    }
    CloseHandle(hCompleteEvent);
    first_connection_to_test_url = FALSE;
}

static void InternetReadFileExA_test(int flags)
{
    DWORD rc;
    DWORD length;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;
    INTERNET_BUFFERS inetbuffers;

    hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    trace("Starting InternetReadFileExA test with flags 0x%x\n",flags);

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
    hor = HttpOpenRequestA(hic, "GET", "/testredirect", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE,
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
        SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
        SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);
    }
    else
    {
        SET_WINE_ALLOW2(INTERNET_STATUS_RESOLVING_NAME,2);
        SET_WINE_ALLOW2(INTERNET_STATUS_NAME_RESOLVED,2);
    }
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTED_TO_SERVER);
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

    if (!rc && (GetLastError() == ERROR_IO_PENDING))
        WaitForSingleObject(hCompleteEvent, INFINITE);

    if (first_connection_to_test_url)
    {
        CHECK_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    else todo_wine
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
    /* Sent on WinXP only if first_connection_to_test_url is TRUE, on Win98 always sent */
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);

    /* tests invalid dwStructSize */
    inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS)+1;
    inetbuffers.lpcszHeader = NULL;
    inetbuffers.dwHeadersLength = 0;
    inetbuffers.dwBufferLength = 10;
    inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, 10);
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    rc = InternetReadFileEx(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_PARAMETER),
        "InternetReadFileEx should have failed with ERROR_INVALID_PARAMETER instead of %s, %u\n",
        rc ? "TRUE" : "FALSE", GetLastError());
    HeapFree(GetProcessHeap(), 0, inetbuffers.lpvBuffer);

    /* tests to see whether lpcszHeader is used - it isn't */
    inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS);
    inetbuffers.lpcszHeader = (LPCTSTR)0xdeadbeef;
    inetbuffers.dwHeadersLength = 255;
    inetbuffers.dwBufferLength = 0;
    inetbuffers.lpvBuffer = NULL;
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    rc = InternetReadFileEx(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(rc, "InternetReadFileEx failed with error %u\n", GetLastError());
        trace("read %i bytes\n", inetbuffers.dwBufferLength);
    todo_wine
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
    }

    rc = InternetReadFileEx(NULL, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_HANDLE),
        "InternetReadFileEx should have failed with ERROR_INVALID_HANDLE instead of %s, %u\n",
        rc ? "TRUE" : "FALSE", GetLastError());

    length = 0;
    trace("Entering Query loop\n");

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    while (TRUE)
    {
        inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS);
        inetbuffers.dwBufferLength = 1024;
        inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, inetbuffers.dwBufferLength+1);
        inetbuffers.dwOffsetHigh = 1234;
        inetbuffers.dwOffsetLow = 5678;

        SET_WINE_ALLOW(INTERNET_STATUS_RECEIVING_RESPONSE);
        SET_WINE_ALLOW(INTERNET_STATUS_RESPONSE_RECEIVED);
        SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
        SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
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
                WaitForSingleObject(hCompleteEvent, INFINITE);
                CHECK_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
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
                todo_wine {
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
                CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
                }
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

    /* WinXP does not send, but Win98 does */
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
abort:
    SET_EXPECT2(INTERNET_STATUS_HANDLE_CLOSING, (hor != 0x0) + (hic != 0x0));
    if (hor) {
        SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
        SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        rc = InternetCloseHandle(hor);
        ok ((rc == 0), "Double close of handle opened by HttpOpenRequestA succeeded\n");
    }
    if (hic) {
        rc = InternetCloseHandle(hic);
        ok ((rc != 0), "InternetCloseHandle of handle opened by InternetConnectA failed\n");
    }
    if (hi) {
      SET_WINE_ALLOW(INTERNET_STATUS_HANDLE_CLOSING);
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed\n");
      if (flags & INTERNET_FLAG_ASYNC)
          Sleep(100);
      CHECK_NOTIFIED2(INTERNET_STATUS_HANDLE_CLOSING, (hor != 0x0) + (hic != 0x0));
    }
    /* to enable once Wine is fixed to never send it
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CHECK_NOT_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    */
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
    CloseHandle(hCompleteEvent);
    first_connection_to_test_url = FALSE;
}

static void InternetOpenUrlA_test(void)
{
  HINTERNET myhinternet, myhttp;
  char buffer[0x400];
  DWORD size, readbytes, totalbytes=0;
  BOOL ret;

  myhinternet = InternetOpen("Winetest",0,NULL,NULL,INTERNET_FLAG_NO_CACHE_WRITE);
  ok((myhinternet != 0), "InternetOpen failed, error %u\n",GetLastError());
  size = 0x400;
  ret = InternetCanonicalizeUrl(TEST_URL, buffer, &size,ICU_BROWSER_MODE);
  ok( ret, "InternetCanonicalizeUrl failed, error %u\n",GetLastError());

  SetLastError(0);
  myhttp = InternetOpenUrl(myhinternet, TEST_URL, 0, 0,
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
}

static void HttpSendRequestEx_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;

    INTERNET_BUFFERS BufferIn;
    DWORD dwBytesWritten;
    DWORD dwBytesRead;
    CHAR szBuffer[256];
    int i;
    BOOL ret;

    static char szPostData[] = "mode=Test";
    static const char szContentType[] = "Content-Type: application/x-www-form-urlencoded";

    hSession = InternetOpen("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnect(hSession, "crossover.codeweavers.com",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://crossover.codeweavers.com\n");
    hRequest = HttpOpenRequest(hConnect, "POST", "/posttest.php",
            NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        skip( "Network unreachable, skipping test\n" );
        goto done;
    }
    ok( hRequest != NULL, "Failed to open request handle err %u\n", GetLastError());


    BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS);
    BufferIn.Next = (LPINTERNET_BUFFERS)0xdeadcab;
    BufferIn.lpcszHeader = szContentType;
    BufferIn.dwHeadersLength = sizeof(szContentType)-1;
    BufferIn.dwHeadersTotal = sizeof(szContentType)-1;
    BufferIn.lpvBuffer = szPostData;
    BufferIn.dwBufferLength = 3;
    BufferIn.dwBufferTotal = sizeof(szPostData)-1;
    BufferIn.dwOffsetLow = 0;
    BufferIn.dwOffsetHigh = 0;

    ret = HttpSendRequestEx(hRequest, &BufferIn, NULL, 0 ,0);
    ok(ret, "HttpSendRequestEx Failed with error %u\n", GetLastError());

    for (i = 3; szPostData[i]; i++)
        ok(InternetWriteFile(hRequest, &szPostData[i], 1, &dwBytesWritten),
                "InternetWriteFile failed\n");

    ok(HttpEndRequest(hRequest, NULL, 0, 0), "HttpEndRequest Failed\n");

    ok(InternetReadFile(hRequest, szBuffer, 255, &dwBytesRead),
            "Unable to read response\n");
    szBuffer[dwBytesRead] = 0;

    ok(dwBytesRead == 13,"Read %u bytes instead of 13\n",dwBytesRead);
    ok(strncmp(szBuffer,"mode => Test\n",dwBytesRead)==0,"Got string %s\n",szBuffer);

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

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    ok(InternetCloseHandle(request), "Close request handle failed\n");

    request = HttpOpenRequestW(connect, NULL, slash, NULL, NULL, typesW, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());
    ok(InternetCloseHandle(request), "Close request handle failed\n");

done:
    ok(InternetCloseHandle(connect), "Close connect handle failed\n");
    ok(InternetCloseHandle(session), "Close session handle failed\n");
}

static void test_http_cache(void)
{
    HINTERNET session, connect, request;
    char file_name[MAX_PATH], url[INTERNET_MAX_URL_LENGTH];
    DWORD size, file_size;
    BYTE buf[100];
    HANDLE file;
    BOOL ret;

    static const char *types[] = { "*", "", NULL };

    session = InternetOpenA("Wine Regression Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(session != NULL ,"Unable to open Internet session\n");

    connect = InternetConnectA(session, "test.winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "Unable to connect to http://test.winehq.org with error %d\n", GetLastError());

    request = HttpOpenRequestA(connect, NULL, "/hello.html", NULL, NULL, types, INTERNET_FLAG_NEED_FILE, 0);
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
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_url) failed: %u\n", GetLastError());
    ok(!strcmp(url, "http://test.winehq.org/hello.html"), "Wrong URL %s\n", url);

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(!ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_ITEM_NOT_FOUND, "GetLastError()=%u\n", GetLastError());
    ok(!size, "size = %d\n", size);

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) failed: %u\n", GetLastError());

    file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %u\n", GetLastError());
    file_size = GetFileSize(file, NULL);
    todo_wine ok(file_size == 106, "file size = %u\n", file_size);

    size = sizeof(buf);
    ret = InternetReadFile(request, buf, sizeof(buf), &size);
    ok(ret, "InternetReadFile failed: %u\n", GetLastError());
    ok(size == 100, "size = %u\n", size);

    file_size = GetFileSize(file, NULL);
    todo_wine ok(file_size == 106, "file size = %u\n", file_size);
    CloseHandle(file);

    ok(InternetCloseHandle(request), "Close request handle failed\n");

    file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    todo_wine ok(file != INVALID_HANDLE_VALUE, "CreateFile succeeded\n");
    CloseHandle(file);

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, types, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(request != NULL, "Failed to open request handle err %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    ok(!ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_ITEM_NOT_FOUND, "GetLastError()=%u\n", GetLastError());
    ok(!size, "size = %d\n", size);

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    size = sizeof(file_name);
    ret = InternetQueryOptionA(request, INTERNET_OPTION_DATAFILE_NAME, file_name, &size);
    todo_wine ok(ret, "InternetQueryOptionA(INTERNET_OPTION_DATAFILE_NAME) failed %u\n", GetLastError());

    file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    todo_wine ok(file != INVALID_HANDLE_VALUE, "CreateFile succeeded\n");
    CloseHandle(file);

    ok(InternetCloseHandle(request), "Close request handle failed\n");
    ok(InternetCloseHandle(connect), "Close connect handle failed\n");
    ok(InternetCloseHandle(session), "Close session handle failed\n");
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

    hSession = InternetOpen("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnect(hSession, "crossover.codeweavers.com",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://crossover.codeweavers.com\n");
    hRequest = HttpOpenRequest(hConnect, "POST", "/posttest.php",
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
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
               buffer,&len,&index)==0,"Warning hearder reported as Existing\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test1",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add new header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    ok(len == 5, "Invalid length (exp. 5, got %d)\n", len);
    ok((len < sizeof(buffer)) && (buffer[len] == 0), "Buffer not NULL-terminated\n"); /* len show only 5 characters but the buffer is NULL-terminated*/
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Second Index Should Not Exist\n");

    index = 0;
    len = 5; /* could store the string but not the NULL terminator */
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index) == FALSE,"Query succeeded on a too small buffer\n");
    ok(strcmp(buffer,"Warning")==0, "incorrect string was returned(%s)\n",buffer); /* string not touched */
    ok(len == 6, "Invalid length (exp. 6, got %d)\n", len); /* unlike success, the length includes the NULL-terminator */

    /* a call with NULL will fail but will return the length */
    index = 0;
    len = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");

    /* even for a len that is too small */
    index = 0;
    len = 15;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");

    index = 0;
    len = 0;
    SetLastError(0xdeadbeef);
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                NULL,&len,&index) == FALSE,"Query worked\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Unexpected last error: %d\n", GetLastError());
    ok(len > 40, "Invalid length (exp. more than 40, got %d)\n", len);
    ok(index == 0, "Index was incremented\n");
    oldlen = len;   /* bytes; at least long enough to hold buffer & nul */


    /* a working query */
    index = 0;
    len = sizeof(buffer);
    memset(buffer, 'x', sizeof(buffer));
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(len + sizeof(CHAR) <= oldlen, "Result longer than advertised\n");
    ok((len < sizeof(buffer)-sizeof(CHAR)) && (buffer[len/sizeof(CHAR)] == 0),"No NUL at end\n");
    ok(len == strlen(buffer) * sizeof(CHAR), "Length wrong\n");
    /* what's in the middle differs between Wine and Windows so currently we check only the beginning and the end */
    ok(strncmp(buffer, "POST /posttest.php HTTP/1", 25)==0, "Invalid beginning of headers string\n");
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
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,
                buffer,&len,&index) == TRUE,"Query failed\n");
    ok(len == 2, "Expected 2, got %d\n", len);
    ok(strcmp(buffer, "\r\n") == 0, "Expected CRLF, got '%s'\n", buffer);
    ok(index == 0, "Index was incremented\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test2",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add duplicate header using HTTP_ADDREQ_FLAG_ADD\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test3",-1,HTTP_ADDREQ_FLAG_REPLACE), "Failed to replace header using HTTP_ADDREQ_FLAG_REPLACE\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test4",-1,HTTP_ADDREQ_FLAG_ADD_IF_NEW)==0, "HTTP_ADDREQ_FLAG_ADD_IF_NEW replaced existing header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test4",-1, HTTP_ADDREQ_FLAG_COALESCE), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4")==0, "incorrect string was returned(%s)\n", buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test5",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test6",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5; test6")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test7",-1, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE), "HTTP_ADDREQ_FLAG_ADD with HTTP_ADDREQ_FLAG_REPALCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test7")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    /* Ensure that blank headers are ignored and don't cause a failure */
    ok(HttpAddRequestHeaders(hRequest,"\r\nBlankTest:value\r\n\r\n",-1, HTTP_ADDREQ_FLAG_ADD_IF_NEW), "Failed to add header with blank entries in list\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"BlankTest");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"value")==0, "incorrect string was returned(%s)\n",buffer);

    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
done:
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}

static const char contmsg[] =
"HTTP/1.1 100 Continue\r\n";

static const char okmsg[] =
"HTTP/1.1 200 OK\r\n"
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

struct server_info {
    HANDLE hEvent;
    int port;
};

static DWORD CALLBACK server_thread(LPVOID param)
{
    struct server_info *si = param;
    int r, c, i, on;
    SOCKET s;
    struct sockaddr_in sa;
    char buffer[0x100];
    WSADATA wsaData;
    int last_request = 0;
    char host_header[22];
    static int test_b = 0;

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

    do
    {
        c = accept(s, NULL, NULL);

        memset(buffer, 0, sizeof buffer);
        for(i=0; i<(sizeof buffer-1); i++)
        {
            r = recv(c, &buffer[i], 1, 0);
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
        if (!test_b && strstr(buffer, "/testB HTTP/1.1"))
        {
            test_b = 1;
            send(c, okmsg, sizeof okmsg-1, 0);
            recvfrom(c, buffer, sizeof buffer, 0, NULL, NULL);
            send(c, okmsg, sizeof okmsg-1, 0);
        }
        if (strstr(buffer, "/testC"))
        {
            if (strstr(buffer, "Cookie: cookie=biscuit"))
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

        shutdown(c, 2);
        closesocket(c);
    } while (!last_request);

    closesocket(s);

    return 0;
}

static void test_basic_request(int port, const char *verb, const char *url)
{
    HINTERNET hi, hc, hr;
    DWORD r, count;
    char buffer[0x100];

    hi = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(hi != NULL, "open failed\n");

    hc = InternetConnect(hi, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequest(hc, verb, url, NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    r = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed\n");

    count = 0;
    memset(buffer, 0, sizeof buffer);
    r = InternetReadFile(hr, buffer, sizeof buffer, &count);
    ok(r, "InternetReadFile failed\n");
    ok(count == sizeof page1 - 1, "count was wrong\n");
    ok(!memcmp(buffer, page1, sizeof page1), "http data wrong\n");

    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static void test_proxy_indirect(int port)
{
    HINTERNET hi, hc, hr;
    DWORD r, sz, val;
    char buffer[0x40];

    hi = InternetOpen(NULL, 0, NULL, NULL, 0);
    ok(hi != NULL, "open failed\n");

    hc = InternetConnect(hi, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequest(hc, NULL, "/test2", NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    r = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_PROXY_AUTHENTICATE, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "Basic realm=\"placebo\""), "proxy auth info wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "407"), "proxy code wrong\n");

    sz = sizeof val;
    r = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER, &val, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(val == 407, "proxy code wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_STATUS_TEXT, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "Proxy Authentication Required"), "proxy text wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_VERSION, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "HTTP/1.1"), "http version wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_SERVER, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "winetest"), "http server wrong\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_CONTENT_ENCODING, buffer, &sz, NULL);
    ok(GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND, "HttpQueryInfo should fail\n");
    ok(r == FALSE, "HttpQueryInfo failed\n");

    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static void test_proxy_direct(int port)
{
    HINTERNET hi, hc, hr;
    DWORD r, sz;
    char buffer[0x40];
    static CHAR username[] = "mike",
                password[] = "1101";

    sprintf(buffer, "localhost:%d\n", port);
    hi = InternetOpen(NULL, INTERNET_OPEN_TYPE_PROXY, buffer, NULL, 0);
    ok(hi != NULL, "open failed\n");

    /* try connect without authorization */
    hc = InternetConnect(hi, "test.winehq.org/", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequest(hc, NULL, "/test2", NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    r = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed\n");

    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "407"), "proxy code wrong\n");


    /* set the user + password then try again */
    todo_wine {
    r = InternetSetOption(hr, INTERNET_OPTION_PROXY_USERNAME, username, 4);
    ok(r, "failed to set user\n");

    r = InternetSetOption(hr, INTERNET_OPTION_PROXY_PASSWORD, password, 4);
    ok(r, "failed to set password\n");
    }

    r = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed\n");
    sz = sizeof buffer;
    r = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE, buffer, &sz, NULL);
    ok(r, "HttpQueryInfo failed\n");
    todo_wine {
    ok(!strcmp(buffer, "200"), "proxy code wrong\n");
    }


    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static void test_header_handling_order(int port)
{
    static char authorization[] = "Authorization: Basic dXNlcjpwd2Q=";
    static char connection[]    = "Connection: Close";

    static const char *types[2] = { "*", NULL };
    HINTERNET session, connect, request;
    DWORD size, status;
    BOOL ret;

    session = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnect(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequest(connect, NULL, "/test3", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeaders(request, authorization, ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(request);

    request = HttpOpenRequest(connect, NULL, "/test4", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(request, connection, ~0u, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200 || status == 400 /* IE6 */, "got status %u, expected 200 or 400\n", status);

    InternetCloseHandle(request);

    request = HttpOpenRequest(connect, "POST", "/test7", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpAddRequestHeaders(request, "Content-Length: 100\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders failed\n");

    ret = HttpSendRequest(request, connection, ~0u, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200 || status == 400 /* IE6 */, "got status %u, expected 200 or 400\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_connection_header(int port)
{
    HINTERNET ses, con, req;
    DWORD size, status;
    BOOL ret;

    ses = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnect(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequest(con, NULL, "/test8", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(req);

    req = HttpOpenRequest(con, NULL, "/test9", NULL, NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(req);

    req = HttpOpenRequest(con, NULL, "/test9", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(req);

    req = HttpOpenRequest(con, "POST", "/testA", NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_http1_1(int port)
{
    HINTERNET ses, con, req;
    BOOL ret;

    ses = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnect(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequest(con, NULL, "/testB", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    if (ret)
    {
        InternetCloseHandle(req);

        req = HttpOpenRequest(con, NULL, "/testB", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
        ok(req != NULL, "HttpOpenRequest failed\n");

        ret = HttpSendRequest(req, NULL, 0, NULL, 0);
        todo_wine
        ok(ret, "HttpSendRequest failed\n");
    }

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_HttpSendRequestW(int port)
{
    static const WCHAR header[] = {'U','A','-','C','P','U',':',' ','x','8','6',0};
    HINTERNET ses, con, req;
    DWORD error;
    BOOL ret;

    ses = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnect(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequest(con, NULL, "/test1", NULL, NULL, NULL, 0, 0);
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
    DWORD size, status, error;
    BOOL ret;
    char buffer[64];

    ses = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnect(ses, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    InternetSetCookie("http://localhost", "cookie", "biscuit");

    req = HttpOpenRequest(con, NULL, "/testC", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    buffer[0] = 0;
    size = sizeof(buffer);
    SetLastError(0xdeadbeef);
    ret = HttpQueryInfo(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    error = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(error == ERROR_HTTP_HEADER_NOT_FOUND, "got %u expected ERROR_HTTP_HEADER_NOT_FOUND\n", error);

    ret = HttpAddRequestHeaders(req, "Cookie: cookie=not biscuit\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD);
    ok(ret, "HttpAddRequestHeaders failed: %u\n", GetLastError());

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!strcmp(buffer, "cookie=not biscuit"), "got '%s' expected \'cookie=not biscuit\'\n", buffer);

    ret = HttpSendRequest(req, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed: %u\n", GetLastError());

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_COOKIE | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!strcmp(buffer, "cookie=biscuit"), "got '%s' expected \'cookie=biscuit\'\n", buffer);

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
}

static void test_basic_authentication(int port)
{
    HINTERNET session, connect, request;
    DWORD size, status;
    BOOL ret;

    session = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnect(session, "localhost", port, "user", "pwd", INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequest(connect, NULL, "/test3", NULL, NULL, NULL, 0, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_invalid_response_headers(int port)
{
    HINTERNET session, connect, request;
    DWORD size, status;
    BOOL ret;
    char buffer[256];

    session = InternetOpen("winetest", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(session != NULL, "InternetOpen failed\n");

    connect = InternetConnect(session, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "InternetConnect failed\n");

    request = HttpOpenRequest(connect, NULL, "/testE", NULL, NULL, NULL, 0, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(request, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed %u\n", GetLastError());

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 401, "unexpected status %u\n", status);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo( request, HTTP_QUERY_RAW_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "HTTP/1.0 401 Anonymous requests or requests on unsecure channel are not allowed"),
       "headers wrong \"%s\"\n", buffer);

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo( request, HTTP_QUERY_SERVER, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed\n");
    ok(!strcmp(buffer, "winetest"), "server wrong \"%s\"\n", buffer);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
}

static void test_HttpQueryInfo(int port)
{
    HINTERNET hi, hc, hr;
    DWORD size, index;
    char buffer[1024];
    BOOL ret;

    hi = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(hi != NULL, "InternetOpen failed\n");

    hc = InternetConnect(hi, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "InternetConnect failed\n");

    hr = HttpOpenRequest(hc, NULL, "/testD", NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_HOST | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_DATE | HTTP_QUERY_FLAG_SYSTEMTIME, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_RAW_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_RAW_HEADERS, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_STATUS_TEXT, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_VERSION, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 0, "expected 0 got %u\n", index);

    index = 0xdeadbeef;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_FORWARDED, buffer, &size, &index);
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(index == 0xdeadbeef, "expected 0xdeadbeef got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_SERVER, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    strcpy(buffer, "Server");
    ret = HttpQueryInfo(hr, HTTP_QUERY_CUSTOM, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    index = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_SET_COOKIE, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 1, "expected 1 got %u\n", index);

    size = sizeof(buffer);
    ret = HttpQueryInfo(hr, HTTP_QUERY_SET_COOKIE, buffer, &size, &index);
    ok(ret, "HttpQueryInfo failed %u\n", GetLastError());
    ok(index == 2, "expected 2 got %u\n", index);

    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static void test_http_connection(void)
{
    struct server_info si;
    HANDLE hThread;
    DWORD id = 0, r;

    si.hEvent = CreateEvent(NULL, 0, 0, NULL);
    si.port = 7531;

    hThread = CreateThread(NULL, 0, server_thread, (LPVOID) &si, 0, &id);
    ok( hThread != NULL, "create thread failed\n");

    r = WaitForSingleObject(si.hEvent, 10000);
    ok (r == WAIT_OBJECT_0, "failed to start wininet test server\n");
    if (r != WAIT_OBJECT_0)
        return;

    test_basic_request(si.port, "GET", "/test1");
    test_proxy_indirect(si.port);
    test_proxy_direct(si.port);
    test_header_handling_order(si.port);
    test_basic_request(si.port, "POST", "/test5");
    test_basic_request(si.port, "RPC_IN_DATA", "/test5");
    test_basic_request(si.port, "RPC_OUT_DATA", "/test5");
    test_basic_request(si.port, "GET", "/test6");
    test_connection_header(si.port);
    test_http1_1(si.port);
    test_cookie_header(si.port);
    test_basic_authentication(si.port);
    test_invalid_response_headers(si.port);
    test_HttpQueryInfo(si.port);
    test_HttpSendRequestW(si.port);

    /* send the basic request again to shutdown the server thread */
    test_basic_request(si.port, "GET", "/quit");

    r = WaitForSingleObject(hThread, 3000);
    ok( r == WAIT_OBJECT_0, "thread wait failed\n");
    CloseHandle(hThread);
}

static void test_user_agent_header(void)
{
    HINTERNET ses, con, req;
    DWORD size, err;
    char buffer[64];
    BOOL ret;

    ses = InternetOpen("Gizmo5", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnect(ses, "test.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequest(con, "GET", "/hello.html", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_USER_AGENT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", err);

    ret = HttpAddRequestHeaders(req, "User-Agent: Gizmo Project\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders succeeded\n");

    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_USER_AGENT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(ret, "HttpQueryInfo failed\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", err);

    InternetCloseHandle(req);

    req = HttpOpenRequest(con, "GET", "/", "HTTP/1.0", NULL, NULL, 0, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    err = GetLastError();
    ok(!ret, "HttpQueryInfo succeeded\n");
    ok(err == ERROR_HTTP_HEADER_NOT_FOUND, "expected ERROR_HTTP_HEADER_NOT_FOUND, got %u\n", err);

    ret = HttpAddRequestHeaders(req, "Accept: audio/*, image/*, text/*\r\nUser-Agent: Gizmo Project\r\n", ~0u, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret, "HttpAddRequestHeaders failed\n");

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
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
    DWORD size;
    char buffer[32];
    BOOL ret;

    ses = InternetOpen("MERONG(0.9/;p)", INTERNET_OPEN_TYPE_DIRECT, "", "", 0);
    con = InternetConnect(ses, "www.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    req = HttpOpenRequest(con, "POST", "/post/post_action.php", "HTTP/1.0", "", types, INTERNET_FLAG_FORMS_SUBMIT, 0);

    ok(req != NULL, "HttpOpenRequest failed: %u\n", GetLastError());

    buffer[0] = 0;
    size = sizeof(buffer);
    ret = HttpQueryInfo(req, HTTP_QUERY_ACCEPT | HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer, &size, NULL);
    ok(ret, "HttpQueryInfo failed: %u\n", GetLastError());
    ok(!strcmp(buffer, ", */*, %p, , , */*") || /* IE6 */
       !strcmp(buffer, "*/*, %p, */*"),
       "got '%s' expected '*/*, %%p, */*' or ', */*, %%p, , , */*'\n", buffer);

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

    trace("%p 0x%08lx %u %p 0x%08x\n", handle, context, status, info, size);

    if (status == INTERNET_STATUS_REQUEST_COMPLETE)
    {
        trace("request handle: 0x%08lx\n", result->dwResult);
        ctx->req = (HINTERNET)result->dwResult;
        SetEvent(ctx->event);
    }
    if (status == INTERNET_STATUS_HANDLE_CLOSING)
    {
        DWORD type = INTERNET_HANDLE_TYPE_CONNECT_HTTP, size = sizeof(type);

        if (InternetQueryOption(handle, INTERNET_OPTION_HANDLE_TYPE, &type, &size))
            ok(type != INTERNET_HANDLE_TYPE_CONNECT_HTTP, "unexpected callback\n");
        SetEvent(ctx->event);
    }
}

static void test_open_url_async(void)
{
    BOOL ret;
    HINTERNET ses, req;
    DWORD size, error;
    struct context ctx;
    ULONG type;

    ctx.req = NULL;
    ctx.event = CreateEvent(NULL, TRUE, FALSE, "Z:_home_hans_jaman-installer.exe_ev1");

    ses = InternetOpen("AdvancedInstaller", 0, NULL, NULL, INTERNET_FLAG_ASYNC);
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

    pInternetSetStatusCallbackA(ses, cb);
    ResetEvent(ctx.event);

    req = InternetOpenUrl(ses, "http://test.winehq.org", NULL, 0, 0, (DWORD_PTR)&ctx);
    ok(!req && GetLastError() == ERROR_IO_PENDING, "InternetOpenUrl failed\n");

    WaitForSingleObject(ctx.event, INFINITE);

    type = 0;
    size = sizeof(type);
    ret = InternetQueryOption(ctx.req, INTERNET_OPTION_HANDLE_TYPE, &type, &size);
    ok(ret, "InternetQueryOption failed: %u\n", GetLastError());
    ok(type == INTERNET_HANDLE_TYPE_HTTP_REQUEST,
       "expected INTERNET_HANDLE_TYPE_HTTP_REQUEST, got %u\n", type);

    size = 0;
    ret = HttpQueryInfo(ctx.req, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &size, NULL);
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
    int          async;    /* delivered from another thread? */
    int          todo;
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
};

static CRITICAL_SECTION notification_cs;

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    BOOL status_ok, function_ok;
    struct info *info = (struct info *)context;
    unsigned int i;

    EnterCriticalSection( &notification_cs );

    if (status == INTERNET_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        HttpQueryInfoA( handle, INTERNET_OPTION_CONTEXT_VALUE, &info, &size, 0 );
    }
    i = info->index;
    if (i >= info->count)
    {
        LeaveCriticalSection( &notification_cs );
        return;
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
    info->index++;

    LeaveCriticalSection( &notification_cs );
}

static void setup_test( struct info *info, enum api function, unsigned int line )
{
    info->function = function;
    info->line = line;
}

static const struct notification async_send_request_ex_test[] =
{
    { internet_connect,      INTERNET_STATUS_HANDLE_CREATED, 0 },
    { http_open_request,     INTERNET_STATUS_HANDLE_CREATED, 0 },
    { http_send_request_ex,  INTERNET_STATUS_RESOLVING_NAME, 1 },
    { http_send_request_ex,  INTERNET_STATUS_NAME_RESOLVED, 1 },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTING_TO_SERVER, 1 },
    { http_send_request_ex,  INTERNET_STATUS_CONNECTED_TO_SERVER, 1 },
    { http_send_request_ex,  INTERNET_STATUS_SENDING_REQUEST, 1 },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_SENT, 1 },
    { http_send_request_ex,  INTERNET_STATUS_REQUEST_COMPLETE, 1 },
    { internet_writefile,    INTERNET_STATUS_SENDING_REQUEST, 0 },
    { internet_writefile,    INTERNET_STATUS_REQUEST_SENT, 0 },
    { http_end_request,      INTERNET_STATUS_RECEIVING_RESPONSE, 1 },
    { http_end_request,      INTERNET_STATUS_RESPONSE_RECEIVED, 1 },
    { http_end_request,      INTERNET_STATUS_REQUEST_COMPLETE, 1 },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, 0, 1 },
    { internet_close_handle, INTERNET_STATUS_HANDLE_CLOSING, 0, 1 }
};

static void test_async_HttpSendRequestEx(void)
{
    BOOL ret;
    HINTERNET ses, req, con;
    struct info info;
    DWORD size, written, error;
    INTERNET_BUFFERSA b;
    static const char *accept[2] = {"*/*", NULL};
    static char data[] = "Public ID=codeweavers";
    char buffer[32];

    InitializeCriticalSection( &notification_cs );

    info.test  = async_send_request_ex_test;
    info.count = sizeof(async_send_request_ex_test)/sizeof(async_send_request_ex_test[0]);
    info.index = 0;
    info.wait = CreateEvent( NULL, FALSE, FALSE, NULL );
    info.thread = GetCurrentThreadId();

    ses = InternetOpen( "winetest", 0, NULL, NULL, INTERNET_FLAG_ASYNC );
    ok( ses != NULL, "InternetOpen failed\n" );

    pInternetSetStatusCallbackA( ses, check_notification );

    setup_test( &info, internet_connect, __LINE__ );
    con = InternetConnect( ses, "crossover.codeweavers.com", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)&info );
    ok( con != NULL, "InternetConnect failed %u\n", GetLastError() );

    WaitForSingleObject( info.wait, 10000 );

    setup_test( &info, http_open_request, __LINE__ );
    req = HttpOpenRequest( con, "POST", "posttest.php", NULL, NULL, accept, 0, (DWORD_PTR)&info );
    ok( req != NULL, "HttpOpenRequest failed %u\n", GetLastError() );

    WaitForSingleObject( info.wait, 10000 );

    memset( &b, 0, sizeof(INTERNET_BUFFERSA) );
    b.dwStructSize = sizeof(INTERNET_BUFFERSA);
    b.lpcszHeader = "Content-Type: application/x-www-form-urlencoded";
    b.dwHeadersLength = strlen( b.lpcszHeader );
    b.dwBufferTotal = strlen( data );

    setup_test( &info, http_send_request_ex, __LINE__ );
    ret = HttpSendRequestExA( req, &b, NULL, 0x28, 0 );
    ok( !ret && GetLastError() == ERROR_IO_PENDING, "HttpSendRequestExA failed %d %u\n", ret, GetLastError() );

    WaitForSingleObject( info.wait, 10000 );

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    ret = HttpQueryInfoA( req, HTTP_QUERY_CONTENT_ENCODING, buffer, &size, 0 );
    error = GetLastError();
    ok( !ret, "HttpQueryInfoA failed %u\n", GetLastError() );
    todo_wine
    ok( error == ERROR_INTERNET_INCORRECT_HANDLE_STATE,
        "expected ERROR_INTERNET_INCORRECT_HANDLE_STATE got %u\n", error );

    written = 0;
    size = strlen( data );
    setup_test( &info, internet_writefile, __LINE__ );
    ret = InternetWriteFile( req, data, size, &written );
    ok( ret, "InternetWriteFile failed %u\n", GetLastError() );
    ok( written == size, "expected %u got %u\n", written, size );

    WaitForSingleObject( info.wait, 10000 );

    SetLastError( 0xdeadbeef );
    ret = HttpEndRequestA( req, (void *)data, 0x28, 0 );
    error = GetLastError();
    ok( !ret, "HttpEndRequestA succeeded\n" );
    ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error );

    SetLastError( 0xdeadbeef );
    setup_test( &info, http_end_request, __LINE__ );
    ret = HttpEndRequestA( req, NULL, 0x28, 0 );
    error = GetLastError();
    ok( !ret, "HttpEndRequestA succeeded\n" );
    ok( error == ERROR_IO_PENDING, "expected ERROR_IO_PENDING got %u\n", error );

    WaitForSingleObject( info.wait, 10000 );

    setup_test( &info, internet_close_handle, __LINE__ );
    InternetCloseHandle( req );
    InternetCloseHandle( con );
    InternetCloseHandle( ses );

    WaitForSingleObject( info.wait, 10000 );
    CloseHandle( info.wait );
}

#define STATUS_STRING(status) \
    memcpy(status_string[status], #status, sizeof(CHAR) * \
           (strlen(#status) < MAX_STATUS_NAME ? \
            strlen(#status) : \
            MAX_STATUS_NAME - 1))
static void init_status_tests(void)
{
    memset(expect, 0, sizeof(expect));
    memset(optional, 0, sizeof(optional));
    memset(wine_allow, 0, sizeof(wine_allow));
    memset(notified, 0, sizeof(notified));
    memset(status_string, 0, sizeof(status_string));
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
}
#undef STATUS_STRING

START_TEST(http)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");
    pInternetSetStatusCallbackA = (void*)GetProcAddress(hdll, "InternetSetStatusCallbackA");

    if (!pInternetSetStatusCallbackA)
        skip("skipping the InternetReadFile tests\n");
    else
    {
        init_status_tests();
        InternetReadFile_test(INTERNET_FLAG_ASYNC);
        InternetReadFile_test(0);
        InternetReadFileExA_test(INTERNET_FLAG_ASYNC);
        test_open_url_async();
        test_async_HttpSendRequestEx();
    }
    InternetOpenRequest_test();
    test_http_cache();
    InternetOpenUrlA_test();
    HttpSendRequestEx_test();
    HttpHeaders_test();
    test_http_connection();
    test_user_agent_header();
    test_bogus_accept_types_array();
}
