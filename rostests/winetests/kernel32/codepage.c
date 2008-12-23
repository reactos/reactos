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
#include <limits.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

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

/* lstrcmpW is not supported on Win9x! */
static int mylstrcmpW(const WCHAR* str1, const WCHAR* str2)
{
    while (*str1 && *str1==*str2) {
        str1++;
        str2++;
    }
    return *str1-*str2;
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
    ok(len == 7 && !lstrcmpA(buf, "foobar") && GetLastError() == 0xdeadbeef,
       "WideCharToMultiByte(-2002): len=%d error=%u\n", len, GetLastError());

    SetLastError( 0xdeadbeef );
    memset(bufW,'x',sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, "foobar", -2002, bufW, 10);
    ok(len == 7 && !mylstrcmpW(bufW, foobarW) && GetLastError() == 0xdeadbeef,
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
    static char buf[LONGBUFLEN];
    static WCHAR originalW[LONGBUFLEN];
    static char originalA[LONGBUFLEN];
    DWORD theError;

    /* Test return on -1 dest length */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buf, -1, NULL, NULL);
    todo_wine {
      ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
         "WideCharToMultiByte(destlen -1): len=%d error=%x\n", len, GetLastError());
    }

    /* Test return on -1000 dest length */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buf, -1000, NULL, NULL);
    todo_wine {
      ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
         "WideCharToMultiByte(destlen -1000): len=%d error=%x\n", len, GetLastError());
    }

    /* Test return on INT_MAX dest length */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -1, buf, INT_MAX, NULL, NULL);
    ok(len == 7 && !lstrcmpA(buf, "foobar") && GetLastError() == 0xdeadbeef,
       "WideCharToMultiByte(destlen INT_MAX): len=%d error=%x\n", len, GetLastError());

    /* Test return on INT_MAX dest length and very long input */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    for (i=0; i < LONGBUFLEN - 1; i++) {
        originalW[i] = 'Q';
        originalA[i] = 'Q';
    }
    originalW[LONGBUFLEN-1] = 0;
    originalA[LONGBUFLEN-1] = 0;
    len = WideCharToMultiByte(CP_ACP, 0, originalW, -1, buf, INT_MAX, NULL, NULL);
    theError = GetLastError();
    ok(len == LONGBUFLEN && !lstrcmpA(buf, originalA) && theError == 0xdeadbeef,
       "WideCharToMultiByte(srclen %d, destlen INT_MAX): len %d error=%x\n", LONGBUFLEN, len, theError);

}

static void test_overlapped_buffers(void)
{
    static const WCHAR strW[] = {'j','u','s','t',' ','a',' ','t','e','s','t',0};
    static const char strA[] = "just a test";
    char buf[256];
    int ret;

    SetLastError(0xdeadbeef);
    memcpy((WCHAR *)(buf + 1), strW, sizeof(strW));
    ret = WideCharToMultiByte(CP_ACP, 0, (WCHAR *)(buf + 1), -1, buf, sizeof(buf), NULL, NULL);
    ok(ret == sizeof(strA), "unexpected ret %d\n", ret);
    ok(!memcmp(buf, strA, sizeof(strA)), "conversion failed: %s\n", buf);
    ok(GetLastError() == 0xdeadbeef, "GetLastError() is %u\n", GetLastError());
}

static void test_string_conversion(LPBOOL bUsedDefaultChar)
{
    char mbc;
    char mbs[5];
    int ret;
    WCHAR wc1 = 228;                           /* Western Windows-1252 character */
    WCHAR wc2 = 1088;                          /* Russian Windows-1251 character not displayable for Windows-1252 */
    WCHAR wcs[5] = {'T', 'h', 1088, 'i', 0};   /* String with ASCII characters and a Russian character */
    WCHAR dbwcs[3] = {28953, 25152, 0};        /* String with Chinese (codepage 950) characters */

    SetLastError(0xdeadbeef);
    ret = WideCharToMultiByte(1252, 0, &wc1, 1, &mbc, 1, NULL, bUsedDefaultChar);
    ok(ret == 1, "ret is %d\n", ret);
    ok(mbc == -28, "mbc is %d\n", mbc);
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
        ok(mbc == -16, "mbc is %d\n", mbc);
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
    ok(!strcmp(mbs, "µH©Ò"), "mbs is %s\n", mbs);
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
    ok(!strcmp(mbs, "µH"), "mbs is %s\n", mbs);
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

START_TEST(codepage)
{
    BOOL bUsedDefaultChar;

    test_destination_buffer();
    test_null_source();
    test_negative_source_length();
    test_negative_dest_length();
    test_overlapped_buffers();

    /* WideCharToMultiByte has two code paths, test both here */
    test_string_conversion(NULL);
    test_string_conversion(&bUsedDefaultChar);
}
