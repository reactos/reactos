/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Display settings related shims
 * COPYRIGHT:   Copyright 2024 William Kent (wjk011@gmail.com)
 */

#include "string.h"
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>

#define SHIM_NS             AeroThemeName
#include <setup_shim.inl>

HRESULT WINAPI SHIM_OBJ_NAME(GetCurrentThemeNameRedirect)(
    LPWSTR pszThemeName, int cchThemeName,
    LPWSTR pszColor, int cchColor,
    LPWSTR pszSize, int cchSize)
{
    StringCbCopyW(pszThemeName, cchThemeName, L"Aero");
    if (pszColor != NULL) StringCbCopyW(pszColor, cchColor, L"NormalColor");
    if (pszSize != NULL) StringCbCopyW(pszSize, cchSize, L"Normal");

    return S_OK;
}

#define SHIM_NUM_HOOKS 1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "UXTHEME.DLL", "GetCurrentThemeName", SHIM_OBJ_NAME(GetCurrentThemeNameRedirect))

#include <implement_shim.inl>
