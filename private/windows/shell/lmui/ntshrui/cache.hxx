//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       cache.hxx
//
//  Contents:   Functions to manage a cache of shares
//
//  History:    11-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

#ifndef __CACHE_HXX__
#define __CACHE_HXX__

// The CShareCache class encapsulates a cache of network shares. The cache
// is thread-safe; all access is protected by a critical section.

class CStrHashTable;
class CShareInfo;

class CShareCache
{
public:

    CShareCache(
        VOID
        );

    ~CShareCache();

    BOOL
    IsPathShared(
        LPCTSTR lpPath,
        BOOL fRefresh
        );

    BOOL
    IsShareNameUsed(
        IN PWSTR pszShareName
        );

    BOOL
    IsExistingShare(
        IN PCWSTR pszShareName,
        IN PCWSTR pszPath,
        OUT PWSTR pszOldPath
        );

    VOID
    Refresh(
        VOID
        );

    HRESULT
    ConstructList(
        IN PCWSTR          pszPath,
        IN OUT CShareInfo* pShareList,
        OUT ULONG*         pcShares
        );

    HRESULT
    ConstructParentWarnList(
        IN PCWSTR        pszPath,
        OUT CShareInfo** ppShareList
        );

private:

    VOID
    Delete(
        VOID
        );

    VOID
    RefreshNoCritSec(
        VOID
        );

    BOOL
    CacheOK(
        VOID
        );

    UINT                m_cShares;
    LPBYTE              m_pBufShares;
    CStrHashTable*      m_pHash;
    CRITICAL_SECTION    m_csBuf; // last for debugging ease
};

//////////////////////////////////////////////////////////////////////////////

extern CShareCache  g_ShareCache;   // the main share cache

//////////////////////////////////////////////////////////////////////////////

#endif // __CACHE_HXX__
