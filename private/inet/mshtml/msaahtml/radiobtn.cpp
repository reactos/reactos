//================================================================================
//		File:	RADIOBTN.CPP
//		Date: 	6/11/97
//		Desc:	Contains the implementation of CRadioButtonAO class.  This class
//				implements the accessible proxy for the Trident radio button control.
//
//		Author: Arunj
//
//================================================================================


//================================================================================
//	Includes
//================================================================================

#include "stdafx.h"
#include "radiobtn.h"

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

BEGIN_EVENT_HANDLER_MAP(CRadioButtonAO,ImplHTMLOptionButtonElementEvents,CEvent)

	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_FOCUS)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONFOCUS,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLELEMENTEVENTS_ONCLICK,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLCONTROLELEMENTEVENTS_ONBLUR,EVENT_OBJECT_STATECHANGE)
	ON_DISPID_FIRE_EVENT(DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE,EVENT_OBJECT_STATECHANGE)
	 
END_EVENT_HANDLER_MAP()

#endif


//================================================================================
//	CRadioButtonAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CRadioButtonAO::CRadioButtonAO()
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

CRadioButtonAO::CRadioButtonAO(CTridentAO* pAOMParent, CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd)
: COptionButtonAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CRadioButtonAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;
	
	//--------------------------------------------------
	// set the role parameter to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_RADIOBUTTON;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_RADIOBUTTON;


#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("RadioButtonAE"));

#endif

}



//-----------------------------------------------------------------------
//	CRadioButtonAO::~CRadioButtonAO()
//
//	DESCRIPTION:
//
//		CRadioButtonAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CRadioButtonAO::~CRadioButtonAO()
{

}


//-----------------------------------------------------------------------
//	CRadioButtonAO::Init()
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

HRESULT CRadioButtonAO::Init( IUnknown* pTOMObjIUnk )
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
			OutputDebugString( "Event handler initialization in CRadioButtonAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//=======================================================================
//	IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CRadioButtonAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CRadioButtonAO object only implements
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

STDMETHODIMP CRadioButtonAO::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;


    if ( riid == IID_IUnknown )
    {
        *ppv = (IUnknown *)m_pIUnknown;
	}

#ifdef _MSAA_EVENTS	
	
	else if ( riid == DIID_HTMLOptionButtonElementEvents )
	{
		//--------------------------------------------------
		//	This is the event interface for CRadioButtonAO.
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
//	CRadioButtonAO Accessible Interface helper methods
//================================================================================


//================================================================================
//	CRadioButtonAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CRadioButtonAO::getDefaultActionString()
//
//	DESCRIPTION:
//
//		Retrieves the default action string for the CRadioButtonAO.
//	
//	PARAMETERS:
//
//		pbstrDefaultAction	BSTR pointer to return the default action in
//
//	RETURNS:
//
//		HRESULT :	S_OK or E_OUTOFMEMORY
//
//	TODO:
//
//		Move strings to string table.
//
//-----------------------------------------------------------------------

HRESULT	CRadioButtonAO::getDefaultActionString( BSTR* pbstrDefaultAction )
{
	assert( pbstrDefaultAction );
	assert( *pbstrDefaultAction == NULL );

    return GetResourceStringValue(IDS_SELECT_ACTION, pbstrDefaultAction);
}



//----  End of RADIOBTN.CPP  ----