//+---------------------------------------------------------------------
//
//  File:       verb.cxx
//
//  Contents:   CServer verb stuff.
//
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

//+---------------------------------------------------------------
//
//  Common verb tables
//
//          These are the common verb tables.  Specialized verb tables
//          can be defined elsewhere.
//
//          DoProperties is duplicated in these tables because most
//          containers will not add menu items for negative lVerb values.
//
//          The sizes for these arrays are defined in CDBASE.HXX.  You
//          must update the sizes in CDBASE if you make changes here.
//
//---------------------------------------------------------------

OLEVERB g_aOleVerbStandard[C_OLEVERB_STANDARD] =
{
    { OLEIVERB_PRIMARY,         NULL, 0, 0 },
    { 1,                        NULL, 0, 0 },
    { OLEIVERB_INPLACEACTIVATE, NULL, 0, 0 },
    { OLEIVERB_UIACTIVATE,      NULL, 0, 0 },
    { OLEIVERB_HIDE,            NULL, 0, 0 },
    { OLEIVERB_SHOW,            NULL, 0, 0 },
    { OLEIVERB_PROPERTIES,      NULL, 0, 0 },
};

CServer::PFNDOVERB g_apfnDoVerbStandard[C_OLEVERB_STANDARD]=
{
    &CServer::DoUIActivate,
#if !defined(PRODUCT_RT) && !defined(PRODUCT_96)
    &CServer::DoProperties,
#endif
    &CServer::DoInPlaceActivate,
    &CServer::DoUIActivate,
    &CServer::DoHide,
    &CServer::DoUIActivate,
#if !defined(PRODUCT_RT) && !defined(PRODUCT_96)
    &CServer::DoProperties,
#endif
};


//+---------------------------------------------------------------------------
//
//  Member:     CServer::PrepareActivationMessage
//
//  Synopsis:   Setup an activation message to be later sent by
//              SendActivationMessage.
//
//  Arguments:  lpmsg       The message handed to us in DoVerb.
//              lpmsgSend   The message to send.
//
//  Notes:      > Handles the lpmsg == NULL case.
//              > Translates message parameters, so that the window
//                can deal with the message as if it arrived
//                normally.
//              > We only send mouse messages which cause activation.
//
//----------------------------------------------------------------------------

void
CServer::PrepareActivationMessage(LPMSG lpmsg, LPMSG lpmsgSend)
{
    POINT   pt;
    HRESULT hr;

    Assert(State() >= OS_INPLACE && _pInPlace);

    // Assume no message to send by default.

    lpmsgSend->message = 0;

    if (!lpmsg)
        return;

    // We send only mouse messages. Derived classes can handle
    // keyboard messages in ActivateUI().

    if (lpmsg->message != WM_LBUTTONDOWN &&
             lpmsg->message != WM_RBUTTONDOWN &&
             lpmsg->message != WM_MBUTTONDOWN)
        return;

#ifdef WIN16
    // since we wrap all RECT, POINT to be RECTL, POINTL
    // this is necessary.
    pt.x = lpmsg->pt.x;
    pt.y = lpmsg->pt.y;
#else
    pt = lpmsg->pt;
#endif

    if (_pInPlace->_hwnd)
    {
        // We need to re-hit-test (since the control may create child windows)
        // and re-translate mouse message coordinates.
        POINT   ptClient = pt;

        ScreenToClient(_pInPlace->_hwnd, &ptClient);
        lpmsgSend->hwnd = ChildWindowFromPointEx(
                _pInPlace->_hwnd,
                ptClient,
                CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
    }
    else
    {
        hr = THR(_pInPlace->_pInPlaceSite->GetWindow(&lpmsgSend->hwnd));
        if (hr)
            return;
    }

    ScreenToClient(lpmsgSend->hwnd, &pt);
    lpmsgSend->message = lpmsg->message;
    lpmsgSend->wParam = lpmsg->wParam;
    lpmsgSend->lParam = MAKELONG(pt.x, pt.y);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::SendActivationMessage
//
//  Synopsis:   Sends message prepared by PrepareActivationMessage
//
//  Arguments:  lpmsg   Message to send.
//
//----------------------------------------------------------------------------

void
CServer::SendActivationMessage(LPMSG lpmsg)
{
    LRESULT lResult;

    if (lpmsg->message != 0)
    {
        if (_pInPlace->_hwnd)
        {
            SendMessage(lpmsg->hwnd,
                    lpmsg->message,
                    lpmsg->wParam,
                    lpmsg->lParam);
        }
        else
        {
            IGNORE_HR(OnWindowMessage(
                    lpmsg->message,
                    lpmsg->wParam,
                    lpmsg->lParam,
                    &lResult));
        }
    }
}

//+---------------------------------------------------------------
//
//  Member:     CServer::ActivateView
//
//  Synopsis:   Activate an IOleDocumentView
//
//---------------------------------------------------------------

HRESULT
CServer::ActivateView()
{
    HRESULT hr;
    IOleDocumentSite * pMsoDocSite = NULL;
    IOleDocumentView * pOleDocumentView = NULL;

    Assert(_fMsoDocMode);

    hr = THR(_pClientSite->QueryInterface(IID_IOleDocumentSite,
                 (void **)&pMsoDocSite));
    if (hr)
        goto Cleanup;

    hr = THR(PrivateQueryInterface(IID_IOleDocumentView, (void **)&pOleDocumentView));
    if (hr)
        goto Cleanup;
    
    hr = THR(pMsoDocSite->ActivateMe(pOleDocumentView));

Cleanup:
    ReleaseInterface(pOleDocumentView);
    ReleaseInterface(pMsoDocSite);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CServer::DoShow, public
//
//  Synopsis:   Implement an OLE verb by asking the client site to show
//              this object.
//
//  Arguments:  [pServer] -- pointer to a CServer object.
//
//              All other parameters are the same as the IOleObject::DoVerb
//              method.
//
//  Returns:    Success if the verb was successfully executed
//
//  Notes:      This and the other static Do functions are provided for
//              use in the server's verb table.  This verb results in
//              a ShowObject call on our container and a transition
//              to the U.I. active state
//
//---------------------------------------------------------------

HRESULT
CServer::DoShow(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    HRESULT hr = S_OK;

    if (!pServer->_pClientSite)
        RRETURN(E_UNEXPECTED);

    if (pServer->_fMsoDocMode)
    {
        hr = THR(pServer->ActivateView());
    }
    else
    {
        IGNORE_HR(pServer->_pClientSite->ShowObject());
        pServer->_fHidden = FALSE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member: CServer::DoHide, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_HIDE
//              This verb results in a transition to the Running state.
//
//---------------------------------------------------------------

HRESULT
CServer::DoHide(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    HRESULT hr;

    if (pServer->_fMsoDocMode)
    {
        //
        //  If we're a docobj, then hide should not be called on us.
        //

        hr = E_UNEXPECTED;
    }
    else
    {
        hr = THR(pServer->TransitionTo(OS_RUNNING, lpmsg));
        if (OK(hr))
        {
            pServer->_fHidden = TRUE;
        }
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DoUIActivate, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_UIACTIVATE
//              This verb results in a transition to the U.I. active state.
//
//---------------------------------------------------------------

HRESULT
CServer::DoUIActivate(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    HRESULT     hr;
    MSG         msg;

    if (pServer->_fMsoDocMode)
    {
        hr = THR(pServer->ActivateView());
    }
    else
    {
        // Get the server into the OS_INPLACE state so that we can setup
        // the activation message.

        if (pServer->State() < OS_INPLACE)
        {
            hr = THR(pServer->TransitionTo(OS_INPLACE, lpmsg));
            if (hr)
                goto Cleanup;
        }

        // Setup the activation message.   We do this before UI activating because
        // this server's screen position (used in setting up the message) can change
        // as a result of UI activation. This can happen because another server removes
        // its UI when deactivting or this server installs UI.

        pServer->PrepareActivationMessage(lpmsg, &msg);

        hr = THR(pServer->TransitionTo(OS_UIACTIVE, lpmsg));
        if (hr)
            goto Cleanup;

        pServer->SendActivationMessage(&msg);
    }

Cleanup:
    RRETURN1(hr, OLEOBJ_S_CANNOT_DOVERB_NOW);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DoInPlaceActivate, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_INPLACEACTIVATE
//              This verb results in a transition to the inplace state.
//
//---------------------------------------------------------------

HRESULT
CServer::DoInPlaceActivate(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    HRESULT     hr;
    MSG         msg;

    if (pServer->_fMsoDocMode)
    {
        hr = THR(pServer->ActivateView());
    }
    else
    {
        hr = THR(pServer->TransitionTo(OS_INPLACE, lpmsg));
        if (!hr)
        {
            pServer->PrepareActivationMessage(lpmsg, &msg);
            pServer->SendActivationMessage(&msg);
        }

        pServer->_fHidden = FALSE;
        // Fixes VB4 problem where they call InPlaceActivate instead
        // of DoShow when setting visible=true.
    }

    RRETURN1(hr, OLEOBJ_S_CANNOT_DOVERB_NOW);
}


#if !defined(PRODUCT_RT) && !defined(PRODUCT_96)
//+---------------------------------------------------------------------------
//
//  Member:     CServer::DoProperties
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_PROPERTIES.
//              This verb results in bringing up the property frame on
//              this object.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::DoProperties(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    HRESULT           hr;
    BSTR              bstrName    = NULL;
    LPTSTR            pszCaption  = NULL;
    IOleControlSite * pSite;
    HWND              hwnd        = NULL;

    Assert(pActiveSite);

    if (!pActiveSite->QueryInterface(IID_IOleControlSite, (LPVOID *) &pSite))
    {
        hr = THR(pSite->ShowPropertyFrame());
        ReleaseInterface(pSite);
        if (!hr)
            return S_OK;
    }

    //  Ignore failure to get display name; in this case, no
    //    display name is given in the property frame title

    hr = pServer->GetAmbientBstr(
            DISPID_AMBIENT_DISPLAYNAME,
            &bstrName);
    if (hr)
        goto Cleanup;

    hr = Format(FMT_OUT_ALLOC,
            &pszCaption, 128,
            MAKEINTRESOURCE(bstrName ? IDS_NAMEDCTRLPROPERTIES : IDS_CTRLPROPERTIES),
            bstrName);
    FormsFreeString(bstrName);

    if (hr)
        goto Cleanup;

    if (pServer->_pInPlace)
    {
        hwnd = pServer->_pInPlace->_hwnd;
    }
#ifndef PRODUCT_96

    int             i;
    const CLSID **  apclsid;
    CLSID           aclsid[32];

#ifdef NO_PROPERTY_PAGE
    apclsid = NULL;
#else
    apclsid = pServer->BaseDesc()->_apclsidPages;
#endif // NO_PROPERTY_PAGE

    if (!apclsid)
    {
        i = 0;
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE(aclsid) && apclsid[i]; i++)
        {
            aclsid[i] = *apclsid[i];
        }
    }

#ifdef WIN16
    MessageBox(NULL, "CServer::DoProperties Need to Implement OleCreatePropertyFrame.", "BUGWIN16", MB_OK);
#else
    hr = THR(OleCreatePropertyFrame(
                hwnd,
                32,
                32,
                pszCaption,
                1,
                (IUnknown **)&pServer,
                i,
                aclsid,
                g_lcidUserDefault,
                0,
                NULL));
#endif
#else

    hr = THR(FormsCreatePropertyFrame(
                hwnd,
                32,
                32,
                pszCaption,
                1,
                (IUnknown **)&pServer,
                0,
                NULL,
                g_lcidUserDefault));

#endif
#if DBG == 1
    if (hr)
        TraceTag((tagError, "CServer::DoProperties -- "
                                  "CreatePropertyFrame failed: %lx.", hr));
#endif

Cleanup:
    FormsFreeString(bstrName);
    delete [] pszCaption;

    RRETURN(hr);
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CServer::DoUIActivateIfDesign, public
//
//  Synopsis:   Implement OLE Verb by showing the object in user mode or
//              transitioning to UI active state in design mode.  If
//              explicitly asked to UI activate the object in user mode,
//              we return E_NOTIMPL.
//
//----------------------------------------------------------------------------

HRESULT
CServer::DoUIActivateIfDesign(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    PFNDOVERB pfnDoVerb;
    BOOL      fUserMode;
    HRESULT   hr;

    fUserMode = pServer->GetAmbientBool(DISPID_AMBIENT_USERMODE, TRUE);

    if (iVerb == OLEIVERB_UIACTIVATE && fUserMode)
    {
        hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
    }
    else
    {
        pfnDoVerb = fUserMode ? DoShow : DoUIActivate;
        hr = THR(pfnDoVerb(pServer,
                iVerb,
                lpmsg,
                pActiveSite,
                lindex,
                hwndParent,
                lprcPosRect));
    }

    RRETURN1(hr, OLEOBJ_S_CANNOT_DOVERB_NOW);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::DoInPlaceActivateIfDesign, public
//
//  Synopsis:   If in design mode, transition to the inplace state.
//
//----------------------------------------------------------------------------

HRESULT
CServer::DoInPlaceActivateIfDesign(
        CServer * pServer,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    BOOL      fUserMode;
    HRESULT   hr;

    fUserMode = pServer->GetAmbientBool(DISPID_AMBIENT_USERMODE, TRUE);

    Assert(iVerb == OLEIVERB_INPLACEACTIVATE);

    if (fUserMode)
    {
        hr = E_NOTIMPL;
    }
    else
    {
        hr = THR(DoInPlaceActivate(pServer,
                iVerb,
                lpmsg,
                pActiveSite,
                lindex,
                hwndParent,
                lprcPosRect));
    }

    RRETURN(hr);
}
