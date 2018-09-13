/*
 * addrm - Dialog box property sheet for "Add/Remove"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
	IDC_UNINSTALL,		IDH_UNINSTALL,
	IDC_UNINSTALLTEXT,	IDH_UNINSTALL,
	IDC_UNINSTALLNEW,	IDH_UNINSTALLNEW,
	IDC_LVDELETE,		IDH_UNINSTALLDELETE,
	IDC_UNINSTALLEDIT,	IDH_UNINSTALLEDIT,
	0,			0,
};

#pragma END_CONST_DATA


typedef TBYTE ARIFL;		/* Random flags */
#define ariflDelPending 2	/* Delete this on apply */
#define ariflEdited 4		/* Rewrite this key */

#define ctchKeyMac  63          /* Maximum length supported by shell32 */

typedef struct ARI {		/* ari - add/remove info */
    ARIFL arifl;
    TCH tszKey[MAX_PATH];
    TCH tszCmd[MAX_PATH];
} ARI, *PARI;

#define iariPlvi(plvi) ((UINT)(plvi)->lParam)
#define pariIari(iari) (&padii->pari[iari])
#define pariPlvi(plvi) pariIari(iariPlvi(plvi))

typedef struct ADII {
    BOOL fDamaged;
    Declare_Gxa(ARI, ari);
} ADII;

ADII adii;
#define padii (&adii)

/*****************************************************************************
 *
 *  AddRm_AddKey
 *
 *  Add an add/remove entry.  We use a listview control instead of a listbox,
 *  because we need to be able to process right-clicks in order to display a
 *  context menu.
 *
 *  Returns the resulting item number.
 *
 *****************************************************************************/

int PASCAL
AddRm_AddKey(HWND hwnd, LPCTSTR ptszDesc)
{
    return LV_AddItem(hwnd, padii->cari++, ptszDesc, -1, -1);
}

/*****************************************************************************
 *
 *  AddRm_OnInitDialog
 *
 *  Create and fill the Add/Remove list box.
 *
 *****************************************************************************/

BOOL PASCAL
AddRm_OnInitDialog(HWND hwnd)
{
    padii->fDamaged = FALSE;

    if (Misc_InitPgxa(&padii->gxa, cbX(ARI))) {
	HKEY hk;
	if (_RegOpenKey(g_hkLMSMWCV, c_tszUninstall, &hk) == 0) {
	    int ihk;
	    PARI pari;
	    for (ihk = 0;
		 (pari = Misc_AllocPx(&padii->gxa)) &&
		 (RegEnumKey(hk, ihk, pari->tszKey, cbX(pari->tszKey)) == 0);
		 ihk++) {

		if (pari->tszKey[0]) {		/* Don't want default key */
		    TCH tszDesc[MAX_PATH];
		    if (GetRegStr(hk, pari->tszKey, c_tszDisplayName,
				  tszDesc, cbX(tszDesc)) &&
			GetRegStr(hk, pari->tszKey, c_tszUninstallString,
				  pari->tszCmd, cbX(pari->tszCmd))) {
			pari->arifl = 0;
			AddRm_AddKey(hwnd, tszDesc);
                        if (lstrlen(pari->tszKey) > ctchKeyMac) {
                            padii->fDamaged = TRUE;
                        }
                    }
		}
	    }
	    RegCloseKey(hk);
	}
	return 1;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  AddRm_Dirtify
 *
 *	Mark an entry as dirty.  Used when somebody does an in-place edit
 *	of an entry.
 *
 *****************************************************************************/

void PASCAL
AddRm_Dirtify(LPARAM iari)
{
    pariIari(iari)->arifl |= ariflEdited;
}

/*****************************************************************************
 *
 *	DIGRESSION
 *
 *  The Edit dialog box.
 *
 *****************************************************************************/

typedef struct AREI {		/* Add/Remove Edit Info */
    HWND hwnd;			/* List view */
    int	iItem;			/* Item number being edited (-1 if add) */
} AREI, *PAREI;

/*****************************************************************************
 *
 *  AddRm_Edit_IsDlgItemPresent
 *
 *  Leading and trailing spaces are ignored.
 *
 *****************************************************************************/

BOOL PASCAL
AddRm_Edit_IsDlgItemPresent(HWND hdlg, int id)
{
    TCH tsz[MAX_PATH];
    GetDlgItemText(hdlg, id, tsz, cA(tsz));
    return Misc_Trim(tsz)[0];
}

/*****************************************************************************
 *
 *  AddRm_Edit_OnCommand_OnEditChange
 *
 *	Enable/disable the OK button based on whether the texts are present.
 *
 *****************************************************************************/

void PASCAL
AddRm_Edit_OnCommand_OnEditChange(HWND hdlg)
{
    EnableDlgItem(hdlg, IDOK,
		    AddRm_Edit_IsDlgItemPresent(hdlg, IDC_UNINSTALLDESC) &&
		    AddRm_Edit_IsDlgItemPresent(hdlg, IDC_UNINSTALLCMD));
}

/*****************************************************************************
 *
 *  AddRm_Edit_OnInitDialog
 *
 *	Fill in the fields with stuff.
 *
 *****************************************************************************/

void PASCAL
AddRm_Edit_OnInitDialog(HWND hdlg, PAREI parei)
{
    SetWindowLong(hdlg, DWL_USER, (LPARAM)parei);
    if (parei->iItem != -1) {
	LV_ITEM lvi;
	TCH tszDesc[MAX_PATH];
	lvi.pszText = tszDesc;
	lvi.cchTextMax = cA(tszDesc);
	Misc_LV_GetItemInfo(parei->hwnd, &lvi, parei->iItem,
			    LVIF_PARAM | LVIF_TEXT);

	SetDlgItemTextLimit(hdlg, IDC_UNINSTALLDESC, lvi.pszText, MAX_PATH);
	SetDlgItemTextLimit(hdlg, IDC_UNINSTALLCMD,
				       pariPlvi(&lvi)->tszCmd,
				       cA(pariPlvi(&lvi)->tszCmd));
    }
    AddRm_Edit_OnCommand_OnEditChange(hdlg);
}

/*****************************************************************************
 *
 *  AddRm_Edit_OnOk
 *
 *	Save the information back out.
 *
 *****************************************************************************/

void PASCAL
AddRm_Edit_OnOk(HWND hdlg)
{
    PAREI parei = (PAREI)GetWindowLong(hdlg, DWL_USER);
    LV_ITEM lvi;
    TCH tszDesc[MAX_PATH];
    /*
     *	Need to get the description early, so that the new entry sorts
     *	into the right place.
     */
    lvi.pszText = tszDesc;
    GetDlgItemText(hdlg, IDC_UNINSTALLDESC, lvi.pszText, cA(tszDesc));

    if (parei->iItem == -1) {
	PARI pari = Misc_AllocPx(&padii->gxa);
	if (pari) {
	    pari->arifl = 0;
	    pari->tszKey[0] = TEXT('\0');
	    parei->iItem = AddRm_AddKey(parei->hwnd, lvi.pszText);
	    Misc_LV_SetCurSel(parei->hwnd, parei->iItem);
	} else {
	    goto failed;
	}
    }
    lvi.iItem = parei->iItem;
    Misc_LV_GetItemInfo(parei->hwnd, &lvi, parei->iItem, LVIF_PARAM);

    pariPlvi(&lvi)->arifl |= ariflEdited;

    GetDlgItemText(hdlg, IDC_UNINSTALLCMD,
		   pariPlvi(&lvi)->tszCmd, cA(pariPlvi(&lvi)->tszCmd));

    lvi.mask ^= LVIF_TEXT ^ LVIF_PARAM;
    ListView_SetItem(parei->hwnd, &lvi);

    Common_SetDirty(GetParent(parei->hwnd));

    failed:;
}

/*****************************************************************************
 *
 *  AddRm_Edit_OnCommand
 *
 *****************************************************************************/

void PASCAL
AddRm_Edit_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDCANCEL:
	EndDialog(hdlg, 0); break;

    case IDOK:
	AddRm_Edit_OnOk(hdlg);
	EndDialog(hdlg, 0); 
	break;

    case IDC_UNINSTALLDESC:
    case IDC_UNINSTALLCMD:
	if (codeNotify == EN_CHANGE) AddRm_Edit_OnCommand_OnEditChange(hdlg);
	break;
    }
}

/*****************************************************************************
 *
 *  AddRm_Edit_DlgProc
 *
 *	Dialog procedure.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA
const static DWORD CODESEG rgdwHelpEdit[] = {
	IDC_UNINSTALLDESCTEXT,	IDH_UNINSTALLEDITDESC,
	IDC_UNINSTALLCMDTEXT,	IDH_UNINSTALLEDITCOMMAND,
	0,			0,
};
#pragma END_CONST_DATA

/*
 * The HANDLE_WM_* macros weren't designed to be used from a dialog
 * proc, so we need to handle the messages manually.  (But carefully.)
 */

BOOL EXPORT
AddRm_Edit_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: AddRm_Edit_OnInitDialog(hdlg, (PAREI)lParam); break;

    case WM_COMMAND:
	AddRm_Edit_OnCommand(hdlg,
			        (int)GET_WM_COMMAND_ID(wParam, lParam),
			        (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
	break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelpEdit[0]); break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}


/*****************************************************************************
 *
 *	END DIGRESSION
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  AddRm_OnEdit
 *
 *****************************************************************************/

void PASCAL
AddRm_OnEdit(HWND hwnd, int iItem)
{
    AREI arei = { hwnd, iItem };
    DialogBoxParam(hinstCur, MAKEINTRESOURCE(IDD_UNINSTALLEDIT), hwnd,
			  AddRm_Edit_DlgProc, (LPARAM)&arei);
}

/*****************************************************************************
 *
 *  AddRm_OnNew
 *
 *****************************************************************************/

#define AddRm_OnNew(hdlg) AddRm_OnEdit(GetDlgItem(hdlg, IDC_UNINSTALL), -1)

/*****************************************************************************
 *
 *  AddRm_OnDelete
 *
 *	Mark it for deletion, but don't actually nuke it until later.
 *
 *****************************************************************************/

void PASCAL
AddRm_OnDelete(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    TCH tszDesc[MAX_PATH];
    lvi.pszText = tszDesc;
    lvi.cchTextMax = cA(tszDesc);
    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM | LVIF_TEXT);
    if (MessageBoxId(GetParent(hwnd), IDS_ADDRMWARN, lvi.pszText,
		     MB_YESNO | MB_DEFBUTTON2) == IDYES) {
	pariPlvi(&lvi)->arifl |= ariflDelPending;
	ListView_DeleteItem(hwnd, iItem);
	Misc_LV_EnsureSel(hwnd, iItem);
	Common_SetDirty(GetParent(hwnd));
    }
}

/*****************************************************************************
 *
 *  AddRm_CreateUniqueKeyName
 *
 *  Find a name that doesn't yet exist.
 *
 *  Search numerically until something works, or a weird error occurs.
 *
#if 0
 *  To avoid O(n^2) behavior, we start with the number of keys.
#endif
 *
 *****************************************************************************/

BOOL PASCAL
AddRm_CreateUniqueKeyName(HKEY hkUninst, LPTSTR ptsz)
{
    BOOL fRc;
    int i;
    for (i = 0; ; i++) {
	DWORD cb;
	wsprintf(ptsz, c_tszPercentU, i);
	cb = 0;
	switch (RegQueryValue(hkUninst, ptsz, 0, &cb)) {
	case ERROR_SUCCESS: break;
	case ERROR_FILE_NOT_FOUND: fRc = 1; goto done;
	default: fRc = 0; goto done;	/* Unknown error */
	}
    }
done:;
    return fRc;
}

/*****************************************************************************
 *
 *  AddRm_OnApply_DoDeletes
 *
 *	Delete the keys that are marked as "delete pending".
 *
 *	pari->tszKey[0] is TEXT('\0') if the key was never in the registry.
 *	(E.g., you "New" it, and then delete it.)
 *
 *****************************************************************************/

void PASCAL
AddRm_OnApply_DoDeletes(HKEY hkUninst)
{
    int iari;
    for (iari = 0; iari < padii->cari; iari++) {
	if (pariIari(iari)->arifl & ariflDelPending) {
	    if (pariIari(iari)->tszKey[0]) {
		RegDeleteTree(hkUninst, pariIari(iari)->tszKey);
	    }
	}
    }
}

/*****************************************************************************
 *
 *  AddRm_OnApply_DoEdits
 *
 *	Apply all the edits.
 *
 *****************************************************************************/

void PASCAL
AddRm_OnApply_DoEdits(HWND hdlg, HKEY hkUninst)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_UNINSTALL);
    int cItems = ListView_GetItemCount(hwnd);
    LV_ITEM lvi;

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++) {
	TCH tsz[MAX_PATH];
	PARI pari;
	lvi.pszText = tsz;
	lvi.cchTextMax = cA(tsz);
	Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_PARAM | LVIF_TEXT);
	pari = pariPlvi(&lvi);

	if (pari->arifl & ariflEdited) {
	    HKEY hk;
	    if (pari->tszKey[0] == TEXT('\0')) {
                if (AddRm_CreateUniqueKeyName(hkUninst, pari->tszKey)) {
		} else {
		    break;			/* Error! */
		}
	    }
	    if (RegCreateKey(hkUninst, pari->tszKey, &hk) == 0) {
		RegSetValuePtsz(hk, c_tszDisplayName, tsz);
		RegSetValuePtsz(hk, c_tszUninstallString, pari->tszCmd);
		RegCloseKey(hk);
	    }
	}
    }
}

/*****************************************************************************
 *
 *  AddRm_OnApply
 *
 *****************************************************************************/

void PASCAL
AddRm_OnApply(HWND hdlg)
{
    HKEY hkUninst;
    if (RegCreateKey(g_hkLMSMWCV, c_tszUninstall, &hkUninst) == 0) {
	AddRm_OnApply_DoDeletes(hkUninst);
	AddRm_OnApply_DoEdits(hdlg, hkUninst);
	RegCloseKey(hkUninst);
    }
}

/*****************************************************************************
 *
 *  AddRm_OnDestroy
 *
 *	Free the memory we allocated.
 *
 *****************************************************************************/

void PASCAL
AddRm_OnDestroy(HWND hdlg)
{
    Misc_FreePgxa(&padii->gxa);
}

/*****************************************************************************
 *
 *  AddRm_OnRepair
 *
 *  Take all the keys that are too long and shorten them.
 *
 *****************************************************************************/

void PASCAL
AddRm_OnRepair(HWND hdlg)
{
    if (padii->fDamaged &&
        MessageBoxId(hdlg, IDS_ASKREPAIRADDRM, g_tszName, MB_YESNO)) {
        HKEY hkUninst;

        if (RegCreateKey(g_hkLMSMWCV, c_tszUninstall, &hkUninst) == 0) {

            int iari;
            for (iari = 0; iari < padii->cari; iari++) {
                TCHAR tszNewKey[MAX_PATH];
                PARI pari = pariIari(iari);
                if (lstrlen(pari->tszKey) > ctchKeyMac &&
                    AddRm_CreateUniqueKeyName(hkUninst, tszNewKey) &&
                    Misc_RenameReg(hkUninst, 0, pari->tszKey, tszNewKey)) {
                    lstrcpy(pari->tszKey, tszNewKey);
                }
            }
            RegCloseKey(hkUninst);
        }
    }
    padii->fDamaged = FALSE;            /* Ask only once */
}

/*****************************************************************************
 *
 *  AddRm_OnCommand
 *
 *****************************************************************************/

void PASCAL
AddRm_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_UNINSTALLNEW:
	if (codeNotify == BN_CLICKED) {
	    AddRm_OnNew(hdlg);
	}
	break;

    case IDC_UNINSTALLCHECK:
        AddRm_OnRepair(hdlg);
        break;

    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvAddRm = {
    AddRm_OnCommand,
    0,				/* AddRm_OnInitContextMenu */
    AddRm_Dirtify,
    0,				/* AddRm_GetIcon */
    AddRm_OnInitDialog,
    AddRm_OnApply,
    AddRm_OnDestroy,
    0,				/* AddRm_OnSelChange */
    2,				/* iMenu */
    rgdwHelp,
    IDC_UNINSTALLEDIT,		/* Double-click action */
    lvvflCanDelete | lvvflCanRename,
    {
	{ IDC_UNINSTALLEDIT,	AddRm_OnEdit },
	{ IDC_LVDELETE,		AddRm_OnDelete },
	{ 0,			0 },
    },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
AddRm_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_SHOWWINDOW:
        if (wParam) {
            FORWARD_WM_COMMAND(hdlg, IDC_UNINSTALLCHECK, 0, 0, PostMessage);
        }
        break;
    }

    return LV_DlgProc(&lvvAddRm, hdlg, wm, wParam, lParam);
}
