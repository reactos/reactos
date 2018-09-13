//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.hxx
//
//  Contents:   Class factory, etc, for all OLE objects:
//              CShare and CShareCopyHook
//
//  History:    6-Apr-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __OLE_HXX__
#define __OLE_HXX__

//////////////////////////////////////////////////////////////////////////////

class CShareCF : public IClassFactory
{
public:

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IClassFactory methods
    //

    STDMETHOD(CreateInstance)(
            IUnknown* pUnkOuter,
            REFIID riid,
            LPVOID* ppvObj);

    STDMETHOD(LockServer)(BOOL fLock);
};

//////////////////////////////////////////////////////////////////////////////

class CShareCopyHookCF : public IClassFactory
{
public:

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IClassFactory methods
    //

    STDMETHOD(CreateInstance)(
            IUnknown* pUnkOuter,
            REFIID riid,
            LPVOID* ppvObj);

    STDMETHOD(LockServer)(BOOL fLock);
};

#endif // __OLE_HXX__
