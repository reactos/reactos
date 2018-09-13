//================================================================================
//		File:	AREA.H
//		Date: 	5/21/97
//		Desc:	contains definition of CAreaAO class.  CAreaAO implements 
//		the MSAA proxy for the TOM Area element
//
//		Author: Arunj
//
//================================================================================

#ifndef __AREAAO__
#define __AREAAO__


//================================================================================
// #includes
//================================================================================

#include "trid_ao.h"

#ifdef _MSAA_EVENTS 

#include "event.h"

#endif


//================================================================================
// class forwards
//================================================================================

class CTridentAO;


//================================================================================
// CAreaAO class definition
//================================================================================

class CAreaAO: public CTridentAO
{
public:

	//--------------------------------------------------
	// IUnknown 
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);


	//--------------------------------------------------
	// Internal IAccessible support 
	//--------------------------------------------------
	

	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);

	virtual HRESULT	GetAccValue(long lChild, BSTR * pbstrValue);

	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);
	
	virtual HRESULT	GetAccState(long lChild, long *plState);

	virtual HRESULT	GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

	virtual HRESULT	AccDoDefaultAction(long lChild);

	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);

	virtual HRESULT AccSelect(long flagsSel, long lChild);

	CAreaAO(CTridentAO * pAOMParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CAreaAO();

	HRESULT Init(IUnknown * pTomObjIUnk);


protected:

	//--------------------------------------------------
	// protected methods
	//--------------------------------------------------

	HRESULT focus( void );


	//--------------------------------------------------
	// member variables
	//--------------------------------------------------

#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLAreaEvents,CEvent,DIID_HTMLAreaEvents)
	
#endif

};


#endif	// __AREAAO__