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

class CShare;
class CShareHashTable;
class CIterateData;

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

    PWSTR           m_pszMachine;
    ULONG           m_uFlags;

    BOOL            m_bDoSmb;
    BOOL            m_bDoFpnw;
    BOOL            m_bDoSfm;

    // SMB shares
    SHARE_INFO_2*   m_pShares;  // may actually point to level 1 info
    ULONG           m_level;    // 1 or 2
    DWORD           m_cShares;

    // FPNW shares
    FPNWVOLUMEINFO* m_pFpnwShares;
    DWORD           m_cFpnwShares;

    // SFM shares
    AFP_VOLUME_INFO* m_pSfmShares;
    DWORD           m_cSfmShares;

    ULONG           m_ulRefs;
	CShare*			m_pShareList;
	CShare*			m_pShareCurrent;
};

#endif // __ENUM_HXX__
