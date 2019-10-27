/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        base/services/dnsrslvr/cache.c
 * PURPOSE:     DNS cache functions
 * PROGRAMMER:  Peter Hater
 */

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

static RESOLVER_CACHE DnsCache;
static BOOL DnsCacheInitialized = FALSE;

#define DnsCacheLock()          do { EnterCriticalSection(&DnsCache.Lock); } while (0)
#define DnsCacheUnlock()        do { LeaveCriticalSection(&DnsCache.Lock); } while (0)

VOID
DnsIntCacheInitialize(VOID)
{
    DPRINT("DnsIntCacheInitialize\n");

    /* Check if we're initialized */
    if (DnsCacheInitialized)
        return;

    /* Initialize the cache lock and namespace list */
    InitializeCriticalSection((LPCRITICAL_SECTION)&DnsCache.Lock);
    InitializeListHead(&DnsCache.RecordList);
    DnsCacheInitialized = TRUE;
}

VOID
DnsIntCacheFree(VOID)
{
    DPRINT("DnsIntCacheFree\n");

    /* Check if we're initialized */
    if (!DnsCacheInitialized)
        return;

    if (!DnsCache.RecordList.Flink)
        return;

    DnsIntCacheFlush();

    DeleteCriticalSection(&DnsCache.Lock);
    DnsCacheInitialized = FALSE;
}
 
VOID
DnsIntCacheRemoveEntryItem(PRESOLVER_CACHE_ENTRY CacheEntry)
{
    DPRINT("DnsIntCacheRemoveEntryItem %p\n", CacheEntry);

    /* Remove the entry from the list */
    RemoveEntryList(&CacheEntry->CacheLink);
 
    /* Free record */
    DnsRecordListFree(CacheEntry->Record, DnsFreeRecordList);

    /* Delete us */
    HeapFree(GetProcessHeap(), 0, CacheEntry);
}

VOID
DnsIntCacheFlush(VOID)
{
    PLIST_ENTRY Entry;
    PRESOLVER_CACHE_ENTRY CacheEntry;

    DPRINT("DnsIntCacheFlush\n");

    /* Lock the cache */
    DnsCacheLock();

    /* Loop every entry */
    Entry = DnsCache.RecordList.Flink;
    while (Entry != &DnsCache.RecordList)
    {
        /* Get this entry */
        CacheEntry = CONTAINING_RECORD(Entry, RESOLVER_CACHE_ENTRY, CacheLink);

        /* Remove it from list */
        DnsIntCacheRemoveEntryItem(CacheEntry);

        /* Move to the next entry */
        Entry = DnsCache.RecordList.Flink;
    }

    /* Unlock the cache */
    DnsCacheUnlock();
}

BOOL
DnsIntCacheGetEntryFromName(LPCWSTR Name,
                            PDNS_RECORDW *Record)
{
    BOOL Ret = FALSE;
    PRESOLVER_CACHE_ENTRY CacheEntry;
    PLIST_ENTRY NextEntry;

    DPRINT("DnsIntCacheGetEntryFromName %ws %p\n", Name, Record);

    /* Assume failure */
    *Record = NULL;

    /* Lock the cache */
    DnsCacheLock();

    /* Match the Id with all the entries in the List */
    NextEntry = DnsCache.RecordList.Flink;
    while (NextEntry != &DnsCache.RecordList)
    {
        /* Get the Current Entry */
        CacheEntry = CONTAINING_RECORD(NextEntry, RESOLVER_CACHE_ENTRY, CacheLink);

        /* Check if this is the Catalog Entry ID we want */
        if (_wcsicmp(CacheEntry->Record->pName, Name) == 0)
        {
            /* Copy the entry and return it */
            *Record = DnsRecordSetCopyEx(CacheEntry->Record, DnsCharSetUnicode, DnsCharSetUnicode);
            Ret = TRUE;
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    /* Release the cache */
    DnsCacheUnlock();

    /* Return */
    return Ret;
}

BOOL
DnsIntCacheRemoveEntryByName(LPCWSTR Name)
{
    BOOL Ret = FALSE;
    PRESOLVER_CACHE_ENTRY CacheEntry;
    PLIST_ENTRY NextEntry;

    DPRINT("DnsIntCacheRemoveEntryByName %ws\n", Name);

    /* Lock the cache */
    DnsCacheLock();

    /* Match the Id with all the entries in the List */
    NextEntry = DnsCache.RecordList.Flink;
    while (NextEntry != &DnsCache.RecordList)
    {
        /* Get the Current Entry */
        CacheEntry = CONTAINING_RECORD(NextEntry, RESOLVER_CACHE_ENTRY, CacheLink);

        /* Check if this is the Catalog Entry ID we want */
        if (_wcsicmp(CacheEntry->Record->pName, Name) == 0)
        {
            /* Remove the entry */
            DnsIntCacheRemoveEntryItem(CacheEntry);
            Ret = TRUE;
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    /* Release the cache */
    DnsCacheUnlock();

    /* Return */
    return Ret;
}

VOID
DnsIntCacheAddEntry(PDNS_RECORDW Record)
{
    PRESOLVER_CACHE_ENTRY Entry;

    DPRINT("DnsIntCacheRemoveEntryByName %p\n", Record);

    /* Lock the cache */
    DnsCacheLock();

    /* Match the Id with all the entries in the List */
    Entry = (PRESOLVER_CACHE_ENTRY)HeapAlloc(GetProcessHeap(), 0, sizeof(*Entry));
    if (!Entry)
        return;

    Entry->Record = DnsRecordSetCopyEx(Record, DnsCharSetUnicode, DnsCharSetUnicode);

    /* Insert it to our List */
    InsertTailList(&DnsCache.RecordList, &Entry->CacheLink);

    /* Release the cache */
    DnsCacheUnlock();
}
