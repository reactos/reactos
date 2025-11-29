/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:         Tests for _wcslwr
 * COPYRIGHT:       Copyright 2025 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <pseh/pseh2.h>
#include <ndk/umtypes.h>

typedef wchar_t* (__cdecl *PFN_wcslwr)(wchar_t* _String);
static PFN_wcslwr p_wcslwr;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_wcslwr = (PFN_wcslwr)GetProcAddress(hdll, "_wcslwr");
    ok(p_wcslwr != NULL, "Failed to load _wcslwr from %s\n", TEST_DLL_NAME);
    return (p_wcslwr != NULL);
}

START_TEST(_wcslwr)
{
    wchar_t *result;

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _wcslwr is not available\n");
        return;
    }
#endif

    // The NT version of _wcslwr has a NULL check (as opposed to _wcsupr)
    StartSeh()
        result = p_wcslwr(NULL);
        ok_ptr(result, NULL);
    EndSeh((is_reactos() || GetNTVersion() >= _WIN32_WINNT_VISTA) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
}
