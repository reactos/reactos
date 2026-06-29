/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for StrToInt{,Ex,Ex64}{A,W}
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>

#define ok_longlong(expression, result) do { \
    long long _value = (expression); \
    ok(_value == (result), "Wrong value for '%s', expected: " #result " (0x%llx), got: 0x%llx\n", \
       #expression, (long long)(result), _value); \
} while (0)

static VOID TEST_StrToIntA(VOID)
{
    INT n0, n1;
    LONGLONG n2;
    LPCSTR psz;
    BOOL ret1, ret2;

    n2 = n1 = 0xDEADFACE;
    psz = NULL;
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 0);
    ok_longlong(n2, 0xFFFFFFFFDEADFACE);
    ok_int(ret1, FALSE);
    ok_int(ret2, FALSE);

    n2 = n1 = 0xDEADFACE;
    psz = "";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 0);
    ok_longlong(n2, 0);
    ok_int(ret1, FALSE);
    ok_int(ret2, FALSE);

    n2 = n1 = 0xDEADFACE;
    psz = "123";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 123);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = "+123";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = "-123";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, -123);
    ok_int(n1, -123);
    ok_longlong(n2, -123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = " +123";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = " -123";
    n0 = StrToIntA(psz);
    ret1 = StrToIntExA(psz, 0, &n1);
    ret2 = StrToInt64ExA(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, -123);
    ok_longlong(n2, -123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);
}

static VOID TEST_StrToIntW(VOID)
{
    INT n0, n1;
    LONGLONG n2;
    LPCWSTR psz;
    BOOL ret1, ret2;

    n2 = n1 = 0xDEADFACE;
    psz = NULL;
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 0);
    ok_longlong(n2, 0xFFFFFFFFDEADFACE);
    ok_int(ret1, FALSE);
    ok_int(ret2, FALSE);

    n2 = n1 = 0xDEADFACE;
    psz = L"";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 0);
    ok_longlong(n2, 0);
    ok_int(ret1, FALSE);
    ok_int(ret2, FALSE);

    n2 = n1 = 0xDEADFACE;
    psz = L"123";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 123);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = L"+123";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = L"-123";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, -123);
    ok_int(n1, -123);
    ok_longlong(n2, -123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = L" +123";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, 123);
    ok_longlong(n2, 123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);

    n2 = n1 = 0xDEADFACE;
    psz = L" -123";
    n0 = StrToIntW(psz);
    ret1 = StrToIntExW(psz, 0, &n1);
    ret2 = StrToInt64ExW(psz, 0, &n2);
    ok_int(n0, 0);
    ok_int(n1, -123);
    ok_longlong(n2, -123);
    ok_int(ret1, TRUE);
    ok_int(ret2, TRUE);
}

START_TEST(StrToInt)
{
    TEST_StrToIntA();
    TEST_StrToIntW();
}
