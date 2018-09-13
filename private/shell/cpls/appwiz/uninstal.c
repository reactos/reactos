//
//  Uninstal.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "priv.h"
#include "appwiz.h"
#include "regstr.h"
#include "dlinst.h"

#ifdef WX86
BOOL bWx86Enabled=FALSE;
BOOL bForceX86Env=FALSE;
const WCHAR ProcArchName[]=L"PROCESSOR_ARCHITECTURE";
#endif

#ifdef DOWNLEVEL

//////////////////////////////////////////////////////////////////////////////
//
// Unistall Page - the basic idea:
//
//  this page has a simple listbox displaying removable software components
//  the user can select one and hit the "remove" button
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// defines
//
//////////////////////////////////////////////////////////////////////////////

#define UNM_WAKEUP     ( WM_APP + 1 )   // from terminated worker thread


//////////////////////////////////////////////////////////////////////////////
//
// constant strings
//
//////////////////////////////////////////////////////////////////////////////

static const TCHAR *c_UninstallKey = REGSTR_PATH_UNINSTALL;
static const TCHAR *c_UninstallItemName = REGSTR_VAL_UNINSTALLER_DISPLAYNAME;
static const TCHAR *c_UninstallItemCommand = REGSTR_VAL_UNINSTALLER_COMMANDLINE;

const static DWORD aUninstallHelpIDs[] = {  // Context Help IDs
                IDC_BUTTONSETUP,     IDH_APPWIZ_DISKINTALLL_BUTTON,
                IDC_MODIFYUNINSTALL, IDH_APPWIZ_UNINSTALL_BUTTON,
                IDC_REGISTERED_APPS, IDH_APPWIZ_UNINSTALL_LIST,
                0, 0
                };

#define DISPLAYNAME_SIZE 64




//////////////////////////////////////////////////////////////////////////////
//
// Uninstall_FreeList -- empties list of removable applications
//
//  NOTE: the third parameter is the listbox to empty, which may be NULL
//
//////////////////////////////////////////////////////////////////////////////

void Uninstall_FreeList( HWND dlg, LPWIZDATA lpwd, HWND listbox )
{
    if( listbox )
        ListBox_ResetContent( listbox );

    if( lpwd->lpUItem )
    {
        LocalFree( (HANDLE)lpwd->lpUItem );
        lpwd->lpUItem = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Uninstall_RefreshList -- fills/refills the list of removable applications
//
//////////////////////////////////////////////////////////////////////////////

void Uninstall_RefreshList( HWND dlg, LPWIZDATA lpwd )
{
    DWORD   dwTotal;

    //
    // we wouldn't have gotten here if dlgmgr failed to create the listbox
    //

    HWND listbox = GetDlgItem( dlg, IDC_REGISTERED_APPS );

    //
    // avoid flicker when visible
    //

    SetWindowRedraw( listbox, FALSE );

    //
    // clean out any residual junk
    //

    Uninstall_FreeList( dlg, lpwd, listbox );

    //
    // Maximum size of lists of removable apps
    //

    dwTotal = 0;

    DL_FillAppListBox(listbox, &dwTotal);

    DL_ConfigureButtonsAndStatic(dlg, listbox, LB_ERR);
    //
    // redraw now that we've filled it
    //

    SetWindowRedraw( listbox, TRUE );

}


//////////////////////////////////////////////////////////////////////////////
//
// UNINSTALL_THREAD_INFO
//
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
    PROCESS_INFORMATION uninstaller;
    HWND dlg;

} UNINSTALL_THREAD_INFO;

//////////////////////////////////////////////////////////////////////////////
//
// Uninstall_TrackingThread
//
//    -- waits for an uninstall command  to complete and refreshes the dialog
//
//////////////////////////////////////////////////////////////////////////////

DWORD Uninstall_TrackingThread( UNINSTALL_THREAD_INFO *info )
{
    ResumeThread( info->uninstaller.hThread );
    CloseHandle( info->uninstaller.hThread );

    WaitForSingleObject( info->uninstaller.hProcess, INFINITE );
    CloseHandle( info->uninstaller.hProcess );

    PostMessage( info->dlg, UNM_WAKEUP, 0, 0 );
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// InstallUninstallDlgProc -- dlgproc for the install/uninstall page (surprise)
//
//////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK
InstallUninstallDlgProc( HWND dlg, UINT message, WPARAM wparam, LPARAM lparam )
{
    LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)GetWindowLongPtr( dlg, DWLP_USER );
    LPWIZDATA lpwd = page ? (LPWIZDATA)page->lParam : NULL;

    switch( message )
    {
        case WM_INITDIALOG:
            SetWindowLongPtr( dlg, DWLP_USER, lparam );
            page = (LPPROPSHEETPAGE)lparam;
            lpwd = (LPWIZDATA)page->lParam;
            lpwd->hwnd = dlg;
            Uninstall_RefreshList( dlg, lpwd );

#ifdef WX86
            //
            // Set initial state of ForceX86Env to FALSE (unchecked)
            //
            bForceX86Env = FALSE;

            if (bWx86Enabled) {
                SendDlgItemMessage(dlg,
                                   IDC_FORCEX86ENV,
                                   BM_SETSTATE,
                                   BST_UNCHECKED,
                                   0
                                   );
           } else {
                ShowWindow(GetDlgItem(dlg,IDC_FORCEX86ENV), SW_HIDE);
           }
#endif

            break;
        case WM_DESTROY:
            Uninstall_FreeList( dlg, lpwd, NULL );
            break;

        case UNM_WAKEUP:
            //
            // an uninstall in another thread just finished
            //

            EnableWindow( GetParent( dlg ), TRUE );
            SetForegroundWindow( GetParent( dlg ) );
            Uninstall_RefreshList( dlg, lpwd );
            break;

        case WM_NOTIFY:
        {
            NMHDR *nmhdr = (NMHDR *)lparam;

            switch( nmhdr->code )
            {
                case PSN_SETACTIVE:
                    lpwd->hwnd = dlg;
                    break;

                case PSN_KILLACTIVE:
                case PSN_HASHELP:
                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lparam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)aUninstallHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wparam, NULL, HELP_CONTEXTMENU,
                    (DWORD_PTR)aUninstallHelpIDs);
            break;

        case WM_COMMAND:
            switch( GET_WM_COMMAND_ID( wparam, lparam ) )
            {
                case IDC_BUTTONSETUP:
                    if( GET_WM_COMMAND_CMD( wparam, lparam ) == BN_CLICKED ) {
                        if (SetupWizard(lpwd)) {
                            DismissCPL(lpwd);
                        }
                    }
                    break;

                case IDC_MODIFY:
                case IDC_REPAIR:
                case IDC_UNINSTALL:
                case IDC_MODIFYUNINSTALL:
                    if( GET_WM_COMMAND_CMD( wparam, lparam ) == BN_CLICKED )
                    {
                        HWND hwndListBox = GetDlgItem(dlg, IDC_REGISTERED_APPS);
                        
                        if (hwndListBox)
                        {
                            int iSel = ListBox_GetCurSel(hwndListBox);
                    
                            if (LB_ERR != iSel)
                                DL_InvokeAction(GET_WM_COMMAND_ID( wparam, lparam ), dlg, hwndListBox, iSel);
                        }
                    }
                    break;

                case IDC_REGISTERED_APPS:
                    switch( GET_WM_COMMAND_CMD( wparam, lparam ) )
                    {
                        case LBN_SELCHANGE:
                            {
                                HWND hwndListBox = GetDlgItem(dlg, IDC_REGISTERED_APPS);
                            
                                if (hwndListBox)
                                {
                                    int iSel = ListBox_GetCurSel(hwndListBox);
                        
                                    if (LB_ERR != iSel)
                                        DL_ConfigureButtonsAndStatic(dlg, hwndListBox, iSel);
                                }
                                break;
                            }

                        case LBN_DBLCLK:
                            {
                                int iButtonID = 0;
                                HWND hwndListBox = GetDlgItem(dlg, IDC_REGISTERED_APPS);
                                
                                if (hwndListBox)
                                {
                                    int iSel = ListBox_GetCurSel(hwndListBox);
                        
                                    if (LB_ERR != iSel)
                                    {
                                        // Go through all the buttons in order of less damaging to more
                                        //   damaging and invoke the first visible+enable one

                                        HWND hwndTmp = GetDlgItem(dlg, IDC_MODIFYUNINSTALL);

                                        if (IsWindowVisible(hwndTmp) && IsWindowEnabled(hwndTmp))
                                            iButtonID = IDC_MODIFYUNINSTALL;
                                        else
                                        {
                                            hwndTmp = GetDlgItem(dlg, IDC_MODIFY);
                                            if (IsWindowVisible(hwndTmp) && IsWindowEnabled(hwndTmp))
                                                iButtonID = IDC_MODIFY;
                                            else
                                            {
                                                hwndTmp = GetDlgItem(dlg, IDC_REPAIR);
                                                if (IsWindowVisible(hwndTmp) && IsWindowEnabled(hwndTmp))
                                                    iButtonID = IDC_REPAIR;
                                                else
                                                {
                                                    hwndTmp = GetDlgItem(dlg, IDC_UNINSTALL);
                                                    if (IsWindowVisible(hwndTmp) && IsWindowEnabled(hwndTmp))
                                                        iButtonID = IDC_UNINSTALL;
                                                }
                                            }
                                        }
                                        if (iButtonID)
                                            DL_InvokeAction(iButtonID, dlg, hwndListBox, iSel);
                                    }
                                }
                                break;
                            }
                    }
                    break;

#ifdef WX86
                case IDC_FORCEX86ENV:
                    if (bWx86Enabled &&
                        GET_WM_COMMAND_CMD( wparam, lparam ) == BN_CLICKED )
                      {
                       bForceX86Env = !bForceX86Env;
                    }
                    break;
#endif

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

#endif // DOWNLEVEL
