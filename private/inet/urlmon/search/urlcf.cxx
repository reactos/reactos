//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlcf.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <stdio.h>
#include "urlcf.hxx"

extern GUID CLSID_ResProtocol;


//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::Create
//
//  Synopsis:
//
//  Arguments:  [clsid] --
//              [ppCF] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CUrlClsFact::Create(REFCLSID clsid, CUrlClsFact **ppCF)
{
    UrlMkDebugOut((DEB_URLMON, "NULL _IN CUrlClsFact::Create\n"));
    HRESULT hr = NOERROR;
    CUrlClsFact * pCF =  NULL;

    if (    (clsid == CLSID_ResProtocol) )
    {
        pCF = (CUrlClsFact *) new CUrlClsFact(clsid);
    }

    if (pCF == NULL)
    {
        UrlMkAssert((pCF));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppCF = pCF;
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::Create (hr:%lx\n", pCF,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CUrlClsFact
//
//  Synopsis:   constructor
//
//  Arguments:  [clsid] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      we need to keep a refcount on the dll if for each object given to
//              outside, including ClassFactories.
//              The corresponding DllRelease is in the destructor
//
//----------------------------------------------------------------------------
CUrlClsFact::CUrlClsFact(REFCLSID clsid) : _CRefs(), _CLocks(0)
{
    _ClsID =  clsid;
    DllAddRef();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::~CUrlClsFact
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CUrlClsFact::~CUrlClsFact()
{
    DllRelease();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CreateInstance
//
//  Synopsis:   creates an instance of an Explode Object
//
//  Arguments:  [pUnkOuter] -- controlling unknown (must be NULL)
//              [riid] --      id of desired interface
//              [ppv] --       pointer to receive the interface
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      S_OK - success
//              CLASS_E_NOAGGREATION - the caller tried to aggregate
//              CLASS_E_CLASSNOTAVAILABLE - couldn't initialize the class
//              E_OUTOFMEMORY - not enough memory to instantiate class
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppv)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::CreateInstance\n", this));
    HRESULT hr = NOERROR;
    // Class factory init time, the pointer to the creation function of
    // the object is given.  Use that to create the object

    //DumpIID(riid);
    //DumpIID(_rClsID);

    if (riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
        AddRef();
    }
    else if (_ClsID == CLSID_ResProtocol)
    {

        hr = CreateAPP(_ClsID, pUnkOuter, riid, (IUnknown **)ppv);

    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::CreateInstance (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::LockServer
//
//  Synopsis:   locks the server, preventing it from being unloaded
//
//  Arguments:  [fLock] -- TRUE to lock, FALSE to unlock
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::LockServer(BOOL fLock)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::LockServer\n", this));
    HRESULT hr = NOERROR;
    if (fLock)
    {
        if (++_CLocks == 1)
        {
            DllAddRef();
        }
    }
    else
    {
        UrlMkAssert((_CLocks > 0));
        if (_CLocks > 0)
        {
            if (--_CLocks == 0)
            {
                DllRelease();
            }
        }
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::LockServer (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    HRESULT hr = NOERROR;
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::QueryInterface\n", this));

    if (   riid == IID_IUnknown
        || riid == IID_IClassFactory)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    if (hr == NOERROR)
    {
        AddRef();
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::AddRef(void)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::Release(void)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::Release (cRefs:%ld)\n", this,lRet));
    return lRet;
}

#if DBG==1
HRESULT DumpIID(REFIID riid)
{

    HRESULT hr;
    LPOLESTR pszStr = NULL;
    hr = StringFromCLSID(riid, &pszStr);
    UrlMkDebugOut((DEB_BINDING, "API >>> DumpIID (riid:%ws) \n", pszStr));

    if (pszStr)
    {
        delete pszStr;
    }
    return hr;
}
#endif




