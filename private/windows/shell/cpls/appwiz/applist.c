//
//  AppList.C
//
//  Copyright (C) Microsoft, 1994, 1995, All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  ral 9/09/94 - Clean up
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//

#include "appwiz.h"
#include "regstr.h"

static const DWORD aApplistHelpIDs[] = {
    IDC_BUTTONSETUPFROMLIST, IDH_APPWIZ_NETINTALLL_BUTTON,
    IDC_APPLIST, IDH_APPWIZ_NETINSTALL_LIST,
    0, 0 };

#define INST_SECTION    TEXT("AppInstallList")   // BUGBUG -- RESOURCE!!!
#define MAX_KEY_SIZE    45000

//
//  Fills in the name of the INF file in the wiz data structure.  If
//  no INF file is specified then returns FALSE, else TRUE.
//

BOOL AppListGetInfName(LPWIZDATA lpwd)
{
    HKEY    hk;
    LONG    RegResult;
    DWORD   cbFileName = sizeof(lpwd->szIniFile);
    DWORD   dwType;
    BOOL    bFileExists = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, &hk) != ERROR_SUCCESS)
    {
        return(FALSE);
    }
    RegResult = RegQueryValueEx(hk, REGSTR_VAL_APPINSTPATH, NULL,
                                &dwType, (LPBYTE) lpwd->szIniFile, &cbFileName);
    RegCloseKey(hk);

    if (RegResult == ERROR_SUCCESS && dwType == REG_SZ)
    {
        bFileExists = PathFileExists(lpwd->szIniFile);
    }

    if (!bFileExists)
    {
        lpwd->szIniFile[0] = 0;
    }

    return(bFileExists);
}


#ifdef DEBUG
void ValidateINIEntry(LPWIZDATA lpwd, LPTSTR lpszKeyName)
{
    TCHAR szFileName[MAX_PATH];

    if (GetPrivateProfileString(INST_SECTION, lpszKeyName, TEXT(""),
                      szFileName, ARRAYSIZE(szFileName),
                      lpwd->szIniFile))
    {
        LPTSTR lpszRealName = szFileName;

        if (*lpszRealName == TEXT('*'))
        {
            lpszRealName++;
        }

        if (!PathFileExists(lpszRealName))
        {
            ShellMessageBox(hInstance, lpwd->hwnd,
                            TEXT("Entry for %1%s points to non-existant setup program: %2%s"),
                            0, MB_OK | MB_ICONEXCLAMATION, lpszKeyName, szFileName);
        }
    }
    else
    {
        ShellMessageBox(hInstance, lpwd->hwnd,
                        TEXT("Bad INI file format for entry %1%s."),
                        0, MB_OK | MB_ICONEXCLAMATION, lpszKeyName);
    }
}
#endif // DEBUG


//
//  Initializes the applist property sheet.  This function assumes that
//  someone else (appwiz.c) has already called AppListGetInfName and the
//  inf file name has already been filled in in the wizard data structure.
//  If the string is empty, then this function simply returns.
//

void AppList_InitListBox(HWND hDlg, LPPROPSHEETPAGE lpp)
{
    LPWIZDATA lpwd = (LPWIZDATA)lpp->lParam;

    SetWindowLong(hDlg, DWL_USER, (LPARAM)lpp);

    if (lpwd->szIniFile[0] != 0)
    {
        HWND   hLB = GetDlgItem(hDlg, IDC_APPLIST);
        LPTSTR lpszKeys = (LPTSTR)GlobalAllocPtr(GPTR, MAX_KEY_SIZE*sizeof(TCHAR));

        ListBox_ResetContent(hLB);

        //
        //  If the localalloc failed then we'll just have a stupid looking
        //  empty list.
        //

        if (lpszKeys)
        {
            HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

            if (GetPrivateProfileString(INST_SECTION, NULL, NULL, lpszKeys,
                              MAX_KEY_SIZE, lpwd->szIniFile))
            {
                LPTSTR lpszCurPos = lpszKeys;
                int    iCurLBPos = 0;

                while (*lpszCurPos)
                {
                    ListBox_InsertString(hLB, iCurLBPos++, lpszCurPos);
#ifdef DEBUG
                    ValidateINIEntry(lpwd, lpszCurPos);
#endif
                    while (*lpszCurPos != 0)
                    {
                        lpszCurPos = CharNext(lpszCurPos);
                    }
                    lpszCurPos++;
                }
            }
            GlobalFreePtr(lpszKeys);
            SetCursor(hcurOld);
        }
        ListBox_SetCurSel(hLB, 0);
    }
}


//
//  Copies the name of the setup program into the wizard data
//

BOOL GetListSel(HWND hLB, int iCurSel, LPWIZDATA lpwd)
{
    TCHAR szKeyName[MAX_PATH];

    if ((iCurSel != LB_ERR) &&
        (ListBox_GetTextLen(hLB, iCurSel) <= ARRAYSIZE(szKeyName)))
    {
        ListBox_GetText(hLB, iCurSel, szKeyName);

        if (GetPrivateProfileString(INST_SECTION, szKeyName, TEXT(""),
                          lpwd->szExeName, ARRAYSIZE(lpwd->szExeName),
                          lpwd->szIniFile))
        {
            lpwd->szParams[0] = 0;  // Make sure this string is empty
            return(TRUE);
        }
    }
    return(FALSE);
}


//
//  Executes the appropriate setup program
//

BOOL ExecSetupProg(LPWIZDATA lpwd, BOOL ForceWx86)
{
    SHELLEXECUTEINFO ei;
    BOOL fWorked= FALSE;

#ifdef WX86
    DWORD  Len;
    WCHAR  ProcArchValue[32];
#endif

    HWND hDlgPropSheet = GetParent(lpwd->hwnd);

    ei.cbSize = sizeof(ei);
    ei.hwnd = lpwd->hwnd;
    ei.lpVerb = NULL;
    ei.fMask = 0;

    if (lpwd->szExeName[0] == TEXT('*'))
    {
        ei.lpFile = CharNext(lpwd->szExeName);
        ei.fMask |= SEE_MASK_CONNECTNETDRV;
    }
    else
    {
        ei.lpFile = lpwd->szExeName;
    }

    if (lpwd->szParams[0] == 0)
    {
        ei.lpParameters = NULL;
    }
    else
    {
        ei.lpParameters = lpwd->szParams;
    }

    if (lpwd->szWorkingDir[0] == TEXT('\0'))
    {
        ei.lpDirectory = NULL;
    }
    else
    {
        ei.lpDirectory = lpwd->szWorkingDir;
    }

    ei.lpClass = NULL;
    ei.nShow = SW_SHOWDEFAULT;
    ei.hInstApp = hInstance;

    SetWindowPos(hDlgPropSheet, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);


#ifdef WX86
    if (ForceWx86) {
        Len = GetEnvironmentVariableW(ProcArchName,
                                      ProcArchValue,
                                      sizeof(ProcArchValue)
                                      );

        if (!Len || Len >= sizeof(ProcArchValue)) {
            ProcArchValue[0]=L'\0';
        }

        SetEnvironmentVariableW(ProcArchName, L"x86");
        ei.fMask |= SEE_MASK_FLAG_SEPVDM;

    }
#endif


    fWorked = ShellExecuteEx(&ei);


#ifdef WX86
    if (ForceWx86) {
        SetEnvironmentVariableW(ProcArchName, ProcArchValue);
    }
#endif



    if (!fWorked)
    {
        //
        // Something went wrong. Put the dialog back up.
        //

        SetWindowPos(hDlgPropSheet, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
        ShellMessageBox(hInstance, lpwd->hwnd, MAKEINTRESOURCE(IDS_BADSETUP),
                        0, MB_OK | MB_ICONEXCLAMATION);
    }

    return(fWorked);
}


//
//  Dismisses the property sheet by pressing the "OK" button.
//

void DismissCPL(LPWIZDATA lpwd)
{
    PropSheet_PressButton(GetParent(lpwd->hwnd), PSBTN_OK);
}


//
//  Install property sheet page -- Used only if there is an AppInstallPath
//  specified in the registry.        Otherwise, use NoListInstallDlgProc.
//

BOOL CALLBACK AppListDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));
    LPWIZDATA        lpwd;


    if (lpPropSheet)
    {
        lpwd = (LPWIZDATA)lpPropSheet->lParam;
    }
    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    lpwd->hwnd = hDlg;
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            AppList_InitListBox(hDlg, lpPropSheet);
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD)aApplistHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
                    (DWORD)aApplistHelpIDs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_BUTTONSETUPFROMLIST:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        HWND        hLB = GetDlgItem(hDlg, IDC_APPLIST);
                        int iCurSel = ListBox_GetCurSel(hLB);

                        if (iCurSel != LB_ERR &&
                            GetListSel(hLB, iCurSel, lpwd) &&
                            ExecSetupProg(lpwd, FALSE))
                        {
                            DismissCPL(lpwd);
                        }
                    }

                case IDC_APPLIST:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
                    {
                        SendMessage(hDlg, WM_COMMAND,
                                     GET_WM_COMMAND_MPS(IDC_BUTTONSETUPFROMLIST,
                                       GetDlgItem(hDlg, IDC_BUTTONSETUPFROMLIST),
                                       BN_CLICKED));
                    }
                    break;

            }
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;

}  // AppListdlgProc
