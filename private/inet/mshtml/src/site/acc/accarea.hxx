//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccArea.hxx
//
//  Contents:   Accessible Area object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCAREA_HXX_
#define I_ACCAREA_HXX_
#pragma INCMSG("--- Beg 'accarea.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif


class CAccArea : public CAccElement
{

public:
    virtual STDMETHODIMP get_accParent(IDispatch ** ppdispParent);

    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccValue( BSTR* pbstrValue);
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription);
    virtual STDMETHODIMP GetAccDefaultAction( BSTR* pbstrDefaultAction);
    virtual STDMETHODIMP accLocation(   long* pxLeft, long* pyTop, 
                                        long* pcxWidth, long* pcyHeight, 
                                        VARIANT varChild);

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccArea( CElement* pElementParent );
};

#pragma INCMSG("--- End 'accarea.hxx'")
#else
#pragma INCMSG("*** Dup 'accarea.hxx'")
#endif
