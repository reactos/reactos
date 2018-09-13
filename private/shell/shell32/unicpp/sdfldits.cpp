#include "stdafx.h"
#pragma hdrstop

class CEnumFolderItems;

class CFolderItems : public FolderItems2,
                     public CObjectSafety,
                     protected CImpIDispatch
{
    friend CEnumFolderItems;

public:
    CFolderItems(CFolder *psdf, BOOL fSelected);

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch members
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    //FolderItems methods
    STDMETHODIMP        get_Application(IDispatch **ppid);
    STDMETHODIMP        get_Parent (IDispatch **ppid);
    STDMETHODIMP        get_Count(long *pCount);
    STDMETHODIMP        Item(VARIANT, FolderItem**);
    STDMETHODIMP        _NewEnum(IUnknown **);

    //FolderItems2 methods
    STDMETHODIMP        InvokeVerbEx(VARIANT vVerb, VARIANT vArgs);

private:
    ~CFolderItems(void);
    HDPA _GetHDPA();
    UINT _GetHDPACount();
    BOOL_PTR _CopyItem(UINT iItem, LPCITEMIDLIST pidl);

    HRESULT _GetShellFolderView(IShellFolderView **ppsfv);

    LONG _cRef;
    CFolder *_psdf;
    HDPA _hdpa;
    BOOL _fSelected;
    BOOL _fGotAllItems;
    IEnumIDList *_penum;
    UINT _cNumEnumed;

    HRESULT _EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidl);
};


//Enumerator of whatever is held in the collection
class CEnumFolderItems : public IEnumVARIANT
{
public:
    CEnumFolderItems(CFolderItems *pfdritms);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IEnumFORMATETC members
    STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumVARIANT **);

private:
    ~CEnumFolderItems();

    LONG _cRef;
    CFolderItems *_pfdritms;
    UINT _iCur;
};

HRESULT CFolderItems_Create(CFolder *psdf, BOOL fSelected, FolderItems **ppitems)
{
    *ppitems = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CFolderItems* psdfi = new CFolderItems(psdf, fSelected);
    if (psdfi)
    {
        hr = psdfi->QueryInterface(IID_PPV_ARG(FolderItems, ppitems));
        psdfi->Release();
    }
    return hr;
}

CFolderItems::CFolderItems(CFolder *psdf, BOOL fSelected) :
    _cRef(1), _psdf(psdf), _fSelected(fSelected), CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_FolderItems2)
{
    DllAddRef();
    _psdf->AddRef();

    ASSERT(_hdpa == NULL);
}

int FreePidlCallBack(void *pv, void *)
{
    ILFree((LPITEMIDLIST)pv);
    return 1;
}

CFolderItems::~CFolderItems(void)
{
    if (_hdpa)
        DPA_DestroyCallback(_hdpa, FreePidlCallBack, 0);

    ATOMICRELEASE(_penum);  // may be NULL

    _psdf->Release();
    DllRelease();
}

HDPA CFolderItems::_GetHDPA()
{
    if (NULL == _hdpa)
        _hdpa = ::DPA_Create(0);
    return _hdpa;
}

UINT CFolderItems::_GetHDPACount()
{
    UINT count = 0;
    HDPA hdpa = _GetHDPA();
    if (hdpa)
        count = DPA_GetPtrCount(hdpa);
    return count;
}

HRESULT CFolderItems::_GetShellFolderView(IShellFolderView **ppsfv)
{
    return IUnknown_QueryService(_psdf->_punkSite, SID_DefView, IID_PPV_ARG(IShellFolderView, ppsfv));
}

BOOL_PTR CFolderItems::_CopyItem(UINT iItem, LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlT = ILClone(pidl);
    if (pidlT)
    {
        if (-1 == ::DPA_InsertPtr(_hdpa, iItem, pidlT))
        {
            ILFree(pidlT);
            pidlT = NULL;   // failure
        }
    }
    return (BOOL_PTR)pidlT;
}

// in:
//      iItemNeeded     zero based index
//          
//
// returns:
//      S_OK        in range value
//      S_FALSE     out of range value
//

HRESULT CFolderItems::_EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidlItem)
{
    HRESULT hr = S_FALSE;   // assume out of range

    if (_GetHDPA())
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_GetPtr(_hdpa, iItemNeeded);
        if (pidl)
        {
            if (ppidlItem)
                *ppidlItem = pidl;
            hr = S_OK;
        }
        else if (!_fGotAllItems)
        {
            IShellFolderView *psfv;
            if (SUCCEEDED(_GetShellFolderView(&psfv)))
            {
                if (_fSelected)
                {
                    UINT cItems;
                    LPCITEMIDLIST *ppidl = NULL;
                    if (SUCCEEDED(psfv->GetSelectedObjects(&ppidl, &cItems)) && ppidl)
                    {
                        for (UINT i = 0; i < cItems; i++)
                        {
                            _CopyItem(i, ppidl[i]);
                        }
                        LocalFree(ppidl);
                    }
                    _fGotAllItems = TRUE;
                    hr = _EnsureItem(iItemNeeded, ppidlItem);
                }
                else
                {
                    UINT cItems;
                    if (SUCCEEDED(psfv->GetObjectCount(&cItems)))
                    {
                        if (iItemNeeded < cItems)
                        {
                            LPCITEMIDLIST pidl;
                            if (SUCCEEDED(GetObjectSafely(psfv, &pidl, iItemNeeded)))
                            {
                                if (_CopyItem(iItemNeeded, pidl))
                                    hr = _EnsureItem(iItemNeeded, ppidlItem);
                            }
                        }
                    }
                }
                psfv->Release();
            }
            else
            {
                if (NULL == _penum)
                    _psdf->m_psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &_penum);

                if (NULL == _penum)
                {
                    _fGotAllItems = TRUE;   // enum empty, we are done
                }
                else 
                {
                    // get more while our count is less than the index
                    while (_cNumEnumed <= iItemNeeded)
                    {
                        LPITEMIDLIST pidl;
                        if (S_OK == _penum->Next(1, &pidl, NULL))
                        {
                            if (-1 != ::DPA_AppendPtr(_hdpa, pidl))
                                _cNumEnumed++;
                            else
                                ILFree(pidl);
                        }
                        else
                        {
                            ATOMICRELEASE(_penum);
                            _fGotAllItems = TRUE;
                            break;
                        }
                    }
                }
                hr = _EnsureItem(iItemNeeded, ppidlItem);
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CFolderItems::QueryInterface(REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItems, FolderItems2),
        QITABENTMULTI(CFolderItems, FolderItems, FolderItems2),
        QITABENTMULTI(CFolderItems, IDispatch, FolderItems2),
        QITABENT(CFolderItems, IObjectSafety),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItems::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderItems::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// FolderItems implementation

STDMETHODIMP CFolderItems::get_Application(IDispatch **ppid)
{
    // let the folder object do the work...
    return _psdf->get_Application(ppid);
}

STDMETHODIMP CFolderItems::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItems::get_Count(long *pCount)
{
    IShellFolderView *psfv;
    HRESULT hr = _GetShellFolderView(&psfv);
    if (SUCCEEDED(hr))
    {
        UINT cCount;
        hr = _fSelected ? psfv->GetSelectedCount(&cCount) : psfv->GetObjectCount(&cCount);
        *pCount = cCount;
        psfv->Release();
    }
    else
    {
        // Well it looks like we need to finish the iteration now to get this!
        *pCount = SUCCEEDED(_EnsureItem(-1, NULL)) ? _GetHDPACount() : 0;
        hr = S_OK;
    }
    return hr;
}

// Folder.Items.Item(1)
// Folder.Items.Item("file name")
// Folder.Items.Item()      - same as Folder.Self

STDMETHODIMP CFolderItems::Item(VARIANT index, FolderItem **ppid)
{
    HRESULT hr = S_FALSE;
    *ppid = NULL;

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF | VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;

    switch (index.vt)
    {
    case VT_ERROR:
        {
            // No Parameters, generate a folder item for the folder itself...
            Folder * psdfParent;
            hr = _psdf->get_ParentFolder(&psdfParent);
            if (SUCCEEDED(hr) && psdfParent)
            {
                hr = CFolderItem_Create((CFolder*)psdfParent, ILFindLastID(_psdf->m_pidl), ppid);
                psdfParent->Release();
            }
        }
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        {
            LPCITEMIDLIST pidl;
            hr = _EnsureItem(index.lVal, &pidl);      // Get the asked for item...
            if (S_OK == hr)
                hr = CFolderItem_Create(_psdf, pidl, ppid);
        }
        break;

    case VT_BSTR:
        {
            LPITEMIDLIST pidl;
            hr = _psdf->m_psf->ParseDisplayName(NULL, NULL, index.bstrVal, NULL, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                hr = CFolderItem_Create(_psdf, pidl, ppid);
                ILFree(pidl);
            }
        }
        break;

    default:
        return E_NOTIMPL;
    }

    if (hr != S_OK)   // Error values cause problems in Java script
    {
        *ppid = NULL;
        hr = S_FALSE;
    }
    else if (ppid && _dwSafetyOptions)
    {
        hr = MakeSafeForScripting((IUnknown**)ppid);
    }
    return hr;
}

STDMETHODIMP CFolderItems::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    long cItems;
    HRESULT hr = get_Count(&cItems);
    if (SUCCEEDED(hr) && cItems)
    {
        LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(*ppidl) * cItems);
        if (ppidl)
        {
            for (int i = 0; i < cItems; i++)
            {
                _EnsureItem(i, &ppidl[i]);
            }

            hr = InvokeVerbHelper(vVerb, vArgs, ppidl, cItems, _dwSafetyOptions, _psdf);

            LocalFree(ppidl);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}


// supports VB "For Each" statement

STDMETHODIMP CFolderItems::_NewEnum(IUnknown **ppunk)
{
    *ppunk = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItems *pNew = new CEnumFolderItems(this);
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        pNew->Release();
    }
    return hr;
}

// CEnumFolderItems implementation of IEnumVARIANT

CEnumFolderItems::CEnumFolderItems(CFolderItems *pfdritms) :
    _cRef(1), _pfdritms(pfdritms), _iCur(0)
{
    _pfdritms->AddRef();
    DllAddRef();
}


CEnumFolderItems::~CEnumFolderItems(void)
{
    _pfdritms->Release();
    DllRelease();
}

STDMETHODIMP CEnumFolderItems::QueryInterface(REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumFolderItems, IEnumVARIANT),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CEnumFolderItems::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CEnumFolderItems::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CEnumFolderItems::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG cReturn = 0;
    HRESULT hr = S_OK;

    if (!pulVar && (cVar != 1))
        return E_POINTER;

    while (cVar)
    {
        LPCITEMIDLIST pidl;
        
        if (S_OK == _pfdritms->_EnsureItem(_iCur + cVar - 1, &pidl))
        {
            FolderItem *pid;

            hr = CFolderItem_Create(_pfdritms->_psdf, pidl, &pid);
            _iCur++;

            if (_pfdritms->_dwSafetyOptions && SUCCEEDED(hr))
                hr = MakeSafeForScripting((IUnknown**)&pid);

            if (SUCCEEDED(hr))
            {
                pVar->pdispVal = pid;
                pVar->vt = VT_DISPATCH;
                pVar++;
                cReturn++;
                cVar--;
            }
            else
                break;
        }
        else
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (pulVar)
            *pulVar = cReturn;
        hr = cReturn ? S_OK : S_FALSE;
    }

    return hr;
}

STDMETHODIMP CEnumFolderItems::Skip(ULONG cSkip)
{
    if ((_iCur + cSkip) >= _pfdritms->_GetHDPACount())
        return S_FALSE;

    _iCur += cSkip;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItems::Reset(void)
{
    _iCur = 0;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItems::Clone(IEnumVARIANT **ppenum)
{
    *ppenum = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItems *pNew = new CEnumFolderItems(_pfdritms);
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_PPV_ARG(IEnumVARIANT, ppenum));
        pNew->Release();
    }
    return hr;
}

