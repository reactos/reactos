//+---------------------------------------------------------------------------
//
//  Microsoft Forms3
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       extibh.hxx
//
//  Contents:   Definition of IDispatchEx for expando property support.
//
//----------------------------------------------------------------------------

#ifndef __EXTOBJ_HXX_
#define __EXTOBJ_HXX_


// {A0AAC450-A77B-11cf-91D0-00AA00C14A7C}
//DEFINE_GUID(IID_IDispatchEx, 0xa0aac450, 0xa77b, 0x11cf, 0x91, 0xd0, 0x0, 0xaa, 0x0, 0xc1, 0x4a, 0x7c);
EXTERN_C const IID IID_IDispatchEx;


//-----------------------------------------------------------------------------
// IDispatchEx::GetIDsOfNamesEx grfdex parameter flags:
//
enum
{
    fdexNil = 0x00,             // empty
    fdexDontCreate = 0x01,      // don't create if non-existant otherwise do
    fdexInitNull = 0x02,        // init new expando as VT_NULL otherwise VT_EMPTY
    fdexCaseSensitive = 0x04,   // match names as case sensitive
};

////
//
// This is the interface for extensible IDispatch objects.
//

class IDispatchEx : public IDispatch
{
public:
    // Get dispID for names, with options
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNamesEx(
            REFIID riid,
            LPOLESTR *prgpsz,
            UINT cpsz,
            LCID lcid,
            DISPID *prgid,
            DWORD grfdex) = 0;

    // Enumerate dispIDs and their associated "names".
    // Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
    // error code if the call fails.
    virtual HRESULT STDMETHODCALLTYPE GetNextDispID(
            DISPID id,
            DISPID *pid,
            BSTR *pbstrName) = 0;
};


#endif // __EXTOBJ_HXX_
