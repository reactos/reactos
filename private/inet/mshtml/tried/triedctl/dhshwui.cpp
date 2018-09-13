/*
 * IDOCHOSTSHOWUI.CPP
 * IDocHostShowUI for Document Objects CSite class
 *
 * Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
 */


#include "stdafx.h"
#include <docobj.h>
#include "site.h"

/**
	Note: the m_cRef count is provided for debugging purposes only.
	CSite controls the destruction of the object through delete,
	not reference counting
*/


/*
 * CImpIDocHostShowUI::CImpIDocHostShowUI
 * CImpIDocHostShowUI::~CImpIDocHostShowUI
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */
CImpIDocHostShowUI::CImpIDocHostShowUI( PCSite pSite, LPUNKNOWN pUnkOuter)
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIDocHostShowUI::~CImpIDocHostShowUI( void )
{
}



/*
 * CImpIDocHostShowUI::QueryInterface
 * CImpIDocHostShowUI::AddRef
 * CImpIDocHostShowUI::Release
 *
 * Purpose:
 *  IUnknown members for CImpIDocHostShowUI object.
 */
STDMETHODIMP CImpIDocHostShowUI::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImpIDocHostShowUI::AddRef( void )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIDocHostShowUI::Release( void )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}


/*
 * CImpIDocHostShowUI::ShowMessage
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 */
STDMETHODIMP CImpIDocHostShowUI::ShowMessage(
            HWND /*hwnd*/,
            LPOLESTR /*lpstrText*/,
            LPOLESTR /*lpstrCaption*/, 
            DWORD /*dwType*/,
            LPOLESTR /*lpstrHelpFile*/,
            DWORD /*dwHelpContext*/,
            LRESULT* /*plResult*/)
{
	return S_FALSE;
}

/*
 * CImpIDocHostShowUI::ShowHelp
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 */
STDMETHODIMP CImpIDocHostShowUI::ShowHelp(
            HWND /*hwnd*/,
            LPOLESTR /*pszHelpFile*/,
            UINT /*uCommand*/,
            DWORD /*dwData*/,
            POINT /*ptMouse*/,
            IDispatch* /*pDispatchObjectHit*/)
{
	return S_FALSE;
}

