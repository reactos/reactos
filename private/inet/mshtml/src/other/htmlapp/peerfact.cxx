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

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_FACTORY_HXX_
#define X_FACTORY_HXX_
#include "factory.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_PEERS_HXX_
#define X_PEERS_HXX_
#include "peers.hxx"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(CBehaviorFactory, CHTMLApp, HTMLApp, _Factory)

CBehaviorFactory::CBehaviorFactory()
{
}

void CBehaviorFactory::Passivate()
{
}

HRESULT CBehaviorFactory::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IElementBehaviorFactory)
        *ppv = (IElementBehaviorFactory *)this;

    if (*ppv)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// IElementBehaviorFactory methods
HRESULT CBehaviorFactory::FindBehavior(BSTR bstrName, BSTR bstrUrl,
            IElementBehaviorSite * pSite, IElementBehavior ** ppPeer)
{
    HRESULT hr = S_OK;
    
    if (bstrName && (0 == StrCmpICW(bstrName, L"Application")))
    {
        CAppBehavior *pApp = new CAppBehavior;
        hr = pApp->QueryInterface(IID_IElementBehavior, (void **)ppPeer);

        // This object should have a refcount of one when this function returns, otherwise
        // it will never get to zero.  CBase contributes an initial refcount of 1, the QI
        // (if successful) increments the refcount to 2.  We need to release one of them now.
        if (SUCCEEDED(hr))
            pApp->Release();
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

	return hr;
}

