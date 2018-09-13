//================================================================================
//		File:	EDITFLD.CPP
//		Date: 	6/1/97
//		Desc:	Contains the implementation of CEditFieldAO class.  The class
//				implements the MSAA proxy for Trident edit fields.
//
//		Author:	Arunj
//
//================================================================================

//================================================================================
//	Includes
//================================================================================

#include "stdafx.h"
#include "editfld.h"


#ifdef _MSAA_EVENTS

//================================================================================
//	Event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CEditFieldAO,ImplIHTMLInputTextElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONCHANGE,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONKEYPRESS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONSELECT,EVENT_OBJECT_SELECTION)
	 
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
//	CEditFieldAO class implementation: public methods
//================================================================================

//-----------------------------------------------------------------------
//	CEditFieldAO::CEditFieldAO()
//
//	DESCRIPTION:
//
//		CEditFieldAO class constructor.
//
//	PARAMETERS:
//
//		pAOMParent			pointer to the parent accessible object in 
//							the AOM tree
//
//		nTOMIndex			index of the element from the TOM document.all 
//							collection.
//		
//		nUID				unique ID.
//
//		hWndTrident			pointer to the window of the trident object that 
//							this object corresponds to.
//
//		pObjUnk				pointer to Trident object IUnknown
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CEditFieldAO::CEditFieldAO( CTridentAO* pAOMParent, CDocumentAO * pDocAO,UINT nTOMIndex, UINT nUID, HWND hTridentWnd )
: CControlAO( pAOMParent, pDocAO, nTOMIndex, nUID, hTridentWnd )
{
	//------------------------------------------------
	// assign the delegating IUnknown to CEditFieldAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;

	//--------------------------------------------------
	// set the role parameter to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_TEXT;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_EDITFIELD;


#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("CEditFieldAO"));

#endif

}


//-----------------------------------------------------------------------
//	CEditFieldAO::~CEditFieldAO()
//
//	DESCRIPTION:
//
//		CEditFieldAO class destructor.
//
//	PARAMETERS:
//
//		none.
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CEditFieldAO::~CEditFieldAO()
{

}


//-----------------------------------------------------------------------
//	CEditFieldAO::Init()
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
//		HRESULT
//
// ----------------------------------------------------------------------

HRESULT CEditFieldAO::Init( IUnknown* pTOMObjIUnk )
{
	HRESULT hr = E_FAIL;


	//--------------------------------------------------
	//	Validate the in parameter.
	//--------------------------------------------------

	assert( pTOMObjIUnk );

	if ( !pTOMObjIUnk )
		return E_INVALIDARG;


	//--------------------------------------------------
	//	Call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAO::Init( pTOMObjIUnk );


#ifdef _MSAA_EVENTS

	if ( hr == S_OK )
	{
		HRESULT	hrEventInit;

		//--------------------------------------------------
		//	Allocate event handling interface and establish
		//	Advise.
		//--------------------------------------------------
				
		hrEventInit = INIT_EVENT_HANDLER( ImplIHTMLInputTextElementEvents, m_pIUnknown, m_hWnd, m_nChildID, pTOMObjIUnk )

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
			OutputDebugString( "Event handler initialization in CEditFieldAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//=======================================================================
//	IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CEditFieldAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CEditFieldAO object only implements
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

STDMETHODIMP CEditFieldAO::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;


    if ( riid == IID_IUnknown )
    {
        *ppv = (IUnknown *)m_pIUnknown;
	}

#ifdef _MSAA_EVENTS	
	
	else if ( riid == DIID_HTMLInputTextElementEvents )
	{
		//--------------------------------------------------
		//	This is the event interface for CEditFieldAO.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLInputTextElementEvents,ppv,HTMLInputTextElementEvents)
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
	//	If ppv has not been set, then we don't support
	//	the requested interface.
	//--------------------------------------------------

	if ( *ppv == NULL )
	{
        return E_NOINTERFACE;
	}

    
	((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
}


//=======================================================================
//	CEditFieldAO Accessible Interface helper methods 
//============================-===========================================

//-----------------------------------------------------------------------
//	CEditFieldAO::GetAccValue()
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
//		HRESULT :	S_OK if success, DISP_E_MEMBERNOTFOUND for error
//
//	NOTES:
//
//		Since the value of the edit field can change at any time,
//		we have to query for it every time this method is called.
//
//-----------------------------------------------------------------------

HRESULT	CEditFieldAO::GetAccValue( long lChild, BSTR* pbstrValue )
{
	HRESULT hr = DISP_E_MEMBERNOTFOUND;


	assert( pbstrValue );


	//------------------------------------------------
	//	The CEditFieldAO can proxy either a TEXTAREA
	//	Trident or an INPUT TYPE=TEXT Trident.
	//	QI first for IHTMLInputTextElement.  If that's
	//	not supported, QI for IHTMLTextAreaElement.
	//------------------------------------------------

	CComQIPtr<IHTMLInputTextElement,&IID_IHTMLInputTextElement> pIHTMLInputTextEl(m_pTOMObjIUnk);

	if ( pIHTMLInputTextEl )
		hr = pIHTMLInputTextEl->get_value( pbstrValue );
	else
	{
		CComQIPtr<IHTMLTextAreaElement,&IID_IHTMLTextAreaElement> pIHTMLTextAreaEl(m_pTOMObjIUnk);

		if ( pIHTMLTextAreaEl )
			hr = pIHTMLTextAreaEl->get_value( pbstrValue );
	}

	if ( hr == S_OK && !pbstrValue )
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}


//-----------------------------------------------------------------------
//	CEditFieldAO::SetAccValue()
//
//	DESCRIPTION:
//		sets name string of object
//	
//	PARAMETERS:
//
//		lChild		child / self ID 
//		pbstrValue		name string
//
//	RETURNS:
//
//		DISP_E_MEMBERNOTFOUND for base class.
//
// ----------------------------------------------------------------------

HRESULT	CEditFieldAO::SetAccValue( long lChild, BSTR szValue )
{
	HRESULT hr = DISP_E_MEMBERNOTFOUND;


	assert( szValue );


	//------------------------------------------------
	//	The CEditFieldAO can proxy either a TEXTAREA
	//	Trident or an INPUT TYPE=TEXT Trident.
	//	QI first for IHTMLInputTextElement.  If that's
	//	not supported, QI for IHTMLTextAreaElement.
	//------------------------------------------------

	CComQIPtr<IHTMLInputTextElement,&IID_IHTMLInputTextElement> pIHTMLInputTextEl(m_pTOMObjIUnk);

	if ( pIHTMLInputTextEl )
		hr = pIHTMLInputTextEl->put_value( szValue );
	else
	{
		CComQIPtr<IHTMLTextAreaElement,&IID_IHTMLTextAreaElement> pIHTMLTextAreaEl(m_pTOMObjIUnk);

		if ( pIHTMLTextAreaEl )
			hr = pIHTMLTextAreaEl->put_value( szValue );
	}


	return hr;
}



//================================================================================
//	CEditFieldAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CEditFieldAO::isControlDisabled()
//
//	DESCRIPTION:
//
//		Determines whether or not the Trident edit field is enabled.
//	
//	PARAMETERS:
//
//		pbDisabled	set to TRUE is Trident edit field is disabled,
//					FALSE if enabled
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND | standard COM error
//
//-----------------------------------------------------------------------

HRESULT	CEditFieldAO::isControlDisabled( VARIANT_BOOL* pbDisabled )
{
	HRESULT hr = DISP_E_MEMBERNOTFOUND;


	assert( pbDisabled );


	//------------------------------------------------
	//	The CEditFieldAO can proxy either a TEXTAREA
	//	Trident or an INPUT TYPE=TEXT Trident.
	//	QI first for IHTMLInputTextElement.  If that's
	//	not supported, QI for IHTMLTextAreaElement.
	//------------------------------------------------

	CComQIPtr<IHTMLInputTextElement,&IID_IHTMLInputTextElement> pIHTMLInputTextEl(m_pTOMObjIUnk);

	if ( pIHTMLInputTextEl )
		hr = pIHTMLInputTextEl->get_disabled( pbDisabled );
	else
	{
		CComQIPtr<IHTMLTextAreaElement,&IID_IHTMLTextAreaElement> pIHTMLTextAreaEl(m_pTOMObjIUnk);

		if ( pIHTMLTextAreaEl )
			hr = pIHTMLTextAreaEl->get_disabled( pbDisabled );
	}


	return hr;
}

//----  End of EDITFLD.CPP  ----