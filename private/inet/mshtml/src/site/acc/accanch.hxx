//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccAnch.hxx
//
//  Contents:   Accessible Anchor object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCANCH_HXX_
#define I_ACCANCH_HXX_
#pragma INCMSG("--- Beg 'accanch.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccAnchor : public CAccElement
{

public:
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccValue( BSTR * pbstrValue );
    virtual STDMETHODIMP GetAccDescription( BSTR * pbstrDescription );
    virtual STDMETHODIMP GetAccDefaultAction( BSTR * pbstrDefaultAction );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccAnchor( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accanch.hxx'")
#else
#pragma INCMSG("*** Dup 'accanch.hxx'")
#endif


