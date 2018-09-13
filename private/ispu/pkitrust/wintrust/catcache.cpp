//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catcache.cpp
//
//  Contents:   Implementation of Catalog Cache (see catcache.h for details)
//
//  History:    26-May-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::Initialize, public
//
//  Synopsis:   initialize the cache
//
//----------------------------------------------------------------------------
BOOL
CCatalogCache::Initialize ()
{
    LRU_CACHE_CONFIG Config;

    InitializeCriticalSection( &m_Lock );

    memset( &Config, 0, sizeof( Config ) );

    m_hCache = NULL;
    Config.dwFlags = LRU_CACHE_NO_SERIALIZE;
    Config.pfnFree = CatalogCacheFreeEntryData;
    Config.pfnHash = CatalogCacheHashIdentifier;
    Config.cBuckets = DEFAULT_CATALOG_CACHE_BUCKETS;
    Config.MaxEntries = DEFAULT_CATALOG_CACHE_MAX_ENTRIES;

    return( I_CryptCreateLruCache( &Config, &m_hCache ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::Uninitialize, public
//
//  Synopsis:   uninitialize the cache
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::Uninitialize ()
{
    if ( m_hCache != NULL )
    {
        I_CryptFreeLruCache( m_hCache, 0, NULL );
    }

    DeleteCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::IsCacheableWintrustCall, public
//
//  Synopsis:   is this a cacheable call
//
//----------------------------------------------------------------------------
BOOL
CCatalogCache::IsCacheableWintrustCall (WINTRUST_DATA* pWintrustData)
{
    if ( pWintrustData->dwUnionChoice != WTD_CHOICE_CATALOG )
    {
        return( FALSE );
    }

    if ( _ISINSTRUCT( WINTRUST_DATA, pWintrustData->cbStruct, hWVTStateData ) )
    {
        if ( ( pWintrustData->dwStateAction == WTD_STATEACTION_AUTO_CACHE ) ||
             ( pWintrustData->dwStateAction == WTD_STATEACTION_AUTO_CACHE_FLUSH ) )
        {
            return( TRUE );
        }
    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::AdjustWintrustDataToCachedState, public
//
//  Synopsis:   adjust the wintrust data structure
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::AdjustWintrustDataToCachedState (
                     WINTRUST_DATA* pWintrustData,
                     PCATALOG_CACHED_STATE pCachedState,
                     BOOL fUndoAdjustment
                     )
{
    PCRYPT_PROVIDER_DATA pProvData;

    if ( fUndoAdjustment == FALSE )
    {
        pWintrustData->dwStateAction = WTD_STATEACTION_VERIFY;

        if ( pCachedState != NULL )
        {
            pWintrustData->hWVTStateData = pCachedState->hStateData;

            pProvData = WTHelperProvDataFromStateData( pCachedState->hStateData );
            pProvData->pWintrustData = pWintrustData;
        }
        else
        {
            pWintrustData->hWVTStateData = NULL;
        }
    }
    else
    {
        if ( pCachedState != NULL )
        {
            pProvData = WTHelperProvDataFromStateData( pCachedState->hStateData );
            pProvData->pWintrustData = NULL;
        }

        pWintrustData->dwStateAction = WTD_STATEACTION_AUTO_CACHE;
        pWintrustData->hWVTStateData = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::CreateCachedStateFromWintrustData, public
//
//  Synopsis:   create cached state
//
//----------------------------------------------------------------------------
BOOL
CCatalogCache::CreateCachedStateFromWintrustData (
                     WINTRUST_DATA* pWintrustData,
                     PCATALOG_CACHED_STATE* ppCachedState
                     )
{
    BOOL                  fResult;
    PCATALOG_CACHED_STATE pCachedState;
    CRYPT_DATA_BLOB       Identifier;

    PCRYPT_PROVIDER_DATA  pProvData;

    if ( pWintrustData->hWVTStateData == NULL )
    {
        return( FALSE );
    }

    pProvData = WTHelperProvDataFromStateData( pWintrustData->hWVTStateData );

    if ( ( pProvData->padwTrustStepErrors[ TRUSTERROR_STEP_FINAL_INITPROV ] != ERROR_SUCCESS ) ||
         ( ( pProvData->padwTrustStepErrors[ TRUSTERROR_STEP_FINAL_OBJPROV ] != ERROR_SUCCESS ) &&
           ( pProvData->padwTrustStepErrors[ TRUSTERROR_STEP_FINAL_OBJPROV ] != TRUST_E_BAD_DIGEST ) ) ||
         ( pProvData->padwTrustStepErrors[ TRUSTERROR_STEP_FINAL_SIGPROV ] != ERROR_SUCCESS ) ||
         ( pProvData->hMsg == NULL ) )
    {
        return( FALSE );
    }

    assert( pProvData->hMsg != NULL );

    pCachedState = new CATALOG_CACHED_STATE;
    if ( pCachedState != NULL )
    {
        pCachedState->hStateData = pWintrustData->hWVTStateData;
        pCachedState->hEntry = NULL;

        Identifier.cbData = wcslen(
                               pWintrustData->pCatalog->pcwszCatalogFilePath
                               );

        Identifier.cbData *= sizeof( WCHAR );

        Identifier.pbData = (LPBYTE)pWintrustData->pCatalog->pcwszCatalogFilePath;

        fResult = I_CryptCreateLruEntry(
                         m_hCache,
                         &Identifier,
                         pCachedState,
                         &pCachedState->hEntry
                         );
    }
    else
    {
        SetLastError( E_OUTOFMEMORY );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        *ppCachedState = pCachedState;
    }
    else
    {
        delete pCachedState;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::ReleaseCachedState, public
//
//  Synopsis:   release the cached state
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::ReleaseCachedState (PCATALOG_CACHED_STATE pCachedState)
{
    if ( pCachedState == NULL )
    {
        return;
    }

    I_CryptReleaseLruEntry( pCachedState->hEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::AddCachedState, public
//
//  Synopsis:   add cached state
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::AddCachedState (PCATALOG_CACHED_STATE pCachedState)
{
    I_CryptInsertLruEntry( pCachedState->hEntry, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::RemoveCachedState, public
//
//  Synopsis:   remove cached state
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::RemoveCachedState (PCATALOG_CACHED_STATE pCachedState)
{
    I_CryptRemoveLruEntry( pCachedState->hEntry, 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::RemoveCachedState, public
//
//  Synopsis:   remove cached state
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::RemoveCachedState (WINTRUST_DATA* pWintrustData)
{
    PCATALOG_CACHED_STATE pCachedState;

    pCachedState = FindCachedState( pWintrustData );

    if ( pCachedState != NULL )
    {
        RemoveCachedState( pCachedState );
        ReleaseCachedState( pCachedState );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::FindCachedState, public
//
//  Synopsis:   find cached state, the state is addref'd via the entry
//
//----------------------------------------------------------------------------
PCATALOG_CACHED_STATE
CCatalogCache::FindCachedState (WINTRUST_DATA* pWintrustData)
{
    PCATALOG_CACHED_STATE pCachedState;
    CRYPT_DATA_BLOB       Identifier;
    HLRUENTRY             hEntry;

    Identifier.cbData = wcslen(
                           pWintrustData->pCatalog->pcwszCatalogFilePath
                           );

    Identifier.cbData *= sizeof( WCHAR );

    Identifier.pbData = (LPBYTE)pWintrustData->pCatalog->pcwszCatalogFilePath;

    pCachedState = (PCATALOG_CACHED_STATE)I_CryptFindLruEntryData(
                                                 m_hCache,
                                                 &Identifier,
                                                 &hEntry
                                                 );

    return( pCachedState );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogCache::FlushCache, public
//
//  Synopsis:   flush the cache
//
//----------------------------------------------------------------------------
VOID
CCatalogCache::FlushCache ()
{
    I_CryptFlushLruCache( m_hCache, 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   CatalogCacheFreeEntryData
//
//  Synopsis:   free entry data
//
//----------------------------------------------------------------------------
VOID WINAPI
CatalogCacheFreeEntryData (LPVOID pvData)
{
    PCATALOG_CACHED_STATE pCachedState = (PCATALOG_CACHED_STATE)pvData;
    WINTRUST_DATA         WintrustData;
    GUID                  ActionGuid;

    memset( &ActionGuid, 0, sizeof( ActionGuid ) );

    memset( &WintrustData, 0, sizeof( WintrustData ) );
    WintrustData.cbStruct = sizeof( WintrustData );
    WintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WintrustData.hWVTStateData = pCachedState->hStateData;

    WinVerifyTrust( NULL, &ActionGuid, &WintrustData );

    delete pCachedState;
}

//+---------------------------------------------------------------------------
//
//  Function:   CatalogCacheHashIdentifier
//
//  Synopsis:   hash the name
//
//----------------------------------------------------------------------------
DWORD WINAPI
CatalogCacheHashIdentifier (PCRYPT_DATA_BLOB pIdentifier)
{
    DWORD  dwHash = 0;
    DWORD  cb = pIdentifier->cbData;
    LPBYTE pb = pIdentifier->pbData;

    while ( cb-- )
    {
        if ( dwHash & 0x80000000 )
        {
            dwHash = ( dwHash << 1 ) | 1;
        }
        else
        {
            dwHash = dwHash << 1;
        }

        dwHash += *pb++;
    }

    return( dwHash );
}


