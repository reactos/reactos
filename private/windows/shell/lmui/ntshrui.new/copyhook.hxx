//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       copyhook.hxx
//
//  Contents:   CShareCopyHook definition
//
//  History:    21-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#ifndef __COPYHOOK_HXX__
#define __COPYHOOK_HXX__

class CShareCopyHook : public ICopyHook
{
    DECLARE_SIG;

public:

    CShareCopyHook();
    ~CShareCopyHook();

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // ICopyHook methods
    //

    STDMETHOD_(UINT,CopyCallback)(
        HWND hwnd,
        UINT wFunc,
        UINT wFlags,
        LPCWSTR pszSrcFile,
        DWORD dwSrcAttribs,
        LPCWSTR pszDestFile,
        DWORD dwDestAttribs
        );

private:

    ULONG           _uRefs;             // OLE reference count
};

#endif // __COPYHOOK_HXX__
