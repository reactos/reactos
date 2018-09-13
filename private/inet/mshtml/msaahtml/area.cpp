//================================================================================
//		File:	Area.CPP
//		Date: 	6/26/97
//		Desc:	contains implementation of CAreaAO class.  CAreaAO implements
//		the MSAA proxy for the Trident Area element
//
//		Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "trid_ao.h"
#include "document.h"
#include "area.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

#ifdef	_MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CAreaAO,ImplIHTMLAreaEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLAREAEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLAREAEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLAREAEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
		 
END_EVENT_HANDLER_MAP()

#endif



//================================================================================
// CAreaAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CAreaAO::CAreaAO()
//
//	DESCRIPTION:
//
//		constructor
//
//	PARAMETERS:
//
//		pAOParent			pointer to the parent accessible object in 
//							the AOM tree
//
//		nTOMIndex			index of the element from the TOM document.all 
//							collection.
//		
//		nChildID			unique Child ID
//
//		hWnd				pointer to the window of the trident object that 
//							this object corresponds to.
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CAreaAO::CAreaAO(CTridentAO * pAOMParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CAreaAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;
	
	//--------------------------------------------------
	// set the role and action parameters to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_LINK;

	
	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_AREA;


#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------

	lstrcpy( m_szAOMName, _T("AreaAE") );

#endif

	
}



//-----------------------------------------------------------------------
//	CAreaAO::~CAreaAO()
//
//	DESCRIPTION:
//
//		CAreaAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CAreaAO::~CAreaAO()
{
		
}


//-----------------------------------------------------------------------
//	CAreaAO::Init()
//
//	DESCRIPTION:
//
//		Initialization : set values of data members
//
//	PARAMETERS:
//
//		pTOMObjUnk		pointer to IUnknown of TOM object.
//
//	RETURNS:
//
//		S_OK | E_FAIL
//
// ----------------------------------------------------------------------

HRESULT CAreaAO::Init( IUnknown* pTOMObjIUnk )
{
	HRESULT hr = S_OK;

	assert( pTOMObjIUnk );
	
	
	//--------------------------------------------------
	// call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAO::Init( pTOMObjIUnk );
	
#ifdef _MSAA_EVENTS

	if ( hr == S_OK )
	{
		HRESULT	hrEventInit;

		//--------------------------------------------------
		// allocate event handling interface  and establish
		// Advise.
		//--------------------------------------------------
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLAreaEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

#ifdef _DEBUG
		//--------------------------------------------------
		//	If we are in debug mode, assert any
		//	event handler initialization errors.
		//
		//	(In release mode, just ignore.  This will allow
		//	the object to be created, it just won't support
		//	events.)
		//--------------------------------------------------

		assert( hrEventInit == S_OK );

		if ( hrEventInit != S_OK )
			OutputDebugString( "Event handler initialization in CAreaAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CAreaAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CAreaAO object only implements
//		IUnknown.
//
//	PARAMETERS:
//
//		riid		REFIID of requested interface.
//		ppv			pointer to interface in.
//
//	RETURNS:
//
//		E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CAreaAO::QueryInterface( REFIID riid, void** ppv )
{


	//--------------------------------------------------
	// validate input.
	//--------------------------------------------------

	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;

    if (riid == IID_IUnknown)  
    {
		*ppv = (IUnknown *)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLAreaElementEvents)
	{
		//--------------------------------------------------
		//	This is the event interface for the
		//	CButtonAO class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLAreaEvents,ppv,HTMLButtonElementEvents)
	} 
	
#endif

	else
	{
		//--------------------------------------------------
		// delegate to base class
		//--------------------------------------------------

        return(CTridentAO::QueryInterface(riid,ppv));
	}

	//--------------------------------------------------
	// if ppv has not been set, then we don't support
	// the requested interface.
	//--------------------------------------------------

	if ( !(*ppv) )
	{
        return E_NOINTERFACE;
	}
    
	((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
    
}



//================================================================================
// CAreaAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CAreaAO::GetAccName()
//
//	DESCRIPTION:
//
//	lChild		child ID
//	pbstrName		pointer to array to return child name in.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::GetAccName( long lChild, BSTR* pbstrName )
{
	HRESULT hr = S_OK;
	

	assert( pbstrName );


	if ( m_bstrName )
		*pbstrName = SysAllocString( m_bstrName );
	else
	{
		hr = getTitleFromIHTMLElement( pbstrName );

		if ( hr == S_OK )
		{
			if ( !*pbstrName )
				hr = DISP_E_MEMBERNOTFOUND;
			else
				m_bstrName = SysAllocString( *pbstrName );
		}
	}


	return hr;
}


//-----------------------------------------------------------------------
//	CAreaAO::GetAccValue()
//
//	DESCRIPTION:
//
//		returns value string to client.
//
//	PARAMETERS:
//
//		lChild		ChildID/SelfID
//
//		pbstrValue	returned value string
//
//	RETURNS:
//
//		HRESULT :	S_OK if success, S_FALSE if fail, DISP_E_MEMBERNOTFOUND
//					for no implement.
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::GetAccValue( long lChild, BSTR* pbstrValue )
{
	HRESULT hr = S_OK;


	assert( pbstrValue );


	if( m_bstrValue )
	{
		*pbstrValue = SysAllocString( m_bstrValue );
	}
	else
	{
		CComQIPtr<IHTMLAreaElement,&IID_IHTMLAreaElement> pIHTMLAreaEl(m_pTOMObjIUnk);

		if ( !pIHTMLAreaEl )
			return E_NOINTERFACE;

		hr = pIHTMLAreaEl->get_href( pbstrValue );

		if ( hr == S_OK )
		{
			if ( !*pbstrValue )
				hr = DISP_E_MEMBERNOTFOUND;
			else
				m_bstrValue = SysAllocString( *pbstrValue );
		}
	}

	return hr;
}


//-----------------------------------------------------------------------
//	CAreaAO::GetAccState()
//
//	DESCRIPTION:
//
//		returns state of area : always linked, maybe focusable/focused.
//
//	PARAMETERS:
//		
//		lChild		ChildID
//		plState		long to store returned state var in.
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------
	
HRESULT	CAreaAO::GetAccState( long lChild, long* plState )
{
	HRESULT hr = S_OK;
	

	assert( plState );


	//--------------------------------------------------
	//	Initialize the out parameter.
	//--------------------------------------------------

	*plState = 0;


	//--------------------------------------------------
	//	Call down to base class to get the visibility.
	//
	//	This call returns either S_OK or
	//	DISP_E_MEMBERNOTFOUND.  If the latter is
	//	returned, reset the state to zero.
	//--------------------------------------------------

	long lTempState = 0;

	hr = CTridentAO::GetAccState( lChild, &lTempState );

	if ( hr != S_OK )
		lTempState = 0;


	//--------------------------------------------------
	//	Area's states area always (at a minimum)
	//	STATE_SYSTEM_LINKED.
	//--------------------------------------------------

	lTempState |= STATE_SYSTEM_LINKED;

	//--------------------------------------------------
	// Update the location to determine offscreen status
	//--------------------------------------------------

	long lDummy;

	hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

	if (SUCCEEDED( hr ) && m_bOffScreen)
		lTempState |= STATE_SYSTEM_INVISIBLE;


	//--------------------------------------------------
	//	If there is a focused element on the page,
	//	that means that this element is FOCUSABLE.
	//--------------------------------------------------

	UINT uTOMID = 0;

	hr = GetFocusedTOMElementIndex( &uTOMID );
	
	//--------------------------------------------------
	//	The CAreaAO is FOCUSABLE only if the Trident
	//	document or one of its children has the focus.
	//--------------------------------------------------

	if ( hr == S_OK  &&  uTOMID > 0 )
	{
		lTempState |= STATE_SYSTEM_FOCUSABLE;

		//--------------------------------------------------
		//	If IDs match, then this element has the focus.
		//--------------------------------------------------

		if ( uTOMID == m_nTOMIndex )
			lTempState |= STATE_SYSTEM_FOCUSED;
	}
	else
	{
		//--------------------------------------------------
		//	GetFocusedTOMElementIndex() returned something
		//	other than S_OK (or an invalid source index)
		//	meaning that the source index of the focused TEO
		//	could not be obtained.
		//	So, just assume that there is no active element,
		//	which means that our CTridentAO can neither be
		//	FOCUSABLE nor FOCUSED.
		//--------------------------------------------------

		hr = S_OK;
	}
	
	*plState = lTempState;


	return hr;
}


//-----------------------------------------------------------------------
//	CAreaAO::GetAccDefaultAction()
//
//	DESCRIPTION:
//		returns description string for default action
//	
//	PARAMETERS:
//
//		lChild			child /self ID
//
//		pbstrDefAction	returned description string.
//
//	RETURNS:
//
//		HRESULT :	S_OK if success, E_FAIL if fail, DISP_E_MEMBERNOTFOUND
//					for no implement.
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
{		
	//--------------------------------------------------
	//	Validate the parameters
	//--------------------------------------------------

	assert( pbstrDefAction );

	if ( !pbstrDefAction )
		return E_INVALIDARG;
    else
        return GetResourceStringValue(IDS_JUMP_ACTION, pbstrDefAction);
}


//-----------------------------------------------------------------------
//	CAreaAO::AccDoDefaultAction()
//
//	DESCRIPTION:
//		(1) select area
//		(2) click it
//	
//	PARAMETERS:
//
//		lChild		child / self ID
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::AccDoDefaultAction( long lChild )
{
	HRESULT hr = S_OK;
	

	//--------------------------------------------------
	//	Scroll the area into view.
	//--------------------------------------------------

	hr = ScrollIntoView();
	
	if ( hr == S_OK )
	{
		//--------------------------------------------------
		//	Focus the area.
		//--------------------------------------------------

		hr = focus();

		if ( hr == S_OK )
		{
			//--------------------------------------------------
			//	Click the area.
			//--------------------------------------------------
	
			hr = click();
		}
	}


	return hr;
}


//-----------------------------------------------------------------------
//	CAreaAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description string to client.
//
//	PARAMETERS:
//
//		lChild				Child/Self ID
//
//		pbstrDescription		Description string returned to client.
//	
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::GetAccDescription( long lChild, BSTR* pbstrDescription )
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrDescription );

	if( !m_bstrDescription )
	{
		BSTR	bstrShape = NULL;

		//--------------------------------------------------
		//	Build a description string that looks like
		//	"Link region type: <shape>" where <shape> is
		//	{ CIRCLE | RECTANGLE | POLYGON }.
		//--------------------------------------------------

		//--------------------------------------------------
		//	Get the Area's shape.
		//--------------------------------------------------

		CComQIPtr<IHTMLAreaElement,&IID_IHTMLAreaElement> pIHTMLAreaElement(m_pTOMObjIUnk);

		if ( !pIHTMLAreaElement )
			return E_NOINTERFACE;

		hr = pIHTMLAreaElement->get_shape( &bstrShape );

		if ( hr == S_OK && bstrShape )
        {
			//--------------------------------------------------
			//	Create the description string.
			//--------------------------------------------------

			WCHAR wszDesc[MAX_PATH];

            hr = GetResourceStringValue(IDS_LINK_DESCRIPTION, pbstrDescription);
            if (hr)
            {
    			SysFreeString( bstrShape );
                goto Cleanup;
            }

			wcscpy( wszDesc, *pbstrDescription);
			wcscat( wszDesc, bstrShape );
			SysFreeString( bstrShape );
            SysFreeString(*pbstrDescription);

			if ( !(m_bstrDescription = SysAllocString( wszDesc )) )
				return E_OUTOFMEMORY;
		}
		else
			return DISP_E_MEMBERNOTFOUND;
	}

	*pbstrDescription = SysAllocString( m_bstrDescription );

Cleanup:
	return hr;
}


//-----------------------------------------------------------------------
//	CAreaAO::AccLocation()
//
//	DESCRIPTION:
//	This override of CTridentAO::AccLocation is due to IE4 behavior
//  (not yet identified as a bug -- may be by design) -- Map elements
//  have no parents in the TOM world.  This breaks default AccLocation().
//	This version of AccLocation() calls its parent elements AccLocation to
//	get the correct offset from the root parent.  Instead of relying on TOM,
//	we rely on AOM -- to a point -- we again rely on TOM once we get to 
//	the parent's AccLocation call to provide us with the correct scroll and 
//	parent offsets.
//
//	PARAMETERS:
//		
//		pxLeft		returns left coord.
//		pyTop		returns top coord
//		pcxWidth	returns width coord.
//		pcyHeight	returns height coord.
//		lChild		child ID
//
//	RETURNS:
//
//		S_OK | E_FAIL
// ----------------------------------------------------------------------

HRESULT	CAreaAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
	HRESULT hr;

	long xLeft		=0;
	long yTop		=0;
	long cxWidth	=0;
	long cyHeight	=0;

	assert( pxLeft );
	assert( pyTop );
	assert( pcxWidth );
	assert( pcyHeight );

	//--------------------------------------------------
	// get screen locations
	//--------------------------------------------------

	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

	if(!(pIHTMLElement))
		return(E_NOINTERFACE);

	hr = pIHTMLElement->get_offsetLeft(&xLeft);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetTop(&yTop);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetWidth(&cxWidth);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetHeight(&cyHeight);
	if(hr != S_OK)
		return(hr);

	//--------------------------------------------------
	// get top and left coords of parent element
	//--------------------------------------------------

	long xParentLeft;
	long yParentTop;
	long cxParentWidth;
	long cyParentHeight;

	assert(m_pParent);

	hr = m_pParent->AccLocation(&xParentLeft,&yParentTop,&cxParentWidth,&cyParentHeight,lChild);

	if(hr != S_OK)
		return hr;


	xLeft += xParentLeft;
	yTop  += yParentTop;


	//--------------------------------------------------
	// Offscreen state to FALSE until proven otherwise.
	//--------------------------------------------------

    m_bOffScreen = FALSE;


	//--------------------------------------------------
	// Get parent's state to see if it's offscreen.
	//--------------------------------------------------

    long lTempState = 0;

    hr = m_pParent->GetAccState(lChild, &lTempState);

    if ( hr != S_OK )
        return hr;

    if ( lTempState & STATE_SYSTEM_INVISIBLE )
    {
        //--------------------------------------------------
        // If the parent (MAP) is offscreen, then the AREA
        // is offscreen.
        //--------------------------------------------------

        m_bOffScreen = TRUE;
    }
    else
    {
        //--------------------------------------------------
        // The parent is onscreen, but the AREA may still be
        // offscreen.  Compare the calculate location of the
        // AREA with that of the document.
        //--------------------------------------------------

        long lDocLeft    =0;
        long lDocTop     =0;
        long lDocWidth   =0;
        long lDocHeight  =0;

        hr = m_pDocAO->AccLocation( &lDocLeft,
                                    &lDocTop,
                                    &lDocWidth,
                                    &lDocHeight,
                                    CHILDID_SELF);

        if ( hr != S_OK )
            return hr;

        if ( (xLeft > lDocLeft + lDocWidth)  ||  // off the right
             (yTop  > lDocTop + lDocHeight)  ||  // off the bottom
             (xLeft + cxWidth  < lDocLeft)   ||  // off to the left
             (yTop  + cyHeight < lDocTop)  )     // off to the top
        {
            m_bOffScreen = TRUE;
        }
    }


	//--------------------------------------------------
	// Set out parameters with the calculated location.
	//--------------------------------------------------

	*pxLeft		= xLeft;
	*pyTop		= yTop;
	*pcxWidth	= cxWidth;
	*pcyHeight	= cyHeight;


	return(S_OK);
}


//-----------------------------------------------------------------------
//	CAreaAO::AccSelect()
//
//	DESCRIPTION:
//		Selects specified object: selection based on flags.  
//
//		NOTE: only the SELFLAG_TAKEFOCUS flag is supported.
//	
//	PARAMETERS:
//
//		flagsSel	selection flags : 
//
//			SELFLAG_NONE            = 0,
//			SELFLAG_TAKEFOCUS       = 1,
//			SELFLAG_TAKESELECTION   = 2,
//			SELFLAG_EXTENDSELECTION = 4,
//			SELFLAG_ADDSELECTION    = 8,
//			SELFLAG_REMOVESELECTION = 16
//
//		lChild		child /self ID 
//
//
//	RETURNS:
//
//		HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CAreaAO::AccSelect(long flagsSel, long lChild)
{
	HRESULT hr = S_OK;


	//--------------------------------------------------
	// only SELFLAG_TAKEFOCUS is supported.
	//--------------------------------------------------

	if ( !(flagsSel & SELFLAG_TAKEFOCUS) )
		return E_INVALIDARG;


	//--------------------------------------------------
	// set focus
	//--------------------------------------------------

	CComQIPtr<IHTMLAreaElement,&IID_IHTMLAreaElement> pIHTMLAreaElement(m_pTOMObjIUnk);

	if ( !pIHTMLAreaElement )
		hr = E_NOINTERFACE;
	else
		hr = pIHTMLAreaElement->focus();

	return hr;

}



//================================================================================
// CAreaAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CAreaAO::focus()
//
//	DESCRIPTION:
//		Sets the focus to the Trident AREA.
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		S_OK | E_NOINTERFACE | standard COM error
//-----------------------------------------------------------------------

HRESULT CAreaAO::focus( void )
{
	CComQIPtr<IHTMLAreaElement,&IID_IHTMLAreaElement> pIHTMLAreaElement(m_pTOMObjIUnk);

	if ( !pIHTMLAreaElement )
		return E_NOINTERFACE;

	return pIHTMLAreaElement->focus();
}


//----  End of AREA.CPP  ----
