/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _strnicmp
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef int (__cdecl *PFN_strnicmp)(const char* _String1, const char* _String2, size_t _MaxCount);
static PFN_strnicmp p_strnicmp;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_strnicmp = (PFN_strnicmp)GetProcAddress(hdll, "_strnicmp");
    ok(p_strnicmp != NULL, "Failed to load _strnicmp from %s\n", TEST_DLL_NAME);
    return (p_strnicmp != NULL);
}

START_TEST(_strnicmp)
{
    int result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _strnicmp is not available\n");
        return;
    }
#endif

    StartSeh()
        result = p_strnicmp("a", NULL, 0);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        result = p_strnicmp("a", NULL, 1);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif

    StartSeh()
        result = p_strnicmp(NULL, "a", 0);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        result = p_strnicmp(NULL, "a", 1);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif
}
