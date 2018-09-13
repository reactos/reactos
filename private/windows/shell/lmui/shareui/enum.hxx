//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       enum.hxx
//
//  Contents:   Implementation of IEnumIDList
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __ENUM_HXX__
#define __ENUM_HXX__

#include "shares.h"

//////////////////////////////////////////////////////////////////////////////

class CSharesEnum : public IEnumIDList
{
public:

    CSharesEnum(
        IN PWSTR pszMachine,
        IN DWORD level
        );

    HRESULT
    Init(
        ULONG uFlags
        );

    ~CSharesEnum();

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IEnumIDList methods
    //

    STDMETHOD(Next)(
        ULONG celt,
        LPITEMIDLIST* rgelt,
        ULONG* pceltFetched
        );

    STDMETHOD(Skip)(
        ULONG celt
        );

    STDMETHOD(Reset)(
        VOID
        );

    STDMETHOD(Clone)(
        IEnumIDList** ppenum
        );


private:

    ULONG           m_uFlags;
    SHARE_INFO_2*   m_pShares;  // may actually point to level 1 info
    PWSTR           m_pszMachine;
    ULONG           m_level;    // 1 or 2
    DWORD           m_dwEnumFlags;
    DWORD           m_cShares;
    DWORD           m_iCurrent;

    ULONG           m_ulRefs;
};

//
// Values for m_dwEnumFlags
//

#ifdef WIZARDS
#define EF_SHOW_NEW_WIZARD  0x00000001
#define EF_SHOW_NW_WIZARD   0x00000002
#define EF_SHOW_MAC_WIZARD  0x00000004
#define EF_SHOW_ALL_WIZARD  0x00000008
#endif // WIZARDS

#endif // __ENUM_HXX__
