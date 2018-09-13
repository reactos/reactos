//================================================================================
//		File:	DIV.CPP
//		Date: 	12/01/97
//		Desc:	Contains implementation of CDivAO class.  CDivAO  
//				implements the accessible proxy for the Trident Div 
//				object.
//				
//		Author: Arun Jacob
//================================================================================


//================================================================================
// Includes
//================================================================================

#include "stdafx.h"
#include "tablcell.h"
#include "div.h"


//================================================================================
// CDivAO implementation: public methods
//================================================================================

//-----------------------------------------------------------------------
//	CDivAO::CDivAO()
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

CDivAO::CDivAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd)
: CTableCellAO(pAOParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
	//------------------------------------------------
	// Assign the delegating IUnknown to CDivAO :
	//  this member will be overridden in derived class
	//  constructors so that the delegating IUnknown 
	//  will always be at the derived class level.
	//------------------------------------------------

	m_pIUnknown	= (LPUNKNOWN)this;									

	//--------------------------------------------------
	// Set the role parameter to be used
	//  in the default CAccElement implementation.
	//--------------------------------------------------

	m_lRole = ROLE_SYSTEM_GROUPING;

	//--------------------------------------------------
	// set the item type so that it can be accessed
	// via base class pointer.
	//--------------------------------------------------

	m_lAOMType = AOMITEM_DIV;

#ifdef _DEBUG

	//--------------------------------------------------
	// Set symbolic name of object for easy identification
	//--------------------------------------------------

	lstrcpy(m_szAOMName,_T("DivAO"));

#endif


}



//-----------------------------------------------------------------------
//	CDivAO::~CDivAO()
//
//	DESCRIPTION:
//
//		CDivAO class destructor.
//
//	PARAMETERS:
//
//	RETURNS:
// ----------------------------------------------------------------------

CDivAO::~CDivAO()
{
}		


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//	CDivAO::QueryInterface()
//
//	DESCRIPTION:
//
//		Standard QI implementation : the CDivAO object only implements
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

STDMETHODIMP CDivAO::QueryInterface(REFIID riid, void** ppv)
{
	if ( !ppv )
		return E_INVALIDARG;


	*ppv = NULL;

    if (riid == IID_IUnknown)  
	{
 		*ppv = (LPUNKNOWN)this;
	}
	else
	{

		//--------------------------------------------------
		// delegate all other QIs to base class.
		//
		// BUGBUG : if events come back to the MSAAHTML
		// codebase, need to investigate 
		// (1) if tablecell tags will support a different
		//  interface than they currently do (HTMLTextContainerEvents)
		// (2) if Div tags will support a different interface than
		// they currently do (again, HTMLTextContainerEvents).
		//--------------------------------------------------

        return(CTableCellAO::QueryInterface(riid,ppv));
	}

	assert( *ppv );
    
	((LPUNKNOWN) *ppv)->AddRef();

    return(NOERROR);
}

//----  End of TABLECELL.CPP  ----
