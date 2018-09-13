#include "stdafx.h"
#pragma hdrstop

//#include "sdspatch.h"
//#include "security.h"
//#include "dutil.h"
//#include <oleauto.h>    // For OLEAUT32.DLL

#define TF_SHELLAUTO TF_CUSTOM1

EXTERN_C const GUID IID_ISDGetPidl       = {0xc066d4e0, 0x6400, 0x11d0, 0x95, 0x25, 0x0, 0xa0, 0xc9, 0x1f, 0x38, 0x80};
EXTERN_C const GUID IID_ICSDFolderItem   = {0xaae84a70, 0x40db, 0x11d0, 0x94, 0xeb, 0x0, 0xa0, 0xc9, 0x1f, 0x38, 0x80};

#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

#define DEFINE_FLOAT_STUFF  // Do this because DATE is being used below


//==========================================================================

class CSDFolderItemVerbs;
class CSDEnumFolderItemVerbs;
class CSDFolderItemVerb;

HRESULT CSDFolderItemVerbs_Create(CSDFldrItem *psdfi, FolderItemVerbs ** ppid);

//==========================================================================
// Folder item Verbs class definition

class CSDFolderItemVerbs : public FolderItemVerbs,
                            protected CImpIDispatch
{
    friend class CSDEnumFolderItemVerbs;
    friend class CSDFolderItemVerb;

    public:
        ULONG           m_cRef; //Public for debug checks


    protected:
        CSDFldrItem  *m_psdfi;

        HMENU           m_hmenu;
        IContextMenu    *m_pcm;
    public:
        CSDFolderItemVerbs(CSDFldrItem *psdfi);
        ~CSDFolderItemVerbs(void);

        BOOL         Init(void);

        //IUnknown methods
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

        //FolderItemVerbs methods
        STDMETHODIMP        get_Application(IDispatch **ppid);
        STDMETHODIMP        get_Parent(IDispatch **ppid);
        STDMETHODIMP        get_Count(long *plCount);
        STDMETHODIMP        Item(VARIANT, FolderItemVerb**);
        STDMETHODIMP        _NewEnum(IUnknown **);
};

class CSDEnumFolderItemVerbs : public IEnumVARIANT
{
    protected:
        ULONG           m_cRef;

        int             m_iCur;
        CSDFolderItemVerbs   *m_psdfiv;

    public:
        CSDEnumFolderItemVerbs(CSDFolderItemVerbs *psdfiv);
        ~CSDEnumFolderItemVerbs();

        BOOL        Init();

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumFORMATETC members
        STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumVARIANT **);
};

HRESULT CSDFolderItemVerb_Create(CSDFolderItemVerbs *psdfivs, UINT id, FolderItemVerb **pfiv);

class CSDFolderItemVerb : public FolderItemVerb,
        protected CImpIDispatch
{
    public:
        ULONG           m_cRef; //Public for debug checks


    protected:
        friend class CSDFolderItemVerbs;

        CSDFolderItemVerbs  *m_psdfivs;
        UINT                m_id;

    public:
        CSDFolderItemVerb(CSDFolderItemVerbs *psdfivs, UINT id);
        ~CSDFolderItemVerb(void);

        //IUnknown methods
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

        //FolderItem methods
        STDMETHODIMP get_Application(IDispatch **ppid);
        STDMETHODIMP get_Parent(IDispatch **ppid);
        STDMETHODIMP get_Name(BSTR *pbs);
        STDMETHODIMP DoIt();

    protected:
};


//==========================================================================

HRESULT CSDFldrItem_Create(CSDFolder *psdf, LPITEMIDLIST pidl, FolderItem **ppid)
{
    *ppid = NULL;
    HRESULT hres = E_OUTOFMEMORY;

    CSDFldrItem* psdfi = new CSDFldrItem(psdf);
    if (psdfi)
    {
        if (psdfi->Init(pidl))
        {
            hres = psdfi->QueryInterface(IID_FolderItem, (void **)ppid);
            if (SUCCEEDED(hres))
                psdf->_GetShellFolder();
        }
        psdfi->Release();
    }
    return hres;
}

CSDFldrItem::CSDFldrItem(CSDFolder *psdf) :
    m_cRef(1), m_psdf(psdf), m_pidl(NULL),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItem)
{
    m_psdf->AddRef();
    DllAddRef();
}


CSDFldrItem::~CSDFldrItem(void)
{
    if (m_pidl)
        ILFree(m_pidl);
    m_psdf->Release();
    DllRelease();
}

BOOL CSDFldrItem::Init(LPITEMIDLIST pidl)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItem::Init called");

    m_pidl = ILClone(pidl);
    if (!m_pidl)
        return FALSE;

    return TRUE;
}

STDMETHODIMP CSDFldrItem::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDFldrItem, FolderItem),
        QITABENTMULTI(CSDFldrItem, IDispatch, FolderItem),
        QITABENTMULTI(CSDFldrItem, ICSDFolderItem, FolderItem),
        QITABENT(CSDFldrItem, ISDGetPidl),
        QITABENT(CSDFldrItem, IObjectSafety),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDFldrItem::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDFldrItem::Release(void)
{
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


//The FolderItem implementation
STDMETHODIMP CSDFldrItem::get_Application(IDispatch **ppid)
{
    // Let the folder object handle security and reference counting of site, etc
    return m_psdf->get_Application(ppid);
}

STDMETHODIMP CSDFldrItem::get_Parent(IDispatch **ppid)
{
    // Assume that the Folder object is the parent of this object...
    HRESULT hres = m_psdf->QueryInterface(IID_IDispatch, (LPVOID*)ppid);
    if (SUCCEEDED(hres) && _dwSafetyOptions)
        hres = MakeSafeForScripting((IUnknown**)ppid);
    return hres;
}


STDMETHODIMP CSDFldrItem::get_Name(BSTR *pbs)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItem::Get_Name called");
    *pbs = NULL;

    STRRET strret;
    if (SUCCEEDED(m_psdf->m_psf->GetDisplayNameOf(m_pidl, SHGDN_NORMAL, &strret)))
    {
        *pbs = StrRetToBStr(m_pidl, &strret);
    }

    return NOERROR;
}

STDMETHODIMP CSDFldrItem::put_Name(BSTR bs)
{
    HRESULT hres;
    LPITEMIDLIST pidlOut;

    if (_dwSafetyOptions && (!m_psdf || LocalZoneCheck(m_psdf->_punkSite) != S_OK))
        return E_ACCESSDENIED;

    hres = m_psdf->m_psf->SetNameOf(m_psdf->m_hwnd, m_pidl, bs, SHGDN_INFOLDER, &pidlOut);
    if (SUCCEEDED(hres))
    {
        ILFree(m_pidl);
        m_pidl = pidlOut;
    }
    return hres;
}

STDMETHODIMP CSDFldrItem::get_Path(BSTR *pbs)
{
    TraceMsg(TF_SHELLAUTO, "CSDFolderItem::Get_Path called");

    *pbs = NULL;

    STRRET strret;
    if (SUCCEEDED(m_psdf->m_psf->GetDisplayNameOf(m_pidl, SHGDN_FORPARSING, &strret)))
    {
        *pbs = StrRetToBStr(m_pidl, &strret);
    }

    return NOERROR;
}

STDMETHODIMP CSDFldrItem::get_GetLink(IDispatch **ppid)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItem::Get_GetLink called");
    HRESULT hres = CSDShellLink_CreateIDispatch(m_psdf->m_hwnd, m_psdf->m_psf, m_pidl,  ppid);
    if (SUCCEEDED(hres) && _dwSafetyOptions)
        hres = MakeSafeForScripting((IUnknown**)ppid);
    return hres;
}

STDMETHODIMP CSDFldrItem::get_GetFolder(IDispatch **ppid)
{
    // BUGBUG:: this should return Folder...
    *ppid = NULL;

    // If in Safe mode we fail this one...
    if (_dwSafetyOptions && (!m_psdf || LocalZoneCheck(m_psdf->_punkSite) != S_OK))
        return E_ACCESSDENIED;

    CSDFolder *psf;
    LPITEMIDLIST pidl = ILCombine(m_psdf->m_pidl, m_pidl);
    if (pidl)
    {
        HRESULT hres = CSDFolder_Create(NULL, pidl, NULL, &psf);
        if (SUCCEEDED(hres) && psf)
        {
            hres = psf->QueryInterface(IID_IDispatch, (LPVOID*)ppid);
            psf->Release();
            if (SUCCEEDED(hres) && _dwSafetyOptions)
                hres = MakeSafeForScripting((IUnknown**)ppid);
        }
        ILFree(pidl);
        return hres;
    }
    return E_OUTOFMEMORY;
}


HRESULT CSDFldrItem::_CheckAttribute( ULONG ulAttrIn, VARIANT_BOOL * pb)
{
    ULONG ulAttr = ulAttrIn;
    HRESULT hres;
    *pb = FALSE;
    if (SUCCEEDED(hres = m_psdf->m_psf->GetAttributesOf(1, (LPCITEMIDLIST*)&m_pidl, &ulAttr))
            && (ulAttr & ulAttrIn))
        *pb = TRUE;

    return hres;
}

HRESULT CSDFldrItem::_GetUIObjectOf(REFIID riid, void **ppvOut)
{
    return m_psdf->m_psf->GetUIObjectOf(m_psdf->m_hwnd, 1, (LPCITEMIDLIST*)&m_pidl, riid, NULL, ppvOut);
}

STDMETHODIMP CSDFldrItem::get_IsLink(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_LINK, pb);
}

STDMETHODIMP CSDFldrItem::get_IsFolder(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FOLDER, pb);
}

STDMETHODIMP CSDFldrItem::get_IsFileSystem(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FILESYSTEM, pb);
}

STDMETHODIMP CSDFldrItem::get_IsBrowsable(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_BROWSABLE, pb);
}

STDMETHODIMP CSDFldrItem::get_ModifyDate(DATE *pdt)
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

STDMETHODIMP CSDFldrItem::put_ModifyDate(DATE dt)
{
    return E_NOTIMPL;
}

// BUGBUG:: Should see about getting larger numbers through
STDMETHODIMP CSDFldrItem::get_Size(LONG *pul)
{
    WIN32_FIND_DATA finddata;
    if (SUCCEEDED(SHGetDataFromIDList(m_psdf->m_psf, m_pidl, SHGDFIL_FINDDATA, &finddata, sizeof(finddata))))
    {
         *pul = (LONG)finddata.nFileSizeLow;
    }
    return NOERROR;
}

STDMETHODIMP CSDFldrItem::get_Type(BSTR *pbs)
{
    HRESULT hres;
    *pbs = NULL;    // in case of error.
    IShellDetails *psd = m_psdf->_GetShellDetails();
    if (!psd)
        return E_FAIL;

    SHELLDETAILS sd;
    if (SUCCEEDED(hres = psd->GetDetailsOf(m_pidl, 2, &sd)))
    {
        *pbs = StrRetToBStr(m_pidl, &sd.str);
    }
    return hres;
}

STDMETHODIMP CSDFldrItem::Verbs (FolderItemVerbs **ppfic)
{
    HRESULT hres = CSDFolderItemVerbs_Create(SAFECAST(this, CSDFldrItem*), ppfic);
    if (SUCCEEDED(hres) && _dwSafetyOptions)
        hres = MakeSafeForScripting((IUnknown**)ppfic);
    return hres;
}

STDMETHODIMP CSDFldrItem::InvokeVerb(VARIANT vVerb)
{
    IContextMenu *pcm;
    BOOL fDefaultVerb = TRUE;
    TCHAR szCmd[128];
    TCHAR   szURL[MAX_URL_STRING];
    DWORD dwPolicy = 0, dwContext = 0;
    BOOL  bSafeZone;
    
    bSafeZone = !_dwSafetyOptions || (m_psdf && LocalZoneCheck(m_psdf->_punkSite) == S_OK);

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
       LPITEMIDLIST pidl = ILCombine(m_psdf->m_pidl, m_pidl);

       if (pidl)
       {
           ILGetDisplayNameEx(NULL, pidl, szURL, SHGDN_FORPARSING);
           ILFree(pidl);
           ZoneCheckUrlEx(szURL, &dwPolicy, SIZEOF(dwPolicy), &dwContext, SIZEOF(dwContext), 
                            URLACTION_SHELL_VERB, PUAF_NOUI, NULL);
            dwPolicy = GetUrlPolicyPermissions(dwPolicy);
            if (dwPolicy != URLPOLICY_ALLOW) 
                return E_ACCESSDENIED;  
        }    
    }
    
    // Start of only doing default verb...
    if (SUCCEEDED(m_psdf->m_psf->GetUIObjectOf(m_psdf->m_hwnd, 1, (LPCITEMIDLIST*)&m_pidl, IID_IContextMenu, NULL, (LPVOID*)&pcm)) && pcm)
    {
        HMENU hmenu = CreatePopupMenu();
        pcm->QueryContextMenu(hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST, fDefaultVerb? CMF_DEFAULTONLY : CMF_CANRENAME);
        int idCmd = 0;

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

        if (idCmd)
        {
            CMINVOKECOMMANDINFO ici = {
                sizeof(CMINVOKECOMMANDINFO),
                0L,
                m_psdf->m_hwnd,
                NULL,
                NULL, NULL,
                SW_SHOWNORMAL,
            };

            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CMD_ID_FIRST);
            // Finally invoke the command
            pcm->InvokeCommand(&ici);
        }

        DestroyMenu(hmenu);
        pcm->Release();
    }
    return NOERROR;
}

STDMETHODIMP CSDFldrItem::GetPidl (LPITEMIDLIST *ppidl)
{
    *ppidl = ILClone(m_pidl);
    if (*ppidl)
        return NOERROR;
    else
    {
        return E_OUTOFMEMORY;
    }
}

//==========================================================================
// Folder item verbs

HRESULT CSDFolderItemVerbs_Create(CSDFldrItem *psdfi, FolderItemVerbs ** ppid)
{
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerbs_CreateIDispatch called");

    *ppid = NULL;
    CSDFolderItemVerbs* psdfiv = new CSDFolderItemVerbs(psdfi);

    if (psdfiv)
    {
        if (!psdfiv->Init())
        {
            psdfiv->Release();
            return E_OUTOFMEMORY;
        }

        HRESULT hres = psdfiv->QueryInterface(IID_FolderItemVerbs, (LPVOID *)ppid);
        psdfiv->Release();
        if (FAILED(hres))
        {
            *ppid = NULL;
            return hres;
        }
	return S_OK;
    }

    return E_OUTOFMEMORY;
}

/*
 * CSDFolderItemVerbs::CSDFolderItemVerbs
 * CSDFolderItemVerbs::~CSDFolderItemVerbs
 *
 * Constructor Parameters:
 *  pCL             PCCosmoClient to the client object that we use
 *                  to implement much of this interface.
 */

CSDFolderItemVerbs::CSDFolderItemVerbs(CSDFldrItem *psdfi) :
    m_cRef(1), m_hmenu(NULL), m_pcm(NULL),
    m_psdfi(psdfi), CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItemVerbs)
{
    DllAddRef();
    m_psdfi->AddRef();
}


CSDFolderItemVerbs::~CSDFolderItemVerbs(void)
{
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerbs::~CSDFolderItemVerbs called");
    DllRelease();

    if (m_pcm)
        m_pcm->Release();

    if (m_hmenu)
        DestroyMenu(m_hmenu);

    m_psdfi->Release();
}


/*
 * CSDFolderItemVerbs::Init
 *
 * Purpose:
 *  Performs any intiailization of a CSDFolderItemVerbs that's prone to failure
 *  that we also use internally before exposing the object outside.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful,
 *                  FALSE otherwise.
 */

BOOL CSDFolderItemVerbs::Init()
{
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerbs::Init called");

    // Start of only doing default verb...
    if (SUCCEEDED(m_psdfi->_GetUIObjectOf(IID_IContextMenu, (LPVOID*)&m_pcm)) && m_pcm)
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
    for (i = GetMenuItemCount(m_hmenu)-1; i >= 0; i--)
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

STDMETHODIMP CSDFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDFolderItemVerbs, FolderItemVerbs),
        QITABENTMULTI(CSDFolderItemVerbs, IDispatch, FolderItemVerbs),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDFolderItemVerbs::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDFolderItemVerbs::Release(void)
{
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP CSDFolderItemVerbs::get_Application(IDispatch **ppid)
{
    // Walk it up the tree, it will get up to folder object that will cache the application object...
    return m_psdfi->get_Application(ppid);
}

STDMETHODIMP CSDFolderItemVerbs::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSDFolderItemVerbs::get_Count(long *plCount)
{
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerbs::Get_Count called");
    *plCount = GetMenuItemCount(m_hmenu);
    return NOERROR;
}



/*
 * This is essentially an array lookup operator for the collection.
 * Collection.Item by itself the same as the collection itself.
 * Otherwise you can refer to the item by index or by path, which
 * shows up in the VARIANT parameter.  We have to check the type
 * of the variant to see if it's VT_I4 (an index) or VT_BSTR (a
 * path) and do the right thing.
 */

STDMETHODIMP CSDFolderItemVerbs::Item(VARIANT index, FolderItemVerb **ppid)
{
    *ppid= NULL;
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerbs::Get_Item called");

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF|VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;

    switch (index.vt)
    {
    case VT_ERROR:
            /*
             * No parameters, get the "Figures" collection
             * IDispatch, which we can easily retrieve with
             * our own QueryInterface.
             */
        QueryInterface(IID_IDispatch, (void **)ppid);
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        if ((index.lVal >= 0) && (index.lVal <= GetMenuItemCount(m_hmenu)))
        {
            if (FAILED(CSDFolderItemVerb_Create(this, GetMenuItemID(m_hmenu, index.lVal),
                        ppid)))
                *ppid = NULL;
        }

        break;
#ifdef LATER    // Should match strings in menu...
    case VT_BSTR:
        {
            HRESULT hres;
            ULONG ulEaten;
            LPITEMIDLIST pidl;
            hres = m_psdf->m_psf->ParseDisplayName(m_psdf->m_hwnd, NULL,
                    (LPOLESTR)index.bstrVal, &ulEaten, &pidl, NULL);
            if (SUCCEEDED(hres))
            {
                hres = CSDFolderItemVerb_Create(this, GetMenuItemID(index.lVal), ppid);
                if (FAILED(hres))
                    *ppid = NULL;
            }
            return hres;
        }


        break;
#endif  // Later
    default:
        return E_NOTIMPL;
    }

    return NOERROR;
}

STDMETHODIMP CSDFolderItemVerbs::_NewEnum(IUnknown **ppunk)
{
    CSDEnumFolderItemVerbs *pNew = new CSDEnumFolderItemVerbs(SAFECAST(this, CSDFolderItemVerbs*));
    if (NULL!=pNew)
    {
        if (!pNew->Init())
        {
            delete pNew;
            pNew=NULL;
        }
    }

    *ppunk = pNew;
    return NOERROR;
}

CSDEnumFolderItemVerbs::CSDEnumFolderItemVerbs(CSDFolderItemVerbs *psdfiv) :
    m_cRef(1), m_iCur(0), m_psdfiv(psdfiv)
{
    m_psdfiv->AddRef();
    DllAddRef();
}


CSDEnumFolderItemVerbs::~CSDEnumFolderItemVerbs(void)
{
    DllRelease();
    m_psdfiv->Release();
}

BOOL CSDEnumFolderItemVerbs::Init()
{
    // Currently no initialization needed
    return TRUE;
}

STDMETHODIMP CSDEnumFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDEnumFolderItemVerbs, IEnumVARIANT),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CSDEnumFolderItemVerbs::AddRef(void)
{
    ++m_cRef;
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSDEnumFolderItemVerbs::Release(void)
{
    ULONG       cRefT;

    cRefT=--m_cRef;

    if (0L==m_cRef)
        delete this;

    return cRefT;
}

STDMETHODIMP CSDEnumFolderItemVerbs::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG       cReturn=0L;
    HRESULT     hr;
    FolderItemVerb *pidv;

    TraceMsg(TF_SHELLAUTO, "CSDEnumFolderItemVerbs::Next called");
    if (!pulVar)
    {
        if (cVar != 1)
            return E_POINTER;
    }
    else
        *pulVar=0L;

    if (!pVar || m_iCur >= GetMenuItemCount(m_psdfiv->m_hmenu))
        return S_FALSE;

    while (m_iCur < GetMenuItemCount(m_psdfiv->m_hmenu) && cVar > 0)
    {
        hr=CSDFolderItemVerb_Create(m_psdfiv, GetMenuItemID(m_psdfiv->m_hmenu, m_iCur), &pidv);
        m_iCur++;
        if (SUCCEEDED(hr))
        {
            pVar->pdispVal=pidv;
            pVar->vt = VT_DISPATCH;
            pVar++;
            cReturn++;
            cVar--;
        }
    }


    if (NULL!=pulVar)
        *pulVar=cReturn;

    return NOERROR;
}


STDMETHODIMP CSDEnumFolderItemVerbs::Skip(ULONG cSkip)
{
    if ((int)(m_iCur+cSkip) >= GetMenuItemCount(m_psdfiv->m_hmenu))
        return S_FALSE;

    m_iCur+=cSkip;
    return NOERROR;
}


STDMETHODIMP CSDEnumFolderItemVerbs::Reset(void)
{
    m_iCur=0;
    return NOERROR;
}


STDMETHODIMP CSDEnumFolderItemVerbs::Clone(LPENUMVARIANT *ppEnum)
{
    *ppEnum = NULL;

    HRESULT hres = E_OUTOFMEMORY;
    CSDEnumFolderItemVerbs *pNew = new CSDEnumFolderItemVerbs(m_psdfiv);
    if (pNew)
    {
        if (pNew->Init())
        {
            hres = pNew->QueryInterface(IID_IEnumVARIANT, (void **)ppEnum);
        }
        pNew->Release();
    }

    return hres;
}

//==========================================================================
// Implemention of the FolderItemVerb...

HRESULT CSDFolderItemVerb_Create( CSDFolderItemVerbs *psdfivs, UINT id, FolderItemVerb ** ppid)
{
    *ppid = NULL;

    HRESULT hres = E_OUTOFMEMORY;
    CSDFolderItemVerb* psdfiv = new CSDFolderItemVerb(psdfivs, id);
    if (psdfiv)
    {
        hres = psdfiv->QueryInterface(IID_FolderItemVerb, (void **)ppid);
        psdfiv->Release();
    }

    return hres;
}

CSDFolderItemVerb::CSDFolderItemVerb(CSDFolderItemVerbs *psdfivs, UINT id) :
    m_cRef(1), m_psdfivs(psdfivs), m_id(id),
    CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItemVerbs)
{
    m_psdfivs->AddRef();
    DllAddRef();
}


CSDFolderItemVerb::~CSDFolderItemVerb(void)
{
    DllRelease();
    TraceMsg(TF_SHELLAUTO, "CSDFolderItemVerb::~CSDFolderItemVerb called");
    m_psdfivs->Release();
}

STDMETHODIMP CSDFolderItemVerb::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDFolderItemVerb, FolderItemVerb),
        QITABENTMULTI(CSDFolderItemVerb, IDispatch, FolderItemVerb),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDFolderItemVerb::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDFolderItemVerb::Release(void)
{
    //CCosmoClient deletes this object during shutdown
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


STDMETHODIMP CSDFolderItemVerb::get_Application(IDispatch **ppid)
{
    // walk this up the chain to get to FOLDER
    return m_psdfivs->get_Application(ppid);
}

STDMETHODIMP CSDFolderItemVerb::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CSDFolderItemVerb::get_Name(BSTR *pbs)
{
    TCHAR szMenuText[MAX_PATH];

    GetMenuString(m_psdfivs->m_hmenu, m_id, szMenuText, ARRAYSIZE(szMenuText), MF_BYCOMMAND);
    *pbs = AllocBStrFromString(szMenuText);

    return NOERROR;
}



STDMETHODIMP CSDFolderItemVerb::DoIt()
{
    CMINVOKECOMMANDINFO ici = {
        sizeof(CMINVOKECOMMANDINFO),
        0L,
        NULL/* m_psdf->m_hwnd */,
        NULL,
        NULL, NULL,
        SW_SHOWNORMAL,
    };

    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(m_id - CMD_ID_FIRST);
    return m_psdfivs->m_pcm->InvokeCommand(&ici);
}

