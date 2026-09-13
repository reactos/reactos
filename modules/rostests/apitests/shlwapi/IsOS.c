/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for IsOS
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <versionhelpers.h>

#ifndef OS_WIN32SORGREATER
#define OS_WIN32SORGREATER 0x00
#endif

typedef struct
{
    DWORD Value;
    PCSTR Name;
} ISOS_ENTRY;

typedef BOOL (WINAPI *PFNISOS)(DWORD);

START_TEST(IsOS)
{
    HMODULE hShlwapi;
    PFNISOS pIsOS;
    size_t i;

#define ISOS_ENTRY_LEVEL(level) { level, #level }
    const ISOS_ENTRY FALSE_OS_Levels[] =
    {
        ISOS_ENTRY_LEVEL(OS_WIN32SORGREATER),
        ISOS_ENTRY_LEVEL(OS_WIN95ORGREATER),
        ISOS_ENTRY_LEVEL(OS_WIN98ORGREATER),
        ISOS_ENTRY_LEVEL(OS_MEORGREATER),
        ISOS_ENTRY_LEVEL(OS_WIN95_GOLD),
        ISOS_ENTRY_LEVEL(OS_WIN98_GOLD),
    };

    const ISOS_ENTRY TRUE_OS_Levels[] =
    {
        ISOS_ENTRY_LEVEL(OS_NT),
        ISOS_ENTRY_LEVEL(OS_NT4ORGREATER),
        ISOS_ENTRY_LEVEL(OS_WIN2000ORGREATER),
        ISOS_ENTRY_LEVEL(OS_XPORGREATER),
        ISOS_ENTRY_LEVEL(OS_FASTUSERSWITCHING),
    };
#undef ISOS_ENTRY_LEVEL

    hShlwapi = GetModuleHandleW(L"shlwapi.dll");
    if (!hShlwapi)
    {
        skip(FALSE, "shlwapi.dll is not available\n");
        return;
    }

    pIsOS = (PFNISOS)GetProcAddress(hShlwapi, "IsOS");
    if (!pIsOS && !IsWindowsVistaOrGreater())
        pIsOS = (PFNISOS)GetProcAddress(hShlwapi, (LPCSTR)437);
    if (!pIsOS)
    {
        skip(FALSE, "IsOS is not available\n");
        return;
    }

    for (i = 0; i < _countof(FALSE_OS_Levels); i++)
    {
        ok(pIsOS(FALSE_OS_Levels[i].Value) == FALSE, "Expected IsOS(%s) to return FALSE, got TRUE\n", FALSE_OS_Levels[i].Name);
    }

    for (i = 0; i < _countof(TRUE_OS_Levels); i++)
    {
        ok(pIsOS(TRUE_OS_Levels[i].Value) == TRUE, "Expected IsOS(%s) to return TRUE, got FALSE\n", TRUE_OS_Levels[i].Name);
    }
}
