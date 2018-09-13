/*
 * mycomp - Dialog box property sheet for "My Computer"
 *
 *  For now, we display only Drives.
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
	IDC_ICONLVTEXT,		IDH_GROUP,
	IDC_ICONLVTEXT2,	IDH_MYCOMP,
	IDC_ICONLV,		IDH_MYCOMP,
	0,			0,
};

typedef struct MDI {		/* mdi = my computer dialog info */
    DWORD dwNoDrives;
    DWORD dwValidDrives;
} MDI, *PMDI;

MDI mdi;
#define pdmi (&mdi)

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  MyComp_BuildRoot
 *
 *	Build the root directory of a drive.  The buffer must be 4 chars.
 *
 *****************************************************************************/

LPTSTR PASCAL
MyComp_BuildRoot(LPTSTR ptsz, UINT uiDrive)
{
    ptsz[0] = uiDrive + TEXT('A');
    ptsz[1] = TEXT(':');
    ptsz[2] = TEXT('\\');
    ptsz[3] = TEXT('\0');
    return ptsz;
}

/*****************************************************************************
 *
 *  MyComp_LV_GetIcon
 *
 *	Produce the icon associated with an item.  This is called when
 *	we need to rebuild the icon list after the icon cache has been
 *	purged.
 *
 *****************************************************************************/

#define idiPhantom	-11	/* Magic index for disconnected drive */

int PASCAL
MyComp_LV_GetIcon(LPARAM insi)
{
    if (pdmi->dwValidDrives & (1 << insi)) {
	SHFILEINFO sfi;
	TCHAR tszRoot[4];		/* Root directory thing */

	SHGetFileInfo(MyComp_BuildRoot(tszRoot, insi), 0, &sfi, cbX(sfi),
		      SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	return sfi.iIcon;
    } else {
	if (g_fNT) {
	    UnicodeFromPtsz(wsz, g_tszPathShell32);
	    return mit.Shell_GetCachedImageIndex(wsz, idiPhantom, 0);
	} else {
	    return mit.Shell_GetCachedImageIndex(g_tszPathShell32,
						 idiPhantom, 0);
	}
    }
}

/*****************************************************************************
 *
 *  MyComp_OnInitDialog
 *
 *  For now, just populate with each physical local drive.
 *
 *****************************************************************************/

BOOL PASCAL
MyComp_OnInitDialog(HWND hwnd)
{
    UINT ui;
    TCHAR tszDrive[3];
    tszDrive[1] = TEXT(':');
    tszDrive[2] = TEXT('\0');

    pdmi->dwNoDrives = GetRegDword(g_hkCUSMWCV, c_tszRestrictions,
				   c_tszNoDrives, 0);
    pdmi->dwValidDrives = GetLogicalDrives();

    for (ui = 0; ui < 26; ui++) {
	int iIcon = MyComp_LV_GetIcon(ui);
	tszDrive[0] = ui + TEXT('A');
	LV_AddItem(hwnd, ui, tszDrive, iIcon, !(pdmi->dwNoDrives & (1 << ui)));
    }
    return 1;
}

/*****************************************************************************
 *
 *  MyComp_OnDestroy
 *
 *  Free the memory we allocated.
 *
 *****************************************************************************/

BOOL PASCAL
MyComp_OnDestroy(HWND hdlg)
{
//    Misc_FreePgxa(&pddii->gxa);
//    if (pddii->hkNS) {
//	RegCloseKey(pddii->hkNS);
//    }
    return 1;
}

#if 0
/*****************************************************************************
 *
 *  MyComp_FactoryReset
 *
 *	This is scary and un-undoable, so let's do extra confirmation.
 *
 *****************************************************************************/

void PASCAL
MyComp_FactoryReset(HWND hdlg)
{
    if (MessageBoxId(hdlg, IDS_MyCompRESETOK,
		     tszName, MB_YESNO + MB_DEFBUTTON2) == IDYES) {
	pcdii->fRunShellInf = 1;
	Common_NeedLogoff(hdlg);
	PropSheet_Apply(GetParent(hdlg));
    }
}
#endif

/*****************************************************************************
 *
 *  MyComp_OnApply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

void PASCAL
MyComp_OnApply(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_ICONLV);
    DWORD dwDrives = 0;
    LV_ITEM lvi;

    for (lvi.iItem = 0; lvi.iItem < 26; lvi.iItem++) {
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_STATE);
	if (!LV_IsChecked(&lvi)) {
	    dwDrives |= 1 << lvi.iItem;
	}
    }

    if (pdmi->dwNoDrives != dwDrives) {
	DWORD dwChanged;
	UINT ui;
	TCHAR tszRoot[4];

	SetRegDword(g_hkCUSMWCV, c_tszRestrictions, c_tszNoDrives, dwDrives);

	/* Recompute GetLogicalDrives() in case new drives are here */
	dwChanged = (pdmi->dwNoDrives ^ dwDrives) & GetLogicalDrives();

	pdmi->dwNoDrives = dwDrives;
	/*
	 *  SHCNE_UPDATEDIR doesn't work for CSIDL_DRIVES because
	 *  Drivesx.c checks the restrictions only in response to a
	 *  SHCNE_ADDDRIVE.  So walk through every drive that changed
	 *  and send a SHCNE_DRIVEADD or SHCNE_DRIVEREMOVED for it.
	 */
	for (ui = 0; ui < 26; ui++) {
	    DWORD dwMask = 1 << ui;
	    if (dwChanged & dwMask) {
		MyComp_BuildRoot(tszRoot, ui);
		SHChangeNotify((dwDrives & dwMask) ? SHCNE_DRIVEREMOVED
						   : SHCNE_DRIVEADD,
			       SHCNF_PATH, tszRoot, 0L);
	    }
	}
#if 0
	if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg, CSIDL_DRIVES, &pidl))) {
	    Ole_Free(pidl);
	}
#endif
    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvMyComp = {
    0,                          /* MyComp_OnCommand */
    0,
    0,                          /* MyComp_LV_Dirtify */
    MyComp_LV_GetIcon,
    MyComp_OnInitDialog,
    MyComp_OnApply,
    MyComp_OnDestroy,
    0,
    4,				/* iMenu */
    rgdwHelp,
    0,				/* Double-click action */
    lvvflIcons |                /* We need icons */
    lvvflCanCheck,              /* And check boxes */
    {
	{ 0,			0 },
    }
};

#pragma END_CONST_DATA


/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
MyComp_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvMyComp, hdlg, wm, wParam, lParam);
}
