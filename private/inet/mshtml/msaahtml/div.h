//================================================================================
//		File:	DIV.H
//		Date: 	12/01/97
//		Desc:	Contains definition of CDivAO class.  CDivAO implements 
//				the accessible proxy for the Trident DIV object.
//
//		Author: Arun Jacob
//================================================================================

#ifndef __DIVAO__
#define __DIVAO__

//================================================================================
// Includes
//================================================================================

#include "trid_ao.h"


//================================================================================
// Class forwards
//================================================================================

class CDocumentAO;

//================================================================================
// CTableCellAO class definition.
//================================================================================


//--------------------------------------------------
// BUGBUG : in the Trident world, DIVs are NOT a 
// derivation of Table Cells.  In the MSAAHTML world,
// they are, because the two MSAAHTML proxys share 
// the same MSAA attributes.  The only difference
// between the two is the role, which is overridden
// in the CDivAO constructor.  
// 
// However, this may lead to strange behavior down the
// road -- enforcing a relationship between these two may
// break for DIVs if the tablecell has to do anything that
// is table cell specific.  This is especially true if
// we move events back into MSAAHTML, and the event
// interface supported by tablecells changes in the 
// IE5.0 time frame. 
//
// TODO: investigate deriving CDivAO from something else if
// table cells are not the correct base class.
//--------------------------------------------------

class CDivAO : public CTableCellAO
{
public:

	//------------------------------------------------
	// IUnknown methods
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void **ppv);
	
	//--------------------------------------------------
	// Constructors/destructors
	//--------------------------------------------------

	CDivAO(CTridentAO *pAOParent,CDocumentAO *pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CDivAO();


};

#endif	// __DIVAO__