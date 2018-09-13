//================================================================================
//		File:	CONTROL.H
//		Date: 	10-16-97
//		Desc:	Defines the CControlAO class.  This is the base class for all
//				control AEs (buttons, check boxes, edit fields, radio buttons).
//				It implements common functionality that the CButtonAE, the
//				CEditFieldAE, and the COptionButtonAE (base for both CCheckboxAE
//				and CRadioButtonAE) classes all share.
//
//		Author: AaronD
//
//================================================================================

#ifndef __CONTROLAO__
#define __CONTROLAO__


//================================================================================
//	Includes
//================================================================================

#include "trid_ao.h"

//================================================================================
//	Class Forwards
//================================================================================

//================================================================================
//	CControlAO class definition
//================================================================================

class CControlAO: public CTridentAO
{
	public:

	//--------------------------------------------------
	//	Internal IAccessible support 
	//--------------------------------------------------
	
	virtual HRESULT	GetAccName( long lChild, BSTR* pbstrName );

	virtual HRESULT	GetAccDescription( long lChild, BSTR* pbstrDescription );

	virtual HRESULT	GetAccState( long lChild, long* plState );

	virtual HRESULT	GetAccDefaultAction( long lChild, BSTR* pbstrDefAction );

	virtual HRESULT	GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut );

	virtual HRESULT	AccDoDefaultAction( long lChild );

	virtual HRESULT GetAccSelection( IUnknown** ppIUnknown );

	virtual HRESULT AccSelect( long flagsSel, long lChild );

	//--------------------------------------------------
	//	standard object methods
	//--------------------------------------------------
	
	CControlAO( CTridentAO* pAOParent, CDocumentAO * pDocAO, UINT nTOMIndex, UINT nChildID, HWND hWnd );
	~CControlAO();


protected:
	
	//--------------------------------------------------
	// protected methods
	//--------------------------------------------------

	virtual	HRESULT	focus( void );
	virtual	HRESULT	getDefaultActionString( BSTR* pbstrDefaultAction );
	virtual	HRESULT	isControlDisabled( VARIANT_BOOL* pbDisabled );

	virtual	HRESULT resolveNameAndDescription( void );
	virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );

	HRESULT getAssociatedLABEL( IHTMLElement** ppIHTMLElementLABEL );
	HRESULT walkLABELCollection( IHTMLElement* pIHTMLElement,
	                             BSTR* pbstrID,
	                             IHTMLElement** ppIHTMLElementLABEL );
	HRESULT getTOMCollection( IHTMLElement* pIHTMLElement,
	                          LPCWSTR lpcstrTagName,
	                          IHTMLElementCollection** ppTagsCollection );

	//--------------------------------------------------
	// member data
	//--------------------------------------------------

	BOOL	m_bCacheNameAndDescription;

};


#endif // __CONTROLAO__
