#include "precomp.hxx"
#pragma hdrstop

#include "util.h"
#include "dll.h"
#include "resource.h"
#include "prop.h"

#include <shellids.h>   // IDH_ values
#include "shlguidp.h"
#include "inetreg.h"

STDAPI SHGetIDispatchForFolder(LPCITEMIDLIST pidl, IWebBrowserApp **ppauto);    // <shdocvw.h>

typedef struct {
    HWND hDlg;
    BOOL bDirty;
    BOOL bInitDone;
    IPersistFolder* ppf;
    TCHAR szMyDocs[ MAX_PATH ];
} CUSTINFO;


TCHAR g_szPropTitle[ MAX_PATH + 32 ];

const static DWORD rgdwHelpTarget[] = {
    IDD_TARGET_TXT,                   IDH_MYDOCS_TARGET,
    IDD_TARGET,                       IDH_MYDOCS_TARGET,
    IDD_FIND,                         IDH_MYDOCS_FIND_TARGET,
    IDD_BROWSE,                       IDH_MYDOCS_BROWSE,
    IDD_RESET,                        IDH_MYDOCS_RESET,
    0, 0
};


LPTSTR GetMessageTitle( VOID )
{
    if (!(*g_szPropTitle))
    {
        TCHAR szFormat[ 64 ];
        TCHAR szName[ MAX_PATH ];

        LoadString( g_hInstance, IDS_PROP_ERROR_TITLE, szFormat, ARRAYSIZE(szFormat) );
        GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );
        wnsprintf( g_szPropTitle, ARRAYSIZE(g_szPropTitle), szFormat, szName );
    }

    return g_szPropTitle;
}

// pPath is assumed to be MAX_PATH big
void GetTargetPath( HWND hDlg, LPTSTR pPath )
{
    TCHAR szUnExPath[ MAX_PATH ];

    *pPath = 0;
    szUnExPath[0] = 0; 
    GetDlgItemText( hDlg, IDD_TARGET, szUnExPath, ARRAYSIZE(szUnExPath) );
    if (szUnExPath[0])
    {
        // Turn "c:" into "c:\", but don't change other paths:
        PathAddBackslash(szUnExPath);
        PathRemoveBackslash(szUnExPath);
        ExpandEnvironmentStrings( szUnExPath, pPath, MAX_PATH );
    }
}

// Check known key in the registry to see if policy has disabled changing
// of My Docs location.

BOOL CanChangePersonalPath( void )
{
    HKEY hkey;
    BOOL bChange = TRUE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), 0, KEY_READ, &hkey))
    {
        bChange = (ERROR_SUCCESS != RegQueryValueEx(hkey, TEXT("DisablePersonalDirChange"), NULL, NULL, NULL, NULL));
        RegCloseKey(hkey);
    }
    return bChange;
}

BOOL InitTargetPage( HWND hDlg, LPARAM lParam )
{
    TCHAR szPath[ MAX_PATH ];
    CUSTINFO *pci;
    TCHAR szFormat[ MAX_PATH ];
    TCHAR szText[ ARRAYSIZE(szFormat) + MAX_NAME_LEN ];
    TCHAR szName[ MAX_PATH ];

    *g_szPropTitle = 0;

    pci = (CUSTINFO *)LocalAlloc(LPTR, sizeof(*pci));
    if (pci == NULL)
        return FALSE;

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pci);
    pci->hDlg = hDlg;


    // Fill in title/instructions...
    GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );
    if (lstrlen(szName) > MAX_NAME_LEN)
    {
        lstrcpy( &szName[MAX_NAME_LEN], TEXT("...") );
    }

    LoadString(g_hInstance, IDS_PROP_INSTRUCTIONS, szFormat, ARRAYSIZE(szFormat));

    wnsprintf(szText, ARRAYSIZE(szText), szFormat, szName);
    SetDlgItemText( hDlg, IDD_INSTRUCTIONS, szText );

    // Limit edit field to MAX_PATH-13 characters.  Why -13?
    // Well, 13 is the number of characters in a DOS style 8.3
    // filename with a '\', and CreateDirectory will fail if you try to create
    // a directory that can't at least contain 8.3 file names.
    SendDlgItemMessage( hDlg, IDD_TARGET, EM_SETLIMITTEXT, MAX_DIR_PATH, 0 );

    // Check whether path can be changed
    if (CanChangePersonalPath())
    {
        // set up autocomplete in target edit box:
        HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFolder, (void **)&(pci->ppf));
        if (SUCCEEDED(hr))
        {
            IAutoComplete2* pac;

            // Create the AutoComplete Object
            hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_IAutoComplete2, (void **)&pac);
            if (SUCCEEDED(hr))
            {
                hr = pac->Init(GetDlgItem(hDlg, IDD_TARGET), pci->ppf, NULL, NULL);

                // Set the autocomplete options
                DWORD dwOptions = 0;
                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
                {
                    dwOptions |= ACO_AUTOAPPEND;
                }

                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
                {
                    dwOptions |= ACO_AUTOSUGGEST;
                }

                pac->SetOptions(dwOptions);
                pac->Release();
            }
        }
    }
    else
    {
        // Make edit field read only
        SendDlgItemMessage( hDlg, IDD_TARGET, EM_SETREADONLY, (WPARAM)TRUE, 0L );
        ShowWindow( GetDlgItem( hDlg, IDD_RESET ), SW_HIDE );
        ShowWindow( GetDlgItem( hDlg, IDD_FIND ), SW_HIDE );
        ShowWindow( GetDlgItem( hDlg, IDD_BROWSE ), SW_HIDE );
    }

    SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);

    if (szPath[0])
    {
        SetDlgItemText(hDlg, IDD_TARGET, szPath);
        lstrcpy(pci->szMyDocs, szPath);
    }

    LPITEMIDLIST pidlMyDocs = MyDocsIDList();
    if (pidlMyDocs)
    {
        SHFILEINFO sfi;

        SHGetFileInfo((LPCTSTR)pidlMyDocs, 0, &sfi, SIZEOF(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_PIDL);
        if (sfi.hIcon)
        {
            if (sfi.hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0L))
                DestroyIcon(sfi.hIcon);
        }
        ILFree(pidlMyDocs);
    }

    pci->bInitDone = TRUE;

    return TRUE;
}

int MyMoveFiles( HWND hDlg, LPCTSTR pNewPath, LPCTSTR pOldPath, LPCTSTR pMyPicsPath, BOOL *pfResetMyPics )
{
    TCHAR           szSrcPath[ MAX_PATH + 1 ];    // +1 for double null
    TCHAR           szDestPath[ MAX_PATH ];
    SHFILEOPSTRUCT  FileOp;
    BOOL            fMyPicsCollide = FALSE;

    memset(&szSrcPath, 0, sizeof(szSrcPath));
    memset(&szDestPath, 0, sizeof(szDestPath));
    memset(&FileOp, 0, sizeof(FileOp));

    ASSERT( pfResetMyPics );
    if( *pfResetMyPics && pMyPicsPath && *pMyPicsPath )
    {
        LPTSTR pszMyPicsName = PathFindFileName( pMyPicsPath );
        PathCombine( szDestPath, pNewPath, pszMyPicsName );

        DWORD dwAtt;
        if( PathFileExistsAndAttributes( szDestPath, &dwAtt ))
        {
            fMyPicsCollide = (FILE_ATTRIBUTE_DIRECTORY == (FILE_ATTRIBUTE_DIRECTORY & dwAtt));
        }
    }
    
    FileOp.hwnd = hDlg;
    FileOp.wFunc = FO_MOVE;
    // While we'll no longer create a duplicate MyPictures, using this
    // flag will create "Copy of..." for other duplicated folders. Adding
    // full folder merging capabilities to SHFileOperation would require
    // numerous changes to the copy engine.
    FileOp.fFlags = FOF_RENAMEONCOLLISION;

    if( fMyPicsCollide )
    {
        ChangeMyPicsPath( szDestPath, pMyPicsPath );    // szDestPath set above

        // Don't want ResetMyPictures to be called later on
        *pfResetMyPics = FALSE;
        
        // Move items in current MyPics to new location
        PathCombine( szSrcPath, pMyPicsPath, TEXT("*.*") );
        FileOp.pFrom = szSrcPath;
        FileOp.pTo = szDestPath;        

        int iResult = SHFileOperation( &FileOp );
        if( 0 != iResult || FileOp.fAnyOperationsAborted )
        {
            return iResult;
        }

        RemoveDirectory( pMyPicsPath );

        // FileOp.pFrom is double \0 terminated, so must reset this
        memset(&szSrcPath, 0, sizeof(szSrcPath));
    }
	
    // Move all other items
    PathCombine(szSrcPath, pOldPath, TEXT("*.*"));
    lstrcpy(szDestPath, pNewPath);

    FileOp.pFrom = szSrcPath;
    FileOp.pTo = szDestPath;

    return SHFileOperation(&FileOp);
}

// Ask the user if they want to create the directory of a given path.
// Returns TRUE if the user decided to create it, FALSE if not.
// If TRUE, the dir attributes are returned in pdwAttr.

BOOL QueryCreateTheDirectory(HWND hDlg, LPCTSTR pPath, DWORD* pdwAttr)
{
    *pdwAttr = 0;
    UINT id = ShellMessageBox(g_hInstance, hDlg,
                              (LPTSTR)IDS_CREATE_FOLDER, (LPTSTR)IDS_CREATE_FOLDER_TITLE,
                              MB_YESNO | MB_ICONQUESTION, pPath );
    if (IDYES == id)
    {
        // user asked us to create the folder
        if (ERROR_SUCCESS == SHCreateDirectoryEx(hDlg, pPath, NULL))
            *pdwAttr = GetFileAttributes(pPath);
    }
    return IDYES == id;
}

void _MaybeUnpinOldFolder(LPCTSTR pszPath, HWND hwnd, BOOL fPrompt)
{
    //
    // Convert the path to canonical UNC form (the CSC and CSCUI
    // functions require the path to be in this form)
    //
    // WNetGetUniversalName fails if you give it a path that's already
    // in canonical UNC form, so in the failure case just try using
    // pszPath.  CSCQueryFileStatus will validate it.
    //
    LPCTSTR pszUNC;

    struct {
       UNIVERSAL_NAME_INFO uni;
       TCHAR szBuf[MAX_PATH];
    } s;
    DWORD cbBuf = SIZEOF(s);

    if (ERROR_SUCCESS == WNetGetUniversalName(pszPath, UNIVERSAL_NAME_INFO_LEVEL,
                                &s, &cbBuf))
    {
        pszUNC = s.uni.lpUniversalName;
    }
    else
    {
        pszUNC = pszPath;
    }

    //
    // Ask CSC if the folder is pinned for this user
    //
    DWORD dwHintFlags = 0;
    if (CSCQueryFileStatus(pszUNC, NULL, NULL, &dwHintFlags))
    {
        if (dwHintFlags & FLAG_CSC_HINT_PIN_USER)
        {
            //
            // Yes; figure out if we should unpin it
            //
            BOOL fUnpin;

            if (fPrompt)
            {
                //
                // Give the unconverted path name in the message box, since
                // that's the name the user knows
                //
                UINT id = ShellMessageBox(g_hInstance, hwnd,
                                  (LPTSTR)IDS_UNPIN_OLDTARGET, (LPTSTR)IDS_UNPIN_OLD_TITLE,
                                  MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON2,
                                  pszPath);

                fUnpin = (id == IDNO);
            }
            else
            {
                fUnpin = TRUE;
            }

            if (fUnpin)
            {
                USES_CONVERSION;

                CSCUIRemoveFolderFromCache(T2CW(pszUNC), 0, NULL, 0);
            }
        }
    }
}

void _DoApply(CUSTINFO *pci)
{
    LONG lres = PSNRET_NOERROR;
    TCHAR szPath[ MAX_PATH ], szText[ MAX_PATH ];
    DWORD dwAttr;

    GetDlgItemText( pci->hDlg, IDD_TARGET, szText, ARRAYSIZE(szText) );
    // Turn "c:" into "c:\", but don't change other paths:
    PathAddBackslash(szText);
    PathRemoveBackslash(szText);

    GetTargetPath( pci->hDlg, szPath );
    if (pci->bDirty && (lstrcmpi(szPath, pci->szMyDocs) != 0))
    {
        DWORD dwRes = IsPathGoodMyDocsPath( pci->hDlg, szPath );

        // all of the special cases

        switch( dwRes )
        {
        case PATH_IS_DESKTOP:   // desktop is not good
            ShellMessageBox( g_hInstance, pci->hDlg,
                             (LPTSTR)IDS_NODESKTOP_FOLDERS, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_SYSTEM:
        case PATH_IS_WINDOWS:   // these would be bad
            ShellMessageBox( g_hInstance, pci->hDlg,
                             (LPTSTR)IDS_NOWINDIR_FOLDER, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST);
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_PROFILE:   // profile is bad
            ShellMessageBox( g_hInstance, pci->hDlg,
                             (LPTSTR)IDS_NOPROFILEDIR_FOLDER, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;

        case PATH_IS_NONEXISTENT:
        case PATH_IS_NONDIR:
        case PATH_IS_GOOD:

            dwAttr = GetFileAttributes( szPath );

            if (dwAttr == 0xFFFFFFFF)
            {
                // Ask user if we should create the directory...
                if (FALSE == QueryCreateTheDirectory(pci->hDlg, szPath, &dwAttr))
                {
                    // They don't want to create the directory.. break here
                    lres = PSNRET_INVALID_NOCHANGEPAGE;
                    break;
                }
            }

            if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                // insure that both paths don't have backslashes so we can proceed safely:
                PathRemoveBackslash(szText);
                PathRemoveBackslash(pci->szMyDocs);
                if (lstrcmpi( szText, pci->szMyDocs )!=0 )
                {
                    // if PathCommonPrefix() returns the same number of chars as szMyDocs,
                    // then szText must be a subdir
                    TCHAR szCommon[MAX_PATH], szOldMyPics[MAX_PATH];
                    BOOL fSubDir = (PathCommonPrefix(pci->szMyDocs, szText, szCommon) == lstrlen(pci->szMyDocs));
                    BOOL fResetMyPics = TRUE;

                    SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szOldMyPics);

                    if (SUCCEEDED(ChangePersonalPath(szText, pci->szMyDocs, &fResetMyPics)))
                    {
                        BOOL fPrompt = TRUE;

                        if (fSubDir)
                        {
                            // can't move old content to a subdir
                            ShellMessageBox(g_hInstance, pci->hDlg,
                                    (LPTSTR) IDS_CANT_MOVE_TO_SUBDIR, (LPTSTR) IDS_MOVE_DOCUMENTS_TITLE,
                                    MB_OK | MB_ICONINFORMATION | MB_TOPMOST,
                                    pci->szMyDocs);
                        }
                        else if (IDYES == ShellMessageBox(g_hInstance, pci->hDlg,
                                        (LPTSTR)IDS_MOVE_DOCUMENTS,
                                        (LPTSTR)IDS_MOVE_DOCUMENTS_TITLE,
                                        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST,
                                        pci->szMyDocs, szText))
                        {
                            // move old mydocs content -- returns 0 on success
                            if (0 != MyMoveFiles(pci->hDlg, szText, pci->szMyDocs, szOldMyPics, &fResetMyPics)) 
                            {
                                ShellMessageBox(g_hInstance, pci->hDlg,
                                    (LPTSTR) IDS_MOVE_ERROR, (LPTSTR) IDS_MOVE_ERROR_TITLE,
                                    MB_OK | MB_ICONSTOP | MB_TOPMOST,
                                    szText, pci->szMyDocs);
                            }
                            else
                            {
                                //
                                // Move succeeded, the old target dir is now empty, so
                                // no need to prompt about unpinning it (just go ahead
                                // and do it).
                                //

                                fPrompt = FALSE;

                                // If the move succeeded, then there is no need to reset
                                // mypics, because if it was a subdirectory, then it
                                // was moved and the shell copyhook will have noticed
                                // that mypics has been moved
                                fResetMyPics = FALSE;
                            }
                        }
                        if (fResetMyPics)
                        {
                            ResetMyPics(szOldMyPics);
                        }

                        if (!fSubDir && pci->szMyDocs[0])
                        {
                            //
                            // If the old folder was pinned, offer to unpin it.
                            //
                            // (Do this only if new target is not a subdir of the 
                            // old target, since otherwise we'd end up unpinning
                            // the new target as well.)
                            //

                            _MaybeUnpinOldFolder(pci->szMyDocs, pci->hDlg, fPrompt);
                        }
                    }
                    else
                    {
                        ShellMessageBox( g_hInstance, pci->hDlg,
                                         (LPTSTR)IDS_GENERAL_BADDIR, (LPTSTR)IDS_INVALID_TITLE,
                                         MB_OK | MB_ICONSTOP | MB_TOPMOST);
                        lres = PSNRET_INVALID_NOCHANGEPAGE;
                    }
                }

            }
            else if (dwAttr)
            {
                DWORD id = IDS_NONEXISTENT_FOLDER;

                // The user entered a path that doesn't exist or isn't a
                // directory...

                if ((dwAttr != 0xFFFFFFFF) && (dwAttr != 0))
                {
                    id = IDS_NOT_DIRECTORY;
                }

                ShellMessageBox( g_hInstance, pci->hDlg,
                                 (LPTSTR)id, (LPTSTR)IDS_INVALID_TITLE,
                                 MB_OK | MB_ICONERROR | MB_TOPMOST);
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            else
            {
                ShellMessageBox( g_hInstance, pci->hDlg,
                                 (LPTSTR)IDS_GENERAL_BADDIR, (LPTSTR)IDS_INVALID_TITLE,
                                 MB_OK | MB_ICONSTOP | MB_TOPMOST);
                lres = PSNRET_INVALID_NOCHANGEPAGE;
            }
            break;

        default:
            //
            // Looks like someone is trying to set the path
            // to something isn't allowed.  Don't allow them to!
            //
            ShellMessageBox( g_hInstance, pci->hDlg,
                             (LPTSTR)IDS_NOTALLOWED_FOLDERS, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST
                            );
            lres = PSNRET_INVALID_NOCHANGEPAGE;
            break;
        }
    }

    if (lres == PSNRET_NOERROR)
    {
        pci->bDirty = FALSE;
        lstrcpy(pci->szMyDocs, szPath);
    }

    SetWindowLongPtr( pci->hDlg, DWLP_MSGRESULT, lres );
}


SAFEARRAY * MakeSafeArrayFromData(const BYTE *pData,DWORD cbData)
{
    SAFEARRAY * psa = SafeArrayCreateVector(VT_UI1,0,cbData);
    if (psa) 
    {
        memcpy(psa->pvData, pData, cbData);
    }
    return psa;
}

BOOL _InitVARIANTFromPidl(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    UINT cb = ILGetSize(pidl);
    SAFEARRAY* psa = MakeSafeArrayFromData((const BYTE *)pidl, cb);
    if (psa) 
    {
        VariantInit(pvar);
        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
        return TRUE;
    }

    return FALSE;
}

HRESULT SelectPidlInSFV(IShellFolderViewDual *psfv, LPCITEMIDLIST pidl, DWORD dwOpts)
{
    HRESULT hres = E_FAIL;
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
    *ppsfv = NULL;

    IWebBrowserApp *pauto;
    HRESULT hres = SHGetIDispatchForFolder(pidlFolder, &pauto);
    if (SUCCEEDED(hres))
    {
        // We have IDispatch for window, now try to get one for
        // the folder object...
        HWND hwnd;

        if (SUCCEEDED(pauto->get_HWND((LONG*)&hwnd)))
        {
            // Make sure we make this the active window
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOWNORMAL);

        }
        IDispatch * pautoDoc;
        hres = pauto->get_Document(&pautoDoc);
        if (SUCCEEDED(hres))
        {
            hres = pautoDoc->QueryInterface(IID_IShellFolderViewDual, (void **)ppsfv);
            pautoDoc->Release();
        }
        pauto->Release();
    }
    return hres;
}

//
// Stolen (and modified) from link.c in shell32.dll
//
void _FindTarget(HWND hDlg, LPTSTR pPath)
{
    USHORT uSave;

    LPITEMIDLIST pidl = ILCreateFromPath( pPath );
    if (!pidl)
    {
        ShellMessageBox( g_hInstance, hDlg,
                     (LPTSTR)IDS_GENERAL_BADDIR, (LPTSTR)IDS_INVALID_TITLE,
                     MB_OK | MB_ICONSTOP | MB_TOPMOST);
        return;
    }

    LPITEMIDLIST pidlLast = ILFindLastID(pidl);

    // get the folder, special case for root objects (My Computer, Network)
    // hack off the end if it is not the root item
    if (pidl != pidlLast)
    {
        uSave = pidlLast->mkid.cb;
        pidlLast->mkid.cb = 0;
    }
    else
        uSave = 0;

    LPITEMIDLIST pidlDesk;
    if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, &pidlDesk)))
    {
        BOOL fIsDesktopDir = pidlDesk && ILIsEqual(pidl, pidlDesk);

        if (fIsDesktopDir || !uSave)  // if it's in the desktop dir or pidl == pidlLast (uSave == 0 from above)
        {
            //
            // It's on the desktop...
            //

            ShellMessageBox(g_hInstance, hDlg, (LPTSTR)IDS_ON_DESKTOP, (LPTSTR)IDS_FIND_TITLE,
                             MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        }
        else
        {
            IShellFolderViewDual *psfv;
            if (SUCCEEDED(OpenContainingFolderAndGetShellFolderView(uSave ? pidl : pidlDesk, &psfv)))
            {
                if (uSave)
                    pidlLast->mkid.cb = uSave;
                SelectPidlInSFV(psfv, pidlLast, SVSI_SELECT | SVSI_FOCUSED | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE);
                psfv->Release();
            }
        }
        ILFree(pidlDesk);
    }

    ILFree(pidl);
}

int _BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
    if ((uMsg == BFFM_SELCHANGED) && lParam)
    {
        TCHAR szPath[ MAX_PATH ];

        szPath[0] = 0;
        SHGetPathFromIDList( (LPITEMIDLIST)lParam, szPath );

        DWORD dwRes = IsPathGoodMyDocsPath( hwnd, szPath );

        if (dwRes == PATH_IS_GOOD || dwRes == PATH_IS_MYDOCS)
        {
            SendMessage( hwnd, BFFM_ENABLEOK, 0, (LPARAM)TRUE);
            SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, 0 );
        }
        else
        {
            TCHAR szStatus[ 128 ];

            SendMessage( hwnd, BFFM_ENABLEOK, 0, 0 );

            szStatus[0] = 0;
            LoadString( g_hInstance, IDS_NOSHELLEXT_FOLDERS, szStatus, ARRAYSIZE(szStatus) );
            SendMessage( hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szStatus );
        }
    }

    return 0;
}

void _MakeDirty(CUSTINFO *pci)
{
    pci->bDirty = TRUE;
    PropSheet_Changed( GetParent( pci->hDlg ), pci->hDlg );
}


void _DoFind(CUSTINFO *pci)
{
    TCHAR szPath[ MAX_PATH ];
    GetTargetPath( pci->hDlg, szPath );
    DWORD dwAttrb = GetFileAttributes( szPath );
    if ( szPath[0] && (dwAttrb != 0xFFFFFFFF) && (dwAttrb & FILE_ATTRIBUTE_DIRECTORY) )
    {
        _FindTarget( pci->hDlg, szPath );
    }
    else
    {
        ShellMessageBox( g_hInstance, pci->hDlg,
                     (LPTSTR)IDS_GENERAL_BADDIR, (LPTSTR)IDS_INVALID_TITLE,
                     MB_OK | MB_ICONSTOP | MB_TOPMOST);
    }
}

void _DoBrowse(CUSTINFO *pci)
{
    BROWSEINFO bi;
    TCHAR szName[ MAX_PATH ];
    TCHAR szTitle[ 128 ];

    LoadString( g_hInstance, IDS_BROWSE_TITLE, szTitle, ARRAYSIZE(szTitle) );

    bi.hwndOwner = pci->hDlg;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szName;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_USENEWUI;
    bi.lParam = 0;
    bi.iImage = 0;
    bi.lpfn = _BrowseCallbackProc;

    // the default root for this folder is MyDocs so we don't need to set that up.

    LPITEMIDLIST pidl = SHBrowseForFolder( &bi );
    if (pidl)
    {
        if (SHGetPathFromIDList( pidl, szName ))
        {
            SetDlgItemText( pci->hDlg, IDD_TARGET, szName );
            _MakeDirty(pci);
        }
        ILFree( pidl );
    }
}

void DoReset(CUSTINFO *pci)
{
    TCHAR szPath[MAX_PATH];

    if (S_OK == SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_DEFAULT, szPath))
    {
        SetDlgItemText(pci->hDlg, IDD_TARGET, szPath);
        _MakeDirty(pci);
    }
}

INT_PTR CALLBACK TargetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUSTINFO *pci = (CUSTINFO *)GetWindowLongPtr( hDlg, DWLP_USER );
    switch (uMsg)
    {
    case WM_INITDIALOG:
        if (!InitTargetPage( hDlg, lParam ))
        {
            ShellMessageBox( g_hInstance, hDlg, (LPTSTR)IDS_INIT_FAILED_TEXT, GetMessageTitle(),
                             MB_OK | MB_ICONSTOP | MB_TOPMOST );
            return 0;
        }
        return 1;

    case WM_DESTROY:
        if (NULL != pci)
        {
            ATOMICRELEASE(pci->ppf);
        }
        LocalFree( pci );
        SetWindowLongPtr( hDlg, DWLP_USER, 0 );
        return 1;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_RESET:
            DoReset(pci);
            return 1;

        case IDD_TARGET:
	    if ((GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) && pci && (pci->bInitDone) && (!pci->bDirty))
            {
                _MakeDirty(pci);
            }
            return 1;

        case IDD_FIND:
            _DoFind(pci);
            return 1;

        case IDD_BROWSE:
            _DoBrowse(pci);
            return 1;
        }
        break;

    case WM_HELP:               /* F1 or title-bar help button */
        if ((((LPHELPINFO)lParam)->iCtrlId != IDD_ITEMICON )     &&
            (((LPHELPINFO)lParam)->iCtrlId != IDD_INSTRUCTIONS ) &&
            (((LPHELPINFO)lParam)->iCtrlId != IDC_TARGET_GBOX ))
        {
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                     NULL, HELP_WM_HELP, (DWORD_PTR) rgdwHelpTarget);
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
                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgdwHelpTarget);
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
            case PSN_APPLY:
                _DoApply(pci);
                return 1;
        }
        break;
    }
    return 0;
}
