//
//  Uninstal.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"
#include "regstr.h"

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
                IDC_UNINSTALL,       IDH_APPWIZ_UNINSTALL_BUTTON,
                IDC_REGISTERED_APPS, IDH_APPWIZ_UNINSTALL_LIST,
                0, 0
                };


#define DISPLAYNAME_SIZE 64

#ifdef WX86
BOOL bWx86Enabled=FALSE;
BOOL bForceX86Env=FALSE;
const WCHAR ProcArchName[]=L"PROCESSOR_ARCHITECTURE";
#endif




//////////////////////////////////////////////////////////////////////////////
//
// Uninstall_EnableDisableTheNukeButton -- take a guess
//
//////////////////////////////////////////////////////////////////////////////

void Uninstall_EnableDisableTheNukeButton( HWND dlg )
{
    //
    // we wouldn't have gotten here if dlgmgr failed to create the controls
    //

    HWND listbox = GetDlgItem( dlg, IDC_REGISTERED_APPS );
    HWND button = GetDlgItem( dlg, IDC_UNINSTALL );
    BOOL state = ( ListBox_GetCurSel( listbox ) >= 0 );

    //
    // are we going to change anything?
    //

    if( state != ( IsWindowEnabled( button ) != 0 ) )
    {
        //
        // don't trap keyboard only users if we're disabling the button
        //

        if( !state && ( GetFocus() == button ) )
            SendMessage( dlg, WM_NEXTDLGCTL, (WPARAM)listbox, (LPARAM)TRUE );

        EnableWindow( button, state );
    }
}


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
// EnumRemovableApps -- fills/refills the list of removable applications from a give hkey
//
//////////////////////////////////////////////////////////////////////////////

void EnumRemovableApps( HKEY uninstallkey, DWORD num_subkeys, LPWIZDATA lpwd, HWND listbox, LPDWORD lpdwTotal )
{
    TCHAR subkey[ 64 ];
    DWORD i;

    for( i = 0; ( i < num_subkeys ) &&
        ( RegEnumKey( uninstallkey, i, subkey, ARRAYSIZE( subkey ) )
                    == ERROR_SUCCESS ); i++ )
    {
        HKEY appkey = NULL;

        if( RegOpenKey( uninstallkey, subkey, &appkey )
                       == ERROR_SUCCESS )
        {
            TCHAR command[ ARRAYSIZE( lpwd->lpUItem->command ) ];
            DWORD len = sizeof( command );

            //
            // do not fix const warning, header is busted!
            //

            ZeroMemory( command, len );
            if( RegQueryValueEx( appkey, c_UninstallItemCommand,
                                NULL, NULL, (LPBYTE) command, &len )
                    == ERROR_SUCCESS )
            {
                TCHAR name[ DISPLAYNAME_SIZE ];

                len = sizeof( name );

                //
                // do not fix const warning, header is busted!
                //

                ZeroMemory( name, len );
                if( RegQueryValueEx( appkey, c_UninstallItemName,
                    NULL, NULL, (LPBYTE) name, &len ) == ERROR_SUCCESS )
                {
                    int index = ListBox_AddString( listbox, name );

                    if( index != LB_ERR )
                    {
                        UNINSTALL_ITEM *slot = lpwd->lpUItem + index;

                        if( (DWORD)index < *lpdwTotal )
                        {
                            MoveMemory( slot + 1, slot,
                                ( *lpdwTotal - (DWORD)index ) *
                                sizeof( UNINSTALL_ITEM ) );
                        }

                        lstrcpy( slot->command, command );
                        (*lpdwTotal)++;
                    }
                }
            }

            RegCloseKey( appkey );
        }
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
    DWORD   dwhklmSubKeys = 0;
    DWORD   dwhkcuSubKeys = 0;
    DWORD   dwSubKeys;

    //
    // we wouldn't have gotten here if dlgmgr failed to create the listbox
    //

    HWND listbox = GetDlgItem( dlg, IDC_REGISTERED_APPS );
    HKEY hklmUninstallKey;
    HKEY hkcuUninstallKey;

    //
    // avoid flicker when visible
    //

    SetWindowRedraw( listbox, FALSE );

    //
    // clean out any residual junk
    //

    Uninstall_FreeList( dlg, lpwd, listbox );

    //
    // Compute maximum size of both lists of removable apps
    //
    dwTotal = 0;

    if(RegOpenKey(HKEY_LOCAL_MACHINE,c_UninstallKey,&hklmUninstallKey) == ERROR_SUCCESS)
        RegQueryInfoKey( hklmUninstallKey, NULL, NULL, NULL, &dwhklmSubKeys,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    else
        hklmUninstallKey = NULL;

    if(RegOpenKey(HKEY_CURRENT_USER,c_UninstallKey,&hkcuUninstallKey) == ERROR_SUCCESS )
        RegQueryInfoKey( hkcuUninstallKey, NULL, NULL, NULL, &dwhkcuSubKeys,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    else
        hkcuUninstallKey = NULL;

    //
    // Allocate a UNINSTALL_ITEM buffer for all of the items
    //
    dwSubKeys = dwhklmSubKeys + dwhkcuSubKeys;  // Room for both sets
    if (dwSubKeys != 0)
    {
        lpwd->lpUItem = (LPUNINSTALL_ITEM)
            LocalAlloc( LPTR, sizeof( UNINSTALL_ITEM ) * dwSubKeys );
    }

    //
    // try to enumerate both lists of removable apps
    //
    if (hklmUninstallKey && lpwd->lpUItem)
    {
        EnumRemovableApps( hklmUninstallKey, dwhklmSubKeys, lpwd, listbox, &dwTotal );
        RegCloseKey( hklmUninstallKey );
    }
    if (hkcuUninstallKey && lpwd->lpUItem)
    {
        EnumRemovableApps( hkcuUninstallKey, dwhkcuSubKeys, lpwd, listbox, &dwTotal );
        RegCloseKey( hkcuUninstallKey );
    }

    if( !dwTotal )
        Uninstall_FreeList( dlg, lpwd, NULL );

    //
    // redraw now that we've filled it
    //

    SetWindowRedraw( listbox, TRUE );

    //
    // update the remove button
    //

    Uninstall_EnableDisableTheNukeButton( dlg );
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
// Uninstall_UninstallCurrentItem -- uninstalls the current listbox selection
//
//////////////////////////////////////////////////////////////////////////////

void Uninstall_UninstallCurrentItem( HWND dlg, LPWIZDATA lpwd )
{
    //
    // we wouldn't have gotten here if dlgmgr failed to create the controls
    //

    HWND listbox = GetDlgItem( dlg, IDC_REGISTERED_APPS );
    int index = ListBox_GetCurSel( listbox );

    UNINSTALL_ITEM *item = ( ( index >= 0 ) && lpwd->lpUItem )?
        ( lpwd->lpUItem + index ) : NULL;

    if( item )
    {
        TCHAR name[ DISPLAYNAME_SIZE ];
        HWND  sheet = GetParent( dlg );
        UNINSTALL_THREAD_INFO *info;

        ListBox_GetText( listbox, index, name );

        //
        //  the uninstallers do their own UI, as we don't really know what
        //  they are going to do.
        //

        if( ( info = (UNINSTALL_THREAD_INFO *)LocalAlloc( LPTR,
            sizeof( UNINSTALL_THREAD_INFO ) ) ) != NULL )
        {
            DWORD idthread = 0;
            HANDLE thread = CreateThread( NULL, 0,
                (LPTHREAD_START_ROUTINE)Uninstall_TrackingThread, info,
                CREATE_SUSPENDED, &idthread );

            if( thread )
            {
                STARTUPINFO startup;
                BOOL bRet;
                DWORD dwCreationFlags;
#ifdef WX86
                DWORD  Len;
                WCHAR  ProcArchValue[32];
#endif


                startup.cb = sizeof( startup );
                startup.lpReserved = NULL;
                startup.lpDesktop = NULL;
                startup.lpTitle = NULL;
                startup.dwFlags = 0L;
                startup.cbReserved2 = 0;
                startup.lpReserved2 = NULL;
                startup.wShowWindow = SW_SHOWNORMAL;

                dwCreationFlags = CREATE_SUSPENDED;

#ifdef WX86
                if (bWx86Enabled && bForceX86Env) {
                    Len = GetEnvironmentVariableW(ProcArchName,
                                                  ProcArchValue,
                                                  sizeof(ProcArchValue)
                                                  );

                    if (!Len || Len >= sizeof(ProcArchValue)) {
                        ProcArchValue[0]=L'\0';
                    }

                    SetEnvironmentVariableW(ProcArchName, L"x86");
                    dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
                }
#endif

                bRet = CreateProcess(NULL,
                                     item->command,
                                     NULL, NULL,
                                     FALSE,
                                     dwCreationFlags,
                                     NULL, NULL,
                                     &startup,
                                     &info->uninstaller
                                     );

#ifdef WX86
                if (bWx86Enabled && bForceX86Env) {
                    SetEnvironmentVariableW(ProcArchName, ProcArchValue);
                }
#endif

                if( bRet ) {

                    info->dlg = dlg;
                    EnableWindow( sheet, FALSE );
                    ResumeThread( thread );
                    CloseHandle( thread );

                    //
                    // thread will clean up for us when it exits
                    //

                    return;
                }

                TerminateThread( thread, 0 );
                CloseHandle( thread );
            }

            LocalFree( (HANDLE)info );
        }

        ShellMessageBox( hInstance, sheet,
            MAKEINTRESOURCE( IDS_UNINSTALL_FAILED ),
            MAKEINTRESOURCE( IDS_UNINSTALL_ERROR ),
            MB_OK | MB_ICONEXCLAMATION, name );
    }
    else
    {
        //
        // the user requested an out of range item
        //

        MessageBeep( 0 );
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// InstallUninstallDlgProc -- dlgproc for the install/uninstall page (surprise)
//
//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
InstallUninstallDlgProc( HWND dlg, UINT message, WPARAM wparam, LPARAM lparam )
{
    LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)GetWindowLong( dlg, DWL_USER );
    LPWIZDATA lpwd = page ? (LPWIZDATA)page->lParam : NULL;

    switch( message )
    {
        case WM_INITDIALOG:
            SetWindowLong( dlg, DWL_USER, lparam );
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
                    HELP_WM_HELP, (DWORD)aUninstallHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wparam, NULL, HELP_CONTEXTMENU,
                    (DWORD)aUninstallHelpIDs);
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

                case IDC_UNINSTALL:
                    if( GET_WM_COMMAND_CMD( wparam, lparam ) == BN_CLICKED )
                        Uninstall_UninstallCurrentItem( dlg, lpwd );
                    break;

                case IDC_REGISTERED_APPS:
                    switch( GET_WM_COMMAND_CMD( wparam, lparam ) )
                    {
                        case LBN_SELCHANGE:
                            Uninstall_EnableDisableTheNukeButton( dlg );
                            break;

                        case LBN_DBLCLK:
                            Uninstall_UninstallCurrentItem( dlg, lpwd );
                            break;
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
