//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       FILELB.CXX
//
//  Contents:
//
//  Classes:    Implements the file-based ILockBytes class.
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "clockbyt.hxx"
#include "filelb.hxx"

FileLockBytes::FileLockBytes(HANDLE filehandle) : _CRefs()
{
    _hFileHandle = filehandle;
}

STDMETHODIMP_(ULONG) FileLockBytes::Release(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        if (_hFileHandle != INVALID_HANDLE_VALUE)
            CloseHandle(_hFileHandle);

        delete this;
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::Release\n", this));
    return lRet;
}

HRESULT FileLockBytes::ReadAt(THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,
    ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::ReadAt\n", this));

    *pcbRead = 0;

    if (!cb)
        goto ReadAtExit;

    if (_hFileHandle == INVALID_HANDLE_VALUE)
        goto ReadAtExit;

    hresult = seekfile(ulOffset);
    if (hresult != NOERROR)
        goto ReadAtExit;

    if (!ReadFile(_hFileHandle, pv, cb, pcbRead, NULL))
    {
        if (GetLastError() == ERROR_LOCK_VIOLATION)
            hresult = STG_E_ACCESSDENIED;
        else
            hresult = E_FAIL;
    }

ReadAtExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::ReadAt\n", this));
    return(hresult);
}

HRESULT FileLockBytes::WriteAt(THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
    ULONG cb, ULONG FAR *pcbWritten)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::WriteAt\n", this));

    *pcbWritten = 0;

    if (!cb)
        goto WriteAtExit;

    if (_hFileHandle == INVALID_HANDLE_VALUE)
        goto WriteAtExit;

    hresult = seekfile(ulOffset);
    if (hresult != NOERROR)
        goto WriteAtExit;

    if (!WriteFile(_hFileHandle, pv, cb, pcbWritten, NULL))
    {
        if (GetLastError() == ERROR_LOCK_VIOLATION)
            hresult = STG_E_ACCESSDENIED;
        else
            hresult = E_FAIL;
    }

WriteAtExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::WriteAt\n", this));
    return(hresult);
}

HRESULT FileLockBytes::Flush()
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::Flush\n", this));

    if (_hFileHandle != INVALID_HANDLE_VALUE)
    {
        if (FlushFileBuffers(_hFileHandle))
            hresult = NOERROR;
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::Flush\n", this));
    return(hresult);
}

HRESULT FileLockBytes::SetSize(THIS_ ULARGE_INTEGER cb)
{
    HRESULT hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::SetSize\n", this));

    if (_hFileHandle == INVALID_HANDLE_VALUE)
        goto SetSizeExit;

    hresult = seekfile(cb);
    if (hresult != NOERROR)
        goto SetSizeExit;

    if (SetEndOfFile(_hFileHandle))
        hresult = NOERROR;

SetSizeExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::SetSize\n", this));
    return(hresult);
}

HRESULT FileLockBytes::LockRegion(THIS_ ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = NOERROR;
    DWORD   offslow, offshigh, lenlow, lenhigh;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::LockRegion\n", this));

    if (_hFileHandle == INVALID_HANDLE_VALUE)
    {
        hresult = E_FAIL;
        goto LockRegionExit;
    }

    offshigh = *(DWORD *)(&libOffset.QuadPart + sizeof(ULONG));
    offslow = *(DWORD *)(&libOffset.QuadPart);

    lenhigh = *(DWORD *)(&cb.QuadPart + sizeof(ULONG));
    lenlow = *(DWORD *)(&cb.QuadPart);

    if (!LockFile(_hFileHandle, offslow, offshigh, lenlow, lenhigh))
    {
        if (GetLastError() == ERROR_LOCK_FAILED)
            hresult = STG_E_LOCKVIOLATION;
        else
            hresult = E_FAIL;
    }

LockRegionExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::LockRegion\n", this));
    return(hresult);
}

HRESULT FileLockBytes::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb, DWORD dwLockType)
{
    HRESULT hresult = NOERROR;
    DWORD   offslow, offshigh, lenlow, lenhigh;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::UnlockRegion\n", this));

    if (_hFileHandle == INVALID_HANDLE_VALUE)
    {
        hresult = E_FAIL;
        goto UnlockRegionExit;
    }

    offshigh = *(DWORD *)(&libOffset.QuadPart + sizeof(ULONG));
    offslow = *(DWORD *)(&libOffset.QuadPart);

    lenhigh = *(DWORD *)(&cb.QuadPart + sizeof(ULONG));
    lenlow = *(DWORD *)(&cb.QuadPart);

    if (!UnlockFile(_hFileHandle, offslow, offshigh, lenlow, lenhigh))
    {
        if (GetLastError() == ERROR_LOCK_FAILED)
            hresult = STG_E_LOCKVIOLATION;
        else
            hresult = E_FAIL;
    }

UnlockRegionExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::UnlockRegion\n", this));
    return(hresult);
}

HRESULT FileLockBytes::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    HRESULT hresult = E_FAIL;
    DWORD   sizelow, sizehigh;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN FileLockBytes::Stat\n", this));

    if (!pStatStg)
        goto StatExit;

    if (_hFileHandle == INVALID_HANDLE_VALUE)
        goto StatExit;

    memset(pStatStg, 0, sizeof(STATSTG));

    pStatStg->pwcsName = NULL;
    pStatStg->type = STGTY_LOCKBYTES;

    sizelow = GetFileSize(_hFileHandle, &sizehigh);
    if (sizelow == -1 && GetLastError() != NOERROR)
        goto StatExit;

    *(DWORD *)(&pStatStg->cbSize.QuadPart + sizeof(ULONG)) = sizehigh;
    *(DWORD *)(&pStatStg->cbSize.QuadPart) = sizelow;

    if (GetFileTime(_hFileHandle, &pStatStg->ctime, &pStatStg->atime, &pStatStg->mtime))
    {
        pStatStg->grfMode = GENERIC_READ | GENERIC_WRITE;
        pStatStg->grfLocksSupported = LOCK_WRITE | LOCK_EXCLUSIVE | LOCK_ONLYONCE;
        pStatStg->clsid = IID_ILockBytes;
        pStatStg->grfStateBits = 0;

        hresult = NOERROR;
    }

StatExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT FileLockBytes::Stat\n", this));
    return(hresult);
}

HRESULT FileLockBytes::seekfile(ULARGE_INTEGER offset)
{
    HRESULT hresult = NOERROR;
    DWORD   offslow, offshigh;

    offshigh = *(DWORD *)(&offset.QuadPart + sizeof(ULONG));
    offslow = *(DWORD *)(&offset.QuadPart);

    offslow = SetFilePointer(_hFileHandle, offslow, (LONG *)&offshigh, FILE_BEGIN);
    if (offslow == -1 && GetLastError() != NO_ERROR)
        hresult = E_FAIL;

    return(hresult);
}

