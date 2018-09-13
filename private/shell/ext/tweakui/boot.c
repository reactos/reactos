/*
 * boot - Dialog box property sheet for "boot-time parameters"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#ifdef BOOTMENUDEFAULT
ConstString(c_tszNetwork,	"Network");
ConstString(c_tszBootMenuDefault, "BootMenuDefault");
#endif

/* BUGBUG - use a dispatch table already */
typedef BYTE MSIOT;		/* msdos.sys ini option type */
#define msiotUint		0
#define msiotBool		1
#define msiotCombo		2

typedef const struct MSIO {	/* msdos.sys ini option */
    const TCH CODESEG *ptszName;
    WORD id;			/* Dialog id */
    MSIOT msiot;		/* Data type */
    BYTE uiDefault;		/* The default value */
} MSIO;
typedef MSIO CODESEG *PMSIO;

MSIO CODESEG rgmsio[] = {
    { c_tszBootKeys,	 IDC_BOOTKEYS,		msiotBool,  1 },
    { c_tszBootDelay,	 IDC_BOOTDELAY, 	msiotUint,  2 },
    { c_tszBootGUI,	 IDC_BOOTGUI,		msiotBool,  1 },
    { c_tszBootMenu,	 IDC_BOOTMENU,		msiotBool,  0 },
    { c_tszBootMenuDelay,IDC_BOOTMENUDELAY,	msiotUint, 30 },
    { c_tszLogo,	 IDC_LOGO,		msiotBool,  1 },
    { c_tszBootMulti,	 IDC_BOOTMULTI, 	msiotBool,  0 },
    { c_tszAutoScan,	 IDC_SCANDISK,		msiotCombo, 1 },
};

#define pmsioMax (&rgmsio[cA(rgmsio)])

const static DWORD CODESEG rgdwHelp[] = {
	IDC_BOOTGROUP1,		IDH_GROUP,
	IDC_BOOTKEYS,		IDH_BOOTKEYS,
	IDC_BOOTDELAYTEXT,	IDH_BOOTKEYS,
	IDC_BOOTDELAY,		IDH_BOOTKEYS,
	IDC_BOOTDELAYUD,	IDH_BOOTKEYS,
	IDC_BOOTDELAYTEXT2,	IDH_BOOTKEYS,
	IDC_BOOTGUI,		IDH_BOOTGUI,
	IDC_LOGO,		IDH_LOGO,
	IDC_BOOTMULTI,		IDH_BOOTMULTI,

	IDC_BOOTMENUGROUP,	IDH_GROUP,
	IDC_BOOTMENU,		IDH_BOOTMENU,
	IDC_BOOTMENUDELAYTEXT,	IDH_BOOTMENUDELAY,
	IDC_BOOTMENUDELAY,	IDH_BOOTMENUDELAY,
	IDC_BOOTMENUDELAYUD,	IDH_BOOTMENUDELAY,
	IDC_BOOTMENUDELAYTEXT2,	IDH_BOOTMENUDELAY,
	IDC_SCANDISKTEXT,	IDH_AUTOSCAN,
	IDC_SCANDISK,		IDH_AUTOSCAN,
	IDC_RESET,		IDH_RESET,
	0,			0,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Boot_fLogo
 *
 *	Nonzer if this machine should have the logo enabled by default.
 *
 *	The answer is yes, unless the display driver is xga.drv,
 *	because XGA cards aren't really VGA compatible, and the logo
 *	code uses VGA mode X.
 *
 *****************************************************************************/

BOOL PASCAL
Boot_fLogo(void)
{
    TCH tsz[12];

    return !(GetPrivateProfileString(c_tszBoot, c_tszDisplayDrv, c_tszNil,
				     tsz, cA(tsz), c_tszSysIni) &&
		lstrcmpi(tsz, c_tszXgaDrv) == 0);
}

/*****************************************************************************
 *
 *  Boot_GetOption
 *
 *****************************************************************************/

int PASCAL
Boot_GetOption(LPCTSTR ptszName, UINT uiDefault)
{
    return GetPrivateProfileInt(c_tszOptions, ptszName,
			        uiDefault, g_tszMsdosSys);
}

#ifdef BOOTMENUDEFAULT
/*****************************************************************************
 *
 *  Boot_GetDefaultBootMenuDefault
 *
 *	Get the default boot menu default.  This is 3 if no network, 
 *	or 4 if network.
 *
 *****************************************************************************/

UINT PASCAL
Boot_GetDefaultBootMenuDefault(void)
{
    return 3 + Boot_GetOption(c_tszNetwork, 0);
}

/*****************************************************************************
 *
 *  Boot_GetBootMenuDefault
 *
 *	Get the boot menu default.
 *
 *****************************************************************************/

int PASCAL
Boot_GetBootMenuDefault(void)
{
    return Boot_GetOption(c_tszBootMenuDefault,
			  Boot_GetDefaultBootMenuDefault());
}
#endif

/*****************************************************************************
 *
 *  Boot_FindMsdosSys
 *
 *	Search the hard drives for the file X:\MSDOS.SYS.
 *
 *	There is no "Get boot drive" function in Win32, so this is the best
 *	I can do.
 *
 *	(MS-DOS function 3305h returns the boot drive.)
 *
 *****************************************************************************/

void PASCAL
Boot_FindMsdosSys(void)
{
    TCH tsz[2];			/* scratch */
    char szRoot[4];		/* Root directory thing */
    (*(LPDWORD)szRoot) = 0x005C3A40; /* @:\ */

    for (szRoot[0] = 'A'; szRoot[0] <= 'Z'; szRoot[0]++) {
	if (GetDriveTypeA(szRoot) == DRIVE_FIXED) {
	    DWORD fl;
	    if (GetVolumeInformation(szRoot, 0, 0, 0, 0, &fl, 0, 0) &&
		!(fl & FS_VOL_IS_COMPRESSED)) {
		g_tszMsdosSys[0] = (TCH)szRoot[0];
		if (GetPrivateProfileString(c_tszPaths, c_tszWinDir, 0,
					    tsz, cA(tsz), g_tszMsdosSys)) {
		    return;
		}
	    }
	}
    }
    g_tszMsdosSys[0] = TEXT('\0');
}

/*****************************************************************************
 *
 *  Boot_WriteOptionUint
 *
 *	Write an option unsigned integer to the msdos.sys file.
 *
 *****************************************************************************/

void PASCAL
Boot_WriteOptionUint(LPCTSTR ptszName, UINT ui, UINT uiDefault)
{
    if (GetPrivateProfileInt(c_tszOptions, ptszName,
			     uiDefault, g_tszMsdosSys) != ui) {
	TCH tsz[32];
	wsprintf(tsz, c_tszPercentU, ui);
	WritePrivateProfileString(c_tszOptions, ptszName, tsz, g_tszMsdosSys);
OutputDebugString(ptszName);
OutputDebugString(tsz);
    }
}

/*****************************************************************************
 *
 *  Boot_SetOptionBool
 *
 *	Propagate an option boolean to the msdos.sys file.
 *
 *****************************************************************************/

void PASCAL
Boot_SetOptionBool(HWND hdlg, PMSIO pmsio)
{
    Boot_WriteOptionUint(pmsio->ptszName, IsDlgButtonChecked(hdlg, pmsio->id),
			 pmsio->uiDefault);
}

/*****************************************************************************
 *
 *  Boot_SetOptionUint
 *
 *	Propagate an option unsigned integer to the msdos.sys file.
 *
 *****************************************************************************/

void PASCAL
Boot_SetOptionUint(HWND hdlg, PMSIO pmsio)
{
    UINT ui;
    BOOL f;

    ui = (int)GetDlgItemInt(hdlg, pmsio->id, &f, 0);
    if (f) {
        Boot_WriteOptionUint(pmsio->ptszName, ui, pmsio->uiDefault);
    }

}

/*****************************************************************************
 *
 *  Boot_SetOptionCombo
 *
 *	Propagate a combo option to the msdos.sys file.
 *
 *	Not propagating the value that is already there means that we don't
 *	set AutoScan if not running OPK2.
 *
 *****************************************************************************/

void PASCAL
Boot_SetOptionCombo(HWND hdlg, PMSIO pmsio)
{
    Boot_WriteOptionUint(pmsio->ptszName,
			 SendDlgItemMessage(hdlg, pmsio->id,
					    CB_GETCURSEL, 0, 0),
			 pmsio->uiDefault);
}


/*****************************************************************************
 *
 *  Boot_FlushIniCache
 *
 *  Make sure all changes are committed to disk.
 *
 *****************************************************************************/

INLINE void
Boot_FlushIniCache(void)
{
    WritePrivateProfileString(0, 0, 0, g_tszMsdosSys);
}

/*****************************************************************************
 *
 *  Boot_Apply
 *
 *	Write the changes to the msdos.sys file.
 *
 *****************************************************************************/

BOOL PASCAL
Boot_Apply(HWND hdlg)
{
    DWORD dwAttr = GetFileAttributes(g_tszMsdosSys);
    if (dwAttr != 0xFFFFFFFF &&
	SetFileAttributes(g_tszMsdosSys, FILE_ATTRIBUTE_NORMAL)) {
	PMSIO pmsio;
	for (pmsio = rgmsio; pmsio < pmsioMax; pmsio++) {
            HWND hwnd = GetDlgItem(hdlg, pmsio->id);
            if (hwnd) {
                switch (pmsio->msiot) {
                case msiotUint:
                    Boot_SetOptionUint(hdlg, pmsio);
                    break;

                case msiotBool:
                    Boot_SetOptionBool(hdlg, pmsio);
                    break;

                case msiotCombo:
                    Boot_SetOptionCombo(hdlg, pmsio);
                    break;
                }
            }
	}
#ifdef BOOTMENUDEFAULT
	Boot_WriteOptionUint(c_tszBootMenuDefault,
	    (UINT)SendDlgItemMessage(hdlg, IDC_BOOTMENUDEFAULT, CB_GETCURSEL,
				     0, 0L) + 1);
#endif
	Boot_FlushIniCache();
	SetFileAttributes(g_tszMsdosSys, dwAttr);
    } else {
	MessageBoxId(hdlg, IDS_ERRMSDOSSYS, g_tszName, MB_OK);
    }
    return 1;
}

/*****************************************************************************
 *
 *  Boot_OnBootKeysChange
 *
 *	When IDC_BOOTKEYS changes, enable or disable the boot delay,
 *	BootMulti, and BootMenu.
 *
 *	BUGBUG -- Bootkeys and bootmenu are mutually exclusive.
 *
 *****************************************************************************/

void PASCAL
Boot_OnBootKeysChange(HWND hdlg)
{
    BOOL f = IsDlgButtonChecked(hdlg, IDC_BOOTKEYS);
    HWND hwnd;

    hwnd = GetDlgItem(hdlg, IDC_BOOTDELAY);
    if (hwnd) {
        EnableWindow(hwnd, f);
    }
    EnableDlgItem(hdlg, IDC_BOOTMULTI, f);
    EnableDlgItem(hdlg, IDC_BOOTMENU, f);
    EnableDlgItem(hdlg, IDC_BOOTMENUDELAY, f);
}

/*****************************************************************************
 *
 *  Boot_SetDlgOption
 *
 *	Set the value of a dialog box item.
 *
 *****************************************************************************/

void PASCAL
Boot_SetDlgOption(HWND hdlg, PMSIO pmsio, UINT ui)
{
    HWND hwnd = GetDlgItem(hdlg, pmsio->id);
    if (hwnd) {
        switch (pmsio->msiot) {
        case msiotUint:
            SetDlgItemInt(hdlg, pmsio->id, ui, 0);
            break;

        case msiotBool:
            CheckDlgButton(hdlg, pmsio->id, ui);
            break;

        case msiotCombo:
            ComboBox_SetCurSel(hwnd, ui);
        }
    }
}

/*****************************************************************************
 *
 *  Boot_FactoryReset
 *
 *	Restore to original factory settings.
 *
 *	The weird one is IDC_BOOTLOGO, which
 *	varies depending on the system configuration.
 *
 *****************************************************************************/

BOOL PASCAL
Boot_FactoryReset(HWND hdlg)
{
    PMSIO pmsio;
    for (pmsio = rgmsio; pmsio < pmsioMax; pmsio++) {
	Boot_SetDlgOption(hdlg, pmsio, pmsio->uiDefault);
    }

    CheckDlgButton(hdlg, IDC_LOGO, Boot_fLogo());

#ifdef BOOTMENUDEFAULT
    SendDlgItemMessage(hdlg, IDC_BOOTMENUDEFAULT, CB_SETCURSEL,
		       (WPARAM)Boot_GetDefaultBootMenuDefault() - 1, 0L);
#endif

    Boot_OnBootKeysChange(hdlg);

    Common_SetDirty(hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  Boot_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Boot_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_RESET:	/* Reset to factory default */
	if (codeNotify == BN_CLICKED) return Boot_FactoryReset(hdlg);
	break;

    case IDC_BOOTKEYS:
	if (codeNotify == BN_CLICKED) Boot_OnBootKeysChange(hdlg);
	/* FALLTHROUGH */

    case IDC_BOOTGUI:
    case IDC_BOOTMENU:
    case IDC_LOGO:
    case IDC_BOOTMULTI:
	if (codeNotify == BN_CLICKED) Common_SetDirty(hdlg);
	break;

    case IDC_BOOTDELAY:
    case IDC_BOOTMENUDELAY:
	if (codeNotify == EN_CHANGE) Common_SetDirty(hdlg);
	break;

#ifdef BOOTMENUDEFAULT
    case IDC_BOOTMENUDEFAULT:
	if (codeNotify == CBN_SELCHANGE) Common_SetDirty(hdlg);
	break;
#endif

    case IDC_SCANDISK:
	if (codeNotify == CBN_SELCHANGE) Common_SetDirty(hdlg);
	break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Boot_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Boot_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Boot_Apply(hdlg);
	break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Boot_InitDlgInt
 *
 *	Initialize a paired edit control / updown control.
 *
 *	hdlg is the dialog box itself.
 *
 *	idc is the edit control identifier.  It is assumed that idc+didcUd is
 *	the identifier for the updown control.
 *
 *	iMin and iMax are the limits of the control.
 *
 *****************************************************************************/

void PASCAL
Boot_InitDlgInt(HWND hdlg, UINT idc, int iMin, int iMax)
{
    HWND hwnd = GetDlgItem(hdlg, idc + didcEdit);
    if (hwnd) {
        Edit_LimitText(hwnd, 2);
        SendDlgItemMessage(hdlg, idc+didcUd,
                           UDM_SETRANGE, 0, MAKELPARAM(iMax, iMin));
    }
}

/*****************************************************************************
 *
 *  Boot_OnInitDialog
 *
 *	Initialize the controls.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
Boot_OnInitDialog(HWND hdlg)
{
    PMSIO pmsio;
    HWND hwnd;
    UINT dids;
    TCH tsz[96];

    /*
     *	Init the Scandisk gizmo.  We need to do this even if not OPK2,
     *	so that we don't freak out on the Apply.  But show it only if OPK2.
     */
    hwnd = GetDlgItem(hdlg, IDC_SCANDISK);
    for (dids = 0; dids < 3; dids++) {
	LoadString(hinstCur, IDS_SCANDISKFIRST + dids, tsz, cA(tsz));
	ComboBox_AddString(hwnd, tsz);
    }
    if (g_fOPK2) {
	ShowWindow(hwnd, 1);
	ShowWindow(GetDlgItem(hdlg, IDC_SCANDISKTEXT), 1);
    }

    for (pmsio = rgmsio; pmsio < pmsioMax; pmsio++) {
	Boot_SetDlgOption(hdlg, pmsio,
			  Boot_GetOption(pmsio->ptszName, pmsio->uiDefault));
    }
    
#ifdef BOOTMENUDEFAULT
    hwnd = GetDlgItem(hdlg, IDC_BOOTMENUDEFAULT);
    for (ids = IDS_BOOTMENU; ids <= IDS_BOOTMENULAST; ids++) {
	if (ids != IDS_BOOTMENUSAFENET || Boot_GetOption(c_tszNetwork, 0)) {
	    LoadString(hinstCur, ids, tsz, cA(tsz));
	    ComboBox_AddString(hwnd, tsz);
	}
    }
    ComboBox_SetCurSel(hwnd, Boot_GetBootMenuDefault() - 1);
#endif

    Boot_InitDlgInt(hdlg, IDC_BOOTDELAY, 0, 99);
    Boot_InitDlgInt(hdlg, IDC_BOOTMENUDELAY, 0, 99);
    Boot_OnBootKeysChange(hdlg);

    if (g_fMemphis) {
        DestroyWindow(GetDlgItem(hdlg, IDC_BOOTDELAYTEXT));
        DestroyWindow(GetDlgItem(hdlg, IDC_BOOTDELAY));
        DestroyWindow(GetDlgItem(hdlg, IDC_BOOTDELAYUD));
        DestroyWindow(GetDlgItem(hdlg, IDC_BOOTDELAYTEXT2));
    }

    return 1;
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
Boot_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Boot_OnInitDialog(hdlg);

    case WM_COMMAND:
	return Boot_OnCommand(hdlg,
			       (int)GET_WM_COMMAND_ID(wParam, lParam),
			       (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
	return Boot_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
