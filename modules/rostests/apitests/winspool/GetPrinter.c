/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetPrinterA/GetPrinterW
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

/* From printing/include/spoolss.h */
#define MAX_PRINTER_NAME        220

START_TEST(GetPrinter)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    DWORD cchDefaultPrinter;
    DWORD Level;
    HANDLE hPrinter;
    PVOID pMem;
    WCHAR wszDefaultPrinter[MAX_PRINTER_NAME + 1];

    // Open a handle to the default printer.
    cchDefaultPrinter = _countof(wszDefaultPrinter);
    ok(GetDefaultPrinterW(wszDefaultPrinter, &cchDefaultPrinter), "GetDefaultPrinterW returns FALSE and requires %lu characters!\n", cchDefaultPrinter);
    if (!OpenPrinterW(wszDefaultPrinter, &hPrinter, NULL))
    {
        skip("Could not retrieve a handle to the default printer!\n");
        return;
    }

    // Try for all levels. Level 0 is valid here and returns the PRINTER_INFO_STRESS structure (documented in MS-RPRN).
    for (Level = 0; Level <= 9; Level++)
    {
        // Try with no valid arguments at all.
        SetLastError(0xDEADBEEF);
        ok(!GetPrinterW(NULL, Level, NULL, 0, NULL), "GetPrinterW returns TRUE for Level %lu!\n", Level);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "GetPrinterW returns error %lu for Level %lu!\n", GetLastError(), Level);

        // Now supply at least a handle.
        SetLastError(0xDEADBEEF);
        ok(!GetPrinterW(hPrinter, Level, NULL, 0, NULL), "GetPrinterW returns TRUE for Level %lu!\n", Level);
        ok(GetLastError() == RPC_X_NULL_REF_POINTER, "GetPrinterW returns error %lu for Level %lu!\n", GetLastError(), Level);

        // Now get the buffer size.
        SetLastError(0xDEADBEEF);
        ok(!GetPrinterW(hPrinter, Level, NULL, 0, &cbNeeded), "GetPrinterW returns TRUE for Level %lu!\n", Level);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetPrinterW returns error %lu for Level %lu!\n", GetLastError(), Level);

        // Finally use the function as intended and aim for success!
        pMem = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
        SetLastError(0xDEADBEEF);
        ok(GetPrinterW(hPrinter, Level, pMem, cbNeeded, &cbTemp), "GetPrinterW returns FALSE for Level %lu!\n", Level);
        ok(cbNeeded == cbTemp, "cbNeeded is %lu, reference size is %lu for Level %lu!\n", cbNeeded, cbTemp, Level);
        HeapFree(GetProcessHeap(), 0, pMem);
    }

    ClosePrinter(hPrinter);
}
