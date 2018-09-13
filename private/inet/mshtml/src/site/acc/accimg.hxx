//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       accimg.hxx
//
//  Contents:   Accessible Image object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCIMG_HXX_
#define I_ACCIMG_HXX_
#pragma INCMSG("--- Beg 'accimg.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif


class CAccImage : public CAccElement
{

public:
    virtual STDMETHODIMP get_accChildCount(long* pChildCount);
    virtual STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccValue( BSTR * pbstrValue );
    virtual STDMETHODIMP GetAccDescription( BSTR * pbstrDescription );
    virtual STDMETHODIMP GetAccDefaultAction( BSTR * pbstrDefaultAction );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccImage( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accimg.hxx'")
#else
#pragma INCMSG("*** Dup 'accimg.hxx'")


#endif

