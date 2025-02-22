/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for OpenPrinterA/OpenPrinterW
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(OpenPrinter)
{
    HANDLE hPrinter;

    // Give no handle at all, this has to fail
    SetLastError(0xDEADBEEF);
    ok(!OpenPrinterW(NULL, NULL, NULL), "OpenPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "OpenPrinterW returns error %lu!\n", GetLastError());

    // Open a handle to the local print server
    SetLastError(0xDEADBEEF);
    ok(OpenPrinterW(NULL, &hPrinter, NULL), "OpenPrinterW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "OpenPrinterW returns error %lu!\n", GetLastError());
    ClosePrinter(hPrinter);
}
