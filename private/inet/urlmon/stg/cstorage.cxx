//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CSTORAGE.CXX
//
//  Contents:
//
//  Classes:    Implements the IStorage class.
//
//  Functions:
//
//  History:    12-20-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "cstorage.hxx"
#include "cstream.hxx"

// Internal Function Prototypes
CStorage    *NewCStorage(IStorage *pStg);


CStorage::CStorage(IStorage *pStorage) : _CRefs()
{
    _pStg = pStorage;
}

STDMETHODIMP CStorage::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::QueryInterface\n", this));

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

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::QueryInterface\n", this));
    return hresult;
}

STDMETHODIMP_(ULONG) CStorage::AddRef(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::AddRef\n", this));

    LONG lRet = ++_CRefs;

    if (_pStg)
        _pStg->AddRef();

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::AddRef\n", this));
    return lRet;
}

STDMETHODIMP_(ULONG) CStorage::Release(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_pStg)
        _pStg->Release();

    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::Release\n", this));
    return lRet;
}

HRESULT CStorage::CreateStream(THIS_ const OLECHAR *pwcsName, DWORD grfMode,
            DWORD dwReserved1, DWORD dwReserved2, LPSTREAM FAR *ppStm)
{
    HRESULT hresult = E_FAIL;
    IStream *pStream;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::CreateStream\n", this));

    if (_pStg)
    {
        hresult = _pStg->CreateStream(pwcsName, grfMode, dwReserved1, dwReserved2,
                    &pStream);

        if (hresult == S_OK)
        {
            if (!(*ppStm = new CStream(pStream)))
            {
                hresult = E_OUTOFMEMORY;

                // BUGBUG: Delete pStream!
            }
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::CreateStream\n", this));
    return(hresult);
}

HRESULT CStorage::OpenStream(THIS_ const OLECHAR *pwcsName,
            void FAR *pReserved1, DWORD grfMode, DWORD dwReserved2,
            LPSTREAM FAR *ppStm)
{
    HRESULT hresult = E_FAIL;
    IStream *pStream;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::OpenStream\n", this));

    if (_pStg)
    {
        hresult = _pStg->OpenStream(pwcsName, pReserved1, grfMode, dwReserved2,
                    &pStream);

        if (hresult == S_OK)
        {
            if (!(*ppStm = new CStream(pStream)))
            {
                hresult = E_OUTOFMEMORY;

                // BUGBUG: Delete pStream!
            }
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::OpenStream\n", this));
    return(hresult);
}


HRESULT CStorage::CreateStorage(THIS_ const OLECHAR *pwcsName, DWORD grfMode,
            DWORD dwReserved1, DWORD dwReserved2, LPSTORAGE FAR *ppStg)
{
    HRESULT     hresult = E_FAIL;
    IStorage    *pStorage;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::CreateStorage\n", this));

    if (_pStg)
    {
        hresult = _pStg->CreateStorage(pwcsName, grfMode, dwReserved1, dwReserved2,
                    &pStorage);

        if (hresult == S_OK)
        {
            if (!(*ppStg = ::NewCStorage(pStorage)))
            {
                hresult = E_OUTOFMEMORY;

                // BUGBUG: Delete pStorage!
            }
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::CreateStorage\n", this));
    return(hresult);
}

HRESULT CStorage::OpenStorage(THIS_ const OLECHAR *pwcsName,
            LPSTORAGE pstgPriority, DWORD grfMode, SNB snbExclude,
            DWORD dwReserved, LPSTORAGE FAR *ppStg)
{
    HRESULT     hresult = E_FAIL;
    IStorage    *pStorage;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::OpenStorage\n", this));

    if (_pStg)
    {
        hresult = _pStg->OpenStorage(pwcsName, pstgPriority, grfMode, snbExclude,
                    dwReserved, &pStorage);

        if (hresult == S_OK)
        {
            if (!(*ppStg = ::NewCStorage(pStorage)))
            {
                hresult = E_OUTOFMEMORY;

                // BUGBUG: Delete pStorage!
            }
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::OpenStorage\n", this));
    return(hresult);
}

HRESULT CStorage::CopyTo(THIS_ DWORD dwCiidExclude, IID const FAR *rgiidExclude,
            SNB snbExclude, LPSTORAGE pStgDest)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::CopyTo\n", this));

    if (_pStg)
        hresult = _pStg->CopyTo(dwCiidExclude, rgiidExclude, snbExclude, pStgDest);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::CopyTo\n", this));
    return(hresult);
}

HRESULT CStorage::MoveElementTo(THIS_ const OLECHAR *lpszName,
            LPSTORAGE pStgDest, const OLECHAR *lpszNewName, DWORD grfFlags)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::MoveElementTo\n", this));

    if (_pStg)
        hresult = _pStg->MoveElementTo(lpszName, pStgDest, lpszNewName, grfFlags);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::MoveElementTo\n", this));
    return(hresult);
}

HRESULT CStorage::Commit(THIS_ DWORD grfCommitFlags)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::Commit\n", this));

    if (_pStg)
        hresult = _pStg->Commit(grfCommitFlags);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::Commit\n", this));
    return(hresult);
}

HRESULT CStorage::Revert(THIS)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::Revert\n", this));

    if (_pStg)
        hresult = _pStg->Revert();

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::Revert\n", this));
    return(hresult);
}

HRESULT CStorage::EnumElements(THIS_ DWORD dwReserved1, void FAR *pReserved2,
            DWORD dwReserved3, LPENUMSTATSTG FAR *ppenumStatStg)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::EnumElements\n", this));

    if (_pStg)
        hresult = _pStg->EnumElements(dwReserved1, pReserved2, dwReserved3,
                    ppenumStatStg);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::EnumElements\n", this));
    return(hresult);
}

HRESULT CStorage::DestroyElement(THIS_ const OLECHAR *pwcsName)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::DestroyElement\n", this));

    if (_pStg)
        hresult = _pStg->DestroyElement(pwcsName);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::DestroyElement\n", this));
    return(hresult);
}

HRESULT CStorage::RenameElement(THIS_ const OLECHAR *pwcsOldName,
            const OLECHAR *pwcsNewName)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::RenameElement\n", this));

    if (_pStg)
        hresult = _pStg->RenameElement(pwcsOldName, pwcsNewName);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::RenameElement\n", this));
    return(hresult);
}

HRESULT CStorage::SetElementTimes(THIS_ const OLECHAR *lpszName,
            FILETIME const FAR *pctime, FILETIME const FAR *patime,
            FILETIME const FAR *pmtime)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::SetElementTimes\n", this));

    if (_pStg)
        hresult = _pStg->SetElementTimes(lpszName, pctime, patime, pmtime);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::SetElementTimes\n", this));
    return(hresult);
}

HRESULT CStorage::SetClass(THIS_ REFCLSID rclsid)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::SetClass\n", this));

    if (_pStg)
        hresult = _pStg->SetClass(rclsid);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::SetClass\n", this));
    return(hresult);
}

HRESULT CStorage::SetStateBits(THIS_ DWORD grfStateBits, DWORD grfMask)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::SetStateBits\n", this));

    if (_pStg)
        hresult = _pStg->SetStateBits(grfStateBits, grfMask);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::SetStateBits\n", this));
    return(hresult);
}

HRESULT CStorage::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT     hresult = E_FAIL;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CStorage::Stat\n", this));

    if (_pStg)
        hresult = _pStg->Stat(pStatStg, grfStatFlag);

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CStorage::Stat\n", this));
    return(hresult);
}

// Internal functions

CStorage    *NewCStorage(IStorage *pStg)
{
    return(new CStorage(pStg));
}

