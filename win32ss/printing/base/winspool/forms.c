/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Forms
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddFormA(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    TRACE("AddFormA(%p, %lu, %p)\n", hPrinter, Level, pForm);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddFormW(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    TRACE("AddFormW(%p, %lu, %p)\n", hPrinter, Level, pForm);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeleteFormA(HANDLE hPrinter, PSTR pFormName)
{
    TRACE("DeleteFormA(%p, %s)\n", hPrinter, pFormName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeleteFormW(HANDLE hPrinter, PWSTR pFormName)
{
    TRACE("DeleteFormW(%p, %S)\n", hPrinter, pFormName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumFormsA(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumFormsA(%p, %lu, %p, %lu, %p, %p)\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumFormsW(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumFormsW(%p, %lu, %p, %lu, %p, %p)\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetFormA(HANDLE hPrinter, PSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("GetFormA(%p, %s, %lu, %p, %lu, %p)\n", hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("GetFormW(%p, %S, %lu, %p, %lu, %p)\n", hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetFormA(HANDLE hPrinter, PSTR pFormName, DWORD Level, PBYTE pForm)
{
    TRACE("SetFormA(%p, %s, %lu, %p)\n", hPrinter, pFormName, Level, pForm);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm)
{
    TRACE("SetFormW(%p, %S, %lu, %p)\n", hPrinter, pFormName, Level, pForm);
    UNIMPLEMENTED;
    return FALSE;
}
