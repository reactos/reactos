//================================================================================
//		File:	OPTNBTN.H
//		Date: 	6/11/97
//		Desc:	Contains definition of COptionButtonAO class.
//				COptionButtonAO is the base class for the CRadioButtonAE and
//				CCheckboxAE classes, and is derived from CControlAO.
//				It implements common functionality that both classes share.
//
//		Author: Arunj
//
//================================================================================

#ifndef __OPTNBTN__
#define __OPTNBTN__


//================================================================================
//	Includes
//================================================================================

#include "control.h"


//================================================================================
//	COptionButtonAO class definition
//================================================================================

class COptionButtonAO: public CControlAO
{
public:

	//--------------------------------------------------
	// Internal IAccessible support 
	//--------------------------------------------------
	
	virtual HRESULT	GetAccState( long lChild, long* plState );

	//--------------------------------------------------
	// standard object methods
	//--------------------------------------------------
	
	COptionButtonAO(CTridentAO* pAOParent, CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~COptionButtonAO();

protected:
	
	//--------------------------------------------------
	// protected helper methods
	//--------------------------------------------------

	virtual	HRESULT	isControlDisabled( VARIANT_BOOL* pbDisabled );

};


#endif	  // __OPTNBTN__