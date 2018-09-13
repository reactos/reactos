/*
 * IDOCHOSTUIHANDLER.CPP
 * IDocHostUIHandler for Document Objects CSite class
 *
 * Copyright (c)1995-1999 Microsoft Corporation, All Rights Reserved
 */


#include "stdafx.h"
#include <docobj.h>
#include "site.h"
#include "DHTMLEd.h"
#include "DHTMLEdit.h"
#include "proxyframe.h"

/**
	Note: the m_cRef count is provided for debugging purposes only.
	CSite controls the destruction of the object through delete,
	not reference counting
*/

/*
 * CImpIDocHostUIHandler::CImpIDocHostUIHandler
 * CImpIDocHostUIHandler::~CImpIDocHostUIHandler
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */
CImpIDocHostUIHandler::CImpIDocHostUIHandler( PCSite pSite, LPUNKNOWN pUnkOuter)
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIDocHostUIHandler::~CImpIDocHostUIHandler( void )
{
}



/*
 * CImpIDocHostUIHandler::QueryInterface
 * CImpIDocHostUIHandler::AddRef
 * CImpIDocHostUIHandler::Release
 *
 * Purpose:
 *  IUnknown members for CImpIOleDocumentSite object.
 */
STDMETHODIMP CImpIDocHostUIHandler::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImpIDocHostUIHandler::AddRef( void )
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIDocHostUIHandler::Release( void )
{
    --m_cRef;
    return m_pUnkOuter->Release();
}



// * CImpIDocHostUIHandler::GetHostInfo
// *
// * Purpose: Called at initialisation
// *
STDMETHODIMP CImpIDocHostUIHandler::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
	DWORD dwFlags = 0;
	BOOL bDialogEditing = FALSE;
	BOOL bDisplay3D= FALSE;
	BOOL bScrollbars = FALSE;
	BOOL bFlatScrollbars = FALSE;
	BOOL bContextMenu = FALSE;

	m_pSite->GetFrame()->HrGetDisplay3D(bDisplay3D);
	m_pSite->GetFrame()->HrGetScrollbars(bScrollbars);
	m_pSite->GetFrame()->HrGetDisplayFlatScrollbars(bFlatScrollbars);
	
	if (bDialogEditing == TRUE)
		dwFlags |= DOCHOSTUIFLAG_DIALOG;
	if (bDisplay3D == FALSE)
		dwFlags |= DOCHOSTUIFLAG_NO3DBORDER;
	if (bScrollbars == FALSE)
		dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
	if (bFlatScrollbars)
		dwFlags |= DOCHOSTUIFLAG_FLAT_SCROLLBAR;
	if (bContextMenu == FALSE)
		dwFlags |= DOCHOSTUIFLAG_DISABLE_HELP_MENU;

	pInfo->dwFlags = dwFlags;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

// * CImpIDocHostUIHandler::ShowUI
// *
// * Purpose: Called when MSHTML.DLL shows its UI
// *
STDMETHODIMP CImpIDocHostUIHandler::ShowUI(
				DWORD /*dwID*/, 
				IOleInPlaceActiveObject * /*pActiveObject*/,
				IOleCommandTarget * /*pCommandTarget*/,
				IOleInPlaceFrame * /*pFrame*/,
				IOleInPlaceUIWindow * /*pDoc*/)
{

	// We've already got our own UI in place so just return S_OK
    return S_OK;
}

// * CImpIDocHostUIHandler::HideUI
// *
// * Purpose: Called when MSHTML.DLL hides its UI
// *
STDMETHODIMP CImpIDocHostUIHandler::HideUI(void)
{
    return S_OK;
}

// * CImpIDocHostUIHandler::UpdateUI
// *
// * Purpose: Called when MSHTML.DLL updates its UI
// *
STDMETHODIMP CImpIDocHostUIHandler::UpdateUI(void)
{
	// we fire this from proxyframe's IOleCommandTarget
	return S_OK;
}

// * CImpIDocHostUIHandler::EnableModeless
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::EnableModeless
// *
STDMETHODIMP CImpIDocHostUIHandler::EnableModeless(BOOL /*fEnable*/)
{
    return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::OnDocWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnDocWindowActivate
// *
STDMETHODIMP CImpIDocHostUIHandler::OnDocWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::OnFrameWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnFrameWindowActivate
// *
STDMETHODIMP CImpIDocHostUIHandler::OnFrameWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::ResizeBorder
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::ResizeBorder
// *
STDMETHODIMP CImpIDocHostUIHandler::ResizeBorder(
				LPCRECT /*prcBorder*/, 
				IOleInPlaceUIWindow* /*pUIWindow*/,
				BOOL /*fRameWindow*/)
{
    return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::ShowContextMenu
// *
// * Purpose: Called when MSHTML.DLL would normally display its context menu
// *
STDMETHODIMP CImpIDocHostUIHandler::ShowContextMenu(
				DWORD /*dwID*/, 
				POINT* pptPosition,
				IUnknown* /*pCommandTarget*/,
				IDispatch* /*pDispatchObjectHit*/)
{
	USES_CONVERSION;

	HMENU hmenu = NULL;
	INT id = 0;
    HRESULT hr = NOERROR;
    LONG lLBound, lUBound, lIndex, lLBoundState, lUBoundState;
    BSTR  bstr=0;
    SAFEARRAY * psaStrings = NULL;
    SAFEARRAY * psaStates = NULL;
    int i;
	BOOL ok = FALSE;
	ULONG	state = 0;
	CComBSTR _bstr;

	_ASSERTE(m_pSite);
	_ASSERTE(m_pSite->GetFrame());
	_ASSERTE(m_pSite->GetFrame()->GetControl());
	_ASSERTE(m_pSite->GetFrame()->GetControl()->m_hWndCD);

	// Correct X & Y position for local coordinates:
	POINT ptPos = *pptPosition;
	HWND hwndDoc = m_pSite->GetFrame()->GetDocWindow ();
	_ASSERTE ( hwndDoc );
	_ASSERTE ( ::IsWindow ( hwndDoc ) );
	if ( ( NULL != hwndDoc ) && ::IsWindow ( hwndDoc ) )
	{
		::ScreenToClient ( hwndDoc, &ptPos );
		// correct for scrolling
		POINT ptScrollPos;
		if ( SUCCEEDED ( m_pSite->GetFrame()->GetScrollPos ( &ptScrollPos ) ) )
		{
			ptPos.x += ptScrollPos.x;
			ptPos.y += ptScrollPos.y;
		}
		m_pSite->GetFrame()->GetControl()->Fire_ShowContextMenu ( ptPos.x, ptPos.y );
	}

	psaStrings = m_pSite->GetFrame()->GetMenuStrings();
	psaStates = m_pSite->GetFrame()->GetMenuStates();

	if (NULL == psaStrings || NULL == psaStates)
		return S_OK;

	SafeArrayGetLBound(psaStrings, 1, &lLBound);
	SafeArrayGetUBound(psaStrings, 1, &lUBound);

	SafeArrayGetLBound(psaStates, 1, &lLBoundState);
	SafeArrayGetUBound(psaStates, 1, &lUBoundState);

	if (lLBound != lLBoundState || lUBound != lUBoundState)
		return S_OK;

	// there arrays have no elements
#if 0
	Bug 15224: lower and upper bound are both zero if there is one element in sthe array.
	psaStrings is NULL if there are no strings.
	if (lLBound == lUBound)
		return S_OK;
#endif

	hmenu = CreatePopupMenu();

	if (NULL == hmenu)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		return hr;
	}

	for (lIndex=lLBound, i=0; lIndex<=lUBound && i <= 256; lIndex++, i++)
	{
		if ( FADF_BSTR & psaStrings->fFeatures )
		{
			SafeArrayGetElement(psaStrings, &lIndex, &bstr);
			_bstr = bstr;
		}
		else if ( FADF_VARIANT & psaStrings->fFeatures )
		{
			VARIANT var;
			VariantInit ( &var );
			SafeArrayGetElement(psaStrings, &lIndex, &var);
			VariantChangeType ( &var, &var, 0, VT_BSTR );
			_bstr = var.bstrVal;
			VariantClear ( &var );
		}
		else
		{
			_ASSERTE ( ( FADF_BSTR | FADF_VARIANT ) & psaStrings->fFeatures );
			return E_UNEXPECTED;
		}

		if ( FADF_VARIANT & psaStates->fFeatures )
		{
			VARIANT var;
			VariantInit ( &var );
			SafeArrayGetElement(psaStates, &lIndex, &var);
			VariantChangeType ( &var, &var, 0, VT_I4 );
			state = var.lVal;
			VariantClear ( &var );
		}
		else
		{
			// A safe array of integers seems to use an fFeatures == 0, which can't
			// safely be tested for.
			SafeArrayGetElement(psaStates, &lIndex, &state);
		}
		
		if (_bstr.Length() == 0)
			state = MF_SEPARATOR|MF_ENABLED;
		else  if (state == triGray)
			state = MF_GRAYED;
		else if (state == triChecked)
			state = MF_CHECKED|MF_ENABLED;
		else  
			state = MF_ENABLED;

		ok = AppendMenu(hmenu, MF_STRING | state, i+35000, W2T(_bstr.m_str));

		_ASSERTE(ok);
	}

	id = (INT)TrackPopupMenu(
			hmenu,
			TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
			pptPosition->x,
			pptPosition->y,
			0,
			m_pSite->GetFrame()->GetControl()->m_hWndCD,
			NULL);

	_ASSERTE(id == 0 || (id >= 35000 && id <= 35000+i));

	if (id >= 35000 && id <= 35000+i)
		m_pSite->GetFrame()->GetControl()->Fire_ContextMenuAction(id-35000);

	DestroyMenu(hmenu);

    return S_OK;
}

// * CImpIDocHostUIHandler::TranslateAccelerator
// *
// * Purpose: Called from MSHTML.DLL's TranslateAccelerator routines
// *
STDMETHODIMP CImpIDocHostUIHandler::TranslateAccelerator(LPMSG /*lpMsg*/,
            /* [in] */ const GUID __RPC_FAR * /*pguidCmdGroup*/,
            /* [in] */ DWORD /*nCmdID*/)
{
    return S_FALSE;
}

// * CImpIDocHostUIHandler::GetOptionKeyPath
// *
// * Purpose: Called by MSHTML.DLL to find where the host wishes to store 
// *	its options in the registry
// *
STDMETHODIMP CImpIDocHostUIHandler::GetOptionKeyPath(BSTR* pbstrKey, DWORD)
{
	pbstrKey = NULL; // docs say this should be set to null if not used
	return S_OK;
}

STDMETHODIMP CImpIDocHostUIHandler::GetDropTarget( 
            /* [in] */ IDropTarget __RPC_FAR * /*pDropTarget*/,
            /* [out] */ IDropTarget __RPC_FAR *__RPC_FAR * /*ppDropTarget*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CImpIDocHostUIHandler::GetExternal( 
    /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
{
	_ASSERTE ( ppDispatch );
	if ( NULL == ppDispatch )
	{
		return E_INVALIDARG;
	}
	*ppDispatch = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CImpIDocHostUIHandler::TranslateUrl( 
    /* [in] */ DWORD /*dwTranslate*/,
    /* [in] */ OLECHAR __RPC_FAR * /*pchURLIn*/,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR * /*ppchURLOut*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CImpIDocHostUIHandler::FilterDataObject( 
    /* [in] */ IDataObject __RPC_FAR * /*pDO*/,
    /* [out] */ IDataObject __RPC_FAR *__RPC_FAR * /*ppDORet*/)
{
    return E_NOTIMPL;
}
