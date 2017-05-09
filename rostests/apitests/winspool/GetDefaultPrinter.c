/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for GetDefaultPrinterA/GetDefaultPrinterW/SetDefaultPrinterA/SetDefaultPrinterW
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(GetDefaultPrinter)
{
    DWORD cchDefaultPrinter;
    PWSTR pwszDefaultPrinter;

    // Don't supply any parameters, this has to fail with ERROR_INVALID_PARAMETER.
    SetLastError(0xDEADBEEF);
    ok(!GetDefaultPrinterW(NULL, NULL), "GetDefaultPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetDefaultPrinterW returns error %lu!\n", GetLastError());

    // Determine the size of the required buffer. This has to bail out with ERROR_INSUFFICIENT_BUFFER.
    cchDefaultPrinter = 0;
    SetLastError(0xDEADBEEF);
    ok(!GetDefaultPrinterW(NULL, &cchDefaultPrinter), "GetDefaultPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetDefaultPrinterW returns error %lu!\n", GetLastError());

    // Try with a buffer large enough.
    pwszDefaultPrinter = HeapAlloc(GetProcessHeap(), 0, cchDefaultPrinter * sizeof(WCHAR));
    SetLastError(0xDEADBEEF);
    ok(GetDefaultPrinterW(pwszDefaultPrinter, &cchDefaultPrinter), "GetDefaultPrinterW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "GetDefaultPrinterW returns error %lu!\n", GetLastError());

    // SetDefaultPrinterW with NULL needs to succeed and leave the default printer unchanged.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterW(NULL), "SetDefaultPrinterW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "SetDefaultPrinterW returns error %lu!\n", GetLastError());

    // SetDefaultPrinterW with the previous default printer also needs to succeed.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterW(pwszDefaultPrinter), "SetDefaultPrinterW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "SetDefaultPrinterW returns error %lu!\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, pwszDefaultPrinter);
}
