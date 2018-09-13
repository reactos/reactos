//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBody.hxx
//
//  Contents:   Accessible Body element definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCBODY_HXX_
#define I_ACCBODY_HXX_
#pragma INCMSG("--- Beg 'accbody.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif


class CAccBody : public CAccElement
{
public:

    virtual STDMETHODIMP get_accParent( IDispatch ** ppdispParent );
    virtual STDMETHODIMP get_accSelection(VARIANT * pvarSelectedChildren);
        
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccValue( BSTR* pbstrValue );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccBody( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'accbody.hxx'")
#else
#pragma INCMSG("*** Dup 'accbody.hxx'")
#endif
