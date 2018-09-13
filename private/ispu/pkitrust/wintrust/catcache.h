//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catcache.h
//
//  Contents:   Catalog Cache for performance improvement to verification path
//
//  History:    26-May-98    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CATCACHE_H__)
#define __CATCACHE_H__

//
// This caches state data for catalog member verification indexed by the file
// path to the catalog.  This relieves the caller from having to user the
// icky WTD_STATEACTION* stuff in order to achieve the same ends.  Someday,
// we will just re-design/re-implement the WVT and Catalog crap and life
// will be good.
//

#include <lrucache.h>

#define DEFAULT_CATALOG_CACHE_BUCKETS     3
#define DEFAULT_CATALOG_CACHE_MAX_ENTRIES 3

typedef struct _CATALOG_CACHED_STATE {

    HANDLE    hStateData;
    HLRUENTRY hEntry;

} CATALOG_CACHED_STATE, *PCATALOG_CACHED_STATE;

class CCatalogCache
{
public:

    //
    // Construction
    //

    inline CCatalogCache ();
    inline ~CCatalogCache ();

    //
    // Initialization
    //

    BOOL Initialize ();
    VOID Uninitialize ();

    //
    // Cache locking
    //

    inline VOID LockCache ();
    inline VOID UnlockCache ();

    //
    // Cached State management
    //

    BOOL IsCacheableWintrustCall (WINTRUST_DATA* pWintrustData);

    VOID AdjustWintrustDataToCachedState (
               WINTRUST_DATA* pWintrustData,
               PCATALOG_CACHED_STATE pCachedState,
               BOOL fUndoAdjustment
               );

    BOOL CreateCachedStateFromWintrustData (
               WINTRUST_DATA* pWintrustData,
               PCATALOG_CACHED_STATE* ppCachedState
               );

    VOID ReleaseCachedState (PCATALOG_CACHED_STATE pCachedState);

    VOID AddCachedState (PCATALOG_CACHED_STATE pCachedState);

    VOID RemoveCachedState (PCATALOG_CACHED_STATE pCachedState);

    VOID RemoveCachedState (WINTRUST_DATA* pWintrustData);
 
    VOID FlushCache ();

    //
    // Cached State lookup
    //

    PCATALOG_CACHED_STATE FindCachedState (WINTRUST_DATA* pWintrustData);

private:

    //
    // Lock
    //

    CRITICAL_SECTION m_Lock;

    //
    // Cache
    //

    HLRUCACHE        m_hCache;
};

//
// Entry data free function
//

VOID WINAPI
CatalogCacheFreeEntryData (LPVOID pvData);

DWORD WINAPI
CatalogCacheHashIdentifier (PCRYPT_DATA_BLOB pIdentifier);

//
// Inline methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::CCatalogCache, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
inline
CCatalogCache::CCatalogCache ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::~CCatalogCache, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
inline
CCatalogCache::~CCatalogCache ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::LockCache, public
//
//  Synopsis:   lock the cache
//
//----------------------------------------------------------------------------
inline VOID
CCatalogCache::LockCache ()
{
    EnterCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::UnlockCache, public
//
//  Synopsis:   unlock the cache
//
//----------------------------------------------------------------------------
inline VOID
CCatalogCache::UnlockCache ()
{
    LeaveCriticalSection( &m_Lock );
}

#endif
