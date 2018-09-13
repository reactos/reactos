//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CLOCKBYT.CXX
//
//  Contents:
//
//  Classes:    Implements the ILockBytes base class.
//
//  Functions:
//
//  History:    12-01-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "clockbyt.hxx"

CLockBytes::CLockBytes() : _CRefs()
{
}

STDMETHODIMP CLockBytes::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CLockBytes::QueryInterface\n", this));

    HRESULT hresult = NOERROR;

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_ILockBytes)
       )
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        hresult = E_NOINTERFACE;
    }

    if (*ppvObj)
        AddRef();

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CLockBytes::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CLockBytes::AddRef(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CLockBytes::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CLockBytes::AddRef\n", this));
    return lRet;
}

HRESULT CLockBytes::Flush()
{
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p CLockBytes::Flush (NoOp)\n", this));
    return(NOERROR);
}

HRESULT CLockBytes::LockRegion(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p CLockBytes::LockRegion (NoOp)\n", this));
    return(NOERROR);
}

HRESULT CLockBytes::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p CLockBytes::UnlockRegion (NoOp)\n", this));
    return(NOERROR);
}

HRESULT CLockBytes::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p CLockBytes::Stat (NoOp)\n", this));
    return(STG_E_UNKNOWN);
}

