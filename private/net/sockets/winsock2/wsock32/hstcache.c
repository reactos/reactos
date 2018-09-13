/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    hstcache.c

Abstract:

    The DNS host cache
Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

--*/

#if defined(CHICAGO)
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspapi.h>
#include <nspapip.h>
#include <svcguid.h>
#include <nspmisc.h>


LIST_ENTRY HostentCacheListHead = { &HostentCacheListHead, &HostentCacheListHead };
DWORD MaxHostentCacheSize = 10;
DWORD CurrentHostentCacheSize = 0;

#if defined(CHICAGO)
#define MY_LARGE_INTEGER    DWORD
#define QUADPART(x)         (x)
#define LSTRCMPI            lstrcmpi
#define REG_OPEN_KEY_EX     RegOpenKeyExA
#else
#define MY_LARGE_INTEGER    LARGE_INTEGER
#define QUADPART(x)         (x).QuadPart
#define LSTRCMPI            _stricmp
#define REG_OPEN_KEY_EX     RegOpenKeyExW
#endif

typedef struct _HOSTENT_CACHE_ENTRY {
    LIST_ENTRY HostentListEntry;
    MY_LARGE_INTEGER LastAccessTime;
    MY_LARGE_INTEGER ExpirationTime;
    HOSTENT HostEntry;
    // BYTE[*];                                 // other hostent info
} HOSTENT_CACHE_ENTRY, *PHOSTENT_CACHE_ENTRY;

VOID
CacheHostent (
    IN PHOSTENT HostEntry,
    IN INT Ttl
    );

PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    );



VOID
CacheHostent (
    IN PHOSTENT HostEntry,
    IN INT Ttl
    )
{
    MY_LARGE_INTEGER currentTime;
    MY_LARGE_INTEGER liTtl;
    DWORD bytesRequired;
    PHOSTENT_CACHE_ENTRY cacheEntry;
    PLIST_ENTRY listEntry;
    PHOSTENT_CACHE_ENTRY testCacheEntry;
    PHOSTENT_CACHE_ENTRY oldestCacheEntry;

    //
    // If the TTL is 0, do not cache the entry.
    //

    if ( ( Ttl <= 0 ) || ( MaxHostentCacheSize == 0 ) ) {
        return;
    }

    //
    // Get the current time and convert the TTL to 64 bit time.
    //

#if defined(CHICAGO)
    currentTime = time(NULL);
    liTtl = Ttl;
#else
    NtQuerySystemTime( &currentTime );
    liTtl = RtlEnlargedIntegerMultiply( Ttl, 10*1000*1000 );
#endif

    //
    // Allocate space to hold the hostent information.
    //

    bytesRequired = sizeof(*cacheEntry) + BytesInHostent( HostEntry ) + 20;

    cacheEntry = ALLOCATE_HEAP( bytesRequired );
    if ( cacheEntry == NULL ) {
        return;
    }

    //
    // Set up the cache entry.
    //

    cacheEntry->LastAccessTime = currentTime;
    QUADPART(cacheEntry->ExpirationTime) = QUADPART(currentTime) + QUADPART(liTtl);

    CopyHostentToBuffer(
        (char FAR *)&cacheEntry->HostEntry,
        bytesRequired - sizeof(*cacheEntry),
        HostEntry
        );

    //
    // Acquire the global lock exclusively to protect our lists and
    // counts, and test whether we're at the limit of the caching we'll
    // do.
    //

    SockAcquireGlobalLockExclusive( );


    if ( CurrentHostentCacheSize < MaxHostentCacheSize ) {
        CurrentHostentCacheSize++;
    }
    else
    {
        //
        // We're at our limit for cached entries.  Remove the oldest
        // entry from the cache.
        //

        oldestCacheEntry = NULL;

        for ( listEntry = HostentCacheListHead.Flink;
              listEntry != &HostentCacheListHead;
              listEntry = listEntry->Flink ) {


            testCacheEntry = CONTAINING_RECORD(
                                 listEntry,
                                 HOSTENT_CACHE_ENTRY,
                                 HostentListEntry
                                 );

            if ( oldestCacheEntry == NULL ||
                     QUADPART(testCacheEntry->LastAccessTime) <
                     QUADPART(oldestCacheEntry->LastAccessTime) ) {

                oldestCacheEntry = testCacheEntry;
            }
        }

        RemoveEntryList( &oldestCacheEntry->HostentListEntry );
        FREE_HEAP( oldestCacheEntry );
    }

    //
    // Place the new entry at the front of the global list and return;
    //

    InsertHeadList( &HostentCacheListHead, &cacheEntry->HostentListEntry );

    SockReleaseGlobalLock( );

    return;

} // CacheHostent


PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    )
{
    PLIST_ENTRY listEntry;
    PHOSTENT_CACHE_ENTRY testCacheEntry;
    DWORD i;
    MY_LARGE_INTEGER currentTime;
    PHOSTENT hostEntry;

    //
    // *** It is assumed that this routine is called while the caller
    //     holds the appropriate global cache lock!
    //

    //
    // First get the current system time.  We'll use this to reset the
    // LastAccessTime if we find a hit.
    //

#if defined(CHICAGO)
    currentTime = time(NULL);
#else
    NtQuerySystemTime( &currentTime );
#endif

    //
    // Walk the host entry cache.  As soon as we find a match, quit
    // walking the list.
    //

    for ( listEntry = HostentCacheListHead.Flink;
          listEntry != &HostentCacheListHead; ) {

        testCacheEntry = CONTAINING_RECORD(
                             listEntry,
                             HOSTENT_CACHE_ENTRY,
                             HostentListEntry
                             );
        hostEntry = &testCacheEntry->HostEntry;

        //
        // If this entry has expired, remove it from the list.
        //

        if ( QUADPART(currentTime) > QUADPART(testCacheEntry->ExpirationTime) ) {

            CurrentHostentCacheSize--;
            RemoveEntryList( &testCacheEntry->HostentListEntry );
            listEntry = listEntry->Flink;
            FREE_HEAP( testCacheEntry );
            continue;
        }

        //
        // First check for a matching name.  A match on either the
        // primary name or any of the aliases results in a hit.
        //

        if ( ARGUMENT_PRESENT( Name ) ) {

            if ( LSTRCMPI( Name, hostEntry->h_name ) == 0 ) {
                testCacheEntry->LastAccessTime = currentTime;
                return hostEntry;
            }

            for ( i = 0; hostEntry->h_aliases[i] != NULL; i++ ) {
                if ( LSTRCMPI( Name, hostEntry->h_aliases[i] ) == 0 ) {
                    testCacheEntry->LastAccessTime = currentTime;
                    return hostEntry;
                }
            }
        }

        //
        // Now check for a match against any of the IP addresses in
        // the hostent.
        //

        if ( IpAddress != 0 ) {

            for ( i = 0; hostEntry->h_addr_list[i] != NULL; i++ ) {
                if ( IpAddress == *(PDWORD)hostEntry->h_addr_list[i] ) {
                    testCacheEntry->LastAccessTime = currentTime;
                    return hostEntry;
                }
            }
        }

        listEntry = listEntry->Flink;
    }

    //
    // We didn't find a match in the hostent cache.
    //

    return NULL;

} // QueryHostentCache
