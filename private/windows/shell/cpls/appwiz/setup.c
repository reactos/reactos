//
//  Setup.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"



void _inline InitSetupWiz(HWND hDlg, LPARAM lParam)
{
    InitWizSheet(hDlg, lParam, 0);
}


//
//  Loads the specified resource ID string and replaces all ';' characters
//  with NULL.        The end of the string will be doubly null-teminated.
//

void LoadAndStrip(int id, LPTSTR lpsz, int cbstr)
{
    LoadString(hInstance, id, lpsz, cbstr-1);

    while (*lpsz)
    {
        if (*lpsz == TEXT('@'))
        {
            *lpsz = 0;
            lpsz++;
        }
        else
        {
            lpsz = CharNext(lpsz);
        }
    }
    *(lpsz+1) = 0;
}


//
//  Skips to the first charcter of the next string in a list of null-terminated
//  strings.  The caller should check to see if the pointer returned points to
//  a null.  If so, the end of the table has been reached.
//

LPTSTR SkipStr(LPTSTR lpsz)
{
    while (*lpsz)
    {
        lpsz = CharNext(lpsz);
    }
    lpsz++;

    return(lpsz);
}


void SetStaticStr(HWND hCtl, int id)
{
    TCHAR szText[MAX_PATH];

    LoadString(hInstance, id, szText, ARRAYSIZE(szText));

    Static_SetText(hCtl, szText);
}


void FreeIcon(HWND hDlg)
{
    HICON hicon = Static_SetIcon(GetDlgItem(hDlg, IDC_SEARCHICON), NULL);

    if (hicon)
    {
        DestroyIcon(hicon);
    }
}


//
//  ProgramExists returns TRUE if the specified file exists.  This function
//  accepts wildcards, and if a file matches the specified name then
//  the file name buffer will be updated to the actual name of the first
//  matching file.  This allows FindBestSetupPrg to pass in *setup to find
//  programs such as WPSETUP.EXE.
//
//  This function assumes that szFindName is of size MAX_PATH.
//

BOOL ProgramExists(LPTSTR lpszFindName)
{
    HANDLE hfind;
    WIN32_FIND_DATA fd;

    hfind = FindFirstFile(lpszFindName, &fd);

    if (hfind == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    FindClose(hfind);

    lstrcpy(lpszFindName+3, fd.cFileName);
}


//
//  This function searches for the "best" setup program.  Once a windows app
//  with the appropriate name is found it stops.  If it finds a install/setup
//  program that is a DOS program, it remembers the first one, but continues
//  searching for a Windows setup program.
//  Games like Math Rabbit have a DOS Install.Exe and a Windows Setup.Exe.
//

BOOL FindBestSetupPrg(LPTSTR lpszExeName, LPTSTR lpszDriveRoot, LPTSTR lpszSpecialCase,
                      LPTSTR lpszAppNames, LPTSTR lpszExtensions)
{
    LPTSTR  lpszCurApp, lpszCurExt;
    TCHAR   szThisOne[MAX_PATH];

    *lpszExeName = 0;

    //
    //        Look for special-case programs first
    //

    lpszCurApp = lpszSpecialCase;

    while(*lpszCurApp)
    {
        lstrcpy(szThisOne, lpszDriveRoot);
        lstrcat(szThisOne, lpszCurApp);

        if (ProgramExists(szThisOne))
        {
            lstrcpy(lpszExeName, szThisOne);
            return(TRUE);
        }

        lpszCurApp = SkipStr(lpszCurApp);
    }

    //
    //        Now look for generic setup program names
    //

    lpszCurApp = lpszAppNames;

    while (*lpszCurApp)
    {
        lpszCurExt = lpszExtensions;

        while (*lpszCurExt)
        {
            lstrcpy(szThisOne, lpszDriveRoot);
            lstrcat(szThisOne, lpszCurApp);
            lstrcat(szThisOne, TEXT("."));
            lstrcat(szThisOne, lpszCurExt);

            if (ProgramExists(szThisOne))
            {
                BOOL fIsWinApp = HIWORD(SHGetFileInfo(szThisOne, 0, NULL,
                                                      0, SHGFI_EXETYPE)) > 0;

                if (*lpszExeName == 0 || fIsWinApp)
                {
                    lstrcpy(lpszExeName, szThisOne);
                }

                if (fIsWinApp)
                {
                    return(TRUE);
                }
            }

            lpszCurExt = SkipStr(lpszCurExt);
        }

        lpszCurApp = SkipStr(lpszCurApp);
    }

    return(*lpszExeName != 0);
}


//
//  Gets information about the specified file/drive root and sets the
//  icon and description fields in the dialog.
//

void _inline UpdateFileInfo(LPWIZDATA lpwd, LPTSTR lpszFileName)
{
    HWND        hKiddie;
    HICON       hOldIcon;
    SHFILEINFO  fi;

    SHGetFileInfo(lpszFileName, 0, &fi, sizeof(fi),
                  SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_LARGEICON);

    hKiddie = GetDlgItem(lpwd->hwnd, IDC_SEARCHICON);

    hOldIcon = Static_SetIcon(hKiddie, fi.hIcon);

    if (hOldIcon)
    {
        DestroyIcon(hOldIcon);
    }

    UpdateWindow(hKiddie);

    hKiddie = GetDlgItem(lpwd->hwnd, IDC_SEARCHNAME);

    Static_SetText(hKiddie, fi.szDisplayName);

    UpdateWindow(hKiddie);
}


//
//  Search for the setup program
//

BOOL SetupNextPressed(LPWIZDATA lpwd)
{
    int   iDrive, iDrvType;
    BOOL  fFoundExe = FALSE;
    HWND  hMainMsg = GetDlgItem(lpwd->hwnd, IDC_SETUPMSG);
    HWND  hSetupName = GetDlgItem(lpwd->hwnd, IDC_SEARCHNAME);
    TCHAR szAppNames[MAX_PATH];
    TCHAR szExtensions[100];
    TCHAR szSpecialCase[MAX_PATH];
    TCHAR szDriveRoot[4];

    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //BOOL fFoundDisk = FALSE;

    lpwd->szExeName[0] = 0;        // Reset any existing name

    SetStaticStr(hMainMsg, IDS_SEARCHING);

    LoadAndStrip(IDS_SETUPPRGNAMES, szAppNames,    ARRAYSIZE(szAppNames));
    LoadAndStrip(IDS_EXTENSIONS,    szExtensions,  ARRAYSIZE(szExtensions));
    LoadAndStrip(IDS_SPECIALCASE,   szSpecialCase, ARRAYSIZE(szSpecialCase));

    for (iDrive = 0; (!fFoundExe) && (iDrive < 26); iDrive++)
    {
        iDrvType = DriveType(iDrive);

        if ((iDrvType == DRIVE_REMOVABLE) || (iDrvType == DRIVE_CDROM))
        {

            PathBuildRoot(szDriveRoot, iDrive);
            UpdateFileInfo(lpwd, szDriveRoot);
            if (PathFileExists(szDriveRoot))
            {
                //fFoundDisk = TRUE;

                fFoundExe = FindBestSetupPrg(lpwd->szExeName, szDriveRoot,
                                             szSpecialCase,
                                             szAppNames, szExtensions);
            }
        }
    }

    FreeIcon(lpwd->hwnd);
    SetCursor(hcurOld);

    return(fFoundExe);
}


void SetupSetToDefault(LPWIZDATA lpwd)
{
    SetStaticStr(GetDlgItem(lpwd->hwnd, IDC_SETUPMSG), IDS_INSERTDISK);

    Static_SetText(GetDlgItem(lpwd->hwnd, IDC_SEARCHNAME), NULL);

    FreeIcon(lpwd->hwnd);

    PropSheet_SetWizButtons(GetParent(lpwd->hwnd), PSWIZB_NEXT);

    //
    // To make sure that the next button always has the focus, we post
    // this message that sets the wiz buttons AFTER we're active.  We have
    // to do the one above to make sure that Back is disabled to avoid any
    // random window where the back button could be hit.
    //

    PostMessage(lpwd->hwnd, WMPRIV_POKEFOCUS, 0, 0);
}



BOOL CALLBACK SetupDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));
    LPWIZDATA lpwd;

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
                    SetupSetToDefault(lpwd);
                    break;

                case PSN_WIZNEXT:
                    SetupNextPressed(lpwd);
                    SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WMPRIV_POKEFOCUS:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            break;

        case WM_INITDIALOG:
            InitSetupWiz(hDlg, lParam);
            break;

        case WM_DESTROY:
            FreeIcon(hDlg);
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;

}  // SetupdlgProc
