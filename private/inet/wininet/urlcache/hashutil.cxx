/*++
Copyright (c) 1996  Microsoft Corp.

Module Name: hashutil.cxx

Abstract:

    Implementation of linked list of hash tables for cache index lookup.

Author:
    Rajeev Dujari (rajeevd) 22-Oct-96

--*/

#include <cache.hxx>

#define SIG_HASH ('H'|('A'<<8)|('S'<<16)|('H'<<24))

typedef LIST_FILEMAP_ENTRY HASH_FILEMAP_ENTRY;

// hash table parameters
#define BYTES_PER_PAGE 4096

#define ITEMS_PER_BUCKET ((BYTES_PER_PAGE - sizeof(HASH_FILEMAP_ENTRY))\
    / (SLOT_COUNT * sizeof(HASH_ITEM)))
#define BYTES_PER_TABLE (sizeof(HASH_FILEMAP_ENTRY) \
    + SLOT_COUNT * ITEMS_PER_BUCKET * sizeof(HASH_ITEM))


//
// Hash Function: Pearson's method
//

PRIVATE DWORD HashKey (LPCSTR lpsz)
{
    union
    {
        DWORD dw;
        BYTE c[4];
    }
    Hash, Hash2;
        
    const static BYTE bTranslate[256] =
    {
        1, 14,110, 25, 97,174,132,119,138,170,125,118, 27,233,140, 51,
        87,197,177,107,234,169, 56, 68, 30,  7,173, 73,188, 40, 36, 65,
        49,213,104,190, 57,211,148,223, 48,115, 15,  2, 67,186,210, 28,
        12,181,103, 70, 22, 58, 75, 78,183,167,238,157,124,147,172,144,
        176,161,141, 86, 60, 66,128, 83,156,241, 79, 46,168,198, 41,254,
        178, 85,253,237,250,154,133, 88, 35,206, 95,116,252,192, 54,221,
        102,218,255,240, 82,106,158,201, 61,  3, 89,  9, 42,155,159, 93,
        166, 80, 50, 34,175,195,100, 99, 26,150, 16,145,  4, 33,  8,189,
        121, 64, 77, 72,208,245,130,122,143, 55,105,134, 29,164,185,194,
        193,239,101,242,  5,171,126, 11, 74, 59,137,228,108,191,232,139,
        6, 24, 81, 20,127, 17, 91, 92,251,151,225,207, 21, 98,113,112,
        84,226, 18,214,199,187, 13, 32, 94,220,224,212,247,204,196, 43,
        249,236, 45,244,111,182,153,136,129, 90,217,202, 19,165,231, 71,
        230,142, 96,227, 62,179,246,114,162, 53,160,215,205,180, 47,109,
        44, 38, 31,149,135,  0,216, 52, 63, 23, 37, 69, 39,117,146,184,
        163,200,222,235,248,243,219, 10,152,131,123,229,203, 76,120,209
    };

    // Seed the hash values based on the first character.
    Hash.c[0] = bTranslate[ *lpsz];
    Hash.c[1] = bTranslate[(*lpsz+1) & 255];
    Hash.c[2] = bTranslate[(*lpsz+2) & 255];
    Hash.c[3] = bTranslate[(*lpsz+3) & 255];

    while (*++lpsz)
    {
        // Allow URLs differing only by trailing slash to collide.
        if (lpsz[0] == '/' && lpsz[1] == 0)
            break;

        Hash2.c[0] = Hash.c[0] ^ *lpsz;
        Hash2.c[1] = Hash.c[1] ^ *lpsz;
        Hash2.c[2] = Hash.c[2] ^ *lpsz;
        Hash2.c[3] = Hash.c[3] ^ *lpsz;
            
        Hash.c[0] = bTranslate[Hash2.c[0]];
        Hash.c[1] = bTranslate[Hash2.c[1]];
        Hash.c[2] = bTranslate[Hash2.c[2]];
        Hash.c[3] = bTranslate[Hash2.c[3]];
    }

    return Hash.dw;
}
    
//
// HashLookupItem support functions specific to urlcache:
//      AllocTable
//      IsMatch
//


PRIVATE HASH_FILEMAP_ENTRY* AllocTable
    (LPVOID pAllocObj, LPBYTE* ppBase, LPDWORD* ppdwOffset)
{
    // Save the offset to the table offset.
    DWORD_PTR dpOffsetToTableOffset = (LPBYTE)*ppdwOffset - *ppBase;  // 64BIT
    
    // Ask for BYTES_PER_PAGE instead of BYTES_PER_TABLE
    // so the allocator knows to align on a page boundary.
    INET_ASSERT (BYTES_PER_PAGE >= BYTES_PER_TABLE);
    MEMMAP_FILE* pmmf = (MEMMAP_FILE*) pAllocObj;
    HASH_FILEMAP_ENTRY* pTable =
        (HASH_FILEMAP_ENTRY *) pmmf->AllocateEntry (BYTES_PER_PAGE);
    if (!pTable)
        return NULL;  
    INET_ASSERT (! (((LPBYTE) pTable - *pmmf->GetHeapStart()) & (BYTES_PER_PAGE-1)) );

    // Chain new table to previous table.
    *ppBase = *pmmf->GetHeapStart();
    *ppdwOffset = (DWORD*) (*ppBase + dpOffsetToTableOffset);
    **ppdwOffset = (DWORD) ((LPBYTE)pTable - *ppBase);             // 64BIT
    
    // Initialize the header.
    pTable->dwSig = SIG_HASH;
    pTable->dwNext = 0;
    
    // Fill the rest of the entry with HASH_END
    DWORD* pdw = (DWORD *) (pTable + 1);
    DWORD cdw = SLOT_COUNT * ITEMS_PER_BUCKET * (sizeof(HASH_ITEM)/sizeof(DWORD));
    INET_ASSERT (!(sizeof(HASH_ITEM) % sizeof(DWORD)));
    while (cdw--)
        *pdw++ = HASH_END;

    // Return the new table.
    return pTable;
}

//
// IsMatch: determine if hash table item with a matching 32-bit hash value
// is an actual match or return NULL if a collision.
//

PRIVATE HASH_ITEM* URL_CONTAINER::IsMatch
    (HASH_ITEM *pItem, LPCSTR pszKey, DWORD dwFlags)
{
    MEMMAP_FILE* pmmf = _UrlObjStorage;

    dwFlags &= (LOOKUP_BIT_REDIR | LOOKUP_BIT_CREATE);

    if (pmmf->IsBadOffset (pItem->dwOffset))
    {
        // Fix up a bad hash table item.  This could happen if a thread
        // died between allocating a hash table item and setting the offset.
        pItem->MarkFree();
        return NULL;
    }

    FILEMAP_ENTRY* pEntry = (FILEMAP_ENTRY*)
        (*pmmf->GetHeapStart() + pItem->dwOffset);

    switch (pEntry->dwSig)
    {
        case SIG_URL:
        {        
            // Fail if lookup flags are inconsistent with url entry type.
            INET_ASSERT (!(pItem->dwHash & HASH_BIT_NOTURL));

            // Get pointer to URL.
            URL_FILEMAP_ENTRY *pUrlEntry = (URL_FILEMAP_ENTRY *) pEntry;
            LPSTR pszUrl = ((LPSTR) pUrlEntry) + pUrlEntry->UrlNameOffset;
            LPCSTR pszKey2 = pszKey, pszUrl2 = pszUrl;
            

            while ( *pszKey2 && *pszUrl2 && *pszKey2 == *pszUrl2 )
            {
                pszKey2++;
                pszUrl2++;
            }

            if (!*pszKey2 && ! *pszUrl2)
            {
                // Found exact match.

                if (dwFlags == LOOKUP_REDIR_CREATE)
                {
                    // We are have a cache entry for a URL which is now
                    // redirecting.  Delete the cache entry.
                    DeleteUrlEntry (pUrlEntry, pItem, SIG_DELETE);
                    return NULL;
                }
                return pItem;
            }

            // If redirects allowed, check for trailing slash match.
            if ((dwFlags == LOOKUP_URL_TRANSLATE)
                && (pItem->dwHash & HASH_BIT_REDIR))
            {
                DWORD cbUrl = strlen (pszUrl);
                DWORD cbKey = strlen (pszKey);
                INET_ASSERT (cbUrl && pszUrl[cbUrl - 1] == '/');
                if (cbUrl == (cbKey + 1) && !memcmp (pszUrl, pszKey, cbKey))
                    return pItem;
            }
                
            return NULL;
        }
        
        case SIG_REDIR:
        {
            // When online, filter out offline redirect entries.
            if (dwFlags == LOOKUP_URL_NOCREATE)
                return NULL;

            // Check that redirect URL matches exactly.
            REDIR_FILEMAP_ENTRY* pRedir = (REDIR_FILEMAP_ENTRY *) pEntry;
            if (lstrcmp (pszKey, pRedir->szUrl))
                return NULL;

            switch (dwFlags)
            {
                case LOOKUP_URL_CREATE:

                    // We are creating a new entry for a URL that once
                    // redirected.  Delete the stale redirect entry.
                    pmmf->FreeEntry (pRedir);
                    pItem->MarkFree();
                    return NULL;

                case LOOKUP_REDIR_CREATE:
                
                    // Return the redirect item if we're looking for it.
                    return pItem;

                case LOOKUP_URL_TRANSLATE:

                    // Otherwise, translate through the redirect item.
                    pItem = (HASH_ITEM *)
                        (*pmmf->GetHeapStart() + pRedir->dwItemOffset);

                    // Perform some consistency checks.
                    if (pItem->dwHash & HASH_BIT_NOTURL)
                        return NULL; // not an URL entry
                    if ((pItem->dwHash & ~SLOT_MASK) != pRedir->dwHashValue)
                        return NULL; // not a matching URL entry
                    return pItem;

                default:
                    INET_ASSERT (FALSE);                
            }
        }
        
        default:
        {
            // Fix up a bad hash table entry.  This can happen if a thread
            // died between allocating a hash table item and setting the offset.
            pItem->MarkFree();
            return NULL;
        }            
    }
}


//
// HashFindItem: finds a matching entry or else the first free slot
//

BOOL URL_CONTAINER::HashFindItem
    (LPCSTR pszKey, DWORD dwFlags, HASH_ITEM** ppItem)
{    
    LPVOID pAllocObj = (LPVOID) _UrlObjStorage;
    LPBYTE pBase = *_UrlObjStorage->GetHeapStart();
    LPDWORD pdwTableOffset = _UrlObjStorage->GetPtrToHashTableOffset();
    
    // Scan flags.
    BOOL fCreate = dwFlags & LOOKUP_BIT_CREATE;

    HASH_ITEM* pFree = NULL;
    DWORD nBlock = 0;

    // Hash the URL and calculate the slot.
    DWORD dwHash = HashKey(pszKey);
    DWORD iSlot = dwHash & SLOT_MASK;
    dwHash &= ~SLOT_MASK;

    // Walk through the list of hash tables.
    while (*pdwTableOffset && !_UrlObjStorage->IsBadOffset(*pdwTableOffset))
    {
        // Calculate offset to next hash table and validate signature.
        HASH_FILEMAP_ENTRY* pTable =
            (HASH_FILEMAP_ENTRY*) (pBase + *pdwTableOffset);
        if (pTable->dwSig != SIG_HASH || pTable->nBlock != nBlock++)
            break;

        // Calculate offset to bucket in this table.
        HASH_ITEM* pItem = ((HASH_ITEM*) (pTable + 1)) + iSlot * ITEMS_PER_BUCKET;

        // Scan the bucket.
        for (DWORD iSeat=0; iSeat<ITEMS_PER_BUCKET; iSeat++, pItem++)
        {
            // No reserved bits should ever be set on an item.
            INET_ASSERT (!(pItem->dwHash & HASH_BIT_RESERVED));

            switch (pItem->dwHash)
            {
                case HASH_FREE: // free item but more items might follow
                {
                    INET_ASSERT (!(pItem->dwHash & ~SLOT_MASK)); 
                    // If caller wants a free item, record the first one we find.
                    if (!pFree && fCreate)
                        pFree = pItem;
                }                        
                    continue;

                case HASH_END: // first previously unused free item; no more to follow
                {
                    INET_ASSERT (!(pItem->dwHash & ~SLOT_MASK)); 
                    if (!fCreate)
                        *ppItem = NULL;
                    else
                    {
                        // Hand out the first free slot.
                        if (pFree)
                        {
                            // Invalidate offset in case caller neglects to set it.
                            pFree->dwOffset = HASH_END;
                            *ppItem = pFree;
                        }
                        else
                        {
                            // The first free slot has never been used before.
                            INET_ASSERT (pItem->dwOffset == HASH_END);
                            *ppItem = pItem;
                        }
                        (*ppItem)->dwHash = dwHash;
                    }
                }
                    return FALSE;

                default:
                {
                    // Check if the key matches.
                    if (dwHash == (pItem->dwHash & ~SLOT_MASK))
                    {
                        *ppItem = (dwFlags & INTERNET_CACHE_FLAG_ALLOW_COLLISIONS) ?
                                    pItem : IsMatch(pItem, pszKey, dwFlags);
                        if (*ppItem)
                                return TRUE;
                    }
                }                    
                    continue;
                    
            } // end switch
          
        } // end for loop to scan seats in bucket
        
        // Follow the link to the next table.
        pdwTableOffset = &pTable->dwNext;

    } // end while (*pdwTableOffset)

    // If we've encountered a corrupt table, we'll have to recover
    if (*pdwTableOffset)
    {
        INET_ASSERT(FALSE);
        *pdwTableOffset = 0;
    }
    
    // We are out a buckets, so an item hasn't been found.

    if (fCreate && !pFree)
    {
        // Caller wanted a free item but we didn't find one.
       
        HASH_FILEMAP_ENTRY* pTable = AllocTable
            (pAllocObj, &pBase, &pdwTableOffset);

//////////////////////////////////////////////////////////////////////
// WARNING: the file might have grown and remapped, so any pointers //
// hereafter must be recalculated by offsets from the new base.     //
//////////////////////////////////////////////////////////////////////

        if (pTable)
        {
            pTable->nBlock = nBlock;
            // Calculate next free slot.
            pFree = ((HASH_ITEM*) (pTable + 1)) + iSlot * ITEMS_PER_BUCKET;
            INET_ASSERT (pFree->dwHash   == HASH_END);
            INET_ASSERT (pFree->dwOffset == HASH_END);
        }
    }

    // Return free item if desired and indicate no item found.
    if (pFree)
    {
        INET_ASSERT (fCreate);
        pFree->dwHash   = dwHash;
        pFree->dwOffset = HASH_END; // invalid in case caller neglects to set it
    }
    *ppItem = pFree;
    return FALSE;
}

//
// HashFindNextItem: scans the table for the next valid URL item
//

PUBLIC
HASH_ITEM*
HashGetNextItem
(
    IN     LPVOID       pAllocObj,      // allocator object
    IN     LPBYTE       pBase,          // base for all offsets
    IN OUT LPDWORD      pdwItemOffset,  // current item offset
    IN     DWORD        dwFlags         // include redirects?
)
{
    INET_ASSERT (!(dwFlags & ~LOOKUP_BIT_REDIR));
    
    // Check if there if the hash table is empty (or we are at the end already.)
    if (!*pdwItemOffset)
        return NULL;

    HASH_ITEM* pItem = (HASH_ITEM*) (pBase + *pdwItemOffset);

    // Calculate current table offset, assuming it's the previous page boundary.
    INET_ASSERT (BYTES_PER_TABLE <= BYTES_PER_PAGE);
    HASH_FILEMAP_ENTRY* pTable =
        (HASH_FILEMAP_ENTRY*) (((DWORD_PTR)pItem) & ~(BYTES_PER_PAGE - 1));

    // Advance item pointer to next location.
    if (pItem == (HASH_ITEM*) pTable)
        pItem = (HASH_ITEM*) (pTable + 1); // first location in table
    else
        pItem++; // next location in table

    do // Scan the list of tables.
    {
        if (pTable->dwSig != SIG_HASH)
            break;
            
        // Scan the current table.
        for (; (LPBYTE) pItem < ((LPBYTE) pTable) + BYTES_PER_TABLE; pItem++)
        {
            // No reserved bits should be set.
            INET_ASSERT (!(pItem->dwHash & HASH_BIT_RESERVED));
            
            if (!(pItem->dwHash & HASH_BIT_NOTURL)
                ||      (dwFlags /* & LOOKUP_BIT_REDIR */)
                    &&  ((pItem->dwHash & HASH_FLAG_MASK) == HASH_REDIR))
            {
                // Found a valid entry.
                *pdwItemOffset = (DWORD) ((LPBYTE)pItem - pBase);  // 64BIT
                return pItem;
            }
        }

        // Follow the link to the next table.
        if (!pTable->dwNext)
            pTable = NULL;
        else
        {
            // Validate the table signature and sequence number.
            DWORD nBlock = pTable->nBlock;
            pTable = (HASH_FILEMAP_ENTRY*) (pBase + pTable->dwNext);
            if (pTable->dwSig != SIG_HASH || pTable->nBlock != nBlock + 1)
                pTable = NULL;

            // Set pointer to first location in table.
            pItem = (HASH_ITEM*) (pTable + 1);
        }
    }
        while (pTable);

    // We reached the end of the last table.
    *pdwItemOffset = 0;
    return NULL;
}

