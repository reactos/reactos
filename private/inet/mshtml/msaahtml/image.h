//================================================================================
//      File:   IMAGE.H
//      Date:   7/1/97/97
//      Desc:   contains definition of CImageAO class.  CImageAO implements 
//              the accessible proxy for the Trident Image object.
//
//      Author: Arunj
//
//================================================================================

#ifndef __IMAGEAO__
#define __IMAGEAO__



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
class CMapAO;

//================================================================================
// CTridentAO class definition.
//================================================================================

class CImageAO : public CTridentAO
{
public:

    //------------------------------------------------
    // IUnknown
    //------------------------------------------------

    virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);
    
    //--------------------------------------------------
    // Internal IAccessible support 
    //--------------------------------------------------

    virtual HRESULT GetAccName(long lChild, BSTR * pbstrName);

    virtual HRESULT GetAccValue(long lChild, BSTR * pbstrValue);

    virtual HRESULT GetAccDescription(long lChild, BSTR * pbstrDescription);
    
    virtual HRESULT GetAccState(long lChild, long *plState);

    virtual HRESULT GetAccRole(long lChild, long *plRole);

    virtual HRESULT GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut );

    virtual HRESULT AccDoDefaultAction(long lChild);

    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

    virtual HRESULT GetAccSelection( IUnknown** ppIUnknown );

    virtual HRESULT AccSelect(long flagsSel, long lChild);

    //--------------------------------------------------
    // standard object methods
    //--------------------------------------------------

    CImageAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd);
    ~CImageAO();
    HRESULT Init(IUnknown * pTOMObjIUnk);

    CMapAO * GetMap(void) { return(m_pMapToUse); }
    void SetMap(CMapAO *pMapAO) { m_pMapToUse = pMapAO; }

    IHTMLImgElement* GetIHTMLImgElement( void ) { return m_pIHTMLImgElement; }

    //--------------------------------------------------
    // derived classes that cache Trident iface pointers
    // need to override this method and call down to
    // the base class's method
    //--------------------------------------------------

    virtual void    ReleaseTridentInterfaces ();

protected:

    //--------------------------------------------------
    // protected methods
    //--------------------------------------------------

    virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );

    //--------------------------------------------------
    // data members
    //--------------------------------------------------

    CMapAO*             m_pMapToUse;

    IHTMLImgElement*    m_pIHTMLImgElement;


#ifdef _MSAA_EVENTS

    DECLARE_EVENT_HANDLER(ImplIHTMLImgEvents,CEvent,DIID_HTMLImgEvents)
    
#endif

};



#endif  // __IMAGEAO__