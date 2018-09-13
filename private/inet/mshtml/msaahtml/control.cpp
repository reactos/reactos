//================================================================================
//		File:	CONTROL.CPP
//		Date: 	10-16-97
//		Desc:	Contains the implementation of the CControlAO class.
//				CControlAO implements common functionality that the CButtonAE, the
//				CEditFieldAE, and the COptionButtonAE (base for both CCheckboxAE
//				and CRadioButtonAE) classes all share.
//================================================================================

//================================================================================
//	Includes
//================================================================================

#include "stdafx.h"
#include "control.h"


//================================================================================
//	CControlAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CControlAO::CControlAO()
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

CControlAO::CControlAO( CTridentAO* pAOMParent, CDocumentAO * pDocAO, UINT nTOMIndex, UINT nChildID, HWND hWnd )
: CTridentAO( pAOMParent, pDocAO, nTOMIndex, nChildID, hWnd )
{

	//--------------------------------------------------
	//	Controls, except buttons, can determine their
	//	accName and accDescription once and then
	//	cache those BSTRs.
	//--------------------------------------------------

	m_bCacheNameAndDescription = TRUE;
}


//-----------------------------------------------------------------------
//	CControlAO::~CControlAO()
//
//	DESCRIPTION:
//
//		CControlAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CControlAO::~CControlAO()
{
}



//================================================================================
//	CControlAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CControlAO::GetAccName()
//
//	DESCRIPTION:
//
//		returns name string to client.
//
//	PARAMETERS:
//
//		lChild			child ID
//		pbstrName		pointer to BSTR to return child name in
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::GetAccName(long lChild, BSTR * pbstrName)
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
//	CControlAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description string to client.
//
//	PARAMETERS:
//
//		lChild				Child ID
//
//		pbstrDescription	pointer to the BSTR to return the description
//							string in
//	
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
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
//	CControlAO::GetAccKeyboardShortcut()
//
//	DESCRIPTION:
//
//		Get shortcut string
//
//	PARAMETERS:
//
//		lChild						child ID 
//		pbstrKeyboardShortcut		pointer to BSTR to return keyboard
//									shortcut string in
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND
//
//	NOTES:
//
//		All Trident controls support the IHTMLControlElement interface.
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
{
	HRESULT hr = S_OK;
	

	assert( pbstrKeyboardShortcut );

	
	if( m_bstrKbdShortcut )
	{
		*pbstrKeyboardShortcut = SysAllocString( m_bstrKbdShortcut );
	}
	else
	{
		//--------------------------------------------------
		//	Get the shortcut via IHTMLControlElement
		//	accessKey property
		//--------------------------------------------------

		CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);

		if( !pIHTMLControlElement )
			hr = DISP_E_MEMBERNOTFOUND;
		else
		{
			hr = pIHTMLControlElement->get_accessKey( pbstrKeyboardShortcut );

			if ( hr == S_OK && *pbstrKeyboardShortcut )
				m_bstrKbdShortcut = SysAllocString( *pbstrKeyboardShortcut );
			else
				hr = DISP_E_MEMBERNOTFOUND;
		}
	}

	return hr;
}



//-----------------------------------------------------------------------
//	CControlAO::GetAccState()
//
//	DESCRIPTION:
//
//		returns state of TOM Option Button.
//		STATE_SYSTEM_UNAVAILABLE
//		STATE_SYSTEM_FOCUSABLE
//		STATE_SYSTEM_FOCUSED
//		STATE_SYSTEM_DEFAULT
//
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
//-----------------------------------------------------------------------
	
HRESULT	CControlAO::GetAccState( long lChild, long *plState )
{
	HRESULT hr = S_OK;
	

	assert( plState );


	//--------------------------------------------------
	//	Initialize the out parameter.
	//--------------------------------------------------

	*plState = 0;


	//--------------------------------------------------
	//	Determine if the control is enabled.  If not,
	//	set state to STATE_SYSTEM_UNAVAILABLE.
	//
	//	To determine whether or not the control is
	//	enabled, call the virtual CControlAO method
	//	isControlDisabled().  This method should be
	//	overridden by derived class for handling of
	//	particular controls.
	//--------------------------------------------------

	VARIANT_BOOL	bDisabled;

	hr = isControlDisabled( &bDisabled );

	if ( hr != S_OK || bDisabled )
	{
		*plState = STATE_SYSTEM_UNAVAILABLE;
		return S_OK;
	}


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
	//	The CControlAO is FOCUSABLE only if the Trident
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
		//	which means that our CControlAO can neither be
		//	FOCUSABLE nor FOCUSED.
		//--------------------------------------------------

		hr = S_OK;
	}
	
	*plState = lTempState;


	return hr;
}


//-----------------------------------------------------------------------
//	CControlAO::GetAccDefaultAction()
//
//	DESCRIPTION:
//		sets input BSTR parameter to value of default action.
//	
//	PARAMETERS:
//
//		lChild			child ID
//
//		pbstrDefAction	returned description string
//
//	RETURNS:
//
//		HRESULT :	S_OK, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND, or other
//					standard COM error as returned by Trident.
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
{
	HRESULT hr;


	assert( pbstrDefAction );

	if ( !m_bstrDefaultAction )
	{
		hr = getDefaultActionString( &m_bstrDefaultAction );

		if ( hr != S_OK )
		{
			assert( !m_bstrDefaultAction );

			if ( hr == S_FALSE )
				hr = DISP_E_MEMBERNOTFOUND;

			return hr;
		}
	}

	if ( *pbstrDefAction = SysAllocString( m_bstrDefaultAction ) )
		return S_OK;
	else
		return E_OUTOFMEMORY;
}


//-----------------------------------------------------------------------
//	CControlAO::AccDoDefaultAction()
//
//	DESCRIPTION:
//		(1) select the control (scroll it into view, focus it)
//		(2) click the control
//	
//	PARAMETERS:
//
//		lChild		Child ID
//
//	RETURNS:
//
//		S_OK | valid COM error
//
// ----------------------------------------------------------------------

HRESULT	CControlAO::AccDoDefaultAction( long lChild )
{
	//--------------------------------------------------
	//	Delegate click action to internal method
	//--------------------------------------------------

	return click();

}


//-----------------------------------------------------------------------
//	CControlAO::GetAccSelection()
//
//	DESCRIPTION:
//
//		get_accSelection() not supported for a control.
//
//
//	PARAMETERS:
//		
//		ppIUnknown	pointer to an IUnknown*
//
//	RETURNS:
//
//		HRESULT		DISP_E_MEMBERNOTFOUND
// ----------------------------------------------------------------------

HRESULT CControlAO::GetAccSelection( IUnknown** ppIUnknown )
{
	return DISP_E_MEMBERNOTFOUND;
}

//-----------------------------------------------------------------------
//	CControlAO::AccSelect()
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

HRESULT	CControlAO::AccSelect(long flagsSel, long lChild)
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

	CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);

	if ( !pIHTMLControlElement )
		hr = E_NOINTERFACE;
	else
		hr = pIHTMLControlElement->focus();

	return hr;

}



//================================================================================
//	CControlAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CControlAO::focus()
//
//	DESCRIPTION:
//
//		Focuses the associated Trident control.
//	
//	PARAMETERS:
//
//		none.
//
//	RETURNS:
//
//		E_NOINTERFACE | return of IHTMLControlElement::focus()
//
//	NOTES:
//
//		All Trident controls support the IHTMLControlElement interface.
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::focus( void )
{
	CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);

	if ( !pIHTMLControlElement )
		return E_NOINTERFACE;

	return pIHTMLControlElement->focus();
}


//-----------------------------------------------------------------------
//	CControlAO::getDefaultActionString()
//
//	DESCRIPTION:
//
//		Retrieves the default action string for the CControlAO.
//	
//	PARAMETERS:
//
//		pbstrDefaultAction	BSTR pointer to return the default action in
//
//	RETURNS:
//
//		HRESULT :	S_OK
//
//	NOTES:
//
//		It is intended this method will be overridden by derived classes.
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::getDefaultActionString( BSTR *pbstrDefaultAction )
{
	return S_FALSE;
}


//-----------------------------------------------------------------------
//	CControlAO::isControlDisabled()
//
//	DESCRIPTION:
//
//		Determines whether or not the Trident button is disabled.
//	
//	PARAMETERS:
//
//		pbDisabled	set to TRUE is Trident button is disabled,
//					FALSE if it is enabled
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND | standard COM error
//
//	NOTES:
//
//		It is intended this method will be overridden by derived classes.
//
//-----------------------------------------------------------------------

HRESULT	CControlAO::isControlDisabled( VARIANT_BOOL* pbDisabled )
{
	assert( pbDisabled );

	*pbDisabled = TRUE;

	return S_OK;
}



//-----------------------------------------------------------------------
//	CControlAO::resolveNameAndDescription()
//
//	DESCRIPTION:
//
//		Determines the accName and accDescription properties.
//
//	PARAMETERS:
//
//		None.
//
//	RETURNS:
//
//		HRESULT
//
//	NOTES:
//
//		This method overrides the method defined in the base class,
//		CTridentAO.
//
//		For IE3 compatibility, the accName of controls is the
//		traditional value: for buttons, its the "face" text and for
//		check boxes, radio buttons, edit fields its the static text
//		associated with the LABEL tag.  The TITLE attribute is
//		used as the accDescription, or as the accName when the
//		traditional value is not set.  So, although the method name
//		is confusing, the BSTR set in getDescriptionString() is
//		used as the accName and the BSTR set in
//		getTitleFromIHTMLElement() is used as the accDescription.
//
//		For IE3 compatibility, the accName property of the CButtonAO
//		must not be cached.  Given the current architecture and the
//		interdependence of the button's accName and accDescription,
//		this means two things.  First, the accDescription property
//		is not cached.  Second, the lack of caching is achieved by
//		never the CButtonAO class setting the member boolean
//		m_bCacheNameAndDescription to FALSE in its ctor.
//
//-----------------------------------------------------------------------

HRESULT CControlAO::resolveNameAndDescription( void )
{
	HRESULT hr = E_FAIL;
	BSTR	bstrTmp = NULL;


	//------------------------------------------------
	//	Initialize affected member variables.
	//------------------------------------------------

	m_bNameAndDescriptionResolved = FALSE;

	if ( m_bstrName )
	{
		SysFreeString( m_bstrName );
		m_bstrName = NULL;
	}

	if ( m_bstrDescription )
	{
		SysFreeString( m_bstrDescription );
		m_bstrDescription = NULL;
	}


	//------------------------------------------------
	//	Get accName first, then accDescription.
	//------------------------------------------------

	//--------------------------------------------------
	//	If the traditional control name is defined, use
	//	it for the control's accName.
	//--------------------------------------------------

	hr = getDescriptionString( &bstrTmp );

	if ( hr != S_OK )
		return hr;

	if ( bstrTmp )
	{
		m_bstrName = SysAllocString( bstrTmp );
		SysFreeString( bstrTmp );

		//--------------------------------------------------
		//	The accName property has been cached, now let's
		//	try to get the accDescription.
		//
		//	If the control has a TITLE attribute, use that
		//	as its description.
		//--------------------------------------------------

		bstrTmp = NULL;

		hr = getTitleFromIHTMLElement( &bstrTmp );

		if ( hr != S_OK )
		{
			SysFreeString( m_bstrName );
			return hr;
		}
		else
		{
			if ( bstrTmp )
			{
				m_bstrDescription = SysAllocString( bstrTmp );
				SysFreeString( bstrTmp );
			}

			m_bNameAndDescriptionResolved = m_bCacheNameAndDescription;
		}
	}

	else
	{
		//--------------------------------------------------
		//	The traditional control name is not defined,
		//	so use its TITLE, if it exists, for the accName.
		//	Note that this means that the control's
		//	description will be empty.
		//--------------------------------------------------

		hr = getTitleFromIHTMLElement( &bstrTmp );

		if ( hr == S_OK )
		{
			if ( bstrTmp )
			{
				m_bstrName = SysAllocString( bstrTmp );
				SysFreeString( bstrTmp );
			}

			m_bNameAndDescriptionResolved = m_bCacheNameAndDescription;
		}
	}


	return hr;
}



//-----------------------------------------------------------------------
//	CControlAO::getDescriptionString()
//
//	DESCRIPTION:
//
//		Obtains the text of the associated LABEL's INNERTEXT property
//		to be used as the CControlAO derived class's accDescription.
//
//	PARAMETERS:
//
//		pbstrDescStr	[out]	pointer to the BSTR to hold the INNERTEXT
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CControlAO::getDescriptionString( BSTR* pbstrDescStr )
{
	HRESULT hr = E_FAIL;


	assert( pbstrDescStr );

	
	//--------------------------------------------------
	//	Initialize the out parameter.
	//--------------------------------------------------

	*pbstrDescStr = NULL;

	
	//--------------------------------------------------
	//	Get the IHTMLElement* of the associated LABEL.
	//--------------------------------------------------

	CComPtr<IHTMLElement> pIHTMLElementLABEL;

	hr = getAssociatedLABEL( &pIHTMLElementLABEL );


	//--------------------------------------------------
	//	If a failure occurred, propagate it.
	//--------------------------------------------------

	if ( FAILED( hr ) )
		return hr;


	//--------------------------------------------------
	//	If S_FALSE was returned, there is no LABEL
	//	associated with this control.
	//--------------------------------------------------

	else if ( hr == S_FALSE )
		hr = S_OK;


	//--------------------------------------------------
	//	Otherwise, get the INNERTEXT of the LABEL.
	//--------------------------------------------------

	else if ( pIHTMLElementLABEL )
		hr = pIHTMLElementLABEL->get_innerText( pbstrDescStr );


	return hr;
}



//--------------------------------------------------------------------------------
//  CControlAO::getAssociatedLABEL()
//
//	DESCRIPTION:
//		Get the IHTMLElement pointer to the LABEL tag associated with the AE's
//		Trident.
//	
//	PARAMETERS:
//
//		ppIHTMLElementLABEL		pointer to a pointer to an IHTMLElement
//		
//
//	RETURNS:
//
//		S_OK if the AE has an associated LABEL tag
//		S_FALSE if the AE has no associated LABEL tag
//		valid COM error otherwise
//
//--------------------------------------------------------------------------------

HRESULT CControlAO::getAssociatedLABEL( /* out */ IHTMLElement** ppIHTMLElementLABEL )
{
	HRESULT	hr = S_OK;


	//--------------------------------------------------
	//	Initialize the out parameter.
	//--------------------------------------------------

	*ppIHTMLElementLABEL = NULL;

	
	//--------------------------------------------------
	//	Obtain the IHTMLElement* for the AE's Trident.
	//--------------------------------------------------

	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);
	if ( !pIHTMLElement )
		return E_NOINTERFACE;


	//--------------------------------------------------
	//	Obtain the IHTMLElement* for the parent of the
	//	AE's Trident.
	//--------------------------------------------------

	CComPtr<IHTMLElement> pIHTMLElementParent;

	if ( hr = pIHTMLElement->get_parentElement( &pIHTMLElementParent ) )
		return hr;
	if ( !pIHTMLElementParent )
		return E_NOINTERFACE;


	//--------------------------------------------------
	//	Is the parent of the AE's Trident a LABEL?
	//	If it is, it should support the
	//	IHTMLLabelElement interface.  QI for this iface.
	//--------------------------------------------------

	CComQIPtr<IHTMLLabelElement,&IID_IHTMLLabelElement> pIHTMLLabelElement(pIHTMLElementParent);


	if ( pIHTMLLabelElement )
	{
		//--------------------------------------------------
		//	The IHTMLLabelElement interface is supported,
		//	so the AE's parent is a LABEL.  Set the out
		//	parameter appropriately.
		//--------------------------------------------------

		*ppIHTMLElementLABEL = (IHTMLElement*)pIHTMLElementParent;
		(*ppIHTMLElementLABEL)->AddRef();
		hr = S_OK;
	}
	else
	{
		BSTR	bstrID;

		if ( hr = pIHTMLElement->get_id( &bstrID ) )
			return hr;

		if ( !bstrID )
			hr = S_FALSE;
		else
		{
			if ( SysStringLen( bstrID ) )
				hr = walkLABELCollection( pIHTMLElement, &bstrID, ppIHTMLElementLABEL );
			else
				hr = S_FALSE;

			SysFreeString( bstrID );
		}
	}

	return hr;
}



//--------------------------------------------------------------------------------
//  CControlAO::walkLABELCollection()
//
//	DESCRIPTION:
//
//		Creates a collection of LABEL tags and traverses this collection looking
//		for the LABEL whose HTMLFOR property equals the passed in string.
//	
//	PARAMETERS:
//
//		pIHTMLElement			pointer to an IHTMLElement (this is the TEO
//								associated with the CTridentAE)
//		pbstrID					pointer to a BSTR
//		ppIHTMLElementLABEL		pointer to a pointer to an IHTMLElement
//		
//
//	RETURNS:
//
//		S_OK if there is a LABEL whose HTMLFOR equals pcstrID
//		S_FALSE if there's no such LABEL and but no errors
//		valid COM error otherwise
//
//--------------------------------------------------------------------------------

HRESULT CControlAO::walkLABELCollection( /* in */	IHTMLElement* pIHTMLElement,
                                         /* in */	BSTR* pbstrID,
                                         /* out */	IHTMLElement** ppIHTMLElementLABEL )
{
	HRESULT hr = S_OK;
	long	nCollectionLen = 0, nCurLabel = 0;
	BOOL	bLabelFound = FALSE;
	VARIANT varIndex;
	VARIANT var2;


	*ppIHTMLElementLABEL = NULL;

	//--------------------------------------------------
	//	Obtain a collection of LABEL tags.
	//--------------------------------------------------

	CComPtr<IHTMLElementCollection> pLabelTagCollection;

	hr = getTOMCollection( pIHTMLElement, L"LABEL", &pLabelTagCollection );

	if ( hr != S_OK )
		return hr;

	if ( !pLabelTagCollection )
		return E_NOINTERFACE;
	
	//--------------------------------------------------
	//	Traverse the collection of LABELs looking for
	//	one whose HTMLFOR property is the same as the
	//	ID property of the control.
	//--------------------------------------------------

	hr = pLabelTagCollection->get_length( &nCollectionLen );

	if ( hr != S_OK )
		return hr;

	while ( nCurLabel < nCollectionLen  &&  hr == S_OK  &&  !bLabelFound )
	{
		//--------------------------------------------------
		//	Get the next LABEL in the collection.
		//--------------------------------------------------

		varIndex.vt = VT_UINT;
		varIndex.lVal = nCurLabel;

		VariantInit( &var2 );

		CComPtr<IDispatch> pIDispatchLabel;

		hr = pLabelTagCollection->item( varIndex, var2, &pIDispatchLabel );

		if ( hr != S_OK )
			continue;

		if ( !pIDispatchLabel )
		{
			hr = E_NOINTERFACE;
			continue;
		}
		
		CComQIPtr<IHTMLLabelElement,&IID_IHTMLLabelElement> pIHTMLLabelElement( pIDispatchLabel );

		if ( !pIHTMLLabelElement )
		{
			hr = E_NOINTERFACE;
			continue;
		}

		//--------------------------------------------------
		//	If the HTMLFOR property of the current LABEL
		//	is the same as ID property of the control,
		//	we've found our LABEL.
		//--------------------------------------------------

		BSTR	bstrHTMLFor;

		hr = pIHTMLLabelElement->get_htmlFor( &bstrHTMLFor );

		if ( hr != S_OK )
			continue;

		if ( bstrHTMLFor )
		{
			if ( !_wcsicmp( *pbstrID, bstrHTMLFor ) )
			{
				//--------------------------------------------------
				//	This is the LABEL we are looking for!
				//--------------------------------------------------

				bLabelFound = TRUE;

				//--------------------------------------------------
				//	Get the IHTMLElement* for the LABEL.
				//--------------------------------------------------

				hr = pIHTMLLabelElement->QueryInterface( IID_IHTMLElement, (void**)ppIHTMLElementLABEL );
			}

			SysFreeString( bstrHTMLFor );
		}

		nCurLabel++;
	}

	if ( hr == S_OK && !bLabelFound )
		hr = S_FALSE;

	return hr;
}


//-----------------------------------------------------------------------
//	CControlAO::getTOMCollection()
//
//	DESCRIPTION:
//
//		Obtains a collection of TEOs whose tag name equals cstrTagName.
//
//	PARAMETERS:
//
//		pIHTMLElement		pointer to a IHTMLElement
//
//		lpcwstrTagName		pointer to a wide string that holds the
//							name of the tag for the desired collection
//
//		ppTagsCollection	pointer to the IHTMLElementCollection pointer
//							that will point to the desired collection
//
//	RETURNS:
//
//		HRESULT				S_OK or a COM error code.
//
//	NOTES:
//
//		It is assumed that the IHTMLElement* pointer (parameter 1)
//		will be valid for the duration of this method and that it
//		is not this method's responsibility to release the interface.
//
//		This method sets the out parameter to point to the equivalent
//		of the script document.all.tags(<tag_name>) where tag_name is
//		specified by the BSTR in parameter.  The IHTMLElement* in
//		parameter is used to obtain the IHTMLDocument2* which starts
//		things off.
//
//-----------------------------------------------------------------------

HRESULT CControlAO::getTOMCollection( /* in  */ IHTMLElement*				pIHTMLElement,
                                      /* in  */ LPCWSTR						lpcwstrTagName,
                                      /* out */ IHTMLElementCollection**	ppTagsCollection )
{
	HRESULT	hr;

	//--------------------------------------------------
	//	Validate the parameters.
	//--------------------------------------------------

	if ( !pIHTMLElement || !ppTagsCollection )
		return E_INVALIDARG;

	//--------------------------------------------------
	//	Get a document (IHTMLDocument2) pointer.
	//--------------------------------------------------

	CComPtr<IDispatch> pIDispatchDocument;

	hr = pIHTMLElement->get_document( &pIDispatchDocument );

	if ( hr != S_OK )
		return hr;

	if ( !pIDispatchDocument )
		return E_NOINTERFACE;

	CComQIPtr<IHTMLDocument2,&IID_IHTMLDocument2> pIHTMLDocument2( pIDispatchDocument );

	if ( !pIHTMLDocument2 )
		return E_NOINTERFACE;

	//--------------------------------------------------
	//	Get the all collection (IHTMLElementCollection)
	//	pointer.
	//--------------------------------------------------

	CComPtr<IHTMLElementCollection> pIHTMLElementCollectionAll;

	hr = pIHTMLDocument2->get_all( &pIHTMLElementCollectionAll );

	if ( hr != S_OK )
		return hr;

	if ( !pIHTMLElementCollectionAll )
		return E_NOINTERFACE;

	//--------------------------------------------------
	//	Get the collection (IHTMLElementCollection) of
	//	requested tags.
	//--------------------------------------------------

	VARIANT varTagName;

	varTagName.vt = VT_BSTR;
	if ( !(varTagName.bstrVal = SysAllocString( lpcwstrTagName )) )
		return E_OUTOFMEMORY;

	CComPtr<IDispatch> pIDispatchTagCollection;

	hr = pIHTMLElementCollectionAll->tags( varTagName, &pIDispatchTagCollection );

	SysFreeString( varTagName.bstrVal );

	if ( hr != S_OK )
		return hr;

	if ( !pIDispatchTagCollection )
		return E_NOINTERFACE;

	hr = pIDispatchTagCollection->QueryInterface( IID_IHTMLElementCollection, (void**) ppTagsCollection );

	if ( hr != S_OK )
		*ppTagsCollection = NULL;

	return hr;
}

//----  End of CONTROL.CPP  ----