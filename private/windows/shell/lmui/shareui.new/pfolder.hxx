//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       pfolder.hxx
//
//  Contents:   Declaration of CSharesPF, an implementation of IPersistFolder
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __PFOLDER_HXX__
#define __PFOLDER_HXX__

//////////////////////////////////////////////////////////////////////////////

class CSharesPF : public IPersistFolder
{
public:

    CSharesPF() {}
    ~CSharesPF() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IPersist methods
    //

    STDMETHOD(GetClassID)(
        LPCLSID lpClassID
        );

    //
    // IPersistFolder methods
    //

    STDMETHOD(Initialize)(
        LPCITEMIDLIST pidl
        );

};

#endif // __PFOLDER_HXX__
