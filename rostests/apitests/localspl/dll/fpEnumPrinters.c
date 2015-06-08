/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for fpEnumPrinters
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
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

START_TEST(fpEnumPrinters)
{
    DWORD cbNeeded;
    DWORD dwReturned;
    HMODULE hLocalspl;
    PInitializePrintProvidor pfnInitializePrintProvidor;
    PRINTPROVIDOR pp;
    PPRINTER_INFO_1W pPrinterInfo1;

    // Get us a handle to the loaded localspl.dll.
    hLocalspl = GetModuleHandleW(L"localspl");
    if (!hLocalspl)
    {
        skip("GetModuleHandleW failed with error %lu!\n", GetLastError());
        return;
    }

    // Get a pointer to its InitializePrintProvidor function.
    pfnInitializePrintProvidor = (PInitializePrintProvidor)GetProcAddress(hLocalspl, "InitializePrintProvidor");
    if (!pfnInitializePrintProvidor)
    {
        skip("GetProcAddress failed with error %lu!\n", GetLastError());
        return;
    }

    // Get localspl's function pointers.
    if (!pfnInitializePrintProvidor(&pp, sizeof(pp), NULL))
    {
        skip("pfnInitializePrintProvidor failed with error %lu!\n", GetLastError());
        return;
    }

    // Verify that localspl only returns information about a single print provider (namely itself).
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(!pp.fpEnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, NULL, 1, NULL, 0, &cbNeeded, &dwReturned), "fpEnumPrinters returns TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    SetLastError(0xDEADBEEF);
    pPrinterInfo1 = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    ok(pp.fpEnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, NULL, 1, (PBYTE)pPrinterInfo1, cbNeeded, &cbNeeded, &dwReturned), "fpEnumPrinters returns FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 1, "dwReturned is %lu!\n", dwReturned);

    // Verify the actual strings returned.
    ok(wcscmp(pPrinterInfo1->pName, L"Windows NT Local Print Providor") == 0, "pPrinterInfo1->pName is \"%S\"!\n", pPrinterInfo1->pName);
    ok(wcscmp(pPrinterInfo1->pDescription, L"Windows NT Local Printers") == 0, "pPrinterInfo1->pDescription is \"%S\"!\n", pPrinterInfo1->pDescription);
    ok(wcscmp(pPrinterInfo1->pComment, L"Locally connected Printers") == 0, "pPrinterInfo1->pComment is \"%S\"!\n", pPrinterInfo1->pComment);
}
