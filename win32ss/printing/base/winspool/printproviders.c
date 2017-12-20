/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Providers
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddPrintProvidorA(PSTR pName, DWORD Level, PBYTE pProviderInfo)
{
    TRACE("AddPrintProvidorA(%s, %lu, %p)\n", pName, Level, pProviderInfo);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPrintProvidorW(PWSTR pName, DWORD Level, PBYTE pProviderInfo)
{
    TRACE("AddPrintProvidorW(%S, %lu, %p)\n", pName, Level, pProviderInfo);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProvidorA(PSTR pName, PSTR pEnvironment, PSTR pPrintProviderName)
{
    TRACE("DeletePrintProvidorW(%s, %s, %s)\n", pName, pEnvironment, pPrintProviderName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProvidorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProviderName)
{
    TRACE("DeletePrintProvidorW(%S, %S, %S)\n", pName, pEnvironment, pPrintProviderName);
    UNIMPLEMENTED;
    return FALSE;
}
