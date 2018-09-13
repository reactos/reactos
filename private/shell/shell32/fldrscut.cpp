//   Implements Folder Shortcut.

#include "shellprv.h"
#include "clsobj.h"
#define DESKTOP 1   //this determines the fscut storage style.

#define FOLDERSHORTCUT_GUID         TEXT("{0AFACED1-E828-11D1-9187-B532F1E9575D}")

BOOL CreateFolderDesktopIni(LPCTSTR pszName)
{
#ifdef DESKTOP
    SHFOLDERCUSTOMSETTINGS fcs = {0};
    fcs.dwSize = sizeof(fcs);
    fcs.dwMask = FCSM_CLSID | FCSM_FLAGS;
    fcs.pclsid = (GUID*)&CLSID_FolderShortcut;
    fcs.dwFlags = FCS_FLAG_DRAGDROP;

    return SUCCEEDED(SHGetSetFolderCustomSettings(&fcs, pszName, FCS_FORCEWRITE));
#else
    return TRUE;
#endif
}

EXTERN_C BOOL IsFolderShortcut(LPCTSTR pszName)
{
#ifdef DESKTOP
    SHFOLDERCUSTOMSETTINGS fcs = {0};
    CLSID clsid = {0};
    fcs.dwSize = sizeof(fcs);
    fcs.dwMask = FCSM_CLSID;
    fcs.pclsid = &clsid;

    if (SUCCEEDED(SHGetSetFolderCustomSettings(&fcs, pszName, FCS_READ)))
    {
        return IsEqualGUID(clsid, CLSID_FolderShortcut);
    }
    return FALSE;
#else
    return lstrcmpi(PathFindExtension(pszName), TEXT(".") FOLDERSHORTCUT_GUID) == 0;
#endif
}


// exported from fsnotify.c
STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);


class CFolderShortcut : public IShellFolder2, 
                        public IPersistFolder3,
                        public IShellLinkA,
                        public IShellLinkW,
                        public IPersistFile,
                        public IExtractIcon,
                        public IQueryInfo,
                        public IFolderShortcutConvert,
                        public IPersistStreamInit,
                        public IPersistPropertyBag,
                        public IBrowserFrameOptions
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                  ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags,
                           LPITEMIDLIST *ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IPersistFolder3
    STDMETHODIMP InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *pfti);
    STDMETHODIMP GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *pfti);

    // IPersistStream
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,int fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize);

    // IPersistPropertyBag
    STDMETHODIMP Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    STDMETHODIMP Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog);

    // IPersistPropertyBag/IPersistStreamInit
    STDMETHODIMP InitNew(void);

    // IPersistFile
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP IsDirty() { return E_NOTIMPL; };
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName) { return E_NOTIMPL; };
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

    // IShellLinkW
    STDMETHODIMP GetPath(LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD flags);
    STDMETHODIMP SetPath(LPCWSTR pszFile);
    STDMETHODIMP GetIDList(LPITEMIDLIST *ppidl);
    STDMETHODIMP SetIDList(LPCITEMIDLIST pidl);
    STDMETHODIMP GetDescription(LPWSTR pszName, int cchMaxName);
    STDMETHODIMP SetDescription(LPCWSTR pszName);
    STDMETHODIMP GetWorkingDirectory(LPWSTR pszDir, int cchMaxPath);
    STDMETHODIMP SetWorkingDirectory(LPCWSTR pszDir);
    STDMETHODIMP GetArguments(LPWSTR pszArgs, int cchMaxPath);
    STDMETHODIMP SetArguments(LPCWSTR pszArgs);
    STDMETHODIMP GetHotkey(WORD *pwHotkey);
    STDMETHODIMP SetHotkey(WORD wHotkey);
    STDMETHODIMP GetShowCmd(int *piShowCmd);
    STDMETHODIMP SetShowCmd(int iShowCmd);
    STDMETHODIMP GetIconLocation(LPWSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHODIMP SetIconLocation(LPCWSTR pszIconPath, int iIcon);
    STDMETHODIMP Resolve(HWND hwnd, DWORD fFlags);
    STDMETHODIMP SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved);

    // IShellLinkA
    STDMETHODIMP GetPath(LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD flags);
    STDMETHODIMP SetPath(LPCSTR pszFile);
    STDMETHODIMP GetDescription(LPSTR pszName, int cchMaxName);
    STDMETHODIMP SetDescription(LPCSTR pszName);
    STDMETHODIMP GetWorkingDirectory(LPSTR pszDir, int cchMaxPath);
    STDMETHODIMP SetWorkingDirectory(LPCSTR pszDir);
    STDMETHODIMP GetArguments(LPSTR pszArgs, int cchMaxPath);
    STDMETHODIMP SetArguments(LPCSTR pszArgs);
    STDMETHODIMP GetIconLocation(LPSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHODIMP SetIconLocation(LPCSTR pszIconPath, int iIcon);
    STDMETHODIMP SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved);

    // IFolderShortcutConvert
    STDMETHODIMP ConvertToLink(LPCOLESTR pszPathLNK, DWORD fFlags);
    STDMETHODIMP ConvertToFolderShortcut(LPCOLESTR pszPathLNK, DWORD fFlags);

    // IExtractIcon
    STDMETHODIMP GetIconLocation(UINT uFlags, LPTSTR pszIconFile, UINT ucchMax, INT *pniIcon, UINT *puFlags);
    STDMETHODIMP Extract(LPCTSTR pcszFile, UINT uIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT ucIconSize);

    // IQueryInfo
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR** ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

    // IBrowserFrameOptions
    STDMETHODIMP GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, IN BROWSERFRAMEOPTIONS * pdwOptions);

    CFolderShortcut();

protected:
    ~CFolderShortcut();

    void _ClearState();
    void _ClearTargetFolder();

private:
    HRESULT _LoadShortcut();
    HRESULT _GetTargetIDList(BOOL fResolve);
    HRESULT _BindFolder(BOOL fResolve);
    HRESULT _GetFolder(BOOL fForceResolve);
    HRESULT _GetFolder2();

    HRESULT _GetLink();
    HRESULT _GetLinkA();
    HRESULT _GetLinkQI(REFIID riid, void **ppv);
    HRESULT _PreBindCtxHelper(IBindCtx **ppbc);

    LONG                   _cRef;      

    LPITEMIDLIST           _pidlRoot;
    LPITEMIDLIST           _pidlTarget;
    LPITEMIDLIST           _pidlTargetFldrFromInit;
    IShellFolder*          _psfTarget;
    IShellFolder2*         _psf2Target;
    IShellLinkW*           _pslTarget;
    IShellLinkA*           _pslTargetA;
    LPTSTR                 _pszLastSave;
    BOOL                   _fHaveResolved;
    TCHAR                  _szFolderPath[MAX_PATH];
};

//constructor/destructor and related functions
CFolderShortcut::CFolderShortcut() : _cRef(1)
{
    ASSERT(_pidlRoot == NULL);
    ASSERT(_pidlTarget == NULL);
    ASSERT(_psfTarget == NULL);
    ASSERT(_psf2Target == NULL);
    ASSERT(_szFolderPath[0] == 0);
    ASSERT(_pidlTargetFldrFromInit == NULL);

    DllAddRef();
}

CFolderShortcut::~CFolderShortcut()
{
    _ClearState();
    DllRelease();
}

void CFolderShortcut::_ClearTargetFolder()
{
    ATOMICRELEASE(_psfTarget);
    ATOMICRELEASE(_psf2Target);
}

void CFolderShortcut::_ClearState()
{
    _fHaveResolved = FALSE;

    Pidl_Set(&_pidlRoot, NULL);
    Pidl_Set(&_pidlTarget, NULL);
    Pidl_Set(&_pidlTargetFldrFromInit, NULL);

    Str_SetPtr(&_pszLastSave, NULL);

    _ClearTargetFolder();

    ATOMICRELEASE(_pslTarget);
    ATOMICRELEASE(_pslTargetA);
}

STDAPI CFolderShortcut_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;
    // aggregation checking is handled in class factory
    CFolderShortcut* pfolder = new CFolderShortcut();
    if (pfolder)
    {
        hr = pfolder->QueryInterface(riid, ppv);
        pfolder->Release();
    }

    return hr;
}

// ensure that _pslTarget has been created and loaded

HRESULT CFolderShortcut::_LoadShortcut()
{
    HRESULT hr;

    if (_pslTarget)
    {
        hr = S_OK;
    }
    else if (_szFolderPath[0])
    {
        TCHAR szPath[MAX_PATH];

        // leave this shortcut visible so down level clients see it and can
        // navigate through it.
        PathCombine(szPath, _szFolderPath, TEXT("target.lnk"));
        hr = LoadFromFile(&CLSID_ShellLink, szPath, IID_PPV_ARG(IShellLinkW, &_pslTarget));
    }
    else
        hr = E_FAIL;

    return hr;
}

// ensure that _pidlTarget is inited (requres _pslTarget)

HRESULT CFolderShortcut::_GetTargetIDList(BOOL bResolve)
{
    HRESULT hr = _LoadShortcut();
    if (SUCCEEDED(hr))
    {
        if (_pidlTarget)
        {
            hr = S_OK;
        }
        else
        {
            if (bResolve)
                _pslTarget->Resolve(NULL, SLR_UPDATE | SLR_NO_UI);

            hr = _pslTarget->GetIDList(&_pidlTarget);
            if (hr == S_FALSE)
                hr = E_FAIL;      // convert empty to failure

            if (SUCCEEDED(hr))
            {
                //  make sure we dont have another shortcut here
                IShellLink *psl;
                if (SUCCEEDED(SHBindToObject(NULL, IID_IShellLink, _pidlTarget, (void**)&psl)))
                {
                    ILFree(_pidlTarget);
                    hr = psl->GetIDList(&_pidlTarget);

                    if (SUCCEEDED(hr))
                    {
                        hr = _pslTarget->SetIDList(_pidlTarget);
                    }
                        
                    psl->Release();
                }
            }

            if (FAILED(hr) && _pidlTarget)
            {
                ILFree(_pidlTarget);
                _pidlTarget = NULL;
            }
        }
    }
    return hr;
}

// create _psfTarget (requires _pidlTarget)

HRESULT CFolderShortcut::_BindFolder(BOOL bResolve)
{
    ASSERT(_psfTarget == NULL);

    HRESULT hr = _GetTargetIDList(bResolve);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc = NULL;   // in/out param below
        hr = _PreBindCtxHelper(&pbc);    // avoid loops in the name space
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr))
            {
                IShellFolder *psfDesktop;
                hr = SHGetDesktopFolder(&psfDesktop);
                if (SUCCEEDED(hr))
                {
                    // Are we trying to bind to the desktop folder?
                    if (ILIsEmpty(_pidlTarget))
                    {
                        // Yes; Clone the desktop shell folder.
                        _psfTarget = psfDesktop;
                        _psfTarget->AddRef();
                        hr = S_OK;
                    }
                    else
                    {
                        // No. Bind to it.
                        hr = psfDesktop->BindToObject(_pidlTarget, pbc, IID_PPV_ARG(IShellFolder, &_psfTarget));
                    }

                    if (SUCCEEDED(hr))
                    {
                        // optionally re-target the folder (if he is a file system folder) 
                        // to separate the location in the name space (_pidlRoot) 
                        // and the folder being viewed (pfsfi.szFolderPath).

                        IPersistFolder3 *ppf;
                        if (SUCCEEDED(_psfTarget->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf))))
                        {
                            PERSIST_FOLDER_TARGET_INFO pfti = { 0 };

                            pfti.pidlTargetFolder = _pidlTarget;
                            pfti.dwAttributes = -1;
                            pfti.csidl = -1;

                            hr = ppf->InitializeEx(pbc, _pidlRoot, &pfti);
                            ppf->Release();
                        }
                    }
                    psfDesktop->Release();
                }
            }
            pbc->Release();
        }
    }
    return hr;
}

// ensure that _psfTarget is inited

HRESULT CFolderShortcut::_GetFolder(BOOL fForceResolve)
{
    HRESULT hr;

    if (fForceResolve)
    {
        if (_fHaveResolved)
        {
            hr = _psfTarget ? S_OK : E_FAIL;
        }
        else
        {
            _fHaveResolved = TRUE;  // don't do this again

            _ClearTargetFolder();
            Pidl_Set(&_pidlTarget, NULL);

            hr = _BindFolder(fForceResolve);
        }
    }
    else if (_psfTarget)
    {
        hr = S_OK;
    }
    else
    {
        hr = _BindFolder(fForceResolve);
    }
    return hr;
}

// ensure that _psf2Target is inited

HRESULT CFolderShortcut::_GetFolder2()
{
    if (_psf2Target)
        return S_OK;

    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf2Target));
    return hr;
}

STDMETHODIMP CFolderShortcut::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CFolderShortcut, IShellFolder, IShellFolder2),
        QITABENT(CFolderShortcut, IShellFolder2),
        QITABENTMULTI(CFolderShortcut, IPersist, IPersistFolder3),
        QITABENTMULTI(CFolderShortcut, IPersistFolder, IPersistFolder3),
        QITABENTMULTI(CFolderShortcut, IPersistFolder2, IPersistFolder3),
        QITABENT(CFolderShortcut, IPersistFolder3),
        QITABENT(CFolderShortcut, IPersistStreamInit),
        QITABENTMULTI(CFolderShortcut, IPersistStream, IPersistStreamInit),
        QITABENT(CFolderShortcut, IShellLinkA),
        QITABENT(CFolderShortcut, IShellLinkW),
        QITABENT(CFolderShortcut, IPersistFile),
        QITABENT(CFolderShortcut, IFolderShortcutConvert),
        QITABENT(CFolderShortcut, IExtractIcon),
        QITABENT(CFolderShortcut, IQueryInfo),
        QITABENT(CFolderShortcut, IPersistPropertyBag),
        QITABENT(CFolderShortcut, IBrowserFrameOptions),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFolderShortcut::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderShortcut::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

// either create or init the passed bind ctx with the params to avoid loops in the name space

HRESULT CFolderShortcut::_PreBindCtxHelper(IBindCtx **ppbc)
{
    HRESULT hr;
    if (*ppbc)
    {
        (*ppbc)->AddRef();
        hr = S_OK;
    }
    else
         hr = CreateBindCtx(0, ppbc);

    if (SUCCEEDED(hr)) 
        (*ppbc)->RegisterObjectParam(STR_SKIP_BINDING_CLSID, SAFECAST(this, IShellFolder2 *));

    return hr;
}

// IShellFolder methods

HRESULT CFolderShortcut::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pwszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
    {
        hr = _PreBindCtxHelper(&pbc);
        if (SUCCEEDED(hr))
        {
            hr = _psfTarget->ParseDisplayName(hwnd, pbc, pwszDisplayName, 
                                                pchEaten, ppidl, pdwAttributes);
            pbc->Release();
        }
    }
    return hr;
}

HRESULT CFolderShortcut::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    HRESULT hr = _GetFolder(TRUE);

    if (SUCCEEDED(hr))
        hr = _psfTarget->EnumObjects(hwnd, grfFlags, ppenumIDList);
    if (SUCCEEDED(hr))
        SHChangeNotifyRegisterAlias(_pidlTarget, _pidlRoot);
    
    return hr;
}

HRESULT CFolderShortcut::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder(TRUE);
    if (SUCCEEDED(hr))
    {
        hr = _PreBindCtxHelper(&pbc);
        if (SUCCEEDED(hr))
        {
            hr = _psfTarget->BindToObject(pidl, pbc, riid, ppv);
            pbc->Release();

            if (SUCCEEDED(hr))
                SHChangeNotifyRegisterAlias(_pidlTarget, _pidlRoot);
        }
    }
    return hr;
}

HRESULT CFolderShortcut::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder(TRUE);
    if (SUCCEEDED(hr))
    {
        hr = _PreBindCtxHelper(&pbc);
        if (SUCCEEDED(hr))
        {
            hr = _psfTarget->BindToStorage(pidl, pbc, riid, ppv);
            pbc->Release();
        }
    }
    return hr;
}

HRESULT CFolderShortcut::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->CompareIDs(lParam, pidl1, pidl2);
    return hr;
}

HRESULT CFolderShortcut::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = _GetFolder(TRUE);

    if ( SUCCEEDED(hr) )
        hr = _psfTarget->CreateViewObject(hwnd, riid, ppv);

    if ( SUCCEEDED(hr) && (IsEqualIID(riid, IID_IShellView) || IsEqualIID(riid, IID_IShellView2)) )
        SHChangeNotifyRegisterAlias(_pidlTarget, _pidlRoot);

    return hr;
}

HRESULT CFolderShortcut::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    if (IsSelf (cidl, apidl))
    {
        if (SHGetAppCompatFlags (ACF_FOLDERSCUTASLINK) & ACF_FOLDERSCUTASLINK)
        {
            *rgfInOut = SFGAO_LINK | SFGAO_CAPABILITYMASK | SFGAO_FILESYSTEM;
        }
        else
        {
            *rgfInOut = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_CANMONIKER |
                          SFGAO_LINK | SFGAO_DROPTARGET | SFGAO_CANRENAME | SFGAO_CANDELETE |
                          SFGAO_CANLINK | SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_HASSUBFOLDER;
        }
        return S_OK;
    }

    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->GetAttributesOf(cidl, apidl, rgfInOut);
    return hr;
}

HRESULT CFolderShortcut::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                       REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    return hr;
}

HRESULT CFolderShortcut::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName)
{
    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->GetDisplayNameOf(pidl, uFlags, pName);
    return hr;
}

HRESULT CFolderShortcut::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
                                   LPCOLESTR pszName, DWORD uFlags,
                                   LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = _GetFolder(FALSE);
    if (SUCCEEDED(hr))
        hr = _psfTarget->SetNameOf(hwnd, pidl, pszName, uFlags, ppidlOut);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDefaultSearchGUID(LPGUID lpGuid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->GetDefaultSearchGUID(lpGuid);
    return hr;
}

STDMETHODIMP CFolderShortcut::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->EnumSearches(ppenum);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->GetDefaultColumn(dwRes, pSort, pDisplay);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{    
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->GetDefaultColumnState(iColumn, pbState);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->GetDetailsEx(pidl, pscid, pv);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetail)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->GetDetailsOf(pidl, iColumn, pDetail);
    return hr;
}

STDMETHODIMP CFolderShortcut::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    HRESULT hr = _GetFolder2();
    if (SUCCEEDED(hr))
        hr = _psf2Target->MapColumnToSCID(iColumn, pscid);
    return hr;
}

// IPersist
HRESULT CFolderShortcut::GetClassID(CLSID *pCLSID)
{
    *pCLSID = CLSID_FolderShortcut;
    return S_OK;
}

// IPersistFolder
HRESULT CFolderShortcut::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hr;

    // is the link loaded (could have been loaded through IPersistStream::Load)?
    if (_pslTarget)
    {
        // Yes, it's loaded so re-initialize
        // note, _szFolderPath will be empty since we are not loaded from the file system

        hr = Pidl_Set(&_pidlRoot, pidl) ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        // we explictly require initialization through 
        // IPersistFolder3::InitializeEx, if we don't do these we can
        // not defent against loops in the name space
        hr = E_FAIL;
    }

    return hr;
}

// IPersistFolder2
STDMETHODIMP CFolderShortcut::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(this->_pidlRoot, ppidl);
}

// IPersistFolder3
STDMETHODIMP CFolderShortcut::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hr = E_INVALIDARG;  // assume failure

    if ( NULL == pbc || (pbc && !SHSkipJunction(pbc, &CLSID_FolderShortcut)) )
    {
        _ClearState();

        if (pidlRoot)
            hr = SHILClone(pidlRoot, &_pidlRoot);

        if (pfti)
        {
            if ( SUCCEEDED(hr) )
                hr = SHILClone(pfti->pidlTargetFolder, &_pidlTargetFldrFromInit);

            if ( SUCCEEDED(hr) && !_szFolderPath[0] )
                hr = SHGetPathFromIDList(pfti->pidlTargetFolder, _szFolderPath);
        }
        else
        {
            if ( SUCCEEDED(hr) && !_szFolderPath[0] )
                hr = SHGetPathFromIDList(_pidlRoot, _szFolderPath);
        }

        if ( SUCCEEDED(hr) )
            hr = _LoadShortcut();
    }
    return hr;
}

HRESULT CFolderShortcut::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hr = S_OK;

    ZeroMemory(pfti, sizeof(*pfti)); 

    if ( _pidlTargetFldrFromInit )
        hr = SHILClone(_pidlTargetFldrFromInit, &pfti->pidlTargetFolder);

    pfti->dwAttributes = -1;
    pfti->csidl = -1;
    return hr;
}

HRESULT CFolderShortcut::_GetLink()
{
    HRESULT hr = _LoadShortcut();
    if (FAILED(hr))
    {
        // get an empty one in case we are going to be asked to save
        hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLinkW, &_pslTarget));
    }
    return hr;
}

HRESULT CFolderShortcut::_GetLinkQI(REFIID riid, void **ppv)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->QueryInterface(riid, ppv);
    return hr;
}

HRESULT CFolderShortcut::_GetLinkA()
{
    return _pslTargetA ? S_OK : _GetLinkQI(IID_PPV_ARG(IShellLinkA, &_pslTargetA));
}

// IPersistFile
STDMETHODIMP CFolderShortcut::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    _ClearState();

    SHUnicodeToTChar(pszFileName, _szFolderPath, ARRAYSIZE(_szFolderPath));
    return _LoadShortcut();
}

BOOL _IsFolder(LPCITEMIDLIST pidl)
{
    ULONG rgInfo = SFGAO_FOLDER;
    HRESULT hr = SHGetNameAndFlags(pidl, SHGDN_NORMAL, NULL, 0, &rgInfo);
    return SUCCEEDED(hr) && (rgInfo & SFGAO_FOLDER);
}

void PathStripTrailingDots(LPTSTR szPath)
{
    if (szPath[0] == TEXT('\0'))
        return;

    LPTSTR psz = &szPath[lstrlen(szPath) - 1];

    while ((*psz == TEXT('.')) && 
           (psz >= szPath))
    {
        *psz-- = TEXT('\0');
    }

}

STDMETHODIMP CFolderShortcut::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    HRESULT hr = _GetTargetIDList(FALSE);

    // We need to make sure the folder shortcut can be saved keeping in mind the MAX_PATH limitation
    // cchFSReserved is the number of characters to reserve for the largest file that will be created
    // in the foldershortcut directory, in this case, it is the ARRAYSIZE of "\\desktop.ini"
    static const int cchFSReserved = ARRAYSIZE(TEXT("\\desktop.ini")); 

    LPITEMIDLIST pidlInternet;

    // Don't create a folder shortcut to the internet folder.
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_INTERNET, &pidlInternet)))
    {
        if (ILIsEqual(_pidlTarget, pidlInternet))
        {
            hr = E_INVALIDARG;
        }
        ILFree(pidlInternet);
    }

    if (SUCCEEDED(hr) && _IsFolder(_pidlTarget))
    {
        // we know the target is a folder, create a folder shortcut.
        BOOL fCreatedDir;
        TCHAR szName[MAX_PATH];

        SHUnicodeToTChar(pszFileName, szName, ARRAYSIZE(szName));

        // Remove any exisiting extension. 
        // We dont want "Shortcut To My Documents.lnk.{GUID}
#ifndef DESKTOP  
        PathRenameExtension(szName, TEXT(".") FOLDERSHORTCUT_GUID);
#else
        PathRemoveExtension(szName);
        PathStripTrailingDots(szName);
#endif
        // Can't create a fldrshcut with too long a path
        if ((MAX_PATH - cchFSReserved) < lstrlen(szName))
        {
            hr = CO_E_PATHTOOLONG;
        }
        
        if (SUCCEEDED(hr))
        {
            if (PathIsDirectory(szName))
                fCreatedDir = FALSE;
            else
                fCreatedDir = SHCreateDirectory(NULL, szName) == 0;

            CreateFolderDesktopIni(szName);

            // Now initialize the child link
            IPersistFile *ppf;
            hr = _pslTarget->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                WCHAR wszName[MAX_PATH];
                SHTCharToUnicode(szName, wszName, ARRAYSIZE(wszName));

                PathAppendW(wszName, L"target.lnk");

                hr = ppf->Save(wszName, fRemember);
                if (SUCCEEDED(hr))
                {
                    if (fRemember)
                        Str_SetPtr(&_pszLastSave, szName);
                }

                ppf->Release();
            }

            if (FAILED(hr) && fCreatedDir) 
            {
                RemoveDirectory(szName);    // cleanup after ourselves.
            }
        }
    }
    else
    {
        // the target is not a folder, create a normal shortcut in this case
        IPersistFile *ppf;
        hr = _pslTarget->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->Save(pszFileName, fRemember);
            ppf->Release();
        }
    }
    return hr;
}

STDMETHODIMP CFolderShortcut::GetCurFile(LPOLESTR *ppszFileName)
{
    HRESULT hr = E_FAIL;
    if (_pszLastSave)
        hr = SHStrDup(_pszLastSave, ppszFileName);
    else if (_pslTarget)
    {
        IPersistFile *ppf;
        hr = _pslTarget->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->GetCurFile(ppszFileName);
            ppf->Release();
        }
    }
    return hr;
}

// IShellLinkW

STDMETHODIMP CFolderShortcut::GetPath(LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD flags)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetPath(pszFile, cchMaxPath, pfd, flags);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetPath(LPCWSTR pwszFile)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr) && PathIsDirectoryW(pwszFile))
    {
        hr = _pslTarget->SetPath(pwszFile);
        Pidl_Set(&_pidlTarget, NULL);
    }
    return hr;
}

STDMETHODIMP CFolderShortcut::GetIDList(LPITEMIDLIST *ppidl)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetIDList(ppidl);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetIDList(LPCITEMIDLIST pidl)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
    {
        hr = _pslTarget->SetIDList(pidl);
        Pidl_Set(&_pidlTarget, NULL);
    }
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDescription(LPWSTR wszName, int cchMaxName)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetDescription(wszName, cchMaxName);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetDescription(LPCWSTR wszName)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->SetDescription(wszName);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetWorkingDirectory(LPWSTR wszDir, int cchMaxPath)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetWorkingDirectory(wszDir, cchMaxPath);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetWorkingDirectory(LPCWSTR wszDir)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->SetWorkingDirectory(wszDir);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetArguments(LPWSTR wszArgs, int cchMaxPath)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetArguments(wszArgs, cchMaxPath);//this is probably not at all useful.
    return hr;
}

STDMETHODIMP CFolderShortcut::SetArguments(LPCWSTR wszArgs)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->SetArguments(wszArgs);//this is probably not at all useful.
    return hr;
}

STDMETHODIMP CFolderShortcut::GetHotkey(WORD *pwHotkey)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetHotkey(pwHotkey);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetHotkey(WORD wHotkey)
{
    HRESULT hr = _GetLink();
   if (SUCCEEDED(hr))
        hr = _pslTarget->SetHotkey(wHotkey);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetShowCmd(int *piShowCmd)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetShowCmd(piShowCmd);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetShowCmd(int iShowCmd)
{
    HRESULT hr = _GetLink();
   if (SUCCEEDED(hr))
        hr = _pslTarget->SetShowCmd(iShowCmd);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetIconLocation(LPWSTR wszIconPath, int cchIconPath, int *piIcon)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->GetIconLocation(wszIconPath, cchIconPath, piIcon);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetIconLocation(LPCWSTR wszIconPath, int iIcon)
{
    HRESULT hr = _GetLink();
    if  (SUCCEEDED(hr))
        hr = _pslTarget->SetIconLocation(wszIconPath, iIcon);

    return hr;
}

STDMETHODIMP CFolderShortcut::Resolve(HWND hwnd, DWORD fFlags)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->Resolve(hwnd, fFlags);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetRelativePath(LPCWSTR wszPathRel, DWORD dwReserved)
{
    HRESULT hr = _GetLink();
    if (SUCCEEDED(hr))
        hr = _pslTarget->SetRelativePath(wszPathRel, dwReserved);

    return hr;
}

// IShellLinkA
STDMETHODIMP CFolderShortcut::GetPath(LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD flags)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->GetPath(pszFile, cchMaxPath, pfd, flags);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetDescription(LPSTR pszName, int cchMaxName)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->GetDescription(pszName, cchMaxName);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetWorkingDirectory(LPSTR pszDir, int cchMaxPath)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->GetWorkingDirectory(pszDir, cchMaxPath);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetArguments(LPSTR pszArgs, int cchMaxPath)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->GetArguments(pszArgs, cchMaxPath);//this is probably not at all useful.
    return hr;
}

STDMETHODIMP CFolderShortcut::GetIconLocation(LPSTR pszIconPath, int cchIconPath, int *piIcon)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr)) 
       hr = _pslTargetA->GetIconLocation(pszIconPath, cchIconPath, piIcon);    
    return hr;
}

STDMETHODIMP CFolderShortcut::SetPath(LPCSTR pszFile)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr) && PathIsDirectoryA(pszFile))
    {
        hr = _pslTargetA->SetPath(pszFile);
        Pidl_Set(&_pidlTarget, NULL);
    }
    return hr;
}

STDMETHODIMP CFolderShortcut::SetDescription(LPCSTR pszName)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->SetDescription(pszName);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetWorkingDirectory(LPCSTR pszDir)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->SetWorkingDirectory(pszDir);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetArguments(LPCSTR pszArgs)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->SetArguments(pszArgs);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetIconLocation(LPCSTR pszIconPath, int iIcon)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))    
        hr = _pslTargetA->SetIconLocation(pszIconPath, iIcon);
    return hr;
}

STDMETHODIMP CFolderShortcut::SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved)
{
    HRESULT hr = _GetLinkA();
    if (SUCCEEDED(hr))
        hr = _pslTargetA->SetRelativePath(pszPathRel, dwReserved);
    return hr;
}

STDMETHODIMP CFolderShortcut::GetIconLocation(UINT uFlags, LPTSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags)
{
    IExtractIcon *pxi;
    HRESULT hr = _GetLinkQI(IID_PPV_ARG(IExtractIcon, &pxi));
    if (SUCCEEDED(hr))
    {
        hr = pxi->GetIconLocation(uFlags, pszIconFile, ucchMax, pniIcon, puFlags);
        pxi->Release();
    }
    return hr;
}

STDMETHODIMP CFolderShortcut::Extract(LPCTSTR pcszFile, UINT uIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT ucIconSize)
{
    IExtractIcon *pxi;
    HRESULT hr = _GetLinkQI(IID_PPV_ARG(IExtractIcon, &pxi));
    if (SUCCEEDED(hr))
    {
        hr = pxi->Extract(pcszFile, uIconIndex, phiconLarge, phiconSmall, ucIconSize);
        pxi->Release();
    }
    return hr;
}

HRESULT CFolderShortcut::GetInfoTip(DWORD dwFlags, WCHAR** ppwszText)
{
    IQueryInfo *pqi;
    HRESULT hr = _GetLinkQI(IID_PPV_ARG(IQueryInfo, &pqi));
    if (SUCCEEDED(hr))
    {
        hr = pqi->GetInfoTip(dwFlags | QITIPF_LINKUSETARGET, ppwszText);
        pqi->Release();
    }
    return hr;
}

HRESULT CFolderShortcut::GetInfoFlags(DWORD *pdwFlags)
{
    IQueryInfo *pqi;
    HRESULT hr = _GetLinkQI(IID_PPV_ARG(IQueryInfo, &pqi));
    if (SUCCEEDED(hr))
    {
        hr = pqi->GetInfoFlags(pdwFlags);
        pqi->Release();
    }
    return hr;
}


// IBrowserFrameOptions
HRESULT CFolderShortcut::GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, IN BROWSERFRAMEOPTIONS * pdwOptions)
{
    HRESULT hr = _GetFolder(TRUE);

    *pdwOptions = BFO_NONE;
    if (SUCCEEDED(hr))
    {
        IBrowserFrameOptions *pbfo;

        hr = _psfTarget->QueryInterface(IID_PPV_ARG(IBrowserFrameOptions, &pbfo));
        if (SUCCEEDED(hr))
        {
            hr = pbfo->GetFrameOptions(dwMask, pdwOptions);        
            pbfo->Release();
        }
    }
    
    return hr;
}


// IPersistStream
STDMETHODIMP CFolderShortcut::Load(IStream *pStm)
{
    _ClearState();

    IPersistStream *pps;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IPersistStream, &pps));
    if (SUCCEEDED(hr))
    {
        hr = pps->Load(pStm);
        if (SUCCEEDED(hr))
            pps->QueryInterface(IID_PPV_ARG(IShellLinkW, &_pslTarget));  // keep this guy
        pps->Release();
    }
    return hr;
}

// IPersistStream
STDMETHODIMP CFolderShortcut::Save(IStream *pStm, int fClearDirty)
{
    return E_NOTIMPL;
}

// IPersistStream
STDMETHODIMP CFolderShortcut::GetSizeMax(ULARGE_INTEGER * pcbSize)
{
    return E_NOTIMPL;
}

//
// IFolderShortcut::ConvertToLink.
//
// destructively convert a Folder Shortcut into a Shell Link.
//
//  pszFolderShortcut is the path to an existing folder shortcut
//  c:\Folder Shortcut.{guid}   - deleted
//  c:\Folder Shortcut.lnk      - created
//
STDMETHODIMP CFolderShortcut::ConvertToLink(LPCOLESTR pszFolderShortcut, DWORD fFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szName[MAX_PATH];

    SHUnicodeToTChar(pszFolderShortcut, szName, ARRAYSIZE(szName));

    if (PathIsDirectory(szName) && IsFolderShortcut(szName))
    {
        TCHAR szLinkName[MAX_PATH];

        // c:\Folder Shortcut\target.lnk 
        StrCpyN(szLinkName, szName, ARRAYSIZE(szLinkName));
        PathAppend(szLinkName, TEXT("target.lnk"));

        PathRenameExtension(szName, TEXT(".lnk"));

        // FS.lnk -> FS.{guid}
        CopyFile(szLinkName, szName, FALSE);

        PathRemoveExtension(szName);

        if (DeleteFile(szLinkName) && 
#ifdef DESKTOP  //delete the desktop.ini file and remove the directory
            PathAppend(szName, TEXT("desktop.ini")) &&
            DeleteFile(szName) &&
            PathRemoveFileSpec(szName) &&
            RemoveDirectory(szName))
#else           //remove the directory
            PathRemoveFileSpec(szLinkName) &&
            RemoveDirectory(szLinkName))
#endif
            hr = S_OK;
    }
    return hr;
}

//
// IFolderShortcut::ConvertToFolderShortcut.
//
// destructively convert a Shell Link (.lnk) -> Folder Shortcut (Folder.{guid}).
//  pszPathLNK is the path to an existing .lnk file
//  c:\Folder Shortcut.lnk      - deleted
//  c:\Folder Shortcut.{guid}   - created
//
STDMETHODIMP CFolderShortcut::ConvertToFolderShortcut(LPCOLESTR pszPathLNK, DWORD fFlags)
{
    //must bind to the link, resolve it, and make sure it points to a folder.
    IShellLink *psl;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hr))  
    {
        IPersistFile *ppf;
        hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->Load(pszPathLNK, STGM_READ);
            if (SUCCEEDED(hr))
            {
                hr = psl->Resolve(NULL, SLR_NO_UI); // make sure the link is real
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;

                    hr = psl->GetIDList(&pidl);
                    if (hr == S_OK)
                    {
                        // this should maybe work on the pidl so that 
                        // it doesn't have to worry about files.
                        if (_IsFolder(pidl))
                        {             
                            hr = E_FAIL;

                            TCHAR szPath[MAX_PATH], szName[MAX_PATH]; 
                            SHUnicodeToTChar(pszPathLNK, szName, ARRAYSIZE(szName));
                            StrCpyN(szPath, szName, ARRAYSIZE(szPath));
#ifndef DESKTOP
                            PathRenameExtension(szName, TEXT(".") FOLDERSHORTCUT_GUID);
#else
                            PathRemoveExtension(szName);
#endif
                            BOOL fCreatedDir = SHCreateDirectory(NULL, szName) == 0;

                            if (CreateFolderDesktopIni(szName) &&
                                PathAppend(szName, TEXT("target.lnk")))
                            {   
                                //copy the link file into the new directory.
                                if (CopyFile(szPath, szName, FALSE))
                                {
                                    if (DeleteFile(szPath)) //if all goes well, delete the old.
                                        hr = S_OK;
                                }
                                else
                                {
                                    PathRemoveFileSpec(szName);
                                    if (fCreatedDir)
                                        RemoveDirectory(szName);
                                }
                            }
                        }
                        else
                            hr = E_FAIL;
                        ILFree(pidl);
                    }
                    else
                        hr = E_FAIL;
                }
            }
            ppf->Release();
        }
        psl->Release();
    }
    
    return hr;
}

// IPersistPropertyBag
STDMETHODIMP CFolderShortcut::Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

// IPersistPropertyBag
STDMETHODIMP CFolderShortcut::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
    _ClearState();

    IPersistPropertyBag* pppb;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IPersistPropertyBag, &pppb));
    if (SUCCEEDED(hr))
    {
        hr = pppb->Load(pPropBag, pErrorLog);
        if (SUCCEEDED(hr))
            hr = pppb->QueryInterface(IID_PPV_ARG(IShellLinkW, &_pslTarget));
        pppb->Release();
    }

    return hr;
}

STDMETHODIMP CFolderShortcut::InitNew(void)
{
    _ClearState();
    return S_OK;
}
