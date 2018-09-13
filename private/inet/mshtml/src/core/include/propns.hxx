//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       propns.hxx
//
//  Contents:   Notification sink base classes for controls
//
//  Classes:    CPropertyNotifySink
//              CFontSink
//
//  Functions:
//
//  History:    05-Jul-95   SumitC      Created
//
//----------------------------------------------------------------------------

#ifndef I_PROPNS_HXX_
#define I_PROPNS_HXX_
#pragma INCMSG("--- Beg 'propns.hxx'")

#ifdef PRODUCT_97
class CWrappedControl;
#endif  // !PRODUCT_97

//+---------------------------------------------------------------------------
//
//  Class:      CPropertyNotifySink (PNS)
//
//  Purpose:    Implements IPropertyNotifySink to field change notifications
//              from control embedded objects (like the Font object)
//
//  Interface:  QueryInterface  -- as per IUnknown
//              AddRef          -- do
//              Release         -- do
//              OnChanged       -- as per IPropertyNotifySink
//              OnRequestEdit   -- do
//              _pControl       -- weak-ref back to owner
//              _dwCookie       -- Advise cookie
//
//  History:    21-Jun-94   SumitC      Created
//
//  Notes:      This class implements QI, but passes AddRef and Release calls
//              to _pControl's private unknown.
//
//----------------------------------------------------------------------------

class CPropertyNotifySink : public IPropertyNotifySink
{
#ifdef PRODUCT_97
    friend CWrappedControl;
#endif  // !PRODUCT_97

public:
    CPropertyNotifySink(void)
        { _dwCookie = 0; }

    //  IUnknown methods

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv);

    //  IPropertyNotifySink methods

    STDMETHOD(OnChanged) (DISPID dispid);
    STDMETHOD(OnRequestEdit) (DISPID dispid);

    //  Helpers

    HRESULT     Advise(IUnknown * pUnkSource);
    HRESULT     Unadvise(IUnknown * pUnkSource);

    DWORD           _dwCookie;          // advise cookie
};


#ifdef PRODUCT_97  // wrapped-control only
class CFontSink : public CPropertyNotifySink
{
public:
    CFontSink(void)
        { ; }

    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CWrappedControl)

    //  IPropertyNotifySink methods

    STDMETHOD(OnChanged) (DISPID dispid);
};
#endif // PRODUCT_97

#pragma INCMSG("--- End 'propns.hxx'")
#else
#pragma INCMSG("*** Dup 'propns.hxx'")
#endif
