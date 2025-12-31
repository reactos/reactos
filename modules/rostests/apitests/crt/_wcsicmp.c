/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _wcsicmp
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef int (__cdecl *PFN_wcsicmp)(const wchar_t* _String1, const wchar_t* _String2);
static PFN_wcsicmp p_wcsicmp;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_wcsicmp = (PFN_wcsicmp)GetProcAddress(hdll, "_wcsicmp");
    ok(p_wcsicmp != NULL, "Failed to load _wcsicmp from %s\n", TEST_DLL_NAME);
    return (p_wcsicmp != NULL);
}

START_TEST(_wcsicmp)
{
    int result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _wcsicmp is not available\n");
        return;
    }
#endif

    StartSeh()
        result = p_wcsicmp(L"a", NULL);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || _winver >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif

    StartSeh()
        result = p_wcsicmp(NULL, L"a");
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || _winver >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif

    ok_eq_int(p_wcsicmp(L"abc", L"ABC"), 0);
    ok_eq_int(p_wcsicmp(L"ABC", L"abc"), 0);
    ok_eq_int(p_wcsicmp(L"abc", L"abd"), -1);
    ok_eq_int(p_wcsicmp(L"abd", L"abc"), 1);
    ok_eq_int(p_wcsicmp(L"abcd", L"ABC"), 'd');
    ok_eq_int(p_wcsicmp(L"ABC", L"abcd"), -'d');
    ok_eq_int(p_wcsicmp(L"ab", L"A "), 'b' - ' ');
    ok_eq_int(p_wcsicmp(L"AB", L"a "), 'b' - ' ');
    ok_eq_int(p_wcsicmp(L"a ", L"aB"), ' ' - 'b');

    /* This shows that _wcsicmp does a lowercase comparison. */
    ok_eq_int(p_wcsicmp(L"_", L"a"), '_' - 'a');
}
