//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccTable.hxx
//
//  Contents:   Accessible Table, table cell, and table header object definitions
//
//----------------------------------------------------------------------------
#ifndef I_ACCTABLE_HXX_
#define I_ACCTABLE_HXX_
#pragma INCMSG("--- Beg 'acctable.hxx'")


#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CAccTable : public CAccElement
{
public:
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccDescription( BSTR * pbstrDescription );

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccTable( CElement* pElementParent );
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CAccTableCell : public CAccElement
{
public:
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    
    CAccTableCell( CElement* pElementParent );
};

#pragma INCMSG("--- End 'acctable.hxx'")
#else
#pragma INCMSG("*** Dup 'acctable.hxx'")
#endif



