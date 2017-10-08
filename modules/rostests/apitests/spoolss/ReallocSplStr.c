/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for ReallocSplStr
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <spoolss.h>

START_TEST(ReallocSplStr)
{
    const WCHAR wszTestString1[] = L"Test";
    const WCHAR wszTestString2[] = L"New";

    DWORD dwResult;
    PWSTR pwszBackup;
    PWSTR pwszTest;

    // Verify that ReallocSplStr raises an exception if all parameters are NULL.
    _SEH2_TRY
    {
        dwResult = 0;
        ReallocSplStr(NULL, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(dwResult == EXCEPTION_ACCESS_VIOLATION, "dwResult is %lx!\n", dwResult);

    // Allocate a string for testing.
    pwszTest = AllocSplStr(wszTestString1);
    if (!pwszTest)
    {
        skip("AllocSplStr failed with error %lu!\n", GetLastError());
        return;
    }

    // Verify that ReallocSplStr frees the old string even if pwszInput is NULL.
    ok(ReallocSplStr(&pwszTest, NULL), "ReallocSplStr is FALSE!\n");
    ok(pwszTest == NULL, "pwszTest is %p\n", pwszTest);

    // Now verify that ReallocSplStr copies the new string into a new block and frees the old one.
    pwszBackup = pwszTest;
    ok(ReallocSplStr(&pwszTest, wszTestString2), "ReallocSplStr is FALSE!\n");
    ok(wcscmp(pwszTest, wszTestString2) == 0, "New string was not copied into pwszTest!\n");

    _SEH2_TRY
    {
        dwResult = (DWORD)wcscmp(pwszBackup, wszTestString1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(dwResult == EXCEPTION_ACCESS_VIOLATION, "dwResult is %lx!\n", dwResult);
}
