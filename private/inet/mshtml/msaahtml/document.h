//=======================================================================
//		File:	DOCUMENT.H
//		Date: 	5/23/97
//		Desc:	contains definition of CDocument class.  CDocumentAO 
//				implements the Document accessible object for the Trident 
//				MSAA Registered Handler.
//=======================================================================

#ifndef __DOCUMENTAO__
#define __DOCUMENTAO__

//================================================================================
// #includes
//================================================================================

#ifdef _MSAA_EVENTS

#include "event.h"

#endif

//================================================================================
// Defines
//================================================================================

//================================================================================
// CDocumentAO class definition
//================================================================================

class CDocumentAO: public CTridentAO
{

public:

	//--------------------------------------------------
	// IUnknown 
	//--------------------------------------------------
	
	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	
	//--------------------------------------------------
	// Internal IAccessible support methods
	//--------------------------------------------------

	virtual HRESULT	AccLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, long lChild);

	virtual HRESULT	AccSelect(long flagsSel, long lChild);

	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);
	
	virtual HRESULT	GetAccState(long lChild, long *plState);

	virtual HRESULT GetAccSelection(IUnknown **ppIUnknown);

	virtual HRESULT	GetAccValue(long lChild, BSTR * pbstrValue);

	//--------------------------------------------------
	// A CDocumentAO object can be created in an
	// uninitialized CWindowAO object : the CWindowAO
	// has no hWnd, no valid TOM Index.  Unless 
	// explicitly set otherwise, the constructor will
	// assume that it is being created as part of an
	// uninitialized CWindowAO object.
	//--------------------------------------------------

	CDocumentAO(CTridentAO * pAOParent,
		UINT nTOMIndex = DUMMY_SOURCEINDEX,
		UINT nChildID = DUMMY_SOURCEINDEX,
		HWND hWnd = NULL);

	~CDocumentAO();

	HRESULT Init(HWND hWndToProxy,long lSourceIndex,IUnknown *pTOMObjIUnk);

	HRESULT Init(IUnknown * pTOMObjIUnk);

    //--------------------------------------------------
	// derived classes that cache Trident iface pointers
    // need to override this method and call down to
    // the base class's method
    //--------------------------------------------------

    virtual void    ReleaseTridentInterfaces ();

	//--------------------------------------------------
	// selection helper methods
	//--------------------------------------------------

	HRESULT	IsTEOSelected( IHTMLElement* pIHTMLElement, BOOL* pbIsSelected );
	HRESULT	IsTextRangeSelected( IHTMLTxtRange* pIHTMLTxtRange, BOOL* pbIsSelected );
	HRESULT	IsTOMSelectionNonEmpty( BOOL* pbHasSelection );

	//--------------------------------------------------
	// TOM interaction helper methods
	//--------------------------------------------------

	HRESULT	Focus( void );
	HRESULT	Focus( IHTMLDocument2* pIHTMLDocument2 );

	HRESULT GetFocusedItem(UINT *pnTOMIndex);
	HRESULT GetActiveElementIndex(UINT *pnTOMIndex);
	
	HRESULT GetScrollOffset(POINT * pPtScrollOffset);
	
	HRESULT ElementFromPoint(POINT *pPtHitTest,IHTMLElement **ppIHTMLElement);


	virtual IHTMLElement* GetTEOIHTMLElement( void ) { return m_pDocIHTMLElement; }


	void NotifySoundElementExist(void);

	virtual HRESULT GetAOMMgr(CAOMMgr ** ppAOMMgr);

    void    SetReadyToDetach ( /* in */ BOOL bFlag) 
                { m_bTreeReadyToDetach= bFlag; };
    BOOL    CanDetachTreeNow () 
                { return m_bTreeReadyToDetach; };

	void	SetFocusedItem( long idObject )
				{ m_lFocusedTOMObj = idObject; m_bReceivedFocus = TRUE; }

	HRESULT GetURL(BSTR * pbstrURL);

	BOOL	IsTOMDocumentReady( void )
				{ return m_bTOMDocComplete; }

protected:

	//--------------------------------------------------
	// protected methods
	//--------------------------------------------------

    HRESULT getIHTMLSelectionObject( void );
    HRESULT getSelectionTextRange( IHTMLTxtRange** ppIHTMLTxtRangeSel );
    HRESULT getIHTMLElementParentOfCurrentSelection( IHTMLElement** ppElem );

	HRESULT getReadyState( IHTMLDocument2* pIHTMLDocument2, BOOL* pbComplete );

	HRESULT callInvokeForLong( IDispatch* pDisp, DISPID dispID, long* plData );

	//--------------------------------------------------
	// data members
	//--------------------------------------------------

    IHTMLSelectionObject*   m_pSelectionObj;
    IHTMLTxtRange*          m_pTextRangeTEO;
    BOOL                  	m_bSoundElementExist;
    BOOL                    m_bTreeReadyToDetach;
	long					m_lFocusedTOMObj;
	BOOL					m_bReceivedFocus;
	BOOL					m_bTOMDocComplete;

	IHTMLDocument2*			m_pIHTMLDocument2;
	IHTMLElement*			m_pDocIHTMLElement;
	IHTMLTextContainer*		m_pIHTMLTextContainer;

#ifdef _MSAA_EVENTS

	//--------------------------------------------------
	// event specific protected methods
	//--------------------------------------------------

	HRESULT initDocumentEventHandlers( void );

	//--------------------------------------------------
	// event interface for Document
	//--------------------------------------------------

    DECLARE_EVENT_HANDLER(ImplIHTMLDocumentEvents,CEvent,DIID_HTMLDocumentEvents)

	//--------------------------------------------------
	// event interface for Body
	//--------------------------------------------------						 

	DECLARE_EVENT_HANDLER(ImplIDispIHTMLBodyElement,CEvent,DIID_HTMLTextContainerEvents)

#endif

};


#endif	// __DOCUMENTAO__