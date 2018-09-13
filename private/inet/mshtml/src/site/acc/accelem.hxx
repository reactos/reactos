//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) MICrosoft Corporation, 1994-1998
//
//  File:       AccElem.hxx
//
//  Contents:   Accessible element object definition.
//              
//----------------------------------------------------------------------------
#ifndef I_ACCELEM_HXX_
#define I_ACCELEM_HXX_
#pragma INCMSG("--- Beg 'accelem.hxx'")

#ifndef X_ACCBASE_HXX_
#define X_ACCBASE_HXX_
#include "accbase.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

// Forward declarations
//---------------------------------
class CElement;
class CTreeNode;
class CLabelElement;

//
//  CLASS   :   CAccElem
//
class CAccElement
: public CAccBase
{
public:
    virtual STDMETHODIMP_(ULONG) PrivateAddRef();
    virtual STDMETHODIMP_(ULONG) PrivateRelease();

    //IAccessible Methods
    virtual STDMETHODIMP get_accParent(IDispatch ** ppdispParent);
    
    virtual STDMETHODIMP get_accChildCount(long* pChildCount);
    
    virtual STDMETHODIMP get_accChild(VARIANT varChild, IDispatch ** ppdispChild);
    
    virtual STDMETHODIMP get_accName(VARIANT varChild, BSTR* pbstrName);
    
    virtual STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pbstrValue);

    virtual STDMETHODIMP get_accDescription(VARIANT varChild, BSTR* pbstrDescription);

    virtual STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);

    virtual STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState); 

//  NYI... CAccBase implementation returns E_NOTIMPL.
//  virtual STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pbstrHelp);
//  virtual STDMETHODIMP get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic);

    virtual STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, 
                                                    BSTR* pbstrKeyboardShortcut);

    virtual STDMETHODIMP get_accFocus(VARIANT * pvarFocusChild);

    virtual STDMETHODIMP get_accSelection(VARIANT * pvarSelectedChildren);

    virtual STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction);

    virtual STDMETHODIMP accSelect( long flagsSel, VARIANT varChild);

    virtual STDMETHODIMP accLocation(   long* pxLeft, long* pyTop, 
                                        long* pcxWidth, long* pcyHeight, 
                                                            VARIANT varChild);

    virtual STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);

    virtual STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);

    virtual STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    virtual STDMETHODIMP put_accValue( VARIANT varChild, BSTR bstrValue );

    //return the CDoc that the object is in.
    virtual CDoc* GetAccDoc(){ return _pElement->Doc(); }

    //Virtual helpers for CAccElement class derivatives
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut ) = 0;
    virtual STDMETHODIMP GetAccState( VARIANT* pvarState ) = 0;
    virtual STDMETHODIMP GetAccValue( BSTR* pbstrVal ){ return E_NOTIMPL; }
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription ){ return E_NOTIMPL; }
    virtual STDMETHODIMP GetAccDefaultAction( BSTR* pbstrDefaultAction ){ return E_NOTIMPL; }
    virtual STDMETHODIMP PutAccValue( BSTR bstrValue){return E_NOTIMPL; }

    CAccElement( CElement* pElementParent );

    virtual ~CAccElement();

    // other helper functions
    //-------------------------------------------------------
    virtual HRESULT GetEnumerator( IEnumVARIANT ** ppEnum);
        
    HRESULT GetLabelorTitle( BSTR* pbstrOutput );
    HRESULT GetTitleorLabel( BSTR* pbstrOutput );
    BOOL    HasLabel();
    CLabelElement * GetLabel();

    HRESULT HitTestArea( CMessage *pMsg,    
                         CTreeNode *pHitNode,
                         VARIANT * pvarChildAtPoint);
    HRESULT GetTitle( BSTR* pbstrOutput );

    virtual CElement* GetElement(){ return _pElement; }
    CAccBase        * GetParentAnchor();
    BOOL              fBrowserWindowHasFocus();


    HRESULT GetChildFromID( long                lChildId, 
                            CAccBase **         ppAccChild, 
                            CMarkupPointer *    pMarkupStart,
                            long *              plChildCnt = NULL);

    HRESULT GetSelectedChildren(    long cChildBase, 
                                    CMarkupPointer *pStart, 
                                    CMarkupPointer * pEnd, 
                                    VARIANT * pvarSelectedChildren,
                                    BOOL    bForceEnumerator = FALSE );

    HRESULT GetNavChildId( long navDir, CElement * pChildElement, long * pChildId);

    BOOL    CanHaveChildren();
    BOOL    IsCacheValid();
    BOOL    IsVisibleRect( CRect * pRectRegion );
    BOOL    IsTextVisible( CMarkupPointer * pTextBegin, CMarkupPointer * pTextEnd);

protected:
//--------------------------------------------------
//  Member Data
//--------------------------------------------------
    CElement        *_pElement;     // pointer to the HTML element we are hanging off.
    

// [FerhanE]
// Use a bitfield, if we decide to add more flags .
    BOOL            _fOffScreen;        // offscreen flag for element : used to determine state. 

    CMarkupPointer  _lastChild;
    long            _lLastChild;
    long            _lTreeVersion;
    long            _lChildCount;
};


inline BOOL
CAccElement::CanHaveChildren( )
{
    if ( _pElement->IsNoScope() ||
        (_pElement->Tag() == ETAG_BUTTON ) ||
        (_pElement->Tag() == ETAG_TEXTAREA ) ||
        (_pElement->Tag() == ETAG_SELECT ) )
    {
        return FALSE;   // can not have children
    }
    else
        return TRUE;    // can have children
}

inline BOOL
CAccElement::IsCacheValid()
{
    return ( _lLastChild && 
                _pElement->Doc() && 
                ( _lTreeVersion == _pElement->Doc()->GetDocTreeVersion() ) );
}

#pragma INCMSG("--- End 'accelem.hxx'")
#else
#pragma INCMSG("*** Dup 'accelem.hxx'")
#endif

