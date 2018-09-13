//=======================================================================
//		File:	DOCUMENT.CPP
//		Date: 	5/23/97
//		Desc:	contains implementation of CDocumentAO class.  CDocumentAO 
//				implements the Document accessible object for the Trident 
//				MSAA Registered Handler.
//		
//		Notes:	The DocumentAO class maps to to the TOM Document 
//				object. Document objects can exist at multiple levels in 
//				the AOM hierarchy.  They are responsible for
//				maintaining the AOM tree of contained AEs and AOs.
//=======================================================================


//=======================================================================
// includes
//=======================================================================

#include "stdafx.h"
#include "focusdef.h"
#include "trid_ae.h"
#include "trid_ao.h"
#include "window.h"
#include "prxymgr.h"
#include "document.h"

//================================================================================
// event map implementations
//================================================================================

#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation for EMBED type plugin event handler
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CDocumentAO,ImplIHTMLDocumentEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE,EVENT_SYSTEM_ALERT)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLDOCUMENTEVENTS_ONSELECTSTART,EVENT_OBJECT_SELECTION)		 

END_EVENT_HANDLER_MAP()

//================================================================================
// event map implementation for APPLET or OBJECT type plugin event handler
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CDocumentAO,ImplIDispIHTMLBodyElement,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_IHTMLBODYELEMENT_ONLOAD,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_IHTMLBODYELEMENT_ONUNLOAD,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_IHTMLCONTROLELEMENT_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_IHTMLCONTROLELEMENT_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_IHTMLCONTROLELEMENT_ONBLUR,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_IHTMLCONTROLELEMENT_ONRESIZE,EVENT_OBJECT_STATECHANGE)
	
END_EVENT_HANDLER_MAP()


#endif


//=======================================================================
// CDocumentAO class implementation : public methods
//=======================================================================

//-----------------------------------------------------------------------
//	CDocumentAO::CDocumentAO()
//
//	DESCRIPTION:
//
//		constructor
//
//	PARAMETERS:
//
//		pAOParent	[in]	pointer to the parent accessible object in 
//							the AOM tree
//
//		nTOMIndex	[in]	index of the element from the TOM document.all 
//							collection.
//		
//		nChildID	[in]	child id
//
//		hWnd		[in]	pointer to the window of the trident object that 
//							this object corresponds to.
//
//	RETURNS:
//
//		None.
// ----------------------------------------------------------------------

CDocumentAO::CDocumentAO(CTridentAO * pAOParent,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,this,nTOMIndex,nChildID,hWnd)
{
	//--------------------------------------------------
	// set the containing AO to point to this class, so
	// that QI delegation works
	//--------------------------------------------------

	m_pIUnknown = this;

	//--------------------------------------------------
	// set the default role
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_PANE;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_DOCUMENT;

	//--------------------------------------------------
	// initialize the selection intersection detection
	// helper objects' pointers
	//--------------------------------------------------

	m_pSelectionObj = NULL;
	m_pTextRangeTEO = NULL;

    m_bTreeReadyToDetach = FALSE;

	//--------------------------------------------------
	// Set flag for caching the readyState of the
	//	TOM document.
	//--------------------------------------------------

	m_bTOMDocComplete = FALSE;

	//--------------------------------------------------
	// Set state flags for handling the currently
	//   focused object on the document.
	//--------------------------------------------------

	m_lFocusedTOMObj = NO_TRIDENT_FOCUSED;
	m_bReceivedFocus = FALSE;			

	//--------------------------------------------------
	// cached interfaces around for lifetime of this 
	// obj.
	//--------------------------------------------------

	m_pIHTMLDocument2		= NULL;
	m_pDocIHTMLElement		= NULL;
	m_pIHTMLTextContainer	= NULL;

#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("DocumentAO"));

#endif

}


//-----------------------------------------------------------------------
//	CDocumentAO::~CDocumentAO()
//
//	DESCRIPTION:
//
//		Destructor.
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		None.
// ----------------------------------------------------------------------

CDocumentAO::~CDocumentAO()
{
	//--------------------------------------------------
	// release the TOM document IUnknown*
	//--------------------------------------------------

	if ( m_pTOMObjIUnk )
    {
		m_pTOMObjIUnk->Release();
        m_pTOMObjIUnk = NULL;
    }

	//--------------------------------------------------
	// free all cached interfaces
	//--------------------------------------------------
	
	if ( m_pSelectionObj )
    {
		m_pSelectionObj->Release();
        m_pSelectionObj = NULL;
    }


	if ( m_pTextRangeTEO )
    {
		m_pTextRangeTEO->Release();
        m_pTextRangeTEO = NULL;
    }

	if(m_pIHTMLDocument2)
	{
		m_pIHTMLDocument2->Release();
		m_pIHTMLDocument2 = NULL;
	}

	if(m_pDocIHTMLElement)
	{
		m_pDocIHTMLElement->Release();
		m_pDocIHTMLElement = NULL;
	}

	if(m_pIHTMLTextContainer)
	{
		m_pIHTMLTextContainer->Release();
		m_pIHTMLTextContainer = NULL;
	}


}

//-----------------------------------------------------------------------
//	CDocumentAO::Init()
//
//	DESCRIPTION:
//
//		initializes CDocument class elements that require post construction
//		initialization.
//
//	PARAMETERS:
//		
//		hwndToProxy		[in]	window to proxy
//		lSourceIndex	[in]	source index of window.
//		pITOMObjUnk		[in]	pointer to TOM object unknown
//
//	RETURNS:
//
//		HRESULT S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CDocumentAO::Init(HWND hWndToProxy, long lSourceIndex,IUnknown *pTOMObjIUnk)
{
	HRESULT hr = E_FAIL;

	if(!hWndToProxy)
		return(E_INVALIDARG);

	if(!pTOMObjIUnk)
		return(E_INVALIDARG);


	//--------------------------------------------------
	// set internal HWND and sourceIndex to correct
	// values.
	//--------------------------------------------------

	m_hWnd = hWndToProxy;

	m_nTOMIndex	= lSourceIndex;


	//--------------------------------------------------
	// rest of init is in overridden method
	//--------------------------------------------------

	return(Init(pTOMObjIUnk));
}


//-----------------------------------------------------------------------
//	CDocumentAO::Init()
//
//	DESCRIPTION:
//
//		initializes CDocument class elements that require post construction
//		initialization.
//
//	PARAMETERS:
//
//		pITOMObjUnk		[in]	pointer to TOM object unknown
//
//	RETURNS:
//
//		HRESULT S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CDocumentAO::Init( IUnknown* pTOMObjIUnk )
{
	HRESULT hr;


	if ( !pTOMObjIUnk )
		return E_INVALIDARG;


	//--------------------------------------------------
	// QI on passed in pointer for base Trident object
	// interface from which all other interfaces will
	// be QI'd.
	//--------------------------------------------------

	if(hr = pTOMObjIUnk->QueryInterface( IID_IHTMLDocument2, (void**) &m_pIHTMLDocument2 ))
		return(hr);

	//--------------------------------------------------
	// set IUnk of Document object to the 
	// IHTMLDocument2 interface.  Addref() to lock 
	// it down for the lifetime of this object.
	//--------------------------------------------------

	m_pTOMObjIUnk = m_pIHTMLDocument2;

	m_pTOMObjIUnk ->AddRef();

	//--------------------------------------------------
	// Determine if the TOM document.readyState is set
	// to "complete".  Cache this setting.
	//--------------------------------------------------

	if ( hr = getReadyState( m_pIHTMLDocument2, &m_bTOMDocComplete ) )
		return hr;

	//--------------------------------------------------
	// Since this Init doesn't call CTridentAO::Init(),
	// it must call createInterfaceImplementors() to
	// fully initialize the object.
	//--------------------------------------------------

	if(hr = createInterfaceImplementors())
		return(hr);
	
	//--------------------------------------------------
	// get pointer to body interface : this pointer
	// is around for the lifetime of the document obj.
	// Addref() is implicit.
	//--------------------------------------------------

	hr = m_pIHTMLDocument2->get_body(&m_pDocIHTMLElement);
    if (!m_pDocIHTMLElement)
        hr = S_FALSE;

    if (hr)
		return(hr);

	//--------------------------------------------------
	// the document may/may not be a text container.
	// keep this interface around if it is.
	//--------------------------------------------------

	if(hr = m_pDocIHTMLElement->QueryInterface(IID_IHTMLTextContainer,(void **)&m_pIHTMLTextContainer))
	{
		if(hr != E_NOINTERFACE)
			return(hr);
		else
			hr = S_OK;
	}



#ifdef _MSAA_EVENTS

	//--------------------------------------------------
	//	Only initialize the event handlers if the
	//	document was successfully initialized.
	//--------------------------------------------------

	
	hr = initDocumentEventHandlers();

	//--------------------------------------------------
	//	Event initialization errors are not propagated
	//	to the caller of CDocumentAO::Init().  This
	//	allows the document object to be created, it
	//	just won't support events.
	//	(In debug mode, if an error occurs, an assertion
	//	will fail and an error message will be sent to
	//	the output window.)
	//--------------------------------------------------

	assert( hr == S_OK );

#ifdef _DEBUG

	//--------------------------------------------------
	//	If we are in debug mode and an initialization
	//	error occurred, pump a message to the output
	//	window.
	//--------------------------------------------------

	if ( hr != S_OK )
		OutputDebugString( "Event handler initialization in CDocumentAO::Init() failed.\n" );

#endif	// _DEBUG


#endif	// _MSAA_EVENTS


	return hr;
}


//-----------------------------------------------------------------------
//  CDocumentAO::ReleaseTridentInterfaces()
//
//  DESCRIPTION: Calls release on all CDocumentAO-specific cached Trident
//               object interface pointers.  Also calls the base class
//               ReleaseTridentInterfaces().
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void CDocumentAO::ReleaseTridentInterfaces ()
{
    if ( m_pSelectionObj )
    {
        m_pSelectionObj->Release();
        m_pSelectionObj = NULL;
    }

    if ( m_pTextRangeTEO )
    {
        m_pTextRangeTEO->Release();
        m_pTextRangeTEO = NULL;
    }

    if ( m_pIHTMLDocument2 )
    {
        m_pIHTMLDocument2->Release();
        m_pIHTMLDocument2 = NULL;
    }

    if ( m_pDocIHTMLElement )
    {
        m_pDocIHTMLElement->Release();
        m_pDocIHTMLElement = NULL;
    }

    if ( m_pIHTMLTextContainer )
    {
        m_pIHTMLTextContainer->Release();
        m_pIHTMLTextContainer = NULL;
    }

    CTridentAO::ReleaseTridentInterfaces();
}


//=======================================================================
// CDocumentAO class implementation : IUnknown methods
//=======================================================================

//-----------------------------------------------------------------------
//	CDocumentAO::QueryInterface()
//
//	DESCRIPTION:
//
//	Standard QI : this object needs to implement IUnknown because it is the 
//	child class, and also because it implements an event interface.
//
//	PARAMETERS:
//
//		pITOMObjUnk	[out]	pointer to TOM object unknown
//
//	RETURNS:
//
//		HRESULT E_NOERROR | E_NOINTERFACE
// ----------------------------------------------------------------------

STDMETHODIMP CDocumentAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return E_NOINTERFACE;


	*ppv = NULL;


	if (riid == IID_IUnknown)
	{
		*ppv = (IUnknown *)this;
	}

#ifdef _MSAA_EVENTS

	else if (riid == DIID_HTMLDocumentEvents) 
	{
		//--------------------------------------------------
		// event interface for TOM document object
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLDocumentEvents,ppv,HTMLDocumentEvents)

	}
	else if (riid == DIID_DispIHTMLBodyElement) 
	{
		//--------------------------------------------------
		// event interface for document BODY element object
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIDispIHTMLBodyElement,ppv,DispIHTMLBodyElement)
	}

#endif

	else
	{
		//--------------------------------------------------
		// return to base class for further interface 
		// checking.
		//--------------------------------------------------

		return CTridentAO::QueryInterface( riid, ppv );
	}

	//--------------------------------------------------
	// *ppv should be set.
	//--------------------------------------------------

	if ( !*ppv )
		return E_NOINTERFACE;


	((IUnknown *)*ppv)->AddRef();

	return NOERROR;
}


//=======================================================================
// CDocumentAO class implementation : IAccessible support methods
//=======================================================================

//-----------------------------------------------------------------------
//	CDocumentAO::GetAccName()
//
//	DESCRIPTION:
//
//		Returns name of object/element, which is the doc's title.
//
//	PARAMETERS:
//
//		lChild		[in]	child ID / Self ID
//		pbstrName	[out]	returned name.
//		
//	RETURNS:
//
//		HRESULT		S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------
	
HRESULT	CDocumentAO::GetAccName(long lChild, BSTR* pbstrName )
{
	HRESULT	hr = S_OK;


	assert( pbstrName );


	if ( m_bstrName )
	{
		*pbstrName = SysAllocString( m_bstrName );
	}
	else
	{
		if ( m_pIHTMLDocument2 )
		{
			HRESULT hr = m_pIHTMLDocument2->get_title( pbstrName );

			if ( hr == S_OK && *pbstrName )
			{
				m_bstrName = SysAllocString( *pbstrName );
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
		}
		else
		{	

			//--------------------------------------------------
			// init should have failed already if no 
			// IHTMLDocument2 pointer.  Alert user here also.
			//--------------------------------------------------

			hr = E_FAIL;
		}
	}

	return hr;
}


//-----------------------------------------------------------------------
//	CDocumentAO::AccLocation()
//
//	DESCRIPTION:
//
//		Returns object location to client. Since a TOM document object
//		(IHTMLDocument2) is not	an IHTMLElement, we need to retrieve
//		and use the document's body object, which is an IHTMLElement, to
//		get the doc's coordinates.
//
//	PARAMETERS:
//
//		pxLeft		[out]	pointer to long containing left coord
//		pyTop		[out]	pointer to long containing top coord
//		pcxWidth	[out]	pointer to long containing width
//		pcyHeight	[out]	pointer to long containing height
//		varChild	[in]	VARIANT containg child ID
//
//	RETURNS:
//
//		HRESULT		S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CDocumentAO::AccLocation(long* pxLeft,long* pyTop,long* pcxWidth,long* pcyHeight,long lChild)
{
	HRESULT		hr;
	POINT		pt;
	long		lWidth = 0;
	long		lHeight = 0;


	assert( pxLeft );
	assert( pyTop );
	assert( pcxWidth );
	assert( pcyHeight );


	//------------------------------------------------
	// Initialize the out parameters.
	//------------------------------------------------

	*pxLeft		= 0;
	*pyTop		= 0;
	*pcxWidth	= 0;
	*pcyHeight	= 0;


	//------------------------------------------------
	// The (left,top) of TOM document is always (0,0)
	// according to the Trident dev team.
	//------------------------------------------------

	pt.x = 0;
	pt.y = 0;

	//------------------------------------------------
	// Get the body object and its coordinate offsets:
	//------------------------------------------------

	if ( !m_pDocIHTMLElement )
		hr = E_NOINTERFACE;
	else
	{
		hr = m_pDocIHTMLElement->get_offsetWidth( &lWidth );

		if ( hr == S_OK )
		{
			hr = m_pDocIHTMLElement->get_offsetHeight( &lHeight );

			if ( hr == S_OK )
			{
				assert( lWidth > 0 );
				assert( lHeight > 0 );

				if ( lWidth <= 0 || lHeight <= 0 )
					hr = E_FAIL;
			}
		}
	}
	

	if ( hr == S_OK )
	{
		//--------------------------------------------------
		// The origin coords need to be translated to
		// screen coords from document client coords.
		// Translate the top and left coords only.
		//--------------------------------------------------

		if ( m_hWnd )
			::ClientToScreen( m_hWnd, &pt );
	
		*pxLeft		= pt.x;
		*pyTop		= pt.y;
		*pcxWidth	= lWidth;
		*pcyHeight	= lHeight;
	}


	return hr;
}


//-----------------------------------------------------------------------
//	CDocumentAO::AccSelect()
//
//	DESCRIPTION:
//
//		Selects specified object based on the specified selection flags.
//		NOTE: we only support the SELFLAG_TAKEFOCUS flag.	
//	
//	PARAMETERS:
//
//		flagsSel	[in]	selection flags : 
//
//			SELFLAG_NONE            = 0,
//			SELFLAG_TAKEFOCUS       = 1,
//			SELFLAG_TAKESELECTION   = 2,
//			SELFLAG_EXTENDSELECTION = 4,
//			SELFLAG_ADDSELECTION    = 8,
//			SELFLAG_REMOVESELECTION = 16
//
//		lChild		[in]	child ID 
//
//	RETURNS:
//
//		HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CDocumentAO::AccSelect(long flagsSel, long lChild)
{
	HRESULT	hr;


	if ( flagsSel != SELFLAG_TAKEFOCUS )
		return E_INVALIDARG;

	//------------------------------------------------
	// Retrieve and empty the current selection
	//------------------------------------------------

	CComPtr<IHTMLSelectionObject> pIHTMLSelectionObject;

	assert(m_pIHTMLDocument2);

	if(!m_pIHTMLDocument2)
		return(E_NOINTERFACE);

	hr = m_pIHTMLDocument2->get_selection( &pIHTMLSelectionObject );

	if ( hr != S_OK )
		return hr;
	if ( !pIHTMLSelectionObject )
		return E_NOINTERFACE;

	hr = pIHTMLSelectionObject->empty();

	if ( hr != S_OK )
		return hr;


	//------------------------------------------------
	// Set the focus to the document
	//------------------------------------------------

	return Focus( m_pIHTMLDocument2 );
}


//-----------------------------------------------------------------------
//	CDocumentAO::GetAccState()
//
//	DESCRIPTION:
//
//		Returns object state to client.
//
//	PARAMETERS:
//
//		lChild		[in]	CHILDID of child to get state of
//		plState		[out]	pointer to store returned state in.
//
//	RETURNS:
//
//		HRESULT		S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CDocumentAO::GetAccState(long lChild, long *plState)
{
	HRESULT	hr = S_OK;


	assert( plState );


	//--------------------------------------------------
	//	Initialize the out parameter
	//--------------------------------------------------

	*plState = 0;


	//--------------------------------------------------
	//	First check if we're in a ready state
	//--------------------------------------------------

	assert(m_pIHTMLDocument2);

	if ( !m_pIHTMLDocument2 )
		return E_NOINTERFACE;

	if ( m_bTOMDocComplete )
	{
		*plState = STATE_SYSTEM_READONLY;
	}
	else
	{
		*plState = STATE_SYSTEM_UNAVAILABLE;
		return S_OK;
	}


	//--------------------------------------------------
	//	Determine the focus state.
	//--------------------------------------------------

	UINT nTOMIndex = 0;

	hr = GetFocusedItem( &nTOMIndex );

	if ( hr == S_OK && nTOMIndex > 0 )
    {
        *plState |= STATE_SYSTEM_FOCUSABLE;


        if ( m_pDocIHTMLElement )
        {
            long lIndex = 0;

            hr = m_pDocIHTMLElement->get_sourceIndex( &lIndex );

            if ( (hr == S_OK) && (nTOMIndex == lIndex) )
                *plState |= STATE_SYSTEM_FOCUSED;
            else if ( hr != S_OK )
                *plState = STATE_SYSTEM_UNAVAILABLE;
		}
    }
    else if ( hr == S_FALSE )
        hr = S_OK;


	return hr;
}



//-----------------------------------------------------------------------
//	CDocumentAO::GetAccSelection()
//
//	DESCRIPTION:
//
//		returns the id or the IDispatch * to the selected object (if
//		single selection), or the IUnknown * to the	IEnumVariant object
//		that contains the VARIANTs of the selected objects.
//			
//		
//
//	PARAMETERS:
//		
//		ppIUnknown	pointer to IUnknown of CAccElement, CAccObject,
//					or IEnumVariant.  QI on this interface to find out what
//					type of object was selected.
//
//	RETURNS:
//
//		HRESULT		S_OK | S_FALSE | E_FAIL | E_INVALIDARG 
//
//					(S_FALSE => nothing selected)
//
//-----------------------------------------------------------------------

HRESULT CDocumentAO::GetAccSelection( IUnknown** ppIUnknown )
{
	HRESULT hr = E_FAIL;
	BOOL	bHasSelection = FALSE;


	assert(ppIUnknown);

	
	*ppIUnknown = NULL;

	
	//--------------------------------------------------
	//	Tree must be fully built prior to getting the 
	//	selected objects.
	//--------------------------------------------------

	if ( hr = ensureResolvedTree() )
		return hr;


	//--------------------------------------------------
	//	Is the TOM document actually a FRAMESET?
	//	If it is, it won't support selection.
	//--------------------------------------------------

	if(!m_pDocIHTMLElement)
		return(E_NOINTERFACE);

	CComQIPtr<IHTMLBodyElement,&IID_IHTMLBodyElement> pIHTMLBodyElement(m_pDocIHTMLElement);

	//--------------------------------------------------
	//	If there is no body element, that means that
	//	the body is actually a FRAMESET.
	//--------------------------------------------------

	if ( !pIHTMLBodyElement )
		return DISP_E_MEMBERNOTFOUND;


	//--------------------------------------------------
	//	Is the TOM document's selection empty?
	//	If it is, return S_FALSE because none of the
	//	CDocumentAO's children will be selected.
	//--------------------------------------------------

	hr = IsTOMSelectionNonEmpty( &bHasSelection );

	if ( hr != S_OK )
		return hr;

	if ( !bHasSelection )
		return S_FALSE;


	//------------------------------------------------
	//	Something on the BODY is selected.
	//	Does this CDocumentAO have any children?
	//------------------------------------------------

	if ( !m_AEList.size() )
	{
		//------------------------------------------------
		//	The CDocumentAO has no children (!?!) so it
		//	alone is selected.
		//------------------------------------------------

		*ppIUnknown = (LPUNKNOWN)this;
	}
	else
	{
		//------------------------------------------------
		//	The CDocumentAO has children, so one or more
		//	of them are selected.
		//------------------------------------------------

		hr = getSelectedChildren( ppIUnknown );
	}

	return hr;
}

//-----------------------------------------------------------------------
//	CDocumentAO::GetAccValue()
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
//		HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CDocumentAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
	return( GetURL( pbstrValue ) );
}


//=======================================================================
// CDocumentAO class implementation : TOM interaction helper methods
//=======================================================================

//-----------------------------------------------------------------------
//	CDocumentAO::IsTEOSelected()
//
//	DESCRIPTION:
//
//		Determines if the text range of the TEO intersects the current
//		TOM selection.
//
//	PARAMETERS:
//
//		pIHTMLElement	[in]	pointer to the TEO
//	
//		pbIsSelected	[out]	pointer to a BOOL that will hold whether
//								or not the text range of pIHTMLElement
//								intersects the current TOM selection
//
//	RETURNS:
//
//		HRESULT:		return from IsTextRangeSelected()
//							| E_NOINTERFACE
//							| standard COM error
//
//-----------------------------------------------------------------------

HRESULT	CDocumentAO::IsTEOSelected( IHTMLElement* pIHTMLElement, BOOL* pbIsSelected )
{
	HRESULT	hr = S_OK;


	assert( pIHTMLElement );
	assert( pbIsSelected );


	//--------------------------------------------------
	//	If this is the first time this method has been
	//	called, create a text range object that will
	//	used to get the text range of the TEO.
	//--------------------------------------------------

	if ( !m_pTextRangeTEO )
	{
		assert(m_pDocIHTMLElement);

		if(!m_pDocIHTMLElement)
			return(E_NOINTERFACE);

		//--------------------------------------------------
		//	Get the IHTMLBodyElement* of the BODY
		//--------------------------------------------------

		CComQIPtr<IHTMLBodyElement,&IID_IHTMLBodyElement> pBodyElem( m_pDocIHTMLElement );

 
		if ( !pBodyElem )
			return E_NOINTERFACE;

		//--------------------------------------------------
		//	Create a text range off the BODY
		//--------------------------------------------------

		hr = pBodyElem->createTextRange( &m_pTextRangeTEO );
		if ( hr != S_OK )
			return hr;
		if ( !m_pTextRangeTEO )
			return E_NOINTERFACE;
	}

	//--------------------------------------------------
	//	Move our member text range object to scope just
	//	the text & HTML of the TEO.
	//--------------------------------------------------

	hr = m_pTextRangeTEO->moveToElementText( pIHTMLElement );
	if ( hr != S_OK )
		return hr;

	//--------------------------------------------------
	//	Delegate selection intersection detection to
	//	sister method IsTextRangeSelected()
	//--------------------------------------------------

	return IsTextRangeSelected( m_pTextRangeTEO, pbIsSelected );
}



//-----------------------------------------------------------------------
//	CDocumentAO::IsTextRangeSelected()
//
//	DESCRIPTION:
//
//		Determines if the current text range TEO intersects the current
//		TOM selection.
//
//	PARAMETERS:
//
//		pIHTMLTxtRange	[in]	pointer to the text range in question
//	
//		pbIsSelected	[out]	pointer to a BOOL that will hold whether
//								or not the text range intersects the
//								current TOM selection
//
//	RETURNS:
//
//		HRESULT:		S_OK | E_NOINTERFACE | standard COM error
//
//	TODO:
//
//		This method can be optimized by only getting a new selection
//		text range when we've been notified that the selection has
//		changed.
//
//-----------------------------------------------------------------------

HRESULT	CDocumentAO::IsTextRangeSelected( IHTMLTxtRange* pIHTMLTxtRange, BOOL* pbIsSelected )
{
	HRESULT	hr = S_OK;
	BSTR	bstrHow;


	assert( pIHTMLTxtRange );
	assert( pbIsSelected );


	*pbIsSelected = FALSE;


	//--------------------------------------------------
	//	Is the TOM document's selection empty?
	//	If it is, there no way this text range can be
	//	selected.
	//--------------------------------------------------

	hr = IsTOMSelectionNonEmpty( pbIsSelected );

	if ( hr != S_OK || !*pbIsSelected )
		return hr;


	//------------------------------------------------
	//	Obtain the selection text range.
	//------------------------------------------------

    CComPtr<IHTMLTxtRange> pIHTMLTxtRangeSel;

    hr = getSelectionTextRange( &pIHTMLTxtRangeSel );

    if ( hr != S_OK )
        return hr;


	//------------------------------------------------
	//	Compare the end points of the selection range
	//	to the pIHTMLTxtRange.  If the start of
	//	pIHTMLTxtRange is less than the end of the
	//	selection range *AND* the end of
	//	pIHTMLTxtRange is greater than the start of
	//	the selection range, then pIHTMLTxtRange is
	//	considered selected.
	//------------------------------------------------

	long lCompare;

	// TODO: make this strings global to alloc once

	bstrHow = SysAllocString( L"STARTTOEND" );

	hr = pIHTMLTxtRange->compareEndPoints( bstrHow, pIHTMLTxtRangeSel, &lCompare );

	SysFreeString( bstrHow );

	if ( hr != S_OK ) 
		return( hr );
	else if ( lCompare != -1 )
	{
		*pbIsSelected = FALSE;
		return hr;
	}

	// TODO: make this string global to alloc once

	bstrHow = SysAllocString( L"ENDTOSTART" );

	hr = pIHTMLTxtRange->compareEndPoints( bstrHow, pIHTMLTxtRangeSel, &lCompare );

	SysFreeString( bstrHow );

	if ( hr != S_OK ) 
		return( hr );

	*pbIsSelected = (lCompare == 1);

	return hr;
}



//-----------------------------------------------------------------------
//	CDocumentAO::IsTOMSelectionNonEmpty()
//
//	DESCRIPTION:
//
//		Determines if the selection object of the BODY associated with
//		this CDocumentAO is non-empty.
//
//	PARAMETERS:
//
//		pbHasSelection	[out]	pointer to a BOOL
//
//	RETURNS:
//
//		HRESULT:		S_OK | E_NOINTERFACE | standard COM error
//
//	NOTES:
//
//		This method will initialize the CDocumentAO's selection object
//		pointer.
//
//-----------------------------------------------------------------------

HRESULT	CDocumentAO::IsTOMSelectionNonEmpty( BOOL* pbHasSelection )
{
	HRESULT	hr = S_OK;


	assert( pbHasSelection );


	*pbHasSelection = FALSE;


    //--------------------------------------------------
    //	Get the IHTMLSelectionObject*
    //--------------------------------------------------

    hr = getIHTMLSelectionObject();

    if ( hr != S_OK )
        return hr;


	//------------------------------------------------
	//	Is anything selected?  The selection type is
	//	0 if nothing is selected, else it's 1.
	//------------------------------------------------

	long	lSelectType = 0;

	hr = callInvokeForLong( (IDispatch*)m_pSelectionObj,
	                        DISPID_IHTMLSELECTIONOBJECT_TYPE,
	                        &lSelectType );

	if ( hr != S_OK )
	{
		lSelectType = 0;

		BSTR	bstrType;

		hr = m_pSelectionObj->get_type( &bstrType );

		if ( hr == S_OK )
		{
			if ( bstrType )
			{
				if ( _wcsicmp(bstrType, L"none") )
					lSelectType = 1;

				SysFreeString( bstrType );
			}
		}
	}

	if ( hr == S_OK && lSelectType )
	{
		*pbHasSelection = TRUE;
	}


	return hr;
}



//-----------------------------------------------------------------------
//	CDocumentAO::Focus()
//
//	DESCRIPTION:
//
//		Sets the focus to the document.
//	
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		HRESULT: return code from Focus( IHTMLDocument2* )
//
//-----------------------------------------------------------------------

HRESULT	CDocumentAO::Focus( void )
{
	assert(m_pIHTMLDocument2);

	if(!m_pIHTMLDocument2)
		return(E_NOINTERFACE);

	return Focus( m_pIHTMLDocument2 );
}


//-----------------------------------------------------------------------
//	CDocumentAO::Focus()
//
//	DESCRIPTION:
//
//		Sets the focus to the document.
//	
//	PARAMETERS:
//
//		pIHTMLDocument2		pointer to the TOM document object
//
//	RETURNS:
//
//		HRESULT: S_OK | E_NOINTERFACE | standard COM error
//
//-----------------------------------------------------------------------

HRESULT	CDocumentAO::Focus( IHTMLDocument2* pIHTMLDocument2 )
{
	HRESULT	hr;


	assert( pIHTMLDocument2 );


	//------------------------------------------------
	// Focus the parent Window
	//------------------------------------------------

	CComPtr<IHTMLWindow2> pIHTMLWindow2;

	hr = pIHTMLDocument2->get_parentWindow( &pIHTMLWindow2 );

	if ( hr != S_OK )
		return hr;
	if ( !pIHTMLWindow2 )
		return E_NOINTERFACE;

	hr = pIHTMLWindow2->focus();

	if ( hr != S_OK )
		return hr;


	//------------------------------------------------
	// Now, focus the TOM BODY object
	//------------------------------------------------

	assert(m_pDocIHTMLElement);

	if(!m_pDocIHTMLElement)
		return E_NOINTERFACE;

	CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElementBody(m_pDocIHTMLElement);

	if ( !pIHTMLControlElementBody )
		return E_NOINTERFACE;

	hr = pIHTMLControlElementBody->focus();


	return hr;
}



//----------------------------------------------------------------------- 
//	CDocumentAO::GetFocusedItem()
//
//	DESCRIPTION:
//
//		Get the object that has the current focus, and return its 
//		source index to the user.  
//
//	PARAMETERS:
//
//		pnTOMIndex  [out] source index of object with current focus.
//
//	RETURNS:
//
//		S_OK if an item is focused
//		S_FALSE if no item is focused
//		COM error if an error occurred.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::GetFocusedItem(UINT *pnTOMIndex)
{
    HRESULT     hr;
    BOOL        bBrowserWindowHasFocus;
    BOOL        bParentWindowHasFocus;


    assert( pnTOMIndex );


    //--------------------------------------------------
    // Detect if the m_hWnd proxied by this
    // document's parent is currently focused. 
    //--------------------------------------------------

    hr = ((CWindowAO *)m_pParent)->IsFocused( &bBrowserWindowHasFocus, &bParentWindowHasFocus );
	
    if ( hr != S_OK )
        return hr;


    if ( !bBrowserWindowHasFocus )
        return S_FALSE;
    else
    {
        *pnTOMIndex = ALL_OBJECTS_FOCUSABLE;

        if ( !bParentWindowHasFocus )
            return hr;
    }
	
	//------------------------------------------------------
	//  If we've already had the focus at some point
	//  in our lifetime, then simply return the ID of the
	//	currently focused object that's updated via  MSAA
	//	focus/statechange event hooks.
	//
	//	BUGBUG
	//
	//	If this is the first time we've received the focus,
	//	then we need to do special focus detection because
	//	it's possible an object on the document already
	//	has the focus, but we weren't in existence to catch
	//	the MSAA focus/statechange event that was fired.
	//	Getting the document's active element will tell us
	//	who has the focus, EXCEPT FOR ANCHORS. This is a known
	//	limitation in Trident and a resultant caveat of the proxy.
	//------------------------------------------------------

	if (m_bReceivedFocus)
	{
#ifdef _DEBUG

		TCHAR  szBuf[80];

		wsprintf( szBuf, _T("CDocumentAO::GetFocusedItem(), ObjID = %ld\n"), m_lFocusedTOMObj );

		OutputDebugString ( szBuf );

#endif

		//-----------------------------------------------
		// Return the ID of the focused object
		//-----------------------------------------------

		if ( m_lFocusedTOMObj != NO_TRIDENT_FOCUSED )
			*pnTOMIndex = m_lFocusedTOMObj;


		return hr;
	}
	else
	{
		//------------------------------------------------
		// The document's active element has the focus,
		//	so first get the document.
		//------------------------------------------------

		assert(m_pIHTMLDocument2);

		if ( !m_pIHTMLDocument2 )
			return E_NOINTERFACE;

		//------------------------------------------------
		// If we can't get an active element or its source
		//  index, we assume no element is currently focused
		//------------------------------------------------

        UINT    nTmpIdx = 0;

        hr = GetActiveElementIndex( &nTmpIdx );

        if ( hr == S_OK )
            *pnTOMIndex = nTmpIdx;
        else if ( hr == S_FALSE )
            hr = S_OK;

        return hr;
	}
}


//-----------------------------------------------------------------------
//	CDocumentAO::GetActiveElementIndex()
//
//	DESCRIPTION:
//	returns the current active element (regardless of whether
//  Trident has the focus or not.
//
//  PARAMETERS:
//
//	pnTOMIndex	pointer to long to store active element index in.
//
//	RETURNS:
//
//	S_OK if good active element, else S_FALSE if no active element,
//  else std COM error.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::GetActiveElementIndex(UINT * pnTOMIndex)
{
    HRESULT hr = E_FAIL;
    CComPtr<IHTMLElement> pIHTMLElement;


    assert( pnTOMIndex );


    assert( m_pIHTMLDocument2 );

    if ( !m_pIHTMLDocument2 )
        return E_FAIL;


    hr = m_pIHTMLDocument2->get_activeElement( &pIHTMLElement );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    // IE4.01 Trident link (A--anchor) behavior:
    //
    //  Focused links don't show up as the document's
    //  active element.  If a link is focused, one of
    //  its containing ancestors is the active element.
    //  Usually this is the BODY, but maybe a TD.
    //  If the focused link is resides in overlapping
    //  HTML, the active element may be NULL.
    //
    //  Nonetheless, if focused, the link shows up as
    //  current document selection.  So, if an
    //  *unexpected* element is active, query the
    //  selection text range to see if it's parented
    //  by a link.  If so, the link is focused.  If not,
    //  and the BODY is active, the BODY is focused.
    //  Otherwise, return no element focused (S_FALSE).
    //--------------------------------------------------

    CComQIPtr<IHTMLBodyElement,&IID_IHTMLBodyElement> pIHTMLBodyElement( pIHTMLElement );

    CComQIPtr<IHTMLTableCell,&IID_IHTMLTableCell> pIHTMLTableCell( pIHTMLElement );

    if ( pIHTMLBodyElement || pIHTMLTableCell || !pIHTMLElement )
    {
        CComPtr<IHTMLElement> pElemSelParent;

        hr = getIHTMLElementParentOfCurrentSelection( &pElemSelParent );

        if ( !SUCCEEDED( hr ) )
            return hr;


        CComQIPtr<IHTMLAnchorElement,&IID_IHTMLAnchorElement> pIHTMLAnchorElement( pElemSelParent );

        if ( pIHTMLAnchorElement )
        {
            hr = pElemSelParent->get_sourceIndex( (long *)pnTOMIndex );
        }
        else if ( pIHTMLBodyElement )
        {
            hr = pIHTMLElement->get_sourceIndex( (long *)pnTOMIndex );
        }
        else
            hr = S_FALSE;
    }
    else
    {
        hr = pIHTMLElement->get_sourceIndex( (long *)pnTOMIndex );
    }


    return hr;
}


//--------------------------------------------------------------------------------
// CDocumentAO::GetScrollOffset()
//
//	DESCRIPTION:
//
//		gets the current scroll left (x) and top (y) offsets, returns them
//		in the passed in POINT struct.
//
//
//	PARAMETERS:
//
//		pPtScrollOffset	: stores the returned scroll offsets.
//
//	RETURNS:
//
//		S_OK  | E_FAIL | E_NOINTERFACE
//--------------------------------------------------------------------------------

HRESULT CDocumentAO::GetScrollOffset( POINT* pPtScrollOffset )
{
	HRESULT hr;
	long xLeft	= 0;
	long yTop	= 0;

	//--------------------------------------------------
	// point must be valid
	//--------------------------------------------------

	assert( pPtScrollOffset );

	//--------------------------------------------------
	// initialize point.
	//--------------------------------------------------
			  
	pPtScrollOffset->x = pPtScrollOffset->y = 0;

	//--------------------------------------------------
	// if the body element doesn't implement 
	// IHTMLTextContainer, then don't get scroll offsets
	//
	// TODO: determine if S_FALSE is a more appropriate
	// return code.
	//--------------------------------------------------
	
	if(!(m_pIHTMLTextContainer))
	{
		pPtScrollOffset->x = 0;
		pPtScrollOffset->y = 0;
		return(S_OK);
	}


	hr = m_pIHTMLTextContainer->get_scrollLeft(&xLeft);

	if(hr != S_OK)
		return(hr);

	hr = m_pIHTMLTextContainer->get_scrollTop(&yTop);

	if(hr != S_OK)
		return(hr);

	pPtScrollOffset->x = xLeft;
	pPtScrollOffset->y = yTop;

	return(hr);

}



//-----------------------------------------------------------------------
//	CDocumentAO::NotifySoundElementExist()
//
//	DESCRIPTION:
//
//		Fires an EVENT_SYSTEM_SOUND to let the client know that a sound
//		object has been loaded onto the page. 
//
//	PARAMETERS:
//
//		none.
//
//	RETURNS:
//
//		none.
// ----------------------------------------------------------------------

void CDocumentAO::NotifySoundElementExist(void)
{
#ifdef _MSAA_EVENTS
	FIRE_EVENT(ImplIHTMLDocumentEvents,EVENT_SYSTEM_SOUND)
#endif
}


//-----------------------------------------------------------------------
//	CDocumentAO::ElementFromPoint()
//
//	DESCRIPTION:
//	returns the IHTMLElement pointer that is under the specified point.
//	stores the specified point.
//
//	**NOTE** If it turns out that more than one hit test can be done at
//	the same time, it will be necessary to store hit test data in an array
//  and key the array with cookies.
//
//	PARAMETERS:
//	
//	pPtHitTest		point to hit test.
//	ppIHTMLElement	element pointer to return.
//
//	RETURNS:
//
//	S_OK | E_FAIL | S_FALSE (if the point was not on any element)
//
// ----------------------------------------------------------------------

HRESULT CDocumentAO::ElementFromPoint(POINT *pPtHitTest,IHTMLElement **ppIHTMLElement)
{
	HRESULT hr;

	assert(pPtHitTest);
	assert(ppIHTMLElement);

	assert(m_pIHTMLDocument2);

	if(!m_pIHTMLDocument2)
		return(E_NOINTERFACE);

	//--------------------------------------------------
	// get the element under the point.
	//--------------------------------------------------

	if(hr = m_pIHTMLDocument2->elementFromPoint(pPtHitTest->x,pPtHitTest->y,ppIHTMLElement) )
	{
		return(hr);
	}

	return(S_OK);
}




//-----------------------------------------------------------------------
//	CDocumentAO::GetAOMMgr()
//
//	DESCRIPTION:
//	returns AOMMgr from CWindowAO parent.
//
//	TODO : move this to the window.
//
//	PARAMETERS:
//	ppAOMMgr		pointer to return AOMMgr in.
//
//	RETURNS:
//
//	S_OK | E_FAIL
// ----------------------------------------------------------------------

HRESULT CDocumentAO::GetAOMMgr(CAOMMgr ** ppAOMMgr)
{
	assert(ppAOMMgr);

	return(m_pParent->GetAOMMgr(ppAOMMgr));

}


//-----------------------------------------------------------------------
//	CDocumentAO::GetURL()
//
//	DESCRIPTION:
//	get the URL associated with this document object.
//
//  PARAMETERS:
//
//	pbstrURL : pointer to BSTR  to store URL in.
//
//	RETURNS:
//
//	S_OK else std COM error.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::GetURL(BSTR * pbstrURL)
{

	if(!pbstrURL)
		return(E_INVALIDARG);

	assert(m_pIHTMLDocument2);

	if(!m_pIHTMLDocument2)
		return(E_NOINTERFACE);

	return(m_pIHTMLDocument2->get_URL(pbstrURL));
}

		

//================================================================================
// protected methods
//================================================================================

//-----------------------------------------------------------------------
//  CDocumentAO::getIHTMLSelectionObject()
//
//  DESCRIPTION:
//  Obtains an interface pointer to the TOM document's selection object.
//  If successful, this method sets the CDocumentAO's m_pSelectionObj
//  member.
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  S_OK | E_NOINTERFACE | other std COM error.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::getIHTMLSelectionObject( void )
{
    HRESULT hr = S_OK;

    if ( !m_pSelectionObj )
    {
        assert( m_pIHTMLDocument2 );

        if ( !m_pIHTMLDocument2 )
            return E_NOINTERFACE;

        hr = m_pIHTMLDocument2->get_selection( &m_pSelectionObj );

        if ( hr != S_OK )
            return hr;
        if ( !m_pSelectionObj )
            return E_NOINTERFACE;
	}

    return hr;
}


//-----------------------------------------------------------------------
//  CDocumentAO::getSelectionTextRange()
//
//  DESCRIPTION:
//  Obtains the selection text range.
//
//  This method could be optimized by retrieving a selection text range
//  only when it has changed.  Right now, a new selection text range is
//  obtained every time this method is called.
//
//  PARAMETERS:
//
//  ppIHTMLTxtRangeSel
//
//  RETURNS:
//
//  S_OK | E_NOINTERFACE | other std COM error.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::getSelectionTextRange( /* out */ IHTMLTxtRange** ppIHTMLTxtRangeSel )
{
    HRESULT hr;


    assert( ppIHTMLTxtRangeSel );
    assert( *ppIHTMLTxtRangeSel == NULL );


    hr = getIHTMLSelectionObject();

    if ( hr != S_OK )
        return hr;


    CComPtr<IDispatch> pDispSelRange;

    hr = m_pSelectionObj->createRange( &pDispSelRange );

    if ( hr != S_OK )
        return hr;

    if ( !pDispSelRange )
        return E_NOINTERFACE;


    return pDispSelRange->QueryInterface( IID_IHTMLTxtRange, (void**) ppIHTMLTxtRangeSel );
}


//-----------------------------------------------------------------------
//  CDocumentAO::getIHTMLElementParentOfCurrentSelection()
//
//  DESCRIPTION:
//  Obtains parent IHTMLElement of the selection text range.
//
//  PARAMETERS:
//
//  ppElem
//
//  RETURNS:
//
//  S_OK | S_FALSE if no parent (NULL range) | other std COM error.
// ----------------------------------------------------------------------

HRESULT CDocumentAO::getIHTMLElementParentOfCurrentSelection( /* out */ IHTMLElement** ppElem )
{
    HRESULT hr;


    assert( ppElem );
    assert( *ppElem == NULL );


    CComPtr<IHTMLTxtRange> pIHTMLTxtRangeSel;

    hr = getSelectionTextRange( &pIHTMLTxtRangeSel );

    if ( hr != S_OK )
        return hr;


    hr = pIHTMLTxtRangeSel->parentElement( ppElem );

    if ( hr == S_OK )
    {
        if ( !*ppElem )
            hr = S_FALSE;
    }


    return hr;
}


//-----------------------------------------------------------------------
//	CDocumentAO::getReadyState()
//
//	DESCRIPTION:
//		
//		Determines the "ready state" of the TOM document object and
//		sets the out parameter pbComplete accordingly.
//
//
//	PARAMETERS:
//
//		pIHTMLDocument2		[in]	pointer to the TOM document object
//
//		pbComplete			[out]	pointer to the boolean to be set to
//									TOM document.readyState == "complete"
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CDocumentAO::getReadyState( /* in */ IHTMLDocument2* pIHTMLDocument2, /* out */ BOOL* pbComplete )
{
	HRESULT		hr = E_FAIL;


	assert( pIHTMLDocument2 );
	assert( pbComplete );


	*pbComplete = FALSE;


	//--------------------------------------------------
	//	BUGBUG!  IE5 REQUIRED!
	//
	//	The following call to callInvokeForLong() has
	//	been commented because this will not succeed
	//	as desired until IE5.
	//--------------------------------------------------
/***
	long		lReadyState = 0;

	CComQIPtr<IDispatch,&IID_IDispatch> pIDispatch( pIHTMLDocument2 );

	hr = callInvokeForLong( pIDispatch, DISPID_IHTMLDOCUMENT2_READYSTATE, &lReadyState );

	if ( hr == S_OK )
	{
		*pbComplete = ( lReadyState == 4 );
	}
***/

	if ( hr != S_OK )
	{
		BSTR	bstrReadyState;

		hr = pIHTMLDocument2->get_readyState( &bstrReadyState );

		if ( hr == S_OK )
		{
			if ( bstrReadyState )
			{
				if ( !_wcsicmp(bstrReadyState, L"complete") )
					*pbComplete = TRUE;

				SysFreeString( bstrReadyState );
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
		}
	}

	return hr;
}
	


//-----------------------------------------------------------------------
//	CDocumentAO::callInvokeForLong()
//
//	DESCRIPTION:
//		
//		Calls the Invoke method of the IDispatch* passed in.
//		This method expects that the dispatch member called is a
//		property-get taking no arguments and returning a long.
//
//
//	PARAMETERS:
//
//		pDisp		[in]	pointer to an IDispatch
//
//		dispID		[in]	dispatch identified
//
//		plData		[out]	pointer to the long returned
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CDocumentAO::callInvokeForLong( IDispatch* pDisp, DISPID dispID, long* plData )
{
	HRESULT		hr;
	DISPPARAMS	dispparamsNoArgs = { NULL, NULL, 0, 0 };
	VARIANT		varResult;


	assert( pDisp );
	assert( plData );


	VariantInit( &varResult );


	hr = pDisp->Invoke( dispID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
	                    &dispparamsNoArgs, &varResult, NULL, NULL );

	if ( hr == S_OK )
	{
		if ( varResult.vt == VT_I4 )
		{
			*plData = varResult.lVal;
		}
		else
		{
			hr = DISP_E_MEMBERNOTFOUND;

			VariantClear( &varResult );
		}
	}


	return hr;
}
	



#ifdef _MSAA_EVENTS

//-----------------------------------------------------------------------
//	CDocumentAO::initDocumentEventHandlers()
//
//	DESCRIPTION:
//		
//		This method initializes the event handlers for the CDocumentAO.
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		HRESULT		S_OK | E_NOINTERFACE | standard COM error
//
//-----------------------------------------------------------------------

HRESULT CDocumentAO::initDocumentEventHandlers( void )
{
	HRESULT	hrEventInit;


	assert(m_pTOMObjIUnk);

	if(!m_pTOMObjIUnk)
		return(E_NOINTERFACE);

	assert(m_pDocIHTMLElement);

	if(!m_pDocIHTMLElement)
		return(E_NOINTERFACE);

	//--------------------------------------------------
	// create event handlers for document and body 
	// elements of CDocumentAO class
	//--------------------------------------------------

	hrEventInit = INIT_EVENT_HANDLER( ImplIHTMLDocumentEvents,
			                          m_pIUnknown,
			                          m_hWnd,
			                          m_nChildID,
			                          m_pTOMObjIUnk )

	if ( hrEventInit == S_OK )
	{
		hrEventInit = INIT_EVENT_HANDLER( ImplIDispIHTMLBodyElement,
				                          m_pIUnknown,
				                          m_hWnd,
				                          m_nChildID,
				                          m_pDocIHTMLElement)
	}

	return hrEventInit;
}

#endif	// _MSAA_EVENTS


//----  End of DOCUMENT.CPP  ----
