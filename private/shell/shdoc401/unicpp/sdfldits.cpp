#include "stdafx.h"
#pragma hdrstop

//#include "security.h"
//#include "sdspatch.h"

#define TF_SHELLAUTO TF_CUSTOM1
#undef TF_SHDLIFE
#define TF_SHDLIFE TF_CUSTOM2

class CSDEnumFldrItems;

class CSDFldrItems : public FolderItems,
                     public CObjectSafety,
                     protected CImpIDispatch
{
    friend CSDEnumFldrItems;

protected:
    ULONG           m_cRef; //Public for debug checks
    CSDFolder       *m_psdf;
    LPSHELLFOLDER   m_psfFldr;
    HDPA            m_hdpa;             // Dpa to hold windows handles...
    LPENUMIDLIST    m_penum;

    // Helper Functions
    void                _EnumItems(int iItemNeeded);

public:
    CSDFldrItems(CSDFolder *psdf);
    ~CSDFldrItems(void);

    BOOL         Init(BOOL fSelected);

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch members
    virtual STDMETHODIMP GetTypeInfoCount(UINT FAR* pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    //FolderItems methods
    STDMETHODIMP        get_Application(IDispatch * FAR* ppid);
    STDMETHODIMP        get_Parent (IDispatch * FAR* ppid);
    STDMETHODIMP        get_Count(long *plCount);
    STDMETHODIMP        Item(VARIANT, FolderItem**);
    STDMETHODIMP        _NewEnum(IUnknown **);

};


//Enumerator of whatever is held in the collection
class CSDEnumFldrItems : public IEnumVARIANT
{
protected:
    ULONG       m_cRef;
    CSDFldrItems *m_pfdritms;
    int         m_iCur;

public:
    CSDEnumFldrItems(CSDFldrItems *pfdritms);
    ~CSDEnumFldrItems();

    BOOL        Init(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IEnumFORMATETC members
    STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumVARIANT **);
};

HRESULT CSDFldrItems_Create(CSDFolder *psdf, BOOL fSelected, FolderItems **ppid)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppid = NULL;

    CSDFldrItems* psdfi = new CSDFldrItems(psdf);
    if (psdfi)
    {
        if (psdfi->Init(fSelected))
        {
            hres = psdfi->QueryInterface(IID_FolderItems, (void **)ppid);
        }
        psdfi->Release();
    }
    return hres;
}

/*
 * CSDFldrItems::CSDFldrItems
 * CSDFldrItems::~CSDFldrItems
 *
 * Constructor Parameters:
 *  pCL             PCCosmoClient to the client object that we use
 *                  to implement much of this interface.
 */

CSDFldrItems::CSDFldrItems(CSDFolder *psdf) :
    m_cRef(1), m_hdpa(NULL),m_psdf(psdf), 
    m_penum(NULL), CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItems)
{
    TraceMsg(TF_SHDLIFE, "ctor CSDFldrItems");
    DllAddRef();
    m_psdf->AddRef();
    return;
}


CSDFldrItems::~CSDFldrItems(void)
{
    TraceMsg(TF_SHDLIFE, "dtor CSDFldrItems");
    DllRelease();
    if (m_hdpa)
    {
        int i;
        for (i=DPA_GetPtrCount(m_hdpa) - 1; i>=0; i--)
        {
            ILFree((LPITEMIDLIST)DPA_FastGetPtr(m_hdpa, i));
        }
        ::DPA_Destroy(m_hdpa);
    }

    if (m_penum)
        m_penum->Release();

    m_psdf->Release();

    // If this is a case where we have a window, tell it we are done...
#ifdef NEED_INPROC_MESSAGE
    // Not needed unless we desire to get this to work in browser only mode...
    if (m_psdf->m_hwnd)
        SHShellInproc_Message(m_psdf->m_hwnd, SIPM_RELEASE, 0L);
#endif

    return;
}

BOOL CSDFldrItems::Init(BOOL fSelected)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItems::Init(%x)called", fSelected);

    // If we have a window tell it we are here...
#ifdef NEED_INPROC_MESSAGE
    if (m_psdf->m_hwnd && !SHShellInproc_Message(m_psdf->m_hwnd, SIPM_ADDREF, 0L))
        return FALSE;
#endif

    m_hdpa = ::DPA_Create(0);
    if (!m_hdpa)
        return FALSE;

    m_psdf->_GetShellFolder();   // make sure we have it.

    // Now lets loop through and get all of  the items...
    if (fSelected)
    {
        if (!m_psdf->m_hwnd)
            return E_FAIL;

#ifdef NEED_INPROC_MESSAGE
        LPITEMIDLIST pidl;
        DWORD dwProcessID = GetCurrentProcessId();
        HANDLE hData = (LPITEMIDLIST*)SHShellInproc_Message(m_psdf->m_hwnd, SIPM_GETSELECTEDOBJECTS,
                dwProcessID);
        if (hData && (pidl = (LPITEMIDLIST)SHLockShared(hData, dwProcessID)))
        {
            LPITEMIDLIST pidlT = pidl;
            while (pidlT->mkid.cb)
            {
                int cb = ILGetSize(pidlT);
                LPITEMIDLIST pidlNew = ILClone(pidlT);
                if (pidlNew)
                    ::DPA_InsertPtr(m_hdpa, 0xffff, pidlNew);
                pidlT = (LPITEMIDLIST)(((LPBYTE)pidlT) + cb);
            }
            SHUnlockShared(pidl);
            SHFreeShared(hData, dwProcessID);
        }
#else
        // In Integrated mode we are already in shell context to process this...
        // For now we will not support browser only mode
        if (WhichPlatform() == PLATFORM_IE3)
            return FALSE;

        LPITEMIDLIST *ppidl;
        int cpidls = (int)SHShellFolderView_Message(m_psdf->m_hwnd, SFVM_GETSELECTEDOBJECTS,
                (LPARAM)&ppidl);
        if (cpidls > 0)
        {
            // now walk through and move into one block...
            LPITEMIDLIST *ppidlT = ppidl;
            int i;
            for (i=0; i < cpidls; i++)
            {
                if (*ppidlT)
                {
                    LPITEMIDLIST pidlNew = ILClone(*ppidlT);
                    if (pidlNew)
                        ::DPA_InsertPtr(m_hdpa, 0xffff, pidlNew);
                }
                ppidlT++;
            }

            LocalFree(ppidl);
        }
#endif
    }
    else
    {
        // This is a simple enumerator which we want to enum the items
        // externally from a folder window.
        if (FAILED(m_psdf->m_psf->EnumObjects(HWND_DESKTOP, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &m_penum)))
            return FALSE;
    }

    return TRUE;
}

void CSDFldrItems::_EnumItems(int iItemNeeded)
{
    ULONG cFetched;
    LPITEMIDLIST pidl;
    if (!m_penum)
        return;

    if (((int)DPA_GetPtrCount(m_hdpa) - 1) >= iItemNeeded)
        return; // Already there...

    while (SUCCEEDED(m_penum->Next(1, &pidl, &cFetched)) && (cFetched == 1))
    {
        if (::DPA_InsertPtr(m_hdpa, 0xffff, pidl) >= iItemNeeded)
            return;
    }
    m_penum->Release();
    m_penum = NULL;
}

STDMETHODIMP CSDFldrItems::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    static const QITAB qit[] = {
        QITABENT(CSDFldrItems, FolderItems),
        QITABENTMULTI(CSDFldrItems, IDispatch, FolderItems),
        QITABENT(CSDFldrItems, IObjectSafety),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDFldrItems::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDFldrItems::Release(void)
{
    //CCosmoClient deletes this object during shutdown
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//The FolderItems implementation

STDMETHODIMP CSDFldrItems::get_Application(IDispatch * FAR* ppid)
{
    // let the folder object do the work...
    return m_psdf->get_Application(ppid);
}

STDMETHODIMP CSDFldrItems::get_Parent (IDispatch * FAR* ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CSDFldrItems::get_Count(long *plCount)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItems::Get_Count called");
    // Well it looks like we need to finish the iteration now to get this!
    _EnumItems(0x7fffffff);         // This should make sure we have everything now
    *plCount = DPA_GetPtrCount(m_hdpa);
    return NOERROR;
}

/*
 * CSDFldrItems::Item
 * Method
 *
 * This is essentially an array lookup operator for the collection.
 * Collection.Item by itself the same as the collection itself.
 * Otherwise you can refer to the item by index or by path, which
 * shows up in the VARIANT parameter.  We have to check the type
 * of the variant to see if it's VT_I4 (an index) or VT_BSTR (a
 * path) and do the right thing.
 */
STDMETHODIMP CSDFldrItems::Item(VARIANT index, FolderItem **ppid)
{
    HRESULT hres = NOERROR;
    *ppid = NULL;
    TraceMsg(TF_SHELLAUTO, "CSDFldrItems::Get_Item called");

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF|VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;

    switch (index.vt)
    {
    case VT_ERROR:
        {
            // No Parameters, generate a folder item for the folder itself...
            Folder * psdfParent;
            hres = m_psdf->get_ParentFolder(&psdfParent);
            if (SUCCEEDED(hres) && psdfParent)
            {
                hres = CSDFldrItem_Create((CSDFolder*)psdfParent, ILFindLastID(m_psdf->m_pidl), ppid);
                psdfParent->Release();  // we don't need access anymore...
            }
        }
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        _EnumItems(index.lVal);      // Get the asked for item...
        if ((index.lVal >= 0) && (index.lVal < DPA_GetPtrCount(m_hdpa)))
        {
            hres = CSDFldrItem_Create(m_psdf, (LPITEMIDLIST)DPA_FastGetPtr(m_hdpa, index.lVal), ppid);
            if (FAILED(hres))
                *ppid = NULL;
        }
        else
            hres = E_INVALIDARG;

        break;
    case VT_BSTR:
        {
            ULONG ulEaten;
            LPITEMIDLIST pidl;
            hres = m_psdf->m_psf->ParseDisplayName(m_psdf->m_hwnd, NULL, index.bstrVal, &ulEaten, &pidl, NULL);
            if (SUCCEEDED(hres))
            {
                hres = CSDFldrItem_Create(m_psdf, pidl, ppid);
            }
        }
        break;

    default:
        return E_NOTIMPL;
    }

    if (SUCCEEDED(hres) && ppid && _dwSafetyOptions)
        hres = MakeSafeForScripting((IUnknown**)ppid);
    return hres;
}

/*
 * CSDFldrItems::_NewEnum
 * Method
 *
 * Creates and returns an enumerator of the current list of
 * figures in this collection.
 */

STDMETHODIMP CSDFldrItems::_NewEnum(IUnknown **ppunk)
{
    TraceMsg(TF_SHELLAUTO, "CSDFldrItems::_NewEnum called");
    CSDEnumFldrItems *pNew = new CSDEnumFldrItems(this);
    if (NULL != pNew)
    {
        if (!pNew->Init())
        {
            delete pNew;
            pNew = NULL;
        }
    }

    *ppunk = pNew;
    return NOERROR;
}

// CSDEnumFldrItems implementation of IEnumVARIANT

CSDEnumFldrItems::CSDEnumFldrItems(CSDFldrItems *pfdritms) :
    m_cRef(1), m_pfdritms(pfdritms), m_iCur(0)
{
    m_pfdritms->AddRef();
    DllAddRef();
}


CSDEnumFldrItems::~CSDEnumFldrItems(void)
{
    DllRelease();
    m_pfdritms->Release();
}



/*
 * CSDEnumFldrItems::Init
 *
 * Purpose:
 *  Initializes the enumeration with any operations that might fail.
 *
 * Parameters:
 *  hListPDoc       HWND of the listbox containing the current
 *                  set of document pointers.
 *  fClone          BOOL indicating if this is a clone creation
 *                  in which case hListPDoc already has what we want
 *                  and we can just copy it directly.
 *
 * Return Value:
 *  BOOL            TRUE if initialization succeeded,
 *                  FALSE otherwise.
 */
BOOL CSDEnumFldrItems::Init()
{
    return TRUE;
}



/*
 * CSDEnumFldrItems::QueryInterface
 * CSDEnumFldrItems::AddRef
 * CSDEnumFldrItems::Release
 */

STDMETHODIMP CSDEnumFldrItems::QueryInterface(REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDEnumFldrItems, IEnumVARIANT),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CSDEnumFldrItems::AddRef(void)
{
    ++m_cRef;
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSDEnumFldrItems::Release(void)
{
    ULONG       cRefT;

    cRefT=--m_cRef;

    if (0L==m_cRef)
        delete this;

    return cRefT;
}


/*
 * CSDEnumFldrItems::Next
 * CSDEnumFldrItems::Skip
 * CSDEnumFldrItems::Reset
 * CSDEnumFldrItems::Clone
 *
 * Standard enumerator members for IEnumVARIANT
 */

STDMETHODIMP CSDEnumFldrItems::Next(ULONG cVar, VARIANT *pVar
    , ULONG *pulVar)
{
    ULONG       cReturn=0L;
    HRESULT     hr;
    FolderItem *pid;

    TraceMsg(TF_SHELLAUTO, "CSDEnumFldrItems::Next called");
    if (!pulVar)
    {
        if (cVar != 1)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulVar=0L;

    m_pfdritms->_EnumItems(m_iCur+cVar-1);

    if (!pVar || m_iCur >= DPA_GetPtrCount(m_pfdritms->m_hdpa))
        return ResultFromScode(S_FALSE);

    while (m_iCur < DPA_GetPtrCount(m_pfdritms->m_hdpa) && cVar > 0)
    {
        hr=CSDFldrItem_Create(m_pfdritms->m_psdf,
                (LPITEMIDLIST)DPA_FastGetPtr(m_pfdritms->m_hdpa, m_iCur),
                &pid);
        m_iCur++;

        if (m_pfdritms->_dwSafetyOptions && SUCCEEDED(hr))
            hr = MakeSafeForScripting((IUnknown**)&pid);

        if (SUCCEEDED(hr))
        {
            pVar->pdispVal=pid;
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


STDMETHODIMP CSDEnumFldrItems::Skip(ULONG cSkip)
{
    if ((int)(m_iCur+cSkip) >= DPA_GetPtrCount(m_pfdritms->m_hdpa))
        return ResultFromScode(S_FALSE);

    m_iCur+=cSkip;
    return NOERROR;
    return ResultFromScode(S_FALSE);
}


STDMETHODIMP CSDEnumFldrItems::Reset(void)
{
    m_iCur=0;
    return NOERROR;
}


STDMETHODIMP CSDEnumFldrItems::Clone(LPENUMVARIANT *ppEnum)
{
    CSDEnumFldrItems *pNew = new CSDEnumFldrItems(m_pfdritms);
    if (NULL != pNew)
    {
        if (!pNew->Init())
        {
            delete pNew;
            pNew = NULL;
        }
        else
            pNew->AddRef();
    }

    *ppEnum = (IEnumVARIANT *)pNew;
    return NULL != pNew ? NOERROR : E_OUTOFMEMORY;
}

