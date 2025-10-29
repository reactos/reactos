/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _wcsupr
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef wchar_t* (__cdecl *PFN_wcsupr)(wchar_t* _String);
static PFN_wcsupr p_wcsupr;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_wcsupr = (PFN_wcsupr)GetProcAddress(hdll, "_wcsupr");
    ok(p_wcsupr != NULL, "Failed to load _wcsupr from %s\n", TEST_DLL_NAME);
    return (p_wcsupr != NULL);
}

START_TEST(_wcsupr)
{
    wchar_t *result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _wcsupr is not available\n");
        return;
    }
#endif

    // The NT version of _wcsupr does not have a NULL check
    StartSeh()
        result = p_wcsupr(NULL);
        ok_ptr(result, NULL);
#ifdef TEST_NTDLL
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
#endif
}
