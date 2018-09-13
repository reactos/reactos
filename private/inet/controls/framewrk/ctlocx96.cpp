//=--------------------------------------------------------------------------=
// CtlOcx96.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of the OCX 96 interfaces that don't quite fit in to the
// categories covered by embedding, persistence, and ctlmisc.cpp
//
//
#include "IPServer.H"

#include "CtrlObj.H"
#include "Globals.H"


//=--------------------------------------------------------------------------=
// COleControl::GetActivationPolicy    [IPointerInactive]
//=--------------------------------------------------------------------------=
// returns the present activation policy for this object.  for non-subclassed
// windows controls, this means we can put off in-place activation for quite
// a while.
//
// Parameters:
//    DWORD *        - [out] activation policy
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetActivationPolicy
(
    DWORD *pdwPolicy
)
{
    CHECK_POINTER(pdwPolicy);

    // just get the policy in the global structure describing this control.
    //
    *pdwPolicy = ACTIVATIONPOLICYOFCONTROL(m_ObjectType);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::OnInactiveMouseMove    [IPointerInactive]
//=--------------------------------------------------------------------------=
// indicates to an inactive oobject that the mouse pointer has moved over the
// object.
//
// Parameters:
//    LPCRECT            - [in]
//    long               - [in]
//    long               - [in]
//    DWORD              - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::OnInactiveMouseMove
(
    LPCRECT pRectBounds,
    long    x,
    long    y,
    DWORD   dwMouseMsg
)
{
    // OVERRIDE: end control writers should just override this if they want
    // to have a control that is never in-place active.
    //
    return S_OK;
}
    
//=--------------------------------------------------------------------------=
// COleControl::OnInactiveSetCursor    [IPointerInactive]
//=--------------------------------------------------------------------------=
// called by the container for the inactive object under the mouse pointer on
// recept of a WM_SETCURSOR message.
//
// Parameters:
//    LPCRECT            - [in]
//    long               - [in]
//    long               - [in]
//    DWORD              - [in]
//    BOOL               - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::OnInactiveSetCursor
(
    LPCRECT pRectBounds,
    long    x,
    long    y,
    DWORD   dwMouseMsg,
    BOOL    fSetAlways
)
{
    // OVERRIDE:  just get the user to override this if they want to never
    // be activated
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::QuickActivate    [IQuickActivate]
//=--------------------------------------------------------------------------=
// allows the container to activate the control.
//
// Parameters:
//    QACONTAINER *        - [in]  info about the container
//    QACONTROL *          - [out] info about the control
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::QuickActivate
(
    QACONTAINER *pContainer,
    QACONTROL *pControl
)
{
    HRESULT hr;
    DWORD   dw;

    // we need these guys.
    //
    if (!pContainer) return E_UNEXPECTED;
    if (!pControl) return E_UNEXPECTED;

    // start grabbing things from the QACONTAINER structure and apply them
    // as relevant
    //
    if (pContainer->cbSize < sizeof(QACONTAINER)) return E_UNEXPECTED;
    if (pControl->cbSize < sizeof(QACONTROL)) return E_UNEXPECTED;

    // save out the client site, of course.
    //
    if (pContainer->pClientSite) {
        hr = SetClientSite(pContainer->pClientSite);
        RETURN_ON_FAILURE(hr);
    }

    // if the lcid is not LANG_NEUTRAL, score!
    //
    if (pContainer->lcid) {
        g_lcidLocale = pContainer->lcid;
        g_fHaveLocale = TRUE;
    }

    // pay attention to some ambients
    //
    if (pContainer->dwAmbientFlags & QACONTAINER_MESSAGEREFLECT) {
        m_fHostReflects = TRUE;
        m_fCheckedReflecting = TRUE;
    }

    // hook up some notifications.  first property notifications.
    //
    if (pContainer->pPropertyNotifySink) {
        pContainer->pPropertyNotifySink->AddRef();
        hr = m_cpPropNotify.AddSink((void *)pContainer->pPropertyNotifySink, &pControl->dwPropNotifyCookie);
        if (FAILED(hr)) {
            pContainer->pPropertyNotifySink->Release();
            return hr;
        }
    }

    // then the event sink.
    //
    if (pContainer->pUnkEventSink) {
        hr = m_cpEvents.Advise(pContainer->pUnkEventSink, &pControl->dwEventCookie);
        if (FAILED(hr)) {
            pContainer->pUnkEventSink->Release();
            return hr;
        }
    }

    // finally, the advise sink.
    //
    if (pContainer->pAdviseSink) {
        // don't need to pass the cookie back since there can only be one
        // person advising at a time.
        //
        hr = Advise(pContainer->pAdviseSink, &dw);
        RETURN_ON_FAILURE(hr);
    }

    // set up a few things in the QACONTROL structure.  we're opaque by default
    //
    pControl->dwMiscStatus = OLEMISCFLAGSOFCONTROL(m_ObjectType);
    pControl->dwViewStatus = FCONTROLISOPAQUE(m_ObjectType) ? VIEWSTATUS_OPAQUE : 0;
    pControl->dwPointerActivationPolicy = ACTIVATIONPOLICYOFCONTROL(m_ObjectType);

    // that's pretty much all we're interested in.  we will, however, pass on the
    // rest of the things to the end control writer and see if they want to do
    // anything with them. they shouldn't touch any of the above except for the
    // ambients.
    //
    return OnQuickActivate(pContainer, &(pControl->dwViewStatus));
}

//=--------------------------------------------------------------------------=
// COleControl::SetContentExtent    [IQuickActivate]
//=--------------------------------------------------------------------------=
// the container calls this to set the content extent of the control.
//
// Parameters:
//    LPSIZEL            - [in] the size of the content extent
//
// Output:
//    HRESULT            - S_OK, or E_FAIL for fixed size control
//
// Notes:
//
STDMETHODIMP COleControl::SetContentExtent
(
    LPSIZEL pSize
)
{
    return SetExtent(DVASPECT_CONTENT, pSize);
}

//=--------------------------------------------------------------------------=
// COleControl::GetContentExtent    [IQuickActivate]
//=--------------------------------------------------------------------------=
// the container calls this to get the content extent of the control
//
// Parameters:
//    LPSIZEL        - [out] returns current size
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetContentExtent
(
    LPSIZEL pSize
)
{
    return GetExtent(DVASPECT_CONTENT, pSize);
}

//=--------------------------------------------------------------------------=
// COleControl::OnQuickActivate    [overridable]
//=--------------------------------------------------------------------------=
// not all the of the members of the QACONTAINER need to be consumed by the
// framework, but are, at least, extremely interesting.  thus, we will pass
// on the struture to the end control writer, and let them consume these.
//
// Parameters:
//    QACONTAINER *            - [in]  contains additional information
//    DWORD *                  - [out] put ViewStatus flags here.
//
// Output:
//    HRESULT
//
// Notes:
//    - control writers should only look at/consume:
//        a. dwAmbientFlags
//        b. colorFore/colorBack
//        c. pFont
//        d. pUndoMgr
//        e. dwAppearance
//        f. hpal
//
//    - all the others are set up the for the user by the framework.
//    - control writers should set up the pdwViewStatus with flags as per
//      IViewObjectEx::GetViewStatus.  if you don't know what this is or don't
//      care, then don't touch.
//
HRESULT COleControl::OnQuickActivate
(
    QACONTAINER *pContainer,
    DWORD       *pdwViewStatus
)
{
    // by default, nuthin much to do!
    //
    return S_OK;
}
