//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       olesite.cxx
//
//  Contents:   implementation of client object
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_CLIENT_HXX_
#define X_CLIENT_HXX_
#include "client.hxx"
#endif

#ifndef X_PRIVCID_H_
#define X_PRIVCID_H_
#include "privcid.h"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(CClient, CHTMLApp, HTMLApp, _Client)

//+---------------------------------------------------------------------------
//
//  Member:     CClient::CClient
//
//  Synopsis:   Initializes data members
//
//----------------------------------------------------------------------------
CClient::CClient()
    : _pUnk(NULL), _pIoo(NULL), _pView(NULL)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::~CClient
//
//  Synopsis:   
//
//----------------------------------------------------------------------------
CClient::~CClient()
{
    Assert(!_pUnk);
    Assert(!_pIoo);
    Assert(!_pView);
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::QueryObjectInterface
//
//  Synopsis:   Query the control for an interface.
//              The purpose of this function is to reduce code size.
//
//  Arguments:  iid     Interface to query for
//              ppv     Returned interface
//
//-------------------------------------------------------------------------

HRESULT
CClient::QueryObjectInterface(REFIID iid, void **ppv)
{
    if (_pUnk)
        return _pUnk->QueryInterface(iid, ppv);
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Passivate
//
//  Synopsis:   Called when app is shutting down.
//
//----------------------------------------------------------------------------
void CClient::Passivate()
{
    IOleInPlaceObject *pIP = NULL;

    // InplaceDeactivate the contained object
    QueryObjectInterface(IID_IOleInPlaceObject, (void **)&pIP);
    if (pIP)
    {
        pIP->InPlaceDeactivate();
        ReleaseInterface(pIP);
    }

    // BUGBUG:  is this the right flag to send?
    if (_pIoo)
        _pIoo->Close(OLECLOSE_NOSAVE);
        
    ClearInterface(&_pIoo);
    ClearInterface(&_pView);
    ClearInterface(&_pUnk);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Show
//
//  Synopsis:   Causes the docobject to show itself in our client area
//
//----------------------------------------------------------------------------

HRESULT CClient::Show()
{
    HRESULT hr = S_OK;
    
    if (_pIoo)
    {
        RECT rc;
        App()->GetViewRect(&rc);
        hr = _pIoo->DoVerb(OLEIVERB_SHOW, NULL, this, 0, App()->_hwnd, &rc);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Resize
//
//  Synopsis:   Resizes this object (usually as a result of the frame windo
//              changing size).
//
//----------------------------------------------------------------------------

void CClient::Resize()
{
    if (_pView)
    {
        RECT rc;
        App()->GetViewRect(&rc);
        _pView->SetRect(&rc);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::Create
//
//  Synopsis:   Creates COM object instance
//
//----------------------------------------------------------------------------

HRESULT CClient::Create(REFCLSID clsid)
{
    HRESULT hr = S_OK;
    IClassFactory *pCF = NULL;

    hr = THR(LocalGetClassObject(clsid, IID_IClassFactory, (void **)&pCF));
    if (hr)
        goto Cleanup;
        
    hr = THR(pCF->CreateInstance(NULL, IID_IUnknown, (void **)&_pUnk));
    if (hr)
        goto Cleanup;
        
    // Store an IOleObject pointer for future use...
    QueryObjectInterface(IID_IOleObject, (void **)&_pIoo);
    
Cleanup:
    ReleaseInterface(pCF);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Load
//
//  Synopsis:   In addition to loading, set's the client site.
//
//----------------------------------------------------------------------------

HRESULT CClient::Load(IMoniker *pMk)
{
    HRESULT hr = S_OK;
	IPersistMoniker *pIpm = NULL;

    if (!_pUnk)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // BUGBUG:  move to its own fn (and check OLEMISC for order)??
    if (_pIoo)
        _pIoo->SetClientSite(this);

	QueryObjectInterface(IID_IPersistMoniker, (void **)&pIpm);
	if (pIpm)
	{
		hr = THR(pIpm->Load(TRUE, pMk, NULL, 0));
		ReleaseInterface(pIpm);
	}

    TEST(hr);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::SendCommand
//
//  Synopsis:   Sends an Exec command to the hosted object.
//
//----------------------------------------------------------------------------

HRESULT
CClient::SendCommand(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT                 hr;
    IOleCommandTarget *     pCmdTarget = NULL;
    
    hr = THR_NOTRACE(QueryObjectInterface(
        IID_IOleCommandTarget,
        (void **)&pCmdTarget));

    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pCmdTarget->Exec(pguidCmdGroup,
            nCmdID,
            nCmdexecopt,
            pvarargIn,
            pvarargOut));

Cleanup:
    ReleaseInterface(pCmdTarget);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::DelegateMessage
//
//  Synopsis:   Delegates messages to the contained instance of Trident.
//
//----------------------------------------------------------------------------

LRESULT
CClient::DelegateMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    IOleInPlaceObject * pAO = NULL;
    LRESULT lRes = 0;
    
    if (_pView)
    {
        HRESULT hr;
        HWND hwnd;
        
        hr = _pView->QueryInterface(IID_IOleInPlaceObject, (void **) &pAO);
        if (hr)
            goto Cleanup;

        hr = pAO->GetWindow(&hwnd);
        if (hr)
            goto Cleanup;
            
        Assert(::IsWindow(hwnd));
        lRes = ::SendMessage(hwnd, msg, wParam, lParam);
    }
    
Cleanup:
    ReleaseInterface(pAO);
    return lRes;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::Frame
//
//  Synopsis:   Returns a pointer to the app's frame object
//
//----------------------------------------------------------------------------

CHTMLAppFrame * CClient::Frame()
{
    return &App()->_Frame;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::App
//
//  Synopsis:   Returns a pointer to the application object.
//
//----------------------------------------------------------------------------

CHTMLApp * CClient::App()
{
    return HTMLApp();
}    

