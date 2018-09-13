//=======================================================================
//		File:	PRXYMGR.H
//		Date: 	8/7/97
//		Desc:	Contains the definition of the CProxyManager class. CProxyManager
//				implements the Trident proxy manager object for the Trident 
//				MSAA Registered Handler. It is responsible for managing
//				multiple CWindowAOs (which own an entire AOM tree) --
//				one for each instance of Trident
//				running in a separate window.
//		
//		Notes:  The CProxyManager class implements the IAccessibleHandler 
//				interface to handle all client requests for accessible
//				objects via a window handle/OBJID combination. 
//
//		Author: Jay Clark
//=======================================================================

#ifndef __PRXYMGR__
#define __PRXYMGR__

//=======================================================================
// includes
//=======================================================================

#include "trid_ao.h"
#include "window.h"

//=======================================================================
// CProxyManager class definition
//=======================================================================

class CDocumentAO;

class CProxyManager : IAccessibleHandler
{
public:

	//------------------------------------
	// IUnknown interface
	//------------------------------------

	virtual STDMETHODIMP			QueryInterface(REFIID riid, void** ppv);
	virtual STDMETHODIMP_(ULONG)	AddRef(void);
	virtual STDMETHODIMP_(ULONG)	Release(void);

	//------------------------------------
	// IAccessibleHandler interface
	//------------------------------------

	virtual STDMETHODIMP AccessibleObjectFromID(	/* in */	long hWnd, 
													/* in */	long lObjectID, 
													/* out */	LPACCESSIBLE *ppIAccessible );

	//------------------------------------------------
	//	Constructor/destructor
	//------------------------------------------------

	CProxyManager();
	~CProxyManager();

	//--------------------------------------------------
	// window creation
	//--------------------------------------------------

	HRESULT CreateAOMWindow(		/* in */	HWND		hwndToProxy,
									/* in */	CTridentAO		* pParent,
									/* in */	IHTMLElement	* pIHTMLElement,
									/* in */	long			lTEOID,
									/* in */	long			lAOMID,
									/* out */	CWindowAO		**ppWindowAO);

	//--------------------------------------------------
	//	window cleanup
	//--------------------------------------------------
	
	HRESULT DestroyAOMWindow(	/* in */	CWindowAO * pWindowAOToFree); 
    void    RemoveAOMWindow(	/* in */	CWindowAO * pWindowAOToRemove) 
    { 
        if (pWindowAOToRemove) 
            m_windowList.remove(pWindowAOToRemove); 
    };
    void DetachReadyTrees();

	//--------------------------------------------------
	//	Miscellaneous support methods
	//--------------------------------------------------

	HRESULT GetDocumentChildOfWindow( /* in */ HWND hwnd, /* out */ CDocumentAO** ppDocAO );
    void    Zombify () { removeMSAAEventSinks(); };


protected:

	//--------------------------------------------------
	// data members
	//--------------------------------------------------

	ULONG					m_cRef;
	std::list<CWindowAO *>	m_windowList;
	HWINEVENTHOOK			m_hFocusEventHook;
	HWINEVENTHOOK			m_hStateChangeEventHook;
	HMODULE					m_hUserMod;

	//--------------------------------------------------
	// methods
	//--------------------------------------------------

	void freeAllMemory( void );
	BOOL loadUserDLLs( void );
	void unloadUserDLLs( void );
	void setMSAAEventSinks( void );
	void removeMSAAEventSinks( void );
};

#endif	// __PRXYMGR__
