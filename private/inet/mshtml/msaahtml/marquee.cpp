//================================================================================
//		File:	MARQUEE.CPP
//		Date: 	7/14/97
//		Desc:	Contains implementation of CMarqueeAO class.  CMarqueeAO 
//				implements the accessible proxy for the Trident Table 
//				object.
//				
//		Author: Arunj
//================================================================================

//================================================================================
// Includes
//================================================================================

#include "stdafx.h"
#include "marquee.h"


#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CMarqueeAO,ImplIHTMLMarqueeElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLTEXTCONTAINEREVENTS_ONCHANGE,EVENT_OBJECT_SELECTION)	 
	ON_DISPID_FIRE_EVENT(DISPID_HTMLMARQUEEELEMENTEVENTS_ONBOUNCE,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLMARQUEEELEMENTEVENTS_ONFINISH,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLMARQUEEELEMENTEVENTS_ONSTART,EVENT_OBJECT_STATECHANGE)

END_EVENT_HANDLER_MAP()

#endif


//================================================================================
// CMarqueeAO public methods
//================================================================================

//-----------------------------------------------------------------------
//	CMarqueeAO::CMarqueeAO()
//
//	DESCRIPTION:
//
//		Constructor
//
//	PARAMETERS:
//
//		pAOParent	[IN]	Pointer to the parent accessible object in 
//							the AOM tree
//
//		nTOMIndex	[IN]	Index of the element from the TOM document.all 
//							collection.
//		
//		hWnd		[IN]	Pointer to the window of the trident object that 
//							this object corresponds to.
//	RETURNS:
//
//		None.
// ----------------------------------------------------------------------

CMarqueeAO::CMarqueeAO(CTridentAO *pAOParent,CDocumentAO *pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// Assign the delegating IUnknown to CMarqueeAO :
	//  this member will be overridden in derived class
	//  constructors so that the delegating IUnknown 
	//  will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown	= (LPUNKNOWN)this;									

	//--------------------------------------------------
	// TODO: check with ChuckOp to make sure that this
	// is appropriate.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_ANIMATION;


	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_MARQUEE;


#ifdef _DEBUG

	//--------------------------------------------------
	// Set symbolic name of object for easy identification
	//--------------------------------------------------

	lstrcpy(m_szAOMName,_T("MARQUEE"));

#endif

}

//-----------------------------------------------------------------------
//	CMarqueeAO::Init()
//
//	DESCRIPTION:
//
//		Initialization : set values of data members
//
//	PARAMETERS:
//
//		pTOMObjIUnk	[IN]	Pointer to IUnknown of TOM object.
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CMarqueeAO::Init(LPUNKNOWN pTOMObjIUnk)
{
	HRESULT hr	= E_FAIL;


	assert( pTOMObjIUnk );

	//--------------------------------------------------
	// Call down to base class to set IUnknown pointer
	//--------------------------------------------------

	hr = CTridentAO::Init(pTOMObjIUnk);


#ifdef _MSAA_EVENTS

	if ( hr == S_OK )
	{
		HRESULT	hrEventInit;

		//--------------------------------------------------
		// initialize event handling interface : establish
		// Advise.
		//--------------------------------------------------
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLMarqueeElementEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

		//--------------------------------------------------
		//	Event initialization errors are not propagated
		//	to the caller of CMarqueeAO::Init().  This
		//	allows the marquee object to be created, it
		//	just won't support events.
		//--------------------------------------------------

		assert( hrEventInit == S_OK );

#ifdef _DEBUG
		//--------------------------------------------------
		//	If we are in debug mode and an error occurred,
		//	pump a message to the output window.
		//--------------------------------------------------

		if ( hrEventInit != S_OK )
			OutputDebugString( "Event handler initialization in CMarqueeAO::Init() failed.\n" );

#endif	// _DEBUG
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//-----------------------------------------------------------------------
//	CMarqueeAO::~CMarqueeAO()
//
//	DESCRIPTION:
//
//		CMarqueeAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
// ----------------------------------------------------------------------

CMarqueeAO::~CMarqueeAO()
{
}		


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//	CMarqueeAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CMarqueeAO object only implements
//		IUnknown.
//
//	PARAMETERS:
//
//		riid	[IN]	REFIID of requested interface.
//		ppv		[OUT]	Pointer to requested interface pointer.
//
//	RETURNS:
//
//		E_NOINTERFACE | NOERROR.
// ----------------------------------------------------------------------

STDMETHODIMP CMarqueeAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;


    if (riid == IID_IUnknown)  
	{
 		*ppv = (LPUNKNOWN)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLMarqueeElementEvents)
	{
		//--------------------------------------------------
		// this is the event interface for the Marquee class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLMarqueeElementEvents,ppv,HTMLMarqueeElementEvents)
	} 
	
#endif
	
	else
    {
		return CTridentAO::QueryInterface( riid, ppv );
	}
    

	if ( !*ppv )
		return E_NOINTERFACE;


	((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
}



//================================================================================
// CMarqueeAO Accessible interface methods
//================================================================================

//-----------------------------------------------------------------------
//	CMarqueeAO::GetAccName()
//
//	DESCRIPTION:
//
//		returns name of Marquee object.
//
//	PARAMETERS:
//
//		lChild		child ID
// 
//		pbstrName	BSTR to return name in.
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CMarqueeAO::GetAccName(long lChild, BSTR * pbstrName)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrName );


	if ( !m_bNameAndDescriptionResolved )
	{
		hr = resolveNameAndDescription();

		if ( hr != S_OK )
			return hr;
	}

	if ( m_bstrName )
		*pbstrName = SysAllocString( m_bstrName );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}
		


//-----------------------------------------------------------------------
//	CMarqueeAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description of the marquee object.
//
//	PARAMETERS:
//		
//		lChild				ChildID
//		pbstrDescription	string to store value in
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CMarqueeAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrDescription );


	if ( !m_bNameAndDescriptionResolved )
	{
		hr = resolveNameAndDescription();

		if ( hr != S_OK )
			return hr;
	}

	if ( m_bstrDescription )
		*pbstrDescription = SysAllocString( m_bstrDescription );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}



//-----------------------------------------------------------------------
//	CMarqueeAO::GetAccState()
//
//	DESCRIPTION:
//
//		returns name of Marquee object.
//
//	PARAMETERS:
//
//		lChild		child ID
// 
//		plState		long * to return state in
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CMarqueeAO::GetAccState(long lChild, long *plState)
{
	HRESULT	hr;
	long	ltempState = 0;

	assert( plState );


	//--------------------------------------------------
	// Call down to the base class to determine
	// visibility.
	//--------------------------------------------------

	hr = CTridentAO::GetAccState( lChild, &ltempState );

	//--------------------------------------------------
	// A marquee is always marqueed.
	//--------------------------------------------------

	ltempState |= STATE_SYSTEM_MARQUEED;

	//--------------------------------------------------
	// Update the location to determine offscreen status
	//--------------------------------------------------

	long lDummy;

	hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

	if (SUCCEEDED( hr ) && m_bOffScreen)
		ltempState |= STATE_SYSTEM_INVISIBLE;

	//--------------------------------------------------
	// TODO :check with ChuckOp to see if there are any
	// other states.
	//--------------------------------------------------
	
	*plState = ltempState;

	return(S_OK);
}



//================================================================================
//	CMarqueeAO protected methods
//================================================================================



//----  End of MARQUEE.CPP  ----