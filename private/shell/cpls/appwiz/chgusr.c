//
//  Chgusr.C
//
//  Copyright (C) Citrix, 1996 All Rights Reserved.
//
//  History:
//  scottn 11/19/96 - First pass
//
//  scottn 12/5/96  - Add storage of chgusr option into registry.
//
//  scottn 12/13/96 - Create the UNINSTALL key if necessary (upon
//			first install of an uninstallable)
//
//  scottn 12/17/96 - Remove cwait (hangs on 16-bit installs).  Now
//			just exec and go to next page.  Add Finish page
//			which will turn option back and end tracking thread.
//
#include "priv.h"
#ifndef DOWNLEVEL_PLATFORM
#ifdef WINNT
#include "appwiz.h"
#include "regstr.h"
#include <uastrfnc.h>
#include <stdio.h>
#include <process.h>
#include <tsappcmp.h>       // for TermsrvAppInstallMode

//
//  Initialize the chgusr property sheet.  Check the "install" radio control.
//

void ChgusrFinishInitPropSheet(HWND hDlg, LPARAM lParam)
{
    LPWIZDATA lpwd = InitWizSheet(hDlg, lParam, 0);
}

void ChgusrFinishPrevInitPropSheet(HWND hDlg, LPARAM lParam)
{
    LPWIZDATA lpwd = InitWizSheet(hDlg, lParam, 0);
}

//
//  Sets the appropriate wizard buttons.
//
void SetChgusrFinishButtons(LPWIZDATA lpwd)
{
    // no BACK button so that they don't relaunch the app and
    // start a new thread, etc.

    int iBtns = PSWIZB_FINISH | PSWIZB_BACK;

    PropSheet_SetWizButtons(GetParent(lpwd->hwnd), iBtns);
}

void SetChgusrFinishPrevButtons(LPWIZDATA lpwd)
{
    // no BACK button so that they don't relaunch the app and
    // start a new thread, etc.

    int iBtns = PSWIZB_NEXT;

    PropSheet_SetWizButtons(GetParent(lpwd->hwnd), iBtns);
}

//
//  NOTES: 1) This function assumes that lpwd->hwnd has already been set to
//           the dialogs hwnd.
//

void ChgusrFinishSetActive(LPWIZDATA lpwd)
{
    if (lpwd->dwFlags & WDFLAG_SETUPWIZ)
    {
        TCHAR szInstruct[MAX_PATH];

        LoadString(g_hinst, IDS_CHGUSRFINISH, szInstruct, ARRAYSIZE(szInstruct));

        Static_SetText(GetDlgItem(lpwd->hwnd, IDC_SETUPMSG), szInstruct);
    }

    SetChgusrFinishButtons(lpwd);

    PostMessage(lpwd->hwnd, WMPRIV_POKEFOCUS, 0, 0);
}

void ChgusrFinishPrevSetActive(LPWIZDATA lpwd)
{
    if (lpwd->dwFlags & WDFLAG_SETUPWIZ)
    {
        TCHAR szInstruct[MAX_PATH];

        LoadString(g_hinst, IDS_CHGUSRFINISH_PREV, szInstruct, ARRAYSIZE(szInstruct));

        Static_SetText(GetDlgItem(lpwd->hwnd, IDC_SETUPMSG), szInstruct);
    }

    SetChgusrFinishPrevButtons(lpwd);

    PostMessage(lpwd->hwnd, WMPRIV_POKEFOCUS, 0, 0);
}

//
//  Main dialog procedure for fourth page of setup wizard.
//

BOOL_PTR CALLBACK ChgusrFinishPrevDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
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
                    ChgusrFinishPrevSetActive(lpwd);
                    break;

                case PSN_WIZNEXT:
                    break;

                case PSN_RESET:
                    SetTermsrvAppInstallMode(lpwd->bPrevMode);
                    
                    CleanUpWizData(lpwd);

                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            ChgusrFinishPrevInitPropSheet(hDlg, lParam);
            break;

        case WMPRIV_POKEFOCUS:
        {
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

                case IDC_COMMAND:
                    break;

            } // end of switch on WM_COMMAND
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}  // ChgusrFinishDlgProc

//
//  Main dialog procedure for last page of setup wizard.
//

BOOL_PTR CALLBACK ChgusrFinishDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
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
                    ChgusrFinishSetActive(lpwd);
                    break;

                case PSN_WIZFINISH:
                case PSN_RESET:
                {
                    SetTermsrvAppInstallMode(lpwd->bPrevMode);

                    if (lpnm->code == PSN_RESET)
                        CleanUpWizData(lpwd);
                    break;
                }

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            ChgusrFinishInitPropSheet(hDlg, lParam);
            break;

        case WMPRIV_POKEFOCUS:
        {
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

                case IDC_COMMAND:
                    break;

            } // end of switch on WM_COMMAND
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}  // ChgusrFinishDlgProc


#endif // WINNT
#endif // DOWNLEVEL_PLATFORM
