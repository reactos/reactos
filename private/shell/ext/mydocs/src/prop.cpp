/*----------------------------------------------------------------------------
/ Title;
/   prop.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Code for property pages of My Documents
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

typedef struct {
    BOOL bChanged;
    BOOL bInitDone;
    TCHAR szMyDocs[ MAX_PATH ];
} CUSTINFO, *LPCUSTINFO;


TCHAR g_szPropTitle[ MAX_PATH + 32 ];

//
// Help ids
//

const static DWORD rgdwHelpTarget[] = {
    IDD_TARGET_TXT,                   IDH_MYDOCS_TARGET,
    IDD_TARGET,                       IDH_MYDOCS_TARGET,
    IDD_FIND,                         IDH_MYDOCS_FIND_TARGET,
    IDD_BROWSE,                       IDH_MYDOCS_BROWSE,
//    IDD_TARGET_GBOX,                  IDH_COMM_GROUPBOX,
    0, 0
};


/*-----------------------------------------------------------------------------
/ GetMessageTitle
/----------------------------------------------------------------------------*/
LPTSTR GetMessageTitle( VOID )
{
    if (!(*g_szPropTitle))
    {
        TCHAR szFormat[ 64 ];
        TCHAR szName[ MAX_PATH ];

        *szFormat = 0;
        LoadString( g_hInstance, IDS_PROP_ERROR_TITLE, szFormat, ARRAYSIZE(szFormat) );
        if (*szFormat)
        {
            *szName = 0;
            GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );
            if (*szName)
            {
                wsprintf( g_szPropTitle, szFormat, szName );
            }
        }

    }

    return g_szPropTitle;
}

/*-----------------------------------------------------------------------------
/ GetRawPersonalPath
/
/ Get personal path directly from the registry so we get the unexpanded
/ version.  pPath is assumed to be MAX_PATH chars big.
/
/----------------------------------------------------------------------------*/
BOOL GetRawPersonalPath( LPTSTR pPath )
{
    HKEY hkey = NULL;
    DWORD dwType, cbSize = (MAX_PATH*sizeof(TCHAR));
    BOOL bRet = FALSE;

    MDTraceEnter( TRACE_PROPS, "GetRawPersonalPath" );

    if (ERROR_SUCCESS!=RegOpenKey( HKEY_CURRENT_USER, c_szRegistrySettings, &hkey ))
    {
        MDTrace(TEXT("Couldn't open %s key"), c_szRegistrySettings);
        goto exit_gracefully;
    }

    *pPath = 0;
    if (ERROR_SUCCESS!=RegQueryValueEx( hkey, c_szPersonal, NULL,
                                        &dwType, (LPBYTE)pPath, &cbSize)
        )
    {
        MDTrace(TEXT("Couldn't query value %s"), c_szPersonal);
        goto exit_gracefully;
    }

    bRet = TRUE;

exit_gracefully:

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeave();

    return bRet;
}




/*-----------------------------------------------------------------------------
/ GetTargetPath
/
/ pPath is assumed to be MAX_PATH big
/
/----------------------------------------------------------------------------*/
void GetTargetPath( HWND hDlg, LPTSTR pPath )
{
    TCHAR szUnExPath[ MAX_PATH ];

    szUnExPath[0] = 0; *pPath = 0;
    GetDlgItemText( hDlg, IDD_TARGET, szUnExPath, MAX_PATH );
    if (szUnExPath[0])
    {
        ExpandEnvironmentStrings( szUnExPath, pPath, MAX_PATH );
    }
}

/*-----------------------------------------------------------------------------
/ InitTargetPage
/----------------------------------------------------------------------------*/
BOOL InitTargetPage( HWND hDlg, LPARAM lParam )
{
    TCHAR szPath[ MAX_PATH ];
    BOOL fResult = FALSE;
    BOOL bRet = TRUE;
    LPCUSTINFO lpci;
    TCHAR szFormat[ MAX_PATH ];
    TCHAR szText[ ARRAYSIZE(szFormat) + MAX_NAME_LEN ];
    TCHAR szName[ MAX_PATH ];

    MDTraceEnter( TRACE_PROPS, "InitTargetPage" );

    // Fill in title/instructions...
    GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );
    if (lstrlen(szName) > MAX_NAME_LEN)
    {
        lstrcpy( &szName[MAX_NAME_LEN], TEXT("...") );
    }

    if (LoadString( g_hInstance,
                    IDS_PROP_INSTRUCTIONS,
                    szFormat,
                    ARRAYSIZE(szFormat)
                   )
        )
    {
        wsprintf( szText, szFormat, szName );
        SetDlgItemText( hDlg, IDD_INSTRUCTIONS, szText );
    }

    // Limit edit field to MAX_PATH-13 characters.  Why -13?
    // Well, 13 is the number of characters in a DOS style 8.3
    // filename with a '\', and CreateDirectory will fail if you try to create
    // a directory that can't at least contain 8.3 file names.
    SendDlgItemMessage( hDlg, IDD_TARGET, EM_SETLIMITTEXT,
                        (WPARAM)(MAX_DIR_PATH), 0L
                       );


    // Check whether path can be changed
    if (!CanChangePersonalPath())
    {
        // Make edit field read only
        SendDlgItemMessage( hDlg, IDD_TARGET, EM_SETREADONLY, (WPARAM)TRUE, 0L );
        ShowWindow( GetDlgItem( hDlg, IDD_FIND ), SW_HIDE );
        ShowWindow( GetDlgItem( hDlg, IDD_BROWSE ), SW_HIDE );

    }

    *szPath = 0;
    if (!GetRawPersonalPath( szPath ))
    {
        int iRes;

        iRes = ShellMessageBox( g_hInstance, hDlg,
                                (LPTSTR)IDS_NO_PATH_TEXT, GetMessageTitle(),
                                MB_OKCANCEL | MB_APPLMODAL | MB_ICONSTOP | MB_TOPMOST
                               );
        if (iRes == IDOK)
        {
            GetDefaultPersonalPath( szPath, TRUE );
            if (SUCCEEDED(ChangePersonalPath( szPath, NULL )))
            {
                StampMyDocsFolder( szPath );
            }
        }

    }

    // Start size thread...
    lpci = (LPCUSTINFO)LocalAlloc( LPTR, sizeof(CUSTINFO) );
    if (lpci)
    {
        lstrcpy( lpci->szMyDocs, szPath );
        SetWindowLong( hDlg, DWL_USER, (LONG)lpci );
    }


    if (szPath[0])
    {
        TCHAR szExpPath[ MAX_PATH ];
        SHFILEINFO sfi;


        ExpandEnvironmentStrings( szPath, szExpPath, MAX_PATH );
        SetDlgItemText( hDlg, IDD_TARGET, szExpPath );
        lstrcpy( lpci->szMyDocs, szExpPath );

        SHGetFileInfo( szExpPath, 0, &sfi, SIZEOF(sfi),
                       SHGFI_ICON|SHGFI_LARGEICON|SHGFI_DISPLAYNAME
                      );

        // icon
        if (sfi.hIcon)
        {
            if (sfi.hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0L))
                DestroyIcon(sfi.hIcon);
        }
    }
    else
    {
        bRet = FALSE;
    }

    if (lpci)
    {
        lpci->bInitDone = TRUE;
    }

    MDTraceLeave();

    return bRet;

}


/*-----------------------------------------------------------------------------
/ MyMoveFiles
/----------------------------------------------------------------------------*/
INT MyMoveFiles( HWND hDlg, LPTSTR pNewPath, LPTSTR pOldPath )
{
    //
    // The MAX_PATH + 1 is to allow room for double-null termination
    //
    TCHAR szSrcPath[ MAX_PATH + 1 ];
    TCHAR szDestPath[ MAX_PATH + 1 ];
    SHFILEOPSTRUCT FileOp;
    INT Result;

    MDTraceEnter( TRACE_PROPS, "MyMoveFiles" );

    memset(&szSrcPath, 0, (MAX_PATH + 1) * sizeof(TCHAR));
    memset(&szDestPath, 0, (MAX_PATH + 1) * sizeof(TCHAR));
    memset(&FileOp, 0, sizeof(SHFILEOPSTRUCT));

    lstrcpy(szSrcPath, pOldPath);
    lstrcat(szSrcPath, TEXT("\\*.*"));

    lstrcpy(szDestPath, pNewPath);

    FileOp.hwnd = hDlg;
    FileOp.wFunc = FO_MOVE;
    FileOp.pFrom = szSrcPath;
    FileOp.pTo = szDestPath;
    FileOp.fFlags = FOF_RENAMEONCOLLISION;

    Result = SHFileOperation(&FileOp);
    MDTrace(TEXT("SHFileOperation() returned 0x%08x"), Result);

    MDTraceLeave();
    return(Result);
}

/*-----------------------------------------------------------------------------
/ HandleApply
/----------------------------------------------------------------------------*/
void HandleApply( HWND hDlg, LPCUSTINFO lpci )
{

    LONG lres = PSNRET_NOERROR;
    TCHAR szPath[ MAX_PATH ];
    TCHAR szText[ MAX_PATH ];
    DWORD dwAttr;

    MDTraceEnter( TRACE_PROPS, "HandleApply" );

    if (!lpci)
        goto exit_gracefully;


    GetDlgItemText( hDlg, IDD_TARGET, szText, ARRAYSIZE(szText) );
    GetTargetPath( hDlg, szPath );
    if (lpci->bChanged && (lstrcmpi(szPath, lpci->szMyDocs)!=0))
    {
        DWORD dwRes = IsPathGoodMyDocsPath( hDlg, szPath );

        //
        // Make sure they haven't changed it to be something on
        // the desktop...
        //

        switch( dwRes )
        {
        case PATH_IS_READONLY:
            //
            // Users generally want to write to My Documents
            //
            
            ShellMessageBox( g_hInstance, hDlg,
                            (LPTSTR)IDS_READONLY_PATH, GetMessageTitle(),
                            MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_DESKTOP:
            //
            // Looks like someone is trying to set the path
            // to be on the desktop.  Don't allow them to!
            //

            ShellMessageBox( g_hInstance, hDlg,
                             (LPTSTR)IDS_NODESKTOP_FOLDERS, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_SYSTEM:
        case PATH_IS_WINDOWS:
            //
            // Looks like someone is trying to set the path
            // to be the windows directory.  Don't allow them to!
            //

            ShellMessageBox( g_hInstance, hDlg,
                             (LPTSTR)IDS_NOWINDIR_FOLDER, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_PROFILE:
            //
            // Looks like someone is trying to set the path
            // to be the profile directory.  Don't allow them to!
            //

            ShellMessageBox( g_hInstance, hDlg,
                             (LPTSTR)IDS_NOPROFILEDIR_FOLDER, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_NONEXISTENT:
        case PATH_IS_NONDIR:
        case PATH_IS_GOOD:
            dwAttr = GetFileAttributes( szPath );

            if (dwAttr == 0xFFFFFFFF)
            {
                // Ask user if we should create the directory...
                dwAttr = QueryCreateTheDirectory( hDlg, szPath );
            }

            if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (lstrcmpi( szText, lpci->szMyDocs )!=0 )
                {
                    if (SUCCEEDED(ChangePersonalPath( szText, lpci->szMyDocs )))
                    {


                        //
                        // Pop up message box asking user whether to move their
                        // documents from the old location to the new one...
                        //
                        if (IDYES == ShellMessageBox( g_hInstance, hDlg,
                                        (LPTSTR)IDS_MOVE_DOCUMENTS,
                                        (LPTSTR)IDS_MOVE_DOCUMENTS_TITLE,
                                        MB_YESNO | MB_ICONQUESTION |
                                            MB_APPLMODAL | MB_TOPMOST,
                                        lpci->szMyDocs,
                                        szText))
                        {
                            //
                            // Need to move all the files...
                            //
                            if (MyMoveFiles( hDlg, szText, lpci->szMyDocs )) {
                                ShellMessageBox(
                                    g_hInstance,
                                    hDlg,
                                    (LPTSTR) IDS_MOVE_ERROR,
                                    (LPTSTR) IDS_MOVE_ERROR_TITLE,
                                    MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST,
                                    szText,
                                    lpci->szMyDocs
                                );
                            }

                        }



                        StampMyDocsFolder( szText );
                    }
                }

            }
            else if (dwAttr)
            {
                DWORD id = IDS_NONEXISTENT_FOLDER;

                // The user entered a path that doesn't exist or isn't a
                // directory...

                if ((dwAttr!=0xFFFFFFFF) && (dwAttr!=0))
                {
                    id = IDS_NOT_DIRECTORY;
                }

                ShellMessageBox( g_hInstance, hDlg,
                                 (LPTSTR)id, (LPTSTR)IDS_INVALID_TITLE,
                                 MB_OK | MB_APPLMODAL | MB_ICONERROR | MB_TOPMOST
                                );
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            else
            {
                ShellMessageBox( g_hInstance, hDlg,
                                 (LPTSTR)IDS_GENERAL_BADDIR, (LPTSTR)IDS_INVALID_TITLE,
                                 MB_OK | MB_APPLMODAL | MB_ICONSTOP | MB_TOPMOST
                                );
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            break;

        default:
            //
            // Looks like someone is trying to set the path
            // to something isn't allowed.  Don't allow them to!
            //
            ShellMessageBox( g_hInstance, hDlg,
                             (LPTSTR)IDS_NOTALLOWED_FOLDERS, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;
        }

    }

    //
    // Mark the item as not changed if it was successful...
    //
    if (lres == PSNRET_NOERROR)
    {
        lpci->bChanged = FALSE;
        lstrcpy(lpci->szMyDocs, szPath);
    }

exit_gracefully:

    SetWindowLong( hDlg, DWL_MSGRESULT, lres );
    MDTraceLeave();

}


SAFEARRAY * MakeSafeArrayFromData(const BYTE *pData,DWORD cbData)
{
    SAFEARRAY * psa;

    if (!pData || 0 == cbData)
        return NULL;  // nothing to do

    // create a one-dimensional safe array
    psa = SafeArrayCreateVector(VT_UI1,0,cbData);
    MDTraceAssert(psa);

    if (psa) {
        // copy data into the area in safe array reserved for data
        // Note we party directly on the pointer instead of using locking/
        // unlocking functions.  Since we just created this and no one
        // else could possibly know about it or be using it, this is OK.

        MDTraceAssert(psa->pvData);
        memcpy(psa->pvData,pData,cbData);
    }

    return psa;
}
BOOL _InitVARIANTFromPidl(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    UINT cb = ILGetSize(pidl);
    SAFEARRAY* psa = MakeSafeArrayFromData((const BYTE *)pidl, cb);
    if (psa) {
        MDTraceAssert(psa->cDims == 1);
        // MDTraceAssert(psa->cbElements == cb);
        MDTraceAssert(ILGetSize((LPCITEMIDLIST)psa->pvData)==cb);
        VariantInit(pvar);
        pvar->vt = VT_ARRAY|VT_UI1;
        pvar->parray = psa;
        return TRUE;
    }

    return FALSE;
}

HRESULT SelectPidlInSFV(IShellFolderViewDual *psfv, LPCITEMIDLIST pidl, DWORD dwOpts)
{
    HRESULT hres;
    VARIANT var;

    if (_InitVARIANTFromPidl(&var, pidl))
    {
        hres = psfv->SelectItem(&var, dwOpts);
        VariantClear(&var);
    }

    return hres;
}


HRESULT OpenContainingFolderAndGetShellFolderView(LPCITEMIDLIST pidlFolder, IShellFolderViewDual **ppsfv)
{
    IWebBrowserApp *pauto;
    HRESULT hres;

    *ppsfv = NULL;

    if (SUCCEEDED(hres = SHGetIDispatchForFolder(pidlFolder, &pauto)))
    {
        // We have IDispatch for window, now try to get one for
        // the folder object...
        IDispatch * pautoDoc;
        HWND hwnd;

        if (SUCCEEDED(pauto->get_HWND((LONG*)&hwnd)))
        {
            // Make sure we make this the active window
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOWNORMAL);

        }

        if (SUCCEEDED(hres = pauto->get_Document(&pautoDoc)))
        {
            hres = pautoDoc->QueryInterface(IID_IShellFolderViewDual, (LPVOID*)ppsfv);
            pautoDoc->Release();
        }
        pauto->Release();
    }
    return hres;
}


//
// Stolen (and modified) from link.c in shell32.dll
//
// Opens the cabinet with the item pointed to by the link being selected
// and selectes that item
//
// History:
//  03-25-93 SatoNa     Sub-object support.
//
void _FindTarget(HWND hDlg, LPTSTR pPath)
{
    LPITEMIDLIST pidl;
    USHORT uSave;
    LPCITEMIDLIST pidlDesk;
    LPITEMIDLIST pidlLast;
    BOOL fIsDesktopDir;
    IShellFolderViewDual *psfv;

    pidl = ILCreateFromPath( pPath );

    if (!pidl)
        return;

    pidlLast = ILFindLastID(pidl);

    // get the folder, special case for root objects (My Computer, Network)
    // hack off the end if it is not the root item
    if (pidl != pidlLast)
    {
        uSave = pidlLast->mkid.cb;
        pidlLast->mkid.cb = 0;
    }
    else
        uSave = 0;

    SHGetSpecialFolderLocation( NULL, CSIDL_DESKTOPDIRECTORY, (LPITEMIDLIST *)&pidlDesk );
    fIsDesktopDir = pidlDesk && ILIsEqual(pidl, pidlDesk);

    if (fIsDesktopDir || !uSave)  // if it's in the desktop dir or pidl == pidlLast (uSave == 0 from above)
    {
        //
        // It's on the desktop...
        //

        ShellMessageBox( g_hInstance, hDlg,
                         (LPTSTR)IDS_ON_DESKTOP, (LPTSTR)IDS_FIND_TITLE,
                         MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_TOPMOST
                        );
    }
    else
    {
        if (SUCCEEDED(OpenContainingFolderAndGetShellFolderView(uSave? pidl : pidlDesk, &psfv)))
        {
            if (uSave)
                pidlLast->mkid.cb = uSave;
            SelectPidlInSFV(psfv, pidlLast, SVSI_SELECT | SVSI_FOCUSED | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE);
            psfv->Release();
        }
    }

    ILFree(pidl);
}

/*-----------------------------------------------------------------------------
/ _BrowseCallbackProc
/----------------------------------------------------------------------------*/
int _BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
    MDTraceEnter( TRACE_PROPS, "_BrowseCallbackProc" );

    if ((uMsg == BFFM_SELCHANGED) && lParam)
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwRes;

        szPath[0] = 0;
        SHGetPathFromIDList( (LPITEMIDLIST)lParam, szPath );

        dwRes = IsPathGoodMyDocsPath( hwnd, szPath );

        if (dwRes != PATH_IS_GOOD)
        {
            TCHAR szStatus[ 128 ];

            SendMessage( hwnd, BFFM_ENABLEOK, 0, 0 );

            szStatus[0] = 0;
            LoadString( g_hInstance, IDS_NOSHELLEXT_FOLDERS, szStatus, ARRAYSIZE(szStatus) );
            SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szStatus );
        }
        else
        {
            SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, 0 );
        }

    }

    MDTraceLeave();
    return 0;
}

/*-----------------------------------------------------------------------------
/ _DoBrowse
/----------------------------------------------------------------------------*/
void _DoBrowse( HWND hDlg, int idCtl, LPCUSTINFO lpci )
{
    BROWSEINFO bi;
    LPITEMIDLIST pidl = NULL;
    TCHAR szName[ MAX_PATH ];
    TCHAR szTitle[ 128 ];

    MDTraceEnter( TRACE_PROPS, "_DoBrowse" );

    szTitle[0] = 0;
    LoadString( g_hInstance, IDS_BROWSE_TITLE, szTitle, ARRAYSIZE(szTitle) );

    bi.hwndOwner = hDlg;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szName;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
    bi.lParam = 0;
    bi.iImage = 0;
    bi.lpfn = (BFFCALLBACK)_BrowseCallbackProc;

    pidl = SHBrowseForFolder( &bi );

    if (pidl)
    {
        if (SHGetPathFromIDList( pidl, szName ))
        {
            SetDlgItemText( hDlg, idCtl, szName );
            lpci->bChanged = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
        }
    }

    DoILFree( pidl );

    MDTraceLeave();
}


/*-----------------------------------------------------------------------------
/ TargetDlgProc
/----------------------------------------------------------------------------*/
BOOL
CALLBACK
TargetDlgProc( HWND hDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam
                )
{
    LPCUSTINFO lpci = (LPCUSTINFO)GetWindowLong( hDlg, DWL_USER );

    switch (uMsg)
    {
    case WM_INITDIALOG:
        OleInitialize(NULL);
        *g_szPropTitle = 0;
        if (!InitTargetPage( hDlg, lParam ))
        {
            ShellMessageBox( g_hInstance, hDlg,
                             (LPTSTR)IDS_INIT_FAILED_TEXT, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_APPLMODAL | MB_TOPMOST
                            );
            return 0;
        }
        return 1;

    case WM_DESTROY:
    {
        LocalFree( lpci );
        SetWindowLong( hDlg, DWL_USER, 0 );
        OleUninitialize();
    }
        return 1;

    case WM_COMMAND:
    {
        TCHAR szPath[ MAX_PATH ];
        WORD wNotify = HIWORD(wParam),wID = LOWORD(wParam);

        switch( wID )
        {


        case IDD_TARGET:
        if ((wNotify == EN_UPDATE) && lpci && (lpci->bInitDone) && (!(lpci->bChanged)))
            {
                lpci->bChanged = TRUE;
                PropSheet_Changed( GetParent( hDlg ), hDlg );
            }
            return 1;

        case IDD_FIND:
            if (wNotify == BN_CLICKED)
            {
                DWORD dwAttrb;

                szPath[0] = 0;
                GetTargetPath( hDlg, szPath );
                dwAttrb = GetFileAttributes( szPath );
                if ( szPath[0] &&
                     (dwAttrb!=0xFFFFFFFF) &&
                     (dwAttrb & FILE_ATTRIBUTE_DIRECTORY)
                    )
                {
                    _FindTarget( hDlg, szPath );
                }
                else
                {
                    SetDlgItemText( hDlg, IDD_TARGET, lpci->szMyDocs );
                    _FindTarget( hDlg, lpci->szMyDocs );
                }
            }
            return 1;

        case IDD_BROWSE:
            if (wNotify == BN_CLICKED)
            {
                wID = IDD_TARGET;
                _DoBrowse( hDlg, wID, lpci );
            }
            return 1;

        }
    }
        break;


    case WM_HELP:               /* F1 or title-bar help button */
        if ((((LPHELPINFO)lParam)->iCtrlId != IDD_ITEMICON )     &&
            (((LPHELPINFO)lParam)->iCtrlId != IDD_INSTRUCTIONS ) &&
            (((LPHELPINFO)lParam)->iCtrlId != IDC_TARGET_GBOX ))
        {
            WinHelp( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD) (LPVOID) &rgdwHelpTarget[0]
                    );
        }
        break;


    case WM_CONTEXTMENU:        /* right mouse click */
        {
            POINT p;
            HWND hwndChild;
            INT ctrlid;

            //
            // Get the point where the user clicked...
            //

            p.x = (LONG)(LOWORD(lParam));
            p.y = (LONG)(HIWORD(lParam));

            //
            // Now, map that to a child control if possible...
            //

            ScreenToClient( hDlg, &p );
            hwndChild = ChildWindowFromPoint( (HWND)wParam, p );
            ctrlid = GetDlgCtrlID( hwndChild );

            //
            // Don't put up the help context menu for the items
            // that don't have help...
            //
            if (( ctrlid != IDD_ITEMICON )     &&
                ( ctrlid != IDD_INSTRUCTIONS ))
            {
                WinHelp( (HWND) wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (DWORD) (LPVOID) &rgdwHelpTarget[0]
                        );
            }

        }

        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {

        case PSN_APPLY:
        {
            HandleApply( hDlg, lpci );
        }
        return 1;

        }
        break;

    }


    return 0;


}
