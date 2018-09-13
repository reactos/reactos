//================================================================================
//		File:	TABLE.CPP
//		Date: 	7/10/97
//		Desc:	Contains implementation of CTableAO class.  CTableAO 
//				implements the accessible proxy for the Trident Table 
//				object.
//				
//		Author: Jay Clark
//================================================================================


//================================================================================
// Includes
//================================================================================

#include "stdafx.h"
#include "table.h"


#ifdef _MSAA_EVENTS

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CTableAO,ImplIHTMLElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
		 
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
// CTableAO implementation: public methods
//================================================================================

//-----------------------------------------------------------------------
//	CTableAO::CTableAO()
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

CTableAO::CTableAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// Assign the delegating IUnknown to CTableAO :
	//  this member will be overridden in derived class
	//  constructors so that the delegating IUnknown 
	//  will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown	= (LPUNKNOWN)this;									

	//--------------------------------------------------
	// Set the role parameter to be used
	//  in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_TABLE;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_TABLE;

#ifdef _DEBUG

	//--------------------------------------------------
	// Set symbolic name of object for easy identification
	//--------------------------------------------------

	lstrcpy(m_szAOMName,_T("TableAO"));

#endif

}


//-----------------------------------------------------------------------
//	CTableAO::Init()
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

HRESULT CTableAO::Init(LPUNKNOWN pTOMObjIUnk)
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
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLElementEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

		assert( hrEventInit == S_OK );
										
#ifdef _DEBUG
		if ( hrEventInit != S_OK )
			OutputDebugString( "Event handler initialization in CTableAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return( hr );
}


//-----------------------------------------------------------------------
//	CTableAO::~CTableAO()
//
//	DESCRIPTION:
//
//		CTableAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
// ----------------------------------------------------------------------

CTableAO::~CTableAO()
{
}		


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//	CTableAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CTableAO object only implements
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

STDMETHODIMP CTableAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return( E_INVALIDARG );


	*ppv = NULL;


    if (riid == IID_IUnknown)  
	{
 		*ppv = (LPUNKNOWN)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLElementEvents)
	{
		//--------------------------------------------------
		// this is the event interface for the table class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLElementEvents,ppv,HTMLElementEvents)
	} 
	
#endif

	else
    {
		return(CTridentAO::QueryInterface(riid,ppv));
	}

	assert( *ppv );
    
	((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}



//================================================================================
// CTableAO accessible interface implementation
//================================================================================

//-----------------------------------------------------------------------
//	CTableAO::GetAccName()
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

HRESULT	CTableAO::GetAccName(long lChild, BSTR *pbstrName)
{
	HRESULT hr = S_OK;


	assert( pbstrName );


	if ( !m_bNameAndDescriptionResolved )
	{
		resolveNameAndDescription();
	}

	if ( m_bstrName )
		*pbstrName = SysAllocString( m_bstrName );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}


//-----------------------------------------------------------------------
//	CTableAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description of table object.
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

HRESULT	CTableAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrDescription );


	if ( !m_bNameAndDescriptionResolved )
	{
		resolveNameAndDescription();
	}

	if ( m_bstrDescription )
		*pbstrDescription = SysAllocString( m_bstrDescription );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}


//-----------------------------------------------------------------------
//	CTableAO::GetAccState()
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
	
HRESULT	CTableAO::GetAccState(long lChild, long *plState)
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


//================================================================================
// CTableAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CTableAO::getDescriptionString()
//
//	DESCRIPTION:
//
//		Obtains the INNERTEXT property of the TABLE's CAPTION to be used
//		as the accDescription of the CTableAO.
//
//	PARAMETERS:
//
//		pbstrDescStr	[out]	pointer to BSTR
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CTableAO::getDescriptionString( BSTR* pbstrDescStr )
{
	HRESULT	hr;


	assert( pbstrDescStr );


	*pbstrDescStr = NULL;


	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);
	if ( !pIHTMLElement )
		return E_NOINTERFACE;


	//--------------------------------------------------
	// get a list of the TABLE's children
	//--------------------------------------------------

	CComPtr<IDispatch> pIDispatch;

	if ( hr = pIHTMLElement->get_children( &pIDispatch ) )
		return hr;

	CComQIPtr<IHTMLElementCollection,&IID_IHTMLElementCollection> pIHTMLChildCollection(pIDispatch);
	if ( !pIHTMLChildCollection )
		return E_NOINTERFACE;
								   	
	
	//--------------------------------------------------
	//	Determine the length of the Child collection
	//	for the processing loop
	//--------------------------------------------------

	long	nTOMChildCollectionLen;

	hr = pIHTMLChildCollection->get_length( &nTOMChildCollectionLen );
	if ( hr != S_OK )
		return hr;
	if ( !nTOMChildCollectionLen )
		return E_FAIL;

		
	//--------------------------------------------------
	// Iterate thru the TABLE's children looking for a
	// IHTMLTableCaption.  The INNERTEXT property of the
	// first one found will be returned to the caller.
	//--------------------------------------------------

	long	i = 0;
	BOOL	bCaptionFound = FALSE;

	while ( i < nTOMChildCollectionLen  &&  hr == S_OK  &&  !bCaptionFound )
	{												 
		VARIANT varIndex;
		varIndex.vt = VT_UINT;
		varIndex.lVal = i;

		VARIANT var2;
		VariantInit( &var2 );

		CComPtr<IDispatch> pIDispChild;

		if ( hr = pIHTMLChildCollection->item( varIndex, var2, &pIDispChild ) )
			continue;
		
		CComQIPtr<IHTMLTableCaption,&IID_IHTMLTableCaption> pCappy(pIDispChild);

		if ( pCappy )
		{
			CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElementCaption( pCappy );
			if ( !pIHTMLElementCaption )
			{
				hr = E_NOINTERFACE;
				continue;
			}

			hr = pIHTMLElementCaption->get_innerText( pbstrDescStr );

			bCaptionFound = TRUE;
		}

		i++;
	}								 

	return hr;
}


//----  End of TABLE.CPP  ----
