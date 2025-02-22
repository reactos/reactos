/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetDefaultPrinterA/GetDefaultPrinterW/SetDefaultPrinterA/SetDefaultPrinterW
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(GetDefaultPrinterA)
{
    DWORD cchDefaultPrinter;
    PSTR pszDefaultPrinter;

    // Don't supply any parameters, this has to fail with ERROR_INVALID_PARAMETER.
    SetLastError(0xDEADBEEF);
    ok(!GetDefaultPrinterA(NULL, NULL), "GetDefaultPrinterA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetDefaultPrinterA returns error %lu!\n", GetLastError());

    // Determine the size of the required buffer. This has to bail out with ERROR_INSUFFICIENT_BUFFER.
    cchDefaultPrinter = 0;
    SetLastError(0xDEADBEEF);
    ok(!GetDefaultPrinterA(NULL, &cchDefaultPrinter), "GetDefaultPrinterA returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetDefaultPrinterA returns error %lu!\n", GetLastError());

    // Try with a buffer large enough.
    pszDefaultPrinter = HeapAlloc(GetProcessHeap(), 0, cchDefaultPrinter);
    SetLastError(0xDEADBEEF);
    ok(GetDefaultPrinterA(pszDefaultPrinter, &cchDefaultPrinter), "GetDefaultPrinterA returns FALSE!\n");

    // SetDefaultPrinterA with NULL needs to succeed and leave the default printer unchanged.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterA(NULL), "SetDefaultPrinterA returns FALSE!\n");

    // SetDefaultPrinterA with the previous default printer also needs to succeed.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterA(pszDefaultPrinter), "SetDefaultPrinterA returns FALSE!\n");

    // SetDefaultPrinterA with an invalid printer name needs to fail with ERROR_INVALID_PRINTER_NAME.
    SetLastError(0xDEADBEEF);
    ok(!SetDefaultPrinterA("INVALID PRINTER NAME!!!"), "SetDefaultPrinterA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PRINTER_NAME, "SetDefaultPrinterA returns error %lu!\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, pszDefaultPrinter);
}

START_TEST(GetDefaultPrinterW)
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

    // SetDefaultPrinterW with NULL needs to succeed and leave the default printer unchanged.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterW(NULL), "SetDefaultPrinterW returns FALSE!\n");

    // SetDefaultPrinterW with the previous default printer also needs to succeed.
    SetLastError(0xDEADBEEF);
    ok(SetDefaultPrinterW(pwszDefaultPrinter), "SetDefaultPrinterW returns FALSE!\n");

    // SetDefaultPrinterW with an invalid printer name needs to fail with ERROR_INVALID_PRINTER_NAME.
    SetLastError(0xDEADBEEF);
    ok(!SetDefaultPrinterW(L"INVALID PRINTER NAME!!!"), "SetDefaultPrinterW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PRINTER_NAME, "SetDefaultPrinterW returns error %lu!\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, pwszDefaultPrinter);
}
