//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ROSTMFIL.CXX
//
//  Contents:
//
//  Classes:    Implements the CReadOnlyStreamFile class.
//
//  Functions:
//
//  History:    03-02-96    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#ifndef unix
#include "..\trans\transact.hxx"
#else
#include "../trans/transact.hxx"
#endif /* unix */
#include "rostmdir.hxx"
#include "rostmfil.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CReadOnlyStreamFile::GetFileName
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
LPWSTR CReadOnlyStreamFile::GetFileName()
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::GetFileName\n", this));
    LPWSTR pwzDupname = NULL;

    if (_pszFileName)
    {
        pwzDupname = DupA2W(_pszFileName);
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::GetFileName (Filename:%ws)\n", this, pwzDupname));
    return pwzDupname;
}


HRESULT CReadOnlyStreamFile::Create(LPSTR pszFileName, CReadOnlyStreamFile **ppCRoStm)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Create (pszFileName:%s)\n", NULL,pszFileName));
    HRESULT hr = NOERROR;

    *ppCRoStm = NULL;

    if (pszFileName == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LPSTR  pszStr = new CHAR [strlen(pszFileName) + 1];

        if (pszStr == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            HANDLE handle = CreateFileA(pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (handle == INVALID_HANDLE_VALUE)
            {
                delete pszStr;
                hr = E_FAIL;
            }
            else
            {

                strcpy(pszStr, pszFileName);
                CReadOnlyStreamFile *pCRoStm = new CReadOnlyStreamFile(pszStr, handle);

                if (pCRoStm == NULL)
                {
                    delete pszStr;
                }
                else
                {
                    *ppCRoStm = pCRoStm;
                }
            }
        }
    }
    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Create (hr:%lx)\n", *ppCRoStm, hr));
    return hr;
}

CReadOnlyStreamFile::CReadOnlyStreamFile(LPSTR pszFileName, HANDLE handle)
                        : CReadOnlyStreamDirect(NULL, 0)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::CReadOnlyStreamFile (pszFileName:%s)\n", NULL,pszFileName));

    _hFileHandle = handle;
    _pszFileName = pszFileName;
    _fDataFullyAvailable = FALSE;

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::CReadOnlyStreamFile \n", this));
}

CReadOnlyStreamFile::~CReadOnlyStreamFile(void)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::~CReadOnlyStreamFile (_pszFileName:%s)\n", this,_pszFileName));

    if (_hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFileHandle);
    }

    if (_pszFileName)
    {
        delete _pszFileName;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::~CReadOnlyStreamFile\n", this));
}

HRESULT CReadOnlyStreamFile::Read(THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = NOERROR;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Read (cbBuffer:%lx)\n", this,cb));

    DWORD dwRead = 0;

    if (!ReadFile(_hFileHandle, pv, cb, &dwRead, NULL))
    {
        if (GetLastError() == ERROR_LOCK_VIOLATION)
        {
            hresult = STG_E_ACCESSDENIED;
        }
        else
        {
            hresult = E_FAIL;
        }

        if (pcbRead)
        {
            *pcbRead = 0;
        }
    }
    else
    {
        if (pcbRead)
        {
            *pcbRead = dwRead;
        }

        if (dwRead == 0)
        {
            hresult = (_fDataFullyAvailable) ? S_FALSE : E_PENDING;
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Read (hr:%lx, cdRead:%lx)\n", this, hresult, dwRead));
    return(hresult);
}

HRESULT CReadOnlyStreamFile::Seek(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition)
{
    HRESULT hresult = NOERROR;
    DWORD   offslow, offshigh;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Seek\n", this));

    offshigh = dlibMove.HighPart;
    offslow = dlibMove.LowPart;

    offslow = SetFilePointer(_hFileHandle, offslow, (LONG *)&offshigh, dwOrigin);
    if (offslow == -1 && GetLastError() != NO_ERROR)
    {
        hresult = E_FAIL;
    }
    else if (plibNewPosition)
    {
        plibNewPosition->HighPart = offshigh;
        plibNewPosition->LowPart = offslow;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Seek\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamFile::CopyTo(THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten)
{
    HRESULT hresult = STG_E_INSUFFICIENTMEMORY;
    LPVOID  memptr = NULL;
    DWORD   readcount, writecount;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::CopyTo\n", this));

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
        if (!ReadFile(_hFileHandle, memptr, cb.LowPart, &readcount, NULL))
        {
            if (GetLastError() == ERROR_LOCK_VIOLATION)
            {
                hresult = STG_E_ACCESSDENIED;
            }
            else
            {
                hresult = E_FAIL;
            }

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

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::CopyTo\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamFile::LockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamFile::LockRegion (NoOp)\n", this));

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamFile::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamFile::UnlockRegion (NoOp)\n", this));

    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamFile::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT hresult = E_FAIL;
    DWORD   sizelow, sizehigh;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Stat\n", this));

    if (pStatStg)
    {
        memset(pStatStg, 0, sizeof(STATSTG));

        pStatStg->type = STGTY_STREAM;
        pStatStg->clsid = IID_IStream;

        sizelow = GetFileSize(_hFileHandle, &sizehigh);
        if (sizelow == -1 && GetLastError() != NOERROR)
        {
            goto StatExit;
        }

        pStatStg->cbSize.HighPart = sizehigh;
        pStatStg->cbSize.LowPart = sizelow;
        pStatStg->pwcsName = GetFileName();

        if (GetFileTime(_hFileHandle, &pStatStg->ctime, &pStatStg->atime, &pStatStg->mtime))
        {
            pStatStg->grfMode = GENERIC_READ;
            pStatStg->grfLocksSupported = 0;
            pStatStg->clsid = IID_IStream;
            pStatStg->grfStateBits = 0;

            hresult = NOERROR;
        }
    }

StatExit:

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Stat\n", this));
    return(hresult);
}

HRESULT CReadOnlyStreamFile::Clone(THIS_ LPSTREAM FAR *ppStm)
{
    HRESULT hr = NOERROR;
    CReadOnlyStreamFile *pCRoStm;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Clone\n", this));

    hr = Create(_pszFileName, &pCRoStm);

    if (hr == NOERROR)
    {
        *ppStm = pCRoStm;
    }
    else
    {
        *ppStm = NULL;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Clone (hr:%lx)\n", this, hr));
    return hr;
}

