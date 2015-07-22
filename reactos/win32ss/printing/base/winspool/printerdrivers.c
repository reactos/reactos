/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Drivers
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
AddPrinterDriverW(PWSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrinterDriverW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrinterDriversW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    UNIMPLEMENTED;
    return FALSE;
}
