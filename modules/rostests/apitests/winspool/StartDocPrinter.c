/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for StartDocPrinterA/StartDocPrinterW
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(StartDocPrinter)
{
    DWORD dwResult;
    DOCINFOW docInfo = { 0 };

    SetLastError(0xDEADBEEF);
    dwResult = StartDocPrinterW(NULL, 0, NULL);
    ok(dwResult == 0, "StartDocPrinterW returns %lu!\n", dwResult);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "StartDocPrinter returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    dwResult = StartDocPrinterW(NULL, 1, NULL);
    ok(dwResult == 0, "StartDocPrinterW returns %lu!\n", dwResult);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "StartDocPrinter returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    dwResult = StartDocPrinterW(NULL, 0, (LPBYTE)&docInfo);
    ok(dwResult == 0, "StartDocPrinterW returns %lu!\n", dwResult);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "StartDocPrinter returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    dwResult = StartDocPrinterW(NULL, 1, (LPBYTE)&docInfo);
    ok(dwResult == 0, "StartDocPrinterW returns %lu!\n", dwResult);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "StartDocPrinter returns error %lu!\n", GetLastError());

    /// ERROR_INVALID_LEVEL with correct handle but invalid Level
}
