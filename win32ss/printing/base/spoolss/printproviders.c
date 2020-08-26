/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for managing print Providers
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

//
// These do not forward!!!
//

BOOL WINAPI
AddPrintProvidorW(PWSTR pName, DWORD Level, PBYTE pProviderInfo)
{
    FIXME("AddPrintProvidorW(%S, %lu, %p)\n", pName, Level, pProviderInfo);
    return FALSE;
}

BOOL WINAPI
DeletePrintProvidorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProviderName)
{
    FIXME("DeletePrintProvidorW(%s, %s, %s)\n", pName, pEnvironment, pPrintProviderName);
    return FALSE;
}
