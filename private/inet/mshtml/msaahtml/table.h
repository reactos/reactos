//================================================================================
//		File:	TABLE.H
//		Date: 	7/10/97/97
//		Desc:	Contains definition of CTableAO class.  CTableAO implements 
//				the accessible proxy for the Trident Table object.
//
//		Author: Jay Clark
//================================================================================

#ifndef __TABLEAO__
#define __TABLEAO__

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
// CTableAO class definition.
//================================================================================

class CTableAO : public CTridentAO
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

	virtual HRESULT	GetAccDescription(long lChild, BSTR * pbstrDescription);

	virtual HRESULT	GetAccState(long lChild, long *plState);

	//--------------------------------------------------
	// Constructors/destructors
	//--------------------------------------------------

	CTableAO(CTridentAO *pAOParent,CDocumentAO *pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
	~CTableAO();

	//--------------------------------------------------
	// Standard methods
	//--------------------------------------------------

	HRESULT Init(LPUNKNOWN pTOMObjIUnk);

protected:

	virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );


#ifdef _MSAA_EVENTS

	DECLARE_EVENT_HANDLER(ImplIHTMLElementEvents,CEvent,DIID_HTMLElementEvents)
	
#endif

};

#endif	// __TABLEAO__