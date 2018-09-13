//================================================================================
//		File:	MAP.H
//		Date: 	7/1/97/97
//		Desc:	contains definition of CMapAO class.  CMapAO implements 
//				the accessible proxy for the Trident Map object.
//
//		Author: Arunj
//
//================================================================================

#ifndef __MAPAO__
#define __MAPAO__



//================================================================================
// includes
//================================================================================

#include "trid_ao.h"

//================================================================================
// class forwards
//================================================================================

class CDocumentAO;

//================================================================================
// CTridentAO class definition.
//================================================================================

class CMapAO : public CTridentAO
{
public:

	//------------------------------------------------
	// IUnknown
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	
	//--------------------------------------------------
	// Internal IAccessible support 
	//--------------------------------------------------

	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);

	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);
	
	virtual HRESULT	GetAccState(long lChild, long *plState);

	virtual HRESULT	AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);

	//--------------------------------------------------
	// standard object methods
	//--------------------------------------------------

	CMapAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CMapAO();
	HRESULT Init(IUnknown * pTOMObjIUnk);

};



#endif	// __MAPAO__