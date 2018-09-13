//================================================================================
//		File:	ACCOBJ.H
//		Date: 	5/30/97
//		Desc:	contains definition of CAccObject class.
//				CAccObject is the abstract base class for all 
//				accessible 	objects
//
//		Author: Arunj
//
//================================================================================

#ifndef __ACCOBJ__
#define __ACCOBJ__


//================================================================================
// CAccObject class definition 
//================================================================================

class CAccObject	: public CAccElement
{
public:

	//------------------------------------------------
	// IUnknown
	//------------------------------------------------

	virtual STDMETHODIMP		QueryInterface(REFIID riid, void** ppv)		=0;

	//--------------------------------------------------
	// internal IAccessible implementation
	// helper methods.
	//--------------------------------------------------

	virtual HRESULT	GetAccChildCount(long* pChildCount)						=0;
	virtual HRESULT	GetAccChild(long lChild, IDispatch ** ppdispChild)		=0;	
	virtual HRESULT GetAccFocus(IUnknown **ppIUnknown)						=0;
	virtual HRESULT GetAccSelection(IUnknown **ppIUnknown)					=0;
	virtual HRESULT AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown)	=0;
	virtual HRESULT AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown)	=0;
	
	//--------------------------------------------------------------------------------
	// standard object methods.
	//--------------------------------------------------------------------------------
	
	CAccObject(long ChildID,HWND hWnd,BOOL bUnSupportedTag = FALSE):
	CAccElement(ChildID,hWnd,bUnSupportedTag)
	{

	}

	virtual ~CAccObject() {}

};



#endif  // __ACCOBJ__