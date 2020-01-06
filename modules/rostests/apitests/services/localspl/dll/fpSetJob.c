/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for fpSetJob
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>

#include "../localspl_apitest.h"
#include <spoolss.h>

extern PWSTR GetDefaultPrinterFromRegistry(VOID);
extern BOOL GetLocalsplFuncs(LPPRINTPROVIDOR pp);

/* From printing/include/spoolss.h */
#define MAX_PRINTER_NAME        220

START_TEST(fpSetJob)
{
    HANDLE hPrinter = NULL;
    PRINTPROVIDOR pp;
    PWSTR pwszDefaultPrinter = NULL;

    if (!GetLocalsplFuncs(&pp))
        goto Cleanup;

    // Verify that fpSetJob returns ERROR_INVALID_HANDLE when nothing is provided.
    ok(!pp.fpSetJob(NULL, 0, 0, NULL, 0), "fpSetJob returns TRUE\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "fpSetJob returns error %lu!\n", GetLastError());

    // Get the default printer.
    pwszDefaultPrinter = GetDefaultPrinterFromRegistry();
    if (!pwszDefaultPrinter)
    {
        skip("Could not determine the default printer!\n");
        goto Cleanup;
    }

    if (!pp.fpOpenPrinter(pwszDefaultPrinter, &hPrinter, NULL))
    {
        skip("Could not open a handle to the default printer, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Verify that fpSetJob returns ERROR_INVALID_PARAMETER if only a printer handle is provided.
    ok(!pp.fpSetJob(hPrinter, 0, 0, NULL, 0), "fpSetJob returns TRUE\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "fpSetJob returns error %lu!\n", GetLastError());

Cleanup:
    if (pwszDefaultPrinter)
        HeapFree(GetProcessHeap(), 0, pwszDefaultPrinter);

    if (hPrinter)
        pp.fpClosePrinter(hPrinter);
}
