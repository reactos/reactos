/*
 * paranoia - For paranoid people
 */

#include "tweakui.h"

/*
 * Stupid OLE.
 */
#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#define END_INTERFACE
#endif

#include <urlhist.h>

#pragma BEGIN_CONST_DATA

KL const c_klParanoia = { &g_hkCUSMWCV, c_tszAppletTweakUI, c_tszParanoia };
KL const c_klAudioPlay = { &c_hkCR, c_tszAudioCDBSShell, 0 };
KL const c_klNoAutorun =
    { &g_hkCUSMWCV, c_tszRestrictions, c_tszNoDriveTypeAutoRun };
KL const c_klFault = { &g_hkLMSMWCV, c_tszFault, c_tszLogFile };
KL const c_klHistoryDir = { &g_hkLMSMIE, c_tszMain, c_tszHistoryDir };

KL const c_klLastUser = { &c_hkLM, c_tszNetLogon, c_tszUserName };
KL const c_klNukeUser = { &g_hkLMSMWNTCV, c_tszWinlogon, c_tszDontDisplayLast };

GUID const CLSID_CUrlHistory = {
   0x3C374A40, 0xBAE4, 0x11CF, 0xBF, 0x7D, 0x00, 0xAA, 0x00, 0x69, 0x46, 0xEE
};

IID const IID_IUrlHistoryStg = {
   0x3C374A41, 0xBAE4, 0x11CF, 0xBF, 0x7D, 0x00, 0xAA, 0x00, 0x69, 0x46, 0xEE
};

const static DWORD CODESEG rgdwHelp[] = {
	IDC_CLEARGROUP,		IDH_GROUP,
        IDC_LISTVIEW,           IDH_CLEARLV,
	IDC_CLEARNOW,		IDH_CLEARNOW,
	IDC_CDROMGROUP,		IDH_GROUP,
	IDC_CDROMAUDIO,		IDH_CDROMAUDIO,
	IDC_CDROMDATA,		IDH_CDROMDATA,
	IDC_FAULTLOG,		IDH_FAULTLOG,
	0,			0,
};

#pragma END_CONST_DATA

/*
 *  Paranoia flags.
 *
 *  These must be less than 64K because we overload the lParam of
 *  the checklist structure; if it is above 64K, then it's treated
 *  as a function.
 */
#define pflRun        0x0001
#define pflFindDocs   0x0002
#define pflFindComp   0x0004
#define pflDoc        0x0008
#define pflUrl        0x0010
#define pflNetUse     0x0020
#define pflToDisk     0x7FFF    /* The flags that get written to disk */
#define pflNukeUser   0x8000    /* Never actually written */

typedef UINT PFL;

typedef PFL (PASCAL *CANCHANGEPROC)(void);

/*****************************************************************************
 *
 *  Paranoia_IsIEInstalled
 *
 *      Returns nonzero if IE3 is installed.
 *
 *****************************************************************************/

PFL PASCAL
Paranoia_IsIEInstalled(void)
{
    if (g_hkCUSMIE) {
        return pflUrl;
    } else {
        return 0;
    }
}

/*****************************************************************************
 *
 *  Paranoia_CanNukeUser
 *
 *      Returns nonzero if the user has permission to mess with the
 *      network logon key.
 *
 *****************************************************************************/

BOOL PASCAL
Paranoia_CanNukeUser(void)
{
    if (RegCanModifyKey(g_hkLMSMWNTCV, c_tszWinlogon)) {
        return pflNukeUser;
    } else {
        return 0;
    }
}

/*****************************************************************************
 *
 *  Paranoia_GetPfl
 *
 *      Extract a PFL bit from a PFL.  If the corresponding PFL is not
 *      supported, then return -1.
 *
 *      lParam is the pfl bit to test (or the CANCHANGE function).
 *
 *      pvRef is the pfl value to test.
 *
 *****************************************************************************/

BOOL PASCAL
Paranoia_GetPfl(LPARAM lParam, LPVOID pvRef)
{
    PFL pfl = (PFL)pvRef;
    PFL pflTest = lParam;

    if (HIWORD(lParam)) {
        CANCHANGEPROC CanChange = (CANCHANGEPROC)lParam;
        pflTest = CanChange();
    }

    if (pflTest) {
        if (pfl & pflTest) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  Paranoia_SetPfl
 *
 *      Set a PFL bit in a PFL.
 *
 *      lParam is the pfl bit to test (or the CANCHANGE function).
 *
 *      pvRef is the pfl value receive the bit.
 *
 *****************************************************************************/

void PASCAL
Paranoia_SetPfl(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    PFL *ppfl = (PFL *)pvRef;
    PFL pflTest = lParam;

    if (HIWORD(lParam)) {
        CANCHANGEPROC CanChange = (CANCHANGEPROC)lParam;
        pflTest = CanChange();
    }

    /*
     *  Note that the right thing happens if pflTest == 0
     *  (i.e., nothing).
     */
    if (f) {
        *ppfl |= pflTest;
    }
}

#pragma BEGIN_CONST_DATA

/*
 *  Note that this needs to be in sync with the IDS_PARANOIA
 *  strings.
 */
CHECKLISTITEM c_rgcliParanoia[] = {
    { Paranoia_GetPfl,  Paranoia_SetPfl, pflRun, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, pflFindDocs, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, pflFindComp, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, pflDoc, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, (LPARAM)Paranoia_IsIEInstalled, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, pflNetUse, },
    { Paranoia_GetPfl,  Paranoia_SetPfl, (LPARAM)Paranoia_CanNukeUser, },
};

#pragma END_CONST_DATA


/*****************************************************************************
 *
 *  Paranoia_ClearUrl
 *
 *	Clear the IE URL history.
 *
 *****************************************************************************/

void PASCAL
Paranoia_ClearUrl(void)
{
    IUrlHistoryStg *phst;
    HRESULT hres;

    RegDeleteKey(g_hkCUSMIE, c_tszTypedURLs);

    /*
     *	Also wipe out the MSN URL history.
     *
     *  Note that MSNVIEWER ignores the WM_SETTINGCHANGE message, so
     *  there is nothing we can do to tell it "Hey, I dorked your regkeys!"
     */
    RegDeleteKey(c_hkCU, c_tszMSNTypedURLs);

    /*
     *  Tell the IE address bar that its history has been wiped.
     *
     *  Note that this cannot be a SendNotifyMessage since it contains
     *  a string.
     */
    SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)c_tszIETypedURLs);

    /*
     *	We yielded during that broadcast.  Put the hourglass back.
     */
    SetCursor(LoadCursor(0, IDC_WAIT));

    /*
     *	Now wipe the history cache.
     */
    hres = mit.SHCoCreateInstance(0, &CLSID_CUrlHistory, 0, &IID_IUrlHistoryStg,
				  (PPV)&phst);
    if (SUCCEEDED(hres)) {
	IEnumSTATURL *peurl;
	hres = phst->lpVtbl->EnumUrls(phst, &peurl);
	if (SUCCEEDED(hres)) {
	    STATURL su;
	    su.cbSize = cbX(su);

	    while (peurl->lpVtbl->Next(peurl, 1, &su, 0) == S_OK) {
		phst->lpVtbl->DeleteUrl(phst, su.pwcsUrl, 0);
		Ole_Free(su.pwcsUrl);
		if (su.pwcsTitle) {
		    Ole_Free(su.pwcsTitle);
		}
	    }

	    Ole_Release(peurl);
	}

	Ole_Release(phst);
    } else {
	/*
	 *  Couldn't do it via OLE.  Do it by brute force.
	 */
	TCHAR tszPath[MAX_PATH];
        GetStrPkl(tszPath, cbX(tszPath), &c_klHistoryDir);
	if (tszPath[0]) {
	    EmptyDirectory(tszPath, 0, 0);
	}
    }
}

/*****************************************************************************
 *
 *  Paranoia_ClearNow
 *
 *	Clear the things that the pfl says.
 *
 *	The logon goo is kept in a separate registry key, so we pass it
 *	separately.
 *
 *****************************************************************************/

void PASCAL
Paranoia_ClearNow(PFL pfl)
{
    HCURSOR hcurPrev = GetCursor();

    SetCursor(LoadCursor(0, IDC_WAIT));

    if (pfl & pflRun) {
	RegDeleteValues(pcdii->hkCUExplorer, c_tszRunMRU);
    }
    if (pfl & pflFindDocs) {
	RegDeleteValues(pcdii->hkCUExplorer, c_tszFindDocsMRU);
    }
    if (pfl & pflFindComp) {
	RegDeleteValues(pcdii->hkCUExplorer, c_tszFindCompMRU);
    }
    if (pfl & pflDoc) {
	SHAddToRecentDocs(0, 0);
    }
    if (pfl & pflUrl) {
	if (g_hkCUSMIE && g_hkLMSMIE) {
	    Paranoia_ClearUrl();
	}
    }

    if (pfl & pflNetUse) {
        if (g_fNT) {
            RegDeleteValues(hkCU, c_tszNetHistoryNT);
        } else {
            RegDeleteKey(hkCU, c_tszNetHistory95);
        }
    }

    if (!g_fNT && (pfl & pflNukeUser)) {
	DelPkl(&c_klLastUser);
    }

    SetCursor(hcurPrev);
}

/*****************************************************************************
 *
 *  Paranoia_GetDlgPfl
 *
 *	Compute the pfl for the dialog box.
 *
 *****************************************************************************/

PFL PASCAL
Paranoia_GetDlgPfl(HWND hdlg)
{
    PFL pfl;

    pfl = 0;
    Checklist_OnApply(hdlg, c_rgcliParanoia, &pfl);

    return pfl;
}

/*****************************************************************************
 *
 *  Paranoia_GetRegPfl
 *
 *	Compute the pfl from the registry.
 *
 *****************************************************************************/

PFL PASCAL
Paranoia_GetRegPfl(void)
{
    PFL pfl;

    pfl = GetIntPkl(0, &c_klParanoia);
    if (GetIntPkl(0, &c_klNukeUser)) {
        pfl |= pflNukeUser;
    }
    return pfl;
}

/*****************************************************************************
 *
 *  Paranoia_CoverTracks
 *
 *****************************************************************************/

void PASCAL
Paranoia_CoverTracks(void)
{
    Paranoia_ClearNow(Paranoia_GetRegPfl());
}

/*****************************************************************************
 *
 *  Paranoia_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
Paranoia_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP,
            IDH_CLEARRUN + lvi.lParam);
}


/*****************************************************************************
 *
 *  Paranoia_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Paranoia_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_CLEARNOW:
	if (codeNotify == BN_CLICKED) {
	    PFL pfl = Paranoia_GetDlgPfl(hdlg);
            if (pfl) {
                Paranoia_ClearNow(pfl);
	    } else {
		MessageBoxId(hdlg, IDS_NOTHINGTOCLEAR, g_tszName, MB_OK);
	    }
	}
	break;

    case IDC_CDROMAUDIO:
    case IDC_CDROMDATA:
    case IDC_FAULTLOG:
	if (codeNotify == BN_CLICKED) Common_SetDirty(hdlg);
	break;

    }

    return 0;
}

/*****************************************************************************
 *
 *  Paranoia_OnInitDialog
 *
 *  Audio CD play is enabled if HKCR\AudioCD\shell = "play".
 *
 *  Fault logging is disabled on NT.
 *
 *****************************************************************************/

BOOL PASCAL
Paranoia_OnInitDialog(HWND hwnd)
{
    PFL pfl;
    HWND hdlg = GetParent(hwnd);
    TCHAR tsz[MAX_PATH];

    pfl = Paranoia_GetRegPfl();
    Checklist_OnInitDialog(hwnd, c_rgcliParanoia, cA(c_rgcliParanoia),
                           IDS_PARANOIA, (LPVOID)pfl);

    GetStrPkl(tsz, cbX(tsz), &c_klAudioPlay);
    CheckDlgButton(hdlg, IDC_CDROMAUDIO,     tsz[0]		? 1 : 0);

    CheckDlgButton(hdlg, IDC_CDROMDATA,
	(GetDwordPkl(&c_klNoAutorun, 0) & (1 << DRIVE_CDROM)) ? 0 : 1);

    if (g_fNT) {
	UINT id;
	for (id = IDC_PARANOIA95ONLYMIN; id < IDC_PARANOIA95ONLYMAX; id++) {
	    ShowWindow(GetDlgItem(hdlg, id), SW_HIDE);
	}
    } else {
	TCHAR tszPath[MAX_PATH];
	CheckDlgButton(hdlg, IDC_FAULTLOG,
		       GetStrPkl(tszPath, cbX(tszPath), &c_klFault));
    }

    return 1;
}

/*****************************************************************************
 *
 *  Paranoia_OnApply
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszFaultLog[] = TEXT("FAULTLOG.TXT");

#pragma END_CONST_DATA

void PASCAL
Paranoia_OnApply(HWND hdlg)
{
    DWORD fl;
    PFL pfl;

    pfl = Paranoia_GetDlgPfl(hdlg);
    SetIntPkl(pfl & pflToDisk, &c_klParanoia);
    SetIntPkl((pfl & pflNukeUser) != 0, &c_klNukeUser);

    SetStrPkl(&c_klAudioPlay, IsDlgButtonChecked(hdlg, IDC_CDROMAUDIO)
						    ? c_tszPlay : c_tszNil);

    fl = GetDwordPkl(&c_klNoAutorun, 0) & ~(1 << DRIVE_CDROM);
    if (!IsDlgButtonChecked(hdlg, IDC_CDROMDATA)) {
	fl |= (1 << DRIVE_CDROM);
    }
    SetDwordPkl(&c_klNoAutorun, fl);

    if (!g_fNT) {
	if (IsDlgButtonChecked(hdlg, IDC_FAULTLOG)) {
	    TCHAR tszPath[MAX_PATH];
	    GetWindowsDirectory(tszPath, cA(tszPath) - cA(c_tszFaultLog));
	    lstrcatnBs(tszPath, c_tszFaultLog, cA(tszPath));
	    SetStrPkl(&c_klFault, tszPath);
	} else {
	    DelPkl(&c_klFault);
	}
    }
}

#if 0
/*****************************************************************************
 *
 *  Paranoia_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Paranoia_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Paranoia_Apply(hdlg);
	break;
    }
    return 0;
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
Paranoia_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {

    case WM_INITDIALOG: return Paranoia_OnInitDialog(hdlg);

    case WM_COMMAND:
	return Paranoia_OnCommand(hdlg,
			          (int)GET_WM_COMMAND_ID(wParam, lParam),
			          (UINT)GET_WM_COMMAND_CMD(wParam, lParam));

    case WM_NOTIFY:
	return Paranoia_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
#endif

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvParanoia = {
    Paranoia_OnCommand,
    0,                          /* Paranoia_OnInitContextMenu */
    0,                          /* Paranoia_Dirtify */
    0,                          /* Paranoia_GetIcon */
    Paranoia_OnInitDialog,
    Paranoia_OnApply,
    0,                          /* Paranoia_OnDestroy */
    0,                          /* Paranoia_OnSelChange */
    5,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    {
        { IDC_WHATSTHIS,        Paranoia_OnWhatsThis },
        { 0,                    0 },
    },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
Paranoia_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvParanoia, hdlg, wm, wParam, lParam);
}
