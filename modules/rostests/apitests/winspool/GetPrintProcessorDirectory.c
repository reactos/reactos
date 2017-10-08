/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetPrintProcessorDirectoryA/GetPrintProcessorDirectoryW
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(GetPrintProcessorDirectoryA)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    PSTR pszBuffer;

    // Try with an invalid level, this needs to be caught first.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryA(NULL, NULL, 0, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0xDEADBEEF, "cbNeeded is %lu!\n", cbNeeded);

    // Now try with valid level, but no pcbNeeded.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryA(NULL, NULL, 1, NULL, 0, NULL), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());

    // Try with an invalid environment as well.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryA(NULL, "invalid", 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_ENVIRONMENT, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);

    // Now get the required buffer size by supplying pcbNeeded. This needs to fail with ERROR_INSUFFICIENT_BUFFER.
    // Note for GetPrintProcessorDirectoryA: cbNeeded will be the same as for GetPrintProcessorDirectoryW, even though the ANSI string only needs half of it!
    SetLastError(0xDEADBEEF);
    cbNeeded = 0;
    ok(!GetPrintProcessorDirectoryA(NULL, NULL, 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");

    // Now provide the demanded size, but no buffer.
    SetLastError(0xDEADBEEF);
    cbTemp = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryA(NULL, NULL, 1, NULL, cbNeeded, &cbTemp), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);

    // Same error has to occur with a size too small.
    SetLastError(0xDEADBEEF);
    cbTemp = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryA(NULL, NULL, 1, NULL, 1, &cbTemp), "GetPrintProcessorDirectoryA returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);

    // Finally use the function as intended and aim for success!
    pszBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(GetPrintProcessorDirectoryA(NULL, NULL, 1, (PBYTE)pszBuffer, cbNeeded, &cbTemp), "GetPrintProcessorDirectoryA returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "GetPrintProcessorDirectoryA returns error %lu!\n", GetLastError());

    // Note for GetPrintProcessorDirectoryA: cbNeeded is the same as for GetPrintProcessorDirectoryW!
    ok(strlen(pszBuffer) == cbNeeded / sizeof(WCHAR) - 1, "GetPrintProcessorDirectoryA string is %Iu characters long, but %lu characters expected!\n", strlen(pszBuffer), cbNeeded / sizeof(WCHAR) - 1);
    HeapFree(GetProcessHeap(), 0, pszBuffer);
}

START_TEST(GetPrintProcessorDirectoryW)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    PWSTR pwszBuffer;

    // Try with an invalid level, this needs to be caught first.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 0, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0xDEADBEEF, "cbNeeded is %lu!\n", cbNeeded);

    // Now try with valid level, but no pcbNeeded.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 0, NULL), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());

    // Try with an invalid environment as well.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryW(NULL, L"invalid", 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_ENVIRONMENT, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);

    // Now get the required buffer size by supplying pcbNeeded. This needs to fail with ERROR_INSUFFICIENT_BUFFER.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0;
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");

    // Now provide the demanded size, but no buffer.
    SetLastError(0xDEADBEEF);
    cbTemp = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, cbNeeded, &cbTemp), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);

    // Same error has to occur with a size too small.
    SetLastError(0xDEADBEEF);
    cbTemp = 0xDEADBEEF;
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 1, &cbTemp), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbTemp is %lu!\n", cbTemp);

    // Finally use the function as intended and aim for success!
    pwszBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(GetPrintProcessorDirectoryW(NULL, NULL, 1, (PBYTE)pwszBuffer, cbNeeded, &cbTemp), "GetPrintProcessorDirectoryW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(wcslen(pwszBuffer) == cbNeeded / sizeof(WCHAR) - 1, "GetPrintProcessorDirectoryW string is %Iu characters long, but %lu characters expected!\n", wcslen(pwszBuffer), cbNeeded / sizeof(WCHAR) - 1);
    HeapFree(GetProcessHeap(), 0, pwszBuffer);
}
