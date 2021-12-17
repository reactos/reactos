/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 * Copyright 2000 Huw D M Davies
 * Copyright 2010 Vitaly Perov
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
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winspool.h"
#include "winerror.h"
#include "objbase.h"
#include "commdlg.h"

#include "wine/debug.h"

#include "dlgs.h"
#include "cderr.h"
#include "cdlg.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

/* Yes these constants are the same, but we're just copying win98 */
#define UPDOWN_ID 0x270f
#define MAX_COPIES 9999

/* This PRINTDLGA internal structure stores
 * pointers to several throughout useful structures.
 */

typedef struct
{
  LPDEVMODEA        lpDevMode;
  LPPRINTDLGA       lpPrintDlg;
  LPPRINTER_INFO_2A lpPrinterInfo;
  LPDRIVER_INFO_3A  lpDriverInfo;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
  HWND              hwndUpDown;
} PRINT_PTRA;

typedef struct
{
  LPDEVMODEW        lpDevMode;
  LPPRINTDLGW       lpPrintDlg;
  LPPRINTER_INFO_2W lpPrinterInfo;
  LPDRIVER_INFO_3W  lpDriverInfo;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
  HWND              hwndUpDown;
} PRINT_PTRW;

/* Debugging info */
struct pd_flags
{
  DWORD  flag;
  LPCSTR name;
};

static const struct pd_flags psd_flags[] = {
  {PSD_MINMARGINS,"PSD_MINMARGINS"},
  {PSD_MARGINS,"PSD_MARGINS"},
  {PSD_INTHOUSANDTHSOFINCHES,"PSD_INTHOUSANDTHSOFINCHES"},
  {PSD_INHUNDREDTHSOFMILLIMETERS,"PSD_INHUNDREDTHSOFMILLIMETERS"},
  {PSD_DISABLEMARGINS,"PSD_DISABLEMARGINS"},
  {PSD_DISABLEPRINTER,"PSD_DISABLEPRINTER"},
  {PSD_NOWARNING,"PSD_NOWARNING"},
  {PSD_DISABLEORIENTATION,"PSD_DISABLEORIENTATION"},
  {PSD_RETURNDEFAULT,"PSD_RETURNDEFAULT"},
  {PSD_DISABLEPAPER,"PSD_DISABLEPAPER"},
  {PSD_SHOWHELP,"PSD_SHOWHELP"},
  {PSD_ENABLEPAGESETUPHOOK,"PSD_ENABLEPAGESETUPHOOK"},
  {PSD_ENABLEPAGESETUPTEMPLATE,"PSD_ENABLEPAGESETUPTEMPLATE"},
  {PSD_ENABLEPAGESETUPTEMPLATEHANDLE,"PSD_ENABLEPAGESETUPTEMPLATEHANDLE"},
  {PSD_ENABLEPAGEPAINTHOOK,"PSD_ENABLEPAGEPAINTHOOK"},
  {PSD_DISABLEPAGEPAINTING,"PSD_DISABLEPAGEPAINTING"},
  {-1, NULL}
};

static const struct pd_flags pd_flags[] = {
  {PD_SELECTION, "PD_SELECTION "},
  {PD_PAGENUMS, "PD_PAGENUMS "},
  {PD_NOSELECTION, "PD_NOSELECTION "},
  {PD_NOPAGENUMS, "PD_NOPAGENUMS "},
  {PD_COLLATE, "PD_COLLATE "},
  {PD_PRINTTOFILE, "PD_PRINTTOFILE "},
  {PD_PRINTSETUP, "PD_PRINTSETUP "},
  {PD_NOWARNING, "PD_NOWARNING "},
  {PD_RETURNDC, "PD_RETURNDC "},
  {PD_RETURNIC, "PD_RETURNIC "},
  {PD_RETURNDEFAULT, "PD_RETURNDEFAULT "},
  {PD_SHOWHELP, "PD_SHOWHELP "},
  {PD_ENABLEPRINTHOOK, "PD_ENABLEPRINTHOOK "},
  {PD_ENABLESETUPHOOK, "PD_ENABLESETUPHOOK "},
  {PD_ENABLEPRINTTEMPLATE, "PD_ENABLEPRINTTEMPLATE "},
  {PD_ENABLESETUPTEMPLATE, "PD_ENABLESETUPTEMPLATE "},
  {PD_ENABLEPRINTTEMPLATEHANDLE, "PD_ENABLEPRINTTEMPLATEHANDLE "},
  {PD_ENABLESETUPTEMPLATEHANDLE, "PD_ENABLESETUPTEMPLATEHANDLE "},
  {PD_USEDEVMODECOPIES, "PD_USEDEVMODECOPIES[ANDCOLLATE] "},
  {PD_DISABLEPRINTTOFILE, "PD_DISABLEPRINTTOFILE "},
  {PD_HIDEPRINTTOFILE, "PD_HIDEPRINTTOFILE "},
  {PD_NONETWORKBUTTON, "PD_NONETWORKBUTTON "},
  {-1, NULL}
};
/* address of wndproc for subclassed Static control */
static WNDPROC lpfnStaticWndProc;
static WNDPROC edit_wndproc;
/* the text of the fake document to render for the Page Setup dialog */
static WCHAR wszFakeDocumentText[1024];
static const WCHAR pd32_collateW[] = { 'P', 'D', '3', '2', '_', 'C', 'O', 'L', 'L', 'A', 'T', 'E', 0 };
static const WCHAR pd32_nocollateW[] = { 'P', 'D', '3', '2', '_', 'N', 'O', 'C', 'O', 'L', 'L', 'A', 'T', 'E', 0 };
static const WCHAR pd32_portraitW[] = { 'P', 'D', '3', '2', '_', 'P', 'O', 'R', 'T', 'R', 'A', 'I', 'T', 0 };
static const WCHAR pd32_landscapeW[] = { 'P', 'D', '3', '2', '_', 'L', 'A', 'N', 'D', 'S', 'C', 'A', 'P', 'E', 0 };
static const WCHAR printdlg_prop[] = {'_','_','W','I','N','E','_','P','R','I','N','T','D','L','G','D','A','T','A',0};
static const WCHAR pagesetupdlg_prop[] = { '_', '_', 'W', 'I', 'N', 'E', '_', 'P', 'A', 'G', 'E',
                                           'S', 'E', 'T', 'U', 'P', 'D', 'L', 'G', 'D', 'A', 'T', 'A', 0 };


static LPWSTR strdupW(LPCWSTR p)
{
    LPWSTR ret;
    DWORD len;

    if(!p) return NULL;
    len = (lstrlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

/***********************************************************************
 * get_driver_info [internal]
 *
 * get DRIVER_INFO_3W for the current printer handle,
 * alloc the buffer, when needed
 */
static DRIVER_INFO_3W * get_driver_infoW(HANDLE hprn)
{
    DRIVER_INFO_3W *di3 = NULL;
    DWORD needed = 0;
    BOOL res;

    res = GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        di3 = HeapAlloc(GetProcessHeap(), 0, needed);
        res = GetPrinterDriverW(hprn, NULL, 3, (LPBYTE)di3, needed, &needed);
    }

    if (res)
        return di3;

    TRACE("GetPrinterDriverW failed with %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, di3);
    return NULL;
}

static DRIVER_INFO_3A * get_driver_infoA(HANDLE hprn)
{
    DRIVER_INFO_3A *di3 = NULL;
    DWORD needed = 0;
    BOOL res;

    res = GetPrinterDriverA(hprn, NULL, 3, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        di3 = HeapAlloc(GetProcessHeap(), 0, needed);
        res = GetPrinterDriverA(hprn, NULL, 3, (LPBYTE)di3, needed, &needed);
    }

    if (res)
        return di3;

    TRACE("GetPrinterDriverA failed with %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, di3);
    return NULL;
}


/***********************************************************************
 * get_printer_info [internal]
 *
 * get PRINTER_INFO_2W for the current printer handle,
 * alloc the buffer, when needed
 */
static PRINTER_INFO_2W * get_printer_infoW(HANDLE hprn)
{
    PRINTER_INFO_2W *pi2 = NULL;
    DWORD needed = 0;
    BOOL res;

    res = GetPrinterW(hprn, 2, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        pi2 = HeapAlloc(GetProcessHeap(), 0, needed);
        res = GetPrinterW(hprn, 2, (LPBYTE)pi2, needed, &needed);
    }

    if (res)
        return pi2;

    TRACE("GetPrinterW failed with %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pi2);
    return NULL;
}

static PRINTER_INFO_2A * get_printer_infoA(HANDLE hprn)
{
    PRINTER_INFO_2A *pi2 = NULL;
    DWORD needed = 0;
    BOOL res;

    res = GetPrinterA(hprn, 2, NULL, 0, &needed);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        pi2 = HeapAlloc(GetProcessHeap(), 0, needed);
        res = GetPrinterA(hprn, 2, (LPBYTE)pi2, needed, &needed);
    }

    if (res)
        return pi2;

    TRACE("GetPrinterA failed with %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pi2);
    return NULL;
}


/***********************************************************************
 * update_devmode_handle [internal]
 *
 * update a devmode handle for the given DEVMODE, alloc the buffer, when needed
 */
static HGLOBAL update_devmode_handleW(HGLOBAL hdm, DEVMODEW *dm)
{
    SIZE_T size = GlobalSize(hdm);
    LPVOID ptr;

    /* Increase / alloc the global memory block, when needed */
    if ((dm->dmSize + dm->dmDriverExtra) > size) {
        if (hdm)
            hdm = GlobalReAlloc(hdm, dm->dmSize + dm->dmDriverExtra, 0);
        else
            hdm = GlobalAlloc(GMEM_MOVEABLE, dm->dmSize + dm->dmDriverExtra);
    }

    if (hdm) {
        ptr = GlobalLock(hdm);
        if (ptr) {
            memcpy(ptr, dm, dm->dmSize + dm->dmDriverExtra);
            GlobalUnlock(hdm);
        }
        else
        {
            GlobalFree(hdm);
            hdm = NULL;
        }
    }
    return hdm;
}

static HGLOBAL update_devmode_handleA(HGLOBAL hdm, DEVMODEA *dm)
{
    SIZE_T size = GlobalSize(hdm);
    LPVOID ptr;

    /* Increase / alloc the global memory block, when needed */
    if ((dm->dmSize + dm->dmDriverExtra) > size) {
        if (hdm)
            hdm = GlobalReAlloc(hdm, dm->dmSize + dm->dmDriverExtra, 0);
        else
            hdm = GlobalAlloc(GMEM_MOVEABLE, dm->dmSize + dm->dmDriverExtra);
    }

    if (hdm) {
        ptr = GlobalLock(hdm);
        if (ptr) {
            memcpy(ptr, dm, dm->dmSize + dm->dmDriverExtra);
            GlobalUnlock(hdm);
        }
        else
        {
            GlobalFree(hdm);
            hdm = NULL;
        }
    }
    return hdm;
}

/***********************************************************
 * convert_to_devmodeA
 *
 * Creates an ansi copy of supplied devmode
 */
static DEVMODEA *convert_to_devmodeA(const DEVMODEW *dmW)
{
    DEVMODEA *dmA;
    DWORD size;

    if (!dmW) return NULL;
    size = dmW->dmSize - CCHDEVICENAME -
                        ((dmW->dmSize > FIELD_OFFSET(DEVMODEW, dmFormName)) ? CCHFORMNAME : 0);

    dmA = HeapAlloc(GetProcessHeap(), 0, size + dmW->dmDriverExtra);
    if (!dmA) return NULL;

    WideCharToMultiByte(CP_ACP, 0, dmW->dmDeviceName, -1,
                        (LPSTR)dmA->dmDeviceName, CCHDEVICENAME, NULL, NULL);

    if (FIELD_OFFSET(DEVMODEW, dmFormName) >= dmW->dmSize)
    {
        memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
               dmW->dmSize - FIELD_OFFSET(DEVMODEW, dmSpecVersion));
    }
    else
    {
        memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
               FIELD_OFFSET(DEVMODEW, dmFormName) - FIELD_OFFSET(DEVMODEW, dmSpecVersion));
        WideCharToMultiByte(CP_ACP, 0, dmW->dmFormName, -1,
                            (LPSTR)dmA->dmFormName, CCHFORMNAME, NULL, NULL);

        memcpy(&dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize - FIELD_OFFSET(DEVMODEW, dmLogPixels));
    }

    dmA->dmSize = size;
    memcpy((char *)dmA + dmA->dmSize, (const char *)dmW + dmW->dmSize, dmW->dmDriverExtra);
    return dmA;
}

/***********************************************************************
 *    PRINTDLG_OpenDefaultPrinter
 *
 * Returns a winspool printer handle to the default printer in *hprn
 * Caller must call ClosePrinter on the handle
 *
 * Returns TRUE on success else FALSE
 */
static BOOL PRINTDLG_OpenDefaultPrinter(HANDLE *hprn)
{
    WCHAR buf[260];
    DWORD dwBufLen = ARRAY_SIZE(buf);
    BOOL res;
    if(!GetDefaultPrinterW(buf, &dwBufLen))
        return FALSE;
    res = OpenPrinterW(buf, hprn, NULL);
    if (!res)
        WARN("Could not open printer %s\n", debugstr_w(buf));
    return res;
}

/***********************************************************************
 *    PRINTDLG_SetUpPrinterListCombo
 *
 * Initializes printer list combox.
 * hDlg:  HWND of dialog
 * id:    Control id of combo
 * name:  Name of printer to select
 *
 * Initializes combo with list of available printers.  Selects printer 'name'
 * If name is NULL or does not exist select the default printer.
 *
 * Returns number of printers added to list.
 */
static INT PRINTDLG_SetUpPrinterListComboA(HWND hDlg, UINT id, LPCSTR name)
{
    DWORD needed, num;
    INT i;
    LPPRINTER_INFO_2A pi;
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &needed, &num);
    pi = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)pi, needed, &needed,
		  &num);

    SendDlgItemMessageA(hDlg, id, CB_RESETCONTENT, 0, 0);
    
    for(i = 0; i < num; i++) {
        SendDlgItemMessageA(hDlg, id, CB_ADDSTRING, 0,
			    (LPARAM)pi[i].pPrinterName );
    }
    HeapFree(GetProcessHeap(), 0, pi);
    if(!name ||
       (i = SendDlgItemMessageA(hDlg, id, CB_FINDSTRINGEXACT, -1,
				(LPARAM)name)) == CB_ERR) {

        char buf[260];
        DWORD dwBufLen = ARRAY_SIZE(buf);
        if (name != NULL)
            WARN("Can't find %s in printer list so trying to find default\n",
	        debugstr_a(name));
	if(!GetDefaultPrinterA(buf, &dwBufLen))
	    return num;
	i = SendDlgItemMessageA(hDlg, id, CB_FINDSTRINGEXACT, -1, (LPARAM)buf);
	if(i == CB_ERR)
	    FIXME("Can't find default printer in printer list\n");
    }
    SendDlgItemMessageA(hDlg, id, CB_SETCURSEL, i, 0);
    return num;
}

static INT PRINTDLG_SetUpPrinterListComboW(HWND hDlg, UINT id, LPCWSTR name)
{
    DWORD needed, num;
    INT i;
    LPPRINTER_INFO_2W pi;
    EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &needed, &num);
    pi = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE)pi, needed, &needed,
		  &num);

    for(i = 0; i < num; i++) {
        SendDlgItemMessageW(hDlg, id, CB_ADDSTRING, 0,
			    (LPARAM)pi[i].pPrinterName );
    }
    HeapFree(GetProcessHeap(), 0, pi);
    if(!name ||
       (i = SendDlgItemMessageW(hDlg, id, CB_FINDSTRINGEXACT, -1,
				(LPARAM)name)) == CB_ERR) {
        WCHAR buf[260];
        DWORD dwBufLen = ARRAY_SIZE(buf);
        if (name != NULL)
            WARN("Can't find %s in printer list so trying to find default\n",
	        debugstr_w(name));
	if(!GetDefaultPrinterW(buf, &dwBufLen))
	    return num;
	i = SendDlgItemMessageW(hDlg, id, CB_FINDSTRINGEXACT, -1, (LPARAM)buf);
	if(i == CB_ERR)
	    TRACE("Can't find default printer in printer list\n");
    }
    SendDlgItemMessageW(hDlg, id, CB_SETCURSEL, i, 0);
    return num;
}

#ifdef __REACTOS__
static const CHAR  cDriverName[] = "winspool";
static const WCHAR wDriverName[] = L"winspool";
#endif

/***********************************************************************
 *             PRINTDLG_CreateDevNames          [internal]
 *
 *
 *   creates a DevNames structure.
 *
 *  (NB. when we handle unicode the offsets will be in wchars).
 */
static BOOL PRINTDLG_CreateDevNames(HGLOBAL *hmem, const char* DeviceDriverName,
				    const char* DeviceName, const char* OutputPort)
{
    long size;
    char*   pDevNamesSpace;
    char*   pTempPtr;
    LPDEVNAMES lpDevNames;
    char buf[260];
    DWORD dwBufLen = ARRAY_SIZE(buf);
    const char *p;

    p = strrchr( DeviceDriverName, '\\' );
    if (p) DeviceDriverName = p + 1;
#ifndef __REACTOS__
    size = strlen(DeviceDriverName) + 1
#else
    size = strlen(cDriverName) + 1
#endif
            + strlen(DeviceName) + 1
            + strlen(OutputPort) + 1
            + sizeof(DEVNAMES);

    if(*hmem)
        *hmem = GlobalReAlloc(*hmem, size, GMEM_MOVEABLE);
    else
        *hmem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (*hmem == 0)
        return FALSE;

    pDevNamesSpace = GlobalLock(*hmem);
    lpDevNames = (LPDEVNAMES) pDevNamesSpace;

    pTempPtr = pDevNamesSpace + sizeof(DEVNAMES);
#ifndef __REACTOS__
    strcpy(pTempPtr, DeviceDriverName);
#else
    strcpy(pTempPtr, cDriverName);
#endif
    lpDevNames->wDriverOffset = pTempPtr - pDevNamesSpace;

    pTempPtr += strlen(DeviceDriverName) + 1;
    strcpy(pTempPtr, DeviceName);
    lpDevNames->wDeviceOffset = pTempPtr - pDevNamesSpace;

    pTempPtr += strlen(DeviceName) + 1;
    strcpy(pTempPtr, OutputPort);
    lpDevNames->wOutputOffset = pTempPtr - pDevNamesSpace;

    GetDefaultPrinterA(buf, &dwBufLen);
    lpDevNames->wDefault = (strcmp(buf, DeviceName) == 0) ? 1 : 0;
    GlobalUnlock(*hmem);
    return TRUE;
}

static BOOL PRINTDLG_CreateDevNamesW(HGLOBAL *hmem, LPCWSTR DeviceDriverName,
				    LPCWSTR DeviceName, LPCWSTR OutputPort)
{
    long size;
    LPWSTR   pDevNamesSpace;
    LPWSTR   pTempPtr;
    LPDEVNAMES lpDevNames;
    WCHAR bufW[260];
    DWORD dwBufLen = ARRAY_SIZE(bufW);
    const WCHAR *p;

    p = wcsrchr( DeviceDriverName, '\\' );
    if (p) DeviceDriverName = p + 1;
#ifndef __REACTOS__
    size = sizeof(WCHAR)*lstrlenW(DeviceDriverName) + 2
#else
    size = sizeof(WCHAR)*lstrlenW(wDriverName) + 2
#endif
            + sizeof(WCHAR)*lstrlenW(DeviceName) + 2
            + sizeof(WCHAR)*lstrlenW(OutputPort) + 2
            + sizeof(DEVNAMES);

    if(*hmem)
        *hmem = GlobalReAlloc(*hmem, size, GMEM_MOVEABLE);
    else
        *hmem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (*hmem == 0)
        return FALSE;

    pDevNamesSpace = GlobalLock(*hmem);
    lpDevNames = (LPDEVNAMES) pDevNamesSpace;

    pTempPtr = (LPWSTR)((LPDEVNAMES)pDevNamesSpace + 1);
#ifndef __REACTOS__
    lstrcpyW(pTempPtr, DeviceDriverName);
#else
    lstrcpyW(pTempPtr, wDriverName);
#endif
    lpDevNames->wDriverOffset = pTempPtr - pDevNamesSpace;

    pTempPtr += lstrlenW(DeviceDriverName) + 1;
    lstrcpyW(pTempPtr, DeviceName);
    lpDevNames->wDeviceOffset = pTempPtr - pDevNamesSpace;

    pTempPtr += lstrlenW(DeviceName) + 1;
    lstrcpyW(pTempPtr, OutputPort);
    lpDevNames->wOutputOffset = pTempPtr - pDevNamesSpace;

    GetDefaultPrinterW(bufW, &dwBufLen);
    lpDevNames->wDefault = (lstrcmpW(bufW, DeviceName) == 0) ? 1 : 0;
    GlobalUnlock(*hmem);
    return TRUE;
}

/***********************************************************************
 *             PRINTDLG_UpdatePrintDlg          [internal]
 *
 *
 *   updates the PrintDlg structure for return values.
 *
 * RETURNS
 *   FALSE if user is not allowed to close (i.e. wrong nTo or nFrom values)
 *   TRUE  if successful.
 */
static BOOL PRINTDLG_UpdatePrintDlgA(HWND hDlg,
				    PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA       lppd = PrintStructures->lpPrintDlg;
    PDEVMODEA         lpdm = PrintStructures->lpDevMode;
    LPPRINTER_INFO_2A pi = PrintStructures->lpPrinterInfo;


    if(!lpdm) {
	FIXME("No lpdm ptr?\n");
	return FALSE;
    }


    if(!(lppd->Flags & PD_PRINTSETUP)) {
        /* check whether nFromPage and nToPage are within range defined by
	 * nMinPage and nMaxPage
	 */
        if (IsDlgButtonChecked(hDlg, rad3) == BST_CHECKED) { /* Pages */
	    WORD nToPage;
	    WORD nFromPage;
            BOOL translated;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, &translated, FALSE);

	    /* if no ToPage value is entered, use the FromPage value */
	    if(!translated) nToPage = nFromPage;

	    if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
		nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage) {
	        WCHAR resourcestr[256];
		WCHAR resultstr[256];
		LoadStringW(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE, resourcestr, 255);
		wsprintfW(resultstr,resourcestr, lppd->nMinPage, lppd->nMaxPage);
		LoadStringW(COMDLG32_hInstance, PD32_PRINT_TITLE, resourcestr, 255);
		MessageBoxW(hDlg, resultstr, resourcestr, MB_OK | MB_ICONWARNING);
		return FALSE;
	    }
	    lppd->nFromPage = nFromPage;
	    lppd->nToPage   = nToPage;
	    lppd->Flags |= PD_PAGENUMS;
	}
	else
	    lppd->Flags &= ~PD_PAGENUMS;

        if (IsDlgButtonChecked(hDlg, rad2) == BST_CHECKED) /* Selection */
            lppd->Flags |= PD_SELECTION;
        else
            lppd->Flags &= ~PD_SELECTION;

	if (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED) {/* Print to file */
	    static char file[] = "FILE:";
	    lppd->Flags |= PD_PRINTTOFILE;
	    pi->pPortName = file;
	}

	if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED) { /* Collate */
	    FIXME("Collate lppd not yet implemented as output\n");
	}

	/* set PD_Collate and nCopies */
	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /*  The application doesn't support multiple copies or collate...
	   */
	    lppd->Flags &= ~PD_COLLATE;
	    lppd->nCopies = 1;
	  /* if the printer driver supports it... store info there
	   * otherwise no collate & multiple copies !
	   */
	    if (lpdm->dmFields & DM_COLLATE)
	        lpdm->dmCollate =
		  (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED);
	    if (lpdm->dmFields & DM_COPIES)
	        lpdm->u1.s1.dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	} else {
            /* Application is responsible for multiple copies */
	    if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
	        lppd->Flags |= PD_COLLATE;
            else
               lppd->Flags &= ~PD_COLLATE;
            lppd->nCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
            /* multiple copies already included in the document. Driver must print only one copy */
            lpdm->u1.s1.dmCopies = 1;
	}

	/* Print quality, PrintDlg16 */
	if(GetDlgItem(hDlg, cmb1))
	{
	    HWND hQuality = GetDlgItem(hDlg, cmb1);
	    int Sel = SendMessageA(hQuality, CB_GETCURSEL, 0, 0);

	    if(Sel != CB_ERR)
	    {
		LONG dpi = SendMessageA(hQuality, CB_GETITEMDATA, Sel, 0);
		lpdm->dmFields |= DM_PRINTQUALITY | DM_YRESOLUTION;
		lpdm->u1.s1.dmPrintQuality = LOWORD(dpi);
		lpdm->dmYResolution = HIWORD(dpi);
	    }
	}
    }
    return TRUE;
}

static BOOL PRINTDLG_UpdatePrintDlgW(HWND hDlg,
				    PRINT_PTRW* PrintStructures)
{
    LPPRINTDLGW       lppd = PrintStructures->lpPrintDlg;
    PDEVMODEW         lpdm = PrintStructures->lpDevMode;
    LPPRINTER_INFO_2W pi = PrintStructures->lpPrinterInfo;


    if(!lpdm) {
	FIXME("No lpdm ptr?\n");
	return FALSE;
    }


    if(!(lppd->Flags & PD_PRINTSETUP)) {
        /* check whether nFromPage and nToPage are within range defined by
	 * nMinPage and nMaxPage
	 */
        if (IsDlgButtonChecked(hDlg, rad3) == BST_CHECKED) { /* Pages */
	    WORD nToPage;
	    WORD nFromPage;
            BOOL translated;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, &translated, FALSE);

	    /* if no ToPage value is entered, use the FromPage value */
	    if(!translated) nToPage = nFromPage;

	    if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
		nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage) {
	        WCHAR resourcestr[256];
		WCHAR resultstr[256];
                DWORD_PTR args[2];
		LoadStringW(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE,
			    resourcestr, 255);
                args[0] = lppd->nMinPage;
                args[1] = lppd->nMaxPage;
                FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               resourcestr, 0, 0, resultstr,
                               ARRAY_SIZE(resultstr),
                               (__ms_va_list*)args);
		LoadStringW(COMDLG32_hInstance, PD32_PRINT_TITLE,
			    resourcestr, 255);
		MessageBoxW(hDlg, resultstr, resourcestr,
			    MB_OK | MB_ICONWARNING);
		return FALSE;
	    }
	    lppd->nFromPage = nFromPage;
	    lppd->nToPage   = nToPage;
	    lppd->Flags |= PD_PAGENUMS;
	}
	else
	    lppd->Flags &= ~PD_PAGENUMS;

        if (IsDlgButtonChecked(hDlg, rad2) == BST_CHECKED) /* Selection */
            lppd->Flags |= PD_SELECTION;
        else
            lppd->Flags &= ~PD_SELECTION;

	if (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED) {/* Print to file */
	    static WCHAR file[] = {'F','I','L','E',':',0};
	    lppd->Flags |= PD_PRINTTOFILE;
	    pi->pPortName = file;
	}

	if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED) { /* Collate */
	    FIXME("Collate lppd not yet implemented as output\n");
	}

	/* set PD_Collate and nCopies */
	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /*  The application doesn't support multiple copies or collate...
	   */
	    lppd->Flags &= ~PD_COLLATE;
	    lppd->nCopies = 1;
	  /* if the printer driver supports it... store info there
	   * otherwise no collate & multiple copies !
	   */
	    if (lpdm->dmFields & DM_COLLATE)
	        lpdm->dmCollate =
		  (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED);
	    if (lpdm->dmFields & DM_COPIES)
	        lpdm->u1.s1.dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	} else {
	    if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
	        lppd->Flags |= PD_COLLATE;
            else
               lppd->Flags &= ~PD_COLLATE;
            lppd->nCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	}
    }
    return TRUE;
}

/************************************************************************
 * PRINTDLG_SetUpPaperComboBox
 *
 * Initialize either the papersize or inputslot combos of the Printer Setup
 * dialog.  We store the associated word (eg DMPAPER_A4) as the item data.
 * We also try to re-select the old selection.
 */
static BOOL PRINTDLG_SetUpPaperComboBoxA(HWND hDlg,
					int   nIDComboBox,
					char* PrinterName,
					char* PortName,
					LPDEVMODEA dm)
{
    int     i;
    int     NrOfEntries;
    char*   Names;
    WORD*   Words;
    DWORD   Sel, old_Sel;
    WORD    oldWord = 0, newWord = 0; /* DMPAPER_ and DMBIN_ start at 1 */
    int     NamesSize;
    int     fwCapability_Names;
    int     fwCapability_Words;

    TRACE(" Printer: %s, Port: %s, ComboID: %d\n",PrinterName,PortName,nIDComboBox);

    /* query the dialog box for the current selected value */
    Sel = SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETCURSEL, 0, 0);
    if(Sel != CB_ERR) {
        /* we enter here only if a different printer is selected after
         * the Print Setup dialog is opened. The current settings are
         * stored into the newly selected printer.
         */
        oldWord = SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA,
                                      Sel, 0);
        if(oldWord >= DMPAPER_USER) /* DMPAPER_USER == DMBIN_USER */
            oldWord = 0; /* There's no point in trying to keep custom
                            paper / bin sizes across printers */
    }

    if (dm)
        newWord = (nIDComboBox == cmb2) ? dm->u1.s1.dmPaperSize : dm->u1.s1.dmDefaultSource;

    if (nIDComboBox == cmb2) {
         NamesSize          = 64;
         fwCapability_Names = DC_PAPERNAMES;
         fwCapability_Words = DC_PAPERS;
    } else {
         nIDComboBox        = cmb3;
         NamesSize          = 24;
         fwCapability_Names = DC_BINNAMES;
         fwCapability_Words = DC_BINS;
    }

    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
                                      fwCapability_Names, NULL, dm);
    if (NrOfEntries == 0)
         WARN("no Name Entries found!\n");
    else if (NrOfEntries < 0)
         return FALSE;

    if(DeviceCapabilitiesA(PrinterName, PortName, fwCapability_Words, NULL, dm)
       != NrOfEntries) {
        ERR("Number of caps is different\n");
	NrOfEntries = 0;
    }

    Names = HeapAlloc(GetProcessHeap(),0, NrOfEntries*sizeof(char)*NamesSize);
    Words = HeapAlloc(GetProcessHeap(),0, NrOfEntries*sizeof(WORD));
    DeviceCapabilitiesA(PrinterName, PortName, fwCapability_Names, Names, dm);
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
				      fwCapability_Words, (LPSTR)Words, dm);

    /* reset any current content in the combobox */
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);

    /* store new content */
    for (i = 0; i < NrOfEntries; i++) {
        DWORD pos = SendDlgItemMessageA(hDlg, nIDComboBox, CB_ADDSTRING, 0,
					(LPARAM)(&Names[i*NamesSize]) );
	SendDlgItemMessageA(hDlg, nIDComboBox, CB_SETITEMDATA, pos,
			    Words[i]);
    }

    /* Look for old selection or the new default.
       Can't do this is previous loop since item order will change as more items are added */
    Sel = 0;
    old_Sel = NrOfEntries;
    for (i = 0; i < NrOfEntries; i++) {
        if(SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) ==
	   oldWord) {
	    old_Sel = i;
	    break;
	}
        if(SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) == newWord)
	    Sel = i;
    }

    if(old_Sel < NrOfEntries)
    {
        if (dm)
        {
            if(nIDComboBox == cmb2)
                dm->u1.s1.dmPaperSize = oldWord;
            else
                dm->u1.s1.dmDefaultSource = oldWord;
        }
        Sel = old_Sel;
    }

    SendDlgItemMessageA(hDlg, nIDComboBox, CB_SETCURSEL, Sel, 0);

    HeapFree(GetProcessHeap(),0,Words);
    HeapFree(GetProcessHeap(),0,Names);
    return TRUE;
}

static BOOL PRINTDLG_SetUpPaperComboBoxW(HWND hDlg,
					int   nIDComboBox,
					const WCHAR* PrinterName,
					const WCHAR* PortName,
					LPDEVMODEW dm)
{
    int     i;
    int     NrOfEntries;
    WCHAR*  Names;
    WORD*   Words;
    DWORD   Sel, old_Sel;
    WORD    oldWord = 0, newWord = 0; /* DMPAPER_ and DMBIN_ start at 1 */
    int     NamesSize;
    int     fwCapability_Names;
    int     fwCapability_Words;

    TRACE(" Printer: %s, Port: %s, ComboID: %d\n",debugstr_w(PrinterName),debugstr_w(PortName),nIDComboBox);

    /* query the dialog box for the current selected value */
    Sel = SendDlgItemMessageW(hDlg, nIDComboBox, CB_GETCURSEL, 0, 0);
    if(Sel != CB_ERR) {
        /* we enter here only if a different printer is selected after
         * the Print Setup dialog is opened. The current settings are
         * stored into the newly selected printer.
         */
        oldWord = SendDlgItemMessageW(hDlg, nIDComboBox, CB_GETITEMDATA,
                                      Sel, 0);

        if(oldWord >= DMPAPER_USER) /* DMPAPER_USER == DMBIN_USER */
            oldWord = 0; /* There's no point in trying to keep custom
                            paper / bin sizes across printers */
    }

    if (dm)
        newWord = (nIDComboBox == cmb2) ? dm->u1.s1.dmPaperSize : dm->u1.s1.dmDefaultSource;

    if (nIDComboBox == cmb2) {
         NamesSize          = 64;
         fwCapability_Names = DC_PAPERNAMES;
         fwCapability_Words = DC_PAPERS;
    } else {
         nIDComboBox        = cmb3;
         NamesSize          = 24;
         fwCapability_Names = DC_BINNAMES;
         fwCapability_Words = DC_BINS;
    }

    NrOfEntries = DeviceCapabilitiesW(PrinterName, PortName,
                                      fwCapability_Names, NULL, dm);
    if (NrOfEntries == 0)
         WARN("no Name Entries found!\n");
    else if (NrOfEntries < 0)
         return FALSE;

    if(DeviceCapabilitiesW(PrinterName, PortName, fwCapability_Words, NULL, dm)
       != NrOfEntries) {
        ERR("Number of caps is different\n");
	NrOfEntries = 0;
    }

    Names = HeapAlloc(GetProcessHeap(),0, NrOfEntries*sizeof(WCHAR)*NamesSize);
    Words = HeapAlloc(GetProcessHeap(),0, NrOfEntries*sizeof(WORD));
    DeviceCapabilitiesW(PrinterName, PortName, fwCapability_Names, Names, dm);
    NrOfEntries = DeviceCapabilitiesW(PrinterName, PortName,
                                      fwCapability_Words, Words, dm);

    /* reset any current content in the combobox */
    SendDlgItemMessageW(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);

    /* store new content */
    for (i = 0; i < NrOfEntries; i++) {
        DWORD pos = SendDlgItemMessageW(hDlg, nIDComboBox, CB_ADDSTRING, 0,
					(LPARAM)(&Names[i*NamesSize]) );
	SendDlgItemMessageW(hDlg, nIDComboBox, CB_SETITEMDATA, pos,
			    Words[i]);
    }

    /* Look for old selection or the new default.
       Can't do this is previous loop since item order will change as more items are added */
    Sel = 0;
    old_Sel = NrOfEntries;
    for (i = 0; i < NrOfEntries; i++) {
        if(SendDlgItemMessageW(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) ==
	   oldWord) {
	    old_Sel = i;
	    break;
	}
        if(SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) == newWord)
            Sel = i;
    }

    if(old_Sel < NrOfEntries)
    {
        if (dm)
        {
            if(nIDComboBox == cmb2)
                dm->u1.s1.dmPaperSize = oldWord;
            else
                dm->u1.s1.dmDefaultSource = oldWord;
        }
        Sel = old_Sel;
    }

    SendDlgItemMessageW(hDlg, nIDComboBox, CB_SETCURSEL, Sel, 0);

    HeapFree(GetProcessHeap(),0,Words);
    HeapFree(GetProcessHeap(),0,Names);
    return TRUE;
}


/***********************************************************************
 *               PRINTDLG_UpdatePrinterInfoTexts               [internal]
 */
static void PRINTDLG_UpdatePrinterInfoTextsA(HWND hDlg, const PRINTER_INFO_2A *pi)
{
    char   StatusMsg[256];
    char   ResourceString[256];
    int    i;

    /* Status Message */
    StatusMsg[0]='\0';

    /* add all status messages */
    for (i = 0; i < 25; i++) {
        if (pi->Status & (1<<i)) {
	    LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_PAUSED+i,
			ResourceString, 255);
	    strcat(StatusMsg,ResourceString);
        }
    }
    /* append "ready" */
    /* FIXME: status==ready must only be appended if really so.
              but how to detect? */
    LoadStringA(COMDLG32_hInstance, PD32_PRINTER_STATUS_READY,
		ResourceString, 255);
    strcat(StatusMsg,ResourceString);
    SetDlgItemTextA(hDlg, stc12, StatusMsg);

    /* set all other printer info texts */
    SetDlgItemTextA(hDlg, stc11, pi->pDriverName);
    
    if (pi->pLocation != NULL && pi->pLocation[0] != '\0')
        SetDlgItemTextA(hDlg, stc14, pi->pLocation);
    else
        SetDlgItemTextA(hDlg, stc14, pi->pPortName);
    SetDlgItemTextA(hDlg, stc13, pi->pComment ? pi->pComment : "");
    return;
}

static void PRINTDLG_UpdatePrinterInfoTextsW(HWND hDlg, const PRINTER_INFO_2W *pi)
{
    WCHAR   StatusMsg[256];
    WCHAR   ResourceString[256];
    static const WCHAR emptyW[] = {0};
    int    i;

    /* Status Message */
    StatusMsg[0]='\0';

    /* add all status messages */
    for (i = 0; i < 25; i++) {
        if (pi->Status & (1<<i)) {
	    LoadStringW(COMDLG32_hInstance, PD32_PRINTER_STATUS_PAUSED+i,
			ResourceString, 255);
	    lstrcatW(StatusMsg,ResourceString);
        }
    }
    /* append "ready" */
    /* FIXME: status==ready must only be appended if really so.
              but how to detect? */
    LoadStringW(COMDLG32_hInstance, PD32_PRINTER_STATUS_READY,
		ResourceString, 255);
    lstrcatW(StatusMsg,ResourceString);
    SetDlgItemTextW(hDlg, stc12, StatusMsg);

    /* set all other printer info texts */
    SetDlgItemTextW(hDlg, stc11, pi->pDriverName);
    if (pi->pLocation != NULL && pi->pLocation[0] != '\0')
        SetDlgItemTextW(hDlg, stc14, pi->pLocation);
    else
        SetDlgItemTextW(hDlg, stc14, pi->pPortName);
    SetDlgItemTextW(hDlg, stc13, pi->pComment ? pi->pComment : emptyW);
}


/*******************************************************************
 *
 *                 PRINTDLG_ChangePrinter
 *
 */
static BOOL PRINTDLG_ChangePrinterA(HWND hDlg, char *name, PRINT_PTRA *PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    LPDEVMODEA lpdm = NULL;
    LONG dmSize;
    DWORD needed;
    HANDLE hprn;

    HeapFree(GetProcessHeap(),0, PrintStructures->lpPrinterInfo);
    HeapFree(GetProcessHeap(),0, PrintStructures->lpDriverInfo);
    if(!OpenPrinterA(name, &hprn, NULL)) {
        ERR("Can't open printer %s\n", name);
	return FALSE;
    }
    GetPrinterA(hprn, 2, NULL, 0, &needed);
    PrintStructures->lpPrinterInfo = HeapAlloc(GetProcessHeap(),0,needed);
    GetPrinterA(hprn, 2, (LPBYTE)PrintStructures->lpPrinterInfo, needed,
		&needed);
    GetPrinterDriverA(hprn, NULL, 3, NULL, 0, &needed);
    PrintStructures->lpDriverInfo = HeapAlloc(GetProcessHeap(),0,needed);
    if (!GetPrinterDriverA(hprn, NULL, 3, (LPBYTE)PrintStructures->lpDriverInfo,
	    needed, &needed)) {
	ERR("GetPrinterDriverA failed for %s, fix your config!\n",PrintStructures->lpPrinterInfo->pPrinterName);
	return FALSE;
    }
    ClosePrinter(hprn);

    PRINTDLG_UpdatePrinterInfoTextsA(hDlg, PrintStructures->lpPrinterInfo);

    HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
    PrintStructures->lpDevMode = NULL;

    dmSize = DocumentPropertiesA(0, 0, name, NULL, NULL, 0);
    if(dmSize == -1) {
        ERR("DocumentProperties fails on %s\n", debugstr_a(name));
	return FALSE;
    }
    PrintStructures->lpDevMode = HeapAlloc(GetProcessHeap(), 0, dmSize);
    dmSize = DocumentPropertiesA(0, 0, name, PrintStructures->lpDevMode, NULL,
				 DM_OUT_BUFFER);
    if(lppd->hDevMode && (lpdm = GlobalLock(lppd->hDevMode)) &&
			  !lstrcmpA( (LPSTR) lpdm->dmDeviceName,
				     (LPSTR) PrintStructures->lpDevMode->dmDeviceName)) {
      /* Supplied devicemode matches current printer so try to use it */
        DocumentPropertiesA(0, 0, name, PrintStructures->lpDevMode, lpdm,
			    DM_OUT_BUFFER | DM_IN_BUFFER);
    }
    if(lpdm)
        GlobalUnlock(lppd->hDevMode);

    lpdm = PrintStructures->lpDevMode;  /* use this as a shortcut */

    if(!(lppd->Flags & PD_PRINTSETUP)) {
	/* Print range (All/Range/Selection) */
	if(lppd->nFromPage != 0xffff)
	    SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
	if(lppd->nToPage != 0xffff)
	    SetDlgItemInt(hDlg, edt2, lppd->nToPage, FALSE);

	CheckRadioButton(hDlg, rad1, rad3, rad1);		/* default */
	if (lppd->Flags & PD_NOSELECTION)
	    EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
	else
	    if (lppd->Flags & PD_SELECTION)
	        CheckRadioButton(hDlg, rad1, rad3, rad2);
	if (lppd->Flags & PD_NOPAGENUMS) {
	    EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc2),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc3),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
	} else {
	    if (lppd->Flags & PD_PAGENUMS)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}

	/* Collate pages */
	if (lppd->Flags & PD_COLLATE) {
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
				(LPARAM)PrintStructures->hCollateIcon);
	    CheckDlgButton(hDlg, chx2, 1);
	} else {
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
				(LPARAM)PrintStructures->hNoCollateIcon);
	    CheckDlgButton(hDlg, chx2, 0);
	}

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no Collate */
	    if (!(lpdm->dmFields & DM_COLLATE)) {
	        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
		EnableWindow(GetDlgItem(hDlg, ico3), FALSE);
	    }
	}

	/* nCopies */
	{
	  INT copies;
	  if (lppd->hDevMode == 0)
	      copies = lppd->nCopies;
	  else
	      copies = lpdm->u1.s1.dmCopies;
	  if(copies == 0) copies = 1;
	  else if(copies < 0) copies = MAX_COPIES;
	  SetDlgItemInt(hDlg, edt3, copies, FALSE);
	}

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no nCopies */
	    if (!(lpdm->dmFields & DM_COPIES)) {
	        EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc5), FALSE);
	    }
	}

	/* print to file */
	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
	if (lppd->Flags & PD_DISABLEPRINTTOFILE)
            EnableWindow(GetDlgItem(hDlg, chx1), FALSE);
	if (lppd->Flags & PD_HIDEPRINTTOFILE)
            ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);

	/* Fill print quality combo, PrintDlg16 */
	if(GetDlgItem(hDlg, cmb1))
	{
	    DWORD numResolutions = DeviceCapabilitiesA(PrintStructures->lpPrinterInfo->pPrinterName,
						       PrintStructures->lpPrinterInfo->pPortName,
						       DC_ENUMRESOLUTIONS, NULL, lpdm);

	    if(numResolutions != -1)
	    {
		HWND hQuality = GetDlgItem(hDlg, cmb1);
		LONG* Resolutions;
		char buf[255];
		DWORD i;
		int dpiX, dpiY;
		HDC hPrinterDC = CreateDCA(PrintStructures->lpPrinterInfo->pDriverName,
					   PrintStructures->lpPrinterInfo->pPrinterName,
					   0, lpdm);

		Resolutions = HeapAlloc(GetProcessHeap(), 0, numResolutions*sizeof(LONG)*2);
		DeviceCapabilitiesA(PrintStructures->lpPrinterInfo->pPrinterName,
				    PrintStructures->lpPrinterInfo->pPortName,
				    DC_ENUMRESOLUTIONS, (LPSTR)Resolutions, lpdm);

		dpiX = GetDeviceCaps(hPrinterDC, LOGPIXELSX);
		dpiY = GetDeviceCaps(hPrinterDC, LOGPIXELSY);
		DeleteDC(hPrinterDC);

		SendMessageA(hQuality, CB_RESETCONTENT, 0, 0);
		for(i = 0; i < (numResolutions * 2); i += 2)
		{
		    BOOL IsDefault = FALSE;
		    LRESULT Index;

		    if(Resolutions[i] == Resolutions[i+1])
		    {
			if(dpiX == Resolutions[i])
			    IsDefault = TRUE;
			sprintf(buf, "%d dpi", Resolutions[i]);
		    } else
		    {
			if(dpiX == Resolutions[i] && dpiY == Resolutions[i+1])
			    IsDefault = TRUE;
			sprintf(buf, "%d dpi x %d dpi", Resolutions[i], Resolutions[i+1]);
		    }

		    Index = SendMessageA(hQuality, CB_ADDSTRING, 0, (LPARAM)buf);

		    if(IsDefault)
			SendMessageA(hQuality, CB_SETCURSEL, Index, 0);

		    SendMessageA(hQuality, CB_SETITEMDATA, Index, MAKELONG(dpiX,dpiY));
		}
		HeapFree(GetProcessHeap(), 0, Resolutions);
	    }
	}
    } else { /* PD_PRINTSETUP */
      BOOL bPortrait = (lpdm->u1.s1.dmOrientation == DMORIENT_PORTRAIT);

      PRINTDLG_SetUpPaperComboBoxA(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBoxA(hDlg, cmb3,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      CheckRadioButton(hDlg, rad1, rad2, bPortrait ? rad1: rad2);
      SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(bPortrait ? PrintStructures->hPortraitIcon :
                                   PrintStructures->hLandscapeIcon));

    }

    /* help button */
    if ((lppd->Flags & PD_SHOWHELP)==0) {
        /* hide if PD_SHOWHELP not specified */
        ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);
    }
    return TRUE;
}

static BOOL PRINTDLG_ChangePrinterW(HWND hDlg, WCHAR *name,
				   PRINT_PTRW *PrintStructures)
{
    LPPRINTDLGW lppd = PrintStructures->lpPrintDlg;
    LPDEVMODEW lpdm = NULL;
    LONG dmSize;
    DWORD needed;
    HANDLE hprn;

    HeapFree(GetProcessHeap(),0, PrintStructures->lpPrinterInfo);
    HeapFree(GetProcessHeap(),0, PrintStructures->lpDriverInfo);
    if(!OpenPrinterW(name, &hprn, NULL)) {
        ERR("Can't open printer %s\n", debugstr_w(name));
	return FALSE;
    }
    GetPrinterW(hprn, 2, NULL, 0, &needed);
    PrintStructures->lpPrinterInfo = HeapAlloc(GetProcessHeap(),0,needed);
    GetPrinterW(hprn, 2, (LPBYTE)PrintStructures->lpPrinterInfo, needed,
		&needed);
    GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
    PrintStructures->lpDriverInfo = HeapAlloc(GetProcessHeap(),0,needed);
    if (!GetPrinterDriverW(hprn, NULL, 3, (LPBYTE)PrintStructures->lpDriverInfo,
	    needed, &needed)) {
	ERR("GetPrinterDriverA failed for %s, fix your config!\n",debugstr_w(PrintStructures->lpPrinterInfo->pPrinterName));
	return FALSE;
    }
    ClosePrinter(hprn);

    PRINTDLG_UpdatePrinterInfoTextsW(hDlg, PrintStructures->lpPrinterInfo);

    HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
    PrintStructures->lpDevMode = NULL;

    dmSize = DocumentPropertiesW(0, 0, name, NULL, NULL, 0);
    if(dmSize == -1) {
        ERR("DocumentProperties fails on %s\n", debugstr_w(name));
	return FALSE;
    }
    PrintStructures->lpDevMode = HeapAlloc(GetProcessHeap(), 0, dmSize);
    dmSize = DocumentPropertiesW(0, 0, name, PrintStructures->lpDevMode, NULL,
				 DM_OUT_BUFFER);
    if(lppd->hDevMode && (lpdm = GlobalLock(lppd->hDevMode)) &&
			  !lstrcmpW(lpdm->dmDeviceName,
				  PrintStructures->lpDevMode->dmDeviceName)) {
      /* Supplied devicemode matches current printer so try to use it */
        DocumentPropertiesW(0, 0, name, PrintStructures->lpDevMode, lpdm,
			    DM_OUT_BUFFER | DM_IN_BUFFER);
    }
    if(lpdm)
        GlobalUnlock(lppd->hDevMode);

    lpdm = PrintStructures->lpDevMode;  /* use this as a shortcut */

    if(!(lppd->Flags & PD_PRINTSETUP)) {
	/* Print range (All/Range/Selection) */
	if(lppd->nFromPage != 0xffff)
	    SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
	if(lppd->nToPage != 0xffff)
	    SetDlgItemInt(hDlg, edt2, lppd->nToPage, FALSE);

	CheckRadioButton(hDlg, rad1, rad3, rad1);		/* default */
	if (lppd->Flags & PD_NOSELECTION)
	    EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
	else
	    if (lppd->Flags & PD_SELECTION)
	        CheckRadioButton(hDlg, rad1, rad3, rad2);
	if (lppd->Flags & PD_NOPAGENUMS) {
	    EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc2),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
	    EnableWindow(GetDlgItem(hDlg, stc3),FALSE);
	    EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
	} else {
	    if (lppd->Flags & PD_PAGENUMS)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}

	/* Collate pages */
	if (lppd->Flags & PD_COLLATE) {
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
				(LPARAM)PrintStructures->hCollateIcon);
	    CheckDlgButton(hDlg, chx2, 1);
	} else {
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
				(LPARAM)PrintStructures->hNoCollateIcon);
	    CheckDlgButton(hDlg, chx2, 0);
	}

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no Collate */
	    if (!(lpdm->dmFields & DM_COLLATE)) {
	        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
		EnableWindow(GetDlgItem(hDlg, ico3), FALSE);
	    }
	}

	/* nCopies */
	{
	  INT copies;
	  if (lppd->hDevMode == 0)
	      copies = lppd->nCopies;
	  else
	      copies = lpdm->u1.s1.dmCopies;
	  if(copies == 0) copies = 1;
	  else if(copies < 0) copies = MAX_COPIES;
	  SetDlgItemInt(hDlg, edt3, copies, FALSE);
	}

	if (lppd->Flags & PD_USEDEVMODECOPIESANDCOLLATE) {
	  /* if printer doesn't support it: no nCopies */
	    if (!(lpdm->dmFields & DM_COPIES)) {
	        EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
		EnableWindow(GetDlgItem(hDlg, stc5), FALSE);
	    }
	}

	/* print to file */
	CheckDlgButton(hDlg, chx1, (lppd->Flags & PD_PRINTTOFILE) ? 1 : 0);
	if (lppd->Flags & PD_DISABLEPRINTTOFILE)
            EnableWindow(GetDlgItem(hDlg, chx1), FALSE);
	if (lppd->Flags & PD_HIDEPRINTTOFILE)
            ShowWindow(GetDlgItem(hDlg, chx1), SW_HIDE);

    } else { /* PD_PRINTSETUP */
      BOOL bPortrait = (lpdm->u1.s1.dmOrientation == DMORIENT_PORTRAIT);

      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb3,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      CheckRadioButton(hDlg, rad1, rad2, bPortrait ? rad1: rad2);
      SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(bPortrait ? PrintStructures->hPortraitIcon :
                                   PrintStructures->hLandscapeIcon));

    }

    /* help button */
    if ((lppd->Flags & PD_SHOWHELP)==0) {
        /* hide if PD_SHOWHELP not specified */
        ShowWindow(GetDlgItem(hDlg, pshHelp), SW_HIDE);
    }
    return TRUE;
}

 /***********************************************************************
 *           check_printer_setup			[internal]
 */
static LRESULT check_printer_setup(HWND hDlg)
{
    DWORD needed,num;
    WCHAR resourcestr[256],resultstr[256];

    EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &needed, &num);
    if(needed == 0)
    {
          EnumPrintersW(PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL, 0, &needed, &num);
    }
    if(needed > 0)
          return TRUE;
    else
    {
          LoadStringW(COMDLG32_hInstance, PD32_NO_DEVICES,resultstr, 255);
          LoadStringW(COMDLG32_hInstance, PD32_PRINT_TITLE,resourcestr, 255);
          MessageBoxW(hDlg, resultstr, resourcestr,MB_OK | MB_ICONWARNING);
          return FALSE;
    }
}

/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog(HWND hDlg,
				     PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    DEVNAMES *pdn;
    DEVMODEA *pdm;
    char *name = NULL;
    UINT comboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;

    /* load Collate ICONs */
    /* We load these with LoadImage because they are not a standard
       size and we don't want them rescaled */
    PrintStructures->hCollateIcon =
      LoadImageA(COMDLG32_hInstance, "PD32_COLLATE", IMAGE_ICON, 0, 0, 0);
    PrintStructures->hNoCollateIcon =
      LoadImageA(COMDLG32_hInstance, "PD32_NOCOLLATE", IMAGE_ICON, 0, 0, 0);

    /* These can be done with LoadIcon */
    PrintStructures->hPortraitIcon =
      LoadIconA(COMDLG32_hInstance, "PD32_PORTRAIT");
    PrintStructures->hLandscapeIcon =
      LoadIconA(COMDLG32_hInstance, "PD32_LANDSCAPE");

    /* display the collate/no_collate icon */
    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                        (LPARAM)PrintStructures->hNoCollateIcon);

    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0 ||
       PrintStructures->hPortraitIcon == 0 ||
       PrintStructures->hLandscapeIcon == 0) {
        ERR("no icon in resource file\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	EndDialog(hDlg, FALSE);
    }

    /*
     * if lppd->Flags PD_SHOWHELP is specified, a HELPMESGSTRING message
     * must be registered and the Help button must be shown.
     */
    if (lppd->Flags & PD_SHOWHELP) {
        if((PrintStructures->HelpMessageID =
	    RegisterWindowMessageA(HELPMSGSTRINGA)) == 0) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
	    return FALSE;
	}
    } else
        PrintStructures->HelpMessageID = 0;

    if(!(lppd->Flags &PD_PRINTSETUP)) {
        PrintStructures->hwndUpDown =
	  CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER |
			      UDS_NOTHOUSANDS | UDS_ARROWKEYS |
			      UDS_ALIGNRIGHT | UDS_SETBUDDYINT, 0, 0, 0, 0,
			      hDlg, UPDOWN_ID, COMDLG32_hInstance,
			      GetDlgItem(hDlg, edt3), MAX_COPIES, 1, 1);
    }

    /* FIXME: I allow more freedom than either Win95 or WinNT,
     *        which do not agree on what errors should be thrown or not
     *        in case nToPage or nFromPage is out-of-range.
     */
    if (lppd->nMaxPage < lppd->nMinPage)
    	lppd->nMaxPage = lppd->nMinPage;
    if (lppd->nMinPage == lppd->nMaxPage)
    	lppd->Flags |= PD_NOPAGENUMS;
    if (lppd->nToPage < lppd->nMinPage)
        lppd->nToPage = lppd->nMinPage;
    if (lppd->nToPage > lppd->nMaxPage)
        lppd->nToPage = lppd->nMaxPage;
    if (lppd->nFromPage < lppd->nMinPage)
        lppd->nFromPage = lppd->nMinPage;
    if (lppd->nFromPage > lppd->nMaxPage)
        lppd->nFromPage = lppd->nMaxPage;

    /* if we have the combo box, fill it */
    if (GetDlgItem(hDlg,comboID)) {
	/* Fill Combobox
	 */
	pdn = GlobalLock(lppd->hDevNames);
	pdm = GlobalLock(lppd->hDevMode);
	if(pdn)
	    name = (char*)pdn + pdn->wDeviceOffset;
	else if(pdm)
	    name = (char*)pdm->dmDeviceName;
	PRINTDLG_SetUpPrinterListComboA(hDlg, comboID, name);
	if(pdm) GlobalUnlock(lppd->hDevMode);
	if(pdn) GlobalUnlock(lppd->hDevNames);

	/* Now find selected printer and update rest of dlg */
	name = HeapAlloc(GetProcessHeap(),0,256);
	if (GetDlgItemTextA(hDlg, comboID, name, 255))
	    PRINTDLG_ChangePrinterA(hDlg, name, PrintStructures);
	HeapFree(GetProcessHeap(),0,name);
    } else {
	/* else use default printer */
	char name[200];
        DWORD dwBufLen = ARRAY_SIZE(name);
	BOOL ret = GetDefaultPrinterA(name, &dwBufLen);

	if (ret)
	    PRINTDLG_ChangePrinterA(hDlg, name, PrintStructures);
	else
	    FIXME("No default printer found, expect problems!\n");
    }
    return TRUE;
}

static LRESULT PRINTDLG_WMInitDialogW(HWND hDlg,
				     PRINT_PTRW* PrintStructures)
{
    LPPRINTDLGW lppd = PrintStructures->lpPrintDlg;
    DEVNAMES *pdn;
    DEVMODEW *pdm;
    WCHAR *name = NULL;
    UINT comboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;

    /* load Collate ICONs */
    /* We load these with LoadImage because they are not a standard
       size and we don't want them rescaled */
    PrintStructures->hCollateIcon =
      LoadImageW(COMDLG32_hInstance, pd32_collateW, IMAGE_ICON, 0, 0, 0);
    PrintStructures->hNoCollateIcon =
      LoadImageW(COMDLG32_hInstance, pd32_nocollateW, IMAGE_ICON, 0, 0, 0);

    /* These can be done with LoadIcon */
    PrintStructures->hPortraitIcon =
      LoadIconW(COMDLG32_hInstance, pd32_portraitW);
    PrintStructures->hLandscapeIcon =
      LoadIconW(COMDLG32_hInstance, pd32_landscapeW);

    /* display the collate/no_collate icon */
    SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                        (LPARAM)PrintStructures->hNoCollateIcon);

    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0 ||
       PrintStructures->hPortraitIcon == 0 ||
       PrintStructures->hLandscapeIcon == 0) {
        ERR("no icon in resource file\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	EndDialog(hDlg, FALSE);
    }

    /*
     * if lppd->Flags PD_SHOWHELP is specified, a HELPMESGSTRING message
     * must be registered and the Help button must be shown.
     */
    if (lppd->Flags & PD_SHOWHELP) {
        if((PrintStructures->HelpMessageID =
	    RegisterWindowMessageW(HELPMSGSTRINGW)) == 0) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
	    return FALSE;
	}
    } else
        PrintStructures->HelpMessageID = 0;

    if(!(lppd->Flags &PD_PRINTSETUP)) {
        PrintStructures->hwndUpDown =
	  CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER |
			      UDS_NOTHOUSANDS | UDS_ARROWKEYS |
			      UDS_ALIGNRIGHT | UDS_SETBUDDYINT, 0, 0, 0, 0,
			      hDlg, UPDOWN_ID, COMDLG32_hInstance,
			      GetDlgItem(hDlg, edt3), MAX_COPIES, 1, 1);
    }

    /* FIXME: I allow more freedom than either Win95 or WinNT,
     *        which do not agree to what errors should be thrown or not
     *        in case nToPage or nFromPage is out-of-range.
     */
    if (lppd->nMaxPage < lppd->nMinPage)
    	lppd->nMaxPage = lppd->nMinPage;
    if (lppd->nMinPage == lppd->nMaxPage)
    	lppd->Flags |= PD_NOPAGENUMS;
    if (lppd->nToPage < lppd->nMinPage)
        lppd->nToPage = lppd->nMinPage;
    if (lppd->nToPage > lppd->nMaxPage)
        lppd->nToPage = lppd->nMaxPage;
    if (lppd->nFromPage < lppd->nMinPage)
        lppd->nFromPage = lppd->nMinPage;
    if (lppd->nFromPage > lppd->nMaxPage)
        lppd->nFromPage = lppd->nMaxPage;

    /* if we have the combo box, fill it */
    if (GetDlgItem(hDlg,comboID)) {
	/* Fill Combobox
	 */
	pdn = GlobalLock(lppd->hDevNames);
	pdm = GlobalLock(lppd->hDevMode);
	if(pdn)
	    name = (WCHAR*)pdn + pdn->wDeviceOffset;
	else if(pdm)
	    name = pdm->dmDeviceName;
	PRINTDLG_SetUpPrinterListComboW(hDlg, comboID, name);
	if(pdm) GlobalUnlock(lppd->hDevMode);
	if(pdn) GlobalUnlock(lppd->hDevNames);

	/* Now find selected printer and update rest of dlg */
	/* ansi is ok here */
	name = HeapAlloc(GetProcessHeap(),0,256*sizeof(WCHAR));
	if (GetDlgItemTextW(hDlg, comboID, name, 255))
	    PRINTDLG_ChangePrinterW(hDlg, name, PrintStructures);
	HeapFree(GetProcessHeap(),0,name);
    } else {
	/* else use default printer */
	WCHAR name[200];
        DWORD dwBufLen = ARRAY_SIZE(name);
	BOOL ret = GetDefaultPrinterW(name, &dwBufLen);

	if (ret)
	    PRINTDLG_ChangePrinterW(hDlg, name, PrintStructures);
	else
	    FIXME("No default printer found, expect problems!\n");
    }
    return TRUE;
}

/***********************************************************************
 *                              PRINTDLG_WMCommand               [internal]
 */
static LRESULT PRINTDLG_WMCommandA(HWND hDlg, WPARAM wParam,
                                   PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->lpPrintDlg;
    UINT PrinterComboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;
    LPDEVMODEA lpdm = PrintStructures->lpDevMode;

    switch (LOWORD(wParam))  {
    case IDOK:
        TRACE(" OK button was hit\n");
        if (!PRINTDLG_UpdatePrintDlgA(hDlg, PrintStructures)) {
	    FIXME("Update printdlg was not successful!\n");
	    return(FALSE);
	}
	EndDialog(hDlg, TRUE);
	return(TRUE);

    case IDCANCEL:
        TRACE(" CANCEL button was hit\n");
        EndDialog(hDlg, FALSE);
	return(FALSE);

     case pshHelp:
        TRACE(" HELP button was hit\n");
        SendMessageA(lppd->hwndOwner, PrintStructures->HelpMessageID,
        			(WPARAM) hDlg, (LPARAM) lppd);
        break;

     case chx2:                         /* collate pages checkbox */
        if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                                    (LPARAM)PrintStructures->hCollateIcon);
        else
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                                    (LPARAM)PrintStructures->hNoCollateIcon);
        break;
     case edt1:                         /* from page nr editbox */
     case edt2:                         /* to page nr editbox */
        if (HIWORD(wParam)==EN_CHANGE) {
	    WORD nToPage;
	    WORD nFromPage;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
            if (nFromPage != lppd->nFromPage || nToPage != lppd->nToPage)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}
        break;

    case edt3:
        if(HIWORD(wParam) == EN_CHANGE) {
	    INT copies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	    if(copies <= 1)
	        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
	    else
	        EnableWindow(GetDlgItem(hDlg, chx2), TRUE);
	}
	break;

     case psh2:                       /* Properties button */
       {
         HANDLE hPrinter;
         char   PrinterName[256];

         GetDlgItemTextA(hDlg, PrinterComboID, PrinterName, 255);
         if (!OpenPrinterA(PrinterName, &hPrinter, NULL)) {
	     FIXME(" Call to OpenPrinter did not succeed!\n");
	     break;
	 }
	 DocumentPropertiesA(hDlg, hPrinter, PrinterName,
			     PrintStructures->lpDevMode,
			     PrintStructures->lpDevMode,
			     DM_IN_BUFFER | DM_OUT_BUFFER | DM_IN_PROMPT);
	 ClosePrinter(hPrinter);
         break;
       }

    case rad1: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u1.s1.dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u1.s1.dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP, quality combobox in PRINT16 */
	 if (PrinterComboID != LOWORD(wParam)) {
	     break;
	 }
	 /* FALLTHROUGH */
    case cmb4:                         /* Printer combobox */
         if (HIWORD(wParam)==CBN_SELCHANGE) {
	     char   *PrinterName;
	     INT index = SendDlgItemMessageW(hDlg, LOWORD(wParam), CB_GETCURSEL, 0, 0);
	     INT length = SendDlgItemMessageW(hDlg, LOWORD(wParam), CB_GETLBTEXTLEN, index, 0);
	     PrinterName = HeapAlloc(GetProcessHeap(),0,length+1);
	     SendDlgItemMessageA(hDlg, LOWORD(wParam), CB_GETLBTEXT, index, (LPARAM)PrinterName);
	     PRINTDLG_ChangePrinterA(hDlg, PrinterName, PrintStructures);
	     HeapFree(GetProcessHeap(),0,PrinterName);
	 }
	 break;

    case cmb2: /* Papersize */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR) {
	      lpdm->u1.s1.dmPaperSize = SendDlgItemMessageA(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
	      GetDlgItemTextA(hDlg, cmb2, (char *)lpdm->dmFormName, CCHFORMNAME);
	  }
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u1.s1.dmDefaultSource = SendDlgItemMessageA(hDlg, cmb3,
							  CB_GETITEMDATA, Sel,
							  0);
      }
      break;
    }
    if(lppd->Flags & PD_PRINTSETUP) {
        switch (LOWORD(wParam)) {
	case rad1:                         /* orientation */
	case rad2:
	    if (IsDlgButtonChecked(hDlg, rad1) == BST_CHECKED) {
	        if(lpdm->u1.s1.dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->u1.s1.dmOrientation = DMORIENT_PORTRAIT;
                    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
                    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->u1.s1.dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->u1.s1.dmOrientation = DMORIENT_LANDSCAPE;
                    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
                    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
		}
	    }
	    break;
	}
    }
    return FALSE;
}

static LRESULT PRINTDLG_WMCommandW(HWND hDlg, WPARAM wParam,
			           PRINT_PTRW* PrintStructures)
{
    LPPRINTDLGW lppd = PrintStructures->lpPrintDlg;
    UINT PrinterComboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;
    LPDEVMODEW lpdm = PrintStructures->lpDevMode;

    switch (LOWORD(wParam))  {
    case IDOK:
        TRACE(" OK button was hit\n");
        if (!PRINTDLG_UpdatePrintDlgW(hDlg, PrintStructures)) {
	    FIXME("Update printdlg was not successful!\n");
	    return(FALSE);
	}
	EndDialog(hDlg, TRUE);
	return(TRUE);

    case IDCANCEL:
        TRACE(" CANCEL button was hit\n");
        EndDialog(hDlg, FALSE);
	return(FALSE);

     case pshHelp:
        TRACE(" HELP button was hit\n");
        SendMessageW(lppd->hwndOwner, PrintStructures->HelpMessageID,
        			(WPARAM) hDlg, (LPARAM) lppd);
        break;

     case chx2:                         /* collate pages checkbox */
        if (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED)
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                                    (LPARAM)PrintStructures->hCollateIcon);
        else
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, IMAGE_ICON,
                                    (LPARAM)PrintStructures->hNoCollateIcon);
        break;
     case edt1:                         /* from page nr editbox */
     case edt2:                         /* to page nr editbox */
        if (HIWORD(wParam)==EN_CHANGE) {
	    WORD nToPage;
	    WORD nFromPage;
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
            if (nFromPage != lppd->nFromPage || nToPage != lppd->nToPage)
	        CheckRadioButton(hDlg, rad1, rad3, rad3);
	}
        break;

    case edt3:
        if(HIWORD(wParam) == EN_CHANGE) {
	    INT copies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
	    if(copies <= 1)
	        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
	    else
                EnableWindow(GetDlgItem(hDlg, chx2), TRUE);
        }
        break;

     case psh2:                       /* Properties button */
       {
         HANDLE hPrinter;
         WCHAR  PrinterName[256];

         if (!GetDlgItemTextW(hDlg, PrinterComboID, PrinterName, 255)) break;
         if (!OpenPrinterW(PrinterName, &hPrinter, NULL)) {
	     FIXME(" Call to OpenPrinter did not succeed!\n");
	     break;
	 }
	 DocumentPropertiesW(hDlg, hPrinter, PrinterName,
			     PrintStructures->lpDevMode,
			     PrintStructures->lpDevMode,
			     DM_IN_BUFFER | DM_OUT_BUFFER | DM_IN_PROMPT);
	 ClosePrinter(hPrinter);
         break;
       }

    case rad1: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u1.s1.dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u1.s1.dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP */
	 /* FALLTHROUGH */
    case cmb4:                         /* Printer combobox */
         if (HIWORD(wParam)==CBN_SELCHANGE) {
	     WCHAR *PrinterName;
	     INT index = SendDlgItemMessageW(hDlg, LOWORD(wParam), CB_GETCURSEL, 0, 0);
	     INT length = SendDlgItemMessageW(hDlg, LOWORD(wParam), CB_GETLBTEXTLEN, index, 0);

	     PrinterName = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(length+1));
	     SendDlgItemMessageW(hDlg, LOWORD(wParam), CB_GETLBTEXT, index, (LPARAM)PrinterName);
	     PRINTDLG_ChangePrinterW(hDlg, PrinterName, PrintStructures);
	     HeapFree(GetProcessHeap(),0,PrinterName);
	 }
	 break;

    case cmb2: /* Papersize */
      {
	  DWORD Sel = SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR) {
	      lpdm->u1.s1.dmPaperSize = SendDlgItemMessageW(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
	      GetDlgItemTextW(hDlg, cmb2, lpdm->dmFormName, CCHFORMNAME);
	  }
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u1.s1.dmDefaultSource = SendDlgItemMessageW(hDlg, cmb3,
							  CB_GETITEMDATA, Sel,
							  0);
      }
      break;
    }
    if(lppd->Flags & PD_PRINTSETUP) {
        switch (LOWORD(wParam)) {
	case rad1:                         /* orientation */
	case rad2:
	    if (IsDlgButtonChecked(hDlg, rad1) == BST_CHECKED) {
	        if(lpdm->u1.s1.dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->u1.s1.dmOrientation = DMORIENT_PORTRAIT;
                    SendDlgItemMessageW(hDlg, stc10, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
                    SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->u1.s1.dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->u1.s1.dmOrientation = DMORIENT_LANDSCAPE;
                    SendDlgItemMessageW(hDlg, stc10, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
                    SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
		}
	    }
	    break;
	}
    }
    return FALSE;
}

/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
static INT_PTR CALLBACK PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
    PRINT_PTRA* PrintStructures;
    INT_PTR res = FALSE;

    if (uMsg!=WM_INITDIALOG) {
        PrintStructures = GetPropW(hDlg, printdlg_prop);
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRA*) lParam;
        SetPropW(hDlg, printdlg_prop, PrintStructures);
        if(!check_printer_setup(hDlg))
        {
            EndDialog(hDlg,FALSE);
            return FALSE;
        }
	res = PRINTDLG_WMInitDialog(hDlg, PrintStructures);

	if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK)
	    res = PrintStructures->lpPrintDlg->lpfnPrintHook(
		hDlg, uMsg, wParam, (LPARAM)PrintStructures->lpPrintDlg
	    );
	return res;
    }

    if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
        res = PrintStructures->lpPrintDlg->lpfnPrintHook(hDlg,uMsg,wParam,
							 lParam);
	if(res) return res;
    }

    switch (uMsg) {
    case WM_COMMAND:
        return PRINTDLG_WMCommandA(hDlg, wParam, PrintStructures);

    case WM_DESTROY:
	DestroyIcon(PrintStructures->hCollateIcon);
	DestroyIcon(PrintStructures->hNoCollateIcon);
        DestroyIcon(PrintStructures->hPortraitIcon);
        DestroyIcon(PrintStructures->hLandscapeIcon);
	if(PrintStructures->hwndUpDown)
	    DestroyWindow(PrintStructures->hwndUpDown);
        return FALSE;
    }
    return res;
}

static INT_PTR CALLBACK PrintDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
    PRINT_PTRW* PrintStructures;
    INT_PTR res = FALSE;

    if (uMsg!=WM_INITDIALOG) {
        PrintStructures = GetPropW(hDlg, printdlg_prop);
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRW*) lParam;
        SetPropW(hDlg, printdlg_prop, PrintStructures);
        if(!check_printer_setup(hDlg))
        {
            EndDialog(hDlg,FALSE);
            return FALSE;
        }
	res = PRINTDLG_WMInitDialogW(hDlg, PrintStructures);

	if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK)
	    res = PrintStructures->lpPrintDlg->lpfnPrintHook(hDlg, uMsg, wParam, (LPARAM)PrintStructures->lpPrintDlg);
	return res;
    }

    if(PrintStructures->lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
        res = PrintStructures->lpPrintDlg->lpfnPrintHook(hDlg,uMsg,wParam, lParam);
	if(res) return res;
    }

    switch (uMsg) {
    case WM_COMMAND:
        return PRINTDLG_WMCommandW(hDlg, wParam, PrintStructures);

    case WM_DESTROY:
	DestroyIcon(PrintStructures->hCollateIcon);
	DestroyIcon(PrintStructures->hNoCollateIcon);
        DestroyIcon(PrintStructures->hPortraitIcon);
        DestroyIcon(PrintStructures->hLandscapeIcon);
	if(PrintStructures->hwndUpDown)
	    DestroyWindow(PrintStructures->hwndUpDown);
        return FALSE;
    }
    return res;
}

/************************************************************
 *
 *      PRINTDLG_GetDlgTemplate
 *
 */
static HGLOBAL PRINTDLG_GetDlgTemplateA(const PRINTDLGA *lppd)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl;

    if (lppd->Flags & PD_PRINTSETUP) {
	if(lppd->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hSetupTemplate;
	} else if(lppd->Flags & PD_ENABLESETUPTEMPLATE) {
	    hResInfo = FindResourceA(lppd->hInstance,
				     lppd->lpSetupTemplateName, (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32_SETUP",
				     (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    } else {
	if(lppd->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hPrintTemplate;
	} else if(lppd->Flags & PD_ENABLEPRINTTEMPLATE) {
	    hResInfo = FindResourceA(lppd->hInstance,
				     lppd->lpPrintTemplateName,
				     (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceA(COMDLG32_hInstance, "PRINT32",
				     (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    }
    return hDlgTmpl;
}

static HGLOBAL PRINTDLG_GetDlgTemplateW(const PRINTDLGW *lppd)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl;
    static const WCHAR xpsetup[] = { 'P','R','I','N','T','3','2','_','S','E','T','U','P',0};
    static const WCHAR xprint[] = { 'P','R','I','N','T','3','2',0};

    if (lppd->Flags & PD_PRINTSETUP) {
	if(lppd->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hSetupTemplate;
	} else if(lppd->Flags & PD_ENABLESETUPTEMPLATE) {
	    hResInfo = FindResourceW(lppd->hInstance,
				     lppd->lpSetupTemplateName, (LPWSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceW(COMDLG32_hInstance, xpsetup, (LPWSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    } else {
	if(lppd->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hPrintTemplate;
	} else if(lppd->Flags & PD_ENABLEPRINTTEMPLATE) {
	    hResInfo = FindResourceW(lppd->hInstance,
				     lppd->lpPrintTemplateName,
				     (LPWSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
	} else {
	    hResInfo = FindResourceW(COMDLG32_hInstance, xprint, (LPWSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo);
	}
    }
    return hDlgTmpl;
}

/***********************************************************************
 *
 *      PRINTDLG_CreateDC
 *
 */
static BOOL PRINTDLG_CreateDCA(LPPRINTDLGA lppd)
{
    DEVNAMES *pdn = GlobalLock(lppd->hDevNames);
    DEVMODEA *pdm = GlobalLock(lppd->hDevMode);

    if(lppd->Flags & PD_RETURNDC) {
        lppd->hDC = CreateDCA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm );
    } else if(lppd->Flags & PD_RETURNIC) {
        lppd->hDC = CreateICA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm );
    }
    GlobalUnlock(lppd->hDevNames);
    GlobalUnlock(lppd->hDevMode);
    return lppd->hDC != NULL;
}

static BOOL PRINTDLG_CreateDCW(LPPRINTDLGW lppd)
{
    DEVNAMES *pdn = GlobalLock(lppd->hDevNames);
    DEVMODEW *pdm = GlobalLock(lppd->hDevMode);

    if(lppd->Flags & PD_RETURNDC) {
        lppd->hDC = CreateDCW((WCHAR*)pdn + pdn->wDriverOffset,
			      (WCHAR*)pdn + pdn->wDeviceOffset,
			      (WCHAR*)pdn + pdn->wOutputOffset,
			      pdm );
    } else if(lppd->Flags & PD_RETURNIC) {
        lppd->hDC = CreateICW((WCHAR*)pdn + pdn->wDriverOffset,
			      (WCHAR*)pdn + pdn->wDeviceOffset,
			      (WCHAR*)pdn + pdn->wOutputOffset,
			      pdm );
    }
    GlobalUnlock(lppd->hDevNames);
    GlobalUnlock(lppd->hDevMode);
    return lppd->hDC != NULL;
}

/***********************************************************************
 *           PrintDlgA   (COMDLG32.@)
 *
 *  Displays the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *  
 * PARAMS
 *  lppd  [IO] ptr to PRINTDLG32 struct
 * 
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *  
 * BUGS
 *  PrintDlg:
 *  * The Collate Icons do not display, even though they are in the code.
 *  * The Properties Button(s) should call DocumentPropertiesA().
 */

BOOL WINAPI PrintDlgA(LPPRINTDLGA lppd)
{
    BOOL      bRet = FALSE;
    LPVOID    ptr;
    HINSTANCE hInst;

    if (!lppd)
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_INITIALIZATION);
        return FALSE;
    }

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
              "pp. %d-%d, min p %d, max p %d, copies %d, hinst %p\n"
              "flags %08x (%s)\n",
	      lppd, lppd->hwndOwner, lppd->hDevMode, lppd->hDevNames,
	      lppd->nFromPage, lppd->nToPage, lppd->nMinPage, lppd->nMaxPage,
	      lppd->nCopies, lppd->hInstance, lppd->Flags, flagstr);
    }

    if(lppd->lStructSize != sizeof(PRINTDLGA)) {
        WARN("structure size failure!!!\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
	return FALSE;
    }

    if(lppd->Flags & PD_RETURNDEFAULT) {
        PRINTER_INFO_2A *pbuf;
	DRIVER_INFO_3A	*dbuf;
	HANDLE hprn;
	DWORD needed;

	if(lppd->hDevMode || lppd->hDevNames) {
	    WARN("hDevMode or hDevNames non-zero for PD_RETURNDEFAULT\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
	    return FALSE;
	}
        if(!PRINTDLG_OpenDefaultPrinter(&hprn)) {
	    WARN("Can't find default printer\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
	    return FALSE;
	}

	GetPrinterA(hprn, 2, NULL, 0, &needed);
	pbuf = HeapAlloc(GetProcessHeap(), 0, needed);
	GetPrinterA(hprn, 2, (LPBYTE)pbuf, needed, &needed);

	GetPrinterDriverA(hprn, NULL, 3, NULL, 0, &needed);
	dbuf = HeapAlloc(GetProcessHeap(),0,needed);
	if (!GetPrinterDriverA(hprn, NULL, 3, (LPBYTE)dbuf, needed, &needed)) {
	    ERR("GetPrinterDriverA failed, le %d, fix your config for printer %s!\n",
	        GetLastError(),pbuf->pPrinterName);
	    HeapFree(GetProcessHeap(), 0, dbuf);
	    HeapFree(GetProcessHeap(), 0, pbuf);
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
	    return FALSE;
	}
	ClosePrinter(hprn);

	PRINTDLG_CreateDevNames(&(lppd->hDevNames),
				  dbuf->pDriverPath,
				  pbuf->pPrinterName,
				  pbuf->pPortName);
	lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, pbuf->pDevMode->dmSize +
				     pbuf->pDevMode->dmDriverExtra);
	ptr = GlobalLock(lppd->hDevMode);
	memcpy(ptr, pbuf->pDevMode, pbuf->pDevMode->dmSize +
	       pbuf->pDevMode->dmDriverExtra);
	GlobalUnlock(lppd->hDevMode);
	HeapFree(GetProcessHeap(), 0, pbuf);
	HeapFree(GetProcessHeap(), 0, dbuf);
	bRet = TRUE;
    } else {
	HGLOBAL hDlgTmpl;
	PRINT_PTRA *PrintStructures;

    /* load Dialog resources,
     * depending on Flags indicates Print32 or Print32_setup dialog
     */
	hDlgTmpl = PRINTDLG_GetDlgTemplateA(lppd);
	if (!hDlgTmpl) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
	ptr = LockResource( hDlgTmpl );
	if (!ptr) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}

        PrintStructures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				    sizeof(PRINT_PTRA));
	PrintStructures->lpPrintDlg = lppd;

	/* and create & process the dialog .
	 * -1 is failure, 0 is broken hwnd, everything else is ok.
	 */
        hInst = COMDLG32_hInstance;
	if (lppd->Flags & (PD_ENABLESETUPTEMPLATE | PD_ENABLEPRINTTEMPLATE)) hInst = lppd->hInstance;
	bRet = (0<DialogBoxIndirectParamA(hInst, ptr, lppd->hwndOwner,
					   PrintDlgProcA,
					   (LPARAM)PrintStructures));

	if(bRet) {
	    DEVMODEA *lpdm = PrintStructures->lpDevMode, *lpdmReturn;
	    PRINTER_INFO_2A *pi = PrintStructures->lpPrinterInfo;
	    DRIVER_INFO_3A *di = PrintStructures->lpDriverInfo;

	    if (lppd->hDevMode == 0) {
	        TRACE(" No hDevMode yet... Need to create my own\n");
		lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE,
					lpdm->dmSize + lpdm->dmDriverExtra);
	    } else {
		lppd->hDevMode = GlobalReAlloc(lppd->hDevMode,
					       lpdm->dmSize + lpdm->dmDriverExtra,
					       GMEM_MOVEABLE);
	    }
	    lpdmReturn = GlobalLock(lppd->hDevMode);
	    memcpy(lpdmReturn, lpdm, lpdm->dmSize + lpdm->dmDriverExtra);

	    PRINTDLG_CreateDevNames(&(lppd->hDevNames),
		    di->pDriverPath,
		    pi->pPrinterName,
		    pi->pPortName
	    );
	    GlobalUnlock(lppd->hDevMode);
	}
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpPrinterInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDriverInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures);
    }
    if(bRet && (lppd->Flags & PD_RETURNDC || lppd->Flags & PD_RETURNIC))
        bRet = PRINTDLG_CreateDCA(lppd);

    TRACE("exit! (%d)\n", bRet);
    return bRet;
}

/***********************************************************************
 *           PrintDlgW   (COMDLG32.@)
 *
 * See PrintDlgA.
 */
BOOL WINAPI PrintDlgW(LPPRINTDLGW lppd)
{
    BOOL      bRet = FALSE;
    LPVOID    ptr;
    HINSTANCE hInst;

    if (!lppd)
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_INITIALIZATION);
        return FALSE;
    }

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
              "pp. %d-%d, min p %d, max p %d, copies %d, hinst %p\n"
              "flags %08x (%s)\n",
	      lppd, lppd->hwndOwner, lppd->hDevMode, lppd->hDevNames,
	      lppd->nFromPage, lppd->nToPage, lppd->nMinPage, lppd->nMaxPage,
	      lppd->nCopies, lppd->hInstance, lppd->Flags, flagstr);
    }

    if(lppd->lStructSize != sizeof(PRINTDLGW)) {
        WARN("structure size failure!!!\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
	return FALSE;
    }

    if(lppd->Flags & PD_RETURNDEFAULT) {
        PRINTER_INFO_2W *pbuf;
	DRIVER_INFO_3W	*dbuf;
	HANDLE hprn;
	DWORD needed;

	if(lppd->hDevMode || lppd->hDevNames) {
	    WARN("hDevMode or hDevNames non-zero for PD_RETURNDEFAULT\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
	    return FALSE;
	}
        if(!PRINTDLG_OpenDefaultPrinter(&hprn)) {
	    WARN("Can't find default printer\n");
	    COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
	    return FALSE;
	}

	GetPrinterW(hprn, 2, NULL, 0, &needed);
	pbuf = HeapAlloc(GetProcessHeap(), 0, needed);
	GetPrinterW(hprn, 2, (LPBYTE)pbuf, needed, &needed);

	GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
	dbuf = HeapAlloc(GetProcessHeap(),0,needed);
	if (!GetPrinterDriverW(hprn, NULL, 3, (LPBYTE)dbuf, needed, &needed)) {
	    ERR("GetPrinterDriverA failed, le %d, fix your config for printer %s!\n",
	        GetLastError(),debugstr_w(pbuf->pPrinterName));
	    HeapFree(GetProcessHeap(), 0, dbuf);
	    HeapFree(GetProcessHeap(), 0, pbuf);
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
	    return FALSE;
	}
	ClosePrinter(hprn);

	PRINTDLG_CreateDevNamesW(&(lppd->hDevNames),
				  dbuf->pDriverPath,
				  pbuf->pPrinterName,
				  pbuf->pPortName);
	lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, pbuf->pDevMode->dmSize +
				     pbuf->pDevMode->dmDriverExtra);
	ptr = GlobalLock(lppd->hDevMode);
	memcpy(ptr, pbuf->pDevMode, pbuf->pDevMode->dmSize +
	       pbuf->pDevMode->dmDriverExtra);
	GlobalUnlock(lppd->hDevMode);
	HeapFree(GetProcessHeap(), 0, pbuf);
	HeapFree(GetProcessHeap(), 0, dbuf);
	bRet = TRUE;
    } else {
	HGLOBAL hDlgTmpl;
	PRINT_PTRW *PrintStructures;

    /* load Dialog resources,
     * depending on Flags indicates Print32 or Print32_setup dialog
     */
	hDlgTmpl = PRINTDLG_GetDlgTemplateW(lppd);
	if (!hDlgTmpl) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
	ptr = LockResource( hDlgTmpl );
	if (!ptr) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}

        PrintStructures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				    sizeof(PRINT_PTRW));
	PrintStructures->lpPrintDlg = lppd;

	/* and create & process the dialog .
	 * -1 is failure, 0 is broken hwnd, everything else is ok.
	 */
        hInst = COMDLG32_hInstance;
	if (lppd->Flags & (PD_ENABLESETUPTEMPLATE | PD_ENABLEPRINTTEMPLATE)) hInst = lppd->hInstance;
	bRet = (0<DialogBoxIndirectParamW(hInst, ptr, lppd->hwndOwner,
					   PrintDlgProcW,
					   (LPARAM)PrintStructures));

	if(bRet) {
	    DEVMODEW *lpdm = PrintStructures->lpDevMode, *lpdmReturn;
	    PRINTER_INFO_2W *pi = PrintStructures->lpPrinterInfo;
	    DRIVER_INFO_3W *di = PrintStructures->lpDriverInfo;

	    if (lppd->hDevMode == 0) {
	        TRACE(" No hDevMode yet... Need to create my own\n");
		lppd->hDevMode = GlobalAlloc(GMEM_MOVEABLE,
					lpdm->dmSize + lpdm->dmDriverExtra);
	    } else {
	        WORD locks;
		if((locks = (GlobalFlags(lppd->hDevMode) & GMEM_LOCKCOUNT))) {
		    WARN("hDevMode has %d locks on it. Unlocking it now\n", locks);
		    while(locks--) {
		        GlobalUnlock(lppd->hDevMode);
			TRACE("Now got %d locks\n", locks);
		    }
		}
		lppd->hDevMode = GlobalReAlloc(lppd->hDevMode,
					       lpdm->dmSize + lpdm->dmDriverExtra,
					       GMEM_MOVEABLE);
	    }
	    lpdmReturn = GlobalLock(lppd->hDevMode);
	    memcpy(lpdmReturn, lpdm, lpdm->dmSize + lpdm->dmDriverExtra);

	    if (lppd->hDevNames != 0) {
	        WORD locks;
		if((locks = (GlobalFlags(lppd->hDevNames) & GMEM_LOCKCOUNT))) {
		    WARN("hDevNames has %d locks on it. Unlocking it now\n", locks);
		    while(locks--)
		        GlobalUnlock(lppd->hDevNames);
		}
	    }
	    PRINTDLG_CreateDevNamesW(&(lppd->hDevNames),
		    di->pDriverPath,
		    pi->pPrinterName,
		    pi->pPortName
	    );
	    GlobalUnlock(lppd->hDevMode);
	}
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpPrinterInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDriverInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures);
    }
    if(bRet && (lppd->Flags & PD_RETURNDC || lppd->Flags & PD_RETURNIC))
        bRet = PRINTDLG_CreateDCW(lppd);

    TRACE("exit! (%d)\n", bRet);
    return bRet;
}

/***********************************************************************
 *
 *          PageSetupDlg
 * rad1 - portrait
 * rad2 - landscape
 * cmb1 - printer select (not in standard dialog template)
 * cmb2 - paper size
 * cmb3 - source (tray?)
 * edt4 - border left
 * edt5 - border top
 * edt6 - border right
 * edt7 - border bottom
 * psh3 - "Printer..."
 */

typedef struct
{
    BOOL unicode;
    union
    {
        LPPAGESETUPDLGA dlga;
        LPPAGESETUPDLGW dlgw;
    } u;
    HWND hDlg;                /* Page Setup dialog handle */
    RECT rtDrawRect;          /* Drawing rect for page */
} pagesetup_data;

static inline DWORD pagesetup_get_flags(const pagesetup_data *data)
{
    return data->u.dlgw->Flags;
}

static inline BOOL is_metric(const pagesetup_data *data)
{
    return pagesetup_get_flags(data) & PSD_INHUNDREDTHSOFMILLIMETERS;
}

static inline LONG tenths_mm_to_size(const pagesetup_data *data, LONG size)
{
    if (is_metric(data))
        return 10 * size;
    else
        return 10 * size * 100 / 254;
}

static inline LONG thousandths_inch_to_size(const pagesetup_data *data, LONG size)
{
    if (is_metric(data))
        return size * 254 / 100;
    else
        return size;
}

static WCHAR get_decimal_sep(void)
{
    static WCHAR sep;

    if(!sep)
    {
        WCHAR buf[] = {'.', 0};
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, ARRAY_SIZE(buf));
        sep = buf[0];
    }
    return sep;
}

static void size2str(const pagesetup_data *data, DWORD size, LPWSTR strout)
{
    static const WCHAR integer_fmt[] = {'%','d',0};
    static const WCHAR hundredths_fmt[] = {'%','d','%','c','%','0','2','d',0};
    static const WCHAR thousandths_fmt[] = {'%','d','%','c','%','0','3','d',0};

    /* FIXME use LOCALE_SDECIMAL when the edit parsing code can cope */

    if (is_metric(data))
    {
        if(size % 100)
            wsprintfW(strout, hundredths_fmt, size / 100, get_decimal_sep(), size % 100);
        else
            wsprintfW(strout, integer_fmt, size / 100);
    }
    else
    {
        if(size % 1000)
            wsprintfW(strout, thousandths_fmt, size / 1000, get_decimal_sep(), size % 1000);
        else
            wsprintfW(strout, integer_fmt, size / 1000);

    }
}

static inline BOOL is_default_metric(void)
{
    DWORD system;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER,
                   (LPWSTR)&system, sizeof(system));
    return system == 0;
}

/**********************************************
 *           rotate_rect
 * Cyclically permute the four members of rc
 * If sense is TRUE l -> t -> r -> b
 * otherwise        l <- t <- r <- b
 */
static inline void rotate_rect(RECT *rc, BOOL sense)
{
    INT tmp;
    if(sense)
    {
        tmp        = rc->bottom;
        rc->bottom = rc->right;
        rc->right  = rc->top;
        rc->top    = rc->left;
        rc->left   = tmp;
    }
    else
    {
        tmp        = rc->left;
        rc->left   = rc->top;
        rc->top    = rc->right;
        rc->right  = rc->bottom;
        rc->bottom = tmp;
    }
}

static void pagesetup_set_orientation(pagesetup_data *data, WORD orient)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);

    assert(orient == DMORIENT_PORTRAIT || orient == DMORIENT_LANDSCAPE);

    if(data->unicode)
        dm->u1.s1.dmOrientation = orient;
    else
    {
        DEVMODEA *dmA = (DEVMODEA *)dm;
        dmA->u1.s1.dmOrientation = orient;
    }
    GlobalUnlock(data->u.dlgw->hDevMode);
}

static WORD pagesetup_get_orientation(const pagesetup_data *data)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);
    WORD orient;

    if(data->unicode)
        orient = dm->u1.s1.dmOrientation;
    else
    {
        DEVMODEA *dmA = (DEVMODEA *)dm;
        orient = dmA->u1.s1.dmOrientation;
    }
    GlobalUnlock(data->u.dlgw->hDevMode);
    return orient;
}

static void pagesetup_set_papersize(pagesetup_data *data, WORD paper)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);

    if(data->unicode)
        dm->u1.s1.dmPaperSize = paper;
    else
    {
        DEVMODEA *dmA = (DEVMODEA *)dm;
        dmA->u1.s1.dmPaperSize = paper;
    }
    GlobalUnlock(data->u.dlgw->hDevMode);
}

static WORD pagesetup_get_papersize(const pagesetup_data *data)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);
    WORD paper;

    if(data->unicode)
        paper = dm->u1.s1.dmPaperSize;
    else
    {
        DEVMODEA *dmA = (DEVMODEA *)dm;
        paper = dmA->u1.s1.dmPaperSize;
    }
    GlobalUnlock(data->u.dlgw->hDevMode);
    return paper;
}

static void pagesetup_set_defaultsource(pagesetup_data *data, WORD source)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);

    if(data->unicode)
        dm->u1.s1.dmDefaultSource = source;
    else
    {
        DEVMODEA *dmA = (DEVMODEA *)dm;
        dmA->u1.s1.dmDefaultSource = source;
    }
    GlobalUnlock(data->u.dlgw->hDevMode);
}

typedef enum
{
    devnames_driver_name,
    devnames_device_name,
    devnames_output_name
} devnames_name;


static inline WORD get_devname_offset(const DEVNAMES *dn, devnames_name which)
{
    switch(which)
    {
    case devnames_driver_name: return dn->wDriverOffset;
    case devnames_device_name: return dn->wDeviceOffset;
    case devnames_output_name: return dn->wOutputOffset;
    }
    ERR("Shouldn't be here\n");
    return 0;
}

static WCHAR *pagesetup_get_a_devname(const pagesetup_data *data, devnames_name which)
{
    DEVNAMES *dn;
    WCHAR *name;

    dn = GlobalLock(data->u.dlgw->hDevNames);
    if(data->unicode)
        name = strdupW((WCHAR *)dn + get_devname_offset(dn, which));
    else
    {
        int len = MultiByteToWideChar(CP_ACP, 0, (char*)dn + get_devname_offset(dn, which), -1, NULL, 0);
        name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, (char*)dn + get_devname_offset(dn, which), -1, name, len);
    }
    GlobalUnlock(data->u.dlgw->hDevNames);
    return name;
}

static WCHAR *pagesetup_get_drvname(const pagesetup_data *data)
{
    return pagesetup_get_a_devname(data, devnames_driver_name);
}

static WCHAR *pagesetup_get_devname(const pagesetup_data *data)
{
    return pagesetup_get_a_devname(data, devnames_device_name);
}

static WCHAR *pagesetup_get_portname(const pagesetup_data *data)
{
    return pagesetup_get_a_devname(data, devnames_output_name);
}

static void pagesetup_release_a_devname(const pagesetup_data *data, WCHAR *name)
{
    HeapFree(GetProcessHeap(), 0, name);
}

static void pagesetup_set_devnames(pagesetup_data *data, LPCWSTR drv, LPCWSTR devname, LPCWSTR port)
{
    DEVNAMES *dn;
    WCHAR def[256];
    DWORD len = sizeof(DEVNAMES), drv_len, dev_len, port_len;

    if(data->unicode)
    {
        drv_len  = (lstrlenW(drv) + 1) * sizeof(WCHAR);
        dev_len  = (lstrlenW(devname) + 1) * sizeof(WCHAR);
        port_len = (lstrlenW(port) + 1) * sizeof(WCHAR);
    }
    else
    {
        drv_len = WideCharToMultiByte(CP_ACP, 0, drv, -1, NULL, 0, NULL, NULL);
        dev_len = WideCharToMultiByte(CP_ACP, 0, devname, -1, NULL, 0, NULL, NULL);
        port_len = WideCharToMultiByte(CP_ACP, 0, port, -1, NULL, 0, NULL, NULL);
    }
    len += drv_len + dev_len + port_len;

    if(data->u.dlgw->hDevNames)
        data->u.dlgw->hDevNames = GlobalReAlloc(data->u.dlgw->hDevNames, len, GMEM_MOVEABLE);
    else
        data->u.dlgw->hDevNames = GlobalAlloc(GMEM_MOVEABLE, len);

    dn = GlobalLock(data->u.dlgw->hDevNames);

    if(data->unicode)
    {
        WCHAR *ptr = (WCHAR *)(dn + 1);
        len = sizeof(DEVNAMES) / sizeof(WCHAR);
        dn->wDriverOffset = len;
        lstrcpyW(ptr, drv);
        ptr += drv_len / sizeof(WCHAR);
        len += drv_len / sizeof(WCHAR);
        dn->wDeviceOffset = len;
        lstrcpyW(ptr, devname);
        ptr += dev_len / sizeof(WCHAR);
        len += dev_len / sizeof(WCHAR);
        dn->wOutputOffset = len;
        lstrcpyW(ptr, port);
    }
    else
    {
        char *ptr = (char *)(dn + 1);
        len = sizeof(DEVNAMES);
        dn->wDriverOffset = len;
        WideCharToMultiByte(CP_ACP, 0, drv, -1, ptr, drv_len, NULL, NULL);
        ptr += drv_len;
        len += drv_len;
        dn->wDeviceOffset = len;
        WideCharToMultiByte(CP_ACP, 0, devname, -1, ptr, dev_len, NULL, NULL);
        ptr += dev_len;
        len += dev_len;
        dn->wOutputOffset = len;
        WideCharToMultiByte(CP_ACP, 0, port, -1, ptr, port_len, NULL, NULL);
    }

    dn->wDefault = 0;
    len = ARRAY_SIZE(def);
    GetDefaultPrinterW(def, &len);
    if(!lstrcmpW(def, devname))
        dn->wDefault = 1;

    GlobalUnlock(data->u.dlgw->hDevNames);
}

static DEVMODEW *pagesetup_get_devmode(const pagesetup_data *data)
{
    DEVMODEW *dm = GlobalLock(data->u.dlgw->hDevMode);
    DEVMODEW *ret;

    if(data->unicode)
    {
        /* We make a copy even in the unicode case because the ptr
           may get passed back to us in pagesetup_set_devmode. */
        ret = HeapAlloc(GetProcessHeap(), 0, dm->dmSize + dm->dmDriverExtra);
        memcpy(ret, dm, dm->dmSize + dm->dmDriverExtra);
    }
    else
        ret = GdiConvertToDevmodeW((DEVMODEA *)dm);

    GlobalUnlock(data->u.dlgw->hDevMode);
    return ret;
}

static void pagesetup_release_devmode(const pagesetup_data *data, DEVMODEW *dm)
{
    HeapFree(GetProcessHeap(), 0, dm);
}

static void pagesetup_set_devmode(pagesetup_data *data, DEVMODEW *dm)
{
    DEVMODEA *dmA = NULL;
    void *src, *dst;
    DWORD size;

    if(data->unicode)
    {
        size = dm->dmSize + dm->dmDriverExtra;
        src = dm;
    }
    else
    {
        dmA = convert_to_devmodeA(dm);
        size = dmA->dmSize + dmA->dmDriverExtra;
        src = dmA;
    }

    if(data->u.dlgw->hDevMode)
        data->u.dlgw->hDevMode = GlobalReAlloc(data->u.dlgw->hDevMode, size,
                                               GMEM_MOVEABLE);
    else
        data->u.dlgw->hDevMode = GlobalAlloc(GMEM_MOVEABLE, size);

    dst = GlobalLock(data->u.dlgw->hDevMode);
    memcpy(dst, src, size);
    GlobalUnlock(data->u.dlgw->hDevMode);
    HeapFree(GetProcessHeap(), 0, dmA);
}

static inline POINT *pagesetup_get_papersize_pt(const pagesetup_data *data)
{
    return &data->u.dlgw->ptPaperSize;
}

static inline RECT *pagesetup_get_margin_rect(const pagesetup_data *data)
{
    return &data->u.dlgw->rtMargin;
}

typedef enum
{
    page_setup_hook,
    page_paint_hook
} hook_type;

static inline LPPAGESETUPHOOK pagesetup_get_hook(const pagesetup_data *data, hook_type which)
{
    switch(which)
    {
    case page_setup_hook: return data->u.dlgw->lpfnPageSetupHook;
    case page_paint_hook: return data->u.dlgw->lpfnPagePaintHook;
    }
    return NULL;
}

/* This should only be used in calls to hook procs so we return the ptr
   already cast to LPARAM */
static inline LPARAM pagesetup_get_dlg_struct(const pagesetup_data *data)
{
    return (LPARAM)data->u.dlgw;
}

static inline void swap_point(POINT *pt)
{
    LONG tmp = pt->x;
    pt->x = pt->y;
    pt->y = tmp;
}

static BOOL pagesetup_update_papersize(pagesetup_data *data)
{
    DEVMODEW *dm;
    LPWSTR devname, portname;
    int i, num;
    WORD *words = NULL, paperword;
    POINT *points = NULL;
    BOOL retval = FALSE;

    dm       = pagesetup_get_devmode(data);
    devname  = pagesetup_get_devname(data);
    portname = pagesetup_get_portname(data);

    num = DeviceCapabilitiesW(devname, portname, DC_PAPERS, NULL, dm);
    if (num <= 0)
    {
        FIXME("No papernames found for %s/%s\n", debugstr_w(devname), debugstr_w(portname));
        goto end;
    }

    words = HeapAlloc(GetProcessHeap(), 0, num * sizeof(WORD));
    points = HeapAlloc(GetProcessHeap(), 0, num * sizeof(POINT));

    if (num != DeviceCapabilitiesW(devname, portname, DC_PAPERS, (LPWSTR)words, dm))
    {
        FIXME("Number of returned words is not %d\n", num);
        goto end;
    }

    if (num != DeviceCapabilitiesW(devname, portname, DC_PAPERSIZE, (LPWSTR)points, dm))
    {
        FIXME("Number of returned sizes is not %d\n", num);
        goto end;
    }

    paperword = pagesetup_get_papersize(data);

    for (i = 0; i < num; i++)
        if (words[i] == paperword)
            break;

    if (i == num)
    {
        FIXME("Papersize %d not found in list?\n", paperword);
        goto end;
    }

    /* this is _10ths_ of a millimeter */
    pagesetup_get_papersize_pt(data)->x = tenths_mm_to_size(data, points[i].x);
    pagesetup_get_papersize_pt(data)->y = tenths_mm_to_size(data, points[i].y);

    if(pagesetup_get_orientation(data) == DMORIENT_LANDSCAPE)
        swap_point(pagesetup_get_papersize_pt(data));

    retval = TRUE;

end:
    HeapFree(GetProcessHeap(), 0, words);
    HeapFree(GetProcessHeap(), 0, points);
    pagesetup_release_a_devname(data, portname);
    pagesetup_release_a_devname(data, devname);
    pagesetup_release_devmode(data, dm);

    return retval;
}

/**********************************************************************************************
 * pagesetup_change_printer
 *
 * Redefines hDevMode and hDevNames HANDLES and initialises it.
 * 
 */
static BOOL pagesetup_change_printer(LPWSTR name, pagesetup_data *data)
{
    HANDLE hprn;
    DWORD needed;
    PRINTER_INFO_2W *prn_info = NULL;
    DRIVER_INFO_3W *drv_info = NULL;
    DEVMODEW *dm = NULL;
    BOOL retval = FALSE;

    if(!OpenPrinterW(name, &hprn, NULL))
    {
        ERR("Can't open printer %s\n", debugstr_w(name));
        goto end;
    }

    GetPrinterW(hprn, 2, NULL, 0, &needed);
    prn_info = HeapAlloc(GetProcessHeap(), 0, needed);
    GetPrinterW(hprn, 2, (LPBYTE)prn_info, needed, &needed);
    GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
    drv_info = HeapAlloc(GetProcessHeap(), 0, needed);
    if(!GetPrinterDriverW(hprn, NULL, 3, (LPBYTE)drv_info, needed, &needed))
    {
        ERR("GetPrinterDriverA failed for %s, fix your config!\n", debugstr_w(prn_info->pPrinterName));
        goto end;
    }
    ClosePrinter(hprn);

    needed = DocumentPropertiesW(0, 0, name, NULL, NULL, 0);
    if(needed == -1)
    {
        ERR("DocumentProperties fails on %s\n", debugstr_w(name));
        goto end;
    }

    dm = HeapAlloc(GetProcessHeap(), 0, needed);
    DocumentPropertiesW(0, 0, name, dm, NULL, DM_OUT_BUFFER);

    pagesetup_set_devmode(data, dm);
    pagesetup_set_devnames(data, drv_info->pDriverPath, prn_info->pPrinterName,
                           prn_info->pPortName);

    retval = TRUE;
end:
    HeapFree(GetProcessHeap(), 0, dm);
    HeapFree(GetProcessHeap(), 0, prn_info);
    HeapFree(GetProcessHeap(), 0, drv_info);
    return retval;
}

/****************************************************************************************
 *  pagesetup_init_combos
 *
 *  Fills Printers, Paper and Source combos
 *
 */
static void pagesetup_init_combos(HWND hDlg, pagesetup_data *data)
{
    DEVMODEW *dm;
    LPWSTR devname, portname;

    dm       = pagesetup_get_devmode(data);
    devname  = pagesetup_get_devname(data);
    portname = pagesetup_get_portname(data);

    PRINTDLG_SetUpPrinterListComboW(hDlg, cmb1, devname);
    PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb2, devname, portname, dm);
    PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb3, devname, portname, dm);

    pagesetup_release_a_devname(data, portname);
    pagesetup_release_a_devname(data, devname);
    pagesetup_release_devmode(data, dm);
}


/****************************************************************************************
 *  pagesetup_change_printer_dialog
 *
 *  Pops up another dialog that lets the user pick another printer.
 *
 *  For now we display the PrintDlg, this should display a striped down version of it.
 */
static void pagesetup_change_printer_dialog(HWND hDlg, pagesetup_data *data)
{
    PRINTDLGW prnt;
    LPWSTR drvname, devname, portname;
    DEVMODEW *tmp_dm, *dm;

    memset(&prnt, 0, sizeof(prnt));
    prnt.lStructSize = sizeof(prnt);
    prnt.Flags     = 0;
    prnt.hwndOwner = hDlg;

    drvname = pagesetup_get_drvname(data);
    devname = pagesetup_get_devname(data);
    portname = pagesetup_get_portname(data);
    prnt.hDevNames = 0;
    PRINTDLG_CreateDevNamesW(&prnt.hDevNames, drvname, devname, portname);
    pagesetup_release_a_devname(data, portname);
    pagesetup_release_a_devname(data, devname);
    pagesetup_release_a_devname(data, drvname);

    tmp_dm = pagesetup_get_devmode(data);
    prnt.hDevMode = GlobalAlloc(GMEM_MOVEABLE, tmp_dm->dmSize + tmp_dm->dmDriverExtra);
    dm = GlobalLock(prnt.hDevMode);
    memcpy(dm, tmp_dm, tmp_dm->dmSize + tmp_dm->dmDriverExtra);
    GlobalUnlock(prnt.hDevMode);
    pagesetup_release_devmode(data, tmp_dm);

    if (PrintDlgW(&prnt))
    {
        DEVMODEW *dm = GlobalLock(prnt.hDevMode);
        DEVNAMES *dn = GlobalLock(prnt.hDevNames);

        pagesetup_set_devnames(data, (WCHAR*)dn + dn->wDriverOffset,
                               (WCHAR*)dn + dn->wDeviceOffset, (WCHAR *)dn + dn->wOutputOffset);
        pagesetup_set_devmode(data, dm);
        GlobalUnlock(prnt.hDevNames);
        GlobalUnlock(prnt.hDevMode);
        pagesetup_init_combos(hDlg, data);
    }

    GlobalFree(prnt.hDevMode);
    GlobalFree(prnt.hDevNames);

}

/******************************************************************************************
 * pagesetup_change_preview
 *
 * Changes paper preview size / position
 *
 */
static void pagesetup_change_preview(const pagesetup_data *data)
{
    LONG width, height, x, y;
    RECT tmp;
    const int shadow = 4;

    if(pagesetup_get_orientation(data) == DMORIENT_LANDSCAPE)
    {
        width  = data->rtDrawRect.right - data->rtDrawRect.left;
        height = pagesetup_get_papersize_pt(data)->y * width / pagesetup_get_papersize_pt(data)->x;
    }
    else
    {
        height = data->rtDrawRect.bottom - data->rtDrawRect.top;
        width  = pagesetup_get_papersize_pt(data)->x * height / pagesetup_get_papersize_pt(data)->y;
    }
    x = (data->rtDrawRect.right + data->rtDrawRect.left - width) / 2;
    y = (data->rtDrawRect.bottom + data->rtDrawRect.top - height) / 2;
    TRACE("draw rect %s x=%d, y=%d, w=%d, h=%d\n",
          wine_dbgstr_rect(&data->rtDrawRect), x, y, width, height);

    MoveWindow(GetDlgItem(data->hDlg, rct2), x + width, y + shadow, shadow, height, FALSE);
    MoveWindow(GetDlgItem(data->hDlg, rct3), x + shadow, y + height, width, shadow, FALSE);
    MoveWindow(GetDlgItem(data->hDlg, rct1), x, y, width, height, FALSE);

    tmp = data->rtDrawRect;
    tmp.right  += shadow;
    tmp.bottom += shadow;
    InvalidateRect(data->hDlg, &tmp, TRUE);
}

static inline LONG *element_from_margin_id(RECT *rc, WORD id)
{
    switch(id)
    {
    case edt4: return &rc->left;
    case edt5: return &rc->top;
    case edt6: return &rc->right;
    case edt7: return &rc->bottom;
    }
    return NULL;
}

static void update_margin_edits(HWND hDlg, const pagesetup_data *data, WORD id)
{
    WCHAR str[100];
    WORD idx;

    for(idx = edt4; idx <= edt7; idx++)
    {
        if(id == 0 || id == idx)
        {
            size2str(data, *element_from_margin_id(pagesetup_get_margin_rect(data), idx), str);
            SetDlgItemTextW(hDlg, idx, str);
        }
    }
}

static void margin_edit_notification(HWND hDlg, const pagesetup_data *data, WORD msg, WORD id)
{
    switch (msg)
    {
    case EN_CHANGE:
      {
        WCHAR buf[10];
        LONG val = 0;
        LONG *value = element_from_margin_id(pagesetup_get_margin_rect(data), id);

        if (GetDlgItemTextW(hDlg, id, buf, ARRAY_SIZE(buf)) != 0)
        {
            WCHAR *end;
            WCHAR decimal = get_decimal_sep();

            val = wcstol(buf, &end, 10);
            if(end != buf || *end == decimal)
            {
                int mult = is_metric(data) ? 100 : 1000;
                val *= mult;
                if(*end == decimal)
                {
                    while(mult > 1)
                    {
                        end++;
                        mult /= 10;
                        if(iswdigit(*end))
                            val += (*end - '0') * mult;
                        else
                            break;
                    }
                }
            }
        }
        *value = val;
        return;
      }

    case EN_KILLFOCUS:
        update_margin_edits(hDlg, data, id);
        return;
    }
}

static void set_margin_groupbox_title(HWND hDlg, const pagesetup_data *data)
{
    WCHAR title[256];

    if(LoadStringW(COMDLG32_hInstance, is_metric(data) ? PD32_MARGINS_IN_MILLIMETERS : PD32_MARGINS_IN_INCHES,
                   title, ARRAY_SIZE(title)))
        SetDlgItemTextW(hDlg, grp4, title);
}

static void pagesetup_update_orientation_buttons(HWND hDlg, const pagesetup_data *data)
{
    if (pagesetup_get_orientation(data) == DMORIENT_LANDSCAPE)
        CheckRadioButton(hDlg, rad1, rad2, rad2);
    else
        CheckRadioButton(hDlg, rad1, rad2, rad1);
}

/****************************************************************************************
 *  pagesetup_printer_properties
 *
 *  Handle invocation of the 'Properties' button (not present in the default template).
 */
static void pagesetup_printer_properties(HWND hDlg, pagesetup_data *data)
{
    HANDLE hprn;
    LPWSTR devname;
    DEVMODEW *dm;
    LRESULT count;
    int i;

    devname = pagesetup_get_devname(data);

    if (!OpenPrinterW(devname, &hprn, NULL))
    {
        FIXME("Call to OpenPrinter did not succeed!\n");
        pagesetup_release_a_devname(data, devname);
        return;
    }

    dm = pagesetup_get_devmode(data);
    DocumentPropertiesW(hDlg, hprn, devname, dm, dm, DM_IN_BUFFER | DM_OUT_BUFFER | DM_IN_PROMPT);
    pagesetup_set_devmode(data, dm);
    pagesetup_release_devmode(data, dm);
    pagesetup_release_a_devname(data, devname);
    ClosePrinter(hprn);

    /* Changing paper */
    pagesetup_update_papersize(data);
    pagesetup_update_orientation_buttons(hDlg, data);

    /* Changing paper preview */
    pagesetup_change_preview(data);

    /* Selecting paper in combo */
    count = SendDlgItemMessageW(hDlg, cmb2, CB_GETCOUNT, 0, 0);
    if(count != CB_ERR)
    {
        WORD paperword = pagesetup_get_papersize(data);
        for(i = 0; i < count; i++)
        {
            if(SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA, i, 0) == paperword) {
                SendDlgItemMessageW(hDlg, cmb2, CB_SETCURSEL, i, 0);
                break;
            }
        }
    }
}

/********************************************************************************
 * pagesetup_wm_command
 * process WM_COMMAND message for PageSetupDlg
 *
 * PARAMS
 *  hDlg 	[in] 	Main dialog HANDLE 
 *  wParam 	[in]	WM_COMMAND wParam
 *  lParam	[in]	WM_COMMAND lParam
 *  pda		[in/out] ptr to PageSetupDataA
 */

static BOOL pagesetup_wm_command(HWND hDlg, WPARAM wParam, LPARAM lParam, pagesetup_data *data)
{
    WORD msg = HIWORD(wParam);
    WORD id  = LOWORD(wParam);

    TRACE("loword (lparam) %d, wparam 0x%lx, lparam %08lx\n",
	    LOWORD(lParam),wParam,lParam);
    switch (id)  {
    case IDOK:
	EndDialog(hDlg, TRUE);
	return TRUE ;

    case IDCANCEL:
        EndDialog(hDlg, FALSE);
	return FALSE ;

    case psh3: /* Printer... */
        pagesetup_change_printer_dialog(hDlg, data);
        return TRUE;

    case rad1: /* Portrait */
    case rad2: /* Landscape */
        if((id == rad1 && pagesetup_get_orientation(data) == DMORIENT_LANDSCAPE) ||
           (id == rad2 && pagesetup_get_orientation(data) == DMORIENT_PORTRAIT))
	{
            pagesetup_set_orientation(data, (id == rad1) ? DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE);
            pagesetup_update_papersize(data);
            rotate_rect(pagesetup_get_margin_rect(data), (id == rad2));
            update_margin_edits(hDlg, data, 0);
            pagesetup_change_preview(data);
	}
	break;
    case cmb1: /* Printer combo */
        if(msg == CBN_SELCHANGE)
        {
            WCHAR *name;
            INT index = SendDlgItemMessageW(hDlg, id, CB_GETCURSEL, 0, 0);
            INT length = SendDlgItemMessageW(hDlg, id, CB_GETLBTEXTLEN, index, 0);
            name = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(length+1));
            SendDlgItemMessageW(hDlg, id, CB_GETLBTEXT, index, (LPARAM)name);
            pagesetup_change_printer(name, data);
            pagesetup_init_combos(hDlg, data);
            HeapFree(GetProcessHeap(),0,name);
        }
        break;
    case cmb2: /* Paper combo */
        if(msg == CBN_SELCHANGE)
        {
            DWORD paperword = SendDlgItemMessageW(hDlg, cmb2, CB_GETITEMDATA,
                                                  SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0), 0);
            if (paperword != CB_ERR)
            {
                pagesetup_set_papersize(data, paperword);
                pagesetup_update_papersize(data);
                pagesetup_change_preview(data);
	    } else
                FIXME("could not get dialog text for papersize cmbbox?\n");
        }
        break;
    case cmb3: /* Paper Source */
        if(msg == CBN_SELCHANGE)
        {
            WORD source = SendDlgItemMessageW(hDlg, cmb3, CB_GETITEMDATA,
                                              SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0), 0);
            pagesetup_set_defaultsource(data, source);
        }
        break;
    case psh2: /* Printer Properties button */
        pagesetup_printer_properties(hDlg, data);
        break;
    case edt4:
    case edt5:
    case edt6:
    case edt7:
        margin_edit_notification(hDlg, data, msg, id);
        break;
    }
    InvalidateRect(GetDlgItem(hDlg, rct1), NULL, TRUE);
    return FALSE;
}

/***********************************************************************
 *           default_page_paint_hook
 * Default hook paint procedure that receives WM_PSD_* messages from the dialog box 
 * whenever the sample page is redrawn.
 */
static UINT_PTR default_page_paint_hook(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                        const pagesetup_data *data)
{
    LPRECT lprc = (LPRECT) lParam;
    HDC hdc = (HDC) wParam;
    HPEN hpen, holdpen;
    LOGFONTW lf;
    HFONT hfont, holdfont;
    INT oldbkmode;
    TRACE("uMsg: WM_USER+%d\n",uMsg-WM_USER);
    /* Call user paint hook if enable */
    if (pagesetup_get_flags(data) & PSD_ENABLEPAGEPAINTHOOK)
        if (pagesetup_get_hook(data, page_paint_hook)(hwndDlg, uMsg, wParam, lParam))
            return TRUE;

    switch (uMsg) {
       /* LPPAGESETUPDLG in lParam */
       case WM_PSD_PAGESETUPDLG:
       /* Inform about the sample page rectangle */
       case WM_PSD_FULLPAGERECT:
       /* Inform about the margin rectangle */
       case WM_PSD_MINMARGINRECT:
            return FALSE;

        /* Draw dashed rectangle showing margins */
        case WM_PSD_MARGINRECT:
            hpen = CreatePen(PS_DASH, 1, GetSysColor(COLOR_3DSHADOW));
            holdpen = SelectObject(hdc, hpen);
            Rectangle(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom);
            DeleteObject(SelectObject(hdc, holdpen));
            return TRUE;
        /* Draw the fake document */
        case WM_PSD_GREEKTEXTRECT:
            /* select a nice scalable font, because we want the text really small */
            SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
            lf.lfHeight = 6; /* value chosen based on visual effect */
            hfont = CreateFontIndirectW(&lf);
            holdfont = SelectObject(hdc, hfont);

            /* if text not loaded, then do so now */
            if (wszFakeDocumentText[0] == '\0')
                 LoadStringW(COMDLG32_hInstance,
                        IDS_FAKEDOCTEXT,
                        wszFakeDocumentText,
                        ARRAY_SIZE(wszFakeDocumentText));

            oldbkmode = SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, wszFakeDocumentText, -1, lprc, DT_TOP|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
            SetBkMode(hdc, oldbkmode);

            DeleteObject(SelectObject(hdc, holdfont));
            return TRUE;

        /* Envelope stamp */
        case WM_PSD_ENVSTAMPRECT:
        /* Return address */
        case WM_PSD_YAFULLPAGERECT:
            FIXME("envelope/stamp is not implemented\n");
            return FALSE;
        default:
            FIXME("Unknown message %x\n",uMsg);
            return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           PagePaintProc
 * The main paint procedure for the PageSetupDlg function.
 * The Page Setup dialog box includes an image of a sample page that shows how
 * the user's selections affect the appearance of the printed output.
 * The image consists of a rectangle that represents the selected paper
 * or envelope type, with a dotted-line rectangle representing
 * the current margins, and partial (Greek text) characters
 * to show how text looks on the printed page. 
 *
 * The following messages in the order sends to user hook procedure:
 *   WM_PSD_PAGESETUPDLG    Draw the contents of the sample page
 *   WM_PSD_FULLPAGERECT    Inform about the bounding rectangle
 *   WM_PSD_MINMARGINRECT   Inform about the margin rectangle (min margin?)
 *   WM_PSD_MARGINRECT      Draw the margin rectangle
 *   WM_PSD_GREEKTEXTRECT   Draw the Greek text inside the margin rectangle
 * If any of first three messages returns TRUE, painting done.
 *
 * PARAMS:
 *   hWnd   [in] Handle to the Page Setup dialog box
 *   uMsg   [in] Received message
 *
 * TODO:
 *   WM_PSD_ENVSTAMPRECT    Draw in the envelope-stamp rectangle (for envelopes only)
 *   WM_PSD_YAFULLPAGERECT  Draw the return address portion (for envelopes and other paper sizes)
 *
 * RETURNS:
 *   FALSE if all done correctly
 *
 */


static LRESULT CALLBACK
PRINTDLG_PagePaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT rcClient, rcMargin;
    HPEN hpen, holdpen;
    HDC hdc;
    HBRUSH hbrush, holdbrush;
    pagesetup_data *data;
    int papersize=0, orientation=0; /* FIXME: set these values for the user paint hook */
    double scalx, scaly;

    if (uMsg != WM_PAINT)
        return CallWindowProcA(lpfnStaticWndProc, hWnd, uMsg, wParam, lParam);

    /* Processing WM_PAINT message */
    data = GetPropW(hWnd, pagesetupdlg_prop);
    if (!data) {
        WARN("__WINE_PAGESETUPDLGDATA prop not set?\n");
        return FALSE;
    }
    if (default_page_paint_hook(hWnd, WM_PSD_PAGESETUPDLG, MAKELONG(papersize, orientation),
                                pagesetup_get_dlg_struct(data), data))
        return FALSE;

    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rcClient);
    
    scalx = rcClient.right  / (double)pagesetup_get_papersize_pt(data)->x;
    scaly = rcClient.bottom / (double)pagesetup_get_papersize_pt(data)->y;
    rcMargin = rcClient;

    rcMargin.left   += pagesetup_get_margin_rect(data)->left   * scalx;
    rcMargin.top    += pagesetup_get_margin_rect(data)->top    * scaly;
    rcMargin.right  -= pagesetup_get_margin_rect(data)->right  * scalx;
    rcMargin.bottom -= pagesetup_get_margin_rect(data)->bottom * scaly;

    /* if the space is too small then we make sure to not draw anything */
    rcMargin.left = min(rcMargin.left, rcMargin.right);
    rcMargin.top = min(rcMargin.top, rcMargin.bottom);

    if (!default_page_paint_hook(hWnd, WM_PSD_FULLPAGERECT, (WPARAM)hdc, (LPARAM)&rcClient, data) &&
        !default_page_paint_hook(hWnd, WM_PSD_MINMARGINRECT, (WPARAM)hdc, (LPARAM)&rcMargin, data) )
    {
        /* fill background */
        hbrush = GetSysColorBrush(COLOR_3DHIGHLIGHT);
        FillRect(hdc, &rcClient, hbrush);
        holdbrush = SelectObject(hdc, hbrush);

        hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
        holdpen = SelectObject(hdc, hpen);
        
        /* paint left edge */
        MoveToEx(hdc, rcClient.left, rcClient.top, NULL);
        LineTo(hdc, rcClient.left, rcClient.bottom-1);

        /* paint top edge */
        MoveToEx(hdc, rcClient.left, rcClient.top, NULL);
        LineTo(hdc, rcClient.right, rcClient.top);

        hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
        DeleteObject(SelectObject(hdc, hpen));

        /* paint right edge */
        MoveToEx(hdc, rcClient.right-1, rcClient.top, NULL);
        LineTo(hdc, rcClient.right-1, rcClient.bottom);

        /* paint bottom edge */
        MoveToEx(hdc, rcClient.left, rcClient.bottom-1, NULL);
        LineTo(hdc, rcClient.right, rcClient.bottom-1);

        DeleteObject(SelectObject(hdc, holdpen));
        DeleteObject(SelectObject(hdc, holdbrush));

        default_page_paint_hook(hWnd, WM_PSD_MARGINRECT, (WPARAM)hdc, (LPARAM)&rcMargin, data);

        /* give text a bit of a space from the frame */
        InflateRect(&rcMargin, -2, -2);

        /* if the space is too small then we make sure to not draw anything */
        rcMargin.left = min(rcMargin.left, rcMargin.right);
        rcMargin.top = min(rcMargin.top, rcMargin.bottom);

        default_page_paint_hook(hWnd, WM_PSD_GREEKTEXTRECT, (WPARAM)hdc, (LPARAM)&rcMargin, data);
    }

    EndPaint(hWnd, &ps);
    return FALSE;
}

/*******************************************************
 * The margin edit controls are subclassed to filter
 * anything other than numbers and the decimal separator.
 */
static LRESULT CALLBACK pagesetup_margin_editproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_CHAR)
    {
        WCHAR decimal = get_decimal_sep();
        WCHAR wc = (WCHAR)wparam;
        if(!iswdigit(wc) && wc != decimal && wc != VK_BACK) return 0;
    }
    return CallWindowProcW(edit_wndproc, hwnd, msg, wparam, lparam);
}

static void subclass_margin_edits(HWND hDlg)
{
    int id;
    WNDPROC old_proc;

    for(id = edt4; id <= edt7; id++)
    {
        old_proc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hDlg, id),
                                              GWLP_WNDPROC,
                                              (ULONG_PTR)pagesetup_margin_editproc);
        InterlockedCompareExchangePointer((void**)&edit_wndproc, old_proc, NULL);
    }
}

/***********************************************************************
 *           pagesetup_dlg_proc
 *
 * Message handler for PageSetupDlg
 */
static INT_PTR CALLBACK pagesetup_dlg_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    pagesetup_data *data;
    INT_PTR		res = FALSE;
    HWND 		hDrawWnd;

    if (uMsg == WM_INITDIALOG) { /*Init dialog*/
        data = (pagesetup_data *)lParam;
        data->hDlg = hDlg;

	hDrawWnd = GetDlgItem(hDlg, rct1); 
        TRACE("set property to %p\n", data);
        SetPropW(hDlg, pagesetupdlg_prop, data);
        SetPropW(hDrawWnd, pagesetupdlg_prop, data);
        GetWindowRect(hDrawWnd, &data->rtDrawRect); /* Calculating rect in client coordinates where paper draws */
        MapWindowPoints( 0, hDlg, (LPPOINT)&data->rtDrawRect, 2 );
        lpfnStaticWndProc = (WNDPROC)SetWindowLongPtrW(
            hDrawWnd,
            GWLP_WNDPROC,
            (ULONG_PTR)PRINTDLG_PagePaintProc);
	
	/* FIXME: Paint hook. Must it be at begin of initialization or at end? */
	res = TRUE;
        if (pagesetup_get_flags(data) & PSD_ENABLEPAGESETUPHOOK)
        {
            if (!pagesetup_get_hook(data, page_setup_hook)(hDlg, uMsg, wParam,
                                                           pagesetup_get_dlg_struct(data)))
		FIXME("Setup page hook failed?\n");
	}

	/* if printer button disabled */
        if (pagesetup_get_flags(data) & PSD_DISABLEPRINTER)
            EnableWindow(GetDlgItem(hDlg, psh3), FALSE);
	/* if margin edit boxes disabled */
        if (pagesetup_get_flags(data) & PSD_DISABLEMARGINS)
        {
            EnableWindow(GetDlgItem(hDlg, edt4), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt5), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt6), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt7), FALSE);
	}

        /* Set orientation radiobuttons properly */
        pagesetup_update_orientation_buttons(hDlg, data);

	/* if orientation disabled */
        if (pagesetup_get_flags(data) & PSD_DISABLEORIENTATION)
        {
	    EnableWindow(GetDlgItem(hDlg,rad1),FALSE);
	    EnableWindow(GetDlgItem(hDlg,rad2),FALSE);
	}

	/* We fill them out enabled or not */
        if (!(pagesetup_get_flags(data) & PSD_MARGINS))
        {
            /* default is 1 inch */
            LONG size = thousandths_inch_to_size(data, 1000);
            SetRect(pagesetup_get_margin_rect(data), size, size, size, size);
        }
        update_margin_edits(hDlg, data, 0);
        subclass_margin_edits(hDlg);
        set_margin_groupbox_title(hDlg, data);

	/* if paper disabled */
        if (pagesetup_get_flags(data) & PSD_DISABLEPAPER)
        {
	    EnableWindow(GetDlgItem(hDlg,cmb2),FALSE);
	    EnableWindow(GetDlgItem(hDlg,cmb3),FALSE);
	}

	/* filling combos: printer, paper, source. selecting current printer (from DEVMODEA) */
        pagesetup_init_combos(hDlg, data);
        pagesetup_update_papersize(data);
        pagesetup_set_defaultsource(data, DMBIN_FORMSOURCE); /* FIXME: This is the auto select bin. Is this correct? */

	/* Drawing paper prev */
        pagesetup_change_preview(data);
	return TRUE;
    } else {
        data = GetPropW(hDlg, pagesetupdlg_prop);
        if (!data)
        {
	    WARN("__WINE_PAGESETUPDLGDATA prop not set?\n");
	    return FALSE;
	}
        if (pagesetup_get_flags(data) & PSD_ENABLEPAGESETUPHOOK)
        {
            res = pagesetup_get_hook(data, page_setup_hook)(hDlg, uMsg, wParam, lParam);
	    if (res) return res;
	}
    }
    switch (uMsg) {
    case WM_COMMAND:
        return pagesetup_wm_command(hDlg, wParam, lParam, data);
    }
    return FALSE;
}

static WCHAR *get_default_printer(void)
{
    WCHAR *name = NULL;
    DWORD len = 0;

    GetDefaultPrinterW(NULL, &len);
    if(len)
    {
        name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        GetDefaultPrinterW(name, &len);
    }
    return name;
}

static void pagesetup_dump_dlg_struct(const pagesetup_data *data)
{
    if(TRACE_ON(commdlg))
    {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = psd_flags;
        for( ; pflag->name; pflag++)
        {
            if(pagesetup_get_flags(data) & pflag->flag)
            {
                strcat(flagstr, pflag->name);
                strcat(flagstr, "|");
            }
        }
        TRACE("%s: (%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
              "hinst %p, flags %08x (%s)\n",
              data->unicode ? "unicode" : "ansi",
              data->u.dlgw, data->u.dlgw->hwndOwner, data->u.dlgw->hDevMode,
              data->u.dlgw->hDevNames, data->u.dlgw->hInstance,
              pagesetup_get_flags(data), flagstr);
    }
}

static void *pagesetup_get_template(pagesetup_data *data)
{
    HRSRC res;
    HGLOBAL tmpl_handle;

    if(pagesetup_get_flags(data) & PSD_ENABLEPAGESETUPTEMPLATEHANDLE)
    {
	tmpl_handle = data->u.dlgw->hPageSetupTemplate;
    }
    else if(pagesetup_get_flags(data) & PSD_ENABLEPAGESETUPTEMPLATE)
    {
        if(data->unicode)
            res = FindResourceW(data->u.dlgw->hInstance,
                                data->u.dlgw->lpPageSetupTemplateName, (LPWSTR)RT_DIALOG);
        else
            res = FindResourceA(data->u.dlga->hInstance,
                                data->u.dlga->lpPageSetupTemplateName, (LPSTR)RT_DIALOG);
        tmpl_handle = LoadResource(data->u.dlgw->hInstance, res);
    }
    else
    {
        res = FindResourceW(COMDLG32_hInstance, MAKEINTRESOURCEW(PAGESETUPDLGORD),
                            (LPWSTR)RT_DIALOG);
        tmpl_handle = LoadResource(COMDLG32_hInstance, res);
    }
    return LockResource(tmpl_handle);
}

static BOOL pagesetup_common(pagesetup_data *data)
{
    BOOL ret;
    void *tmpl;

    if(!pagesetup_get_dlg_struct(data))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_INITIALIZATION);
        return FALSE;
    }

    pagesetup_dump_dlg_struct(data);

    if(data->u.dlgw->lStructSize != sizeof(PAGESETUPDLGW))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
        return FALSE;
    }

    if ((pagesetup_get_flags(data) & PSD_ENABLEPAGEPAINTHOOK) &&
        (pagesetup_get_hook(data, page_paint_hook) == NULL))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_NOHOOK);
        return FALSE;
    }

    if(!(pagesetup_get_flags(data) & (PSD_INTHOUSANDTHSOFINCHES | PSD_INHUNDREDTHSOFMILLIMETERS)))
        data->u.dlgw->Flags |= is_default_metric() ?
            PSD_INHUNDREDTHSOFMILLIMETERS : PSD_INTHOUSANDTHSOFINCHES;

    if (!data->u.dlgw->hDevMode || !data->u.dlgw->hDevNames)
    {
        WCHAR *def = get_default_printer();
        if(!def)
        {
            if (!(pagesetup_get_flags(data) & PSD_NOWARNING))
            {
                WCHAR errstr[256];
                LoadStringW(COMDLG32_hInstance, PD32_NO_DEFAULT_PRINTER, errstr, 255);
                MessageBoxW(data->u.dlgw->hwndOwner, errstr, 0, MB_OK | MB_ICONERROR);
            }
            COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
            return FALSE;
        }
        pagesetup_change_printer(def, data);
        HeapFree(GetProcessHeap(), 0, def);
    }

    if (pagesetup_get_flags(data) & PSD_RETURNDEFAULT)
    {
        pagesetup_update_papersize(data);
        return TRUE;
    }

    tmpl = pagesetup_get_template(data);

    ret = DialogBoxIndirectParamW(data->u.dlgw->hInstance, tmpl,
                                  data->u.dlgw->hwndOwner,
                                  pagesetup_dlg_proc, (LPARAM)data) > 0;
    return ret;
}

/***********************************************************************
 *            PageSetupDlgA  (COMDLG32.@)
 *
 *  Displays the PAGE SETUP dialog box, which enables the user to specify
 *  specific properties of a printed page such as
 *  size, source, orientation and the width of the page margins.
 *
 * PARAMS
 *  setupdlg [IO] PAGESETUPDLGA struct
 *
 * RETURNS
 *  TRUE    if the user pressed the OK button
 *  FALSE   if the user cancelled the window or an error occurred
 *
 * NOTES
 *    The values of hDevMode and hDevNames are filled on output and can be
 *    changed in PAGESETUPDLG when they are passed in PageSetupDlg.
 *
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg)
{
    pagesetup_data data;

    data.unicode = FALSE;
    data.u.dlga  = setupdlg;

    return pagesetup_common(&data);
}

/***********************************************************************
 *            PageSetupDlgW  (COMDLG32.@)
 *
 * See PageSetupDlgA.
 */
BOOL WINAPI PageSetupDlgW(LPPAGESETUPDLGW setupdlg)
{
    pagesetup_data data;

    data.unicode = TRUE;
    data.u.dlgw  = setupdlg;

    return pagesetup_common(&data);
}

static void pdlgex_to_pdlg(const PRINTDLGEXW *pdlgex, PRINTDLGW *pdlg)
{
    pdlg->lStructSize = sizeof(*pdlg);
    pdlg->hwndOwner = pdlgex->hwndOwner;
    pdlg->hDevMode = pdlgex->hDevMode;
    pdlg->hDevNames = pdlgex->hDevNames;
    pdlg->hDC = pdlgex->hDC;
    pdlg->Flags = pdlgex->Flags;
    if ((pdlgex->Flags & PD_NOPAGENUMS) || !pdlgex->nPageRanges || !pdlgex->lpPageRanges)
    {
        pdlg->nFromPage = 0;
        pdlg->nToPage = 65534;
    }
    else
    {
        pdlg->nFromPage = pdlgex->lpPageRanges[0].nFromPage;
        pdlg->nToPage = pdlgex->lpPageRanges[0].nToPage;
    }
    pdlg->nMinPage = pdlgex->nMinPage;
    pdlg->nMaxPage = pdlgex->nMaxPage;
    pdlg->nCopies = pdlgex->nCopies;
    pdlg->hInstance = pdlgex->hInstance;
    pdlg->lCustData = 0;
    pdlg->lpfnPrintHook = NULL;
    pdlg->lpfnSetupHook = NULL;
    pdlg->lpPrintTemplateName = pdlgex->lpPrintTemplateName;
    pdlg->lpSetupTemplateName = NULL;
    pdlg->hPrintTemplate = NULL;
    pdlg->hSetupTemplate = NULL;
}

/* Only copy fields that are supposed to be changed. */
static void pdlg_to_pdlgex(const PRINTDLGW *pdlg, PRINTDLGEXW *pdlgex)
{
    pdlgex->hDevMode = pdlg->hDevMode;
    pdlgex->hDevNames = pdlg->hDevNames;
    pdlgex->hDC = pdlg->hDC;
    if (!(pdlgex->Flags & PD_NOPAGENUMS) && pdlgex->nPageRanges && pdlgex->lpPageRanges)
    {
        pdlgex->lpPageRanges[0].nFromPage = pdlg->nFromPage;
        pdlgex->lpPageRanges[0].nToPage = pdlg->nToPage;
    }
    pdlgex->nMinPage = pdlg->nMinPage;
    pdlgex->nMaxPage = pdlg->nMaxPage;
    pdlgex->nCopies = pdlg->nCopies;
}

struct callback_data
{
    IPrintDialogCallback *callback;
    IObjectWithSite *object;
};

static UINT_PTR CALLBACK pdlgex_hook_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_INITDIALOG)
    {
        PRINTDLGW *pd = (PRINTDLGW *)lp;
        struct callback_data *cb = (struct callback_data *)pd->lCustData;

        if (cb->callback)
        {
            cb->callback->lpVtbl->SelectionChange(cb->callback);
            cb->callback->lpVtbl->InitDone(cb->callback);
        }
    }
    else
    {
/* FIXME: store interface pointer somewhere in window properties and call it
        HRESULT hres;
        cb->callback->lpVtbl->HandleMessage(cb->callback, hwnd, msg, wp, lp, &hres);
*/
    }

    return 0;
}

/***********************************************************************
 * PrintDlgExA (COMDLG32.@)
 *
 * See PrintDlgExW.
 *
 * BUGS
 *   Only a Stub
 *
 */
HRESULT WINAPI PrintDlgExA(LPPRINTDLGEXA lppd)
{
    PRINTER_INFO_2A *pbuf;
    DRIVER_INFO_3A *dbuf;
    DEVMODEA *dm;
    HRESULT hr = S_OK;
    HANDLE hprn;

    if ((lppd == NULL) || (lppd->lStructSize != sizeof(PRINTDLGEXA)))
        return E_INVALIDARG;

    if (!IsWindow(lppd->hwndOwner))
        return E_HANDLE;

    if (lppd->nStartPage != START_PAGE_GENERAL)
    {
        if (!lppd->nPropertyPages)
            return E_INVALIDARG;

        FIXME("custom property sheets (%d at %p) not supported\n", lppd->nPropertyPages, lppd->lphPropertyPages);
    }

    /* Use PD_NOPAGENUMS or set nMaxPageRanges and lpPageRanges */
    if (!(lppd->Flags & PD_NOPAGENUMS) && (!lppd->nMaxPageRanges || !lppd->lpPageRanges))
    {
        return E_INVALIDARG;
    }

    if (lppd->Flags & PD_RETURNDEFAULT)
    {
        if (lppd->hDevMode || lppd->hDevNames)
        {
            WARN("hDevMode or hDevNames non-zero for PD_RETURNDEFAULT\n");
            COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
            return E_INVALIDARG;
        }
        if (!PRINTDLG_OpenDefaultPrinter(&hprn))
        {
            WARN("Can't find default printer\n");
            COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
            return E_FAIL;
        }

        pbuf = get_printer_infoA(hprn);
        if (!pbuf)
        {
            ClosePrinter(hprn);
            return E_FAIL;
        }

        dbuf = get_driver_infoA(hprn);
        if (!dbuf)
        {
            HeapFree(GetProcessHeap(), 0, pbuf);
            COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
            ClosePrinter(hprn);
            return E_FAIL;
        }
        dm = pbuf->pDevMode;
    }
    else
    {
        PRINTDLGA pdlg;
        struct callback_data cb_data = { 0 };

        FIXME("(%p) semi-stub\n", lppd);

        if (lppd->lpCallback)
        {
            IUnknown_QueryInterface((IUnknown *)lppd->lpCallback, &IID_IPrintDialogCallback, (void **)&cb_data.callback);
            IUnknown_QueryInterface((IUnknown *)lppd->lpCallback, &IID_IObjectWithSite, (void **)&cb_data.object);
        }

        /*
         * PRINTDLGEXA/W and PRINTDLGA/W layout is the same for A and W variants.
         */
        pdlgex_to_pdlg((const PRINTDLGEXW *)lppd, (PRINTDLGW *)&pdlg);
        pdlg.Flags |= PD_ENABLEPRINTHOOK;
        pdlg.lpfnPrintHook = pdlgex_hook_proc;
        pdlg.lCustData = (LPARAM)&cb_data;

        if (PrintDlgA(&pdlg))
        {
            pdlg_to_pdlgex((const PRINTDLGW *)&pdlg, (PRINTDLGEXW *)lppd);
            lppd->dwResultAction = PD_RESULT_PRINT;
        }
        else
            lppd->dwResultAction = PD_RESULT_CANCEL;

        if (cb_data.callback)
            cb_data.callback->lpVtbl->Release(cb_data.callback);
        if (cb_data.object)
            cb_data.object->lpVtbl->Release(cb_data.object);

        return S_OK;
    }

    ClosePrinter(hprn);

    PRINTDLG_CreateDevNames(&(lppd->hDevNames), dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName);
    if (!lppd->hDevNames)
        hr = E_FAIL;

    lppd->hDevMode = update_devmode_handleA(lppd->hDevMode, dm);
    if (hr == S_OK && lppd->hDevMode) {
        if (lppd->Flags & PD_RETURNDC) {
            lppd->hDC = CreateDCA(dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, dm);
            if (!lppd->hDC)
                hr = E_FAIL;
        }
        else if (lppd->Flags & PD_RETURNIC) {
            lppd->hDC = CreateICA(dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, dm);
            if (!lppd->hDC)
                hr = E_FAIL;
        }
    }
    else
        hr = E_FAIL;

    HeapFree(GetProcessHeap(), 0, pbuf);
    HeapFree(GetProcessHeap(), 0, dbuf);

    return hr;
}

/***********************************************************************
 * PrintDlgExW (COMDLG32.@)
 *
 * Display the property sheet style PRINT dialog box
 *  
 * PARAMS
 *  lppd  [IO] ptr to PRINTDLGEX struct
 * 
 * RETURNS
 *  Success: S_OK
 *  Failure: One of the following COM error codes:
 *    E_OUTOFMEMORY Insufficient memory.
 *    E_INVALIDARG  One or more arguments are invalid.
 *    E_POINTER     Invalid pointer.
 *    E_HANDLE      Invalid handle.
 *    E_FAIL        Unspecified error.
 *  
 * NOTES
 * This Dialog enables the user to specify specific properties of the print job.
 * The property sheet can also have additional application-specific and
 * driver-specific property pages.
 *
 * BUGS
 *   Not fully implemented
 *
 */
HRESULT WINAPI PrintDlgExW(LPPRINTDLGEXW lppd)
{
    PRINTER_INFO_2W *pbuf;
    DRIVER_INFO_3W *dbuf;
    DEVMODEW *dm;
    HRESULT hr = S_OK;
    HANDLE hprn;

    if ((lppd == NULL) || (lppd->lStructSize != sizeof(PRINTDLGEXW))) {
        return E_INVALIDARG;
    }

    if (!IsWindow(lppd->hwndOwner)) {
        return E_HANDLE;
    }

    if (lppd->nStartPage != START_PAGE_GENERAL)
    {
        if (!lppd->nPropertyPages)
            return E_INVALIDARG;

        FIXME("custom property sheets (%d at %p) not supported\n", lppd->nPropertyPages, lppd->lphPropertyPages);
    }

    /* Use PD_NOPAGENUMS or set nMaxPageRanges and lpPageRanges */
    if (!(lppd->Flags & PD_NOPAGENUMS) && (!lppd->nMaxPageRanges || !lppd->lpPageRanges))
    {
        return E_INVALIDARG;
    }

    if (lppd->Flags & PD_RETURNDEFAULT) {

        if (lppd->hDevMode || lppd->hDevNames) {
            WARN("hDevMode or hDevNames non-zero for PD_RETURNDEFAULT\n");
            COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
            return E_INVALIDARG;
        }
        if (!PRINTDLG_OpenDefaultPrinter(&hprn)) {
            WARN("Can't find default printer\n");
            COMDLG32_SetCommDlgExtendedError(PDERR_NODEFAULTPRN);
            return E_FAIL;
        }

        pbuf = get_printer_infoW(hprn);
        if (!pbuf)
        {
            ClosePrinter(hprn);
            return E_FAIL;
        }

        dbuf = get_driver_infoW(hprn);
        if (!dbuf)
        {
            HeapFree(GetProcessHeap(), 0, pbuf);
            COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
            ClosePrinter(hprn);
            return E_FAIL;
        }
        dm = pbuf->pDevMode;
    }
    else
    {
        PRINTDLGW pdlg;
        struct callback_data cb_data = { 0 };

        FIXME("(%p) semi-stub\n", lppd);

        if (lppd->lpCallback)
        {
            IUnknown_QueryInterface((IUnknown *)lppd->lpCallback, &IID_IPrintDialogCallback, (void **)&cb_data.callback);
            IUnknown_QueryInterface((IUnknown *)lppd->lpCallback, &IID_IObjectWithSite, (void **)&cb_data.object);
        }

        pdlgex_to_pdlg(lppd, &pdlg);
        pdlg.Flags |= PD_ENABLEPRINTHOOK;
        pdlg.lpfnPrintHook = pdlgex_hook_proc;
        pdlg.lCustData = (LPARAM)&cb_data;

        if (PrintDlgW(&pdlg))
        {
            pdlg_to_pdlgex(&pdlg, lppd);
            lppd->dwResultAction = PD_RESULT_PRINT;
        }
        else
            lppd->dwResultAction = PD_RESULT_CANCEL;

        if (cb_data.callback)
            cb_data.callback->lpVtbl->Release(cb_data.callback);
        if (cb_data.object)
            cb_data.object->lpVtbl->Release(cb_data.object);

        return S_OK;
    }

    ClosePrinter(hprn);

    PRINTDLG_CreateDevNamesW(&(lppd->hDevNames), dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName);
    if (!lppd->hDevNames)
        hr = E_FAIL;

    lppd->hDevMode = update_devmode_handleW(lppd->hDevMode, dm);
    if (hr == S_OK && lppd->hDevMode) {
        if (lppd->Flags & PD_RETURNDC) {
            lppd->hDC = CreateDCW(dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, dm);
            if (!lppd->hDC)
                hr = E_FAIL;
        }
        else if (lppd->Flags & PD_RETURNIC) {
            lppd->hDC = CreateICW(dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, dm);
            if (!lppd->hDC)
                hr = E_FAIL;
        }
    }
    else
        hr = E_FAIL;

    HeapFree(GetProcessHeap(), 0, pbuf);
    HeapFree(GetProcessHeap(), 0, dbuf);

    return hr;
}
