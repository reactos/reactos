/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

LONG WINAPI
AdvancedDocumentPropertiesW(HWND hWnd, HANDLE hPrinter, PWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD WINAPI
GetPrinterDataW(HANDLE hPrinter, PWSTR pValueName, PDWORD pType, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD WINAPI
SetPrinterDataW(HANDLE hPrinter, PWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    UNIMPLEMENTED;
    return FALSE;
}
