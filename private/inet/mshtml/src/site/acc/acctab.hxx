//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccTab.hxx
//
//  Contents:   generic Accessible object definition for elements that are 
//              normally not supported.  If an element has a tabstop specified
//              then it is focusable and will have a focus rect around it as a 
//              user tabs through the page.  Because of this we need to expose 
//              it as an accesible element.  Thus this classs
//
//----------------------------------------------------------------------------
#ifndef I_ACCTAB_HXX_
#define I_ACCTAB_HXX_
#pragma INCMSG("--- Beg 'accTab.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccTabStopped : public CAccElement
{

public:
    //--------------------------------------------------
    //CAccBase methods that are overwritten by this class
    //--------------------------------------------------

    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );
    virtual STDMETHODIMP GetAccValue( BSTR * pbstrValue );
    virtual STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, 
                                                 BSTR* pbstrKeyboardShortcut);
    // get_accRole is set in cTOR
    // get_accHelpTopic: none, use accBase (E_NOTIMPL)
    // get_accHelp : none, use accBase (E_NOTIMPL)
    // the accElement implementation is used for all of the following:
    //--------------------------------------------
    // get_accSelection     get_accFocus 
    // get_accParent        get_accChildCount
    // get_accChild         accHitTest 
    // accDoDefaultAction   accLocation 
    // accSelect            accNavigate
    // put_accName          put_accvalue 
    // GetDefaultAcction 
   
    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccTabStopped ( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'acctab.hxx'")
#else
#pragma INCMSG("*** Dup 'acctab.hxx'")
#endif



