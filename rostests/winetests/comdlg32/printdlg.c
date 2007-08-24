/* 
 * Unit test suite for comdlg32 API functions: printer dialogs
 *
 * Copyright 2006 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wingdi.h"
#include "winuser.h"

#include "cderr.h"
#include "commdlg.h"

#include "wine/test.h"


/* ######## */

static void test_PageSetupDlgA(void)
{
    LPPAGESETUPDLGA pDlg;
    DWORD res;

    pDlg = HeapAlloc(GetProcessHeap(), 0, (sizeof(PAGESETUPDLGA)) * 2);
    if (!pDlg) return;

    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(NULL);
    ok( !res && (CommDlgExtendedError() == CDERR_INITIALIZATION),
        "returned %u with %u and 0x%x (expected '0' and "
        "CDERR_INITIALIZATION)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA) -1;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %u with %u and 0x%x (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA) +1;
    pDlg->Flags = PSD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %u with %u and 0x%x (expected '0' and CDERR_STRUCTSIZE)\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA);
    pDlg->Flags = PSD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    trace("after pagesetupdlga res = %d, le %d, ext error 0x%x\n",
	res, GetLastError(), CommDlgExtendedError());
    ok( res || (CommDlgExtendedError() == PDERR_NODEFAULTPRN),
        "returned %u with %u and 0x%x (expected '!= 0' or '0' and "
        "PDERR_NODEFAULTPRN)\n", res, GetLastError(), CommDlgExtendedError());
    if (!res && (CommDlgExtendedError() == PDERR_NODEFAULTPRN)) {
	skip("No printer configured.\n");
	HeapFree(GetProcessHeap(), 0, pDlg);
	return;
    }
    ok( pDlg->hDevMode && pDlg->hDevNames,
        "got %p and %p (expected '!= NULL' for both)\n",
        pDlg->hDevMode, pDlg->hDevNames);

    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);

    HeapFree(GetProcessHeap(), 0, pDlg);

}

/* ##### */

static void test_PrintDlgA(void)
{
    DWORD       res;
    LPPRINTDLGA pDlg;


    pDlg = HeapAlloc(GetProcessHeap(), 0, (sizeof(PRINTDLGA)) * 2);
    if (!pDlg) return;


    /* will crash with unpatched wine */
    SetLastError(0xdeadbeef);
    res = PrintDlgA(NULL);
    ok( !res && (CommDlgExtendedError() == CDERR_INITIALIZATION),
        "returned %d with 0x%x and 0x%x (expected '0' and "
        "CDERR_INITIALIZATION)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA) - 1;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %d with 0x%x and 0x%x (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA) + 1;
    pDlg->Flags = PD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %u with %u and 0x%x (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA);
    pDlg->Flags = PD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( res || (CommDlgExtendedError() == PDERR_NODEFAULTPRN),
        "returned %d with 0x%x and 0x%x (expected '!= 0' or '0' and "
        "PDERR_NODEFAULTPRN)\n", res, GetLastError(), CommDlgExtendedError());

    HeapFree(GetProcessHeap(), 0, pDlg);

}


START_TEST(printdlg)
{
    test_PageSetupDlgA();
    test_PrintDlgA();

}
