/*
 * IIPSITE.CPP
 * IOleInPlaceSite for Document Objects CSite class
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
 * CImpIOleInPlaceSite::CImpIOleInPlaceSite
 * CImpIOleInPlaceSite::~CImpIOleInPlaceSite
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImpIOleInPlaceSite::CImpIOleInPlaceSite( PCSite pSite, LPUNKNOWN pUnkOuter)
{
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleInPlaceSite::~CImpIOleInPlaceSite( void )
{
}


/*
 * CImpIOleInPlaceSite::QueryInterface
 * CImpIOleInPlaceSite::AddRef
 * CImpIOleInPlaceSite::Release
 *
 * Purpose:
 *  IUnknown members for CImpIOleInPlaceSite object.
 */

STDMETHODIMP CImpIOleInPlaceSite::QueryInterface( REFIID riid, void **ppv )
{
    return m_pUnkOuter->QueryInterface( riid, ppv );
}


STDMETHODIMP_(ULONG) CImpIOleInPlaceSite::AddRef(void)
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIOleInPlaceSite::Release(void)
{
    --m_cRef;
    return m_pUnkOuter->Release();
}




/*
 * CImpIOleInPlaceActiveObject::GetWindow
 *
 * Purpose:
 *  Retrieves the handle of the window associated with the object
 *  on which this interface is implemented.
 *
 * Parameters:
 *  phWnd           HWND * in which to store the window handle.
 *
 * Return Value:
 *  HRESULT         S_OK if successful, E_FAIL if there is no
 *                  window.
 */
STDMETHODIMP CImpIOleInPlaceSite::GetWindow( HWND *phWnd )
{
    //This is the client-area window in the frame
    *phWnd = m_pSite->GetWindow();
    return S_OK;
}


/*
 * CImpIOleInPlaceActiveObject::ContextSensitiveHelp
 *
 * Purpose:
 *  Instructs the object on which this interface is implemented to
 *  enter or leave a context-sensitive help mode.
 *
 * Parameters:
 *  fEnterMode      BOOL TRUE to enter the mode, FALSE otherwise.
 *
 * Return Value:
 *  HRESULT         S_OK
 */

STDMETHODIMP CImpIOleInPlaceSite::ContextSensitiveHelp( 
											BOOL /*fEnterMode*/ )
{
    return S_OK;
}


/*
 * CImpIOleInPlaceSite::CanInPlaceActivate
 *
 * Purpose:
 *  Answers the server whether or not we can currently in-place
 *  activate its object.  By implementing this interface we say
 *  that we support in-place activation, but through this function
 *  we indicate whether the object can currently be activated
 *  in-place.  Iconic aspects, for example, cannot, meaning we
 *  return S_FALSE.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         S_OK if we can in-place activate the object
 *                  in this site, S_FALSE if not.
 */
STDMETHODIMP CImpIOleInPlaceSite::CanInPlaceActivate( void )
{    
    /*
     * We can always in-place activate--no restrictions for DocObjects.
     * We don't worry about other cases since CSite only ever creates
     * embedded files.
     */
    return S_OK;
}


/*
 * CImpIOleInPlaceSite::OnInPlaceActivate
 *
 * Purpose:
 *  Informs the container that an object is being activated in-place
 *  such that the container can prepare appropriately.  The
 *  container does not, however, make any user interface changes at
 *  this point.  See OnUIActivate.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */
STDMETHODIMP CImpIOleInPlaceSite::OnInPlaceActivate( void )
{
	LPOLEINPLACEOBJECT pIOleIPObject;
    HRESULT hr = m_pSite->GetObjectUnknown()->QueryInterface(
					IID_IOleInPlaceObject, (void**) &pIOleIPObject );

	m_pSite->SetIPObject( pIOleIPObject );
    return hr;
}



/*
 * CImpIOleInPlaceSite::OnInPlaceDeactivate
 *
 * Purpose:
 *  Notifies the container that the object has deactivated itself
 *  from an in-place state.  Opposite of OnInPlaceActivate.  The
 *  container does not change any UI at this point.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */

STDMETHODIMP CImpIOleInPlaceSite::OnInPlaceDeactivate( void )
{
    /*
     * Since we don't have an Undo command, we can tell the object
     * right away to discard its Undo state.
     */
    m_pSite->Activate(OLEIVERB_DISCARDUNDOSTATE);
    m_pSite->GetIPObject()->Release();
    return NOERROR;
}




/*
 * CImpIOleInPlaceSite::OnUIActivate
 *
 * Purpose:
 *  Informs the container that the object is going to start munging
 *  around with user interface, like replacing the menu.  The
 *  container should remove any relevant UI in preparation.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */

STDMETHODIMP CImpIOleInPlaceSite::OnUIActivate( void )
{
    //No state we have to set up here.
    return NOERROR;
}




/*
 * CImpIOleInPlaceSite::OnUIDeactivate
 *
 * Purpose:
 *  Informs the container that the object is deactivating its
 *  in-place user interface at which time the container may
 *  reinstate its own.  Opposite of OnUIActivate.
 *
 * Parameters:
 *  fUndoable       BOOL indicating if the object will actually
 *                  perform an Undo if the container calls
 *                  ReactivateAndUndo.
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */
STDMETHODIMP CImpIOleInPlaceSite::OnUIDeactivate( BOOL /*fUndoable*/ )
{
	// Normally we'd tidy up here, but since MSHTML.DLL is the only thing we host
	// the Frame will go away on deactivation so there's no point in restoring
	// the Frame's empty state

    return NOERROR;
}


/*
 * CImpIOleInPlaceSite::DeactivateAndUndo
 *
 * Purpose:
 *  If immediately after activation the object does an Undo, the
 *  action being undone is the activation itself, and this call
 *  informs the container that this is, in fact, what happened.
 *  The container should call IOleInPlaceObject::UIDeactivate.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */
STDMETHODIMP CImpIOleInPlaceSite::DeactivateAndUndo( void )
{
	// Tell the object we are deactivating
    m_pSite->GetIPObject()->InPlaceDeactivate();
    return NOERROR;
}




/*
 * CImpIOleInPlaceSite::DiscardUndoState
 *
 * Purpose:
 *  Informs the container that something happened in the object
 *  that means the container should discard any undo information
 *  it currently maintains for the object.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or an appropriate error code.
 */

STDMETHODIMP CImpIOleInPlaceSite::DiscardUndoState( void )
{
    return E_NOTIMPL;
}




/*
 * CImpIOleInPlaceSite::GetWindowContext
 *
 * Purpose:
 *  Provides an in-place object with pointers to the frame and
 *  document level in-place interfaces (IOleInPlaceFrame and
 *  IOleInPlaceUIWindow) such that the object can do border
 *  negotiation and so forth.  Also requests the position and
 *  clipping rectangles of the object in the container and a
 *  pointer to an OLEINPLACEFRAME info structure which contains
 *  accelerator information.
 *
 *  Note that the two interfaces this call returns are not
 *  available through QueryInterface on IOleInPlaceSite since they
 *  live with the frame and document, but not the site.
 *
 * Parameters:
 *  ppIIPFrame      LPOLEINPLACEFRAME * in which to return the
 *                  AddRef'd pointer to the container's
 *                  IOleInPlaceFrame.
 *  ppIIPUIWindow   LPOLEINPLACEUIWINDOW * in which to return
 *                  the AddRef'd pointer to the container document's
 *                  IOleInPlaceUIWindow.
 *  prcPos          LPRECT in which to store the object's position.
 *  prcClip         LPRECT in which to store the object's visible
 *                  region.
 *  pFI             LPOLEINPLACEFRAMEINFO to fill with accelerator
 *                  stuff.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CImpIOleInPlaceSite::GetWindowContext(
						LPOLEINPLACEFRAME* ppIIPFrame,
						LPOLEINPLACEUIWINDOW* ppIIPUIWindow,
						LPRECT prcPos,
						LPRECT prcClip,
						LPOLEINPLACEFRAMEINFO pFI )
{
    *ppIIPUIWindow = NULL;
    m_pSite->QueryInterface(
						IID_IOleInPlaceFrame, (void **)ppIIPFrame);
    
    if (NULL != prcPos)
	{
        GetClientRect( m_pSite->GetWindow(), prcPos );
	}

    *prcClip = *prcPos;

    pFI->cb = sizeof(OLEINPLACEFRAMEINFO);
    pFI->fMDIApp = FALSE;

	m_pSite->GetFrame()->GetWindow(&pFI->hwndFrame);
	SetWindowLong ( pFI->hwndFrame, GWL_STYLE,
		GetWindowLong ( pFI->hwndFrame, GWL_STYLE ) |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN );

    pFI->haccel = NULL;
    pFI->cAccelEntries = 0;

    return NOERROR;
}


/*
 * CImpIOleInPlaceSite::Scroll
 *
 * Purpose:
 *  Asks the container to scroll the document, and thus the object,
 *  by the given amounts in the sz parameter.
 *
 * Parameters:
 *  sz              SIZE containing signed horizontal and vertical
 *                  extents by which the container should scroll.
 *                  These are in device units.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CImpIOleInPlaceSite::Scroll( SIZE /*sz*/ )
{
    //Not needed for DocObjects
    return E_NOTIMPL;
}


/*
 * CImpIOleInPlaceSite::OnPosRectChange
 *
 * Purpose:
 *  Informs the container that the in-place object was resized.
 *  The container must call IOleInPlaceObject::SetObjectRects.
 *  This does not change the site's rectangle in any case.
 *
 * Parameters:
 *  prcPos          LPCRECT containing the new size of the object.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CImpIOleInPlaceSite::OnPosRectChange( LPCRECT /*prcPos*/ )
{
    //Not needed for DocObjects
    return E_NOTIMPL;
}
