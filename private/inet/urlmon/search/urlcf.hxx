//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlcf.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _URLCF_HXX_DEFINED_
#define _URLCF_HXX_DEFINED_

#include <urlint.h>
#include <stdio.h>
#include "urlmon.h"

class CUrlClsFact : public IClassFactory
{
public:
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    STDMETHOD(CreateInstance)   (LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv);
    STDMETHOD(LockServer)       (BOOL fLock);

    static HRESULT Create(REFCLSID clsid, CUrlClsFact **ppCF);

    CUrlClsFact(REFCLSID clsid);
    ~CUrlClsFact();

private:
    CRefCount   _CRefs;         // object refcount
    CRefCount   _CLocks;        // dll lock refcount
    CLSID       _ClsID;        // the class this CF can generate objects
};

HRESULT CreateAPP(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);

#endif //_URLCF_HXX_DEFINED_

