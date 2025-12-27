/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _wcsnicmp
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef int (__cdecl *PFN_wcsnicmp)(const wchar_t* _String1, const wchar_t* _String2, size_t _MaxCount);
static PFN_wcsnicmp p_wcsnicmp;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_wcsnicmp = (PFN_wcsnicmp)GetProcAddress(hdll, "_wcsnicmp");
    ok(p_wcsnicmp != NULL, "Failed to load _wcsnicmp from %s\n", TEST_DLL_NAME);
    return (p_wcsnicmp != NULL);
}

START_TEST(_wcsnicmp)
{
    int result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _wcsnicmp is not available\n");
        return;
    }
#endif

    StartSeh()
        result = p_wcsnicmp(L"a", NULL, 0);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        result = p_wcsnicmp(L"a", NULL, 1);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif

    StartSeh()
        result = p_wcsnicmp(NULL, L"a", 0);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        result = p_wcsnicmp(NULL, L"a", 1);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif
}
