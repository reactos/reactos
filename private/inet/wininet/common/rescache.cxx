/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    rescache.cxx

Abstract:

    Contains functions which manipulate hostent cache for winsock gethostbyObject
    calls

    Contents:
        InitializeHostentCache
        TerminateHostentCache
        QueryHostentCache
        CacheHostent
        FlushHostentCache
        ReleaseHostentCacheEntry
        ThrowOutHostentCacheEntry
        (RemoveCacheEntry)
        (ResolverCacheHit)
        (HostentMatch)
        (CreateCacheEntry)
        (CompareHostentNames)
        (BytesInHostent)
        (CopyHostentToBuffer)

Author:

    Richard L Firth (rfirth) 10-Jul-1994

Environment:

    Win-16/32 user level

Revision History:

    rfirth 10-Jul-1994
        Created

--*/

//
// includes
//

#include "wininetp.h"

//
// private manifests
//

//
// private macros
//

#define SET_EXPIRATION_TIME(cacheEntry)

//
// private data
//

PRIVATE BOOL HostentCacheInitialized = FALSE;

//
// DnsCachingEnabled - caching is enabled by default
//

PRIVATE BOOL DnsCachingEnabled = TRUE;

//
// DnsCacheTimeout - number of seconds before a cache entry expires. This value
// is added to the current time (in seconds) to get the expiry time
//

PRIVATE DWORD DnsCacheTimeout = DEFAULT_DNS_CACHE_TIMEOUT;

//
// MaximumDnsCacheEntries - the maximum number of RESOLVER_CACHE_ENTRYs in the
// cache before we start throwing out the LRU
//

PRIVATE DWORD MaximumDnsCacheEntries = DEFAULT_DNS_CACHE_ENTRIES;

//
// CurrentDnsCacheEntries - the number of RESOLVER_CACHE_ENTRYs currently in the
// cache
//

PRIVATE DWORD CurrentDnsCacheEntries = 0;

//
// ResolverCache - serialized list of RESOLVER_CACHE_ENTRYs, kept in MRU order.
// We only need to remove the tail of the list to remove the LRU entry
//

PRIVATE SERIALIZED_LIST ResolverCache = {0};

//
// private prototypes
//

PRIVATE
VOID
RemoveCacheEntry(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry
    );

PRIVATE
BOOL
ResolverCacheHit(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry,
    IN LPSTR Name OPTIONAL,
    IN LPBYTE Address OPTIONAL
    );

PRIVATE
BOOL
HostentMatch(
    IN LPHOSTENT Hostent,
    IN LPSTR Name OPTIONAL,
    IN LPBYTE Address OPTIONAL
    );

PRIVATE
LPRESOLVER_CACHE_ENTRY
CreateCacheEntry(
    IN LPSTR lpszHostName,
    IN LPHOSTENT Hostent,
    IN DWORD TimeToLive
    );

PRIVATE
BOOL
CompareHostentNames(
    IN LPHOSTENT Hostent,
    IN LPSTR Name
    );

PRIVATE
DWORD
BytesInHostent(
    IN LPHOSTENT Hostent
    );

PRIVATE
DWORD
CopyHostentToBuffer(
    OUT PCHAR Buffer,
    IN UINT BufferLength,
    IN PHOSTENT Hostent
    );

#if INET_DEBUG

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheTimestr(
    IN DWORD Time
    );

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheHostentStr(
    IN LPHOSTENT Hostent
    );

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheMapIpAddress(
    IN LPBYTE Address
    );

#endif

//
// functions
//


VOID
InitializeHostentCache(
    VOID
    )

/*++

Routine Description:

    Initializes the hostent cache:

        * Initializes the cache list anchor
        * loads the cache

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "InitializeHostentCache",
                 NULL
                 ));


    if (!(BOOL)InterlockedExchange((LPLONG)&HostentCacheInitialized, TRUE)) {
        InternetReadRegistryDword("DnsCacheEnabled", (LPDWORD)&DnsCachingEnabled);
        InternetReadRegistryDword("DnsCacheEntries", &MaximumDnsCacheEntries);
        InternetReadRegistryDword("DnsCacheTimeout", &DnsCacheTimeout);
        InitializeSerializedList(&ResolverCache);

        //
        // if the size of the cache in the registry is 0 then its the same as
        // no caching
        //

        if (MaximumDnsCacheEntries == 0) {
            DnsCachingEnabled = FALSE;
        }
    } else {

        //
        // shouldn't be calling this more than once
        //

        INET_ASSERT(FALSE);

    }

    DEBUG_LEAVE(0);
}


VOID
TerminateHostentCache(
    VOID
    )

/*++

Routine Description:

    Free up all resources allocated by InitializeHostentCache()

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "TerminateHostentCache",
                 NULL
                 ));

    if (InterlockedExchange((LPLONG)&HostentCacheInitialized, FALSE)) {

        //
        // short-circuit any other cache attempts (shouldn't be any by now)
        //

        DnsCachingEnabled = FALSE;

        //
        // and clear out the list
        //

        FlushHostentCache();

        //
        // we are done with the serialized list
        //

        TerminateSerializedList(&ResolverCache);
    }

    DEBUG_LEAVE(0);
}


BOOL
QueryHostentCache(
    IN LPSTR Name OPTIONAL,
    IN LPBYTE Address OPTIONAL,
    OUT LPHOSTENT * Hostent,
    OUT LPDWORD TimeToLive
    )

/*++

Routine Description:

    Checks if Name is stored in the last resolved name cache. If the entry is
    found, but has expired then it is removed from the cache

Arguments:

    Name        - pointer to name string

    Address     - pointer to IP address

    Hostent     - pointer to returned pointer to hostent

    TimeToLive  - pointer to returned time to live

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "QueryHostentCache",
                 "%q, %s, %#x, %#x",
                 Name,
                 CacheMapIpAddress(Address),
                 Hostent,
                 TimeToLive
                 ));

    BOOL found;

    if (!DnsCachingEnabled) {

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled\n"
                    ));

        *Hostent = NULL;
        found = FALSE;
        goto quit;
    }

    LockSerializedList(&ResolverCache);

    LPRESOLVER_CACHE_ENTRY cacheEntry;
    LPRESOLVER_CACHE_ENTRY previousEntry;
    DWORD timeNow;

    cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(&ResolverCache);
    timeNow = (DWORD)time(NULL);

    while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(&ResolverCache)) {

        //
        // on every cache lookup, purge any stale entries. LIVE_FOREVER means
        // that we don't expect the entry's net address to expire, but it
        // DOESN'T mean that we can't throw out the entry if its the LRU and
        // we're at maximum cache capacity. We can't do this if the item is
        // still in-use. In this case, we mark it stale
        //

        if ((cacheEntry->ExpirationTime != LIVE_FOREVER)
        && (cacheEntry->ExpirationTime <= timeNow)) {

            //
            // if reference count not zero then another thread is using
            // this entry - mark as stale else delete it
            //

            if (cacheEntry->ReferenceCount != 0) {

                INET_ASSERT(cacheEntry->State == ENTRY_IN_USE);

                cacheEntry->State = ENTRY_DELETE;
            } else {

                //
                // this entry is stale; throw it out
                // "my hovercraft is full of eels"
                //

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("throwing out stale DNS entry %q, expiry = %s\n",
                            cacheEntry->Hostent.h_name,
                            CacheTimestr(cacheEntry->ExpirationTime)
                            ));

                //
                // BUGBUG - what happens if ExpirationTime == timeNow?
                //

                previousEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Blink;
                RemoveCacheEntry(cacheEntry);
                cacheEntry = previousEntry;
            }
        } else if (ResolverCacheHit(cacheEntry, Name, Address)
        && ((cacheEntry->State == ENTRY_UNUSED)
        || (cacheEntry->State == ENTRY_IN_USE))) {

            //
            // we found the entry, and it still has time to live. Make it the
            // head of the list (MRU first), set the state to in-use and increase
            // the reference count
            //

            RemoveFromSerializedList(&ResolverCache, &cacheEntry->ListEntry);
            InsertAtHeadOfSerializedList(&ResolverCache, &cacheEntry->ListEntry);
            cacheEntry->State = ENTRY_IN_USE;
            ++cacheEntry->ReferenceCount;
            *Hostent = &cacheEntry->Hostent;
            found = TRUE;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("entry found in DNS cache\n"
                        ));

            goto done;
        }
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
    }

    *Hostent = NULL;
    found = FALSE;

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("didn't find entry in DNS cache\n"
                ));

done:

    UnlockSerializedList(&ResolverCache);

quit:

    DEBUG_LEAVE(found);

    return found;
}


VOID
CacheHostent(
    IN LPSTR lpszHostName,
    IN LPHOSTENT Hostent,
    IN DWORD TimeToLive
    )

/*++

Routine Description:

    Adds a hostent structure to the cache. Creates a new structure and links it
    into the cache list, displacing the LRU entry if required. If we cannot
    create the entry, no action is taken, no errors returned

Arguments:

    lpszHostName    - the name we originally requested be resolved. May be
                      different than the names returned by the resolver, e.g.
                      "proxy" => "proxy1.microsoft.com, proxy2.microsoft.com"

    Hostent         - pointer to hostent to add to cache

    TimeToLive      - amount of time this information has to live. Can be:

                        LIVE_FOREVER    - don't timeout (but can be discarded)

                        LIVE_DEFAULT    - use the default value

                        anything else   - number of seconds to live

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "CacheHostent",
                 "%q, %#x, %d",
                 lpszHostName,
                 Hostent,
                 TimeToLive
                 ));

    if (!DnsCachingEnabled) {

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled\n"
                    ));

        goto quit;
    }

    LockSerializedList(&ResolverCache);

    //
    // check that the entry is not already in the cache - 2 or more threads may
    // have been simultaneously resolving the same name
    //

    LPHOSTENT lpHostent;
    DWORD ttl;

    INET_ASSERT(lpszHostName != NULL);

    if (!QueryHostentCache(lpszHostName, NULL, &lpHostent, &ttl)) {

        LPRESOLVER_CACHE_ENTRY cacheEntry;

        //
        // remove as many entries as we can beginning at the tail of the list.
        // We try to remove enough to get the cache size back below the limit.
        // This may consist of removing expired entries or entries marked as
        // DELETE. If there are expired, in-use entries then we mark them as
        // DELETE. This may result in the cache list growing until those threads
        // which have referenced cache entries release them
        //

        cacheEntry = (LPRESOLVER_CACHE_ENTRY)TailOfSerializedList(&ResolverCache);

        while ((CurrentDnsCacheEntries >= MaximumDnsCacheEntries)
        && (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(&ResolverCache))) {

            //
            // cache has maximum entries: throw out the Least Recently Used (its
            // the one at the back of the queue, ma'am) but only if no-one else
            // is currently accessing it
            //

            if ((cacheEntry->State != ENTRY_IN_USE)
            && (cacheEntry->ReferenceCount == 0)) {

                INET_ASSERT((cacheEntry->State == ENTRY_UNUSED)
                            || (cacheEntry->State == ENTRY_DELETE));

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("throwing out LRU %q\n",
                            cacheEntry->Hostent.h_name
                            ));

                LPRESOLVER_CACHE_ENTRY nextEntry;

                nextEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
                RemoveCacheEntry(cacheEntry);
                cacheEntry = nextEntry;
            } else if (cacheEntry->State == ENTRY_IN_USE) {

                //
                // this entry needs to be freed when it is released
                //

                cacheEntry->State = ENTRY_DELETE;
            }
            cacheEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Blink;
        }

        //
        // add the entry at the head of the queue - it is the Most Recently Used
        // after all. If we fail to allocate memory, its no problem: it'll just
        // take a little longer if this entry would have been hit before we needed
        // to throw out another entry
        //

        if (cacheEntry = CreateCacheEntry(lpszHostName, Hostent, TimeToLive)) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("caching %q, expiry = %s\n",
                        CacheHostentStr(&cacheEntry->Hostent),
                        CacheTimestr(cacheEntry->ExpirationTime)
                        ));

            InsertAtHeadOfSerializedList(&ResolverCache, &cacheEntry->ListEntry);
            ++CurrentDnsCacheEntries;
        }
    } else {

        //
        // this entry is already in the cache. 2 or more threads must have been
        // resolving the same name simultaneously. We just bump the expiration
        // time to the more recent value
        //

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("found %q already in the cache!?\n",
                    Hostent->h_name
                    ));

        ReleaseHostentCacheEntry(lpHostent);

    }

    UnlockSerializedList(&ResolverCache);

quit:

    DEBUG_LEAVE(0);
}


VOID
FlushHostentCache(
    VOID
    )

/*++

Routine Description:

    Removes all entries in DNS hostent cache

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "FlushHostentCache",
                 NULL
                 ));

    LPRESOLVER_CACHE_ENTRY cacheEntry;
    LPRESOLVER_CACHE_ENTRY previousEntry;

    LockSerializedList(&ResolverCache);

    previousEntry = (LPRESOLVER_CACHE_ENTRY)SlSelf(&ResolverCache);
    cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(&ResolverCache);
    while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(&ResolverCache)) {
        if (cacheEntry->State == ENTRY_UNUSED) {
            RemoveCacheEntry(cacheEntry);
        } else {

            DEBUG_PRINT(SOCKETS,
                        WARNING,
                        ("cache entry %#x (%q) still in-use\n",
                        cacheEntry->HostName
                        ));

            cacheEntry->State = ENTRY_DELETE;
            previousEntry = cacheEntry;
        }
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)previousEntry->ListEntry.Flink;
    }

    UnlockSerializedList(&ResolverCache);

    DEBUG_LEAVE(0);
}


VOID
ReleaseHostentCacheEntry(
    IN LPHOSTENT lpHostent
    )

/*++

Routine Description:

    Either mark a hostent unused or if it is stale, delete it

Arguments:

    lpHostent   - pointer to hostent to free

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "ReleaseHostentCacheEntry",
                 "%#x",
                 lpHostent
                 ));

    LPRESOLVER_CACHE_ENTRY cacheEntry = CONTAINING_RECORD(lpHostent,
                                                          RESOLVER_CACHE_ENTRY,
                                                          Hostent
                                                          );

    LockSerializedList(&ResolverCache);

    //
    // reference count should never go below zero!
    //

    INET_ASSERT(cacheEntry->ReferenceCount > 0);

    if (--cacheEntry->ReferenceCount <= 0) {

        //
        // last releaser gets to decide what to do - mark unused or delete
        //

        if (cacheEntry->State == ENTRY_IN_USE) {
            cacheEntry->State = ENTRY_UNUSED;
        } else if (cacheEntry->State == ENTRY_DELETE) {

            //
            // entry is already stale - throw it out
            //

            RemoveCacheEntry(cacheEntry);
        } else {

            //
            // unused? or bogus value? Someone changed state while refcount
            // not zero?
            //

            INET_ASSERT((cacheEntry->State == ENTRY_IN_USE)
                        || (cacheEntry->State == ENTRY_DELETE));

        }
    }

    UnlockSerializedList(&ResolverCache);

    DEBUG_LEAVE(0);
}


VOID
ThrowOutHostentCacheEntry(
    IN LPHOSTENT lpHostent
    )

/*++

Routine Description:

    Removes this entry from the DNS cache, based on the host name. We assume
    that the entry came from the cache, so unless it has been already purged,
    we should be able to throw it out

Arguments:

    lpHostent   - pointer to host entry containing details (name) of entry to
                  throw out

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "ThrowOutHostentCacheEntry",
                 "%#x [%q]",
                 lpHostent,
                 lpHostent->h_name
                 ));

    if (DnsCachingEnabled) {
        LockSerializedList(&ResolverCache);

        LPRESOLVER_CACHE_ENTRY cacheEntry;

        cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(&ResolverCache);
        while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(&ResolverCache)) {
            if (HostentMatch(&cacheEntry->Hostent, lpHostent->h_name, NULL)) {

                //
                // if the entry is unused then we can delete it, else we have
                // to leave it to the thread with the last reference
                //

                if (cacheEntry->State == ENTRY_UNUSED) {
                    RemoveCacheEntry(cacheEntry);
                } else {
                    cacheEntry->State = ENTRY_DELETE;
                }
                break;
            }
        }

        UnlockSerializedList(&ResolverCache);
    } else {

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled\n"
                    ));

    }

    DEBUG_LEAVE(0);
}


PRIVATE
VOID
RemoveCacheEntry(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry
    )

/*++

Routine Description:

    Takes a cache entry off the list and frees it

    N.B.: This function must be called with the resolver cache serialized list
    already locked

Arguments:

    lpCacheEntry    - currently queued entry to remove

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "RemoveCacheEntry",
                 "%#x",
                 lpCacheEntry
                 ));

    RemoveFromSerializedList(&ResolverCache, &lpCacheEntry->ListEntry);

    INET_ASSERT(lpCacheEntry->ReferenceCount == 0);
    INET_ASSERT((lpCacheEntry->State == ENTRY_UNUSED)
                || (lpCacheEntry->State == ENTRY_DELETE));

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("throwing out %q, expiry = %s\n",
                CacheHostentStr(&lpCacheEntry->Hostent),
                CacheTimestr(lpCacheEntry->ExpirationTime)
                ));

    lpCacheEntry = (LPRESOLVER_CACHE_ENTRY)FREE_MEMORY((HLOCAL)lpCacheEntry);

    INET_ASSERT(lpCacheEntry == NULL);

    --CurrentDnsCacheEntries;

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("CurrentDnsCacheEntries = %d\n",
                CurrentDnsCacheEntries
                ));

    INET_ASSERT((CurrentDnsCacheEntries >= 0)
                && (CurrentDnsCacheEntries <= MaximumDnsCacheEntries));

    DEBUG_LEAVE(0);
}


PRIVATE
BOOL
ResolverCacheHit(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry,
    IN LPSTR Name OPTIONAL,
    IN LPBYTE Address OPTIONAL
    )

/*++

Routine Description:

    Checks this RESOLVER_CACHE_ENTRY for a match with Name or Address. If Name,
    can match with name or alias(es) in hostent, or with originally resolved
    name

Arguments:

    lpCacheEntry    - pointer to RESOLVER_CACHE_ENTRY to check

    Name            - optional name to check

    Address         - optional server address to check

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "ResolverCacheHit",
                 "%#x, %q, %s",
                 lpCacheEntry,
                 Name,
                 CacheMapIpAddress(Address)
                 ));

    BOOL found;

    if ((Name != NULL)
    && (lpCacheEntry->HostName != NULL)
    && (lstrcmpi(lpCacheEntry->HostName, Name) == 0)) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("matched name %q\n",
                    lpCacheEntry->HostName
                    ));

        found = TRUE;
    } else {
        found = FALSE;
    }
    if (!found) {
        found = HostentMatch(&lpCacheEntry->Hostent, Name, Address);
    }

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
BOOL
HostentMatch(
    IN LPHOSTENT Hostent,
    IN LPSTR Name OPTIONAL,
    IN LPBYTE Address OPTIONAL
    )

/*++

Routine Description:

    Compares a hostent structure for a match with a host name or address

Arguments:

    Hostent - pointer to hostent to compare

    Name    - pointer to name string

    Address - pointer to IP address in network byte order

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "HostentMatch",
                 "%#x, %q, %s",
                 Hostent,
                 Name,
                 CacheMapIpAddress(Address)
                 ));

    BOOL found;

    if (Name) {
        found = CompareHostentNames(Hostent, Name);
    } else {

        LPBYTE* addressList = (LPBYTE*)Hostent->h_addr_list;
        LPBYTE address;

        found = FALSE;

        while (address = *addressList++) {
            if (*(LPDWORD)address == *(LPDWORD)Address) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("matched %s\n",
                            CacheMapIpAddress(address)
                            ));

                found = TRUE;
                break;
            }
        }
    }

    if (found) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("hostent = %q\n",
                    CacheHostentStr(Hostent)
                    ));

    }

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
LPRESOLVER_CACHE_ENTRY
CreateCacheEntry(
    IN LPSTR lpszHostName,
    IN LPHOSTENT Hostent,
    IN DWORD TimeToLive
    )

/*++

Routine Description:

    Allocates a RESOLVER_CACHE_ENTRY and packs it with the hostent information
    and sets the ExpirationTime

Arguments:

    lpszHostName    - name we resolved

    Hostent         - pointer to hostent to add

    TimeToLive      - amount of time before this hostent expires

Return Value:

    LPRESOLVER_CACHE_ENTRY

--*/

{
    LPRESOLVER_CACHE_ENTRY cacheEntry;
    UINT hostentSize;

    //
    // BytesInHostent gives us the size of the fixed and variable parts of the
    // hostent structure
    //

    hostentSize = (UINT)BytesInHostent(Hostent);

    INET_ASSERT(lpszHostName != NULL);

    //
    // only copy lpszHostName if it is different to the names in the hostent
    //

    UINT hostNameSize;

    if (!CompareHostentNames(Hostent, lpszHostName)) {
        hostNameSize = lstrlen(lpszHostName) + 1;
    } else {
        hostNameSize = 0;
    }

    //
    // allocate space for the cache entry (take off the size of the fixed part
    // of the hostent - BytesInHostent already accounted for it)
    //

    cacheEntry = (LPRESOLVER_CACHE_ENTRY)ALLOCATE_MEMORY(LMEM_FIXED,
                                                         sizeof(RESOLVER_CACHE_ENTRY)
                                                         - sizeof(HOSTENT)
                                                         + hostentSize
                                                         + hostNameSize
                                                         );
    if (cacheEntry != NULL) {
        CopyHostentToBuffer((PCHAR)&cacheEntry->Hostent, hostentSize, Hostent);

        //
        // copy the host name to the end of the buffer if required
        //

        if (hostNameSize != 0) {
            cacheEntry->HostName = (LPSTR)&cacheEntry->Hostent + hostentSize;
            RtlCopyMemory(cacheEntry->HostName, lpszHostName, hostNameSize);
        } else {
            cacheEntry->HostName = NULL;
        }

        //
        // calculate the expiration time as the current time (in seconds since
        // 1/1/70) + number of seconds to live OR indefinite if TimeToLive is
        // specified as LIVE_FOREVER, which is what we use if the host
        // information didn't originate from DNS
        //

        cacheEntry->ExpirationTime = (TimeToLive == LIVE_FOREVER)
                                        ? LIVE_FOREVER
                                        : time(NULL)
                                            + ((TimeToLive == LIVE_DEFAULT)
                                                ? DnsCacheTimeout
                                                : TimeToLive);

        //
        // the entry state is initially unused
        //

        cacheEntry->State = ENTRY_UNUSED;

        //
        // and reference is zero
        //

        cacheEntry->ReferenceCount = 0;
    }

    return cacheEntry;
}


PRIVATE
BOOL
CompareHostentNames(
    IN LPHOSTENT Hostent,
    IN LPSTR Name
    )

/*++

Routine Description:

    Compares a prospective host name against all names in a hostent

Arguments:

    Hostent - pointer to hostent containing names to compare

    Name    - prospective host name

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CompareHostentNames",
                 "%#x, %q",
                 Hostent,
                 Name
                 ));

    BOOL found;

    if (!lstrcmpi(Hostent->h_name, Name)) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("matched name %q\n",
                    Hostent->h_name
                    ));

        found = TRUE;
        goto done;
    }

    LPSTR alias;
    LPSTR* aliasList;

    aliasList = Hostent->h_aliases;
    while (alias = *aliasList++) {
        if (!lstrcmpi(alias, Name)) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("matched alias %q\n",
                        alias
                        ));

            found = TRUE;
            goto done;
        }
    }

    DEBUG_PRINT(SOCKETS,
                WARNING,
                ("%q not matched\n",
                Name
                ));

    found = FALSE;

done:

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
DWORD
BytesInHostent(
    IN LPHOSTENT Hostent
    )
{
    DWORD total;
    int i;

    total = sizeof(HOSTENT);
    total += lstrlen(Hostent->h_name) + 1;

    //
    // Account for the NULL terminator pointers at the end of the
    // alias and address arrays.
    //

    total += sizeof(char *) + sizeof(char *);

    for (i = 0; Hostent->h_aliases[i] != NULL; i++) {
        total += lstrlen(Hostent->h_aliases[i]) + 1 + sizeof(char *);
    }

    for (i = 0; Hostent->h_addr_list[i] != NULL; i++) {
        total += Hostent->h_length + sizeof(char *);
    }

    //
    // Pad the answer to an eight-byte boundary.
    //

    return (total + 7) & ~7;
}


PRIVATE
DWORD
CopyHostentToBuffer (
    OUT PCHAR Buffer,
    IN UINT BufferLength,
    IN LPHOSTENT Hostent
    )
{
    UINT requiredBufferLength;
    UINT bytesFilled;
    PCHAR currentLocation = Buffer;
    UINT aliasCount;
    UINT addressCount;
    UINT i;
    PHOSTENT outputHostent = (PHOSTENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = (UINT)BytesInHostent(Hostent);

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        RtlZeroMemory( Buffer, requiredBufferLength );
    } else {
        RtlZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the hostent structure if it fits.
    //

    bytesFilled = sizeof(*Hostent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    RtlCopyMemory( currentLocation, Hostent, sizeof(*Hostent) );
    currentLocation = Buffer + bytesFilled;

    outputHostent->h_name = NULL;
    outputHostent->h_aliases = NULL;
    outputHostent->h_addr_list = NULL;

    //
    // Count the host's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Hostent->h_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_aliases = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Count the host's addresses and set up an array to hold pointers to
    // them.
    //

    for ( addressCount = 0;
          Hostent->h_addr_list[addressCount] != NULL;
          addressCount++ );

    bytesFilled += (addressCount+1) * sizeof(void FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_addr_list = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_addr_list = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in addresses.  Do addresses before filling in the
    // host name and aliases in order to avoid alignment problems.
    //

    for ( i = 0; i < addressCount; i++ ) {

        bytesFilled += Hostent->h_length;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_addr_list[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_addr_list[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_addr_list[i],
            Hostent->h_length
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_addr_list[i] = NULL;

    //
    // Copy the host name if it fits.
    //

    bytesFilled += lstrlen(Hostent->h_name) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputHostent->h_name = currentLocation;

    RtlCopyMemory(currentLocation, Hostent->h_name, lstrlen(Hostent->h_name) + 1);
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += lstrlen(Hostent->h_aliases[i]) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_aliases[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_aliases[i],
            lstrlen(Hostent->h_aliases[i]) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_aliases[i] = NULL;

    return requiredBufferLength;
}

#if INET_DEBUG

//
// CAVEAT - can only call these functions once per printf() etc. because of
//          static buffers (but still thread-safe)
//

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheTimestr(IN DWORD Time) {

    //
    // previous code - writes formatted human-sensible date/time to buffer
    //

    //LPSTR p;
    //
    ////
    //// remove the LF from the time string returned by ctime()
    ////
    //
    //p = ctime((const time_t *)&Time);
    //p[strlen(p) - 1] = '\0';
    //return p;

    //
    // abbreviated CRT version - just write # seconds since 1970 to buffer
    //

    static char buf[16];

    wsprintf(buf, "%d", Time);
    return (LPSTR)buf;
}

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheHostentStr(IN LPHOSTENT Hostent) {

    static char buf[1024];
    LPSTR p;

    p = buf;
    p += wsprintf(p, "n=%s", Hostent->h_name);

    for (LPSTR * aliases = (LPSTR *)Hostent->h_aliases; *aliases; ++aliases) {
        p += wsprintf(p, ", a=%s", *aliases);
    }

    for (LPBYTE * addrs = (LPBYTE *)Hostent->h_addr_list; *addrs; ++addrs) {
        p += wsprintf(p,
                      ", i=%s",
                      CacheMapIpAddress(*addrs)
                      );
    }

    return (LPSTR)buf;
}

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheMapIpAddress(IN LPBYTE Address) {

    if (!Address) {
        return "";
    }

    static char buf[16];

    wsprintf(buf,
             "%d.%d.%d.%d",
             Address[0] & 0xff,
             Address[1] & 0xff,
             Address[2] & 0xff,
             Address[3] & 0xff
             );

    return (LPSTR)buf;
}

#endif

#if defined(RNR_SUPPORTED)

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rescache.c

Abstract:

    Contains name resolution cache

    Contents:

Author:

    Shishir Pardikar    2-14-96

Environment:

    Win32 user mode

Revision History:

        2-14-96 shishirp
        Created

--*/

//
//BUGBUG: This include should be removed, duplicate of above
//
#ifndef SPX_SUPPORT
#include <wininetp.h>
#endif


//
// private manifests
//

#define NAMERES_CACHE_USED            0x00000001
#define NAMERES_CACHE_USES_GUID       0x00000002

#define ENTERCRIT_NAMERESCACHE()  EnterCriticalSection(&vcritNameresCache)
#define LEAVECRIT_NAMERESCACHE()  LeaveCriticalSection(&vcritNameresCache)
#define IS_EMPTY(indx)            ((vlpNameresCache[(indx)].dwFlags & NAMERES_CACHE_USED) == 0)
#define USES_GUID(indx)           ((vlpNameresCache[(indx)].dwFlags & NAMERES_CACHE_USES_GUID))

// number of cache entries
#define DEFAULT_NAMERES_CACHE_ENTRIES   10

// expiry time for an addresslist
#define DEFAULT_EXPIRY_DELTA            (24 * 60 * 60 * (LONGLONG)10000000)


//
//  structure definition
//

typedef struct tagNAMERES_CACHE {
    DWORD               dwFlags;       // general flags to be used as needed
    DWORD               dwNameSpace;   // namespace ??
    GUID                sGuid;         // GUID describing service type
    LPSTR               lpszName;      // ptr to name that needs resolution
    FILETIME            ftLastUsedTime;    // last accesstime, mainly for purging
    FILETIME            ftCreationTime;// When it was created
    ADDRESS_INFO_LIST   sAddrList;     // List of address (defined in ixport.h)
} NAMERES_CACHE, far *LPNAMERES_CACHE;





//
// private variables for name resolution cache
//


// Name cache size allocated in init
LPNAMERES_CACHE vlpNameresCache = NULL;

// Number of elements allowed in the nameres cache
int vcntNameresCacheEntries = DEFAULT_NAMERES_CACHE_ENTRIES;


// time in 100ns after which an address is expired
LONGLONG vftExpiryDelta = DEFAULT_EXPIRY_DELTA;

BOOL vfNameresCacheInited = FALSE;

// serialization
CRITICAL_SECTION vcritNameresCache;

//
// private function prototypes
//


PRIVATE
DWORD
CreateNameresCacheEntry(
    int     indx,
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName,
    INT     cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
);


PRIVATE
DWORD
DeleteNameresCacheEntry(
    int indx
);


PRIVATE
int
FindNameresCacheEntry(
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName
);


PRIVATE
int
FindNameresCacheEntryByAddr(
    int cntAddr,
    LPCSADDR_INFO lpCsaddrInfo
);

PRIVATE
int
PurgeEntries(
    BOOL    fForce  // purge atleast one entry
);


PRIVATE
DWORD
CopyCsaddr(
    LPCSADDR_INFO   lpSrc,
    int             cntAddr,
    LPCSADDR_INFO   *lplpDst
);

//
// functions
//


DWORD
InitNameresCache(
    VOID
)
/*++

Routine Description:

    Init name resolution cache. This routine a) allocates a table of
    name cache entries b)

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.

--*/
{


    if (vfNameresCacheInited) {
        return (ERROR_SUCCESS);
    }

    // first try to alloc the memory, if it fails just quit
    vlpNameresCache = (LPNAMERES_CACHE)ALLOCATE_MEMORY(
                        LPTR,
                        vcntNameresCacheEntries * sizeof(NAMERES_CACHE)
                        );

    if (!vlpNameresCache) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    InitializeCriticalSection(&vcritNameresCache);

    ENTERCRIT_NAMERESCACHE();

    vfNameresCacheInited = TRUE;

    LEAVECRIT_NAMERESCACHE();

    return (ERROR_SUCCESS);

}


DWORD
AddNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    int      cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_SUCCESS;

    if (!vfNameresCacheInited) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTERCRIT_NAMERESCACHE();

    indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpName);

    // if indx is valid, delete the entry, do some purging too
    if (indx != -1) {
        DeleteNameresCacheEntry(indx);
        PurgeEntries(FALSE);
    }
    else {
        // create atleast one hole
        indx = PurgeEntries(TRUE);
    }

    INET_ASSERT((indx >=0 && (indx < vcntNameresCacheEntries)));

    dwError = CreateNameresCacheEntry(indx,
                            dwNameSpace,
                            lpGuid,
                            lpName,
                            cntAddresses,
                            lpCsaddrInfo);

    LEAVECRIT_NAMERESCACHE();

    return (dwError);
}




DWORD
RemoveNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpszName
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (vfNameresCacheInited) {

        ENTERCRIT_NAMERESCACHE();

        indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpszName);

        if (indx != -1) {

            DeleteNameresCacheEntry(indx);

            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_FILE_NOT_FOUND; //yuk
        }

        LEAVECRIT_NAMERESCACHE();
    }
    return (dwError);
}


#ifdef MAYBE

DWORD
RemoveNameresCacheEntryByAddr(
    int cntAddresses,
    LPCSADDR_INFO lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (vfNameresCacheInited) {
        ENTERCRIT_NAMERESCACHE();

        indx = FindNameresCacheEntryByAddr(cntAddresses, lpCsaddrInfo);

        if (indx != -1) {

            DeleteNameresCacheEntry(indx);

            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_FILE_NOT_FOUND;
        }

        LEAVECRIT_NAMERESCACHE();
    }
    return (dwError);

}
#endif //MAYBE

DWORD
GetNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    INT      *lpcntAddresses,
    LPCSADDR_INFO  *lplpCsaddrInfo
)
/*++

Routine Description:

    This routine looks up the cache and returns the list of addresses
    corresponding to lpGuid/lpName.

Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD   dwError = ERROR_FILE_NOT_FOUND; // lame error

    if (!vfNameresCacheInited) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTERCRIT_NAMERESCACHE();

    // is this entry already cached?
    indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpName);


    if (indx != -1) {
        // yes, let use give back the info

        *lpcntAddresses = vlpNameresCache[indx].sAddrList.AddressCount;

        if ((dwError = CopyCsaddr(vlpNameresCache[indx].sAddrList.Addresses, *lpcntAddresses, lplpCsaddrInfo))
            != ERROR_SUCCESS) {

            goto bailout;
        }
        // update the last used time, we will use this to
        // age out the entries

        GetCurrentGmtTime(&(vlpNameresCache[indx].ftLastUsedTime));
        dwError = ERROR_SUCCESS;
    }

bailout:

    LEAVECRIT_NAMERESCACHE();

    return (dwError);
}


DWORD
DeinitNameresCache(
    VOID
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    if (vfNameresCacheInited) {
        ENTERCRIT_NAMERESCACHE();

        for (i = 0; i < vcntNameresCacheEntries; ++i) {
            if (!IS_EMPTY(i)) {
                DeleteNameresCacheEntry(i);
            }
        }

        FREE_MEMORY(vlpNameresCache);

        vlpNameresCache = NULL;

        vfNameresCacheInited = FALSE;

        LEAVECRIT_NAMERESCACHE();
        DeleteCriticalSection(&vcritNameresCache);
    }
    return (ERROR_SUCCESS);
}


PRIVATE
DWORD
CreateNameresCacheEntry(
    int     indx,
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName,
    int     cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    DWORD dwError = ERROR_NOT_ENOUGH_MEMORY;

    INET_ASSERT((indx >=0 && (indx < vcntNameresCacheEntries)));

    INET_ASSERT(IS_EMPTY(indx));


    memset(&vlpNameresCache[indx], 0, sizeof(vlpNameresCache[indx]));

    // we could get a name or a guid
    // do it for name first before doing it for GUID

    // BUGBUG in future we should consider name+GUID+port
    if (lpszName) {
       vlpNameresCache[indx].lpszName = (LPSTR)ALLOCATE_MEMORY(LPTR, lstrlen(lpszName)+1);
       if (!vlpNameresCache[indx].lpszName) {
           goto bailout;
       }
       strcpy(vlpNameresCache[indx].lpszName, lpszName);
    }
    else if (lpGuid) {
        INET_ASSERT(FALSE); // rigth now. In future this should go away
        memcpy(&(vlpNameresCache[indx].sGuid), lpGuid, sizeof(GUID));
        vlpNameresCache[indx].dwFlags |= NAMERES_CACHE_USES_GUID;
    }
    else {
        dwError = ERROR_INVALID_PARAMETER;
        goto bailout;
    }

    INET_ASSERT(cntAddresses > 0);

    if (CopyCsaddr(lpCsaddrInfo, cntAddresses, &(vlpNameresCache[indx].sAddrList.Addresses))
        != ERROR_SUCCESS) {
        goto bailout;
    }

    vlpNameresCache[indx].sAddrList.AddressCount = cntAddresses;

    // mark this as being non-empty
    vlpNameresCache[indx].dwFlags |= NAMERES_CACHE_USED;

    // set the creation and last-used times as now

    GetCurrentGmtTime(&(vlpNameresCache[indx].ftCreationTime));
    vlpNameresCache[indx].ftLastUsedTime = vlpNameresCache[indx].ftCreationTime ;

    dwError = ERROR_SUCCESS;

bailout:

    if (dwError != ERROR_SUCCESS) {
        if (vlpNameresCache[indx].sAddrList.Addresses) {
            FREE_MEMORY(vlpNameresCache[indx].sAddrList.Addresses);
            vlpNameresCache[indx].sAddrList.Addresses = NULL;
        }
        if (vlpNameresCache[indx].lpszName) {
            FREE_MEMORY(vlpNameresCache[indx].lpszName);
            vlpNameresCache[indx].lpszName = NULL;
        }
        memset(&vlpNameresCache[indx], 0, sizeof(vlpNameresCache[indx]));
    }

    return (dwError);
}


PRIVATE
DWORD
DeleteNameresCacheEntry(
    int indx
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    INET_ASSERT((indx >=0) && (indx < vcntNameresCacheEntries));

    if (vlpNameresCache[indx].lpszName) {
        FREE_MEMORY(vlpNameresCache[indx].lpszName);
    }

    INET_ASSERT(vlpNameresCache[indx].sAddrList.Addresses);

    FREE_MEMORY(vlpNameresCache[indx].sAddrList.Addresses);

    memset(&vlpNameresCache[indx], 0, sizeof(NAMERES_CACHE));

    return (ERROR_SUCCESS);
}

#ifdef MAYBE

PRIVATE
int
FindNameresCacheEntryByAddr(
    int cntAddr,
    LPCSADDR_INFO lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i) && // not empty
            (vlpNameresCache[i].sAddrList.AddressCount == cntAddr) && // count is the same
            (!memcmp(vlpNameresCache[i].sAddrList.Addresses,    // list matches
                     lpCsaddrInfo,
                     cntAddr * sizeof(CSADDR_INFO)))) {
            return (i);
        }
    }
    return (-1);
}
#endif //MAYBE


PRIVATE
int
FindNameresCacheEntry(
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i)) {
            if (vlpNameresCache[i].dwNameSpace == dwNameSpace) {
                if (!USES_GUID(i)) {

                    INET_ASSERT(vlpNameresCache[i].lpszName);

                    if (lpszName &&
                        !lstrcmpi(lpszName, vlpNameresCache[i].lpszName)) {
                        return (i);
                    }
                }
                else{

                    if (lpGuid && !memcmp(lpGuid, &vlpNameresCache[i].sGuid, sizeof(GUID))) {
                        return (i);
                    }
                }
            }
        }
    }
    return (-1);
}


PRIVATE
int
PurgeEntries(
    BOOL    fForce  // purge atleast one entry
)
/*++

Routine Description:


Arguments:


Return Value:

    index of a free entry

--*/
{
    int i, indxlru = -1, indxHole=-1;
    FILETIME ft;
    BOOL fFoundHole = FALSE;

    GetCurrentGmtTime(&ft);

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i)) {

            // purge stale entries
            if ( (FT2LL(ft) - FT2LL(vlpNameresCache[i].ftCreationTime))
                    > FT2LL(vftExpiryDelta)) {
                DeleteNameresCacheEntry(i);
                indxHole = i;
            }
            else if (FT2LL(vlpNameresCache[i].ftLastUsedTime) <= FT2LL(ft)) {
                ft = vlpNameresCache[i].ftLastUsedTime;
                indxlru = i; // LRU entry if we need to purge it
            }
        }
        else {
            indxHole = i;
        }
    }

    // if there is no hole, purge the LRU entry if forced
    if (indxHole == -1) {

        INET_ASSERT(indxlru != -1);

        if (fForce) {
            DeleteNameresCacheEntry(indxlru);
            indxHole = indxlru;
        }
    }
    return (indxHole);
}

PRIVATE
DWORD
CopyCsaddr(
    LPCSADDR_INFO   lpSrc,
    int             cntAddr,
    LPCSADDR_INFO   *lplpDst
)
{
    int i;
    LPCSADDR_INFO lpDst;
    UINT uSize;


    // BUGBUG assumes the way Compressaddress (ixport.cxx) allocates memory
    uSize = LocalSize(lpSrc);
    if (!uSize) {
        return (GetLastError());
    }

    *lplpDst = (LPCSADDR_INFO)ALLOCATE_MEMORY(LPTR, uSize);

    if (!*lplpDst) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    lpDst = *lplpDst;


    memcpy(lpDst, lpSrc, uSize);

    // now start doing fixups
    for (i=0; i<cntAddr; ++i) {
        lpDst[i].LocalAddr.lpSockaddr = (LPSOCKADDR)((LPBYTE)lpDst+((DWORD)(lpSrc[i].LocalAddr.lpSockaddr) - (DWORD)lpSrc));
        lpDst[i].RemoteAddr.lpSockaddr = (LPSOCKADDR)((LPBYTE)lpDst+((DWORD)(lpSrc[i].RemoteAddr.lpSockaddr) - (DWORD)lpSrc));
    }
    return (ERROR_SUCCESS);
}

#endif // defined(RNR_SUPPORTED)
