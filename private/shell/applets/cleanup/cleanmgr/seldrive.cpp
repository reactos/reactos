/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    seldrive.cpp
**
** Purpose: Defines the IEmptyVoluemCacheCallback interface for 
**          the cleanup manager.
** Notes:   
** Mod Log: Created by Jason Cobb (12/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "seldrive.h"   
#include "msprintf.h"

#include <regstr.h>
#include <help.h>

/*
**------------------------------------------------------------------------------
** Definitions
**------------------------------------------------------------------------------
*/
const DWORD aSelDriveHelpIDs[]=
{
    IDC_SELDRIVE_COMBO,             IDH_CLEANMGR_SELDRIVE,
    IDOK,                           IDH_CLEANMGR_SELDRIVE_OK,
    IDCANCEL,                       IDH_CLEANMGR_SELDRIVE_EXIT,                   
	IDC_SELDRIVE_TEXT,              ((DWORD)-1),
	IDC_SELDRIVE_TEXT2,             ((DWORD)-1),
	0, 0
};

static struct tagDriveSelectionData
{
    drenum  dreDef;     // default drive to choose
    drvlist dvl;        // type of drive list
    drenum  dreChose;   // drive selected at end of dialog
} dsd;


/*
**------------------------------------------------------------------------------
** Prototypes
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK
SelectDriveProc(
	HWND hDlg,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam
	);

BOOL
fillSelDriveList(
    HWND hDlg
    );

WPARAM
AddComboString(
    HWND hDlg, 
    int id, 
    TCHAR *psz, 
    DWORD val
    );

void 
SelectComboItem(
    HWND hDlg, 
    int id, 
    WPARAM w
    );

void 
SelectDriveDlgDrawItem(
    HWND hDlg, 
    DRAWITEMSTRUCT *lpdis, 
    BOOL bHighlightBackground    
    );

#ifdef NEC_98
drenum
GetNECBootDrive(void);
#endif

/*
**------------------------------------------------------------------------------
** Routines
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** SelectDrive
**
** Purpose:    Main entry point for the "Select Drive" dialog.
** Parameters:
**    dre   -  default drive to choose
**    dvl   -  type of drives to list
** Return:     TRUE if the user selected a drive, FALSE if a user selected Exit.
**             If a user does select a drive then that drive is returned in the
**             szDrive string pointer.
** Notes;
** Mod Log:    Created by Jason Cobb (12/97)
**------------------------------------------------------------------------------
*/
BOOL 
SelectDrive(
    PTSTR szDrive, 
    drvlist dvl
    )
{
    drenum   dre;

    GetDriveFromString(szDrive, dre);

    dsd.dreDef  = dre;
    dsd.dvl     = dvl;

    if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SELDRIVE), NULL, SelectDriveProc) != IDOK)
        return FALSE;

    CreateStringFromDrive(dsd.dreChose, szDrive, 4);
    
    return TRUE;
}


/*
**------------------------------------------------------------------------------
** SelectDriveProc
**
** Purpose:    Callback for the "Select Drive" dialog.
** Parameters:
**  hDlg
**  Message
**  wParam
**  lParam
**
** Return:     
** Notes;
** Mod Log:    Created by Jason Cobb (12/97)
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK
SelectDriveProc(
	HWND hDlg,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam
	)
{
    PAINTSTRUCT ps;


    switch (Message)
    {
        case WM_INITDIALOG:
            if (!fillSelDriveList (hDlg))
            {
                EndDialog (hDlg, IDCANCEL);
            }

            SetFocus(GetDlgItem(hDlg, IDC_SELDRIVE_COMBO));
            break;

        case WM_DESTROY:
            EndDialog (hDlg, IDCANCEL);
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)(LPTSTR)aSelDriveHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)aSelDriveHelpIDs);
            return TRUE;

        case WM_PAINT:
            BeginPaint (hDlg, &ps);
            EndPaint (hDlg, &ps);
            break;
        
        case WM_DRAWITEM:
            SelectDriveDlgDrawItem(hDlg, (DRAWITEMSTRUCT *)lParam, TRUE);
            break;


        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    dsd.dreChose = (drenum)SendDlgItemMessage(hDlg, IDC_SELDRIVE_COMBO, CB_GETITEMDATA, 
                                  (WPARAM)SendDlgItemMessage(hDlg, IDC_SELDRIVE_COMBO, CB_GETCURSEL, 0, 0L), 0L);
                    //Fall through
                    
                case IDCANCEL:
                    EndDialog (hDlg, wParam);
                    break;
            }
            break;

        default:
            return FALSE;
        }

    return TRUE;
}

void 
SelectDriveDlgDrawItem(
    HWND hDlg, 
    DRAWITEMSTRUCT *lpdis, 
    BOOL bHighlightBackground    
    )
{
    HDC             hdc = lpdis->hDC;
    TCHAR           szText[MAX_DESC_LEN*2];
    SIZE            size;
    drenum          dre;
    HICON           hIcon = NULL;
    DWORD       dwExStyle = 0L;
    UINT        fuETOOptions = 0;

    if ((int)lpdis->itemID < 0)
        return;

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (DWORD_PTR)(LPTSTR)szText);
    GetTextExtentPoint32(hdc, szText, lstrlen(szText), &size);
    dre = (drenum)SendMessage(lpdis->hwndItem, CB_GETITEMDATA, lpdis->itemID, 0);

    if (lpdis->itemAction != ODA_FOCUS)
    {
        int clrBackground = COLOR_WINDOW;
        int clrText = COLOR_WINDOWTEXT;
        if (bHighlightBackground && lpdis->itemState & ODS_SELECTED) {
            clrBackground = COLOR_HIGHLIGHT;
            clrText = COLOR_HIGHLIGHTTEXT;
        }

        //
        //For multiple selection, we don't want to draw items as
        //selected.  Just focus rect below.
        //
        SetBkColor(hdc, GetSysColor(clrBackground));
        SetTextColor(hdc, GetSysColor(clrText));

        //
        //Fill in the background; do this before mini-icon is drawn
        //
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &(lpdis->rcItem), NULL, 0, NULL);

        //
        //Draw mini-icon for this item and move string accordingly
        //
        if ((hIcon = GetDriveIcon(dre, TRUE)) != NULL)
        {
            DrawIconEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon,
                       16, 16, 0, NULL, DI_NORMAL);
            lpdis->rcItem.left += 16;
        }

        lpdis->rcItem.left += INDENT;

        //
        //Draw the cleanup client display name text transparently on top of the background
        //
        SetBkMode(hdc, TRANSPARENT);
        dwExStyle = GetWindowLong(hDlg, GWL_EXSTYLE);
        if(dwExStyle & WS_EX_RTLREADING)
        {
           fuETOOptions |= ETO_RTLREADING; 
        }        
        ExtTextOut(hdc, lpdis->rcItem.left, lpdis->rcItem.top +
                   ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2,
                   fuETOOptions, NULL, szText, lstrlen(szText), NULL);
    }

    if (lpdis->itemAction == ODA_FOCUS || (lpdis->itemState & ODS_FOCUS))
        DrawFocusRect(hdc, &(lpdis->rcItem));
}

BOOL 
fillSelDriveList(
    HWND hDlg
    )
{
    BOOL      bDoDrive[Drive_Z+1];
    int       dre;
    hardware  hw;
    WPARAM    dw;
    WPARAM    dwSelected = 0;
    USHORT    nFound = 0;
    drenum    dreSelected = Drive_INV;
    TCHAR pszText[cbRESOURCE];
    int cDrv;
#ifdef NEC_98
    drenum    dreBootDrive = Drive_A;
#endif

    for (dre = (int)Drive_A; dre <= (int)Drive_Z; dre++)
    {
        bDoDrive[dre] = FALSE;
    }

    cDrv = 0;

	//
	//First, figger out what drives to hit.
	//
    for (dre = (int)Drive_A; dre <= (int)Drive_Z; dre++)
    {
        switch (dsd.dvl)
        {
            case dvlANYLOCAL:
            {
                GetHardwareType((drenum)dre, hw);
                switch (hw)
                {
                    case hwFixed:
                        cDrv += 1;
                        // Fall through - only count hard disks

                    case hwRemoveable:
                        bDoDrive[dre] = TRUE;
                        break;
                }
            }
            break;
				      
            default:            
                break;
        }
    }

#ifdef NEC_98
	dreBootDrive = GetNECBootDrive();
#endif

    for (dre = (int)Drive_A; dre <= (int)Drive_Z; dre++)
    {
        if (!bDoDrive[dre])
            continue;

        GetDriveDescription((drenum)dre, pszText);
        dw = AddComboString(hDlg, IDC_SELDRIVE_COMBO, pszText, (DWORD)dre);
        nFound++;
        
#ifdef NEC_98
		if ( l.dreDef == (drenum)dre ||
				dreSelected == Drive_INV ||
				(dreSelected < dreBootDrive && l.dreDef == Drive_ALL) ||
				(dreSelected < dreBootDrive && l.dreDef == Drive_INV) )
#else
        if (dsd.dreDef == (drenum)dre ||
            dreSelected == Drive_INV ||
            (dreSelected < Drive_C && dsd.dreDef == Drive_ALL) ||
            (dreSelected < Drive_C && dsd.dreDef == Drive_INV) )
#endif
        {
            dwSelected = dw;
            dreSelected = (drenum)dre;
        }
    }

    //
    //Found some drives? Pick one, and leave.
    //
    if (nFound != 0)
    {
        SelectComboItem(hDlg, IDC_SELDRIVE_COMBO, dwSelected);
        dsd.dreDef = dreSelected;
        return TRUE;
    }

    return FALSE;
}



/*
 * SERVICE FUNCTIONS __________________________________________________________
 *
 */


void 
SelectComboItem(
    HWND hDlg, 
    int id, 
    WPARAM w
    )
{
   LPARAM  lParam;

   lParam = MAKELONG((WORD)GetDlgItem(hDlg,id), (WORD)CBN_SELCHANGE );
   SendDlgItemMessage(hDlg, id, CB_SETCURSEL, w, 0L);
   SendMessage(hDlg, WM_COMMAND, id, lParam);
}

WPARAM
AddComboString(
    HWND hDlg, 
    int id, 
    TCHAR *psz, 
    DWORD val
    )
{
   WPARAM dw;

   dw = SendDlgItemMessage(hDlg, id, CB_ADDSTRING, 0, (LPARAM)psz);
   SendDlgItemMessage(hDlg, id, CB_SETITEMDATA, dw, (LPARAM)val);

   return dw;
}

#ifdef NEC_98
drenum
GetNECBootDrive(
    void
    )
{
    drenum dre;
    TCHAR szDrive[4];

    GetBootDrive(szDrive, sizeof(szDrive));

    dre = (drenum)(toupper(szDrive[0] - 'A'));

    return dre;
}
#endif

/*
**------------------------------------------------------------------------------
** GetBootDrive
**
** Purpose:    Gets the boot drive from the registry
** Parameters:
**    pDrive	Buffer that the drive string will be returned
**				in, the format will be x:\
**	  Size		Size of the pDrive buffer
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
void
GetBootDrive(
	PTCHAR pDrive,
	DWORD Size
	)
{
	HKEY	hKey;
	DWORD	cbSize, dwType;

	pDrive[0] = '\0';

	if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP_SETUP, &hKey) == ERROR_SUCCESS)
	{
		cbSize = Size;
		dwType = REG_SZ;
		RegQueryValueEx(hKey, REGSTR_VAL_BOOTDIR, NULL, &dwType, (LPBYTE)pDrive, &cbSize);

		RegCloseKey(hKey);
	}

	if (pDrive[0] == '\0')
		lstrcpy(pDrive, SZ_DEFAULT_DRIVE);
}
