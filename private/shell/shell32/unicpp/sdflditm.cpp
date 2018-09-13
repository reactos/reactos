#include "stdafx.h"
#pragma hdrstop

#include "prop.h"

#define TF_SHELLAUTO TF_CUSTOM1

#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

#define DEFINE_FLOAT_STUFF  // Do this because DATE is being used below

class CFolderItemVerbs;
class CEnumFolderItemVerbs;
class CFolderItemVerb;

HRESULT CFolderItemVerbs_Create(CFolderItem *psdfi, FolderItemVerbs **ppid);

class CFolderItemVerbs : public FolderItemVerbs,
                         public CObjectSafety,
                         protected CImpIDispatch
{
friend class CEnumFolderItemVerbs;
friend class CFolderItemVerb;

public:
    CFolderItemVerbs(CFolderItem *psdfi);
    BOOL Init(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItemVerbs
    STDMETHODIMP        get_Application(IDispatch **ppid);
    STDMETHODIMP        get_Parent(IDispatch **ppid);
    STDMETHODIMP        get_Count(long *plCount);
    STDMETHODIMP        Item(VARIANT, FolderItemVerb**);
    STDMETHODIMP        _NewEnum(IUnknown **);

private:
    ~CFolderItemVerbs(void);

    LONG m_cRef;
    CFolderItem  *m_psdfi;
    HMENU m_hmenu;
    IContextMenu *m_pcm;
};

class CEnumFolderItemVerbs : public IEnumVARIANT,
                             public CObjectSafety
{
public:
    CEnumFolderItemVerbs(CFolderItemVerbs *psdfiv);
    BOOL Init();

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumFORMATETC
    STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumVARIANT **);

private:
    ~CEnumFolderItemVerbs();
    LONG m_cRef;
    int m_iCur;
    CFolderItemVerbs *m_psdfiv;
};

HRESULT CFolderItemVerb_Create(CFolderItemVerbs *psdfivs, UINT id, FolderItemVerb **pfiv);

class CFolderItemVerb : 
    public FolderItemVerb,
    public CObjectSafety,
    protected CImpIDispatch
{

friend class CFolderItemVerbs;

public:
    CFolderItemVerb(CFolderItemVerbs *psdfivs, UINT id);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItemVerbs
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_Name(BSTR *pbs);
    STDMETHODIMP DoIt();

private:
    ~CFolderItemVerb(void);

    LONG m_cRef;
    CFolderItemVerbs *m_psdfivs;
    UINT m_id;
};


// in:
//      psdf    folder that contains pidl
//      pidl    pidl retlative to psdf (the item in this folder)

HRESULT CFolderItem_Create(CFolder *psdf, LPCITEMIDLIST pidl, FolderItem **ppid)
{
    *ppid = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CFolderItem* psdfi = new CFolderItem();
    if (psdfi)
    {
        hr = psdfi->Init(psdf, pidl);
        if (SUCCEEDED(hr))
            hr = psdfi->QueryInterface(IID_FolderItem, (void **)ppid);
        psdfi->Release();
    }
    return hr;
}

// in:
//      pidl    fully qualified pidl
//      hwnd    hacks

HRESULT CFolderItem_CreateFromIDList(HWND hwnd, LPCITEMIDLIST pidl, FolderItem **ppfi)
{
    HRESULT hr;

    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent)
    {
        ILRemoveLastID(pidlParent);

        CFolder *psdf;
        hr = CFolder_Create2(hwnd, pidlParent, NULL, &psdf);
        if (SUCCEEDED(hr))
        {
            hr = CFolderItem_Create(psdf, ILFindLastID(pidl), ppfi);
            psdf->Release();
        }
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

CFolderItem::CFolderItem() :
    m_cRef(1), m_psdf(NULL), m_pidl(NULL),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItem2)
{
    DllAddRef();
}


CFolderItem::~CFolderItem(void)
{
    if (m_pidl)
        ILFree(m_pidl);
    m_psdf->Release();
    DllRelease();
}

HRESULT CFolderItem::Init(CFolder *psdf, LPCITEMIDLIST pidl)
{
    m_psdf = psdf;
    m_psdf->AddRef();

    return SHILClone(pidl, &m_pidl);
}

STDMETHODIMP CFolderItem::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItem, FolderItem2),
        QITABENTMULTI(CFolderItem, FolderItem, FolderItem2),
        QITABENTMULTI(CFolderItem, IDispatch, FolderItem2),
        QITABENTMULTI(CFolderItem, IPersist, IPersistFolder2),
        QITABENTMULTI(CFolderItem, IPersistFolder, IPersistFolder2),
        QITABENT(CFolderItem, IPersistFolder2),
        QITABENT(CFolderItem, IObjectSafety),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && IsEqualGUID(CLSID_ShellFolderItem, riid))
    {
        *ppv = (CFolderItem *)this; // unrefed
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CFolderItem::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFolderItem::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//The FolderItem implementation
STDMETHODIMP CFolderItem::get_Application(IDispatch **ppid)
{
    // Let the folder object handle security and reference counting of site, etc
    return m_psdf->get_Application(ppid);
}

STDMETHODIMP CFolderItem::get_Parent(IDispatch **ppid)
{
    // Assume that the Folder object is the parent of this object...
    HRESULT hr = m_psdf->QueryInterface(IID_IDispatch, (void **)ppid);
    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppid);
    return hr;
}

HRESULT CFolderItem::_ItemName(UINT dwFlags, BSTR *pbs)
{
    HRESULT hr = S_OK;
    STRRET strret;
    if (SUCCEEDED(m_psdf->m_psf->GetDisplayNameOf(m_pidl, dwFlags, &strret)))
    {
        *pbs = StrRetToBStr(m_pidl, &strret);
    }
    else
    {
        *pbs = SysAllocString(L"");
        hr = S_FALSE;
    }
    return *pbs ? hr : E_OUTOFMEMORY;
}

STDMETHODIMP CFolderItem::get_Name(BSTR *pbs)
{
    return _ItemName(SHGDN_INFOLDER, pbs);
}

STDMETHODIMP CFolderItem::put_Name(BSTR bs)
{
    HRESULT hr;
    LPITEMIDLIST pidlOut;

    if (_SecurityVetosRequest())
        return E_ACCESSDENIED;

    hr = m_psdf->m_psf->SetNameOf(m_psdf->m_hwnd, m_pidl, bs, SHGDN_INFOLDER, &pidlOut);
    if (SUCCEEDED(hr))
    {
        ILFree(m_pidl);
        m_pidl = pidlOut;
    }
    return hr;
}

// BUGBUG: this should validate that the path is a file system
// object (SFGAO_FILESYSTEM)

STDMETHODIMP CFolderItem::get_Path(BSTR *pbs)
{
    return _ItemName(SHGDN_FORPARSING, pbs);
}

STDMETHODIMP CFolderItem::get_GetLink(IDispatch **ppid)
{
    HRESULT hr = CShortcut_CreateIDispatch(m_psdf->m_hwnd, m_psdf->m_psf, m_pidl,  ppid);
    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppid);
    return hr;
}

// BUGBUG:: this should return Folder **...
STDMETHODIMP CFolderItem::get_GetFolder(IDispatch **ppid /* Folder **ppf */)
{
    *ppid = NULL;

    // If in Safe mode we fail this one...
    if (_SecurityVetosRequest())
        return E_ACCESSDENIED;

    HRESULT hr;
    LPITEMIDLIST pidl = ILCombine(m_psdf->m_pidl, m_pidl);
    if (pidl)
    {
        hr = CFolder_Create(NULL, pidl, NULL, IID_IDispatch, (void **)ppid);
        if (SUCCEEDED(hr))
        {
            if (_dwSafetyOptions)
                hr = MakeSafeForScripting((IUnknown**)ppid);
        }
        ILFree(pidl);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}


HRESULT CFolderItem::_CheckAttribute(ULONG ulAttrIn, VARIANT_BOOL * pb)
{
    ULONG ulAttr = ulAttrIn;
    HRESULT hr = m_psdf->m_psf->GetAttributesOf(1, (LPCITEMIDLIST*)&m_pidl, &ulAttr);
    *pb = (SUCCEEDED(hr) && (ulAttr & ulAttrIn)) ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;
}

HRESULT CFolderItem::_GetUIObjectOf(REFIID riid, void **ppv)
{
    return m_psdf->m_psf->GetUIObjectOf(m_psdf->m_hwnd, 1, (LPCITEMIDLIST*)&m_pidl, riid, NULL, ppv);
}

// returns:
//      TRUE    fail the call due to security
//      FALSE   OK, go for it

BOOL CFolderItem::_SecurityVetosRequest(void)
{
    if (!_dwSafetyOptions)
        return FALSE;   // We are not running in safe mode so say everything is OK...   

    if (!m_psdf)
        return TRUE;

    return IsSafePage(m_psdf->_punkSite) != S_OK;
}

// allow friend classes to get the IDList for a CFolderItem automation object

LPCITEMIDLIST CFolderItem::_GetIDListFromVariant(const VARIANT *pv)
{
    LPCITEMIDLIST pidl = NULL;
    if (pv)
    {
        VARIANT v;

        if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
            v = *pv->pvarVal;
        else
            v = *pv;

        switch (v.vt)
        {
        case VT_DISPATCH | VT_BYREF:
            if (v.ppdispVal == NULL)
                break;

            v.pdispVal = *v.ppdispVal;

            // fall through...

        case VT_DISPATCH:
            CFolderItem *pfi;
            if (v.pdispVal && SUCCEEDED(v.pdispVal->QueryInterface(CLSID_ShellFolderItem, (void **)&pfi)))
            {
                pidl = pfi->m_pidl; // alias
            }
            break;
        }
    }
    return pidl;
}


STDMETHODIMP CFolderItem::get_IsLink(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_LINK, pb);
}

STDMETHODIMP CFolderItem::get_IsFolder(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FOLDER, pb);
}

STDMETHODIMP CFolderItem::get_IsFileSystem(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FILESYSTEM, pb);
}

STDMETHODIMP CFolderItem::get_IsBrowsable(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_BROWSABLE, pb);
}

STDMETHODIMP CFolderItem::get_ModifyDate(DATE *pdt)
{
    WIN32_FIND_DATA finddata;
    if (SUCCEEDED(SHGetDataFromIDList(m_psdf->m_psf, m_pidl, SHGDFIL_FINDDATA, &finddata, sizeof(finddata))))
    {
        WORD wDosDate, wDosTime;
        FILETIME filetime;
        FileTimeToLocalFileTime(&finddata.ftLastWriteTime, &filetime);
        FileTimeToDosDateTime(&filetime, &wDosDate, &wDosTime);
        DosDateTimeToVariantTime(wDosDate, wDosTime, pdt);
    }
    return NOERROR;
}

STDMETHODIMP CFolderItem::put_ModifyDate(DATE dt)
{
    HRESULT hr = S_FALSE;
    SYSTEMTIME st;
    FILETIME ftLocal, ft;
    BSTR bstrPath;
    
    if (SUCCEEDED(VariantTimeToSystemTime(dt, &st)) && SystemTimeToFileTime(&st, &ftLocal)
            && LocalFileTimeToFileTime(&ftLocal, &ft) && SUCCEEDED(get_Path(&bstrPath)))
    {
        TCHAR szPath[MAX_PATH];
        
        SHUnicodeToTChar(bstrPath, szPath, ARRAYSIZE(szPath));
        SysFreeString(bstrPath);
        HANDLE hFile = CreateFile(szPath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ,
                NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_NO_RECALL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            if (SetFileTime(hFile, NULL, NULL, &ft))
            {
                hr = S_OK;
            }
            CloseHandle(hFile);
        }
    }
    return hr;
}

// BUGBUG:: Should see about getting larger numbers through
STDMETHODIMP CFolderItem::get_Size(LONG *pul)
{
    WIN32_FIND_DATA finddata;
    if (SUCCEEDED(SHGetDataFromIDList(m_psdf->m_psf, m_pidl, SHGDFIL_FINDDATA, &finddata, sizeof(finddata))))
    {
        *pul = (LONG)finddata.nFileSizeLow;
    }
    else
    {
        *pul = 0L;
    }
    return NOERROR; // Scripts don't like error return values
}

STDMETHODIMP CFolderItem::get_Type(BSTR *pbs)
{
    VARIANT var;
    var.vt = VT_EMPTY;
    if (SUCCEEDED(ExtendedProperty(L"Type", &var)) && var.vt == VT_BSTR)
    {
        *pbs = SysAllocString(var.bstrVal);
    }
    else
    {
        *pbs = SysAllocString(L"");
    }
    VariantClear(&var);
    return *pbs ? S_OK : E_OUTOFMEMORY;
}


STDMETHODIMP CFolderItem::ExtendedProperty(BSTR bstrPropName, VARIANT *pvRet)
{
    HRESULT hr = S_OK;      // Java scripts barf on E_FAIL
    pvRet->vt = VT_EMPTY;

    // currently, MCNL is 80, guidstr is 39, and 6 is the width of an int
    if (StrCmpIW(bstrPropName, L"infotip") == 0)
    {
        // They want the info tip for the item.
        if (m_pidl && m_psdf)
        {
            TCHAR szInfo[INFOTIPSIZE];
            GetInfoTip(m_psdf->m_psf, m_pidl, szInfo, ARRAYSIZE(szInfo));
            hr = InitVariantFromStr(pvRet, szInfo);
        }
    }
    else if (m_psdf->m_psf2)
    {
        SHCOLUMNID scid;
        TCHAR szTemp[128];

        SHUnicodeToTChar(bstrPropName, szTemp, ARRAYSIZE(szTemp));

        if (ParseSCIDString(szTemp, &scid, NULL))
        {
            // Note that GetDetailsEx expects an absolute pidl
            m_psdf->m_psf2->GetDetailsEx(m_pidl, &scid, pvRet);
        }
    }
    return hr;
}


STDMETHODIMP CFolderItem::Verbs(FolderItemVerbs **ppfic)
{
    HRESULT hr = CFolderItemVerbs_Create(SAFECAST(this, CFolderItem*), ppfic);
    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppfic);
    return hr;
}

// Helper function to invoke a verb on an array of pidls.
HRESULT InvokeVerbHelper(VARIANT vVerb, VARIANT vArgs, LPCITEMIDLIST *ppidl, int cItems, DWORD dwSafetyOptions, CFolder * psdf)
{
    IContextMenu *pcm;
    BOOL fDefaultVerb = TRUE;
    TCHAR szCmd[128];
    TCHAR   szURL[MAX_URL_STRING];
    DWORD dwPolicy = 0, dwContext = 0;
    BOOL  bSafeZone;
    MENUITEMINFO mii;

    bSafeZone = !dwSafetyOptions || (psdf->_punkSite && IsSafePage(psdf->_punkSite) == S_OK);

    switch (vVerb.vt)
    {
    case VT_BSTR:
        if (!bSafeZone)
            return E_ACCESSDENIED;  // not allowed in safe mode
        fDefaultVerb = FALSE;
        SHUnicodeToTChar(vVerb.bstrVal, szCmd, ARRAYSIZE(szCmd));
        break;
    }

    // Do a zones check 
    if (!bSafeZone)
    {
        LPITEMIDLIST pidl;

        for (int i = 0; i < cItems; i++)
        {
            pidl = ILCombine(psdf->m_pidl, ppidl[i]);

            if (pidl)
            {
                SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szURL, SIZECHARS(szURL), NULL);
    
                ILFree(pidl);
                ZoneCheckUrlEx(szURL, &dwPolicy, SIZEOF(dwPolicy), &dwContext, SIZEOF(dwContext), 
                                URLACTION_SHELL_VERB, PUAF_NOUI, NULL);
                dwPolicy = GetUrlPolicyPermissions(dwPolicy);
                if (dwPolicy != URLPOLICY_ALLOW)
                {
                    return E_ACCESSDENIED;
                }
            }
        }
    }
     

    if (SUCCEEDED(psdf->m_psf->GetUIObjectOf(psdf->m_hwnd, cItems, ppidl, IID_IContextMenu, NULL, (void **)&pcm)) && pcm)
    {
        HMENU hmenu = CreatePopupMenu();
        pcm->QueryContextMenu(hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, fDefaultVerb ? CMF_DEFAULTONLY : CMF_CANRENAME);
        int idCmd = 0, iItem;

        if (fDefaultVerb)
            idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
        else
        {
            // Lets try to find a verb that matches name:
            // BUGBUG:: Right now must match & in verbs...
            int i;
            MENUITEMINFO mii;
            TCHAR szText[128];    // should be big enough for this
            for (i = GetMenuItemCount(hmenu)-1; i >= 0; i--)
            {
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.dwTypeData = szText;
                mii.fMask = MIIM_ID | MIIM_TYPE;
                mii.cch = ARRAYSIZE(szText);
                mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
                mii.dwItemData = 0;
                GetMenuItemInfo(hmenu, i, TRUE, &mii);

                if (lstrcmpi(szText, szCmd) == 0)
                {
                    idCmd = mii.wID;
                    break;
                }
            }
        }

        // If we couldn't find it the old way see if it is a canonical verb
        if (!idCmd && (-1 != (iItem = GetMenuIndexForCanonicalVerb(hmenu, pcm, CMD_ID_FIRST, vVerb.bstrVal))))
        {
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID;
    
            if (GetMenuItemInfo(hmenu, iItem, MF_BYPOSITION, &mii))
                idCmd = mii.wID;
        }

        if (idCmd)
        {
            CMINVOKECOMMANDINFO ici = {
                sizeof(CMINVOKECOMMANDINFO),
                0L,
                psdf->m_hwnd,
                NULL,
                NULL, NULL,
                SW_SHOWNORMAL,
            };

            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CMD_ID_FIRST);

            char szArgs[MAX_PATH];       // max size we will currently use.

            // See if we are supposed to pass any arguments on the command line
            switch (vArgs.vt)
            {
            case VT_BSTR:
                SHUnicodeToAnsi(vArgs.bstrVal, szArgs, ARRAYSIZE(szArgs));
                ici.lpParameters =  szArgs;
                break;
            }

            // Finally invoke the command
            pcm->InvokeCommand(&ici);
        }

        DestroyMenu(hmenu);
        pcm->Release();
    }
    return NOERROR;
} 

STDMETHODIMP CFolderItem::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    return InvokeVerbHelper(vVerb, vArgs, (LPCITEMIDLIST *)&m_pidl, 1, _dwSafetyOptions, m_psdf);
}

STDMETHODIMP CFolderItem::InvokeVerb(VARIANT vVerb)
{
    VARIANT vtEmpty = {VT_EMPTY};
    return InvokeVerbEx(vVerb, vtEmpty);
}

STDMETHODIMP CFolderItem::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ShellFolderItem;
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItem::Initialize(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItem::GetCurFolder(LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetIDListFromUnk(m_psdf->m_psf, &pidlFolder);
    if (S_OK == hr)
    {
        hr = SHILCombine(pidlFolder, m_pidl, ppidl);
        ILFree(pidlFolder);
    }
    else
        hr = E_FAIL;
    return hr;
}


HRESULT CFolderItemVerbs_Create(CFolderItem *psdfi, FolderItemVerbs ** ppid)
{
    *ppid = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CFolderItemVerbs* pfiv = new CFolderItemVerbs(psdfi);
    if (pfiv)
    {
        if (pfiv->Init())
            hr = pfiv->QueryInterface(IID_FolderItemVerbs, (void **)ppid);
        pfiv->Release();
    }
    return hr;
}

CFolderItemVerbs::CFolderItemVerbs(CFolderItem *psdfi) :
    m_cRef(1), m_hmenu(NULL), m_pcm(NULL),
    m_psdfi(psdfi), CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItemVerbs)
{
    DllAddRef();
    m_psdfi->AddRef();
}

CFolderItemVerbs::~CFolderItemVerbs(void)
{
    DllRelease();

    if (m_pcm)
        m_pcm->Release();

    if (m_hmenu)
        DestroyMenu(m_hmenu);

    m_psdfi->Release();
}

BOOL CFolderItemVerbs::Init()
{
    TraceMsg(TF_SHELLAUTO, "CFolderItemVerbs::Init called");

    // Start of only doing default verb...
    if (SUCCEEDED(m_psdfi->_GetUIObjectOf(IID_IContextMenu, (void **)&m_pcm)) && m_pcm)
    {
        m_hmenu = CreatePopupMenu();
        if (FAILED(m_pcm->QueryContextMenu(m_hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, 0)))
            return FALSE;
    }
    else
        return FALSE;

    // Just for the heck of it, remove junk like sepearators from the menu...
    int i;
    MENUITEMINFO mii;
    TCHAR szText[80];    // should be big enough for this
    for (i = GetMenuItemCount(m_hmenu) - 1; i >= 0; i--)
    {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.dwTypeData = szText;
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.cch = ARRAYSIZE(szText);
        mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
        mii.dwItemData = 0;
        GetMenuItemInfo(m_hmenu, i, TRUE, &mii);
        if (mii.fType & MFT_SEPARATOR)
            DeleteMenu(m_hmenu, i, MF_BYPOSITION);
    }
    return TRUE;
}

STDMETHODIMP CFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItemVerbs, FolderItemVerbs),
        QITABENT(CFolderItemVerbs, IObjectSafety),
        QITABENTMULTI(CFolderItemVerbs, IDispatch, FolderItemVerbs),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItemVerbs::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFolderItemVerbs::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFolderItemVerbs::get_Application(IDispatch **ppid)
{
    return m_psdfi->get_Application(ppid);
}

STDMETHODIMP CFolderItemVerbs::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItemVerbs::get_Count(long *plCount)
{
    *plCount = 0;
    if (m_psdfi->_SecurityVetosRequest())
        return E_ACCESSDENIED;

    *plCount = GetMenuItemCount(m_hmenu);
    return NOERROR;
}

STDMETHODIMP CFolderItemVerbs::Item(VARIANT index, FolderItemVerb **ppid)
{
    *ppid = NULL;

    if (m_psdfi->_SecurityVetosRequest())
        return E_ACCESSDENIED;

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF | VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;

    switch (index.vt)
    {
    case VT_ERROR:
        QueryInterface(IID_IDispatch, (void **)ppid);
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        if ((index.lVal >= 0) && (index.lVal <= GetMenuItemCount(m_hmenu)))
        {
            CFolderItemVerb_Create(this, GetMenuItemID(m_hmenu, index.lVal), ppid);
        }

        break;
#ifdef LATER    // Should match strings in menu...
    case VT_BSTR:
        {
            // map canonical name into item id
            if (SUCCEEDED(hr))
            {
                CFolderItemVerb_Create(this, GetMenuItemID(index.lVal), ppid);
            }
            return hr;
        }


        break;
#endif  // Later
    default:
        return E_NOTIMPL;
    }

    if (*ppid && _dwSafetyOptions)
        return MakeSafeForScripting((IUnknown**)ppid);

    return NOERROR;
}

STDMETHODIMP CFolderItemVerbs::_NewEnum(IUnknown **ppunk)
{
    *ppunk = NULL;

    if (m_psdfi->_SecurityVetosRequest())
        return E_ACCESSDENIED;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItemVerbs *pNew = new CEnumFolderItemVerbs(SAFECAST(this, CFolderItemVerbs*));
    if (pNew)
    {
        if (pNew->Init())
            hr = pNew->QueryInterface(IID_IEnumVARIANT, (void **)ppunk);
        pNew->Release();
    }
    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting(ppunk);
    return hr;
}

CEnumFolderItemVerbs::CEnumFolderItemVerbs(CFolderItemVerbs *pfiv) :
    m_cRef(1), m_iCur(0), m_psdfiv(pfiv)
{
    m_psdfiv->AddRef();
    DllAddRef();
}


CEnumFolderItemVerbs::~CEnumFolderItemVerbs(void)
{
    DllRelease();
    m_psdfiv->Release();
}

BOOL CEnumFolderItemVerbs::Init()
{
    return TRUE;    // Currently no initialization needed
}

STDMETHODIMP CEnumFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumFolderItemVerbs, IEnumVARIANT),
        QITABENT(CEnumFolderItemVerbs, IObjectSafety),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CEnumFolderItemVerbs::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumFolderItemVerbs::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CEnumFolderItemVerbs::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG cReturn = 0;
    HRESULT hr;

    if (!pulVar)
    {
        if (cVar != 1)
            return E_POINTER;
    }
    else
        *pulVar = 0;

    if (!pVar || m_iCur >= GetMenuItemCount(m_psdfiv->m_hmenu))
        return S_FALSE;

    while (m_iCur < GetMenuItemCount(m_psdfiv->m_hmenu) && cVar > 0)
    {
	    FolderItemVerb *pidv;
        hr = CFolderItemVerb_Create(m_psdfiv, GetMenuItemID(m_psdfiv->m_hmenu, m_iCur), &pidv);

        if (SUCCEEDED(hr) && _dwSafetyOptions)
            hr = MakeSafeForScripting((IUnknown**)&pidv);

        m_iCur++;
        if (SUCCEEDED(hr))
        {
            pVar->pdispVal = pidv;
            pVar->vt = VT_DISPATCH;
            pVar++;
            cReturn++;
            cVar--;
        }
    }

    if (pulVar)
        *pulVar = cReturn;

    return NOERROR;
}

STDMETHODIMP CEnumFolderItemVerbs::Skip(ULONG cSkip)
{
    if ((int)(m_iCur+cSkip) >= GetMenuItemCount(m_psdfiv->m_hmenu))
        return S_FALSE;

    m_iCur+=cSkip;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItemVerbs::Reset(void)
{
    m_iCur = 0;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItemVerbs::Clone(IEnumVARIANT **ppEnum)
{
    *ppEnum = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItemVerbs *pNew = new CEnumFolderItemVerbs(m_psdfiv);
    if (pNew)
    {
        if (pNew->Init())
            hr = pNew->QueryInterface(IID_IEnumVARIANT, (void **)ppEnum);
        pNew->Release();
    }

    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppEnum);

    return hr;
}

HRESULT CFolderItemVerb_Create(CFolderItemVerbs *psdfivs, UINT id, FolderItemVerb **ppid)
{
    *ppid = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CFolderItemVerb* psdfiv = new CFolderItemVerb(psdfivs, id);
    if (psdfiv)
    {
        hr = psdfiv->QueryInterface(IID_FolderItemVerb, (void **)ppid);
        psdfiv->Release();
    }

    return hr;
}

CFolderItemVerb::CFolderItemVerb(CFolderItemVerbs *psdfivs, UINT id) :
    m_cRef(1), m_psdfivs(psdfivs), m_id(id),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItemVerb)
{
    m_psdfivs->AddRef();
    DllAddRef();
}

CFolderItemVerb::~CFolderItemVerb(void)
{
    DllRelease();
    m_psdfivs->Release();
}

STDMETHODIMP CFolderItemVerb::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItemVerb, FolderItemVerb),
        QITABENT(CFolderItemVerb, IObjectSafety),
        QITABENTMULTI(CFolderItemVerb, IDispatch, FolderItemVerb),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItemVerb::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFolderItemVerb::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFolderItemVerb::get_Application(IDispatch **ppid)
{
    return m_psdfivs->get_Application(ppid);
}

STDMETHODIMP CFolderItemVerb::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CFolderItemVerb::get_Name(BSTR *pbs)
{
    TCHAR szMenuText[MAX_PATH];

    // Warning: did not check security here as could not get here if unsafe...
    GetMenuString(m_psdfivs->m_hmenu, m_id, szMenuText, ARRAYSIZE(szMenuText), MF_BYCOMMAND);
    *pbs = SysAllocStringT(szMenuText);

    return *pbs ? S_OK : E_OUTOFMEMORY;
}


STDMETHODIMP CFolderItemVerb::DoIt()
{
    CMINVOKECOMMANDINFO ici = {
        sizeof(CMINVOKECOMMANDINFO),
        0L,
        NULL,
        NULL,
        NULL, NULL,
        SW_SHOWNORMAL,
    };

    // Warning: did not check security here as could not get here if unsafe...
    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(m_id - CMD_ID_FIRST);
    return m_psdfivs->m_pcm->InvokeCommand(&ici);
}
