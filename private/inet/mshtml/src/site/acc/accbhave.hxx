//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBhave.hxx
//
//  Contents:   Accessible object definition for binary behaviors.
//
//----------------------------------------------------------------------------
#ifndef I_ACCBHAVE_HXX_
#define I_ACCBHAVE_HXX_
#pragma INCMSG("--- Beg 'accbhave.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccBehavior : public CAccElement
{
typedef CAccElement super;

public:
    virtual STDMETHODIMP get_accChildCount(long* pChildCount);
    virtual STDMETHODIMP get_accChild(VARIANT varChild, IDispatch ** ppdispChild);
    virtual STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut);
    virtual STDMETHODIMP get_accFocus(VARIANT * pvarFocusChild);
    virtual STDMETHODIMP get_accSelection(VARIANT * pvarSelectedChildren);
    virtual STDMETHODIMP accSelect( long flagsSel, VARIANT varChild);
    virtual STDMETHODIMP accLocation(   long* pxLeft, long* pyTop, 
                                        long* pcxWidth, long* pcyHeight, 
                                        VARIANT varChild);
    virtual STDMETHODIMP accNavigate( long navDir, VARIANT varStart, VARIANT * pvarEndUpAt );
    virtual STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
    virtual STDMETHODIMP accDoDefaultAction(VARIANT varChild);
    virtual STDMETHODIMP get_accName(VARIANT varChild, BSTR* pbstrOut );
    virtual STDMETHODIMP get_accState(VARIANT varChild, VARIANT * pvarState );
    virtual STDMETHODIMP get_accValue(VARIANT varChild, BSTR * pbstrValue );
    virtual STDMETHODIMP get_accDescription(VARIANT varChild, BSTR * pbstrDescription );
    virtual STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);
    virtual STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR * pbstrDefaultAction );

    //helper function
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut ) { return E_NOTIMPL; }
    virtual STDMETHODIMP GetAccState( VARIANT* pvarState ) { return E_NOTIMPL; }

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccBehavior( CElement* pElementParent );
    
private:

};

#pragma INCMSG("--- End 'accbhave.hxx'")
#else
#pragma INCMSG("*** Dup 'accbhave.hxx'")
#endif
