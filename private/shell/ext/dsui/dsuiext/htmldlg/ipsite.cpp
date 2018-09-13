//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:      ipsite.cpp
//
//  Contents:  CInPlaceSite implimentation
//
//  History:   16-Jan-97 EricB      Adapted from Trident sample.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

CInPlaceSite::CInPlaceSite(LPSITE pSite) :
    m_cRef(0)
{
#ifdef _DEBUG
	ASSERT(pSite != NULL);
    strcpy(szClass, "CInPlaceSite");
#endif
    m_pSite = pSite;
}

CInPlaceSite::~CInPlaceSite( void )
{
	ASSERT(m_cRef == 0);
}

STDMETHODIMP
CInPlaceSite::QueryInterface( REFIID riid, void **ppv )
{
    return m_pSite->QueryInterface( riid, ppv );
}

STDMETHODIMP_(ULONG)
CInPlaceSite::AddRef( void )
{
    ++m_cRef;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG)
CInPlaceSite::Release( void )
{
    ASSERT(m_cRef > 0);
    --m_cRef;
    return m_pSite->Release();
}

/*
 * CInPlaceSite::GetWindow
 *
 * Purpose:
 *  Retrieves the handle of the window associated with the object
 *  on which this interface is implemented.
 *
 * Parameters:
 *  phWnd           HWND * in which to store the window handle.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, E_FAIL if there is no
 *                  window.
 */
STDMETHODIMP
CInPlaceSite::GetWindow( HWND *phWnd )
{
    //This is the client-area window in the frame
    *phWnd = m_pSite->GetWindow();
    return NOERROR;
}

/*
 * CInPlaceSite::ContextSensitiveHelp
 *
 * Purpose:
 *  Instructs the object on which this interface is implemented to
 *  enter or leave a context-sensitive help mode.
 *
 * Parameters:
 *  fEnterMode      BOOL TRUE to enter the mode, FALSE otherwise.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */

STDMETHODIMP
CInPlaceSite::ContextSensitiveHelp(BOOL /*fEnterMode*/)
{
    return NOERROR;
}

/*
 * CInPlaceSite::CanInPlaceActivate
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
 *  HRESULT         NOERROR if we can in-place activate the object
 *                  in this site, S_FALSE if not.
 */
STDMETHODIMP
CInPlaceSite::CanInPlaceActivate( void )
{    
    /*
     * We can always in-place activate--no restrictions for DocObjects.
     * We don't worry about other cases since CSite only ever creates
     * embedded files.
     */
    return NOERROR;
}


/*
 * CInPlaceSite::OnInPlaceActivate
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
STDMETHODIMP CInPlaceSite::OnInPlaceActivate( void )
{
	LPOLEINPLACEOBJECT pIOleIPObject;
    HRESULT hr = m_pSite->GetObjectUnknown()->QueryInterface(
					IID_IOleInPlaceObject, (void**) &pIOleIPObject );
	m_pSite->SetIPObject( pIOleIPObject );


    return hr;
}



/*
 * CInPlaceSite::OnInPlaceDeactivate
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

STDMETHODIMP CInPlaceSite::OnInPlaceDeactivate( void )
{
    /*
     * Since we don't have an Undo command, we can tell the object
     * right away to discard its Undo state.
     */
    m_pSite->Activate( OLEIVERB_DISCARDUNDOSTATE );
    m_pSite->GetIPObject()->Release();

    return NOERROR;
}




/*
 * CInPlaceSite::OnUIActivate
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

STDMETHODIMP CInPlaceSite::OnUIActivate( void )
{
    //No state we have to set up here.
    return NOERROR;
}




/*
 * CInPlaceSite::OnUIDeactivate
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
STDMETHODIMP CInPlaceSite::OnUIDeactivate( BOOL /*fUndoable*/ )
{
	// Normally we'd tidy up here, but since Trident is the only thing we host
	// the Frame will go away on deactivation so there's no point in restoring
	// the Frame's empty state

    return NOERROR;
}


/*
 * CInPlaceSite::DeactivateAndUndo
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
STDMETHODIMP CInPlaceSite::DeactivateAndUndo( void )
{
	// Tell the object we are deactivating
    m_pSite->GetIPObject()->InPlaceDeactivate();
    return NOERROR;
}




/*
 * CInPlaceSite::DiscardUndoState
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

STDMETHODIMP CInPlaceSite::DiscardUndoState( void )
{
    return E_NOTIMPL;
}




/*
 * CInPlaceSite::GetWindowContext
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
STDMETHODIMP CInPlaceSite::GetWindowContext(
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

    pFI->haccel = NULL;
    pFI->cAccelEntries = 0;

    return NOERROR;
}


/*
 * CInPlaceSite::Scroll
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
STDMETHODIMP CInPlaceSite::Scroll( SIZE /*sz*/ )
{
    //Not needed for DocObjects
    return E_NOTIMPL;
}

/*
 * CInPlaceSite::OnPosRectChange
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
STDMETHODIMP
CInPlaceSite::OnPosRectChange( LPCRECT /*prcPos*/ )
{
    //Not needed for DocObjects
    return E_NOTIMPL;
}
