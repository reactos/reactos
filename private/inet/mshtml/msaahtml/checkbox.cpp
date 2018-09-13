//================================================================================
//		File:	CHECKBOX.CPP
//		Date: 	6/11/97
//		Desc:	Contains the implementation of CCheckboxAO class.  This class
//				implements the accessible proxy for the Trident check box control.
//
//		Author: Arunj
//
//================================================================================

//================================================================================
//	Includes
//================================================================================

#include "stdafx.h"
#include "checkbox.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

#ifdef _MSAA_EVENTS

//================================================================================
//	Event map implementation
//================================================================================

BEGIN_EVENT_HANDLER_MAP(CCheckboxAO,ImplHTMLOptionButtonElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE,EVENT_OBJECT_STATECHANGE)
	 
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
//	CCheckboxAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CCheckboxAO::CCheckboxAO()
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

CCheckboxAO::CCheckboxAO(CTridentAO* pAOMParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd)
: COptionButtonAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CCheckboxAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown = (IUnknown *)this;


	//--------------------------------------------------
	// set the role parameter to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_CHECKBUTTON;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_CHECKBOX;


#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("CheckboxAE"));

#endif
	
}



//-----------------------------------------------------------------------
//	CCheckboxAO::~CCheckboxAO()
//
//	DESCRIPTION:
//
//		CCheckboxAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CCheckboxAO::~CCheckboxAO()
{

}


//-----------------------------------------------------------------------
//	CCheckboxAO::Init()
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

HRESULT CCheckboxAO::Init( IUnknown* pTOMObjIUnk )
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
		HRESULT hrEventInit;

		//--------------------------------------------------
		//	Allocate event handling interface and establish
		//	Advise.
		//--------------------------------------------------

		hrEventInit = INIT_EVENT_HANDLER(ImplHTMLOptionButtonElementEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

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
			OutputDebugString( "Event handler initialization in CCheckboxAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//=======================================================================
//	IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CCheckboxAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CCheckboxAO object only implements
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
//-----------------------------------------------------------------------

STDMETHODIMP CCheckboxAO::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;


    if (riid == IID_IUnknown)  
    {
        *ppv = (IUnknown *)m_pIUnknown;
	}

#ifdef _MSAA_EVENTS	
	
	else if ( riid == DIID_HTMLOptionButtonElementEvents )
	{
		//--------------------------------------------------
		//	This is the event interface for the
		//	CCheckboxAO class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplHTMLOptionButtonElementEvents,ppv,HTMLOptionButtonElementEvents)
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
	//	If *ppv not set by now, it hasn't been found.
	//--------------------------------------------------

	if ( *ppv == NULL )
	{
        return E_NOINTERFACE;
	}


	((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
}

//================================================================================
//	CCheckboxAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CCheckboxAO::GetAccDefaultAction()
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
//		HRESULT :	S_OK if success, E_FAIL if fail, DISP_E_MEMBERNOTFOUND
//					for no implement.
//
//	NOTE:
//
//		The default action of the CCheckboxAO depends upon the check
//		box's current state.  The simplest way to handle this is to
//		override CControlAO::GetAccDefaultAction() to prevent the default
//		action string from being cached.
//
//	TODO:
//
//		Provide caching of the default action string and use the state
//		change as the dirty trigger.
//
//-----------------------------------------------------------------------

HRESULT	CCheckboxAO::GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
{
	HRESULT hr;


	assert( pbstrDefAction );
	assert( *pbstrDefAction == NULL );

	hr = getDefaultActionString( pbstrDefAction );

#ifdef _DEBUG
	if ( hr != S_OK )
	{
		assert( *pbstrDefAction == NULL );
	}
#endif

	return hr;
}



//================================================================================
//	CCheckboxAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CCheckboxAO::getDefaultActionString()
//
//	DESCRIPTION:
//
//		Retrieves the default action string for the CCheckboxAO.
//	
//	PARAMETERS:
//
//		pbstrDefaultAction	BSTR pointer to return the default action in
//
//	RETURNS:
//
//		HRESULT :	S_OK, DISP_E_MEMBERNOTFOUND, E_OUTOFMEMORY, or
//					some other standard COM error code as returned
//					by Trident.
//
//	NOTES:
//
//		If the Trident check box is checked, the default action is to
//		"Uncheck" it; otherwise, the default action is to "Check" it.
//
//	TODO:
//
//		Move strings to string table.
//
//-----------------------------------------------------------------------

HRESULT	CCheckboxAO::getDefaultActionString( BSTR* pbstrDefaultAction )
{
	HRESULT			hr = DISP_E_MEMBERNOTFOUND;
	VARIANT_BOOL	bChecked = 0;


	assert( pbstrDefaultAction );
	assert( *pbstrDefaultAction == NULL );


	CComQIPtr<IHTMLOptionButtonElement,&IID_IHTMLOptionButtonElement> pCheckboxEl(m_pTOMObjIUnk);

	if ( pCheckboxEl )
	{
		hr = pCheckboxEl->get_checked( &bChecked );

		if ( hr == S_OK )
		{
			// UNDONE: Localize 

			if ( bChecked )
			{
                hr = GetResourceStringValue(IDS_UNCHECK_ACTION, pbstrDefaultAction);
			}
			else
			{
                hr = GetResourceStringValue(IDS_CHECK_ACTION, pbstrDefaultAction);
			}
		}
	}

	return hr;
}


//----  End of CHECKBOX.CPP  ----