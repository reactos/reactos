/*
 * desktop - Dialog box property sheet for "desktop customization"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

const GUID c_clsidBfc = { 0x85BBD920, 0x42A0, 0x1069,
			{ 0xA2, 0xE4, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D} };

const static DWORD CODESEG rgdwHelp[] = {
	IDC_ICONLVTEXT,		IDH_GROUP,
	IDC_ICONLVTEXT2,	IDH_ICONLV,
	IDC_ICONLV,		IDH_ICONLV,
	IDC_CREATENOWTEXT,	IDH_GROUP,
	IDC_CREATENOW,		IDH_CREATENOW,
#if 0
	IDC_RESET,		IDH_RESET,
#endif
	0,			0,
};

#pragma END_CONST_DATA

/*
 * cchFriendlyMax should be at least MAX_PATH, because we also use it
 * to hold icon file names.
 */
#define cchFriendlyMax 256

/*
 *  Evil hack!  Since the ::{guid} hack doesn't work unless the object
 *  really exists in the name space, we pull an evil trick and just
 *  create the idlist that the shell would've made if you had asked for
 *  it...
 */
typedef struct RIDL {
    USHORT cb;		    /* Must be 20 */
    USHORT sig;		    /* Must be 0x1F */
    CLSID clsid;	    /* Guid goes here */
    USHORT zero;	    /* Must be zero */
} RIDL, *PRIDL;

#define pidlPnsi(pnsi) ((PIDL)&(pnsi)->ridl)

/*
 * The only abnormal key is Network Neighborhood, since its presence
 * is controlled by a system policy...
 *
 * You need to set nsiflNever if something should not have a check
 * box next to it because it cannot be added to the namespace.
 *
 * You need to set nsiflDir for things that aren't valid namespace
 * items but should be created as directory-like objects.
 *
 * Briefcase is doubly abnormal, because it doesn't do anything at all!
 * So we just exclude him from the enumeration.
 */

typedef BYTE NSIFL;		/* Random flags */
#define nsiflNormal 1		/* Is a regular thing */
#define nsiflDir 2		/* Is a directory-like object */
#define nsiflNever 4		/* Not a valid namespace item */
#define nsiflEdited 8		/* The name has been edited */

typedef struct NSI {		/* namespace item */
    NSIFL nsifl;		/* Is this a normal regkey? */
    TCH tszClsid[ctchClsid];	/* Class id */
    RIDL ridl;			/* Regitem idlist */
} NSI, *PNSI;

#define insiPlvi(plvi) ((UINT)(plvi)->lParam)
#define pnsiInsi(insi) (&pddii->pnsi[insi])
#define pnsiPlvi(plvi) pnsiInsi(insiPlvi(plvi))

typedef struct DDII {
    Declare_Gxa(NSI, nsi);
    HKEY    hkNS;
} DDII, *PDDII;

DDII ddii;
#define pddii (&ddii)

/*****************************************************************************
 *
 *  Desktop_GetClsidAttributes
 *
 *  Return the Attributes registry key for a class id.
 *
 *****************************************************************************/

#define ctchPathShellFolder 21

DWORD PASCAL
Desktop_GetClsidAttributes(PCTSTR ptszClsid)
{
    TCH tsz[ctchPathShellFolder + ctchClsid];
    wsprintf(tsz, c_tszPathShellFolder, ptszClsid);
    return GetRegDword(hhkCR, tsz, c_tszAttributes, 0);
}

/*****************************************************************************
 *
 *  Desktop_HasSubkey
 *
 *  Return whether the key has a child key with the specified name.
 *
 *****************************************************************************/

BOOL PASCAL
Desktop_HasSubkey(HKEY hk, LPCTSTR ptszChild)
{
    HKEY hkS;
    if (_RegOpenKey(hk, ptszChild, &hkS) == 0) {
	RegCloseKey(hkS);
	return 1;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  Desktop_IsNSKey
 *
 *  Determine whether the specified class is in the desktop namespace
 *  right now.
 *
 *****************************************************************************/

#define Desktop_IsNSKey(ptszClsid) Desktop_HasSubkey(pddii->hkNS, ptszClsid)

/*****************************************************************************
 *
 *  Desktop_GetNetHood
 *
 *  Determine whether the network neighborhood is visible now.
 *
 *****************************************************************************/

#define Desktop_GetNetHood() GetRestriction(c_tszNoNetHood)

/*****************************************************************************
 *
 *  Desktop_SetNetHood
 *
 *  Set the new network neighborhood visibility.
 *
 *****************************************************************************/

#define Desktop_SetNetHood(f) SetRestriction(c_tszNoNetHood, f)

/*****************************************************************************
 *
 *  Desktop_IsHereNow
 *
 *	Determine the state of the object as it is in the world today.
 *
 *****************************************************************************/

#define Desktop_IsHereNow(pnsi, ptszClsid) \
    ((pnsi->nsifl & nsiflNormal) ? Desktop_IsNSKey(ptszClsid) \
				 : Desktop_GetNetHood())

/*****************************************************************************
 *
 *  Desktop_AddNSKey
 *
 *  Okay, we've committed ourselves to adding the key to the listview.
 *  No turning back now!
 *
 *  We default to the shared desktop, so set the initial state accordingly.
 *
 *****************************************************************************/

void PASCAL
Desktop_AddNSKey(HWND hwnd, PNSI pnsi, LPCTSTR ptszClsid,
		 LPCTSTR ptszFriendly, int iImage, NSIFL nsifl)
{
    pnsi->nsifl = nsifl;
    lstrcpy(pnsi->tszClsid, ptszClsid);
    LV_AddItem(hwnd, pddii->cnsi++, ptszFriendly, iImage,
	       (nsifl & nsiflNever) ? -1 : Desktop_IsHereNow(pnsi, ptszClsid));
}

/*****************************************************************************
 *
 *  Desktop_MakeRidl
 *
 *  Initialize a RIDL from a GUID display name.
 *
 *****************************************************************************/

HRESULT PASCAL
Desktop_MakeRidl(PNSI pnsi, LPCTSTR ptszClsid)
{
    pnsi->ridl.cb = 20;	    /* Always */
    pnsi->ridl.sig = 0x1F;  /* Always */
    pnsi->ridl.zero = 0;    /* Always */
    return Ole_ClsidFromString(ptszClsid, &pnsi->ridl.clsid);
}

/*****************************************************************************
 *
 *  Desktop_CheckNSKey
 *
 *  Check and possibly add a new namespace key.  My Computer is
 *  excluded because you can't get rid of it.  Network Neighborhood
 *  is here, although it is somewhat weird.  We handle the weirdness
 *  as it arises...
 *
 *  Briefcase is excluded because it doesn't work.
 *
 *  All that has been validated so far is that the key exists, it
 *  has a ShellEx subkey, and it has nonzero attributes.
 *
 *  We haven't yet validated that it has an icon.  We'll notice that
 *  when we try to build up the listview info.
 *
 *  And people complain that lisp has too many levels of nesting...
 *
 *****************************************************************************/

void PASCAL
Desktop_CheckNSKey(HWND hwnd, HKEY hk, LPCTSTR ptszClsid, NSIFL nsifl)
{
    PNSI pnsi = Misc_AllocPx(&pddii->gxa);
    if (pnsi) {
	if (SUCCEEDED(Desktop_MakeRidl(pnsi, ptszClsid)) &&
	    !IsEqualGUID(&pnsi->ridl.clsid, &c_clsidBfc)) {
	    SHFILEINFO sfi;
	    if (SHGetFileInfo((LPCSTR)pidlPnsi(pnsi), 0, &sfi, cbX(sfi),
		SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX |
		SHGFI_SMALLICON)) {
		/*
		 * Stupid gacky Net Hood hack.  Shell won't give me a name
		 * if I don't have one in the registry.
		 */
		if (sfi.szDisplayName[0] == 0 && !(nsifl & nsiflNormal)) {
		    LoadString(hinstCur, IDS_NETHOOD,
			       sfi.szDisplayName, cA(sfi.szDisplayName));
		}

		/*
		 *  It must have a name and must have a custom icon.
		 */
		if (sfi.szDisplayName[0] && sfi.iIcon != 3) {
		    Desktop_AddNSKey(hwnd, pnsi, ptszClsid, sfi.szDisplayName,
				     sfi.iIcon, nsifl);
		}
	    } /* couldn't get file info */
	} /* not a valid guid */
    } /* else no memory to store this entry */
}

/*****************************************************************************
 *
 *  Desktop_AddSpecialNSKey
 *
 *	Add some special namespace keys, which eluded our enumeration.
 *
 *****************************************************************************/

void PASCAL
Desktop_AddSpecialNSKey(HWND hwnd, LPCTSTR ptszClsid, NSIFL nsifl)
{
    int insi;
    HKEY hk;
    for (insi = 0; insi < pddii->cnsi; insi++) {
	if (lstrcmpi(pddii->pnsi[insi].tszClsid, ptszClsid) == 0) {
	    goto found;
	}
    }

    hk = hkOpenClsid(ptszClsid);
    if (hk) {
	Desktop_CheckNSKey(hwnd, hk, ptszClsid, nsifl);
	RegCloseKey(hk);
    }
    found:;
}

/*****************************************************************************
 *
 *  Desktop_EnumClasses
 *
 *  Locate all the classes that are possible namespace keys.
 *
 *****************************************************************************/

void PASCAL
Desktop_EnumClasses(HWND hwnd)
{
    int ihk;
    TCH tsz[ctchClsid];
    for (ihk = 0; RegEnumKey(pcdii->hkClsid, ihk, tsz, cA(tsz)) == 0; ihk++) {
/* BUGBUG -- exclude bfc here */
	HKEY hk = hkOpenClsid(tsz);
	if (hk) {
	    if (Desktop_GetClsidAttributes(tsz)) {
		Desktop_CheckNSKey(hwnd, hk, tsz, nsiflNormal);
	    }
	    RegCloseKey(hk);
	}
    }

    Desktop_AddSpecialNSKey(hwnd, c_tszClsidNetHood, nsiflDir);
    Desktop_AddSpecialNSKey(hwnd, c_tszClsidCpl,
			    nsiflNormal | nsiflNever | nsiflDir);
    Desktop_AddSpecialNSKey(hwnd, c_tszClsidPrint,
			    nsiflNormal | nsiflNever | nsiflDir);
    Misc_LV_SetCurSel(hwnd, 0);		/* Default to top of list */
}

/*****************************************************************************
 *
 *  Desktop_OnInitDialog
 *
 *  We have much nontrivial work to do.  Fill all the list boxes with
 *  defaults.
 *
 *****************************************************************************/

BOOL PASCAL
Desktop_OnInitDialog(HWND hwnd)
{
    ZeroMemory(pddii, cbX(*pddii));
    
    if (Misc_InitPgxa(&pddii->gxa, cbX(NSI))) {
	if (RegCreateKey(pcdii->hkLMExplorer, c_tszDesktopNameSpace,
			 &pddii->hkNS) == 0) {
	    Desktop_EnumClasses(hwnd);
	}
    }
    return 1;
}

/*****************************************************************************
 *
 *  Desktop_OnDestroy
 *
 *  Free the memory we allocated.
 *
 *  We also destroy the imagelist, because listview gets confused if
 *  it gets two image lists which are the same.
 *
 *****************************************************************************/

BOOL PASCAL
Desktop_OnDestroy(HWND hdlg)
{
    Misc_FreePgxa(&pddii->gxa);
    if (pddii->hkNS) {
	RegCloseKey(pddii->hkNS);
    }
    return 1;
}

#if 0
/*****************************************************************************
 *
 *  Desktop_FactoryReset
 *
 *	This is scary and un-undoable, so let's do extra confirmation.
 *
 *****************************************************************************/

void PASCAL
Desktop_FactoryReset(HWND hdlg)
{
    if (MessageBoxId(hdlg, IDS_DESKTOPRESETOK,
		     tszName, MB_YESNO + MB_DEFBUTTON2) == IDYES) {
	pcdii->fRunShellInf = 1;
	Common_NeedLogoff(hdlg);
	PropSheet_Apply(GetParent(hdlg));
    }
}
#endif

/*****************************************************************************
 *
 *  Desktop_OnCreateNow
 *
 *	Somebody asked to create another one...
 *
 *	Use common dialogs to do the work.
 *
 *****************************************************************************/

void PASCAL
Desktop_OnCreateNow(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    COFN cofn;

    lvi.pszText = cofn.tsz;
    lvi.cchTextMax = cA(cofn.tsz);
    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM | LVIF_TEXT);

    InitOpenFileName(GetParent(hwnd), &cofn, IDS_ALLFILES, cofn.tsz);
    cofn.ofn.nMaxFile -= ctchClsid + 1;	/* Leave room for dot and clsid */
    cofn.ofn.Flags |= OFN_NOREADONLYRETURN;

    if (GetSaveFileName(&cofn.ofn)) {
	PNSI pnsi;
	lstrcat(cofn.tsz, c_tszDot);
	pnsi = pnsiPlvi(&lvi);
	lstrcat(cofn.tsz, pnsi->tszClsid);
	if ((pnsi->nsifl & nsiflDir) ||
	    Desktop_GetClsidAttributes(pnsi->tszClsid) & SFGAO_FOLDER) {
	    CreateDirectory(cofn.tsz, 0);
	} else {
	    fCreateNil(cofn.tsz);
	}
    }
}

#if 0
/*****************************************************************************
 *
 *  Desktop_OnCommand
 *
 *****************************************************************************/

void PASCAL
Desktop_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_RESET:
	if (codeNotify == BN_CLICKED) {
	    Desktop_FactoryReset(hdlg);
	}
	break;
    }
}
#endif

/*****************************************************************************
 *
 *  Desktop_LV_Dirtify
 *
 *	Mark this item as having been renamed during the property sheet
 *	page's lifetime.
 *
 *****************************************************************************/

void PASCAL
Desktop_LV_Dirtify(LPARAM insi)
{
    pddii->pnsi[insi].nsifl |= nsiflEdited;
}

/*****************************************************************************
 *
 *  Desktop_LV_GetIcon
 *
 *	Produce the icon associated with an item.  This is called when
 *	we need to rebuild the icon list after the icon cache has been
 *	purged.
 *
 *****************************************************************************/

int PASCAL
Desktop_LV_GetIcon(LPARAM insi)
{
    SHFILEINFO sfi;
    sfi.iIcon = 0;
    SHGetFileInfo((LPCSTR)pidlPnsi(pnsiInsi(insi)), 0, &sfi, cbX(sfi),
	SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    return sfi.iIcon;
}

/*****************************************************************************
 *
 *  Desktop_OnSelChange
 *
 *	Disable the "Create as File" button if we are on Net Hood.
 *
 *****************************************************************************/

void PASCAL
Desktop_OnSelChange(HWND hwnd, int iItem)
{
    PNSI pnsi = pnsiInsi(Misc_LV_GetParam(hwnd, iItem));
    EnableWindow(GetDlgItem(GetParent(hwnd), IDC_CREATENOW),
		 pnsi->nsifl & nsiflNormal);
}

/*****************************************************************************
 *
 *  Desktop_OnApply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

void PASCAL
Desktop_OnApply(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_ICONLV);
    int cItems = ListView_GetItemCount(hwnd);
    BOOL fChanged = 0;
    LV_ITEM lvi;
    TCH tsz[cchFriendlyMax];

    lvi.pszText = tsz;
    lvi.cchTextMax = cA(tsz);

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++) {
	PNSI pnsi;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem,
			    LVIF_PARAM | LVIF_TEXT | LVIF_STATE);
	pnsi = pnsiPlvi(&lvi);

	if (Desktop_IsHereNow(pnsi, pnsi->tszClsid) != LV_IsChecked(&lvi)) {
	    fChanged = 1;
	    if (pnsi->nsifl & nsiflNormal) {
		if (LV_IsChecked(&lvi)) {
		    HKEY hk;
		    if (RegCreateKey(pddii->hkNS, pnsi->tszClsid, &hk) == 0) {
			RegSetValuePtsz(hk, 0, lvi.pszText);
			RegCloseKey(hk);
		    }
		} else {
		    RegDeleteTree(pddii->hkNS, pnsi->tszClsid);
		}
	    } else {		/* Ah, the Net Hood... */
		Desktop_SetNetHood(LV_IsChecked(&lvi));
		Common_NeedLogoff(hdlg);
		if (!LV_IsChecked(&lvi)) {
		    if (MessageBoxId(hdlg, IDS_NONETHOOD, g_tszName, MB_YESNO)
				    == IDYES) {
			WinHelp(hdlg, c_tszMyHelp, HELP_CONTEXT, IDH_NONETHOOD);
		    }
		}
	    }
	}

	/*  Not worth cacheing this */
	if (pnsi->nsifl & nsiflEdited) {
	    SetNameOfPidl(psfDesktop, pidlPnsi(pnsi), lvi.pszText);
	    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD,
			   (LPVOID)lvi.iImage, 0L);
	}
    }
    if (fChanged) {
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_DWORD, 0L, 0L);
    }
}

/*****************************************************************************
 *
 *  Desktop_LV_OnInitContextMenu
 *
 *	Propagate the status of the Create Now button.
 *
 *****************************************************************************/

void PASCAL
Desktop_LV_OnInitContextMenu(HWND hwnd, int iItem, HMENU hmenu)
{
    Misc_EnableMenuFromHdlgId(hmenu, GetParent(hwnd), IDC_CREATENOW);
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvDesktop = {
#if 0
    Desktop_OnCommand,
#else
    0,
#endif
    Desktop_LV_OnInitContextMenu,
    Desktop_LV_Dirtify,
    Desktop_LV_GetIcon,
    Desktop_OnInitDialog,
    Desktop_OnApply,
    Desktop_OnDestroy,
    Desktop_OnSelChange,
    1,				/* iMenu */
    rgdwHelp,
    0,				/* Double-click action */
    lvvflIcons |                /* We need icons */
    lvvflCanCheck |             /* And check boxes */
    lvvflCanRename,             /* and you can rename by clicking */
    {
	{ IDC_CREATENOW,	Desktop_OnCreateNow },
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
Desktop_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvDesktop, hdlg, wm, wParam, lParam);
}
