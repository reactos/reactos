/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for EnumPrintersA/EnumPrintersW
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(EnumPrinters)
{
    BYTE TempBuffer[50];
    BYTE ZeroBuffer[50] = { 0 };
    DWORD cbNeeded;
    DWORD cbTemp;
    DWORD cchComputerName;
    DWORD dwReturned;
    PPRINTER_INFO_1W pPrinterInfo1;
    PVOID pMem;
    DWORD Level;
    WCHAR wszComputerName[2 + MAX_COMPUTERNAME_LENGTH + 2 + 1];

    wszComputerName[0] = L'\\';
    wszComputerName[1] = L'\\';
    cchComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(&wszComputerName[2], &cchComputerName))
    {
        skip("GetComputerNameW failed with error %lu!\n", GetLastError());
        return;
    }

    cchComputerName += 2;

    // Verify that EnumPrintersW returns success and zeroes all input variables even though no flag has been specified.
    memset(TempBuffer, 0xDE, sizeof(TempBuffer));
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(EnumPrintersW(0, NULL, 1, TempBuffer, sizeof(TempBuffer), &cbNeeded, &dwReturned), "EnumPrintersW returns FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(memcmp(TempBuffer, ZeroBuffer, sizeof(TempBuffer)) == 0, "TempBuffer has not been zeroed!\n");
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Level 5 is the highest supported under Windows Server 2003. Higher levels need to fail and leave the variables untouched!
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintersW(0, NULL, 6, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0xDEADBEEF, "cbNeeded is %lu!\n", cbNeeded);
    ok(dwReturned == 0xDEADBEEF, "dwReturned is %lu!\n", dwReturned);

    // Same goes for level 3.
    cbNeeded = 0xDEADBEEF;
    dwReturned = 0xDEADBEEF;
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintersW(0, NULL, 3, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0xDEADBEEF, "cbNeeded is %lu!\n", cbNeeded);
    ok(dwReturned == 0xDEADBEEF, "dwReturned is %lu!\n", dwReturned);

    // Try for all levels. Level 0 is valid here and returns the PRINTER_INFO_STRESS structure (documented in MS-RPRN).
    for (Level = 0; Level <= 5; Level++)
    {
        if (Level == 3)
            continue;

        // Try with no valid arguments at all.
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(0, NULL, Level, NULL, 0, NULL, NULL), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
        ok(GetLastError() == RPC_X_NULL_REF_POINTER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);

        // It has to succeed if we supply the required pointers and query no information.
        SetLastError(0xDEADBEEF);
        ok(EnumPrintersW(0, NULL, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", Level);
        ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
        ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

        // This constant is from Windows 9x/ME times and mustn't work anymore.
        SetLastError(0xDEADBEEF);
        ok(EnumPrintersW(PRINTER_ENUM_DEFAULT, NULL, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", Level);
        ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
        ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

        // Now things get interesting. Let's query the buffer size for information about the local printers.
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

        // There need to be installed local printers for the next steps.
        if (cbNeeded > 0)
        {
            // Same error has to occur with no buffer, but a size < 4 (AlignRpcPtr comes into play here).
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, Level, NULL, 1, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded > 0, "cbNeeded is 0 for Level %lu!\n", Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // Now provide the demanded size, but no buffer.
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, Level, NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbTemp == 0, "cbTemp is %lu for Level %lu!\n", cbTemp, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // Finally use the function as intended and aim for success!
            // After that, cbTemp contains the needed buffer size without the computer name prepended.
            pMem = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
            SetLastError(0xDEADBEEF);
            ok(EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, Level, pMem, cbNeeded, &cbTemp, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            HeapFree(GetProcessHeap(), 0, pMem);

            if (Level != 4)
            {
                // Show that the Name parameter is checked when PRINTER_ENUM_NAME is also specified.
                SetLastError(0xDEADBEEF);
                ok(!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, L"LOREM IPSUM", Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
                ok(GetLastError() != ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
                ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
                ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);
            }

            // Show that the structure is returned with its known size again when PRINTER_ENUM_NAME is specified and Name
            // is the (case-insensitively compared) name of the Local Print Provider.
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, L"wInDoWs NT Local Print Providor", Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded == cbTemp, "cbNeeded is %lu, reference size is %lu for Level %lu!\n", cbNeeded, cbTemp, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // Now we specify the correct "\\COMPUTERNAME" for Name.
            // The returned structure should have some strings prepended with the Computer Name and thus require a larger buffer.
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, wszComputerName, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded > cbTemp, "cbNeeded is %lu, reference size is %lu for Level %lu!\n", cbNeeded, cbTemp, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            if (Level != 4)
            {
                // This won't work when there is a trailing backslash (i.e. "\\COMPUTERNAME\").
                wszComputerName[cchComputerName++] = L'\\';
                wszComputerName[cchComputerName] = 0;
                SetLastError(0xDEADBEEF);
                ok(!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, wszComputerName, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
                ok(GetLastError() == ERROR_INVALID_NAME, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
                ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
                ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);
                wszComputerName[--cchComputerName] = 0;
            }

            // Now it gets funky. There are also cases where EnumPrintersW takes the Name parameter into account,
            // although PRINTER_ENUM_NAME is not given.
            // A bogus Name without two backslashes is ignored.
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, L"LOREM IPSUM", Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded == cbTemp, "cbNeeded is %lu, reference size is %lu for Level %lu!\n", cbNeeded, cbTemp, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // Specifying "\\COMPUTERNAME" again prepends it to some strings.
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, wszComputerName, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded > cbTemp, "cbNeeded is %lu, reference size is %lu for Level %lu!\n", cbNeeded, cbTemp, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // Specifying "\\COMPUTERNAME\" also verifies the Computer Name, but doesn't prepend it.
            // This logic is crazy, and different to PRINTER_ENUM_NAME...
            wszComputerName[cchComputerName++] = L'\\';
            wszComputerName[cchComputerName] = 0;
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, wszComputerName, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded == cbTemp, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);

            // I can even put an additional bogus character after the trailing backslash, doesn't change anything.
            wszComputerName[cchComputerName++] = L'a';
            wszComputerName[cchComputerName] = 0;
            SetLastError(0xDEADBEEF);
            ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, wszComputerName, Level, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", Level);
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), Level);
            ok(cbNeeded == cbTemp, "cbNeeded is %lu for Level %lu!\n", cbNeeded, Level);
            ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, Level);
            cchComputerName -= 2;
            wszComputerName[cchComputerName] = 0;
        }
        else
        {
            skip("cbNeeded is 0 on Level %lu, skipping additional tests!\n", Level);
        }
    }

    // Using EnumPrintersW with PRINTER_ENUM_NAME, Level 1 and no Name must return information about the Print Providers.
    // First record must always be the Local Print Provider.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, NULL, 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    SetLastError(0xDEADBEEF);
    pPrinterInfo1 = (PPRINTER_INFO_1W)HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    ok(EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_NAME, NULL, 1, (PBYTE)pPrinterInfo1, cbNeeded, &cbTemp, &dwReturned), "EnumPrintersW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbTemp == cbNeeded, "cbTemp is %lu, cbNeeded is %lu!\n", cbTemp, cbNeeded);
    ok(dwReturned > 0, "dwReturned is %lu!\n", dwReturned);
    ok(!wcscmp(pPrinterInfo1->pName, L"Windows NT Local Print Providor"), "pPrinterInfo1->pName is %S!\n", pPrinterInfo1->pName);
    ok(!wcscmp(pPrinterInfo1->pComment, L"Locally connected Printers"), "pPrinterInfo1->pComment is %S!\n", pPrinterInfo1->pComment);
    ok(!wcscmp(pPrinterInfo1->pDescription, L"Windows NT Local Printers"), "pPrinterInfo1->pDescription is %S!\n", pPrinterInfo1->pDescription);
    HeapFree(GetProcessHeap(), 0, pPrinterInfo1);
}
