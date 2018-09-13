//================================================================================
//      File:   ANCHOR.H
//      Date:   7/1/97
//      Desc:   contains definition of CAnchorAO class.  CAnchorAO implements
//              the proxy for the Trident Anchor HTML Element
//
//      Author: Arunj
//
//================================================================================

#ifndef __ANCHORAO__
#define __ANCHORAO__



//================================================================================
// includes
//================================================================================

#include "trid_ao.h"

#ifdef _MSAA_EVENTS 

#include "event.h"

#endif

//================================================================================
// class forwards
//================================================================================

class CDocumentAO;

//================================================================================
// CAnchorAO class definition.
//================================================================================

class CAnchorAO : public CTridentAO
{
public:

    //------------------------------------------------
    // IUnknown
    //------------------------------------------------

    virtual STDMETHODIMP        QueryInterface(REFIID riid, void** ppv);

    
    //--------------------------------------------------
    // Internal IAccessible support 
    //--------------------------------------------------
    

    virtual HRESULT GetAccName(long lChild, BSTR * pbstrName);

    virtual HRESULT GetAccDescription(long lChild, BSTR * pbstrDescription);

    virtual HRESULT GetAccValue(long lChild, BSTR * pbstrValue);

    virtual HRESULT GetAccState(long lChild, long *plState);

    virtual HRESULT GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);
    
    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

    virtual HRESULT AccSelect(long flagsSel, long lChild);

    virtual HRESULT AccDoDefaultAction(long lChild);


    CAnchorAO(CTridentAO * pAOParent,CDocumentAO * pAODoc,UINT nTOMIndex,UINT nChildID,HWND hWnd);
    virtual ~CAnchorAO();
    virtual HRESULT Init(IUnknown * pTOMObjIUnk);


protected:

    //--------------------------------------------------
    // helper methods
    //--------------------------------------------------

    virtual HRESULT resolveNameAndDescription( void );

#ifdef _MSAA_EVENTS

    DECLARE_EVENT_HANDLER(ImplIHTMLAnchorEvents,CEvent,DIID_HTMLAnchorEvents)
    
#endif


};



#endif  // __ANCHORAO__