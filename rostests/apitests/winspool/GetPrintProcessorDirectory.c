/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for GetPrintProcessorDirectoryA/GetPrintProcessorDirectoryW
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(GetPrintProcessorDirectory)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    PWSTR pwszBuffer;

    // Try with an invalid level, this needs to be caught first.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 0, NULL, 0, NULL), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());

    // Now try with valid level, but no pcbNeeded.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 0, NULL), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());

    // Try with an invalid environment as well.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, L"invalid", 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_ENVIRONMENT, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbNeeded == 0, "cbNeeded is %lu!\n", cbNeeded);

    // Now get the required buffer size by supplying pcbNeeded. This needs to fail with ERROR_INSUFFICIENT_BUFFER.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 0, &cbNeeded), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is 0!\n");

    // Now provide the demanded size, but no buffer.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, cbNeeded, &cbTemp), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbNeeded is %lu!\n", cbNeeded);

    // Same error has to occur with a size too small.
    SetLastError(0xDEADBEEF);
    ok(!GetPrintProcessorDirectoryW(NULL, NULL, 1, NULL, 1, &cbTemp), "GetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0, "cbNeeded is %lu!\n", cbNeeded);

    // Finally use the function as intended and aim for success!
    pwszBuffer = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(GetPrintProcessorDirectoryW(NULL, NULL, 1, (PBYTE)pwszBuffer, cbNeeded, &cbNeeded), "GetPrintProcessorDirectoryW returns FALSE!\n");
    ok(GetLastError() == ERROR_SUCCESS, "GetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pwszBuffer);
}
