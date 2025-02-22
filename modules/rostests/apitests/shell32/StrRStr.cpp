/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for StrRStrA/W
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <versionhelpers.h>

typedef PSTR (WINAPI *FN_StrRStrA)(PCSTR, PCSTR, PCSTR pszSearch);
typedef PWSTR (WINAPI *FN_StrRStrW)(PCWSTR, PCWSTR, PCWSTR pszSearch);

static VOID TEST_StrRStrA(VOID)
{
    PCSTR psz, pch;
    PSTR ret;
    FN_StrRStrA StrRStrA = (FN_StrRStrA)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(389));

    if (!StrRStrA)
    {
        skip("StrRStrA not found\n");
        return;
    }

    psz = "ABCBC";
    ret = StrRStrA(psz, NULL, "BC");
    ok_ptr(ret, psz + 3);

    psz = "ABCBC";
    pch = &psz[2];
    ret = StrRStrA(psz, pch, "BC");
    ok_ptr(ret, &psz[1]);

    psz = "ABCBC";
    ret = StrRStrA(psz, psz, "BC");
    ok(!ret, "ret was '%s'\n", ret);

    psz = "ABCBC";
    pch = &psz[lstrlenA(psz)];
    ret = StrRStrA(psz, pch, "BC");
    ok_ptr(ret, psz + 3);
}

static VOID TEST_StrRStrW(VOID)
{
    PCWSTR psz, pch;
    PWSTR ret;
    FN_StrRStrW StrRStrW = (FN_StrRStrW)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(392));

    if (!StrRStrW)
    {
        skip("StrRStrW not found\n");
        return;
    }

    psz = L"ABCBC";
    ret = StrRStrW(psz, NULL, L"BC");
    ok_ptr(ret, psz + 3);

    psz = L"ABCBC";
    pch = &psz[2];
    ret = StrRStrW(psz, pch, L"BC");
    ok_ptr(ret, &psz[1]);

    psz = L"ABCBC";
    ret = StrRStrW(psz, psz, L"BC");
    ok(!ret, "ret was '%S'\n", ret);

    psz = L"ABCBC";
    pch = &psz[lstrlenW(psz)];
    ret = StrRStrW(psz, pch, L"BC");
    ok_ptr(ret, psz + 3);
}

static BOOL IsWindowsServer2003SP2OrGreater(VOID)
{
    return IsWindowsVersionOrGreater(5, 2, 2);
}

START_TEST(StrRStr)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n");
        return;
    }

    if (!IsWindowsServer2003SP2OrGreater())
    {
        skip("Before 2K3 SP3\n");
        return;
    }

    TEST_StrRStrA();
    TEST_StrRStrW();
}
