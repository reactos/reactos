//================================================================================
//		File:	SOUND.H
//		Date: 	7/14/97
//		Desc:	contains definition of CSoundAE class.  CSoundAE implements 
//				the proxy for an HTML Sound element
//
//		Author: Arunj
//
//================================================================================

#ifndef __SOUND__
#define __SOUND__


//================================================================================
// #includes
//================================================================================

#include "trid_ae.h"


//================================================================================
// CSoundAE class definition
//================================================================================

class CSoundAE: public CTridentAE
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
	// Internal IAccessible support 
	//--------------------------------------------------

	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);

	virtual HRESULT	GetAccValue(long lChild, BSTR * pbstrValue);

	virtual HRESULT	GetAccState(long lChild, long *plState);

	virtual HRESULT	AccSelect(long flagsSel, long lChild);

	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
		
	//--------------------------------------------------
	// standard object methods
	//--------------------------------------------------
	
	CSoundAE(CTridentAO * pAOParent,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	virtual ~CSoundAE();

	HRESULT Init(IUnknown * pTOMObjIUnk);
};


#endif	// __SOUND__