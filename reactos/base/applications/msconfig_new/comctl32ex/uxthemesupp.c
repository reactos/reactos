/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/uxthemesupp.c
 * PURPOSE:     UX Theming helpers.
 * COPYRIGHT:   Copyright 2015 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "uxthemesupp.h"

static HMODULE hUxTheme = NULL;

static BOOL
InitUxTheme(VOID)
{
    static BOOL Initialized = FALSE;

    if (Initialized) return TRUE;

    hUxTheme = LoadLibraryW(L"uxtheme.dll");
    if (hUxTheme == NULL) return FALSE;

    Initialized = TRUE;
    return TRUE;
}

#if 0
static VOID
CleanupUxTheme(VOID)
{
    FreeLibrary(hUxTheme);
    hUxTheme = NULL;
    // Initialized = FALSE;
}
#endif


////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// http://www.catch22.net/software/winspy
// Copyright (c) 2002 by J Brown
//

typedef HRESULT (WINAPI* ETDTProc)(HWND, DWORD);

HRESULT
WINAPI
EnableThemeDialogTexture(_In_ HWND  hwnd,
                         _In_ DWORD dwFlags)
{
    ETDTProc fnEnableThemeDialogTexture;

    if (!InitUxTheme())
        return HRESULT_FROM_WIN32(GetLastError());

    fnEnableThemeDialogTexture =
        (ETDTProc)GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
    if (!fnEnableThemeDialogTexture)
        return HRESULT_FROM_WIN32(GetLastError());

    return fnEnableThemeDialogTexture(hwnd, dwFlags);
}


typedef HRESULT (WINAPI* SWTProc)(HWND, LPCWSTR, LPCWSTR);

HRESULT
WINAPI
SetWindowTheme(_In_ HWND    hwnd,
               _In_ LPCWSTR pszSubAppName,
               _In_ LPCWSTR pszSubIdList)
{
    SWTProc fnSetWindowTheme;

    if (!InitUxTheme())
        return HRESULT_FROM_WIN32(GetLastError());

    fnSetWindowTheme =
        (SWTProc)GetProcAddress(hUxTheme, "SetWindowTheme");
    if (!fnSetWindowTheme)
        return HRESULT_FROM_WIN32(GetLastError());

    return fnSetWindowTheme(hwnd, pszSubAppName, pszSubIdList);
}
