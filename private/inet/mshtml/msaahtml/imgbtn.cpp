//================================================================================
//		File:	IMGBTN.CPP
//		Date: 	10/29/97
//		Desc:	Contains implementation of CImageButtonAO class.
//				CImageButtonAO implements the accessible proxy for the
//				Trident INPUT TYPE=IMAGE button object.
//
//		Author: AaronD
//
//================================================================================

//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "trid_ao.h"
#include "imgbtn.h"
#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif



//================================================================================
// CImageButtonAO : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CImageButtonAO::CImageButtonAO()
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
//		pDocAO				pointer to the nearest CDocumentAO ancestor
//
//		nTOMIndex			index of the element from the TOM document.all 
//							collection
//
//		nChildID			AOM child ID for the CImageButtonAO
//		
//		hWnd				pointer to the window of the trident object that 
//							this object corresponds to
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CImageButtonAO::CImageButtonAO(CTridentAO * pAOParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CControlAO(pAOParent, pDocAO, nTOMIndex, nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to
	// CImageButtonAO : this member will be overridden
	// in derived class constructors so that the
	// delegating IUnknown will always be at the
	// derived class level.
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

#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("ImageButtonAO"));

#endif
	
}


//-----------------------------------------------------------------------
//	CImageButtonAO::Init()
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

HRESULT CImageButtonAO::Init( IUnknown* pTOMObjIUnk )
{
	HRESULT hr = S_OK;


	assert( pTOMObjIUnk );

	if ( !pTOMObjIUnk )
		return E_INVALIDARG;


	//--------------------------------------------------
	// call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAO::Init( pTOMObjIUnk );

#ifdef _MSAA_EVENTS

	if ( hr == S_OK )
	{

		HRESULT	hrEventInit;

		//--------------------------------------------------
		// initialize event handling interface : establish
		// Advise.
		//--------------------------------------------------
				
		hrEventInit = INIT_EVENT_HANDLER(ImplIHTMLInputImageEvents,m_pIUnknown,m_hWnd,m_nChildID,pTOMObjIUnk)

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
			OutputDebugString( "Event handler initialization in CImageButtonAO::Init() failed.\n" );
#endif
	}

#endif	// _MSAA_EVENTS


	return hr;
}


//-----------------------------------------------------------------------
//	CImageButtonAO::~CImageButtonAO()
//
//	DESCRIPTION:
//
//		CImageButtonAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.					   
//
// ----------------------------------------------------------------------

CImageButtonAO::~CImageButtonAO()
{
}		


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CImageButtonAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CImageButtonAO object only
//		implements IUnknown.
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

STDMETHODIMP CImageButtonAO::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;

    if (riid == IID_IUnknown)  
    {
		*ppv = (IUnknown *)this;
	}

#ifdef _MSAA_EVENTS	
	
	else if (riid == DIID_HTMLInputImageEvents)
	{
		//--------------------------------------------------
		//	This is the event interface for the
		//	CImageButtonAO class.
		//--------------------------------------------------

		ASSIGN_TO_EVENT_HANDLER(ImplIHTMLInputImageEvents,ppv,HTMLInputImageEvents)
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
// CImageButtonAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CImageButtonAO::GetAccName()
//
//	DESCRIPTION:
//
//		lChild		child ID
//		pbstrName	pointer to array to return child name in.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CImageButtonAO::GetAccName(long lChild, BSTR * pbstrName)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrName );


	if ( !m_bNameAndDescriptionResolved )
	{

		//--------------------------------------------------
		// call base class method to insure that image buttons
		// behave like images. 
		//--------------------------------------------------

		if ( (hr = CTridentAO::resolveNameAndDescription()) != S_OK )
			return hr;
	}

	if ( m_bstrName )
		*pbstrName = SysAllocString( m_bstrName );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}
		


//-----------------------------------------------------------------------
//	CImageButtonAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description of image button object.
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

HRESULT	CImageButtonAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
	HRESULT hr = S_OK;
	
	
	assert( pbstrDescription );


	if ( !m_bNameAndDescriptionResolved )
	{
		//--------------------------------------------------
		// call base class method to insure that image buttons
		// behave like images. 
		//--------------------------------------------------

		if ( (hr = CTridentAO::resolveNameAndDescription()) != S_OK )
			return hr;
	}

	if ( m_bstrDescription )
		*pbstrDescription = SysAllocString( m_bstrDescription );
	else
		hr = DISP_E_MEMBERNOTFOUND;


	return hr;
}

	
//-----------------------------------------------------------------------
//	CImageButtonAO::GetAccValue()
//
//	DESCRIPTION:
//
//		returns value of image button object.
//
//	PARAMETERS:
//		
//		lChild		ChildID
//		pbstrValue  string to store value in
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CImageButtonAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
	HRESULT hr = S_OK;


	assert( pbstrValue );


	if ( m_bstrValue )
	{
		*pbstrValue = SysAllocString( m_bstrValue );
	}
	else
	{
		CComQIPtr<IHTMLInputImage,&IID_IHTMLInputImage> pIHTMLInputImage(m_pTOMObjIUnk);

		if ( !pIHTMLInputImage )
			return E_NOINTERFACE;

		//--------------------------------------------------
		// query for the DYNSRC property
		//--------------------------------------------------

		hr = pIHTMLInputImage->get_dynsrc( pbstrValue );

		if ( hr == S_OK )
		{
			if ( *pbstrValue )
				m_bstrValue = SysAllocString( *pbstrValue );
			else
			{
				//--------------------------------------------------
				// since DYNSRC wasn't found, query for SRC
				//--------------------------------------------------

				hr = pIHTMLInputImage->get_src( pbstrValue );

				if ( hr == S_OK )
				{
					if ( *pbstrValue )
						m_bstrValue = SysAllocString( *pbstrValue );
					else
						hr = DISP_E_MEMBERNOTFOUND;
				}
			}
		}
	}


	return hr;
}


//-----------------------------------------------------------------------
//	CImageButtonAO::GetAccState()
//
//	DESCRIPTION:
//
//		state of image button depends on whether it is completely
//		downloaded or not.
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
	
HRESULT	CImageButtonAO::GetAccState(long lChild, long *plState)
{
	HRESULT			hr			= E_FAIL;
	VARIANT_BOOL	bComplete	= 0;
	long			ltempState	= 0;



	assert( plState );


	*plState = 0;


	//--------------------------------------------------
	// Determine if the image has been fully downloaded.
	//--------------------------------------------------

	CComQIPtr<IHTMLInputImage,&IID_IHTMLInputImage> pIHTMLInputImage(m_pTOMObjIUnk);

	if ( !pIHTMLInputImage )
		return E_NOINTERFACE;
	else
	{
		if ( (hr = pIHTMLInputImage->get_complete( &bComplete )) != S_OK )
			return hr;
		else
		{
			if ( !bComplete )
			{
				*plState = STATE_SYSTEM_UNAVAILABLE;
				return hr;
			}
		}
	}


	//--------------------------------------------------
	// Call down to the base class to determine
	// visibility and focus.
	//--------------------------------------------------

	hr = CControlAO::GetAccState( lChild, plState );

	return hr;
}




//================================================================================
//	CImageButtonAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	CImageButtonAO::getDefaultActionString()
//
//	DESCRIPTION:
//
//		Retrieves the default action string for the CImageButtonAO.
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

HRESULT	CImageButtonAO::getDefaultActionString( BSTR *pbstrDefaultAction )
{
	assert( pbstrDefaultAction );
	assert( *pbstrDefaultAction == NULL );

    return GetResourceStringValue(IDS_PRESS_ACTION, pbstrDefaultAction);
}


//-----------------------------------------------------------------------
//	CImageButtonAO::isControlDisabled()
//
//	DESCRIPTION:
//
//		Determines whether or not the INPUT IMAGE button is enabled.
//	
//	PARAMETERS:
//
//		pbDisabled	set to TRUE is the button is disabled,
//					FALSE if enabled
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND | standard COM error
//
//-----------------------------------------------------------------------

HRESULT	CImageButtonAO::isControlDisabled( VARIANT_BOOL* pbDisabled )
{
	HRESULT hr;


	assert( pbDisabled );


	CComQIPtr<IHTMLInputImage,&IID_IHTMLInputImage> pIHTMLInputImage(m_pTOMObjIUnk);

	if ( !pIHTMLInputImage )
		hr = E_NOINTERFACE;
	else
		hr = pIHTMLInputImage->get_disabled( pbDisabled );

	
	return hr;
}



//-----------------------------------------------------------------------
//	CImageButtonAO::getDescriptionString()
//
//	DESCRIPTION:
//
//		Obtains the text of the INPUT TYPE=IMAGE's ALT property to be
//		used as the CImageButtonAO's accDescription.
//
//	PARAMETERS:
//
//		pbstrDescStr	[out]	pointer to the BSTR to hold the ALT text
//
//	RETURNS:
//
//		HRESULT
//
//-----------------------------------------------------------------------

HRESULT CImageButtonAO::getDescriptionString( BSTR* pbstrDescStr )
{
	HRESULT	hr = S_OK;


	assert( pbstrDescStr );


	*pbstrDescStr = NULL;


	CComQIPtr<IHTMLInputImage,&IID_IHTMLInputImage> pIHTMLInputImage(m_pTOMObjIUnk);

	if ( !pIHTMLInputImage )
		hr = E_NOINTERFACE;
	else
		hr = pIHTMLInputImage->get_alt( pbstrDescStr );


	return hr;
}


//----  End of IMGBTN.CPP  ----