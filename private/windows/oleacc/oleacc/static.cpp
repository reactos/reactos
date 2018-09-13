// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  STATIC.CPP
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "static.h"




// --------------------------------------------------------------------------
//
//  CreateStaticClient()
//
// --------------------------------------------------------------------------
HRESULT CreateStaticClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvStatic)
{
    CStatic * pstatic;
    HRESULT hr;

    InitPv(ppvStatic);

    pstatic = new CStatic(hwnd, idChildCur);
    if (!pstatic)
        return(E_OUTOFMEMORY);

    hr = pstatic->QueryInterface(riid, ppvStatic);
    if (!SUCCEEDED(hr))
        delete pstatic;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CStatic::CStatic()
//
// --------------------------------------------------------------------------
CStatic::CStatic(HWND hwnd, long idChildCur)
{
    long    lStyle;

    Initialize(hwnd, idChildCur);

    //
    // Is this a graphic?
    //
    //
    // Get window style
    //
    lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    switch (lStyle & SS_TYPEMASK)
    {
        case SS_LEFT:
        case SS_CENTER:
        case SS_RIGHT:
        case SS_SIMPLE:
        case SS_LEFTNOWORDWRAP:
        case SS_EDITCONTROL:
            m_fGraphic = FALSE;
            break;

        default:
            m_fGraphic = TRUE;
            break;
    }

    // If this is a graphic static, override the window text and always
    // look for a label first.
    //m_fUseLabel = m_fGraphic;
}



// --------------------------------------------------------------------------
//
//  CStatic::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatic::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    if (m_fGraphic)
        pvarRole->lVal = ROLE_SYSTEM_GRAPHIC;
    else
        pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CStatic::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatic::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (m_fGraphic)
        return(S_FALSE);
    else
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
}



// --------------------------------------------------------------------------
//
//  CStatic::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CStatic::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    WINDOWINFO wi;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

	pvarState->lVal |= STATE_SYSTEM_READONLY;
//	pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

    if (!MyGetWindowInfo(m_hwnd, &wi))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }
    
	if (!(wi.dwStyle & WS_VISIBLE))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return(S_OK);
}
