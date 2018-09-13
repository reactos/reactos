//================================================================================
//		File:	MAP.CPP
//		Date: 	7/1/97
//		Desc:	contains implementation of CMapAO class.  CMapAO
//				implements the accessible proxy for the Trident Map 
//				object.
//
//		Author: Arunj
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "map.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif

//================================================================================
// CMapAO : public methods
//================================================================================

//-----------------------------------------------------------------------
//	CMapAO::CMapAO()
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

CMapAO::CMapAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTridentAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// assign the delegating IUnknown to CMapAO :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown		= (IUnknown *)this;									

	//--------------------------------------------------
	// set the role parameter to be used
	// in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_GROUPING;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_MAP;

#ifdef _DEBUG

	//--------------------------------------------------
	// set this string for debugging use
	//--------------------------------------------------
	lstrcpy(m_szAOMName,_T("MapAO"));

#endif

}


//-----------------------------------------------------------------------
//	CMapAO::Init()
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

HRESULT CMapAO::Init(IUnknown * pTOMObjIUnk)
{
	HRESULT hr	= E_FAIL;

	assert( pTOMObjIUnk );
		
	//--------------------------------------------------
	// call down to base class to set unknown pointer.
	//--------------------------------------------------

	hr = CTridentAO::Init(pTOMObjIUnk);

	return(hr);
}


//-----------------------------------------------------------------------
//	CMapAO::~CMapAO()
//
//	DESCRIPTION:
//
//		CMapAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		None.					   
//
// ----------------------------------------------------------------------

CMapAO::~CMapAO()
{


}		


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//	CMapAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CMapAO object only implements
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

STDMETHODIMP CMapAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return E_INVALIDARG;

	*ppv = NULL;

    if (riid == IID_IUnknown)  
    {
		*ppv = (IUnknown *)this;
	}
	else
	{
		//--------------------------------------------------
		// delegate to base class
		//--------------------------------------------------

        return(CTridentAO::QueryInterface(riid,ppv));
	}
    
	((LPUNKNOWN) *ppv)->AddRef();
    return(NOERROR);

}


//================================================================================
// CMapAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//	CMapAO::GetAccName()
//
//	DESCRIPTION:
//
//	lChild		child ID
//	pbstrName		pointer to array to return child name in.
//
//	PARAMETERS:
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CMapAO::GetAccName(long lChild, BSTR * pbstrName)
{
	HRESULT hr = S_OK;
	

	assert( pbstrName );


	if ( m_bstrName )
	{
		//--------------------------------------------------
		// copy the current name BSTR to the input parameter.
		//--------------------------------------------------

		*pbstrName = SysAllocString( m_bstrName );
	}

	//------------------------------------------------
	// if this var hasn't been created yet, then 
	// set it the first time.
	//------------------------------------------------

	else
	{
		hr = getTitleFromIHTMLElement( pbstrName );

		if ( hr == S_OK )
		{
			if ( *pbstrName )
			{
				m_bstrName = SysAllocString( *pbstrName );
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
		}
		else
			hr = DISP_E_MEMBERNOTFOUND;
	}
			

	return hr;
}



//-----------------------------------------------------------------------
//	CMapAO::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description of image object.
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

HRESULT	CMapAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
	HRESULT hr = S_OK;				  


	assert( pbstrDescription );

	if ( !m_bstrDescription )
	{
		//--------------------------------------------------
		// Until we find something better, synthesize a
		// description string based on name of parent AO,
		// if it exists, and number of children.
		//--------------------------------------------------

		BSTR	bstrParentName = NULL;
		WCHAR	wszDesc[MAX_PATH];


		hr = m_pParent->GetAccName( lChild, &bstrParentName );

		if ( hr == S_OK && bstrParentName )
		{
			//--------------------------------------------------
			// if the parent name exists, add it to the 
			// synthesized string.
			//--------------------------------------------------
            BSTR bstrFormat = NULL;

            hr = GetResourceStringValue(IDS_MAP_DESCRIPTION, &bstrFormat);
            if (!hr)
            {
    			swprintf(wszDesc, bstrFormat, bstrParentName, m_AEList.size());

                SysFreeString(bstrFormat);

    			if ( !(m_bstrDescription = SysAllocString( wszDesc )) )
			    	hr = E_OUTOFMEMORY;
            }

			SysFreeString( bstrParentName );
		}
		else
			return DISP_E_MEMBERNOTFOUND;
	}

	*pbstrDescription = SysAllocString( m_bstrDescription );

	return hr;
}



//-----------------------------------------------------------------------
//	CMapAO::GetAccState()
//
//	DESCRIPTION:
//
//		state of map is conditional on state of image parent : direct
//		call to image parent.
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
	
HRESULT	CMapAO::GetAccState(long lChild, long *plState)
{

	assert(m_pParent);

	//--------------------------------------------------
	// map state is same as parent ImageAO
	//--------------------------------------------------

	return(m_pParent->GetAccState(lChild,plState));

}


//-----------------------------------------------------------------------
//	CMapAO::AccLocation
//
//	DESCRIPTION:
//	
//		This override of the standard AccLocation is required because of 
//		an IE4 bug : the Map element doesn't have a parent, and therefore
//		it breaks our AccLocation logic.  Since a map can correspond to one 
//		and only one image, and shares the size of that image, we map the 
//		maps location to its parent image location.
//
//	PARAMETERS:
//	
//		pxLeft		returns left coord
//		pyTop		returns top coord
//		pcxWidth	returns width coord.
//		pcyHeight	returns height coord.
//		lChild		child ID
//
//	RETURNS:
//		HRESULT S_OK | E_FAIL
//
// ----------------------------------------------------------------------

HRESULT	CMapAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
	
	assert(m_pParent);

	//--------------------------------------------------
	// map location is same as parent ImageAO
	//--------------------------------------------------

	return(m_pParent->AccLocation(pxLeft,pyTop,pcxWidth,pcyHeight,lChild));

}



//----  End of MAP.CPP  ----