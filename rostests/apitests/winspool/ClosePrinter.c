/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for ClosePrinter
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>

START_TEST(ClosePrinter)
{
    SetLastError(0xDEADBEEF);
    ok(!ClosePrinter(NULL), "ClosePrinter returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "ClosePrinter returns error %lu!\n", GetLastError());
}
