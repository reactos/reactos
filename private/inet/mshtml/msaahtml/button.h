//=======================================================================
//		File:	BUTTON.H
//		Date: 	6/1/97
//		Desc:	contains definition of CButtonAO class.  CButtonAO 
//				implements the accessible proxy for the Trident Button 
//				object.
//		
//		Author:	Arunj
//
//=======================================================================

#ifndef __BUTTONAO__
#define __BUTTONAO__

//================================================================================
// includes
//================================================================================

#include "control.h"

#ifdef _MSAA_EVENTS 
#include "event.h"
#endif

//================================================================================
// class forwards
//================================================================================

class CTridentAO;

//=======================================================================
// CButtonAO class definition
//=======================================================================
							 
class CButtonAO: public CControlAO
{
public:

	//--------------------------------------------------
	// IUnknown 
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);


	//--------------------------------------------------
	// Internal IAccessible support 
	//--------------------------------------------------
	
	virtual HRESULT	GetAccState( long lChild, long* plState );

	//------------------------------------------------
	// class init and cleanup methods
	//------------------------------------------------
	
	CButtonAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CButtonAO();

	HRESULT Init(IUnknown * pTOMObjIUnk);
	BOOL IsInputTypeButton(void) { return(m_bIsInputButtonType); }


protected:

	//--------------------------------------------------
	// helper methods
	//--------------------------------------------------

	virtual HRESULT getDefaultActionString( BSTR* pbstrDefaultAction );
	virtual	HRESULT	isControlDisabled( VARIANT_BOOL* pbDisabled );

	virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );
	
	//--------------------------------------------------
	// member variables
	//--------------------------------------------------
	
	BOOL m_bIsInputButtonType;					// toggles whether tag == INPUT
	
#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLButtonElementEvents,CEvent,DIID_HTMLButtonElementEvents)

#endif

};

#endif	// __BUTTONAO__