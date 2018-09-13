
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: Hash.c
//
// Comments:
//      This file contains functions that are roughly equivelent to the
//      kernel atom function.  There are two main differences.  The first
//      is that in 32 bit land the tables are maintined in our shared heap,
//      which makes it shared between all of our apps.  The second is that
//      we can assocate a long pointer with each of the items, which in many
//      cases allows us to keep from having to do a secondary lookup from
//      a different table
//
// History:
//  09/08/93 - Created                                      KurtE
//  ??/??/94 - ported for unicode                           (anonymous)
//  10/26/95 - rearranged hashitems for perf, alignment     FrancisH
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#define DM_PERF     0           // perf stats

//--------------------------------------------------------------------------
// First define a data structure to use to maintain the list

#define PRIME                   37
#define DEF_HASH_BUCKET_COUNT   71

// NOTE a PHASHITEM is defined as a LPCSTR externaly (for old code to work)
#undef PHASHITEM
typedef struct _HashItem * PHASHITEM;

//-----------------------------------------------------------------------------
//
// Hash item layout:
//
//  [extra data][_HashItem struct][item text]
//
//-----------------------------------------------------------------------------

typedef struct _HashItem
{
    //
    // this part of the struct is aligned
    //
    PHASHITEM   phiNext;        //
    WORD        wCount;         // Usage count
    WORD        cchLen;          // Length of name in characters.

    //
    // this member is just a placeholder
    //
    TCHAR        szName[1];      // name

} HASHITEM;

#pragma warning(disable:4200)   // Zero-sized array in struct

typedef struct _HashTable
{
    UINT    wBuckets;           // Number of buckets
    UINT    wcbExtra;           // Extra bytes per item
    BOOL    fCaseSensitive;     // hash name sensitivity
    LPCTSTR pszHTCache;         // MRU ptr for last lookup/add/etc.
    PHASHITEM ahiBuckets[0];    // Set of buckets for the table
} HASHTABLE, * PHASHTABLE;

#define HIFROMSZ(sz)            ((PHASHITEM)((BYTE*)(sz) - FIELD_OFFSET(HASHITEM, szName)))
#define HIDATAPTR(pht, sz)      ((void *)(((BYTE *)HIFROMSZ(sz)) - (pht? pht->wcbExtra : 0)))
#define HIDATAARRAY(pht, sz)    ((DWORD_PTR *)HIDATAPTR(pht, sz))

#define  LOOKUPHASHITEM     0
#define  ADDHASHITEM        1
#define  DELETEHASHITEM     2
#define  PURGEHASHITEM      3   // DANGER: EVIL!

static PHASHTABLE g_pHashTable = NULL;

PHASHTABLE GetGlobalHashTable();

//--------------------------------------------------------------------------
// This function allocs a hashitem.
//
PHASHITEM _AllocHashItem(PHASHTABLE pht, DWORD cchName)
{
    BYTE *mem;

    ASSERT(pht);

    mem = (BYTE *)Alloc(SIZEOF(HASHITEM) + (cchName * SIZEOF(TCHAR)) + pht->wcbExtra);

    if (mem)
        mem += pht->wcbExtra;

    return (PHASHITEM)mem;
}

//--------------------------------------------------------------------------
// This function frees a hashitem.
//
__inline void _FreeHashItem(PHASHTABLE pht, PHASHITEM phi)
{
    ASSERT(pht && phi);
    Free((BYTE *)phi - pht->wcbExtra);
}

// PERF_CACHE
//***   c_szHTNil -- 1-element MRU for hashtable
// DESCRIPTION
//  it turns out we have long runs of duplicate lookups (e.g. "Directory"
// and ".lnk").  a 1-element MRU is a v. cheap way of speeding things up.

// rather than check for the (rare) special case of NULL each time we
// check our cache, we pt at at this guy.  then iff we think it's a
// cache hit, we make sure it's not pting at this special guy.
const TCHAR c_szHTNil[] = TEXT("");     // arbitrary value, unique-&

#ifdef DEBUG
int g_cHTTot, g_cHTHit;
int g_cHTMod = 100;
#endif

//--------------------------------------------------------------------------
// This function looks up the name in the hash table and optionally does
// things like add it, or delete it.
//
LPCTSTR LookupItemInHashTable(PHASHTABLE pht, LPCTSTR pszName, int iOp)
{
    // First thing to do is calculate the hash value for the item
    DWORD   dwHash = 0;
    UINT    wBucket;
    BYTE    cchName = 0;
    PHASHITEM phi, phiPrev;
    LPCTSTR pch;
    TCHAR sz[MAX_PATH];

    if (pht == NULL) {
        pht = GetGlobalHashTable();
        if (pht == NULL)
            return NULL;
    }

#ifdef DEBUG
    if ((g_cHTTot % g_cHTMod) == 0)
        TraceMsg(DM_PERF, "ht: tot=%d hit=%d", g_cHTTot, g_cHTHit);
#endif
    DBEXEC(TRUE, g_cHTTot++);
    if (*pszName == *pht->pszHTCache && iOp == LOOKUPHASHITEM) {
        ENTERCRITICAL;
        // StrCmpC is a fast ansi strcmp, good enough for a quick/approx check
        if (StrCmpC(pszName, pht->pszHTCache) == 0 && EVAL(pht->pszHTCache != c_szHTNil)) {
            ASSERT(pht->pszHTCache != c_szHTNil);  // collided w/ our sentinel
            DBEXEC(TRUE, g_cHTHit++);

            LEAVECRITICAL;          // see 'semi-race' comment below
            return (LPCTSTR)pht->pszHTCache;

#if 0 // currently not worth it (very few ADDHASHITEMs of existing)
            // careful!  this is o.k. for ADDHASHITEM but not for (e.g.)
            // DELETEHASHITEM (which needs phiPrev)
            phi = HIFROMSZ(pht->pszHTCache);
            goto Ldoop;     // warning: pending ENTERCRITICAL
#endif
        }
        LEAVECRITICAL;
    }

    //  init the pch
    if (!pht->fCaseSensitive)
    {
        //  
        //  if we are not case sensitive, make it lower case.
        //  this should be cheaper than upper because in most
        //  places in the shell we prettify the names to 
        //  be mostly lower case.
        //
        StrCpyN(sz, pszName, SIZECHARS(sz));
        pch = CharLower(sz);
    }
    else
        pch = pszName;

    //  calc hash
    while (*pch)
    {
        TUCHAR  c = *pch++;

        dwHash += (c << 1) + (c >> 1) + c;

        //  keep track of the length
        cchName++;

        //  OverFlow!!
        ASSERT(cchName);
        if (cchName == 0)
            return(NULL);       // Length to long!
    }

    // now search for the item in the buckets.
    phiPrev = NULL;
    ENTERCRITICAL;
    phi = pht->ahiBuckets[wBucket = (UINT)(dwHash % pht->wBuckets)];

    while (phi)
    {
        if (phi->cchLen == cchName)
        {
            if (!pht->fCaseSensitive)
            {
                if (!lstrcmpi(pszName, phi->szName))
                    break;      // Found match
            }
            else
            {
                if (!lstrcmp(pszName, phi->szName))
                    break;      // Found match
            }
        }
        phiPrev = phi;      // Keep the previous item
        phi = phi->phiNext;
    }

    //
    // Sortof gross, but do the work here
    //
    switch (iOp)
    {
    case ADDHASHITEM:
        if (phi)
        {
            // Simply increment the reference count
            DebugMsg(TF_HASH, TEXT("Add Hit on '%s'"), pszName);

            phi->wCount++;
        }
        else
        {
            DebugMsg(TF_HASH, TEXT("Add MISS on '%s'"), pszName);

            // Not Found, try to allocate it out of the heap
            if ((phi = _AllocHashItem(pht, cchName)) != NULL)
            {
                // Initialize it
                phi->wCount = 1;        // One use of it
                phi->cchLen = cchName;        // The length of it;
                lstrcpy(phi->szName, pszName);

                // And link it in to the right bucket
                phi->phiNext = pht->ahiBuckets[wBucket];
                pht->ahiBuckets[wBucket] = phi;
            }
        }
        break;

    case PURGEHASHITEM:
    case DELETEHASHITEM:
        if (phi && ((iOp == PURGEHASHITEM) || (!--phi->wCount)))
        {
            // Useage count went to zero so unlink it and delete it
            if (phiPrev != NULL)
                phiPrev->phiNext = phi->phiNext;
            else
                pht->ahiBuckets[wBucket] = phi->phiNext;

            // And delete it
            _FreeHashItem(pht, phi);
            phi = NULL;
        }
    }

    // kill cache if this was a PURGE/DELETEHASHITEM, o.w. cache it.
    // note that there's a semi-race on pht->pszHTCache ops, viz. that
    // we LEAVECRITICAL but then return a ptr into our table.  however
    // it's 'no worse' than the existing races.  so i guess the caller
    // is supposed to avoid a concurrent lookup/delete.
    pht->pszHTCache = phi ? phi->szName : c_szHTNil;

    LEAVECRITICAL;

    // If find was passed in simply return it.
    if (phi)
        return (LPCTSTR)phi->szName;
    else
        return NULL;
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI FindHashItem(PHASHTABLE pht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, LOOKUPHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI AddHashItem(PHASHTABLE pht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, ADDHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI DeleteHashItem(PHASHTABLE pht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, DELETEHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI PurgeHashItem(PHASHTABLE pht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, PURGEHASHITEM);
}

//--------------------------------------------------------------------------
// this sets the extra data in an HashItem

void WINAPI SetHashItemData(PHASHTABLE pht, LPCTSTR sz, int n, DWORD_PTR dwData)
{
    // string must be from the hash table
    ASSERT(FindHashItem(pht, sz) == sz);

    // the default hash table does not have extra data!
    if (pht != NULL && n <= (int)(pht->wcbExtra/SIZEOF(DWORD_PTR)))
        HIDATAARRAY(pht, sz)[n] = dwData;
}

//======================================================================
// this is like SetHashItemData, except it gets the HashItem data...

DWORD_PTR WINAPI GetHashItemData(PHASHTABLE pht, LPCTSTR sz, int n)
{
    // string must be from the hash table
    ASSERT(FindHashItem(pht, sz) == sz);

    // the default hash table does not have extra data!
    if (pht != NULL && n <= (int)(pht->wcbExtra/SIZEOF(DWORD_PTR)))
        return HIDATAARRAY(pht, sz)[n];
    else
        return 0;
}

//======================================================================
// like GetHashItemData, except it just gets a pointer to the buffer...

void * WINAPI GetHashItemDataPtr(PHASHTABLE pht, LPCTSTR sz)
{
    // string must be from the hash table
    ASSERT(FindHashItem(pht, sz) == sz);

    // the default hash table does not have extra data!
    return (pht? HIDATAPTR(pht, sz) : NULL);
}

//======================================================================

PHASHTABLE WINAPI CreateHashItemTable(UINT wBuckets, UINT wExtra, BOOL fCaseSensitive)
{
    PHASHTABLE pht;

    if (wBuckets == 0)
        wBuckets = DEF_HASH_BUCKET_COUNT;

    pht = (PHASHTABLE)Alloc(SIZEOF(HASHTABLE) + wBuckets * SIZEOF(PHASHITEM));

    if (pht) {
        pht->fCaseSensitive = fCaseSensitive;
        pht->wBuckets = wBuckets;
        pht->wcbExtra = (wExtra + sizeof(DWORD_PTR) - 1) / sizeof(DWORD_PTR) * sizeof(DWORD_PTR);
        pht->pszHTCache = c_szHTNil;
    }

    return pht;
}

//======================================================================

void WINAPI EnumHashItems(PHASHTABLE pht, HASHITEMCALLBACK callback, DWORD dwParam)
{
    ENTERCRITICAL;

    if (pht == NULL)
        pht = g_pHashTable;

    if (pht) {
        int i;
        PHASHITEM phi;
        PHASHITEM phiNext;

        for (i=0; i<(int)pht->wBuckets; i++) {
            for (phi=pht->ahiBuckets[i]; phi; phi=phiNext) {
                phiNext = phi->phiNext;
                (*callback)(pht, phi->szName, phi->wCount, dwParam);
            }
        }
    }

    LEAVECRITICAL;
} 

//======================================================================

void _DeleteHashItem(PHASHTABLE pht, LPCTSTR sz, UINT usage, DWORD param)
{
    _FreeHashItem(pht, HIFROMSZ(sz));
} 

//======================================================================

void WINAPI DestroyHashItemTable(PHASHTABLE pht)
{
    ENTERCRITICAL;

    if (pht == NULL) {
        pht = g_pHashTable;
        g_pHashTable = NULL;
    }

    if (pht) {
        EnumHashItems(pht, _DeleteHashItem, 0);
        Free(pht);
    }

    LEAVECRITICAL;
} 


//======================================================================

PHASHTABLE GetGlobalHashTable()
{
    if (!g_pHashTable)
    {
        ENTERCRITICAL;

        if (!g_pHashTable)
            g_pHashTable = CreateHashItemTable(0, 0, FALSE);

        LEAVECRITICAL;
    }

    return g_pHashTable;
}

//======================================================================

#ifdef DEBUG

static int TotalBytes;

void CALLBACK _DumpHashItem(PHASHTABLE pht, LPCTSTR sz, UINT usage, DWORD param)
{
    DebugMsg(TF_ALWAYS, TEXT("    %08x %5ld \"%s\""), HIFROMSZ(sz), usage, sz);
    TotalBytes += (HIFROMSZ(sz)->cchLen * SIZEOF(TCHAR)) + SIZEOF(HASHITEM);
}

void CALLBACK _DumpHashItemWithData(PHASHTABLE pht, LPCTSTR sz, UINT usage, DWORD param)
{
    DebugMsg(TF_ALWAYS, TEXT("    %08x %5ld %08x \"%s\""), HIFROMSZ(sz), usage, HIDATAARRAY(pht, sz)[0], sz);
    TotalBytes += (HIFROMSZ(sz)->cchLen * SIZEOF(TCHAR)) + SIZEOF(HASHITEM) + (pht? pht->wcbExtra : 0);
}

void WINAPI DumpHashItemTable(PHASHTABLE pht)
{
    ENTERCRITICAL;
    TotalBytes = 0;

    if (IsFlagSet(g_dwDumpFlags, DF_HASH))
    {
        DebugMsg(TF_ALWAYS, TEXT("Hash Table: %08x"), pht);

        if (pht && (pht->wcbExtra > 0)) {
            DebugMsg(TF_ALWAYS, TEXT("    Hash     Usage dwEx[0]  String"));
            DebugMsg(TF_ALWAYS, TEXT("    -------- ----- -------- ------------------------------"));
            EnumHashItems(pht, _DumpHashItemWithData, 0);
        }
        else {
            DebugMsg(TF_ALWAYS, TEXT("    Hash     Usage String"));
            DebugMsg(TF_ALWAYS, TEXT("    -------- ----- --------------------------------"));
            EnumHashItems(pht, _DumpHashItem, 0);
        }

        DebugMsg(TF_ALWAYS, TEXT("Total Bytes: %d"), TotalBytes);
    }
    LEAVECRITICAL;
}

#endif
