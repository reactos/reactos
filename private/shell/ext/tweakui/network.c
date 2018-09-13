/*
 * network - Dialog box property sheet for "network goo"
 *
 * BUGBUG -- someday: Allow anonymous logon (this is tricky)
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL const c_klAutoLogon = { &g_hkLMSMWNTCV, c_tszWinlogon, c_tszAutoLogon };
KL const c_klDefUser = { &g_hkLMSMWNTCV, c_tszWinlogon, c_tszDefaultUserName };
KL const c_klDefPass = { &g_hkLMSMWNTCV, c_tszWinlogon, c_tszDefaultPassword };
KL const c_klRunServ = { &g_hkLMSMWCV, c_tszRunServices, g_tszName };

const static DWORD CODESEG rgdwHelp[] = {
	IDC_LOGONGROUP,	    IDH_GROUP,
	IDC_LOGONAUTO,	    IDH_AUTOLOGON,
	IDC_LOGONUSERTXT,   IDH_AUTOLOGONUSER,
	IDC_LOGONUSER,	    IDH_AUTOLOGONUSER,
	IDC_LOGONPASSTXT,   IDH_AUTOLOGONPASS,
	IDC_LOGONPASS,	    IDH_AUTOLOGONPASS,
	0,		    0,
};

/*
 * Instanced.  We're a cpl so have only one instance, but I declare
 * all the instance stuff in one place so it's easy to convert this
 * code to multiple-instance if ever we need to.
 *
 * Note: All our instance data is kept in the dialog itself; don't need this
 */
#if 0
typedef struct NDII {		/* network dialog instance info */
} NDII, *PNDII;

NDII ndii;
#define pndii (&ndii)
#endif

/*****************************************************************************
 *
 *  Network_SetDirty
 *
 *	Make a control dirty.
 *
 *****************************************************************************/

#define Network_SetDirty    Common_SetDirty

/*****************************************************************************
 *
 *  Network_PklToDlgItemText
 *
 *	Read dialog item text from the registry.
 *
 *****************************************************************************/

#define ctchDlgItem 256

void PASCAL
Network_PklToDlgItemText(HWND hdlg, UINT idc, PKL pkl)
{
    TCHAR tsz[ctchDlgItem];

    GetStrPkl(tsz, cbX(tsz), pkl);
    SetDlgItemTextLimit(hdlg, idc, tsz, cA(tsz));
}

/*****************************************************************************
 *
 *  Network_DlgItemTextToPkl
 *
 *	Copy dialog item text to the registry.
 *
 *****************************************************************************/

void PASCAL
Network_DlgItemTextToPkl(HWND hdlg, UINT idc, PKL pkl)
{
    TCHAR tsz[ctchDlgItem];

    GetDlgItemText(hdlg, idc, tsz, cA(tsz));
    if (tsz[0]) {
	SetStrPkl(pkl, tsz);
    } else {
	DelPkl(pkl);
    }
}

/*****************************************************************************
 *
 *  Network_Reset
 *
 *	Reset all controls to initial values.  This also marks
 *	the control as clean.
 *
 *****************************************************************************/

BOOL PASCAL
Network_Reset(HWND hdlg)
{
    CheckDlgButton(hdlg, IDC_LOGONAUTO, GetIntPkl(0, &c_klAutoLogon));

    Network_PklToDlgItemText(hdlg, IDC_LOGONUSER, &c_klDefUser);
    Network_PklToDlgItemText(hdlg, IDC_LOGONPASS, &c_klDefPass);

    Common_SetClean(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *  Network_Apply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
Network_Apply(HWND hdlg)
{
    BOOL fAuto;

    Network_DlgItemTextToPkl(hdlg, IDC_LOGONUSER, &c_klDefUser);
    Network_DlgItemTextToPkl(hdlg, IDC_LOGONPASS, &c_klDefPass);

    fAuto = IsDlgButtonChecked(hdlg, IDC_LOGONAUTO);
    if (fAuto) {
	SetIntPkl(fAuto, &c_klAutoLogon);
    } else {
	DelPkl(&c_klAutoLogon);
    }

    /*
     *	NT does this automatically, so we need to be hacky only on Win95a.
     */
    if (!g_fNT) {
	if (fAuto) {
	    SetStrPkl(&c_klRunServ, c_tszFixAutoLogon);
	} else {
	    DelPkl(&c_klRunServ);
	}
    }

    return Network_Reset(hdlg);
}

/*****************************************************************************
 *
 *  Network_FactoryReset
 *
 *	Autologon = false
 *	AutoUser = none
 *	AutoPass = none
 *	Allow anonymous = true
 *
 *****************************************************************************/

BOOL PASCAL
Network_FactoryReset(HWND hdlg)
{
    Network_SetDirty(hdlg);

    CheckDlgButton(hdlg, IDC_LOGONAUTO, FALSE);
    SetDlgItemText(hdlg, IDC_LOGONUSER, c_tszNil);
    SetDlgItemText(hdlg, IDC_LOGONPASS, c_tszNil);
#pragma message("BUGBUG -- anonymous")
    return 1;
}

/*****************************************************************************
 *
 *  Network_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Network_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_RESET:	/* Reset to factory default */
	if (codeNotify == BN_CLICKED) return Network_FactoryReset(hdlg);
	break;

    case IDC_LOGONAUTO:
	if (codeNotify == BN_CLICKED) Network_SetDirty(hdlg);
	break;

    case IDC_LOGONUSER:
    case IDC_LOGONPASS:
	if (codeNotify == EN_CHANGE) Network_SetDirty(hdlg);
	break;

    }
    return 0;
}

/*****************************************************************************
 *
 *  Network_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Network_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Network_Apply(hdlg);
	break;

    }
    return 0;
}

/*****************************************************************************
 *
 *  Network_OnInitDialog
 *
 *	Initialize the controls.
 *
 *****************************************************************************/

BOOL INLINE
Network_OnInitDialog(HWND hdlg)
{
    return Network_Reset(hdlg);
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
Network_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Network_OnInitDialog(hdlg);

    case WM_COMMAND:
	return Network_OnCommand(hdlg,
			       (int)GET_WM_COMMAND_ID(wParam, lParam),
			       (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
	return Network_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}

/*****************************************************************************
 *
 *  GetClassAtom
 *
 *****************************************************************************/

WORD PASCAL
GetClassAtom(HWND hwnd)
{
    return GetClassWord(hwnd, GCW_ATOM);
}

/*****************************************************************************
 *
 *  Network_FindVictim
 *
 *  Look arund to see if there is a window that meets the following
 *  criteria:
 *
 *	1. Is a dialog box.
 *	2. Contains two edit controls, one of which is password-protected.
 *
 *  If so, then the hwnd list is filled in with the two edit controls.
 *
 *****************************************************************************/

typedef struct AUTOLOGON {
    TCHAR tszUser[ctchDlgItem];
    TCHAR tszPass[ctchDlgItem];
} AUTOLOGON, *PAUTOLOGON;


#define GetWindowClass(hwnd)	GetClassWord(hwnd, GCW_ATOM)

HWND PASCAL
Network_FindVictim(HWND hwndEdit, HWND rghwnd[])
{
    WORD atmEdit = GetClassAtom(hwndEdit);
    HWND hdlg;

    for (hdlg = GetWindow(GetDesktopWindow(), GW_CHILD); hdlg;
	 hdlg = GetWindow(hdlg, GW_HWNDNEXT)) {

	/*
	 *  If we have a dialog box, study it.
	 */
	if (GetClassAtom(hdlg) == 0x8002) {
	    HWND hwnd, hwndUser = 0;
	    for (hwnd = GetWindow(hdlg, GW_CHILD); hwnd;
		 hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

		/*
		 *  We care only about visible non-read-only edit controls.
		 */
		if (GetClassAtom(hwnd) == atmEdit) {
		    LONG ws = GetWindowLong(hwnd, GWL_STYLE);
		    if (!(ws & ES_READONLY) && (ws & WS_VISIBLE)) {
			/*
			 *  If we haven't found a "user name",
			 *  then the first edit we find had better
			 *  not be password-protected.  If it is,
			 *  then we punt, because we're confused.
			 *
			 *  If we have found a "user name", then the
			 *  next edit we find had better be
			 *  password-protected.
			 */
			if (hwndUser == 0) {
			    if (ws & ES_PASSWORD) goto nextdialog;
			    hwndUser = hwnd;
			} else {
			    if (!(ws & ES_PASSWORD)) goto nextdialog;
			    rghwnd[0] = hwndUser;
			    rghwnd[1] = hwnd;
			    return hdlg;
			}
		    }
		}
	    }
	}
    nextdialog:;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Network_ForceString
 *
 *  Force a string into an edit control.  We cannot use SetWindowText
 *  because that doesn't work inter-thread.
 *
 *****************************************************************************/

void PASCAL
Network_ForceString(HWND hwnd, LPCTSTR ptsz)
{
    Edit_SetSel(hwnd, 0, -1);
    FORWARD_WM_CLEAR(hwnd, SendMessage);

    for (; *ptsz && IsWindow(hwnd); ptsz++) {
	SendMessage(hwnd, WM_CHAR, *ptsz, 0L);
    }
}

/*****************************************************************************
 *
 *  Network_Snoop
 *
 *  Look to see if we have a winner.  The shift key suppresses autologon.
 *
 *****************************************************************************/

void PASCAL
Network_Snoop(HWND hwnd)
{
    HWND rghwnd[2];
    HWND hdlg;

    hdlg = Network_FindVictim(hwnd, rghwnd);
    if (hdlg && GetAsyncKeyState(VK_SHIFT) >= 0) {
	PAUTOLOGON pal = (PV)GetWindowLong(hwnd, GWL_USERDATA);
	if (pal) {
	    Network_ForceString(rghwnd[0], pal->tszUser);
	    Network_ForceString(rghwnd[1], pal->tszPass);
	    FORWARD_WM_COMMAND(hdlg, IDOK, GetDlgItem(hdlg, IDOK),
			       BN_CLICKED, PostMessage);
	    PostMessage(hwnd, WM_CLOSE, 0, 0L);
	}
    }
}

/*****************************************************************************
 *
 *  Network_WndProc
 *
 *  Window procedure for our "Keep an eye on the logon process".
 *
 *  When the timer fires, we nuke ourselves, under the assumption that the
 *  network dialog box ain't a-comin' so there's no point a-waitin' fer it.
 *
 *****************************************************************************/

LRESULT CALLBACK
Network_WndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {

    case WM_DESTROY:
	KillTimer(hwnd, 1);
	PostQuitMessage(0);
	break;

    case WM_TIMER:
    case WM_KILLFOCUS:
	Network_Snoop(hwnd);
	PostMessage(hwnd, WM_CLOSE, 0, 0L);
	break;

    }

    return DefWindowProc(hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  TweakLogon
 *
 *  Rundll entry point for automatic logon.  This is run as a system service.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

void EXPORT
TweakLogon(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    AUTOLOGON al;

    hwnd;
    hinst;
    lpszCmdLine;
    nCmdShow;

    GetStrPkl(al.tszUser, cbX(al.tszUser), &c_klDefUser);
    GetStrPkl(al.tszPass, cbX(al.tszPass), &c_klDefPass);

    /*
     *	Null password is okay.  But make sure there's a user and that
     *	the feature has been enabled.  And skip it all if the shift key
     *	is down.
     */
    if (GetIntPkl(0, &c_klAutoLogon) && al.tszUser[0] &&
        GetAsyncKeyState(VK_SHIFT) >= 0) {
	MSG msg;

	/*
	 *  We create our window visible but 0 x 0.
	 *
	 *  We use a dummy edit control because that lets us extract the
	 *  class word for edit controls.
	 *
	 *  The GWL_USERDATA of the control points to the logon strings.
	 *
	 */
	hwnd = CreateWindow(TEXT("edit"), "dummy text", WS_POPUP | WS_VISIBLE,
			    0, 0, 0, 0,
			    hwnd, 0, hinstCur, 0);

	SubclassWindow(hwnd, Network_WndProc);
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)&al);

	Network_Snoop(hwnd);

	/* Thirty seconds */
	SetTimer(hwnd, 1, 30000, 0);
	
	while (GetMessage(&msg, 0, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
}
