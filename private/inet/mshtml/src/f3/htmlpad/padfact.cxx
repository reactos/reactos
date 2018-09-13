//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padfact.cxx
//
//  Contents:   Class factories.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

CPadFactory * CPadFactory::s_pFactoryFirst = NULL;

CPadFactory::CPadFactory(REFCLSID clsid, HRESULT (*pfnCreate)(IUnknown **), void (*pfnRevoke)())
{
    _pfnCreate = pfnCreate;
    _pfnRevoke = pfnRevoke;
    _clsid = clsid;
    _dwRegister = 0;
    _pFactoryNext = s_pFactoryFirst;
    s_pFactoryFirst = this;
}

HRESULT 
CPadFactory::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IClassFactory || iid == IID_IUnknown)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}
    

ULONG 
CPadFactory::AddRef()
{
    return 0;
}

ULONG 
CPadFactory::Release()
{
    return 0;
}

HRESULT
CPadFactory::CreateInstance(
        IUnknown *pUnkOuter,
        REFIID   iid,
        void **  ppv)
{
    IUnknown *pUnk = NULL;
    HRESULT hr;
    
    if (pUnkOuter)
        RRETURN(CLASS_E_NOAGGREGATION);

    hr = THR(_pfnCreate(&pUnk));
    if (hr)
        goto Cleanup;

    hr = THR(pUnk->QueryInterface(iid, ppv));

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN(hr);
}

HRESULT
CPadFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        IncrementObjectCount();
    }
    else
    {
        DecrementObjectCount();
    }

    return S_OK;
}

HRESULT
CPadFactory::Register()
{
    HRESULT hr;
    CPadFactory *pFactory;

    for (pFactory = s_pFactoryFirst; pFactory; pFactory = pFactory->_pFactoryNext)
    {
        hr = THR(CoRegisterClassObject(
            pFactory->_clsid,
            pFactory,
            CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE,
            &pFactory->_dwRegister));
        if (hr)
            RRETURN(hr);
    }

    return S_OK;
}

HRESULT
CPadFactory::Revoke()
{
    CPadFactory *pFactory;

    for (pFactory = s_pFactoryFirst; pFactory; pFactory = pFactory->_pFactoryNext)
    {
        if (pFactory->_dwRegister)
        {
            IGNORE_HR(CoRevokeClassObject(pFactory->_dwRegister));
            pFactory->_dwRegister = 0;

            if(pFactory->_pfnRevoke)
                (*pFactory->_pfnRevoke)();
        }
    }

    return S_OK;
}
