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
#include "wine/wingdi16.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "cdlg16.h"
#include "printdlg.h"

typedef struct
{
    PRINT_PTRA   print32;
    LPPRINTDLG16 lpPrintDlg16;
} PRINT_PTRA16;

/* Internal Functions */

static BOOL PRINTDLG_CreateDevNames16(HGLOBAL16 *hmem, const char* DeviceDriverName,
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
        *hmem = GlobalReAlloc16(*hmem, size, GMEM_MOVEABLE);
    else
        *hmem = GlobalAlloc16(GMEM_MOVEABLE, size);
    if (*hmem == 0)
        return FALSE;

    pDevNamesSpace = GlobalLock16(*hmem);
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
    GlobalUnlock16(*hmem);
    return TRUE;
}


/***********************************************************************
 *           PRINTDLG_WMInitDialog                      [internal]
 */
static LRESULT PRINTDLG_WMInitDialog16(HWND hDlg, WPARAM wParam, PRINT_PTRA16* ptr16)
{
    PRINT_PTRA *PrintStructures = &ptr16->print32;
    LPPRINTDLG16 lppd = ptr16->lpPrintDlg16;
    DEVNAMES *pdn;
    DEVMODEA *pdm;
    char *name = NULL;
    UINT comboID = (lppd->Flags & PD_PRINTSETUP) ? cmb1 : cmb4;

    /* load Collate ICONs */
    PrintStructures->hCollateIcon =
      LoadIconA(COMDLG32_hInstance, "PD32_COLLATE");
    PrintStructures->hNoCollateIcon =
      LoadIconA(COMDLG32_hInstance, "PD32_NOCOLLATE");
    if(PrintStructures->hCollateIcon == 0 ||
       PrintStructures->hNoCollateIcon == 0) {
        ERR("no icon in resourcefile\n");
	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	EndDialog(hDlg, FALSE);
    }

    /* load Paper Orientation ICON */
    /* FIXME: not implemented yet */

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

    if (!(lppd->Flags & PD_PRINTSETUP)) {
	/* We have a print quality combo box. What shall we do? */
	if (GetDlgItem(hDlg,cmb1)) {
	    char buf [20];

	    FIXME("Print quality only displaying currently.\n");

	    pdm = GlobalLock16(lppd->hDevMode);
	    if(pdm) {
		switch (pdm->u1.s1.dmPrintQuality) {
		case DMRES_HIGH		: strcpy(buf,"High");break;
		case DMRES_MEDIUM	: strcpy(buf,"Medium");break;
		case DMRES_LOW		: strcpy(buf,"Low");break;
		case DMRES_DRAFT	: strcpy(buf,"Draft");break;
		case 0			: strcpy(buf,"Default");break;
		default			: sprintf(buf,"%ddpi",pdm->u1.s1.dmPrintQuality);break;
		}
	        GlobalUnlock16(lppd->hDevMode);
	    } else
		strcpy(buf,"Default");
	    SendDlgItemMessageA(hDlg,cmb1,CB_ADDSTRING,0,(LPARAM)buf);
	    SendDlgItemMessageA(hDlg,cmb1,CB_SETCURSEL,0,0);
	    EnableWindow(GetDlgItem(hDlg,cmb1),FALSE);
	}
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

    /* If the printer combo box is in the dialog, fill it */
    if (GetDlgItem(hDlg,comboID)) {
	/* Fill Combobox
	 */
	pdn = GlobalLock16(lppd->hDevNames);
	pdm = GlobalLock16(lppd->hDevMode);
	if(pdn)
	    name = (char*)pdn + pdn->wDeviceOffset;
	else if(pdm)
	    name = (char*)pdm->dmDeviceName;
	PRINTDLG_SetUpPrinterListComboA(hDlg, comboID, name);
	if(pdm) GlobalUnlock16(lppd->hDevMode);
	if(pdn) GlobalUnlock16(lppd->hDevNames);

	/* Now find selected printer and update rest of dlg */
	name = HeapAlloc(GetProcessHeap(),0,256);
	if (GetDlgItemTextA(hDlg, comboID, name, 255))
	    PRINTDLG_ChangePrinterA(hDlg, name, PrintStructures);
    } else {
	/* else just use default printer */
	char name[200];
        DWORD dwBufLen = sizeof(name);
	BOOL ret = GetDefaultPrinterA(name, &dwBufLen);

	if (ret)
	    PRINTDLG_ChangePrinterA(hDlg, name, PrintStructures);
	else
	    FIXME("No default printer found, expect problems!\n");
    }
    HeapFree(GetProcessHeap(),0,name);

    return TRUE;
}

/************************************************************
 *
 *      PRINTDLG_Get16TemplateFrom32             [Internal]
 *      Generates a 16 bits template from the Wine 32 bits resource
 *
 */
static HGLOBAL16 PRINTDLG_Get16TemplateFrom32(LPCSTR PrintResourceName)
{
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
        HGLOBAL16 hGlobal16;
        LPVOID template;

        if (!(hResInfo = FindResourceA(COMDLG32_hInstance,
               PrintResourceName, (LPSTR)RT_DIALOG)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return 0;
        }
        if (!(hDlgTmpl32 = LoadResource(COMDLG32_hInstance, hResInfo )) ||
            !(template32 = LockResource( hDlgTmpl32 )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return 0;
        }
        size = SizeofResource(COMDLG32_hInstance, hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %d bytes\n", size);
            return 0;
        }
        template = GlobalLock16(hGlobal16);
        if (!template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hGlobal16);
            GlobalFree16(hGlobal16);
            return 0;
        }
        ConvertDialog32To16(template32, size, template);
        GlobalUnlock16(hGlobal16);
        return hGlobal16;
}

static BOOL PRINTDLG_CreateDC16(LPPRINTDLG16 lppd)
{
    DEVNAMES *pdn = GlobalLock16(lppd->hDevNames);
    DEVMODEA *pdm = GlobalLock16(lppd->hDevMode);

    if(lppd->Flags & PD_RETURNDC) {
        lppd->hDC = HDC_16(CreateDCA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm ));
    } else if(lppd->Flags & PD_RETURNIC) {
        lppd->hDC = HDC_16(CreateICA((char*)pdn + pdn->wDriverOffset,
			      (char*)pdn + pdn->wDeviceOffset,
			      (char*)pdn + pdn->wOutputOffset,
			      pdm ));
    }
    GlobalUnlock16(lppd->hDevNames);
    GlobalUnlock16(lppd->hDevMode);
    return lppd->hDC ? TRUE : FALSE;
}

/************************************************************
 *
 *      PRINTDLG_GetDlgTemplate
 *
 */
static HGLOBAL16 PRINTDLG_GetDlgTemplate16(const PRINTDLG16 *lppd)
{
    HGLOBAL16 hDlgTmpl, hResInfo;

    if (lppd->Flags & PD_PRINTSETUP) {
	if(lppd->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hSetupTemplate;
	} else if(lppd->Flags & PD_ENABLESETUPTEMPLATE) {
	    hResInfo = FindResource16(lppd->hInstance,
				     MapSL(lppd->lpSetupTemplateName), (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource16(lppd->hInstance, hResInfo);
	} else {
	    hDlgTmpl = PRINTDLG_Get16TemplateFrom32("PRINT32_SETUP");
	}
    } else {
	if(lppd->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) {
	    hDlgTmpl = lppd->hPrintTemplate;
	} else if(lppd->Flags & PD_ENABLEPRINTTEMPLATE) {
	    hResInfo = FindResource16(lppd->hInstance,
				     MapSL(lppd->lpPrintTemplateName),
				     (LPSTR)RT_DIALOG);
	    hDlgTmpl = LoadResource16(lppd->hInstance, hResInfo);
	} else {
	    hDlgTmpl = PRINTDLG_Get16TemplateFrom32("PRINT32");
	}
    }
    return hDlgTmpl;
}

/**********************************************************************
 *
 *      16 bit commdlg
 */

/***********************************************************************
 *           PrintDlg   (COMMDLG.20)
 *
 *  Displays the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  * calls up to the 32-bit versions of the Dialogs, which look different
 *  * Customizing is *not* implemented.
 */

BOOL16 WINAPI PrintDlg16(
	      LPPRINTDLG16 lppd /* [in/out] ptr to PRINTDLG struct */
) {
    BOOL      bRet = FALSE;
    LPVOID   ptr;
    HINSTANCE16 hInst = GetWindowLongPtrW( HWND_32(lppd->hwndOwner), GWLP_HINSTANCE );

    if(TRACE_ON(commdlg)) {
        char flagstr[1000] = "";
	const struct pd_flags *pflag = pd_flags;
	for( ; pflag->name; pflag++) {
	    if(lppd->Flags & pflag->flag)
	        strcat(flagstr, pflag->name);
	}
	TRACE("(%p): hwndOwner = %08x, hDevMode = %08x, hDevNames = %08x\n"
              "pp. %d-%d, min p %d, max p %d, copies %d, hinst %08x\n"
              "flags %08x (%s)\n",
	      lppd, lppd->hwndOwner, lppd->hDevMode, lppd->hDevNames,
	      lppd->nFromPage, lppd->nToPage, lppd->nMinPage, lppd->nMaxPage,
	      lppd->nCopies, lppd->hInstance, lppd->Flags, flagstr);
    }

    if(lppd->lStructSize != sizeof(PRINTDLG16)) {
        ERR("structure size %d\n",lppd->lStructSize);
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
	    ERR("GetPrinterDriverA failed for %s, le %d, fix your config!\n",
	        pbuf->pPrinterName,GetLastError());
	    HeapFree(GetProcessHeap(), 0, dbuf);
	    HeapFree(GetProcessHeap(), 0, pbuf);
	    COMDLG32_SetCommDlgExtendedError(PDERR_RETDEFFAILURE);
	    return FALSE;
	}
	ClosePrinter(hprn);
	PRINTDLG_CreateDevNames16(&(lppd->hDevNames),
				dbuf->pDriverPath,
				pbuf->pPrinterName,
				pbuf->pPortName);
	lppd->hDevMode = GlobalAlloc16(GMEM_MOVEABLE,pbuf->pDevMode->dmSize+
				     pbuf->pDevMode->dmDriverExtra);
	ptr = GlobalLock16(lppd->hDevMode);
	memcpy(ptr, pbuf->pDevMode, pbuf->pDevMode->dmSize +
	       pbuf->pDevMode->dmDriverExtra);
	GlobalUnlock16(lppd->hDevMode);
	HeapFree(GetProcessHeap(), 0, pbuf);
	HeapFree(GetProcessHeap(), 0, dbuf);
	bRet = TRUE;
    } else {
	HGLOBAL16 hDlgTmpl;
	PRINT_PTRA *PrintStructures;
	PRINT_PTRA16 *ptr16;

    /* load Dialog resources,
     * depending on Flags indicates Print32 or Print32_setup dialog
     */
	hDlgTmpl = PRINTDLG_GetDlgTemplate16(lppd);
	if (!hDlgTmpl) {
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        ptr16 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PRINT_PTRA16));
        ptr16->lpPrintDlg16 = lppd;
        PrintStructures = &ptr16->print32;
        PrintStructures->lpPrintDlg = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(PRINTDLGA));
#define CVAL(x)	PrintStructures->lpPrintDlg->x = lppd->x;
#define MVAL(x)	PrintStructures->lpPrintDlg->x = MapSL(lppd->x);
	CVAL(Flags);
	PrintStructures->lpPrintDlg->hwndOwner = HWND_32(lppd->hwndOwner);
	PrintStructures->lpPrintDlg->hDC = HDC_32(lppd->hDC);
	CVAL(nFromPage);CVAL(nToPage);CVAL(nMinPage);CVAL(nMaxPage);
	CVAL(nCopies);
	PrintStructures->lpPrintDlg->hInstance = HINSTANCE_32(lppd->hInstance);
	CVAL(lCustData);
	MVAL(lpPrintTemplateName);MVAL(lpSetupTemplateName);
	/* Don't copy rest, it is 16 bit specific */
#undef MVAL
#undef CVAL

	PrintStructures->lpDevMode = HeapAlloc(GetProcessHeap(),0,sizeof(DEVMODEA));

	/* and create & process the dialog .
	 * -1 is failure, 0 is broken hwnd, everything else is ok.
	 */
	bRet =	(0<DialogBoxIndirectParam16(
		 hInst, hDlgTmpl, lppd->hwndOwner,
		 (DLGPROC16)GetProcAddress16(GetModuleHandle16("COMMDLG"),(LPCSTR)21),
		 (LPARAM)PrintStructures
		)
	);
	if (!PrintStructures->lpPrinterInfo) bRet = FALSE;
	if(bRet) {
	    DEVMODEA *lpdm = PrintStructures->lpDevMode, *lpdmReturn;
	    PRINTER_INFO_2A *pi = PrintStructures->lpPrinterInfo;
	    DRIVER_INFO_3A *di = PrintStructures->lpDriverInfo;

	    if (lppd->hDevMode == 0) {
	        TRACE(" No hDevMode yet... Need to create my own\n");
		lppd->hDevMode = GlobalAlloc16(GMEM_MOVEABLE,
					lpdm->dmSize + lpdm->dmDriverExtra);
	    } else {
	        WORD locks;
		if((locks = (GlobalFlags16(lppd->hDevMode)&GMEM_LOCKCOUNT))) {
		    WARN("hDevMode has %d locks on it. Unlocking it now\n", locks);
		    while(locks--) {
		        GlobalUnlock16(lppd->hDevMode);
			TRACE("Now got %d locks\n", locks);
		    }
		}
		lppd->hDevMode = GlobalReAlloc16(lppd->hDevMode,
					       lpdm->dmSize + lpdm->dmDriverExtra,
					       GMEM_MOVEABLE);
	    }
	    lpdmReturn = GlobalLock16(lppd->hDevMode);
	    memcpy(lpdmReturn, lpdm, lpdm->dmSize + lpdm->dmDriverExtra);

	    if (lppd->hDevNames != 0) {
	        WORD locks;
		if((locks = (GlobalFlags16(lppd->hDevNames)&GMEM_LOCKCOUNT))) {
		    WARN("hDevNames has %d locks on it. Unlocking it now\n", locks);
		    while(locks--)
		        GlobalUnlock16(lppd->hDevNames);
		}
	    }
	    PRINTDLG_CreateDevNames16(&(lppd->hDevNames),
		    di->pDriverPath,
		    pi->pPrinterName,
		    pi->pPortName
	    );
	    GlobalUnlock16(lppd->hDevMode);
	    /* Copy back the [out] integer parameters */
#define CVAL(x)	lppd->x = PrintStructures->lpPrintDlg->x;
	    CVAL(Flags);
	    CVAL(nFromPage);
	    CVAL(nToPage);
	    CVAL(nCopies);
#undef CVAL

	}
	if (!(lppd->Flags & (PD_ENABLESETUPTEMPLATEHANDLE | PD_ENABLESETUPTEMPLATE)))
            GlobalFree16(hDlgTmpl); /* created from the 32 bits resource */
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDevMode);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpPrinterInfo);
	HeapFree(GetProcessHeap(), 0, PrintStructures->lpDriverInfo);
	HeapFree(GetProcessHeap(), 0, ptr16);
    }
    if(bRet && (lppd->Flags & PD_RETURNDC || lppd->Flags & PD_RETURNIC))
        bRet = PRINTDLG_CreateDC16(lppd);

    TRACE("exit! (%d)\n", bRet);
    return bRet;
}

/***********************************************************************
 *           PrintDlgProc   (COMMDLG.21)
 */
BOOL16 CALLBACK PrintDlgProc16(HWND16 hDlg16, UINT16 uMsg, WPARAM16 wParam,
                            LPARAM lParam)
{
    HWND hDlg = HWND_32(hDlg16);
    PRINT_PTRA16 *PrintStructures;
    BOOL16 res = FALSE;

    if (uMsg!=WM_INITDIALOG) {
        PrintStructures = GetPropA(hDlg,"__WINE_PRINTDLGDATA");
	if (!PrintStructures)
	    return FALSE;
    } else {
        PrintStructures = (PRINT_PTRA16*) lParam;
	SetPropA(hDlg,"__WINE_PRINTDLGDATA",PrintStructures);
	res = PRINTDLG_WMInitDialog16(hDlg, wParam, PrintStructures);

	if(PrintStructures->lpPrintDlg16->Flags & PD_ENABLEPRINTHOOK) {
	    res = CallWindowProc16(
		(WNDPROC16)PrintStructures->lpPrintDlg16->lpfnPrintHook,
		hDlg16, uMsg, wParam, (LPARAM)PrintStructures->lpPrintDlg16
	    );
	}
	return res;
    }

    if(PrintStructures->lpPrintDlg16->Flags & PD_ENABLEPRINTHOOK) {
        res = CallWindowProc16(
		(WNDPROC16)PrintStructures->lpPrintDlg16->lpfnPrintHook,
		hDlg16,uMsg, wParam, lParam
	);
	if(LOWORD(res)) return res;
    }

    switch (uMsg) {
    case WM_COMMAND: {
	 /* We need to map those for the 32bit window procedure, compare
	  * with 32Ato16 mapper in winproc.c
	  */
        return PRINTDLG_WMCommandA(
		hDlg,
		MAKEWPARAM(wParam,HIWORD(lParam)),
		LOWORD(lParam),
		&PrintStructures->print32
	);
    }
    case WM_DESTROY:
	DestroyIcon(PrintStructures->print32.hCollateIcon);
	DestroyIcon(PrintStructures->print32.hNoCollateIcon);
    /* FIXME: don't forget to delete the paper orientation icons here! */

        return FALSE;
    }
    return res;
}

/***********************************************************************
 *           PrintSetupDlgProc   (COMMDLG.22)
 */
BOOL16 CALLBACK PrintSetupDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
				   LPARAM lParam)
{
  HWND hWnd = HWND_32(hWnd16);
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE("WM_INITDIALOG lParam=%08lX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam) {
      case IDOK:
	EndDialog(hWnd, TRUE);
	return(TRUE);
      case IDCANCEL:
	EndDialog(hWnd, FALSE);
	return(TRUE);
      }
      return(FALSE);
    }
  return FALSE;
}
