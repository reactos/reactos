/*
 * IPROPSINK.CPP
 * IPropertyNotifySink for Document Objects CSite class
 *
 * Copyright (c)1995-1999 Microsoft Corporation, All Rights Reserved
 */


#include "stdafx.h"
#include "site.h"

/**
	Note: the m_cRef count is provided for debugging purposes only.
	CSite controls the destruction of the object through delete,
	not reference counting
*/

/*
 * CImplPropertyNotifySink::CImplPropertyNotifySink
 * CImplPropertyNotifySink::~CImplPropertyNotifySink
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImplPropertyNotifySink::CImplPropertyNotifySink( PCSite pSite, LPUNKNOWN pUnkOuter )
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImplPropertyNotifySink::~CImplPropertyNotifySink( void )
{
}


/*
 * CImplPropertyNotifySink::QueryInterface
 * CImplPropertyNotifySink::AddRef
 * CImplPropertyNotifySink::Release
 *
 * Purpose:
 *  IUnknown members for CImplPropertyNotifySink object.
 */

STDMETHODIMP CImplPropertyNotifySink::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImplPropertyNotifySink::AddRef( void )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImplPropertyNotifySink::Release( void )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}


STDMETHODIMP CImplPropertyNotifySink::OnChanged(DISPID dispid)
{
    if (dispid == DISPID_READYSTATE)
        m_pSite->OnReadyStateChanged();
    return S_OK;
}


STDMETHODIMP CImplPropertyNotifySink::OnRequestEdit (DISPID /*dispid*/)
{
    return S_OK;
}

