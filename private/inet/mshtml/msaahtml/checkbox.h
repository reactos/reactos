//================================================================================
//		File:	CHECKBOX.H
//		Date: 	6/11/97
//		Desc:	Contains the definition of the CCheckboxAO class.  This class
//				implements the accessible proxy for the Trident check box control.
//
//		Author: Arunj
//
//================================================================================

#ifndef __CHECKBOXAO__
#define __CHECKBOXAO__


//================================================================================
//	Includes
//================================================================================

#include "optnbtn.h"

#ifdef _MSAA_EVENTS
#include "event.h"
#endif


//================================================================================
//	CCheckboxAO class definition
//================================================================================

class CCheckboxAO: public COptionButtonAO
{
public:

	//--------------------------------------------------
	//	IUnknown 
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);


	//--------------------------------------------------
	//	Internal IAccessible support 
	//--------------------------------------------------
	
	virtual HRESULT	GetAccDefaultAction( long lChild, BSTR* pbstrDefAction );

	//--------------------------------------------------
	//	standard object methods
	//--------------------------------------------------
	
	CCheckboxAO(CTridentAO* pAOParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CCheckboxAO();

	HRESULT Init(IUnknown* pTOMObjIUnk);

	
protected:
	
	//--------------------------------------------------
	// protected helper methods
	//--------------------------------------------------

	virtual HRESULT getDefaultActionString( BSTR* pbstrDefaultAction );


#ifdef _MSAA_EVENTS

	//--------------------------------------------------
	//	Event handler implementation
	//--------------------------------------------------

	DECLARE_EVENT_HANDLER(ImplHTMLOptionButtonElementEvents,CEvent,DIID_HTMLOptionButtonElementEvents)
	
#endif

};


#endif	  // __CHECKBOXAO__