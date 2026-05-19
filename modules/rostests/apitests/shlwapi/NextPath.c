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
    PSTR pch;
    CHAR sz[MAX_PATH];

    /* NULL pszStart returns NULL */
    pch = s_pNextPathA(NULL, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Basic semicolon-separated paths */
    pch = s_pNextPathA("C:\\TEST1;C:\\TEST2;C:\\TEST3", sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST2");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST3");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Whitespace-only segment */
    pch = s_pNextPathA("C:\\TEST1;   ;C:\\TEST3", sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Empty string: no paths at all */
    pch = s_pNextPathA("", sz, _countof(sz));
    ok(pch == NULL, "empty string: pch was %p\n", pch);

    /* Leading semicolons are skipped */
    pch = s_pNextPathA(";;;C:\\TEST1", sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    ok(pch != NULL, "leading semicolons: pch should not be NULL\n");

    /* Trailing semicolon */
    pch = s_pNextPathA("C:\\TEST1;", sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok(pch == NULL, "trailing semicolon: pch was %p\n", pch);

    /* Only semicolons */
    pch = s_pNextPathA(";;;", sz, _countof(sz));
    ok(pch == NULL, "only semicolons: pch was %p\n", pch);

    /* Path with surrounding spaces */
    pch = s_pNextPathA("  C:\\TEST1  ;C:\\TEST2", sz, _countof(sz));
    ok_str(sz, "C:\\TEST1");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok_str(sz, "C:\\TEST2");

    /* Single path, no semicolon */
    pch = s_pNextPathA("C:\\SINGLE", sz, _countof(sz));
    ok_str(sz, "C:\\SINGLE");
    pch = s_pNextPathA(pch, sz, _countof(sz));
    ok(pch == NULL, "single path: pch was %p\n", pch);

    /* cchDest = 0 */
    sz[0] = '*';
    sz[1] = ANSI_NULL;
    pch = s_pNextPathA("C:\\TEST1;C:\\TEST2;C:\\TEST3", sz, 0);
    ok_str(pch, "C:\\TEST2;C:\\TEST3");
    ok_str(sz, "*");
}

static void TEST_NextPathW(void)
{
    PWSTR pch;
    WCHAR sz[MAX_PATH];

    /* NULL pszStart returns NULL */
    pch = s_pNextPathW(NULL, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Basic semicolon-separated paths */
    pch = s_pNextPathW(L"C:\\TEST1;C:\\TEST2;C:\\TEST3", sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST2");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST3");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Whitespace-only segment */
    pch = s_pNextPathW(L"C:\\TEST1;   ;C:\\TEST3", sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok(pch == NULL, "pch was %p\n", pch);

    /* Empty string */
    pch = s_pNextPathW(L"", sz, _countof(sz));
    ok(pch == NULL, "empty string: pch was %p\n", pch);

    /* Leading semicolons are skipped */
    pch = s_pNextPathW(L";;;C:\\TEST1", sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    ok(pch != NULL, "leading semicolons: pch should not be NULL\n");

    /* Trailing semicolon */
    pch = s_pNextPathW(L"C:\\TEST1;", sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok(pch == NULL, "trailing semicolon: pch was %p\n", pch);

    /* Only semicolons */
    pch = s_pNextPathW(L";;;", sz, _countof(sz));
    ok(pch == NULL, "only semicolons: pch was %p\n", pch);

    /* Path with surrounding spaces */
    pch = s_pNextPathW(L"  C:\\TEST1  ;C:\\TEST2", sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST1");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok_wstr(sz, L"C:\\TEST2");

    /* Single path, no semicolon */
    pch = s_pNextPathW(L"C:\\SINGLE", sz, _countof(sz));
    ok_wstr(sz, L"C:\\SINGLE");
    pch = s_pNextPathW(pch, sz, _countof(sz));
    ok(pch == NULL, "single path: pch was %p\n", pch);

    /* cchDest = 0 */
    sz[0] = L'*';
    sz[1] = UNICODE_NULL;
    pch = s_pNextPathW(L"C:\\TEST1;C:\\TEST2;C:\\TEST3", sz, 0);
    ok_wstr(pch, L"C:\\TEST2;C:\\TEST3");
    ok_wstr(sz, L"*");
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
