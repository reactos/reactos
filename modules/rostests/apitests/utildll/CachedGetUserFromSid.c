/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for CachedGetUserFromSid
 * COPYRIGHT:   Copyright 2020 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>

#include <lmcons.h>
#include <wchar.h>

static VOID (WINAPI *pCachedGetUserFromSid)(PSID pSid, LPWSTR pUserName, PULONG pcwcUserName);

static void test_Sid_Null(void)
{
    const WCHAR szUserNameFromNull[] = L"(unknown)";
    const ULONG cchUserNameFromNull = (ULONG)wcslen(szUserNameFromNull);

    WCHAR szUserName[UNLEN + 1];
    ULONG cchUserName;

    // First, full success case, to load result into cache.
    // Otherwise, expect misbehavior/corruption/crash!
    // Same issues with 'NULL' or '0' arguments.
    // Behavior changed on NT6.0 then NT6.1+...

    wmemset(szUserName, L'?', _countof(szUserName));
    cchUserName = _countof(szUserName);
    pCachedGetUserFromSid(NULL, szUserName, &cchUserName);
    ok(cchUserName == cchUserNameFromNull, "cchUserName: expected %lu, got %lu\n", cchUserNameFromNull, cchUserName);
    ok(wcscmp(szUserName, szUserNameFromNull) == 0, "szUserName: expected \"%S\", got \"%.*S\"\n", szUserNameFromNull, (int)cchUserName, szUserName);

    wmemset(szUserName, L'?', _countof(szUserName));
    cchUserName = 1;
    pCachedGetUserFromSid(NULL, szUserName, &cchUserName);
    ok(cchUserName == 0, "cchUserName: expected 0, got %lu\n", cchUserName);
    ok(szUserName[0] == UNICODE_NULL, "szUserName: missing UNICODE_NULL, got \"%.*S\"\n", (int)cchUserName, szUserName);

    wmemset(szUserName, L'?', _countof(szUserName));
    cchUserName = 2;
    pCachedGetUserFromSid(NULL, szUserName, &cchUserName);
    ok(cchUserName == 1, "cchUserName: expected 0, got %lu\n", cchUserName);
    ok(szUserName[1] == UNICODE_NULL, "szUserName: missing UNICODE_NULL, got \"%.*S\"\n", (int)cchUserName, szUserName);
    ok(wcsncmp(szUserName, szUserNameFromNull, 1) == 0, "szUserName: expected \"%.*S\", got \"%.*S\"\n", 1, szUserNameFromNull, (int)cchUserName, szUserName);

    wmemset(szUserName, L'?', _countof(szUserName));
    cchUserName = cchUserNameFromNull;
    pCachedGetUserFromSid(NULL, szUserName, &cchUserName);
    ok(cchUserName == cchUserNameFromNull - 1, "cchUserName: expected %lu, got %lu\n", cchUserNameFromNull - 1, cchUserName);
    ok(szUserName[cchUserNameFromNull - 1] == UNICODE_NULL, "szUserName: missing UNICODE_NULL, got \"%.*S\"\n", (int)cchUserName, szUserName);
    ok(wcsncmp(szUserName, szUserNameFromNull, cchUserNameFromNull - 1) == 0, "szUserName: expected \"%.*S\", got \"%.*S\"\n", (int)cchUserNameFromNull - 1, szUserNameFromNull, (int)cchUserName, szUserName);

    wmemset(szUserName, L'?', _countof(szUserName));
    cchUserName = cchUserNameFromNull + 1;
    pCachedGetUserFromSid(NULL, szUserName, &cchUserName);
    ok(cchUserName == cchUserNameFromNull, "cchUserName: expected %lu, got %lu\n", cchUserNameFromNull, cchUserName);
    ok(wcscmp(szUserName, szUserNameFromNull) == 0, "szUserName: expected \"%S\", got \"%.*S\"\n", szUserNameFromNull, (int)cchUserName, szUserName);
}

START_TEST(CachedGetUserFromSid)
{
    const char szFunction[] = "CachedGetUserFromSid";
    void* pFunction;

    // TODO: Dynamically checking, until ReactOS implements this dll.
    HMODULE hModule;
    DWORD dwLE;

    hModule = LoadLibraryW(L"utildll.dll");
    if (!hModule)
    {
        dwLE = GetLastError();
        ok(FALSE, "LoadLibraryW(\"%S\") failed! (dwLE = %lu)\n", L"utildll.dll", dwLE);
        skip("No DLL\n");
        return;
    }

    pFunction = (void*)GetProcAddress(hModule, szFunction);
    if (!pFunction)
    {
        dwLE = GetLastError();
        ok(FALSE, "GetProcAddress(\"%s\") failed! (dwLE = %lu)\n", szFunction, dwLE);
        skip("No function\n");
        FreeLibrary(hModule);
        return;
    }

    pCachedGetUserFromSid = pFunction;

    test_Sid_Null();

    FreeLibrary(hModule);
}
