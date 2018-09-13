//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccMarq.hxx
//
//  Contents:   Accessible Marquee object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCMARQ_HXX_
#define I_ACCMARQ_HXX_
#pragma INCMSG("--- Beg 'accmarq.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccMarquee : public CAccElement
{

public:

    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccMarquee( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accanch.hxx'")
#else
#pragma INCMSG("*** Dup 'accanch.hxx'")
#endif



