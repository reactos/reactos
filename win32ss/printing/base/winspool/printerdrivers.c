/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Drivers
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddPrinterDriverA(PSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    TRACE("AddPrinterDriverA(%s, %lu, %p)\n", pName, Level, pDriverInfo);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPrinterDriverExA(PSTR pName, DWORD Level, PBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    TRACE("AddPrinterDriverExA(%s, %lu, %p, %lu)\n", pName, Level, pDriverInfo, dwFileCopyFlags);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPrinterDriverExW(PWSTR pName, DWORD Level, PBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    TRACE("AddPrinterDriverExW(%S, %lu, %p, %lu)\n", pName, Level, pDriverInfo, dwFileCopyFlags);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPrinterDriverW(PWSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    TRACE("AddPrinterDriverW(%S, %lu, %p)\n", pName, Level, pDriverInfo);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrinterDriverA(PSTR pName, PSTR pEnvironment, PSTR pDriverName)
{
    TRACE("DeletePrinterDriverA(%s, %s, %s)\n", pName, pEnvironment, pDriverName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrinterDriverExA(PSTR pName, PSTR pEnvironment, PSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    TRACE("DeletePrinterDriverExA(%s, %s, %s, %lu, %lu)\n", pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrinterDriverExW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    TRACE("DeletePrinterDriverExW(%S, %S, %S, %lu, %lu)\n", pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrinterDriverW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName)
{
    TRACE("DeletePrinterDriverW(%S, %S, %S)\n", pName, pEnvironment, pDriverName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrinterDriversA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumPrinterDriversA(%s, %s, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrinterDriversW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumPrinterDriversW(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverDirectoryA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("GetPrinterDriverDirectoryA(%s, %s, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    TRACE("GetPrinterDriverDirectoryW(%S, %S, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);
    UNIMPLEMENTED;
    return FALSE;
}
