//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      advise.cpp
//
//  Contents:  CAdviseSink implimentation
//
//  History:   22-Jan-97 EricB      Adapted from Trident sample.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "SiteObj.h"

CAdviseSink::CAdviseSink(LPSITE pSite) :
    m_cRef(0)
{
    m_pSite = pSite;
#ifdef _DEBUG
	ASSERT(pSite != NULL);
    strcpy(szClass, "CAdviseSink");
#endif
}

CAdviseSink::~CAdviseSink(void)
{
	ASSERT(m_cRef == 0);
}

STDMETHODIMP
CAdviseSink::QueryInterface(REFIID riid, void **ppv)
{
    return m_pSite->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CAdviseSink::AddRef(void)
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG)
CAdviseSink::Release(void)
{
	ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
}

/*
 * Unused members in CAdviseSink
 *  OnDataChange
 *  OnViewChange
 *  OnRename
 *  OnSave
 */

STDMETHODIMP_(void)
CAdviseSink::OnDataChange(LPFORMATETC /*pFEIn*/, LPSTGMEDIUM /*pSTM*/)
{
}

STDMETHODIMP_(void)
CAdviseSink::OnViewChange(DWORD /*dwAspect*/, LONG /*lindex*/)
{    
/*
    if (dwAspect != DVASPECT_CONTENT)
    {
        return;
    }

    ASSERT(m_pWnd != NULL);

    if (m_pWnd->GetWnd() != NULL)
    {
        InvalidateRect(m_pWnd->GetWnd(), NULL, TRUE);
    }
*/
}

STDMETHODIMP_(void)
CAdviseSink::OnRename(LPMONIKER /*pmk*/)
{
}

STDMETHODIMP_(void)
CAdviseSink::OnSave(void)
{
}

/*
 * CAdviseSink::OnClose
 *
 * Purpose:
 *  Informs the advise sink that the OLE object has closed and is
 *  no longer bound in any way.  We use this to do a File/Close
 *  to delete the object since we don't hold onto any.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

STDMETHODIMP_(void)
CAdviseSink::OnClose(void)
{
	//This does the same as File/Close
	//Not quite because CFrame::Close requires some synchronous
	//calls that will fail if done here, so just post a message to
	//do this m_pSite->m_pView->Close();

//	HWND		hwnd;
//	m_pSite->GetFrame()->GetWindow(&hwnd);
//	PostMessage(
//		hwnd, 
//		WM_COMMAND,
//		MAKELONG(IDM_FILECLOSE, 0),
//		0);
//    return;
}
