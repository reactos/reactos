//================================================================================
//		File:	SELECT.H
//		Date: 	7/17/97
//		Desc:	contains definition of CSelectAO class.  CSelectAO implements
//				the Accessible Object proxy for an HTML Select element.
//				
//
//		Author: Arunj
//
//================================================================================

#ifndef __SELECTAO__
#define __SELECTAO__


//================================================================================
// #includes
//================================================================================

#include "trid_ao.h"

#ifdef _MSAA_EVENTS 

#include "event.h"

#endif


//================================================================================
// CSelectAO class definition
//================================================================================

class CSelectAO: public CTridentAO
{
	public:

	//--------------------------------------------------
	// IUnknown 
	//
	// these methods are implemented in derived classes, 
	// which implement the controlling IUnknowns.
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	
	//--------------------------------------------------
	// Internal IAccessible support : 
	// these methods either delegate to the embedded 
	// object or are implemented in CSelectAO
	//--------------------------------------------------

	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);

	virtual HRESULT	GetAccValue(long lChild, BSTR * pbstrValue);

	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);

	virtual HRESULT	GetAccState(long lChild, long *plState);

	virtual HRESULT	GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

	virtual HRESULT	AccDoDefaultAction(long lChild);

	//--------------------------------------------------
	// these methods either delegate to the embedded 
	// object or delegate to the base CTridentAO methods
	//--------------------------------------------------

	virtual HRESULT	GetAccChildCount(long* pChildCount);
	
	virtual HRESULT	GetAccChild(long lChild, IDispatch ** ppdispChild);

	virtual HRESULT	GetAccHelp(long lChild, BSTR * pbstrHelp);

	virtual HRESULT	GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic);
	
	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);

	virtual HRESULT	GetAccRole(long lChild, long *plRole);

	virtual HRESULT GetAccFocus(IUnknown **ppIUnknown);

	virtual HRESULT GetAccSelection(IUnknown **ppIUnknown);
	
	virtual HRESULT AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown);
	
	virtual HRESULT AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown);


	//--------------------------------------------------
	// these methods either delegate to the embedded
	// object or return not implemented.
	//--------------------------------------------------

	virtual HRESULT	GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);
	
	virtual HRESULT	AccSelect(long flagsSel, long lChild);

	virtual HRESULT	SetAccName(long lChild, BSTR bstrName);

	virtual HRESULT	SetAccValue(long lChild, BSTR bstrValue);

	//--------------------------------------------------
	// standard object methods
	//--------------------------------------------------
	
	CSelectAO(CTridentAO * pAOParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd);
	virtual ~CSelectAO();

	HRESULT Init(IUnknown * pTOMObjIUnk); 

	protected:

	IAccessible * m_pIAccessible;
	
	HRESULT createStdAccessibleObjectIfVisible(IAccessible ** ppIAccessible);

    HRESULT getObjectId( long* plObjId );

	//--------------------------------------------------
	// TODO: move these to a VARIANT abstractor class.
	//--------------------------------------------------

	HRESULT loadVariant(VARIANT * pVar,long lVar);
	HRESULT unpackVariant(VARIANT pVarToUnpack,IUnknown ** ppIUnknown);
	HRESULT unpackVariant(VARIANT pVarToUnpack,IDispatch ** ppIDispatch);
	HRESULT unpackVariant(VARIANT pVarToUnpack,long * plong);


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLSelectElementEvents,CEvent,DIID_HTMLSelectElementEvents)
	
#endif

};


#endif	// __SELECTAO__