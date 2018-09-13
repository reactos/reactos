//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       factory.cxx
//
//  Contents:   class factory
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FACTORY_HXX_
#define X_FACTORY_HXX_
#include "factory.hxx"
#endif

#ifndef X_SERVER_HXX_
#define X_SERVER_HXX_
#include "server.hxx"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTAClassFactory, CHTMLApp, HTMLApp, _Factory)

EXTERN_C const GUID CLSID_HTMLApplication;

CHTAClassFactory::CHTAClassFactory()
    : _dwRegCookie(0)
{
}

void CHTAClassFactory::Passivate()
{
    // If we registered a class factory, revoke it now.
    if (_dwRegCookie)
    {
        CoRevokeClassObject(_dwRegCookie);
    }
}

HRESULT CHTAClassFactory::Register()
{
    HRESULT hr = THR(CoRegisterClassObject(CLSID_HTMLApplication, this, CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &_dwRegCookie));
    RRETURN(hr);
}

HRESULT CHTAClassFactory::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
        *ppv = (IClassFactory *)this;

    if (*ppv)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT CHTAClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void ** ppv)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CServerObject *pServer = new CServerObject(HTMLApp());
    if (!pServer)
        return E_OUTOFMEMORY;

    return (pServer->QueryInterface(riid, ppv));
}

HRESULT CHTAClassFactory::LockServer(BOOL fLock)
{
    // Keep a combined ref/lock count.  No need to distinguish in the case.
    if (fLock)
    {
        AddRef();
    }
    else
    {
        Release();
    }

    return S_OK;
}


