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

typedef HRESULT (WINAPI* ETDTProc)(HWND, DWORD);
static ETDTProc fnEnableThemeDialogTexture = NULL;

typedef HRESULT (WINAPI* SWTProc)(HWND, LPCWSTR, LPCWSTR);
static SWTProc fnSetWindowTheme = NULL;


static BOOL
InitUxTheme(VOID)
{
    if (hUxTheme) return TRUE;

    hUxTheme = LoadLibraryW(L"uxtheme.dll");
    if (hUxTheme == NULL) return FALSE;

    fnEnableThemeDialogTexture =
        (ETDTProc)GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
    fnSetWindowTheme =
        (SWTProc)GetProcAddress(hUxTheme, "SetWindowTheme");

    return TRUE;
}

#if 0
static VOID
CleanupUxTheme(VOID)
{
    FreeLibrary(hUxTheme);
    hUxTheme = NULL;
}
#endif


////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// http://www.catch22.net/software/winspy
// Copyright (c) 2002 by J Brown
//

HRESULT
WINAPI
EnableThemeDialogTexture(_In_ HWND  hwnd,
                         _In_ DWORD dwFlags)
{
    if (!InitUxTheme())
        return HRESULT_FROM_WIN32(GetLastError());

    if (!fnEnableThemeDialogTexture)
        return HRESULT_FROM_WIN32(GetLastError());

    return fnEnableThemeDialogTexture(hwnd, dwFlags);
}

HRESULT
WINAPI
SetWindowTheme(_In_ HWND    hwnd,
               _In_ LPCWSTR pszSubAppName,
               _In_ LPCWSTR pszSubIdList)
{
    if (!InitUxTheme())
        return HRESULT_FROM_WIN32(GetLastError());

    if (!fnSetWindowTheme)
        return HRESULT_FROM_WIN32(GetLastError());

    return fnSetWindowTheme(hwnd, pszSubAppName, pszSubIdList);
}
