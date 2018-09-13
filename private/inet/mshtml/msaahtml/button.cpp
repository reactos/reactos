//================================================================================
//		File:	BUTTON.CPP
//		Date: 	5/27/97
//		Desc:	contains implementation of CButtonAO class.  CButtonAO 
//				implements the accessible proxy for the Trident Button 
//				object.
//
//		Author: Arunj
//
//================================================================================

//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "trid_ao.h"
#include "button.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

#ifdef _MSAA_EVENTS

#include "event.h"

//================================================================================
// event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CButtonAO,ImplIHTMLButtonElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
		 
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
// CButtonAO : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CButtonAO::CButtonAO()
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
//		hWnd				pointer to the window of the trident object that 
//							this object corresponds to.
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CButtonAO::CButtonAO(CTridentAO * pAOParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CControlAO(pAOParent, pDocAO, nTOMIndex, nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CButtonAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;									


	//--------------------------------------------------
	// set the role parameter to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_PUSHBUTTON;
	
	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_BUTTON;


	//--------------------------------------------------
	// for IE3 compatibility, buttons must determine
	// their accName and accDescription each time they
	// are queried.
	//--------------------------------------------------

	m_bCacheNameAndDescription = FALSE;


#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("ButtonAO"));

#endif
	
}


//-----------------------------------------------------------------------
//	CButtonAO::Init()
//
//	DESCRIPTION:
//
//		Initialization : set values of data members
//
//	PARAMETERS:
//
//		pTOMObjIUnk		pointer to IUnknown of TOM object.
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CButtonAO::Init( IUnknown* pTOMObjIUnk )
{
	HRESULT hr = S_OK;


	assert( pTOMObjIUnk );

	if ( !pTOMObjIUnk )
		return E_INVALIDARG;


	//--------------------------------------------------
	// call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAO::Init( pTOMObjIUnk );

	if ( hr == S_OK )
	{
		//--------------------------------------------------
		// set type of button (INPUT or BUTTON tag)
		//--------------------------------------------------

		CComQIPtr<IHTMLButtonElement, &IID_IHTMLButtonElement> pIHTMLButton(m_pTOMObjIUnk);

		if ( !pIHTMLButton )
			m_bIsInputButtonType = TRUE;
		else
			m_bIsInputButtonType = FALSE;


		//--------------------------------------------------
		// we want to treat the button as an atomic unit
		// (even if the html would suggest that the button
		// should have children) because we'll fail when
		// trying to resolve the text ranges of elements
		// scoped by the button due to a Trident bug
		//--------------------------------------------------

		m_bResolvedState = TRUE;


#ifdef _MSAA_EVENTS

		HRESULT	hrEventInit;

		//--------------------------------------------------
		// initialize event handling interface : establish
		// Advise.
		//--------------------------------------------------
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLButtonElementEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

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
			OutputDebugString( "Event handler initialization in CButtonAO::Init() failed.\n" );
#endif

#endif	// _MSAA_EVENTS
	}


	return hr;
}


//-----------------------------------------------------------------------
//	CButtonAO::~CButtonAO()
//
//	DESCRIPTION:
//
//		CButtonAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.					   
//
// ----------------------------------------------------------------------

CButtonAO::~CButtonAO()
{
}		


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CButtonAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CButtonAO object only implements
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

STDMETHODIMP CButtonAO::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;

    if (riid == IID_IUnknown)  
    {
		*ppv = (IUnknown *)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLButtonElementEvents)
	{
		//--------------------------------------------------
		//	This is the event interface for the
		//	CButtonAO class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLButtonElementEvents,ppv,HTMLButtonElementEvents)
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
// CButtonAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CButtonAO::GetAccState()
//
//	DESCRIPTION:
//
//		returns state of TOM button
//
//	PARAMETERS:
//		
//		lChild		ChildID
//		plState		long to store returned state var in
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------
	
HRESULT	CButtonAO::GetAccState( long lChild, long* plState )
{
	HRESULT hr;


	assert( plState );

	*plState = 0;

	//--------------------------------------------------
	//	Call the base class to determine general state.
	//--------------------------------------------------

	hr = CControlAO::GetAccState( lChild, plState );

	if ( hr == S_OK && *plState != STATE_SYSTEM_UNAVAILABLE )
	{
		//--------------------------------------------------
		//	If the Trident button is an INPUT TYPE=SUBMIT,
		//	then this is a default button.
		//
		//	If we fail to determine the button's
		//	"defaultness", simply ignore this check.
		//--------------------------------------------------

		if ( m_bIsInputButtonType )
		{
			CComQIPtr<IHTMLInputButtonElement,&IID_IHTMLInputButtonElement> pIHTMLInputButtonElement(m_pTOMObjIUnk);

			if ( pIHTMLInputButtonElement )
			{
				BSTR bstrType;

				hr = pIHTMLInputButtonElement->get_type( &bstrType );

				if ( hr != S_OK )
					hr = S_OK;
				else if ( bstrType )
				{
					if ( !_wcsicmp( bstrType, L"SUBMIT" ) )
						*plState |= STATE_SYSTEM_DEFAULT;

					SysFreeString( bstrType );
				}
			}
		}
	}


	return hr;
}


//================================================================================
//	CButtonAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CButtonAO::getDefaultActionString()
//
//	DESCRIPTION:
//
//		Retrieves the default action string for the CButtonAO.
//	
//	PARAMETERS:
//
//		pbstrDefaultAction	BSTR pointer to return the default action in
//
//	RETURNS:
//
//		HRESULT :	S_OK or E_OUTOFMEMORY
//
//-----------------------------------------------------------------------

HRESULT	CButtonAO::getDefaultActionString( BSTR* pbstrDefaultAction )
{
	assert( pbstrDefaultAction );
	assert( *pbstrDefaultAction == NULL );
           
    return GetResourceStringValue(IDS_PRESS_ACTION, pbstrDefaultAction);
}


//-----------------------------------------------------------------------
//	CButtonAO::isControlDisabled()
//
//	DESCRIPTION:
//
//		Determines whether or not the Trident button is enabled.
//	
//	PARAMETERS:
//
//		pbDisabled	set to TRUE is Trident button is disabled,
//					FALSE if enabled
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND | standard COM error
//
//-----------------------------------------------------------------------

HRESULT	CButtonAO::isControlDisabled( VARIANT_BOOL* pbDisabled )
{
	HRESULT hr;


	if ( m_bIsInputButtonType )
	{
		CComQIPtr<IHTMLInputButtonElement,&IID_IHTMLInputButtonElement> pIHTMLInputButtonElement(m_pTOMObjIUnk);

		if ( !pIHTMLInputButtonElement )
			hr = DISP_E_MEMBERNOTFOUND;
		else
			hr = pIHTMLInputButtonElement->get_disabled( pbDisabled );
	}
	else
	{
		CComQIPtr<IHTMLButtonElement,&IID_IHTMLButtonElement> pIHTMLButtonElement(m_pTOMObjIUnk);

		if ( !pIHTMLButtonElement )
			hr = DISP_E_MEMBERNOTFOUND;
		else
			hr = pIHTMLButtonElement->get_disabled( pbDisabled );
	}


	return hr;
}



//-----------------------------------------------------------------------
//	CButtonAO::getDescriptionString()
//
//	DESCRIPTION:
//
//		Obtains the text displayed on the face of the button.
//
//	PARAMETERS:
//
//		pbstrDescStr	[out]	pointer to the BSTR to hold the text
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CButtonAO::getDescriptionString( BSTR* pbstrDescStr )
{
	HRESULT hr = S_OK;

	
	assert( pbstrDescStr );


	*pbstrDescStr = NULL;


	//--------------------------------------------------
	//	If the Trident is an INPUT button, the
	//	face text is the INPUT's VALUE.
	//	Otherwise, the Trident is a BUTTON button and
	//	the face text is the BUTTON's INNERTEXT.
	//--------------------------------------------------
	
	if ( m_bIsInputButtonType )
	{
		CComQIPtr<IHTMLInputButtonElement,&IID_IHTMLInputButtonElement> pIHTMLInputButtonElement(m_pTOMObjIUnk);

		if ( !pIHTMLInputButtonElement )
			hr = E_NOINTERFACE;
		else
			hr = pIHTMLInputButtonElement->get_value( pbstrDescStr );
	}
	else
	{
		CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

		if ( !pIHTMLElement )
			hr = E_NOINTERFACE;
		else
			hr = pIHTMLElement->get_innerText( pbstrDescStr );
	}


	return hr;
}




//----  End of BUTTON.CPP  ----