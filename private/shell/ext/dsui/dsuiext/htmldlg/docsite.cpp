//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      docsite.cpp
//
//  Contents:  CDocSite implimentation
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

CDocSite::CDocSite(LPSITE pSite) :
    m_cRef(0)
{
#ifdef _DEBUG
	ASSERT(pSite != NULL);
    strcpy(szClass, "CDocSite");
#endif
    m_pSite = pSite;
}

CDocSite::~CDocSite(void)
{
	ASSERT(m_cRef == 0);
}

STDMETHODIMP
CDocSite::QueryInterface(REFIID riid, void **ppv)
{
    return m_pSite->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CDocSite::AddRef(void)
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG)
CDocSite::Release(void)
{
    ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
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
 *  HRESULT         NOERROR if successful, error code otherwise.
 */
STDMETHODIMP
CDocSite::ActivateMe(IOleDocumentView *pView)
{
    RECT                rc;
    IOleDocument*       pDoc;
    
    /*
     * If we're passed a NULL view pointer, then try to get one from
     * the document object (the object within us).
     */
    if (NULL == pView)
    {
        if (FAILED(m_pSite->GetObjectUnknown()->QueryInterface(
 								IID_IOleDocument, (void **)&pDoc)))
		{
            return E_FAIL;
		}

        if (FAILED(pDoc->CreateView(m_pSite->GetIPSite(),
												NULL, 0, &pView)))
		{
            return E_OUTOFMEMORY;
		}

        // Release doc pointer since CreateView is a good com method that addrefs
        pDoc->Release();
    }        
    else
    {
        //Make sure that the view has our client site
        pView->SetInPlaceSite(m_pSite->GetIPSite());

        //We're holding onto the pointer, so AddRef it.
        pView->AddRef();
    }

    // Activation steps, now that we have a view:

    m_pSite->SetDocView(pView);
    
    //This sets up toolbars and menus first    
    pView->UIActivate(TRUE);

    //Set the window size sensitive to new toolbars
    GetClientRect(m_pSite->GetFrame()->GetWnd(), &rc);
    pView->SetRect(&rc);

	//Makes it all active
    pView->Show(TRUE);    
    return NOERROR;
}
