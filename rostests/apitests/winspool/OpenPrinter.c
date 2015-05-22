/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for OpenPrinterA/OpenPrinterW
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
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

    SetLastError(0xDEADBEEF);
    ok(!OpenPrinterW(NULL, NULL, NULL), "OpenPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "OpenPrinterW returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(!OpenPrinterW(NULL, &hPrinter, NULL), "OpenPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "OpenPrinterW returns error %lu!\n", GetLastError());
}
