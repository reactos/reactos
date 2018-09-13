//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ROSTMDIR.CXX
//
//  Contents:
//
//  Classes:    Implements the CReadOnlyStreamDirect class.
//
//  Functions:
//
//  History:    12-21-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#ifndef unix
#include "..\trans\transact.hxx"
#else
#include "../trans/transact.hxx"
#endif /* unix */
#include "rostmdir.hxx"

CReadOnlyStreamDirect::CReadOnlyStreamDirect(CTransData *pCTransData, DWORD grfBindF) : _CRefs()
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN  CReadOnlyStreamDirect::CReadOnlyStreamDirect (pCTransData:%p)\n", NULL,pCTransData));

    _pCTransData = pCTransData;
    _grfBindF = grfBindF;
    if (_pCTransData)
    {
        _pCTransData->AddRef();
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::CReadOnlyStreamDirect \n", this));
}

CReadOnlyStreamDirect::~CReadOnlyStreamDirect()
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN/OUT  CReadOnlyStreamDirect::~CReadOnlyStreamDirect\n", this));
    if (_pCTransData)
    {
        _pCTransData->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CReadOnlyStreamDirect::GetFileName
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    7-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR CReadOnlyStreamDirect::GetFileName()
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::GetFileName\n", this));

    LPWSTR pwzFilename = NULL;
    LPWSTR pwzDupname = NULL;

    if (_pCTransData)
    {
        pwzFilename = _pCTransData->GetFileName();
    }

    if (pwzFilename)
    {
        pwzDupname = OLESTRDuplicate(pwzFilename);
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::GetFileName (Filename:%ws)\n", this, pwzDupname));
    return pwzDupname;
}



STDMETHODIMP CReadOnlyStreamDirect::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::QueryInterface\n", this));

    HRESULT hresult = NOERROR;

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IStream)
       )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hresult = E_NOINTERFACE;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CReadOnlyStreamDirect::AddRef(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::::AddRef (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(ULONG) CReadOnlyStreamDirect::Release(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::Release (%ld)\n", this, lRet));
    return lRet;
}

HRESULT CReadOnlyStreamDirect::Read(THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = NOERROR;
    DWORD dwRead = 0;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::Read\n", this));

    if( _pCTransData )
    {
        hresult = _pCTransData->ReadHere((LPBYTE) pv, cb, &dwRead);
        if( hresult != S_OK && hresult != E_PENDING )
        {
            // break connection with CTransData
            CTransData *pCTD = _pCTransData;
            _pCTransData = 0;
            pCTD->Release();
        }
    }
    else
    {
        hresult = S_FALSE;
        dwRead = 0;
    }

    if (pcbRead)
    {
        *pcbRead = dwRead;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::Read\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamDirect::Write(THIS_ VOID const HUGEP *pv, ULONG cb,
            ULONG FAR *pcbWritten)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::Write (NoOp)\n", this));

    return(STG_E_ACCESSDENIED);
}

HRESULT CReadOnlyStreamDirect::Seek(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition)
{
    HRESULT hresult = NOERROR;
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::Seek (NoOp)\n", this));
    
    // This is a true stream, and thus seeking is not possible.
    if( _pCTransData )
    {
        hresult = _pCTransData->Seek(dlibMove, dwOrigin, plibNewPosition);
    }
    else
    {
        hresult = S_FALSE;
    }

    return hresult;
}

HRESULT CReadOnlyStreamDirect::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::SetSize (NoOp)\n", this));

    // BUGBUG: Should we just return S_OK here?

    return(E_FAIL);
}

HRESULT CReadOnlyStreamDirect::CopyTo(THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten)
{
    HRESULT hresult = STG_E_INSUFFICIENTMEMORY;
    LPVOID  memptr = NULL;
    DWORD   readcount, writecount;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::CopyTo\n", this));

    if (cb.HighPart || (pStm == NULL))
    {
        hresult = E_INVALIDARG;
        goto CopyToExit;
    }

    // do not need to copy to ourself
    if (pStm == this)
    {
        hresult = NOERROR;
        goto CopyToExit;
    }

    memptr = new BYTE [cb.LowPart];

    if (memptr)
    {
        hresult = Read((LPBYTE) memptr, cb.LowPart, &readcount);
        if (hresult)
        {
            goto CopyToExit;
        }

        if (pcbRead)
        {
            pcbRead->HighPart = 0;
            pcbRead->LowPart = readcount;
        }

        hresult = pStm->Write(memptr, readcount, &writecount);

        if (pcbWritten && !hresult)
        {
            pcbWritten->HighPart = 0;
            pcbWritten->LowPart = writecount;
        }
    }

CopyToExit:

    if (memptr)
    {
        delete memptr;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::CopyTo\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamDirect::Commit(THIS_ DWORD dwCommitFlags)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::Commit (NoOp)\n", this));

    // This is a read-only stream, so nothing to commit.

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamDirect::Revert(THIS)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::Revert (NoOp)\n", this));

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamDirect::LockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::LockRegion (NoOp)\n", this));

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamDirect::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamDirect::UnlockRegion (NoOp)\n", this));

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamDirect::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::Stat\n", this));

    if (pStatStg)
    {
        memset(pStatStg, 0, sizeof(STATSTG));

        pStatStg->type = STGTY_STREAM;
        pStatStg->clsid = IID_IStream;
        pStatStg->pwcsName = GetFileName();

        hresult = NOERROR;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::Stat\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamDirect::Clone(THIS_ LPSTREAM FAR *ppStm)
{
    HRESULT hresult = NOERROR;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamDirect::Clone\n", this));

    if (!(*ppStm = new CReadOnlyStreamDirect(_pCTransData, _grfBindF)))
        hresult = E_OUTOFMEMORY;

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamDirect::Clone\n", this));
    return(hresult);
}

