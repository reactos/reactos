/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 * Copyright 2000 Huw D M Davies
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winspool.h"
#include "winerror.h"

#include "wine/debug.h"

#include "commdlg.h"
#include "dlgs.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "printdlg.h"

/* Yes these constants are the same, but we're just copying win98 */
#define UPDOWN_ID 0x270f
#define MAX_COPIES 9999

/* Debugging info */
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

/* address of wndproc for subclassed Static control */
static WNDPROC lpfnStaticWndProc;
/* the text of the fake document to render for the Page Setup dialog */
static WCHAR wszFakeDocumentText[1024];

/***********************************************************************
 *    PRINTDLG_OpenDefaultPrinter
 *
 * Returns a winspool printer handle to the default printer in *hprn
 * Caller must call ClosePrinter on the handle
 *
 * Returns TRUE on success else FALSE
 */
BOOL PRINTDLG_OpenDefaultPrinter(HANDLE *hprn)
{
    WCHAR buf[260];
    DWORD dwBufLen = sizeof(buf) / sizeof(buf[0]);
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
INT PRINTDLG_SetUpPrinterListComboA(HWND hDlg, UINT id, LPCSTR name)
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
        DWORD dwBufLen = sizeof(buf);
        FIXME("Can't find '%s' in printer list so trying to find default\n",
	      name);
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
        DWORD dwBufLen = sizeof(buf)/sizeof(buf[0]);
        TRACE("Can't find '%s' in printer list so trying to find default\n",
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
    DWORD dwBufLen = sizeof(buf);

    size = strlen(DeviceDriverName) + 1
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
    strcpy(pTempPtr, DeviceDriverName);
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
    DWORD dwBufLen = sizeof(bufW) / sizeof(WCHAR);

    size = sizeof(WCHAR)*lstrlenW(DeviceDriverName) + 2
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
    lstrcpyW(pTempPtr, DeviceDriverName);
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
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
	    if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
		nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage) {
	        char resourcestr[256];
		char resultstr[256];
		LoadStringA(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE,
			    resourcestr, 255);
		sprintf(resultstr,resourcestr, lppd->nMinPage, lppd->nMaxPage);
		LoadStringA(COMDLG32_hInstance, PD32_PRINT_TITLE,
			    resourcestr, 255);
		MessageBoxA(hDlg, resultstr, resourcestr,
			    MB_OK | MB_ICONWARNING);
		return FALSE;
	    }
	    lppd->nFromPage = nFromPage;
	    lppd->nToPage   = nToPage;
	    lppd->Flags |= PD_PAGENUMS;
	}
	else
	    lppd->Flags &= ~PD_PAGENUMS;

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
	        lpdm->u.s.dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
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
	    nFromPage = GetDlgItemInt(hDlg, edt1, NULL, FALSE);
	    nToPage   = GetDlgItemInt(hDlg, edt2, NULL, FALSE);
	    if (nFromPage < lppd->nMinPage || nFromPage > lppd->nMaxPage ||
		nToPage < lppd->nMinPage || nToPage > lppd->nMaxPage) {
	        WCHAR resourcestr[256];
		WCHAR resultstr[256];
		LoadStringW(COMDLG32_hInstance, PD32_INVALID_PAGE_RANGE,
			    resourcestr, 255);
		wsprintfW(resultstr,resourcestr, lppd->nMinPage, lppd->nMaxPage);
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
	        lpdm->u.s.dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
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

static BOOL PRINTDLG_PaperSizeA(
	PRINTDLGA	*pdlga,const WORD PaperSize,LPPOINT size
) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    LPSTR	devname,portname;
    int		i;
    INT		NrOfEntries,ret;
    WORD	*Words = NULL;
    POINT	*points = NULL;
    BOOL	retval = FALSE;

    dn = GlobalLock(pdlga->hDevNames);
    dm = GlobalLock(pdlga->hDevMode);
    devname	= ((char*)dn)+dn->wDeviceOffset;
    portname	= ((char*)dn)+dn->wOutputOffset;


    NrOfEntries = DeviceCapabilitiesA(devname,portname,DC_PAPERNAMES,NULL,dm);
    if (!NrOfEntries) {
	FIXME("No papernames found for %s/%s\n",devname,portname);
	goto out;
    }
    if (NrOfEntries == -1) {
	ERR("Hmm ? DeviceCapabilities() DC_PAPERNAMES failed, ret -1 !\n");
	goto out;
    }

    Words = HeapAlloc(GetProcessHeap(),0,NrOfEntries*sizeof(WORD));
    if (NrOfEntries != (ret=DeviceCapabilitiesA(devname,portname,DC_PAPERS,(LPSTR)Words,dm))) {
	FIXME("Number of returned vals %d is not %d\n",NrOfEntries,ret);
	goto out;
    }
    for (i=0;i<NrOfEntries;i++)
	if (Words[i] == PaperSize)
	    break;
    HeapFree(GetProcessHeap(),0,Words);
    if (i == NrOfEntries) {
	FIXME("Papersize %d not found in list?\n",PaperSize);
	goto out;
    }
    points = HeapAlloc(GetProcessHeap(),0,sizeof(points[0])*NrOfEntries);
    if (NrOfEntries!=(ret=DeviceCapabilitiesA(devname,portname,DC_PAPERSIZE,(LPSTR)points,dm))) {
	FIXME("Number of returned sizes %d is not %d?\n",NrOfEntries,ret);
	goto out;
    }
    /* this is _10ths_ of a millimeter */
    size->x=points[i].x;
    size->y=points[i].y;
    retval = TRUE;
out:
    GlobalUnlock(pdlga->hDevNames);
    GlobalUnlock(pdlga->hDevMode);
    HeapFree(GetProcessHeap(),0,Words);
    HeapFree(GetProcessHeap(),0,points);
    return retval;
}

static BOOL PRINTDLG_PaperSizeW(
	PRINTDLGW	*pdlga,const WCHAR *PaperSize,LPPOINT size
) {
    DEVNAMES	*dn;
    DEVMODEW	*dm;
    LPWSTR	devname,portname;
    int		i;
    INT		NrOfEntries,ret;
    WCHAR	*Names = NULL;
    POINT	*points = NULL;
    BOOL	retval = FALSE;

    dn = GlobalLock(pdlga->hDevNames);
    dm = GlobalLock(pdlga->hDevMode);
    devname	= ((WCHAR*)dn)+dn->wDeviceOffset;
    portname	= ((WCHAR*)dn)+dn->wOutputOffset;


    NrOfEntries = DeviceCapabilitiesW(devname,portname,DC_PAPERNAMES,NULL,dm);
    if (!NrOfEntries) {
	FIXME("No papernames found for %s/%s\n",debugstr_w(devname),debugstr_w(portname));
	goto out;
    }
    if (NrOfEntries == -1) {
	ERR("Hmm ? DeviceCapabilities() DC_PAPERNAMES failed, ret -1 !\n");
	goto out;
    }

    Names = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*NrOfEntries*64);
    if (NrOfEntries != (ret=DeviceCapabilitiesW(devname,portname,DC_PAPERNAMES,Names,dm))) {
	FIXME("Number of returned vals %d is not %d\n",NrOfEntries,ret);
	goto out;
    }
    for (i=0;i<NrOfEntries;i++)
	if (!lstrcmpW(PaperSize,Names+(64*i)))
	    break;
    HeapFree(GetProcessHeap(),0,Names);
    if (i==NrOfEntries) {
	FIXME("Papersize %s not found in list?\n",debugstr_w(PaperSize));
	goto out;
    }
    points = HeapAlloc(GetProcessHeap(),0,sizeof(points[0])*NrOfEntries);
    if (NrOfEntries!=(ret=DeviceCapabilitiesW(devname,portname,DC_PAPERSIZE,(LPWSTR)points,dm))) {
	FIXME("Number of returned sizes %d is not %d?\n",NrOfEntries,ret);
	goto out;
    }
    /* this is _10ths_ of a millimeter */
    size->x=points[i].x;
    size->y=points[i].y;
    retval = TRUE;
out:
    GlobalUnlock(pdlga->hDevNames);
    GlobalUnlock(pdlga->hDevMode);
    HeapFree(GetProcessHeap(),0,Names);
    HeapFree(GetProcessHeap(),0,points);
    return retval;
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
    DWORD   Sel;
    WORD    oldWord = 0;
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
        if (dm) {
            if (nIDComboBox == cmb2)
                dm->u.s.dmPaperSize = oldWord;
            else
                dm->u.s.dmDefaultSource = oldWord;
        }
    }
    else {
        /* we enter here only when the Print setup dialog is initially
         * opened. In this case the settings are restored from when
         * the dialog was last closed.
         */
        if (dm) {
            if (nIDComboBox == cmb2)
                oldWord = dm->u.s.dmPaperSize;
            else
                oldWord = dm->u.s.dmDefaultSource;
        }
    }

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

    /* for some printer drivers, DeviceCapabilities calls a VXD to obtain the
     * paper settings. As Wine doesn't allow VXDs, this results in a crash.
     */
    WARN(" if your printer driver uses VXDs, expect a crash now!\n");
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
    NrOfEntries = DeviceCapabilitiesA(PrinterName, PortName,
                                      fwCapability_Names, Names, dm);
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

    /* Look for old selection - can't do this is previous loop since
       item order will change as more items are added */
    Sel = 0;
    for (i = 0; i < NrOfEntries; i++) {
        if(SendDlgItemMessageA(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) ==
	   oldWord) {
	    Sel = i;
	    break;
	}
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
    DWORD   Sel;
    WORD    oldWord = 0;
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
        if (dm) {
            if (nIDComboBox == cmb2)
                dm->u.s.dmPaperSize = oldWord;
            else
                dm->u.s.dmDefaultSource = oldWord;
        }
    }
    else {
        /* we enter here only when the Print setup dialog is initially
         * opened. In this case the settings are restored from when
         * the dialog was last closed.
         */
        if (dm) {
            if (nIDComboBox == cmb2)
                oldWord = dm->u.s.dmPaperSize;
            else
                oldWord = dm->u.s.dmDefaultSource;
        }
    }

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

    /* for some printer drivers, DeviceCapabilities calls a VXD to obtain the
     * paper settings. As Wine doesn't allow VXDs, this results in a crash.
     */
    WARN(" if your printer driver uses VXDs, expect a crash now!\n");
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
    NrOfEntries = DeviceCapabilitiesW(PrinterName, PortName,
                                      fwCapability_Names, Names, dm);
    NrOfEntries = DeviceCapabilitiesW(PrinterName, PortName,
				      fwCapability_Words, (LPWSTR)Words, dm);

    /* reset any current content in the combobox */
    SendDlgItemMessageW(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);

    /* store new content */
    for (i = 0; i < NrOfEntries; i++) {
        DWORD pos = SendDlgItemMessageW(hDlg, nIDComboBox, CB_ADDSTRING, 0,
					(LPARAM)(&Names[i*NamesSize]) );
	SendDlgItemMessageW(hDlg, nIDComboBox, CB_SETITEMDATA, pos,
			    Words[i]);
    }

    /* Look for old selection - can't do this is previous loop since
       item order will change as more items are added */
    Sel = 0;
    for (i = 0; i < NrOfEntries; i++) {
        if(SendDlgItemMessageW(hDlg, nIDComboBox, CB_GETITEMDATA, i, 0) ==
	   oldWord) {
	    Sel = i;
	    break;
	}
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
BOOL PRINTDLG_ChangePrinterA(HWND hDlg, char *name,
				   PRINT_PTRA *PrintStructures)
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
        SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
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

	/* Collate pages
	 *
	 * FIXME: The ico3 is not displayed for some reason. I don't know why.
	 */
	if (lppd->Flags & PD_COLLATE) {
	    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
				(LPARAM)PrintStructures->hCollateIcon);
	    CheckDlgButton(hDlg, chx2, 1);
	} else {
	    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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
	      copies = lpdm->u.s.dmCopies;
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
      BOOL bPortrait = (lpdm->u.s.dmOrientation == DMORIENT_PORTRAIT);

      PRINTDLG_SetUpPaperComboBoxA(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBoxA(hDlg, cmb3,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      CheckRadioButton(hDlg, rad1, rad2, bPortrait ? rad1: rad2);
      SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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
    PrintStructures->lpPrinterInfo = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*needed);
    GetPrinterW(hprn, 2, (LPBYTE)PrintStructures->lpPrinterInfo, needed,
		&needed);
    GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
    PrintStructures->lpDriverInfo = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*needed);
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
        SetDlgItemInt(hDlg, edt1, lppd->nFromPage, FALSE);
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

	/* Collate pages
	 *
	 * FIXME: The ico3 is not displayed for some reason. I don't know why.
	 */
	if (lppd->Flags & PD_COLLATE) {
	    SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
				(LPARAM)PrintStructures->hCollateIcon);
	    CheckDlgButton(hDlg, chx2, 1);
	} else {
	    SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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
	      copies = lpdm->u.s.dmCopies;
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
      BOOL bPortrait = (lpdm->u.s.dmOrientation == DMORIENT_PORTRAIT);

      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb3,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      CheckRadioButton(hDlg, rad1, rad2, bPortrait ? rad1: rad2);
      SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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
    int res;

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
          res = MessageBoxW(hDlg, resultstr, resourcestr,MB_OK | MB_ICONWARNING);
          return FALSE;
    }
}

/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam,
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
    SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                        (LPARAM)PrintStructures->hNoCollateIcon);

    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0 ||
       PrintStructures->hPortraitIcon == 0 ||
       PrintStructures->hLandscapeIcon == 0) {
        ERR("no icon in resourcefile\n");
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
        DWORD dwBufLen = sizeof(name);
	BOOL ret = GetDefaultPrinterA(name, &dwBufLen);

	if (ret)
	    PRINTDLG_ChangePrinterA(hDlg, name, PrintStructures);
	else
	    FIXME("No default printer found, expect problems!\n");
    }
    return TRUE;
}

static LRESULT PRINTDLG_WMInitDialogW(HWND hDlg, WPARAM wParam,
				     PRINT_PTRW* PrintStructures)
{
    static const WCHAR PD32_COLLATE[] = { 'P', 'D', '3', '2', '_', 'C', 'O', 'L', 'L', 'A', 'T', 'E', 0 };
    static const WCHAR PD32_NOCOLLATE[] = { 'P', 'D', '3', '2', '_', 'N', 'O', 'C', 'O', 'L', 'L', 'A', 'T', 'E', 0 };
    static const WCHAR PD32_PORTRAIT[] = { 'P', 'D', '3', '2', '_', 'P', 'O', 'R', 'T', 'R', 'A', 'I', 'T', 0 };
    static const WCHAR PD32_LANDSCAPE[] = { 'P', 'D', '3', '2', '_', 'L', 'A', 'N', 'D', 'S', 'C', 'A', 'P', 'E', 0 };
    LPPRINTDLGW lppd = PrintStructures->lpPrintDlg;
    DEVNAMES *pdn;
    DEVMODEW *pdm;
    WCHAR *name = NULL;
    UINT comboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;

    /* load Collate ICONs */
    /* We load these with LoadImage because they are not a standard
       size and we don't want them rescaled */
    PrintStructures->hCollateIcon =
      LoadImageW(COMDLG32_hInstance, PD32_COLLATE, IMAGE_ICON, 0, 0, 0);
    PrintStructures->hNoCollateIcon =
      LoadImageW(COMDLG32_hInstance, PD32_NOCOLLATE, IMAGE_ICON, 0, 0, 0);

    /* These can be done with LoadIcon */
    PrintStructures->hPortraitIcon =
      LoadIconW(COMDLG32_hInstance, PD32_PORTRAIT);
    PrintStructures->hLandscapeIcon =
      LoadIconW(COMDLG32_hInstance, PD32_LANDSCAPE);

    /* display the collate/no_collate icon */
    SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                        (LPARAM)PrintStructures->hNoCollateIcon);

    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0 ||
       PrintStructures->hPortraitIcon == 0 ||
       PrintStructures->hLandscapeIcon == 0) {
        ERR("no icon in resourcefile\n");
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
        DWORD dwBufLen = sizeof(name) / sizeof(WCHAR);
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
LRESULT PRINTDLG_WMCommandA(HWND hDlg, WPARAM wParam,
			LPARAM lParam, PRINT_PTRA* PrintStructures)
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
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                                    (LPARAM)PrintStructures->hCollateIcon);
        else
            SendDlgItemMessageA(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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

#if 0
     case psh1:                       /* Print Setup */
	{
	    PRINTDLG16	pdlg;

	    if (!PrintStructures->dlg.lpPrintDlg16) {
		FIXME("The 32bit print dialog does not have this button!?\n");
		break;
	    }

	    memcpy(&pdlg,PrintStructures->dlg.lpPrintDlg16,sizeof(pdlg));
	    pdlg.Flags |= PD_PRINTSETUP;
	    pdlg.hwndOwner = HWND_16(hDlg);
	    if (!PrintDlg16(&pdlg))
		break;
	}
	break;
#endif
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
              lpdm->u.s.dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u.s.dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP, quality combobox in PRINT */
	 if (PrinterComboID != LOWORD(wParam)) {
	     FIXME("No handling for print quality combo box yet.\n");
	     break;
	 }
	 /* FALLTHROUGH */
    case cmb4:                         /* Printer combobox */
         if (HIWORD(wParam)==CBN_SELCHANGE) {
	     char   PrinterName[256];
	     GetDlgItemTextA(hDlg, LOWORD(wParam), PrinterName, 255);
	     PRINTDLG_ChangePrinterA(hDlg, PrinterName, PrintStructures);
	 }
	 break;

    case cmb2: /* Papersize */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u.s.dmPaperSize = SendDlgItemMessageA(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u.s.dmDefaultSource = SendDlgItemMessageA(hDlg, cmb3,
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
	        if(lpdm->u.s.dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->u.s.dmOrientation = DMORIENT_PORTRAIT;
		    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->u.s.dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->u.s.dmOrientation = DMORIENT_LANDSCAPE;
		    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
		    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
		}
	    }
	    break;
	}
    }
    return FALSE;
}

static LRESULT PRINTDLG_WMCommandW(HWND hDlg, WPARAM wParam,
			LPARAM lParam, PRINT_PTRW* PrintStructures)
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
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                                    (LPARAM)PrintStructures->hCollateIcon);
        else
            SendDlgItemMessageW(hDlg, ico3, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
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

     case psh1:                       /* Print Setup */
	{
		ERR("psh1 is called from 16bit code only, we should not get here.\n");
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
              lpdm->u.s.dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->u.s.dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP, quality combobox in PRINT */
	 if (PrinterComboID != LOWORD(wParam)) {
	     FIXME("No handling for print quality combo box yet.\n");
	     break;
	 }
	 /* FALLTHROUGH */
    case cmb4:                         /* Printer combobox */
         if (HIWORD(wParam)==CBN_SELCHANGE) {
	     WCHAR   PrinterName[256];
	     GetDlgItemTextW(hDlg, LOWORD(wParam), PrinterName, 255);
	     PRINTDLG_ChangePrinterW(hDlg, PrinterName, PrintStructures);
	 }
	 break;

    case cmb2: /* Papersize */
      {
	  DWORD Sel = SendDlgItemMessageW(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u.s.dmPaperSize = SendDlgItemMessageW(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageW(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->u.s.dmDefaultSource = SendDlgItemMessageW(hDlg, cmb3,
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
	        if(lpdm->u.s.dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->u.s.dmOrientation = DMORIENT_PORTRAIT;
		    SendDlgItemMessageW(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		    SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->u.s.dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->u.s.dmOrientation = DMORIENT_LANDSCAPE;
		    SendDlgItemMessageW(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hLandscapeIcon);
		    SendDlgItemMessageW(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
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
        PrintStructures = (PRINT_PTRA*)GetPropA(hDlg,"__WINE_PRINTDLGDATA");
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRA*) lParam;
	SetPropA(hDlg,"__WINE_PRINTDLGDATA",PrintStructures);
        if(!check_printer_setup(hDlg))
        {
            EndDialog(hDlg,FALSE);
            return FALSE;
        }
	res = PRINTDLG_WMInitDialog(hDlg, wParam, PrintStructures);

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
        return PRINTDLG_WMCommandA(hDlg, wParam, lParam, PrintStructures);

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
    static const WCHAR propW[] = {'_','_','W','I','N','E','_','P','R','I','N','T','D','L','G','D','A','T','A',0};
    PRINT_PTRW* PrintStructures;
    INT_PTR res = FALSE;

    if (uMsg!=WM_INITDIALOG) {
	PrintStructures = (PRINT_PTRW*) GetPropW(hDlg, propW);
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRW*) lParam;
	SetPropW(hDlg, propW, PrintStructures);
        if(!check_printer_setup(hDlg))
        {
            EndDialog(hDlg,FALSE);
            return FALSE;
        }
	res = PRINTDLG_WMInitDialogW(hDlg, wParam, PrintStructures);

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
        return PRINTDLG_WMCommandW(hDlg, wParam, lParam, PrintStructures);

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
    return lppd->hDC ? TRUE : FALSE;
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
    return lppd->hDC ? TRUE : FALSE;
}

/***********************************************************************
 *           PrintDlgA   (COMDLG32.@)
 *
 *  Displays the the PRINT dialog box, which enables the user to specify
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

    hInst = (HINSTANCE)GetWindowLongPtrA( lppd->hwndOwner, GWLP_HINSTANCE );
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
        WARN("structure size failure !!!\n");
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
            ERR("GetPrinterDriverA failed, le %d, fix your config for printer %s!\n",GetLastError(),pbuf->pPrinterName);
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

    hInst = (HINSTANCE)GetWindowLongPtrW( lppd->hwndOwner, GWLP_HINSTANCE );
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
        WARN("structure size failure !!!\n");
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
	pbuf = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*needed);
	GetPrinterW(hprn, 2, (LPBYTE)pbuf, needed, &needed);

	GetPrinterDriverW(hprn, NULL, 3, NULL, 0, &needed);
	dbuf = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*needed);
	if (!GetPrinterDriverW(hprn, NULL, 3, (LPBYTE)dbuf, needed, &needed)) {
            ERR("GetPrinterDriverA failed, le %d, fix your config for printer %s!\n",GetLastError(),debugstr_w(pbuf->pPrinterName));
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

typedef struct {
    LPPAGESETUPDLGA	dlga; /* Handler to user defined struct */
    PRINTDLGA		pdlg;
    HWND 		hDlg; /* Page Setup dialog handler */
    PAGESETUPDLGA	curdlg; /* Stores the current dialog state */
    RECT		rtDrawRect; /* Drawing rect for page */
} PageSetupDataA;

typedef struct {
    LPPAGESETUPDLGW	dlga;
    PRINTDLGW		pdlg;
} PageSetupDataW;


static HGLOBAL PRINTDLG_GetPGSTemplateA(const PAGESETUPDLGA *lppd)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl;
	
    if(lppd->Flags & PSD_ENABLEPAGESETUPTEMPLATEHANDLE) {
	hDlgTmpl = lppd->hPageSetupTemplate;
    } else if(lppd->Flags & PSD_ENABLEPAGESETUPTEMPLATE) {
	hResInfo = FindResourceA(lppd->hInstance,
				 lppd->lpPageSetupTemplateName, (LPSTR)RT_DIALOG);
	hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
    } else {
	hResInfo = FindResourceA(COMDLG32_hInstance,(LPCSTR)PAGESETUPDLGORD,(LPSTR)RT_DIALOG);
	hDlgTmpl = LoadResource(COMDLG32_hInstance,hResInfo);
    }
    return hDlgTmpl;
}

static HGLOBAL PRINTDLG_GetPGSTemplateW(const PAGESETUPDLGW *lppd)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl;

    if(lppd->Flags & PSD_ENABLEPAGESETUPTEMPLATEHANDLE) {
	hDlgTmpl = lppd->hPageSetupTemplate;
    } else if(lppd->Flags & PSD_ENABLEPAGESETUPTEMPLATE) {
	hResInfo = FindResourceW(lppd->hInstance,
				 lppd->lpPageSetupTemplateName, (LPWSTR)RT_DIALOG);
	hDlgTmpl = LoadResource(lppd->hInstance, hResInfo);
    } else {
	hResInfo = FindResourceW(COMDLG32_hInstance,(LPCWSTR)PAGESETUPDLGORD,(LPWSTR)RT_DIALOG);
	hDlgTmpl = LoadResource(COMDLG32_hInstance,hResInfo);
    }
    return hDlgTmpl;
}

static DWORD
_c_10mm2size(PAGESETUPDLGA *dlga,DWORD size) {
    if (dlga->Flags & PSD_INTHOUSANDTHSOFINCHES)
	return 10*size*100/254;
    /* If we don't have a flag, we can choose one. Use millimeters
     * to avoid confusing me
     */
    dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    return 10*size;
}


static DWORD
_c_inch2size(PAGESETUPDLGA *dlga,DWORD size) {
    if (dlga->Flags & PSD_INTHOUSANDTHSOFINCHES)
	return size;
    if (dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
	return (size*254)/100;
    /* if we don't have a flag, we can choose one. Use millimeters
     * to avoid confusing me
     */
    dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    return (size*254)/100;
}

static void
_c_size2strA(PageSetupDataA *pda,DWORD size,LPSTR strout) {
    strcpy(strout,"<undef>");
    if (pda->dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
	sprintf(strout,"%d",(size)/100);
	return;
    }
    if (pda->dlga->Flags & PSD_INTHOUSANDTHSOFINCHES) {
	sprintf(strout,"%din",(size)/1000);
	return;
    }
    pda->dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    sprintf(strout,"%d",(size)/100);
    return;
}
static void
_c_size2strW(PageSetupDataW *pda,DWORD size,LPWSTR strout) {
    static const WCHAR UNDEF[] = { '<', 'u', 'n', 'd', 'e', 'f', '>', 0 };
    static const WCHAR mm_fmt[] = { '%', '.', '2', 'f', 'm', 'm', 0 };
    static const WCHAR in_fmt[] = { '%', '.', '2', 'f', 'i', 'n', 0 };
    lstrcpyW(strout, UNDEF);
    if (pda->dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
	wsprintfW(strout,mm_fmt,(size*1.0)/100.0);
	return;
    }
    if (pda->dlga->Flags & PSD_INTHOUSANDTHSOFINCHES) {
	wsprintfW(strout,in_fmt,(size*1.0)/1000.0);
	return;
    }
    pda->dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    wsprintfW(strout,mm_fmt,(size*1.0)/100.0);
    return;
}

static DWORD
_c_str2sizeA(const PAGESETUPDLGA *dlga, LPCSTR strin) {
    float	val;
    char	rest[200];

    rest[0]='\0';
    if (!sscanf(strin,"%f%s",&val,rest))
	return 0;

    if (!strcmp(rest,"in") || !strcmp(rest,"inch")) {
	if (dlga->Flags & PSD_INTHOUSANDTHSOFINCHES)
	    return 1000*val;
	else
	    return val*25.4*100;
    }
    if (!strcmp(rest,"cm")) { rest[0]='m'; val = val*10.0; }
    if (!strcmp(rest,"m")) { strcpy(rest,"mm"); val = val*1000.0; }

    if (!strcmp(rest,"mm")) {
	if (dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
	    return 100*val;
	else
	    return 1000.0*val/25.4;
    }
    if (rest[0]=='\0') {
	/* use application supplied default */
	if (dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
	    /* 100*mm */
	    return 100.0*val;
	}
	if (dlga->Flags & PSD_INTHOUSANDTHSOFINCHES) {
	    /* 1000*inch */
	    return 1000.0*val;
	}
    }
    ERR("Did not find a conversion for type '%s'!\n",rest);
    return 0;
}


static DWORD
_c_str2sizeW(const PAGESETUPDLGW *dlga, LPCWSTR strin) {
    char	buf[200];

    /* this W -> A transition is OK */
    /* we need a unicode version of sscanf to avoid it */
    WideCharToMultiByte(CP_ACP, 0, strin, -1, buf, sizeof(buf), NULL, NULL);
    return _c_str2sizeA((const PAGESETUPDLGA *)dlga, buf);
}


/****************************************************************************
 * PRINTDLG_PS_UpdateDlgStructA
 *
 * Updates pda->dlga structure 
 * Function calls when user presses OK button
 *
 * PARAMS
 *  hDlg	[in] 	 main window dialog HANDLE
 *  pda 	[in/out] ptr to PageSetupDataA structure
 *
 * RETURNS
 *  TRUE
 */
static BOOL
PRINTDLG_PS_UpdateDlgStructA(HWND hDlg, PageSetupDataA *pda) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    DWORD 	paperword;

    memcpy(pda->dlga, &pda->curdlg, sizeof(pda->curdlg));
    pda->dlga->hDevMode  = pda->pdlg.hDevMode;
    pda->dlga->hDevNames = pda->pdlg.hDevNames;
    
    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);

    /* Save paper orientation into device context */
    if(pda->curdlg.ptPaperSize.x > pda->curdlg.ptPaperSize.y)
        dm->u.s.dmOrientation = DMORIENT_LANDSCAPE;
    else
        dm->u.s.dmOrientation = DMORIENT_PORTRAIT;

    /* Save paper size into the device context */
    paperword = SendDlgItemMessageA(hDlg,cmb2,CB_GETITEMDATA,
        SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0), 0);
    if (paperword != CB_ERR)
        dm->u.s.dmPaperSize = paperword;
    else
        FIXME("could not get dialog text for papersize cmbbox?\n");

    /* Save paper source into the device context */
    paperword = SendDlgItemMessageA(hDlg,cmb1,CB_GETITEMDATA,
        SendDlgItemMessageA(hDlg, cmb1, CB_GETCURSEL, 0, 0), 0);
    if (paperword != CB_ERR)
        dm->u.s.dmDefaultSource = paperword;
    else
        FIXME("could not get dialog text for papersize cmbbox?\n");

    GlobalUnlock(pda->pdlg.hDevNames);
    GlobalUnlock(pda->pdlg.hDevMode);

    return TRUE;
}

static BOOL
PRINTDLG_PS_UpdateDlgStructW(HWND hDlg, PageSetupDataW *pda) {
    DEVNAMES	*dn;
    DEVMODEW	*dm;
    LPWSTR	devname,portname;
    WCHAR	papername[64];
    WCHAR	buf[200];

    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	= ((WCHAR*)dn)+dn->wDeviceOffset;
    portname	= ((WCHAR*)dn)+dn->wOutputOffset;

    /* Save paper size into device context */
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb2,devname,portname,dm);
    /* Save paper source into device context */
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb3,devname,portname,dm);

    if (GetDlgItemTextW(hDlg,cmb2,papername,sizeof(papername))>0) {
    	PRINTDLG_PaperSizeW(&(pda->pdlg),papername,&(pda->dlga->ptPaperSize));
	pda->dlga->ptPaperSize.x = _c_10mm2size((LPPAGESETUPDLGA)pda->dlga,pda->dlga->ptPaperSize.x);
	pda->dlga->ptPaperSize.y = _c_10mm2size((LPPAGESETUPDLGA)pda->dlga,pda->dlga->ptPaperSize.y);
    } else
	FIXME("could not get dialog text for papersize cmbbox?\n");
#define GETVAL(id,val) if (GetDlgItemTextW(hDlg,id,buf,sizeof(buf)/sizeof(buf[0]))>0) { val = _c_str2sizeW(pda->dlga,buf); } else { FIXME("could not get dlgitemtextw for %x\n",id); }
    GETVAL(edt4,pda->dlga->rtMargin.left);
    GETVAL(edt5,pda->dlga->rtMargin.top);
    GETVAL(edt6,pda->dlga->rtMargin.right);
    GETVAL(edt7,pda->dlga->rtMargin.bottom);
#undef GETVAL

    /* If we are in landscape, swap x and y of page size */
    if (IsDlgButtonChecked(hDlg, rad2)) {
	DWORD tmp;
	tmp = pda->dlga->ptPaperSize.x;
	pda->dlga->ptPaperSize.x = pda->dlga->ptPaperSize.y;
	pda->dlga->ptPaperSize.y = tmp;
    }
    GlobalUnlock(pda->pdlg.hDevNames);
    GlobalUnlock(pda->pdlg.hDevMode);
    return TRUE;
}

/**********************************************************************************************
 * PRINTDLG_PS_ChangeActivePrinerA
 *
 * Redefines hDevMode and hDevNames HANDLES and initialises it.
 * 
 * PARAMS
 * 	name	[in] 	 Name of a printer for activation
 * 	pda	[in/out] ptr to PageSetupDataA structure
 * 	
 * RETURN 
 * 	TRUE if success
 * 	FALSE if fail
 */
static BOOL
PRINTDLG_PS_ChangeActivePrinterA(LPSTR name, PageSetupDataA *pda){
	HANDLE            hprn;
	DWORD             needed;
	LPPRINTER_INFO_2A lpPrinterInfo;
	LPDRIVER_INFO_3A  lpDriverInfo;
	DEVMODEA          *pDevMode, *dm;
	
	if(!OpenPrinterA(name, &hprn, NULL)){
		ERR("Can't open printer %s\n", name);
		return FALSE;
	}
	GetPrinterA(hprn, 2, NULL, 0, &needed);
	lpPrinterInfo = HeapAlloc(GetProcessHeap(), 0, needed);
	GetPrinterA(hprn, 2, (LPBYTE)lpPrinterInfo, needed, &needed);
	GetPrinterDriverA(hprn, NULL, 3, NULL, 0, &needed);
	lpDriverInfo  = HeapAlloc(GetProcessHeap(), 0, needed);
	if(!GetPrinterDriverA(hprn, NULL, 3, (LPBYTE)lpDriverInfo, needed, &needed)) {
		ERR("GetPrinterDriverA failed for %s, fix your config!\n", lpPrinterInfo->pPrinterName);
		return FALSE;
	}
	ClosePrinter(hprn);
	
	needed = DocumentPropertiesA(0, 0, name, NULL, NULL, 0);
	if(needed == -1) {
		ERR("DocumentProperties fails on %s\n", debugstr_a(name));
		return FALSE;
	}
	pDevMode = HeapAlloc(GetProcessHeap(), 0, needed);
	DocumentPropertiesA(0, 0, name, pDevMode, NULL, DM_OUT_BUFFER);

	pda->pdlg.hDevMode = GlobalReAlloc(pda->pdlg.hDevMode,
			                 pDevMode->dmSize + pDevMode->dmDriverExtra,
							 GMEM_MOVEABLE);
	dm = GlobalLock(pda->pdlg.hDevMode);
	memcpy(dm, pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);
	
	PRINTDLG_CreateDevNames(&(pda->pdlg.hDevNames),
			lpDriverInfo->pDriverPath,
			lpPrinterInfo->pPrinterName,
			lpPrinterInfo->pPortName);
	
	GlobalUnlock(pda->pdlg.hDevMode);
	HeapFree(GetProcessHeap(), 0, pDevMode);
	HeapFree(GetProcessHeap(), 0, lpPrinterInfo);
	HeapFree(GetProcessHeap(), 0, lpDriverInfo);
	return TRUE;
}

/****************************************************************************************
 *  PRINTDLG_PS_ChangePrinterA
 *
 *  Fills Printers, Paper and Source combo
 *
 *  RETURNS 
 *   TRUE
 */
static BOOL
PRINTDLG_PS_ChangePrinterA(HWND hDlg, PageSetupDataA *pda) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    LPSTR	devname,portname;
	
    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	    = ((char*)dn)+dn->wDeviceOffset;
    portname	= ((char*)dn)+dn->wOutputOffset;
    PRINTDLG_SetUpPrinterListComboA(hDlg, cmb1, devname);
    PRINTDLG_SetUpPaperComboBoxA(hDlg,cmb2,devname,portname,dm);
    PRINTDLG_SetUpPaperComboBoxA(hDlg,cmb3,devname,portname,dm);
    GlobalUnlock(pda->pdlg.hDevNames);
    GlobalUnlock(pda->pdlg.hDevMode);
    return TRUE;
}

static BOOL
PRINTDLG_PS_ChangePrinterW(HWND hDlg, PageSetupDataW *pda) {
    DEVNAMES	*dn;
    DEVMODEW	*dm;
    LPWSTR	devname,portname;

    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	= ((WCHAR*)dn)+dn->wDeviceOffset;
    portname	= ((WCHAR*)dn)+dn->wOutputOffset;
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb2,devname,portname,dm);
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb3,devname,portname,dm);
    GlobalUnlock(pda->pdlg.hDevNames);
    GlobalUnlock(pda->pdlg.hDevMode);
    return TRUE;
}

/******************************************************************************************
 * PRINTDLG_PS_ChangePaperPrev 
 * 
 * Changes paper preview size / position
 *
 * PARAMS:
 * 	pda		[i] Pointer for current PageSetupDataA structure
 *
 * RETURNS:
 *  always - TRUE
 */
static BOOL 
PRINTDLG_PS_ChangePaperPrev(const PageSetupDataA *pda)
{
    LONG width, height, x, y;
    RECT rtTmp;
    
    if(pda->curdlg.ptPaperSize.x > pda->curdlg.ptPaperSize.y) {
	width  = pda->rtDrawRect.right - pda->rtDrawRect.left;
	height = pda->curdlg.ptPaperSize.y * width / pda->curdlg.ptPaperSize.x;
    } else {
	height = pda->rtDrawRect.bottom - pda->rtDrawRect.top;
	width  = pda->curdlg.ptPaperSize.x * height / pda->curdlg.ptPaperSize.y;
    }
    x = (pda->rtDrawRect.right + pda->rtDrawRect.left - width) / 2;
    y = (pda->rtDrawRect.bottom + pda->rtDrawRect.top - height) / 2;
    TRACE("rtDrawRect(%d, %d, %d, %d) x=%d, y=%d, w=%d, h=%d\n",
	pda->rtDrawRect.left, pda->rtDrawRect.top, pda->rtDrawRect.right, pda->rtDrawRect.bottom,
	x, y, width, height);

#define SHADOW 4
    MoveWindow(GetDlgItem(pda->hDlg, rct2), x+width, y+SHADOW, SHADOW, height, FALSE);
    MoveWindow(GetDlgItem(pda->hDlg, rct3), x+SHADOW, y+height, width, SHADOW, FALSE);
    MoveWindow(GetDlgItem(pda->hDlg, rct1), x, y, width, height, FALSE);
    memcpy(&rtTmp, &pda->rtDrawRect, sizeof(RECT));
    rtTmp.right  += SHADOW;
    rtTmp.bottom += SHADOW;
#undef SHADOW 

    InvalidateRect(pda->hDlg, &rtTmp, TRUE);
    return TRUE;
}

#define GETVAL(idc,val) \
if(msg == EN_CHANGE){ \
    if (GetDlgItemTextA(hDlg,idc,buf,sizeof(buf)) > 0)\
        val = _c_str2sizeA(pda->dlga,buf); \
    else\
	FIXME("could not get dlgitemtexta for %x\n",id);  \
}

/********************************************************************************
 * PRINTDLG_PS_WMCommandA
 * process WM_COMMAND message for PageSetupDlgA
 *
 * PARAMS
 *  hDlg 	[in] 	Main dialog HANDLE 
 *  wParam 	[in]	WM_COMMAND wParam
 *  lParam	[in]	WM_COMMAND lParam
 *  pda		[in/out] ptr to PageSetupDataA
 */

static BOOL
PRINTDLG_PS_WMCommandA(
    HWND hDlg, WPARAM wParam, LPARAM lParam, PageSetupDataA *pda
) {
    WORD msg = HIWORD(wParam);
    WORD id  = LOWORD(wParam);
    char buf[200];
	
    TRACE("loword (lparam) %d, wparam 0x%lx, lparam %08lx\n",
	    LOWORD(lParam),wParam,lParam);
    switch (id)  {
    case IDOK:
        if (!PRINTDLG_PS_UpdateDlgStructA(hDlg, pda))
	    return(FALSE);
	EndDialog(hDlg, TRUE);
	return TRUE ;

    case IDCANCEL:
        EndDialog(hDlg, FALSE);
	return FALSE ;

    case psh3: {
	pda->pdlg.Flags		= 0;
	pda->pdlg.hwndOwner	= hDlg;
	if (PrintDlgA(&(pda->pdlg)))
	    PRINTDLG_PS_ChangePrinterA(hDlg,pda);
        }
	return TRUE;
    case rad1:
	    if (pda->curdlg.ptPaperSize.x > pda->curdlg.ptPaperSize.y){
	        DWORD tmp = pda->curdlg.ptPaperSize.x;
		pda->curdlg.ptPaperSize.x = pda->curdlg.ptPaperSize.y;
		pda->curdlg.ptPaperSize.y = tmp;
	    }
	    PRINTDLG_PS_ChangePaperPrev(pda);
	break;
    case rad2:
	    if (pda->curdlg.ptPaperSize.y > pda->curdlg.ptPaperSize.x){
	        DWORD tmp = pda->curdlg.ptPaperSize.x;
		pda->curdlg.ptPaperSize.x = pda->curdlg.ptPaperSize.y;
		pda->curdlg.ptPaperSize.y = tmp;
	    }
	    PRINTDLG_PS_ChangePaperPrev(pda);
	    break;
    case cmb1: /* Printer combo */
	    if(msg == CBN_SELCHANGE){
		char crPrinterName[256];
		GetDlgItemTextA(hDlg, id, crPrinterName, 255);
		PRINTDLG_PS_ChangeActivePrinterA(crPrinterName, pda);
		PRINTDLG_PS_ChangePrinterA(hDlg, pda);
	    }
	    break;
    case cmb2: /* Paper combo */
	if(msg == CBN_SELCHANGE){
	    DWORD paperword = SendDlgItemMessageA(hDlg,cmb2,CB_GETITEMDATA,
	        SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0), 0);
   	    if (paperword != CB_ERR) {
	        PRINTDLG_PaperSizeA(&(pda->pdlg), paperword,&(pda->curdlg.ptPaperSize));
	        pda->curdlg.ptPaperSize.x = _c_10mm2size(pda->dlga,pda->curdlg.ptPaperSize.x);
	        pda->curdlg.ptPaperSize.y = _c_10mm2size(pda->dlga,pda->curdlg.ptPaperSize.y);
	    
		if (IsDlgButtonChecked(hDlg, rad2)) {
	            DWORD tmp = pda->curdlg.ptPaperSize.x;
 		    pda->curdlg.ptPaperSize.x = pda->curdlg.ptPaperSize.y;
		    pda->curdlg.ptPaperSize.y = tmp;
	        }
	        PRINTDLG_PS_ChangePaperPrev(pda);
	    } else
	        FIXME("could not get dialog text for papersize cmbbox?\n");
	}    
	break;
    case cmb3:
	if(msg == CBN_SELCHANGE){
	    DEVMODEA *dm = GlobalLock(pda->pdlg.hDevMode);
	    dm->u.s.dmDefaultSource = SendDlgItemMessageA(hDlg, cmb3,CB_GETITEMDATA,
                SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0), 0);
	    GlobalUnlock(pda->pdlg.hDevMode);
	}
	break;
    case psh2:                       /* Printer Properties button */
       {
	    HANDLE hPrinter;
	    char   PrinterName[256];
	    DEVMODEA *dm;
	    LRESULT  count;
	    int      i;
	    
            GetDlgItemTextA(hDlg, cmb1, PrinterName, 255);
	    if (!OpenPrinterA(PrinterName, &hPrinter, NULL)) {
	        FIXME("Call to OpenPrinter did not succeed!\n");
		break;
	    }
	    dm = GlobalLock(pda->pdlg.hDevMode);
	    DocumentPropertiesA(hDlg, hPrinter, PrinterName, dm, dm,
	                        DM_IN_BUFFER | DM_OUT_BUFFER | DM_IN_PROMPT);
	    ClosePrinter(hPrinter);
	    /* Changing paper */
	    PRINTDLG_PaperSizeA(&(pda->pdlg), dm->u.s.dmPaperSize, &(pda->curdlg.ptPaperSize));
	    pda->curdlg.ptPaperSize.x = _c_10mm2size(pda->dlga, pda->curdlg.ptPaperSize.x);
	    pda->curdlg.ptPaperSize.y = _c_10mm2size(pda->dlga, pda->curdlg.ptPaperSize.y);
            if (dm->u.s.dmOrientation == DMORIENT_LANDSCAPE){
                DWORD tmp = pda->curdlg.ptPaperSize.x;
                pda->curdlg.ptPaperSize.x = pda->curdlg.ptPaperSize.y;
                pda->curdlg.ptPaperSize.y = tmp;
		CheckRadioButton(hDlg, rad1, rad2, rad2);
	    }
	    else
		CheckRadioButton(hDlg, rad1, rad2, rad1);
	    /* Changing paper preview */
	    PRINTDLG_PS_ChangePaperPrev(pda);
	    /* Selecting paper in combo */
	    count = SendDlgItemMessageA(hDlg, cmb2, CB_GETCOUNT, 0, 0);
	    if(count != CB_ERR){ 
                for(i=0; i<count; ++i){
		    if(SendDlgItemMessageA(hDlg, cmb2, CB_GETITEMDATA, i, 0) == dm->u.s.dmPaperSize) {
			SendDlgItemMessageA(hDlg, cmb2, CB_SETCURSEL, i, 0);
			break;
		    }
		}
	    }
									    
	    GlobalUnlock(pda->pdlg.hDevMode);
	    break;
	}       
    case edt4:
    	GETVAL(id, pda->curdlg.rtMargin.left);
	break;
    case edt5:
    	GETVAL(id, pda->curdlg.rtMargin.right);
	break;
    case edt6:
    	GETVAL(id, pda->curdlg.rtMargin.top);
	break;
    case edt7:
    	GETVAL(id, pda->curdlg.rtMargin.bottom);
	break;
    }
    InvalidateRect(GetDlgItem(hDlg, rct1), NULL, TRUE);
    return FALSE;
}
#undef GETVAL			   

static BOOL
PRINTDLG_PS_WMCommandW(
    HWND hDlg, WPARAM wParam, LPARAM lParam, PageSetupDataW *pda
) {
    TRACE("loword (lparam) %d, wparam 0x%lx, lparam %08lx\n",
	    LOWORD(lParam),wParam,lParam);
    switch (LOWORD(wParam))  {
    case IDOK:
        if (!PRINTDLG_PS_UpdateDlgStructW(hDlg, pda))
	    return(FALSE);
	EndDialog(hDlg, TRUE);
	return TRUE ;

    case IDCANCEL:
        EndDialog(hDlg, FALSE);
	return FALSE ;

    case psh3: {
	pda->pdlg.Flags		= 0;
	pda->pdlg.hwndOwner	= hDlg;
	if (PrintDlgW(&(pda->pdlg)))
	    PRINTDLG_PS_ChangePrinterW(hDlg,pda);
	return TRUE;
    }
    }
    return FALSE;
}


/***********************************************************************
 *           DefaultPagePaintHook
 * Default hook paint procedure that receives WM_PSD_* messages from the dialog box 
 * whenever the sample page is redrawn.
*/

static UINT_PTR
PRINTDLG_DefaultPagePaintHook(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam,
                              const PageSetupDataA *pda)
{
    LPRECT lprc = (LPRECT) lParam;
    HDC hdc = (HDC) wParam;
    HPEN hpen, holdpen;
    LOGFONTW lf;
    HFONT hfont, holdfont;
    INT oldbkmode;
    TRACE("uMsg: WM_USER+%d\n",uMsg-WM_USER);
    /* Call user paint hook if enable */
    if (pda->dlga->Flags & PSD_ENABLEPAGEPAINTHOOK)
        if (pda->dlga->lpfnPagePaintHook(hwndDlg, uMsg, wParam, lParam))
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
                        sizeof(wszFakeDocumentText)/sizeof(wszFakeDocumentText[0]));

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
    PageSetupDataA *pda;
    int papersize=0, orientation=0; /* FIXME: set this values for user paint hook */
    double scalx, scaly;
#define CALLPAINTHOOK(msg,lprc) PRINTDLG_DefaultPagePaintHook( hWnd, msg, (WPARAM)hdc, (LPARAM)lprc, pda)

    if (uMsg != WM_PAINT)
        return CallWindowProcA(lpfnStaticWndProc, hWnd, uMsg, wParam, lParam);

    /* Processing WM_PAINT message */
    pda = (PageSetupDataA*)GetPropA(hWnd, "__WINE_PAGESETUPDLGDATA");
    if (!pda) {
        WARN("__WINE_PAGESETUPDLGDATA prop not set?\n");
        return FALSE;
    }
    if (PRINTDLG_DefaultPagePaintHook(hWnd, WM_PSD_PAGESETUPDLG, MAKELONG(papersize, orientation), (LPARAM)pda->dlga, pda))
        return FALSE;

    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rcClient);
    
    scalx = rcClient.right  / (double)pda->curdlg.ptPaperSize.x;
    scaly = rcClient.bottom / (double)pda->curdlg.ptPaperSize.y; 
    rcMargin = rcClient;
 
    rcMargin.left   += (LONG)pda->curdlg.rtMargin.left   * scalx;
    rcMargin.top    += (LONG)pda->curdlg.rtMargin.top    * scalx;
    rcMargin.right  -= (LONG)pda->curdlg.rtMargin.right  * scaly;
    rcMargin.bottom -= (LONG)pda->curdlg.rtMargin.bottom * scaly;
    
    /* if the space is too small then we make sure to not draw anything */
    rcMargin.left = min(rcMargin.left, rcMargin.right);
    rcMargin.top = min(rcMargin.top, rcMargin.bottom);

    if (!CALLPAINTHOOK(WM_PSD_FULLPAGERECT, &rcClient) &&
        !CALLPAINTHOOK(WM_PSD_MINMARGINRECT, &rcMargin) )
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

        CALLPAINTHOOK(WM_PSD_MARGINRECT, &rcMargin);

        /* give text a bit of a space from the frame */
        rcMargin.left += 2;
        rcMargin.top += 2;
        rcMargin.right -= 2;
        rcMargin.bottom -= 2;
        
        /* if the space is too small then we make sure to not draw anything */
        rcMargin.left = min(rcMargin.left, rcMargin.right);
        rcMargin.top = min(rcMargin.top, rcMargin.bottom);

        CALLPAINTHOOK(WM_PSD_GREEKTEXTRECT, &rcMargin);
    }

    EndPaint(hWnd, &ps);
    return FALSE;
#undef CALLPAINTHOOK
}

/***********************************************************************
 *           PRINTDLG_PageDlgProcA
 * Message handler for PageSetupDlgA
 */
static INT_PTR CALLBACK
PRINTDLG_PageDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DEVMODEA		*dm;
    PageSetupDataA	*pda;
    INT_PTR		res = FALSE;
    HWND 		hDrawWnd;

    if (uMsg == WM_INITDIALOG) { /*Init dialog*/
        pda = (PageSetupDataA*)lParam;
	pda->hDlg   = hDlg; /* saving handle to main window to PageSetupDataA structure */
	memcpy(&pda->curdlg, pda->dlga, sizeof(pda->curdlg));
	
	hDrawWnd = GetDlgItem(hDlg, rct1); 
        TRACE("set property to %p\n", pda);
	SetPropA(hDlg, "__WINE_PAGESETUPDLGDATA", pda);
	SetPropA(hDrawWnd, "__WINE_PAGESETUPDLGDATA", pda);
	GetWindowRect(hDrawWnd, &pda->rtDrawRect); /* Calculating rect in client coordinates where paper draws */
	ScreenToClient(hDlg, (LPPOINT)&pda->rtDrawRect);
	ScreenToClient(hDlg, (LPPOINT)(&pda->rtDrawRect.right));
        lpfnStaticWndProc = (WNDPROC)SetWindowLongPtrW(
            hDrawWnd,
            GWLP_WNDPROC,
            (ULONG_PTR)PRINTDLG_PagePaintProc);
	
	/* FIXME: Paint hook. Must it be at begin of initializtion or at end? */
	res = TRUE;
	if (pda->dlga->Flags & PSD_ENABLEPAGESETUPHOOK) {
            if (!pda->dlga->lpfnPageSetupHook(hDlg,uMsg,wParam,(LPARAM)pda->dlga))
		FIXME("Setup page hook failed?\n");
	}

	/* if printer button disabled */
	if (pda->dlga->Flags & PSD_DISABLEPRINTER)
            EnableWindow(GetDlgItem(hDlg, psh3), FALSE);
	/* if margin edit boxes disabled */
	if (pda->dlga->Flags & PSD_DISABLEMARGINS) {
            EnableWindow(GetDlgItem(hDlg, edt4), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt5), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt6), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt7), FALSE);
	}
        /* Set orientation radiobutton properly */
        dm = GlobalLock(pda->dlga->hDevMode);
        if (dm->u.s.dmOrientation == DMORIENT_LANDSCAPE)
            CheckRadioButton(hDlg, rad1, rad2, rad2);
	else /* this is default if papersize is not set */
            CheckRadioButton(hDlg, rad1, rad2, rad1);
        GlobalUnlock(pda->dlga->hDevMode);

	/* if orientation disabled */
	if (pda->dlga->Flags & PSD_DISABLEORIENTATION) {
	    EnableWindow(GetDlgItem(hDlg,rad1),FALSE);
	    EnableWindow(GetDlgItem(hDlg,rad2),FALSE);
	}
	/* We fill them out enabled or not */
	if (pda->dlga->Flags & PSD_MARGINS) {
	    char str[100];
	    _c_size2strA(pda,pda->dlga->rtMargin.left,str);
	    SetDlgItemTextA(hDlg,edt4,str);
	    _c_size2strA(pda,pda->dlga->rtMargin.top,str);
	    SetDlgItemTextA(hDlg,edt5,str);
	    _c_size2strA(pda,pda->dlga->rtMargin.right,str);
	    SetDlgItemTextA(hDlg,edt6,str);
	    _c_size2strA(pda,pda->dlga->rtMargin.bottom,str);
	    SetDlgItemTextA(hDlg,edt7,str);
	} else {
	    /* default is 1 inch */
	    DWORD size = _c_inch2size(pda->dlga,1000);
	    char	str[20];
	    _c_size2strA(pda,size,str);
	    SetDlgItemTextA(hDlg,edt4,str);
	    SetDlgItemTextA(hDlg,edt5,str);
	    SetDlgItemTextA(hDlg,edt6,str);
	    SetDlgItemTextA(hDlg,edt7,str);
	    pda->curdlg.rtMargin.left   = size;
	    pda->curdlg.rtMargin.top    = size;
	    pda->curdlg.rtMargin.right  = size;
	    pda->curdlg.rtMargin.bottom = size;
	}
	/* if paper disabled */
	if (pda->dlga->Flags & PSD_DISABLEPAPER) {
	    EnableWindow(GetDlgItem(hDlg,cmb2),FALSE);
	    EnableWindow(GetDlgItem(hDlg,cmb3),FALSE);
	}
	/* filling combos: printer, paper, source. selecting current printer (from DEVMODEA) */
        PRINTDLG_PS_ChangePrinterA(hDlg, pda);
	dm = GlobalLock(pda->pdlg.hDevMode);
	if(dm){
	    dm->u.s.dmDefaultSource = 15; /*FIXME: Automatic select. Does it always 15 at start? */
	    PRINTDLG_PaperSizeA(&(pda->pdlg), dm->u.s.dmPaperSize, &pda->curdlg.ptPaperSize);
            GlobalUnlock(pda->pdlg.hDevMode);
	    pda->curdlg.ptPaperSize.x = _c_10mm2size(pda->dlga, pda->curdlg.ptPaperSize.x);
	    pda->curdlg.ptPaperSize.y = _c_10mm2size(pda->dlga, pda->curdlg.ptPaperSize.y);
            if (IsDlgButtonChecked(hDlg, rad2) == BST_CHECKED) { /* Landscape orientation */
                DWORD tmp = pda->curdlg.ptPaperSize.y;
                pda->curdlg.ptPaperSize.y = pda->curdlg.ptPaperSize.x;
                pda->curdlg.ptPaperSize.x = tmp;
            }
	} else 
	    WARN("GlobalLock(pda->pdlg.hDevMode) fail? hDevMode=%p\n", pda->pdlg.hDevMode);
	/* Drawing paper prev */
	PRINTDLG_PS_ChangePaperPrev(pda);
	return TRUE;
    } else {
	pda = (PageSetupDataA*)GetPropA(hDlg,"__WINE_PAGESETUPDLGDATA");
	if (!pda) {
	    WARN("__WINE_PAGESETUPDLGDATA prop not set?\n");
	    return FALSE;
	}
	if (pda->dlga->Flags & PSD_ENABLEPAGESETUPHOOK) {
	    res = pda->dlga->lpfnPageSetupHook(hDlg,uMsg,wParam,lParam);
	    if (res) return res;
	}
    }
    switch (uMsg) {
    case WM_COMMAND:
        return PRINTDLG_PS_WMCommandA(hDlg, wParam, lParam, pda);
    }
    return FALSE;
}

static INT_PTR CALLBACK
PageDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR __WINE_PAGESETUPDLGDATA[] = 
	{ '_', '_', 'W', 'I', 'N', 'E', '_', 'P', 'A', 'G', 'E', 
	  'S', 'E', 'T', 'U', 'P', 'D', 'L', 'G', 'D', 'A', 'T', 'A', 0 };
    PageSetupDataW	*pda;
    BOOL		res = FALSE;

    if (uMsg==WM_INITDIALOG) {
	res = TRUE;
        pda = (PageSetupDataW*)lParam;
	SetPropW(hDlg, __WINE_PAGESETUPDLGDATA, pda);
	if (pda->dlga->Flags & PSD_ENABLEPAGESETUPHOOK) {
	    res = pda->dlga->lpfnPageSetupHook(hDlg,uMsg,wParam,(LPARAM)pda->dlga);
	    if (!res) {
		FIXME("Setup page hook failed?\n");
		res = TRUE;
	    }
	}

	if (pda->dlga->Flags & PSD_ENABLEPAGEPAINTHOOK) {
	    FIXME("PagePaintHook not yet implemented!\n");
	}
	if (pda->dlga->Flags & PSD_DISABLEPRINTER)
            EnableWindow(GetDlgItem(hDlg, psh3), FALSE);
	if (pda->dlga->Flags & PSD_DISABLEMARGINS) {
            EnableWindow(GetDlgItem(hDlg, edt4), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt5), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt6), FALSE);
            EnableWindow(GetDlgItem(hDlg, edt7), FALSE);
	}
	/* width larger as height -> landscape */
	if (pda->dlga->ptPaperSize.x > pda->dlga->ptPaperSize.y)
            CheckRadioButton(hDlg, rad1, rad2, rad2);
	else /* this is default if papersize is not set */
            CheckRadioButton(hDlg, rad1, rad2, rad1);
	if (pda->dlga->Flags & PSD_DISABLEORIENTATION) {
	    EnableWindow(GetDlgItem(hDlg,rad1),FALSE);
	    EnableWindow(GetDlgItem(hDlg,rad2),FALSE);
	}
	/* We fill them out enabled or not */
	if (pda->dlga->Flags & PSD_MARGINS) {
	    WCHAR str[100];
	    _c_size2strW(pda,pda->dlga->rtMargin.left,str);
	    SetDlgItemTextW(hDlg,edt4,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.top,str);
	    SetDlgItemTextW(hDlg,edt5,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.right,str);
	    SetDlgItemTextW(hDlg,edt6,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.bottom,str);
	    SetDlgItemTextW(hDlg,edt7,str);
	} else {
	    /* default is 1 inch */
	    DWORD size = _c_inch2size((LPPAGESETUPDLGA)pda->dlga,1000);
	    WCHAR	str[20];
	    _c_size2strW(pda,size,str);
	    SetDlgItemTextW(hDlg,edt4,str);
	    SetDlgItemTextW(hDlg,edt5,str);
	    SetDlgItemTextW(hDlg,edt6,str);
	    SetDlgItemTextW(hDlg,edt7,str);
	}
	PRINTDLG_PS_ChangePrinterW(hDlg,pda);
	if (pda->dlga->Flags & PSD_DISABLEPAPER) {
	    EnableWindow(GetDlgItem(hDlg,cmb2),FALSE);
	    EnableWindow(GetDlgItem(hDlg,cmb3),FALSE);
	}

	return TRUE;
    } else {
	pda = (PageSetupDataW*)GetPropW(hDlg, __WINE_PAGESETUPDLGDATA);
	if (!pda) {
	    WARN("__WINE_PAGESETUPDLGDATA prop not set?\n");
	    return FALSE;
	}
	if (pda->dlga->Flags & PSD_ENABLEPAGESETUPHOOK) {
	    res = pda->dlga->lpfnPageSetupHook(hDlg,uMsg,wParam,lParam);
	    if (res) return res;
	}
    }
    switch (uMsg) {
    case WM_COMMAND:
        return PRINTDLG_PS_WMCommandW(hDlg, wParam, lParam, pda);
    }
    return FALSE;
}

/***********************************************************************
 *            PageSetupDlgA  (COMDLG32.@)
 *
 *  Displays the the PAGE SETUP dialog box, which enables the user to specify
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

BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
    HGLOBAL		hDlgTmpl;
    LPVOID		ptr;
    BOOL		bRet;
    PageSetupDataA	*pda;
    PRINTDLGA		pdlg;

    /* TRACE */
    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = psd_flags;
	for( ; pflag->name; pflag++) {
	    if(setupdlg->Flags & pflag->flag) {
	        strcat(flagstr, pflag->name);
	        strcat(flagstr, "|");
	    }
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
              "hinst %p, flags %08x (%s)\n",
	      setupdlg, setupdlg->hwndOwner, setupdlg->hDevMode,
	      setupdlg->hDevNames,
	      setupdlg->hInstance, setupdlg->Flags, flagstr);
    }
    /* Checking setupdlg structure */
    if (setupdlg == NULL) {
	   COMDLG32_SetCommDlgExtendedError(CDERR_INITIALIZATION);
	   return FALSE;
    }
    if(setupdlg->lStructSize != sizeof(PAGESETUPDLGA)) {
	   COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
	   return FALSE;
    }
    if ((setupdlg->Flags & PSD_ENABLEPAGEPAINTHOOK) &&
        (setupdlg->lpfnPagePaintHook == NULL)) {
            COMDLG32_SetCommDlgExtendedError(CDERR_NOHOOK);
            return FALSE;
        }

    /* Initialize default printer struct. If no printer device info is specified
       retrieve the default printer data. */
    memset(&pdlg,0,sizeof(pdlg));
    pdlg.lStructSize	= sizeof(pdlg);
    if (setupdlg->hDevMode && setupdlg->hDevNames) {
        pdlg.hDevMode = setupdlg->hDevMode;
        pdlg.hDevNames = setupdlg->hDevNames;
    } else {
        pdlg.Flags = PD_RETURNDEFAULT;
        bRet = PrintDlgA(&pdlg);
        if (!bRet){
            if (!(setupdlg->Flags & PSD_NOWARNING)) {
                char errstr[256];
                LoadStringA(COMDLG32_hInstance, PD32_NO_DEFAULT_PRINTER, errstr, 255);
                MessageBoxA(setupdlg->hwndOwner, errstr, 0, MB_OK | MB_ICONERROR);
            }
            return FALSE;
        }
    }

    /* short cut exit, just return default values */
    if (setupdlg->Flags & PSD_RETURNDEFAULT) {
	DEVMODEA *dm;
	
	dm = GlobalLock(pdlg.hDevMode);
    	PRINTDLG_PaperSizeA(&pdlg, dm->u.s.dmPaperSize, &setupdlg->ptPaperSize);
	GlobalUnlock(pdlg.hDevMode);
	setupdlg->ptPaperSize.x=_c_10mm2size(setupdlg,setupdlg->ptPaperSize.x);
	setupdlg->ptPaperSize.y=_c_10mm2size(setupdlg,setupdlg->ptPaperSize.y);
	return TRUE;
    }

    /* get dialog template */
    hDlgTmpl = PRINTDLG_GetPGSTemplateA(setupdlg);
    if (!hDlgTmpl) {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    ptr = LockResource( hDlgTmpl );
    if (!ptr) {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    
    pda = HeapAlloc(GetProcessHeap(),0,sizeof(*pda));
    pda->dlga = setupdlg;
    memcpy(&pda->pdlg,&pdlg,sizeof(pdlg));

    bRet = (0<DialogBoxIndirectParamA(
		setupdlg->hInstance,
		ptr,
		setupdlg->hwndOwner,
		PRINTDLG_PageDlgProcA,
		(LPARAM)pda)
    );

    HeapFree(GetProcessHeap(),0,pda);
    return bRet;
}
/***********************************************************************
 *            PageSetupDlgW  (COMDLG32.@)
 *
 * See PageSetupDlgA.
 */
BOOL WINAPI PageSetupDlgW(LPPAGESETUPDLGW setupdlg) {
    HGLOBAL		hDlgTmpl;
    LPVOID		ptr;
    BOOL		bRet;
    PageSetupDataW	*pdw;
    PRINTDLGW		pdlg;

    FIXME("Unicode implementation is not done yet\n");
    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = psd_flags;
	for( ; pflag->name; pflag++) {
	    if(setupdlg->Flags & pflag->flag) {
	        strcat(flagstr, pflag->name);
	        strcat(flagstr, "|");
	    }
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
              "hinst %p, flags %08x (%s)\n",
	      setupdlg, setupdlg->hwndOwner, setupdlg->hDevMode,
	      setupdlg->hDevNames,
	      setupdlg->hInstance, setupdlg->Flags, flagstr);
    }

    /* Initialize default printer struct. If no printer device info is specified
       retrieve the default printer data. */
    memset(&pdlg,0,sizeof(pdlg));
    pdlg.lStructSize	= sizeof(pdlg);
    if (setupdlg->hDevMode && setupdlg->hDevNames) {
        pdlg.hDevMode = setupdlg->hDevMode;
        pdlg.hDevNames = setupdlg->hDevNames;
    } else {
        pdlg.Flags = PD_RETURNDEFAULT;
        bRet = PrintDlgW(&pdlg);
        if (!bRet){
            if (!(setupdlg->Flags & PSD_NOWARNING)) {
                WCHAR errstr[256];
                LoadStringW(COMDLG32_hInstance, PD32_NO_DEFAULT_PRINTER, errstr, 255);
                MessageBoxW(setupdlg->hwndOwner, errstr, 0, MB_OK | MB_ICONERROR);
            }
            return FALSE;
        }
    }

    /* short cut exit, just return default values */
    if (setupdlg->Flags & PSD_RETURNDEFAULT) {
        static const WCHAR a4[] = {'A','4',0};
	setupdlg->hDevMode	= pdlg.hDevMode;
	setupdlg->hDevNames	= pdlg.hDevNames;
	/* FIXME: Just return "A4" for now. */
    	PRINTDLG_PaperSizeW(&pdlg,a4,&setupdlg->ptPaperSize);
	setupdlg->ptPaperSize.x=_c_10mm2size((LPPAGESETUPDLGA)setupdlg,setupdlg->ptPaperSize.x);
	setupdlg->ptPaperSize.y=_c_10mm2size((LPPAGESETUPDLGA)setupdlg,setupdlg->ptPaperSize.y);
	return TRUE;
    }
    hDlgTmpl = PRINTDLG_GetPGSTemplateW(setupdlg);
    if (!hDlgTmpl) {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    ptr = LockResource( hDlgTmpl );
    if (!ptr) {
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	return FALSE;
    }
    pdw = HeapAlloc(GetProcessHeap(),0,sizeof(*pdw));
    pdw->dlga = setupdlg;
    memcpy(&pdw->pdlg,&pdlg,sizeof(pdlg));

    bRet = (0<DialogBoxIndirectParamW(
		setupdlg->hInstance,
		ptr,
		setupdlg->hwndOwner,
		PageDlgProcW,
		(LPARAM)pdw)
    );
    return bRet;
}

/***********************************************************************
 *	PrintDlgExA (COMDLG32.@)
 *
 * See PrintDlgExW.
 *
 * FIXME
 *   Stub
 */
HRESULT WINAPI PrintDlgExA(LPPRINTDLGEXA lpPrintDlgExA)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}

/***********************************************************************
 *	PrintDlgExW (COMDLG32.@)
 *
 * Display the the PRINT dialog box, which enables the user to specify
 * specific properties of the print job.  The property sheet can also have
 * additional application-specific and driver-specific property pages.
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
 * FIXME
 *   Stub
 */
HRESULT WINAPI PrintDlgExW(LPPRINTDLGEXW lpPrintDlgExW)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}
