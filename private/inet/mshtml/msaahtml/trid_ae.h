//================================================================================
//      File:   TRID_AE.H
//      Date:   5/21/97
//      Desc:   contains definition of CTridentAE class.  CTridentAE is 
//              the base class for all accessible Trident elements and
//              objects.
//
//      Author: Arunj
//
//================================================================================

#ifndef __TRID_AE__
#define __TRID_AE__


//================================================================================
// #includes
//================================================================================

#include "accelem.h"
#include "accobj.h"


//================================================================================
// class forwards
//================================================================================

class CTridentAO;

//================================================================================
// CTridentAE class definition
//================================================================================

class CTridentAE: public CAccElement
{
    public:

    //--------------------------------------------------
    // IUnknown 
    //
    // these methods are implemented in base classes, 
    // which implement the controlling IUnknowns.
    //--------------------------------------------------
    
    virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv) =0;

    virtual void Detach () { Release(); }
    virtual void Zombify ()
    {
        if ( m_pTOMObjIUnk )
        {
            m_pTOMObjIUnk->Release();
            m_pTOMObjIUnk = NULL;
        }

        ReleaseTridentInterfaces();
    };

    //--------------------------------------------------
    // Internal IAccessible support 
    //--------------------------------------------------
    

    virtual HRESULT GetAccParent(IDispatch ** ppdispParent);

    virtual HRESULT GetAccName(long lChild, BSTR * pbstrName);

    virtual HRESULT GetAccValue(long lChild, BSTR * pbstrValue);

    virtual HRESULT GetAccDescription(long lChild, BSTR * pbstrDescription);
    
    virtual HRESULT GetAccState(long lChild, long *plState);

    virtual HRESULT GetAccHelp(long lChild, BSTR * pbstrHelp);

    virtual HRESULT GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic);
    
    virtual HRESULT GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);
        
    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

    virtual HRESULT AccSelect(long flagsSel, long lChild);

    virtual HRESULT AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
        
    virtual HRESULT AccDoDefaultAction(long lChild);

    virtual HRESULT SetAccName(long lChild, BSTR bstrName);

    virtual HRESULT SetAccValue(long lChild, BSTR bstrValue);


    //--------------------------------------------------
    // object state/selection/manipulation methods.
    //--------------------------------------------------

    HRESULT GetFocusedTOMElementIndex(UINT * puTOMIndexID);
    
    //--------------------------------------------------
    // standard object methods
    //--------------------------------------------------
    
    CTridentAE(CTridentAO * pAOParent,UINT nTOMIndex,UINT nChildID,HWND hWnd);
    virtual ~CTridentAE();

    HRESULT Init(IUnknown * pTOMObjIUnk);

    CTridentAO * GetParent(void) { return(m_pParent); }
    void SetParent(CTridentAO *pNewParentAO) { m_pParent = pNewParentAO; }


    //--------------------------------------------------
    // access method to the TOM all collection index
    //--------------------------------------------------

    long    GetTOMAllIndex() { return m_nTOMIndex; }

    //--------------------------------------------------
    // access method to the TEO's IUnknown pointer
    //--------------------------------------------------

    LPUNKNOWN GetTEOIUnknown() { return m_pTOMObjIUnk; }

    //--------------------------------------------------
    // access method to the TEO's IHTMLElement pointer
    //--------------------------------------------------

    virtual IHTMLElement* GetTEOIHTMLElement( void ) = 0;

protected:
    
    //--------------------------------------------------
    // protected methods
    //--------------------------------------------------

    HRESULT getTitleFromIHTMLElement(BSTR *pbstrTitle);
    HRESULT clickAE(void);
    HRESULT adjustOffsetToRootParent(long * pxLeft,long * pyTop);
    virtual HRESULT adjustOffsetForClientArea(long *pxLeft,long *pyTop);

    //--------------------------------------------------
    // member variables.
    //--------------------------------------------------

    CTridentAO *    m_pParent;          // owner object.
    UINT            m_nTOMIndex;        // all collection index
    IUnknown *      m_pTOMObjIUnk;      // pointer to Trident Object interface.
    BOOL            m_bOffScreen;       // offscreen flag for element : used
                                        // to determine state.
    
    
    //------------------------------------------------
    // Data : IAccessible property storage  : used
    // to store variables that are static for the
    // lifetime of the AE/AO. Initialize these 
    // in derived classes.
    //------------------------------------------------

    BSTR    m_bstrName;
    BSTR    m_bstrValue;
    BSTR    m_bstrDescription;
    BSTR    m_bstrDefaultAction;
    BSTR    m_bstrKbdShortcut;

#ifdef _DEBUG

    //--------------------------------------------------
    // use this string (set in derived constructor) to validate
    // routines at debug time.
    //--------------------------------------------------

    TCHAR m_szAOMName[256];
#endif
    
};


#endif  // __TRID_AE__