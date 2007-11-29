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

#define TEST_URL "http://www.winehq.org/site/about"

static BOOL first_connection_to_test_url = TRUE;

/* Adapted from dlls/urlmon/tests/protocol.c */

#define SET_EXPECT2(status, num) \
    expect[status] = num

#define SET_EXPECT(status) \
    SET_EXPECT2(status, 1)

/* SET_WINE_ALLOW's should be used with an appropriate
 * todo_wine CHECK_NOTIFIED at a later point in the code */
#define SET_WINE_ALLOW2(status, num) \
    wine_allow[status] = num

#define SET_WINE_ALLOW(status) \
    SET_WINE_ALLOW2(status, 1)

#define CHECK_EXPECT(status) \
    do { \
        if (!expect[status] && wine_allow[status]) \
        { \
            todo_wine ok(expect[status], "unexpected status %d (%s)\n", status, \
                         status < MAX_INTERNET_STATUS && status_string[status][0] != 0 ? \
                         status_string[status] : "unknown");            \
            wine_allow[status]--; \
        } \
        else \
        { \
            ok(expect[status], "unexpected status %d (%s)\n", status,   \
               status < MAX_INTERNET_STATUS && status_string[status][0] != 0 ? \
               status_string[status] : "unknown");                      \
            expect[status]--;                                           \
        } \
        notified[status]++; \
    }while(0)

/* CLEAR_NOTIFIED used in cases when notification behavior
 * differs between Windows versions */
#define CLEAR_NOTIFIED(status) \
    expect[status] = wine_allow[status] = notified[status] = 0;

#define CHECK_NOTIFIED2(status, num) \
    do { \
        ok(notified[status] == (num), "expected status %d (%s) %d times, received %d times\n", \
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
static int expect[MAX_INTERNET_STATUS], wine_allow[MAX_INTERNET_STATUS],
    notified[MAX_INTERNET_STATUS];
static CHAR status_string[MAX_INTERNET_STATUS][MAX_STATUS_NAME];

static HANDLE hCompleteEvent;

static INTERNET_STATUS_CALLBACK (WINAPI *pInternetSetStatusCallbackA)(HINTERNET ,INTERNET_STATUS_CALLBACK);
static BOOL (WINAPI *pInternetTimeFromSystemTimeA)(CONST SYSTEMTIME *,DWORD ,LPSTR ,DWORD);
static BOOL (WINAPI *pInternetTimeFromSystemTimeW)(CONST SYSTEMTIME *,DWORD ,LPWSTR ,DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeA)(LPCSTR ,SYSTEMTIME *,DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeW)(LPCWSTR ,SYSTEMTIME *,DWORD);


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
    DWORD rc;
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
    hic=InternetConnectA(hi, "www.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %u\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
    SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/about/", NULL, NULL, types,
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
    todo_wine
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    if (first_connection_to_test_url)
    {
        SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
        SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
    }
    else
    {
        SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
        SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);
    }
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT2(INTERNET_STATUS_SENDING_REQUEST, 2);
    SET_EXPECT2(INTERNET_STATUS_REQUEST_SENT, 2);
    SET_EXPECT2(INTERNET_STATUS_RECEIVING_RESPONSE, 2);
    SET_EXPECT2(INTERNET_STATUS_RESPONSE_RECEIVED, 2);
    SET_EXPECT(INTERNET_STATUS_REDIRECT);
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
        todo_wine CHECK_NOT_NOTIFIED(INTERNET_STATUS_REQUEST_COMPLETE);
    /* Sent on WinXP only if first_connection_to_test_url is TRUE, on Win98 always sent */
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTING_TO_SERVER);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTED_TO_SERVER);

    length = 4;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_REQUEST_FLAGS,&out,&length);
    trace("Option 0x17 -> %i  %i\n",rc,out);

    length = 100;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    trace("Option 0x22 -> %i  %s\n",rc,buffer);

    length = 4000;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x16 -> %i  %s\n",rc,buffer);

    length = 4000;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    buffer[length]=0;
    trace("Option 0x22 -> %i  %s\n",rc,buffer);

    length = 16;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,&buffer,&length,0x0);
    trace("Option 0x5 -> %i  %s  (%u)\n",rc,buffer,GetLastError());

    length = 100;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_TYPE,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x1 -> %i  %s\n",rc,buffer);

    SetLastError(0xdeadbeef);
    rc = InternetReadFile(NULL, buffer, 100, &length);
    ok(!rc, "InternetReadFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "InternetReadFile should have set last error to ERROR_INVALID_HANDLE instead of %u\n",
        GetLastError());

    length = 100;
    trace("Entering Query loop\n");

    SET_EXPECT(INTERNET_STATUS_CLOSING_CONNECTION);
    SET_EXPECT(INTERNET_STATUS_CONNECTION_CLOSED);
    while (TRUE)
    {
        if (flags & INTERNET_FLAG_ASYNC)
            SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
        rc = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok(!(rc == 0 && length != 0),"InternetQueryDataAvailable failed with non-zero length\n");
        ok(rc != 0 || ((flags & INTERNET_FLAG_ASYNC) && GetLastError() == ERROR_IO_PENDING),
           "InternetQueryDataAvailable failed, error %d\n", GetLastError());
        if (flags & INTERNET_FLAG_ASYNC)
        {
            if (rc != 0)
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

            rc = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            trace("ReadFile -> %i %i\n",rc,length);

            HeapFree(GetProcessHeap(),0,buffer);
        }
        if (length == 0)
            break;
    }
    /* WinXP does not send, but Win98 does */
    CLEAR_NOTIFIED(INTERNET_STATUS_CLOSING_CONNECTION);
    CLEAR_NOTIFIED(INTERNET_STATUS_CONNECTION_CLOSED);
abort:
    SET_EXPECT2(INTERNET_STATUS_HANDLE_CLOSING, (hor != 0x0) + (hic != 0x0));
    if (hor != 0x0) {
        SET_WINE_ALLOW(INTERNET_STATUS_CLOSING_CONNECTION);
        SET_WINE_ALLOW(INTERNET_STATUS_CONNECTION_CLOSED);
        SetLastError(0xdeadbeef);
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        SetLastError(0xdeadbeef);
        rc = InternetCloseHandle(hor);
        ok ((rc == 0), "Double close of handle opened by HttpOpenRequestA succeeded\n");
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
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed\n");
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
    hic=InternetConnectA(hi, "www.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %u\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    CHECK_NOTIFIED(INTERNET_STATUS_HANDLE_CREATED);
    SET_EXPECT(INTERNET_STATUS_HANDLE_CREATED);
    SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
    SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/about/", NULL, NULL, types,
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
    todo_wine
    {
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_RESOLVING_NAME);
        CHECK_NOT_NOTIFIED(INTERNET_STATUS_NAME_RESOLVED);
    }
    if (first_connection_to_test_url)
    {
        SET_EXPECT(INTERNET_STATUS_RESOLVING_NAME);
        SET_EXPECT(INTERNET_STATUS_NAME_RESOLVED);
    }
    else
    {
        SET_WINE_ALLOW(INTERNET_STATUS_RESOLVING_NAME);
        SET_WINE_ALLOW(INTERNET_STATUS_NAME_RESOLVED);
    }
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTING_TO_SERVER);
    SET_WINE_ALLOW(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT(INTERNET_STATUS_CONNECTED_TO_SERVER);
    SET_EXPECT2(INTERNET_STATUS_SENDING_REQUEST, 2);
    SET_EXPECT2(INTERNET_STATUS_REQUEST_SENT, 2);
    SET_EXPECT2(INTERNET_STATUS_RECEIVING_RESPONSE, 2);
    SET_EXPECT2(INTERNET_STATUS_RESPONSE_RECEIVED, 2);
    SET_EXPECT(INTERNET_STATUS_REDIRECT);
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
    SET_WINE_ALLOW(INTERNET_STATUS_RECEIVING_RESPONSE);
    SET_WINE_ALLOW(INTERNET_STATUS_RESPONSE_RECEIVED);
    rc = InternetReadFileEx(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(rc, "InternetReadFileEx failed with error %u\n", GetLastError());
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

        SET_EXPECT(INTERNET_STATUS_RECEIVING_RESPONSE);
        SET_EXPECT(INTERNET_STATUS_REQUEST_COMPLETE);
        SET_EXPECT(INTERNET_STATUS_RESPONSE_RECEIVED);
        rc = InternetReadFileExA(hor, &inetbuffers, IRF_ASYNC | IRF_USE_CONTEXT, 0xcafebabe);
        if (!rc)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                trace("InternetReadFileEx -> PENDING\n");
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
                CHECK_NOTIFIED(INTERNET_STATUS_RECEIVING_RESPONSE);
                CHECK_NOTIFIED(INTERNET_STATUS_RESPONSE_RECEIVED);
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

static void InternetTimeFromSystemTimeA_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    char string[INTERNET_RFC1123_BUFSIZE];
    static const char expect[] = "Fri, 07 Jan 2005 12:06:35 GMT";

    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeA failed (%u)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeA failed (%u)\n", GetLastError() );
}

static void InternetTimeFromSystemTimeW_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    WCHAR string[INTERNET_RFC1123_BUFSIZE + 1];
    static const WCHAR expect[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };

    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeW failed (%u)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeW failed (%u)\n", GetLastError() );
}

static void InternetTimeToSystemTimeA_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const char string[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    static const char string2[] = " fri 7 jan 2005 12 06 35";

    ret = pInternetTimeToSystemTimeA( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeA( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
}

static void InternetTimeToSystemTimeW_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const WCHAR string[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };
    static const WCHAR string2[] = { ' ','f','r','i',' ','7',' ','j','a','n',' ','2','0','0','5',' ',
                                     '1','2',' ','0','6',' ','3','5',0 };
    static const WCHAR string3[] = { 'F','r',0 };

    ret = pInternetTimeToSystemTimeW( NULL, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( NULL, &time, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string3, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
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
        trace( "Network unreachable, skipping test\n" );
        goto done;
    }
    ok( hRequest != NULL, "Failed to open request handle err %u\n", GetLastError());


    BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS);
    BufferIn.Next = (LPINTERNET_BUFFERS)0xdeadcab;
    BufferIn.lpcszHeader = szContentType;
    BufferIn.dwHeadersLength = sizeof(szContentType);
    BufferIn.dwHeadersTotal = sizeof(szContentType);
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

    connect = InternetConnectA(session, "winehq.org", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                              INTERNET_SERVICE_HTTP, 0, 0);
    ok(connect != NULL, "Unable to connect to http://winehq.org\n");

    request = HttpOpenRequestA(connect, NULL, "/", NULL, NULL, types, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!request && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED)
    {
        trace( "Network unreachable, skipping test\n" );
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

static void HttpHeaders_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    CHAR      buffer[256];
    DWORD     len = 256;
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
        trace( "Network unreachable, skipping test\n" );
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
    ok(buffer[len] == 0, "Buffer not NULL-terminated\n"); /* len show only 5 characters but the buffer is NULL-terminated*/
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


    /* a working query */
    index = 0;
    len = sizeof(buffer);
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    /* what's in the middle differs between Wine and Windows so currently we check only the beginning and the end */
    ok(strncmp(buffer, "POST /posttest.php HTTP/1", 25)==0, "Invalid beginning of headers string\n");
    ok(strcmp(buffer + strlen(buffer) - 4, "\r\n\r\n")==0, "Invalid end of headers string\n");
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

    
    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
done:
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}

static const char okmsg[] =
"HTTP/1.0 200 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char notokmsg[] =
"HTTP/1.0 400 Bad Request\r\n"
"Server: winetest\r\n"
"\r\n";

static const char noauthmsg[] =
"HTTP/1.0 401 Unauthorized\r\n"
"Server: winetest\r\n"
"\r\n";

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
            send(c, okmsg, sizeof okmsg-1, 0);
            send(c, page1, sizeof page1-1, 0);
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

        if (strstr(buffer, "/test5"))
        {
            if (strstr(buffer, "Content-Length: 0"))
            {
                send(c, okmsg, sizeof okmsg-1, 0);
                send(c, page1, sizeof page1-1, 0);
            }
            else
                send(c, notokmsg, sizeof notokmsg-1, 0);
        }

        if (strstr(buffer, "/quit"))
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

static void test_basic_request(int port, const char *url)
{
    HINTERNET hi, hc, hr;
    DWORD r, count;
    char buffer[0x100];

    hi = InternetOpen(NULL, 0, NULL, NULL, 0);
    ok(hi != NULL, "open failed\n");

    hc = InternetConnect(hi, "localhost", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequest(hc, NULL, url, NULL, NULL, NULL, 0, 0);
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
    hc = InternetConnect(hi, "www.winehq.org/", port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
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

    ret = HttpSendRequest(request, authorization, ~0UL, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(request);

    request = HttpOpenRequest(connect, NULL, "/test4", NULL, NULL, types, INTERNET_FLAG_KEEP_CONNECTION, 0);
    ok(request != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequest(request, connection, ~0UL, NULL, 0);
    ok(ret, "HttpSendRequest failed\n");

    status = 0;
    size = sizeof(status);
    ret = HttpQueryInfo( request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &size, NULL );
    ok(ret, "HttpQueryInfo failed\n");
    ok(status == 200, "request failed with status %u\n", status);

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);
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

    test_basic_request(si.port, "/test1");
    test_proxy_indirect(si.port);
    test_proxy_direct(si.port);
    test_header_handling_order(si.port);
    test_basic_request(si.port, "/test5");

    /* send the basic request again to shutdown the server thread */
    test_basic_request(si.port, "/quit");

    r = WaitForSingleObject(hThread, 3000);
    ok( r == WAIT_OBJECT_0, "thread wait failed\n");
    CloseHandle(hThread);
}

#define STATUS_STRING(status) \
    memcpy(status_string[status], #status, sizeof(CHAR) * \
           (strlen(#status) < MAX_STATUS_NAME ? \
            strlen(#status) : \
            MAX_STATUS_NAME - 1))
static void init_status_tests(void)
{
    memset(expect, 0, sizeof(expect));
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
    pInternetTimeFromSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeA");
    pInternetTimeFromSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeW");
    pInternetTimeToSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeA");
    pInternetTimeToSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeW");

    if (!pInternetSetStatusCallbackA)
        skip("skipping the InternetReadFile tests\n");
    else
    {
        init_status_tests();
        InternetReadFile_test(INTERNET_FLAG_ASYNC);
        InternetReadFile_test(0);
        InternetReadFileExA_test(INTERNET_FLAG_ASYNC);
    }
    InternetOpenRequest_test();
    InternetOpenUrlA_test();
    if (!pInternetTimeFromSystemTimeA)
        skip("skipping the InternetTime tests\n");
    else
    {
        InternetTimeFromSystemTimeA_test();
        InternetTimeFromSystemTimeW_test();
        InternetTimeToSystemTimeA_test();
        InternetTimeToSystemTimeW_test();
    }
    HttpSendRequestEx_test();
    HttpHeaders_test();
    test_http_connection();
}
