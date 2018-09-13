//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       HTTPCLS.CXX
//
//  Contents:   Implements the HTTPMoniker class, which is derived from
//              the CAsyncMoniker class.
//
//  Classes:    HTTPMoniker
//
//  Functions:
//
//  History:    11-02-95   JoeS (Joe Souza)     Created
//
//----------------------------------------------------------------------------

#include <urlint.h>
#include "urlapi.hxx"
#include "casyncmk.hxx"
#include "httpcls.hxx"

STDMETHODIMP_(ULONG) HTTPMoniker::Release(void)
{
    // Decrement refcount, destroy object if refcount goes to zero.
    // Return the new refcount.

    if (!(--m_refs))
    {
        delete this;
        return(0);
    }

    return(m_refs);
}

STDMETHODIMP HTTPMoniker::GetClassID(CLSID *pClassID)
{
    VDATEPTRIN(pClassID, CLSID);

    *pClassID = CLSID_StdURLMoniker;
    return(NOERROR);
}

STDMETHODIMP HTTPMoniker::BindToStorage (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    REFIID riid, LPVOID FAR* ppvObj)
{
    VDATEPTROUT(ppvObj, LPVOID);
    *ppvObj = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft)
        VDATEIFACE(pmkToLeft);

    HRESULT hresult = NOERROR;

    //if (!::CreateURLBinding(pbc, this, m_pwzURL))
    //if (!CreateURLBinding(pbc, this, m_pwzURL))
    //    hresult = E_OUTOFMEMORY;    // BUGBUG: Should we use a better error code?

    // BUGBUG: Must attach riid interface to new storage, and return storage
    // object in ppvObj.  (I.e. get temp file name and call StgOpenStorage, etc.

    return(hresult);
}

STDMETHODIMP HTTPMoniker::ParseDisplayName (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
    LPMONIKER FAR* ppmkOut)
{
    VDATEPTROUT(ppmkOut, LPMONIKER);
    *ppmkOut = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft)
        VDATEIFACE(pmkToLeft);
    VDATEPTRIN(lpszDisplayName, char);
    VDATEPTROUT(pchEaten, ULONG);

    HRESULT hresult = NOERROR;
    int     len;

    if (m_pwzURL)
        delete [] m_pwzURL;

    len = wcslen(lpszDisplayName);

    m_pwzURL = new WCHAR [len + 1];
    if (!m_pwzURL)
        return(E_OUTOFMEMORY);
    wcscpy(m_pwzURL, lpszDisplayName);

    *pchEaten = len;
    *ppmkOut = this;
    AddRef();
    return(hresult);
}


