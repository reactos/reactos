//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:      clntsite.cpp
//
//  Contents:  CClientSite implimentation
//
//  History:   16-Jan-97 EricB      Adapted from Trident sample.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

CClientSite::CClientSite(LPSITE pSite) :
    m_cRef(0)
{
#ifdef _DEBUG
	ASSERT(pSite != NULL);
    strcpy(szClass, "CClientSite");
#endif
    m_pSite = pSite;
}

CClientSite::~CClientSite(void)
{
	ASSERT(m_cRef == 0); // the only ref count should be the artifical one!
}

STDMETHODIMP CClientSite::QueryInterface(REFIID riid, void **ppv)
{
    return m_pSite->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CClientSite::AddRef(void)
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG) CClientSite::Release(void)
{
	ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
}

/*
 * CClientSite::SaveObject
 *
 * Purpose:
 *  Requests that the container call OleSave for the object that
 *  lives here.  Typically this happens on server shutdown.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         Standard.
 */
STDMETHODIMP CClientSite::SaveObject(void)
{
    return NOERROR;
}

/*
 * Unimplemented/trivial members
 *  GetMoniker
 *  GetContainer
 *  RequestNewObjectLayout
 *  OnShowWindow
 *  ShowObject
 */

STDMETHODIMP CClientSite::GetMoniker(DWORD /*dwAssign*/,
							DWORD /*dwWhich*/, LPMONIKER* /*ppmk*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CClientSite::GetContainer(LPOLECONTAINER* /*ppContainer*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CClientSite::RequestNewObjectLayout(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CClientSite::OnShowWindow(BOOL /*fShow*/)
{
/*
    ASSERT(m_pSite != NULL);

    //if (m_pSite->IsObjectVisible() == fShow)
    //       return NOERROR;

    //m_pSite->SetObjectVisible(fShow);
    HWND hWnd = m_pSite->m_pView->GetWnd();

    if (::IsWindow(hWnd) == FALSE)
        return NOERROR;

    InvalidateRect(hWnd, NULL, TRUE);

    if (fShow == FALSE)
    {
        ::BringWindowToTop(hWnd);
        ::SetFocus(hWnd);
    }
*/
    return NOERROR;
}

STDMETHODIMP CClientSite::ShowObject(void)
{
    return NOERROR;
}
