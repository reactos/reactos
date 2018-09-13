//
//  GetTitle.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"


//
//  Enables the appropriate buttons depending upon the state of the
//  description edit control and what type of shortcut we're trying to
//  make.
//

void EnableNextFinish(LPWIZDATA lpwd)
{
    DWORD dwEnable = PSWIZB_BACK;
    if (GetWindowTextLength(GetDlgItem(lpwd->hwnd, IDC_TITLE)) > 0)
    {
        //
        //  If this is a "known" application then enalble finish, else next.
        //

        dwEnable |= (lpwd->dwFlags & (WDFLAG_APPKNOWN | WDFLAG_COPYLINK)) ?
                                           PSWIZB_FINISH : PSWIZB_NEXT;
    }
    PropSheet_SetWizButtons(GetParent(lpwd->hwnd), dwEnable);
}


//
//  Called from PSN_SETACTIVE.        Assumes lpwd->hwnd already initialized.
//

void GetTitleSetActive(LPWIZDATA lpwd)
{
    //
    // Most of the code to process this was moved into the Next button
    // processing of the previous page as there were some failure cases
    // that we could not get a title that we should detect before we
    // allow the user to change to this page...  HOWEVER, there are some
    // cases where we can't determine the name until we get to this page.
    // If we don't have a name for the sortcut, try to figure one out here.
    //

    if (lpwd->szProgDesc[0] == 0)
    {
        DetermineDefaultTitle(lpwd);
    }

    SetDlgItemText(lpwd->hwnd, IDC_TITLE, lpwd->szProgDesc);
    EnableNextFinish(lpwd);
    PostMessage(lpwd->hwnd, WMPRIV_POKEFOCUS, 0, 0);
}


//
//  Check to see if link name is a duplicate.  If it is then ask the user
//  if they want to replace the old link.  If they say "no" then this function
//  returns FALSE.
//

BOOL GetTitleNextPushed(LPWIZDATA lpwd)
{
    TCHAR szLinkName[MAX_PATH];

    GetDlgItemText(lpwd->hwnd, IDC_TITLE, lpwd->szProgDesc, ARRAYSIZE(lpwd->szProgDesc));
    if (lpwd->szProgDesc[0] == 0)
    {
        return(FALSE);
    }

    if( ( PathCleanupSpec( lpwd->lpszFolder, lpwd->szProgDesc ) != 0 ) ||
        !GetLinkName( szLinkName, lpwd ) )
    {
        ShellMessageBox(hInstance, lpwd->hwnd, MAKEINTRESOURCE(IDS_MODNAME),
                        0, MB_OK | MB_ICONEXCLAMATION);
        return(FALSE);
    }

    if (PathFileExists(szLinkName))
    {
        //
        //  Obscure boundary case.  If we're creating a new link and the user
        //  happens to want to name it exactly it's current name then we'll let
        //  them do it without a warning.
        //

        if (lpwd->lpszOriginalName && lstrcmpi(lpwd->lpszOriginalName, szLinkName) == 0)
        {
            WIZERROR(TEXT("Unbelieveable!  User selected exactly the same name"));
            return(TRUE);
        }
        return(IDYES == ShellMessageBox(hInstance, lpwd->hwnd,
                                    MAKEINTRESOURCE(IDS_DUPLINK), 0,
                                    MB_YESNO | MB_DEFBUTTON1 | MB_ICONHAND,
                                    lpwd->szProgDesc));
    }
    return(TRUE);
}


//
//  Dialog procedure for title dialog
//
BOOL CALLBACK GetTitleDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
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
                    GetTitleSetActive(lpwd);
                    break;

                case PSN_WIZNEXT:
                    if (!GetTitleNextPushed(lpwd))
                    {
                        GetTitleSetActive(lpwd);
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    break;

                case PSN_WIZFINISH:
                    {
                        int        iResult = -1;

                        if (GetTitleNextPushed(lpwd))
                        {
                            if (lpwd->dwFlags & WDFLAG_SINGLEAPP)
                            {
                                PIFWIZERR err = ConfigRealModeOptions(lpwd, NULL,
                                                    CRMOACTION_DEFAULT);

                                if (err == PIFWIZERR_SUCCESS ||
                                    err == PIFWIZERR_UNSUPPORTEDOPT)
                                {
                                    iResult = 0;
                                }
                            }
                            else
                            {
                                if (CreateLink(lpwd))
                                {
                                    iResult = 0;
                                }
                            }
                        }
                        if (iResult != 0)
                        {
                            GetTitleSetActive(lpwd);
                        }
                        SetDlgMsgResult(hDlg, WM_NOTIFY, iResult);
                    }
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            lpwd = InitWizSheet(hDlg, lParam, 0);
            Edit_LimitText(GetDlgItem(hDlg, IDC_TITLE), ARRAYSIZE(lpwd->szProgDesc)-1);
            break;

        case WMPRIV_POKEFOCUS:
            {
            HWND hTitle = GetDlgItem(hDlg, IDC_TITLE);
            SetFocus(hTitle);
            Edit_SetSel(hTitle, 0, -1);
            break;
            }

        case WM_DESTROY:
        case WM_HELP:
        case WM_CONTEXTMENU:
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDHELP:
                        break;

                case IDC_TITLE:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case EN_CHANGE:
                            EnableNextFinish(lpwd);
                            break;
                    }
                    break;

            } // end of switch on WM_COMMAND
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;

}  // GetTitleDlgProc
