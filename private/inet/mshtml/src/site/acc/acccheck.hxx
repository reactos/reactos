//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccCheck.hxx
//
//  Contents:   Accessible INPUT TYPE=CheckBox object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCCHECK_HXX_
#define I_ACCCHECK_HXX_
#pragma INCMSG("--- Beg 'acccheck.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccCheckbox : public CAccElement
{

public:

    virtual STDMETHODIMP GetAccName( BSTR* pbstrName );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );
    virtual STDMETHODIMP GetAccDefaultAction( BSTR* pbstrDefaultAction );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccCheckbox( CElement* pElementParent );
   
};

#pragma INCMSG("--- End 'acccheck.hxx'")
#else
#pragma INCMSG("*** Dup 'acccheck.hxx'")
#endif


