/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for EnumPrintProcessorDatatypesA/EnumPrintProcessorDatatypesW
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(EnumPrintProcessorDatatypes)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    DWORD dwReturned;
    PDATATYPES_INFO_1W pDatatypesInfo1;

    // Try with an invalid level, this needs to be caught first.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, NULL, 0, NULL, 0, NULL, NULL), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Now try with valid level, but no pcbNeeded and no pcReturned.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, NULL, NULL), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Now try with pcbNeeded and pcReturned, but give no Print Processor.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_UNKNOWN_PRINTPROCESSOR, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Same error has to occur when looking for an invalid Print Processor.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, L"invalid", 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_UNKNOWN_PRINTPROCESSOR, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Now get the required buffer size by supplying all information. This needs to fail with ERROR_INSUFFICIENT_BUFFER.
    // This also verifies that the function is really doing a case-insensitive lookup.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, L"wInPrInT", 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Same error has to occur with a size to small.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, L"wInPrInT", 1, NULL, 1, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Now provide the demanded size, but no buffer.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, L"wInPrInT", 1, NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // This also has to fail the same way when no Print Processor was given at all.
    SetLastError(0xDEADBEEF);
    ok(!EnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Finally use the function as intended and aim for success!
    pDatatypesInfo1 = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(EnumPrintProcessorDatatypesW(NULL, L"wInPrInT", 1, (PBYTE)pDatatypesInfo1, cbNeeded, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pDatatypesInfo1);
}
