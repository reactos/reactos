//================================================================================
//		File:	TABLCELL.CPP
//		Date: 	7/10/97
//		Desc:	Contains implementation of CTableCellAO class.  CTableCellAO 
//				implements the accessible proxy for the Trident Table 
//				object.
//				
//		Author: Jay Clark
//================================================================================


//================================================================================
// Includes
//================================================================================

#include "stdafx.h"
#include "tablcell.h"


#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CTableCellAO,ImplIHTMLTextContainerEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLTEXTCONTAINEREVENTS_ONCHANGE,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLTEXTCONTAINEREVENTS_ONSELECT,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONSELECTSTART,EVENT_OBJECT_STATECHANGE)

END_EVENT_HANDLER_MAP()

#endif


//================================================================================
// CTableCellAO implementation: public methods
//================================================================================

//-----------------------------------------------------------------------
//	CTableCellAO::CTableCellAO()
//
//	DESCRIPTION:
//
//		constructor
//
//	PARAMETERS:
//
//		pAOParent	[IN]	pointer to the parent accessible object in 
//							the AOM tree
//
//		nTOMIndex	[IN]	index of the element from the TOM document.all 
//							collection.
//		
//		hWnd		[IN]	pointer to the window of the trident object that 
//							this object corresponds to.
//	RETURNS:
//
//		None.
// ----------------------------------------------------------------------

CTableCellAO::CTableCellAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// Assign the delegating IUnknown to CTableCellAO :
	//  this member will be overridden in derived class
	//  constructors so that the delegating IUnknown 
	//  will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown	= (LPUNKNOWN)this;									

	//--------------------------------------------------
	// Set the role parameter to be used
	//  in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_CELL;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_TABLECELL;

#ifdef _DEBUG

	//--------------------------------------------------
	// Set symbolic name of object for easy identification
	//--------------------------------------------------

	lstrcpy(m_szAOMName,_T("TableCellAO"));

#endif

}


//-----------------------------------------------------------------------
//	CTableCellAO::Init()
//
//	DESCRIPTION:
//
//		Initialization : set values of data members
//
//	PARAMETERS:
//
//		pTOMObjIUnk	[IN]	pointer to IUnknown of TOM object.
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT CTableCellAO::Init(LPUNKNOWN pTOMObjIUnk)
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
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLTextContainerEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

		assert( hrEventInit == S_OK );

#ifdef _DEBUG
		if ( hrEventInit != S_OK )
			OutputDebugString( "Event handler initialization in CTableCellAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return( hr );
}


//-----------------------------------------------------------------------
//	CTableCellAO::~CTableCellAO()
//
//	DESCRIPTION:
//
//		CTableCellAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
// ----------------------------------------------------------------------

CTableCellAO::~CTableCellAO()
{
}		


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//	CTableCellAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CTableCellAO object only implements
//		IUnknown.
//
//	PARAMETERS:
//
//		riid	[IN]	REFIID of requested interface.
//		ppv		[OUT]	pointer to interface in.
//
//	RETURNS:
//
//		E_NOINTERFACE | NOERROR.
// ----------------------------------------------------------------------

STDMETHODIMP CTableCellAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;


    if (riid == IID_IUnknown)  
	{
 		*ppv = (LPUNKNOWN)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLTextContainerEvents)
	{
		//--------------------------------------------------
		// this is the event interface for the table cell
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLTextContainerEvents,ppv,HTMLTextContainerEvents)
	} 
	
#endif

	else
        return(CTridentAO::QueryInterface(riid,ppv));

	assert( *ppv );
    
	((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}



//================================================================================
// CTableCellAO accessible interface implementation
//================================================================================

//-----------------------------------------------------------------------
//	CTableCellAO::GetAccName()
//
//	DESCRIPTION:
//
//		Returns the accessible name of the object.
//
//	PARAMETERS:
//
//		lChild		[IN]	child ID
//		pbstrName	[OUT]	pointer to array to return child name in.
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
// ----------------------------------------------------------------------

HRESULT	CTableCellAO::GetAccName(long lChild, BSTR *pbstrName)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrName );

	//------------------------------------------------
	// If name has already been retrieved, use cached
	//	value, otherwise retrieve from TOM.
	//------------------------------------------------

	if (m_bstrName)
		*pbstrName = SysAllocString(m_bstrName);
	else
	{
		hr = getTitleFromIHTMLElement(pbstrName);

		if(hr != S_OK)
			return(hr);

		if ( !*pbstrName )
			return DISP_E_MEMBERNOTFOUND;

		m_bstrName = SysAllocString(*pbstrName);
	}

	return( hr );
}


//-----------------------------------------------------------------------
//	CTableCellAO::GetAccState()
//
//	DESCRIPTION:
//
//		Returns current state of the object.
//
//	PARAMETERS:
//		
//		lChild		[IN]	ChildID
//		plState		[OUT]	Accessible state.
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//	NOTES:
//
//		Possible states: INVISIBLE | normal
//
//-----------------------------------------------------------------------
	
HRESULT	CTableCellAO::GetAccState(long lChild, long *plState)
{
	long ltempState =0;


	assert( plState );

	//--------------------------------------------------
	// Call down to the base class to determine
	// visibility.
	//--------------------------------------------------

	HRESULT hr = CTridentAO::GetAccState( lChild, &ltempState );


	//--------------------------------------------------
	// Update the location to determine offscreen status
	//--------------------------------------------------

	long lDummy;

	hr = AccLocation( &lDummy, &lDummy, &lDummy, &lDummy, CHILDID_SELF );

	if (SUCCEEDED( hr ) && m_bOffScreen)
		ltempState |= STATE_SYSTEM_INVISIBLE;


	*plState = ltempState;


	return( S_OK );
}


//----  End of TABLCELL.CPP  ----
