//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      dhshowui.cpp
//
//  Contents:  CDocHostShowUi implimentation
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

CDosHostShowUi::CDosHostShowUi(LPSITE pSite) :
    m_cRef(0)
{
#ifdef _DEBUG
	ASSERT(pSite != NULL);
    strcpy(szClass, "CDocHostShowUi");
#endif
    m_pSite = pSite;
}

CDosHostShowUi::~CDosHostShowUi( void )
{
	ASSERT(m_cRef == 0);
}

STDMETHODIMP
CDosHostShowUi::QueryInterface( REFIID riid, void **ppv )
{
    return m_pSite->QueryInterface( riid, ppv );
}

STDMETHODIMP_(ULONG)
CDosHostShowUi::AddRef( void )
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG)
CDosHostShowUi::Release( void )
{
    ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
}

/*
 * CDosHostShowUi::ShowMessage
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 */
STDMETHODIMP CDosHostShowUi::ShowMessage(
            HWND /*hwnd*/,
            LPOLESTR /*lpstrText*/,
            LPOLESTR /*lpstrCaption*/, 
            DWORD /*dwType*/,
            LPOLESTR /*lpstrHelpFile*/,
            DWORD /*dwHelpContext*/,
            LRESULT* /*plResult*/)
{
	// Well I would intercept a message here to throw up an alternative message box
	// But it looks like Trident doesn't generate any at the moment so I'm not gong to bother!
	return S_FALSE;
}

/*
 * CDosHostShowUi::ShowHelp
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 */
STDMETHODIMP CDosHostShowUi::ShowHelp(
            HWND /*hwnd*/,
            LPOLESTR /*pszHelpFile*/,
            UINT /*uCommand*/,
            DWORD /*dwData*/,
            POINT /*ptMouse*/,
            IDispatch* /*pDispatchObjectHit*/)
{
	return S_FALSE;
}

