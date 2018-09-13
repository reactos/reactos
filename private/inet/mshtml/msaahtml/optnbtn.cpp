//================================================================================
//		File:	OPTNBTN.CPP
//		Date: 	6/11/97
//		Desc:	Contains the implementation of COptionButtonAO class.
//
//		Author: Arunj
//
//================================================================================

//================================================================================
//	Includes
//================================================================================

#include "stdafx.h"
#include "optnbtn.h"


//================================================================================
//	COptionButtonAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//	COptionButtonAO::COptionButtonAO()
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

COptionButtonAO::COptionButtonAO(CTridentAO * pAOMParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CControlAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{

}


//-----------------------------------------------------------------------
//	COptionButtonAO::~COptionButtonAO()
//
//	DESCRIPTION:
//
//		COptionButtonAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

COptionButtonAO::~COptionButtonAO()
{

}


//================================================================================
//	COptionButtonAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	COptionButtonAO::GetAccState()
//
//	DESCRIPTION:
//
//		returns state of TOM option button
//
//	PARAMETERS:
//		
//		lChild		Child ID
//		plState		long to store returned state var in
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//-----------------------------------------------------------------------
	
HRESULT	COptionButtonAO::GetAccState( long lChild, long* plState )
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
		//	Determine if the option button is checked.
		//
		//	If this determination fails for some reason,
		//	simply ignore the error since we already have
		//	state information from the base class.
		//--------------------------------------------------

		CComQIPtr<IHTMLOptionButtonElement,&IID_IHTMLOptionButtonElement> pIHTMLOptionButtonEl(m_pTOMObjIUnk);

		if ( pIHTMLOptionButtonEl )
		{
			VARIANT_BOOL bChecked = 0;
	
			hr = pIHTMLOptionButtonEl->get_checked( &bChecked );

			if ( hr != S_OK )
				hr = S_OK;
			else
			{
				if ( bChecked )
					*plState |= STATE_SYSTEM_CHECKED;
			}
		}
	}


	return hr;
}



//================================================================================
//	COptionButtonAO protected methods
//================================================================================

//-----------------------------------------------------------------------
//	COptionButtonAO::isControlDisabled()
//
//	DESCRIPTION:
//
//		Determines whether or not the Trident option button (check box,
//		radio button, etc) is disabled.
//	
//	PARAMETERS:
//
//		pbDisabled	set to TRUE if Trident option button is disabled,
//					FALSE if enabled
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND | standard COM error
//
//-----------------------------------------------------------------------

HRESULT	COptionButtonAO::isControlDisabled( VARIANT_BOOL* pbDisabled )
{
	CComQIPtr<IHTMLOptionButtonElement,&IID_IHTMLOptionButtonElement> pIHTMLOptionButtonEl(m_pTOMObjIUnk);

	if ( !pIHTMLOptionButtonEl )
		return DISP_E_MEMBERNOTFOUND;

	return pIHTMLOptionButtonEl->get_disabled( pbDisabled );
}


//----  End of OPTNBTN.CPP  ----