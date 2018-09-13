// PeerDraw.cpp : Implementation of CPeerFactory
#include "headers.h"
#include "iextag.h"
#include "peerfact.h"
#include "ccaps.h"
#include "homepg.h"
#include "userdata.hxx"
#include "persist.hxx"
#include "download.h"

#ifndef UNIX // UNIX doesn't support this.
#include "httpwfh.h"
#include "ancrclk.h"
#endif


//+-----------------------------------------------------------
//
// Member:  CPeerFactory constructor
//
//+-----------------------------------------------------------

CPeerFactory::CPeerFactory()
{
}

//+-----------------------------------------------------------
//
// Member:  CPeerFactory destructor
//
//+-----------------------------------------------------------

CPeerFactory::~CPeerFactory()
{
}

#define CREATE_OBJECT(name, classname)                                  \
    if (0 == StrCmpICW(bstrName, name))                                 \
    {                                                                   \
        CComObject<classname> *pInstance;                               \
        hr = CComObject<classname>::CreateInstance(&pInstance);         \
        if (SUCCEEDED(hr))                                              \
        {                                                               \
            hr = pInstance->QueryInterface(                             \
                IID_IElementBehavior, (void **)ppBehavior);             \
        }                                                               \
    }

//+-----------------------------------------------------------
//
//  Member : FindBehavior
//
//  synopsis : IHTMLPeerFactory memeber. this function knows about 
//      the peers that this factory can provide
//
//+-----------------------------------------------------------

STDMETHODIMP
CPeerFactory::FindBehavior(
    BSTR                    bstrName,
    BSTR	                bstrUrl, 
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppBehavior)
{
    HRESULT     hr = E_FAIL;

    if (!ppBehavior)
        goto Cleanup;

         CREATE_OBJECT(L"ClientCaps", CClientCaps)
    else CREATE_OBJECT(L"HomePage", CHomePage)
    else CREATE_OBJECT(L"userData", CPersistUserData)
    else CREATE_OBJECT(L"saveHistory", CPersistHistory)
    else CREATE_OBJECT(L"saveFavorite", CPersistShortcut)
    else CREATE_OBJECT(L"saveSnapshot", CPersistSnapshot)
    else CREATE_OBJECT(L"download", CDownloadBehavior)
#ifndef UNIX // UNIX doesn't support this.
    else CREATE_OBJECT(L"httpFolder", Cwfolders)
    else CREATE_OBJECT(L"AnchorClick", CAnchorClick)
#endif

Cleanup:
    return hr;
}
