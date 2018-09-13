//================================================================================
//		File:	Frame.H
//		Date: 	7/27/97
//		Desc:	Contains definition of CFrameAO class.  CFrameAO implements 
//				the accessible proxy for the Trident Table object.
//================================================================================

#ifndef __FrameAO__
#define __FrameAO__

//================================================================================
// Includes
//================================================================================
#include "trid_ao.h"
#include "window.h"

//================================================================================
// Class forwards
//================================================================================

class CWindowAO;
class CImplIUnknown;

//================================================================================
// CFrameAO class definition.
//================================================================================

class CFrameAO : public CWindowAO
{
public:

	//------------------------------------------------
	// IUnknown methods
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	
	//--------------------------------------------------
	// Internal IAccessible methods
	//--------------------------------------------------

	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);
	virtual HRESULT GetAccFocus(IUnknown **ppIUnknown);
	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);
	virtual HRESULT	GetAccState(long lChild, long *plState);
	virtual HRESULT GetAccSelection( IUnknown** ppIUnknown );

	virtual HRESULT AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown);
	virtual HRESULT AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown);
	virtual HRESULT	GetAccParent(IDispatch ** ppdispParent);
	virtual HRESULT	GetAccChildCount(long* pChildCount);
	virtual HRESULT	GetAccChild(long lChild, IDispatch ** ppdispChild);
	virtual HRESULT	GetAccValue(long lChild, BSTR * pbstrValue);
	virtual HRESULT	GetAccHelp(long lChild, BSTR * pbstrHelp);
	virtual HRESULT	GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic);
	virtual HRESULT	GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);
	virtual HRESULT	GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);
	virtual HRESULT	AccSelect(long flagsSel, long lChild);
	virtual HRESULT	AccDoDefaultAction(long lChild);
	virtual HRESULT	SetAccName(long lChild, BSTR bstrName);
	virtual HRESULT	SetAccValue(long lChild, BSTR bstrValue);


	//--------------------------------------------------
	// Constructors/Destructors
	//--------------------------------------------------

	CFrameAO(CProxyManager * pProxyMgr,CTridentAO *pAOParent,UINT nTOMIndex,UINT nChildID);
	~CFrameAO();

	//--------------------------------------------------
	// Standard class methods
	//--------------------------------------------------

	HRESULT Init(IUnknown *pTOMObjIUnk);

	//--------------------------------------------------
	// access method to the TEO's IHTMLElement pointer
	//--------------------------------------------------

	virtual IHTMLElement* GetTEOIHTMLElement() { return m_pIHTMLElement; }


    virtual BOOL DoBlockForDetach( void );

    virtual void Zombify();

protected:

    //--------------------------------------------------
    // methods
    //--------------------------------------------------

    HRESULT createIUnknownImplementor( void );

	HRESULT findTridentWindow(	/* in */	HWND hWndParent,
								/* out */	HWND * phWndFound);

	HRESULT initializeFrame(void);
	
	virtual HRESULT getVisibleCorner(POINT * pPt,DWORD * pdwCorner);

    void    cleanupMemberData( void );
    HRESULT reInit( void );

    //--------------------------------------------------
    // data
    //--------------------------------------------------

    CImplIUnknown*  m_pImplIUnknown;
};

#endif	// __FrameAO__