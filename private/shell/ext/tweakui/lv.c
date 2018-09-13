/*
 * lv - common dialog proc handler for listview pages
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  LV_AddItem
 *
 *  Add an entry to the listview.
 *
 *  Returns the resulting item number.
 *
 *****************************************************************************/

int PASCAL
LV_AddItem(HWND hwnd, int ix, LPCTSTR ptszDesc, int iImage, BOOL fState)
{
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = MAXLONG;
    lvi.iSubItem = 0;			/* Must be zero */
    lvi.pszText = (LPTSTR)ptszDesc;
    lvi.lParam = ix;			/* Take it */

    if (iImage >= 0) {
	lvi.iImage = iImage;
	lvi.mask |= LVIF_IMAGE;
    }

    if (fState >= 0) {
	lvi.state = INDEXTOSTATEIMAGEMASK(fState + 1);
	lvi.mask |= LVIF_STATE;
    }

    return ListView_InsertItem(hwnd, &lvi);
}


/*****************************************************************************
 *
 *  LV_Toggle
 *
 *  Toggle the state icon of the current selection.
 *
 *****************************************************************************/

void PASCAL
LV_Toggle(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_STATE);
    if (lvi.state & LVIS_STATEIMAGEMASK) {
	lvi.state ^= INDEXTOSTATEIMAGEMASK(1^2); /* toggle checkmark */
	ListView_SetItem(hwnd, &lvi);	/* Set the state */
	Common_SetDirty(GetParent(hwnd));
    }
}

/*****************************************************************************
 *
 *  LV_Rename
 *
 *	Rename an item in the list view.
 *
 *****************************************************************************/

void PASCAL
LV_Rename(HWND hwnd, int iItem)
{
    ListView_EditLabel(hwnd, iItem);
}

/*****************************************************************************
 *
 *  LV_OnInitDialog
 *
 *  The callback will initialize the listview.
 *
 *  Once the callback is happy, we initialize the columns.
 *
 *  All of our listviews are simple reports with but one column.
 *
 *****************************************************************************/

BOOL PASCAL
LV_OnInitDialog(PLVV plvv, HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_LISTVIEW);

    if (plvv->lvvfl & lvvflCanCheck) {
        ListView_SetImageList(hwnd, pcdii->himlState, LVSIL_STATE);
    }

    if (plvv->lvvfl & lvvflIcons) {
	ListView_SetImageList(hwnd, GetSystemImageList(SHGFI_SMALLICON),
						 LVSIL_SMALL);
    }

    if (plvv->OnInitDialog(hwnd)) {	/* Add the column to the report */
    /* BUGBUG -- if metrics change, we need to re-do this */
	LV_COLUMN col;
	RECT rc;

	GetClientRect(hwnd, &rc);
	col.mask = LVCF_WIDTH;
	col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
			  - 4 * GetSystemMetrics(SM_CXEDGE);
	ListView_InsertColumn(hwnd, 0, &col);
	Misc_LV_SetCurSel(hwnd, 0);
    }
    return 1;
}

/*****************************************************************************
 *
 *  LV_OnLvContextMenu
 *
 *	The context menu shall appear in the listview.
 *
 *	Allow the callback to modify the menu before we pop it up.
 *	Then let the window procedure's WM_COMMAND do the rest.
 *
 *      If lvvflCanCheck is set, we will automatically adjust IDC_LVTOGGLE
 *	to match.
 *
 *	If lvvflCanDelete is set, we will adjust IDC_LVDELETE to match.
 *
 *****************************************************************************/

void PASCAL
LV_OnLvContextMenu(PLVV plvv, HWND hdlg, HWND hwnd, POINT pt)
{
    int iItem = Misc_LV_GetCurSel(hwnd);
    if (iItem != -1) {
	HMENU hmenu;

	ClientToScreen(hwnd, &pt);	/* Make it screen coordinates */
	hmenu = GetSubMenu(pcdii->hmenu, plvv->iMenu);

        if (plvv->lvvfl & lvvflCanCheck) {
	    MENUITEMINFO mii;
	    LV_ITEM lvi;
	    lvi.stateMask = LVIS_STATEIMAGEMASK;
	    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_STATE);

	    mii.cbSize = cbX(mii);
	    mii.fMask = MIIM_STATE;
	    switch (isiPlvi(&lvi)) {
	    case isiUnchecked: mii.fState = MFS_ENABLED | MFS_UNCHECKED; break;
	    case isiChecked:   mii.fState = MFS_ENABLED | MFS_CHECKED;   break;
	    default:	       mii.fState = MFS_DISABLED;	         break;
	    }

	    SetMenuItemInfo(hmenu, IDC_LVTOGGLE, 0, &mii);
	}

	if (plvv->lvvfl & lvvflCanDelete) {
	    Misc_EnableMenuFromHdlgId(hmenu, hdlg, IDC_LVDELETE);
	}

	if (plvv->OnInitContextMenu) {
	    plvv->OnInitContextMenu(hwnd, iItem, hmenu);
	}
	TrackPopupMenuEx(hmenu, TPM_RIGHTBUTTON | TPM_VERTICAL |
			 TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, hdlg, 0);
    }
}


/*****************************************************************************
 *
 *  LV_OnContextMenu
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
LV_OnContextMenu(PLVV plvv, HWND hdlg, HWND hwnd, LPARAM lp)
{
    if (GetDlgCtrlID(hwnd) == IDC_LISTVIEW) {
	LV_HITTESTINFO hti;
	if (lp == (LPARAM)0xFFFFFFFF) {
	    /* Pretend it was on the center of the small icon */
	    ListView_GetItemPosition(hwnd, Misc_LV_GetCurSel(hwnd),
				     &hti.pt);
	    hti.pt.x += GetSystemMetrics(SM_CXSMICON) / 2;
	    hti.pt.y += GetSystemMetrics(SM_CYSMICON) / 2;
	    LV_OnLvContextMenu(plvv, hdlg, hwnd, hti.pt);
	} else {
	    Misc_LV_HitTest(hwnd, &hti, lp);
	    if ((hti.flags & LVHT_ONITEM)) {
		/* Because LV sometimes forgets to move the focus... */
		Misc_LV_SetCurSel(hwnd, hti.iItem);
		LV_OnLvContextMenu(plvv, hdlg, hwnd, hti.pt);
	    } else {
		Common_OnContextMenu((WPARAM)hwnd, plvv->pdwHelp);
	    }
	}
    } else {
	Common_OnContextMenu((WPARAM)hwnd, plvv->pdwHelp);
    }
}


/*****************************************************************************
 *
 *  LV_OnCommand_Dispatch
 *
 *	Dispatch a recognized command to the handler.
 *
 *****************************************************************************/

void PASCAL
LV_OnCommand_Dispatch(void (PASCAL *pfn)(HWND hwnd, int iItem), HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_LISTVIEW);
    int iItem = Misc_LV_GetCurSel(hwnd);
    if (iItem != -1) {
	pfn(hwnd, iItem);
    }
}

/*****************************************************************************
 *
 *  LV_OnCommand
 *
 *	Ooh, we got a command.
 *
 *	See if it's one of ours.  If not, pass it through to the handler.
 *
 *****************************************************************************/

BOOL PASCAL
LV_OnCommand(PLVV plvv, HWND hdlg, int id, UINT codeNotify)
{
    PLVCI plvci;
    switch (id) {
    case IDC_LVTOGGLE:
	LV_OnCommand_Dispatch(LV_Toggle, hdlg);
	break;

    case IDC_LVRENAME:
	if (plvv->Dirtify) LV_OnCommand_Dispatch(LV_Rename, hdlg);
	break;

    default:
	for (plvci = plvv->rglvci; plvci[0].id; plvci++) {
	    if (id == plvci->id) {
		LV_OnCommand_Dispatch(plvci->pfn, hdlg);
		goto dispatched;
	    }
	}
	if (plvv->OnCommand) {
	    plvv->OnCommand(hdlg, id, codeNotify);
	}
    dispatched:;
	break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  LV_OnNotify_OnClick
 *
 *	Somebody clicked or double-clicked on the listview.
 *
 *****************************************************************************/

void PASCAL
LV_OnNotify_OnClick(PLVV plvv, HWND hwnd, NMHDR FAR *pnm)
{
    LV_HITTESTINFO hti;
    Misc_LV_HitTest(hwnd, &hti, (LPARAM)GetMessagePos());

    /*
     *  A click/dblclick on the item state icon toggles.
     *
     *  A click/dblclick anywhere toggles *if* you can't rename.
     */
    if (((hti.flags & LVHT_ONITEMSTATEICON) ||
         ((hti.flags & LVHT_ONITEM) && !(plvv->lvvfl & lvvflCanRename)))) {
	Misc_LV_SetCurSel(hwnd, hti.iItem);	/* LV doesn't do this, oddly */
	LV_Toggle(hwnd, hti.iItem);
    } else if (pnm->code == NM_DBLCLK && plvv->idDblClk) {
	LV_OnCommand(plvv, GetParent(hwnd), plvv->idDblClk, 0);
    } else if (pnm->code == NM_DBLCLK && (hti.flags & LVHT_ONITEMLABEL)) {
        if (plvv->lvvfl & lvvflCanRename) {
            LV_Rename(hwnd, hti.iItem);
        }
    }
}


/*****************************************************************************
 *
 *  LV_OnNotify_OnKeyDown
 *
 *	Somebody pressed a key while focus is on a listview.
 *
 *	F2 = Rename
 *	Space = Toggle
 *	Del = Delete
 *
 *****************************************************************************/

void PASCAL
LV_OnNotify_OnKeyDown(PLVV plvv, HWND hwnd, LV_KEYDOWN FAR *lvkd)
{
    int iItem = Misc_LV_GetCurSel(hwnd);
    if (iItem != -1) {
	switch (lvkd->wVKey) {
        case VK_SPACE:
            /*
             *  But not if the ALT key is down!
             */
            if (GetKeyState(VK_MENU) >= 0) {
                LV_Toggle(hwnd, iItem);
            }
            break;

        case VK_F2:
            if (plvv->lvvfl & lvvflCanRename) {
                LV_Rename(hwnd, iItem);
            }
            break;
	case VK_DELETE:
	    LV_OnCommand(plvv, GetParent(hwnd), IDC_LVDELETE, 0); break;
	    break;
	}
    }
}

/*****************************************************************************
 *
 *  LV_OnNotify_OnBeginLabelEdit
 *
 *      Allow it to go through if label editing is permitted.
 *
 *****************************************************************************/

BOOL INLINE
LV_OnNotify_OnBeginLabelEdit(PLVV plvv, HWND hdlg, HWND hwnd)
{
    if (plvv->lvvfl & lvvflCanRename) {
        return 0;
    } else {
        SetWindowLong(hdlg, DWL_MSGRESULT, TRUE);
        return 1;
    }
}

/*****************************************************************************
 *
 *  LV_OnNotify_OnEndLabelEdit
 *
 *	Trim leading and trailing whitespace.  If there's anything left,
 *	then we'll accept it.
 *
 *****************************************************************************/

void PASCAL
LV_OnNotify_OnEndLabelEdit(PLVV plvv, HWND hwnd, LV_DISPINFO FAR *lpdi)
{
    if (lpdi->item.iItem != -1 && lpdi->item.pszText) {
	LV_ITEM lvi;
	lvi.pszText = Misc_Trim(lpdi->item.pszText);
	if (lvi.pszText[0]) {
	    Misc_LV_GetItemInfo(hwnd, &lvi, lpdi->item.iItem, LVIF_PARAM);
	    lvi.mask ^= LVIF_TEXT ^ LVIF_PARAM;
	    ListView_SetItem(hwnd, &lvi);
	    Common_SetDirty(GetParent(hwnd));
	    plvv->Dirtify(lvi.lParam);
	}
    }
}

/*****************************************************************************
 *
 *  LV_OnNotify_OnItemChanged
 *
 *	If we are being told about a new selection, call the callback.
 *
 *****************************************************************************/

void PASCAL
LV_OnNotify_OnItemChanged(PLVV plvv, HWND hwnd, NM_LISTVIEW *pnmlv)
{
    if ((pnmlv->uChanged & LVIF_STATE) && (pnmlv->uNewState & LVIS_SELECTED)) {
	if (plvv->OnSelChange) {
	    plvv->OnSelChange(hwnd, pnmlv->iItem);
	}
    }
}

/*****************************************************************************
 *
 *  LV_OnNotify
 *
 *	Ooh, we got a notification.  See if it's something we recognize.
 *
 *	NOTE!  We don't support private notifications.
 *
 *****************************************************************************/

BOOL PASCAL
LV_OnNotify(PLVV plvv, HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->idFrom) {
    case 0:			/* Property sheet */
	switch (pnm->code) {
	case PSN_APPLY:
	    plvv->OnApply(hdlg);
	    break;
	}
	break;

    case IDC_LISTVIEW:		/* List view */
	{
	    HWND hwnd = GetDlgItem(hdlg, IDC_LISTVIEW);
	    switch (pnm->code) {
	    case NM_CLICK:
	    case NM_DBLCLK:
		LV_OnNotify_OnClick(plvv, hwnd, pnm);
		break;

	    case LVN_KEYDOWN:
		LV_OnNotify_OnKeyDown(plvv, hwnd, (LPVOID)pnm);
		break;

            case LVN_BEGINLABELEDIT:
                return LV_OnNotify_OnBeginLabelEdit(plvv, hdlg, hwnd);

	    case LVN_ENDLABELEDIT:
		LV_OnNotify_OnEndLabelEdit(plvv, hwnd, (LPVOID)pnm);
		break;

	    case LVN_ITEMCHANGED:
		LV_OnNotify_OnItemChanged(plvv, hwnd, (LPVOID)pnm);
		break;

	    }
	}
	break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  LV_OnSettingChange
 *
 *	If the non-client metrics changed, go rebuild the icons (if the
 *	listview has icons).
 *
 *****************************************************************************/

void PASCAL
LV_OnSettingChange(PLVV plvv, HWND hdlg, WPARAM wp)
{
    if (wp == SPI_SETNONCLIENTMETRICS && plvv->GetIcon) {
	HWND hwnd = GetDlgItem(hdlg, IDC_LISTVIEW);
	int iItem;
	int cItem = ListView_GetItemCount(hwnd);
	for (iItem = 0; iItem < cItem; iItem++) {
	    LV_ITEM lvi;
	    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);
	    lvi.iImage = plvv->GetIcon(lvi.lParam);
	    lvi.mask |= LVIF_IMAGE;
	    ListView_SetItem(hwnd, &lvi);
	}
    }
}

/*****************************************************************************
 *
 *  The common listview window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
LV_DlgProc(PLVV plvv, HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG:
	return LV_OnInitDialog(plvv, hdlg);

    case WM_COMMAND:
	return LV_OnCommand(plvv, hdlg,
			        (int)GET_WM_COMMAND_ID(wParam, lParam),
			        (UINT)GET_WM_COMMAND_CMD(wParam, lParam));

    case WM_HELP: Common_OnHelp(lParam, plvv->pdwHelp); break;

    case WM_NOTIFY:
	return LV_OnNotify(plvv, hdlg, (NMHDR FAR *)lParam);

    case WM_SYSCOLORCHANGE:
	FORWARD_WM_SYSCOLORCHANGE(GetDlgItem(hdlg, IDC_LISTVIEW), SendMessage);
	break;

    /* BUGBUG -- WM_WININICHANGE, too */

    case WM_DESTROY:
        if (plvv->OnDestroy) plvv->OnDestroy(hdlg);
        break;

    case WM_CONTEXTMENU:
	LV_OnContextMenu(plvv, hdlg, (HWND)wParam, lParam);
	break;

    case WM_SETTINGCHANGE:
	LV_OnSettingChange(plvv, hdlg, wParam);
	break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
