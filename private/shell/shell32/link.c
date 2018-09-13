//---------------------------------------------------------------------------
//
// link.c       linke property page implementation
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma hdrstop

#include "fstreex.h"
#include "docfind.h"
#include "lnkcon.h"
#include "trayp.h"      // for WMTRAY_ messages
#include "util.h"	// for GetIconLocationFromExt

#define LNKM_ACTIVATEOTHER      (WM_USER + 100)         // don't conflict with DM_ messages

//
// This string defined in shlink.c - hack to allow user to set working dir to $$
// and have it map to whatever "My Documents" is mapped to.
//

void _UpdateLinkIcon(LPLINKPROP_DATA plpd, HICON hIcon)
{
    if (!hIcon)
        hIcon = SHGetFileIcon(NULL, plpd->szFile, 0, SHGFI_LARGEICON);

    if (hIcon)
    {
        HICON hOldIcon = (HICON)SendDlgItemMessage(plpd->hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)hIcon, 0L);
        if (hOldIcon)
            DestroyIcon(hOldIcon);
    }
}

// make sure LFN paths are nicly quoted and have args at the end

void PathComposeWithArgs(LPTSTR pszPath, LPTSTR pszArgs)
{
    PathQuoteSpaces(pszPath);

    if (pszArgs[0]) 
    {
        int len = lstrlen(pszPath);

        if (len < (MAX_PATH - 3)) 
        {     // 1 for null, 1 for space, 1 for arg
            pszPath[len++] = TEXT(' ');
            lstrcpyn(pszPath + len, pszArgs, MAX_PATH - len);
        }
    }
}

// do the inverse of the above, parse pszPath into a unquoted
// path string and put the args in pszArgs
//
// returns:
//      TRUE    we verified the thing exists
//      FALSE   it may not exist

BOOL PathSeperateArgs(LPTSTR pszPath, LPTSTR pszArgs)
{
    LPTSTR pszT;

    PathRemoveBlanks(pszPath);

    // if the unquoted sting exists as a file just use it

    if (PathFileExistsAndAttributes(pszPath, NULL))
    {
        *pszArgs = 0;
        return TRUE;
    }

    pszT = PathGetArgs(pszPath);
    if (*pszT)
        *(pszT - 1) = 0;
    lstrcpy(pszArgs, pszT);

    PathUnquoteSpaces(pszPath);

    return FALSE;
}

// put a path into an edit field, doing quoting as necessary

void SetDlgItemPath(HWND hdlg, int id, LPTSTR pszPath)
{
    PathQuoteSpaces(pszPath);
    SetDlgItemText(hdlg, id, pszPath);
}

// get a path from an edit field, unquoting as possible

void GetDlgItemPath(HWND hdlg, int id, LPTSTR pszPath)
{
    GetDlgItemText(hdlg, id, pszPath, MAX_PATH);
    PathRemoveBlanks(pszPath);
    PathUnquoteSpaces(pszPath);
}


const int c_iShowCmds[] = {
    SW_SHOWNORMAL,
    SW_SHOWMINNOACTIVE,
    SW_SHOWMAXIMIZED,
};

void _DisableAllChildren(HWND hwnd)
{
    HWND hwndChild;

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        // we don't want to disable the static text controls (makes the dlg look bad)
        if (!(SendMessage(hwndChild, WM_GETDLGCODE, 0, 0) & DLGC_STATIC))
        {
            EnableWindow(hwndChild, FALSE);
        }
    }
}

void _GetPathAndArgs(LPLINKPROP_DATA plpd, LPTSTR pszPath, LPTSTR pszArgs)
{
    GetDlgItemText(plpd->hDlg, IDD_FILENAME, pszPath, MAX_PATH);
    PathSeperateArgs(pszPath, pszArgs);
}


#ifdef WINNT
//
// Returns fully qualified path to target of link, and # of characters
// in fully qualifed path as return value
//
INT _GetTargetOfLink(LPLINKPROP_DATA plpd, LPTSTR pszTarget )
{
    TCHAR szFile[MAX_PATH], szArgs[MAX_PATH];
    INT cch = 0;

    *pszTarget = 0;

    _GetPathAndArgs(plpd, szFile, szArgs);

    if (szFile[0])
    {
        LPTSTR psz;
        TCHAR szExp[MAX_PATH];

        if (SHExpandEnvironmentStrings(szFile, szExp, ARRAYSIZE(szExp)))
        {
            cch = SearchPath(NULL, szExp, TEXT(".EXE"), MAX_PATH, pszTarget, &psz);
        }
    }

    return cch;
}


//
// Do checking of the .exe type in the background so the UI doesn't
// get hung up while we scan.  This is particularly important with
// the .exe is over the network or on a floppy.
//
STDAPI_(DWORD) _LinkCheckThreadProc(void *pv)
{
    LINKPROP_DATA *plpd = (LINKPROP_DATA *)pv;
    BOOL fCheck = TRUE, fEnable = FALSE;

    DebugMsg(DM_TRACE, TEXT("_LinkCheckThreadProc created and running"));

    while (plpd->bCheckRunInSep)
    {
        WaitForSingleObject( plpd->hCheckNow, INFINITE);
        ResetEvent(plpd->hCheckNow);

        if (plpd->bCheckRunInSep)
        {
            TCHAR szFullFile[MAX_PATH];
            DWORD cch = _GetTargetOfLink(plpd, szFullFile);

            if ((cch != 0) && (cch < ARRAYSIZE(szFullFile)))
            {
                LONG lBinaryType;

                if (PathIsUNC( szFullFile ) || IsRemoteDrive(DRIVEID(szFullFile)))
                {
                    // Net Path, let the user decide...
                    fCheck = FALSE;
                    fEnable = TRUE;
                }
                else if (GetBinaryType( szFullFile, &lBinaryType) && (lBinaryType==SCS_WOW_BINARY))
                {
                    // 16-bit binary, let the user decide, default to same VDM
                    fCheck = FALSE;
                    fEnable = TRUE;
                }
                else
                {
                    // 32-bit binary, or non-net path.  don't enable the control
                    fCheck = TRUE;
                    fEnable = FALSE;
                }
            } 
            else 
            {
                // Error getting target of the link.  don't enable the control
                fCheck = TRUE;
                fEnable = FALSE;
            }

            CheckDlgButton(plpd->hDlg, IDD_RUNINSEPARATE, fCheck ? 1 : 0);
            EnableWindow(GetDlgItem(plpd->hDlg, IDD_RUNINSEPARATE), fEnable);
        }
    }

    CloseHandle(plpd->hCheckNow);

    DebugMsg(DM_TRACE, TEXT("_LinkCheckThreadProc exiting now..."));
    return 0;
}

// shut down the thread

void _StopThread(LINKPROP_DATA *plpd)
{
    if (plpd->hThread)
    {
        plpd->bCheckRunInSep = FALSE;
        SetEvent(plpd->hCheckNow);

        if (WaitForSingleObject(plpd->hThread, 2000) == WAIT_TIMEOUT)
            TerminateThread(plpd->hThread, (DWORD)-1);   // Blow it away!

        CloseHandle(plpd->hThread);
        plpd->hThread = NULL;
    }
}
#endif // WINNT


LPVOID _GetLinkExtraData(IShellLink* psl, DWORD dwSig)
{
    LPVOID pDataBlock = NULL;
    IShellLinkDataList *psld;

    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->CopyDataBlock(psld, dwSig, &pDataBlock);
        psld->lpVtbl->Release(psld);
    }

    return pDataBlock;
}

DWORD _GetLinkFlags(IShellLink *psl)
{
    DWORD dw = 0;
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->GetFlags(psld, &dw);
        psld->lpVtbl->Release(psld);
    }
    return dw;
}

void _SetLinkFlags(IShellLink *psl, DWORD dwFlags)
{
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->SetFlags(psld, dwFlags);
        psld->lpVtbl->Release(psld);
    }
}

// Initializes the generic link dialog box.
void _UpdateLinkDlg(LPLINKPROP_DATA plpd, BOOL bUpdatePath)
{
    WORD wHotkey;
    int  i, iShowCmd;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szCommand[MAX_PATH];
    HRESULT hres;
    SHFILEINFO sfi;
    BOOL fIsDarwinLink = _GetLinkFlags(plpd->psl) & SLDF_HAS_DARWINID;

    // do this here so we don't slow down the loading
    // of other pages

    if (!bUpdatePath)
    {
        IPersistFile *ppf;

        if (SUCCEEDED(plpd->psl->lpVtbl->QueryInterface(plpd->psl, &IID_IPersistFile, &ppf)))
        {
            WCHAR wszPath[MAX_PATH];

            SHTCharToUnicode(plpd->szFile, wszPath, ARRAYSIZE(wszPath));
            hres = ppf->lpVtbl->Load(ppf, wszPath, 0);
            ppf->lpVtbl->Release(ppf);

            if (FAILED(hres))
            {
                LoadString(HINST_THISDLL, IDS_LINKNOTLINK, szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText(plpd->hDlg, IDD_FILETYPE, szBuffer);
                _DisableAllChildren(plpd->hDlg);

                DebugMsg(DM_TRACE, TEXT("Shortcut IPersistFile::Load() failed %x"), hres);
                return;
            }
        }
    }

    SHGetFileInfo(plpd->szFile, 0, &sfi, SIZEOF(sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES);
    SetDlgItemText(plpd->hDlg, IDD_NAME, sfi.szDisplayName);

    // we need to check for darwin links here so that we can gray out
    // things that don't apply to darwin
    if (fIsDarwinLink)
    {
        LPEXP_DARWIN_LINK pDarwinData;
        TCHAR szAppState[MAX_PATH];
        DWORD cchAppState = ARRAYSIZE(szAppState);
        HWND hwndTargetType = GetDlgItem(plpd->hDlg, IDD_FILETYPE);

        // disable the children
        _DisableAllChildren(plpd->hDlg);

        // then special case the icon and the "Target type:" text
        _UpdateLinkIcon(plpd, NULL);

        pDarwinData = _GetLinkExtraData(plpd->psl, EXP_DARWIN_ID_SIG);

        if (pDarwinData && IsDarwinAdW(pDarwinData->szwDarwinID))
        {
            // the app is advertised (e.g. not installed), but will be faulted in on first use
            LoadString(HINST_THISDLL, IDS_APP_NOT_FAULTED_IN, szAppState, ARRAYSIZE(szAppState));
        }
        else
        {
            // the darwin app is installed
            LoadString(HINST_THISDLL, IDS_APP_FAULTED_IN, szAppState, ARRAYSIZE(szAppState));
        }

        SetWindowText(hwndTargetType, szAppState);
        EnableWindow(hwndTargetType, TRUE);

        // if we can ge the package name, put that in the Target field
        if (pDarwinData &&
#ifdef UNICODE
            MsiGetProductInfo(pDarwinData->szwDarwinID,
                              INSTALLPROPERTY_PRODUCTNAME,
                              szAppState,
                              &cchAppState) == ERROR_SUCCESS)
#else
            MsiGetProductInfo(pDarwinData->szDarwinID,
                              INSTALLPROPERTY_PRODUCTNAME,
                              szAppState,
                              &cchAppState) == ERROR_SUCCESS)
#endif
        {
            SetWindowText(GetDlgItem(plpd->hDlg, IDD_FILENAME), szAppState);
        }

        if (pDarwinData)
            LocalFree(pDarwinData);


        
        // we disabled everything in _DisableAllChildren, so re-enable the ones we still apply for darwin
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_NAME), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_PATH), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_HOTKEY), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_SHOWCMD), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_DESCRIPTION), TRUE);

        // we skip all of the gook below if we are darwin since we only support the IDD_NAME, IDD_PATH, IDD_LINK_HOTKEY, 
        // IDD_LINK_SHOWCMD, and IDD_LINK_DESCRIPTION fields
    }
    else
    {
        hres = plpd->psl->lpVtbl->GetPath(plpd->psl, szCommand, ARRAYSIZE(szCommand), NULL, SLGP_RAWPATH);
        
        if (FAILED(hres))
            hres = plpd->psl->lpVtbl->GetPath(plpd->psl, szCommand, ARRAYSIZE(szCommand), NULL, 0);


        if (SUCCEEDED(hres) && (hres != S_FALSE))
        {
            plpd->bIsFile = TRUE;

            // get type
            if (!SHGetFileInfo(szCommand, 0, &sfi, SIZEOF(sfi), SHGFI_TYPENAME))
            {
                TCHAR szExp[MAX_PATH];

                // Let's see if the string has expandable environment strings
                if (SHExpandEnvironmentStrings(szCommand, szExp, ARRAYSIZE(szExp))
                && lstrcmp(szCommand, szExp)) // don't hit the disk a second time if the string hasn't changed
                {
                    SHGetFileInfo(szExp, 0, &sfi, SIZEOF(sfi), SHGFI_TYPENAME );
                }
            }
            SetDlgItemText(plpd->hDlg, IDD_FILETYPE, sfi.szTypeName);

            // location
            lstrcpy(szBuffer, szCommand);
            PathRemoveFileSpec(szBuffer);
            SetDlgItemText(plpd->hDlg, IDD_LOCATION, PathFindFileName(szBuffer));

            // command
            plpd->psl->lpVtbl->GetArguments(plpd->psl, szBuffer, ARRAYSIZE(szBuffer));
            PathComposeWithArgs(szCommand, szBuffer);
            GetDlgItemText(plpd->hDlg, IDD_FILENAME, szBuffer, ARRAYSIZE(szBuffer));
            // Conditionally change to prevent "Apply" button from enabling
            if (lstrcmp(szCommand, szBuffer) != 0)
                SetDlgItemText(plpd->hDlg, IDD_FILENAME, szCommand);
        }
        else
        {
            LPITEMIDLIST pidl;

            plpd->bIsFile = FALSE;

            EnableWindow(GetDlgItem(plpd->hDlg, IDD_FILENAME), FALSE);
            EnableWindow(GetDlgItem(plpd->hDlg, IDD_PATH), FALSE);

            plpd->psl->lpVtbl->GetIDList(plpd->psl, &pidl);

            if (pidl)
            {
                SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szCommand, SIZECHARS(szCommand), NULL);
                ILRemoveLastID(pidl);
                SHGetNameAndFlags(pidl, SHGDN_NORMAL, szBuffer, SIZECHARS(szBuffer), NULL);
                ILFree(pidl);

                SetDlgItemText(plpd->hDlg, IDD_LOCATION, szBuffer);
                SetDlgItemText(plpd->hDlg, IDD_FILETYPE, szCommand);
                SetDlgItemText(plpd->hDlg, IDD_FILENAME, szCommand);
            }
        }

#ifdef WINNT
        {
            TCHAR szFullFile[MAX_PATH];
            DWORD cchVerb;
            UINT cch = _GetTargetOfLink(plpd, szFullFile);


            if ((cch != 0) && (cch < ARRAYSIZE(szFullFile)))
            {
                LONG lBinaryType;
                if (GetBinaryType( szFullFile, &lBinaryType) && (lBinaryType == SCS_WOW_BINARY))
                {
                    if (_GetLinkFlags(plpd->psl) & SLDF_RUN_IN_SEPARATE)
                    {
                        // check it
                        EnableWindow(GetDlgItem(plpd->hDlg, IDD_RUNINSEPARATE), TRUE);
                        CheckDlgButton( plpd->hDlg, IDD_RUNINSEPARATE, 1 );
                    } 
                    else 
                    {
                        // Uncheck it
                        EnableWindow(GetDlgItem(plpd->hDlg, IDD_RUNINSEPARATE), TRUE);
                        CheckDlgButton( plpd->hDlg, IDD_RUNINSEPARATE, 0 );
                    }
                } 
                else 
                {
                    // check it
                    CheckDlgButton( plpd->hDlg, IDD_RUNINSEPARATE, 1 );
                    EnableWindow(GetDlgItem( plpd->hDlg, IDD_RUNINSEPARATE ), FALSE);
                }
            } 
            else 
            {
                // check it
                CheckDlgButton( plpd->hDlg, IDD_RUNINSEPARATE, 1 );
                EnableWindow( GetDlgItem( plpd->hDlg, IDD_RUNINSEPARATE ), FALSE );
            }


            // enable "runas" if the link target has that verb 
            if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_COMMAND, szFullFile, TEXT("runas"), NULL, &cchVerb)) &&
                cchVerb)
            {
                EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_RUNASUSER), TRUE);
                CheckDlgButton(plpd->hDlg, IDD_LINK_RUNASUSER, (_GetLinkFlags(plpd->psl) & SLDF_RUNAS_USER) ? BST_CHECKED : BST_UNCHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_RUNASUSER), FALSE);
                CheckDlgButton(plpd->hDlg, IDD_LINK_RUNASUSER, BST_UNCHECKED);
                _SetLinkFlags(plpd->psl, _GetLinkFlags(plpd->psl) & ~SLDF_RUNAS_USER);
            }
        }
#endif
    }

    if (bUpdatePath)
        return;

    plpd->psl->lpVtbl->GetWorkingDirectory(plpd->psl, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemPath(plpd->hDlg, IDD_PATH, szBuffer);

    plpd->psl->lpVtbl->GetDescription(plpd->psl, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(plpd->hDlg, IDD_LINK_DESCRIPTION, szBuffer);

    plpd->psl->lpVtbl->GetHotkey(plpd->psl, &wHotkey);
    SendDlgItemMessage(plpd->hDlg, IDD_LINK_HOTKEY, HKM_SETHOTKEY, wHotkey, 0);

    //
    // Now initialize the Run SHOW Command combo box
    //
    for (iShowCmd = IDS_RUN_NORMAL; iShowCmd <= IDS_RUN_MAXIMIZED; iShowCmd++)
    {
        LoadString(HINST_THISDLL, iShowCmd, szBuffer, ARRAYSIZE(szBuffer));
        SendDlgItemMessage(plpd->hDlg, IDD_LINK_SHOWCMD, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuffer);
    }

    // Now setup the Show Command - Need to map to index numbers...
    plpd->psl->lpVtbl->GetShowCmd(plpd->psl, &iShowCmd);

    for (i = 0; i < ARRAYSIZE(c_iShowCmds); i++)
    {
        if (c_iShowCmds[i] == iShowCmd)
            break;
    }
    if (i == ARRAYSIZE(c_iShowCmds))
    {
        ASSERT(0);      // bogus link show cmd
        i = 0;  // SW_SHOWNORMAL
    }

    SendDlgItemMessage(plpd->hDlg, IDD_LINK_SHOWCMD, CB_SETCURSEL, i, 0);

    // the icon
    _UpdateLinkIcon(plpd, NULL);
}

//
// Opens a folder window with the target of the link selected
//
void _FindTarget(LPLINKPROP_DATA plpd)
{
    USHORT uSave;
    LPITEMIDLIST pidl, pidlDesk, pidlLast;

    if (plpd->psl->lpVtbl->Resolve(plpd->psl, plpd->hDlg, 0) != NOERROR)
        return; // above already did UI if needed

    _UpdateLinkDlg(plpd, TRUE);

    plpd->psl->lpVtbl->GetIDList(plpd->psl, &pidl);
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

    pidlDesk = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, FALSE);
    if (pidlDesk)
    {
        BOOL fIsDesktopDir = ILIsEqual(pidl, pidlDesk);

        if (fIsDesktopDir || !uSave)  // if it's in the desktop dir or pidl == pidlLast (uSave == 0 from above)
        {
            ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_ORIGINALONDESKTOP), NULL, MB_OK);
            
            // get keyboard focus to the desktop (away from this prop sheet)
            PostMessage(plpd->hDlg, LNKM_ACTIVATEOTHER, 0, (LPARAM)FindWindow(TEXT(STR_DESKTOPCLASS), NULL));
        }
        else
        {
            IShellFolderViewDual *psfv;
            if (SUCCEEDED(OpenContainingFolderAndGetShellFolderView(uSave ? pidl : pidlDesk, &psfv)))
            {
                if (uSave)
                    pidlLast->mkid.cb = uSave;
                SelectPidlInSFV(psfv, pidlLast, SVSI_SELECT | SVSI_FOCUSED | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE);
                psfv->lpVtbl->Release(psfv);
            }
        }
        ILFree(pidlDesk);
    }
    ILFree(pidl);
}

// let the user pick a new icon for a link...

BOOL _DoPickIcon(LPLINKPROP_DATA plpd)
{
    int iIconIndex;
    SHFILEINFO sfi;
    TCHAR *pszIconPath = sfi.szDisplayName;
    IShellLinkDataList *psldl; 
    EXP_SZ_LINK *esli;
    HRESULT hr;

    *pszIconPath = 0;

    //
    // if the user has picked a icon before use it.
    //
    if (plpd->szIconPath[0] != 0 && plpd->iIconIndex >= 0)
    {
        lstrcpy(pszIconPath, plpd->szIconPath);
        iIconIndex = plpd->iIconIndex;
    }
    else
    {
        //
        // if this link has a icon use that.
        //
        plpd->psl->lpVtbl->GetIconLocation(plpd->psl, pszIconPath, MAX_PATH, &iIconIndex);

        //
        // check for an escaped version, if its there, use that 
        // 
        if (SUCCEEDED(hr = plpd->psl->lpVtbl->QueryInterface(plpd->psl, &IID_IShellLinkDataList, (LPVOID*)&psldl))) 
        { 
            if (SUCCEEDED(hr = psldl->lpVtbl->CopyDataBlock(psldl, EXP_SZ_ICON_SIG, (LPVOID*)&esli))) 
            { 
                ASSERT(esli);
#ifdef UNICODE 
                lstrcpyn(pszIconPath, esli->swzTarget, MAX_PATH); 
#else 
                lstrcpyn(pszIconPath, esli->szTarget, MAX_PATH); 
#endif 
                LocalFree(esli);
            } 

            psldl->lpVtbl->Release(psldl); 
        } 


	if (pszIconPath[0] == TEXT('.'))
	{
	    TCHAR szFullIconPath[MAX_PATH];

	    // We now allow ".txt" for the icon path, but since the user is clicking
	    // on the "Change Icon..." button, we show the current icon that ".txt" is
	    // associated with
	    GetIconLocationFromExt(pszIconPath, szFullIconPath, ARRAYSIZE(szFullIconPath), &iIconIndex);
	    lstrcpyn(pszIconPath, szFullIconPath, ARRAYSIZE(sfi.szDisplayName));
	}
	else if (pszIconPath[0] == TEXT('\0'))
        {
            //
            // link does not have a icon, if it is a link to a file
            // use the file name
            //
            TCHAR szArgs[MAX_PATH];
            _GetPathAndArgs(plpd, pszIconPath, szArgs);

            iIconIndex = 0;

            if (!plpd->bIsFile || !PathIsExe(pszIconPath))
            {
                //
                // link is not to a file, go get the icon
                //
                SHGetFileInfo(plpd->szFile, 0, &sfi, SIZEOF(sfi), SHGFI_ICONLOCATION);
                iIconIndex = sfi.iIcon;
                ASSERT(pszIconPath == sfi.szDisplayName);
            }
        }
    }

    if (PickIconDlg(plpd->hDlg, pszIconPath, MAX_PATH, &iIconIndex))
    {
        HICON hIcon = ExtractIcon(HINST_THISDLL, pszIconPath, iIconIndex);
        _UpdateLinkIcon(plpd, hIcon);

        // don't save it out to the link yet, just store it in our instance data
        plpd->iIconIndex = iIconIndex;
        lstrcpy(plpd->szIconPath, pszIconPath);

        PropSheet_Changed(GetParent(plpd->hDlg), plpd->hDlg);
        return TRUE;
    }

    return FALSE;
}

HRESULT _SaveLink(LPLINKDATA pld)
{
    WORD wHotkey;
    int iShowCmd;
    IPersistFile *ppf;
    HRESULT hres;
    TCHAR szBuffer[MAX_PATH];

#ifdef WINNT
    if (!(pld->lpd.bIsDirty || (pld->cpd.lpConsole && pld->cpd.bConDirty)))
#else
    if (!pld->lpd.bIsDirty)
#endif
        return S_OK;

    if (pld->lpd.bIsFile)
    {
        TCHAR szBuffer[MAX_PATH];
        TCHAR szArgs[MAX_PATH];

        _GetPathAndArgs(&pld->lpd, szBuffer, szArgs);

        // set the path (and pidl) of the link
        pld->lpd.psl->lpVtbl->SetPath(pld->lpd.psl, szBuffer);

        // may be null
        pld->lpd.psl->lpVtbl->SetArguments(pld->lpd.psl, szArgs);

#ifdef WINNT
        {
            DWORD dwOldFlags = _GetLinkFlags(pld->lpd.psl);

            // Set whether to run in separate memory space
            dwOldFlags &= ~SLDF_RUN_IN_SEPARATE;

            _SetLinkFlags(pld->lpd.psl, dwOldFlags);

            if (IsWindowEnabled( GetDlgItem( pld->lpd.hDlg, IDD_RUNINSEPARATE )))
            {
                if (IsDlgButtonChecked( pld->lpd.hDlg, IDD_RUNINSEPARATE ))
                    _SetLinkFlags(pld->lpd.psl, dwOldFlags | SLDF_RUN_IN_SEPARATE);
            }

            dwOldFlags = _GetLinkFlags(pld->lpd.psl);
            if (IsWindowEnabled(GetDlgItem(pld->lpd.hDlg, IDD_LINK_RUNASUSER)) &&
                IsDlgButtonChecked(pld->lpd.hDlg, IDD_LINK_RUNASUSER))
            {
                _SetLinkFlags(pld->lpd.psl, dwOldFlags | SLDF_RUNAS_USER);
            }
            else
            {
                _SetLinkFlags(pld->lpd.psl, dwOldFlags & ~SLDF_RUNAS_USER);
            }
        }
#endif
    }

    if (pld->lpd.bIsFile || (_GetLinkFlags(pld->lpd.psl) & SLDF_HAS_DARWINID))
    {
        // set the working directory of the link
        GetDlgItemPath(pld->lpd.hDlg, IDD_PATH, szBuffer);
        pld->lpd.psl->lpVtbl->SetWorkingDirectory(pld->lpd.psl, szBuffer);
    }

    // set the description of the link
    GetDlgItemText(pld->lpd.hDlg, IDD_LINK_DESCRIPTION, szBuffer, MAX_PATH);
    pld->lpd.psl->lpVtbl->SetDescription(pld->lpd.psl, szBuffer);

    // the hotkey
    wHotkey = (WORD)SendDlgItemMessage(pld->lpd.hDlg, IDD_LINK_HOTKEY , HKM_GETHOTKEY, 0, 0);
    pld->lpd.psl->lpVtbl->SetHotkey(pld->lpd.psl, wHotkey);

    // the show command combo box
    iShowCmd = (int)SendDlgItemMessage(pld->lpd.hDlg, IDD_LINK_SHOWCMD, CB_GETCURSEL, 0, 0L);
    if ((iShowCmd >= 0) && (iShowCmd < ARRAYSIZE(c_iShowCmds)))
        pld->lpd.psl->lpVtbl->SetShowCmd(pld->lpd.psl, c_iShowCmds[iShowCmd]);

    // If the user explicitly selected a new icon, invalidate
    // the icon cache entry for this link and then send around a file
    // sys refresh message to all windows in case they are looking at
    // this link.
    if (pld->lpd.iIconIndex >= 0)
        pld->lpd.psl->lpVtbl->SetIconLocation(pld->lpd.psl, pld->lpd.szIconPath, pld->lpd.iIconIndex);

#ifdef WINNT
    // Update/Save the console information in the pExtraData section of
    // the shell link.

    if (pld->cpd.lpConsole && pld->cpd.bConDirty)
        LinkConsolePagesSave( pld );
#endif

    hres = pld->lpd.psl->lpVtbl->QueryInterface(pld->lpd.psl, &IID_IPersistFile, &ppf);
    if (SUCCEEDED(hres))
    {
        if (ppf->lpVtbl->IsDirty(ppf) == NOERROR)
        {
            // save using existing file name (pld->lpd.szFile)
            hres = ppf->lpVtbl->Save(ppf, NULL, TRUE);

            if (FAILED(hres))
            {
                SHSysErrorMessageBox(pld->lpd.hDlg, NULL, IDS_LINKCANTSAVE,
                    hres & 0xFFF, PathFindFileName(pld->lpd.szFile),
                    MB_OK | MB_ICONEXCLAMATION);
            }
            else
            {
                pld->lpd.bIsDirty = FALSE;
            }
        }
        ppf->lpVtbl->Release(ppf);
    }

    return hres;
}

void SetEditFocus(HWND hwnd)
{
    SetFocus(hwnd);
    Edit_SetSel(hwnd, 0, -1);
}

// returns:
//      TRUE    all link fields are valid
//      FALSE   some thing is wrong with what the user has entered

BOOL _ValidateLink(LPLINKPROP_DATA plpd)
{
    TCHAR szDir[MAX_PATH], szPath[MAX_PATH], szArgs[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    LPTSTR dirs[2];
    BOOL  bValidPath = FALSE;

    if (!plpd->bIsFile)
        return TRUE;

    // validate the working directory field

    GetDlgItemPath(plpd->hDlg, IDD_PATH, szDir);

    if (*szDir &&
        StrChr(szDir, TEXT('%')) == NULL &&       // has environement var %USER%
        !IsRemovableDrive(DRIVEID(szDir)) &&
        !PathIsDirectory(szDir))
    {
        ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKBADWORKDIR),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION, szDir);

        SetEditFocus(GetDlgItem(plpd->hDlg, IDD_PATH));

        return FALSE;
    }

    // validate the path (with arguments) field

    _GetPathAndArgs(plpd, szPath, szArgs);


    if (szPath[0] == 0)
        return TRUE;

    if (PathIsRoot(szPath) && IsRemovableDrive(DRIVEID(szPath)))
        return TRUE;

    if (PathIsLnk(szPath))
    {
        ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKTOLINK),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION);
        SetEditFocus(GetDlgItem(plpd->hDlg, IDD_FILENAME));
        return FALSE;
    }

    dirs[0] = szDir;
    dirs[1] = NULL;
    bValidPath = PathResolve(szPath, dirs, PRF_DONTFINDLNK | PRF_TRYPROGRAMEXTENSIONS);
    if (!bValidPath)
    {
        // The path "as is" was invalid.  See if it has environment variables
        // which need to be expanded.

        _GetPathAndArgs(plpd, szPath, szArgs);

        if (SHExpandEnvironmentStrings(szPath, szExpPath, ARRAYSIZE(szExpPath)))
        {
            if (PathIsRoot(szExpPath) && IsRemovableDrive(DRIVEID(szDir)))
                return TRUE;

            bValidPath = PathResolve(szExpPath, dirs, PRF_DONTFINDLNK | PRF_TRYPROGRAMEXTENSIONS);
        }
    }

    if (bValidPath)
    {
#ifdef WINNT
        BOOL bSave;

        if (plpd->hThread)
        {
            bSave = plpd->bCheckRunInSep;
            plpd->bCheckRunInSep = FALSE;
        }
#endif
        PathComposeWithArgs(szPath, szArgs);
        GetDlgItemText(plpd->hDlg, IDD_FILENAME, szExpPath, ARRAYSIZE(szExpPath));
        // only do this if something changed... that way we avoid having the PSM_CHANGED
        // for nothing
        if (lstrcmpi(szPath, szExpPath))
            SetDlgItemText(plpd->hDlg, IDD_FILENAME, szPath);
#ifdef WINNT
        if (plpd->hThread)
        {
            plpd->bCheckRunInSep = bSave;
        }
#endif
        return TRUE;
    }

    ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKBADPATH),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION, szPath);
    SetEditFocus(GetDlgItem(plpd->hDlg, IDD_FILENAME));
    return FALSE;
}

// Array for context help:
const DWORD aLinkHelpIDs[] = {
    IDD_LINE_1,             NO_HELP,
    IDD_LINE_2,             NO_HELP,
    IDD_ITEMICON,           IDH_FCAB_LINK_ICON,
    IDD_NAME,               IDH_FCAB_LINK_NAME,
    IDD_FILETYPE_TXT,       IDH_FCAB_LINK_LINKTYPE,
    IDD_FILETYPE,           IDH_FCAB_LINK_LINKTYPE,
    IDD_LOCATION_TXT,       IDH_FCAB_LINK_LOCATION,
    IDD_LOCATION,           IDH_FCAB_LINK_LOCATION,
    IDD_FILENAME,           IDH_FCAB_LINK_LINKTO,
    IDD_PATH,               IDH_FCAB_LINK_WORKING,
    IDD_LINK_HOTKEY,        IDH_FCAB_LINK_HOTKEY,
    IDD_LINK_SHOWCMD,       IDH_FCAB_LINK_RUN,
    IDD_LINK_DESCRIPTION,   IDH_FCAB_LINK_DESCRIPTION,
    IDD_FINDORIGINAL,       IDH_FCAB_LINK_FIND,
    IDD_LINKDETAILS,        IDH_FCAB_LINK_CHANGEICON,
#ifdef WINNT
    IDD_RUNINSEPARATE,      IDH_TRAY_RUN_SEPMEM,
    IDD_LINK_RUNASUSER,     IDH_FCAB_LINK_RUNASUSER,
#endif
    0, 0
};

// Dialog proc for the generic link property sheet
//
// uses DLG_LINKPROP template
//

BOOL_PTR CALLBACK _LinkDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hdlg, DWLP_USER);

    switch (msg) {
    case WM_INITDIALOG:

        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)pld);

        // setup dialog state variables

        pld->lpd.hDlg = hdlg;

        SendDlgItemMessage(hdlg, IDD_FILENAME, EM_LIMITTEXT, MAX_PATH-1, 0);
        SetPathWordBreakProc(GetDlgItem(hdlg, IDD_FILENAME), TRUE);
        SendDlgItemMessage(hdlg, IDD_PATH, EM_LIMITTEXT, MAX_PATH-1, 0);
        SetPathWordBreakProc(GetDlgItem(hdlg, IDD_PATH), TRUE);
        SendDlgItemMessage(hdlg, IDD_LINK_DESCRIPTION, EM_LIMITTEXT, MAX_PATH-1, 0);

        // set valid combinations for the hotkey
        SendDlgItemMessage(hdlg, IDD_LINK_HOTKEY, HKM_SETRULES,
                            HKCOMB_NONE | HKCOMB_A | HKCOMB_S | HKCOMB_C,
                            HOTKEYF_CONTROL | HOTKEYF_ALT);

        SHAutoComplete(GetDlgItem(hdlg, IDD_FILENAME), 0);
        SHAutoComplete(GetDlgItem(hdlg, IDD_PATH), 0);

#ifdef WINNT
        ASSERT(pld->lpd.hThread == NULL);
#endif
        _UpdateLinkDlg(&pld->lpd, FALSE);

        // Set up background thread to handle "Run In Separate Memory Space"
        // check box.

#ifdef WINNT
        pld->lpd.bCheckRunInSep = TRUE;
        pld->lpd.hCheckNow = CreateEvent( NULL, TRUE, FALSE, NULL );
        if (pld->lpd.hCheckNow)
        {
            DWORD dwDummy;

            pld->lpd.hThread = CreateThread(NULL, 0, _LinkCheckThreadProc, &pld->lpd, 0, &dwDummy);
            if (pld->lpd.hThread == NULL)
            {
                CloseHandle(pld->lpd.hCheckNow);
                pld->lpd.hCheckNow = NULL;
            }
        }
#endif

        // start off clean.
        // do this here because we call some stuff above which generates
        // wm_command/en_changes which we then think makes it dirty
        pld->lpd.bIsDirty = FALSE;

        break;

#ifdef WINNT
    case WM_DESTROY:
        _StopThread(&pld->lpd);
        break;
#endif

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {

#ifdef WINNT
        case PSN_RESET:
                _StopThread(&pld->lpd);
            break;
#endif
        case PSN_APPLY:

#ifdef WINNT
            if ((((PSHNOTIFY *)lParam)->lParam))
                _StopThread(&pld->lpd);
#endif

            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

        case PSN_KILLACTIVE:
            // we implement the save on page change model, so
            // validate and save changes here.  this works for
            // Apply Now, OK, and Page chagne.

            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, !_ValidateLink(&pld->lpd));   // don't allow close
            break;
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDD_FINDORIGINAL:
            _FindTarget(&pld->lpd);
            break;

        case IDD_LINKDETAILS:
            if (_DoPickIcon(&pld->lpd))
                pld->lpd.bIsDirty = TRUE;
            break;

        case IDD_LINK_SHOWCMD:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->lpd.bIsDirty = TRUE;
            }
            break;

#ifdef WINNT
        case IDD_RUNINSEPARATE:
            if (IsWindowEnabled( GetDlgItem( hdlg, IDD_RUNINSEPARATE )) )
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->lpd.bIsDirty = TRUE;
            }
            break;

        case IDD_LINK_RUNASUSER:
            if (IsWindowEnabled(GetDlgItem(hdlg, IDD_LINK_RUNASUSER)))
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->lpd.bIsDirty = TRUE;
            }

            break;
#endif

        case IDD_LINK_HOTKEY:
        case IDD_FILENAME:
        case IDD_PATH:
        case IDD_LINK_DESCRIPTION:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->lpd.bIsDirty = TRUE;
#ifdef WINNT
                if (pld->lpd.hThread && pld->lpd.bCheckRunInSep)
                    SetEvent( pld->lpd.hCheckNow );
#endif
            }
            break;

        default:
            return FALSE;
        }
        break;

    case LNKM_ACTIVATEOTHER:
        SwitchToThisWindow(GetLastActivePopup((HWND)lParam), TRUE);
        SetForegroundWindow((HWND)lParam);
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aLinkHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)aLinkHelpIDs);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

//
// Release the link object allocated during the initialize
//
UINT CALLBACK _LinkPrshtCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    LPLINKDATA pld = (LPLINKDATA)((PROPSHEETPAGE *)ppsp->lParam);
    switch (uMsg) {
    case PSPCB_RELEASE:
#ifdef WINNT
        if (pld->cpd.lpConsole)
        {
            LocalFree(pld->cpd.lpConsole );
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree(pld->cpd.lpFEConsole );
        }
        DestroyFonts( &pld->cpd );
#endif
        pld->lpd.psl->lpVtbl->Release(pld->lpd.psl);
        LocalFree(pld);
        break;
    }

    return 1;
}

BOOL AddLinkPage(LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    IShellLink *psl;

    // HACK: call constuctor directly (should use CoCreateInstance())
    // NOTE: do not call CoCreateInstance, because then you are not guranteed
    //       to get our CShellLink implementation, and we pass this object
    //       on to AddLinkConsolePages which assumes it is a CShellLink object.

    if (PathIsLnk(pszFile) && SUCCEEDED(CShellLink_CreateInstance(NULL, &IID_IShellLink, &psl)))
    {
        // alloc this data, since is it shared across several pages
        // instead of putting it in as extra data in the page header
        LPLINKDATA pld = LocalAlloc(LPTR, SIZEOF(LINKDATA));
        if (pld)
        {
            HPROPSHEETPAGE hpage;
            PROPSHEETPAGE psp;

            psp.dwSize      = SIZEOF( psp );
            psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
            psp.hInstance   = HINST_THISDLL;
            psp.pszTemplate = MAKEINTRESOURCE(DLG_LINKPROP);
            psp.pfnDlgProc  = _LinkDlgProc;
            psp.pfnCallback = _LinkPrshtCallback;
            psp.lParam      = (LPARAM)pld;  // pass to all dlg procs

            lstrcpyn(pld->lpd.szFile, pszFile, ARRAYSIZE(pld->lpd.szFile));
            // pld->lpd.hThread = NULL;  // zero-init allo
            // pld->lpd.szIconPath[0] = 0;
            pld->lpd.iIconIndex = -1;
            pld->lpd.psl = psl;
            ASSERT(!pld->lpd.szIconPath[0]);

            hpage = CreatePropertySheetPage( &psp );
            if (hpage)
            {
                if (pfnAddPage(hpage, lParam))
                {
#ifdef WINNT
                    // Add console property pages if appropriate...
                    AddLinkConsolePages( pld, psl, pszFile, pfnAddPage, lParam );
#endif
                    return TRUE;    // we added the link page
                }
                else
                {
                    DestroyPropertySheetPage(hpage);
                }
            }
            LocalFree(pld);
        }
        psl->lpVtbl->Release(psl);
    }
    return FALSE;
}
