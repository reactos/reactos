/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Int64ToString
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <undocshell.h>
#include <versionhelpers.h>

typedef struct tagTEST_RETURN
{
    INT ret;
    LPCWSTR text;
} TEST_RETURN, *PTEST_RETURN;

typedef struct tagTEST_ENTRY
{
    INT lineno;
    LONGLONG value;

    UINT NumDigits;
    UINT LeadingZero;
    UINT Grouping;
    LPCWSTR pszDecimalSep;
    LPCWSTR pszThousandSep;
    UINT NegativeOrder;

    TEST_RETURN NonVista;
    TEST_RETURN Vista;
} TEST_ENTRY, *PTEST_ENTRY;

#define DATA0 2, FALSE, 3, L".", L",", 0
#define DATA1 3, TRUE, 3, L"<>", L":", 1

static const TEST_ENTRY s_Entries[] =
{
    { __LINE__, 0, DATA0, { 3, L".00" }, { 3, L".00" } },
    { __LINE__, 0, DATA1, { 6, L"0<>000" }, { 6, L"0<>000" } },
    { __LINE__, 1, DATA0, { 4, L"1.00" }, { 4, L"1.00" } },
    { __LINE__, 1, DATA1, { 6, L"1<>000" }, { 6, L"1<>000" } },
    { __LINE__, -999, DATA0, { 0, L"#####" }, { 8, L"(999.00)" } },
    { __LINE__, -999, DATA1, { 0, L"#####" }, { 9, L"-999<>000" } },
    { __LINE__, 100000, DATA0, { 10, L"100,000.00" }, { 10, L"100,000.00" } },
    { __LINE__, 100000, DATA1, { 12, L"100:000<>000" }, { 12, L"100:000<>000" } },
    { __LINE__, 0xE8D4A51000LL, DATA0, { 20, L"1,000,000,000,000.00" }, { 20, L"1,000,000,000,000.00" } },
    { __LINE__, 0xE8D4A51000LL, DATA1, { 22, L"1:000:000:000:000<>000" }, { 22, L"1:000:000:000:000<>000" } },
    { __LINE__, 0x7FFFFFFFFFFFFFFFLL, DATA0, { 28, L"9,223,372,036,854,775,807.00" }, { 28, L"9,223,372,036,854,775,807.00" } },
    { __LINE__, 0x7FFFFFFFFFFFFFFFLL, DATA1, { 30, L"9:223:372:036:854:775:807<>000" }, { 30, L"9:223:372:036:854:775:807<>000" } },
};
static const SIZE_T s_cEntries = _countof(s_Entries);

static void DoTestEntry(BOOL bVista, const TEST_ENTRY *pEntry)
{
    INT lineno = pEntry->lineno;

    WCHAR szDecimalSep[10], szThousandSep[10];
    lstrcpynW(szDecimalSep, pEntry->pszDecimalSep, _countof(szDecimalSep));
    lstrcpynW(szThousandSep, pEntry->pszThousandSep, _countof(szThousandSep));

    NUMBERFMTW format =
    {
        pEntry->NumDigits, pEntry->LeadingZero, pEntry->Grouping,
        szDecimalSep, szThousandSep, pEntry->NegativeOrder
    };

    WCHAR szBuff[64];
    lstrcpynW(szBuff, L"#####", _countof(szBuff));

    INT ret = Int64ToString(pEntry->value, szBuff, _countof(szBuff), TRUE, &format, -1);
    if (bVista)
    {
        ok(pEntry->Vista.ret == ret, "Line %d: %d vs %d\n", lineno, pEntry->Vista.ret, ret);
        ok(lstrcmpW(pEntry->Vista.text, szBuff) == 0, "Line %d: %ls vs %ls\n",
           lineno, pEntry->Vista.text, szBuff);
    }
    else
    {
        ok(pEntry->NonVista.ret == ret, "Line %d: %d vs %d\n", lineno, pEntry->NonVista.ret, ret);
        ok(lstrcmpW(pEntry->NonVista.text, szBuff) == 0, "Line %d: %ls vs %ls\n",
           lineno, pEntry->NonVista.text, szBuff);
    }

    lstrcpynW(szBuff, L"#####", _countof(szBuff));

    LARGE_INTEGER LInt;
    LInt.QuadPart = pEntry->value;
    ret = LargeIntegerToString(&LInt, szBuff, _countof(szBuff), TRUE, &format, -1);
    if (bVista)
    {
        ok(pEntry->Vista.ret == ret, "Line %d: %d vs %d\n", lineno, pEntry->Vista.ret, ret);
        ok(lstrcmpW(pEntry->Vista.text, szBuff) == 0, "Line %d: %ls vs %ls\n",
           lineno, pEntry->Vista.text, szBuff);
    }
    else
    {
        ok(pEntry->NonVista.ret == ret, "Line %d: %d vs %d\n", lineno, pEntry->NonVista.ret, ret);
        ok(lstrcmpW(pEntry->NonVista.text, szBuff) == 0, "Line %d: %ls vs %ls\n",
           lineno, pEntry->NonVista.text, szBuff);
    }
}

static void Test_EntryTest(BOOL bVista)
{
    for (SIZE_T i = 0; i < s_cEntries; ++i)
    {
        DoTestEntry(bVista, &s_Entries[i]);
    }
}

static void Test_Int64ToString(BOOL bVista)
{
    WCHAR szBuff[64];
    INT ret;

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
}

static void Test_LargeIntegerToString(BOOL bVista)
{
    LARGE_INTEGER LInt;
    WCHAR szBuff[64];
    INT ret;

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
}

START_TEST(Int64ToString)
{
    BOOL bVista = IsWindowsVistaOrGreater();
    trace("bVista: %d\n", bVista);

    Test_EntryTest(bVista);
    Test_Int64ToString(bVista);
    Test_LargeIntegerToString(bVista);
}
