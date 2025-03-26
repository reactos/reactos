/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for StrDupA/W
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <versionhelpers.h>

static void TEST_StrDupA(void)
{
    LPSTR ptrA;

    ptrA = StrDupA(NULL);

    if (IsWindowsXPOrGreater())
        ok_ptr(ptrA, NULL);
    else
        ok(ptrA && !*ptrA, "ptrA: '%s'\n", wine_dbgstr_a(ptrA));

    if (ptrA)
        LocalFree(ptrA);
}

static void TEST_StrDupW(void)
{
    LPWSTR ptrW;

    ptrW = StrDupW(NULL);

    if (IsWindowsXPOrGreater())
        ok_ptr(ptrW, NULL);
    else
        ok(ptrW && !*ptrW, "ptrW: '%s'\n", wine_dbgstr_w(ptrW));

    if (ptrW)
        LocalFree(ptrW);
}

static void TEST_StrRet(void)
{
    LPWSTR input, output;
    if (SUCCEEDED(SHStrDupW(L"Test", &input)))
    {
        STRRET strret;
        strret.uType = STRRET_WSTR;
        U(strret).pOleStr = input;
        output = NULL;
        ok_int(StrRetToStrW(&strret, NULL, &output), S_OK);
        ok_ptr(U(strret).pOleStr, NULL);
        ok_ptr(input, output);
        CoTaskMemFree(output);
    }
}

START_TEST(StrDup)
{
    TEST_StrDupA();
    TEST_StrDupW();
    TEST_StrRet();
}
