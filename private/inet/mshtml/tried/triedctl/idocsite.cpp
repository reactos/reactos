/*
 * IDOCSITE.CPP
 * IOleDocumentSite for Document Objects CSite class
 *
 * Copyright (c)1995-1999 Microsoft Corporation, All Rights Reserved
 */


#include "stdafx.h"
#include <docobj.h>
#include "DHTMLEd.h"
#include "DHTMLEdit.h"
#include "site.h"
#include "proxyframe.h"

/**
	Note: the m_cRef count is provided for debugging purposes only.
	CSite controls the destruction of the object through delete,
	not reference counting
*/

/*
 * CImpIOleDocumentSite::CImpIOleDocumentSite
 * CImpIOleDocumentSite::~CImpIOleDocumentSite
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */
CImpIOleDocumentSite::CImpIOleDocumentSite( PCSite pSite, LPUNKNOWN pUnkOuter)
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleDocumentSite::~CImpIOleDocumentSite( void )
{
}



/*
 * CImpIOleDocumentSite::QueryInterface
 * CImpIOleDocumentSite::AddRef
 * CImpIOleDocumentSite::Release
 *
 * Purpose:
 *  IUnknown members for CImpIOleDocumentSite object.
 */
STDMETHODIMP CImpIOleDocumentSite::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImpIOleDocumentSite::AddRef( void )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIOleDocumentSite::Release( void )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}



/*
 * CImpIOleDocumentsite::ActivateMe
 *
 * Purpose:
 *  Instructs the container to activate the object in this site as
 *  a document object.
 *
 * Parameters:
 *  pView           IOleDocumentView * of the object to activate.
 *
 * Return Value:
 *  HRESULT         S_OK if successful, error code otherwise.
 */
STDMETHODIMP CImpIOleDocumentSite::ActivateMe( IOleDocumentView *pView )
{
    RECT                rc;
    IOleDocument*       pDoc;
    
    /*
     * If we're passed a NULL view pointer, then try to get one from
     * the document object (the object within us).
     */
    if ( NULL == pView )
    {
        if ( FAILED( m_pSite->GetObjectUnknown()->QueryInterface( 
 								IID_IOleDocument, (void **)&pDoc ) ) )
		{
            return E_FAIL;
		}

        if ( FAILED( pDoc->CreateView( m_pSite->GetIPSite(),
												NULL, 0, &pView ) ) )
		{
			pDoc->Release();
            return E_OUTOFMEMORY;
		}

        // Release doc pointer since CreateView is a good com method that addrefs
        pDoc->Release();
    }        
    else
    {
        //Make sure that the view has our client site
        pView->SetInPlaceSite( m_pSite->GetIPSite() );

        //We're holding onto the pointer, so AddRef it.
        pView->AddRef();
    }


    // Activation steps, now that we have a view:

    m_pSite->SetDocView( pView );
    
    //This sets up toolbars and menus first    
    pView->UIActivate( TRUE );

    //Set the window size sensitive to new toolbars
    m_pSite->GetFrame()->GetControl()->GetClientRect( &rc );
    pView->SetRect( &rc );

	//Makes it all active
    pView->Show( TRUE );    
    return S_OK;
}
