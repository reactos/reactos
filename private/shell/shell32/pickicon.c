#include "shellprv.h"
#pragma  hdrstop

#define MAX_ICONS   500             // that is a lot 'o icons

#define CX_BORDER    4
#define CY_BORDER    12

typedef struct {
    LPTSTR pszIconPath;              // input/output
    int cbIconPath;                 // input
    int iIconIndex;                 // input/output
    // private state variables
    HWND hDlg;
    BOOL fFirstPass;
    TCHAR szPathField[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
} PICKICON_DATA, *LPPICKICON_DATA;


typedef struct 
{
    int iResult;                    // icon index within the resources
    int iResId;                     // resource ID to search for!
} ICONENUMSTATE, *LPICONENUMSTATE;


// Call back function used when trying to find the correct icon to be 
// highlighted, called with the name of each resource - we compare this
// against the one specified in the structure and bail out if we get
// a match.

BOOL CALLBACK IconEnumProc( HANDLE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam )
{
    LPICONENUMSTATE pState = (LPICONENUMSTATE)lParam;

    if ( (INT_PTR)lpszName == pState->iResId )
        return FALSE;                        // bail out of enum loop

    pState->iResult++;
    return TRUE;
}




// Checks if the file exists, if it doesn't it tries tagging on .exe and
// if that fails it reports an error. The given path is environment expanded.
// If it needs to put up an error box, it changes the cursor back.
// Path s assumed to be MAXITEMPATHLEN long.
// The main reason for moving this out of the DlgProc was because we're
// running out of stack space on the call to the comm dlg.

BOOL IconFileExists(LPPICKICON_DATA lppid)
{
    TCHAR szExpBuffer[ ARRAYSIZE(lppid->szBuffer) ];

    if (lppid->szBuffer[0] == 0)
        return FALSE;

    if (SHExpandEnvironmentStrings(lppid->szBuffer, szExpBuffer, ARRAYSIZE(szExpBuffer)))
    {
        PathUnquoteSpaces(lppid->szBuffer);
        PathUnquoteSpaces(szExpBuffer);

        if (PathResolve(szExpBuffer, NULL, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS))
            return TRUE;

        ShellMessageBox(HINST_THISDLL, lppid->hDlg, MAKEINTRESOURCE(IDS_BADPATHMSG), 0, MB_OK | MB_ICONEXCLAMATION, (LPTSTR)lppid->szPathField);
    }

    return FALSE;
}

//
// GetDefaultIconImageName:
//     szBuffer should be at least MAX_PATH chars big
//
void GetDefaultIconImageName( LPTSTR szBuffer )
{
// BUGBUG - BobDay - Why not do this on Win95 too?  Doesn't Win95 version
// of shell now support expanded env. strings? (swap "system32" w. "system")
#ifdef WINNT
    TCHAR szModName[ MAX_PATH ];
    TCHAR szSystemDir[ MAX_PATH ];
    DWORD cbSysDir;

    GetModuleFileName(HINST_THISDLL, szModName, ARRAYSIZE(szModName));
    cbSysDir = GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir) );
    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szSystemDir, cbSysDir, szModName, cbSysDir) == 2)
    {
        //
        // Okay, the path for SHELL32.DLL starts w/the system directory.
        // To be sneaky and helpfull, we're gonna change it to "%systemroot%"
        //

        lstrcpy( szBuffer, TEXT("%SystemRoot%\\system32") );
        PathAppend( szBuffer, PathFindFileName(szModName) );
    }
    else
    {
        lstrcpy(szBuffer, szModName );
    }
#else 
    GetModuleFileName(HINST_THISDLL, szBuffer, MAX_PATH);
#endif
}

void PutIconsInList(LPPICKICON_DATA lppid)
{
    HICON  *rgIcons;
    int  cIcons;
    HWND hDlg = lppid->hDlg;
    DECLAREWAITCURSOR;
    LONG err = LB_ERR;

    SendDlgItemMessage(hDlg, IDD_ICON, LB_RESETCONTENT, 0, 0L);

    GetDlgItemText(hDlg, IDD_PATH, lppid->szPathField, ARRAYSIZE(lppid->szPathField));

    lstrcpy(lppid->szBuffer, lppid->szPathField);

    if (!IconFileExists(lppid)) {
        if (lppid->fFirstPass) {

            // Icon File doesn't exist, use progman
            lppid->fFirstPass = FALSE;  // Only do this bit once.
            GetDefaultIconImageName( lppid->szBuffer );
        } else {
            return;
        }
    }

    lstrcpy(lppid->szPathField, lppid->szBuffer);
    SetDlgItemText(hDlg, IDD_PATH, lppid->szPathField);

    SetWaitCursor();

    rgIcons = (HICON *)LocalAlloc(LPTR, MAX_ICONS*SIZEOF(HICON));

    if (rgIcons != NULL)
        cIcons = (int)ExtractIconEx(lppid->szBuffer, 0, rgIcons, NULL, MAX_ICONS);
    else
        cIcons = 0;

    ResetWaitCursor();
    if (!cIcons) {

        if (lppid->fFirstPass) {

            lppid->fFirstPass = FALSE;  // Only do this bit once.

            ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_NOICONSMSG1), 0, MB_OK | MB_ICONEXCLAMATION, (LPCTSTR)lppid->szBuffer);

            // No icons here - change the path do somewhere where we
            // know there are icons. Get the path to progman.
            GetDefaultIconImageName( lppid->szPathField );
            SetDlgItemText(hDlg, IDD_PATH, lppid->szPathField);
            PutIconsInList(lppid);
        } else {

            ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_NOICONSMSG), 0, MB_OK | MB_ICONEXCLAMATION, (LPCTSTR)lppid->szBuffer);
            return;
        }
    }

    SetWaitCursor();

    SendDlgItemMessage(hDlg, IDD_ICON, WM_SETREDRAW, FALSE, 0L);

    if (rgIcons) {
        int i;
        for (i = 0; i < cIcons; i++) {
            SendDlgItemMessage(hDlg, IDD_ICON, LB_ADDSTRING, 0, (LPARAM)(UINT_PTR)rgIcons[i]);
        }
        LocalFree((HLOCAL)rgIcons);
    }

    // Cope with being given a resource ID, not an index into the icon array.  To do this
    // we must enumerate the icon names checking for a match.  If we have one then highlight
    // that, otherwise default to the first.
    //
    // A resource icon reference is indicated by being passed a -ve iIconIndex.

    if ( lppid->iIconIndex >= 0 )
    {
        err = (LONG) SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, lppid->iIconIndex, 0L);
    }
    else
    {
        HMODULE hModule = LoadLibrary( lppid->szBuffer );
        if (hModule)
        {
            ICONENUMSTATE state;

            state.iResult = 0;
            state.iResId = -(lppid->iIconIndex);

            EnumResourceNames( hModule, RT_GROUP_ICON, IconEnumProc, (LONG_PTR)&state );

            err = (LONG) SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, state.iResult, 0L );
            FreeLibrary( hModule );
        }
    }

    // Check for failure, if we did then ensure we highlight the first!

    if ( err == LB_ERR )
        SendDlgItemMessage( hDlg, IDD_ICON, LB_SETCURSEL, 0, 0L );
       
    SendDlgItemMessage(hDlg, IDD_ICON, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(GetDlgItem(hDlg, IDD_ICON), NULL, TRUE);

    ResetWaitCursor();
}


void InitPickIconDlg(HWND hDlg, LPPICKICON_DATA lppid)
{
    RECT rc;
    UINT cy;
    HWND hwndIcons;

    // init state variables

    lppid->hDlg = hDlg;
    lstrcpyn(lppid->szPathField, lppid->pszIconPath, ARRAYSIZE(lppid->szPathField));

    // this first pass stuff is so that the first time something
    // bogus happens (file not found, no icons) we give the user
    // a list of icons from progman.
    lppid->fFirstPass = TRUE;

    // init the dialog controls

    SetDlgItemText(hDlg, IDD_PATH, lppid->pszIconPath);

    // Cannot max against 0 because 0 means "no limit"
    SendDlgItemMessage(hDlg, IDD_PATH, EM_LIMITTEXT, max(lppid->cbIconPath-1, 1), 0L);

    SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCOLUMNWIDTH, GetSystemMetrics(SM_CXICON) + CX_BORDER, 0L);

    hwndIcons = GetDlgItem(hDlg, IDD_ICON);

    /* compute the height of the listbox based on icon dimensions */
    GetClientRect(hwndIcons, &rc);

    cy = ((GetSystemMetrics(SM_CYICON) + CY_BORDER) * 4) + 
         GetSystemMetrics(SM_CYHSCROLL) + 
         GetSystemMetrics(SM_CYEDGE) * 3;

    SetWindowPos(hwndIcons, NULL, 0, 0, rc.right, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    SHAutoComplete(GetDlgItem(hDlg, IDD_PATH), 0);

    PutIconsInList(lppid);
}


// call the common browse code for this

BOOL BrowseForIconFile(LPPICKICON_DATA lppid)
{
    TCHAR szTitle[80];

    GetWindowText(lppid->hDlg, szTitle, ARRAYSIZE(szTitle));
    GetDlgItemText(lppid->hDlg, IDD_PATH, lppid->szBuffer, ARRAYSIZE(lppid->szBuffer));

    // We will never be quoted here because IconFileExists() removes quotes (of course user could type them in)
    if (lppid->szBuffer[0] != '"')
        PathQuoteSpaces(lppid->szBuffer);

    if (GetFileNameFromBrowse(lppid->hDlg, lppid->szBuffer, ARRAYSIZE(lppid->szBuffer), NULL, MAKEINTRESOURCE(IDS_ICO), MAKEINTRESOURCE(IDS_ICONSFILTER), szTitle))
    {
        PathQuoteSpaces(lppid->szBuffer);
        SetDlgItemText(lppid->hDlg, IDD_PATH, lppid->szBuffer);
        // Set default button to OK.
        SendMessage(lppid->hDlg, DM_SETDEFID, IDOK, 0);
        return TRUE;
    } else
        return FALSE;
}

// test if the name field is different from the last file we looked at

BOOL NameChange(LPPICKICON_DATA lppid)
{
    GetDlgItemText(lppid->hDlg, IDD_PATH, lppid->szBuffer, ARRAYSIZE(lppid->szBuffer));

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

BOOL_PTR CALLBACK PickIconDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    LPPICKICON_DATA lppid = (LPPICKICON_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    DWORD dwOldLayout;

        // Array for context help:

        static const DWORD aPickIconHelpIDs[] = {
                IDD_PATH,   IDH_FCAB_LINK_ICONNAME,
                IDD_ICON,   IDH_FCAB_LINK_CURRENT_ICON,
                IDD_BROWSE, IDH_BROWSE,

                0, 0
        };

    switch (wMsg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        InitPickIconDlg(hDlg, (LPPICKICON_DATA)lParam);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDHELP:        // BUGBUG, not wired
            break;

        case IDD_BROWSE:
            if (BrowseForIconFile(lppid))
                PutIconsInList(lppid);
            break;

        case IDD_PATH:
            if (NameChange(lppid))
                SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCURSEL, (WPARAM)-1, 0);
            break;

        case IDD_ICON:
            if (NameChange(lppid)) {
                PutIconsInList(lppid);
                break;
            }

            if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
                break;

            /*** FALL THRU on double click ***/

        case IDOK:

            if (NameChange(lppid)) {
                PutIconsInList(lppid);
            } else {
                int iIconIndex = (int)SendDlgItemMessage(hDlg, IDD_ICON, LB_GETCURSEL, 0, 0L);
                if (iIconIndex < 0)
                    iIconIndex = 0;
                lppid->iIconIndex = iIconIndex;
                lstrcpyn(lppid->pszIconPath, lppid->szPathField, lppid->cbIconPath);

                EndDialog(hDlg, TRUE);
            }
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        default:
            return(FALSE);
        }
        break;

    // owner draw messages for icon listbox

    case WM_DRAWITEM:
        #define lpdi ((DRAWITEMSTRUCT *)lParam)

        if (lpdi->itemState & ODS_SELECTED)
            SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
        else
            SetBkColor(lpdi->hDC, GetSysColor(COLOR_WINDOW));


        /* repaint the selection state */
        ExtTextOut(lpdi->hDC, 0, 0, ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL);

        if (g_bMirroredOS && (dwOldLayout = GET_DC_LAYOUT(lpdi->hDC)))
        {
            SET_DC_LAYOUT(lpdi->hDC, dwOldLayout | LAYOUT_PRESERVEBITMAP);
        }

        /* draw the icon */
        if ((int)lpdi->itemID >= 0)
          DrawIcon(lpdi->hDC, (lpdi->rcItem.left + lpdi->rcItem.right - GetSystemMetrics(SM_CXICON)) / 2,
                              (lpdi->rcItem.bottom + lpdi->rcItem.top - GetSystemMetrics(SM_CYICON)) / 2, (HICON)lpdi->itemData);
        if (dwOldLayout)
        {
            SET_DC_LAYOUT(lpdi->hDC, dwOldLayout);
        }                              

        // InflateRect(&lpdi->rcItem, -1, -1);

        /* if it has the focus, draw the focus */
        if (lpdi->itemState & ODS_FOCUS)
            DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

        #undef lpdi
        break;

    case WM_MEASUREITEM:
        #define lpmi ((MEASUREITEMSTRUCT *)lParam)

        lpmi->itemWidth = GetSystemMetrics(SM_CXICON) + CX_BORDER;
        lpmi->itemHeight = GetSystemMetrics(SM_CYICON) + CY_BORDER;

        #undef lpmi
        break;

    case WM_DELETEITEM:
        #define lpdi ((DELETEITEMSTRUCT *)lParam)

        DestroyIcon((HICON)lpdi->itemData);

        #undef lpdi
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aPickIconHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)aPickIconHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// puts up the pick icon dialog

STDAPI_(int) PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cbIconPath, int *piIconIndex)
{
    PICKICON_DATA *pid = (PICKICON_DATA *)LocalAlloc(LPTR, SIZEOF(PICKICON_DATA));
    if (pid)
    {
        int res;

        pid->pszIconPath = pszIconPath;
        pid->cbIconPath = cbIconPath;
        pid->iIconIndex = *piIconIndex;

        res = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_PICKICON), hwnd, PickIconDlgProc, (LPARAM)pid);

        *piIconIndex = pid->iIconIndex;

        LocalFree(pid);

        return res;
    }

    *piIconIndex = 0;
    *pszIconPath = 0;

    return 0;
}
