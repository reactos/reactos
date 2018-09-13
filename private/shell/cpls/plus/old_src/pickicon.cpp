//---------------------------------------------------------------------------
//
//      File: PICKICON.CPP
//
//      Support code for the Change Icon dialog.
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include "rc.h"
#include "pickicon.h"

#define MAX_ICONS   500             // that is a lot 'o icons

#define ARRAYSIZE(s)                (sizeof(s) / sizeof((s)[0]))

typedef struct
{
        LPSTR pszIconPath;              // input/output
        int cbIconPath;                 // input
        int iIconIndex;             // input/output
        // private state variables
        HWND hDlg;
        BOOL fFirstPass;
        char szPathField[MAX_PATH];
        char szBuffer[MAX_PATH];
} PICKICON_DATA, FAR *LPPICKICON_DATA;

extern HINSTANCE g_hInst;

// Checks if the file exists, if it doesn't it tries tagging on .exe and if that
// fails it reports an error. The given path is environment expanded.  If it needs
// to put up an error box, it changes the cursor back.  Path's assumed to be
// MAXITEMPATHLEN long.  The main reason for moving this out of the DlgProc was
// because we're running out of stack space on the call to the comm dlg.
BOOL NEAR PASCAL IconFileExists( LPPICKICON_DATA lppid )
{
TCHAR szTitle[128];
TCHAR szInvPath[ 64 ];
TCHAR szText[MAX_PATH+40];
TCHAR szPath[MAX_PATH];
LPTSTR psz;
DWORD dwRet;

        if( lppid->szBuffer[0] == 0 )
                return FALSE;

        // Use the Win32 version instead of the shell version.  The shell version
        // is/was really only there for 16-bit apps.  (RickTu)
        //
        // DoEnvironmentSubst( lppid->szBuffer, sizeof(lppid->szBuffer) );
        //

        dwRet = ExpandEnvironmentStrings( lppid->szBuffer, szPath, MAX_PATH );
        if (dwRet > 0 && dwRet <= MAX_PATH)
        {
            lstrcpy( lppid->szBuffer, szPath );
        }

//      PathUnquoteSpaces( lppid->szBuffer );   // JER

//      if( PathResolve( lppid->szBuffer, NULL, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS ) ) // JER
        if (SearchPath(NULL, lppid->szBuffer, NULL, ARRAYSIZE(szPath), szPath, &psz) != 0)
                return TRUE;

        LoadString( g_hInst, IDS_BADPATHMSG, szTitle, ARRAYSIZE(szTitle) );
        LoadString( g_hInst, IDS_INVALIDPATH, szInvPath, ARRAYSIZE(szInvPath) );
        wsprintf( szText, szTitle, lppid->szBuffer );
        GetWindowText( lppid->hDlg, szTitle, sizeof(szTitle) );
        lstrcat( szTitle, szInvPath );
        MessageBox( GetDesktopWindow(), szText, szTitle , MB_OK | MB_ICONEXCLAMATION );
        return FALSE;
}

void NEAR PASCAL PutIconsInList( LPPICKICON_DATA lppid )
{
HICON  *rgIcons;
int      iTempIcon;
int  cIcons;
HWND hDlg = lppid->hDlg;
//HCURSOR hOldCursor;

        SendDlgItemMessage( hDlg, IDD_ICON, LB_RESETCONTENT, 0, 0L );

        GetDlgItemText( hDlg, IDD_PATH, lppid->szPathField, sizeof(lppid->szPathField) );

        lstrcpy( lppid->szBuffer, lppid->szPathField );

        if( !IconFileExists(lppid) )
        {
                if( lppid->fFirstPass )
                {
                        // Icon File doesn't exist, use progman
                        lppid->fFirstPass = FALSE;  // Only do this bit once.
                        GetModuleFileName( g_hInst, lppid->szBuffer, ARRAYSIZE(lppid->szBuffer) );
                }
                else
                {
                        return;
                }
        }
        lstrcpy( lppid->szPathField, lppid->szBuffer );
        SetDlgItemText( hDlg, IDD_PATH, lppid->szPathField );

//      hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

        rgIcons = (HICON *)LocalAlloc(LPTR, MAX_ICONS*sizeof(HICON));

        if( rgIcons != NULL )
                cIcons = (int)ExtractIconEx( lppid->szBuffer, 0, rgIcons, NULL, MAX_ICONS );
        else
                cIcons = 0;

//      SetCursor( hOldCursor );
    if( !cIcons )
    {
        char szText[256];
        char szTitle[40];

                GetWindowText( lppid->hDlg, szTitle, sizeof(szTitle) );
                if( lppid->fFirstPass )
                {
                        lppid->fFirstPass = FALSE;  // Only do this bit once.
                        LoadString( g_hInst, IDS_NOICONSMSG1, szText, 256 );
                        MessageBox( GetDesktopWindow(), szText, szTitle, MB_OK | MB_ICONEXCLAMATION );

                        // No icons here - change the path do somewhere where we
                        // know there are icons. Get the path to progman.
//                      GetModuleFileName( g_hInst, lppid->szPathField, sizeof(lppid->szPathField) );
                        GetSystemDirectory( lppid->szPathField, sizeof(lppid->szPathField) );
                        lstrcat( lppid->szPathField, "\\shell32.dll" );
                        SetDlgItemText( hDlg, IDD_PATH, lppid->szPathField );
                        PutIconsInList( lppid );
                }
                else
                {
                        LoadString( g_hInst, IDS_NOICONSMSG, szText, 256 );
                        MessageBox( GetDesktopWindow(), szText, szTitle, MB_OK | MB_ICONEXCLAMATION );
                        return;
                }
        }
//      hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

        SendDlgItemMessage( hDlg, IDD_ICON, WM_SETREDRAW, FALSE, 0L );

    if( rgIcons )
    {
                for( iTempIcon = 0; iTempIcon < cIcons; iTempIcon++ )
                {
                        SendDlgItemMessage( hDlg, IDD_ICON, LB_ADDSTRING, 0, (LPARAM)(UINT)rgIcons[iTempIcon] );
                }
                LocalFree((HLOCAL)rgIcons);
        }

        if( SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, lppid->iIconIndex, 0L ) == LB_ERR )
        {
                // select the first.
                SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, 0, 0L );
        }

        SendDlgItemMessage( hDlg, IDD_ICON, WM_SETREDRAW, TRUE, 0L );
        InvalidateRect( GetDlgItem(hDlg, IDD_ICON), NULL, TRUE );

//      SetCursor( hOldCursor );
}

void NEAR PASCAL InitPickIconDlg( HWND hDlg, LPPICKICON_DATA lppid )
{
RECT rc;
UINT cy;
HWND hwndIcons;

        // init state variables
        lppid->hDlg = hDlg;
        lstrcpyn( lppid->szPathField, lppid->pszIconPath, sizeof(lppid->szPathField) );

        // this first pass stuff is so that the first time something bogus happens
        // (file not found, no icons) we give the user a list of icons from progman.
        lppid->fFirstPass = TRUE;

        // init the dialog controls
        SetDlgItemText( hDlg, IDD_PATH, lppid->pszIconPath );
        SendDlgItemMessage( hDlg, IDD_PATH, EM_LIMITTEXT, lppid->cbIconPath, 0L );

        SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCOLUMNWIDTH, GetSystemMetrics(SM_CXICON) + 12, 0L );

        hwndIcons = GetDlgItem( hDlg, IDD_ICON );

        // compute the height of the listbox based on icon dimensions
        GetClientRect( hwndIcons, &rc );

        cy = GetSystemMetrics( SM_CYICON ) + GetSystemMetrics( SM_CYHSCROLL ) + GetSystemMetrics( SM_CYEDGE ) * 3;

        SetWindowPos( hwndIcons, NULL, 0, 0, rc.right, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

        // REVIEW, explicitly position this dialog?

        cy = rc.bottom - cy;

        GetWindowRect( hDlg, &rc );
        rc.bottom -= rc.top;
        rc.right -= rc.left;
        rc.bottom = rc.bottom - cy;

        SetWindowPos( hDlg, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOACTIVATE );

        PutIconsInList( lppid );
}

// call the common browse code for this
BOOL NEAR PASCAL BrowseForIconFile( LPPICKICON_DATA lppid )
{
OPENFILENAME  ofn;
CHAR szFilter[256] = TEXT("Icon Files\0*.ico;*.exe;*.dll\0Programs (*.exe)\0*.exe\0Libraries (*.dll)\0*.dll\0Icons (*.ico)\0*.ico\0All Files (*.*)\0*.*\0\0");
char szTitle[40];

        ZeroMemory(&ofn, sizeof(ofn));

        if (LoadString( g_hInst, IDS_ICONFILTER, szFilter, ARRAYSIZE(szFilter) ) != 0) {
            LPTSTR psz;

            for( psz = szFilter; *psz != TEXT('\0'); psz++ ) {
#ifdef DBCS
                if ( IsDBCSLeadByte(*psz) ) {
                    psz = CharNext(psz) - 1;
                    continue;
                }
#endif

                if (*psz == TEXT('\1')) {
                    *psz = TEXT('\0');
                }
            }
        }



        GetWindowText( lppid->hDlg, szTitle, sizeof(szTitle) );
        GetDlgItemText( lppid->hDlg, IDD_PATH, lppid->szBuffer, sizeof(lppid->szBuffer) );

        ofn.lStructSize                 = sizeof(OPENFILENAME);
        ofn.hwndOwner                   = lppid->hDlg;
        ofn.lpstrFilter                 = szFilter;
//      ofn.lpstrCustomFilter   = (LPSTR)NULL;
//      ofn.nMaxCustFilter              = 0L;
        ofn.nFilterIndex                = 1L;
        ofn.lpstrFile                   = lppid->szBuffer;
        ofn.nMaxFile                    = sizeof(lppid->szBuffer);
//      ofn.lpstrFileTitle              = (LPSTR)NULL;
//      ofn.lpstrInitialDir             = NULL;
        ofn.lpstrTitle                  = szTitle;
        ofn.Flags                               = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
//      ofn.nFileOffset                 = 0;
//      ofn.nFileExtension              = 0;
//      ofn.lCustData                   = 0;

        if( GetOpenFileName( &ofn ) )
        {
//              PathQuoteSpaces( lppid->szBuffer );     // JER
                SetDlgItemText( lppid->hDlg, IDD_PATH, lppid->szBuffer );
                // Set default button to OK.
                SendMessage( lppid->hDlg, DM_SETDEFID, IDOK, 0 );
                return TRUE;
        }
        else
                return FALSE;
}

// test if the name field is different from the last file we looked at
BOOL NEAR PASCAL NameChange( LPPICKICON_DATA lppid )
{
        GetDlgItemText( lppid->hDlg, IDD_PATH, lppid->szBuffer, sizeof(lppid->szBuffer) );
        return lstrcmpi(lppid->szBuffer, lppid->szPathField);
}


//
// dialog procedure for picking an icon (ala progman change icon)
// uses DLG_PICKICON template
//
// in:
//      pszIconFile
//      cbIconFile
//      iIndex
//
// out:
//      pszIconFile
//      iIndex
//
BOOL CALLBACK PickIconDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
LPPICKICON_DATA lppid = (LPPICKICON_DATA)GetWindowLong(hDlg, DWL_USER);

// Array for context help:
/*static DWORD aPickIconHelpIDs[] = {           // JER
        IDD_PATH,   IDH_FCAB_LINK_ICONNAME,
        IDD_ICON,   IDH_FCAB_LINK_CURRENT_ICON,
        IDD_BROWSE, IDH_BROWSE,
        0, 0
        };
*/
        switch( wMsg )
        {
                case WM_INITDIALOG:
                        SetWindowLong( hDlg, DWL_USER, lParam );
                        InitPickIconDlg( hDlg, (LPPICKICON_DATA)lParam );
                        break;

                case WM_COMMAND:
                        switch( GET_WM_COMMAND_ID(wParam, lParam) )
                        {
                                case IDHELP:        // BUGBUG, not wired
                                        break;

                                case IDD_BROWSE:
                                        if( BrowseForIconFile( lppid ) )
                                                PutIconsInList( lppid );
                                        break;

                                case IDD_PATH:
                                        if( NameChange( lppid ) )
                                                SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, (WPARAM)-1, 0 );
                                        break;

                                case IDD_ICON:
                                        if( NameChange( lppid ) )
                                        {
                                                PutIconsInList( lppid );
                                                break;
                                        }
                                        if( GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK )
                                                break;

                                        /*** FALL THRU on double click ***/

                                case IDOK:
                                        if( NameChange( lppid ) )
                                        {
                                                PutIconsInList( lppid );
                                        }
                                        else
                                        {
                                        int iIconIndex = (int)SendDlgItemMessage( hDlg, IDD_ICON, LB_GETCURSEL, 0, 0L );
                                                if( iIconIndex < 0 )
                                                        iIconIndex = 0;
                                                lppid->iIconIndex = iIconIndex;
                                                lstrcpy( lppid->pszIconPath, lppid->szPathField );

                                                EndDialog( hDlg, TRUE );
                                        }
                                        break;

                                case IDCANCEL:
                                        EndDialog( hDlg, FALSE );
                                        break;

                                default:
                                        return( FALSE );
                        }
                        break;

                // owner draw messages for icon listbox
                case WM_DRAWITEM:
                        #define lpdi ((DRAWITEMSTRUCT FAR *)lParam)

                        if( lpdi->itemState & ODS_SELECTED )
                                SetBkColor( lpdi->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
                        else
                                SetBkColor( lpdi->hDC, GetSysColor( COLOR_WINDOW ) );

                        /* repaint the selection state */
                        ExtTextOut( lpdi->hDC, 0, 0, ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL );

                        /* draw the icon */
                        if( (int)lpdi->itemID >= 0 )
                                DrawIcon(lpdi->hDC, (lpdi->rcItem.left + lpdi->rcItem.right - GetSystemMetrics(SM_CXICON)) / 2,
                                        (lpdi->rcItem.bottom + lpdi->rcItem.top - GetSystemMetrics(SM_CYICON)) / 2, (HICON)lpdi->itemData);

                        // InflateRect(&lpdi->rcItem, -1, -1);

                        /* if it has the focus, draw the focus */
                        if( lpdi->itemState & ODS_FOCUS )
                                DrawFocusRect( lpdi->hDC, &lpdi->rcItem );

                        #undef lpdi
                        break;

                case WM_MEASUREITEM:
                        #define lpmi ((MEASUREITEMSTRUCT FAR *)lParam)

                        lpmi->itemWidth = GetSystemMetrics( SM_CXICON ) + 12;
                        lpmi->itemHeight = GetSystemMetrics( SM_CYICON ) + 4;

                        #undef lpmi
                        break;

                case WM_DELETEITEM:
                        #define lpdi ((DELETEITEMSTRUCT FAR *)lParam)

                        DestroyIcon( (HICON)lpdi->itemData );

                        #undef lpdi
                        break;

                case WM_HELP:
//                      WinHelp( ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD)(LPSTR) aPickIconHelpIDs ); // JER
                        break;

                case WM_CONTEXTMENU:
//                      WinHelp( (HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD)(LPVOID)aPickIconHelpIDs ); // JER
                        break;

                default:
                        return FALSE;
        }
        return TRUE;
}


// puts up the pick icon dialog
int WINAPI PickIconDlg( HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int FAR *piIconIndex )
{
PICKICON_DATA *pid;
int iResult;

        // if we are coming up from a 16->32 thunk.  it is possible that SHELL32 will
        // not be loaded in this context, so we will load ourself if we are not loaded.
//      IsDllLoaded( g_hInst, "SHELL32" );      // JER

        pid = (PICKICON_DATA *)LocalAlloc( LPTR, sizeof(PICKICON_DATA) );

        if( pid == NULL )
                return 0;

        pid->pszIconPath = pszIconPath;
        pid->cbIconPath = cbIconPath;
        pid->iIconIndex = *piIconIndex;

        iResult = DialogBoxParam( g_hInst, MAKEINTRESOURCE(DLG_PICKICON), hwnd, (DLGPROC)PickIconDlgProc, (LPARAM)(LPPICKICON_DATA)pid );

        *piIconIndex = pid->iIconIndex;

        LocalFree( pid );

        return iResult;
}
