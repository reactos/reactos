/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "classfactory.h"

extern long g_cServerLocks; // defined in mxl\dll\msxml.cxx


HRESULT __stdcall 
CClassFactory::QueryInterface(
    REFIID riid, 
    void** ppv)
{
    STACK_ENTRY;

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
CClassFactory::LockServer(BOOL fLock)
{
    STACK_ENTRY;

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
CClassFactory::CreateInstance(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj)
{
    STACK_ENTRY;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    return (_pfnCreateInstance)(iid, ppvObj);
}


