/*
 * IADVSINK.CPP
 * IAdviseSink for Document Objects CSite class
 *
 * Copyright (c)1995-1999 Microsoft Corporation, All Rights Reserved
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
 * CImpIAdviseSink::CImpIAdviseSink
 * CImpIAdviseSink::~CImpIAdviseSink
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImpIAdviseSink::CImpIAdviseSink( PCSite pSite, LPUNKNOWN pUnkOuter )
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIAdviseSink::~CImpIAdviseSink( void )
{
}


/*
 * CImpIAdviseSink::QueryInterface
 * CImpIAdviseSink::AddRef
 * CImpIAdviseSink::Release
 *
 * Purpose:
 *  IUnknown members for CImpIAdviseSink object.
 */

STDMETHODIMP CImpIAdviseSink::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImpIAdviseSink::AddRef( void )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIAdviseSink::Release( void )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}



/*
 * Unused members in CImpIAdviseSink
 *  OnDataChange
 *  OnViewChange
 *  OnRename
 *  OnSave
 */

STDMETHODIMP_(void) CImpIAdviseSink::OnDataChange(LPFORMATETC /*pFEIn*/,
											LPSTGMEDIUM /*pSTM*/)
{
}

STDMETHODIMP_(void) CImpIAdviseSink::OnViewChange(DWORD /*dwAspect*/,
												LONG /*lindex*/)
{    
}

STDMETHODIMP_(void) CImpIAdviseSink::OnRename( LPMONIKER /*pmk*/ )
{
}

STDMETHODIMP_(void) CImpIAdviseSink::OnSave( void )
{
}


/*
 * CImpIAdviseSink::OnClose
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

STDMETHODIMP_(void) CImpIAdviseSink::OnClose( void )
{

}
