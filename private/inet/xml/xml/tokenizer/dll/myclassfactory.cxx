/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <unknwn.h>

#include "myclassfactory.h"

extern long g_cServerLocks;

HRESULT __stdcall 
MyClassFactory::QueryInterface(
    REFIID riid, 
    void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IClassFactory) 
    {
        *ppv = this;
        AddRef();
        return S_OK;    
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


HRESULT __stdcall 
MyClassFactory::LockServer(BOOL fLock)
{
    // As per INSIDE COM - we do not count class factories.
    // Instead we count calls to LockServer.
    if (fLock)
    {
        ::InterlockedIncrement(&g_cServerLocks);
    }
    else
    {
        ::InterlockedDecrement(&g_cServerLocks);
    }
    return S_OK;
}


HRESULT __stdcall 
MyClassFactory::CreateInstance(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    return (_pfnCreateInstance)(iid, ppvObj);
}


