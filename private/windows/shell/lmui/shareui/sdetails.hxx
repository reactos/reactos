//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       sdetails.hxx
//
//  Contents:   Declaration of CSharesSD, an implementation of IShellDetails
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SDETAILS_HXX__
#define __SDETAILS_HXX__

//////////////////////////////////////////////////////////////////////////////

class CSharesSD : public IShellDetails
{
public:

    CSharesSD(IN HWND hwnd, IN ULONG level)
        : m_ulRefs(0), m_hwnd(hwnd), m_level(level) { AddRef(); }

    ~CSharesSD() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IShellDetails methods
    //

    STDMETHOD(GetDetailsOf)(
            LPCITEMIDLIST pidl,
            UINT iColumn,
            LPSHELLDETAILS pDetails
            );

    STDMETHOD(ColumnClick)(
            UINT iColumn
            );

private:

    // helpers

    STDMETHOD(GetDetailsOf1)(
            LPCITEMIDLIST pidl,
            UINT iColumn,
            LPSHELLDETAILS pDetails
            );

    STDMETHOD(GetDetailsOf2)(
            LPCITEMIDLIST pidl,
            UINT iColumn,
            LPSHELLDETAILS pDetails
            );

    ULONG m_level;
    ULONG m_ulRefs;
    HWND  m_hwnd;
};

#endif // __SDETAILS_HXX__
