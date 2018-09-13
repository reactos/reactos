/*
NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 

This file is #include'd in browseui\ and shdocvw\ util.cpp. these are too small
to add an extra dependency, so they're just shared. ideally, these should move
to shlwapi or comctl32 or some lib or ...

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
*/

#include "ccstock2.h"
#include "mluisupp.h"

STDAPI_(BOOL) IsBrowseNewProcess()
{
    return SHRegGetBoolUSValue(REGSTR_PATH_EXPLORER TEXT("\\BrowseNewProcess"), TEXT("BrowseNewProcess"), FALSE, FALSE);
}

// Should we run browser in a new process?
STDAPI_(BOOL) IsBrowseNewProcessAndExplorer()
{
    if (GetModuleHandle(TEXT("EXPLORER.EXE")))
        return IsBrowseNewProcess();

    return FALSE;   // Not in shell process so ignore browse new process flag
}

HRESULT _NavigateFrame(IUnknown *punkFrame, LPCTSTR pszPath, BOOL fIsInternetShortcut)
{
    HRESULT hr = E_OUTOFMEMORY;
    BSTR bstr = SysAllocStringT(pszPath);

    if (bstr)
    {
        if (fIsInternetShortcut)
        {
            IOleCommandTarget *pcmdt;
            hr = IUnknown_QueryService(punkFrame, SID_SHlinkFrame, IID_PPV_ARG(IOleCommandTarget, &pcmdt));
            if (SUCCEEDED(hr))
            {
                VARIANT varShortCutPath = {0};
                VARIANT varFlag = {0};

                varFlag.vt = VT_BOOL;
                varFlag.boolVal = VARIANT_TRUE;

                varShortCutPath.vt = VT_BSTR;
                varShortCutPath.bstrVal = bstr;

                hr = pcmdt->Exec(&CGID_Explorer, SBCMDID_IESHORTCUT, 0, &varShortCutPath, &varFlag);                
                pcmdt->Release();
            }
        }
        else
        {
            IWebBrowser2 *pwb;
            hr = IUnknown_QueryService(punkFrame, SID_SHlinkFrame, IID_PPV_ARG(IWebBrowser2, &pwb));
            if (SUCCEEDED(hr))
            {
                hr = pwb->Navigate(bstr, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);
                hr = pwb->put_Visible(VARIANT_TRUE);
                pwb->Release();
            }
        }
        SysFreeString(bstr);
    }
    return hr;
}

//
// Take a path or an URL and create a shorcut to navigare to it
//
STDAPI IENavigateIEProcess(LPCTSTR pszPath, BOOL fIsInternetShortcut)
{
    IUnknown *punk;
    HRESULT hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        hr = _NavigateFrame(punk, pszPath, fIsInternetShortcut);
        punk->Release();
    }
    
    return hr;
}
        


// If this is an internet shortcut (.url file), we want it to
// navigate using using the file name so the frame frame
// can read data beyond out of that file. this includes frame set
// navigation and data that script on the page may have stored.

/*
    Purpose : This function takes a path to a file. if that file is a .URL we try
    to navigate with that file name. this is because .URL files have extra data stored
    in them that we want to let script on the page get to. the exec we send here
    lets the frame know the .URL file that this came from

    Parameters : file name of .URL file (maybe) : In param
    pUnk :       Pointer to Object from which you can get the IOleCommandTarget

  returns:
    TRUE    handled
    FALSE   not handled, file might not be a .URL
*/


STDAPI NavFrameWithFile(LPCTSTR pszPath, IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    LPTSTR pszExt = PathFindExtension(pszPath);
    // HACK: we hard code .URL. this should be a property of the file type
    if (0 == StrCmpI(pszExt, TEXT(".url")))
    {
#ifdef BROWSENEWPROCESS_STRICT // "Nav in new process" has become "Launch in new process", so this is no longer needed
        if (IsBrowseNewProcessAndExplorer())
            hr = IENavigateIEProcess(pszPath, TRUE);
        else
#endif
            hr = _NavigateFrame(punk, pszPath, TRUE);
    }

    return hr;
}

// get the win32 file system name (path) for the item
// and optional attributes
//
// pdwAttrib may be NULL
// in/out:
//      pdwAttrib   may be NULL, attributes to query on the item

STDAPI GetPathForItem(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pszPath, DWORD *pdwAttrib)
{
    HRESULT hres = E_FAIL;
    DWORD dwAttrib;

    if (pdwAttrib == NULL)
    {
        pdwAttrib = &dwAttrib;
        dwAttrib = SFGAO_FILESYSTEM;
    }
    else
        *pdwAttrib |= SFGAO_FILESYSTEM;

    if (SUCCEEDED(psf->GetAttributesOf(1, &pidl, pdwAttrib)) &&
        (*pdwAttrib & SFGAO_FILESYSTEM))
    {
        STRRET str;
        hres = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);
        if (SUCCEEDED(hres))
            StrRetToStrN(pszPath, MAX_PATH, &str, pidl);
    }
    return hres;
}

STDAPI EditBox_TranslateAcceleratorST(LPMSG lpmsg)
{

    switch (lpmsg->message) {
    case WM_KEYUP:      // eat these (if we process corresponding WM_KEYDOWN)
    case WM_KEYDOWN:    // process these
        if (lpmsg->wParam != VK_TAB)
        {
            // all keydown messages except for the tab key should go straight to
            // the edit control -- unless the Ctrl key is down, in which case there
            // are 9 messages that should go straight to the edit control
#ifdef DEBUG
            if (lpmsg->wParam == VK_CONTROL)
                return S_FALSE;
#endif

            if (GetKeyState(VK_CONTROL) & 0x80000000)
            {
                switch (lpmsg->wParam)
                {
                case VK_RIGHT:
                case VK_LEFT:
                case VK_UP:
                case VK_DOWN:
                case VK_HOME:
                case VK_END:
                case VK_F4:
                case VK_INSERT:
                case VK_DELETE:
                case 'C':
                case 'X':
                case 'V':
                case 'A':
                case 'Z':
                    // these Ctrl+key messages are used by the edit control
                    // send 'em straight there
                    break;

                default:
                    return(S_FALSE);
                }
            }
            else
            {
                switch(lpmsg->wParam)
                {
                case VK_F5: // for refresh
                case VK_F6: // for cycle focus
                    return(S_FALSE);
                }
            }

            // Note that we return S_OK.
            goto TranslateDispatch;
        }
        break;


    case WM_CHAR:
TranslateDispatch:
        TranslateMessage(lpmsg);
        DispatchMessage(lpmsg);
        return(S_OK);
    }

    return S_FALSE;
}

// BUGBUG: dupe with shell32 util.cpp function
// like OLE GetClassFile(), but it only works on ProgID\CLSID type registration
// not real doc files or pattern matched files
//
STDAPI _CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid)
{
    TCHAR szProgID[80];
    DWORD cb = SIZEOF(szProgID);
    if (SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, szProgID, &cb) == ERROR_SUCCESS)
    {
        TCHAR szCLSID[80];

        StrCatBuff(szProgID, TEXT("\\CLSID"), ARRAYSIZE(szProgID));
        cb = SIZEOF(szCLSID);

        if (SHGetValue(HKEY_CLASSES_ROOT, szProgID, NULL, NULL, szCLSID, &cb) == ERROR_SUCCESS)
        {
            return GUIDFromString(szCLSID, pclsid) ? S_OK : E_FAIL;
        }
    }
    return E_FAIL;
}

#if 0 // not used yet
// IShellLink is #defined to IShellLinkA or IShellLinkW depending on compile flags,
// bug Win95 did not support IShellLinkW.  So call this function instead and you
// get the correct results regardless of what platform you are running on.
// REVIEW: In fact, we probably want these for ALL IShellLink functions...
//
LWSTDAPI IShellLink_GetPathA(IUnknown *punk, LPSTR pszBuf, UINT cchBuf, DWORD dwFlags)
{
    HRESULT hres = E_INVALIDARG;
    
    RIPMSG(cchBuf && pszBuf && IS_VALID_WRITE_BUFFER(pszBuf, char, cchBuf), "IShellLink_GetPathA: callre passed bad pszBuf/cchBuf");
    DEBUGWhackPathBufferA(pszBuf, cchBuf);

    if (cchBuf && pszBuf)
    {
        // In case of gross failure, NULL output buffer
        *pszBuf = 0;

        IShellLinkA * pslA;
        hres = punk->QueryInterface(IID_IShellLinkA, (void**)&pslA);
        if (SUCCEEDED(hres))
        {
            hres = pslA->GetPath(pszBuf, cchBuf, NULL, dwFlags);
            pslA->Release();
        }
        else if (FAILED(hres))
        {
#ifdef UNICODE
            IShellLinkW *pslW;
            hres = punk->QueryInterface(IID_IShellLinkW, (void**)&pslW);
            if (SUCCEEDED(hres))
            {
                WCHAR wszPath[MAX_BUF];
                LPWSTR pwszBuf = wszPath;
                UINT cch = ARRAYSIZE(wszPath);

                // Our stack buffer is too small, allocate one of the output buffer size
                if (cchBuf > cch)
                {
                    LPWSTR pwsz = LocalAlloc(LPTR, cchBuf * sizeof(WCHAR));
                    if (pwsz)
                    {
                        pwszBuf = pwsz;
                        cch = cchBuf;
                    }
                }

                hres = pslW->GetPath(pwszBuf, cch, NULL, dwFlags);
                if (SUCCEEDED(hres))
                {
                    SHUnicodeToAnsi(pwszBuf, pszBuf, cchBuf);
                }

                pslW->Release();
            }
#endif
        }
    }

    return hres;
}

LWSTDAPI IShellLink_GetPathW(IUnknown *punk, LPWSTR pwszBuf, UINT cchBuf, DWORD dwFlags)
{
    HRESULT hres = E_INVALIDARG;
    
    RIPMSG(cchBuf && pwszBuf && IS_VALID_WRITE_BUFFER(pwszBuf, WCHAR, cchBuf), "IShellLink_GetPathW: caller passed bad pwszBuf/cchBuf");
    DEBUGWhackPathBufferW(pwszBuf, cchBuf);

    if (cchBuf && pwszBuf)
    {
        // In case of gross failure, NULL output buffer
        *pwszBuf = 0;

#ifdef UNICODE
        IShellLinkW * pslW;
        hres = punk->QueryInterface(IID_IShellLinkW, (void**)&pslW);
        if (SUCCEEDED(hres))
        {
            hres = pslW->GetPath(pszBuf, cchBuf, NULL, dwFlags);
            pslW->Release();
        }
        else if (FAILED(hres))
#endif
        {
            IShellLinkA *pslA;
            hres = punk->QueryInterface(IID_IShellLinkA, (void**)&pslA);
            if (SUCCEEDED(hres))
            {
                char szPath[MAX_BUF];
                LPSTR pszBuf = szPath;
                UINT cch = ARRAYSIZE(szPath);

                // Our stack buffer is too small, allocate one of the output buffer size
                if (cchBuf > cch)
                {
                    LPSTR psz = LocalAlloc(LPTR, cchBuf * sizeof(char));
                    if (psz)
                    {
                        pszBuf = psz;
                        cch = cchBuf;
                    }
                }

                hres = pslA->GetPath(pszBuf, cch, NULL, dwFlags);
                if (SUCCEEDED(hres))
                {
                    SHAnsiToUnicode(pszBuf, pwszBuf, cchBuf);
                }

                pslA->Release();
            }
        }
    }

    return hres;
}
#endif // 0

HRESULT IShellLinkAorW_GetPath(IShellLinkA *pslA, LPTSTR pszBuf, UINT cchBuf, 
DWORD dwFlags)
{
    HRESULT hres = E_FAIL;

// If we store the string unicode, we could be losing file information by asking
// through A version. Be unicode friendly and use the W version if it exists
//
#ifdef UNICODE
    IShellLinkW *pslW;
    hres = pslA->QueryInterface(IID_IShellLinkW, (void**)&pslW);
    if (SUCCEEDED(hres))
    {
        hres = pslW->GetPath(pszBuf, cchBuf, NULL, dwFlags);
        pslW->Release();
    }
#endif

    if (FAILED(hres))
    {
        char szBuf[MAX_URL_STRING];  // BOGUS, but this is a common size used, perhaps we should LocalAlloc...

        cchBuf = ARRAYSIZE(szBuf);

        hres = pslA->GetPath(szBuf, cchBuf, NULL, dwFlags);

        SHAnsiToTChar(szBuf, pszBuf, cchBuf);
    }

    return hres;
}

STDAPI GetLinkTargetIDList(LPCTSTR pszPath, LPTSTR pszTarget, DWORD cchTarget, LPITEMIDLIST *ppidl)
{
    IShellLinkA *psl;
    CLSID clsid;
    HRESULT hres;

    *ppidl = NULL;  // assume failure

    // BUGBUG: we really should call GetClassFile() but this could
    // slow this down a lot... so chicken out and just look in the registry

    if (FAILED(_CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void **)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];

            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
            hres = ppf->Load(wszPath, 0);
            if (SUCCEEDED(hres))
            {
                psl->GetIDList(ppidl);

                if (*ppidl == NULL)
                    hres = E_FAIL;  // NULL pidl is valid, but
                                    // lets not return that to clients
                if (pszTarget)
                {
                    IShellLinkAorW_GetPath(psl, pszTarget, cchTarget, 0);
                }
            }
            ppf->Release();
        }
        psl->Release();
    }

    // pszPath might == pszTarget so don't null out on entry always
    if (FAILED(hres) && pszTarget)
        *pszTarget = 0;
    return hres;
}



STDAPI_(void) PathToDisplayNameW(LPCTSTR pszPath, LPTSTR pszDisplayName, UINT cchDisplayName)
{
    SHFILEINFO sfi;
    if (SHGetFileInfo(pszPath, 0, &sfi, SIZEOF(sfi), SHGFI_DISPLAYNAME))
    {
        StrCpyN(pszDisplayName, sfi.szDisplayName, cchDisplayName);
    }
    else
    {
        StrCpyN(pszDisplayName, PathFindFileName(pszPath), cchDisplayName);
        PathRemoveExtension(pszDisplayName);
    }
}


STDAPI_(void) PathToDisplayNameA(LPSTR pszPathA, LPSTR pszDisplayNameA, int cchDisplayName)
{
    SHFILEINFOA sfi;
    if (SHGetFileInfoA(pszPathA, 0, &sfi, SIZEOF(sfi), SHGFI_DISPLAYNAME))
    {
        StrCpyNA(pszDisplayNameA, sfi.szDisplayName, cchDisplayName);
    }
    else
    {
        pszPathA = PathFindFileNameA(pszPathA);
        StrCpyNA(pszDisplayNameA, pszPathA, cchDisplayName);
        PathRemoveExtensionA(pszDisplayNameA);
    }
}

// this looks for the file descriptor format to get the display name of a data object
STDAPI DataObj_GetNameFromFileDescriptor(IDataObject *pdtobj, LPWSTR pszDisplayName, UINT cch)
{
    HRESULT hres = E_FAIL;
    STGMEDIUM mediumFGD;

    InitClipboardFormats();
    FILEGROUPDESCRIPTORW * pfgd = (FILEGROUPDESCRIPTORW *)DataObj_GetDataOfType(pdtobj, g_cfFileDescW, &mediumFGD);
    if (pfgd)
    {
        if (pfgd->cItems > 0)
        {
            LPFILEDESCRIPTORW pfd = &(pfgd->fgd[0]);
            SHUnicodeToTChar(pfd->cFileName, pszDisplayName, cch);
            hres = S_OK;
        }
        ReleaseStgMediumHGLOBAL(&mediumFGD);
    }
    else
    {
        FILEGROUPDESCRIPTORA * pfgd = (FILEGROUPDESCRIPTORA *)DataObj_GetDataOfType(pdtobj, g_cfFileDescA, &mediumFGD);
        if (pfgd)
        {
            if (pfgd->cItems > 0)
            {
                LPFILEDESCRIPTORA pfd = &(pfgd->fgd[0]);
                SHAnsiToTChar(pfd->cFileName, pszDisplayName, cch);
                hres = S_OK;
            }
            ReleaseStgMediumHGLOBAL(&mediumFGD);
        }
    }
    return hres;
}

STDAPI SHPidlFromDataObject2(IDataObject *pdtobj, LPITEMIDLIST * ppidl)
{
    HRESULT hres = E_FAIL;
    STGMEDIUM medium;

    InitClipboardFormats();
    void *pdata = DataObj_GetDataOfType(pdtobj, g_cfHIDA, &medium);
    if (pdata)
    {
        *ppidl = IDA_ILClone((LPIDA)pdata, 0);
        if (*ppidl)
            hres = S_OK;
        else
            hres = E_OUTOFMEMORY;
        ReleaseStgMediumHGLOBAL(&medium);
    }

    return hres;
}

STDAPI SHPidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST *ppidl,
                           LPWSTR pszDisplayNameW, DWORD cchDisplayName)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    *ppidl = NULL;

    HRESULT hres = pdtobj->GetData(&fmte, &medium);
    if (hres == S_OK)
    {
        // This string is also used to store an URL in case it's an URL file
        TCHAR szPath[MAX_URL_STRING];
        hres = E_FAIL;
        if (DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)))
        {
            SHFILEINFO sfi;
            SHGetFileInfo(szPath, 0, &sfi, SIZEOF(sfi), SHGFI_ATTRIBUTES | SHGFI_DISPLAYNAME);

            if (pszDisplayNameW)
                SHTCharToUnicode(sfi.szDisplayName, pszDisplayNameW, MAX_PATH);

            if (sfi.dwAttributes & SFGAO_LINK)
                hres = GetLinkTargetIDList(szPath, szPath, ARRAYSIZE(szPath), ppidl);

            if (FAILED(hres))
                hres = IECreateFromPath(szPath, ppidl);
        }
        ReleaseStgMedium(&medium);
    }
    else
    {
        hres = SHPidlFromDataObject2(pdtobj, ppidl);
        if (FAILED(hres))
        {
            void *pdata = DataObj_GetDataOfType(pdtobj, g_cfURL, &medium);
            if (pdata)
            {
                LPSTR pszPath = (LPSTR)pdata;

                if (pszDisplayNameW) {
                    if (FAILED(DataObj_GetNameFromFileDescriptor(pdtobj, pszDisplayNameW, cchDisplayName))) {
                        CHAR szDisplayNameA[MAX_URL_STRING];
                        ASSERT(cchDisplayName < MAX_URL_STRING);
                        SHUnicodeToAnsi(pszDisplayNameW, szDisplayNameA, cchDisplayName);
                        PathToDisplayNameA(pszPath, szDisplayNameA, cchDisplayName);
                    }
                }
                hres = IECreateFromPathA(pszPath, ppidl);
                ReleaseStgMediumHGLOBAL(&medium);
            }
        }

    }

    return hres;
}



// BharatS - Perhaps all the stuff below here should be moved to shlwapi after beta 2 ?

typedef struct _broadcastmsgparams
{
    BOOL fSendMessage; // If true - we call SendMessageTimeout
    UINT uTimeout; // Only Matters if fSendMessage is set
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
} BROADCAST_MSG_PARAMS;

BOOL CALLBACK EnumShellIEWindowsProc(  
    HWND hwnd,      // handle to parent window
    LPARAM lParam   // application-defined value - this has the info needed for posting/sending the message 
)
{
    BROADCAST_MSG_PARAMS *pParams = (BROADCAST_MSG_PARAMS *)lParam;
    BOOL fRet = TRUE;

    if(IsExplorerWindow(hwnd) || IsFolderWindow(hwnd))
    {
        if(pParams->fSendMessage)
        {
            UINT  uTimeout = (pParams->uTimeout < 4000) ? pParams->uTimeout : 4000;
            LRESULT lResult;
            DWORD_PTR dwpResult;
            if (g_fRunningOnNT)
            {
                lResult = SendMessageTimeout(hwnd, pParams->uMsg, pParams->wParam, pParams->lParam, SMTO_ABORTIFHUNG | SMTO_NORMAL, uTimeout, &dwpResult);
            }
            else
            {           
                lResult = SendMessageTimeoutA(hwnd, pParams->uMsg, pParams->wParam, pParams->lParam, SMTO_ABORTIFHUNG | SMTO_NORMAL, uTimeout, &dwpResult);
            }
            fRet = BOOLIFY(lResult);
        }
        else
        {
            fRet = PostMessage(hwnd, pParams->uMsg, pParams->wParam, pParams->lParam);

        }
    }
    return fRet;

}

// PostShellIEBroadcastMessage is commented out since it is not used currentl
/*

STDAPI_(LRESULT)  PostShellIEBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{   
    BROADCAST_MSG_PARAMS MsgParam;

    MsgParam.uMsg = uMsg;
    MsgParam.wParam = wParam;
    MsgParam.lParam = lParam;
    MsgParam.fSendMessage = FALSE;
    
    return EnumWindows((WNDENUMPROC )EnumShellIEWindowsProc, (LPARAM)&MsgParam);

}
*/

//
// We can be hung if we use sendMessage, and you can not use pointers with asynchronous
// calls such as PostMessage or SendNotifyMessage.  So we resort to using a timeout.
// This function should be used to broadcast notification messages, such as WM_SETTINGCHANGE, 
// that pass pointers. (stevepro)
//
STDAPI_(LRESULT) SendShellIEBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT uTimeout)
{

    // Note that each this timeout is applied to each window that we broadcast to 

    BROADCAST_MSG_PARAMS MsgParam;

    MsgParam.uMsg = uMsg;
    MsgParam.wParam = wParam;

#ifdef UNICODE
    CHAR szSection[MAX_PATH];
    
    if (!g_fRunningOnNT && (uMsg == WM_WININICHANGE) && (0 != lParam))
    {
        SHUnicodeToAnsi((LPCWSTR)lParam, szSection, ARRAYSIZE(szSection));
        lParam = (LPARAM)szSection;
    }
#endif

    MsgParam.lParam = lParam;
    MsgParam.fSendMessage = TRUE;
    MsgParam.uTimeout = uTimeout;

    return EnumWindows((WNDENUMPROC )EnumShellIEWindowsProc, (LPARAM)&MsgParam);
}

// Return the parent psf and relative pidl given a pidl.
STDAPI IEBindToParentFolder(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild)
{
    HRESULT hres;

    //
    //  if this is a rooted pidl and it is just the root
    //  then we can bind to the target pidl of the root instead
    //
    if (ILIsRooted(pidl) && ILIsEmpty(_ILNext(pidl)))
        pidl = ILRootedFindIDList(pidl);
        
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    
    if (pidlParent)
    {
        hres = IEBindToObject(pidlParent, ppsfParent);
        ILFree(pidlParent);
    }
    else
        hres = E_OUTOFMEMORY;

    if (ppidlChild)
        *ppidlChild = ILFindLastID(pidl);

    return hres;
}

STDAPI GetDataObjectForPidl(LPCITEMIDLIST pidl, IDataObject ** ppdtobj)
{
    HRESULT hres = E_FAIL;
    if (pidl)
    {
        IShellFolder *psfParent;
        LPCITEMIDLIST pidlChild;
        hres = IEBindToParentFolder(pidl, &psfParent, &pidlChild);
        if (SUCCEEDED(hres))
        {
            hres = psfParent->GetUIObjectOf(NULL, 1, &pidlChild, IID_IDataObject, NULL, (void**)ppdtobj);
            psfParent->Release();
        }
    }
    return hres;
}

// Is this pidl a Folder/Directory in the File System?
STDAPI_(BOOL) ILIsFileSysFolder(LPCITEMIDLIST pidl)
{
    if (!pidl)
        return FALSE;

    DWORD dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    HRESULT hr = IEGetAttributesOf(pidl, &dwAttributes);
    return SUCCEEDED(hr) && ((dwAttributes & (SFGAO_FOLDER | SFGAO_FILESYSTEM)) == (SFGAO_FOLDER | SFGAO_FILESYSTEM));
}


// HACKHACK HACKHACK
// the following functions are to work around the menu
// munging that happens in the shlwapi wrappers... when we're
// manipulating menus which are tracked by the system, the
// menu munging code in our shlwapi wrappers (necessary
// for xcp plugUI) trashes them since the system doesn't
// understand munged menus... hence the work arounds below.
// note that many of these functions are copies of the shlwapi
// *WrapW functions (minus the munging).

#undef LoadMenuW

// from winuser.h
EXTERN_C WINUSERAPI HMENU WINAPI LoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName);

STDAPI_(HMENU)
LoadMenu_PrivateNoMungeW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
    ASSERT(HIWORD64(lpMenuName) == 0);

    if (g_fRunningOnNT)
    {
        return LoadMenuW(hInstance, lpMenuName);
    }

    return LoadMenuA(hInstance, (LPCSTR) lpMenuName);
}

#define CP_ATOM         0xFFFFFFFF          /* not a string at all */
#undef InsertMenuW

// from winuser.h
EXTERN_C WINUSERAPI BOOL WINAPI InsertMenuW(IN HMENU hMenu, IN UINT uPosition, IN UINT uFlags, IN UINT_PTR uIDNewItem, IN LPCWSTR lpNewItem);

STDAPI_(BOOL)
InsertMenu_PrivateNoMungeW(HMENU       hMenu,
                           UINT        uPosition,
                           UINT        uFlags,
                           UINT_PTR    uIDNewItem,
                           LPCWSTR     lpNewItem)
{
    if (g_fRunningOnNT)
    {
        return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
    }

    char szMenuItem[CCH_MENUMAX];

    SHUnicodeToAnsiCP((uFlags & MFT_NONSTRING) ? CP_ATOM : CP_ACP,
                      lpNewItem,
                      szMenuItem,
                      ARRAYSIZE(szMenuItem));

    return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, szMenuItem);
}

STDAPI_(HMENU)
LoadMenuPopup_PrivateNoMungeW(UINT id)
{
    HINSTANCE hinst;

    hinst = MLLoadShellLangResources();

    HMENU hMenuSub = NULL;
    HMENU hMenu = LoadMenu_PrivateNoMungeW(hinst, MAKEINTRESOURCEW(id));
    if (hMenu)
    {
        hMenuSub = GetSubMenu(hMenu, 0);
        if (hMenuSub)
        {
            RemoveMenu(hMenu, 0, MF_BYPOSITION);
        }

        // note this calls the shlwapi wrapper (that handles
        // destroying munged menus) but it looks like
        // it's safe to do so.
        DestroyMenu(hMenu);
    }

    MLFreeLibrary(hinst);

    return hMenuSub;
}

// determine if a path is just a filespec (contains no path parts)
//
// REVIEW: we may want to count the # of elements, and make sure
// there are no illegal chars, but that is probably another routing
// PathIsValid()
//
// in:
//      lpszPath    path to look at
// returns:
//      TRUE        no ":" or "\" chars in this path
//      FALSE       there are path chars in there
//
//

BOOL PathIsFilePathA(LPCSTR lpszPath)
{
#ifdef UNIX
    if (lpszPath[0] == '/')
#else
    if ((lpszPath[0] == '\\') || (lpszPath[1] == ':'))
#endif
        return TRUE;

    return IsFileUrl(lpszPath);
}

//
// PrepareURLForDisplay
//
//     Decodes without stripping file:// prefix
//
STDAPI_(BOOL) PrepareURLForDisplayA(LPCSTR psz, LPSTR pszOut, LPDWORD pcchOut)
{
    if (PathIsFilePathA(psz))
    {
        if (IsFileUrl(psz))
            return SUCCEEDED(PathCreateFromUrlA(psz, pszOut, pcchOut, 0));

        StrCpyNA(pszOut, psz, *pcchOut);
        *pcchOut = lstrlenA(pszOut);
        return TRUE;
    }


    return SUCCEEDED(UrlUnescapeA((LPSTR)psz, pszOut, pcchOut, 0));
}


#undef InsertMenuW
#undef LoadMenuW

// from w95wraps.h
#define InsertMenuW                 InsertMenuWrapW
#define LoadMenuW                   LoadMenuWrapW

STDAPI SHTitleFromPidl(LPCITEMIDLIST pidl, LPTSTR psz, DWORD cch, BOOL fFullPath)
{
    // Tries to get a system-displayable string from a pidl.
    // (On Win9x and NT4, User32 doesn't support font-linking,
    // so we can't display non-system language strings as window
    // titles or menu items.  In those cases, we call this function
    // to grab the path/URL instead, which will likely be system-
    // displayable).

    UINT uType;

    *psz = NULL;
    TCHAR szName[MAX_URL_STRING];

    if (fFullPath)
        uType = SHGDN_FORPARSING;
    else
        uType = SHGDN_NORMAL;

    uType |= SHGDN_FORADDRESSBAR; 
    DWORD dwAttrib = SFGAO_LINK;

    HRESULT hr = IEGetNameAndFlags(pidl, uType, szName, SIZECHARS(szName), &dwAttrib);
    if (SUCCEEDED(hr))
    {
        if ((uType & SHGDN_FORPARSING) && (dwAttrib & SFGAO_LINK))
        {
            // folder shortcut special case
            IShellLinkA *psl;  // Use A version for W95.
            if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IShellLinkA, &psl))))
            {
                LPITEMIDLIST pidlTarget;
                if (SUCCEEDED(psl->GetIDList(&pidlTarget)) && pidlTarget)
                {
                    hr = IEGetNameAndFlags(pidlTarget, uType, szName, SIZECHARS(szName), NULL);
                    ILFree(pidlTarget);
                }
            }
        }
    }
    else
    {
        // didn't work, try the reverse
        uType ^= SHGDN_FORPARSING;  // flip the for parsing bit
        hr = IEGetNameAndFlags(pidl, uType, szName, SIZECHARS(szName), NULL);

        // some old namespaces get confused by all our funny bits...
        if (FAILED(hr))
        {
            hr = IEGetNameAndFlags(pidl, SHGDN_NORMAL, szName, SIZECHARS(szName), NULL);
        }
    }

    if (SUCCEEDED(hr))
    {
        SHRemoveURLTurd(szName);

        // HTTP URLs are not escaped because they come from the
        // user or web page which is required to create correctly
        // escaped URLs.  FTP creates then via results from the
        // FTP session, so their pieces (username, password, path)
        // need to be escaped when put in URL form.  However,
        // we are going to put that URL into the Caption Bar, and
        // and we want to unescape it because it's assumed to be
        // a DBCS name.  All of this is done because unescaped URLs
        // are pretty. (NT #1272882)
        if (URL_SCHEME_FTP == GetUrlScheme(szName))
        {
            CHAR szUrlTemp[MAX_BROWSER_WINDOW_TITLE];
            CHAR szUnEscaped[MAX_BROWSER_WINDOW_TITLE];
            DWORD cchSizeTemp = ARRAYSIZE(szUnEscaped);

            // This this thunking crap is necessary.  Unescaping won't
            // gell into DBCS chars unless it's in ansi.
            SHTCharToAnsi(szName, szUrlTemp, ARRAYSIZE(szUrlTemp));
            PrepareURLForDisplayA(szUrlTemp, szUnEscaped, &cchSizeTemp);
            SHAnsiToTChar(szUnEscaped, psz, cch);
        }
        else
        {
            StrCpyN(psz, szName, cch);
        }
    }

    return hr;
}

BOOL IsSpecialUrl(LPCWSTR pchURL)
{
    UINT      uProt;
    uProt = GetUrlSchemeW(pchURL);
    return (URL_SCHEME_JAVASCRIPT == uProt || 
            URL_SCHEME_VBSCRIPT == uProt ||
            URL_SCHEME_ABOUT == uProt);
}

//encode any incoming %1 so that people can't spoof our domain security code
HRESULT WrapSpecialUrl(BSTR * pbstrUrl)
{
    HRESULT     hr = S_OK;
    TCHAR      *pchSafeUrl = NULL;
    TCHAR      *pch;
    TCHAR       achUrl[4096];
    DWORD       dwSize;
    BSTR        bstrURL = *pbstrUrl;
    int         cSize;

    if (IsSpecialUrl(bstrURL))
    {
        //
        // If this is javascript:, vbscript: or about:, append the
        // url of this document so that on the other side we can
        // decide whether or not to allow script execution.
        //

        // QFE 2735 (Georgi XDomain): [alanau]
        //
        // If the special URL contains an %00 sequence, then it will be converted to a Null char when
        // encoded.  This will effectively truncate the Security ID.  For now, simply disallow this
        // sequence, and display a "Permission Denied" script error.
        //
        if (StrStrW(bstrURL, L"%00"))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        // Copy the URL so we can munge it.
        //
        cSize = SysStringLen(bstrURL) + 1;
        pchSafeUrl = new TCHAR[cSize];
        if (!pchSafeUrl)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } 
        StrCpyN(pchSafeUrl, bstrURL, cSize);

        // someone could put in a string like this:
        //     %2501 OR %252501 OR %25252501
        // which, depending on the number of decoding steps, will bypass security
        // so, just keep decoding while there are %s and the string is getting shorter
        int nPreviousLen = 0;
        while ( (nPreviousLen != lstrlen(pchSafeUrl)) && (StrChrW(pchSafeUrl, L'%')))
        {
            nPreviousLen = lstrlen(pchSafeUrl);
            int nNumPercents;
            int nNumPrevPercents = 0;

            // Reduce the URL
            //
            for (;;)
            {
                // Count the % signs.
                //
                nNumPercents = 0;

                pch = pchSafeUrl;
                while (pch = StrChrW(pch, L'%'))
                {
                    pch++;
                    nNumPercents++;
                }

                // If the number of % signs has changed, we've reduced the URL one iteration.
                //
                if (nNumPercents != nNumPrevPercents)
                {
                    // Encode the URL 
                    hr = THR(CoInternetParseUrl(pchSafeUrl, 
                        PARSE_ENCODE, 
                        0, 
                        achUrl, 
                        ARRAYSIZE(achUrl), 
                        &dwSize,
                        0));

                    StrCpyN(pchSafeUrl, achUrl, cSize);

                    nNumPrevPercents = nNumPercents;
                }
                else
                {
                    // The URL is fully reduced.  Break out of loop.
                    //
                    break;
                }
            }
        }

        // Now scan for '\1' characters.
        //
        if (StrChrW(pchSafeUrl, L'\1'))
        {
            // If there are '\1' characters, we can't guarantee the safety.  Put up "Permission Denied".
            //
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        SysFreeString(*pbstrUrl);
        *pbstrUrl = SysAllocString(pchSafeUrl);
        if (!*pbstrUrl)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

Cleanup:
    delete [] pchSafeUrl;
    return hr;
}

HRESULT WrapSpecialUrlFlat(LPWSTR pszUrl, DWORD cchUrl)
{
    HRESULT     hr = S_OK;

    if (IsSpecialUrl(pszUrl))
    {
        //
        // If this is javascript:, vbscript: or about:, append the
        // url of this document so that on the other side we can
        // decide whether or not to allow script execution.
        //

        // QFE 2735 (Georgi XDomain): [alanau]
        //
        // If the special URL contains an %00 sequence, then it will be converted to a Null char when
        // encoded.  This will effectively truncate the Security ID.  For now, simply disallow this
        // sequence, and display a "Permission Denied" script error.
        //
        if (StrStrW(pszUrl, L"%00"))
        {
            hr = E_ACCESSDENIED;
        }
        else
        {
            // munge the url in place
            //

            // someone could put in a string like this:
            //     %2501 OR %252501 OR %25252501
            // which, depending on the number of decoding steps, will bypass security
            // so, just keep decoding while there are %s and the string is getting shorter
            int nPreviousLen = 0;
            while ( (nPreviousLen != lstrlen(pszUrl)) && (StrChrW(pszUrl, L'%')))
            {
                nPreviousLen = lstrlen(pszUrl);
                int nNumPercents;
                int nNumPrevPercents = 0;

                // Reduce the URL
                //
                for (;;)
                {
                    // Count the % signs.
                    //
                    nNumPercents = 0;

                    WCHAR *pch = pszUrl;
                    while (pch = StrChrW(pch, L'%'))
                    {
                        pch++;
                        nNumPercents++;
                    }

                    // If the number of % signs has changed, we've reduced the URL one iteration.
                    //
                    if (nNumPercents != nNumPrevPercents)
                    {
                        WCHAR szBuf[MAX_URL_STRING];
                        DWORD dwSize;

                        // Encode the URL 
                        hr = THR(CoInternetParseUrl(pszUrl, 
                            PARSE_ENCODE, 
                            0,
                            szBuf,
                            ARRAYSIZE(szBuf),
                            &dwSize,
                            0));

                        StrCpyN(pszUrl, szBuf, cchUrl);

                        nNumPrevPercents = nNumPercents;
                    }
                    else
                    {
                        // The URL is fully reduced.  Break out of loop.
                        //
                        break;
                    }
                }
            }

            // Now scan for '\1' characters.
            //
            if (StrChrW(pszUrl, L'\1'))
            {
                // If there are '\1' characters, we can't guarantee the safety.  Put up "Permission Denied".
                //
                hr = E_ACCESSDENIED;
            }
        }
    }

    return hr;
}

//
//      GetUIVersion()
//
//  returns the version of shell32
//  3 == win95 gold / NT4
//  4 == IE4 Integ / win98
//  5 == win2k
//
STDAPI_(UINT) GetUIVersion()
{
    static UINT s_uiShell32 = 0;
    if (s_uiShell32 == 0)
    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hinst)
        {
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                s_uiShell32 = dllinfo.dwMajorVersion;
            else
                s_uiShell32 = 3;
        }
    }
    return s_uiShell32;
}

STDAPI GetBrowserFrameOptions(IUnknown *punkFolder, IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions)
{
    HRESULT hr = E_INVALIDARG;

    *pdwOptions = BFO_NONE;
    if (punkFolder)
    {
        IBrowserFrameOptions *pbfo;
        hr = punkFolder->QueryInterface(IID_PPV_ARG(IBrowserFrameOptions, &pbfo));
        if (SUCCEEDED(hr))
        {
            hr = pbfo->GetFrameOptions(dwMask, pdwOptions);
            pbfo->Release();
        }
    }

    return hr;
}

STDAPI GetBrowserFrameOptionsPidl(IN LPCITEMIDLIST pidl, IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions)
{
    HRESULT hr = E_INVALIDARG;

    *pdwOptions = BFO_NONE;
    if (pidl)
    {
        IBrowserFrameOptions *pbfo;
        hr = IEBindToObjectEx(pidl, NULL, IID_PPV_ARG(IBrowserFrameOptions, &pbfo));
        if (SUCCEEDED(hr))
        {
            hr = pbfo->GetFrameOptions(dwMask, pdwOptions);
            pbfo->Release();
        }
    }

    return hr;
}

// Return TRUE only if all the bits in dwMask are set.
STDAPI_(BOOL) IsBrowserFrameOptionsSet(IN IShellFolder * psf, IN BROWSERFRAMEOPTIONS dwMask)
{
    BOOL fSet = FALSE;
    BROWSERFRAMEOPTIONS dwOptions = 0;

    if (SUCCEEDED(GetBrowserFrameOptions(psf, dwMask, &dwOptions)) &&
        (dwOptions == dwMask))
    {
        fSet = TRUE;
    }

    return fSet;
}


// Return TRUE only if all the bits in dwMask are set.
STDAPI_(BOOL) IsBrowserFrameOptionsPidlSet(IN LPCITEMIDLIST pidl, IN BROWSERFRAMEOPTIONS dwMask)
{
    BOOL fSet = FALSE;
    BROWSERFRAMEOPTIONS dwOptions = 0;

    if (SUCCEEDED(GetBrowserFrameOptionsPidl(pidl, dwMask, &dwOptions)) &&
        (dwOptions == dwMask))
    {
        fSet = TRUE;
    }

    return fSet;
}


STDAPI_(BOOL) IsFTPFolder(IShellFolder * psf)
{
    BOOL fIsFTPFolder = FALSE;
    CLSID clsid;

    if (psf && SUCCEEDED(IUnknown_GetClassID(psf, &clsid)))
    {
        // Is this an FTP Folder?
        if (IsEqualIID(clsid, CLSID_FtpFolder))
            fIsFTPFolder = TRUE;
        else
        {
            // Not directly, but let's see if it is an Folder Shortcut to
            // an FTP Folder
            if (IsEqualIID(clsid, CLSID_FolderShortcut))
            {
                IShellLinkA * psl = NULL;
                HRESULT hr = psf->QueryInterface(IID_IShellLinkA, (void **) &psl);

                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;

                    hr = psl->GetIDList(&pidl);
                    if (SUCCEEDED(hr))
                    {
                        IShellFolder * psfTarget;

                        hr = IEBindToObject(pidl, &psfTarget);
                        if (SUCCEEDED(hr))
                        {
                            if (SUCCEEDED(IUnknown_GetClassID(psfTarget, &clsid)) &&
                                IsEqualIID(clsid, CLSID_FtpFolder))
                            {
                                fIsFTPFolder = TRUE;
                            }

                            psfTarget->Release();
                        }

                        ILFree(pidl);
                    }

                    psl->Release();
                }
            }
        }
    }

    return fIsFTPFolder;
}


