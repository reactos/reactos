/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Int64ToString
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <undocshell.h>
#include <versionhelpers.h>

static void Test_Int64ToString(BOOL bVista)
{
    WCHAR szBuff[64];
    WCHAR szDecimalSep[16] = L".";
    WCHAR szThousandSep[16] = L",";
    INT ret;
    NUMBERFMTW format;

    ret = Int64ToString(0, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 1);
    ok_wstr(szBuff, L"0");

    ret = Int64ToString(1, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 1);
    ok_wstr(szBuff, L"1");

    ret = Int64ToString(-9999, szBuff, _countof(szBuff), FALSE, NULL, 0);
    if (bVista)
    {
        ok_int(ret, 5);
        ok_wstr(szBuff, L"-9999");
    }
    else
    {
        ok_int(ret, 4);
        ok_wstr(szBuff, L"''''");
    }

    ret = Int64ToString(10000, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 5);
    ok_wstr(szBuff, L"10000");

    ret = Int64ToString(0xE8D4A51000LL, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 13);
    ok_wstr(szBuff, L"1000000000000");

    format.NumDigits = 2;
    format.LeadingZero = 0;
    format.Grouping = 3;
    format.lpDecimalSep = szDecimalSep;
    format.lpThousandSep = szThousandSep;
    format.NegativeOrder = 0;

    ret = Int64ToString(10000000000, szBuff, _countof(szBuff), FALSE, &format, -1);
    ok_int(ret, 11);
    ok_wstr(szBuff, L"10000000000");

    ret = Int64ToString(10000000000, szBuff, _countof(szBuff), TRUE, &format, -1);
    ok_int(ret, 17);
    ok_wstr(szBuff, L"10,000,000,000.00");
}

static void Test_LargeIntegerToString(BOOL bVista)
{
    LARGE_INTEGER LInt;
    WCHAR szBuff[64];
    WCHAR szDecimalSep[16] = L".";
    WCHAR szThousandSep[16] = L",";
    INT ret;
    NUMBERFMTW format;

    LInt.QuadPart = 0;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 1);
    ok_wstr(szBuff, L"0");

    LInt.QuadPart = 1;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 1);
    ok_wstr(szBuff, L"1");

    LInt.QuadPart = -9999;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, NULL, 0);
    if (bVista)
    {
        ok_int(ret, 5);
        ok_wstr(szBuff, L"-9999");
    }
    else
    {
        ok_int(ret, 4);
        ok_wstr(szBuff, L"''''");
    }

    LInt.QuadPart = 10000;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 5);
    ok_wstr(szBuff, L"10000");

    LInt.QuadPart = 0xE8D4A51000LL;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, NULL, 0);
    ok_int(ret, 13);
    ok_wstr(szBuff, L"1000000000000");

    format.NumDigits = 2;
    format.LeadingZero = 0;
    format.Grouping = 3;
    format.lpDecimalSep = szDecimalSep;
    format.lpThousandSep = szThousandSep;
    format.NegativeOrder = 0;

    LInt.QuadPart = 10000000000;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), FALSE, &format, -1);
    ok_int(ret, 11);
    ok_wstr(szBuff, L"10000000000");

    LInt.QuadPart = 10000000000;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), TRUE, &format, -1);
    ok_int(ret, 17);
    ok_wstr(szBuff, L"10,000,000,000.00");
}

START_TEST(Int64ToString)
{
    BOOL bVista = IsWindowsVistaOrGreater();
    trace("bVista: %d\n", bVista);

    Test_Int64ToString(bVista);
    Test_LargeIntegerToString(bVista);
}
