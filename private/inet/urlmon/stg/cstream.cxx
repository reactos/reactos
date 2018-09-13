//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CSTREAM.CXX
//
//  Contents:
//
//  Classes:    Implements the IStream class.
//
//  Functions:
//
//  History:    12-01-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "cstream.hxx"

CStream::CStream(IStream *pStr) : _CRefs()
{
    _pStream = pStr;
}

STDMETHODIMP CStream::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::QueryInterface\n", this));

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

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CStream::AddRef(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::AddRef\n", this));

    LONG lRet = ++_CRefs;

    if (_pStream)
        _pStream->AddRef();

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::AddRef\n", this));
    return lRet;
}

STDMETHODIMP_(ULONG) CStream::Release(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_pStream)
        _pStream->Release();

    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Release\n", this));
    return lRet;
}

HRESULT CStream::Read(THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = E_FAIL;
    ULONG   readcount;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Read\n", this));

    if (_pStream)
    {
#if 0
        while (TRUE)
        {
#endif
            hresult = _pStream->Read(pv, cb, &readcount);
            if (pcbRead)
                *pcbRead = readcount;

#if 0
            if (hresult != NOERROR)
                break;

            if (readcount == cb)
                break;

            hresult = BlockOnIFillLockBytes(this);  // BUGBUG:
            if (hresult != S_OK)
                break;
        }
#endif
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Read\n", this));
    return(hresult);
}

HRESULT CStream::Write(THIS_ VOID const HUGEP *pv, ULONG cb,
            ULONG FAR *pcbWritten)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Write\n", this));

    if (_pStream)
        hresult = _pStream->Write(pv, cb, pcbWritten);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Write\n", this));
    return(hresult);
}

HRESULT CStream::Seek(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Seek\n", this));

    if (_pStream)
        hresult = _pStream->Seek(dlibMove, dwOrigin, plibNewPosition);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Seek\n", this));
    return(hresult);
}

HRESULT CStream::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::SetSize\n", this));

    if (_pStream)
        hresult = _pStream->SetSize(libNewSize);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::SetSize\n", this));
    return(hresult);
}

HRESULT CStream::CopyTo(THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::CopyTo\n", this));

    if (_pStream)
        hresult = _pStream->CopyTo(pStm, cb, pcbRead, pcbWritten);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::CopyTo\n", this));
    return(hresult);
}

HRESULT CStream::Commit(THIS_ DWORD dwCommitFlags)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Commit\n", this));

    if (_pStream)
        hresult = _pStream->Commit(dwCommitFlags);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Commit\n", this));
    return(hresult);
}

HRESULT CStream::Revert(THIS)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Revert\n", this));

    if (_pStream)
        hresult = _pStream->Revert();

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Revert\n", this));
    return(hresult);
}

HRESULT CStream::LockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::LockRegion\n", this));

    if (_pStream)
        hresult = _pStream->LockRegion(libOffset, cb, dwLockType);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::LockRegion\n", this));
    return(hresult);
}

HRESULT CStream::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::UnlockRegion\n", this));

    if (_pStream)
        hresult = _pStream->UnlockRegion(libOffset, cb, dwLockType);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::UnlockRegion\n", this));
    return(hresult);
}

HRESULT CStream::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Stat\n", this));

    if (_pStream)
        hresult = _pStream->Stat(pStatStg, grfStatFlag);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Stat\n", this));
    return(hresult);
}

HRESULT CStream::Clone(THIS_ LPSTREAM FAR *ppStm)
{
    HRESULT hresult = E_FAIL;
    IStream *pStr, *pStrTop;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStream::Clone\n", this));

    *ppStm = NULL;

    if (_pStream)
    {
        hresult = _pStream->Clone(&pStr);
        if (hresult == S_OK)
        {
            if (!(pStrTop = new CStream(pStr)))
            {
                // BUGBUG: Delete pStr here!

                hresult = E_OUTOFMEMORY;
                goto CloneExit;
            }

            *ppStm = pStrTop;
        }
    }

CloneExit:

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStream::Clone\n", this));
    return(hresult);
}

