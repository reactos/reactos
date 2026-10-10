/* 
 * Unit test suite for comdlg32 API functions: printer dialogs
 *
 * Copyright 2006-2007 Detlef Riekenberg
 * Copyright 2013 Dmitry Timoshkov
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "cderr.h"
#include "commdlg.h"
#include "dlgs.h"
#include "winspool.h"

#include "wine/test.h"

/* ########################### */

extern const IID IID_IObjectWithSite;

static HMODULE  hcomdlg32;
static HRESULT (WINAPI * pPrintDlgExW)(LPPRINTDLGEXW);

/* ########################### */

static const CHAR emptyA[] = "";
static const CHAR PrinterPortsA[] = "PrinterPorts";

/* ########################### */

static void test_PageSetupDlgA(void)
{
    LPPAGESETUPDLGA pDlg;
    DWORD res;

    pDlg = malloc((sizeof(PAGESETUPDLGA)) * 2);
    if (!pDlg) return;

    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(NULL);
    ok( !res && (CommDlgExtendedError() == CDERR_INITIALIZATION),
        "returned %lu with %lu and 0x%lx (expected '0' and "
        "CDERR_INITIALIZATION)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA) -1;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %lu with %lu and 0x%lx (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA) +1;
    pDlg->Flags = PSD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %lu with %lu and 0x%lx (expected '0' and CDERR_STRUCTSIZE)\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PAGESETUPDLGA));
    pDlg->lStructSize = sizeof(PAGESETUPDLGA);
    pDlg->Flags = PSD_RETURNDEFAULT | PSD_NOWARNING;
    SetLastError(0xdeadbeef);
    res = PageSetupDlgA(pDlg);
    ok( res || (CommDlgExtendedError() == PDERR_NODEFAULTPRN),
        "returned %lu with %lu and 0x%lx (expected '!= 0' or '0' and "
        "PDERR_NODEFAULTPRN)\n", res, GetLastError(), CommDlgExtendedError());

    if (!res && (CommDlgExtendedError() == PDERR_NODEFAULTPRN)) {
        skip("No printer configured.\n");
        free(pDlg);
        return;
    }

    ok( pDlg->hDevMode && pDlg->hDevNames,
        "got %p and %p (expected '!= NULL' for both)\n",
        pDlg->hDevMode, pDlg->hDevNames);

    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);

    free(pDlg);
}

/* ########################### */

static UINT_PTR CALLBACK print_hook_proc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_INITDIALOG)
    {
        /* some driver popup a dialog and hung the test or silently limit the number of copies,
           when trying to set more than 999 copies */
        SetDlgItemInt(hdlg, edt3, 123, FALSE);
        PostMessageA(hdlg, WM_COMMAND, IDOK, FALSE);
    }
    return 0;
}

static void test_PrintDlgA(void)
{
    DWORD res, n_copies = 0;
    LPPRINTDLGA pDlg;
    DEVNAMES    *pDevNames;
    LPCSTR driver;
    LPCSTR device;
    LPCSTR port;
    CHAR   buffer[MAX_PATH];
    LPSTR  ptr;
    DEVMODEA *dm;

    pDlg = malloc((sizeof(PRINTDLGA)) * 2);
    if (!pDlg) return;


    /* will crash with unpatched wine */
    SetLastError(0xdeadbeef);
    res = PrintDlgA(NULL);
    ok( !res && (CommDlgExtendedError() == CDERR_INITIALIZATION),
        "returned %ld with 0x%lx and 0x%lx (expected '0' and "
        "CDERR_INITIALIZATION)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA) - 1;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %ld with 0x%lx and 0x%lx (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());

    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA) + 1;
    pDlg->Flags = PD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( !res && (CommDlgExtendedError() == CDERR_STRUCTSIZE),
        "returned %lu with %lu and 0x%lx (expected '0' and "
        "CDERR_STRUCTSIZE)\n", res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGA));
    pDlg->lStructSize = sizeof(PRINTDLGA);
    pDlg->Flags = PD_RETURNDEFAULT;
    SetLastError(0xdeadbeef);
    res = PrintDlgA(pDlg);
    ok( res || (CommDlgExtendedError() == PDERR_NODEFAULTPRN),
        "returned %ld with 0x%lx and 0x%lx (expected '!= 0' or '0' and "
        "PDERR_NODEFAULTPRN)\n", res, GetLastError(), CommDlgExtendedError());

    if (!res && (CommDlgExtendedError() == PDERR_NODEFAULTPRN)) {
        skip("No printer configured.\n");
        free(pDlg);
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
        ok( ptr == NULL, "got %p for '%s' (expected NULL for a simple name)\n", ptr, driver);

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
            "got %lu with %lu and %p for '%s' (expected '>1' and '!= NULL')\n",
            res, GetLastError(), ptr, buffer);

        if (ptr) ptr[0] = '\0';
        ok( lstrcmpiA(driver, buffer) == 0,
            "got driver '%s' (expected '%s')\n", driver, buffer);

        n_copies = DeviceCapabilitiesA(device, port, DC_COPIES, NULL, NULL);
        ok(n_copies > 0, "DeviceCapabilities(DC_COPIES) failed\n");
    }

    GlobalUnlock(pDlg->hDevNames);
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);

    /* if device doesn't support printing of multiple copies then
     * an attempt to set number of copies > 1 in print dialog would
     * cause the PrintDlg under Windows display the MessageBox and
     * the test will hang waiting for user response.
     */
    if (n_copies > 1)
    {
        ZeroMemory(pDlg, sizeof(*pDlg));
        pDlg->lStructSize = sizeof(*pDlg);
        pDlg->Flags = PD_ENABLEPRINTHOOK;
        pDlg->lpfnPrintHook = print_hook_proc;
        res = PrintDlgA(pDlg);
        ok(res, "PrintDlg error %#lx\n", CommDlgExtendedError());
        /* Version of Microsoft XPS Document Writer driver shipped before Win7
         * reports that it can print multiple copies, but returns 1.
         */
        ok(pDlg->nCopies == 123 || broken(pDlg->nCopies == 1), "expected nCopies 123, got %d\n", pDlg->nCopies);
        ok(pDlg->hDevMode != 0, "hDevMode should not be 0\n");
        dm = GlobalLock(pDlg->hDevMode);
        /* some broken drivers use always PD_USEDEVMODECOPIES */
        ok((dm->dmCopies == 1) || broken(dm->dmCopies == 123),
            "expected dm->dmCopies 1, got %d\n", dm->dmCopies);
        GlobalUnlock(pDlg->hDevMode);
        GlobalFree(pDlg->hDevMode);
        GlobalFree(pDlg->hDevNames);

        ZeroMemory(pDlg, sizeof(*pDlg));
        pDlg->lStructSize = sizeof(*pDlg);
        pDlg->Flags = PD_ENABLEPRINTHOOK | PD_USEDEVMODECOPIES;
        pDlg->lpfnPrintHook = print_hook_proc;
        res = PrintDlgA(pDlg);
        ok(res, "PrintDlg error %#lx\n", CommDlgExtendedError());
        ok(pDlg->nCopies == 1, "expected nCopies 1, got %d\n", pDlg->nCopies);
        ok(pDlg->hDevMode != 0, "hDevMode should not be 0\n");
        dm = GlobalLock(pDlg->hDevMode);
        ok(dm->dmCopies == 123, "expected dm->dmCopies 123, got %d\n", dm->dmCopies);
        GlobalUnlock(pDlg->hDevMode);
        GlobalFree(pDlg->hDevMode);
        GlobalFree(pDlg->hDevNames);
    }

    free(pDlg);
}

/* ########################### */

static HRESULT WINAPI callback_QueryInterface(IPrintDialogCallback *iface,
                                              REFIID riid, void **ppv)
{
    ok(0, "callback_QueryInterface(%s): unexpected call\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI callback_AddRef(IPrintDialogCallback *iface)
{
    trace("callback_AddRef\n");
    return 2;
}

static ULONG WINAPI callback_Release(IPrintDialogCallback *iface)
{
    trace("callback_Release\n");
    return 1;
}

static HRESULT WINAPI callback_InitDone(IPrintDialogCallback *iface)
{
    trace("callback_InitDone\n");
    return S_OK;
}

static HRESULT WINAPI callback_SelectionChange(IPrintDialogCallback *iface)
{
    trace("callback_SelectionChange\n");
    return S_OK;
}

static HRESULT WINAPI callback_HandleMessage(IPrintDialogCallback *iface,
    HWND hdlg, UINT msg, WPARAM wp, LPARAM lp, LRESULT *res)
{
    trace("callback_HandleMessage %p,%04x,%Ix,%Ix,%p\n", hdlg, msg, wp, lp, res);
    /* *res = PD_RESULT_PRINT; */
    return S_OK;
}

static const IPrintDialogCallbackVtbl callback_Vtbl =
{
    callback_QueryInterface,
    callback_AddRef,
    callback_Release,
    callback_InitDone,
    callback_SelectionChange,
    callback_HandleMessage
};

static IPrintDialogCallback callback = { &callback_Vtbl };

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    trace("unknown_QueryInterface %s\n", wine_dbgstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IPrintDialogCallback))
    {
        *ppv = &callback;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IObjectWithSite))
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ok(0, "unexpected IID %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI unknown_AddRef(IUnknown *iface)
{
    trace("unknown_AddRef\n");
    return 2;
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    trace("unknown_Release\n");
    return 1;
}

static const IUnknownVtbl unknown_Vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release
};

static IUnknown unknown = { &unknown_Vtbl };

static void test_PrintDlgExW(void)
{
    PRINTPAGERANGE pagerange[2];
    LPPRINTDLGEXW pDlg;
    DEVNAMES *dn;
    HRESULT res;

    /* PrintDlgEx not present before w2k */
    if (!pPrintDlgExW) {
        win_skip("PrintDlgExW not available\n");
        return;
    }

    if (0) /* Crashes on Win10 */
    {
        /* Set CommDlgExtendedError != 0 */
        PrintDlgA(NULL);
        SetLastError(0xdeadbeef);
        res = pPrintDlgExW(NULL);
        ok( (res == E_INVALIDARG),
            "got 0x%lx with %lu and %lu (expected 'E_INVALIDARG')\n",
            res, GetLastError(), CommDlgExtendedError() );
    }

    pDlg = malloc(sizeof(PRINTDLGEXW) + 8);
    if (!pDlg) return;

    /* lStructSize must be exact */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW) - 1;
    PrintDlgA(NULL);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_INVALIDARG),
        "got 0x%lx with %lu and %lu (expected 'E_INVALIDARG')\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW) + 1;
    PrintDlgA(NULL);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_INVALIDARG),
        "got 0x%lx with %lu and %lu (expected 'E_INVALIDARG')\n",
        res, GetLastError(), CommDlgExtendedError());


    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    SetLastError(0xdeadbeef);
    res = pPrintDlgExW(pDlg);
    ok( (res == E_HANDLE),
        "got 0x%lx with %lu and %lu (expected 'E_HANDLE')\n",
        res, GetLastError(), CommDlgExtendedError());

    /* nStartPage must be START_PAGE_GENERAL for the general page or a valid property sheet index */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING | PD_NOPAGENUMS;
    res = pPrintDlgExW(pDlg);
    ok((res == E_INVALIDARG), "got 0x%lx (expected 'E_INVALIDARG')\n", res);

    /* Use PD_NOPAGENUMS or set nMaxPageRanges and lpPageRanges */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok((res == E_INVALIDARG), "got 0x%lx (expected 'E_INVALIDARG')\n", res);

    /* this is invalid: a valid lpPageRanges with 0 for nMaxPageRanges */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING;
    pDlg->lpPageRanges = pagerange;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok((res == E_INVALIDARG), "got 0x%lx (expected 'E_INVALIDARG')\n", res);

    /* this is invalid: NULL for lpPageRanges with a valid nMaxPageRanges */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING;
    pDlg->nMaxPageRanges = 1;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok((res == E_INVALIDARG), "got 0x%lx (expected 'E_INVALIDARG')\n", res);

    /* this works: lpPageRanges with a valid nMaxPageRanges */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING;
    pDlg->nMaxPageRanges = 1;
    pDlg->lpPageRanges = pagerange;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    if (res == E_FAIL)
    {
        skip("No printer configured.\n");
        free(pDlg);
        return;
    }

    ok(res == S_OK, "got 0x%lx (expected S_OK)\n", res);

    dn = GlobalLock(pDlg->hDevNames);
    ok(dn != NULL, "expected '!= NULL' for GlobalLock(%p)\n",pDlg->hDevNames);
    if (dn)
    {
        ok(dn->wDriverOffset, "(expected '!= 0' for wDriverOffset)\n");
        ok(dn->wDeviceOffset, "(expected '!= 0' for wDeviceOffset)\n");
        ok(dn->wOutputOffset, "(expected '!= 0' for wOutputOffset)\n");
        ok(dn->wDefault == DN_DEFAULTPRN, "got 0x%x (expected DN_DEFAULTPRN)\n", dn->wDefault);

        GlobalUnlock(pDlg->hDevNames);
    }
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);

    /* this works also: PD_NOPAGENUMS */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING | PD_NOPAGENUMS;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok(res == S_OK, "got 0x%lx (expected S_OK)\n", res);
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);

    /* this works: PD_RETURNDC with PD_RETURNDEFAULT */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING | PD_NOPAGENUMS | PD_RETURNDC;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok(res == S_OK, "got 0x%lx (expected S_OK)\n", res);
    ok(pDlg->hDC != NULL, "HDC missing for PD_RETURNDC\n");
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);
    DeleteDC(pDlg->hDC);

    /* this works: PD_RETURNIC with PD_RETURNDEFAULT */
    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_RETURNDEFAULT | PD_NOWARNING | PD_NOPAGENUMS | PD_RETURNIC;
    pDlg->nStartPage = START_PAGE_GENERAL;
    res = pPrintDlgExW(pDlg);
    ok(res == S_OK, "got 0x%lx (expected S_OK)\n", res);
    ok(pDlg->hDC != NULL, "HDC missing for PD_RETURNIC\n");
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);
    DeleteDC(pDlg->hDC);

    /* interactive PrintDlgEx tests */

    if (!winetest_interactive)
    {
        skip("interactive PrintDlgEx tests (set WINETEST_INTERACTIVE=1)\n");
        free(pDlg);
        return;
    }

    ZeroMemory(pDlg, sizeof(PRINTDLGEXW));
    pDlg->lStructSize = sizeof(PRINTDLGEXW);
    pDlg->hwndOwner = GetDesktopWindow();
    pDlg->Flags = PD_NOPAGENUMS | PD_RETURNIC;
    pDlg->nStartPage = START_PAGE_GENERAL;
    pDlg->lpCallback = &unknown;
    pDlg->dwResultAction = S_OK;
    res = pPrintDlgExW(pDlg);
    ok(res == S_OK, "got 0x%lx (expected S_OK)\n", res);
    ok(pDlg->dwResultAction == PD_RESULT_PRINT, "expected PD_RESULT_PRINT, got %#lx\n", pDlg->dwResultAction);
    ok(pDlg->hDC != NULL, "HDC missing for PD_RETURNIC\n");
    GlobalFree(pDlg->hDevMode);
    GlobalFree(pDlg->hDevNames);
    DeleteDC(pDlg->hDC);

    free(pDlg);
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
        goto end;
    }
    GlobalFree(pd.hDevMode);
    GlobalFree(pd.hDevNames);

    ok(pd.hDC != NULL, "PrintDlg didn't return a DC.\n");
    if (!(print_dc = pd.hDC))
        goto end;

    ok(SetAbortProc(print_dc, abort_proc) > 0, "SetAbortProc failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by SetAbortProc.\n");
    abort_proc_called = FALSE;

    doc_info.cbSize = sizeof(doc_info);
    doc_info.lpszDocName = "Some document";
    doc_info.lpszOutput = filename;

    job_id = StartDocA(print_dc, &doc_info);

    ok(job_id > 0 ||
       GetLastError() == ERROR_SPL_NO_STARTDOC, /* Vista can fail with this error when using the XPS driver */
       "StartDocA failed ret %d gle %ld\n", job_id, GetLastError());

    if(job_id <= 0)
    {
        skip("StartDoc failed\n");
        goto end;
    }

    /* StartDoc may or may not call abort proc */

    abort_proc_called = FALSE;
    ok(StartPage(print_dc) > 0, "StartPage failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by StartPage.\n");
    abort_proc_called = FALSE;

    /* following functions sometimes call abort proc too */
    ok(FillRect(print_dc, &rect, (HBRUSH)(COLOR_BACKGROUND + 1)), "FillRect failed\n");
    ok(EndPage(print_dc) > 0, "EndPage failed\n");
    ok(EndDoc(print_dc) > 0, "EndDoc failed\n");

    abort_proc_called = FALSE;
    ok(DeleteDC(print_dc), "DeleteDC failed\n");
    ok(!abort_proc_called, "AbortProc got called unexpectedly by DeleteDC.\n");
    abort_proc_called = FALSE;

end:
    SetLastError(0xdeadbeef);
    if(!DeleteFileA(filename))
        trace("Failed to delete temporary file (err = %lx)\n", GetLastError());
}

/* ########################### */

START_TEST(printdlg)
{
    hcomdlg32 = GetModuleHandleA("comdlg32.dll");
    pPrintDlgExW = (void *) GetProcAddress(hcomdlg32, "PrintDlgExW");

    test_PageSetupDlgA();
    test_PrintDlgA();
    test_PrintDlgExW();
    test_abort_proc();
}
