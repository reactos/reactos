/*
 * PROJECT:    ReactOS IF Monitor DLL
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    NetSh Helper main functions
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#include "guid.h"
#include "resource.h"

HINSTANCE hDllInstance;

DWORD
WINAPI
InitHelperDll(
    _In_ DWORD dwNetshVersion,
    _Out_ PVOID pReserved)
{
    DWORD dwError;

    dwError = RegisterInterfaceHelper();
    if (dwError == ERROR_SUCCESS)
        dwError = RegisterIpHelper();

    return dwError;
}


BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD dwReason,
    _In_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
