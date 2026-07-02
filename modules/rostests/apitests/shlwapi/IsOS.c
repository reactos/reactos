/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for IsOS
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <versionhelpers.h>

typedef struct
{
    DWORD Value;
    const char *Name;
} ISOS_ENTRY;

typedef BOOL (WINAPI *PFNISOS)(DWORD);

START_TEST(IsOS)
{
    HMODULE hShlwapi;
    PFNISOS pIsOS;

    const ISOS_ENTRY FALSE_OS_LEVELS[] = 
    { 
        { 0x00,                         "OS_WIN32SORGREATER" },
        { OS_WIN95ORGREATER,            "OS_WIN95ORGREATER" },
        { OS_WIN98ORGREATER,            "OS_WIN98ORGREATER" },
        { OS_MEORGREATER,               "OS_MEORGREATER" },
        { OS_WIN95_GOLD,                "OS_WIN95_GOLD" },
        { OS_WIN98_GOLD,                "OS_WIN98_GOLD" },
    };

    const ISOS_ENTRY TRUE_OS_LEVELS[] = 
    {
        { OS_NT,                        "OS_NT" },
        { OS_NT4ORGREATER,              "OS_NT4ORGREATER" },
        { OS_WIN2000ORGREATER,          "OS_WIN2000ORGREATER" },
        { OS_XPORGREATER,               "OS_XPORGREATER" },
        { OS_FASTUSERSWITCHING,         "OS_FASTUSERSWITCHING" },
    };

    hShlwapi = LoadLibraryA("shlwapi.dll");
    if (!hShlwapi)
    {
        skip(FALSE, "shlwapi.dll is not available\n");
        return;
    }

    pIsOS = (PFNISOS)GetProcAddress(hShlwapi, "IsOS");
    if (!pIsOS && !IsWindowsVistaOrGreater())
    {
        pIsOS = (PFNISOS)GetProcAddress(hShlwapi, (LPCSTR)437);
    }
    if (!pIsOS)
    {
        skip(FALSE, "IsOS is not available\n");
        FreeLibrary(hShlwapi);
        return;
    }

    for (size_t i = 0; i < _countof(FALSE_OS_LEVELS); i++)
    {
        ok(pIsOS(FALSE_OS_LEVELS[i].Value) == FALSE, "Expected IsOS(%s) to return FALSE, got TRUE\n", FALSE_OS_LEVELS[i].Name);
    }

    for (size_t i = 0; i < _countof(TRUE_OS_LEVELS); i++)
    {
        ok(pIsOS(TRUE_OS_LEVELS[i].Value) == TRUE, "Expected IsOS(%s) to return TRUE, got FALSE\n", TRUE_OS_LEVELS[i].Name);
    }
}
