//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccRadio.hxx
//
//  Contents:   Accessible radio button object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCRADIO_HXX_
#define I_ACCRADIO_HXX_
#pragma INCMSG("--- Beg 'accradio.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccRadio : public CAccElement
{

public:
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );
    virtual STDMETHODIMP GetAccDefaultAction( BSTR* pbstrDefaultAction );


    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccRadio( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accradio.hxx'")
#else
#pragma INCMSG("*** Dup 'accradio.hxx'")
#endif



