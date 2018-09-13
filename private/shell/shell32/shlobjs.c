#include "shellprv.h"
#pragma  hdrstop

#include "defext.h"

#include "recdocs.h"
#include "drives.h"

// from mtpt.cpp
STDAPI_(BOOL) CMtPt_IsAudioCD(int iDrive);

// from fsassoc.c
BOOL GetClassDescription(HKEY hkClasses, LPCTSTR pszClass, LPTSTR szDisplayName, int cbDisplayName, UINT uFlags);

//==========================================================================
// System Default Pages/Menu Extension
//==========================================================================

typedef struct
{
    CCommonUnknown              cunk;
    CCommonShellExtInit         cshx;
    CCommonShellPropSheetExt    cspx;
    CKnownContextMenu           kcxm;
    HDKA                        hdka;
} CDefExt;

STDMETHODIMP CDefExt_QueryInterface(IUnknown *punk, REFIID riid, void **ppvObj)
{
    CDefExt *this = IToClass(CDefExt, cunk.unk, punk);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *((IUnknown **)ppvObj) = &this->cunk.unk;
    } 
    else if (IsEqualIID(riid, &IID_IShellExtInit) || 
             IsEqualIID(riid, &CLSID_CCommonShellExtInit))
    {
        *((IShellExtInit **)ppvObj) = &this->cshx.kshx.unk;
    }
    else if (IsEqualIID(riid, &IID_IShellPropSheetExt))
    {
        *((IShellPropSheetExt **)ppvObj) = &this->cspx.kspx.unk;
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        *((IContextMenu **)ppvObj) = &this->kcxm.unk;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    this->cunk.cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CDefExt_AddRef(IUnknown *punk)
{
    CDefExt *this = IToClass(CDefExt, cunk.unk, punk);

    this->cunk.cRef++;
    return this->cunk.cRef;
}

STDMETHODIMP_(ULONG) CDefExt_Release(IUnknown *punk)
{
    CDefExt *this = IToClass(CDefExt, cunk.unk, punk);

    this->cunk.cRef--;
    if (this->cunk.cRef > 0)
        return this->cunk.cRef;


    CCommonShellExtInit_Delete(&this->cshx);

    if (this->hdka)
        DKA_Destroy(this->hdka);

    LocalFree((HLOCAL)this);
    return 0;
}

const IUnknownVtbl c_CDefExtVtbl =
{
    CDefExt_QueryInterface, CDefExt_AddRef, CDefExt_Release,
};

HDKA DefExt_GetDKA(CDefExt * this, BOOL fExploreFirst)
{
    if (this->hdka == NULL && this->cshx.hkeyProgID)
    {
        TCHAR szTemp[80];
        // create either "open" or "explore open"
        if (fExploreFirst)
            wsprintf(szTemp, TEXT("%s %s"), c_szExplore, c_szOpen);

        // Always get the whole DKA (not just the default) since we may
        // need different parts of it at different times.
        this->hdka = DKA_Create(this->cshx.hkeyProgID, c_szShell,
                                NULL,  fExploreFirst ? szTemp : c_szOpen, 0);
    }
    return this->hdka;
}

// Descriptions:
//   This function generates appropriate menu string from the given
//  verb key string. This function is called if the verb key does
//  not have the value.
//
// Arguments:
//  szMenuString -- specifies a string buffer to be filled with menu string.
//  pszVerbKey   -- specifies the verb key string.
//
// Requires:
//  The size of szMenuString buffer should be larger than CCH_MENUMAX
//
// History:
//  12-31-92 SatoNa     Created
//

BOOL _IGenerateMenuString(LPTSTR pszMenuString, LPCTSTR pszVerbKey, UINT cchMax)
{
    // Table look-up (verb key -> menu string mapping)
    const static struct {
        LPCTSTR pszVerb;
        UINT  id;
    } sVerbTrans[] = {
        c_szOpen,    IDS_MENUOPEN,
        c_szExplore, IDS_MENUEXPLORE,
        c_szFind,    IDS_MENUFIND,
        c_szPrint,   IDS_MENUPRINT,
        c_szOpenAs,  IDS_MENUOPEN,
        TEXT("runas"),IDS_MENURUNAS
    };
    const struct {
        LPCTSTR pszVerb;
    } sVerbIgnore[] = {
        c_szPrintTo
    };

    int i;

    VDATEINPUTBUF(pszMenuString, TCHAR, cchMax);

    for (i = 0; i < ARRAYSIZE(sVerbTrans); i++)
    {
        if (lstrcmpi(pszVerbKey, sVerbTrans[i].pszVerb) == 0)
        {
            if (LoadString(HINST_THISDLL, sVerbTrans[i].id, pszMenuString, cchMax))
                return TRUE;
            break;
        }
    }

    for (i = 0; i < ARRAYSIZE(sVerbIgnore); i++)
    {
        if (lstrcmpi(pszVerbKey, sVerbIgnore[i].pszVerb) == 0)
        {
            return FALSE;
        }
    }

    //
    // Worst case: Just put '&' on the top.
    //
    if (!IsDBCSLeadByte(*pszVerbKey))
    {
        pszMenuString[0] = TEXT('&');
        pszMenuString++;
    }
    lstrcpy(pszMenuString, pszVerbKey);

    return TRUE;
}

BOOL _GetMenuStringFromDKA(HDKA hdka, UINT id, BOOL fExtended, LPTSTR pszMenu, UINT cchMax)
{
    LONG cbVerb = CbFromCch(cchMax);
    LPCTSTR pszVerbKey = DKA_GetKey(hdka, id);

    VDATEINPUTBUF(pszMenu, TCHAR, cchMax);

    //
    // Get the menu string.
    //
    if (fExtended || ERROR_SUCCESS != DKA_QueryOtherValue(hdka, id, TEXT("Extended"), NULL, NULL))
    {
        //  this is not an extended verb, or
        //  the request includes extended verbs
        
        if (DKA_QueryValue(hdka, id, pszMenu, &cbVerb) != ERROR_SUCCESS || cbVerb <= SIZEOF(TCHAR))
        {
            //
            // If it does not have the value, generate it.
            //
            return _IGenerateMenuString(pszMenu, pszVerbKey, cchMax);
        }

        return TRUE;
    }

    //  this is an extended value on a non-extended menu
    return FALSE;
}

STDMETHODIMP CDefExt_QueryContextMenu(IContextMenu *pcxm, HMENU hmenu,
        UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    CDefExt * this = IToClass(CDefExt, kcxm.unk, pcxm);
    HDKA hdka;
    TCHAR szMenu[CCH_MENUMAX];
    DWORD cb;
    UINT cVerbs = 0;
    BOOL fFind;

    fFind = !SHRestricted(REST_NOFIND);

    hdka = DefExt_GetDKA(this, uFlags & CMF_EXPLORE);
    if (hdka)
    {
        UINT idCmd;

        for (idCmd = idCmdFirst;
             idCmd <= idCmdLast && (idCmd - idCmdFirst) < (UINT)DKA_GetItemCount(hdka);
             idCmd++)
        {
            UINT uMenuFlags = MF_BYPOSITION | MF_STRING;

            if (fFind || lstrcmpi(DKA_GetKey(hdka, idCmd-idCmdFirst), c_szFind) != 0)
            {
                if (_GetMenuStringFromDKA(hdka, idCmd-idCmdFirst, uFlags & CMF_EXTENDEDVERBS, szMenu, ARRAYSIZE(szMenu)))
                {
                    InsertMenu(hmenu, indexMenu, uMenuFlags, idCmd, szMenu);
                    indexMenu++;
                }
            }
        }

        cVerbs = idCmd - idCmdFirst;

        if (GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1)
        {
            //
            //  if there is a default command make it so
            //
            if (cVerbs > 0 
            && (0 != (cb = SIZEOF(szMenu))) 
            && SHRegQueryValue(this->cshx.hkeyProgID, c_szShell, szMenu, &cb) == ERROR_SUCCESS 
            && szMenu[0])
            {
                //  we have to make sure the default command actually exists.
                HKEY hk;
                TCHAR sz[MAX_PATH];
                lstrcpy(sz, TEXT("shell\\"));
                StrCatBuff(sz, szMenu, SIZECHARS(sz));
                if (ERROR_SUCCESS == RegOpenKeyEx(this->cshx.hkeyProgID, sz, 0, KEY_READ, &hk))
                {
                    SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
                    RegCloseKey(hk);
                }
            }

            //
            // if there is no default command yet, and this key has a open
            // verb make that the default, if the SHIRT key is down make the
            // second verb default not the first.
            //
            else if (cVerbs>0 && (0 != (cb=SIZEOF(szMenu))) &&
                SHRegQueryValue(this->cshx.hkeyProgID, c_szShellOpenCmd, szMenu, &cb) == ERROR_SUCCESS && szMenu[0])
            {
                SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
            }
        }
    }

    //
    //  if we added no verbs we dont need the DKA anymore, make sure
    //  we nuke it in case we get IShellExt::Initialize again.
    //
    if (cVerbs == 0)
    {
        if (this->hdka)
            DKA_Destroy(this->hdka);
        this->hdka = NULL;
    }

    return ResultFromShort(cVerbs);
}


// Thought about making this perinstance, decided not to as to allow secondary
// process to abort this out.

STATIC BOOL s_fAbortInvoke = FALSE;

//----------------------------------------------------------------------------
// This private export allows the folder code a way to cause the main invoke
// loops processing several different files to abort.
void WINAPI SHAbortInvokeCommand()
{
    DebugMsg(DM_TRACE, TEXT("AbortInvokeCommand was called"));
    s_fAbortInvoke = TRUE;
}

//----------------------------------------------------------------------------
// Call shell exec (for the folder class) using the given file and the
// given pidl. The file will be passed as %1 in the dde command and the pidl
// will be passed as %2.
STDAPI InvokeFolderCommandUsingPidl(LPCMINVOKECOMMANDINFOEX pici, LPCTSTR pszPath,
        LPCITEMIDLIST pidl, HKEY hkClass, ULONG fExecuteFlags)
{
    SHELLEXECUTEINFO ei;
    INT iDrive = -1;

    if (FAILED(ICIX2SEI(pici, &ei)))
        return E_OUTOFMEMORY;

    ei.fMask |= SEE_MASK_IDLIST | fExecuteFlags;

    ei.lpFile = pszPath;
    ei.lpIDList = (LPVOID)pidl;

    //
    // if a directory is specifed use that, else make the current
    // directory be the folder it self. UNLESS it is a AUDIO CDRom, it
    // should never be the current directory (causes CreateProcess errors)
    //
    if (!ei.lpDirectory)
        ei.lpDirectory = pszPath;

    if (pszPath)
        iDrive = PathGetDriveNumber(ei.lpDirectory);

    if (CMtPt_IsAudioCD(iDrive))
    {
        ei.lpDirectory = NULL;
    }


    if (hkClass)
    {
        ei.hkeyClass = hkClass;
        ei.fMask |= SEE_MASK_CLASSKEY;
    }
    else
    {
        ei.fMask |= SEE_MASK_CLASSNAME;
        ei.lpClass = c_szFolderClass;
    }
    
    if (ShellExecuteEx(&ei))
        return S_OK;

    return HRESULT_FROM_WIN32(GetLastError());
}

STDMETHODIMP CDefExt_InvokeCommand(IContextMenu *pcxm, LPCMINVOKECOMMANDINFO pici)
{
    CDefExt * this = IToClass(CDefExt, kcxm.unk, pcxm);
    HRESULT hres = E_INVALIDARG;
    CMINVOKECOMMANDINFOEX ici;
    LPCTSTR pszVerbKey;
    LPVOID pvFree;

    //  thunk the incoming one...
    if (FAILED(ICI2ICIX(pici, &ici, &pvFree)))
        return E_OUTOFMEMORY;

    //
    // Check if ici.lpVerb specifying the verb index (0-based).
    //
    if (IS_INTRESOURCE(ici.lpVerb))
    {
        //
        // Yes, map it to the verb key string.
        //
        HDKA hdka = DefExt_GetDKA(this, FALSE);
        if (hdka)
        {
            pszVerbKey = DKA_GetKey(hdka, LOWORD((ULONG_PTR)ici.lpVerb));
        }
        else
        {
            //
            //  We come here if the registry is broken or missing critical
            // information like "*" classes. We assume all the commands are
            // open.
            //
            pszVerbKey = c_szOpen;
        }

        //  it is unnecessary to thunk, because 
        //  we only use the TCHAR version of the VERB...
        if (pszVerbKey)
        {
#ifdef UNICODE
            ici.lpVerbW = pszVerbKey;
#else
            ici.lpVerb = pszVerbKey;
#endif
        }
    }
    else
    {
#ifdef UNICODE
        pszVerbKey = ici.lpVerbW;
#else
        pszVerbKey = ici.lpVerb;
#endif
    }

    //
    // Check if the pszVerbKey correctly points to the verb string
    //
    if (!IS_INTRESOURCE(pszVerbKey) && this->cshx.medium.hGlobal)
    {
        int iItem, cItems = HIDA_GetCount(this->cshx.medium.hGlobal);
        HKEY hkeyFolder = NULL;
        LPITEMIDLIST pidl = NULL;       // allocated on first use

        //
        // Invoke that named command on all the selected objects.
        //
        s_fAbortInvoke = FALSE; // reset this global for this run...

        for (iItem = 0; iItem < cItems; iItem++)
        {
            MSG msg;
            TCHAR szFilePath[MAX_PATH];
            LPITEMIDLIST pidlTemp;
            DWORD dwAttrib;

            // Try to give the user a way to escape out of this
            if (s_fAbortInvoke || GetAsyncKeyState(VK_ESCAPE) < 0)
                break;

            // And the next big mondo hack to handle CAD of our window
            // because the user thinks it is hung.
            if (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_NOREMOVE))
                break;  // Lets also bail..

            pidlTemp = HIDA_FillIDList(this->cshx.medium.hGlobal, iItem, pidl);
            if (pidlTemp == NULL)
            {
                hres = E_OUTOFMEMORY;
                break;
            }

            pidl = pidlTemp;

            // Can we get a path from this idlist (ie is it file system stuff)?

            dwAttrib = SFGAO_FILESYSTEM | SFGAO_FOLDER;
            if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szFilePath, ARRAYSIZE(szFilePath), &dwAttrib)) &&
                (dwAttrib & SFGAO_FILESYSTEM))
            {
                // BUGBUG: we know the contents of the pidl so we should not
                // have to hit the disk with this call
                SHELLEXECUTEINFO ei = {0};

                if (!(dwAttrib & SFGAO_FOLDER) && SUCCEEDED(ICIX2SEI(&ici, &ei)))
                {
                    ei.lpFile = szFilePath;

                    //
                    // only use the HKEY for the first file, let ShellExecute
                    // figure out what to do for all other files, by verb name.
                    //
                    if (iItem == 0)
                    {
                        ei.hkeyClass = this->cshx.hkeyProgID;
                        ei.fMask |= SEE_MASK_CLASSKEY;
                    }
#ifdef WINNT
                    // Shrink the shell since the user is about to run an application.
                    ShrinkWorkingSet();
#endif					
                    // REVIEW: make current dir same as location?
                    if (ShellExecuteEx(&ei))
                    {
                        TCHAR szTemp[CCH_KEYMAX];
                        LPTSTR pszExt = PathFindExtension(szFilePath);
                        // now add it to the mru
                        // the GetClassDescription ensures that this is a registered object
                        // if it's not, then the OpenWith dialog will deal with adding it to the MRU
                        // (or not if the user hits cancel)

                        if (pszExt && !PathIsExe(szFilePath) &&
                            !PathIsShortcut(szFilePath) &&
                            GetClassDescription(HKEY_CLASSES_ROOT, pszExt, 
                                                szTemp, ARRAYSIZE(szTemp),
                                                GCD_ALLOWPSUDEOCLASSES | GCD_MUSTHAVEOPENCMD))
                        {
                            AddToRecentDocs(pidl, szFilePath);
                        }
#ifdef WINNT
                        // Shrink the shell since the user just ran an application.
                        ShrinkWorkingSet();
#endif
                        hres = S_OK;
                    }
                    else
                    {
                        // let caller know we failed (we may be calling this
                        // function from within ShellExecuteEx, and the caller
                        // of that may care about failure!)
                        hres = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                else
                {
                    // set this when iItem == 0 so that if we get back
                    // here for other folders, we'll reuse the key,
                    // but if the 0th item wasn't a folder,
                    // don't use it's key and try to open a folder
                    // with Notepad's shell\open\command or something like that.
                    if (iItem == 0)
                        hkeyFolder = this->cshx.hkeyProgID;

                    // Yes, we have to be careful with folders. We need to
                    // provide both the path (for folder extensions) and
                    // the pidl (so cabinet can find it quickly).
                    hres = InvokeFolderCommandUsingPidl(&ici, szFilePath, pidl, hkeyFolder, 0);
                }
            }
            else
            {
                // Nope, no alternative but to just use the pidl.
                hres = InvokeFolderCommandUsingPidl(&ici, NULL, pidl, this->cshx.hkeyProgID, 0);
            }

            if (hres == E_OUTOFMEMORY)
            {
                DebugMsg(TF_ERROR, TEXT("FileDefExt::InvokeCommand - Fail Out of Memory"));
                break;
            }
        }

        if (pidl)
            ILFree(pidl);
    }

    if (pvFree)
        LocalFree(pvFree);

    return hres;
}

UINT_PTR DKA_FindIndex(HDKA hdka, LPCTSTR pszVerb)
{
    int i;

    for (i = DKA_GetItemCount(hdka) - 1; i >= 0; --i)
    {
        if (!lstrcmpi(pszVerb, DKA_GetKey(hdka, i)))
            return i;       // found it!
    }
    return MAXUINT_PTR;
}

STDAPI_(BOOL) IsFarEastPlatform();


//
// CDefExt::GetCommandString
//
STDMETHODIMP CDefExt_GetCommandString(IContextMenu *pcxm,
        UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    CDefExt * this = IToClass(CDefExt, kcxm.unk, pcxm);
    HRESULT hres = E_OUTOFMEMORY;

    //
    // First, create hdka for this object.
    //
    HDKA hdka = DefExt_GetDKA(this, FALSE);
    if (hdka)
    {
        if (HIWORD64(idCmd))
        {
            if (uType & GCS_UNICODE)
            {
                TCHAR szCmd[MAX_PATH];

                if (IsBadStringPtrW((LPCWSTR)idCmd, (UINT)-1))
                    return E_INVALIDARG;

                SHUnicodeToTChar((LPCWSTR)idCmd, szCmd, ARRAYSIZE(szCmd));
                idCmd = DKA_FindIndex(hdka, szCmd);
            }
            else
            {
                TCHAR szCmd[MAX_PATH];
                SHAnsiToTChar((LPCSTR)idCmd, szCmd, ARRAYSIZE(szCmd));
                idCmd = DKA_FindIndex(hdka, szCmd);
            }
            
            if (idCmd == MAXUINT_PTR)
            {
                // that failed, try TCHAR version just in case caller messed up
                idCmd = DKA_FindIndex(hdka, (LPCTSTR)idCmd);
            }

            if (idCmd == MAXUINT_PTR)
                return E_INVALIDARG;
        }

        switch (uType)
        {
        case GCS_HELPTEXTA:
        case GCS_HELPTEXTW:
        {
            TCHAR szMenuString[CCH_MENUMAX];

            if (_GetMenuStringFromDKA(hdka, (UINT)idCmd, TRUE, szMenuString, ARRAYSIZE(szMenuString)))
            {
                LPTSTR pszHelp;
                
                // skip "?)" FE specific mnemonic sequence
                // we don't want to do this for non-FE platform
                //
                if (IsFarEastPlatform())
                {
                    pszHelp = StrChr(szMenuString, TEXT('(')); // dbcs safe
                    if (pszHelp && *(pszHelp + 1) == TEXT('&'))
                    {
                        LPTSTR pszHelpT = pszHelp+2;
                        int i;
                        
                        for(i=0; i<2 && *pszHelpT; i++, pszHelpT=CharNext(pszHelpT))
                            ;

                        if (*pszHelpT == TEXT(')'))
                        {
                            MoveMemory(pszHelp, pszHelpT+1, lstrlen(pszHelpT) * SIZEOF(TCHAR));
                        }
                    }
                }
                // We need to remove first '&' for any platform

                pszHelp = StrChr(szMenuString, TEXT('&'));
                if (pszHelp)
                    MoveMemory(pszHelp, pszHelp+1, lstrlen(pszHelp) * SIZEOF(TCHAR));

                pszHelp = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_VERBHELP), szMenuString);
                if (pszHelp)
                {
                    if (uType == GCS_HELPTEXTA)
                        SHTCharToAnsi(pszHelp, pszName, cchMax);
                    else
                        SHTCharToUnicode(pszHelp, (LPWSTR)pszName, cchMax);
                    LocalFree(pszHelp);
                    hres = NOERROR;
                }
            }
        }
            break;

        case GCS_VERBA:
        case GCS_VERBW:
        {
            LPCTSTR pszVerbKey = DKA_GetKey(hdka, (int)idCmd);
            if (pszVerbKey)
            {
                if (uType == GCS_VERBA)
                    SHTCharToAnsi(pszVerbKey, pszName, cchMax);
                else
                    SHTCharToUnicode(pszVerbKey, (LPWSTR)pszName, cchMax);
                hres = NOERROR;
            }
        }
            break;

        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
                hres = idCmd < (UINT)DKA_GetItemCount(hdka) ? NOERROR : S_FALSE;
                break;

        default:
                hres = E_NOTIMPL;
                break;
        }
    }

    return hres;
}

IContextMenuVtbl c_CDefExtCXMVtbl =
{
    Common_QueryInterface, Common_AddRef, Common_Release,
    CDefExt_QueryContextMenu,
    CDefExt_InvokeCommand,
    CDefExt_GetCommandString
};

//
// CDefExt constructor
//
STDMETHODIMP CDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv, LPFNADDPAGES pfnAddPages)
{
    HRESULT hres = E_OUTOFMEMORY;
    CDefExt *pshcmd;

    if (punkOuter)
    {
        *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    pshcmd = (CDefExt *)LocalAlloc(LPTR, SIZEOF(CDefExt));
    if (pshcmd)
    {
        // Initialize CommonUnknown
        pshcmd->cunk.unk.lpVtbl = &c_CDefExtVtbl;
        pshcmd->cunk.cRef = 1;

        // Initialize CCommonShellExtInit
        CCommonShellExtInit_Init(&pshcmd->cshx, &pshcmd->cunk);

        // Initialize CCommonShellPropSheetExt
        CCommonShellPropSheetExt_Init(&pshcmd->cspx, &pshcmd->cunk, pfnAddPages);

        // Initialize CKnownContextMenu
        pshcmd->kcxm.unk.lpVtbl = &c_CDefExtCXMVtbl;
        pshcmd->kcxm.nOffset = (int)((INT_PTR)&pshcmd->kcxm - (INT_PTR)&pshcmd->cunk);

        hres = CDefExt_QueryInterface(&pshcmd->cunk.unk, riid, ppv);
        CDefExt_Release(&pshcmd->cunk.unk);
    }

    return hres;
}

HRESULT CALLBACK CShellFileDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CDefExt_CreateInstance(punkOuter, riid, ppv, FileSystem_AddPages);
}

HRESULT CALLBACK CShellDrvDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CDefExt_CreateInstance(punkOuter, riid, ppv, CDrives_AddPages);
}
