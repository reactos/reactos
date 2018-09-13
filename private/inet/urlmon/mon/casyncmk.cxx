//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CAsyncMk.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//  Note: this class servers as a base class for async moniker
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include "casyncmk.hxx"

//  The derived class must implement this method
inline HRESULT DerivedMustImplement( void )
{
    return E_NOTIMPL;
}

STDMETHODIMP CAsyncMoniker::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "%p IN CAsyncMoniker::QueryInterface\n", this));

    HRESULT hresult = NOERROR;
    // Do not validate input as it has already been validated
    // by derived classes.

    if (   IsEqualIID(riid, IID_IMoniker)
        || IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IPersistStream)
        || IsEqualIID(riid, IID_IInternalMoniker)
        || IsEqualIID(riid, IID_IAsyncMoniker)
        || IsEqualIID(riid, IID_IAsyncURLMoniker)
       )
    {
        *ppvObj = this;
        InterlockedIncrement((long *)&m_refs);
    }
    else
    {
        *ppvObj = NULL;
        hresult = E_NOINTERFACE;
    }

    UrlMkDebugOut((DEB_ASYNCMONIKER, "%p OUT CAsyncMoniker::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CAsyncMoniker::AddRef ()
{
    ULONG crefs;
    UrlMkDebugOut((DEB_ASYNCMONIKER, "%p IN CAsyncMoniker::AddRef(%ld)\n", this, m_refs));

    crefs = InterlockedIncrement((long *)&m_refs);

    UrlMkDebugOut((DEB_ASYNCMONIKER, "%p OUT CAsyncMoniker::AddRef(%ld)\n", this, m_refs));
    return crefs;
}

STDMETHODIMP CAsyncMoniker::IsDirty (THIS)
{
    VDATETHIS(this);
    //  monikers are immutable so they are either always dirty or never dirty.
    HRESULT hresult = S_FALSE;
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN  CAsyncMoniker::(%p)\n", this));

    UrlMkDebugOut((DEB_ASYNCMONIKER, "OUT CAsyncMoniker::(%p) hr (%x)\n", this, hresult));
    return hresult;
}

STDMETHODIMP CAsyncMoniker::Load (THIS_ LPSTREAM pStm)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::Save (THIS_ LPSTREAM pStm,
            BOOL fClearDirty)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::GetSizeMax (THIS_ ULARGE_INTEGER FAR * pcbSize)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

    // *** IMoniker methods ***
STDMETHODIMP CAsyncMoniker::BindToObject (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    REFIID riidResult, LPVOID FAR* ppvResult)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::BindToStorage (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::Reduce (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
    ppmkToLeft, LPMONIKER FAR * ppmkReduced)
{
    VDATETHIS(this);
    *ppmkReduced = this;
    AddRef();
    return ResultFromScode(MK_S_REDUCED_TO_SELF);
}

STDMETHODIMP CAsyncMoniker::ComposeWith (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
    LPMONIKER FAR* ppmkComposite)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::Hash (THIS_ LPDWORD pdwHash)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::GetTimeOfLastChange (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    FILETIME FAR* pfiletime)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::Inverse (THIS_ LPMONIKER FAR* ppmk)
{
    VDATETHIS(this);
    *ppmk = NULL;
    return ResultFromScode(MK_E_NOINVERSE);
}

STDMETHODIMP CAsyncMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkPrefix)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkRelPath)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::GetDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    LPWSTR FAR* lplpszDisplayName)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::ParseDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
    LPMONIKER FAR* ppmkOut)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ASYNCMONIKER, "IN/OUT CAsyncMoniker::(%p) hr=E_NOTIMPL\n", this));
    return DerivedMustImplement();
}

STDMETHODIMP CAsyncMoniker::IsSystemMoniker (THIS_ LPDWORD pdwMksys)
{
    VDATEPTROUT (pdwMksys, DWORD);
    *pdwMksys = 0;

    return NOERROR;
}

STDMETHODIMP CAsyncMoniker::IsRunning (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
              LPMONIKER pmkNewlyRunning)
{
    VDATETHIS(this);
    VDATEIFACE (pbc);
    LPRUNNINGOBJECTTABLE pROT;
    HRESULT hresult = S_FALSE;

    return hresult;
}



