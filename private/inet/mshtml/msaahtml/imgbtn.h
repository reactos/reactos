//=======================================================================
//		File:	IMGBTN.H
//		Date: 	10/29/97
//		Desc:	Contains definition of CImageButtonAO class.
//				CImageButtonAO implements the accessible proxy for the
//				Trident INPUT TYPE=IMAGE button object.
//		
//		Author:	AaronD
//
//=======================================================================

#ifndef __IMGBTN__
#define __IMGBTN__

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
							 
class CImageButtonAO: public CControlAO
{
public:

	//--------------------------------------------------
	// IUnknown 
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);


	//--------------------------------------------------
	// Internal IAccessible support 
	//--------------------------------------------------
	
	virtual HRESULT	GetAccName(long lChild, BSTR* pbstrName);

	virtual HRESULT	GetAccValue(long lChild, BSTR* pbstrValue);

	virtual HRESULT	GetAccDescription(long lChild, BSTR* pbstrDescription);

	virtual HRESULT	GetAccState(long lChild, long* plState);

	//------------------------------------------------
	// class init and cleanup methods
	//------------------------------------------------
	
	CImageButtonAO(CTridentAO* pAOParent,CDocumentAO* pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CImageButtonAO();

	HRESULT Init(IUnknown* pTOMObjIUnk);


protected:

	//--------------------------------------------------
	// helper methods
	//--------------------------------------------------

	virtual HRESULT getDefaultActionString( BSTR* pbstrDefaultAction );
	virtual	HRESULT	isControlDisabled( VARIANT_BOOL* pbDisabled );
	virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLInputImageEvents,CEvent,DIID_HTMLInputImageEvents)

#endif

};

#endif	// __IMGBTN__