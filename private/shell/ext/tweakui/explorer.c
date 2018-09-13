/*
 * explorer - Dialog box property sheet for "explorer ui tweaks"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

typedef struct SCEI {			/* Shortcut effect info */
    PCTSTR ptszDll;
    UINT iIcon;
} SCEI;

const SCEI CODESEG rgscei[] = {
    { g_tszPathShell32, 29 },
    { g_tszPathMe, IDI_ALTLINK - 1 },
    { g_tszPathMe, IDI_BLANK - 1 },
};

KL const c_klHackPtui = { &g_hkLMSMWCV, c_tszAppletTweakUI, c_tszHackPtui };
KL const c_klLinkOvl = { &pcdii->hkLMExplorer, c_tszShellIcons, c_tszLinkOvl };
KL const c_klWelcome =  { &pcdii->hkCUExplorer, c_tszTips, c_tszShow };
KL const c_klAltColor =  { &pcdii->hkCUExplorer, 0, c_tszAltColor };

const static DWORD CODESEG rgdwHelp[] = {
	IDC_LINKGROUP,		IDH_LINKEFFECT,
	IDC_LINKARROW,		IDH_LINKEFFECT,
	IDC_LIGHTARROW,		IDH_LINKEFFECT,
	IDC_NOARROW,		IDH_LINKEFFECT,
	IDC_CUSTOMARROW,	IDH_LINKEFFECT,

	IDC_LINKBEFORETEXT,	IDH_LINKEFFECT,
	IDC_LINKBEFORE,		IDH_LINKEFFECT,
	IDC_LINKAFTERTEXT,	IDH_LINKEFFECT,
	IDC_LINKAFTER,		IDH_LINKEFFECT,
	IDC_LINKHELP,		IDH_LINKEFFECT,

	IDC_STARTUPGROUP,	IDH_GROUP,
	IDC_BANNER,		IDH_BANNER,
	IDC_WELCOME,		IDH_WELCOME,

	IDC_SETGROUP,		IDH_GROUP,
	IDC_PREFIX,		IDH_PREFIX,
	IDC_EXITSAVE,		IDH_EXITSAVE,
	IDC_MAKEPRETTY,		IDH_MAKEPRETTY,
	IDC_COMPRESSTXT,	IDH_COMPRESSCLR,
	IDC_COMPRESSCLR,	IDH_COMPRESSCLR,
	IDC_COMPRESSBTN,	IDH_COMPRESSCLR,

	IDC_RESET,		IDH_RESET,
	0,			0,
};


#pragma END_CONST_DATA

typedef struct EDII {
    HIMAGELIST himl;			/* Private image list */
#ifndef USE_IMAGELIST_MERGE
    WNDPROC wpAfter;			/* Subclass procedure */
#endif
    UINT idcCurEffect;			/* currently selected effect */
    BOOL fIconDirty;			/* if the shortcut icon is dirty */
    UINT iIcon;				/* Custom shortcut effect icon */
    BOOL fInEffectChange;		/* Inside an effect change */
    HBRUSH hbrComp;			/* Compressed color brush */
    COLORREF clrComp;			/* Compressed color */
    TCH tszPathDll[MAX_PATH];		/* Custom shortcut effect dll */
} EDII, *PEDII;

EDII edii;
#define pedii (&edii)

/*****************************************************************************
 *
 *  GetRestriction
 *
 *  Determine whether a restriction is set.  Restrictions are reverse-sense,
 *  so we un-reverse them here, so that this returns 1 if the feature is
 *  enabled.
 *
 *****************************************************************************/

BOOL PASCAL
GetRestriction(LPCTSTR ptszKey)
{
    return GetRegDword(g_hkCUSMWCV, c_tszRestrictions, ptszKey, 0) == 0;
}

/*****************************************************************************
 *
 *  SetRestriction
 *
 *  Set a restriction.  Again, since restrictions are reverse-sense, we
 *  un-reverse them here, so that passing 1 enables the feature.
 *
 *****************************************************************************/

void PASCAL
SetRestriction(LPCTSTR ptszKey, BOOL f)
{
    SetRegDword(g_hkCUSMWCV, c_tszRestrictions, ptszKey, !f);
}

/*****************************************************************************
 *
 *  Explorer_GetWelcome
 *
 *  Determine whether we show a tip of the day.
 *
 *****************************************************************************/

INLINE BOOL
Explorer_GetWelcome(void)
{
    return GetDwordPkl(&c_klWelcome, 1);
}

/*****************************************************************************
 *
 *  Explorer_SetWelcome
 *
 *  Record the welcome setting.
 *
 *****************************************************************************/

INLINE void
Explorer_SetWelcome(BOOL f)
{
    SetDwordPkl(&c_klWelcome, f);
}

/*****************************************************************************
 *
 *  Explorer_HackPtui
 *
 *	Patch up a bug in comctl32, where a blank overlay image gets
 *	the wrong rectangle set into it because comctl32 gets confused
 *	when all the pixels are transparent.  As a result, the link
 *	overlay becomes THE ENTIRE IMAGELIST instead of nothing.
 *
 *	We do this by (hack!) swiping himlIcons and himlIconsSmall
 *	from SHELL32, and then (ptui!) partying DIRECTLY INTO THEM
 *	and fixing up the rectangle coordinates.
 *
 *	I'm really sorry I have to do this, but if I don't, people will
 *	just keep complaining.
 *
 *  Helper procedures:
 *
 *	Explorer_HackPtuiCough - fixes one himl COMCTL32's data structures.
 *
 *****************************************************************************/

/*
 * On entry to Explorer_HackPtuiCough, the pointer has already been
 * validated.
 */
BOOL PASCAL
Explorer_HackPtuiCough(LPBYTE lpb, LPVOID pvRef)
{
#if 0
    if (*(LPWORD)lpb == 0x4C49 &&
	*(LPDWORD)(lpb + 0x78) == 0 && *(LPDWORD)(lpb + 0x88) == 0) {
#else
    if (*(LPWORD)lpb == 0x4C49) {
#endif
	*(LPDWORD)(lpb + 0x78) = 1;
	*(LPDWORD)(lpb + 0x88) = 1;
	return 1;
    } else {
	return 0;
    }
}

void PASCAL
Explorer_HackPtui(void)
{
    if (g_fBuggyComCtl32 && GetIntPkl(0, &c_klHackPtui)) {
	if (WithSelector((DWORD)GetSystemImageList(0),
			 0x8C, (WITHPROC)Explorer_HackPtuiCough, 0, 1) &&
	    WithSelector((DWORD)GetSystemImageList(SHGFI_SMALLICON),
			 0x8C, (WITHPROC)Explorer_HackPtuiCough, 0, 1)) {
	    RedrawWindow(0, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
    }
}

/*****************************************************************************
 *
 *  Explorer_SetAfterImage
 *
 *  The link overlay image has changed, so update the "after" image, too.
 *  All we have to do is invalidate the window; our WM_PAINT handler will
 *  paint the new effect.
 *
 *****************************************************************************/

INLINE void
Explorer_SetAfterImage(HWND hdlg)
{
    InvalidateRect(GetDlgItem(hdlg, IDC_LINKAFTER), 0, 1);
}

/*****************************************************************************
 *
 *  Explorer_After_OnPaint
 *
 *  Paint the merged images.
 *
 *  I used to use ILD_TRANSPARENT, except for some reason the background
 *  wasn't erased by WM_ERASEBKGND.  (Probably because statics don't
 *  process that message.)
 *
 *****************************************************************************/

LRESULT PASCAL
Explorer_After_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (hdc) {
	ImageList_SetBkColor(pedii->himl, GetSysColor(COLOR_BTNFACE));
	ImageList_Draw(pedii->himl, 0, hdc, 0, 0,
		       ILD_NORMAL | INDEXTOOVERLAYMASK(1));
	EndPaint(hwnd, &ps);
    }
    return 0;
}

/*****************************************************************************
 *
 *  Explorer_After_WndProc
 *
 *  Subclass window procedure for the after-image.
 *
 *****************************************************************************/

LRESULT EXPORT
Explorer_After_WndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_PAINT: return Explorer_After_OnPaint(hwnd);
    }
    return CallWindowProc(pedii->wpAfter, hwnd, wm, wp, lp);
}


/*****************************************************************************
 *
 *  Explorer_GetIconSpecFromRegistry
 *
 *  The output buffer must be MAX_PATH characters in length.
 *
 *  Returns the icon index.
 *
 *****************************************************************************/

UINT PASCAL
Explorer_GetIconSpecFromRegistry(LPTSTR ptszBuf)
{
    UINT iIcon;
    if (GetStrPkl(ptszBuf, cbCtch(MAX_PATH), &c_klLinkOvl)) {
	iIcon = ParseIconSpec(ptszBuf);
    } else {
	ptszBuf[0] = TEXT('\0');
	iIcon = 0;
    }
    return iIcon;
}

/*****************************************************************************
 *
 *  Explorer_RefreshOverlayImage
 *
 *	There was a change to the shortcut effects.  Put a new overlay
 *	image into the imagelist and update the After image as well.
 *
 *****************************************************************************/

void PASCAL
Explorer_RefreshOverlayImage(HWND hdlg)
{
    HICON hicon = ExtractIcon(hinstCur, pedii->tszPathDll, pedii->iIcon);
    if ((UINT)hicon <= 1) {
	hicon = ExtractIcon(hinstCur, g_tszPathShell32, 29); /* default */
    }
    ImageList_ReplaceIcon(pedii->himl, 1, hicon);
    SafeDestroyIcon(hicon);
    Explorer_SetAfterImage(hdlg);
}

/*****************************************************************************
 *
 *  Explorer_OnEffectChange
 *
 *	There was a change to the shortcut effects.
 *
 *	Put the new effect icon into the image list and update stuff.
 *
 *	Note that if we are activated by a keypress, we get re-entered
 *	(for reasons as-yet unclear), so we need a re-entrancy detector,
 *	or we end up recursing ourselves to death.
 *
 *	BUGBUG -- THIS STILL DOESN'T WORK!  On a keyboard activation,
 *	we pop up twice.  Sigh.  I need to redesign this interface.
 *
 *****************************************************************************/

void PASCAL
Explorer_OnEffectChange(HWND hdlg, UINT id, BOOL fUI)
{
    if (!pedii->fInEffectChange) {
	pedii->fInEffectChange = 1;
	if (id != IDC_CUSTOMARROW) {
	    if (id == pedii->idcCurEffect) goto done;
	    lstrcpy(pedii->tszPathDll, rgscei[id - IDC_LINKFIRST].ptszDll);
	    pedii->iIcon = rgscei[id - IDC_LINKFIRST].iIcon;
	} else if (fUI) {
	    PickIcon(hdlg, pedii->tszPathDll, cA(pedii->tszPathDll),
			    &pedii->iIcon);
	}
	pedii->idcCurEffect = id;
	pedii->fIconDirty = 1;
	Explorer_RefreshOverlayImage(hdlg);

	Common_SetDirty(hdlg);
    done:;
	pedii->fInEffectChange = 0;
    }
}

/*****************************************************************************
 *
 *  Explorer_InitBrushes
 *
 *	Init the brushes again.
 *
 *****************************************************************************/

void PASCAL
Explorer_InitBrushes(HWND hdlg)
{
    if (pedii->hbrComp) {
	DeleteObject(pedii->hbrComp);
    }
    pedii->hbrComp = CreateSolidBrush(pedii->clrComp);
    InvalidateRect(GetDlgItem(hdlg, IDC_COMPRESSCLR), 0, TRUE);
}

/*****************************************************************************
 *
 *  Explorer_FactoryReset
 *
 *	Restore to factory settings.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_FactoryReset(HWND hdlg)
{
    if (pedii->idcCurEffect != IDC_LINKARROW) {
	if (pedii->idcCurEffect) {
	    CheckDlgButton(hdlg, pedii->idcCurEffect, 0);
	}
	CheckDlgButton(hdlg, IDC_LINKARROW, 1);
	Explorer_OnEffectChange(hdlg, IDC_LINKARROW, 0);
    }
    CheckDlgButton(hdlg, IDC_PREFIX, 1);
    CheckDlgButton(hdlg, IDC_EXITSAVE, 1);
    CheckDlgButton(hdlg, IDC_BANNER, 1);
    CheckDlgButton(hdlg, IDC_WELCOME, 1);

    if (mit.ReadCabinetState) {
	CheckDlgButton(hdlg, IDC_MAKEPRETTY, 1);
	pedii->clrComp = RGB(0x00, 0x00, 0xFF);
	Explorer_InitBrushes(hdlg);
    }
    Common_SetDirty(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *  Explorer_CompressClr_Change
 *
 *	Change the compressed color.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_CompressClr_Change(HWND hdlg)
{
    CHOOSECOLOR cc;
    DWORD rgdw[16];
    HKEY hk;

    ZeroMemory(rgdw, cbX(rgdw));
    if (RegOpenKey(HKEY_CURRENT_USER, c_tszRegPathAppearance, &hk) == 0) {
	DWORD cb = cbX(rgdw);
	RegQueryValueEx(hk, c_tszCustomColors, 0, 0, (LPVOID)rgdw, &cb);
	RegCloseKey(hk);
    }

    cc.lStructSize = cbX(cc);
    cc.hwndOwner = hdlg;
    cc.rgbResult = pedii->clrComp;
    cc.lpCustColors = rgdw;
    cc.Flags = CC_RGBINIT;
    if (ChooseColor(&cc) && pedii->clrComp != cc.rgbResult) {
	pedii->clrComp = cc.rgbResult;
	Explorer_InitBrushes(hdlg);
	Common_SetDirty(hdlg);
    }
    return 1;
}

/*****************************************************************************
 *
 *  Explorer_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_LINKARROW:
    case IDC_LIGHTARROW:
    case IDC_NOARROW:
    case IDC_CUSTOMARROW:
	if (codeNotify == BN_CLICKED) {
	    Explorer_OnEffectChange(hdlg, id, 1);
	}
	break;

    case IDC_PREFIX:
    case IDC_EXITSAVE:
    case IDC_BANNER:
    case IDC_WELCOME:
    case IDC_MAKEPRETTY:
	if (codeNotify == BN_CLICKED) Common_SetDirty(hdlg);
	break;

    case IDC_COMPRESSBTN:
	if (codeNotify == BN_CLICKED) Explorer_CompressClr_Change(hdlg);
	break;

    case IDC_RESET:	/* Reset to factory default */
	if (codeNotify == BN_CLICKED) return Explorer_FactoryReset(hdlg);
	break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Explorer_OnInitDialog
 *
 *  Find out which link icon people are using.
 *
 *  When we initialize the image list, we just throw something random
 *  into position 1.  Explorer_OnEffectChange will put the right thing in.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_OnInitDialog(HWND hdlg)
{
    int iscei;
    CABINETSTATE cs;

    pedii->fInEffectChange = 0;
    pedii->iIcon = Explorer_GetIconSpecFromRegistry(pedii->tszPathDll);

    if (pedii->tszPathDll[0]) {
	for (iscei = 0; iscei < cA(rgscei); iscei++) {
	    if (pedii->iIcon == rgscei[iscei].iIcon &&
		lstrcmpi(pedii->tszPathDll, rgscei[iscei].ptszDll) == 0) {
		break;
	    }
	}
    } else {
	iscei = 0;		/* Default */
				/* Explorer_OnEffectChange will finish */
    }
    if (iscei == IDC_LINKARROW - IDC_LINKFIRST && GetIntPkl(0, &c_klHackPtui)) {
	iscei = IDC_NOARROW - IDC_LINKFIRST;
    }

    /* BUGBUG -- wininichange */
    pedii->himl = ImageList_Create(GetSystemMetrics(SM_CXICON),
				   GetSystemMetrics(SM_CYICON), 1,
				   2, 1);
    if (pedii->himl) {
	HICON hicon;

	hicon = (HICON)SendDlgItemMessage(hdlg, IDC_LINKBEFORE,
					  STM_GETICON, 0, 0L);
	/* We start with whatever icon got dropped into IDC_BEFORE. */
	ImageList_AddIcon(pedii->himl, hicon);	/* zero */
	ImageList_AddIcon(pedii->himl, hicon);	/* one */

        if (pedii->tszPathDll[0]) {
            hicon = ExtractIcon(hinstCur, pedii->tszPathDll, pedii->iIcon);
            if (ImageList_AddIcon(pedii->himl, hicon) != 1) {
                /* Oh dear */
            }
            SafeDestroyIcon(hicon);
        }

#ifndef USE_IMAGELIST_MERGE
	ImageList_SetOverlayImage(pedii->himl, 1, 1);
	pedii->wpAfter = SubclassWindow(GetDlgItem(hdlg, IDC_LINKAFTER),
				        Explorer_After_WndProc);
#endif
    } else {
	/* Oh dear */
    }
    CheckDlgButton(hdlg, IDC_LINKFIRST + iscei, TRUE);
    Explorer_OnEffectChange(hdlg, IDC_LINKFIRST + iscei, 0);

    CheckDlgButton(hdlg, IDC_PREFIX  , Link_GetShortcutTo());
    CheckDlgButton(hdlg, IDC_EXITSAVE, GetRestriction(c_tszNoExitSave));
    CheckDlgButton(hdlg, IDC_BANNER  , GetRestriction(c_tszNoBanner  ));
    CheckDlgButton(hdlg, IDC_WELCOME , Explorer_GetWelcome());

    if (mit.ReadCabinetState && mit.ReadCabinetState(&cs, cbX(cs))) {
	CheckDlgButton(hdlg, IDC_MAKEPRETTY, !cs.fDontPrettyNames);
	pedii->clrComp = GetDwordPkl(&c_klAltColor, RGB(0x00, 0x00, 0xFF));
	Explorer_InitBrushes(hdlg);
    } else {
	UINT ui;
	mit.ReadCabinetState = 0;
	for (ui = IDC_MAKEPRETTY; ui <= IDC_COMPRESSBTN; ui++) {
	    DestroyWindow(GetDlgItem(hdlg, ui));
	}
    }

    pedii->fIconDirty = 0;
    PropSheet_UnChanged(GetParent(hdlg), hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  Explorer_ApplyOverlay
 *
 *	This applies the overlay customization.
 *
 *	HackPtui makes life (unfortunately) difficult.  We signal that
 *	HackPtui is necessary by setting the "HackPtui" registry
 *	entry to 1.
 *
 *****************************************************************************/

void PASCAL
Explorer_ApplyOverlay(void)
{
    /*
     *  Assume that nothing special is needed.
     */
    DelPkl(&c_klLinkOvl);
    DelPkl(&c_klHackPtui);

    switch (pedii->idcCurEffect) {
    case IDC_LINKARROW:
	break;				/* Nothing to do */

    case IDC_NOARROW:			/* This is the tough one */
	if (g_fBuggyComCtl32) {
	    SetIntPkl(1, &c_klHackPtui);
	} else {
	    TCH tszBuild[MAX_PATH + 1 + 6];	/* comma + 65535 */
    default:
	    wsprintf(tszBuild, c_tszSCommaU, pedii->tszPathDll, pedii->iIcon);
	    SetStrPkl(&c_klLinkOvl, tszBuild);
	}
	break;
    }
    Misc_RebuildIcoCache();
    pedii->fIconDirty = 0;
}

/*****************************************************************************
 *
 *  Explorer_Apply
 *
 *	Write the changes to the registry and force a refresh.
 *
 *	HackPtui makes life (unfortunately) difficult.  We signal that
 *	HackPtui is necessary by setting the "HackPtui" registry
 *	entry to 1.
 *
 *****************************************************************************/

void NEAR PASCAL
Explorer_Apply(HWND hdlg)
{
    CABINETSTATE cs;

    if (pedii->fIconDirty) {
	Explorer_ApplyOverlay();
    }

    if (!Link_SetShortcutTo(IsDlgButtonChecked(hdlg, IDC_PREFIX))) {
	Common_NeedLogoff(hdlg);
    }

    SetRestriction(c_tszNoExitSave, IsDlgButtonChecked(hdlg, IDC_EXITSAVE));
    SetRestriction(c_tszNoBanner  , IsDlgButtonChecked(hdlg, IDC_BANNER  ));
    Explorer_SetWelcome(IsDlgButtonChecked(hdlg, IDC_WELCOME));

    if (mit.ReadCabinetState && mit.ReadCabinetState(&cs, cbX(cs))) {
	BOOL f = !IsDlgButtonChecked(hdlg, IDC_MAKEPRETTY);
	if (cs.fDontPrettyNames != f) {
	    cs.fDontPrettyNames = f;
	    mit.WriteCabinetState(&cs);
	    Common_NeedLogoff(hdlg);
	}
	if (pedii->clrComp != GetDwordPkl(&c_klAltColor,
					  RGB(0x00, 0x00, 0xFF))) {
	    SetDwordPkl(&c_klAltColor, pedii->clrComp);
	    Common_NeedLogoff(hdlg);
	}
    }
}


/*****************************************************************************
 *
 *  Explorer_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Explorer_Apply(hdlg);
	break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Explorer_OnDestroy
 *
 *  Clean up
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_OnDestroy(HWND hdlg)
{
    ImageList_Destroy(pedii->himl);
    if (pedii->hbrComp) {
	DeleteObject(pedii->hbrComp);
    }
    return 1;
}

/*****************************************************************************
 *
 *  Explorer_OnCtlColorStatic
 *
 *	Paint the magic thing in the compressed file color.
 *
 *****************************************************************************/

HBRUSH PASCAL
Explorer_OnCtlColorStatic(HWND hwndChild)
{
    if (GetDlgCtrlID(hwndChild) == IDC_COMPRESSCLR) {
	return pedii->hbrComp;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

/*
 * The HANDLE_WM_* macros weren't designed to be used from a dialog
 * proc, so we need to handle the messages manually.  (But carefully.)
 */

BOOL EXPORT
Explorer_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Explorer_OnInitDialog(hdlg);

    case WM_COMMAND:
	return Explorer_OnCommand(hdlg,
			         (int)GET_WM_COMMAND_ID(wParam, lParam),
			         (UINT)GET_WM_COMMAND_CMD(wParam, lParam));

    case WM_NOTIFY:
	return Explorer_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_DESTROY: return Explorer_OnDestroy(hdlg);

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    case WM_CTLCOLORSTATIC:
	return (BOOL)Explorer_OnCtlColorStatic((HWND)lParam);

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
