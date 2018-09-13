/*
 * template - Dialog box property sheet for "Templates"
 */

#include "tweakui.h"

#define ctchProgMax 80

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
	IDC_TEMPLATETEXT,	IDH_TEMPLATE,
	IDC_TEMPLATE,		IDH_TEMPLATE,
	IDC_LVDELETE,		IDH_TEMPLATEDEL,
	0,			0,
};

#pragma END_CONST_DATA

typedef struct TI {		/* ti - template info */
    BYTE isi;			/* Index to state icon */
    HKEY hkRoot;		/* Root for locating the key */
				/* HKCR or HKCR\CLSID */
    TCH tszExt[10];		/* Filename extension (for icon refresh) */
    TCH tszKey[ctchKeyMax + 6];	/* 6 = strlen("\\CLSID") */
} TI, *PTI;

#define itiPlvi(plvi) ((UINT)(plvi)->lParam)
#define ptiIti(iti) (&ptdii->pti[iti])
#define ptiPlvi(plvi) ptiIti(itiPlvi(plvi))

typedef struct TDII {
    Declare_Gxa(TI, ti);
    WNDPROC wpTemplate;
    BOOL fRundll;		/* Need to run Rundll on Apply */
} TDII, *PTDII;

TDII tdii;
#define ptdii (&tdii)

/*****************************************************************************
 *
 *  ptszStrRChr
 *
 *	Get the rightmost occurrence.
 *
 *****************************************************************************/

PTSTR PASCAL
ptszStrRChr(PCTSTR ptsz, TCH tch)
{
    PTSTR ptszRc = 0;
    for (ptsz = ptszStrChr(ptsz, tch); ptsz; ptsz = ptszStrChr(ptsz + 1, tch)) {
	ptszRc = (PTSTR)ptsz;
    }
    return ptszRc;
}

/*****************************************************************************
 *
 *  ptszFilenameCqn
 *
 *	Get the filename part of a cqn.
 *
 *****************************************************************************/

PTSTR PASCAL
ptszFilenameCqn(PCTSTR cqn)
{
    PTSTR ptsz = ptszStrRChr(cqn, TEXT('\\'));
    return ptsz ? ptsz + 1 : (PTSTR)cqn;
}

/*****************************************************************************
 *
 *  Template_NudgeExplorer
 *
 *  Explorer doesn't recognize changes to templates until an application
 *  terminates, so we simply execute Rundll spuriously.  It realizes that
 *  there is nothing to do and exits.  This exit triggers Explorer to
 *  rebuild the filename extension list.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA
ConstString(c_tszRundll, "rundll32");
#pragma END_CONST_DATA

void PASCAL
Template_NudgeExplorer(void)
{
    WinExec(c_tszRundll, SW_HIDE);
}

/*****************************************************************************
 *
 *  Template_SubkeyExists
 *
 *****************************************************************************/

BOOL PASCAL
Template_SubkeyExists(HKEY hk, PCTSTR ptszSubkey)
{
    return GetRegStr(hk, 0, ptszSubkey, 0, 0);
}

/*****************************************************************************
 *
 *  Template_AddTemplateInfo
 *
 *  pti must be the next ti in the array (i.e., Misc_AllocPx)
 *
 *  Returns the icon index, if successful, or -1 on error.
 *
 *****************************************************************************/

int PASCAL
Template_AddTemplateInfo(HWND hwnd, PCTSTR ptszExt, PTI pti, BOOL fCheck)
{
    int iRc;
    SHFILEINFO sfi;
    if (SHGetFileInfo(ptszExt, 0, &sfi, cbX(sfi),
	SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES |
	SHGFI_SYSICONINDEX | SHGFI_SMALLICON)) {
	pti->isi = fCheck + 1;
	iRc = LV_AddItem(hwnd, ptdii->cti++, sfi.szTypeName, sfi.iIcon, fCheck);
    } else {
	iRc = -1;
    }
    return iRc;
}

/*****************************************************************************
 *
 *  TemplateCallbackInfo
 *
 *  Information handed to a template callback.
 *
 *  hk - The ptszShellNew subkey itself
 *
 *****************************************************************************/

typedef BOOL (PASCAL *TCICALLBACK)(struct TCI *ptci, HKEY hk,
				   PCTSTR ptszShellNew);

typedef struct TCI {
    TCICALLBACK pfn;
    PTI	pti;			/* Template info containing result */
    HWND hwnd;			/* Listview window handle */
    PCTSTR ptszExt;		/* The .ext being studied */
} TCI, *PTCI;

/*****************************************************************************
 *
 *  Template_CheckShellNew
 *
 *	Look for the ShellNew stuff (or ShellNew- if temporarily disabled).
 *
 *	Returns boolean success/failure.
 *
 *	If ptszExt exists, then we create the key, too.
 *
 *****************************************************************************/

BOOL PASCAL
Template_CheckShellNew(PTCI ptci, HKEY hkBase, PCTSTR ptszShellNew)
{
    HKEY hk;
    BOOL fRc;
    if (_RegOpenKey(hkBase, ptszShellNew, &hk) == 0) {
	if (Template_SubkeyExists(hk, c_tszNullFile) ||
	    Template_SubkeyExists(hk, c_tszFileName) ||
	    Template_SubkeyExists(hk, c_tszCommand) ||
	    Template_SubkeyExists(hk, c_tszData)) {
	    fRc = ptci->pfn(ptci, hk, ptszShellNew);
	} else {
	    fRc = 0;
	}
	RegCloseKey(hk);
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Template_AddHkeyTemplate
 *
 *	We have a candidate location for a ShellNew.
 *
 *	There is some weirdness here.  The actual ShellNew can live at
 *	a sublevel.  For example, the ".doc" extension contains several
 *	ShellNew's:
 *
 *	HKCR\.doc = WordPad.Document.1
 *	HKCR\.doc\WordDocument\ShellNew
 *	HKCR\.doc\Word.Document.6\ShellNew
 *	HKCR\.doc\WordPad.Document.1\ShellNew
 *
 *	Based on the main value (HKCR\.doc), we choose to use the template
 *	for "Wordpad.Document.1".
 *
 *	pti->hkRoot - hkey at which to start (either HKCR or HKCR\CLSID)
 *	pti->tszKey - subkey to open (.ext or {guid})
 *
 *	pti->hkRoot\pti->tszKey = actual key to open
 *
 *	hkBase = the key at hkRoot\ptszKey
 *	hk     = the key where ShellNew actually lives
 *
 *****************************************************************************/

BOOL PASCAL
Template_AddHkeyTemplate(PTCI ptci)
{
    HKEY hkBase;
    BOOL fRc;
    PTI pti = ptci->pti;
    if (_RegOpenKey(pti->hkRoot, pti->tszKey, &hkBase) == 0) {
	TCH tszProg[ctchProgMax+1];
	if (GetRegStr(hkBase, 0, 0, tszProg, cbX(tszProg))) {
	    HKEY hk;
	    if (_RegOpenKey(hkBase, tszProg, &hk) == 0) {
		lstrcatnBsA(pti->tszKey, tszProg);
	    } else {
		_RegOpenKey(hkBase, 0, &hk);	/* hk = AddRef(hkBase) */
	    }
	    /* Now open the ShellNew subkey or the ShellNew- subkey */
	    fRc = fLorFF(Template_CheckShellNew(ptci, hk, c_tszShellNew),
		         Template_CheckShellNew(ptci, hk, c_tszShellNewDash));
            RegCloseKey(hk);
	} else {
	    fRc = 0;
	}
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Template_LocateExtension
 *
 *	We have a filename extension (ptci->ptszExt).
 *
 *	Get its program id (progid).
 *
 *	We look in two places.  If the progid has a registered CLSID
 *	(HKCR\progid\CLSID), then we look in HKCR\CLSID\{guid}.
 *
 *	If the CLSID fails to locate anything, then we look under
 *	HKCR\.ext directly.
 *
 *****************************************************************************/

BOOL PASCAL
Template_LocateExtension(PTCI ptci)
{
    BOOL fRc;
    PTI pti = ptci->pti = Misc_AllocPx(&ptdii->gxa);
    if (pti) {
	TCH tszProg[ctchProgMax + ctchKeyMax + 6]; /* 6 = strlen(\\CLSID) */
	lstrcpyn(pti->tszExt, ptci->ptszExt, cA(pti->tszExt));
	if (GetRegStr(hkCR, ptci->ptszExt, 0, tszProg, cbCtch(ctchProgMax)) &&
	    tszProg[0]) {
	    DWORD cb;
	    /*
	     *	Make sure the progid exists, or we end up putting garbage
	     *	into the listview.
	     */
	    if (RegQueryValue(hkCR, tszProg, 0, &cb) == 0) {

		/*
		 *  Is this an OLE class?
		 */
		lstrcatnBsA(tszProg, c_tszClsid);
		if (GetRegStr(hkCR, tszProg, 0,
			      pti->tszKey, cbX(pti->tszKey))) {
		    pti->hkRoot = pcdii->hkClsid;
		    fRc = Template_AddHkeyTemplate(ptci);
		} else {
		    fRc = 0;
		}

		/*
		 *  If we haven't succeeded yet, then try under the extension
		 *  itself.
		 */
		if (!fRc) {
		    pti->hkRoot = hkCR;
		    lstrcpyn(pti->tszKey, ptci->ptszExt, cA(pti->tszKey));
		    fRc = Template_AddHkeyTemplate(ptci);
		}
	    } else {
		fRc = 0;
	    }
	} else {
	    fRc = 0;
	}
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Template_AddTemplates_AddMe
 *
 *	Add any templates that exist.
 *
 *	For each filename extension, get its corresponding class.
 *	Then ask Template_LocateExtension to do the rest.
 *
 *****************************************************************************/

BOOL PASCAL
Template_AddTemplates_AddMe(PTCI ptci, HKEY hk, PCTSTR ptszShellNew)
{
    return Template_AddTemplateInfo(ptci->hwnd, ptci->ptszExt,
				    ptci->pti,
				    ptszShellNew == c_tszShellNew) + 1;
}

/*****************************************************************************
 *
 *  Template_AddTemplates
 *
 *	Add any templates that exist.
 *
 *	For each filename extension, get its corresponding class.
 *	Then ask Template_LocateExtension to do the rest.
 *
 *****************************************************************************/

void PASCAL
Template_AddTemplates(HWND hwnd)
{
    int i;
    TCI tci;
    TCH tszExt[10];

    tci.pfn = Template_AddTemplates_AddMe;
    tci.hwnd = hwnd;
    tci.ptszExt = tszExt;

    for (i = 0; ; i++) {
	switch (RegEnumKey(HKEY_CLASSES_ROOT, i, tszExt, cbX(tszExt))) {
	case ERROR_SUCCESS:
	    /*
	     *  Don't show ".lnk" in the templates list because it's weird.
	     */
	    if (tszExt[0] == TEXT('.') && lstrcmpi(tszExt, c_tszDotLnk)) {
		Template_LocateExtension(&tci);
	    }
	    break;

	case ERROR_MORE_DATA:		/* Can't be a .ext if > 10 */
	    break;

	default: goto endenum;
	}
    }
    endenum:;
}

/*****************************************************************************
 *
 *  File template callback info
 *
 *****************************************************************************/

typedef struct FTCI {
    TCI tci;
    PCTSTR ptszSrc;
    PCTSTR ptszDst;
    PCTSTR ptszLastBS;
    PCTSTR ptszShellNew;
} FTCI, *PFTCI;

/*****************************************************************************
 *
 *  Template_CopyFile
 *
 *	Copy a file with the hourglass.
 *
 *****************************************************************************/

BOOL PASCAL
Template_CopyFile(PFTCI pftci)
{
    HCURSOR hcurPrev;
    BOOL fRc;
    hcurPrev = SetCursor(LoadCursor(0, MAKEINTRESOURCE(IDC_WAIT)));
    fRc = CopyFile(pftci->ptszSrc, pftci->ptszDst, 0);
    SetCursor(hcurPrev);
    return fRc;
}

/*****************************************************************************
 *
 *  Template_AddFileTemplate_CheckMe
 *
 *	Make sure the key isn't a Command key.
 *
 *****************************************************************************/

BOOL PASCAL
Template_AddFileTemplate_CheckMe(PTCI ptci, HKEY hk, PCTSTR ptszShellNew)
{
    PFTCI pftci = (PFTCI)ptci;
    pftci->ptszShellNew = ptszShellNew;
    if (Template_SubkeyExists(hk, c_tszCommand)) {
	ptci->ptszExt = 0;
    }
    return 1;
}

/*****************************************************************************
 *
 *  Template_IsTemplatable
 *
 *	Determine whether there is an application that can handle this
 *	extension.
 *
 *	The incoming hk is the HKEY_CLASSES_ROOT\<extension> key.
 *
 *	BUGBUG -- should also fail if the app is "%1", meaning that
 *	the document is self-running.
 *
 *****************************************************************************/

/* BUGBUG -- FindExecutable? */
/* BUGBUG -- some bozos leave c_tszShell blank */

BOOL PASCAL
Template_IsTemplatable(HKEY hk)
{
    LONG cb;
    TCH tsz[MAX_PATH];
    cb = cbX(tsz);
    if (RegQueryValue(hk, 0, tsz, &cb) == 0) {
	HKEY hkClass;
	if (_RegOpenKey(HKEY_CLASSES_ROOT, tsz, &hkClass) == 0) {
	    BOOL fRc;
	    cb = cbX(tsz);
	    fRc = RegQueryValue(hkClass, c_tszShell, 0, &cb) == 0;
	    RegCloseKey(hkClass);
	    return fRc;
	} else {
	    return 0;
	}
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  Template_ReplaceTemplate
 *
 *	Replace the current template with the new one.
 *
 *****************************************************************************/

UINT PASCAL
Template_ReplaceTemplate(PFTCI pftci)
{
#define ptci (&pftci->tci)
    UINT id;
    if (ptci->ptszExt) {
	if (MessageBoxId(GetParent(ptci->hwnd), IDS_CONFIRMNEWTEMPLATE,
			 pftci->ptszSrc,
			 MB_YESNO | MB_DEFBUTTON2 | MB_SETFOREGROUND)
			== IDYES) {
	    HKEY hk;
	    lstrcatnBsA(ptci->pti->tszKey, pftci->ptszShellNew);
	    if (_RegOpenKey(ptci->pti->hkRoot, ptci->pti->tszKey, &hk) == 0) {
		if (Template_CopyFile(pftci)) {
		    RegDeleteValue(hk, c_tszNullFile);
		    RegDeleteValue(hk, c_tszData);
		    RegSetValuePtsz(hk, c_tszFileName, pftci->ptszLastBS+1);
		    id = 0;
		} else {
		    id = IDS_COPYFAIL;
		}
		RegCloseKey(hk);
	    } else {
		id = IDS_REGFAIL;
	    }
	} else {
	    id = 0;
	}
    } else {
	id = IDS_CANNOTTEMPLATE;
    }
    return id;
}
#undef ptci


/*****************************************************************************
 *
 *  Template_AddFileTemplate
 *
 *	Add the file as a new template type.
 *
 *****************************************************************************/

UINT PASCAL
Template_AddFileTemplate(HWND hwnd, PCTSTR ptszSrc)
{
#define pftci (&ftci)
    UINT id;
    FTCI ftci;
    pftci->tci.pfn = Template_AddFileTemplate_CheckMe;
    pftci->tci.hwnd = hwnd;
    pftci->ptszSrc = ptszSrc;
    pftci->ptszLastBS = ptszFilenameCqn(ptszSrc) - 1;	/* -> \filename.ext */
    if (pftci->ptszLastBS) {
	pftci->tci.ptszExt = ptszStrRChr(pftci->ptszLastBS, '.'); /* -> .ext */
	if (pftci->tci.ptszExt) {
	    HKEY hk;
	    if (_RegOpenKey(hkCR, pftci->tci.ptszExt, &hk) == 0) {
		if (Template_IsTemplatable(hk)) {
		    PTI pti;
		    TCH tszDst[MAX_PATH];
		    pftci->ptszDst = tszDst;
		    SHGetPathFromIDList(pcdii->pidlTemplates, tszDst);
		    lstrcatnBsA(tszDst, pftci->ptszLastBS+1);
		    /* Snoop at the next pti to ensure we can get it later */
		    pti = Misc_AllocPx(&ptdii->gxa);
		    if (pti) {
			if (Template_LocateExtension(&pftci->tci)) {
			    id = Template_ReplaceTemplate(pftci);
			} else {
			    if (Template_CopyFile(pftci)) {
				HKEY hk2;
				if (RegCreateKey(hk, c_tszShellNew, &hk2) == 0) {
				    RegSetValuePtsz(hk2, c_tszFileName, pftci->ptszLastBS+1);
				    RegCloseKey(hk2);
				    Misc_LV_SetCurSel(hwnd,
					    Template_AddTemplateInfo(hwnd,
						pftci->tci.ptszExt, pti, 1));
				    Template_NudgeExplorer();
				    id = 0;	/* No problemo */
				} else {
				    id = IDS_REGFAIL;
				}
			    } else {
				id = IDS_COPYFAIL;
			    }
			}
		    } else {
			id = 0;		/* out of memory! */
		    }
		} else {
		    id = IDS_BADEXT;
		}
		RegCloseKey(hk);
	    } else {
		id = IDS_BADEXT;
	    }
	} else {
	    id = IDS_BADEXT;
	}
    } else {
	id = 0;				/* This can't happen! */
    }
    return id;
}

/*****************************************************************************
 *
 *  Tools_Template_OnDropFiles
 *
 *	Put the file into the template edit control.
 *
 *****************************************************************************/

void PASCAL
Tools_Template_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    if (DragQueryFile(hdrop, (UINT)-1, 0, 0) == 1) {
	UINT id;
	TCH tszSrc[MAX_PATH];

	DragQueryFile(hdrop, 0, tszSrc, cA(tszSrc));
	id = Template_AddFileTemplate(hwnd, tszSrc);
	if (id) {
	    MessageBoxId(GetParent(hwnd), id, tszSrc, MB_OK | MB_SETFOREGROUND);
	}
    } else {
	MessageBoxId(GetParent(hwnd), IDS_TOOMANY, g_tszName,
		     MB_OK | MB_SETFOREGROUND);
    }
    DragFinish(hdrop);
}

/*****************************************************************************
 *
 *  Tools_Template_WndProc
 *
 *	Subclass procedure so we can handle drag-drop to the template
 *	edit control.
 *
 *****************************************************************************/

LRESULT EXPORT
Tools_Template_WndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_DROPFILES:
	Tools_Template_OnDropFiles(hwnd, (HDROP)wParam);
	return 0;
    }
    return CallWindowProc(ptdii->wpTemplate, hwnd, wm, wParam, lParam);
}

/*****************************************************************************
 *
 *  Template_GetIcon
 *
 *	Produce the icon associated with an item.  This is called when
 *	we need to rebuild the icon list after the icon cache has been
 *	purged.
 *
 *****************************************************************************/

int PASCAL
Template_GetIcon(LPARAM iti)
{
    SHFILEINFO sfi;
    PTI pti = ptiIti(iti);
    sfi.iIcon = 0;
OutputDebugString(pti->tszExt);
    SHGetFileInfo(pti->tszExt, 0, &sfi, cbX(sfi),
	SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    return sfi.iIcon;
}

/*****************************************************************************
 *
 *  Template_OnInitDialog
 *
 *  Turn the "drop a file here" gizmo into a valid drop target.
 *
 *****************************************************************************/

BOOL PASCAL
Template_OnInitDialog(HWND hwnd)
{
    ptdii->wpTemplate = SubclassWindow(hwnd, Tools_Template_WndProc);
    DragAcceptFiles(hwnd, 1);

    if (Misc_InitPgxa(&ptdii->gxa, cbX(TI))) {
	Template_AddTemplates(hwnd);
    }
    return 1;
}

/*****************************************************************************
 *
 *  Template_OnDelete
 *
 *	Really nuke it.  The interaction between this and adding a new
 *	template is sufficiently weird that I don't want to try to do
 *	delayed-action.
 *
 *****************************************************************************/

void PASCAL
Template_OnDelete(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    HKEY hk;
    PTI pti;
    TCH tszDesc[MAX_PATH];
    lvi.pszText = tszDesc;
    lvi.cchTextMax = cA(tszDesc);
    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM | LVIF_TEXT);
    pti = ptiPlvi(&lvi);
    if (MessageBoxId(GetParent(hwnd), IDS_TEMPLATEDELETEWARN,
		    lvi.pszText, MB_YESNO | MB_DEFBUTTON2) == IDYES) {
	if (_RegOpenKey(pti->hkRoot, pti->tszKey, &hk) == 0) {
	    RegDeleteTree(hk, c_tszShellNewDash);
	    RegDeleteTree(hk, c_tszShellNew);
	    ListView_DeleteItem(hwnd, iItem);
	    Misc_LV_EnsureSel(hwnd, iItem);
	    Common_SetDirty(GetParent(hwnd));
	    RegCloseKey(hk);
	}
    }
}

/*****************************************************************************
 *
 *  Template_OnSelChange
 *
 *	Disable the Remove button if we can't remove the thing.
 *
 *****************************************************************************/

void PASCAL
Template_OnSelChange(HWND hwnd, int iItem)
{
    PTI pti = ptiIti(Misc_LV_GetParam(hwnd, iItem));
    HKEY hk;
    BOOL fEnable;
    if (_RegOpenKey(pti->hkRoot, pti->tszKey, &hk) == 0) {
	if (GetRegStr(hk, pti->isi == isiUnchecked ?
		c_tszShellNewDash : c_tszShellNew, c_tszCommand, 0, 0)) {
	    fEnable = 0;
	} else {
	    fEnable = 1;
	}
	RegCloseKey(hk);
    } else {
	fEnable = 0;
    }
    EnableWindow(GetDlgItem(GetParent(hwnd), IDC_LVDELETE), fEnable);
}

/*****************************************************************************
 *
 *  Template_OnApply
 *
 *****************************************************************************/

void PASCAL
Template_OnApply(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_TEMPLATE);
    int cItems = ListView_GetItemCount(hwnd);
    BOOL fDirty;
    LV_ITEM lvi;

    fDirty = 0;
    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++) {
	PTI pti;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_PARAM | LVIF_STATE);
	pti = ptiPlvi(&lvi);

	if (pti->isi != isiPlvi(&lvi)) {
	    PCTSTR ptszFrom, ptszTo;
	    if (pti->isi == isiUnchecked) {
		ptszFrom = c_tszShellNewDash;
		ptszTo   = c_tszShellNew;
	    } else {
		ptszFrom = c_tszShellNew;
		ptszTo   = c_tszShellNewDash;
	    }
            if (Misc_RenameReg(pti->hkRoot, pti->tszKey, ptszFrom, ptszTo)) {
                pti->isi = isiPlvi(&lvi);
	    }
	    fDirty = 1;
	}
    }
    if (fDirty) {
	Template_NudgeExplorer();
    }
}

/*****************************************************************************
 *
 *  Template_OnDestroy
 *
 *	Free the memory we allocated.
 *
 *****************************************************************************/

void PASCAL
Template_OnDestroy(HWND hdlg)
{
    Misc_FreePgxa(&ptdii->gxa);
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvTemplate = {
    0,				/* Template_OnCommand */
    0,				/* Template_OnInitContextMenu */
    0,				/* Template_Dirtify */
    Template_GetIcon,
    Template_OnInitDialog,
    Template_OnApply,
    Template_OnDestroy,
    Template_OnSelChange,
    3,
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflIcons |                /* We need icons */
    lvvflCanCheck |             /* And check boxes */
    lvvflCanDelete,             /* and you can delete them too */
    {
	{ IDC_LVDELETE,		Template_OnDelete },
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
Template_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvTemplate, hdlg, wm, wParam, lParam);
}
