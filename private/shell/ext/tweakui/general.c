/*
 * general - Dialog box property sheet for "general ui tweaks"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL const c_klSmooth = { &c_hkCU, c_tszRegPathDesktop, c_tszSmoothScroll };
KL const c_klEngine = { &g_hkCUSMIE, c_tszSearchUrl, 0 };
KL const c_klPaintVersion =
                      { &c_hkCU, c_tszRegPathDesktop, c_tszPaintDesktop };

/* BUGBUG 
Software\Microsoft\Internet Explorer\RestrictUI::Toolbar, ::History
IE\Toolbar\::BackBitmap
 */

const static DWORD CODESEG rgdwHelp[] = {
	IDC_EFFECTGROUP,    IDH_GROUP,
//        IDC_ANIMATE,        IDH_ANIMATE,
//        IDC_SMOOTHSCROLL,   IDH_SMOOTHSCROLL,
//        IDC_BEEP,           IDH_BEEP,

	IDC_FLDGROUP,	    IDH_GROUP,
	IDC_FLDNAMETXT,	    IDH_FOLDERNAME,
	IDC_FLDNAMELIST,    IDH_FOLDERNAME,
	IDC_FLDLOCTXT,	    IDH_FOLDERNAME,
	IDC_FLDLOC,	    IDH_FOLDERNAME,
	IDC_FLDCHG,	    IDH_FOLDERNAME,

	IDC_IE3GROUP,	    IDH_GROUP,
	IDC_IE3TXT,	    IDH_IE3ENGINE,
	IDC_IE3ENGINETXT,   IDH_IE3ENGINE,
	IDC_IE3ENGINE,	    IDH_IE3ENGINE,

	0,		    0,
};

#define MAKEKL(nm) \
    KL const c_kl##nm = { &pcdii->hkCUExplorer, \
			  c_tszUserShellFolders, c_tsz##nm }

MAKEKL(Desktop);
MAKEKL(Programs);
MAKEKL(Personal);
MAKEKL(Favorites);
MAKEKL(Startup);
MAKEKL(Recent);
MAKEKL(SendTo);
MAKEKL(StartMenu);
MAKEKL(Templates);

#undef MAKEKL

/*
 *  HACKHACK - Fake some private CSIDL's
 *             by stealing the various CSIDL_COMMON_* values.
 *
 *  WARNING!  This must agree with the IDS_FOLDER_* goo in
 *            tweakui.h.
 */
KL const c_klProgramFiles = { &g_hkLMSMWCV, 0, c_tszProgramFilesDir };
KL const c_klCommonFiles  = { &g_hkLMSMWCV, 0, c_tszCommonFilesDir };
KL const c_klSourcePath   = { &g_hkLMSMWCV, c_tszSetup, c_tszSourcePath };

#define CSIDL_PROGRAMFILES  CSIDL_COMMON_STARTMENU
#define CSIDL_COMMONFILES   CSIDL_COMMON_PROGRAMS
#define CSIDL_SOURCEPATH    CSIDL_COMMON_STARTUP

typedef struct FOLDERDESC {
    UINT csidl;
    PKL pkl;
} FLDD, *PFLDD;

const FLDD c_rgfldd[] = {
    {	CSIDL_DESKTOPDIRECTORY,	&c_klDesktop },
    {	CSIDL_PROGRAMS,		&c_klPrograms },
    {	CSIDL_PERSONAL,		&c_klPersonal },
    {	CSIDL_FAVORITES,	&c_klFavorites },
    {	CSIDL_STARTUP,		&c_klStartup },
    {	CSIDL_RECENT,		&c_klRecent },
    {	CSIDL_SENDTO,		&c_klSendTo },
    {	CSIDL_STARTMENU,	&c_klStartMenu },
    {	CSIDL_TEMPLATES,	&c_klTemplates },
    {   CSIDL_PROGRAMFILES,     &c_klProgramFiles },
    {   CSIDL_COMMONFILES,      &c_klCommonFiles },
    {   CSIDL_SOURCEPATH,       &c_klSourcePath },
};

#define cfldd	cA(c_rgfldd)

#pragma END_CONST_DATA

typedef struct FOLDERINSTANCE {
    BOOL    fEdited;
    PIDL    pidl;
} FINST, *PFINST;

/*
 * Instanced.  We're a cpl so have only one instance, but I declare
 * all the instance stuff in one place so it's easy to convert this
 * code to multiple-instance if ever we need to.
 */
typedef struct GDII {           /* general_dialog instance info */
    FINST   rgfinst[cfldd];
    TCHAR   tszUrl[1024];	/* Search URL */
} GDII, *PGDII;

GDII gdii;
#define pgdii (&gdii)

#define DestroyCursor(hcur) SafeDestroyIcon((HICON)(hcur))

/*****************************************************************************
 *
 *  General_GetAni
 *
 *	Determine whether minimize animations are enabled.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetAni(LPARAM lParam, LPVOID pvRef)
{
    ANIMATIONINFO anii;
    anii.cbSize = sizeof(anii);
    SystemParametersInfo(SPI_GETANIMATION, sizeof(anii), &anii, 0);
    return anii.iMinAnimate != 0;
}

/*****************************************************************************
 *
 *  General_SetAni
 *
 *	Set the new animation flag.
 *
 *****************************************************************************/

void PASCAL
General_SetAni(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    ANIMATIONINFO anii;
    anii.cbSize = sizeof(anii);
    anii.iMinAnimate = f;
    SystemParametersInfo(SPI_SETANIMATION, sizeof(anii), &anii,
			 SPIF_UPDATEINIFILE);
    if (pvRef) {
        LPBOOL pf = pvRef;
        *pf = TRUE;
    }
}

/*****************************************************************************
 *
 *  General_GetSmooth
 *
 *	Determine whether smooth scrolling is enabled.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetSmooth(LPARAM lParam, LPVOID pvRef)
{
    if (g_fSmoothScroll) {
        return GetDwordPkl(&c_klSmooth, 1) != 0;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetSmooth
 *
 *	Set the new smooth-scroll flag.
 *
 *****************************************************************************/

void PASCAL
General_SetSmooth(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    SetDwordPkl(&c_klSmooth, f);
    if (pvRef) {
        LPBOOL pf = pvRef;
        *pf = TRUE;
    }
}

/*****************************************************************************
 *
 *  General_GetPntVer
 *
 *      Determine whether we should paint the version on the desktop.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetPntVer(LPARAM lParam, LPVOID pvRef)
{
    if (g_fMemphis) {
        return GetIntPkl(0, &c_klPaintVersion) != 0;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetPntVer
 *
 *      Set the PaintVersion flag.
 *
 *****************************************************************************/

void PASCAL
General_SetPntVer(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    SetIntPkl(f, &c_klPaintVersion);
    if (pvRef) {
        LPBOOL pf = pvRef;
        *pf = TRUE;
    }
}

/*****************************************************************************
 *
 *  General_GetSpi
 *
 *      Return the setting of an SPI.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetSpi(LPARAM lParam, LPVOID pvRef)
{
    BOOL f;

    /*
     *  Win95 will crash if you pass SPI's above 0xFFFF because it
     *  truncates the SPI code to a WORD, and then you end up issuing
     *  some random SPI instead of the one you really wanted.
     *
     *  We can detect if we are on Win95 by issuing the 0x10000000
     *  SPI (SPI_GETACTIVEWINDOWTRACKING) and see if it works.
     *  Fortunately, the truncation to a WORD yields 0, which is an
     *  invalid parameter.
     */
    if (HIWORD(lParam) &&
        !SystemParametersInfo(SPI_GETACTIVEWINDOWTRACKING, 0, &f, 0)) {
        return -1;      /* Win95 - don't crash! */
    }

    if (SystemParametersInfo(lParam, 0, &f, 0)) {
        return f;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetSpiW
 *
 *      Set the SPI value in the wParam.  Only SPI_SETBEEP needs this.
 *
 *****************************************************************************/

void
General_SetSpiW(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    SystemParametersInfo(lParam+1, f, NULL, SPIF_UPDATEINIFILE);
    if (pvRef) {
        LPBOOL pf = pvRef;
        *pf = TRUE;
    }
}

/*****************************************************************************
 *
 *  General_SetSpi
 *
 *      Set the SPI value in the lParam.
 *
 *****************************************************************************/

void
General_SetSpi(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    SystemParametersInfo(lParam+1, f, (LPVOID)f, SPIF_UPDATEINIFILE);
    if (pvRef) {
        LPBOOL pf = pvRef;
        *pf = TRUE;
    }
}

#pragma BEGIN_CONST_DATA

/*
 *  Note that this needs to be in sync with the IDS_GENERALEFFECTS
 *  strings.
 *
 *  Note that SPI_GETGRADIENTCAPTIONS is not needed, because the
 *  standard Control Panel.Desktop.Appearance lets you munge the
 *  gradient flag.
 */
CHECKLISTITEM c_rgcliGeneral[] = {
    { General_GetAni,    General_SetAni,    0,  },
    { General_GetSmooth, General_SetSmooth, 0,  },
    { General_GetSpi,    General_SetSpiW,   SPI_GETBEEP },
    { General_GetSpi,    General_SetSpi,    SPI_GETMENUANIMATION },
    { General_GetSpi,    General_SetSpi,    SPI_GETCOMBOBOXANIMATION },
    { General_GetSpi,    General_SetSpi,    SPI_GETLISTBOXSMOOTHSCROLLING },
    { General_GetSpi,    General_SetSpi,    SPI_GETMENUUNDERLINES },
    { General_GetSpi,    General_SetSpi,    SPI_GETACTIVEWNDTRKZORDER },
    { General_GetSpi,    General_SetSpi,    SPI_GETHOTTRACKING },
    { General_GetPntVer, General_SetPntVer, 0,  },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  General_SetDirty
 *
 *	Make a control dirty.
 *
 *****************************************************************************/

void INLINE
General_SetDirty(HWND hdlg)
{
    Common_SetDirty(hdlg);
}

/*****************************************************************************
 *
 *  General_FreePidls
 *
 *****************************************************************************/

void PASCAL
General_FreePidls(HWND hdlg)
{
    UINT i;

    for (i = 0; i < cfldd; i++) {
	pgdii->rgfinst[i].fEdited = 0;
	if (pgdii->rgfinst[i].pidl) {
	    Ole_Free(pgdii->rgfinst[i].pidl);
	    pgdii->rgfinst[i].pidl = 0;
	}
    }
}

/*****************************************************************************
 *
 *  General_GetDlgItemData
 *
 *	Get the current item data from a combo box.
 *
 *****************************************************************************/

int PASCAL
General_GetDlgItemData(HWND hdlg, UINT idc)
{
    HWND hwnd = GetDlgItem(hdlg, idc);
    int iItem = ComboBox_GetCurSel(hwnd);
    return ComboBox_GetItemData(hwnd, iItem);
}


/*****************************************************************************
 *
 *  General_UpdateFolder
 *
 *	Update goo since the combo box changed.
 *
 *****************************************************************************/

void PASCAL
General_UpdateFolder(HWND hdlg)
{
    int icsidl = General_GetDlgItemData(hdlg, IDC_FLDNAMELIST);
    TCHAR tsz[MAX_PATH];

    tsz[0] = TEXT('\0');
    SHGetPathFromIDList(pgdii->rgfinst[icsidl].pidl, tsz);

    SetDlgItemText(hdlg, IDC_FLDLOC, tsz);
}

/*****************************************************************************
 *
 *  General_Engine_OnEditChange
 *
 *	Enable the OK button if the URL contains exactly one %s.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnEditChange(HWND hdlg)
{
    TCHAR tszUrl[cA(pgdii->tszUrl)];
    PTSTR ptsz;
    int cPercentS;
    GetDlgItemText(hdlg, IDC_SEARCHURL, tszUrl, cA(tszUrl));

    /*
     *	All appearances of "%" must be followed by "%" or "s".
     */
    cPercentS = 0;
    ptsz = tszUrl;
    while ((ptsz = ptszStrChr(ptsz, TEXT('%'))) != 0) {
	if (ptsz[1] == TEXT('%')) {
	    ptsz += 2;
	} else if (ptsz[1] == TEXT('s')) {
	    cPercentS++;
	    ptsz += 2;
	} else {
	    cPercentS = 0; break;	/* Percent-mumble */
	}
    }

    EnableWindow(GetDlgItem(hdlg, IDOK), cPercentS == 1);
}

/*****************************************************************************
 *
 *  General_Engine_OnOk
 *
 *	Save the answer.
 *
 *****************************************************************************/

void INLINE
General_Engine_OnOk(HWND hdlg)
{
    GetDlgItemText(hdlg, IDC_SEARCHURL, pgdii->tszUrl, cA(pgdii->tszUrl));
}

/*****************************************************************************
 *
 *  General_Engine_OnCommand
 *
 *	If the edit control changed, update the OK button.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDCANCEL:
	EndDialog(hdlg, 0); break;

    case IDOK:
	General_Engine_OnOk(hdlg);
	EndDialog(hdlg, 1); 
	break;

    case IDC_SEARCHURL:
	if (codeNotify == EN_CHANGE) General_Engine_OnEditChange(hdlg);
	break;
    }
}

/*****************************************************************************
 *
 *  General_Engine_OnInitDialog
 *
 *	Shove the current engine URL in so the user can edit it.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnInitDialog(HWND hdlg)
{
    SetDlgItemTextLimit(hdlg, IDC_SEARCHURL, pgdii->tszUrl, cA(pgdii->tszUrl));
    General_Engine_OnEditChange(hdlg);
}

/*****************************************************************************
 *
 *  General_Engine_DlgProc
 *
 *	Dialog procedure.
 *
 *****************************************************************************/

BOOL EXPORT
General_Engine_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: General_Engine_OnInitDialog(hdlg); break;

    case WM_COMMAND:
	General_Engine_OnCommand(hdlg,
			        (int)GET_WM_COMMAND_ID(wParam, lParam),
			        (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
	break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}


/*****************************************************************************
 *
 *  General_UpdateEngine
 *
 *	If the person selected "Custom", then pop up the customize dialog.
 *
 *	At any rate, put the matching URL into pgdii->tszUrl.
 *
 *****************************************************************************/

void PASCAL
General_UpdateEngine(HWND hdlg)
{
    int ieng = General_GetDlgItemData(hdlg, IDC_IE3ENGINE);

    if (ieng == 0) {
	if (DialogBox(hinstCur, MAKEINTRESOURCE(IDD_SEARCHURL), hdlg,
		      General_Engine_DlgProc)) {
	} else {
	    goto skip;
	}
    } else {
	LoadString(hinstCur, IDS_URL+ieng, pgdii->tszUrl, cA(pgdii->tszUrl));
    }

    General_SetDirty(hdlg);
    skip:;
}

/*****************************************************************************
 *
 *  General_GetFlddPidl
 *
 *      Wrapper around SHGetSpecialFolderLocation that also knows how
 *      to read our hacky values.
 *
 *****************************************************************************/

PIDL PASCAL
General_GetFlddPidl(HWND hdlg, const FLDD *pfldd)
{
    HRESULT hres;
    PIDL pidl;
    TCHAR tszPath[MAX_PATH];

    switch (pfldd->csidl) {
    case CSIDL_PROGRAMFILES:
    case CSIDL_COMMONFILES:
    case CSIDL_SOURCEPATH:
        if (GetStrPkl(tszPath, cbX(tszPath), pfldd->pkl)) {
            pidl = pidlFromPath(psfDesktop, tszPath);
        } else {
            pidl = NULL;
        }
        break;

    default:
        if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg, pfldd->csidl, &pidl))) {
        } else {
            pidl = NULL;
        }
        break;
    }

    return pidl;
}

/*****************************************************************************
 *
 *  General_Reset
 *
 *	Reset all controls to initial values.  This also marks
 *	the control as clean.
 *
 *      Note: This doesn't really work any more.
 *
 *****************************************************************************/

BOOL PASCAL
General_Reset(HWND hdlg)
{
    HWND hwnd;
    UINT i;
    TCHAR tsz[256];

    General_FreePidls(hdlg);

    hwnd = GetDlgItem(hdlg, IDC_FLDNAMELIST);
    ComboBox_ResetContent(hwnd);
    for (i = 0; i < cfldd; i++) {
        pgdii->rgfinst[i].pidl = General_GetFlddPidl(hdlg, &c_rgfldd[i]);
        if (pgdii->rgfinst[i].pidl) {
	    int iItem;
	    LoadString(hinstCur, c_rgfldd[i].csidl + IDS_FOLDER_BASE,
		       tsz, cA(tsz));
	    iItem = ComboBox_AddString(hwnd, tsz);
	    ComboBox_SetItemData(hwnd, iItem, i);
	}
    }
    ComboBox_SetCurSel(hwnd, 0);
    General_UpdateFolder(hdlg);

    GetStrPkl(pgdii->tszUrl, cbX(pgdii->tszUrl), &c_klEngine);
    if (pgdii->tszUrl[0]) {
	hwnd = GetDlgItem(hdlg, IDC_IE3ENGINE);
	ComboBox_ResetContent(hwnd);
	for (i = 0; LoadString(hinstCur, IDS_ENGINE+i, tsz, cA(tsz)); i++) {
	    int iItem = ComboBox_AddString(hwnd, tsz);
	    ComboBox_SetItemData(hwnd, iItem, i);
	    if (i) LoadString(hinstCur, IDS_URL+i, tsz, cA(tsz));
	    if (i == 0 || lstrcmpi(tsz, pgdii->tszUrl) == 0) {
		ComboBox_SetCurSel(hwnd, iItem);
	    }
	}
    } else {
	UINT idc;
	for (idc = IDC_IE3FIRST; idc <= IDC_IE3LAST; idc++) {
	    EnableWindow(GetDlgItem(hdlg, idc), 0);
	}
    }

    Common_SetClean(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *  General_UnexpandEnvironmentString
 *
 *	If the string begins with the value of the environment string,
 *	then change it to said string.
 *
 *	Example:
 *		In: "C:\WINNT\SYSTEM32\FOO.TXT", "%SystemRoot%"
 *		Out: "%SystemRoot%\SYSTEM32\FOO.TXT"
 *
 *****************************************************************************/

BOOL PASCAL
General_UnexpandEnvironmentString(LPTSTR ptsz, LPCTSTR ptszEnv)
{
    TCHAR tszEnv[MAX_PATH];
    DWORD ctch;
    BOOL fRc;

    /*
     *	Note that NT ExpandEnvironmentStrings returns the wrong
     *	value, so we can't rely on it.
     */
    ExpandEnvironmentStrings(ptszEnv, tszEnv, cA(tszEnv));
    ctch = lstrlen(tszEnv);

    /*
     *	Source must be at least as long as the env string for
     *	us to have a chance of succeeding.  This check avoids
     *	accidentally reading past the end of the source.
     */
    if ((DWORD)lstrlen(ptsz) >= ctch) {
	if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
			  tszEnv, ctch, ptsz, ctch) == 2) {
	    int ctchEnv = lstrlen(ptszEnv);
	    /*
	     *	Must use hmemcpy to avoid problems with overlap.
	     */
	    hmemcpy(ptsz + ctchEnv, ptsz + ctch,
		    cbCtch(1 + lstrlen(ptsz + ctch)));
	    hmemcpy(ptsz, ptszEnv, ctchEnv);
	    fRc = 1;
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
 *  General_SetUserShellFolder
 *
 *  Don't use REG_EXPAND_SZ if the shell doesn't support it.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszUserProfile[] = TEXT("%USERPROFILE%");
TCHAR c_tszSystemRoot[] = TEXT("%SystemRoot%");

#pragma END_CONST_DATA

void PASCAL
General_SetUserShellFolder(LPTSTR ptsz, LPCTSTR ptszSubkey)
{
    HKEY hk;
    if (g_fShellSz) {
	if (!General_UnexpandEnvironmentString(ptsz, c_tszUserProfile) &&
	    !General_UnexpandEnvironmentString(ptsz, c_tszSystemRoot)) {
	}
    }

    if (RegCreateKey(pcdii->hkCUExplorer, c_tszUserShellFolders, &hk) == 0) {
	RegSetValueEx(hk, ptszSubkey, 0, g_fShellSz ? REG_EXPAND_SZ
						    : REG_SZ, (LPBYTE)ptsz,
		      cbCtch(1 + lstrlen(ptsz)));
	RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  General_Apply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
General_OnApply(HWND hdlg)
{
    BOOL fSendWinIniChange = 0;
    int i;

    Checklist_OnApply(hdlg, c_rgcliGeneral, &fSendWinIniChange);

    if (fSendWinIniChange) {
	SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		    (LPARAM)(LPCTSTR)c_tszWindows);
    }

    /*
     *	Updating the Folder locations is really annoying, thanks
     *	to NT's roving profiles.
     */
    for (i = 0; i < cfldd; i++) {
	if (pgdii->rgfinst[i].fEdited) {
	    TCHAR tsz[MAX_PATH];
	    TCHAR tszDefault[MAX_PATH];
	    UINT ctch;

	    SHGetPathFromIDList(pgdii->rgfinst[i].pidl, tsz);

	    /*
	     *	Is it already in the default location?
             *
             *  Note that the special gizmos don't have default
             *  locations.
	     */
	    ctch = GetWindowsDirectory(tszDefault, cA(tszDefault));
	    if (tszDefault[ctch - 1] != TEXT('\\')) {
		tszDefault[ctch++] =  TEXT('\\');
	    }

            if (LoadString(hinstCur, c_rgfldd[i].csidl + IDS_DEFAULT_BASE,
                           &tszDefault[ctch], cA(tszDefault) - ctch)) {
                if (lstrcmpi(tszDefault, tsz) == 0) {
                    /*
                     *  In default location.
                     */
                    DelPkl(c_rgfldd[i].pkl);
                } else {
                    /*
                     *  In other location.
                     *
                     *  Note that we cannot use SetStrPkl here, because
                     *  we need to set the value type to REG_EXPAND_SZ.
                     *
                     */
                    General_SetUserShellFolder(tsz, c_rgfldd[i].pkl->ptszSubkey);

                }
                Common_NeedLogoff(hdlg);
            } else {
                SetStrPkl(c_rgfldd[i].pkl, tsz);
            }
	}
    }

    if (pgdii->tszUrl[0]) {
	SetStrPkl(&c_klEngine, pgdii->tszUrl);
    }

    return General_Reset(hdlg);
}

/*****************************************************************************
 *
 *  General_OnChangeFolder_Callback
 *
 *	Start the user at the old location, and don't let the user pick
 *	something that collides with something else.
 *
 *****************************************************************************/

int CALLBACK
General_OnChangeFolder_Callback(HWND hwnd, UINT wm, LPARAM lp, LPARAM lpRef)
{
    int icsidl;
    TCHAR tsz[MAX_PATH];

    switch (wm) {
    case BFFM_INITIALIZED:
	SendMessage(hwnd, BFFM_SETSELECTION, 0, lpRef);
	break;

    case BFFM_SELCHANGED:
	/* Picking nothing is bad */
	if (!lp) goto bad;

	/* Picking yourself is okay; just don't pick somebody else */
	if (ComparePidls((PIDL)lp, (PIDL)lpRef) == 0) goto done;

	for (icsidl = 0; icsidl < cfldd; icsidl++) {
	    if (pgdii->rgfinst[icsidl].pidl &&
		ComparePidls(pgdii->rgfinst[icsidl].pidl, (PIDL)lp) == 0) {
	bad:;
		SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
		goto done;
	    }
	}

	/* Don't allow a removable drive */
	tsz[1] = TEXT('\0');		/* Not a typo */
	SHGetPathFromIDList((PIDL)lp, tsz);

	if (tsz[1] == TEXT(':')) {
	    tsz[3] = TEXT('\0');
	    if (GetDriveType(tsz) == DRIVE_REMOVABLE) {
		goto bad;
	    }
	}

	break;
    }
done:;
    return 0;
}

/*****************************************************************************
 *
 *  General_OnChangeFolder
 *
 *****************************************************************************/

void PASCAL
General_OnChangeFolder(HWND hdlg)
{
    int iItem;
    int icsidl;
    HWND hwnd;
    BROWSEINFO bi;
    PIDL pidl;
    TCHAR tsz[MAX_PATH];
    TCHAR tszTitle[MAX_PATH];
    TCHAR tszName[MAX_PATH];

    hwnd = GetDlgItem(hdlg, IDC_FLDNAMELIST);
    iItem = ComboBox_GetCurSel(hwnd);
    ComboBox_GetLBText(hwnd, iItem, tszName);

    LoadString(hinstCur, IDS_FOLDER_PATTERN, tsz, cA(tsz));
    wsprintf(tszTitle, tsz, tszName);

    bi.hwndOwner = hdlg;
    bi.pidlRoot = 0;
    bi.pszDisplayName = tsz; /* Garbage */
    bi.lpszTitle = tszTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.lpfn = General_OnChangeFolder_Callback;
    icsidl = ComboBox_GetItemData(hwnd, iItem);
    bi.lParam = (LPARAM)pgdii->rgfinst[icsidl].pidl;

    pidl = SHBrowseForFolder(&bi);

    if (pidl) {
	if (ComparePidls(pidl, pgdii->rgfinst[icsidl].pidl) != 0) {
	    Ole_Free(pgdii->rgfinst[icsidl].pidl);
	    pgdii->rgfinst[icsidl].pidl = pidl;
	    pgdii->rgfinst[icsidl].fEdited = 1;
	    General_SetDirty(hdlg);
	    General_UpdateFolder(hdlg);
	} else {
	    Ole_Free(pidl);
	}
    }
}

/*****************************************************************************
 *
 *  General_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
General_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_ANIMATE:
    case IDC_SMOOTHSCROLL:
    case IDC_BEEP:
	if (codeNotify == BN_CLICKED) General_SetDirty(hdlg);
	break;

    case IDC_FLDNAMELIST:
	if (codeNotify == CBN_SELCHANGE) General_UpdateFolder(hdlg);
	break;

    case IDC_IE3ENGINE:
	if (codeNotify == CBN_SELCHANGE) General_UpdateEngine(hdlg);

    case IDC_FLDCHG:
	if (codeNotify == BN_CLICKED) General_OnChangeFolder(hdlg);
	break;

    }
    return 0;
}

#if 0
/*****************************************************************************
 *
 *  General_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
General_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	General_Apply(hdlg);
	break;

    }
    return 0;
}
#endif

/*****************************************************************************
 *
 *  General_OnInitDialog
 *
 *	Initialize the controls.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
General_OnInitDialog(HWND hwnd)
{
    HWND hdlg = GetParent(hwnd);

    ZeroMemory(pgdii, cbX(*pgdii));

    Checklist_OnInitDialog(hwnd, c_rgcliGeneral, cA(c_rgcliGeneral),
                           IDS_GENERALEFFECTS, 0);

    General_Reset(hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  General_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
General_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP,
            IDH_ANIMATE + lvi.lParam);
}

#if 0
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
General_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return General_OnInitDialog(hdlg);

    case WM_COMMAND:
	return General_OnCommand(hdlg,
			       (int)GET_WM_COMMAND_ID(wParam, lParam),
			       (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
	return General_OnNotify(hdlg, (NMHDR FAR *)lParam);

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

LVV lvvGeneral = {
    General_OnCommand,
    0,                          /* General_OnInitContextMenu */
    0,                          /* General_Dirtify */
    0,                          /* General_GetIcon */
    General_OnInitDialog,
    General_OnApply,
    0,                          /* General_OnDestroy */
    0,                          /* General_OnSelChange */
    6,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    {
        { IDC_WHATSTHIS,        General_OnWhatsThis },
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
General_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvGeneral, hdlg, wm, wParam, lParam);
}
