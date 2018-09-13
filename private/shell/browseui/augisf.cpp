#include "priv.h"
#include "sccls.h"
#include "iface.h"
#include "augisf.h"
#include "menuisf.h"

//=================================================================
// Implementation of an IShellFolder that wraps a collection of
// other IShellFolders.  We call this an augmented IShellFolder
// object.
//
//=================================================================

// The CAugmentedISF wraps all the pidls so it can identify which pidl
// belongs to which IShellFolder object.

typedef struct tagIDWRAP
{
    // Real pidl goes on the end
    
    UINT   nID;         // Refers to a specific IShellFolder object
    UINT   cbOriginal;  // the original size of the pidl.  we need this because we dword align the pidl before wrapping it
} IDWRAP, * PIDWRAP;

#define IDWrap_GetWrap(pidl)            ((PIDWRAP)(((LPBYTE)pidl) + (pidl)->mkid.cb - SIZEOF(IDWRAP)))
#define IDWrap_GetID(pidl)              (IDWrap_GetWrap(pidl)->nID)
#define IDWrap_GetOriginalSize(pidl)    (IDWrap_GetWrap(pidl)->cbOriginal)

/*----------------------------------------------------------
    The CAugmentedISF object holds an array of CISFElems, each of which
    refers to an IShellFolder which will be enumerated.
*/
class CISFElem
{
public:
    CISFElem *      Clone(void);
    HRESULT         AcquireEnumerator(DWORD dwFlags);
    IShellFolder *  GetPSF()                { return _psf; };
    IEnumIDList *   GetEnumerator()         { return _pei; };
    void            GetNameSpaceID(GUID * rguid);
    HRESULT         SetPidl(LPCITEMIDLIST pidl);
    LPCITEMIDLIST   GetPidl()               { return _pidl; };
    DWORD           GetFlags()              { return _dwFlags; };
    void            SetRegister(UINT uReg)  { _uRegister = uReg; };
    UINT            GetRegister()           { return _uRegister; };

    CISFElem(const GUID * pguid, IShellFolder * psf, DWORD dwFlags);
    ~CISFElem();

protected:

    GUID           _guid;       // Unique ID
    IShellFolder * _psf;
    IEnumIDList *  _pei;        // Used by CAugSIFEnum only
    LPITEMIDLIST   _pidl;
    DWORD          _dwFlags;
    UINT           _uRegister;

    friend BOOL IsValidPCISFElem(CISFElem * pel);
};

//
// CAugmentedISF enumerator
//
class CAugISFEnum : public IEnumIDList
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef) () ;
    STDMETHOD_(ULONG,Release) ();

    // *** IEnumIDList methods ***
    STDMETHOD(Next)  (ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched);
    STDMETHOD(Skip)  (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (IEnumIDList **ppenum);

    // Other methods
    HRESULT     Init(HDPA hdpaISF, DWORD dwEnumFlags);
        
    CAugISFEnum();
    ~CAugISFEnum();

protected:
    IEnumIDList *   _GetObjectEnumerator(int nID);

    UINT    _cRef;
    int     _iCurISF;       // current item in _hdpaISF
    HDPA    _hdpaISF;
};

/*----------------------------------------------------------
    Pidl wrapping routine
*/
LPITEMIDLIST AugISF_WrapPidl( LPCITEMIDLIST pidl, int nID )
{
    LPITEMIDLIST pidlRet = NULL;

    // get the size of the pidl
    // round up to dword align.  
    UINT cbPidlSize = (pidl->mkid.cb + 3) & ~3; 
    
    ASSERT(cbPidlSize >= pidl->mkid.cb);
    UINT cbSize = SIZEOF(IDWRAP) + cbPidlSize + SIZEOF(DWORD); // pidl plus terminator
    LPBYTE p = (LPBYTE)SHAlloc(cbSize);
    if (p)
    {
        ZeroMemory(p, cbSize); 
        memcpy(p, pidl, pidl->mkid.cb);

        IDWRAP* pidw = (IDWRAP*) (p + cbPidlSize);
        pidw->nID = nID;
        pidw->cbOriginal = pidl->mkid.cb;
                           
        // now make the cb be the whole pidl (not including the final null)
        pidlRet = (LPITEMIDLIST)p;
        pidlRet->mkid.cb = (USHORT) (cbPidlSize + SIZEOF(IDWRAP));
    }
    return pidlRet;
}    

// GetIDListWrapCount and GetNameSpaceCount are not used anywhere
#if 0
/*----------------------------------------------------------
Purpose: IAugmentedShellFolder2::GetIDListWrapCount
*/
STDMETHODIMP CAugmentedISF::GetNameSpaceCount( OUT LONG* pcNamespaces )
{
    if( NULL == pcNamespaces )
        return E_INVALIDARG ;

    *pcNamespaces = (NULL != _hdpa) ? DPA_GetPtrCount( _hdpa ) : 0 ;
    return S_OK ;
}

/*----------------------------------------------------------
Purpose: IAugmentedShellFolder2::GetIDListWrapCount
*/
STDMETHODIMP CAugmentedISF::GetIDListWrapCount( 
    LPCITEMIDLIST pidlWrap, 
    OUT LONG * pcPidls )
{
    if( NULL == pidlWrap || NULL == pcPidls )   
        return E_INVALIDARG ;

    PIDWRAP pWrap = IDWrap_GetWrap(pidlWrap) ;
    *pcPidls = 0 ;

    if( NULL != _hdpa && 
        DPA_GetPtrCount( _hdpa ) > (int)pWrap->nID && 
        pWrap->cbOriginal < pidlWrap->mkid.cb + sizeof(IDWRAP) )
        *pcPidls = 1 ;

    return S_OK ;
}
#endif
/*----------------------------------------------------------
Purpose: IAugmentedShellFolder2::UnWrapIDList
*/
STDMETHODIMP CAugmentedISF::UnWrapIDList( 
    LPCITEMIDLIST pidl, 
    LONG cPidls, 
    IShellFolder ** ppsf, 
    LPITEMIDLIST * ppidlFolder, 
    LPITEMIDLIST * ppidl, 
    LONG * pcFetched )
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(IS_VALID_PIDL(pidl));

    if( pcFetched )
        *pcFetched = 0 ;
    
    if (pidl)
    {
        UINT nId = IDWrap_GetID(pidl);
        CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, nId);

        if (pel)
        {
            LPITEMIDLIST pidlNew = ILClone(GetNativePidl(pidl, nId));
            LPITEMIDLIST pidlFolderNew = ILClone(pel->GetPidl());

            if (pidlNew && pidlFolderNew)
            {
                if ( ppsf )
                {
                    *ppsf = pel->GetPSF();
                    (*ppsf)->AddRef();
                }

                *ppidl = pidlNew;
                *ppidlFolder = pidlFolderNew;
                
                if( pcFetched ) 
                    *pcFetched = 1 ;
                
                hres = (cPidls == 1) ? S_OK : S_FALSE ;
            }
            else
            {
                ILFree(pidlNew);
                ILFree(pidlFolderNew);
                hres = E_OUTOFMEMORY;
            }
        }
        else
            hres = E_FAIL;
    }

    return hres;
}

/*----------------------------------------------------------
    Purpose: CAugmentedISF::TranslatePidl
*/
LPITEMIDLIST CAugmentedISF::TranslatePidl( LPCITEMIDLIST pidlNS, LPCITEMIDLIST pidl, LPARAM nID )
{
    LPITEMIDLIST pidlRet = NULL;

    // Is this not empty and an immediate child?
    if (ILIsParent(pidlNS, pidl, TRUE))
    {
        LPCITEMIDLIST pidlChild;
        LPITEMIDLIST pidlNew;
        TCHAR szFullName[MAX_PATH];
        LPITEMIDLIST pidlFull = NULL;

        // HACKHACK (lamadio): You cannot use SHGetRealIDL for augisf encapsulated
        // IShellFolders. This routine QIs for INeedRealShellFolder, which IAugISF 
        // doesnot forward. The fstree code does not handle aggregation, so this 
        // cannot be forwarded anyway. This code can be cleaned up when we rewrite 
        // the fstree stuff... Sep.4.1997

        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szFullName, SIZECHARS(szFullName), NULL))
        && (pidlFull = ILCreateFromPath(szFullName)) != NULL)
        {
            pidlChild = ILFindLastID(pidlFull);
            pidlNew = ILClone(pidlFull);
        }
        else
        {
            pidlChild = ILFindLastID(pidl);
            pidlNew = ILClone(pidl);
        }

        // Yes; create a new full pidl where the last element is wrapped

        if (pidlNew)
        {
            ILRemoveLastID(pidlNew);

            LPITEMIDLIST pidlWrap = AugISF_WrapPidl( pidlChild, (int)nID );
            if (pidlWrap)
            {
                pidlRet = ILCombine(pidlNew, pidlWrap);
                ILFree(pidlWrap);
            }
            ILFree(pidlNew);
        }

        ILFree(pidlFull);   //Checks for a NULL pidl
    }
    else
        pidlRet = (LPITEMIDLIST)pidl;

    return pidlRet;
}    

/*----------------------------------------------------------
    Purpose: CAugmentedISF::GetNativePidl
    Clones and returns a copy of the native ('source') pidl 
    contained in the specified wrap.
*/
LPITEMIDLIST CAugmentedISF::GetNativePidl(LPCITEMIDLIST pidl, LPARAM lParam /*int nID*/ )
{
    ASSERT(IS_VALID_PIDL(pidl));
    UNREFERENCED_PARAMETER( lParam ) ; // only one source ID in the wrap!

    LPITEMIDLIST pidlNew = ILClone(pidl);

    if (pidlNew)
    {
        // Take off our trailing wrap signature
        pidlNew->mkid.cb = IDWrap_GetOriginalSize(pidl);

        ASSERT(sizeof(IDWRAP) >= sizeof(USHORT));

        USHORT * pu = (USHORT *)_ILNext(pidlNew);
        *pu = 0;
    }
    return pidlNew;
}    




CISFElem::CISFElem(const GUID * pguid, IShellFolder * psf, DWORD dwFlags) : _dwFlags(dwFlags)
{
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(NULL == pguid || IS_VALID_READ_PTR(pguid, GUID));

    if (pguid)
        CopyMemory(&_guid, pguid, sizeof(_guid));

    _psf = psf;
    _psf->AddRef();
}    

CISFElem::~CISFElem()
{
    ASSERT(IS_VALID_CODE_PTR(_psf, IShellFolder));
    
    _psf->Release();

    if (_pei)
        _pei->Release();

    Pidl_Set(&_pidl, NULL);
}   

CISFElem * CISFElem::Clone(void)
{
    CISFElem * pelem = new CISFElem(&_guid, _psf, _dwFlags);

    if (pelem)
    {
        // If this fails, we're punting and going ahead anyway
        pelem->SetPidl(_pidl);      
    }

    return pelem;
}     


void CISFElem::GetNameSpaceID(GUID * pguid)
{
    ASSERT(IS_VALID_WRITE_PTR(pguid, GUID));
    CopyMemory(pguid, &_guid, sizeof(_guid));
}    


HRESULT CISFElem::SetPidl(LPCITEMIDLIST pidl)
{
    HRESULT hres = S_OK;

    Pidl_Set(&_pidl, pidl);

    if (pidl && NULL == _pidl)
        hres = E_OUTOFMEMORY;

    return hres;
}

/*----------------------------------------------------------
Purpose: Gets an enumerator for the IShellFolder and caches it.

*/
HRESULT CISFElem::AcquireEnumerator(DWORD dwFlags)
{
    return IShellFolder_EnumObjects(_psf, NULL, dwFlags, &_pei);
}    


//
// CAugmentedISF object
//


#undef SUPERCLASS


#ifdef DEBUG

BOOL IsValidPCISFElem(CISFElem * pel)
{
    return (IS_VALID_WRITE_PTR(pel, CISFElem) &&
            IS_VALID_CODE_PTR(pel->_psf, IShellFolder) &&
            (NULL == pel->_pidl || IS_VALID_PIDL(pel->_pidl)));
}   
 
#endif

// Constructor
CAugmentedISF::CAugmentedISF() : 
    _cRef(1)
{
    DllAddRef();
}


/*----------------------------------------------------------
Purpose: Callback to destroy each element

*/
int CISFElem_DestroyCB(LPVOID pv, LPVOID pvData)
{
    CISFElem * pel = (CISFElem *)pv;

    ASSERT(NULL == pel || IS_VALID_STRUCT_PTR(pel, CISFElem));

    if (pel)
        delete pel;

    return TRUE;
}   


/*----------------------------------------------------------
Purpose: Callback to set the owner of each element

*/
int CISFElem_SetOwnerCB(LPVOID pv, LPVOID pvData)
{
    CISFElem * pel = (CISFElem *)pv;

    ASSERT(IS_VALID_STRUCT_PTR(pel, CISFElem));

    IShellFolder * psf = pel->GetPSF();
    if (psf)
    {
        IUnknown_SetOwner(psf, (IUnknown *)pvData);
        // don't need to release psf
    }

    return TRUE;
}   


typedef struct {
    HRESULT hres;
    HWND hwnd;
    const IID * piid;
    void ** ppvObj;
} CVODATA;

    
/*----------------------------------------------------------
Purpose: Callback to call CreateViewObject

*/
int CISFElem_CreateViewObjectCB(LPVOID pv, LPVOID pvData)
{
    CISFElem * pel = (CISFElem *)pv;
    CVODATA * pdata = (CVODATA *)pvData;

    ASSERT(IS_VALID_STRUCT_PTR(pel, CISFElem));
    ASSERT(IS_VALID_WRITE_PTR(pdata, CVODATA));

    IShellFolder * psf = pel->GetPSF();
    if (psf)
    {
        pdata->hres = psf->CreateViewObject(pdata->hwnd, *(pdata->piid), pdata->ppvObj);
        if (SUCCEEDED(pdata->hres))
            return FALSE;       // stop on first success
            
        // don't need to release psf
    }

    return TRUE;
}   



// Destructor
CAugmentedISF::~CAugmentedISF()
{
    SetOwner(NULL);

    DPA_DestroyCallback(_hdpa, CISFElem_DestroyCB, NULL);
    DllRelease();
}


STDMETHODIMP_(ULONG) CAugmentedISF::AddRef()
{
    _cRef++;
    return _cRef;
}


STDMETHODIMP_(ULONG) CAugmentedISF::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0) 
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CAugmentedISF::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAugmentedISF, IShellFolder),
        QITABENT(CAugmentedISF, IAugmentedShellFolder),
        QITABENT(CAugmentedISF, IAugmentedShellFolder2),
        QITABENT(CAugmentedISF, IShellService),
        QITABENT(CAugmentedISF, ITranslateShellChangeNotify),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}


/*----------------------------------------------------------
Purpose: IShellService::SetOwner method

*/
STDMETHODIMP CAugmentedISF::SetOwner(IUnknown* punk)
{
    HRESULT hres = S_OK;

    ASSERT(NULL == punk || IS_VALID_CODE_PTR(punk, IUnknown));

    if (_hdpa && _punkOwner)
        DPA_EnumCallback(_hdpa, CISFElem_SetOwnerCB, NULL);
        
    ATOMICRELEASE(_punkOwner);
    
    if (punk) 
    {
        hres = punk->QueryInterface(IID_IUnknown, (LPVOID *)&_punkOwner);
        
        if (_hdpa)
            DPA_EnumCallback(_hdpa, CISFElem_SetOwnerCB, (void *)_punkOwner);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::EnumObjects method

*/
STDMETHODIMP CAugmentedISF::EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList)
{
    HRESULT hres = E_FAIL;

    if (_hdpa)
    {
        *ppenumIDList = new CAugISFEnum();

        if (*ppenumIDList)
        {
            hres = ((CAugISFEnum *)(*ppenumIDList))->Init(_hdpa, grfFlags);

            if (FAILED(hres))
            {
                delete *ppenumIDList;
                *ppenumIDList = NULL;
            }
        }
        else
            hres = E_OUTOFMEMORY;
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::BindToObject method

*/
STDMETHODIMP CAugmentedISF::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID * ppvOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_WRITE_PTR(ppvOut, LPVOID));

    *ppvOut = NULL;

    UINT id = IDWrap_GetID(pidl);
    IShellFolder * psf = _GetObjectPSF(id);

    if (psf)
    {
        LPITEMIDLIST pidlReal = GetNativePidl(pidl, id);

        if (pidlReal)
        {
            hres = psf->BindToObject(pidlReal, pbcReserved, riid, ppvOut);
            ILFree(pidlReal);
        }
        else
            hres = E_OUTOFMEMORY;

        psf->Release();
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::BindToStorage method

*/
STDMETHODIMP CAugmentedISF::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj)
{
    TraceMsg(TF_WARNING, "Called unimplemented CAugmentedISF::BindToStorage");
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IShellFolder::CompareIDs method

*/
STDMETHODIMP CAugmentedISF::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = 0;
    int nID1 = IDWrap_GetID(pidl1);
    int nID2 = IDWrap_GetID(pidl2);

    if (nID1 == nID2)
    {
        IShellFolder * psf = _GetObjectPSF(nID1);
        if (psf)
        {
            LPITEMIDLIST pidlReal1 = GetNativePidl(pidl1, nID1);

            if (pidlReal1)
            {
                LPITEMIDLIST pidlReal2 = GetNativePidl(pidl2, nID2);
                if (pidlReal2)
                {
                    hres = psf->CompareIDs(lParam, pidlReal1, pidlReal2);
                    ILFree(pidlReal2);
                }
                ILFree(pidlReal1);
            }
            psf->Release();
        }
    }
    else
    {
        //In this situation, we want to see if one of these items wants to be sorted
        // below the other. 
        CISFElem * pel1 = (CISFElem *)DPA_GetPtr(_hdpa, nID1);
        CISFElem * pel2 = (CISFElem *)DPA_GetPtr(_hdpa, nID2);
        DWORD dwel1 = 0;
        DWORD dwel2 = 0;

        if (pel1)
            dwel1 = pel1->GetFlags();

        if (pel2)
            dwel2 = pel2->GetFlags();

        // if both want their items sorted below the other, punt and do neither.
        if ((dwel1 & ASFF_SORTDOWN) ^ (dwel2 & ASFF_SORTDOWN))
            hres = ResultFromShort((dwel1 & ASFF_SORTDOWN)? 1 : -1);
        else
            hres = (nID1 - nID2);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::CreateViewObject method

*/
STDMETHODIMP CAugmentedISF::CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    HRESULT hres = E_FAIL;

    if (_hdpa)
    {
        CVODATA cvodata;

        cvodata.hres = E_FAIL;
        cvodata.hwnd = hwndOwner;
        cvodata.piid = &riid;
        cvodata.ppvObj = ppvOut;
        
        // Whoever responds first wins
        DPA_EnumCallback(_hdpa, CISFElem_CreateViewObjectCB, (void *)&cvodata);

        hres = cvodata.hres;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetAttributesOf method

*/
STDMETHODIMP CAugmentedISF::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                   ULONG * pfInOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_READ_PTR(apidl, LPCITEMIDLIST));
    ASSERT(IS_VALID_WRITE_PTR(pfInOut, ULONG));

    ULONG fInOut = *pfInOut;
    *pfInOut &= 0;

    // We only handle one pidl
    if (1 == cidl && apidl)
    {
        UINT id = IDWrap_GetID(*apidl);
        IShellFolder * psf = _GetObjectPSF(id);

        if (psf)
        {
            LPITEMIDLIST pidlReal = GetNativePidl(*apidl, id);

            if (pidlReal)
            {
                hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlReal, &fInOut);
                *pfInOut = fInOut;
                ILFree(pidlReal);
            }
            else
                hres = E_OUTOFMEMORY;

            psf->Release();
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetUIObjectOf method

*/
STDMETHODIMP CAugmentedISF::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_READ_PTR(apidl, LPCITEMIDLIST));

    *ppvOut = NULL;

    // We only handle one pidl
    if (1 == cidl && apidl)
    {
        UINT id = IDWrap_GetID(*apidl);
        IShellFolder * psf = _GetObjectPSF(id);

        if (psf)
        {
            LPITEMIDLIST pidlReal = GetNativePidl(*apidl, id);

            if (pidlReal)
            {
                hres = psf->GetUIObjectOf(hwndOwner, 1, (LPCITEMIDLIST *)&pidlReal, riid, prgfInOut, ppvOut);
                ILFree(pidlReal);
            }
            else
                hres = E_OUTOFMEMORY;

            psf->Release();
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::GetDisplayNameOf method

*/
STDMETHODIMP CAugmentedISF::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, 
                                             LPSTRRET pstrret)
{
    HRESULT hres = E_FAIL;

    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_WRITE_PTR(pstrret, STRRET));

    if (pidl) 
    {
        UINT id = IDWrap_GetID(pidl);
        IShellFolder * psf = _GetObjectPSF(id);

        if (psf)
        {
            LPITEMIDLIST pidlReal = GetNativePidl(pidl, id);

            if (pidlReal)
            {
                hres = psf->GetDisplayNameOf(pidlReal, uFlags, pstrret);
                ILFree(pidlReal);
            }
            else
                hres = E_OUTOFMEMORY;

            psf->Release();
        }
        else
            hres = E_FAIL;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::SetNameOf method

*/
STDMETHODIMP CAugmentedISF::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                             LPCOLESTR lpszName, DWORD uFlags,
                             LPITEMIDLIST * ppidlOut)
{
    HRESULT hres = E_FAIL;

    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));
    if (pidl) 
    {
        UINT id = IDWrap_GetID(pidl);
        IShellFolder * psf = _GetObjectPSF(id);

        if (psf)
        {
            LPITEMIDLIST pidlReal = GetNativePidl(pidl, id);

            if (pidlReal)
            {
                LPITEMIDLIST pidlOut = NULL;
                hres = psf->SetNameOf(hwndOwner, pidlReal,
                             lpszName, uFlags,
                             &pidlOut);

                // Do they want a pidl back?
                if (SUCCEEDED(hres) && ppidlOut)
                {
                    *ppidlOut = AugISF_WrapPidl( pidlOut, id );

                    if (!*ppidlOut)
                        hres = E_OUTOFMEMORY;
                }

                ILFree(pidlOut);
                ILFree(pidlReal);
            }
            else
                hres = E_OUTOFMEMORY;

            psf->Release();
        }
        else
            hres = E_FAIL;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolder::ParseDisplayName method

*/
STDMETHODIMP CAugmentedISF::ParseDisplayName(HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    TraceMsg(TF_WARNING, "Called unimplemented CAugmentedISF::ParseDisplayNameOf");
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IAugmentedShellFolder::AddNameSpace 

*/
STDMETHODIMP CAugmentedISF::AddNameSpace(const GUID * pguid, 
                                         IShellFolder * psf, LPCITEMIDLIST pidl, DWORD dwFlags)
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(NULL == pguid || IS_VALID_READ_PTR(pguid, GUID));
    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));

    if (NULL == _hdpa)
    {
        _hdpa = DPA_Create(4);
    }

    if (psf && _hdpa)
    {
        hres = S_OK;        // Assume success

        CISFElem * pel = new CISFElem(pguid, psf, dwFlags);
        if (pel)
        {
            hres = pel->SetPidl(pidl);
            if (SUCCEEDED(hres))
            {
                if (DPA_ERR == DPA_AppendPtr(_hdpa, pel))
                    hres = E_OUTOFMEMORY;
            }

            if (FAILED(hres))
                delete pel;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    return hres;
}    


/*----------------------------------------------------------
Purpose: IAugmentedShellFolder::GetNameSpaceID 

*/
STDMETHODIMP CAugmentedISF::GetNameSpaceID(LPCITEMIDLIST pidl, GUID * pguidOut)
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_WRITE_PTR(pguidOut, GUID));

    if (pidl && pguidOut)
    {
        UINT id = IDWrap_GetID(pidl);

        hres = E_FAIL;

        if (_hdpa)
        {
            CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, id);
            if (pel)
            {
                pel->GetNameSpaceID(pguidOut);
                hres = S_OK;
            }
        }
    }

    return hres;
}    


/*----------------------------------------------------------
Purpose: IAugmentedShellFolder::QueryNameSpace

*/
STDMETHODIMP CAugmentedISF::QueryNameSpace(DWORD dwID, GUID * pguidOut, 
                                           IShellFolder ** ppsf)
{
    HRESULT hres = E_FAIL;

    ASSERT(NULL == pguidOut || IS_VALID_WRITE_PTR(pguidOut, GUID));
    ASSERT(NULL == ppsf || IS_VALID_WRITE_PTR(ppsf, IShellFolder));

    if (_hdpa)
    {
        CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, dwID);
        if (pel)
        {
            if (ppsf)
            {
                IShellFolder * psf = pel->GetPSF();
                psf->AddRef();
                *ppsf = psf;
            }

            if (pguidOut)
                pel->GetNameSpaceID(pguidOut);

            hres = S_OK;
        }
    }

    return hres;
}    


/*----------------------------------------------------------
Purpose: IAugmentedShellFolder::EnumNameSpace 

*/
STDMETHODIMP CAugmentedISF::EnumNameSpace(DWORD uNameSpace, DWORD * pdwID)
{
    HRESULT hres = E_FAIL;

    if (_hdpa)
    {
        DWORD celem = DPA_GetPtrCount(_hdpa);

        if (-1 == uNameSpace)
            hres = celem;
        else
        {
            if (uNameSpace >= celem)
                hres = E_FAIL;
            else
            {
                // For now, simply use the index given
                *pdwID = uNameSpace;
                hres = S_OK;
            }
        }
    }

    return hres;
}    



/*----------------------------------------------------------
    Purpose: ITranslateShellChangeNotify::TranslateIDs 
*/
STDMETHODIMP CAugmentedISF::TranslateIDs(LONG *plEvent, 
                                LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, 
                                LPITEMIDLIST * ppidlOut1, LPITEMIDLIST * ppidlOut2,
                                LONG *plEvent2, LPITEMIDLIST *ppidlOut1Event2, 
                                LPITEMIDLIST *ppidlOut2Event2)
{
    HRESULT hres = S_OK;

    *plEvent2 = (LONG)-1;

    *ppidlOut1Event2 = NULL;
    *ppidlOut2Event2 = NULL;


    *ppidlOut1 = (LPITEMIDLIST)pidl1;
    *ppidlOut2 = (LPITEMIDLIST)pidl2;

    if (_hdpa)
    {
        int cElem = DPA_GetPtrCount(_hdpa);
        int i;

        // Go thru all the namespaces and find which one should 
        // translate this notification
        for (i = 0; i < cElem; i++)
        {
            CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpa, i);
            if (pel)
            {
                LPCITEMIDLIST pidlNS = pel->GetPidl();

                if (pidlNS)
                {
                    if (pidl1)
                    {
                        *ppidlOut1 = TranslatePidl(pidlNS, pidl1, i);
                        if (NULL == *ppidlOut1)
                            hres = E_OUTOFMEMORY;
                    }

                    if (SUCCEEDED(hres) && pidl2)
                    {
                        *ppidlOut2 = TranslatePidl(pidlNS, pidl2, i);
                        if (NULL == *ppidlOut2)
                            hres = E_OUTOFMEMORY;
                    }

                    if (FAILED(hres))
                    {
                        if (*ppidlOut1 != pidl1)
                            Pidl_Set(ppidlOut1, NULL);

                        if (*ppidlOut2 != pidl2)
                            Pidl_Set(ppidlOut2, NULL);
                        break;
                    }
                    else
                    {
                        if (*ppidlOut1 != pidl1 || *ppidlOut2 != pidl2)
                            break;
                    }
                }
            }
        }
    }

    return hres;
}    


/*----------------------------------------------------------
Purpose: ITranslateShellChangeNotify::IsChildID
*/
STDMETHODIMP CAugmentedISF::IsChildID(LPCITEMIDLIST pidlKid, BOOL fImmediate)
{
    HRESULT hres = S_FALSE;
    //At this point we should have a translated pidl
    if (pidlKid)
    {
        // Weirdness: If fImmediate is TRUE, then this is a Wrapped pidl. If it's
        // false, then it's not, and we need to check to see if it's a Real FS Child.
        if (fImmediate)
        {
            LPCITEMIDLIST pidlRelKid = ILFindLastID(pidlKid);
            if (pidlRelKid)
            {
                int nID = IDWrap_GetID(pidlRelKid);
                CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, nID);
                if (pel && pel->GetPidl())
                {
                
                    if (ILIsParent(pel->GetPidl(), pidlKid, TRUE))
                        hres = S_OK;
                }
            }
        }
        else
        {
            int cElem = DPA_GetPtrCount(_hdpa);
            int i;

            for (i = 0; i < cElem; i++)
            {
                CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, i);
                if (pel && pel->GetPidl())
                {
                    if (ILIsParent(pel->GetPidl(), pidlKid, FALSE))
                    {
                        hres = S_OK;
                        break;
                    }
                }
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: ITranslateShellChangeNotify::IsEqualID

*/
STDMETHODIMP CAugmentedISF::IsEqualID(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int cElem = DPA_GetPtrCount(_hdpa);
    int i;

    for (i = 0; i < cElem; i++)
    {
        CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpa, i);

        if (pel)
        {
            if (pidl1)
            {
                if (ILIsEqual(pel->GetPidl(),pidl1))
                    return S_OK;
            }
            else if (pidl2)
            {
                if (ILIsParent(pidl2, pel->GetPidl(), FALSE))
                    return S_OK;
            }
        }
    }

    return S_FALSE;
}

/*----------------------------------------------------------
Purpose: ITranslateShellChangeNotify::Register
    Registers all pidls contained to the passed in window

*/
STDMETHODIMP CAugmentedISF::Register(HWND hwnd, UINT uMsg, long lEvents)
{
    HRESULT hres = NOERROR;
    if (_hdpa)
    {
        int cElem = DPA_GetPtrCount(_hdpa);
        int i;

        for (i = 0; i < cElem; i++)
        {
            CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpa, i);

            // Has this namespace been registered yet?
            if (pel && 0 == pel->GetRegister())
            {
                // No; register it
                LPCITEMIDLIST pidlNS = pel->GetPidl();

                if (pidlNS)
                {
                    pel->SetRegister(RegisterNotify(hwnd, uMsg, pidlNS, lEvents,
                                                    SHCNRF_ShellLevel | SHCNRF_InterruptLevel, TRUE));
                }
            }
        }
    }
    else
        hres = E_FAIL;

    return hres;

}

/*----------------------------------------------------------
Purpose: ITranslateShellChangeNotify::Unregister

*/
STDMETHODIMP CAugmentedISF::Unregister()
{
    HRESULT hres = NOERROR;
    if (_hdpa)
    {
        int cElem = DPA_GetPtrCount(_hdpa);
        int i;

        for (i = 0; i < cElem; i++)
        {
            CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpa, i);
            UINT uReg;
            if (pel && (uReg = pel->GetRegister()) != 0)
            {
                // SHChangeNotifyDeregister will flush messages
                // which will send a notify which will come back here...
                pel->SetRegister(0);
                SHChangeNotifyDeregister(uReg);
            }
        }
    }
    else
        hres = E_FAIL;

    return hres;

}

/*----------------------------------------------------------
Purpose: Returns the psf associated with the ID.

*/
IShellFolder * CAugmentedISF::_GetObjectPSF(int nID)
{
    IShellFolder * psf = NULL;

    if (_hdpa)
    {
        CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpa, nID);
        if (pel)
        {
            psf = pel->GetPSF();
            ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));

            psf->AddRef();
        }
    }
    return psf;
}    



//
//  CAugISF Enumerator object
//

#undef SUPERCLASS


// Constructor
CAugISFEnum::CAugISFEnum() :
   _cRef(1)
{
}


// Destructor
CAugISFEnum::~CAugISFEnum()
{
    if (_hdpaISF)
        DPA_DestroyCallback(_hdpaISF, CISFElem_DestroyCB, NULL);
}


HRESULT CAugISFEnum::Init(HDPA hdpaISF, DWORD dwEnumFlags)
{
    HRESULT hres = S_OK;

    ASSERT(IS_VALID_HANDLE(hdpaISF, DPA));

    // Clone the DPA
    _hdpaISF = DPA_Clone(hdpaISF, NULL);
    if (_hdpaISF)
    {
        // Clone the elements too
        int cElem = DPA_GetPtrCount(_hdpaISF);
        int i;

        // If something fails in the loop, at least try to enumerate 
        // other namespaces.
        for (i = 0; i < cElem; i++)
        {
            CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpaISF, i);
            if (pel)
            {
                CISFElem * pelNew = pel->Clone();
                if (pelNew)
                {
                    // Get the enumerator
                    if (SUCCEEDED(pelNew->AcquireEnumerator(dwEnumFlags)))
                        DPA_SetPtr(_hdpaISF, i, pelNew);
                    else
                    {
                        TraceMsg(TF_WARNING, "CAugISFEnum::Init.  Namespace %d has no enumerator.", i);

                        // Remove it from the list to enumerate, and continue
                        DPA_DeletePtr(_hdpaISF, i);
                        cElem--;
                        i--;
                        delete pelNew;
                    }
                }
            }
        }
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}    


STDMETHODIMP CAugISFEnum::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAugISFEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CAugISFEnum::AddRef()
{
    return ++_cRef;
}


STDMETHODIMP_(ULONG) CAugISFEnum::Release()
{
    if (--_cRef > 0) {
        return _cRef;
    }

    delete this;
    return 0;
}


/*----------------------------------------------------------
Purpose: IEnumIDList::Next method

         This will call the current enumerator for the next
         object.  The object's pidl is wrapped in an IDWRAP
         (which is stamped with the identifier of the specific
         IShellFolder the object belongs to) and handed back.
         
         If the current enumerator has no more items to return,
         this function will call the next enumerator for its
         first item, and returns that.  The subsequent call 
         will pick up from there.

*/
STDMETHODIMP CAugISFEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    ULONG celtFetched = 0;
    HRESULT hres = S_FALSE;

    if (celt > 0)
    {
        IEnumIDList * pei = _GetObjectEnumerator(_iCurISF);
        if (pei)
        {
            LPITEMIDLIST pidl;

            hres = pei->Next(1, &pidl, &celtFetched);

            if (SUCCEEDED(hres))
            {
                // End of enumeration for this object?
                if (S_FALSE == hres)
                {
                    // Yes; go to next ISF object
                    _iCurISF++;
                    hres = Next(celt, rgelt, &celtFetched);
                }
                else
                {
                    // No; now wrap the pidl.  
                    rgelt[0] = AugISF_WrapPidl( pidl, _iCurISF );
                    if (rgelt[0]) 
                    {
                        celtFetched = 1;
                        hres = S_OK;
                    } 
                    else 
                        hres = E_OUTOFMEMORY;

                    ILFree(pidl);
                }
            }

            pei->Release();
        }
    }
    
    if (pceltFetched) 
        *pceltFetched = celtFetched;

    return hres;
}


STDMETHODIMP CAugISFEnum::Skip(ULONG celt)
{
    return E_NOTIMPL;
}


STDMETHODIMP CAugISFEnum::Reset()
{
    if (_hdpaISF)
    {
        // Reset all the enumerators
        int cel = DPA_GetPtrCount(_hdpaISF);
        int i;

        for (i = 0; i < cel; i++)
        {
            CISFElem * pel = (CISFElem *)DPA_FastGetPtr(_hdpaISF, i);
            if (pel)
            {
                IEnumIDList * pei = pel->GetEnumerator();
                if (pei)
                {
                    pei->Reset();
                    // Don't Release b/c GetEnumerator doesn't AddRef
                }
            }
        }
    }

    _iCurISF = 0;

    return S_OK;
}


STDMETHODIMP CAugISFEnum::Clone(IEnumIDList **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: Returns the enumerator associated with the ID.

*/
IEnumIDList * CAugISFEnum::_GetObjectEnumerator(int nID)
{
    IEnumIDList * pei = NULL;

    if (_hdpaISF)
    {
        CISFElem * pel = (CISFElem *)DPA_GetPtr(_hdpaISF, nID);
        if (pel)
        {
            pei = pel->GetEnumerator();
            ASSERT(IS_VALID_CODE_PTR(pei, IEnumIDList));

            pei->AddRef();
        }
    }
    return pei;
}    


/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
STDAPI CAugmentedISF_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres;
    CAugmentedISF* pObj;

    hres = E_OUTOFMEMORY;

    pObj = new CAugmentedISF();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IShellFolder *);
        hres = S_OK;
    }

    return hres;
}
