//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       MEMLB.CXX
//
//  Contents:
//
//  Classes:    Implements the memory-based ILockBytes class.
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "clockbyt.hxx"
#include "memlb.hxx"

MemLockBytes::MemLockBytes() : _CRefs()
{
    memhandle = NULL;
}

STDMETHODIMP_(ULONG) MemLockBytes::Release(void)
{
    VDATETHIS(this);
    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN MemLockBytes::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        if (memhandle)
            GlobalFree(memhandle);

        delete this;
    }

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT MemLockBytes::Release\n", this));
    return lRet;
}

HRESULT MemLockBytes::ReadAt(THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,
    ULONG cb, ULONG FAR *pcbRead)
{
    HRESULT hresult = E_FAIL;
    LPVOID  memptr;
    DWORD   memsize;
    ULONG   count;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN MemLockBytes::ReadAt\n", this));

    *pcbRead = 0;

    if (!cb)
        goto ReadAtExit;

    if (!memhandle)
        goto ReadAtExit;

    memsize = GlobalSize(memhandle);

    count = cb;
    if (ulOffset.QuadPart + cb > memsize)
    {
        if (ulOffset.QuadPart > memsize)
        {
            goto ReadAtExit;
        }
        else
        {
            count = memsize - (DWORD)ulOffset.QuadPart;
        }
    }

    memptr = GlobalLock(memhandle);
    if (!memptr)
        goto ReadAtExit;

    *(DWORD *)&memptr += (DWORD)ulOffset.QuadPart;
    memcpy(pv, memptr, count);
    *pcbRead = count;
    GlobalUnlock(memhandle);

    hresult = NOERROR;

ReadAtExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT MemLockBytes::ReadAt\n", this));
    return(hresult);
}

HRESULT MemLockBytes::WriteAt(THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
    ULONG cb, ULONG FAR *pcbWritten)
{
    HRESULT hresult = E_FAIL;
    LPVOID  memptr;
    DWORD   memsize;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN MemLockBytes::WriteAt\n", this));

    *pcbWritten = 0;

    if (!cb)
        goto WriteAtExit;

    if (!memhandle)
    {
        memhandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE,
                        cb + (DWORD)ulOffset.QuadPart);
    }

    if (!memhandle)
        goto WriteAtExit;

    while (1)
    {
        memsize = GlobalSize(memhandle);

        if (ulOffset.QuadPart + cb > memsize)
        {
            memhandle = GlobalReAlloc(memhandle, cb + (DWORD)ulOffset.QuadPart,
                            GMEM_MOVEABLE);
        }
        else
            break;
    }

    memptr = GlobalLock(memhandle);
    if (!memptr)
        goto WriteAtExit;

    *(DWORD *)&memptr += (DWORD)ulOffset.QuadPart;
    memcpy(memptr, pv, cb);
    GlobalUnlock(memhandle);
    *pcbWritten = cb;

    hresult = NOERROR;

WriteAtExit:

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT MemLockBytes::WriteAt\n", this));
    return(hresult);
}

HRESULT MemLockBytes::SetSize(THIS_ ULARGE_INTEGER cb)
{
    HRESULT hresult = NOERROR;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p IN MemLockBytes::SetSize\n", this));

    if (!memhandle)
    {
        memhandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE,
                        (DWORD)cb.QuadPart);
    }
    else
    {
        memhandle = GlobalReAlloc(memhandle, (DWORD)cb.QuadPart, GMEM_MOVEABLE);
    }

    if (!memhandle)
        hresult = E_FAIL;

    UrlMkDebugOut((DEB_ILOCKBYTES, "%p OUT MemLockBytes::SetSize\n", this));
    return(hresult);
}

