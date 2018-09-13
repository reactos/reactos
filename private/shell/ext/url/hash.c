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
//  09/08/93 - Created  KurtE
//
//---------------------------------------------------------------------------

#ifdef STRICT
#undef STRICT
#endif
#define NO_SHELL_HEAP_ALLOCATOR  //Kludge to get around new mem heaps in nt

#define NO_SHELL_VALIDATION

// to fix assert problem
#ifdef ASSERT
#undef ASSERT
#endif

#include "project.h"
#include "urlshell.h"
#include <platform.h>


//--------------------------------------------------------------------------
// First define a data structure to use to maintain the list

#define PRIME   37

// NOTE a PHASHITEM is defined as a LPCSTR externaly (for old code to work)
#undef PHASHITEM
typedef struct _HashItem FAR * PHASHITEM;

typedef struct _HashItem
{
    PHASHITEM   phiNext;        //
    UINT        wCount;         // Usage count
    BYTE        cbLen;          // Length of name in characters.
    char        szName[1];      // name
} HASHITEM;

typedef struct _HashTable
{
    UINT    wBuckets;           // Number of buckets
    UINT    wcbExtra;           // Extra bytes per item
    BOOL    fUpperCase;         // Uppercase names
    PHASHITEM phiLast;          // Pointer to the last item we worked with
    PHASHITEM ahiBuckets[EMPTY_SIZE];    // Set of buckets for the table
} HASHTABLE, FAR * PHASHTABLE;

#define HIFROMSZ(sz)  ((PHASHITEM)((BYTE*)(sz) - FIELDOFFSET(HASHITEM, szName)))
#define HIDATAPTR(sz) ((DWORD UNALIGNED *)((BYTE*)(sz) + HIFROMSZ(sz)->cbLen+1))

#define  LOOKUPHASHITEM     0
#define  ADDHASHITEM        1
#define  DELETEHASHITEM     2

static PHASHTABLE g_pHashTable = NULL;

PHASHTABLE NEAR PASCAL GetGlobalHashTable();

//--------------------------------------------------------------------------
// This function looks up the name in the hash table and optionally does
// things like add it, or delete it.
//

//// BUGBUG
// we will need to serialize this eventually.

LPCSTR NEAR PASCAL LookupItemInHashTable(PHASHTABLE pht, LPCSTR pszName, int iOp)
{
    // First thing to do is calculate the hash value for the item
    DWORD   dwHash = 0;
    UINT    wBucket;
    BYTE    cbName = 0;
    BYTE    c;
    PHASHITEM phi, phiPrev;
    LPCSTR psz = pszName;

    if (pht == NULL) {
        pht = GetGlobalHashTable();
    }

    while (*psz)
    {
        // Same type of hash like HashItem manager
        c = *psz++;

        if (pht->fUpperCase && ( c >= 'a') && (c <= 'z'))
            c = c - 'a' + 'A';
        dwHash += (c << 1) + (c >> 1) + c;
        cbName++;
        ASSERT(cbName);

        if (cbName == 0)
            return(NULL);       // Length to long!
    }

    // now search for the item in the buckets.
    phiPrev = NULL;
    ENTERCRITICAL;
    phi = pht->ahiBuckets[wBucket = (UINT)(dwHash % pht->wBuckets)];

    while (phi)
    {
        if (phi->cbLen == cbName)
        {
            if (pht->fUpperCase)
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
#ifdef HITTRACE
            DebugMsg(DM_TRACE, "Add Hit on '%s'", pszName);
#endif
            phi->wCount++;
        }
        else
        {
#ifdef HITTRACE
            DebugMsg(DM_TRACE, "Add MISS on '%s'", pszName);
#endif
            // Not Found, try to allocate it out of the heap
            phi = (PHASHITEM)Alloc(sizeof(HASHITEM) + cbName + pht->wcbExtra);

            if (phi != NULL)
            {
                // Initialize it
                phi->wCount = 1;        // One use of it
                phi->cbLen = cbName;        // The length of it;
                lstrcpy(phi->szName, pszName);

                // And link it in to the right bucket
                phi->phiNext = pht->ahiBuckets[wBucket];
                pht->ahiBuckets[wBucket] = phi;
            }
        }
        pht->phiLast = phi;
        break;

    case DELETEHASHITEM:
        if (phi)
        {
            phi->wCount--;
            if (phi->wCount == 0)
            {
                // Useage count went to zero so unlink it and delete it
                if (phiPrev != NULL)
                    phiPrev->phiNext = phi->phiNext;
                else
                    pht->ahiBuckets[wBucket] = phi->phiNext;

                // And delete it
                Free(phi);

                phi = NULL;
            }
        }
        
        case LOOKUPHASHITEM:
            pht->phiLast = phi;
            break;
    }

    LEAVECRITICAL;
    
    // If find was passed in simply return it.
    if (phi)
        return (LPCSTR)phi->szName;
    else
        return NULL;
}

//--------------------------------------------------------------------------

LPCSTR WINAPI AddHashItem(PHASHTABLE pht, LPCSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, ADDHASHITEM);
}

//--------------------------------------------------------------------------

LPCSTR WINAPI DeleteHashItem(PHASHTABLE pht, LPCSTR lpszStr)
{
    return LookupItemInHashTable(pht, lpszStr, DELETEHASHITEM);
}

//======================================================================

PHASHTABLE WINAPI CreateHashItemTable(UINT wBuckets, UINT wExtra, BOOL fCaseSensitive)
{
    PHASHTABLE pht;

    if (wBuckets == 0)
        wBuckets = 71;

    pht = (PHASHTABLE)Alloc(sizeof(HASHTABLE) + wBuckets * sizeof(PHASHITEM));

    if (pht) {
        pht->fUpperCase = !fCaseSensitive;
        pht->wBuckets = wBuckets;
        pht->wcbExtra = wExtra;
    }
    
    return pht;    
}

//======================================================================

PHASHTABLE NEAR PASCAL GetGlobalHashTable()
{
    if (g_pHashTable == NULL) {
        g_pHashTable = CreateHashItemTable(71, 0, FALSE);
    }
    return g_pHashTable;
}

