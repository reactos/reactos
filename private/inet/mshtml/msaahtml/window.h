//=======================================================================
//		File:	WINDOW.H
//		Date: 	5/23/97
//		Desc:	contains definition of Window class and member classes.  
//				CWindowAO implements the root window accessible object 
//				for the Trident MSAA Registered Handler. This object
//				contains the CImplAccHandler, which implements the 
//				IAccHandler interface.
//=======================================================================

#ifndef __WINDOWAO__
#define __WINDOWAO__

//================================================================================
// includes
//================================================================================

#include "event.h"
#include "document.h"

//=======================================================================
// class forwards
//=======================================================================

class CAOMMgr;
class CProxyManager;

//=======================================================================
// CWindowAO class definition
//=======================================================================

class CWindowAO: public CTridentAO 
{
public:

	//------------------------------------------------
	//	IUnknown overrides
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);

	//------------------------------------------------
	//	CTridentAO internal IAccessible overrides
	//------------------------------------------------

	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);
	virtual HRESULT GetAccFocus(IUnknown **ppIUnknown);
	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);
	virtual HRESULT	GetAccState(long lChild, long *plState);
	virtual HRESULT GetAccSelection( IUnknown** ppIUnknown );
    virtual HRESULT GetAccParent(IDispatch ** ppdispParent);
	virtual HRESULT	AccSelect(long flagsSel, long lChild);

    virtual void Detach (); 

    //--------------------------------------------------
	// derived classes that cache Trident iface pointers
    // need to override this method and call down to
    // the base class's method
    //--------------------------------------------------

    virtual void    ReleaseTridentInterfaces ();

	//------------------------------------------------
	//	Object construction/destruction.
	//------------------------------------------------

	CWindowAO(CProxyManager * pProxyMgr,CTridentAO * pAOMParent, long nTOMIndex, long nUID, HWND hTridentWnd);
	virtual ~CWindowAO();

	virtual HRESULT	Init(IUnknown * pTOMObjIUnk);
	
	HRESULT GetAccessibleObjectFromID(long lObjectID, CAccElement **ppAE);

	//--------------------------------------------------
	// object member access methods.
	//--------------------------------------------------

	HRESULT GetAOMMgr(CAOMMgr ** ppAOMMgr);
	HRESULT GetDocumentChild( CDocumentAO** ppDocAO );

	//--------------------------------------------------
	// access method to the TEO's IHTMLElement pointer
	//
	// The CWindowAO is associated with a Trident window
	// object not an element object, so return the
	// IHTMLElement* of the CWindowAO's document child.
	//--------------------------------------------------

	virtual IHTMLElement* GetTEOIHTMLElement()
	{
		if ( !m_pDocAO )
			return NULL;
		return m_pDocAO->GetTEOIHTMLElement();
	}

	//--------------------------------------------------
	// object state access methods
	//--------------------------------------------------

	HRESULT IsFocused( LPBOOL pbIsBrowserWindowFocused, LPBOOL pbIsThisWindowFocused );

protected:

	IHTMLWindow2				*m_pIHTMLWindow2;
	CAOMMgr						*m_pAOMMgr;
	CProxyManager				*m_pProxyMgr;
	
	UINT						 m_nMsgHTMLGetObject;

	//------------------------------------------------
	//	Helper methods.
	//------------------------------------------------

	HRESULT	getIHTMLWindow2Ptr( IHTMLWindow2** ppTridentWnd, IHTMLDocument2** ppTridentDoc );
    HRESULT getDocumentFromTridentHost( IUnknown* pUnk, IHTMLDocument2** ppTridentDoc );
	HRESULT	registerHTMLGetObjectMsg( void );
	HRESULT	createMemberObjects(void);
	

	//--------------------------------------------------
	// this macro declares a public Notify() method that
	// is implemented in window.cpp
	//--------------------------------------------------

	DECLARE_NOTIFY_OWNER_EVENT_HANDLER(CWindowAO,ImplIHTMLWindowEvents,CEvent,IID_IDispatch)
};

#endif	// __WINDOWAO__
