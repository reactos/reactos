/*
 * Unit tests for code page to/from unicode translations
 *
 * Copyright (c) 2002 Dmitry Timoshkov
 * Copyright (c) 2008 Colin Finck
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
#include <limits.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

static const char foobarA[] = "foobar";
static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};

static void test_destination_buffer(void)
{
    LPSTR   buffer;
    INT     maxsize;
    INT     needed;
    INT     len;

    SetLastError(0xdeadbeef);
    needed = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, NULL, 0, NULL, NULL);
    ok( (needed > 0), "returned %d with %u (expected '> 0')\n",
        needed, GetLastError());

    maxsize = needed*2;
    buffer = HeapAlloc(GetProcessHeap(), 0, maxsize);
    if (buffer == NULL) return;

    maxsize--;
    memset(buffer, 'x', maxsize);
    buffer[maxsize] = '\0';
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buffer, needed+1, NULL, NULL);
    ok( (len > 0), "returned %d with %u and '%s' (expected '> 0')\n",
        len, GetLastError(), buffer);

    memset(buffer, 'x', maxsize);
    buffer[maxsize] = '\0';
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buffer, needed, NULL, NULL);
    ok( (len > 0), "returned %d with %u and '%s' (expected '> 0')\n",
        len, GetLastError(), buffer);

    memset(buffer, 'x', maxsize);
    buffer[maxsize] = '\0';
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buffer, needed-1, NULL, NULL);
    ok( !len && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "returned %d with %u and '%s' (expected '0' with "
        "ERROR_INSUFFICIENT_BUFFER)\n", len, GetLastError(), buffer);

    memset(buffer, 'x', maxsize);
    buffer[maxsize] = '\0';
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buffer, 1, NULL, NULL);
    ok( !len && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "returned %d with %u and '%s' (expected '0' with "
        "ERROR_INSUFFICIENT_BUFFER)\n", len, GetLastError(), buffer);

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buffer, 0, NULL, NULL);
    ok( (len > 0), "returned %d with %u (expected '> 0')\n",
        len, GetLastError());

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, NULL, needed, NULL, NULL);
    ok( !len && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %d with %u (expected '0' with "
        "ERROR_INVALID_PARAMETER)\n", len, GetLastError());

    HeapFree(GetProcessHeap(), 0, buffer);
}


static void test_null_source(void)
{
    int len;
    DWORD GLE;

    SetLastError(0);
    len = WideCharToMultiByte(CP_ACP, 0, NULL, 0, NULL, 0, NULL, NULL);
    GLE = GetLastError();
    ok(!len && GLE == ERROR_INVALID_PARAMETER,
        "WideCharToMultiByte returned %d with GLE=%u (expected 0 with ERROR_INVALID_PARAMETER)\n",
        len, GLE);

    SetLastError(0);
    len = WideCharToMultiByte(CP_ACP, 0, NULL, -1, NULL, 0, NULL, NULL);
    GLE = GetLastError();
    ok(!len && GLE == ERROR_INVALID_PARAMETER,
        "WideCharToMultiByte returned %d with GLE=%u (expected 0 with ERROR_INVALID_PARAMETER)\n",
        len, GLE);
}

static void test_negative_source_length(void)
{
    int len;
    char buf[10];
    WCHAR bufW[10];

    /* Test, whether any negative source length works as strlen() + 1 */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -2002, buf, 10, NULL, NULL);
    ok(len == 7 && GetLastError() == 0xdeadbeef,
       "WideCharToMultiByte(-2002): len=%d error=%u\n", len, GetLastError());
    ok(!lstrcmpA(buf, "foobar"),
       "WideCharToMultiByte(-2002): expected \"foobar\" got \"%s\"\n", buf);

    SetLastError( 0xdeadbeef );
    memset(bufW,'x',sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, "foobar", -2002, bufW, 10);
    ok(len == 7 && !lstrcmpW(bufW, foobarW) && GetLastError() == 0xdeadbeef,
       "MultiByteToWideChar(-2002): len=%d error=%u\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    memset(bufW, 'x', sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, "foobar", -1, bufW, 6);
    ok(len == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "MultiByteToWideChar(-1): len=%d error=%u\n", len, GetLastError());
}

#define LONGBUFLEN 100000
static void test_negative_dest_length(void)
{
    int len, i;
    static WCHAR bufW[LONGBUFLEN];
    static char bufA[LONGBUFLEN];
    static WCHAR originalW[LONGBUFLEN];
    static char originalA[LONGBUFLEN];
    DWORD theError;

    /* Test return on -1 dest length */
    SetLastError( 0xdeadbeef );
    memset(bufA,'x',sizeof(bufA));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, bufA, -1, NULL, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
       "WideCharToMultiByte(destlen -1): len=%d error=%x\n", len, GetLastError());

    SetLastError( 0xdeadbeef );
    memset(bufW,'x',sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, foobarA, -1, bufW, -1);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
       "MultiByteToWideChar(destlen -1): len=%d error=%x\n", len, GetLastError());

    /* Test return on -1000 dest length */
    SetLastError( 0xdeadbeef );
    memset(bufA,'x',sizeof(bufA));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, bufA, -1000, NULL, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
       "WideCharToMultiByte(destlen -1000): len=%d error=%x\n", len, GetLastError());

    SetLastError( 0xdeadbeef );
    memset(bufW,'x',sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, foobarA, -1000, bufW, -1);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
       "MultiByteToWideChar(destlen -1000): len=%d error=%x\n", len, GetLastError());

    /* Test return on INT_MAX dest length */
    SetLastError( 0xdeadbeef );
    memset(bufA,'x',sizeof(bufA));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, bufA, INT_MAX, NULL, NULL);
    ok(len == 7 && !lstrcmpA(bufA, "foobar") && GetLastError() == 0xdeadbeef,
       "WideCharToMultiByte(destlen INT_MAX): len=%d error=%x\n", len, GetLastError());

    /* Test return on INT_MAX dest length and very long input */
    SetLastError( 0xdeadbeef );
    memset(bufA,'x',sizeof(bufA));
    for (i=0; i < LONGBUFLEN - 1; i++) {
        originalW[i] = 'Q';
        originalA[i] = 'Q';
    }
    originalW[LONGBUFLEN-1] = 0;
    originalA[LONGBUFLEN-1] = 0;
    len = WideCharToMultiByte(CP_ACP, 0, originalW, -1, bufA, INT_MAX, NULL, NULL);
    theError = GetLastError();
    ok(len == LONGBUFLEN && !lstrcmpA(bufA, originalA) && theError == 0xdeadbeef,
       "WideCharToMultiByte(srclen %d, destlen INT_MAX): len %d error=%x\n", LONGBUFLEN, len, theError);

}

static void test_other_invalid_parameters(void)
{
    char c_string[] = "Hello World";
    size_t c_string_len = sizeof(c_string);
    WCHAR w_string[] = {'H','e','l','l','o',' ','W','o','r','l','d',0};
    size_t w_string_len = sizeof(w_string) / sizeof(WCHAR);
    BOOL used;
    INT len;

    /* srclen=0 => ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, w_string, 0, c_string, c_string_len, NULL, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = MultiByteToWideChar(CP_ACP, 0, c_string, 0, w_string, w_string_len);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());


    /* dst=NULL but dstlen not 0 => ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_ACP, 0, w_string, w_string_len, NULL, c_string_len, NULL, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = MultiByteToWideChar(CP_ACP, 0, c_string, c_string_len, NULL, w_string_len);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());


    /* CP_UTF7, CP_UTF8, or CP_SYMBOL and defchar not NULL => ERROR_INVALID_PARAMETER */
    /* CP_SYMBOL's behavior here is undocumented */
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_UTF7, 0, w_string, w_string_len, c_string, c_string_len, c_string, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_UTF8, 0, w_string, w_string_len, c_string, c_string_len, c_string, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_SYMBOL, 0, w_string, w_string_len, c_string, c_string_len, c_string, NULL);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());


    /* CP_UTF7, CP_UTF8, or CP_SYMBOL and used not NULL => ERROR_INVALID_PARAMETER */
    /* CP_SYMBOL's behavior here is undocumented */
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_UTF7, 0, w_string, w_string_len, c_string, c_string_len, NULL, &used);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_UTF8, 0, w_string, w_string_len, c_string, c_string_len, NULL, &used);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());

    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_SYMBOL, 0, w_string, w_string_len, c_string, c_string_len, NULL, &used);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());


    /* CP_UTF7, flags not 0 and used not NULL => ERROR_INVALID_PARAMETER */
    /* (tests precedence of ERROR_INVALID_PARAMETER over ERROR_INVALID_FLAGS) */
    /* The same test with CP_SYMBOL instead of CP_UTF7 gives ERROR_INVALID_FLAGS
       instead except on Windows NT4 */
    SetLastError(0xdeadbeef);
    len = WideCharToMultiByte(CP_UTF7, 1, w_string, w_string_len, c_string, c_string_len, NULL, &used);
    ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "len=%d error=%x\n", len, GetLastError());
}

static void test_overlapped_buffers(void)
{
    static const WCHAR strW[] = {'j','u','s','t',' ','a',' ','t','e','s','t',0};
    static const char strA[] = "just a test";
    char buf[256];
    int ret;

    SetLastError(0xdeadbeef);
    memcpy(buf + 1, strW, sizeof(strW));
    ret = WideCharToMultiByte(CP_ACP, 0, (WCHAR *)(buf + 1), -1, buf, sizeof(buf), NULL, NULL);
    ok(ret == sizeof(strA), "unexpected ret %d\n", ret);
    ok(!memcmp(buf, strA, sizeof(strA)), "conversion failed: %s\n", buf);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
}

static void test_string_conversion(LPBOOL bUsedDefaultChar)
{
    char mbc;
    char mbs[15];
    int ret;
    WCHAR wc1 = 228;                           /* Western Windows-1252 character */
    WCHAR wc2 = 1088;                          /* Russian Windows-1251 character not displayable for Windows-1252 */
    static const WCHAR wcs[] = {'T', 'h', 1088, 'i', 0}; /* String with ASCII characters and a Russian character */
    static const WCHAR dbwcs[] = {28953, 25152, 0}; /* String with Chinese (codepage 950) characters */
    static const WCHAR dbwcs2[] = {0x7bb8, 0x3d, 0xc813, 0xac00, 0xb77d, 0};
    static const char default_char[] = {0xa3, 0xbf, 0};

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, &wc1, 1, &mbc, 1, NULL, bUsedDefaultChar);
    ok(ret == 1, "ret is %d\n", ret);
    ok(mbc == '\xe4', "mbc is %d\n", mbc);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, &wc2, 1, &mbc, 1, NULL, bUsedDefaultChar);
    ok(ret == 1, "ret is %d\n", ret);
    ok(mbc == 63, "mbc is %d\n", mbc);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    if (IsValidCodePage(1251))
    {
        SetLastError(0xdeadbeef);
        ret = WideCharToMultiByte(1251, 0, &wc2, 1, &mbc, 1, NULL, bUsedDefaultChar);
        ok(ret == 1, "ret is %d\n", ret);
        ok(mbc == '\xf0', "mbc is %d\n", mbc);
        if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
        ok(GetLastError() == 0xdeadbeef ||
           broken(GetLastError() == 0), /* win95 */
           "GetLastError() is %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = WideCharToMultiByte(1251, 0, &wc1, 1, &mbc, 1, NULL, bUsedDefaultChar);
        ok(ret == 1, "ret is %d\n", ret);
        ok(mbc == 97, "mbc is %d\n", mbc);
        if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
        ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
    }
    else
        skip("Codepage 1251 not available\n");

    /* This call triggers the last Win32 error */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, wcs, -1, &mbc, 1, NULL, bUsedDefaultChar);
    ok(ret == 0, "ret is %d\n", ret);
    ok(mbc == 84, "mbc is %d\n", mbc);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() is %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, wcs, -1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 5, "ret is %d\n", ret);
    ok(!strcmp(mbs, "Th?i"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
    mbs[0] = 0;

    /* WideCharToMultiByte mustn't add any null character automatically.
       So in this case, we should get the same string again, even if we only copied the first three bytes. */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, wcs, 3, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 3, "ret is %d\n", ret);
    ok(!strcmp(mbs, "Th?i"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
    ZeroMemory(mbs, 5);

    /* Now this shouldn't be the case like above as we zeroed the complete string buffer. */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, wcs, 3, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 3, "ret is %d\n", ret);
    ok(!strcmp(mbs, "Th?"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    /* Double-byte tests */
    ret = WideCharToMultiByte(1252, 0, dbwcs, 3, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 3, "ret is %d\n", ret);
    ok(!strcmp(mbs, "??"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);

    ret = WideCharToMultiByte(936, WC_COMPOSITECHECK, dbwcs2, -1, mbs, sizeof(mbs), (const char *)default_char, bUsedDefaultChar);
    ok(ret == 10, "ret is %d\n", ret);
    ok(!strcmp(mbs, "\xf3\xe7\x3d\xa3\xbf\xa3\xbf\xa3\xbf"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);

    /* Length-only tests */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, &wc2, 1, NULL, 0, NULL, bUsedDefaultChar);
    ok(ret == 1, "ret is %d\n", ret);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, wcs, -1, NULL, 0, NULL, bUsedDefaultChar);
    ok(ret == 5, "ret is %d\n", ret);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    if (!IsValidCodePage(950))
    {
        skip("Codepage 950 not available\n");
        return;
    }

    /* Double-byte tests */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(950, 0, dbwcs, -1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 5, "ret is %d\n", ret);
    ok(!strcmp(mbs, "\xb5H\xa9\xd2"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(950, 0, dbwcs, 1, &mbc, 1, NULL, bUsedDefaultChar);
    ok(ret == 0, "ret is %d\n", ret);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() is %u\n", GetLastError());
    ZeroMemory(mbs, 5);

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(950, 0, dbwcs, 1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    ok(ret == 2, "ret is %d\n", ret);
    ok(!strcmp(mbs, "\xb5H"), "mbs is %s\n", mbs);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    /* Length-only tests */
    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(950, 0, dbwcs, 1, NULL, 0, NULL, bUsedDefaultChar);
    ok(ret == 2, "ret is %d\n", ret);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(950, 0, dbwcs, -1, NULL, 0, NULL, bUsedDefaultChar);
    ok(ret == 5, "ret is %d\n", ret);
    if(bUsedDefaultChar) ok(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d\n", *bUsedDefaultChar);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
}

static void test_utf7_encoding(void)
{
    WCHAR input[16];
    char output[16], expected[16];
    int i, len, expected_len;

    static const BOOL directly_encodable_table[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0F */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1F */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5F */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6F */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0  /* 0x70 - 0x7F */
    };
    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const struct
    {
        /* inputs */
        WCHAR src[16];
        int srclen;
        char *dst;
        int dstlen;
        /* expected outputs */
        char expected_dst[16];
        int chars_written;
        int len;
    }
    tests[] =
    {
        /* tests string conversion with srclen=-1 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, sizeof(output) - 1,
            "+T2BZfVQX-", 11, 11
        },
        /* tests string conversion with srclen=-2 */
        {
            {0x4F60,0x597D,0x5417,0}, -2, output, sizeof(output) - 1,
            "+T2BZfVQX-", 11, 11
        },
        /* tests string conversion with dstlen=strlen(expected_dst) */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, 10,
            "+T2BZfVQX-", 10, 0
        },
        /* tests string conversion with dstlen=strlen(expected_dst)+1 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, 11,
            "+T2BZfVQX-", 11, 11
        },
        /* tests string conversion with dstlen=strlen(expected_dst)+2 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, 12,
            "+T2BZfVQX-", 11, 11
        },
        /* tests dry run with dst=NULL and dstlen=0 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, NULL, 0,
            {0}, 0, 11
        },
        /* tests dry run with dst!=NULL and dstlen=0 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, 0,
            {0}, 0, 11
        },
        /* tests srclen < strlenW(src) with directly encodable chars */
        {
            {'h','e','l','l','o',0}, 2, output, sizeof(output) - 1,
            "he", 2, 2
        },
        /* tests srclen < strlenW(src) with non-directly encodable chars */
        {
            {0x4F60,0x597D,0x5417,0}, 2, output, sizeof(output) - 1,
            "+T2BZfQ-", 8, 8
        },
        /* tests a single null char */
        {
            {0}, -1, output, sizeof(output) - 1,
            "", 1, 1
        },
        /* tests a buffer that runs out while not encoding a UTF-7 sequence */
        {
            {'h','e','l','l','o',0}, -1, output, 2,
            "he", 2, 0
        },
        /* tests a buffer that runs out after writing 1 base64 character */
        {
            {0x4F60,0x0001,0}, -1, output, 2,
            "+T", 2, 0
        },
        /* tests a buffer that runs out after writing 2 base64 characters */
        {
            {0x4F60,0x0001,0}, -1, output, 3,
            "+T2", 3, 0
        },
        /* tests a buffer that runs out after writing 3 base64 characters */
        {
            {0x4F60,0x0001,0}, -1, output, 4,
            "+T2A", 4, 0
        },
        /* tests a buffer that runs out just after writing the + sign */
        {
            {0x4F60,0}, -1, output, 1,
            "+", 1, 0
        },
        /* tests a buffer that runs out just before writing the - sign
         * the number of bits to encode here is evenly divisible by 6 */
        {
            {0x4F60,0x597D,0x5417,0}, -1, output, 9,
            "+T2BZfVQX", 9, 0
        },
        /* tests a buffer that runs out just before writing the - sign
         * the number of bits to encode here is NOT evenly divisible by 6 */
        {
            {0x4F60,0}, -1, output, 4,
            "+T2", 3, 0
        },
        /* tests a buffer that runs out in the middle of escaping a + sign */
        {
            {'+',0}, -1, output, 1,
            "+", 1, 0
        }
    };

    /* test which characters are encoded if surrounded by non-encoded characters */
    for (i = 0; i <= 0xFFFF; i++)
    {
        input[0] = ' ';
        input[1] = i;
        input[2] = ' ';
        input[3] = 0;

        memset(output, '#', sizeof(output) - 1);
        output[sizeof(output) - 1] = 0;

        len = WideCharToMultiByte(CP_UTF7, 0, input, 4, output, sizeof(output) - 1, NULL, NULL);

        if (i == '+')
        {
            /* '+' is a special case and is encoded as "+-" */
            expected_len = 5;
            strcpy(expected, " +- ");
        }
        else if (i <= 0x7F && directly_encodable_table[i])
        {
            /* encodes directly */
            expected_len = 4;
            sprintf(expected, " %c ", i);
        }
        else
        {
            /* base64-encodes */
            expected_len = 8;
            sprintf(expected, " +%c%c%c- ",
                    base64_encoding_table[(i & 0xFC00) >> 10],
                    base64_encoding_table[(i & 0x03F0) >> 4],
                    base64_encoding_table[(i & 0x000F) << 2]);
        }

        ok(len == expected_len, "i=0x%04x: expected len=%i, got len=%i\n", i, expected_len, len);
        ok(memcmp(output, expected, expected_len) == 0,
           "i=0x%04x: expected output='%s', got output='%s'\n", i, expected, output);
        ok(output[expected_len] == '#', "i=0x%04x: expected output[%i]='#', got output[%i]=%i\n",
           i, expected_len, expected_len, output[expected_len]);
    }

    /* test which one-byte characters are absorbed into surrounding base64 blocks
     * (Windows always ends the base64 block when it encounters a directly encodable character) */
    for (i = 0; i <= 0xFFFF; i++)
    {
        input[0] = 0x2672;
        input[1] = i;
        input[2] = 0x2672;
        input[3] = 0;

        memset(output, '#', sizeof(output) - 1);
        output[sizeof(output) - 1] = 0;

        len = WideCharToMultiByte(CP_UTF7, 0, input, 4, output, sizeof(output) - 1, NULL, NULL);

        if (i == '+')
        {
            /* '+' is a special case and is encoded as "+-" */
            expected_len = 13;
            strcpy(expected, "+JnI-+-+JnI-");
        }
        else if (i <= 0x7F && directly_encodable_table[i])
        {
            /* encodes directly */
            expected_len = 12;
            sprintf(expected, "+JnI-%c+JnI-", i);
        }
        else
        {
            /* base64-encodes */
            expected_len = 11;
            sprintf(expected, "+Jn%c%c%c%cZy-",
                    base64_encoding_table[8 | ((i & 0xC000) >> 14)],
                    base64_encoding_table[(i & 0x3F00) >> 8],
                    base64_encoding_table[(i & 0x00FC) >> 2],
                    base64_encoding_table[((i & 0x0003) << 4) | 2]);
        }

        ok(len == expected_len, "i=0x%04x: expected len=%i, got len=%i\n", i, expected_len, len);
        ok(memcmp(output, expected, expected_len) == 0,
           "i=0x%04x: expected output='%s', got output='%s'\n", i, expected, output);
        ok(output[expected_len] == '#', "i=0x%04x: expected output[%i]='#', got output[%i]=%i\n",
           i, expected_len, expected_len, output[expected_len]);
    }

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        memset(output, '#', sizeof(output) - 1);
        output[sizeof(output) - 1] = 0;
        SetLastError(0xdeadbeef);

        len = WideCharToMultiByte(CP_UTF7, 0, tests[i].src, tests[i].srclen,
                                  tests[i].dst, tests[i].dstlen, NULL, NULL);

        if (!tests[i].len)
        {
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
               "tests[%i]: expected error=0x%x, got error=0x%x\n",
               i, ERROR_INSUFFICIENT_BUFFER, GetLastError());
        }
        ok(len == tests[i].len, "tests[%i]: expected len=%i, got len=%i\n", i, tests[i].len, len);

        if (tests[i].dst)
        {
            ok(memcmp(tests[i].dst, tests[i].expected_dst, tests[i].chars_written) == 0,
               "tests[%i]: expected dst='%s', got dst='%s'\n",
               i, tests[i].expected_dst, tests[i].dst);
            ok(tests[i].dst[tests[i].chars_written] == '#',
               "tests[%i]: expected dst[%i]='#', got dst[%i]=%i\n",
               i, tests[i].chars_written, tests[i].chars_written, tests[i].dst[tests[i].chars_written]);
        }
    }
}

static void test_utf7_decoding(void)
{
    char input[32];
    WCHAR output[32], expected[32];
    int i, len, expected_len;

    static const signed char base64_decoding_table[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
    };

    struct
    {
        /* inputs */
        char src[32];
        int srclen;
        WCHAR *dst;
        int dstlen;
        /* expected outputs */
        WCHAR expected_dst[32];
        int chars_written;
        int len;
    }
    tests[] =
    {
        /* tests string conversion with srclen=-1 */
        {
            "+T2BZfQ-", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60,0x597D,0}, 3, 3
        },
        /* tests string conversion with srclen=-2 */
        {
            "+T2BZfQ-", -2, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60,0x597D,0}, 3, 3
        },
        /* tests string conversion with dstlen=strlen(expected_dst) */
        {
            "+T2BZfQ-", -1, output, 2,
            {0x4F60,0x597D}, 2, 0
        },
        /* tests string conversion with dstlen=strlen(expected_dst)+1 */
        {
            "+T2BZfQ-", -1, output, 3,
            {0x4F60,0x597D,0}, 3, 3
        },
        /* tests string conversion with dstlen=strlen(expected_dst)+2 */
        {
            "+T2BZfQ-", -1, output, 4,
            {0x4F60,0x597D,0}, 3, 3
        },
        /* tests dry run with dst=NULL and dstlen=0 */
        {
            "+T2BZfQ-", -1, NULL, 0,
            {0}, 0, 3
        },
        /* tests dry run with dst!=NULL and dstlen=0 */
        {
            "+T2BZfQ-", -1, output, 0,
            {0}, 0, 3
        },
        /* tests ill-formed UTF-7: 6 bits, not enough for a byte pair */
        {
            "+T-+T-+T-hello", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'h','e','l','l','o',0}, 6, 6
        },
        /* tests ill-formed UTF-7: 12 bits, not enough for a byte pair */
        {
            "+T2-+T2-+T2-hello", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'h','e','l','l','o',0}, 6, 6
        },
        /* tests ill-formed UTF-7: 18 bits, not a multiple of 16 and the last bit is a 1 */
        {
            "+T2B-+T2B-+T2B-hello", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60,0x4F60,0x4F60,'h','e','l','l','o',0}, 9, 9
        },
        /* tests ill-formed UTF-7: 24 bits, a multiple of 8 but not a multiple of 16 */
        {
            "+T2BZ-+T2BZ-+T2BZ-hello", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60,0x4F60,0x4F60,'h','e','l','l','o',0}, 9, 9
        },
        /* tests UTF-7 followed by characters that should be encoded but aren't */
        {
            "+T2BZ-\x82\xFE", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60,0x0082,0x00FE,0}, 4, 4
        },
        /* tests srclen > strlen(src) */
        {
            "a\0b", 4, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'a',0,'b',0}, 4, 4
        },
        /* tests srclen < strlen(src) outside of a UTF-7 sequence */
        {
            "hello", 2, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'h','e'}, 2, 2
        },
        /* tests srclen < strlen(src) inside of a UTF-7 sequence */
        {
            "+T2BZfQ-", 4, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60}, 1, 1
        },
        /* tests srclen < strlen(src) right at the beginning of a UTF-7 sequence */
        {
            "hi+T2A-", 3, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'h','i'}, 2, 2
        },
        /* tests srclen < strlen(src) right at the end of a UTF-7 sequence */
        {
            "+T2A-hi", 5, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0x4F60}, 1, 1
        },
        /* tests srclen < strlen(src) at the beginning of an escaped + sign */
        {
            "hi+-", 3, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'h','i'}, 2, 2
        },
        /* tests srclen < strlen(src) at the end of an escaped + sign */
        {
            "+-hi", 2, output, sizeof(output) / sizeof(WCHAR) - 1,
            {'+'}, 1, 1
        },
        /* tests len=0 but no error */
        {
            "+", 1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0}, 0, 0
        },
        /* tests a single null char */
        {
            "", -1, output, sizeof(output) / sizeof(WCHAR) - 1,
            {0}, 1, 1
        },
        /* tests a buffer that runs out while not decoding a UTF-7 sequence */
        {
            "hello", -1, output, 2,
            {'h','e'}, 2, 0
        },
        /* tests a buffer that runs out in the middle of decoding a UTF-7 sequence */
        {
            "+T2BZfQ-", -1, output, 1,
            {0x4F60}, 1, 0
        }
    };

    /* test which one-byte characters remove stray + signs */
    for (i = 0; i < 256; i++)
    {
        sprintf(input, "+%c+AAA", i);

        memset(output, 0x23, sizeof(output) - sizeof(WCHAR));
        output[sizeof(output) / sizeof(WCHAR) - 1] = 0;

        len = MultiByteToWideChar(CP_UTF7, 0, input, 7, output, sizeof(output) / sizeof(WCHAR) - 1);

        if (i == '-')
        {
            /* removes the - sign */
            expected_len = 3;
            expected[0] = 0x002B;
            expected[1] = 0;
            expected[2] = 0;
        }
        else if (i <= 0x7F && base64_decoding_table[i] != -1)
        {
            /* absorbs the character into the base64 sequence */
            expected_len = 2;
            expected[0] = (base64_decoding_table[i] << 10) | 0x03E0;
            expected[1] = 0;
        }
        else
        {
            /* removes the + sign */
            expected_len = 3;
            expected[0] = i;
            expected[1] = 0;
            expected[2] = 0;
        }
        expected[expected_len] = 0x2323;

        ok(len == expected_len, "i=0x%02x: expected len=%i, got len=%i\n", i, expected_len, len);
        ok(memcmp(output, expected, (expected_len + 1) * sizeof(WCHAR)) == 0,
           "i=0x%02x: expected output=%s, got output=%s\n",
           i, wine_dbgstr_wn(expected, expected_len + 1), wine_dbgstr_wn(output, expected_len + 1));
    }

    /* test which one-byte characters terminate a sequence
     * also test whether the unfinished byte pair is discarded or not */
    for (i = 0; i < 256; i++)
    {
        sprintf(input, "+B%c+AAA", i);

        memset(output, 0x23, sizeof(output) - sizeof(WCHAR));
        output[sizeof(output) / sizeof(WCHAR) - 1] = 0;

        len = MultiByteToWideChar(CP_UTF7, 0, input, 8, output, sizeof(output) / sizeof(WCHAR) - 1);

        if (i == '-')
        {
            /* explicitly terminates */
            expected_len = 2;
            expected[0] = 0;
            expected[1] = 0;
        }
        else if (i <= 0x7F)
        {
            if (base64_decoding_table[i] != -1)
            {
                /* absorbs the character into the base64 sequence */
                expected_len = 3;
                expected[0] = 0x0400 | (base64_decoding_table[i] << 4) | 0x000F;
                expected[1] = 0x8000;
                expected[2] = 0;
            }
            else
            {
                /* implicitly terminates and discards the unfinished byte pair */
                expected_len = 3;
                expected[0] = i;
                expected[1] = 0;
                expected[2] = 0;
            }
        }
        else
        {
            /* implicitly terminates but does not the discard unfinished byte pair */
            expected_len = 3;
            expected[0] = i;
            expected[1] = 0x0400;
            expected[2] = 0;
        }
        expected[expected_len] = 0x2323;

        ok(len == expected_len, "i=0x%02x: expected len=%i, got len=%i\n", i, expected_len, len);
        ok(memcmp(output, expected, (expected_len + 1) * sizeof(WCHAR)) == 0,
           "i=0x%02x: expected output=%s, got output=%s\n",
           i, wine_dbgstr_wn(expected, expected_len + 1), wine_dbgstr_wn(output, expected_len + 1));
    }

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        memset(output, 0x23, sizeof(output) - sizeof(WCHAR));
        output[sizeof(output) / sizeof(WCHAR) - 1] = 0;
        SetLastError(0xdeadbeef);

        len = MultiByteToWideChar(CP_UTF7, 0, tests[i].src, tests[i].srclen,
                                  tests[i].dst, tests[i].dstlen);

        tests[i].expected_dst[tests[i].chars_written] = 0x2323;

        if (!tests[i].len && tests[i].chars_written)
        {
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
               "tests[%i]: expected error=0x%x, got error=0x%x\n",
               i, ERROR_INSUFFICIENT_BUFFER, GetLastError());
        }
        ok(len == tests[i].len, "tests[%i]: expected len=%i, got len=%i\n", i, tests[i].len, len);

        if (tests[i].dst)
        {
            ok(memcmp(tests[i].dst, tests[i].expected_dst, (tests[i].chars_written + 1) * sizeof(WCHAR)) == 0,
               "tests[%i]: expected dst=%s, got dst=%s\n",
               i, wine_dbgstr_wn(tests[i].expected_dst, tests[i].chars_written + 1),
               wine_dbgstr_wn(tests[i].dst, tests[i].chars_written + 1));
        }
    }
}

static void test_undefined_byte_char(void)
{
    static const struct tag_testset {
        INT codepage;
        LPCSTR str;
        BOOL is_error;
    } testset[] = {
        {  874, "\xdd", TRUE },
        {  932, "\xfe", TRUE },
        {  932, "\x80", FALSE },
        {  936, "\xff", TRUE },
        {  949, "\xff", TRUE },
        {  950, "\xff", TRUE },
        { 1252, "\x90", FALSE },
        { 1253, "\xaa", TRUE },
        { 1255, "\xff", TRUE },
        { 1257, "\xa5", TRUE },
    };
    INT i, ret;

    for (i = 0; i < (sizeof(testset) / sizeof(testset[0])); i++) {
        if (! IsValidCodePage(testset[i].codepage))
        {
            skip("Codepage %d not available\n", testset[i].codepage);
            continue;
        }

        SetLastError(0xdeadbeef);
        ret = MultiByteToWideChar(testset[i].codepage, MB_ERR_INVALID_CHARS,
                                  testset[i].str, -1, NULL, 0);
        if (testset[i].is_error) {
            ok(ret == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION,
               "ret is %d, GetLastError is %u (cp %d)\n",
               ret, GetLastError(), testset[i].codepage);
        }
        else {
            ok(ret == strlen(testset[i].str)+1 && GetLastError() == 0xdeadbeef,
               "ret is %d, GetLastError is %u (cp %d)\n",
               ret, GetLastError(), testset[i].codepage);
        }

        SetLastError(0xdeadbeef);
        ret = MultiByteToWideChar(testset[i].codepage, 0,
                                  testset[i].str, -1, NULL, 0);
        ok(ret == strlen(testset[i].str)+1 && GetLastError() == 0xdeadbeef,
           "ret is %d, GetLastError is %u (cp %d)\n",
           ret, GetLastError(), testset[i].codepage);
    }
}

static void test_threadcp(void)
{
    static const LCID ENGLISH  = MAKELCID(MAKELANGID(LANG_ENGLISH,  SUBLANG_ENGLISH_US),         SORT_DEFAULT);
    static const LCID HINDI    = MAKELCID(MAKELANGID(LANG_HINDI,    SUBLANG_HINDI_INDIA),        SORT_DEFAULT);
    static const LCID GEORGIAN = MAKELCID(MAKELANGID(LANG_GEORGIAN, SUBLANG_GEORGIAN_GEORGIA),   SORT_DEFAULT);
    static const LCID RUSSIAN  = MAKELCID(MAKELANGID(LANG_RUSSIAN,  SUBLANG_RUSSIAN_RUSSIA),     SORT_DEFAULT);
    static const LCID JAPANESE = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN),     SORT_DEFAULT);
    static const LCID CHINESE  = MAKELCID(MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT);

    BOOL islead, islead_acp;
    CPINFOEXA cpi;
    UINT cp, acp;
    int  i, num;
    LCID last;
    BOOL ret;

    struct test {
        LCID lcid;
        UINT threadcp;
    } lcids[] = {
        { HINDI,    0    },
        { GEORGIAN, 0    },
        { ENGLISH,  1252 },
        { RUSSIAN,  1251 },
        { JAPANESE, 932  },
        { CHINESE,  936  }
    };

    struct test_islead_nocp {
        LCID lcid;
        BYTE testchar;
    } isleads_nocp[] = {
        { HINDI,    0x00 },
        { HINDI,    0x81 },
        { HINDI,    0xa0 },
        { HINDI,    0xe0 },

        { GEORGIAN, 0x00 },
        { GEORGIAN, 0x81 },
        { GEORGIAN, 0xa0 },
        { GEORGIAN, 0xe0 },
    };

    struct test_islead {
        LCID lcid;
        BYTE testchar;
        BOOL islead;
    } isleads[] = {
        { ENGLISH,  0x00, FALSE },
        { ENGLISH,  0x81, FALSE },
        { ENGLISH,  0xa0, FALSE },
        { ENGLISH,  0xe0, FALSE },

        { RUSSIAN,  0x00, FALSE },
        { RUSSIAN,  0x81, FALSE },
        { RUSSIAN,  0xa0, FALSE },
        { RUSSIAN,  0xe0, FALSE },

        { JAPANESE, 0x00, FALSE },
        { JAPANESE, 0x81,  TRUE },
        { JAPANESE, 0xa0, FALSE },
        { JAPANESE, 0xe0,  TRUE },

        { CHINESE,  0x00, FALSE },
        { CHINESE,  0x81,  TRUE },
        { CHINESE,  0xa0,  TRUE },
        { CHINESE,  0xe0,  TRUE },
    };

    last = GetThreadLocale();
    acp  = GetACP();

    for (i = 0; i < sizeof(lcids)/sizeof(lcids[0]); i++)
    {
        SetThreadLocale(lcids[i].lcid);

        cp = 0xdeadbeef;
        GetLocaleInfoA(lcids[i].lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (LPSTR)&cp, sizeof(cp));
        ok(cp == lcids[i].threadcp, "wrong codepage %u for lcid %04x, should be %u\n", cp, lcids[i].threadcp, cp);

        /* GetCPInfoEx/GetCPInfo - CP_ACP */
        SetLastError(0xdeadbeef);
        memset(&cpi, 0, sizeof(cpi));
        ret = GetCPInfoExA(CP_ACP, 0, &cpi);
        ok(ret, "GetCPInfoExA failed for lcid %04x, error %d\n", lcids[i].lcid, GetLastError());
        ok(cpi.CodePage == acp, "wrong codepage %u for lcid %04x, should be %u\n", cpi.CodePage, lcids[i].lcid, acp);

        /* WideCharToMultiByte - CP_ACP */
        num = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, NULL, 0, NULL, NULL);
        ok(num == 7, "ret is %d (%04x)\n", num, lcids[i].lcid);

        /* MultiByteToWideChar - CP_ACP */
        num = MultiByteToWideChar(CP_ACP, 0, "foobar", -1, NULL, 0);
        ok(num == 7, "ret is %d (%04x)\n", num, lcids[i].lcid);

        /* GetCPInfoEx/GetCPInfo - CP_THREAD_ACP */
        SetLastError(0xdeadbeef);
        memset(&cpi, 0, sizeof(cpi));
        ret = GetCPInfoExA(CP_THREAD_ACP, 0, &cpi);
        ok(ret, "GetCPInfoExA failed for lcid %04x, error %d\n", lcids[i].lcid, GetLastError());
        if (lcids[i].threadcp)
            ok(cpi.CodePage == lcids[i].threadcp, "wrong codepage %u for lcid %04x, should be %u\n",
               cpi.CodePage, lcids[i].lcid, lcids[i].threadcp);
        else
            ok(cpi.CodePage == acp, "wrong codepage %u for lcid %04x, should be %u\n",
               cpi.CodePage, lcids[i].lcid, acp);

        /* WideCharToMultiByte - CP_THREAD_ACP */
        num = WideCharToMultiByte(CP_THREAD_ACP, 0, foobarW, -1, NULL, 0, NULL, NULL);
        ok(num == 7, "ret is %d (%04x)\n", num, lcids[i].lcid);

        /* MultiByteToWideChar - CP_THREAD_ACP */
        num = MultiByteToWideChar(CP_THREAD_ACP, 0, "foobar", -1, NULL, 0);
        ok(num == 7, "ret is %d (%04x)\n", num, lcids[i].lcid);
    }

    /* IsDBCSLeadByteEx - locales without codepage */
    for (i = 0; i < sizeof(isleads_nocp)/sizeof(isleads_nocp[0]); i++)
    {
        SetThreadLocale(isleads_nocp[i].lcid);

        islead_acp = IsDBCSLeadByteEx(CP_ACP,        isleads_nocp[i].testchar);
        islead     = IsDBCSLeadByteEx(CP_THREAD_ACP, isleads_nocp[i].testchar);

        ok(islead == islead_acp, "wrong islead %i for test char %x in lcid %04x.  should be %i\n",
            islead, isleads_nocp[i].testchar, isleads_nocp[i].lcid, islead_acp);
    }

    /* IsDBCSLeadByteEx - locales with codepage */
    for (i = 0; i < sizeof(isleads)/sizeof(isleads[0]); i++)
    {
        SetThreadLocale(isleads[i].lcid);

        islead = IsDBCSLeadByteEx(CP_THREAD_ACP, isleads[i].testchar);
        ok(islead == isleads[i].islead, "wrong islead %i for test char %x in lcid %04x.  should be %i\n",
            islead, isleads[i].testchar, isleads[i].lcid, isleads[i].islead);
    }

    SetThreadLocale(last);
}

START_TEST(codepage)
{
    BOOL bUsedDefaultChar;

    test_destination_buffer();
    test_null_source();
    test_negative_source_length();
    test_negative_dest_length();
    test_other_invalid_parameters();
    test_overlapped_buffers();

    /* WideCharToMultiByte has two code paths, test both here */
    test_string_conversion(NULL);
    test_string_conversion(&bUsedDefaultChar);

    test_utf7_encoding();
    test_utf7_decoding();

    test_undefined_byte_char();
    test_threadcp();
}
