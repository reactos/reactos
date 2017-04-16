/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for EnumPrintersA/EnumPrintersW
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
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
    DWORD dwReturned;
    PVOID pMem;
    DWORD i;
    DWORD dwValidLevels[] = { 0, 1, 2, 4, 5 };

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

    // Try for all valid levels. Level 0 is valid here and returns the PRINTER_INFO_STRESS structure (documented in MS-RPRN).
    for (i = 0; i < sizeof(dwValidLevels) / sizeof(DWORD); i++)
    {
        // Try with no valid arguments at all.
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(0, NULL, dwValidLevels[i], NULL, 0, NULL, NULL), "EnumPrintersW returns TRUE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == RPC_X_NULL_REF_POINTER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);

        // It has to succeed if we supply the required pointers and query no information.
        SetLastError(0xDEADBEEF);
        ok(EnumPrintersW(0, NULL, dwValidLevels[i], NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, dwValidLevels[i]);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, dwValidLevels[i]);

        // This constant is from Windows 9x/ME times and mustn't work anymore.
        SetLastError(0xDEADBEEF);
        ok(EnumPrintersW(PRINTER_ENUM_DEFAULT, NULL, dwValidLevels[i], NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        ok(cbNeeded == 0, "cbNeeded is %lu for Level %lu!\n", cbNeeded, dwValidLevels[i]);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, dwValidLevels[i]);

        // Now things get interesting. Let's query the buffer size for information about the local printers.
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, dwValidLevels[i], NULL, 0, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        ok(cbNeeded > 0, "cbNeeded is 0 for Level %lu!\n", dwValidLevels[i]);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, dwValidLevels[i]);

        // Same error has to occur with no buffer, but a size < 4 (AlignRpcPtr comes into play here).
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, dwValidLevels[i], NULL, 1, &cbNeeded, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        ok(cbNeeded > 0, "cbNeeded is 0 for Level %lu!\n", dwValidLevels[i]);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, dwValidLevels[i]);

        // Now provide the demanded size, but no buffer.
        SetLastError(0xDEADBEEF);
        ok(!EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, dwValidLevels[i], NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintersW returns TRUE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        ok(cbTemp == 0, "cbTemp is %lu for Level %lu!\n", cbTemp, dwValidLevels[i]);
        ok(dwReturned == 0, "dwReturned is %lu for Level %lu!\n", dwReturned, dwValidLevels[i]);

        // Finally use the function as intended and aim for success!
        pMem = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
        SetLastError(0xDEADBEEF);
        ok(EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, dwValidLevels[i], pMem, cbNeeded, &cbTemp, &dwReturned), "EnumPrintersW returns FALSE for Level %lu!\n", dwValidLevels[i]);
        ok(GetLastError() == ERROR_SUCCESS, "EnumPrintersW returns error %lu for Level %lu!\n", GetLastError(), dwValidLevels[i]);
        HeapFree(GetProcessHeap(), 0, pMem);
    }
}
