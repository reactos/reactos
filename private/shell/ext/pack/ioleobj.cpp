#include "priv.h"
#include "privcpp.h"

HWND            g_hTaskWnd;
BOOL CALLBACK GetTaskWndProc(HWND hwnd, DWORD lParam);
DWORD CALLBACK MainWaitOnChildThreadProc(void *lpv);

typedef struct {
    CPackage_IOleObject *pObj;
    HANDLE h;
} MAINWAITONCHILD;

CPackage_IOleObject::CPackage_IOleObject(CPackage *pPackage) : 
    _pPackage(pPackage)
{
    ASSERT(_cRef == 0); 
}

CPackage_IOleObject::~CPackage_IOleObject()
{
    DebugMsg(DM_TRACE,"CPackage_IOleObject destroyed with ref count %d",_cRef);
}


//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CPackage_IOleObject::QueryInterface(REFIID iid, void ** ppv)
{
    return _pPackage->QueryInterface(iid,ppv);
}

ULONG CPackage_IOleObject::AddRef(void) 
{
    _cRef++;    // interface ref count for debugging
    return _pPackage->AddRef();
}

ULONG CPackage_IOleObject::Release(void)
{
    _cRef--;    // interface ref count for debugging
    return _pPackage->Release();
}

//////////////////////////////////
//
// IOleObject Methods...
//
HRESULT CPackage_IOleObject::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    DebugMsg(DM_TRACE, "pack oo - SetClientSite() called.");

    if (!pClientSite)
        return E_POINTER;

    if (_pPackage->_pIOleClientSite != NULL)
        _pPackage->_pIOleClientSite->Release();
    
    _pPackage->_pIOleClientSite = pClientSite;
    _pPackage->_pIOleClientSite->AddRef();
    return S_OK;
}

HRESULT CPackage_IOleObject::GetClientSite(LPOLECLIENTSITE *ppClientSite) 
{
    DebugMsg(DM_TRACE, "pack oo - GetClientSite() called.");

    if (ppClientSite == NULL)
        return E_INVALIDARG;
    
    // Be sure to AddRef the pointer we're giving away.
    *ppClientSite = _pPackage->_pIOleClientSite;
    _pPackage->_pIOleClientSite->AddRef();
    
    return S_OK;
}

HRESULT CPackage_IOleObject::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    DebugMsg(DM_TRACE, "pack oo - SetHostNames() called.");

    delete _pPackage->_lpszContainerApp;
    
    if (NULL != (_pPackage->_lpszContainerApp = new OLECHAR[lstrlenW(szContainerApp) + 1]))
    {
        lstrcpyW(_pPackage->_lpszContainerApp,szContainerApp);
    }
    
    delete _pPackage->_lpszContainerObj;
    
    if (NULL != (_pPackage->_lpszContainerObj = new OLECHAR[lstrlenW(szContainerObj) + 1]))
    {
        lstrcpyW(_pPackage->_lpszContainerObj,szContainerObj);
    }

    switch(_pPackage->_panetype) {
        case PEMBED:
            if (_pPackage->_pEmbed->poo) 
                _pPackage->_pEmbed->poo->SetHostNames(szContainerApp,szContainerObj);
            break;
        case CMDLINK:
            // nothing to do...we're a link to a file, so we don't ever get
            // opened and need to be edited or some such thing.
            break;
    }
    
    return S_OK;
}

HRESULT CPackage_IOleObject::Close(DWORD dwSaveOption) 
{
    DebugMsg(DM_TRACE, "pack oo - Close() called.");

    switch (_pPackage->_panetype) {
        case PEMBED:
            if (_pPackage->_pEmbed == NULL)
                return S_OK;
            
            // tell the server to close, and release our pointer to it
            if (_pPackage->_pEmbed->poo) {
                _pPackage->_pEmbed->poo->Close(dwSaveOption);
                _pPackage->_pEmbed->poo->Unadvise(_pPackage->_dwCookie);
                _pPackage->_pEmbed->poo->Release();
                _pPackage->_pEmbed->poo = NULL;
            }
            break;
        case CMDLINK:
            // again, nothing to do...we shouldn't be getting close
            // messages since we're never activated through OLE.
            break;
    }
    if ((dwSaveOption != OLECLOSE_NOSAVE) && (_pPackage->_fIsDirty)) {
        _pPackage->_pIOleClientSite->SaveObject();
        _pPackage->_pIOleAdviseHolder->SendOnSave();
    }

    
    
    return S_OK;
}

HRESULT CPackage_IOleObject::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    DebugMsg(DM_TRACE, "pack oo - SetMoniker() called.");
    
    // NOTE: Uninteresting for embeddings only.
    return (E_NOTIMPL);
}

HRESULT CPackage_IOleObject::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, 
                               LPMONIKER *ppmk)
{
    DebugMsg(DM_TRACE, "pack oo - GetMoniker() called.");
    
    // NOTE: Unintersting for embeddings only.
    return (E_NOTIMPL);
}

HRESULT CPackage_IOleObject::InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, 
                                 DWORD dwReserved)
{
    DebugMsg(DM_TRACE, "pack oo - InitFromData() called.");
    
    // NOTE: This isn't supported at this time
    return (E_NOTIMPL);
}

HRESULT CPackage_IOleObject::GetClipboardData(DWORD dwReserved, LPDATAOBJECT *ppDataObject)
{
    DebugMsg(DM_TRACE, "pack oo - GetClipboardData() called.");
    
    if (ppDataObject == NULL) 
        return E_INVALIDARG;
    
    *ppDataObject = _pPackage->_pIDataObject;
    AddRef();
    return S_OK;
}

HRESULT CPackage_IOleObject::DoVerb(LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, 
                           LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{

    void *lpFileData = NULL;
    BOOL fError = TRUE;
    DWORD id;
    SHELLEXECUTEINFO sheinf = {sizeof(SHELLEXECUTEINFO)};
    LPEMBED lpembed = _pPackage->_pEmbed;       // saves some indirection

    
    DebugMsg(DM_TRACE, "pack oo - DoVerb() called.");
    DebugMsg(DM_TRACE, "            iVerb==%d",iVerb);

    // We allow show, primary verb, edit, and context menu verbs on our packages...
    //
    if (iVerb < OLEIVERB_SHOW)
        return E_NOTIMPL;
    
    /////////////////////////////////////////////////////////////////
    // SHOW VERB
    /////////////////////////////////////////////////////////////////
    //
    if (iVerb == OLEIVERB_SHOW) {
        PACKAGER_INFO packInfo;
        
        // Run the Wizard...
        PackWiz_CreateWizard(hwndParent, &packInfo);

        if (*packInfo.szFilename == TEXT('\0'))
        {
            ShellMessageBox(g_hinst,
                            NULL,
                            MAKEINTRESOURCE(IDS_CREATE_ERROR),
                            MAKEINTRESOURCE(IDS_APP_TITLE),
                            MB_ICONERROR | MB_TASKMODAL | MB_OK);
            return E_FAIL;
        }

        return _pPackage->InitFromPackInfo(&packInfo);
    }

    /////////////////////////////////////////////////////////////////
    // EDIT PACKAGE VERB
    /////////////////////////////////////////////////////////////////
    //
    else if (iVerb == OLEIVERB_EDITPACKAGE) {
        // Call the edit package dialog.  Which dialog is ultimately called will
        // depend on whether we're a cmdline package or an embedded file
        // package.
        int idDlg;
        PACKAGER_INFO packInfo;
        int ret;

        lstrcpy(packInfo.szLabel,_pPackage->_lpic->szIconText);
        lstrcpy(packInfo.szIconPath,_pPackage->_lpic->szIconPath);
        packInfo.iIcon = _pPackage->_lpic->iDlgIcon;
        
        switch(_pPackage->_panetype) {
            case PEMBED:
                lstrcpy(packInfo.szFilename, _pPackage->_pEmbed->fd.cFileName);
                idDlg = IDD_EDITEMBEDPACKAGE;
                break;
            case CMDLINK:
                lstrcpy(packInfo.szFilename, _pPackage->_pCml->szCommandLine);
                idDlg = IDD_EDITCMDPACKAGE;
                break;
        }

        ret = PackWiz_EditPackage(hwndParent,idDlg, &packInfo);

        // If User cancells the edit package...just return.  
        if (ret == -1)
            return S_OK;

        switch(_pPackage->_panetype) {
            case PEMBED:
                // if we have a tempfile, we'll want to delete it
                if (_pPackage->_pEmbed->pszTempName) {
                    DeleteFile(_pPackage->_pEmbed->pszTempName);
                    delete _pPackage->_pEmbed->pszTempName;
                    _pPackage->_pEmbed->pszTempName = NULL;
                    _pPackage->ReleaseContextMenu();
                }
                // fall through
                
            case CMDLINK:
                _pPackage->InitFromPackInfo(&packInfo);
                break;
        }
        return S_OK;
    }
    else if (iVerb == OLEIVERB_PRIMARY)
    {
        /////////////////////////////////////////////////////////////////
        // ACTIVATE CONTENTS VERB
        /////////////////////////////////////////////////////////////////
        // NOTE: This is kind of crazy looking code, partially because we have
        // to worry about two ways of launching things--ShellExecuteEx and 
        // calling through OLE.
        //
        
        switch(_pPackage->_panetype)
        {
            case PEMBED:
                if (FAILED(_pPackage->CreateTempFile()))  // will just return S_OK
                    return E_FAIL;                        // if we have a temp file
        
                // if this is an OLE file then, activate through OLE
                //
                if (lpembed->fIsOleFile)
                {
            
                    // If we've activated the server, then we can just pass this
                    // call along to it.
                    if (_pPackage->_pEmbed->poo) 
                    {
                        return _pPackage->_pEmbed->poo->DoVerb(iVerb,lpmsg,
                            pActiveSite,lindex, hwndParent, lprcPosRect);
                    }
            
                    WCHAR wszFile[MAX_PATH];
#ifdef UNICODE
                    lstrcpy(wszFile, _pPackage->_pEmbed->pszTempName);
#else  // UNICODE
                    MultiByteToWideChar(CP_ACP, 0, _pPackage->_pEmbed->pszTempName, -1, wszFile, ARRAYSIZE(wszFile));
#endif // UNICODE
            
                    CLSID clsid;
                    HRESULT hr = GetClassFile(wszFile, &clsid);
                    if (SUCCEEDED(hr)) 
                    {
                        IOleObject* poo;
                        hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IOleObject, (void **)&poo);
                        if (SUCCEEDED(hr)) 
                        {
                            hr = poo->Advise(_pPackage->_pIAdviseSink, &_pPackage->_dwCookie);
                            if (SUCCEEDED(hr)) 
                            {
                                // NOTE: This is really stupid, but apparently we have to call
                                // OleRun before we can get IPersistFile from some apps, namely
                                // Word and Excel. If we don't call OleRun, they fail our QI
                                // for IPersistfile.
                                OleRun(poo);
            
                                IPersistFile* ppf;
                                hr = poo->QueryInterface(IID_IPersistFile, (void **)&ppf);
                                if (SUCCEEDED(hr))
                                {

                                    hr = ppf->Load(wszFile, STGM_READWRITE | STGM_SHARE_DENY_WRITE);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = poo->DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent,
                                             lprcPosRect);
                                        if (SUCCEEDED(hr))
                                        {
                                            poo->SetHostNames(_pPackage->_lpszContainerApp, _pPackage->_lpszContainerObj);
                                            _pPackage->_pIOleClientSite->ShowObject();
                                            _pPackage->_pIOleClientSite->OnShowWindow(TRUE);
                                            _pPackage->_pEmbed->poo = poo;  // save this so when we get a
                                            poo = NULL;
                                        }
                                    }
                                    ppf->Release();
                                }
                            }
                            if (poo)
                                poo->Release();
                        }
                    }
                    if (SUCCEEDED(hr))
                        return hr;
                }   

                // Try to execute the file
                lpembed->fIsOleFile = FALSE;
                lpembed->hTask = NULL;

                sheinf.fMask  = SEE_MASK_NOCLOSEPROCESS;
                sheinf.lpFile = _pPackage->_pEmbed->pszTempName;
                sheinf.nShow  = SW_SHOWNORMAL;

                if (ShellExecuteEx(&sheinf))
                {
                    // if we get a valid process handle, we want to create a thread
                    // to wait for the process to exit so we know when we can load
                    // the tempfile back into memory.
                    //
                    if (sheinf.hProcess)
                    {
                        lpembed->hTask = sheinf.hProcess;
                        MAINWAITONCHILD *pmwoc = new MAINWAITONCHILD;
                        if (pmwoc)
                        {
                            pmwoc->pObj = this;
                            pmwoc->h = sheinf.hProcess;
                        
                            if (CreateThread(NULL, 0, MainWaitOnChildThreadProc, pmwoc, 0, &id))
                                fError = FALSE;
                            else 
                            {
                                CloseHandle(sheinf.hProcess);
                                return E_FAIL;
                            }
                        }
                    }
                    // NOTE: there's not much we can do if the ShellExecute
                    // succeeds and we don't get a valid handle back.  we'll just
                    // load from the temp file when we're asked to save and hope
                    // for the best.
                }   
                else // ShellExecuteEx failed!
                {
                    return E_FAIL;
                }           
        
                // show that the object is now active
                if (!fError) 
                {
                    _pPackage->_pIOleClientSite->ShowObject();
                    _pPackage->_pIOleClientSite->OnShowWindow(TRUE);
                }
                return fError ? E_FAIL : S_OK;

            case CMDLINK: 
                {
                    TCHAR szPath[MAX_PATH];
                    TCHAR szArgs[CBCMDLINKMAX-MAX_PATH];

                    lstrcpy(szPath, _pPackage->_pCml->szCommandLine);
                    PathSeparateArgs(szPath, szArgs);

                    sheinf.fMask  = SEE_MASK_NOCLOSEPROCESS;
                    sheinf.lpFile = szPath;
                    sheinf.lpParameters = szArgs;   
                    sheinf.nShow  = SW_SHOWNORMAL;

                    // NOTE: This code is much nicer than ShellExec-ing an embedded
                    // file.  Here, we just need to ShellExec the command line and
                    // the we're done.  We don't need to know when that process
                    // finishes or anything else.
                
                    return ShellExecuteEx(&sheinf) ? S_OK : E_FAIL;
                }
                break;
        }
    }
    else
    {
        // Let's see if it is a context menu verb:
        HRESULT hr;
        IContextMenu* pcm;
        if (SUCCEEDED(hr = _pPackage->GetContextMenu(&pcm)))
        {
            HMENU hmenu = CreatePopupMenu();
            if (NULL != hmenu)
            {
                if (SUCCEEDED(hr = pcm->QueryContextMenu(hmenu,
                                                         0,
                                                         OLEIVERB_FIRST_CONTEXT,
                                                         OLEIVERB_LAST_CONTEXT,
                                                         CMF_NORMAL)))
                {
                    MENUITEMINFO mii;
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID;
                    if (GetMenuItemInfo(hmenu, (UINT) (iVerb - OLEIVERB_FIRST_CONTEXT), TRUE, &mii))
                    {
                        if (PEMBED == _pPackage->_panetype)
                        {
                            // If we have an embedding, we have to make sure that
                            // the temp file is created before we execute a command:
                            hr =_pPackage->CreateTempFile();
                        }
                        if (SUCCEEDED(hr))
                        {
                            CMINVOKECOMMANDINFO ici;
                            ici.cbSize = sizeof(ici);
                            ici.fMask = 0;
                            ici.hwnd = NULL;
                            ici.lpVerb = (LPCSTR) (mii.wID - OLEIVERB_FIRST_CONTEXT);
                            ici.lpParameters = NULL;
                            ici.lpDirectory = NULL;
                            ici.nShow = SW_SHOWNORMAL;
                            // REVIEW: should we return OLEOBJ_S_CANNOT_DOVERB_NOW if this fails?
                            hr = pcm->InvokeCommand(&ici);
                        }
                    }
                    else
                    {
                        hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
                    }
                }
                DestroyMenu(hmenu);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            pcm->Release();
        }
        return hr;
    }
    return E_FAIL;
}
    
HRESULT CPackage_IOleObject::EnumVerbs(LPENUMOLEVERB *ppEnumOleVerb)
{
    DebugMsg(DM_TRACE, "pack oo - EnumVerbs() called.");
    HRESULT hr;
    
    IContextMenu* pcm;
     // tell the package to release the cached context menu:
    _pPackage->ReleaseContextMenu();
    if (SUCCEEDED(hr = _pPackage->GetContextMenu(&pcm)))
    {
        HMENU hmenu = CreatePopupMenu();
        if (NULL != hmenu)
        {
            if (SUCCEEDED(hr = pcm->QueryContextMenu(hmenu,
                                                     0,
                                                     OLEIVERB_FIRST_CONTEXT,
                                                     OLEIVERB_LAST_CONTEXT,
                                                     CMF_NORMAL)))
            {
                // BUGBUG: remove problematic items by canonical names
                int nItems = GetMenuItemCount(hmenu);
                int cOleVerbs = 0;
                if (nItems > 0)
                {
                    // NOTE: we allocate nItems, but we may not use all of them
                    // BUGBUG: add in menu items from the registry: "activate" & "edit"
                    OLEVERB* pVerbs = new OLEVERB[nItems];
                    if (NULL != pVerbs)
                    {
                        MENUITEMINFO mii;
                        TCHAR szMenuName[MAX_PATH];
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE | MIIM_ID;
                        for (int i = 0; i < nItems; i++)
                        {
                            mii.dwTypeData = szMenuName;
                            mii.cch = ARRAYSIZE(szMenuName);
                            // NOTE: use GetMenuState() to avoid converting flags:
                            DWORD dwState = GetMenuState(hmenu, i, MF_BYPOSITION);
                            if (0 == (dwState & (MF_BITMAP | MF_OWNERDRAW | MF_POPUP)))
                            {
                                if (GetMenuItemInfo(hmenu, i, TRUE, &mii) && (MFT_STRING == mii.fType))
                                {
                                    TCHAR szVerb[MAX_PATH];
                                    if (FAILED(pcm->GetCommandString(mii.wID - OLEIVERB_FIRST_CONTEXT,
                                                                     GCS_VERB,
                                                                     NULL,
                                                                     (LPSTR) szVerb,
                                                                     ARRAYSIZE(szVerb))))
                                    {
                                        // Some commands don't have canonical names - just
                                        // set the verb string to empty
                                        szVerb[0] = TEXT('\0');
                                    }
                                    if ((0 != lstrcmp(szVerb, TEXT("cut"))) &&
                                        (0 != lstrcmp(szVerb, TEXT("delete"))))
                                    {
                                        // In the first design, the context menu ID was used as
                                        // the lVerb - however MFC apps only give us a range of
                                        // 16 ID's and context menu ID's are often over 100
                                        // (they aren't contiguous)
                                        // Instead, we use the menu position plus the verb offset
                                        pVerbs[cOleVerbs].lVerb = (LONG) OLEIVERB_FIRST_CONTEXT + i;
                                        int cchMenu = lstrlen(mii.dwTypeData) + 1;
                                        if (NULL != (pVerbs[cOleVerbs].lpszVerbName = new WCHAR[cchMenu]))
                                        {
                                            SHTCharToUnicode(mii.dwTypeData, pVerbs[cOleVerbs].lpszVerbName, cchMenu);
                                        }
                                        pVerbs[cOleVerbs].fuFlags = dwState;
                                        pVerbs[cOleVerbs].grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;
                                        DebugMsg(DM_TRACE, "  Adding verb: id==%d,name=%s,verb=%s",mii.wID,mii.dwTypeData,szVerb);
                                        cOleVerbs++;
                                    }
                                }
                            }
                        }
                        if (SUCCEEDED(hr = _pPackage->InitVerbEnum(pVerbs, cOleVerbs)))
                        {
                            hr = _pPackage->QueryInterface(IID_IEnumOLEVERB, (void**) ppEnumOleVerb);
                        }
                        else
                        {
                            delete pVerbs;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = OLEOBJ_E_NOVERBS;
                }
            }
            DestroyMenu(hmenu);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pcm->Release();
    }

    return hr; // OleRegEnumVerbs(CLSID_CPackage, ppEnumOleVerb);
}

HRESULT CPackage_IOleObject::Update(void)
{
    return S_OK;
}

    
HRESULT CPackage_IOleObject::IsUpToDate(void)
{
    return S_OK;
}

    
HRESULT CPackage_IOleObject::GetUserClassID(LPCLSID pClsid)
{
    *pClsid = CLSID_CPackage;
    return S_OK;
}

    
HRESULT CPackage_IOleObject::GetUserType(DWORD dwFromOfType, LPOLESTR *pszUserType)
{
    return OleRegGetUserType(CLSID_CPackage, dwFromOfType, pszUserType);
}

    
HRESULT CPackage_IOleObject::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    return E_FAIL;
}

    
HRESULT CPackage_IOleObject::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    return _pPackage->_pIViewObject2->GetExtent(dwDrawAspect,-1,NULL,psizel);
}

    
HRESULT CPackage_IOleObject::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    if (_pPackage->_pIOleAdviseHolder == NULL) 
    {
        HRESULT hr = CreateOleAdviseHolder(&_pPackage->_pIOleAdviseHolder);
        if (FAILED(hr))
            return hr;
    }
    return _pPackage->_pIOleAdviseHolder->Advise(pAdvSink, pdwConnection);
}

    
HRESULT CPackage_IOleObject::Unadvise(DWORD dwConnection)
{
    DebugMsg(DM_TRACE, "pack oo - Unadvise() called.");
    
    if (_pPackage->_pIOleAdviseHolder != NULL)
        return _pPackage->_pIOleAdviseHolder->Unadvise(dwConnection);
    
    return E_FAIL;
}

    
HRESULT CPackage_IOleObject::EnumAdvise(LPENUMSTATDATA *ppenumAdvise)
{
    DebugMsg(DM_TRACE, "pack oo - EnumAdvise() called.");
    
    if (_pPackage->_pIOleAdviseHolder != NULL)
        return _pPackage->_pIOleAdviseHolder->EnumAdvise(ppenumAdvise);
    
    return E_FAIL;
}

    
HRESULT CPackage_IOleObject::GetMiscStatus(DWORD dwAspect, LPDWORD pdwStatus)
{
    return OleRegGetMiscStatus(CLSID_CPackage, dwAspect, pdwStatus);
}


HRESULT CPackage_IOleObject::SetColorScheme(LPLOGPALETTE pLogpal)
{
    return E_NOTIMPL;
}


DWORD CALLBACK MainWaitOnChildThreadProc(void *lpv)
{
    INT ret;
    MAINWAITONCHILD *pmwoc = (MAINWAITONCHILD *)lpv;
    CPackage *pPack = pmwoc->pObj->_pPackage;
    
    DebugMsg(DM_TRACE, "pack oo - MainWaitOnChildThreadProc() called.");
    DebugMsg(DM_TRACE, "            handle = %d",(DWORD)lpv);
    
    ret = WaitForSingleObject(pmwoc->h, INFINITE);
    DebugMsg(DM_TRACE,"WaitForSingObject exits...ret==%d",ret);
    
    if (ret == -1)
        DebugMsg(DM_TRACE,"GetLastError==%d",GetLastError());
      
    CloseHandle(pmwoc->h);

    // this will set our dirty flag...
    if (FAILED(pPack->EmbedInitFromFile(pPack->_pEmbed->pszTempName,FALSE)))
    {
        ShellMessageBox(g_hinst,
                        NULL,
                        MAKEINTRESOURCE(IDS_UPDATE_ERROR),
                        MAKEINTRESOURCE(IDS_APP_TITLE),
                        MB_ICONERROR | MB_TASKMODAL | MB_OK);
    }

    pPack->_pIOleClientSite->SaveObject();
    pPack->_pIOleAdviseHolder->SendOnSave();
    
    if (pPack->_pIOleAdviseHolder)
        pPack->_pIOleAdviseHolder->SendOnClose();
    
    if (pPack->_pIOleClientSite)
        pPack->_pIOleClientSite->OnShowWindow(FALSE);
    
    pPack->_pEmbed->hTask = NULL;
    
    // NOTE: we should probably pop up some sort of message box here if we 
    // can't reinit from the temp file.
    DebugMsg(DM_TRACE, "            MainWaitOnChildThreadProc exiting.");
    
    delete pmwoc;
    return 0;
}

BOOL CALLBACK GetTaskWndProc(HWND hwnd, DWORD lParam)
{
    DebugMsg(DM_TRACE, "pack oo - GetTaskWndProc() called.");
    
    if (IsWindowVisible(hwnd))
    {
        g_hTaskWnd = hwnd;
        return FALSE;
    }

    return TRUE;
}
