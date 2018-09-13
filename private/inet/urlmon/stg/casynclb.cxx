//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CASYNCLB.CXX
//
//  Contents:
//
//  Classes:    Implements the IAsyncLockBytes class.
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "casynclb.hxx"

CAsyncLockBytes::CAsyncLockBytes(ILockBytes *pLB) : _CRefs()
{
    _cbFillOffset.QuadPart = 0;
    _fFillDone = FALSE;
    _pLBchain = pLB;
}

STDMETHODIMP CAsyncLockBytes::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::QueryInterface\n", this));

    HRESULT hresult = NOERROR;

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_ILockBytes)
       )
    {
        *ppvObj = (ILockBytes *)this;
    }
    else if (IsEqualIID(riid, IID_IFillLockBytes))
    {
        *ppvObj = (IFillLockBytes *)this;
    }
    else
    {
        *ppvObj = NULL;
        hresult = E_NOINTERFACE;
    }

    if (*ppvObj)
    {
        AddRef();

        if (_pLBchain)
            _pLBchain->AddRef();
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CAsyncLockBytes::AddRef(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::AddRef\n", this));

    LONG lRet = ++_CRefs;

    if (_pLBchain)
        _pLBchain->AddRef();

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::AddRef\n", this));
    return lRet;
}

STDMETHODIMP_(ULONG) CAsyncLockBytes::Release(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_pLBchain)
        _pLBchain->Release();

    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::Release\n", this));
    return lRet;
}

HRESULT CAsyncLockBytes::ReadAt(THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,
    ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::ReadAt\n", this));

    if (_pLBchain)
    {
#if 0
        while (TRUE)
        {
#endif
            hresult = _pLBchain->ReadAt(ulOffset, pv, cb, pcbRead);
#if 0
            if (hresult != NOERROR)
                break;

            if (*pcbRead == cb || _fFillDone)
                break;

            hresult = BlockOnIFillLockBytes(this);
            if (hresult != S_OK)
                break;
        }
#endif
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::ReadAt\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::WriteAt(THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
    ULONG cb, ULONG FAR *pcbWritten)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::WriteAt\n", this));

    if (_pLBchain)
        hresult = _pLBchain->WriteAt(ulOffset, pv, cb, pcbWritten);

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::WriteAt\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::Flush()
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::Flush\n", this));

    if (_pLBchain)
        hresult = _pLBchain->Flush();

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::Flush\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::SetSize(THIS_ ULARGE_INTEGER cb)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::SetSize\n", this));

    if (_pLBchain)
        hresult = _pLBchain->SetSize(cb);

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::SetSize\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::LockRegion(THIS_ ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::LockRegion\n", this));

    if (_pLBchain)
        hresult = _pLBchain->LockRegion(libOffset, cb, dwLockType);

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::LockRegion\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::UnlockRegion\n", this));

    if (_pLBchain)
        hresult = _pLBchain->UnlockRegion(libOffset, cb, dwLockType);

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::UnlockRegion\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::Stat\n", this));

    if (_pLBchain)
        hresult = _pLBchain->Stat(pStatStg, grfStatFlag);

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::Stat\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::FillAppend(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::FillAppend\n", this));

    if (!_fFillDone)
    {
        hresult = WriteAt(_cbFillOffset, pv, cb, pcbWritten);
        _cbFillOffset.QuadPart += *pcbWritten;
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::FillAppend\n", this));
    return(hresult);
}

HRESULT CAsyncLockBytes::Terminate(BOOL bCanceled)
{
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN CAsyncLockBytes::Terminate\n", this));

    _fFillDone = TRUE;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT CAsyncLockBytes::Terminate\n", this));
    return(NOERROR);
}

