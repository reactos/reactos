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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//#define NONAMELESSUNION
//#define NONAMELESSSTRUCT
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

#include "ros.h"

/* Yes these constants are the same, but we're just copying win98 */
#define UPDOWN_ID 0x270f
#define MAX_COPIES 9999

/* Debugging info */
static struct pd_flags psd_flags[] = {
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
    char buf[260];
    DWORD dwBufLen = sizeof(buf);
    BOOL res;
    if(!GetDefaultPrinterA(buf, &dwBufLen))
        return FALSE;
    res = OpenPrinterA(buf, hprn, NULL);
    if (!res)
	FIXME("Could not open printer %s?!\n",buf);
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

	/* ansi is ok */
        char buf[260];
        DWORD dwBufLen = sizeof(buf);
        FIXME("Can't find '%s' in printer list so trying to find default\n",
	      debugstr_w(name));
	if(!GetDefaultPrinterA(buf, &dwBufLen))
	    return num;
	i = SendDlgItemMessageA(hDlg, id, CB_FINDSTRINGEXACT, -1, (LPARAM)buf);
	if(i == CB_ERR)
	    FIXME("Can't find default printer in printer list\n");
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
static BOOL PRINTDLG_CreateDevNames(HGLOBAL *hmem, char* DeviceDriverName,
				    char* DeviceName, char* OutputPort)
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
    LPPRINTDLGA       lppd = PrintStructures->dlg.lpPrintDlg;
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
	    lppd->Flags |= PD_PRINTTOFILE;
	    pi->pPortName = "FILE:";
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
	        lpdm->dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
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
    LPPRINTDLGW       lppd = PrintStructures->dlg.lpPrintDlg;
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
	}

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
	        lpdm->dmCopies = GetDlgItemInt(hDlg, edt3, NULL, FALSE);
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
	PRINTDLGA	*pdlga,const char *PaperSize,LPPOINT size
) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    LPSTR	devname,portname;
    int		i;
    INT		NrOfEntries,ret;
    char	*Names = NULL;
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

    Names = (char*)HeapAlloc(GetProcessHeap(),0,NrOfEntries*64);
    if (NrOfEntries != (ret=DeviceCapabilitiesA(devname,portname,DC_PAPERNAMES,Names,dm))) {
	FIXME("Number of returned vals %d is not %d\n",NrOfEntries,ret);
	goto out;
    }
    for (i=0;i<NrOfEntries;i++)
	if (!strcmp(PaperSize,Names+(64*i)))
	    break;
    HeapFree(GetProcessHeap(),0,Names);
    if (i==NrOfEntries) {
	FIXME("Papersize %s not found in list?\n",PaperSize);
	goto out;
    }
    points = HeapAlloc(GetProcessHeap(),0,sizeof(points[0])*NrOfEntries);
    if (NrOfEntries!=(ret=DeviceCapabilitiesA(devname,portname,DC_PAPERSIZE,(LPBYTE)points,dm))) {
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
    if (Names) HeapFree(GetProcessHeap(),0,Names);
    if (points) HeapFree(GetProcessHeap(),0,points);
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

    Names = (WCHAR*)HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*NrOfEntries*64);
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
    if (Names) HeapFree(GetProcessHeap(),0,Names);
    if (points) HeapFree(GetProcessHeap(),0,points);
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
                dm->dmPaperSize = oldWord;
            else
                dm->dmDefaultSource = oldWord;
        }
    }
    else {
        /* we enter here only when the Print setup dialog is initially
         * opened. In this case the settings are restored from when
         * the dialog was last closed.
         */
        if (dm) {
            if (nIDComboBox == cmb2)
                oldWord = dm->dmPaperSize;
            else
                oldWord = dm->dmDefaultSource;
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
					WCHAR* PrinterName,
					WCHAR* PortName,
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
                dm->dmPaperSize = oldWord;
            else
                dm->dmDefaultSource = oldWord;
        }
    }
    else {
        /* we enter here only when the Print setup dialog is initially
         * opened. In this case the settings are restored from when
         * the dialog was last closed.
         */
        if (dm) {
            if (nIDComboBox == cmb2)
                oldWord = dm->dmPaperSize;
            else
                oldWord = dm->dmDefaultSource;
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
    SendDlgItemMessageA(hDlg, nIDComboBox, CB_RESETCONTENT, 0, 0);

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


/***********************************************************************
 *               PRINTDLG_UpdatePrinterInfoTexts               [internal]
 */
static void PRINTDLG_UpdatePrinterInfoTextsA(HWND hDlg, LPPRINTER_INFO_2A pi)
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

static void PRINTDLG_UpdatePrinterInfoTextsW(HWND hDlg, LPPRINTER_INFO_2W pi)
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
    LPPRINTDLGA lppd = PrintStructures->dlg.lpPrintDlg;
    LPDEVMODEA lpdm = NULL;
    LONG dmSize;
    DWORD needed;
    HANDLE hprn;

    if(PrintStructures->lpPrinterInfo)
        HeapFree(GetProcessHeap(),0, PrintStructures->lpPrinterInfo);
    if(PrintStructures->lpDriverInfo)
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

    if(PrintStructures->lpDevMode) {
        HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	PrintStructures->lpDevMode = NULL;
    }

    dmSize = DocumentPropertiesA(0, 0, name, NULL, NULL, 0);
    if(dmSize == -1) {
        ERR("DocumentProperties fails on %s\n", debugstr_a(name));
	return FALSE;
    }
    PrintStructures->lpDevMode = HeapAlloc(GetProcessHeap(), 0, dmSize);
    dmSize = DocumentPropertiesA(0, 0, name, PrintStructures->lpDevMode, NULL,
				 DM_OUT_BUFFER);
    if(lppd->hDevMode && (lpdm = GlobalLock(lppd->hDevMode)) &&
			  !strcmp(lpdm->dmDeviceName,
				  PrintStructures->lpDevMode->dmDeviceName)) {
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
	/* "All xxx pages"... */
	{
	    char        resourcestr[64];
	    char        result[64];
	    LoadStringA(COMDLG32_hInstance, PD32_PRINT_ALL_X_PAGES,
			resourcestr, 49);
	    sprintf(result,resourcestr,lppd->nMaxPage - lppd->nMinPage + 1);
	    SendDlgItemMessageA(hDlg, rad1, WM_SETTEXT, 0, (LPARAM) result);
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
	      copies = lpdm->dmCopies;
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
      BOOL bPortrait = (lpdm->dmOrientation == DMORIENT_PORTRAIT);

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
    LPPRINTDLGW lppd = PrintStructures->dlg.lpPrintDlg;
    LPDEVMODEW lpdm = NULL;
    LONG dmSize;
    DWORD needed;
    HANDLE hprn;

    if(PrintStructures->lpPrinterInfo)
        HeapFree(GetProcessHeap(),0, PrintStructures->lpPrinterInfo);
    if(PrintStructures->lpDriverInfo)
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

    if(PrintStructures->lpDevMode) {
        HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	PrintStructures->lpDevMode = NULL;
    }

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
	/* "All xxx pages"... */
	{
	    /* ansi is ok */
	    char        resourcestr[64];
	    char        result[64];
	    LoadStringA(COMDLG32_hInstance, PD32_PRINT_ALL_X_PAGES,
			resourcestr, 49);
	    sprintf(result,resourcestr,lppd->nMaxPage - lppd->nMinPage + 1);
	    SendDlgItemMessageA(hDlg, rad1, WM_SETTEXT, 0, (LPARAM) result);
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
	      copies = lpdm->dmCopies;
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
      BOOL bPortrait = (lpdm->dmOrientation == DMORIENT_PORTRAIT);

      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb2,
				  PrintStructures->lpPrinterInfo->pPrinterName,
				  PrintStructures->lpPrinterInfo->pPortName,
				  lpdm);
      PRINTDLG_SetUpPaperComboBoxW(hDlg, cmb3,
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

/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog(HWND hDlg, WPARAM wParam,
				     PRINT_PTRA* PrintStructures)
{
    LPPRINTDLGA lppd = PrintStructures->dlg.lpPrintDlg;
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
	    name = pdm->dmDeviceName;
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
    LPPRINTDLGW lppd = PrintStructures->dlg.lpPrintDlg;
    DEVNAMES *pdn;
    DEVMODEW *pdm;
    WCHAR *name = NULL;
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
    LPPRINTDLGA lppd = PrintStructures->dlg.lpPrintDlg;
    UINT PrinterComboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;
    LPDEVMODEA lpdm = PrintStructures->lpDevMode;

    switch (LOWORD(wParam))  {
    case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTDLG_UpdatePrintDlgA(hDlg, PrintStructures)!=TRUE) {
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
              lpdm->dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP, quality combobox in PRINT */
	 if (PrinterComboID != wParam) {
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
	      lpdm->dmPaperSize = SendDlgItemMessageA(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->dmDefaultSource = SendDlgItemMessageA(hDlg, cmb3,
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
	        if(lpdm->dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->dmOrientation = DMORIENT_PORTRAIT;
		    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->dmOrientation = DMORIENT_LANDSCAPE;
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
    LPPRINTDLGW lppd = PrintStructures->dlg.lpPrintDlg;
    UINT PrinterComboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;
    LPDEVMODEW lpdm = PrintStructures->lpDevMode;

    switch (LOWORD(wParam))  {
    case IDOK:
        TRACE(" OK button was hit\n");
        if (PRINTDLG_UpdatePrintDlgW(hDlg, PrintStructures)!=TRUE) {
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

     case psh1:                       /* Print Setup */
	{
		ERR("psh1 is called from 16bit code only, we should not get here.\n");
	}
	break;
     case psh2:                       /* Properties button */
       {
         HANDLE hPrinter;
         WCHAR  PrinterName[256];

         GetDlgItemTextW(hDlg, PrinterComboID, PrinterName, 255);
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
              lpdm->dmOrientation = DMORIENT_PORTRAIT;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hPortraitIcon));
        }
        break;

    case rad2: /* Paperorientation */
        if (lppd->Flags & PD_PRINTSETUP)
        {
              lpdm->dmOrientation = DMORIENT_LANDSCAPE;
              SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE, (WPARAM) IMAGE_ICON,
                          (LPARAM)(PrintStructures->hLandscapeIcon));
        }
        break;

    case cmb1: /* Printer Combobox in PRINT SETUP, quality combobox in PRINT */
	 if (PrinterComboID != wParam) {
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
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb2, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->dmPaperSize = SendDlgItemMessageA(hDlg, cmb2,
							    CB_GETITEMDATA,
							    Sel, 0);
      }
      break;

    case cmb3: /* Bin */
      {
	  DWORD Sel = SendDlgItemMessageA(hDlg, cmb3, CB_GETCURSEL, 0, 0);
	  if(Sel != CB_ERR)
	      lpdm->dmDefaultSource = SendDlgItemMessageW(hDlg, cmb3,
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
	        if(lpdm->dmOrientation != DMORIENT_PORTRAIT) {
		    lpdm->dmOrientation = DMORIENT_PORTRAIT;
		    SendDlgItemMessageA(hDlg, stc10, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		    SendDlgItemMessageA(hDlg, ico1, STM_SETIMAGE,
					(WPARAM)IMAGE_ICON,
					(LPARAM)PrintStructures->hPortraitIcon);
		}
	    } else {
	        if(lpdm->dmOrientation != DMORIENT_LANDSCAPE) {
	            lpdm->dmOrientation = DMORIENT_LANDSCAPE;
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

/***********************************************************************
 *           PrintDlgProcA			[internal]
 */
INT_PTR CALLBACK PrintDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam,
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
	res = PRINTDLG_WMInitDialog(hDlg, wParam, PrintStructures);

	if(PrintStructures->dlg.lpPrintDlg->Flags & PD_ENABLEPRINTHOOK)
	    res = PrintStructures->dlg.lpPrintDlg->lpfnPrintHook(
		hDlg, uMsg, wParam, (LPARAM)PrintStructures->dlg.lpPrintDlg
	    );
	return res;
    }

    if(PrintStructures->dlg.lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
        res = PrintStructures->dlg.lpPrintDlg->lpfnPrintHook(hDlg,uMsg,wParam,
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

INT_PTR CALLBACK PrintDlgProcW(HWND hDlg, UINT uMsg, WPARAM wParam,
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
	res = PRINTDLG_WMInitDialogW(hDlg, wParam, PrintStructures);

	if(PrintStructures->dlg.lpPrintDlg->Flags & PD_ENABLEPRINTHOOK)
	    res = PrintStructures->dlg.lpPrintDlg->lpfnPrintHook(hDlg, uMsg, wParam, (LPARAM)PrintStructures->dlg.lpPrintDlg);
	return res;
    }

    if(PrintStructures->dlg.lpPrintDlg->Flags & PD_ENABLEPRINTHOOK) {
        res = PrintStructures->dlg.lpPrintDlg->lpfnPrintHook(hDlg,uMsg,wParam, lParam);
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
static HGLOBAL PRINTDLG_GetDlgTemplateA(PRINTDLGA *lppd)
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

static HGLOBAL PRINTDLG_GetDlgTemplateW(PRINTDLGW *lppd)
{
    HRSRC hResInfo;
    HGLOBAL hDlgTmpl;
    const WCHAR xpsetup[] = { 'P','R','I','N','T','3','2','_','S','E','T','U','P',0};
    const WCHAR xprint[] = { 'P','R','I','N','T','3','2',0};

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
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  PrintDlg:
 *  * The Collate Icons do not display, even though they are in the code.
 *  * The Properties Button(s) should call DocumentPropertiesA().
 *  PrintSetupDlg:
 *  * The Paper Orientation Icons are not implemented yet.
 *  * The Properties Button(s) should call DocumentPropertiesA().
 *  * Settings are not yet taken from a provided DevMode or
 *    default printer settings.
 */

BOOL WINAPI PrintDlgA(
		      LPPRINTDLGA lppd /* [in/out] ptr to PRINTDLG32 struct */
		      )
{
    BOOL      bRet = FALSE;
    LPVOID   ptr;
    HINSTANCE hInst = (HINSTANCE)GetWindowLongA( lppd->hwndOwner, GWL_HINSTANCE );

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
	      "pp. %d-%d, min p %d, max p %d, copies %d, hinst %p\n"
	      "flags %08lx (%s)\n",
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
	    ERR("GetPrinterDriverA failed, le %ld, fix your config for printer %s!\n",GetLastError(),pbuf->pPrinterName);
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
	PrintStructures->dlg.lpPrintDlg = lppd;

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
 */
BOOL WINAPI PrintDlgW(
		      LPPRINTDLGW lppd /* [in/out] ptr to PRINTDLG32 struct */
		      )
{
    BOOL      bRet = FALSE;
    LPVOID   ptr;
    HINSTANCE hInst = (HINSTANCE)GetWindowLongW( lppd->hwndOwner, GWL_HINSTANCE );

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
	      "pp. %d-%d, min p %d, max p %d, copies %d, hinst %p\n"
	      "flags %08lx (%s)\n",
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
	    ERR("GetPrinterDriverA failed, le %ld, fix your config for printer %s!\n",GetLastError(),debugstr_w(pbuf->pPrinterName));
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
	PrintStructures->dlg.lpPrintDlg = lppd;

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
 * cmb2 - paper size
 * cmb3 - source (tray?)
 * edt4 - border left
 * edt5 - border top
 * edt6 - border right
 * edt7 - border bottom
 * psh3 - "Printer..."
 */

typedef struct {
    LPPAGESETUPDLGA	dlga;
    PRINTDLGA		pdlg;
} PageSetupDataA;

typedef struct {
    LPPAGESETUPDLGW	dlga;
    PRINTDLGW		pdlg;
} PageSetupDataW;

static HGLOBAL PRINTDLG_GetPGSTemplateA(PAGESETUPDLGA *lppd)
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

static HGLOBAL PRINTDLG_GetPGSTemplateW(PAGESETUPDLGW *lppd)
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
	return 10*size*10/25.4;
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
	return (size*254)/10;
    /* if we don't have a flag, we can choose one. Use millimeters
     * to avoid confusing me
     */
    dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    return (size*254)/10;
}

static void
_c_size2strA(PageSetupDataA *pda,DWORD size,LPSTR strout) {
    strcpy(strout,"<undef>");
    if (pda->dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
	sprintf(strout,"%.2fmm",(size*1.0)/100.0);
	return;
    }
    if (pda->dlga->Flags & PSD_INTHOUSANDTHSOFINCHES) {
	sprintf(strout,"%.2fin",(size*1.0)/1000.0);
	return;
    }
    pda->dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    sprintf(strout,"%.2fmm",(size*1.0)/100.0);
    return;
}
static void
_c_size2strW(PageSetupDataW *pda,DWORD size,LPSTR strout) {
    strcpy(strout,"<undef>");
    if (pda->dlga->Flags & PSD_INHUNDREDTHSOFMILLIMETERS) {
	sprintf(strout,"%.2fmm",(size*1.0)/100.0);
	return;
    }
    if (pda->dlga->Flags & PSD_INTHOUSANDTHSOFINCHES) {
	sprintf(strout,"%.2fin",(size*1.0)/1000.0);
	return;
    }
    pda->dlga->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
    sprintf(strout,"%.2fmm",(size*1.0)/100.0);
    return;
}

static DWORD
_c_str2size(PAGESETUPDLGA *dlga,LPCSTR strin) {
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


/*
 * This is called on finish and will update the output fields of the
 * struct.
 */
static BOOL
PRINTDLG_PS_UpdateDlgStructA(HWND hDlg, PageSetupDataA *pda) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    LPSTR	devname,portname;
    char	papername[64];
    char	buf[200];

    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	= ((char*)dn)+dn->wDeviceOffset;
    portname	= ((char*)dn)+dn->wOutputOffset;
    PRINTDLG_SetUpPaperComboBoxA(hDlg,cmb2,devname,portname,dm);
    PRINTDLG_SetUpPaperComboBoxA(hDlg,cmb3,devname,portname,dm);

    if (GetDlgItemTextA(hDlg,cmb2,papername,sizeof(papername))>0) {
    	PRINTDLG_PaperSizeA(&(pda->pdlg),papername,&(pda->dlga->ptPaperSize));
	pda->dlga->ptPaperSize.x = _c_10mm2size(pda->dlga,pda->dlga->ptPaperSize.x);
	pda->dlga->ptPaperSize.y = _c_10mm2size(pda->dlga,pda->dlga->ptPaperSize.y);
    } else
	FIXME("could not get dialog text for papersize cmbbox?\n");
#define GETVAL(id,val) if (GetDlgItemTextA(hDlg,id,buf,sizeof(buf))>0) { val = _c_str2size(pda->dlga,buf); } else { FIXME("could not get dlgitemtexta for %x\n",id); }
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

static BOOL
PRINTDLG_PS_UpdateDlgStructW(HWND hDlg, PageSetupDataW *pda) {
    DEVNAMES	*dn;
    DEVMODEW	*dm;
    LPWSTR	devname,portname;
    WCHAR	papername[64];

    char	buf[200];

    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	= ((WCHAR*)dn)+dn->wDeviceOffset;
    portname	= ((WCHAR*)dn)+dn->wOutputOffset;
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb2,devname,portname,dm);
    PRINTDLG_SetUpPaperComboBoxW(hDlg,cmb3,devname,portname,dm);

    if (GetDlgItemTextW(hDlg,cmb2,papername,sizeof(papername))>0) {
    	PRINTDLG_PaperSizeW(&(pda->pdlg),papername,&(pda->dlga->ptPaperSize));
	pda->dlga->ptPaperSize.x = _c_10mm2size((LPPAGESETUPDLGA)pda->dlga,pda->dlga->ptPaperSize.x);
	pda->dlga->ptPaperSize.y = _c_10mm2size((LPPAGESETUPDLGA)pda->dlga,pda->dlga->ptPaperSize.y);
    } else
	FIXME("could not get dialog text for papersize cmbbox?\n");
#define GETVAL(id,val) if (GetDlgItemTextA(hDlg,id,buf,sizeof(buf))>0) { val = _c_str2size((LPPAGESETUPDLGA)pda->dlga,buf); } else { FIXME("could not get dlgitemtexta for %x\n",id); }
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

/*
 * This is called after returning from PrintDlg().
 */
static BOOL
PRINTDLG_PS_ChangePrinterA(HWND hDlg, PageSetupDataA *pda) {
    DEVNAMES	*dn;
    DEVMODEA	*dm;
    LPSTR	devname,portname;

    dn = GlobalLock(pda->pdlg.hDevNames);
    dm = GlobalLock(pda->pdlg.hDevMode);
    devname	= ((char*)dn)+dn->wDeviceOffset;
    portname	= ((char*)dn)+dn->wOutputOffset;
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

static BOOL
PRINTDLG_PS_WMCommandA(
    HWND hDlg, WPARAM wParam, LPARAM lParam, PageSetupDataA *pda
) {
    switch (LOWORD(wParam))  {
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
	return TRUE;
    }
    }
    FIXME("loword (lparam) %d, wparam 0x%x, lparam %08lx, STUB mostly.\n",
	    LOWORD(lParam),wParam,lParam
    );
    return FALSE;
}

static BOOL
PRINTDLG_PS_WMCommandW(
    HWND hDlg, WPARAM wParam, LPARAM lParam, PageSetupDataW *pda
) {
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
    FIXME("loword (lparam) %d, wparam 0x%x, lparam %08lx, STUB mostly.\n",
	    LOWORD(lParam),wParam,lParam
    );
    return FALSE;
}


static INT_PTR CALLBACK
PageDlgProcA(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PageSetupDataA	*pda;
    INT_PTR		res = FALSE;

    if (uMsg==WM_INITDIALOG) {
	res = TRUE;
        pda = (PageSetupDataA*)lParam;
	SetPropA(hDlg,"__WINE_PAGESETUPDLGDATA",pda);
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
	}
	PRINTDLG_PS_ChangePrinterA(hDlg,pda);
	if (pda->dlga->Flags & PSD_DISABLEPAPER) {
	    EnableWindow(GetDlgItem(hDlg,cmb2),FALSE);
	    EnableWindow(GetDlgItem(hDlg,cmb3),FALSE);
	}
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
    PageSetupDataW	*pda;
    BOOL		res = FALSE;

    if (uMsg==WM_INITDIALOG) {
	res = TRUE;
        pda = (PageSetupDataW*)lParam;
	SetPropA(hDlg,"__WINE_PAGESETUPDLGDATA",pda);
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
	    char str[100];
	    _c_size2strW(pda,pda->dlga->rtMargin.left,str);
	    SetDlgItemTextA(hDlg,edt4,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.top,str);
	    SetDlgItemTextA(hDlg,edt5,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.right,str);
	    SetDlgItemTextA(hDlg,edt6,str);
	    _c_size2strW(pda,pda->dlga->rtMargin.bottom,str);
	    SetDlgItemTextA(hDlg,edt7,str);
	} else {
	    /* default is 1 inch */
	    DWORD size = _c_inch2size((LPPAGESETUPDLGA)pda->dlga,1000);
	    char	str[20];
	    _c_size2strW(pda,size,str);
	    SetDlgItemTextA(hDlg,edt4,str);
	    SetDlgItemTextA(hDlg,edt5,str);
	    SetDlgItemTextA(hDlg,edt6,str);
	    SetDlgItemTextA(hDlg,edt7,str);
	}
	PRINTDLG_PS_ChangePrinterW(hDlg,pda);
	if (pda->dlga->Flags & PSD_DISABLEPAPER) {
	    EnableWindow(GetDlgItem(hDlg,cmb2),FALSE);
	    EnableWindow(GetDlgItem(hDlg,cmb3),FALSE);
	}
	return TRUE;
    } else {
	pda = (PageSetupDataW*)GetPropA(hDlg,"__WINE_PAGESETUPDLGDATA");
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
 */
BOOL WINAPI PageSetupDlgA(LPPAGESETUPDLGA setupdlg) {
    HGLOBAL		hDlgTmpl;
    LPVOID		ptr;
    BOOL		bRet;
    PageSetupDataA	*pda;
    PRINTDLGA		pdlg;

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	struct pd_flags *pflag = psd_flags;
	for( ; pflag->name; pflag++) {
	    if(setupdlg->Flags & pflag->flag) {
	        strcat(flagstr, pflag->name);
	        strcat(flagstr, "|");
	    }
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
	      "hinst %p, flags %08lx (%s)\n",
	      setupdlg, setupdlg->hwndOwner, setupdlg->hDevMode,
	      setupdlg->hDevNames,
	      setupdlg->hInstance, setupdlg->Flags, flagstr);
    }

    /* First get default printer data, we need it right after that. */
    memset(&pdlg,0,sizeof(pdlg));
    pdlg.lStructSize	= sizeof(pdlg);
    pdlg.Flags		= PD_RETURNDEFAULT;
    bRet = PrintDlgA(&pdlg);
    if (!bRet) return FALSE;

    /* short cut exit, just return default values */
    if (setupdlg->Flags & PSD_RETURNDEFAULT) {
	setupdlg->hDevMode	= pdlg.hDevMode;
	setupdlg->hDevNames	= pdlg.hDevNames;
	/* FIXME: Just return "A4" for now. */
    	PRINTDLG_PaperSizeA(&pdlg,"A4",&setupdlg->ptPaperSize);
	setupdlg->ptPaperSize.x=_c_10mm2size(setupdlg,setupdlg->ptPaperSize.x);
	setupdlg->ptPaperSize.y=_c_10mm2size(setupdlg,setupdlg->ptPaperSize.y);
	return TRUE;
    }
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
		PageDlgProcA,
		(LPARAM)pda)
    );
    return bRet;
}
/***********************************************************************
 *            PageSetupDlgW  (COMDLG32.@)
 */
BOOL WINAPI PageSetupDlgW(LPPAGESETUPDLGW setupdlg) {
    HGLOBAL		hDlgTmpl;
    LPVOID		ptr;
    BOOL		bRet;
    PageSetupDataW	*pdw;
    PRINTDLGW		pdlg;

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	struct pd_flags *pflag = psd_flags;
	for( ; pflag->name; pflag++) {
	    if(setupdlg->Flags & pflag->flag) {
	        strcat(flagstr, pflag->name);
	        strcat(flagstr, "|");
	    }
	}
	TRACE("(%p): hwndOwner = %p, hDevMode = %p, hDevNames = %p\n"
	      "hinst %p, flags %08lx (%s)\n",
	      setupdlg, setupdlg->hwndOwner, setupdlg->hDevMode,
	      setupdlg->hDevNames,
	      setupdlg->hInstance, setupdlg->Flags, flagstr);
    }

    /* First get default printer data, we need it right after that. */
    memset(&pdlg,0,sizeof(pdlg));
    pdlg.lStructSize	= sizeof(pdlg);
    pdlg.Flags		= PD_RETURNDEFAULT;
    bRet = PrintDlgW(&pdlg);
    if (!bRet) return FALSE;

    /* short cut exit, just return default values */
    if (setupdlg->Flags & PSD_RETURNDEFAULT) {
        const WCHAR a4[] = {'A','4',0};
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
 */
HRESULT WINAPI PrintDlgExA(LPPRINTDLGEXA lpPrintDlgExA)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}
/***********************************************************************
 *	PrintDlgExW (COMDLG32.@)
 */
HRESULT WINAPI PrintDlgExW(LPPRINTDLGEXW lpPrintDlgExW)
{
	FIXME("stub\n");
	return E_NOTIMPL;
}
