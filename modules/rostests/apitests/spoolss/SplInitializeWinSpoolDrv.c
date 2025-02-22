/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for SplInitializeWinSpoolDrv
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <spoolss.h>

START_TEST(SplInitializeWinSpoolDrv)
{
    HINSTANCE hWinspool;
    PVOID Table[9];

    hWinspool = LoadLibraryW(L"winspool.drv");
    if (!hWinspool)
    {
        skip("Could not load winspool.drv, last error is %lu!\n", GetLastError());
        return;
    }

    ok(SplInitializeWinSpoolDrv(Table), "SplInitializeWinSpoolDrv returns FALSE!\n");
    ok(Table[0] == GetProcAddress(hWinspool, "OpenPrinterW"), "Table[0] is %p\n", Table[0]);
    ok(Table[1] == GetProcAddress(hWinspool, "ClosePrinter"), "Table[1] is %p\n", Table[1]);
    ok(Table[2] == GetProcAddress(hWinspool, "SpoolerDevQueryPrintW"), "Table[2] is %p\n", Table[2]);
    ok(Table[3] == GetProcAddress(hWinspool, "SpoolerPrinterEvent"), "Table[3] is %p\n", Table[3]);
    ok(Table[4] == GetProcAddress(hWinspool, "DocumentPropertiesW"), "Table[4] is %p\n", Table[4]);
    ok(Table[5] == GetProcAddress(hWinspool, (LPSTR)212), "Table[5] is %p\n", Table[5]);
    ok(Table[6] == GetProcAddress(hWinspool, (LPSTR)213), "Table[6] is %p\n", Table[6]);
    ok(Table[7] == GetProcAddress(hWinspool, (LPSTR)214), "Table[7] is %p\n", Table[7]);
    ok(Table[8] == GetProcAddress(hWinspool, (LPSTR)215), "Table[8] is %p\n", Table[8]);
}
