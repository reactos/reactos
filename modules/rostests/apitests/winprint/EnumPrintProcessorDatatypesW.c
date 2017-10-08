/*
 * PROJECT:     ReactOS Standard Print Processor API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for EnumPrintProcessorDatatypesW
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

typedef BOOL (WINAPI *PEnumPrintProcessorDatatypesW)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
extern PVOID GetWinprintFunc(const char* FunctionName);

START_TEST(EnumPrintProcessorDatatypesW)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    DWORD dwReturned;
    PDATATYPES_INFO_1W pDatatypesInfo1;
    PEnumPrintProcessorDatatypesW pEnumPrintProcessorDatatypesW;

    // Get the function we want to test from one of the possible Print Processor DLLs.
    pEnumPrintProcessorDatatypesW = (PEnumPrintProcessorDatatypesW)GetWinprintFunc("EnumPrintProcessorDatatypesW");
    if (!pEnumPrintProcessorDatatypesW)
        return;

    // Try with an invalid level. The error needs to be set by winspool, but not by the Print Processor.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, NULL, 0, NULL, 0, NULL, NULL), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == 0xDEADBEEF, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Now try with valid level, but no pcbNeeded and no pcReturned. The error needs to be set by RPC.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, NULL, NULL), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == 0xDEADBEEF, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());

    // Now try with pcbNeeded and pcReturned, but give no Print Processor. Show that winprint actually ignores the given Print Processor.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Same error has to occur when looking for an invalid Print Processor.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, L"invalid", 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Now get the required buffer size by supplying all information. This needs to fail with ERROR_INSUFFICIENT_BUFFER.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, L"winprint", 1, NULL, 0, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Same error has to occur with a size to small.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, L"winprint", 1, NULL, 1, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "EnumPrintersW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Now provide the demanded size, but no buffer. Show that winprint returns a different error than the same function in winspool.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, L"winprint", 1, NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbTemp == cbNeeded, "cbTemp is %lu!\n", cbTemp);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // This also has to fail the same way when no Print Processor was given at all.
    SetLastError(0xDEADBEEF);
    ok(!pEnumPrintProcessorDatatypesW(NULL, NULL, 1, NULL, cbNeeded, &cbTemp, &dwReturned), "EnumPrintProcessorDatatypesW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    ok(cbTemp == cbNeeded, "cbTemp is %lu!\n", cbTemp);
    ok(dwReturned == 0, "dwReturned is %lu!\n", dwReturned);

    // Finally use the function as intended and aim for success! Show that winprint doesn't modify the error code at all.
    pDatatypesInfo1 = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(pEnumPrintProcessorDatatypesW(NULL, L"winprint", 1, (PBYTE)pDatatypesInfo1, cbNeeded, &cbNeeded, &dwReturned), "EnumPrintProcessorDatatypesW returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "EnumPrintProcessorDatatypesW returns error %lu!\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pDatatypesInfo1);
}
