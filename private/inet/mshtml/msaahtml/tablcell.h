//================================================================================
//		File:	TABLCELL.H
//		Date: 	7/10/97/97
//		Desc:	Contains definition of CTableCellAO class.  CTableCellAO implements 
//				the accessible proxy for the Trident Table object.
//
//		Author: Jay Clark
//================================================================================

#ifndef __TABLCELL__
#define __TABLCELL__

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
// CTableCellAO class definition.
//================================================================================

class CTableCellAO : public CTridentAO
{
public:

	//------------------------------------------------
	// IUnknown methods
	//------------------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void **ppv);
	
	//--------------------------------------------------
	// Internal IAccessible methods
	//--------------------------------------------------

	virtual HRESULT	GetAccName(long lChild, BSTR *pbstrName);

	virtual HRESULT	GetAccState(long lChild, long *plState);

	//--------------------------------------------------
	// Constructors/destructors
	//--------------------------------------------------

	CTableCellAO(CTridentAO *pAOParent,CDocumentAO *pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CTableCellAO();

	//--------------------------------------------------
	// Standard methods
	//--------------------------------------------------

	HRESULT Init(LPUNKNOWN pTOMObjIUnk);

protected:


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLTextContainerEvents,CEvent,DIID_HTMLTextContainerEvents)
	
#endif

};

#endif	// __TABLCELL__