/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _strupr
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef char* (__cdecl *PFN_strupr)(char* _String);
static PFN_strupr p_strupr;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_strupr = (PFN_strupr)GetProcAddress(hdll, "_strupr");
    ok(p_strupr != NULL, "Failed to load _strupr from %s\n", TEST_DLL_NAME);
    return (p_strupr != NULL);
}

START_TEST(_strupr)
{
    char *result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _strupr is not available\n");
        return;
    }
#endif

    // The NT version of _strupr has a NULL check (as opposed to _strlwr)
    StartSeh()
        result = p_strupr(NULL);
        ok_ptr(result, NULL);
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
}
