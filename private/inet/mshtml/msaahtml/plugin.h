//================================================================================
//		File:	PLUGIN.H
//		Date: 	7/9/97
//		Desc:	contains definition of CPluginAO class.  CPluginAO implements
//				the Accessible Object proxy for an HTML Plugin element.
//				
//
//		Author: Arunj
//
//================================================================================

#ifndef __PLUGINAO__
#define __PLUGINAO__


//================================================================================
// #includes
//================================================================================

#include "trid_ao.h"

#ifdef _MSAA_EVENTS 

#include "event.h"

#endif

//================================================================================
// CPluginAO class definition
//================================================================================

class CPluginAO: public CTridentAO
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
	// object or are implemented in CPluginAO
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
	
	CPluginAO(CTridentAO * pAOParent,CDocumentAO * pDocAO, UINT nTOMIndex,UINT nChildID,HWND hWnd);
	virtual ~CPluginAO();

	HRESULT Init(IUnknown * pTOMObjIUnk); 

protected:

	enum _PLUGINTYPE
	{
		IOBJECT = 1,
		IEMBED,
		IAPPLET
	};

	int m_iPluginType;
	
	IAccessible * m_pIAccessible;

    BOOL m_bCheckedForHWND;


    BOOL    scrolledIntoViewAlready( void ) { return m_bCheckedForHWND; }
	HRESULT doesPluginSupportIAccessible( void );
    HRESULT createStdAccObjIfVisibleAndHasHWND( IAccessible** ppIAccessible );


	//--------------------------------------------------
	// TODO : move these to a VARIANT abstractor class.
	//--------------------------------------------------

	HRESULT loadVariant(VARIANT * pVar,long lVar);
	HRESULT unpackVariant(VARIANT pVarToUnpack,IUnknown ** ppIUnknown);
	HRESULT unpackVariant(VARIANT pVarToUnpack,IDispatch ** ppIDispatch);
	HRESULT unpackVariant(VARIANT pVarToUnpack,long * plong);

};


#endif	// __PLUGINAO__