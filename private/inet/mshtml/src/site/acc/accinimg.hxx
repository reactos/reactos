//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccInImg.hxx
//
//  Contents:   Accessible INPUT=IMG object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCINIMG_HXX_
#define I_ACCINIMG_HXX_
#pragma INCMSG("--- Beg 'accinimg.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccInputImg : public CAccElement
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
    CAccInputImg( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accinimg.hxx'")
#else
#pragma INCMSG("*** Dup 'accinimg.hxx'")
#endif



