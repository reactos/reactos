//================================================================================
//		File:	EDITFLD.H
//		Date: 	6/1/97
//		Desc:	Contains the definition of the CEditFieldAO class.  This class
//				implements the proxy for the Trident Edit Field control.
//
//		Author: Arunj
//
//================================================================================

#ifndef __EDITFLD__
#define __EDITFLD__


//================================================================================
//	Includes
//================================================================================

#include "control.h"

#ifdef _MSAA_EVENTS 
#include "event.h"
#endif


//================================================================================
//	CEditFieldAO class definition
//================================================================================

class CEditFieldAO: public CControlAO
{
public:

	//--------------------------------------------------
	//	IUnknown Interface
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface( REFIID riid, void** ppv );


	//--------------------------------------------------
	//	Internal IAccessible support 
	//--------------------------------------------------

	virtual HRESULT	GetAccValue( long lChild, BSTR* pbstrValue );

	virtual HRESULT	GetAccDefaultAction( long lChild, BSTR* pbstrDefAction )
	{
		return DISP_E_MEMBERNOTFOUND;
	}

	virtual HRESULT	SetAccValue( long lChild, BSTR szValue );


	//--------------------------------------------------
	//	Standard object methods
	//--------------------------------------------------
	
	CEditFieldAO( CTridentAO* pAOMParent, CDocumentAO * pDocAO, UINT nTOMIndex, UINT nUID, HWND hTridentWnd );
	virtual ~CEditFieldAO();
	
	HRESULT Init( IUnknown* pTOMObjIUnk );


protected:

	//--------------------------------------------------
	//	Protected helper methods
	//--------------------------------------------------

	virtual	HRESULT	isControlDisabled( VARIANT_BOOL* pbDisabled );


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLInputTextElementEvents,CEvent,DIID_HTMLInputTextElementEvents)
	
#endif

};

#endif	//__EDITFLD__