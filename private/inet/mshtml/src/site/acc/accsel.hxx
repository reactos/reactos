//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccSel.hxx
//
//  Contents:   Accessible Select object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCSEL_HXX_
#define I_ACCSEL_HXX_
#pragma INCMSG("--- Beg 'accsel.hxx'")

#ifndef X_ACCOBJ_HXX_
#define X_ACCOBJ_HXX_
#include "accobj.hxx"
#endif

class CAccSelect : public CAccObject
{
    typedef CAccObject super;

public:
    virtual STDMETHODIMP get_accState(VARIANT varChild, VARIANT * pvarState );
    virtual STDMETHODIMP get_accDescription(VARIANT varChild, BSTR * pbstrDescription );
    virtual STDMETHODIMP get_accChild( VARIANT varChild, IDispatch ** ppdispChild );
    virtual STDMETHODIMP accSelect( long flagsSel, VARIANT varChild);

    //helper function
    virtual BOOL EnsureAccObject( );
    virtual STDMETHODIMP GetAccName( BSTR* pbstrOut ) { return E_NOTIMPL; }
    virtual STDMETHODIMP GetAccState( VARIANT* pvarState ) { return E_NOTIMPL; }
    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccSelect( CElement* pElementParent );
    ~CAccSelect();
    
protected:
    HRESULT CreateStdAccObj( HWND hWnd, long lObjId, void ** ppAccObj );

};

#pragma INCMSG("--- End 'accanch.hxx'")
#else
#pragma INCMSG("*** Dup 'accanch.hxx'")
#endif



