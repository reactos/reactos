//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       STGAPI.CXX
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-15-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include <urlmon.hxx>
#include "clockbyt.hxx"
#include "casynclb.hxx"
#include "filelb.hxx"
#include "memlb.hxx"
#include "stgapi.hxx"


HRESULT StgGetFillLockByteOnMem(IFillLockBytes **pFLB)
{
    HRESULT     hresult = NOERROR;
    ILockBytes  *pLB;

    *pFLB = NULL;

    if (!(pLB = new MemLockBytes))
        return(E_OUTOFMEMORY);

    hresult = StgGetFillLockByteILockBytes(pLB, pFLB);
    if (hresult != NOERROR)
        delete pLB;

    return(hresult);
}

HRESULT StgGetFillLockByteOnFile(OLECHAR *pwcFileName, IFillLockBytes **pFLB)
{
    HRESULT     hresult = NOERROR;
    ILockBytes  *pLB;
    HANDLE      fhandle;

    *pFLB = NULL;

    fhandle = CreateFileW(pwcFileName, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

    if (fhandle == INVALID_HANDLE_VALUE)
        return(E_FAIL);

    if (!(pLB = new FileLockBytes(fhandle)))
    {
        CloseHandle(fhandle);
        return(E_OUTOFMEMORY);
    }

    hresult = StgGetFillLockByteILockBytes(pLB, pFLB);
    if (hresult != NOERROR)
        delete pLB;

    return(hresult);
}

HRESULT StgGetFillLockByteILockBytes(ILockBytes *pLB, IFillLockBytes **pFLB)
{
    IFillLockBytes  *flb;

    *pFLB = NULL;

    if (!(flb = new CAsyncLockBytes(pLB)))
        return(E_OUTOFMEMORY);

    *pFLB = flb;

    return(NOERROR);
}


