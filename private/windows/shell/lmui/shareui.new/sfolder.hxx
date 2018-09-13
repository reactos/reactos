//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       sfolder.hxx
//
//  Contents:   Declaration of CSharesSF, an implementation of IShellFolder
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SFOLDER_HXX__
#define __SFOLDER_HXX__

#include "shares.h"

//////////////////////////////////////////////////////////////////////////////

class CSharesSF : public IShellFolder
{
public:

    CSharesSF() {}
    ~CSharesSF() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IShellFolder methods
    //

    STDMETHOD(ParseDisplayName)(
        HWND hwndOwner,
        LPBC pbcReserved,
        LPOLESTR lpszDisplayName,
        ULONG* pchEaten,
        LPITEMIDLIST* ppidl,
        ULONG* pdwAttributes
        );

    STDMETHOD(EnumObjects)(
        HWND hwndOwner,
        DWORD grfFlags,
        LPENUMIDLIST* ppenumIDList
        );

    STDMETHOD(BindToObject)(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        LPVOID* ppvOut
        );

    STDMETHOD(BindToStorage)(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        LPVOID* ppvObj
        );

    STDMETHOD(CompareIDs)(
        LPARAM lParam,
        LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2
        );

    STDMETHOD(CreateViewObject)(
        HWND hwndOwner,
        REFIID riid,
        LPVOID* ppvOut
        );

    STDMETHOD(GetAttributesOf)(
        UINT cidl,
        LPCITEMIDLIST* apidl,
        ULONG* pdwInOut
        );

    STDMETHOD(GetUIObjectOf)(
        HWND hwndOwner,
        UINT cidl,
        LPCITEMIDLIST* apidl,
        REFIID riid,
        UINT* prgfInOut,
        LPVOID* ppvOut
        );

    STDMETHOD(GetDisplayNameOf)(
        LPCITEMIDLIST pidl,
        DWORD uFlags,
        LPSTRRET lpName
        );

    STDMETHOD(SetNameOf)(
        HWND hwndOwner,
        LPCITEMIDLIST pidl,
        LPCOLESTR lpszName,
        DWORD uFlags,
        LPITEMIDLIST* ppidlOut
        );

private:

    //
    // Other
    //

    static
    HRESULT CALLBACK
    _SFVCallBack(
        LPSHELLVIEW psvOuter,
        LPSHELLFOLDER psf,
        HWND hwndOwner,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    int
    _CompareOne(
        DWORD iCol,
        LPIDSHARE pids1,
        LPIDSHARE pids2
        );

};

#endif // __SFOLDER_HXX__
