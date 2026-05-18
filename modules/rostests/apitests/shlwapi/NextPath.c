/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for NextPathA/W
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>

typedef PSTR  (WINAPI *FN_NextPathA)(PCSTR, PSTR, UINT);
typedef PWSTR (WINAPI *FN_NextPathW)(PCWSTR, PWSTR, UINT);

static FN_NextPathA s_pNextPathA = NULL;
static FN_NextPathW s_pNextPathW = NULL;

static void TEST_NextPathA(void)
{
    PCSTR psz1 = "C:\\TEST1;C:\\TEST2;C:\\TEST3";
    PSTR pch = (PSTR)psz1;
    CHAR sz[MAX_PATH];

    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST2");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST3");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %s\n", pch);
}

static void TEST_NextPathW(void)
{
    PCWSTR psz1 = L"C:\\TEST1;C:\\TEST2;C:\\TEST3";
    PWSTR pch = (PWSTR)psz1;
    WCHAR sz[MAX_PATH];

    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST2");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST3");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %S\n", pch);
}

START_TEST(NextPath)
{
    HINSTANCE hSHLWAPI = GetModuleHandleA("shlwapi");
    s_pNextPathA = (FN_NextPathA)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(449));
    s_pNextPathW = (FN_NextPathW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(450));
    if (!s_pNextPathA || !s_pNextPathW)
    {
        skip("NextPath not found\n");
        return;
    }

    TEST_NextPathA();
    TEST_NextPathW();
}
