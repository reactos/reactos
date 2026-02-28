/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _strlwr
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef char* (__cdecl *PFN_strlwr)(char* _String);
static PFN_strlwr p_strlwr;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_strlwr = (PFN_strlwr)GetProcAddress(hdll, "_strlwr");
    ok(p_strlwr != NULL, "Failed to load _strlwr from %s\n", TEST_DLL_NAME);
    return (p_strlwr != NULL);
}

START_TEST(_strlwr)
{
    char *result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _strlwr is not available\n");
        return;
    }
#endif

    // The NT version of _strlwr does not have a NULL check
    StartSeh()
        result = p_strlwr(NULL);
        ok_ptr(result, NULL);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif
}
