/*
 * WinHTTP - tests
 *
 * Copyright 2008 Google (Zac Brown)
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
#include <wincrypt.h>

#include "wine/test.h"

static const WCHAR test_useragent[] =
    {'W','i','n','e',' ','R','e','g','r','e','s','s','i','o','n',' ','T','e','s','t',0};
static const WCHAR test_server[] = {'w','i','n','e','h','q','.','o','r','g',0};

static void test_QueryOption(void)
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
    connection = WinHttpConnect(session, test_server, INTERNET_DEFAULT_HTTP_PORT, 0);
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

static void test_OpenRequest (void)
{
    BOOL ret;
    HINTERNET session, request, connection;

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    ok(session != NULL, "WinHttpOpen failed to open session.\n");

    /* Test with a bad server name */
    SetLastError(0xdeadbeef);
    connection = WinHttpConnect(session, NULL, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok (connection == NULL, "WinHttpConnect succeeded in opening connection to NULL server argument.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u.\n", GetLastError());

    /* Test with a valid server name */
    connection = WinHttpConnect (session, test_server, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, NULL, NULL, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    ok(request != NULL, "WinHttpOpenrequest failed to open a request, error: %u.\n", GetLastError());

    ret = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
    ok(ret == TRUE, "WinHttpSendRequest failed: %u\n", GetLastError());
    ret = WinHttpCloseHandle(request);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);

 done:
    ret = WinHttpCloseHandle(connection);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);

}

static void test_SendRequest (void)
{
    HINTERNET session, request, connection;
    DWORD header_len, optional_len, total_len;
    DWORD bytes_rw;
    BOOL ret;
    CHAR buffer[256];
    int i;

    static const WCHAR test_site[] = {'c','r','o','s','s','o','v','e','r','.',
                                'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR content_type[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','a','p','p','l','i','c','a','t','i','o','n',
         '/','x','-','w','w','w','-','f','o','r','m','-','u','r','l','e','n','c','o','d','e','d',0};
    static const WCHAR test_file[] = {'/','p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR test_verb[] = {'P','O','S','T',0};
    static CHAR post_data[] = "mode=Test";
    static CHAR test_post[] = "mode => Test\\0\n";

    header_len = -1L;
    total_len = optional_len = sizeof(post_data);
    memset(buffer, 0xff, sizeof(buffer));

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    ok(session != NULL, "WinHttpOpen failed to open session.\n");

    connection = WinHttpConnect (session, test_site, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok(connection != NULL, "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, test_verb, test_file, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    ok(request != NULL, "WinHttpOpenrequest failed to open a request, error: %u.\n", GetLastError());

    ret = WinHttpSendRequest(request, content_type, header_len, post_data, optional_len, total_len, 0);
    ok(ret == TRUE, "WinHttpSendRequest failed: %u\n", GetLastError());

    for (i = 3; post_data[i]; i++)
    {
        bytes_rw = -1;
        ret = WinHttpWriteData(request, &post_data[i], 1, &bytes_rw);
        ok(ret == TRUE, "WinHttpWriteData failed: %u.\n", GetLastError());
        ok(bytes_rw == 1, "WinHttpWriteData failed, wrote %u bytes instead of 1 byte.\n", bytes_rw);
    }

    ret = WinHttpReceiveResponse(request, NULL);
    ok(ret == TRUE, "WinHttpReceiveResponse failed: %u.\n", GetLastError());

    bytes_rw = -1;
    ret = WinHttpReadData(request, buffer, sizeof(buffer) - 1, &bytes_rw);
    ok(ret == TRUE, "WinHttpReadData failed: %u.\n", GetLastError());

    ok(bytes_rw == strlen(test_post), "Read %u bytes instead of %d.\n", bytes_rw, lstrlen(test_post));
    ok(strncmp(buffer, test_post, bytes_rw) == 0, "Data read did not match, got '%s'.\n", buffer);

    ret = WinHttpCloseHandle(request);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);
 done:
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

    ret = WinHttpTimeFromSystemTime(&time, time_string);
    ok(ret == TRUE, "WinHttpTimeFromSystemTime failed: %u\n", GetLastError());
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

    ret = WinHttpTimeToSystemTime(time_string1, &time);
    ok(ret == TRUE, "WinHttpTimeToSystemTime failed: %u\n", GetLastError());
    ok(memcmp(&time, &expected_time, sizeof(SYSTEMTIME)) == 0,
        "Returned SYSTEMTIME structure did not match expected SYSTEMTIME structure.\n");

    ret = WinHttpTimeToSystemTime(time_string2, &time);
    ok(ret == TRUE, "WinHttpTimeToSystemTime failed: %u\n", GetLastError());
    ok(memcmp(&time, &expected_time, sizeof(SYSTEMTIME)) == 0,
        "Returned SYSTEMTIME structure did not match expected SYSTEMTIME structure.\n");
}

static void test_WinHttpAddHeaders(void)
{
    HINTERNET session, request, connection;
    BOOL ret;
    WCHAR buffer[MAX_PATH];
    WCHAR check_buffer[MAX_PATH];
    DWORD index, len, oldlen;

    static const WCHAR test_site[] = {'c','r','o','s','s','o','v','e','r','.',
                                'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR test_file[] = {'/','p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR test_verb[] = {'P','O','S','T',0};

    static const WCHAR test_header_begin[] =
        {'P','O','S','T',' ','/','p','o','s','t','t','e','s','t','.','p','h','p',' ','H','T','T','P','/','1'};
    static const WCHAR test_header_end[] = {'\r','\n','\r','\n',0};
    static const WCHAR test_header_name[] = {'W','a','r','n','i','n','g',0};

    static const WCHAR test_flag_coalesce[] = {'t','e','s','t','2',',',' ','t','e','s','t','4',0};
    static const WCHAR test_flag_coalesce_comma[] =
        {'t','e','s','t','2',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',0};
    static const WCHAR test_flag_coalesce_semicolon[] =
        {'t','e','s','t','2',',',' ','t','e','s','t','4',',',' ','t','e','s','t','5',';',' ','t','e','s','t','6',0};

    static const WCHAR field[] = {'f','i','e','l','d',0};
    static const WCHAR value[] = {'v','a','l','u','e',' ',0};

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
            {'f','i','e','l','d',':',' ','v','a','l','u','e',' ',0}
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

    connection = WinHttpConnect (session, test_site, INTERNET_DEFAULT_HTTP_PORT, 0);
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
    ret = WinHttpAddRequestHeaders(request, test_headers[0], -1L, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret == TRUE, "WinHttpAddRequestHeader failed to add new header, got %d with error %u.\n", ret, GetLastError());

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
        "WinHttpQueryHeaders set incorrect error: expected ERROR_INSUFFICENT_BUFFER, go %u\n", GetLastError());
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
    ok(memcmp(buffer, test_header_begin, sizeof(test_header_begin)) == 0,
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
    ok(!memcmp(buffer, test_header_begin, sizeof(test_header_begin)), "invalid beginning of header string\n");
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
    ok(memcmp(buffer, test_indices[1], sizeof(test_indices[1])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    /* add if new flag */
    ret = WinHttpAddRequestHeaders(request, test_headers[3], -1L, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);
    ok(ret == FALSE, "WinHttpAddRequestHeaders incorrectly replaced existing header.\n");

    index = 0;
    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 1, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[1], sizeof(test_indices[1])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

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
    ok(memcmp(buffer, test_flag_coalesce, sizeof(test_flag_coalesce)) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

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
    ok(memcmp(buffer, test_flag_coalesce_comma, sizeof(test_flag_coalesce_comma)) == 0,
        "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

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
    ok(memcmp(buffer, test_flag_coalesce_semicolon, sizeof(test_flag_coalesce_semicolon)) == 0,
            "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

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
    ok(memcmp(buffer, test_indices[2], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == TRUE, "WinHttpQueryHeaders failed: %u\n", GetLastError());
    ok(index == 2, "WinHttpQueryHeaders failed to increment index.\n");
    ok(memcmp(buffer, test_indices[3], sizeof(test_indices[2])) == 0, "WinHttpQueryHeaders returned incorrect string.\n");

    len = sizeof(buffer);
    ret = WinHttpQueryHeaders(request, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
        test_header_name, buffer, &len, &index);
    ok(ret == FALSE, "WinHttpQueryHeaders succeeded unexpectedly, found third header.\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[8], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(!ret, "WinHttpAddRequestHeaders failed\n");

    ret = WinHttpAddRequestHeaders(request, test_headers[9], ~0u, WINHTTP_ADDREQ_FLAG_ADD);
    ok(ret, "WinHttpAddRequestHeaders failed\n");

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
    ok(!memcmp(buffer, value, sizeof(value)), "unexpected result\n");

    ret = WinHttpCloseHandle(request);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);
 done:
    ret = WinHttpCloseHandle(connection);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);

}

static void test_secure_connection(void)
{
    static const WCHAR google[] = {'w','w','w','.','g','o','o','g','l','e','.','c','o','m',0};

    HANDLE ses, con, req;
    DWORD size, status, policy, bitness;
    BOOL ret;
    CERT_CONTEXT *cert;

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    ret = WinHttpSetOption(ses, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    ok(ret, "failed to set redirect policy %u\n", GetLastError());

    con = WinHttpConnect(ses, google, 443, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    /* try without setting WINHTTP_FLAG_SECURE */
    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(!ret, "succeeded unexpectedly\n");

    size = 0;
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &size, NULL);
    ok(!ret, "succeeded unexpectedly\n");

    WinHttpCloseHandle(req);

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    size = sizeof(cert);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &cert, &size );
    ok(ret, "failed to retrieve certificate context %u\n", GetLastError());

    size = sizeof(bitness);
    ret = WinHttpQueryOption(req, WINHTTP_OPTION_SECURITY_KEY_BITNESS, &bitness, &size );
    ok(ret, "failed to retrieve key bitness %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    size = 0;
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &size, NULL);
    ok(!ret, "succeeded unexpectedly\n");

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

static void test_request_parameter_defaults(void)
{
    static const WCHAR empty[] = {0};
    static const WCHAR codeweavers[] = {'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};

    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;

    ses = WinHttpOpen(test_useragent, 0, NULL, NULL, 0);
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    con = WinHttpConnect(ses, codeweavers, 0, 0);
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    req = WinHttpOpenRequest(con, NULL, NULL, NULL, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    WinHttpCloseHandle(req);

    req = WinHttpOpenRequest(con, empty, empty, empty, NULL, NULL, 0);
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSendRequest(req, NULL, 0, NULL, 0, 0, 0);
    ok(ret, "failed to send request %u\n", GetLastError());

    ret = WinHttpReceiveResponse(req, NULL);
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
}

START_TEST (winhttp)
{
    test_OpenRequest();
    test_SendRequest();
    test_WinHttpTimeFromSystemTime();
    test_WinHttpTimeToSystemTime();
    test_WinHttpAddHeaders();
    test_secure_connection();
    test_request_parameter_defaults();
    test_QueryOption();
}
