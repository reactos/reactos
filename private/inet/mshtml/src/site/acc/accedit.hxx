//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccEdit.hxx
//
//  Contents:   Accessible editbox object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCEDIT_HXX_
#define I_ACCEDIT_HXX_
#pragma INCMSG("--- Beg 'accedit.hxx'")

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

class CAccEdit : public CAccElement
{

public:
    virtual STDMETHODIMP GetAccName( BSTR* pbstrName );
    virtual STDMETHODIMP GetAccState( VARIANT * pvarState );
    virtual STDMETHODIMP GetAccValue( BSTR* pbstrValue );
    virtual STDMETHODIMP GetAccDescription( BSTR* pbstrDescription );
    virtual STDMETHODIMP PutAccValue( BSTR bstrValue);

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccEdit( CElement* pElementParent, BOOL bIsPassword = FALSE );
    
private:
    BOOL    _bIsPassword;       // Is this edit control of type input password?
};

#pragma INCMSG("--- End 'accedit.hxx'")
#else
#pragma INCMSG("*** Dup 'accedit.hxx'")
#endif




