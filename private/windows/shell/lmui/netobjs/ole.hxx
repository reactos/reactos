//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.hxx
//
//  Contents:   Class factory, etc, for all OLE objects
//
//  History:    25-Sep-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __OLE_HXX__
#define __OLE_HXX__

//////////////////////////////////////////////////////////////////////////////

class CNetObjCF : public IClassFactory
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

#endif // __OLE_HXX__
