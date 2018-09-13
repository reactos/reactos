//================================================================================
//		File:	RADIOBTN.H
//		Date: 	6/11/97
//		Desc:	Contains the definition of the CRadioButtonAO class.  This class
//				implements the accessible proxy for the Trident radio button control.
//
//		Author: Arunj
//
//================================================================================

#ifndef __RADIOBTN__
#define __RADIOBTN__


//================================================================================
//	Includes
//================================================================================

#include "optnbtn.h"

#ifdef _MSAA_EVENTS
#include "event.h"
#endif


//================================================================================
//	CRadioButtonAO class definition
//================================================================================

class CRadioButtonAO: public COptionButtonAO
{
public:

	//--------------------------------------------------
	//	IUnknown interface definition
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface( REFIID riid, void** ppv );


	//--------------------------------------------------
	//	Internal IAccessible support 
	//--------------------------------------------------
	
	//--------------------------------------------------
	//	Standard object methods
	//--------------------------------------------------
	
	CRadioButtonAO( CTridentAO* pAOParent, CDocumentAO * pDocAO, UINT nTOMIndex, UINT nChildID, HWND hWnd );
	~CRadioButtonAO();

	HRESULT Init( IUnknown* pTOMObjIUnk );
  
	
protected:
	
	//--------------------------------------------------
	//	Protected helper methods
	//--------------------------------------------------

	virtual HRESULT getDefaultActionString( BSTR* pbstrDefaultAction );


#ifdef _MSAA_EVENTS

	//--------------------------------------------------
	//	Event handler declaration
	//--------------------------------------------------

	DECLARE_EVENT_HANDLER(ImplHTMLOptionButtonElementEvents,CEvent,DIID_HTMLOptionButtonElementEvents)

#endif

};


#endif	  // __RADIOBTN__