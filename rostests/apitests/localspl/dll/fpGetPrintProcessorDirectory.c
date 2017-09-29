/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for fpGetPrintProcessorDirectory
 * COPYRIGHT:   Copyright 2016-2017 Colin Finck (colin@reactos.org)
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

typedef BOOL (WINAPI *PGetPrintProcessorDirectoryW)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
extern BOOL GetLocalsplFuncs(LPPRINTPROVIDOR pp);
extern PVOID GetSpoolssFunc(const char* FunctionName);

START_TEST(fpGetPrintProcessorDirectory)
{
    DWORD cbNeeded;
    DWORD cbTemp;
    //DWORD dwReturned;
    PGetPrintProcessorDirectoryW pGetPrintProcessorDirectoryW;
    PRINTPROVIDOR pp;
    PWSTR pwszBuffer;

    if (!GetLocalsplFuncs(&pp))
        return;

    pGetPrintProcessorDirectoryW = GetSpoolssFunc("GetPrintProcessorDirectoryW");
    if (!pGetPrintProcessorDirectoryW)
        return;

    // In contrast to GetPrintProcessorDirectoryW, fpGetPrintProcessorDirectory needs an environment and doesn't just accept NULL.
    SetLastError(0xDEADBEEF);
    ok(!pp.fpGetPrintProcessorDirectory(NULL, NULL, 0, NULL, 0, NULL), "fpGetPrintProcessorDirectory returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_ENVIRONMENT, "fpGetPrintProcessorDirectory returns error %lu!\n", GetLastError());

    // Try with an invalid environment as well.
    SetLastError(0xDEADBEEF);
    ok(!pp.fpGetPrintProcessorDirectory(NULL, L"invalid", 0, NULL, 0, NULL), "fpGetPrintProcessorDirectory returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_ENVIRONMENT, "fpGetPrintProcessorDirectory returns error %lu!\n", GetLastError());

	// This test corrupts something inside spoolsv so that it's only runnable once without restarting spoolsv. Therefore it's disabled.
#if 0
    // Now provide a valid environment and prove that it is checked case-insensitively.
    // In contrast to GetPrintProcessorDirectoryW, the level isn't the next thing checked here, but fpGetPrintProcessorDirectory
    // already tries to access the non-supplied pcbNeeded variable.
    _SEH2_TRY
    {
        dwReturned = 0;
        pp.fpGetPrintProcessorDirectory(NULL, L"wIndows nt x86", 0, NULL, 0, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwReturned = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(dwReturned == EXCEPTION_ACCESS_VIOLATION, "dwReturned is %lu!\n", dwReturned);
#endif

    // fpGetPrintProcessorDirectory doesn't care about the supplied level at all. Prove this here.
    // With no buffer given, this needs to fail with ERROR_INSUFFICIENT_BUFFER.
    SetLastError(0xDEADBEEF);
    cbNeeded = 0;
    ok(!pp.fpGetPrintProcessorDirectory(NULL, L"wIndows nt x86", 1337, NULL, 0, &cbNeeded), "fpGetPrintProcessorDirectory returns TRUE!\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "fpGetPrintProcessorDirectory returns error %lu!\n", GetLastError());
    ok(cbNeeded > 0, "cbNeeded is %lu!\n", cbNeeded);

	// This test corrupts something inside spoolsv so that it's only runnable once without restarting spoolsv. Therefore it's disabled.
#if 0
    // Now provide the demanded size, but no buffer.
    // Unlike GetPrintProcessorDirectoryW, fpGetPrintProcessorDirectory doesn't check for this case and tries to access the buffer.
    _SEH2_TRY
    {
        dwReturned = 0;
        pp.fpGetPrintProcessorDirectory(NULL, L"wIndows nt x86", 1, NULL, cbNeeded, &cbTemp);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwReturned = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(dwReturned == EXCEPTION_ACCESS_VIOLATION, "dwReturned is %lu!\n", dwReturned);
#endif

    // Prove that this check is implemented in spoolss' GetPrintProcessorDirectoryW instead.
    // In contrast to winspool's GetPrintProcessorDirectoryW, cbTemp is left untouched though. This comes from the fact that RPC isn't involved here.
    SetLastError(0xDEADBEEF);
    cbTemp = 0xDEADBEEF;
    ok(!pGetPrintProcessorDirectoryW(NULL, L"wIndows nt x86", 1, NULL, cbNeeded, &cbTemp), "pGetPrintProcessorDirectoryW returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "pGetPrintProcessorDirectoryW returns error %lu!\n", GetLastError());
    ok(cbTemp == 0xDEADBEEF, "cbTemp is %lu!\n", cbTemp);

    // Finally use the function as intended and aim for success!
    // We only check success by the boolean return value though. GetLastError doesn't return anything meaningful here.
    pwszBuffer = DllAllocSplMem(cbNeeded);
    SetLastError(0xDEADBEEF);
    ok(pp.fpGetPrintProcessorDirectory(NULL, L"wIndows nt x86", 1, (PBYTE)pwszBuffer, cbNeeded, &cbTemp), "fpGetPrintProcessorDirectory returns FALSE!\n");
    ok(wcslen(pwszBuffer) == cbNeeded / sizeof(WCHAR) - 1, "fpGetPrintProcessorDirectory string is %Iu characters long, but %lu characters expected!\n", wcslen(pwszBuffer), cbNeeded / sizeof(WCHAR) - 1);
    DllFreeSplMem(pwszBuffer);
}
