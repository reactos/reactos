// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  ANIMATED.CPP
//
//  Wrapper for COMCTL32's animation control
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "animated.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOPROGRESS
#include <commctrl.h>



// --------------------------------------------------------------------------
//
//  CreateAnimatedClient()
//
// --------------------------------------------------------------------------
HRESULT CreateAnimatedClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvAnimation)
{
    CAnimation* panimated;
    HRESULT     hr;

    InitPv(ppvAnimation);

    panimated = new CAnimation(hwnd, idChildCur);
    if (!panimated)
        return(E_OUTOFMEMORY);

    hr = panimated->QueryInterface(riid, ppvAnimation);
    if (!SUCCEEDED(hr))
        delete panimated;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAnimation::CAnimation()
//
// --------------------------------------------------------------------------
CAnimation::CAnimation(HWND hwnd, long idCurChild)
{
    Initialize(hwnd, idCurChild);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CAnimation::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAnimation::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_ANIMATION;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CAnimation::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAnimation::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    HRESULT hr;

    // Get the client's state and add on STATE_SYSTEM_ANIMATED
    // Remove STATE_SYSTEM_FOCUSABLE
    hr = CClient::get_accState(varChild, pvarState);
    if (!SUCCEEDED(hr))
        return(hr);

    Assert(pvarState->vt == VT_I4);
    pvarState->lVal &= ~STATE_SYSTEM_FOCUSABLE;
    // BOGUS! no way to tell if it is actually animated or not,
    // so we just say it is always. 
    pvarState->lVal |= STATE_SYSTEM_ANIMATED;

    return(hr);
}


