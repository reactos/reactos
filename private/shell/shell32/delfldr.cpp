#include "shellprv.h"
#include "clsobj.h"
#include "caggunk.h"
#pragma hdrstop


// Delegated IShellFolder object.  Takes an ISF inner and adds the list of
// delegated shell folders to its namespace, wrapping the IDLISTs as required.

class CDelegateFolder : public CAggregatedUnknown, IDelegateShellFolder, IShellFolder2, IPersistFreeThreadedObject
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
                { return CAggregatedUnknown::QueryInterface(riid, ppv); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // IPersistFreeThreadedObject
    STDMETHODIMP GetClassID(CLSID *pCLSID);

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                  ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwnd, REFIID riid, void **ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                               REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                           DWORD uFlags, LPITEMIDLIST *ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapNameToSCID(LPCWSTR pwszName, SHCOLUMNID *pscid);

    // *** IDelegateShellFolder ***
    STDMETHODIMP Initialize(WORD id, IShellFolder2* psf);
    STDMETHODIMP AddFolders(CLSID* aCLISD, INT count);

protected:
    CDelegateFolder(IUnknown *punkOuter);
    ~CDelegateFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid,void **ppv);
    
    LPCITEMIDLIST _GetFolderIDList();
    PDELEGATEITEMID _IsDelegateObject(LPCITEMIDLIST pidl, LPCLSID pclsid);
    HRESULT _InitFolder(IUnknown *punk);
    HRESULT _GetDelegateFolder(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
    HRESULT _GetItemFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf);
    HRESULT _GetItemFolder2(LPCITEMIDLIST pidl, IShellFolder2 **ppsf);

private:
    HDCA            _dcaDelegates;              // array of CLSIDs we are using

    IShellFolder2   *_psfOuter;                 // IShellFolder2 of the inner object
    WORD            _id;                        // ID used for marking our IDLISTs

    BOOL            _fFreeThread:1;             // supports the IPersistFreeThreadedObject iface

    LPITEMIDLIST    _pidl;                      // IDLIST passed to our ::Initialize method

    friend HRESULT CDelegateFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut);
};  


//-----------------------------------------------------------------------------
// Stuff
//-----------------------------------------------------------------------------

// flow control helpers

#define ExitGracefully(_hr, _r)             \
            { _hr = (_r); goto exit; }

#define FailGracefully(_hr)                 \
            { if (FAILED(hr)) goto exit; }
                

STDAPI CDelegateMalloc_Create(void *pv, UINT cbSize, WORD wOuter, IMalloc **ppmalloc);

class CDelegateFolderEnum : public IEnumIDList
{

public:
    CDelegateFolderEnum(HWND hwnd, DWORD grfFlags, HDPA dpaFolders);
    ~CDelegateFolderEnum();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IEnumIDList methods ***
    STDMETHOD(Next)  (ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched);
    STDMETHOD(Skip)  (ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (IEnumIDList **ppenum);

private:
    LONG _cRef;
    HWND _hwnd;
    DWORD _grfFlags;
    HDPA _dpaFolders;
    INT _index;
    IEnumIDList* _pCurrentEnum;
};


//
// get the clsid of the delegate from the pidl.
//

#define _GetDelegateCLSID(pdi)               \
            (*(UNALIGNED CLSID*)&(((PDELEGATEITEMID)(pdi))->rgb[((PDELEGATEITEMID)(pdi))->cbInner]))


//
// callback used to comapre two delegate item idlists CLSID
//

INT _CompareDelegateItem(void *pv1, void *pv2, LPARAM lParam)
{
    PDELEGATEITEMID pdi1 = (PDELEGATEITEMID)pv1;
    PDELEGATEITEMID pdi2 = (PDELEGATEITEMID)pv2;

    return memcmp((UNALIGNED CLSID*)&(pdi1->rgb[pdi1->cbInner]), 
                  (UNALIGNED CLSID*)&(pdi2->rgb[pdi2->cbInner]),
                  SIZEOF(CLSID));
        
}


//
// callback used to destroy the IUnkown DPA.
//

static INT _ReleaseCB(void *pItem, void *pData)
{
    IUnknown *pUnknown = (IUnknown *)pItem;
    pUnknown->Release();
    return 1;
}


//-----------------------------------------------------------------------------
// Constructors etc
//-----------------------------------------------------------------------------

// Constructor
CDelegateFolder::CDelegateFolder(IUnknown *punkOuter) : 
    CAggregatedUnknown(punkOuter),
    _dcaDelegates(NULL),
    _psfOuter(NULL), 
    _id(0),
    _fFreeThread(FALSE),
    _pidl(NULL)
{
    DllAddRef();
}

CDelegateFolder::~CDelegateFolder()
{
    if ( _dcaDelegates )
        DCA_Destroy(_dcaDelegates);
   
    ILFree(_pidl);

    DllRelease();
}


//
// get the pidl used to initialize this namespace
//

LPCITEMIDLIST CDelegateFolder::_GetFolderIDList()
{
    if (!_pidl)
        SHGetIDListFromUnk(_psfOuter, &_pidl);

    return _pidl;
}


//
// aggregated unknown handling
//

HRESULT CDelegateFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDelegateFolder, IDelegateShellFolder),                // IID_IDelegateShellFolder
        QITABENTMULTI(CDelegateFolder, IShellFolder, IShellFolder2),    // IID_IShellFolder
        QITABENT(CDelegateFolder, IShellFolder2),                       // IID_IShellFolder2
        QITABENT(CDelegateFolder, IPersistFreeThreadedObject),          // IID_IPersistFreeThreadedObject
        { 0 },
    };

    if ( IsEqualIID(riid, IID_IPersistFreeThreadedObject) && !_fFreeThread )
        return E_NOINTERFACE;

    return QISearch(this, qit, riid, ppv);
}


//-----------------------------------------------------------------------------
// IDelegateShellFolder
//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::Initialize(WORD id, IShellFolder2* psf)
{
    // ensure we have the DCA for storing the delegate folders into, then lets grav the ISF
    // we have been given.
    
    if ( !_dcaDelegates )
    {
        _dcaDelegates = DCA_Create();
        if ( !_dcaDelegates )
            return E_OUTOFMEMORY;
    }

    ASSERT(NULL != psf);
    // NOTE: AddRef() is not called because psf is required to be an interface on the
    // same aggregated object
    _psfOuter = psf;

    _id = id;

    // use the CLSID (from our delegate) as the location in the registry
    // where we look for the delegates to load

    CLSID clsid;
    if (SUCCEEDED(GetClassID(&clsid)))
    {
        HKEY hKey;
        if (SUCCEEDED(SHRegGetCLSIDKey(clsid, NULL, FALSE, FALSE, &hKey)))
        {
            DCA_AddItemsFromKey(_dcaDelegates, hKey, TEXT("shellex\\DelegateShellFolders"));
            RegCloseKey(hKey);
        }
    }

    // is the inner object free threaded?  if so we must honor this IID as the regitems
    // code uses it to cache us.

    IPersistFreeThreadedObject* ppfto;
    if ( SUCCEEDED(_psfOuter->QueryInterface(IID_IPersistFreeThreadedObject, (void **)&ppfto)))
    {        
        _fFreeThread = TRUE;
        ppfto->Release();
    }

    return S_OK;
}

//-----------------------------------------------------------------------------

// given an array of CLISDs add them to the delegate object list

STDMETHODIMP CDelegateFolder::AddFolders(CLSID* aCLISD, INT count)
{
    INT i;

    // fail if either we don't have a delegate list to ad items to or, we have
    // no array of CLSIDs to add.

    if ( !aCLISD || !_dcaDelegates )
        return E_INVALIDARG;

    for ( i = 0 ; i < count ; i++ )
    {
        if ( !DCA_AddItem(_dcaDelegates, aCLISD[i]) )
            return E_FAIL;
    }    

    return S_OK;
}


/*-----------------------------------------------------------------------------
/   Crack the pidl and see if it looks like a delegate object, we return S_OK
/   if it is, otherwise we hand back S_FALSE.
/
/ In:
/   pidl -> pidl to be cracked
/   pclsid -> receives the CLSID for the object (if non-zero)
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
PDELEGATEITEMID CDelegateFolder::_IsDelegateObject(LPCITEMIDLIST pidl, LPCLSID pclsid)
{
    PDELEGATEITEMID pdi = (PDELEGATEITEMID)pidl;

    if ( pdi && (pdi->cbSize > SIZEOF(DELEGATEITEMID)-1) && (pdi->wOuter == _id) )
    {
        if ( pclsid )
            *pclsid = *((UNALIGNED CLSID*)&(pdi->rgb[pdi->cbInner]));
        return pdi;
    }

    return NULL;
}

HRESULT CDelegateFolder::_InitFolder(IUnknown *punk)
{
    HRESULT hr = S_OK;
    IPersistFolder* ppf;
    if ( SUCCEEDED(punk->QueryInterface(IID_IPersistFolder, (void **)&ppf)) )
    {
        hr = ppf->Initialize(_GetFolderIDList());
        ppf->Release();
    }
    return hr;
}

/*-----------------------------------------------------------------------------
/   Extract the delegate CLSID from the pidl we see, if the type is not a delegate
/   item then return failure.  if ppv is non-NULL then create an object from that
/   CLSID that maps to the thing we want.
/
/ In:
/   pidl = pidl to look into
/   pCLSID = receives the clisd
/   riid, ppv -> if you want an instance then pass the IID and a ppv
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDelegateFolder::_GetDelegateFolder(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr;   
    CLSID clsid;
    PDELEGATEITEMID pdi = _IsDelegateObject(pidl, &clsid);

    if (!pdi)
        return E_FAIL;

    // the caller wants an instance of this delegate folder, so lets co-create it and set
    // its allocator as required.

    IDelegateFolder* pDelegateFolder;
    hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_IDelegateFolder, (void **)&pDelegateFolder);
    FailGracefully(hr);

    IMalloc* pAlloc;
    if ( SUCCEEDED(CDelegateMalloc_Create(&clsid, SIZEOF(CLSID), _id, &pAlloc)) )
    {
        pDelegateFolder->SetItemAlloc(pAlloc);
        pAlloc->Release();
    }

    hr = pDelegateFolder->QueryInterface(riid, ppv);
    pDelegateFolder->Release();

    // the caller is requesting IShellFolder for this object, therefore we need to bind 
    // and ensure that its parent is correctly initialized (by calling IPersistFolder),
    // note that at this point just calling Initialize with our cached PIDL should be
    // sufficent.

    if ( SUCCEEDED(hr) && _GetFolderIDList() && 
            (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IShellFolder2)) )
    {
        _InitFolder((IUnknown *)*ppv);
    }

    hr = S_OK;          // success

exit:

    return hr;
}

HRESULT CDelegateFolder::_GetItemFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf)
{
    HRESULT hr;
    if (_IsDelegateObject(pidl, NULL))
    {
        hr = _GetDelegateFolder(pidl, IID_IShellFolder, (void **)ppsf);
    }
    else
    {
        *ppsf = _psfOuter;
        _psfOuter->AddRef();
        hr = S_OK;
    }
    return hr;
}

HRESULT CDelegateFolder::_GetItemFolder2(LPCITEMIDLIST pidl, IShellFolder2 **ppsf)
{
    HRESULT hr;
    if (_IsDelegateObject(pidl, NULL))
    {
        hr = _GetDelegateFolder(pidl, IID_IShellFolder2, (void **)ppsf);
    }
    else
    {
        *ppsf = _psfOuter;
        _psfOuter->AddRef();
        hr = S_OK;
    }
    return hr;
}


// IPersist method

STDMETHODIMP CDelegateFolder::GetClassID(CLSID *pCLSID)
{
    IPersist* pps;
    HRESULT hr = _psfOuter->QueryInterface(IID_IPersist, (void **)&pps);
    if ( SUCCEEDED(hr) )
    {
        hr = pps->GetClassID(pCLSID);
        pps->Release();
    }

    return hr;
}

//-----------------------------------------------------------------------------
// IShellFolder methods
//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                               ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    // round robbin the delegate namespaces, seeing if any want to parse the string,
    // if they don't they will return E_NOTIMPL or E_INVALIDARG.  If they return anything
    // other than that then we return their result, otherwise we continue.

    for (int i = 0 ; i < DCA_GetItemCount(_dcaDelegates) ; i++)
    {
        IShellFolder* psf;
        if ( SUCCEEDED(DCA_CreateInstance(_dcaDelegates, i, IID_IShellFolder, (void**)&psf)) )
        {   
            IDelegateFolder* pdf;
            if ( SUCCEEDED(psf->QueryInterface(IID_IDelegateFolder, (void**)&pdf)) )
            {
                IMalloc* pAlloc;
                if ( SUCCEEDED(CDelegateMalloc_Create((LPVOID)DCA_GetItem(_dcaDelegates, i), SIZEOF(CLSID), _id, &pAlloc)) )
                {
                    pdf->SetItemAlloc(pAlloc);
                    pAlloc->Release();
                }
                pdf->Release();
            }

            HRESULT hr = psf->ParseDisplayName(hwnd, pbc, pszDisplayName, pchEaten, ppidl, pdwAttributes);
            psf->Release();

            if ( (hr != E_INVALIDARG) && (hr != E_NOTIMPL) )
            {
                return hr;
            }
        }
    }

    // None of the delegates were interested so lets call the _psfOuter and let them have a crack at it!

    return _psfOuter->ParseDisplayName(hwnd, pbc, pszDisplayName,
                                       pchEaten, ppidl, pdwAttributes);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    CDelegateFolderEnum* pEnum;
    IDelegateFolder* pDelegateFolder;
    IMalloc* pAlloc;
    HDPA dpaFolders = NULL;
    INT i;

    *ppenumIDList = NULL; // in case of failure

    // to construct the enumerator we are going to give out to the oustside world we
    // must fill a DPA with the IShellFolder iface's we want to use.  Therefore lets
    // walk the list of delegate folders add those to the DPA (having both set their
    // malloc and then initialized them).

    dpaFolders = DPA_Create(4);

    if ( !dpaFolders )
        return E_OUTOFMEMORY;

    for ( i = 0 ; i < DCA_GetItemCount(_dcaDelegates) ; i++ )
    {
        if ( SUCCEEDED(DCA_CreateInstance(_dcaDelegates, i, IID_IDelegateFolder, (void **)&pDelegateFolder)) )
        {
            if ( SUCCEEDED(CDelegateMalloc_Create((LPVOID)DCA_GetItem(_dcaDelegates, i), SIZEOF(CLSID), _id, &pAlloc)) )
            {
                pDelegateFolder->SetItemAlloc(pAlloc);
                pAlloc->Release();

                _InitFolder((IUnknown *)pDelegateFolder);

                IShellFolder* psf;
                if ( SUCCEEDED(pDelegateFolder->QueryInterface(IID_IShellFolder, (void **)&psf)) )
                {
                    if ( -1 == DPA_AppendPtr(dpaFolders, psf) )
                    {
                        psf->Release();            // failed to place into the DPA.
                    }
                }
            }

            pDelegateFolder->Release();
        }
    }

    _psfOuter->AddRef();
    DPA_AppendPtr(dpaFolders, _psfOuter);           // BUGBUG: what about failure

    pEnum = new CDelegateFolderEnum(hwnd, grfFlags, dpaFolders);
    if ( !pEnum )
    {
        DPA_DestroyCallback(dpaFolders, _ReleaseCB, NULL);
        return E_OUTOFMEMORY;
    }

    *ppenumIDList = SAFECAST(pEnum, IEnumIDList*);      // pass out the enumerator
    return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc,
                                           REFIID riid, void **ppvOut)
{
    IShellFolder* psfItem;
    HRESULT hr = _GetItemFolder(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->BindToObject(pidl, pbc, riid, ppvOut);
        psfItem->Release();
    }
    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc,
                                            REFIID riid, void **ppv)
{
    IShellFolder* psfItem;
    HRESULT hr = _GetItemFolder(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->BindToStorage(pidl, pbc, riid, ppv);
        psfItem->Release();
    }
    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr;
    LPCITEMIDLIST pidlNext1 = _ILNext(pidl1);
    LPCITEMIDLIST pidlNext2 = _ILNext(pidl2);
    PDELEGATEITEMID pdi1 = _IsDelegateObject(pidl1, NULL);
    PDELEGATEITEMID pdi2 = _IsDelegateObject(pidl2, NULL);
    IShellFolder* psfItem1 = NULL;
    IShellFolder* psfItem2 = NULL;
    
    // if we have two delegate objects then attempt to compare, if we have a single then
    // the delegate comes first.  if neither of the items are delegates then lets pass
    // them onto the innerISF.

    if ( pdi1 )
    {
        INT iResult = 0;

        if ( pdi2 )
        {
            STRRET StrRet1, StrRet2;
            TCHAR szItemName1[MAX_PATH];
            TCHAR szItemName2[MAX_PATH];

            hr = _GetDelegateFolder(pidl1, IID_IShellFolder, (void **)&psfItem1);
            FailGracefully(hr);
        
            hr = _GetDelegateFolder(pidl2, IID_IShellFolder, (void **)&psfItem2);
            FailGracefully(hr);
            
            if ( FAILED(psfItem1->GetDisplayNameOf(pidl1, SHGDN_NORMAL, &StrRet1)) ||
                   FAILED(psfItem2->GetDisplayNameOf(pidl2, SHGDN_NORMAL, &StrRet2)) )
            {
                ExitGracefully(hr, E_FAIL);
            }

            StrRetToStrN(szItemName1, ARRAYSIZE(szItemName1), &StrRet1, pidl1);
            StrRetToStrN(szItemName2, ARRAYSIZE(szItemName2), &StrRet2, pidl2);

            iResult = lstrcmp(szItemName1,szItemName2);
        }
        else
        {
            iResult = 1;        
        }

        // their names match so lets check out the rest of the  data, starting with the CLSIDs, 
        // if they match then we must continue the binding process and let them be compared.

        if ( !iResult )
            iResult = memcmp(&pdi1->rgb[pdi1->cbInner], &pdi2->rgb[pdi2->cbInner], SIZEOF(CLSID));

        if ( iResult )
            ExitGracefully(hr, ResultFromShort(iResult));

        if ( ILIsEmpty(pidlNext1) )
        {
            if ( ILIsEmpty(pidlNext2) )
                ExitGracefully(hr, ResultFromShort(0));     // they are the same size
                
            ExitGracefully(hr, ResultFromShort(-1));        // pidl1 is shorter
        }
        else if ( ILIsEmpty(pidlNext2) )
            ExitGracefully(hr, ResultFromShort(1));         // pidl2 is shorter

        // call into the delegate namespace that owns these IDLISTs and let it compare

        if ( psfItem1 )
        {
            hr = _GetDelegateFolder(pidl1, IID_IShellFolder, (void **)&psfItem1);
            FailGracefully(hr);
        }

        hr = psfItem1->CompareIDs(lParam, pidl1, pidl2);        // done
    }
    else if ( pdi2 )
    {
        hr = ResultFromShort(-1);       // only pidl2 was a delegate
    }
    else 
    {
        // both items belong to the inner ISF, so lets pass them to that so they can be compared,
        // having done that we can just return the result from there.

        hr = _psfOuter->CompareIDs(lParam, pidl1, pidl2);
    }

exit:

    if ( psfItem1 )
        psfItem1->Release();

    if ( psfItem2 )
        psfItem2->Release();

    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppvOut)
{
    return _psfOuter->CreateViewObject(hwnd, riid, ppvOut);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut)
{
    HRESULT hr = E_FAIL;
    IShellFolder* psfItem;
    LPCITEMIDLIST* apidl2 = NULL;
    HDPA dpa = NULL;
    CLSID clsid;
    ULONG i, cidl2;

    if ( cidl == 1 )
    {
        // pass real IDLs through, otherwise get the delegate folder that
        // this idlist maps to and get attributes from it.

        IShellFolder* psfItem;
        hr = _GetItemFolder(*apidl, &psfItem);
        if (SUCCEEDED(hr))
        {
            hr = psfItem->GetAttributesOf(cidl, apidl, rgfInOut);
            psfItem->Release();
        }
    }
    else if ( cidl > 1 )
    {
        // the selection is large, so we build two lists, the first contains the items that
        // relate to the inner ISF, the second is a list of the delegate items.  the items 
        // destined for the inner ISF are stored in a LocalAlloc, the delegates are in a 
        // DPA.

        dpa = DPA_Create(4);
        apidl2 = (LPCITEMIDLIST*)LocalAlloc(LPTR, SIZEOF(LPCITEMIDLIST)*cidl);

        if ( !apidl2 || !dpa )
            ExitGracefully(hr, E_OUTOFMEMORY);

        for ( cidl2 = 0, i = 0 ; i != cidl ; i++ )
        {        
            if ( !_IsDelegateObject(apidl[i], NULL) )
            {
                apidl2[cidl2++] = apidl[i];
            }
            else
            {
                if ( -1 == DPA_AppendPtr(dpa, (LPVOID)apidl[i]) )
                    ExitGracefully(hr, E_OUTOFMEMORY);
            }
        }

        // call the innerISF with the items it is interested in.

        if ( cidl2 )
        {
            hr = _psfOuter->GetAttributesOf(cidl2, apidl2, rgfInOut);
            FailGracefully(hr);
        }

        // if we have any destined for the delegate namespace then lets
        // first sort that list (basedon the CLSID) and bind to the
        // namespace as required.  the initial sort avoids us having
        // to CoCreate the delegate object multiple times.

        if ( DPA_GetPtrCount(dpa) )
        {
            DPA_Sort(dpa, _CompareDelegateItem, NULL);

            for ( cidl2 = 0, i = 0 ; i != (ULONG)DPA_GetPtrCount(dpa) ; i++ )
            {
                // if we are no longer in the same CLSID and the list
                // is long we pass them onto the namespace

                if ( cidl2 &&
                       !IsEqualCLSID(clsid, _GetDelegateCLSID((PDELEGATEITEMID)DPA_GetPtr(dpa, i))) )
                {
                    hr = _GetDelegateFolder(apidl2[0], IID_IShellFolder, (void **)&psfItem);
                    FailGracefully(hr);

                    hr = psfItem->GetAttributesOf(cidl2, apidl2, rgfInOut);
                    psfItem->Release();

                    cidl2 = 0;      // no new items in the dpa copy yet!
                }

                // add this item to the list, if we have none
                // then take a snapshot of the GUID

                if ( !cidl2 )
                    clsid = _GetDelegateCLSID((PDELEGATEITEMID)DPA_GetPtr(dpa, i));

                apidl2[cidl2++] = (LPCITEMIDLIST)DPA_GetPtr(dpa, i);
            }

            if ( cidl2 )
            {
                // on exit ensure that we have passed out the remainng items.

                hr = _GetDelegateFolder(apidl2[0], IID_IShellFolder, (void **)&psfItem);
                FailGracefully(hr);

                hr = psfItem->GetAttributesOf(cidl2, apidl2, rgfInOut);
                psfItem->Release();
            }
        }
    }

exit:

    if ( dpa )
        DPA_Destroy(dpa);

    if ( apidl2 )
        LocalFree((HLOCAL)apidl2);

    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName)
{
    IShellFolder* psfItem;
    HRESULT hr = _GetItemFolder(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->GetDisplayNameOf(pidl, uFlags, pName);
        psfItem->Release();
    }
    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
                                        LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    IShellFolder* psfItem;
    HRESULT hr = _GetItemFolder(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->SetNameOf(hwnd, pidl, pszName, uFlags, ppidlOut);
        psfItem->Release();
    }
    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                          REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    HRESULT hr = E_FAIL;
    IShellFolder* psfItem;
    BOOL fDelegate = FALSE;
    UINT i, cidl2;
    LPCITEMIDLIST* apidl2 = NULL;

    if ( cidl == 1 )
    {
        // single selection is simple, we must just pass that down to the 
        // correct owner.

        fDelegate = (_IsDelegateObject(*apidl, NULL) != NULL);

        if ( fDelegate )
        {
            hr = _GetDelegateFolder(*apidl, IID_IShellFolder, (void **)&psfItem);
            FailGracefully(hr);

            hr = psfItem->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);
            psfItem->Release();
        }
    }
    else if ( cidl > 1 )
    {
        // walk the list of IDLISTs and see what items we have, if there are any delegate
        // items then this complicates things, if not then we can just pass to the 
        // inner ISF to get the information from that.

        for ( i = 0 ; (i != cidl) && !fDelegate ; i++ )
            fDelegate = (_IsDelegateObject(apidl[i], NULL) != NULL);

        if ( fDelegate )
        {
            if ( IsEqualIID(riid, IID_IDataObject) )
            {
                hr = CIDLData_CreateFromIDArray(_GetFolderIDList(), cidl, apidl, (IDataObject **)ppvOut);
            }
            else if ( IsEqualIID(riid, IID_IContextMenu) )
            {
                // the selection is large, there is at least one delegate item in it
                // so lets build an alternate list where all the items match the
                // first (clsid and delegate-ness).

                apidl2 = (LPCITEMIDLIST*)LocalAlloc(LPTR, SIZEOF(LPCITEMIDLIST)*cidl);

                if ( !apidl2 )  
                    ExitGracefully(hr, E_OUTOFMEMORY);

                for ( cidl2 = 0, i = 0 ; i != cidl ; i++ )
                {
                    if ( (_IsDelegateObject(apidl[0], NULL) == _IsDelegateObject(apidl[i], NULL)) &&
                            ( !_IsDelegateObject(apidl[0], NULL) || 
                                 IsEqualCLSID(_GetDelegateCLSID(apidl[0]), _GetDelegateCLSID(apidl[i]))) )
                    {
                        apidl2[cidl2++] = apidl[i];
                    }
                }

                // if there is a delegate in the first the bind to it and call it, otherwise
                // just call the innerISF.

                if ( !_IsDelegateObject(apidl2[0], NULL) )
                {
                    hr = _psfOuter->GetUIObjectOf(hwnd, cidl2, apidl2, riid, prgfInOut, ppvOut);
                }
                else
                {
                    hr = _GetDelegateFolder(apidl2[0], IID_IShellFolder, (void **)&psfItem);
                    FailGracefully(hr);

                    hr = psfItem->GetUIObjectOf(hwnd, cidl2, apidl2, riid, prgfInOut, ppvOut);
                    psfItem->Release();
                }

                fDelegate = TRUE;           // already handled
            }
            else
            {
                ExitGracefully(hr, E_NOTIMPL);          // BUGBUG: we must expand on this guy!
            }
        }            
    }

    // handled yet?  if not then pass onto the inner ISF

    if ( !fDelegate )
        hr = _psfOuter->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppvOut);

exit:

    if ( apidl2 )
        LocalFree(apidl2);

    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolder::GetDefaultSearchGUID(LPGUID lpGUID)
{
    return _psfOuter->GetDefaultSearchGUID(lpGUID);
}

STDMETHODIMP CDelegateFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    return _psfOuter->EnumSearches(ppenum);
}

STDMETHODIMP CDelegateFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return _psfOuter->GetDefaultColumn(dwRes, pSort, pDisplay);
}

STDMETHODIMP CDelegateFolder::GetDefaultColumnState(UINT iColumn, DWORD *pbState)
{
    return _psfOuter->GetDefaultColumnState(iColumn, pbState);
}

STDMETHODIMP CDelegateFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    IShellFolder2* psfItem;
    HRESULT hr = _GetItemFolder2(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->GetDetailsEx(pidl, pscid, pv);
        psfItem->Release();
    }
    return hr;
}

STDMETHODIMP CDelegateFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    IShellFolder2* psfItem;
    HRESULT hr = _GetItemFolder2(pidl, &psfItem);
    if (SUCCEEDED(hr))
    {
        hr = psfItem->GetDetailsOf(pidl, iColumn, pDetails);
        psfItem->Release();
    }
    return hr;
}

STDMETHODIMP CDelegateFolder::MapNameToSCID(LPCWSTR pwszName, SHCOLUMNID *pscid)
{
    return _psfOuter->MapNameToSCID(pwszName, pscid);
}


/*-----------------------------------------------------------------------------
/ Instance creation 
/----------------------------------------------------------------------------*/

STDAPI CDelegateFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    // we only suport being created as an agregate
    if ( !punkOuter || !IsEqualIID(riid, IID_IUnknown))
    {
        ASSERT(0);
        return E_FAIL;
    }

    CDelegateFolder *pdelisf = new CDelegateFolder(punkOuter);
    if ( !pdelisf )
        return E_OUTOFMEMORY;

    *ppvOut = pdelisf->_GetInner();
    return S_OK;
}


//-----------------------------------------------------------------------------
// Enumerator, this handles enumerating the IF and bringing in the delegate
// folder objects.
//-----------------------------------------------------------------------------

CDelegateFolderEnum::CDelegateFolderEnum(HWND hwnd, DWORD grfFlags, HDPA dpaFolders) :
    _cRef(1),
    _hwnd(hwnd),
    _grfFlags(grfFlags),
    _dpaFolders(dpaFolders),
    _index(0),
    _pCurrentEnum(NULL)
{   
    DllAddRef();
}

CDelegateFolderEnum::~CDelegateFolderEnum()
{
    if ( _dpaFolders )
        DPA_DestroyCallback(_dpaFolders, _ReleaseCB, NULL);

    if ( _pCurrentEnum )
        _pCurrentEnum->Release();

    DllRelease();
}

// IUnknown goop

STDMETHODIMP CDelegateFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDelegateFolderEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDelegateFolderEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDelegateFolderEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//-----------------------------------------------------------------------------
// IEnumIDList
//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolderEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    ULONG ulDummy;

    if (pceltFetched == NULL)
        pceltFetched = &ulDummy;

    *pceltFetched = 0;             // nothing has been returned yet

    // whilst we have an enumerator and there have been no items returned
    // lets go around to see how many objects we should be removing.

    while ( !*pceltFetched )
    {
        // no enumerator, lets see if we can get an instance of the next
        // one to enumerate from.

        if ( !_pCurrentEnum )
        {
            while ( _index < DPA_GetPtrCount(_dpaFolders) )
            {
                IShellFolder* pShellFolder = (IShellFolder*)DPA_GetPtr(_dpaFolders, _index++);
                ASSERT(pShellFolder);

                if ( SUCCEEDED(pShellFolder->EnumObjects(_hwnd, _grfFlags, &_pCurrentEnum)) )
                    break;
            }

            if ( !_pCurrentEnum )
                break;
        }

        // do we have an enumerator now? if so then lets call it and return the items back
        // to the caller.

        if ( _pCurrentEnum )
        {
            hr = _pCurrentEnum->Next(celt, rgelt, pceltFetched);

            if ( hr == S_FALSE )
            {
                _pCurrentEnum->Release();
                _pCurrentEnum = NULL;

                if ( !*pceltFetched )
                    continue;
            }

            break;
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolderEnum::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolderEnum::Reset(THIS)
{
    // lets start the enumeration process again, so set the index back to the head
    // and release the enumerator we are currently using.

    _index = 0;

    ATOMICRELEASE(_pCurrentEnum);

    return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDelegateFolderEnum::Clone(IEnumIDList **ppenum)
{
    return E_NOTIMPL;
}


//-----------------------------------------------------------------------------
// This code used to be in shdocvw, it is the implementation of a IMalloc for handling
// delegate folder items
//-----------------------------------------------------------------------------

class CDelagateMalloc : public IMalloc
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID,void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IMalloc
    virtual STDMETHODIMP_(LPVOID)   Alloc(ULONG cb);
    virtual STDMETHODIMP_(LPVOID)   Realloc(void *pv, ULONG cb);
    virtual STDMETHODIMP_(void)     Free(void *pv);
    virtual STDMETHODIMP_(ULONG)    GetSize(void *pv);
    virtual STDMETHODIMP_(int)      DidAlloc(void *pv);
    virtual STDMETHODIMP_(void)     HeapMinimize();

private:
    CDelagateMalloc(void *pv, UINT cbSize, WORD wOuter);
    ~CDelagateMalloc();
    void* operator new(size_t cbClass, UINT cbSize);

    friend HRESULT CDelegateMalloc_Create(void *pv, UINT cbSize, WORD wOuter, IMalloc **ppmalloc);

protected:
    LONG _cRef;
    WORD _wOuter;           // delegate item outer signature
    WORD _wUnused;          // to allign
#ifdef DEBUG
    UINT _cAllocs;
#endif
    UINT _cb;
    BYTE _data[];
};

void* CDelagateMalloc::operator new(size_t cbClass, UINT cbSize)
{
    return ::operator new(cbClass + cbSize);
}

CDelagateMalloc::CDelagateMalloc(void *pv, UINT cbSize, WORD wOuter)
{
    _cRef = 1;
    _wOuter = wOuter;
    _cb = cbSize;

    memcpy(_data, pv, _cb);
}

CDelagateMalloc::~CDelagateMalloc()
{
    DEBUG_CODE( TraceMsg(DM_TRACE, "DelegateMalloc destroyed with %d allocs performed", _cAllocs); )
}

HRESULT CDelagateMalloc::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDelagateMalloc, IMalloc), // IID_IMalloc
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CDelagateMalloc::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDelagateMalloc::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

void *CDelagateMalloc::Alloc(ULONG cb)
{
    WORD cbActualSize = (WORD)(
                        SIZEOF(DELEGATEITEMID) - 1 +    // header (-1 sizeof(rgb[0])
                        cb +                            // inner
                        _cb);                           // outer data

    PDELEGATEITEMID pidl = (PDELEGATEITEMID)SHAlloc(cbActualSize + 2);  // +2 for pidl term
    if (pidl)
    {
        pidl->cbSize = cbActualSize;
        pidl->wOuter = _wOuter;
        pidl->cbInner = (WORD)cb;
        memcpy(&pidl->rgb[cb], _data, _cb);
        *(WORD *)&(((BYTE *)pidl)[cbActualSize]) = 0;
#ifdef DEBUG
        _cAllocs++;
#endif
    }
    return pidl;
}

void *CDelagateMalloc::Realloc(void *pv, ULONG cb)
{
    return NULL;
}

void CDelagateMalloc::Free(void *pv)
{
    SHFree(pv);
}

ULONG CDelagateMalloc::GetSize(void *pv)
{
    return (ULONG)-1;
}

int CDelagateMalloc::DidAlloc(void *pv)
{
    return -1;
}

void CDelagateMalloc::HeapMinimize()
{
}

STDAPI CDelegateMalloc_Create(void *pv, UINT cbSize, WORD wOuter, IMalloc **ppmalloc)
{
    CDelagateMalloc *pdm = new(cbSize) CDelagateMalloc(pv, cbSize, wOuter);
    if (pdm)
    {
        HRESULT hres = pdm->QueryInterface(IID_IMalloc, (void **)ppmalloc);
        pdm->Release();
        return hres;
    }
    return E_OUTOFMEMORY;
}
