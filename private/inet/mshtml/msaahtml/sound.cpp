//================================================================================
//		File:	SOUND.CPP
//		Date: 	7/9/97
//		Desc:	contains implementation of CSoundAE class.  CSoundAE implements
//				the Accessible Element proxy for an HTML Plugin element.
//
//		Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "trid_ao.h"
#include "document.h"
#include "sound.h"


//================================================================================
// CSoundAE class implementation : public methods
//================================================================================



//-----------------------------------------------------------------------
//	CSoundAE::CSoundAE()
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

CSoundAE::CSoundAE(CTridentAO * pAOMParent,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAE(pAOMParent,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CSoundAE :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;
	
	m_lRole			= ROLE_SYSTEM_SOUND;
	
}



//-----------------------------------------------------------------------
//	CSoundAE::~CSoundAE()
//
//	DESCRIPTION:
//
//		CSoundAE class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CSoundAE::~CSoundAE()
{
		
}


//-----------------------------------------------------------------------
//	CSoundAE::Init()
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

HRESULT CSoundAE::Init(IUnknown * pTOMObjIUnk)
{

	HRESULT hr = E_FAIL;

	assert( pTOMObjIUnk );

	//--------------------------------------------------
	// call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAE::Init(pTOMObjIUnk);
	

	if(hr != S_OK)
		return(hr);


	//--------------------------------------------------
	// notify the parent document that a sound element
	// exists 
	//--------------------------------------------------

	assert( m_pParent );

	CDocumentAO * pDoc = m_pParent->GetDocumentAO();

	assert( pDoc );

	pDoc->NotifySoundElementExist();


	return(S_OK);
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CSoundAE::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CSoundAE object only implements
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

STDMETHODIMP CSoundAE::QueryInterface( REFIID riid, void** ppv )
{
	if ( !ppv )
		return E_INVALIDARG;

	*ppv = NULL;

    if ( riid == IID_IUnknown )
    {
        *ppv = (IUnknown *)m_pIUnknown;

		((LPUNKNOWN) *ppv)->AddRef();

	    return NOERROR;
	}
	else
        return E_NOINTERFACE;
}


//================================================================================
// CSoundAE Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CSoundAE::GetAccName()
//
//	DESCRIPTION:
//
//	lChild			child ID
//	pbstrName		pointer to array to return child name in.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CSoundAE::GetAccName( long lChild, BSTR* pbstrName )
{
	HRESULT hr;
	
	
	assert( pbstrName );


	if ( m_bstrName )
		*pbstrName = SysAllocString( m_bstrName );
	else
	{
		hr = getTitleFromIHTMLElement( pbstrName );

		if ( hr == S_OK )
		{
			if ( !*pbstrName )
				hr = DISP_E_MEMBERNOTFOUND;
			else
				m_bstrName = SysAllocString( *pbstrName );
		}
	}


	return hr;
}

//-----------------------------------------------------------------------
//	CSoundAE::GetAccValue(long lChild, BSTR * pbstrValue)
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
//		HRESULT :	S_OK if success, S_FALSE if fail, DISP_E_MEMBERNOTFOUND
//					for no implement.
//
// ----------------------------------------------------------------------

HRESULT	CSoundAE::GetAccValue(long lChild, BSTR * pbstrValue)
{
	HRESULT hr = S_OK;


	assert( pbstrValue );


	if( m_bstrValue )
	{
		*pbstrValue = SysAllocString( m_bstrValue );
	}
	else
	{
		CComQIPtr<IHTMLBGsound,&IID_IHTMLBGsound> pIHTMLBGsound(m_pTOMObjIUnk);

		if ( !pIHTMLBGsound )
			return E_NOINTERFACE;

		hr = pIHTMLBGsound->get_src( pbstrValue );

		if ( hr == S_OK )
		{
			if ( !*pbstrValue )
				hr = DISP_E_MEMBERNOTFOUND;
			else
				m_bstrValue = SysAllocString( *pbstrValue );
		}
	}

	return hr;
}



//-----------------------------------------------------------------------
//	CSoundAE::GetAccState()
//
//	DESCRIPTION:
//
//		returns state of area : always STATE_SYSTEM_OFFSCREEN for CSoundAE.
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
	
HRESULT	CSoundAE::GetAccState(long lChild, long *plState)
{
	*plState = STATE_SYSTEM_OFFSCREEN;

	return(S_OK);

}


//-----------------------------------------------------------------------
//	CSoundAE::AccSelect()
//
//	DESCRIPTION:
//		override default implementation (AccSelect() is not implemented for
//		the CSoundAE object)
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
//		S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CSoundAE::AccSelect(long flagsSel, long lChild)
{
	return(DISP_E_MEMBERNOTFOUND);
}

//-----------------------------------------------------------------------
//	CSoundAE::AccLocation()
//
//	DESCRIPTION:
//		override default implementation : AccLocation() is not implemented
//		for CSoundAE.
//
//	PARAMETERS:
//
//		pxLeft		left screen coordinate
//		pyTop		top screen coordinate
//		pcxWidth	screen width of object
//		pcyHeight	screen height of object
//		lChild		child/self ID
//
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CSoundAE::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
	return(DISP_E_MEMBERNOTFOUND);
}


//----  End of SOUND.CPP  ----
