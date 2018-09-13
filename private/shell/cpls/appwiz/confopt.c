//
//  ConfOpt.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "priv.h"
#include "appwiz.h"

//
//  BUGBUG -- Where do these options go???
//
#define MAX_CFG_FILE_SIZE   20000
#define MAX_DESC_SIZE         100

//
//  Return codes for OptSelected
//
#define OPTSEL_YES      0
#define OPTSEL_NO       1
#define OPTSEL_NOTSUPP  2

//
//  Define checkbox states for listview
//
#define LVIS_GCNOCHECK      0x1000
#define LVIS_GCCHECK        0x2000

//
//  Character definitions
//
#define CR                13
#define LF                10


TCHAR const c_szCRLF[]            = {CR, LF, 0};
TCHAR const c_szRegValAutoexec[]  = REGSTR_VAL_AUTOEXEC;
TCHAR const c_szRegValConfigSys[] = REGSTR_VAL_CONFIGSYS;
TCHAR const c_szRegDosOptFlags[]  = REGSTR_VAL_DOSOPTFLAGS;
TCHAR const c_szRegStandardOpt[]  = REGSTR_VAL_STDDOSOPTION;
TCHAR const c_szRegDosOptTip[]    = REGSTR_VAL_DOSOPTTIP;
TCHAR const c_szRegDosOptsPath[]  = REGSTR_PATH_MSDOSOPTS;
TCHAR const c_szRegGlobalFlags[]  = REGSTR_VAL_DOSOPTGLOBALFLAGS;
TCHAR const c_szRegShutdownKey[]  = REGSTR_PATH_SHUTDOWN;
TCHAR const c_szForceRebootVal[]  = REGSTR_VAL_FORCEREBOOT;



BOOL MustRebootSystem(void)
{
    HKEY hk;
    BOOL bMustReboot = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRegShutdownKey, &hk) == ERROR_SUCCESS)
    {
        bMustReboot = (RegQueryValueEx(hk, c_szForceRebootVal, NULL,
                                       NULL, NULL, NULL) == ERROR_SUCCESS);
        RegCloseKey(hk);
    }
    return(bMustReboot);
}


DWORD GetMSDOSOptGlobalFlags(LPWIZDATA lpwd)
{
    if ((lpwd->dwFlags & WDFLAG_READOPTFLAGS) == 0)
    {
        HKEY hk;

        lpwd->dwFlags |= WDFLAG_READOPTFLAGS;

        if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRegDosOptsPath, &hk)
               == ERROR_SUCCESS)
        {
            UINT cb = sizeof(lpwd->dwDosOptGlobalFlags);

            if (RegQueryValueEx(hk, c_szRegGlobalFlags, NULL, NULL,
                                (LPVOID)(&(lpwd->dwDosOptGlobalFlags)), &cb)
                                != ERROR_SUCCESS)
            {
                lpwd->dwDosOptGlobalFlags = 0;
            }
            RegCloseKey(hk);
        }

        if (MustRebootSystem())
        {
            lpwd->dwDosOptGlobalFlags |= DOSOPTGF_DEFCLEAN;
        }
    }
    return(lpwd->dwDosOptGlobalFlags);
}


//
//  Structure used to store text for Autoexec.Bat and Config.Sys
//

typedef struct _TEXTDATA {
    UINT    cb;
    LPTSTR  lpszData;
} TEXTDATA, FAR * LPTEXTDATA;


//
//  Gets a single value from the specified option index.  If no data is
//  found for the specified value name then a 0 DWORD is stored.
//

void GetOptVal(LPWIZDATA lpwd, int i, LPCTSTR lpszValName, LPVOID lpData, UINT cb)
{
    if (RegQueryValueEx(lpwd->DosOpt[i].hk, lpszValName, NULL, NULL, lpData, &cb) != ERROR_SUCCESS)
    {
        *(LPDWORD)lpData = (DWORD)0;
    }
}


BOOL FindDriver(int idFiles, int idLoadHigh, HKEY hk, LPCTSTR lpszRegVal,
                LPCTSTR FAR dirs[])
{
    TCHAR  szFiles[MAX_PATH];
    LPTSTR lpszCur, lpszFileName;
    TCHAR  szCommand[MAX_PATH+20];

    LoadAndStrip(idFiles, szFiles, ARRAYSIZE(szFiles));
    LoadString(g_hinst, idLoadHigh, szCommand, ARRAYSIZE(szCommand));

    lpszFileName = &szCommand[lstrlen(szCommand)];
    lpszCur = szFiles;

    while (*lpszCur)
    {
        lstrcpy(lpszFileName, lpszCur);
        if (PathResolve(lpszFileName, dirs, PRF_VERIFYEXISTS))
        {
            PathGetShortPath(lpszFileName);

            RegSetValueEx(hk, lpszRegVal, 0, REG_SZ, (LPBYTE) szCommand,
                            (lstrlen(szCommand)+1)*sizeof(TCHAR));

            return(TRUE);
        }
        lpszCur = SkipStr(lpszCur);
    }
    return(FALSE);
}


void SetUpMouse(LPWIZDATA lpwd, int i)
{
    #define MOpt lpwd->DosOpt[i]
    TCHAR  szMouseEnv[64];
    TCHAR  szMouseDir[64];
    LPCTSTR FAR dirs[] = {szMouseDir, NULL};

    LoadString(g_hinst, IDS_MOUSEENV, szMouseEnv, ARRAYSIZE(szMouseEnv));
    lstrcpy(szMouseDir, szMouseEnv);

    DoEnvironmentSubst(szMouseDir, ARRAYSIZE(szMouseDir));

    if (lstrcmp(szMouseDir, szMouseEnv) == 0)
    {
        dirs[0] = NULL;
    }

    if (FindDriver(IDS_MOUSETSRS, IDS_LOADHIGH,MOpt.hk, c_szRegValAutoexec,  dirs) ||
        FindDriver(IDS_MOUSEDRVS, IDS_DEVHIGH, MOpt.hk, c_szRegValConfigSys, dirs))
    {
        MOpt.dwFlags |= DOSOPTF_SUPPORTED;
        WIZERROR(TEXT("Found real mode mouse driver"));
    }

    MOpt.dwFlags &= ~DOSOPTF_NEEDSETUP;
    RegSetValueEx(MOpt.hk, c_szRegDosOptFlags, 0, REG_DWORD,
                  (LPBYTE)&(MOpt.dwFlags), sizeof(MOpt.dwFlags));
    #undef MOpt
}


//
//  The option is not configured.  Here's where to add code to set up any
//  standard option.  Currently, we only set up the mouse.
//

void SetupOption(LPWIZDATA lpwd, int i)
{
    if (RMOPT_MOUSE == lpwd->DosOpt[i].dwStdOpt)
    {
        WIZERROR(TEXT("About to search for real mode mouse driver"));
        SetUpMouse(lpwd, i);
    }
}


//
//  Closes all open hkeys and frees the memory.  NOTE:        This function can
//  be called any time, even if ReadRegInfo has not been called previously.
//

void FreeRegInfo(LPWIZDATA lpwd)
{
    int i;

    if (!lpwd->DosOpt)
    {
        return;
    }

    for (i = 0; i < lpwd->NumOpts; i++)
    {
        RegCloseKey(lpwd->DosOpt[i].hk);
    }

    LocalFree(lpwd->DosOpt);
    lpwd->DosOpt = NULL;
    lpwd->NumOpts = 0;
}


//
//  Initializes the option table in the wizard data header.
//

BOOL ReadRegInfo(LPWIZDATA lpwd)
{
    HKEY    hk;
    BOOL    bSuccess = FALSE;
    int     i;

    if (lpwd->DosOpt)
    {
        return(TRUE);
    }

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRegDosOptsPath, &hk) !=
            ERROR_SUCCESS)
    {
        goto Exit;
    }

    if (RegQueryInfoKey(hk, NULL, NULL, NULL, &(lpwd->NumOpts),
                        NULL, NULL, NULL, NULL, NULL, NULL, NULL) !=
            ERROR_SUCCESS)
    {
        goto ExitCloseKey;
    }

    lpwd->DosOpt = LocalAlloc(LPTR, lpwd->NumOpts * sizeof(DOSOPT));

    if (!lpwd->DosOpt)
    {
        goto ExitCloseKey;
    }

    for (i = 0; i < lpwd->NumOpts; i++)
    {
        UINT   cb;
        TCHAR  szOptKey[80];        // BUGBUG -- Size
        HKEY   hkOpt;
        UINT   uOrder = 0;
        int    InsPos;

        cb = ARRAYSIZE(szOptKey);

        if ((RegEnumKeyEx(hk, i, szOptKey, &cb, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) ||
            (RegOpenKey(hk, szOptKey, &hkOpt) != ERROR_SUCCESS))
        {
            lpwd->NumOpts = i - 1;
            FreeRegInfo(lpwd);        // This frees the DosOpt memory
            goto ExitCloseKey;
        }

        cb = sizeof(uOrder);

        RegQueryValueEx(hkOpt, REGSTR_VAL_OPTORDER, NULL, NULL, (LPVOID)&uOrder, &cb);

        for (InsPos = i;
             (InsPos > 0) && (lpwd->DosOpt[InsPos-1].uOrder > uOrder);
             InsPos--);

        if (InsPos < i)
        {
            MoveMemory(&(lpwd->DosOpt[InsPos+1]), &(lpwd->DosOpt[InsPos]),
                       (i - InsPos) * sizeof(DOSOPT));
        }

        lpwd->DosOpt[InsPos].hk = hkOpt;
        lpwd->DosOpt[InsPos].uOrder = uOrder;

        GetOptVal(lpwd, InsPos, c_szRegDosOptFlags, &(lpwd->DosOpt[InsPos].dwFlags), sizeof(DWORD));
        GetOptVal(lpwd, InsPos, c_szRegStandardOpt, &(lpwd->DosOpt[InsPos].dwStdOpt), sizeof(DWORD));

        if (lpwd->DosOpt[InsPos].dwFlags & DOSOPTF_NEEDSETUP)
        {
            SetupOption(lpwd, InsPos);
        }
    }
    bSuccess = TRUE;

ExitCloseKey:
    RegCloseKey(hk);
Exit:
    return(bSuccess);
}


//
//  Inserts a single column into the specified ListView.
//

void InitSingleColListView(HWND hLV)
{
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT    rc;

    GetClientRect(hLV, &rc);

    col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
            - GetSystemMetrics(SM_CXSMICON)
            - 2 * GetSystemMetrics(SM_CXEDGE);

    ListView_InsertColumn(hLV, 0, &col);
}


//
//  Initializes the listview with all available options
//

void ConfOptInit(HWND hDlg, LPPROPSHEETPAGE lpp)
{
    HIMAGELIST himlState;
    HWND hwndOptions = GetDlgItem(hDlg, IDC_OPTIONLIST);
    LPWIZDATA lpwd = InitWizSheet(hDlg, (LPARAM)lpp, 0);

    InitSingleColListView(hwndOptions);

    //
    // Lets load our bitmap as an imagelist
    //

    himlState = ImageList_LoadImage(g_hinst, MAKEINTRESOURCE(IDB_CHECKSTATES), 0, 2,
            CLR_NONE, IMAGE_BITMAP, LR_LOADTRANSPARENT);

    ListView_SetImageList(hwndOptions, himlState, LVSIL_STATE);

    //
    //        Find all options for MS-DOS configs and set their approropriate state
    //        information
    //

    if (ReadRegInfo(lpwd))
    {
        int i;

        for (i = 0; i < lpwd->NumOpts; i++)
        {
            DWORD dwFlags = lpwd->DosOpt[i].dwFlags;

            if ((dwFlags & DOSOPTF_SUPPORTED) &&
                ((dwFlags & DOSOPTF_ALWAYSUSE) == 0) &&
                ((dwFlags & DOSOPTF_USESPMODE) == 0 ||
                 (lpwd->dwFlags & WDFLAG_REALMODEONLY) == 0) &&
                (lpwd->DosOpt[i].uOrder > 0))
            {
                TCHAR   szDesc[MAX_DESC_SIZE];
                LV_ITEM lvi;

                GetOptVal(lpwd, i, NULL, szDesc, sizeof(szDesc));

                lvi.mask     = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
                lvi.iItem    = 0x7FFF;
                lvi.iSubItem = 0;

                //
                ///// BUGBUG -- If program properties contains real mode flags, use them for defaults!
                //

                lvi.state      = (dwFlags & DOSOPTF_DEFAULT) ? LVIS_GCCHECK : LVIS_GCNOCHECK;
                lvi.stateMask  = LVIS_ALL;
                lvi.pszText    = szDesc;
                lvi.lParam     = (LPARAM)i;
                lvi.cchTextMax = 0;

                ListView_InsertItem(hwndOptions, &lvi);
            }
        }
    }
}


//
//  Toggles the state of the specified item in the list view.
//

void ToggleState(HWND hwndLV, int i)
{
    UINT    state = ListView_GetItemState(hwndLV, i, LVIS_STATEIMAGEMASK);

    state = (state == LVIS_GCNOCHECK) ? LVIS_GCCHECK : LVIS_GCNOCHECK;

    ListView_SetItemState(hwndLV, i, state, LVIS_STATEIMAGEMASK);
}


//
//  Returns the path to the boot directory.  If we can't find it in the
//  registry then this function returns the default (drive that windows
//  directory is on)
//

void GetBootDir(LPTSTR lpszBootDir, int cchBootDir)
{
    HKEY    hkSetup;

    *lpszBootDir = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP REGSTR_KEY_SETUP,
                   &hkSetup) == ERROR_SUCCESS)
    {
        UINT cb = cchBootDir * SIZEOF(TCHAR);

        RegQueryValueEx(hkSetup, REGSTR_VAL_BOOTDIR, NULL,
                        NULL, (LPBYTE) lpszBootDir, &cb);
        RegCloseKey(hkSetup);
    }

    if (*lpszBootDir == 0)
    {
        GetWindowsDirectory(lpszBootDir, cchBootDir);
        if (lpszBootDir[0] != TEXT('\0')
             && lpszBootDir[1] == TEXT(':')
             && lpszBootDir[2] == TEXT('\\'))
            lpszBootDir[3] = TEXT('\0');
        else
            LoadString(g_hinst, IDS_DEFBOOTDIR, lpszBootDir, cchBootDir);
    }
}


//
//  Process the clicks on the listview.  do a hittest to see where the user
//  clicked.  If on one of the state bitmaps, toggle it.
//

void ConfOptClick(HWND hDlg, LPNMHDR pnmhdr)
{
    //
    // The user clicked on one the listview see where...
    //

    DWORD dwpos;
    LV_HITTESTINFO lvhti;

    dwpos = GetMessagePos();
    lvhti.pt.x = LOWORD(dwpos);
    lvhti.pt.y = HIWORD(dwpos);

    MapWindowPoints(HWND_DESKTOP, pnmhdr->hwndFrom, &lvhti.pt, 1);

    ListView_HitTest(pnmhdr->hwndFrom, &lvhti);

    if (lvhti.flags & LVHT_ONITEMSTATEICON)
    {
        ToggleState(pnmhdr->hwndFrom, lvhti.iItem);
    }
}


//
//  When the user hits the space bar, toggle the state of the selected item.
//
BOOL ConfOptKeyDown(HWND hDlg, LV_KEYDOWN *plvkd)
{
    int iCursor;

    if (plvkd->wVKey == VK_SPACE && !(GetAsyncKeyState(VK_MENU) < 0))
    {
        //
        // Lets toggle the cursored item.
        //

        iCursor = ListView_GetNextItem(plvkd->hdr.hwndFrom, -1, LVNI_FOCUSED);

        if (iCursor != -1)
        {
            ToggleState(plvkd->hdr.hwndFrom, iCursor);
        }
        return TRUE;
    }
    return FALSE;
}


//
//  Item selection changed.  Update tip.
//

void ItemChanged(LPWIZDATA lpwd, LPNM_LISTVIEW lpnmlv)
{
    LV_ITEM lvi;
    TCHAR   szTip[200]; ///???

    if ((lpnmlv->uNewState & LVIS_FOCUSED) &&
        (!(lpnmlv->uOldState & LVIS_FOCUSED)))
    {
        lvi.iItem = lpnmlv->iItem;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;

        ListView_GetItem(lpnmlv->hdr.hwndFrom, &lvi);

        GetOptVal(lpwd, (int)lvi.lParam, c_szRegDosOptTip, szTip, sizeof(szTip));

        Static_SetText(GetDlgItem(lpwd->hwnd, IDC_OPTIONTIP), szTip);
    }
}


void _inline NoSupportMsg(LPWIZDATA lpwd, int iOpt)
{
    LPTSTR lpszDesc = (LPTSTR)LocalAlloc(LMEM_FIXED, MAX_DESC_SIZE*sizeof(TCHAR));

    if (lpszDesc)
    {
        LPTSTR lpszMsg = (LPTSTR)LocalAlloc(LMEM_FIXED, 512*sizeof(TCHAR)); // Max 2 resource size

        if (lpszMsg)
        {
            GetOptVal(lpwd, iOpt, NULL, lpszDesc, MAX_DESC_SIZE*sizeof(TCHAR));
            LoadString(g_hinst, IDS_NOSUPPORT1, lpszMsg, 512);
            LoadString(g_hinst, IDS_NOSUPPORT2, lpszMsg+lstrlen(lpszMsg), 256);
            ShellMessageBox(g_hinst, lpwd->hwnd,
                            lpszMsg, 0,
                            MB_OK | MB_ICONEXCLAMATION,
                            lpszDesc);
            LocalFree(lpszMsg);
        }
        LocalFree(lpszDesc);
    }
}


//
//  Returns OPTSEL_YES if the option indicated by i is selected in the list
//  box, or, if hwndLV is NULL then OPTSEL_YES if option should be added,
//  OPTSEL_NO if should not be added, and OPTSEL_NOTSUPP if the option is
//  required but can't be added.
//

int OptSelected(LPWIZDATA lpwd, int i, HWND hwndLV)
{
    BOOL bSelected = FALSE;

    if (lpwd->DosOpt[i].dwFlags & DOSOPTF_ALWAYSUSE)
    {
        bSelected = TRUE;
    }
    else
    {
        if (hwndLV)
        {
            LV_ITEM lvi;
            int     NumItems = ListView_GetItemCount(hwndLV);

            for (lvi.iItem = 0; lvi.iItem < NumItems; lvi.iItem++)
            {
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM | LVIF_STATE;
                lvi.stateMask = LVIS_STATEIMAGEMASK;
                ListView_GetItem(hwndLV, &lvi);
                if ((int)lvi.lParam == i)
                {
                    bSelected = ((lvi.state & LVIS_STATEIMAGEMASK) == LVIS_GCCHECK);
                }
            }
        }
        else
        {
            BOOL bSupported = lpwd->DosOpt[i].dwFlags & DOSOPTF_SUPPORTED;

            if (lpwd->PropPrg.dwRealModeFlags & lpwd->DosOpt[i].dwStdOpt)
            {
                bSelected = TRUE;

                if (!bSupported)
                {
                    NoSupportMsg(lpwd, i);
                    return(OPTSEL_NOTSUPP);
                }
            }
            else
            {
                if (bSupported)
                {
                    bSelected = (lpwd->PropPrg.dwRealModeFlags & (lpwd->DosOpt[i].dwStdOpt >> 16));
                }
            }
        }
    }

    if (bSelected)
    {
        return(OPTSEL_YES);
    }
    else
    {
        return(OPTSEL_NO);
    }
}


void AppendStr(LPTEXTDATA lpTD, LPTSTR lpStr)
{
    int cb = lstrlen(lpStr)*sizeof(TCHAR);

    if ((lpTD->cb + cb + 2) <= MAX_CFG_FILE_SIZE*sizeof(TCHAR))
    {
        memcpy(lpTD->lpszData+lpTD->cb, lpStr, cb);
        lpTD->cb += cb;
    }
}


//
//  Appends the string+cr/lf to the TextData structure.
//

void AppendLine(LPTEXTDATA lpTD, LPTSTR lpStr)
{
    AppendStr(lpTD, lpStr);
    AppendStr(lpTD, (LPTSTR)c_szCRLF);
}


//
//  Returns NULL if none of the strings in szKeys matches the first entry
//  in the specified string.
//

LPTSTR MatchesKey(LPTSTR lpszStr, LPTSTR lpszKeys)
{
    UINT    cb = lstrlen(lpszStr);
    TCHAR   szUpLine[20];
    LPTSTR  lpszCurKey;

    if (cb >= ARRAYSIZE(szUpLine))
    {
        cb = ARRAYSIZE(szUpLine)-1;
    }

    memcpy(szUpLine, lpszStr, cb*sizeof(TCHAR));

    szUpLine[cb] = 0;

    CharUpper(szUpLine);

    for (lpszCurKey = lpszKeys; *lpszCurKey; lpszCurKey = SkipStr(lpszCurKey))
    {
        UINT cbKey = lstrlen(lpszCurKey);

        if ((cbKey < cb) &&
            ((szUpLine[cbKey] == TEXT(' ')) || (szUpLine[cbKey] == TEXT('='))) &&
            (memcmp(lpszCurKey, szUpLine, cbKey*sizeof(TCHAR)) == 0))
        {
            return(lpszCurKey);
        }
    }
    return(NULL);
}


//
//  Copy the current environment into Autoexec.Bat
//

void CopyEnvironment(LPTEXTDATA lpAE)
{
    TCHAR   szKeys[MAX_PATH];         //BUGBUG- SIZE?
    TCHAR   szSetCmd[20];
    LPTSTR  lpszCur = (LPTSTR)GetEnvironmentStrings();

    LoadString(g_hinst, IDS_SETCMD, szSetCmd, ARRAYSIZE(szSetCmd));
    LoadAndStrip(IDS_NOCOPYENV, szKeys, ARRAYSIZE(szKeys));

    while (*lpszCur)
    {
        if (!MatchesKey(lpszCur, szKeys))
        {
            AppendStr(lpAE, szSetCmd);
            AppendLine(lpAE, lpszCur);
        }

        lpszCur = SkipStr(lpszCur);
    }
}


//
//  Add keyboard type option for JKEYB.SYS
//

void SetJkeybOpt(LPTSTR lpszStr)
{
    UINT    cb, cbKey;
    TCHAR   szUpLine[64];
    TCHAR   lpszKey[] = TEXT("JKEYB.SYS");
    TCHAR   *lpszOpts[] = {TEXT(" /101"), TEXT(" /AX"), TEXT(" /106"), TEXT(" /J31DT"), TEXT(" /J31NB"), TEXT(" /J31LT")};
    int     KeybOpt;
    int     i;

    cb = lstrlen(lpszStr);
    cbKey = lstrlen(lpszKey);

    if (cb >= ARRAYSIZE(szUpLine))
    {
        cb = ARRAYSIZE(szUpLine)-1;
    }

    memcpy(szUpLine, lpszStr, cb*sizeof(TCHAR));

    szUpLine[cb] = 0;

//    AnsiUpper(szUpLine);
    CharUpper(szUpLine);

    for (i = 0; cbKey <= cb; i++, cb--)
    {
        if (memcmp(lpszKey, &szUpLine[i], cbKey*sizeof(TCHAR)) == 0)
        {

//            if (GetKeyboardType(0) == 7){
//                switch(GetKeyboardType(1)) {

            if (GetPrivateProfileInt(TEXT("keyboard"), TEXT("type"), 0, TEXT("SYSTEM.INI")) == 7)
            {
                switch(GetPrivateProfileInt(TEXT("keyboard"), TEXT("subtype"), 0, TEXT("SYSTEM.INI")))
                {
                    case 0:
                        KeybOpt = 0;
                        break;
                    case 1:
                        KeybOpt = 1;
                        break;
                    case 2:
                    case 3:
                    case 4:
                        KeybOpt = 2;
                        break;
                    case 13:
                        KeybOpt = 3;
                        break;
                    case 14:
                        KeybOpt = 4;
                        break;
                    case 15:
                        KeybOpt = 5;
                        break;
                    default:
                        KeybOpt = 0;
                        break;
                }
            }
            else
                KeybOpt = 0;

            lstrcat(lpszStr, lpszOpts[KeybOpt]);
            break;
        }
    }
}


//
//  Adds an option line from the specified value name to the TEXTDATA structure
//

BOOL AddOption(HKEY hk, LPCTSTR lpszValName, LPTEXTDATA lpTD,
               LPTSTR lpszHighCmds, LPTSTR lpszLowCmd, BOOL bCanLoadHigh)
{
    TCHAR  szOptData[256];
    UINT   cb = sizeof(szOptData);
    DWORD  dwType;
    LCID   lcid = GetThreadLocale();

    if (RegQueryValueEx(hk, lpszValName, NULL,
            &dwType, (LPBYTE) szOptData, &cb) == ERROR_SUCCESS)
    {
        LPTSTR  lpszAddData = szOptData;

        DoEnvironmentSubst(szOptData, ARRAYSIZE(szOptData));

        //
        //  Now remove LH or LoadHigh or DeviceHigh and replace with the
        //  appropriate string if EMM386 has not loaded yet.
        //

        if (!bCanLoadHigh)
        {
            LPTSTR lpszMatch = MatchesKey(szOptData, lpszHighCmds);

            if (lpszMatch)
            {
                int cbHigh = lstrlen(lpszMatch);
                if (lpszLowCmd)
                {
                    int cbLow = lstrlen(lpszLowCmd);

                    lpszAddData += cbHigh - cbLow;

                    memcpy(lpszAddData, lpszLowCmd, cbLow*sizeof(TCHAR));

                }
                else
                {
                    lpszAddData += cbHigh + 1;
                }
            }
        }

        if (PRIMARYLANGID(LANGIDFROMLCID(lcid))==LANG_JAPANESE)
        {
            SetJkeybOpt(lpszAddData);
        }
        AppendLine(lpTD, lpszAddData);

        return(TRUE);
    }
    return(FALSE);
}


//
//  Sets the appropriate configuration options in the PIF proprties
//  If fForceCleanCfg is TRUE then a clean configuration is created.  Otherwise
//  the autoexec and config fields are nuked to force the app to use the current
//  configuration.
//

PIFWIZERR SetConfOptions(LPWIZDATA lpwd, HWND hwndOptions, BOOL fForceCleanCfg)
{
    TEXTDATA  AE;
    TEXTDATA  CS;
    int       i;
    PIFWIZERR err = PIFWIZERR_SUCCESS;
    TCHAR     szCSHighCmds[100];
    TCHAR     szCSLowCmd[20];
    TCHAR     szAEHighCmds[100];
    BOOL      bCanLoadHigh = FALSE;

    //
    // Make sure the real mode flag is set in the program properties.
    //

    PifMgr_GetProperties(lpwd->hProps, (LPSTR)GROUP_PRG, &(lpwd->PropPrg),
                         sizeof(lpwd->PropPrg), GETPROPS_NONE);

    lpwd->PropPrg.flPrgInit |= PRGINIT_REALMODE;

    PifMgr_SetProperties(lpwd->hProps, (LPSTR)GROUP_PRG, &(lpwd->PropPrg),
                         sizeof(lpwd->PropPrg), SETPROPS_NONE);

    if (!fForceCleanCfg)
    {
        TCHAR NullStr = 0;

        PifMgr_SetProperties(lpwd->hProps, AUTOEXECHDRSIG40, &NullStr, 0, SETPROPS_NONE);
        PifMgr_SetProperties(lpwd->hProps, CONFIGHDRSIG40,   &NullStr, 0, SETPROPS_NONE);
        return(err);
    }

    //
    // Load strings used to force drivers to load low if no EMM386.
    //

    LoadString(g_hinst, IDS_CSLOWSTR, szCSLowCmd, ARRAYSIZE(szCSLowCmd));
    LoadAndStrip(IDS_CSHIGHSTRS, szCSHighCmds, ARRAYSIZE(szCSHighCmds));
    LoadAndStrip(IDS_AEHIGHSTRS, szAEHighCmds, ARRAYSIZE(szAEHighCmds));

    //
    // Allocate memory for autoexec/config.sys buffers.
    //

    AE.lpszData = (LPTSTR)LocalAlloc(LPTR, MAX_CFG_FILE_SIZE*sizeof(TCHAR));

    if (AE.lpszData == NULL)
    {
        return(PIFWIZERR_OUTOFMEM);
    }

    CS.lpszData = (LPTSTR)LocalAlloc(LPTR, MAX_CFG_FILE_SIZE*sizeof(TCHAR));

    if (CS.lpszData == NULL)
    {
        LocalFree(AE.lpszData);
        return(PIFWIZERR_OUTOFMEM);
    }

    AE.cb = CS.cb = 0;

    //
    // Copy the appropriate goop out of config.sys and autoexec.bat
    //

    CopyEnvironment(&AE);

    for (i = 0; i < lpwd->NumOpts; i++)
    {
        int OptSel = OptSelected(lpwd, i, hwndOptions);

        if (OptSel == OPTSEL_YES)
        {
            bCanLoadHigh |= (lpwd->DosOpt[i].dwFlags & DOSOPTF_PROVIDESUMB);

            AddOption(lpwd->DosOpt[i].hk, c_szRegValAutoexec,  &AE,
                      szAEHighCmds, NULL, bCanLoadHigh);

            AddOption(lpwd->DosOpt[i].hk, c_szRegValConfigSys, &CS,
                      szCSHighCmds, szCSLowCmd, bCanLoadHigh);

            //
            // DOSOPTF_MULTIPLE will load multiple configuration from
            // Config.Sys1 - Config.Sys9 and Autoexec.Bat1 - Autoexec.Bat9
            //

            if (lpwd->DosOpt[i].dwFlags & DOSOPTF_MULTIPLE)
            {
                BOOL  ret;
                TCHAR multicount[2];
                TCHAR multiAutoexec[64];
                TCHAR multiConfig[64];

                lstrcpy(multicount,TEXT("1"));

                while (1)
                {
                    lstrcpy(multiAutoexec, c_szRegValAutoexec);
                    lstrcat(multiAutoexec, multicount);
                    lstrcpy(multiConfig, c_szRegValConfigSys);
                    lstrcat(multiConfig, multicount);

                    ret = AddOption(lpwd->DosOpt[i].hk, multiAutoexec,  &AE,
                                     szAEHighCmds, NULL, bCanLoadHigh);

                    ret |= AddOption(lpwd->DosOpt[i].hk, multiConfig, &CS,
                                     szCSHighCmds, szCSLowCmd, bCanLoadHigh);

                    if (!ret)
                        break;

                    multicount[0] += 1;

                    if (multicount[0] > TEXT('9'))
                        break;
                }
            }
        }
        else
        {
            if (OptSel == OPTSEL_NOTSUPP)
            {
                err = PIFWIZERR_UNSUPPORTEDOPT;
            }
        }
    }

    //
    //  Set the properties in the PIF file
    //

    PifMgr_SetProperties(lpwd->hProps, AUTOEXECHDRSIG40, AE.lpszData, AE.cb, SETPROPS_NONE);
    PifMgr_SetProperties(lpwd->hProps, CONFIGHDRSIG40, CS.lpszData, CS.cb, SETPROPS_NONE);

    //
    //  Clean up allocated memory
    //

    LocalFree(AE.lpszData);
    LocalFree(CS.lpszData);

    return(err);
}


//
//  The user hit the finish button.  Set the proper configuration.
//

PIFWIZERR ConfigRealModeOptions(LPWIZDATA lpwd, HWND hwndOptList,
                                UINT uAction)
{
    PIFWIZERR err = PIFWIZERR_GENERALFAILURE;
    BOOL      fCleanCfg;

    switch(uAction)
    {
        case CRMOACTION_DEFAULT:
            fCleanCfg = GetMSDOSOptGlobalFlags(lpwd) & DOSOPTGF_DEFCLEAN;
            break;

        case CRMOACTION_CLEAN:
            fCleanCfg = TRUE;
            break;

        case CRMOACTION_CURRENT:
            fCleanCfg = FALSE;
            break;
    }

    //
    // OK to call twice -- Returns true
    //

    if (ReadRegInfo(lpwd))
    {
        if (lpwd->hProps == 0)
        {
            if (CreateLink(lpwd))
            {
                TCHAR szLinkName[MAX_PATH];

                GetLinkName(szLinkName, lpwd);

                lpwd->hProps = PifMgr_OpenProperties(szLinkName, NULL, 0, OPENPROPS_NONE);

                if (lpwd->hProps)
                {
                    err = SetConfOptions(lpwd, hwndOptList, fCleanCfg);

                    PifMgr_CloseProperties(lpwd->hProps, CLOSEPROPS_NONE);

                    lpwd->hProps = 0;
                }
            }
        }
        else
        {
            err = SetConfOptions(lpwd, hwndOptList, fCleanCfg);
        }

        FreeRegInfo(lpwd);
    }
    return(err);
}


BOOL_PTR CALLBACK ConfigOptionsDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;
    LPPROPSHEETPAGE lpp = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPWIZDATA lpwd = lpp ? (LPWIZDATA)lpp->lParam : NULL;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case LVN_ITEMCHANGED:
                    ItemChanged(lpwd, (LPNM_LISTVIEW)lParam);
                    break;

                case NM_CLICK:
                case NM_DBLCLK:
                    ConfOptClick(hDlg, lpnm);
                    break;

                case LVN_KEYDOWN:
                    SetDlgMsgResult(hDlg, WM_NOTIFY,
                                    ConfOptKeyDown(hDlg, (LV_KEYDOWN *)lParam));
                    break;


                case PSN_SETACTIVE:
                    lpwd->hwnd = hDlg;

                    if (lpwd->dwFlags & WDFLAG_PIFPROP)
                    {
                        TCHAR szOK[20];

                        LoadString(g_hinst, IDS_OK, szOK, ARRAYSIZE(szOK));
                        PropSheet_SetFinishText(GetParent(hDlg), szOK);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hDlg),
                                                 PSWIZB_FINISH | PSWIZB_BACK);
                    }
                    break;

                case PSN_WIZFINISH:
                    ConfigRealModeOptions(lpwd, GetDlgItem(hDlg, IDC_OPTIONLIST),
                                          CRMOACTION_CLEAN);
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                default:
                    return FALSE;
            }
            break;


        case WM_INITDIALOG:
            ConfOptInit(hDlg, (LPPROPSHEETPAGE)lParam);
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

void WhichConfInit(HWND hDlg, LPPROPSHEETPAGE lpp)
{
    LPWIZDATA lpwd = InitWizSheet(hDlg, (LPARAM)lpp, 0);

    CheckRadioButton(hDlg, IDB_CURCFG, IDB_CLEANCFG,
                     GetMSDOSOptGlobalFlags(lpwd) & DOSOPTGF_DEFCLEAN ?
                     IDB_CLEANCFG : IDB_CURCFG);
}


void SetChoiceWizBtns(LPWIZDATA lpwd)
{
    PropSheet_SetWizButtons(GetParent(lpwd->hwnd),
                            IsDlgButtonChecked(lpwd->hwnd, IDB_CLEANCFG) ?
                            PSWIZB_NEXT | PSWIZB_BACK :
                            PSWIZB_FINISH | PSWIZB_BACK);
}


BOOL_PTR CALLBACK PickConfigDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;
    LPPROPSHEETPAGE lpp = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPWIZDATA lpwd = lpp ? (LPWIZDATA)lpp->lParam : NULL;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    lpwd->hwnd = hDlg;
                    if (MustRebootSystem())
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    else
                    {
                        SetChoiceWizBtns(lpwd);
                    }
                    break;

                case PSN_WIZFINISH:
                    ConfigRealModeOptions(lpwd, GetDlgItem(hDlg, IDC_OPTIONLIST),
                                          CRMOACTION_CURRENT);
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDB_CURCFG:
                case IDB_CLEANCFG:
                    SetChoiceWizBtns(lpwd);
            }
            break;

        case WM_INITDIALOG:
            WhichConfInit(hDlg, (LPPROPSHEETPAGE)lParam);
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}
