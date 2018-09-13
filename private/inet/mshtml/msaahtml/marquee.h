//================================================================================
//		File:	MARQUEE.H
//		Date: 	7/14/97
//		Desc:	Contains definition of CMarqueeAO class.  CMarqueeAO implements 
//				the accessible proxy for the Trident Marquee object.
//
//		Author: Arunj
//================================================================================

#ifndef __MARQUEEAO__
#define __MARQUEEAO__

//================================================================================
// Includes
//================================================================================

#include "trid_ao.h"

#ifdef _MSAA_EVENTS 

#include "event.h"

#endif


//================================================================================
// Class forwards
//================================================================================

class CDocumentAO;

//================================================================================
// CMarqueeAO class definition.
//================================================================================

class CMarqueeAO : public CTridentAO
{
public:

	//------------------------------------------------
	// IUnknown methods
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	
	virtual HRESULT	GetAccName(long lChild, BSTR * pbstrName);

	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);

	virtual HRESULT	GetAccState(long lChild, long *plState);

	//--------------------------------------------------
	// Constructors/Destructors
	//--------------------------------------------------

	CMarqueeAO(CTridentAO *pAOParent,CDocumentAO *pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CMarqueeAO();

	//--------------------------------------------------
	// Standard class methods
	//--------------------------------------------------

	HRESULT Init(LPUNKNOWN pTOMObjIUnk);


protected:

	//--------------------------------------------------
	// helper methods
	//--------------------------------------------------


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLMarqueeElementEvents,CEvent,DIID_HTMLMarqueeElementEvents)
	
#endif

};

#endif	// __MARQUEEAO__