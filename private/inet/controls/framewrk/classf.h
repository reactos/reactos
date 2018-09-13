//=--------------------------------------------------------------------------=
// ClassF.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// header for the ClassFactory Object.  we support IClassFactory and 
// IClassFactory2
//
#ifndef _CLASSF_H_

#include "olectl.h"

class CClassFactory : public IClassFactory2 {

  public:
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IClassFactory methods
    //
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppbObjOut);
    STDMETHOD(LockServer)(BOOL fLock);

    // IClassFactory2 methods
    //
    STDMETHOD(GetLicInfo)(LICINFO *pLicInfo);
    STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR *pbstrKey);
    STDMETHOD(CreateInstanceLic)(IUnknown *pUnkOuter, IUnknown *pUnkReserved, REFIID riid, BSTR bstrKey, void **ppvObjOut);

    CClassFactory(int iIndex);
    ~CClassFactory();

  private:
    ULONG m_cRefs;
    int   m_iIndex;
};


// global variable for Locks on our DLL
//
extern LONG g_cLocks;

#define _CLASSF_H_
#endif // _CLASSF_H_
