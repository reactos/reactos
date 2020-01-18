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
    char buf[25];
    unsigned int i;
    int rc;

    rc=wsprintfA(buf, "%010ld", -1);
    ok(rc == 10, "wsprintfA length failure: rc=%d error=%d\n",rc,GetLastError());
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
}

static void wsprintfWTest(void)
{
    static const WCHAR fmt_010ld[] = {'%','0','1','0','l','d','\0'};
    static const WCHAR res_010ld[] = {'-','0','0','0','0','0','0','0','0','1', '\0'};
    static const WCHAR fmt_I64x[] = {'%','I','6','4','x',0};
    WCHAR buf[25], fmt[25], res[25];
    unsigned int i;
    int rc;

    rc=wsprintfW(buf, fmt_010ld, -1);
    if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("wsprintfW is not implemented\n");
        return;
    }
    ok(rc == 10, "wsPrintfW length failure: rc=%d error=%d\n",rc,GetLastError());
    ok((lstrcmpW(buf, res_010ld) == 0),
       "wsprintfW zero padded negative value failure\n");
    rc = wsprintfW(buf, fmt_I64x, (ULONGLONG)0 );
    if (rc == 4 && !lstrcmpW(buf, fmt_I64x + 1))
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
    ok(!failed,"CharUpper failed - 16bit input (0x%0lx) returned 32bit result (0x%0lx)\n",i,out);
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
    ok(!failed,"CharLower failed - 16bit input (0x%0lx) returned 32bit result (0x%0lx)\n",i,out);
}


START_TEST(wsprintf)
{
    wsprintfATest();
    wsprintfWTest();
    CharUpperTest();
    CharLowerTest();
}
