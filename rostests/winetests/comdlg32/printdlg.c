/* 
 * Unit test suite for comdlg32 API functions: printer dialogs
 *
 * Copyright 2006-2007 Detlef Riekenberg
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
#include "winuser.h"
#include "objbase.h"

#include "cderr.h"
#include "commdlg.h"

#include "wine/test.h"

/* ########################### */

static HMODULE  hcomdlg32;
static HRESULT (WINAPI * pPrintDlgExA)(LPPRINTDLGEXA);
static HRESULT (WINAPI * pPrintDlgExW)(LPPRINTDLGEXW);

/* ########################### */

static const CHAR emptyA[] = "";
static const CHAR PrinterPortsA[] = "PrinterPorts";

/* ########################### */

static LPCSTR load_functions(void)
{
    LPCSTR  ptr;

    ptr = "comdlg32.dll";
    hcomdlg32 = GetModuleHandleA(ptr);

    ptr = "PrintDlgExA";
    pPrintDlgExA = (void *) GetProcAddress(hcomdlg32, ptr);
    if (!pPrintDlgExA) return ptr;

    ptr = "PrintDlgExW";
    pPrintDlgExW = (void *) GetProcAddress(hcomdlg32, ptr);
    if (!pPrintDlgExW) return ptr;

    return NULL;

}

/* ########################### */

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
    pDlg->Flags = PSD_RETURNDEFAULT | PSD_NOWARNING;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
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

/* ########################### */

static void test_PrintDlgA(void)
{
    DWORD       res;
    LPPRINTDLGA pDlg;
    DEVNAMES    *pDevNames;
    LPCSTR driver;
    LPCSTR device;
    LPCSTR port;
    CHAR   buffer[MAX_PATH];
    LPSTR  ptr;


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

    if (!res && (CommDlgExtendedError() == PDERR_NODEFAULTPRN)) {
        skip("No printer configured.\n");
        HeapFree(GetProcessHeap(), 0, pDlg);
        return;
    }

    ok(pDlg->hDevNames != NULL, "(expected '!= NULL')\n");
    pDevNames = GlobalLock(pDlg->hDevNames);
    ok(pDevNames != NULL, "(expected '!= NULL')\n");

    if (pDevNames) {
        ok(pDevNames->wDriverOffset, "(expected '!= 0' for wDriverOffset)\n");
        ok(pDevNames->wDeviceOffset, "(expected '!= 0' for wDeviceOffset)\n");
        ok(pDevNames->wOutputOffset, "(expected '!= 0' for wOutputOffset)\n");
        ok(pDevNames->wDefault == DN_DEFAULTPRN, "got 0x%x (expected DN_DEFAULTPRN)\n", pDevNames->wDefault);

        driver = (LPCSTR)pDevNames + pDevNames->wDriverOffset;
        device = (LPCSTR)pDevNames + pDevNames->wDeviceOffset;
        port = (LPCSTR)pDevNames + pDevNames->wOutputOffset;
        trace("driver '%s' device '%s' port '%s'\n", driver, device, port);

        /* The Driver Entry does not include a Path */
        ptr = strrchr(driver, '\\');
        todo_wine {
        ok( ptr == NULL, "got %p for '%s' (expected NULL for a simple name)\n", ptr, driver);
        }

        /* The Driver Entry does not have an extension (fixed to ".drv") */
        ptr = strrchr(driver, '.');
        todo_wine {
        ok( ptr == NULL, "got %p for '%s' (expected NULL for no extension)\n", ptr, driver);
        }


        buffer[0] = '\0';
        SetLastError(0xdeadbeef);
        res = GetProfileStringA(PrinterPortsA, device, emptyA, buffer, sizeof(buffer));
        ptr = strchr(buffer, ',');
        ok( (res > 1) && (ptr != NULL),
            "got %u with %u and %p for '%s' (expected '>1' and '!= NULL')\n",
            res, GetLastError(), ptr, buffer);

        if (ptr) ptr[0] = '\0';
        todo_wine {
        ok( lstrcmpiA(driver, buffer) == 0,
            "got driver '%s' (expected '%s')\n", driver, buffer);
        }

    }

    GlobalUnlock(pDlg->hDevNames);

    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);
    HeapFree(GetProcessHeap(), 0, pDlg);

}

/* ########################### */

static void test_PrintDlgExW(void)
{
    LPPRINTDLGEXW pDlg;
    HRESULT res;

    /* Set CommDlgExtendedError != 0 */
    PrintDlg(NULL);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(NULL);
    ok( (res == E_INVALIDARG),
        "got 0x%x with %u and %u (expected 'E_INVALIDARG')\n",
        res, GetLastError(), CommDlgExtendedError());


    pDlg = HeapAlloc(GetProcessHeap(), 0, (sizeof(PRINTDLGEXW)) + 8);
    if (!pDlg) return;

    /* lStructSize must be exact */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW) - 1;
    PrintDlg(NULL);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_INVALIDARG),
        "got 0x%x with %u and %u (expected 'E_INVALIDARG')\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW) + 1;
    PrintDlg(NULL);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_INVALIDARG),
        "got 0x%x with %u and %u (expected 'E_INVALIDARG')\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_HANDLE),
        "got 0x%x with %u and %u (expected 'E_HANDLE')\n",
        res, GetLastError(), CommDlgExtendedError());


    HeapFree(GetProcessHeap(), 0, pDlg);
    return;

}

static BOOL abort_proc_called = FALSE;
static BOOL CALLBACK abort_proc(HDC hdc, int error) { return abort_proc_called = TRUE; }
static void test_abort_proc(void)
{
    HDC print_dc;
    RECT rect = {0, 0, 100, 100};
    DOCINFOA doc_info = {0};
    PRINTDLGA pd = {0};
    char filename[MAX_PATH];
    int job_id;

    if (!GetTempFileNameA(".", "prn", 0, filename))
    {
        skip("Failed to create a temporary file name\n");
        return;
    }

    pd.lStructSize = sizeof(pd);
    pd.Flags = PD_RETURNDEFAULT | PD_ALLPAGES | PD_RETURNDC | PD_PRINTTOFILE;
    pd.nFromPage = 1;
    pd.nToPage = 1;
    pd.nCopies = 1;

    if (!PrintDlgA(&pd))
    {
        skip("No default printer available.\n");
        ok(DeleteFileA(filename), "Failed to delete temporary file\n");
        return;
    }

    ok(pd.hDC != NULL, "PrintDlg didn't return a DC.\n");
    if (!(print_dc = pd.hDC))
    {
        ok(DeleteFileA(filename), "Failed to delete temporary file\n");
        return;
    }

    ok(SetAbortProc(print_dc, abort_proc) > 0, "SetAbortProc failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by SetAbortProc.\n");
    abort_proc_called = FALSE;

    doc_info.cbSize = sizeof(doc_info);
    doc_info.lpszDocName = "Some document";
    doc_info.lpszOutput = filename;

    job_id = StartDocA(print_dc, &doc_info);

    ok(job_id > 0 ||
       GetLastError() == ERROR_SPL_NO_STARTDOC, /* Vista can fail with this error when using the XPS driver */
       "StartDocA failed ret %d gle %d\n", job_id, GetLastError());

    if(job_id <= 0)
    {
        skip("StartDoc failed\n");
        goto end;
    }

    ok(abort_proc_called, "AbortProc didn't get called by StartDoc.\n");
    abort_proc_called = FALSE;

    ok(StartPage(print_dc) > 0, "StartPage failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by StartPage.\n");
    abort_proc_called = FALSE;

    ok(FillRect(print_dc, &rect, (HBRUSH)(COLOR_BACKGROUND + 1)), "FillRect failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by StretchBlt.\n");
    abort_proc_called = FALSE;

    ok(EndPage(print_dc) > 0, "EndPage failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by EndPage.\n");
    abort_proc_called = FALSE;

    ok(EndDoc(print_dc) > 0, "EndDoc failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by EndDoc.\n");
    abort_proc_called = FALSE;

    ok(DeleteDC(print_dc), "DeleteDC failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by DeleteDC.\n");
    abort_proc_called = FALSE;

end:
    ok(DeleteFileA(filename), "Failed to delete temporary file\n");
}

/* ########################### */

START_TEST(printdlg)
{
    LPCSTR  ptr;

    ptr = load_functions();

    test_PageSetupDlgA();
    test_PrintDlgA();
    test_abort_proc();

    /* PrintDlgEx not present before w2k */
    if (ptr) {
        win_skip("%s\n", ptr);
        return;
    }

    test_PrintDlgExW();
}
