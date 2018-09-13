//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBtn.hxx
//
//  Contents:   Accessible INPUT=BUTTON object definition 
//              this class is used for BUTTON, RESET and SUBMIT 
//
//----------------------------------------------------------------------------
#ifndef I_ACCBTN_HXX_
#define I_ACCBTN_HXX_
#pragma INCMSG("--- Beg 'accbtn.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccButton : public CAccElement
{

public:
    //--------------------------------------------------
    //CAccBase methods that are overwritten by this class
    //--------------------------------------------------

    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );
    virtual STDMETHODIMP GetAccDefaultAction( BSTR* pbstrDefaultAction );

   
    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccButton( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accbtn.hxx'")
#else
#pragma INCMSG("*** Dup 'accbtn.hxx'")
#endif



