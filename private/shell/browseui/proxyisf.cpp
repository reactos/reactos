#include "priv.h"
#include "sccls.h"
#include "proxyisf.h"
#include "isfband.h"    // For ISFBandCreateMenuPopup

#include "shguidp.h"


class CProxyEnumIDList : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID, void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumIDList methods ***
    virtual STDMETHODIMP Next  (ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip  (ULONG celt);
    virtual STDMETHODIMP Reset (void);
    virtual STDMETHODIMP Clone (IEnumIDList **ppenum);

    CProxyEnumIDList(LPITEMIDLIST pidl, IEnumIDList* penum, IUnknown* punk);
private:
    ~CProxyEnumIDList();

    IOleCommandTarget*  _poct;
    IEnumIDList*        _penum;
    LPITEMIDLIST        _pidl;

    int                 _cRef;
};



/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface method

*/
STDMETHODIMP CProxyEnumIDList::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CProxyEnumIDList, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

/*----------------------------------------------------------
Purpose: IUnknown::AddRef method

*/
STDMETHODIMP_(ULONG) CProxyEnumIDList::AddRef(void)
{
    return ++_cRef;
}

/*----------------------------------------------------------
Purpose: IUnknown::Release method

*/
STDMETHODIMP_(ULONG) CProxyEnumIDList::Release(void)
{
    ASSERT(_cRef > 0);
    _cRef--;
    if(_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: IEnumIDList::Next method

*/
STDMETHODIMP CProxyEnumIDList::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    ULONG celtFetched = 0;
    HRESULT hres = S_FALSE;

    if (celt > 0 && _penum)
    {
EnumLoop:
        hres = _penum->Next(1, rgelt, &celtFetched);
        if (SUCCEEDED(hres) && celtFetched == 1 && _poct)
        {
            LPITEMIDLIST pidlFull = ILCombine(_pidl, rgelt[0]);
            if (pidlFull)
            {
                VARIANT varRet;
                VariantInit(&varRet);

                VARIANT varPidl;
                InitVariantFromIDList(&varPidl, pidlFull);

                if (SUCCEEDED(_poct->Exec(&CGID_FilterObject, PHID_FilterOutPidl, 0, &varPidl, &varRet)))
                {
                    if (varRet.vt == VT_BOOL && varRet.boolVal == VARIANT_TRUE)
                    {
                        celtFetched = 0;    // kill this guy, but don't leak!
                        ILFree(rgelt[0]);
                        rgelt[0] = NULL;
                    }

                    VariantClearLazy(&varRet);
                }
                ILFree(pidlFull);
            }
            if (celtFetched == 0)
                goto EnumLoop;           // start again
        }
    }

    if (pceltFetched)
        *pceltFetched = celtFetched;

    return hres;
}

/*----------------------------------------------------------
Purpose: IEnumIDList::Next method

*/
STDMETHODIMP CProxyEnumIDList::Skip  (ULONG celt)
{
    if (_penum)
        return _penum->Skip(celt);
    return E_FAIL;
}

/*----------------------------------------------------------
Purpose: IEnumIDList::Next method

*/
STDMETHODIMP CProxyEnumIDList::Reset (void)
{
    if (_penum)
        return _penum->Reset();
    return E_FAIL;
}

/*----------------------------------------------------------
Purpose: IEnumIDList::Next method

*/
STDMETHODIMP CProxyEnumIDList::Clone (IEnumIDList **ppenum)
{
    ASSERT(IS_VALID_WRITE_PTR(ppenum, IEnumIDList));
    CProxyEnumIDList *pPeil = new CProxyEnumIDList(_pidl, _penum, _poct);
    if (pPeil)
    {
        *ppenum = SAFECAST(pPeil, IEnumIDList*);
        return NOERROR;
    }
    else
        return E_OUTOFMEMORY;
}

// Constructor
CProxyEnumIDList::CProxyEnumIDList(LPITEMIDLIST pidl, IEnumIDList* penum, IUnknown* punk) : 
    _cRef(1), _penum(penum)
{
    if (_penum)
        penum->AddRef();
    _pidl = ILClone(pidl);

    IUnknown_QueryService(punk, SID_SHostProxyFilter, IID_IOleCommandTarget, (void**)&_poct);
}

// Destructor
CProxyEnumIDList::~CProxyEnumIDList()
{
    ATOMICRELEASE(_poct);
    ATOMICRELEASE(_penum);
    ILFree(_pidl);
}






//=================================================================
// Implementation of an IShellFolder that wraps a single object that
// supports IShellFolder.  This gets to respond to GetUIObjectOf to
// support additional objects.  We call this a HostProxyShellFolder
// object.
//
//=================================================================

//
// CHostProxyISF object
//


#undef SUPERCLASS


// Constructor
CHostProxyISF::CHostProxyISF() : 
    _cRef(1)
{
    DllAddRef();
    
    ASSERT(NULL == _pidl);
}


// Destructor
CHostProxyISF::~CHostProxyISF()
{
    ATOMICRELEASE(_psf);

    Pidl_Set(&_pidl, NULL);

    ATOMICRELEASE(_psi);
    ATOMICRELEASE(_pp);
    ATOMICRELEASE(_ppf);
    ATOMICRELEASE(_psfRealCFSFolder);
    DllRelease();
}


STDMETHODIMP_(ULONG) CHostProxyISF::AddRef()
{
    _cRef++;
    return _cRef;
}


STDMETHODIMP_(ULONG) CHostProxyISF::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0) 
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CHostProxyISF::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CHostProxyISF, IShellFolder),
        QITABENT(CHostProxyISF, IProxyShellFolder),
        QITABENT(CHostProxyISF, IShellService),
        { 0 },
    };
    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres) && _psf)
    {
        void ** ppv;

        // We're supporting the interfaces that CFSFolder supports
        // since Win95 shell32 can't aggregate.  Here's how this 
        // works: 
        //
        // CHostProxyISF implements all the interfaces that
        // CFSFolder implements.  When someone QIs for one of those
        // interfaces, we QI the psf we host and cache that away
        // (via ppv).  But to maintain COM rules, we return a pointer
        // back to our implementation, which simply delegates to
        // the cached pointer when called.
        //
        // The exception is INeedRealCFSFolder which is a hack in
        // the first place.  CFSFolder QI's for this interface
        // to get its 'this' pointer.  So we must return our 
        // cached object, which breaks COM rules.
        //  

        if (IsEqualIID(riid, IID_IShellIcon))
            ppv = (void **)&_psi;
        else if (IsEqualIID(riid, IID_IPersist))
            ppv = (void **)&_pp;
        else if (IsEqualIID(riid, IID_IPersistFolder))
            ppv = (void **)&_ppf;
        else if (IsEqualIID(riid, IID_INeedRealCFSFolder))
        {
            // CFSFolder uses a hack to QI to this private interface
            // in order to get its 'this' pointer.  We cannot really 
            // return our 'this' pointer, otherwise it will GP-fault.

            // WARNING!!!!  We're breaking COM rules here for this 
            // special interface!!

            // BUGBUG (scotth): we need to do this hack only for browser-only.

            return _psf->QueryInterface(riid, ppvObj);
        }
        else
            ppv = NULL;

        if (ppv)
        {
            // Does the host object support this interface?
            if (NULL == *ppv)
            {
                // ppv points to one of our cached pointers
                hres = _psf->QueryInterface(riid, ppv);
            }
            else
            {
                // Already have the pointer cached, so carry on
                hres = S_OK;
            }

            if (SUCCEEDED(hres))
            {
                // Yes; hand back a pointer to us.  We will delegate.
                EVAL(SUCCEEDED(QueryInterface(riid, ppvObj)));            
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellService::SetOwner method

*/
STDMETHODIMP CHostProxyISF::SetOwner(IUnknown* punk)
{
    HRESULT hres = S_OK;

    ASSERT(NULL == punk || IS_VALID_CODE_PTR(punk, IUnknown));

    if (_punkOwner)
        SetOwnerPSF(NULL);
        
    ATOMICRELEASE(_punkOwner);
    
    if (punk) 
    {
        hres = punk->QueryInterface(IID_IUnknown, (void **)&_punkOwner);

        SetOwnerPSF(_punkOwner);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::EnumObjects method

*/
STDMETHODIMP CHostProxyISF::EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList)
{
    HRESULT hres = E_FAIL;

    *ppenumIDList = NULL;

    if(_dwFlags & SPF_HAVECALLBACK)
    {
        IEnumIDList* penum;
        if(SUCCEEDED(hres = _psf->EnumObjects(hwndOwner, grfFlags, &penum)))
        {
            CProxyEnumIDList* pPeil = new CProxyEnumIDList(_pidl, penum, _punkOwner);
            if(pPeil)
            {
                *ppenumIDList = pPeil;
            }
            else
                hres = E_OUTOFMEMORY;
            penum->Release();
        }
    }
    else if (_psf)
        hres = _psf->EnumObjects(hwndOwner, grfFlags, ppenumIDList);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::BindToObject method

*/
STDMETHODIMP CHostProxyISF::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, void ** ppvOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_WRITE_PTR(ppvOut, LPVOID));

    *ppvOut = NULL;

    if (_psf)
    {
        if (_dwFlags & SPF_INHERIT)
        {
            hres = E_OUTOFMEMORY;

            IProxyShellFolder * pproxyNew;

            hres = CloneProxyPSF(IID_IProxyShellFolder, (void **)&pproxyNew);
            if (SUCCEEDED(hres))
            {
                IShellFolder * psfNew;
                IUnknown * punk;

                hres = pproxyNew->QueryInterface(riid, (void **)&punk);
                if (SUCCEEDED(hres))
                {
                    hres = _psf->BindToObject(pidl, pbcReserved, IID_IShellFolder, (void **)&psfNew);
                    if (SUCCEEDED(hres))
                    {
                        ASSERT(_pidl);
                        LPITEMIDLIST pidlFull = ILCombine(_pidl, pidl);
                        if (pidlFull)
                        {
                            hres = pproxyNew->InitHostProxy(psfNew, pidlFull, _dwFlags);
                            if (SUCCEEDED(hres))
                                *ppvOut = punk;
                                
                            ILFree(pidlFull);
                        }
                        else
                            hres = E_OUTOFMEMORY;
                            
                        psfNew->Release();
                    }

                    if (FAILED(hres))
                        punk->Release();
                }
                
                pproxyNew->Release();
            }
        }
        else 
            hres = _psf->BindToObject(pidl, pbcReserved, riid, ppvOut);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::BindToStorage method

*/
STDMETHODIMP CHostProxyISF::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, void ** ppvObj)
{
    HRESULT hres = E_FAIL;

    *ppvObj = NULL;

    if (_psf)
        hres = _psf->BindToStorage(pidl, pbcReserved, riid, ppvObj);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::CompareIDs method

*/
STDMETHODIMP CHostProxyISF::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = E_FAIL;
    DWORD Attrib1, Attrib2;
    if (_psf)
    {
        // If we are giving Folders precedence, then see if we can get the attribs of both.
        // then test if they are both of different types. If they are of different types, then
        // they cannot point to the same object..... Don't handle that case.
        // if any fail, just send if off to the _psf's handler. 
        if (  (_dwFlags & SPF_FOLDERPRECEDENCE) &&
            (SUCCEEDED(_psf->GetAttributesOf(1, &pidl1, &Attrib1)) && SUCCEEDED(_psf->GetAttributesOf(1, &pidl2, &Attrib2))) &&
                ((Attrib1 & SFGAO_FOLDER) ^ (Attrib2 & SFGAO_FOLDER)) )
        {
            if(Attrib1 & SFGAO_FOLDER)
                hres = ResultFromShort(-1);
            else
                hres = ResultFromShort(1);
        }
        else
            hres = _psf->CompareIDs(lParam, pidl1, pidl2);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::CreateViewObject method

*/
STDMETHODIMP CHostProxyISF::CreateViewObject (HWND hwndOwner, REFIID riid, void ** ppvOut)
{
    HRESULT hres = E_FAIL;

    // If they're both set, we just waste time doing the same thing twice.
    ASSERT((_dwFlags & SPF_PRIORITY) ^ (_dwFlags & SPF_SECONDARY));

    *ppvOut = NULL;

    if (_dwFlags & SPF_PRIORITY)
    {
        // Proxy goes first
        hres = CreateViewObjectPSF(hwndOwner, riid, ppvOut);
    }

    if (FAILED(hres) && _psf)
    {
        // Let host try
        hres = _psf->CreateViewObject(hwndOwner, riid, ppvOut);
    }

    if (FAILED(hres) && (_dwFlags & SPF_SECONDARY) )
    {
        // Proxy tries last
        hres = CreateViewObjectPSF(hwndOwner, riid, ppvOut);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetAttributesOf method

*/
STDMETHODIMP CHostProxyISF::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                   ULONG * pfInOut)
{
    HRESULT hres = E_FAIL;

    if (_psf)
        hres = _psf->GetAttributesOf(cidl, apidl, pfInOut);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetUIObjectOf method

*/
STDMETHODIMP CHostProxyISF::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, void ** ppvOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_READ_PTR(apidl, LPCITEMIDLIST));

    // If they're both set, we just waste time doing the same thing twice.
    ASSERT((_dwFlags & SPF_PRIORITY) ^ (_dwFlags & SPF_SECONDARY));

    *ppvOut = NULL;

    if (_dwFlags & SPF_PRIORITY)
    {
        // Proxy goes first
        hres = GetUIObjectOfPSF(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    if (FAILED(hres) && _psf)
    {
        // Let host try
        hres = _psf->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    if (FAILED(hres) && (_dwFlags & SPF_SECONDARY))
    {
        // Proxy tries last
        hres = GetUIObjectOfPSF(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetDisplayNameOf method

*/
STDMETHODIMP CHostProxyISF::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, 
                                             LPSTRRET pstrret)
{
    HRESULT hres = E_FAIL;

    if (_psf)
        hres = _psf->GetDisplayNameOf(pidl, uFlags, pstrret);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::SetNameOf method

*/
STDMETHODIMP CHostProxyISF::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                             LPCOLESTR lpszName, DWORD uFlags,
                             LPITEMIDLIST * ppidlOut)
{
    HRESULT hres = E_FAIL;

    if (ppidlOut)
        *ppidlOut = NULL;

    if (_psf)
        hres = _psf->SetNameOf(hwndOwner, pidl, lpszName, uFlags, ppidlOut);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::ParseDisplayName method

*/
STDMETHODIMP CHostProxyISF::ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    HRESULT hres = E_FAIL;

    *ppidl = NULL;

    if (_psf)
        hres = _psf->ParseDisplayName(hwndOwner, pbcReserved, lpszDisplayName, pchEaten, ppidl, pdwAttributes);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellIcon::GetIconOf

*/
STDMETHODIMP CHostProxyISF::GetIconOf(LPCITEMIDLIST pidl, UINT uFlags, int * pnIconIndex)
{
    HRESULT hres = E_FAIL;

    if (_psi)
        hres = _psi->GetIconOf(pidl, uFlags, pnIconIndex);

    return hres;
}    


/*----------------------------------------------------------
Purpose: IPersist::GetClassID

*/
STDMETHODIMP CHostProxyISF::GetClassID(CLSID * pclsid)
{
    HRESULT hres = E_FAIL;

    if (_pp)
        hres = _pp->GetClassID(pclsid);

    return hres;
}    


/*----------------------------------------------------------
Purpose: IPersistFolder::Initialize

*/
STDMETHODIMP CHostProxyISF::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hres = E_FAIL;

    if (_ppf)
        hres = _ppf->Initialize(pidl);

    return hres;
}    


/*----------------------------------------------------------
Purpose: IHostProxyShellFolder::InitHostProxy

*/
STDMETHODIMP CHostProxyISF::InitHostProxy(
    IShellFolder * psf, 
    LPCITEMIDLIST  pidl, 
    DWORD          dwFlags)
{
    HRESULT hres = S_OK;

    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(IS_VALID_PIDL(pidl));

    if (_psf)
        hres = E_UNEXPECTED;
    else if (NULL == psf || NULL == pidl)
        hres = E_INVALIDARG;
    else
    {
        _dwFlags = dwFlags;

        _pidl = ILClone(pidl);
        if (_pidl)
        {
            _psf = psf;
            _psf->AddRef();
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}    


//=================================================================
//
// Base proxy object for the file-system shell folder
//

#undef SUPERCLASS  
#undef CHostProxyISF

// Constructor
CFSProxyISF::CFSProxyISF()
{
}

// Destructor
CFSProxyISF::~CFSProxyISF()
{
    // WARNING.  SetOwner is a base-class method which calls a derived-class
    // method (SetOwnerPSF).  We must call this method here, before our
    // derived class is destroyed.
    SetOwner(NULL);
}


/*----------------------------------------------------------
Purpose: IProxyShellFolder::CloneProxy method

*/
STDMETHODIMP CFSProxyISF::CloneProxyPSF(REFIID riid, void ** ppvObj)
{
    HRESULT hres = E_OUTOFMEMORY;
    IShellService * pproxyNew = v_NewProxy();

    if (pproxyNew)
    {
        pproxyNew->SetOwner(_punkOwner);

        hres = pproxyNew->QueryInterface(riid, ppvObj);
        pproxyNew->Release();
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IProxyShellFolder::GetUIObjectOfPSF method

*/
STDMETHODIMP CFSProxyISF::GetUIObjectOfPSF(HWND hwndOwner, UINT cidl, 
                                            LPCITEMIDLIST * apidl, REFIID riid, 
                                            UINT * prgfInOut, void ** ppvOut)
{
    HRESULT hres = E_FAIL;

    *ppvOut = NULL;

    if (1 == cidl)
    {
        LPCITEMIDLIST pidl = *apidl;

        if (IsEqualIID(riid, IID_IMenuPopup))
        {
            // For cascading menus
            IProxyShellFolder* ppsfChild;
            LPITEMIDLIST pidlFull = NULL;

            BindToObject(pidl, NULL, IID_IProxyShellFolder, (LPVOID*)&ppsfChild);
            if (_pidl)
                pidlFull = ILCombine(_pidl, pidl);

            if (ppsfChild || pidlFull)
            {
                IMenuPopup * pmp = ISFBandCreateMenuPopup(_punkOwner, ppsfChild, pidlFull, NULL, TRUE);
                *ppvOut = pmp;
                hres = pmp ? S_OK : E_OUTOFMEMORY;

                // Don't ReleaseProxy here, otherwise pmp (which cached it)
                // won't be able to call the proxy.  CISFBand releases the
                // proxy.
                if (ppsfChild)
                    ppsfChild->Release();
                    
                Pidl_Set(&pidlFull, NULL);
            }
        }
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IProxyShellFolder::CreateViewObjectPSF method

*/
STDMETHODIMP CFSProxyISF::CreateViewObjectPSF(HWND hwndOwner, REFIID riid, void ** ppvOut)
{
    HRESULT hres = E_FAIL;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        // Pass NULL as the hwnd so the window is not activated.
        hres = _psf->CreateViewObject(NULL, IID_IDropTarget, ppvOut);
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellService::SetOwner method

*/
STDMETHODIMP CFSProxyISF::SetOwnerPSF(IUnknown* punk)
{
    HRESULT hres = S_OK;

    ASSERT(NULL == punk || IS_VALID_CODE_PTR(punk, IUnknown));

    TraceMsg(TF_MENUBAND, "CFSProxyISF::SetOwner(punk=%#lx)", punk);

    ATOMICRELEASE(_punkOwner);

    if (punk)
        hres = punk->QueryInterface(IID_IUnknown, (void **)&_punkOwner);
    
    return hres;
}



