/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for fpEnumPrinters
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
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

extern BOOL GetLocalsplFuncs(LPPRINTPROVIDOR pp);

START_TEST(fpEnumPrinters)
{
    BYTE TempBuffer[50];
    BYTE ZeroBuffer[50];
    DWORD cbNeeded;
    DWORD cbTemp;
    DWORD dwReturned;
    DWORD i;
    PRINTPROVIDOR pp;
    PPRINTER_INFO_1W pPrinterInfo1;
    PVOID pMem;

    if (!GetLocalsplFuncs(&pp))
        return;

    // Verify that fpEnumPrinters returns success and zeros cbNeeded and dwReturned (but not TempBuffer!) if no flag has been specified.
    memset(TempBuffer, 0xDE, sizeof(TempBuffer));
    memset(ZeroBuffer, 0, sizeof(ZeroBuffer));
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(pp.fpEnumPrinters(0, NULL, 1, TempBuffer, sizeof(TempBuffer), &cbNeeded, &dwReturned), "fpEnumPrinters returns FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(memcmp(TempBuffer, ZeroBuffer, sizeof(TempBuffer)) != 0, "TempBuffer has been zeroed!\n");
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Verify that localspl only returns information about a single print provider (namely itself).
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(!pp.fpEnumPrinters(PRINTER_ENUM_NAME, NULL, 1, NULL, 0, &cbNeeded, &dwReturned), "fpEnumPrinters returns TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    SetLastError(0xDEADBEEF);
    pPrinterInfo1 = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    ok(pp.fpEnumPrinters(PRINTER_ENUM_NAME, NULL, 1, (PBYTE)pPrinterInfo1, cbNeeded, &cbNeeded, &dwReturned), "fpEnumPrinters returns FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 1, "dwReturned is %lu!\n", dwReturned);

    // Verify the actual strings returned.
    ok(wcscmp(pPrinterInfo1->pName, L"Windows NT Local Print Providor") == 0, "pPrinterInfo1->pName is \"%S\"!\n", pPrinterInfo1->pName);
    ok(wcscmp(pPrinterInfo1->pDescription, L"Windows NT Local Printers") == 0, "pPrinterInfo1->pDescription is \"%S\"!\n", pPrinterInfo1->pDescription);
    ok(wcscmp(pPrinterInfo1->pComment, L"Locally connected Printers") == 0, "pPrinterInfo1->pComment is \"%S\"!\n", pPrinterInfo1->pComment);

    // Level 7 is the highest supported for localspl under Windows Server 2003.
    // Higher levels need to fail, but they don't set an error code, just cbNeeded to 0.
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(!pp.fpEnumPrinters(PRINTER_ENUM_LOCAL, NULL, 8, NULL, 0, &cbNeeded, &dwReturned), "fpEnumPrinters returns TRUE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Verify that all valid levels work.
    // In contrast to EnumPrintersW, which only accepts levels 0, 1, 2, 4 and 5, localspl returns information for level 0 to 7.
    for (i = 0; i <= 7; i++)
    {
        // Try with no valid arguments at all.
        // This scenario is usually caugt by RPC, so it just raises an exception here.
        _SEH2_TRY
        {
            dwReturned = 0;
            pp.fpEnumPrinters(PRINTER_ENUM_LOCAL, NULL, i, NULL, 0, NULL, NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwReturned = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        ok(dwReturned == EXCEPTION_ACCESS_VIOLATION, "dwReturned is %lu for Level %lu!\n", dwReturned, i);

        // Now get the required buffer size.
        cbNeeded = 0xDEADBEEF;
        dwReturned = 0xDEADBEEF;
        SetLastError(0xDEADBEEF);
        ok(!pp.fpEnumPrinters(PRINTER_ENUM_LOCAL, NULL, i, NULL, 0, &cbNeeded, &dwReturned), "fpEnumPrinters returns TRUE for Level %lu!\n", i);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "fpEnumPrinters returns error %lu for Level %lu!\n", GetLastError(), i);
        ok(cbNeeded > 0, "cbNeeded is 0 for Level %lu!\n", i);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, i);

        // This test corrupts something inside spoolsv so that it's only runnable once without restarting spoolsv. Therefore it's disabled.
#if 0
        // Now provide the demanded size, but no buffer. This also mustn't touch cbNeeded.
        // This scenario is also caught by RPC and we just have an exception here.
        _SEH2_TRY
        {
            dwReturned = 0;
            pp.fpEnumPrinters(PRINTER_ENUM_LOCAL, NULL, i, NULL, cbNeeded, &cbTemp, &dwReturned);
        }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwReturned = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        ok(dwReturned == EXCEPTION_ACCESS_VIOLATION, "dwReturned is %lu for Level %lu!\n", dwReturned, i);
        ok(cbNeeded == cbTemp, "cbNeeded is %lu, cbTemp is %lu for Level %lu!\n", cbNeeded, cbTemp, i);
#endif

        // Finally use the function as intended and aim for success!
        pMem = DllAllocSplMem(cbNeeded);
        SetLastError(0xDEADBEEF);
        ok(pp.fpEnumPrinters(PRINTER_ENUM_LOCAL, NULL, i, pMem, cbNeeded, &cbTemp, &dwReturned), "fpEnumPrinters returns FALSE for Level %lu!\n", i);

        // This is crazy. For level 3, fpEnumPrinters always returns ERROR_INSUFFICIENT_BUFFER even if we supply a buffer large enough.
        if (i == 3)
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "fpEnumPrinters returns error %lu for Level %lu!\n", GetLastError(), i);
        else
            ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu for Level %lu!\n", GetLastError(), i);

        DllFreeSplMem(pMem);
    }

    // fpEnumPrinters has to succeed independent of the level (valid or not) if we query no information.
    for (i = 0; i < 10; i++)
    {
        SetLastError(0xDEADBEEF);
        ok(pp.fpEnumPrinters(0, NULL, i, NULL, 0, &cbNeeded, &dwReturned), "fpEnumPrinters returns FALSE for Level %lu!\n", i);
        ok(GetLastError() == ERROR_SUCCESS, "fpEnumPrinters returns error %lu for Level %lu!\n", GetLastError(), i);
        ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, i);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, i);
    }
}
