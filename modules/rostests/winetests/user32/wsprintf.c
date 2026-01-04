 /* Unit test suite for the wsprintf functions
 *
 * Copyright 2002 Bill Medland
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

static const struct
{
    const char *fmt;
    ULONGLONG value;
    const char *res;
} i64_formats[] =
{
    { "%I64X", ((ULONGLONG)0x12345 << 32) | 0x67890a, "123450067890A" },
    { "%I32X", ((ULONGLONG)0x12345 << 32) | 0x67890a, "67890A" },
    { "%I64d", (ULONGLONG)543210 * 1000000, "543210000000" },
    { "%I64X", (LONGLONG)-0x12345, "FFFFFFFFFFFEDCBB" },
    { "%I32x", (LONGLONG)-0x12345, "fffedcbb" },
    { "%I64u", (LONGLONG)-123, "18446744073709551493" },
    { "%Id",   (LONGLONG)-12345, "-12345" },
#ifdef _WIN64
    { "%Ix",   ((ULONGLONG)0x12345 << 32) | 0x67890a, "123450067890a" },
    { "%Ix",   (LONGLONG)-0x12345, "fffffffffffedcbb" },
    { "%p",    (LONGLONG)-0x12345, "FFFFFFFFFFFEDCBB" },
#else
    { "%Ix",   ((ULONGLONG)0x12345 << 32) | 0x67890a, "67890a" },
    { "%Ix",   (LONGLONG)-0x12345, "fffedcbb" },
    { "%p",    (LONGLONG)-0x12345, "FFFEDCBB" },
#endif
};

static void wsprintfATest(void)
{
    char buf[25], star[4], partial[4];
    static const WCHAR starW[] = L"\x2606";
    static const WCHAR fffeW[] = L"\xfffe";
    static const WCHAR wineW[] = L"\xd83c\xdf77"; /* U+1F377: wine glass */
    const struct {
        const void *input;
        const char *fmt;
        const char *str;
        int rc;
    }
    testcase[] = {
        { starW, "%.1S", partial, 1 },
        { starW, "%.2S", star,    2 },
        { starW, "%.3S", star,    2 },
        { fffeW, "%.1S", "?",     1 },
        { fffeW, "%.2S", "?",     1 },
        { wineW, "%.2S", "??",    2 },
        { star,  "%.1s", partial, 1 },
        { (void *)0x2606, "%2C",  star,    2 },
        { (void *)0xfffd, "%C",   "?",     1 },
        { (void *)0xe199, "%2c",  " \x99", 2 },
    };
    CPINFO cpinfo;
    unsigned int i;
    int rc;

    rc=wsprintfA(buf, "%010ld", -1);
    ok(rc == 10, "wsprintfA length failure: rc=%d error=%ld\n",rc,GetLastError());
    ok((lstrcmpA(buf, "-000000001") == 0),
       "wsprintfA zero padded negative value failure: buf=[%s]\n",buf);
    rc = wsprintfA(buf, "%I64X", (ULONGLONG)0);
    if (rc == 4 && !lstrcmpA(buf, "I64X"))
    {
        win_skip( "I64 formats not supported\n" );
        return;
    }
    for (i = 0; i < ARRAY_SIZE(i64_formats); i++)
    {
        rc = wsprintfA(buf, i64_formats[i].fmt, i64_formats[i].value);
        ok(rc == strlen(i64_formats[i].res), "%u: wsprintfA length failure: rc=%d\n", i, rc);
        ok(!strcmp(buf, i64_formats[i].res), "%u: wrong result [%s]\n", i, buf);
    }

    if (!GetCPInfo(CP_ACP, &cpinfo) || cpinfo.MaxCharSize != 2)
    {
        skip("Multi-byte wsprintfA test isn't available for the current codepage\n");
        return;
    }

    rc = WideCharToMultiByte(CP_ACP, 0, starW, -1, star, sizeof(star), NULL, NULL);
    ok(rc == 3, "unexpected rc, got %d\n", rc);
    partial[0] = star[0];
    partial[1] = '\0';

    for (i = 0; i < ARRAY_SIZE(testcase); i++)
    {
        memset(buf, 0x11, sizeof(buf));
        rc = wsprintfA(buf, testcase[i].fmt, testcase[i].input);

        ok(rc == testcase[i].rc,
           "%u: expected %d, got %d\n",
           i, testcase[i].rc, rc);

        ok(!strcmp(buf, testcase[i].str),
           "%u: expected %s, got %s\n",
           i, wine_dbgstr_a(testcase[i].str), wine_dbgstr_an(buf, rc));
    }
}

static WCHAR my_btowc(BYTE c)
{
    WCHAR wc;
    if (!IsDBCSLeadByte(c) &&
        MultiByteToWideChar(CP_ACP, 0, (const char *)&c, 1, &wc, 1) > 0)
        return wc;
    return 0;
}

static void wsprintfWTest(void)
{
    static const WCHAR stars[] = L"*\x2606\x2605";
    WCHAR def_spc[] = L"*?\x2605 ";
    WCHAR buf[25], fmt[25], res[25], wcA1, wc99;
    char stars_mb[8], partial00[8], partialFF[8];
    const struct {
        const char *input;
        const WCHAR *fmt;
        const WCHAR *str;
        int rc;
    }
    testcase[] = {
        { stars_mb, L"%.3S", stars, 3 },
        { partial00, L"%-4S", L"*\0  ", 4 },
        { partialFF, L"%-4S", def_spc, 4 },
    };
    CPINFOEXW cpinfoex;
    unsigned int i;
    int rc;

    rc=wsprintfW(buf, L"%010ld", -1);
    if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("wsprintfW is not implemented\n");
        return;
    }
    ok(rc == 10, "wsPrintfW length failure: rc=%d error=%ld\n",rc,GetLastError());
    ok((lstrcmpW(buf, L"-000000001") == 0),
       "wsprintfW zero padded negative value failure\n");
    rc = wsprintfW(buf, L"%I64x", (ULONGLONG)0 );
    if (rc == 4 && !lstrcmpW(buf, L"I64x"))
    {
        win_skip( "I64 formats not supported\n" );
        return;
    }
    for (i = 0; i < ARRAY_SIZE(i64_formats); i++)
    {
        MultiByteToWideChar( CP_ACP, 0, i64_formats[i].fmt, -1, fmt, ARRAY_SIZE(fmt));
        MultiByteToWideChar( CP_ACP, 0, i64_formats[i].res, -1, res, ARRAY_SIZE(res));
        rc = wsprintfW(buf, fmt, i64_formats[i].value);
        ok(rc == lstrlenW(res), "%u: wsprintfW length failure: rc=%d\n", i, rc);
        ok(!lstrcmpW(buf, res), "%u: wrong result [%s]\n", i, wine_dbgstr_w(buf));
    }

    rc = wsprintfW(buf, L"%2c", L'*');
    ok(rc == 2, "expected 2, got %d\n", rc);
    ok(buf[0] == L' ', "expected \\x0020, got \\x%04x\n", buf[0]);
    ok(buf[1] == L'*', "expected \\x%04x, got \\x%04x\n", L'*', buf[1]);

    rc = wsprintfW(buf, L"%c", L'\x2605');
    ok(rc == 1, "expected 1, got %d\n", rc);
    ok(buf[0] == L'\x2605', "expected \\x%04x, got \\x%04x\n", L'\x2605', buf[0]);

    wcA1 = my_btowc(0xA1);
    rc = wsprintfW(buf, L"%C", 0xA1);
    ok(rc == 1, "expected 1, got %d\n", rc);
    ok(buf[0] == wcA1, "expected \\x%04x, got \\x%04x\n", wcA1, buf[0]);

    rc = wsprintfW(buf, L"%C", 0x81A1);
    ok(rc == 1, "expected 1, got %d\n", rc);
    ok(buf[0] == wcA1, "expected \\x%04x, got \\x%04x\n", wcA1, buf[0]);

    wc99 = my_btowc(0x99);
    rc = wsprintfW(buf, L"%2C", 0xe199);
    ok(rc == 2, "expected 1, got %d\n", rc);
    ok(buf[0] == L' ', "expected \\x0020, got \\x%04x\n", buf[0]);
    ok(buf[1] == wc99, "expected \\x%04x, got \\x%04x\n", wc99, buf[1]);

    if (!GetCPInfoExW(CP_ACP, 0, &cpinfoex) || cpinfoex.MaxCharSize != 2)
    {
        skip("Multi-byte wsprintfW test isn't available for the current codepage\n");
        return;
    }
    def_spc[1] = cpinfoex.UnicodeDefaultChar;

    rc = WideCharToMultiByte(CP_ACP, 0, stars, -1, stars_mb, sizeof(stars_mb), NULL, NULL);
    ok(rc == 6, "expected 6, got %d\n", rc);
    strcpy(partial00, stars_mb);
    partial00[2] = '\0';
    strcpy(partialFF, stars_mb);
    partialFF[2] = 0xff;

    for (i = 0; i < ARRAY_SIZE(testcase); i++)
    {
        memset(buf, 0x11, sizeof(buf));
        rc = wsprintfW(buf, testcase[i].fmt, testcase[i].input);

        ok(rc == testcase[i].rc,
           "%u: expected %d, got %d\n",
           i, testcase[i].rc, rc);

        ok(!memcmp(buf, testcase[i].str, (testcase[i].rc + 1) * sizeof(WCHAR)),
           "%u: expected %s, got %s\n", i,
           wine_dbgstr_wn(testcase[i].str, testcase[i].rc + 1),
           wine_dbgstr_wn(buf, rc + 1));
    }
}

/* Test if the CharUpper / CharLower functions return true 16 bit results,
   if the input is a 16 bit input value. */

static void CharUpperTest(void)
{
    INT_PTR i, out;
    BOOL failed = FALSE;

    for (i=0;i<256;i++)
    	{
	out = (INT_PTR)CharUpperA((LPSTR)i);
	if ((out >> 16) != 0)
	   {
           failed = TRUE;
	   break;
	   }
	}
    ok(!failed,"CharUpper failed - 16bit input (0x%0Ix) returned 32bit result (0x%0Ix)\n",i,out);
}

static void CharLowerTest(void)
{
    INT_PTR i, out;
    BOOL failed = FALSE;

    for (i=0;i<256;i++)
    	{
	out = (INT_PTR)CharLowerA((LPSTR)i);
	if ((out >> 16) != 0)
	   {
           failed = TRUE;
	   break;
	   }
	}
    ok(!failed,"CharLower failed - 16bit input (0x%0Ix) returned 32bit result (0x%0Ix)\n",i,out);
}


START_TEST(wsprintf)
{
    wsprintfATest();
    wsprintfWTest();
    CharUpperTest();
    CharLowerTest();
}
