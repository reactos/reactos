/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFPRG.C
 *  User interface dialogs for GROUP_PRG
 *
 *  History:
 *  Created 04-Jan-1993 1:10pm by Jeff Parsons
 */

#include "shellprv.h"
#pragma hdrstop

#ifdef WIN32
#define hModule GetModuleHandle(TEXT("SHELL32.DLL"))
#endif




extern const TCHAR szDefIconFile[];


BINF abinfPrg[] = {
    {IDC_CLOSEONEXIT,   BITNUM(PRG_CLOSEONEXIT)},
};

#ifndef WINNT

BINF abinfAdvPrg[] = {
    {IDC_SUGGESTMSDOS,  BITNUM(PRG_NOSUGGESTMSDOS) | 0x80},
};

BINF abinfAdvPrgInit[] = {
    {IDC_WINLIE,        BITNUM(PRGINIT_WINLIE)},
    {IDC_REALMODE,      BITNUM(PRGINIT_REALMODE)},
    {IDC_WARNMSDOS,     BITNUM(PRGINIT_REALMODESILENT) | 0x80},
//    {IDC_QUICKSTART,    BITNUM(PRGINIT_QUICKSTART)},
};

#endif

//  Per-Dialog data

typedef struct PRGINFO {     /* pi */
    PPROPLINK ppl;
    HICON     hIcon;
    TCHAR     atchIconFile[PIFDEFFILESIZE];
    WORD      wIconIndex;
    LPVOID    hConfig;
    LPVOID    hAutoexec;
    WORD      flPrgInitPrev;
    BOOL      fCfgSetByWiz;
} PRGINFO;
typedef PRGINFO * PPRGINFO;     /* ppi */


//  Private function prototypes

void            InitPrgDlg(HWND hDlg, PPRGINFO ppi);
void            AdjustMSDOSModeControls(PPROPLINK ppl, HWND hDlg);
void            ApplyPrgDlg(HWND hDlg, PPRGINFO ppi);
void            BrowseIcons(HWND hDlg, PPRGINFO ppi);

#ifdef WINNT
BOOL_PTR CALLBACK   DlgPifNtProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
WORD            InitNtPifDlg(HWND hDlg, register PPRGINFO ppi);
void            ApplyNtPifDlg( HWND hDlg, PPRGINFO ppi );
#else
BOOL_PTR CALLBACK   DlgAdvPrgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
WORD            InitAdvPrgDlg(HWND hDlg, PPRGINFO ppi);
void            ApplyAdvPrgDlg(HWND hDlg, PPRGINFO ppi);
HLOCAL          GetAdvPrgData(HWND hDlg, PPROPLINK ppl, int cbMax, int idCtrl, LPCSTR lpszName);
void            SetAdvPrgData(HWND hDlg, PPROPLINK ppl, HLOCAL hData, int idCtrl, LPCSTR lpszName);
void            AdjustAdvControls(PPROPLINK ppl, HWND hDlg);
BOOL            IsCleanCfgEmpty(HWND hDlg);
void            SetWizProps(HWND hDlg, PPRGINFO ppi, UINT uWizAction);
#endif





// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
    IDC_ICONBMP,        IDH_DOS_PROGRAM_ICON,
    IDC_TITLE,          IDH_DOS_PROGRAM_DESCRIPTION,
    IDC_CMDLINE,        IDH_DOS_PROGRAM_CMD_LINE,
    IDC_CMDLINELBL,     IDH_DOS_PROGRAM_CMD_LINE,
    IDC_WORKDIR,        IDH_DOS_PROGRAM_WORKDIR,
    IDC_WORKDIRLBL,     IDH_DOS_PROGRAM_WORKDIR,
    IDC_HOTKEY,         IDH_DOS_PROGRAM_SHORTCUT,
    IDC_HOTKEYLBL,      IDH_DOS_PROGRAM_SHORTCUT,
    IDC_BATCHFILE,      IDH_DOS_PROGRAM_BATCH,
    IDC_BATCHFILELBL,   IDH_DOS_PROGRAM_BATCH,
    IDC_WINDOWSTATE,    IDH_DOS_PROGRAM_RUN,
    IDC_WINDOWSTATELBL, IDH_DOS_PROGRAM_RUN,
    IDC_CLOSEONEXIT,    IDH_DOS_WINDOWS_QUIT_CLOSE,
    IDC_CHANGEICON,     IDH_DOS_PROGRAM_CHANGEICON,
    IDC_ADVPROG,        IDH_DOS_PROGRAM_ADV_BUTTON,
    0, 0
};

#ifdef WINNT

const static DWORD rgdwNTHelp[] = {
    IDC_DOS,            IDH_COMM_GROUPBOX,
    10,                 IDH_DOS_ADV_AUTOEXEC,
    11,                 IDH_DOS_ADV_CONFIG,
    IDC_NTTIMER,        IDH_DOS_PROGRAM_PIF_TIMER_EMULATE,
    0, 0
};

#else

// BUGBUG: need new IDH for IDC_WARNMSDOS,
// and need to remove IDH_DOS_ADV_DIRECTDISK and IDH_DOS_ADV_QUICK_START -JTP

const static DWORD rgdwHelpAdvPrg[] = {
    IDC_PIFNAMELBL,     IDH_DOS_ADV_PIFNAME,
    IDC_PIFNAME,        IDH_DOS_ADV_PIFNAME,
    IDC_REALMODE,       IDH_DOS_TASKING_SINGLE,
    IDC_WARNMSDOS,      IDH_NOMSDOSWARNING,     // BUGBUG here
    IDC_CONFIGLBL,      IDH_DOS_ADV_CONFIG,
    IDC_CONFIG,         IDH_DOS_ADV_CONFIG,
    IDC_AUTOEXECLBL,    IDH_DOS_ADV_AUTOEXEC,
    IDC_AUTOEXEC,       IDH_DOS_ADV_AUTOEXEC,
    IDC_WINLIE,         IDH_DOS_ADV_HIDEWINDOWS,
    IDC_SUGGESTMSDOS,   IDH_DOS_ADV_PRG_SUGGEST,
    IDC_REALMODEWIZARD, IDH_DOS_ADV_CONFIG_BTN,
    IDC_CLEANCONFIG,    IDH_DOS_ADV_CLEANCFG,
    IDC_CURCONFIG,      IDH_DOS_ADV_CURCFG,
    IDC_OK,             IDH_OK,
    IDC_CANCEL,         IDH_CANCEL,
    0, 0
};

#endif

BOOL MustRebootSystem(void)
{
    HKEY hk;
    BOOL bMustReboot = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SHUTDOWN, &hk) == ERROR_SUCCESS) {
        bMustReboot = (SHQueryValueEx(hk, REGSTR_VAL_FORCEREBOOT, NULL,
                                       NULL, NULL, NULL) == ERROR_SUCCESS);
        RegCloseKey(hk);
    }
    return(bMustReboot);
}


DWORD GetMSDOSOptGlobalFlags(void)
{
    HKEY hk;
    DWORD dwDosOptGlobalFlags = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_MSDOSOPTS, &hk) == ERROR_SUCCESS) {
        DWORD cb = SIZEOF(dwDosOptGlobalFlags);
        if (SHQueryValueEx(hk, REGSTR_VAL_DOSOPTGLOBALFLAGS, NULL, NULL,
                            (LPVOID)(&dwDosOptGlobalFlags), &cb)
                            != ERROR_SUCCESS) {
            dwDosOptGlobalFlags = 0;
        }
        RegCloseKey(hk);
    }
    return(dwDosOptGlobalFlags);
}



BOOL_PTR CALLBACK DlgPrgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRGINFO ppi = (PPRGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {

    case WM_INITDIALOG:
        // allocate dialog instance data
        if (NULL != (ppi = (PPRGINFO)LocalAlloc(LPTR, SIZEOF(PRGINFO)))) {
            ppi->ppl = (PPROPLINK)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)ppi);

            SHAutoComplete(GetDlgItem(hDlg, IDC_CMDLINE), 0);
            SHAutoComplete(GetDlgItem(hDlg, IDC_WORKDIR), 0);
            SHAutoComplete(GetDlgItem(hDlg, IDC_BATCHFILE), 0);
            InitPrgDlg(hDlg, ppi);
        } else {
            EndDialog(hDlg, FALSE);     // fail the dialog create
        }
        break;

    case WM_DESTROY:
        // free the ppi
        if (ppi) {
            EVAL(LocalFree(ppi) == NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
        }
        break;

    HELP_CASES(rgdwHelp)                // handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

        case IDC_TITLE:
        case IDC_CMDLINE:
        case IDC_WORKDIR:
        case IDC_BATCHFILE:
        case IDC_HOTKEY:
            if (HIWORD(wParam) == EN_CHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_WINDOWSTATE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_CLOSEONEXIT:
            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_ADVPROG:
            if (HIWORD(wParam) == BN_CLICKED) {
#ifdef WINNT
                DialogBoxParam(hModule,
                               MAKEINTRESOURCE(IDD_PIFNTTEMPLT),
                               hDlg,
                               DlgPifNtProc,
                               (LPARAM)ppi);
#else
                DialogBoxParam(hModule,
                               MAKEINTRESOURCE(IDD_ADVPROG),
                               hDlg,
                               DlgAdvPrgProc,
                               (LPARAM)ppi);
                AdjustMSDOSModeControls(ppi->ppl, hDlg);
#endif

            }
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDC_CHANGEICON:
            if (HIWORD(wParam) == BN_CLICKED)
                BrowseIcons(hDlg, ppi);
            return FALSE;               // return 0 if we process WM_COMMAND
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyPrgDlg(hDlg, ppi);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


void InitPrgDlg(HWND hDlg, register PPRGINFO ppi)
{
    int i;
    PROPPRG prg;
    PROPENV env;
#ifdef UNICODE
    PROPNT40 nt40;
#endif
    PPROPLINK ppl = ppi->ppl;
    TCHAR szBuf[MAX_STRING_SIZE];
    FunctionName(InitPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                              &prg, SIZEOF(prg), GETPROPS_NONE
                             ) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_ENV),
                              &env, SIZEOF(env), GETPROPS_NONE
                             )
#ifdef UNICODE
                               ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40),
                              &nt40, SIZEOF(nt40), GETPROPS_NONE
                             )
#endif

       ) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    /*
     * Initialize Icon and IconFile information
     *
     */

    ppi->wIconIndex = prg.wIconIndex;

#ifdef UNICODE
    lstrcpyW(ppi->atchIconFile, nt40.awchIconFile);
    if (NULL != (ppi->hIcon = LoadPIFIcon(&prg, &nt40))) {
#else
    lstrcpyA(ppi->atchIconFile, prg.achIconFile);
    if (NULL != (ppi->hIcon = LoadPIFIcon(&prg))) {
#endif
        VERIFYFALSE(SendDlgItemMessage(hDlg, IDC_ICONBMP, STM_SETICON, (WPARAM)ppi->hIcon, 0));
    }


    /*
     * Initialize window Title information
     *
     */

    LimitDlgItemText(hDlg, IDC_TITLE, ARRAYSIZE(prg.achTitle)-1);
#ifdef UNICODE
    SetDlgItemTextW(hDlg, IDC_TITLE, nt40.awchTitle);
#else
    SetDlgItemTextA(hDlg, IDC_TITLE, prg.achTitle);
#endif

    /*
     * Initialize command line information
     *
     */

    LimitDlgItemText(hDlg, IDC_CMDLINE, ARRAYSIZE(prg.achCmdLine)-1);
#ifdef UNICODE
    SetDlgItemTextW(hDlg, IDC_CMDLINE, nt40.awchCmdLine);
#else
    SetDlgItemTextA(hDlg, IDC_CMDLINE, prg.achCmdLine);
#endif

    /*
     * Initialize command line information
     *
     */

    LimitDlgItemText(hDlg, IDC_WORKDIR, ARRAYSIZE(prg.achWorkDir)-1);
#ifdef UNICODE
    SetDlgItemTextW(hDlg, IDC_WORKDIR, nt40.awchWorkDir);
#else
    SetDlgItemTextA(hDlg, IDC_WORKDIR, prg.achWorkDir);
#endif

    /*
     *  Require at least one of Ctrl, Alt or Shift to be pressed.
     *  The hotkey control does not enforce the rule on function keys
     *  and other specials, which is good.
     */
    SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_SETRULES, HKCOMB_NONE, HOTKEYF_CONTROL | HOTKEYF_ALT);
    SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_SETHOTKEY, prg.wHotKey, 0);

    /*
     * Initialize batch file information
     *
     */

    LimitDlgItemText(hDlg, IDC_BATCHFILE, ARRAYSIZE(env.achBatchFile)-1);
#ifdef UNICODE
    SetDlgItemTextW(hDlg, IDC_BATCHFILE, nt40.awchBatchFile);
#else
    SetDlgItemTextA(hDlg, IDC_BATCHFILE, env.achBatchFile);
#endif

    /*
     *  Fill in the "Run" combo box.
     */
    for (i=0; i < 3; i++) {
        VERIFYTRUE(LoadString(hModule, IDS_NORMALWINDOW+i, szBuf, ARRAYSIZE(szBuf)));
        VERIFYTRUE((int)SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == i);
    }
    i = 0;
    if (prg.flPrgInit & PRGINIT_MINIMIZED)
        i = 1;
    if (prg.flPrgInit & PRGINIT_MAXIMIZED)
        i = 2;
    SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_SETCURSEL, i, 0);

    SetDlgBits(hDlg, &abinfPrg[0], ARRAYSIZE(abinfPrg), prg.flPrg);

    AdjustMSDOSModeControls(ppl, hDlg);
}


void AdjustMSDOSModeControls(PPROPLINK ppl, HWND hDlg)
{
    int i;
    BOOL f = TRUE;

    AdjustRealModeControls(ppl, hDlg);

    /*
     *  The working directory and startup batch file controls are only
     *  supported in real-mode if there is a private configuration (only
     *  because it's more work).  So, disable the controls appropriately.
     */
    if (ppl->flProp & PROP_REALMODE) {
        f = (PifMgr_GetProperties(ppl, szCONFIGHDRSIG40, NULL, 0, GETPROPS_NONE) != 0 ||
             PifMgr_GetProperties(ppl, szAUTOEXECHDRSIG40, NULL, 0, GETPROPS_NONE) != 0);
    }
    #if (IDC_WORKDIRLBL != IDC_WORKDIR-1)
    #error Error in IDC constants: IDC_WORKDIRLBL != IDC_WORKDIR-1
    #endif

    #if (IDC_WORKDIR != IDC_BATCHFILELBL-1)
    #error Error in IDC constants: IDC_WORKDIR != IDC_BATCHFILELBL-1
    #endif

    #if (IDC_BATCHFILELBL != IDC_BATCHFILE-1)
    #error Error in IDC constants: IDC_BATCHFILELBL != IDC_BATCHFILE-1
    #endif

    for (i=IDC_WORKDIRLBL; i<=IDC_BATCHFILE; i++)
        EnableWindow(GetDlgItem(hDlg, i), f);
}


void ApplyPrgDlg(HWND hDlg, PPRGINFO ppi)
{
    int i;
    PROPPRG prg;
    PROPENV env;
#ifdef UNICODE
    PROPNT40 nt40;
#endif
    PPROPLINK ppl = ppi->ppl;
    FunctionName(ApplyPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    // Get the current set of properties, then overlay the new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                              &prg, SIZEOF(prg), GETPROPS_NONE
                             ) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_ENV),
                              &env, SIZEOF(env), GETPROPS_NONE
                             )
#ifdef UNICODE
                               ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40),
                              &nt40, SIZEOF(nt40), GETPROPS_NONE
                             )

#endif
       ) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }


    // Retrieve Icon information

#ifdef UNICODE
    lstrcpyW( nt40.awchIconFile, ppi->atchIconFile );
    PifMgr_WCtoMBPath( nt40.awchIconFile, nt40.achSaveIconFile, ARRAYSIZE(nt40.achSaveIconFile) );
    lstrcpyA( prg.achIconFile, nt40.achSaveIconFile );
#else
    lstrcpyA( prg.achIconFile, ppi->atchIconFile );
#endif
    prg.wIconIndex = ppi->wIconIndex;

    // Retrieve strings for Title, Command Line,
    // Working Directory and Batch File

#ifdef UNICODE
    // Title
    GetDlgItemTextW(hDlg, IDC_TITLE, nt40.awchTitle, ARRAYSIZE(nt40.awchTitle));
    GetDlgItemTextA(hDlg, IDC_TITLE, nt40.achSaveTitle, ARRAYSIZE(nt40.achSaveTitle));
    nt40.awchTitle[ ARRAYSIZE(nt40.awchTitle)-1 ] = TEXT('\0');
    nt40.achSaveTitle[ ARRAYSIZE(nt40.achSaveTitle)-1 ] = '\0';
    lstrcpyA( prg.achTitle, nt40.achSaveTitle );

    // Command Line
    GetDlgItemTextW(hDlg, IDC_CMDLINE, nt40.awchCmdLine, ARRAYSIZE(nt40.awchCmdLine));
    GetDlgItemTextA(hDlg, IDC_CMDLINE, nt40.achSaveCmdLine, ARRAYSIZE(nt40.achSaveCmdLine));
    nt40.awchCmdLine[ ARRAYSIZE(nt40.awchCmdLine)-1 ] = TEXT('\0');
    nt40.achSaveCmdLine[ ARRAYSIZE(nt40.achSaveCmdLine)-1 ] = '\0';
    lstrcpyA( prg.achCmdLine, nt40.achSaveCmdLine );

    // Working Directory
    GetDlgItemTextW(hDlg, IDC_WORKDIR, nt40.awchWorkDir, ARRAYSIZE(nt40.awchWorkDir));
    nt40.awchWorkDir[ ARRAYSIZE(nt40.awchWorkDir)-1 ] = TEXT('\0');
    PifMgr_WCtoMBPath(nt40.awchWorkDir, nt40.achSaveWorkDir, ARRAYSIZE(nt40.achSaveWorkDir));
    lstrcpyA(prg.achWorkDir, nt40.achSaveWorkDir);

    // Batch File
    GetDlgItemTextW(hDlg, IDC_BATCHFILE, nt40.awchBatchFile, ARRAYSIZE(nt40.awchBatchFile));
    nt40.awchBatchFile[ ARRAYSIZE(nt40.awchBatchFile)-1 ] = TEXT('\0');
    PifMgr_WCtoMBPath(nt40.awchBatchFile, nt40.achSaveBatchFile, ARRAYSIZE(nt40.achSaveBatchFile));
    lstrcpyA(env.achBatchFile, nt40.achSaveBatchFile);

#else
    GetDlgItemTextA(hDlg, IDC_TITLE,     prg.achTitle,     ARRAYSIZE(prg.achTitle));
    GetDlgItemTextA(hDlg, IDC_CMDLINE,   prg.achCmdLine,   ARRAYSIZE(prg.achCmdLine));
    GetDlgItemTextA(hDlg, IDC_WORKDIR,   prg.achWorkDir,   ARRAYSIZE(prg.achWorkDir));
    GetDlgItemTextA(hDlg, IDC_BATCHFILE, env.achBatchFile, ARRAYSIZE(env.achBatchFile));
#endif

    prg.wHotKey = (WORD)SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_GETHOTKEY, 0, 0);


    i = (int)SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_GETCURSEL, 0, 0);
    prg.flPrgInit &= ~(PRGINIT_MINIMIZED | PRGINIT_MAXIMIZED);
    if (i == 1)
        prg.flPrgInit |= PRGINIT_MINIMIZED;
    if (i == 2)
        prg.flPrgInit |= PRGINIT_MAXIMIZED;

    GetDlgBits(hDlg, &abinfPrg[0], ARRAYSIZE(abinfPrg), &prg.flPrg);

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_PRG),
                        &prg, SIZEOF(prg), SETPROPS_NONE) ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_ENV),
                        &env, SIZEOF(env), SETPROPS_NONE)
#ifdef UNICODE
                                                           ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT40),
                        &nt40, SIZEOF(nt40), SETPROPS_NONE)
#endif
       )
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    else
    if (ppl->hwndNotify) {
        ppl->flProp |= PROP_NOTIFY;
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(prg), (LPARAM)MAKELP(0,GROUP_PRG));
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(env), (LPARAM)MAKELP(0,GROUP_ENV));
#ifdef UNICODE
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(nt40), (LPARAM)MAKELP(0,GROUP_NT40));
#endif
    }
}


void BrowseIcons(HWND hDlg, PPRGINFO ppi)
{
    HICON hIcon;
    int wIconIndex;


    wIconIndex = (int)ppi->wIconIndex;
    if (PickIconDlg(hDlg, ppi->atchIconFile, ARRAYSIZE(ppi->atchIconFile), (int *)&wIconIndex)) {
        hIcon = ExtractIcon(hModule, ppi->atchIconFile, wIconIndex);
        if ((UINT_PTR)hIcon <= 1)
            Warning(hDlg, IDS_NO_ICONS, MB_ICONINFORMATION | MB_OK);
        else {
            ppi->hIcon = hIcon;
            ppi->wIconIndex = (WORD)wIconIndex;
            hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_ICONBMP, STM_SETICON, (WPARAM)ppi->hIcon, 0);
            if (hIcon)
                VERIFYTRUE(DestroyIcon(hIcon));
        }
    }
}


BOOL WarnUserCfgChange(HWND hDlg)
{
    TCHAR szTitle[MAX_STRING_SIZE];
    TCHAR szWarning[MAX_STRING_SIZE];

    LoadString(hModule, IDS_NUKECONFIGTITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(hModule, IDS_NUKECONFIGMSG, szWarning, ARRAYSIZE(szWarning));
    return(IDYES == MessageBox(hDlg, szWarning, szTitle,
                               MB_YESNO | MB_DEFBUTTON1 | MB_ICONHAND));
}


#ifdef WINNT
BOOL_PTR CALLBACK DlgPifNtProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRGINFO ppi;

    ppi = (PPRGINFO)GetWindowLongPtr( hDlg, DWLP_USER );

    switch (uMsg) {

    case WM_INITDIALOG:
        ppi = (PPRGINFO)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        InitNtPifDlg(hDlg, ppi);
        break;

    case WM_DESTROY:
        SetWindowLongPtr(hDlg, DWLP_USER, 0);
        break;

    HELP_CASES(rgdwNTHelp)               // handle help messages

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
        case IDC_OK:
            // BUGBUG - "You will need to exit this application and restart
            //           it in order for Advanced properties to take effect."
            ApplyNtPifDlg(hDlg, ppi);
            // fall through

        case IDCANCEL:
        case IDC_CANCEL :
            EndDialog(hDlg, 0);
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDC_NTTIMER:
            CheckDlgButton(hDlg, IDC_NTTIMER, !IsDlgButtonChecked(hDlg, IDC_NTTIMER));
            break;
        }
        break;

    default:
        return(FALSE);

    }
    return(TRUE);
}

#else
// disable "inline assembler precludes global optimizations" warning
#pragma warning(disable:4704)

BOOL WarnUserOldCompression(HWND hDlg)
{
    BOOL fOldCompression = FALSE;
    char szTitle[MAX_STRING_SIZE];
    char szWarning[MAX_STRING_SIZE];

    _asm {
        mov     ax,4A33h
        int     2Fh             ; returns boot drive in BH
        test    ax,ax           ; success?
        jnz     l9              ; no
        test    bh,bh           ; is there really a host drive?
        jz      l9              ; no
        test    bl,10h          ; was the compression driver preloaded?
        jnz     l9              ; yes
        inc     [fOldCompression]
    l9:
    }
    if (fOldCompression) {
        LoadString(hModule, IDS_NUKECONFIGTITLE, szTitle, SIZEOF(szTitle));
        LoadString(hModule, IDS_OLDCOMPRESSIONMSG, szWarning, SIZEOF(szWarning));
        return(IDYES == MessageBox(hDlg, szWarning, szTitle,
                                   MB_YESNO | MB_DEFBUTTON1 | MB_ICONHAND));
    }
    return TRUE;
}


BOOL_PTR CALLBACK DlgAdvPrgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRGINFO ppi;
    FunctionName(DlgAdvPrgProc);

    ppi = (PPRGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {

    case WM_INITDIALOG:
        ppi = (PPRGINFO)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppi->flPrgInitPrev = InitAdvPrgDlg(hDlg, ppi);
        break;

    case WM_DESTROY:
        SetWindowLongPtr(hDlg, DWLP_USER, 0);
        break;

    HELP_CASES(rgdwHelpAdvPrg)          // handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0 && LOWORD(wParam) > IDCANCEL)
            break;                      // message not from a control
                                        // nor from the system menu

        switch (LOWORD(wParam)) {

        case IDOK:
        case IDC_OK:
            // BUGBUG - "You will need to exit this application and restart
            //           it in order for Advanced properties to take effect."
            ApplyAdvPrgDlg(hDlg, ppi, TRUE);
            EndDialog(hDlg, 0);
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDCANCEL:
        case IDC_CANCEL :
            ppi->ppl->flProp &= ~PROP_REALMODE; // Revert PROP_REALMODE
            if (ppi->flPrgInitPrev & PRGINIT_REALMODE) {
                ppi->ppl->flProp |= PROP_REALMODE;
            }
            EndDialog(hDlg, 0);
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDC_REALMODE:
            if (HIWORD(wParam) == BN_CLICKED) {
                ppi->ppl->flProp &= ~PROP_REALMODE;
                if (IsDlgButtonChecked(hDlg, IDC_REALMODE)) {
                    ppi->ppl->flProp |= PROP_REALMODE;
                    if (IsDlgButtonChecked(hDlg, IDC_CLEANCONFIG)) {
                        goto SetCleanCfg;
                    }
                }
                AdjustAdvControls(ppi->ppl, hDlg);
            }
            return FALSE;

        case IDC_REALMODEWIZARD:
            if (HIWORD(wParam) == BN_CLICKED) {
                if (IsCleanCfgEmpty(hDlg) ||
                    (ppi->fCfgSetByWiz &&
                     (!SendDlgItemMessage(hDlg, IDC_CONFIG, EM_GETMODIFY, 0, 0)) &&
                     (!SendDlgItemMessage(hDlg, IDC_AUTOEXEC, EM_GETMODIFY, 0, 0))) ||
                    WarnUserCfgChange(hDlg)) {
                    SetWizProps(hDlg, ppi, WIZACTION_UICONFIGPROP);
                }
            }
            return FALSE;

        case IDC_CLEANCONFIG:
          SetCleanCfg:
            if (IsCleanCfgEmpty(hDlg)) {
                if (WarnUserOldCompression(hDlg)) {
                    SetWizProps(hDlg, ppi, WIZACTION_CREATEDEFCLEANCFG);
                }
                else {
                    CheckRadioButton(hDlg, IDC_CURCONFIG, IDC_CLEANCONFIG, IDC_CURCONFIG);
                    SetFocus(GetDlgItem(hDlg, IDC_CURCONFIG));
                }
            }
            // WE DO WANT TO FALL-THROUGH
        case IDC_CURCONFIG:
            AdjustAdvControls(ppi->ppl, hDlg);
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


void SetWizProps(HWND hDlg, PPRGINFO ppi, UINT uWizAction)
{
    PROPPRG prg;


    // Save original properties before applying

    PifMgr_GetProperties((int)ppi->ppl, MAKELP(0,GROUP_PRG),
            &prg, SIZEOF(prg), GETPROPS_NONE);

    // Apply the settings currently displayed so that
    // AppWiz can obtain them/modify them

    ApplyAdvPrgDlg(hDlg, ppi, FALSE);

    if (AppWizard(hDlg, (int)ppi->ppl, uWizAction) == PIFWIZERR_SUCCESS) {
        ppi->fCfgSetByWiz = TRUE;
    }

    // When AppWizard returns, we need to refresh our prop sheet

    InitAdvPrgDlg(hDlg, ppi);


    // Restore original properties after re-initing, so that
    // the user can still hit cancel and undo everything

    PifMgr_SetProperties((int)ppi->ppl, MAKELP(0,GROUP_PRG),
        &prg, SIZEOF(prg), SETPROPS_NONE);
}

#endif


#ifdef WINNT

WORD InitNtPifDlg(HWND hDlg, register PPRGINFO ppi)
{
    PROPNT31 nt31;
    PPROPLINK ppl = ppi->ppl;
    FunctionName(InitAdvPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // initialize the DLG controls
    SetDlgItemTextA( hDlg, IDC_CONFIGNT, nt31.achConfigFile );
    SetDlgItemTextA( hDlg, IDC_AUTOEXECNT, nt31.achAutoexecFile );

    if (nt31.dwWNTFlags & COMPAT_TIMERTIC)
        CheckDlgButton( hDlg, IDC_NTTIMER, 1 );
    else
        CheckDlgButton( hDlg, IDC_NTTIMER, 0 );

    SHAutoComplete(GetDlgItem(hDlg, IDC_AUTOEXECNT), 0);
    SHAutoComplete(GetDlgItem(hDlg, IDC_CONFIGNT), 0);
    return 0;
}

#else

WORD InitAdvPrgDlg(HWND hDlg, register PPRGINFO ppi)
{
    PROPPRG prg;
    PPROPLINK ppl = ppi->ppl;
    FunctionName(InitAdvPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                        &prg, SIZEOF(prg), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    if (prg.flPrgInit & PRGINIT_REALMODE) {
        ppl->flProp |= PROP_REALMODE;
    } else {
        ppl->flProp &= ~PROP_REALMODE;
    }

    SetDlgBits(hDlg, &abinfAdvPrg[0], ARRAYSIZE(abinfAdvPrg), prg.flPrg);
    SetDlgBits(hDlg, &abinfAdvPrgInit[0], ARRAYSIZE(abinfAdvPrgInit), prg.flPrgInit);

    SetDlgItemTextA(hDlg, IDC_PIFNAME, prg.achPIFFile);

    ppi->hConfig = GetAdvPrgData(hDlg, ppl, MAX_CONFIG_SIZE, IDC_CONFIG, szCONFIGHDRSIG40);
    ppi->hAutoexec = GetAdvPrgData(hDlg, ppl, MAX_AUTOEXEC_SIZE, IDC_AUTOEXEC, szAUTOEXECHDRSIG40);

    if (MustRebootSystem()) {
        CheckRadioButton(hDlg, IDC_CURCONFIG, IDC_CLEANCONFIG, IDC_CLEANCONFIG);
        ShowWindow(GetDlgItem(hDlg, IDC_CURCONFIG), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_CLEANCONFIG), SW_HIDE);
    } else {
        BOOL fUseCur = IsCleanCfgEmpty(hDlg);
        if ((ppl->flProp & PROP_REALMODE) == 0 &&
            (GetMSDOSOptGlobalFlags() & DOSOPTGF_DEFCLEAN)) {
            fUseCur = FALSE;
        }
        CheckRadioButton(hDlg, IDC_CURCONFIG, IDC_CLEANCONFIG,
                        fUseCur ? IDC_CURCONFIG : IDC_CLEANCONFIG);
    }

    AdjustAdvControls(ppl, hDlg);

  //  /* By default, hide the quick-start control,
  //   * at least until I can get it to work on *my* machine. -JP
  //   */
  //  if (GetKeyState(VK_CONTROL) >= 0 || GetKeyState(VK_SHIFT) >= 0) {
  //      ShowWindow(GetDlgItem(hDlg, IDC_QUICKSTART), SW_HIDE);
  //  }
    return prg.flPrgInit;
}

#endif //WINNT


#ifdef WINNT

void ApplyNtPifDlg( HWND hDlg, PPRGINFO ppi )
{
    PROPNT31 nt31;
    PPROPLINK ppl = ppi->ppl;

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    // Get current set of properties, then overlay new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetDlgItemTextA( hDlg,
                     IDC_CONFIGNT,
                     nt31.achConfigFile,
                     ARRAYSIZE( nt31.achConfigFile )
                    );
    GetDlgItemTextA( hDlg,
                     IDC_AUTOEXECNT,
                     nt31.achAutoexecFile,
                     ARRAYSIZE( nt31.achAutoexecFile )
                    );

    nt31.dwWNTFlags &= (~COMPAT_TIMERTIC);
    if (IsDlgButtonChecked( hDlg, IDC_NTTIMER ))
        nt31.dwWNTFlags |= COMPAT_TIMERTIC;


    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), SETPROPS_NONE)) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    }
    if (ppl->hwndNotify) {
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(nt31), (LPARAM)MAKELP(0,GROUP_NT31));
    }


}

#else // ifdef WINNT

//
// flPrgInit switches which, if changed, require the app to be restarted.
//

#define PRGINIT_RELAUNCH (PRGINIT_WINLIE|PRGINIT_REALMODE|PRGINIT_REALMODESILENT)

void ApplyAdvPrgDlg(HWND hDlg, PPRGINFO ppi, BOOL fDoWarns)
{
    PROPPRG prg;
    PPROPLINK ppl = ppi->ppl;
    FunctionName(ApplyAdvPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    // Get the current set of properties, then overlay the new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                        &prg, SIZEOF(prg), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetDlgBits(hDlg, &abinfAdvPrg[0], ARRAYSIZE(abinfAdvPrg), &prg.flPrg);
    GetDlgBits(hDlg, &abinfAdvPrgInit[0], ARRAYSIZE(abinfAdvPrgInit), &prg.flPrgInit);

    SetAdvPrgData(hDlg, ppl, ppi->hConfig, IDC_CONFIG, szCONFIGHDRSIG40);
    SetAdvPrgData(hDlg, ppl, ppi->hAutoexec, IDC_AUTOEXEC, szAUTOEXECHDRSIG40);

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_PRG),
                        &prg, SIZEOF(prg), SETPROPS_NONE)) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    } else {
        if (ppl->hwndNotify) {
            ppl->flProp |= PROP_NOTIFY;
            PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(prg), (LPARAM)MAKELP(0,GROUP_PRG));
        }
        if (fDoWarns && ppl->hVM && ((prg.flPrgInit ^ ppi->flPrgInitPrev) & PRGINIT_RELAUNCH)) {
            Warning(hDlg, IDS_ADVANCED_RELAUNCH, MB_ICONWARNING | MB_OK );
        }
    }
}

#undef LocalAlloc
#undef LocalFree
#undef LocalLock
#undef LocalUnlock
#define LocalAlloc LocalAlloc
#define LocalLock LocalLock
#define LocalFree LocalFree
#define LocalUnlock LocalUnlock
HLOCAL GetAdvPrgData(HWND hDlg, PPROPLINK ppl, int cbMax, int idCtrl, LPCSTR lpszName)
{
    int cbData;
    HLOCAL hData = NULL;
    HLOCAL hTemp;

    cbData = PifMgr_GetProperties(ppl, lpszName, NULL, 0, GETPROPS_NONE);

    //
    // Note we need to use LocalAlloc because USER32 does a LocalSize on
    // this handle later.
    hData = LocalAlloc(LHND, cbMax);

    if (!hData)
        hData = LocalAlloc(LHND, cbData+128);
    if (hData) {
        cbData = min(cbData, cbMax);
        PifMgr_GetProperties(ppl, lpszName, LocalLock(hData), cbData, GETPROPS_NONE);
        LocalUnlock(hData);
        hTemp = (HLOCAL)SendDlgItemMessage(hDlg, idCtrl, EM_GETHANDLE, 0, 0);
        VERIFYFALSE(LocalFree(hTemp));
        SendDlgItemMessage(hDlg, idCtrl, EM_SETHANDLE, (WPARAM)hData, 0);
        LimitDlgItemText(hDlg, idCtrl, cbMax);
        SendDlgItemMessage(hDlg, idCtrl, EM_SETMODIFY, (WPARAM)FALSE, 0);
    }
    return hData;
}


void AdjustAdvControls(PPROPLINK ppl, HWND hDlg)
{
    BOOL f;
    int i, id;
    BOOL bEnableNorm = TRUE;
    BOOL bEnableCfg = FALSE;

    const static int aidAdv[] = {
        IDC_WINLIE,
        IDC_SUGGESTMSDOS,
        IDC_WARNMSDOS,
        IDC_CURCONFIG,
        IDC_CLEANCONFIG,
        IDC_CONFIGLBL,
        IDC_CONFIG,
        IDC_AUTOEXECLBL,
        IDC_AUTOEXEC,
        IDC_REALMODEWIZARD,
    };

    if (ppl->flProp & PROP_REALMODE) {
        bEnableNorm = FALSE;
        bEnableCfg = IsDlgButtonChecked(hDlg, IDC_CLEANCONFIG);
    }
    for (i=0; i<ARRAYSIZE(aidAdv); i++) {

        f = bEnableCfg;

        switch(id = aidAdv[i]) {
        case IDC_WINLIE:
        case IDC_SUGGESTMSDOS:
            f = bEnableNorm;
            break;
        case IDC_WARNMSDOS:
        case IDC_CURCONFIG:
        case IDC_CLEANCONFIG:
            f = !bEnableNorm;
            break;
        }
        EnableWindow(GetDlgItem(hDlg, id), f);
    }
}


BOOL IsCleanCfgEmpty(HWND hDlg)
{
    BOOL fIsEmpty = Edit_GetTextLength(GetDlgItem(hDlg, IDC_CONFIG)) == 0 &&
                    Edit_GetTextLength(GetDlgItem(hDlg, IDC_AUTOEXEC)) == 0;
    return(fIsEmpty);
}


void SetAdvPrgData(HWND hDlg, PPROPLINK ppl, HLOCAL hData, int idCtrl, LPCSTR lpszName)
{
    int cbData, cLines;

    if (IsDlgButtonChecked(hDlg, IDC_CURCONFIG)) {
        CHAR NullStr = (CHAR)0;
        PifMgr_SetProperties(ppl, lpszName, &NullStr, 0, SETPROPS_NONE);
    } else {
        if (hData) {
            if (SendDlgItemMessage(hDlg, idCtrl, EM_GETMODIFY, 0, 0)) {

                cLines = (int)SendDlgItemMessage(hDlg, idCtrl, EM_GETLINECOUNT, 0, 0);
                cbData = (int)SendDlgItemMessage(hDlg, idCtrl, EM_LINEINDEX, cLines-1, 0);
                cbData += (int)SendDlgItemMessage(hDlg, idCtrl, EM_LINELENGTH, cbData, 0);

                if (hData) {
                    if (cbData != PifMgr_SetProperties(ppl, lpszName,
                                        (LPSTR)hData, cbData, SETPROPS_NONE)) {
                        Warning(hDlg, (WORD)IDS_UPDATE_ERROR, (WORD)(MB_ICONEXCLAMATION | MB_OK));
                    }
                }
            }
            /* Do not free the hData; USER will do that for us. */
        }
    }
}
#endif // ifdef WINNT


#ifdef UNICODE
HICON LoadPIFIcon(LPPROPPRG lpprg, LPPROPNT40 lpnt40)
#else
HICON LoadPIFIcon(LPPROPPRG lpprg)
#endif
{
    HICON hIcon = NULL;
#ifdef UNICODE
    WCHAR awchTmp[ MAX_PATH ];

    ualstrcpy( awchTmp, lpnt40->awchIconFile );
    PifMgr_WCtoMBPath( awchTmp, lpprg->achIconFile, ARRAYSIZE(lpprg->achIconFile) );
    hIcon = ExtractIcon(hModule, awchTmp, lpprg->wIconIndex);
#else
    hIcon = ExtractIcon(hModule, lpprg->achIconFile, lpprg->wIconIndex);
#endif
    if ((DWORD_PTR)hIcon <= 1) {         // 0 means none, 1 means bad file
        hIcon = NULL;
    }
    return hIcon;
}
