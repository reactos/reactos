/*
 * common - Common user interface services
 */

#include "tweakui.h"

/*****************************************************************************
 *
 *  Common_SetDirty
 *
 *	Make a property sheet dirty.
 *
 *****************************************************************************/

void NEAR PASCAL
Common_SetDirty(HWND hdlg)
{
    PropSheet_Changed(GetParent(hdlg), hdlg);
}

/*****************************************************************************
 *
 *  Common_NeedLogoff
 *
 *	Indicate that you need to log off for the changes to take effect.
 *
 *****************************************************************************/

void NEAR PASCAL
Common_NeedLogoff(HWND hdlg)
{
    PropSheet_RestartWindows(GetParent(hdlg));
}

/*****************************************************************************
 *
 *  Common_OnHelp
 *
 *	Respond to the F1 key.
 *
 *****************************************************************************/

void PASCAL
Common_OnHelp(LPARAM lp, const DWORD CODESEG *pdwHelp)
{
    WinHelp((HWND) ((LPHELPINFO) lp)->hItemHandle, c_tszMyHelp,
            HELP_WM_HELP, (DWORD) (LPSTR) pdwHelp);
}

/*****************************************************************************
 *
 *  Common_OnContextMenu
 *
 *	Respond to a right-click.
 *
 *****************************************************************************/

void PASCAL
Common_OnContextMenu(WPARAM wp, const DWORD CODESEG *pdwHelp)
{
    WinHelp((HWND) wp, c_tszMyHelp, HELP_CONTEXTMENU, (DWORD) (LPSTR) pdwHelp);
}

/*****************************************************************************
 *
 *  Common_OnLvContextMenu
 *
 *	The context menu shall appear in the listview.
 *
 *	Have the callback generate the menu, then pop it up and let the
 *	window procedure's WM_COMMAND do the rest.
 *
 *****************************************************************************/

void PASCAL
Common_OnLvContextMenu(HWND hwnd, POINT pt, CMCALLBACK pfn)
{
    int iItem = Misc_LV_GetCurSel(hwnd);
    if (iItem != -1) {
	ClientToScreen(hwnd, &pt);	/* Make it screen coordinates */
	TrackPopupMenuEx(pfn(hwnd, iItem),
			 TPM_RIGHTBUTTON | TPM_VERTICAL |
			 TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y,
			 GetParent(hwnd), 0);
    }
}

/*****************************************************************************
 *
 *  Common_LV_OnContextMenu
 *
 *	If the context menu came from the listview, figure out which
 *	item got clicked on.  If we find an item, pop up its context
 *	menu.  Otherwise, just do the standard help thing.
 *
 *	NOTE!  We don't use LVHT_ONITEM because ListView is broken!
 *	Watch:
 *
 *	    #define LVHT_ONITEMSTATEICON 0x0008
 *	    #define LVHT_ABOVE          0x0008
 *
 *	Oops.  This means that clicks above the item are treated as
 *	clicks on the state icon.
 *
 *	Fortunately, we reside completely in report view, so you can't
 *	legally click above the item.  The only way it can happen is
 *	if the coordinates to OnContextMenu are out of range, so we
 *	catch that up front and munge it accordingly.
 *
 *****************************************************************************/

void PASCAL
Common_LV_OnContextMenu(HWND hdlg, HWND hwnd, LPARAM lp,
		        CMCALLBACK pfn, const DWORD CODESEG *pdwHelp)
{
    if (GetDlgCtrlID(hwnd) == IDC_LISTVIEW) {
	LV_HITTESTINFO hti;
	if (lp == (LPARAM)0xFFFFFFFF) {
	    /* Pretend it was on the center of the small icon */
	    ListView_GetItemPosition(hwnd, Misc_LV_GetCurSel(hwnd),
				     &hti.pt);
	    hti.pt.x += GetSystemMetrics(SM_CXSMICON) / 2;
	    hti.pt.y += GetSystemMetrics(SM_CYSMICON) / 2;
	    Common_OnLvContextMenu(hwnd, hti.pt, pfn);
	} else {
	    Misc_LV_HitTest(hwnd, &hti, lp);
	    if ((hti.flags & LVHT_ONITEM)) {
		/* Because LV sometimes forgets to move the focus... */
		Misc_LV_SetCurSel(hwnd, hti.iItem);
		Common_OnLvContextMenu(hwnd, hti.pt, pfn);
	    } else {
		Common_OnContextMenu((WPARAM)hwnd, pdwHelp);
	    }
	}
    } else {
	Common_OnContextMenu((WPARAM)hwnd, pdwHelp);
    }
}


/*****************************************************************************
 *
 *  Common_OnLvCommand
 *
 *	Handle a WM_COMMAND that might be for the listview.
 *
 *****************************************************************************/

void PASCAL
Common_OnLvCommand(HWND hdlg, int idCmd, PLVCI rglvci, int clvci)
{
    do {
	if (idCmd == rglvci[0].id) {
	    HWND hwnd = GetDlgItem(hdlg, IDC_LISTVIEW);
	    int iItem = Misc_LV_GetCurSel(hwnd);
	    if (iItem != -1) {
		rglvci[0].pfn(hwnd, iItem);
	    }
	    break;
	}
	rglvci++;
    } while (--clvci);
}

/*****************************************************************************
 *
 *  EnableDlgItem
 *
 *	A simple wrapper.
 *
 *****************************************************************************/

void NEAR PASCAL
EnableDlgItem(HWND hdlg, UINT idc, BOOL f)
{
    EnableWindow(GetDlgItem(hdlg, idc), f);
}

/*****************************************************************************
 *
 *  SetDlgItemTextLimit
 *
 *	Set the dialog item text as well as limiting its size.
 *
 *****************************************************************************/

void NEAR PASCAL
SetDlgItemTextLimit(HWND hdlg, UINT id, LPCTSTR ptsz, UINT ctch)
{
    HWND hwnd = GetDlgItem(hdlg, id);
    SetWindowText(hwnd, ptsz);
    Edit_LimitText(hwnd, ctch - 1);
}
