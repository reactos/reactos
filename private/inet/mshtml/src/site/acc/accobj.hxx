//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccObj.hxx
//
//  Contents:   Accessible Object/Plugin/Embed object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCOBJ_HXX_
#define I_ACCOBJ_HXX_
#pragma INCMSG("--- Beg 'accobj.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif


class CAccObject : public CAccElement
{
typedef CAccElement super;

public:
    STDMETHOD (PrivateQueryInterface)(REFIID, void **);

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
    virtual STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR * pbstrDefaultAction );
    virtual STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);

    //IOleWindow Methods
    STDMETHODIMP  GetWindow( HWND* phwnd );
    STDMETHODIMP  ContextSensitiveHelp( BOOL fEnterMode );


    //helper function
    virtual BOOL EnsureAccObject( );
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut ) { return E_NOTIMPL; }
    virtual STDMETHODIMP GetAccState( VARIANT* pvarState ) { return E_NOTIMPL; }

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccObject( CElement* pElementParent );
    
    ~CAccObject();

protected: 
    IAccessible *   _pAccObject;    // IAccessible interface pointer to the IAccessible 
                                    // implementation of the object/embed/plugin
};

#pragma INCMSG("--- End 'accobj.hxx'")
#else
#pragma INCMSG("*** Dup 'accobj.hxx'")
#endif
