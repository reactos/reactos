/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _stricmp
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef int (__cdecl *PFN_stricmp)(const char* _String1, const char* _String2);
static PFN_stricmp p_stricmp;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_stricmp = (PFN_stricmp)GetProcAddress(hdll, "_stricmp");
    ok(p_stricmp != NULL, "Failed to load _stricmp from %s\n", TEST_DLL_NAME);
    return (p_stricmp != NULL);
}

START_TEST(_stricmp)
{
    int result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _stricmp is not available\n");
        return;
    }
#endif

    StartSeh()
        result = p_stricmp("a", NULL);
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif

    StartSeh()
        result = p_stricmp(NULL, "a");
        ok_int(result, MAXLONG);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif
}
