//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       rcomp.hxx
//
//  Contents:   Declaration of CSharesRC, an implementation of IRemoteComputer
//
//  History:    7-Jan-96    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __RCOMP_HXX__
#define __RCOMP_HXX__

//////////////////////////////////////////////////////////////////////////////

class CSharesRC : public IRemoteComputer
{
public:

    CSharesRC() {}
    ~CSharesRC() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IRemoteComputer methods
    //

    STDMETHOD(Initialize)(
        LPWSTR pszMachine,
        BOOL bEnumerating
        );

};

#endif // __RCOMP_HXX__
