//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccFrame.hxx
//
//  Contents:   Accessible Frame object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCFRAME_HXX_
#define I_ACCFRAME_HXX_
#pragma INCMSG("--- Beg 'accframe.hxx'")

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

class CAccElement;

class CAccFrame : public CAccWindow
{

public:
    typedef CAccWindow  super;

    //--------------------------------------------------
    //Methods that are overwritten by this class
    //--------------------------------------------------
    virtual STDMETHODIMP get_accParent(IDispatch ** ppdispParent);
    virtual STDMETHODIMP accLocation(   long* pxLeft, long* pyTop, 
                                        long* pcxWidth, long* pcyHeight, 
                                        VARIANT varChild);

    virtual STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccFrame( CDoc* pDocInner, CElement * pFrameElement);

    HRESULT GetParentOfFrame( CAccElement ** ppAccParent);
    
protected:
    CElement * _pFrameElement;  //The element pointer to the frame element. This is 
                                //needed to climb up the parent chain.
};

#pragma INCMSG("--- End 'accframe.hxx'")
#else
#pragma INCMSG("*** Dup 'accframe.hxx'")
#endif


