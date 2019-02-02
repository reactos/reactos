/*
 * Unit tests for misc shdocvw functions
 *
 * Copyright 2008 Detlef Riekenberg
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wininet.h"
#include "winnls.h"

#include "wine/test.h"

/* ################ */

static HMODULE hshdocvw;
static HRESULT (WINAPI *pURLSubRegQueryA)(LPCSTR, LPCSTR, DWORD, LPVOID, DWORD, DWORD);
static DWORD (WINAPI *pParseURLFromOutsideSourceA)(LPCSTR, LPSTR, LPDWORD, LPDWORD);
static DWORD (WINAPI *pParseURLFromOutsideSourceW)(LPCWSTR, LPWSTR, LPDWORD, LPDWORD);

static const char appdata[] = "AppData";
static const char common_appdata[] = "Common AppData";
static const char default_page_url[] = "Default_Page_URL";
static const char does_not_exist[] = "does_not_exist";
static const char regpath_iemain[] = "Software\\Microsoft\\Internet Explorer\\Main";
static const char regpath_shellfolders[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static const char start_page[] = "Start Page";

/* ################ */

static const  struct {
    const char *url;
    const char *newurl;
    DWORD len;
} ParseURL_table[] = {
    {"http://www.winehq.org", "http://www.winehq.org/", 22},
    {"www.winehq.org", "http://www.winehq.org/", 22},
    {"winehq.org", "http://winehq.org/", 18},
    {"ftp.winehq.org", "ftp://ftp.winehq.org/", 21},
    {"http://winehq.org", "http://winehq.org/", 18},
    {"https://winehq.org", "https://winehq.org/", 19},
    {"https://www.winehq.org", "https://www.winehq.org/", 23},
    {"ftp://winehq.org", "ftp://winehq.org/", 17},
    {"ftp://ftp.winehq.org", "ftp://ftp.winehq.org/", 21},
    {"about:blank", "about:blank", 11},
    {"about:home", "about:home", 10},
    {"about:mozilla", "about:mozilla", 13},
    /* a space at the start is not allowed */
    {" http://www.winehq.org", "http://%20http://www.winehq.org", 31}

};

/* ################ */

static void init_functions(void)
{
    hshdocvw = LoadLibraryA("shdocvw.dll");
    pURLSubRegQueryA = (void *) GetProcAddress(hshdocvw, (LPSTR) 151);
    pParseURLFromOutsideSourceA = (void *) GetProcAddress(hshdocvw, (LPSTR) 169);
    pParseURLFromOutsideSourceW = (void *) GetProcAddress(hshdocvw, (LPSTR) 170);
}

/* ################ */

static void test_URLSubRegQueryA(void)
{
    CHAR buffer[INTERNET_MAX_URL_LENGTH];
    HRESULT hr;
    DWORD used;
    DWORD len;

    if (!pURLSubRegQueryA) {
        skip("URLSubRegQueryA not found\n");
        return;
    }

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    /* called by inetcpl.cpl */
    hr = pURLSubRegQueryA(regpath_iemain, default_page_url, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    ok(hr == E_FAIL || hr == S_OK, "got 0x%x (expected E_FAIL or S_OK)\n", hr);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    /* called by inetcpl.cpl */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    len = lstrlenA(buffer);
    /* respect privacy: do not dump the url */
    ok(hr == S_OK, "got 0x%x and %d (expected S_OK)\n", hr, len);

    /* test buffer length: just large enough */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len+1, -1);
    used = lstrlenA(buffer);
    /* respect privacy: do not dump the url */
    ok((hr == S_OK) && (used == len),
        "got 0x%x and %d (expected S_OK and %d)\n", hr, used, len);

    /* no space for terminating 0: result is truncated */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && (used == len - 1),
        "got 0x%x and %d (expected S_OK and %d)\n", hr, used, len - 1);

    /* no space for the complete result: truncate another char */
    if (len > 1) {
        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len-1, -1);
        used = lstrlenA(buffer);
        ok((hr == S_OK) && (used == (len - 2)),
            "got 0x%x and %d (expected S_OK and %d)\n", hr, used, len - 2);
    }

    /* only space for the terminating 0: function still succeeded */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, 1, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && !used,
        "got 0x%x and %d (expected S_OK and 0)\n", hr, used);

    /* size of buffer is 0, but the function still succeed.
       buffer[0] is cleared in IE 5.01 and IE 5.5 (Buffer Overflow) */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, 0, -1);
    used = lstrlenA(buffer);
    ok( (hr == S_OK) &&
        ((used == INTERNET_MAX_URL_LENGTH - 1) || broken(used == 0)) ,
        "got 0x%x and %d (expected S_OK and INTERNET_MAX_URL_LENGTH - 1)\n",
        hr, used);

    /* still succeed without a buffer for the result */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, NULL, 0, -1);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* still succeed, when a length is given without a buffer */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, NULL, INTERNET_MAX_URL_LENGTH, -1);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* this value does not exist */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, does_not_exist, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    /* random bytes are copied to the buffer */
    ok((hr == E_FAIL), "got 0x%x (expected E_FAIL)\n", hr);

    /* the third parameter is ignored. Is it really a type? (data is REG_SZ) */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_DWORD, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && (used == len),
        "got 0x%x and %d (expected S_OK and %d)\n", hr, used, len);

    /* the function works for HKCU and HKLM */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_shellfolders, appdata, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok(hr == S_OK, "got 0x%x and %d (expected S_OK)\n", hr, used);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_shellfolders, common_appdata, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok(hr == S_OK, "got 0x%x and %d (expected S_OK)\n", hr, used);

    /* todo: what does the last parameter mean? */
}

/* ################ */

static void test_ParseURLFromOutsideSourceA(void)
{
    CHAR buffer[INTERNET_MAX_URL_LENGTH];
    DWORD dummy;
    DWORD maxlen;
    DWORD len;
    DWORD res;
    int i;

    if (!pParseURLFromOutsideSourceA) {
        skip("ParseURLFromOutsideSourceA not found\n");
        return;
    }

    for(i = 0; i < ARRAY_SIZE(ParseURL_table); i++) {
        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = sizeof(buffer);
        dummy = 0;
        /* on success, string size including terminating 0 is returned for ansi version */
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, &len, &dummy);
        /* len does not include the terminating 0, when buffer is large enough */
        ok( res == (ParseURL_table[i].len+1) && len == ParseURL_table[i].len &&
            !lstrcmpA(buffer, ParseURL_table[i].newurl),
            "#%d: got %d and %d with '%s' (expected %d and %d with '%s')\n",
            i, res, len, buffer, ParseURL_table[i].len+1, ParseURL_table[i].len, ParseURL_table[i].newurl);


        /* use the size test only for the first examples */
        if (i > 4) continue;

        maxlen = len;

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = maxlen + 1;
        dummy = 0;
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, &len, &dummy);
        ok( res != 0 && len == ParseURL_table[i].len &&
            !lstrcmpA(buffer, ParseURL_table[i].newurl),
            "#%d (+1): got %d and %d with '%s' (expected '!=0' and %d with '%s')\n",
            i, res, len, buffer, ParseURL_table[i].len, ParseURL_table[i].newurl);

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = maxlen;
        dummy = 0;
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, &len, &dummy);
        /* len includes the terminating 0, when the buffer is too small */
        ok( res == 0 && len == ParseURL_table[i].len + 1,
            "#%d (==): got %d and %d (expected '0' and %d)\n",
            i, res, len, ParseURL_table[i].len + 1);

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = maxlen-1;
        dummy = 0;
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, &len, &dummy);
        /* len includes the terminating 0 on XP SP1 and before, when the buffer is too small */
        ok( res == 0 && (len == ParseURL_table[i].len || len == ParseURL_table[i].len + 1),
            "#%d (-1): got %d and %d (expected '0' and %d or %d)\n",
            i, res, len, ParseURL_table[i].len, ParseURL_table[i].len + 1);

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = maxlen+1;
        dummy = 0;
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, NULL, &len, &dummy);
        /* len does not include the terminating 0, when buffer is NULL */
        ok( res == 0 && len == ParseURL_table[i].len,
            "#%d (buffer): got %d and %d (expected '0' and %d)\n",
            i, res, len, ParseURL_table[i].len);

        if (0) {
            /* that test crash on native shdocvw */
            pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, NULL, &dummy);
        }

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        len = maxlen+1;
        dummy = 0;
        res = pParseURLFromOutsideSourceA(ParseURL_table[i].url, buffer, &len, NULL);
        ok( res != 0 && len == ParseURL_table[i].len &&
            !lstrcmpA(buffer, ParseURL_table[i].newurl),
            "#%d (unknown): got %d and %d with '%s' (expected '!=0' and %d with '%s')\n",
            i, res, len, buffer, ParseURL_table[i].len, ParseURL_table[i].newurl);
    }
}

/* ################ */

static void test_ParseURLFromOutsideSourceW(void)
{
    WCHAR urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR bufferW[INTERNET_MAX_URL_LENGTH];
    CHAR  bufferA[INTERNET_MAX_URL_LENGTH];
    DWORD maxlen;
    DWORD dummy;
    DWORD len;
    DWORD res;

    if (!pParseURLFromOutsideSourceW) {
        skip("ParseURLFromOutsideSourceW not found\n");
        return;
    }
    MultiByteToWideChar(CP_ACP, 0, ParseURL_table[0].url, -1, urlW, INTERNET_MAX_URL_LENGTH);

    memset(bufferA, '#', sizeof(bufferA)-1);
    bufferA[sizeof(bufferA) - 1] = '\0';
    MultiByteToWideChar(CP_ACP, 0, bufferA, -1, bufferW, INTERNET_MAX_URL_LENGTH);

    /* len is in characters */
    len = ARRAY_SIZE(bufferW);
    dummy = 0;
    /* on success, 1 is returned for unicode version */
    res = pParseURLFromOutsideSourceW(urlW, bufferW, &len, &dummy);
    WideCharToMultiByte(CP_ACP, 0, bufferW, -1, bufferA, sizeof(bufferA), NULL, NULL);
    ok( res == 1 && len == ParseURL_table[0].len &&
        !lstrcmpA(bufferA, ParseURL_table[0].newurl),
        "got %d and %d with '%s' (expected 1 and %d with '%s')\n",
        res, len, bufferA, ParseURL_table[0].len, ParseURL_table[0].newurl);


    maxlen = len;

    memset(bufferA, '#', sizeof(bufferA)-1);
    bufferA[sizeof(bufferA) - 1] = '\0';
    MultiByteToWideChar(CP_ACP, 0, bufferA, -1, bufferW, INTERNET_MAX_URL_LENGTH);
    len = maxlen+1;
    dummy = 0;
    res = pParseURLFromOutsideSourceW(urlW, bufferW, &len, &dummy);
    WideCharToMultiByte(CP_ACP, 0, bufferW, -1, bufferA, sizeof(bufferA), NULL, NULL);
    /* len does not include the terminating 0, when buffer is large enough */
    ok( res != 0 && len == ParseURL_table[0].len &&
        !lstrcmpA(bufferA, ParseURL_table[0].newurl),
        "+1: got %d and %d with '%s' (expected '!=0' and %d with '%s')\n",
        res, len, bufferA, ParseURL_table[0].len, ParseURL_table[0].newurl);

    len = maxlen;
    dummy = 0;
    res = pParseURLFromOutsideSourceW(urlW, bufferW, &len, &dummy);
    /* len includes the terminating 0, when the buffer is too small */
    ok( res == 0 && len == ParseURL_table[0].len + 1,
        "==: got %d and %d (expected '0' and %d)\n",
        res, len, ParseURL_table[0].len + 1);

    len = maxlen - 1;
    dummy = 0;
    res = pParseURLFromOutsideSourceW(urlW, bufferW, &len, &dummy);
    /* len includes the terminating 0 on XP SP1 and before, when the buffer is too small */
    ok( res == 0 && (len == ParseURL_table[0].len || len == ParseURL_table[0].len + 1),
        "-1: got %d and %d (expected '0' and %d or %d)\n",
        res, len, ParseURL_table[0].len, ParseURL_table[0].len + 1);

}

/* ################ */

START_TEST(shdocvw)
{
    init_functions();
    test_URLSubRegQueryA();
    test_ParseURLFromOutsideSourceA();
    test_ParseURLFromOutsideSourceW();
    FreeLibrary(hshdocvw);
}
