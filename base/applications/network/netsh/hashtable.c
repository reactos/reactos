/*
 * PROJECT:    Hash Table
 * LICENSE:    MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:    Hash Table main file
 * COPYRIGHT:  Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */

//#define HASH_TABLE_UNIT_TEST
#define NDEBUG
#include <debug.h>

#ifdef HASH_TABLE_UNIT_TEST
#define STATIC
#else
#define STATIC static
#include "hashtable.h"
#endif

typedef struct _INTERNAL_HASH_TABLE
{
    size_t nNumberOfEntries;
    size_t nNumberOfBuckets;
    size_t nNumberOfBucketsUsed;
    PHASH_ENTRY* Buckets;
    DOUBLE dLoadFactor;
}INTERNAL_HASH_TABLE, *PINTERNAL_HASH_TABLE;

#ifndef HASH_TABLE_UNIT_TEST
typedef struct _HASH_TABLE_ITERATOR
{
    PHASH_TABLE pHashTable;
    PHASH_ENTRY pHashEntry;
    SIZE_T nIndex;
} HASH_TABLE_ITERATOR, *PHASH_TABLE_ITERATOR;
#endif

STATIC
PHASH_ENTRY
_FindEntry(
    struct _HASH_TABLE* self,
    PVOID pKey,
    SIZE_T nKeyLength);

STATIC
BOOL
_HashTableFreeEntry(
    PHASH_ENTRY pHashEntry,
    BOOL bFreeKey,
    BOOL bFreeValue);
// *********************************************************************

STATIC
BOOL 
IsPrime(SIZE_T n)
{
    // 0 or 1 are not prime
    if (n <= 1)
    {
        return FALSE;
    }

    // 2 and 3 are prime
    if (n <= 3)
    {
        return TRUE;
    }

    // Eliminate multiples of 2 and 3
    if ((n % 2 == 0) || (n % 3 == 0))
    {
        return FALSE; 
    }

    // Only check up to the square root of n
    SIZE_T limit = (SIZE_T)sqrt(n); 

    // Check for factors of the form 6k ± 1
    for (SIZE_T i = 5; i <= limit; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
        {
            return FALSE; // Check both 6k - 1 and 6k + 1 forms
        }
    }

    return TRUE;
}


// Find the closest prime number
STATIC
SIZE_T 
FindClosestPrime(
    SIZE_T number)
{
    // If we are at the maximum value, return it directly
    if (number == SIZE_MAX)
    {
        return SIZE_MAX; 
    }

    SIZE_T candidate = number; // Start from the next number
    while (TRUE) {
        if (IsPrime(candidate))
        {
            return candidate; // Return the first prime found
        }

        // Prevent overflow
        if (candidate == SIZE_MAX)
            break;
        
        candidate++;
    }

    return SIZE_MAX; 
}


STATIC
DWORD 
_Fnv1aHash32(
    PVOID data, 
    SIZE_T data_length)
{
    if (data == NULL || data_length == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    DWORD hash = 0x811c9dc5;
    DWORD FNV_prime = 0x01000193;

    unsigned char* c_data = (unsigned char*)data;
    
    for (size_t index = 0; index < data_length; index++)
    {
        hash ^= c_data[index];
        hash *= FNV_prime;
    }

    return hash;
}

/**
 * @brief Deletes an entry.
 *
 * This function deletes an entry in @p self by searching for a key @p pKey.
 * The code does not free any memory allocated in the Bucket array, but it
 * will free either the key or value as specified by @p bFreeKey and @p FreeValue.
 * If the entry is a node in a linked list, the memory that holds the node will be
 * freed.
 *
 * @param[in] self
 * Contains a pointer to the current HASH_tABLE structure.
 *
 * @param[in] pKey
 * The key to hash and use as an entry point into the hash table.
 *
 * @param[in] nKeyLength
 * The length of pKey.
 *
 * @param[in] bFreeKey
 * If TRUE, _DeleteEntry will attempt to free the memory used by pKey in the HASH_ENTRY
 *
 * @param[in] bFreeValue
 * If TRUE, _DeleteEntry will attempt to free the memory used by pValue in the HASH_ENTRY
 * 
 * @return
 * TRUE on success and FALSE on failure. Check GetLastError for more detailed information.
 * GetLastError returns the following:
 * STATUS_SUCCESS in case of success.
 * ERROR_INVALID_PARAMETER if the input arguments are invalid.
 * ERROR_INVALID_HANDLE if pInternal is NULL.
 * ERROR_NO_MATCH if the key cannot be found.
 * 
 **/
STATIC
BOOL
_DeleteEntry(
    _In_ struct _HASH_TABLE* self, 
    _In_ PVOID pKey,
    _In_ SIZE_T nKeyLength,
    _In_ BOOL bFreeKey,
    _In_ BOOL bFreeValue)
{
    DWORD dwHash = 0;
    SIZE_T nIndex = 0;
    PHASH_ENTRY pHashEntry = NULL;
    PHASH_ENTRY pParentHashEntry = NULL;
    BOOL bIsNode = FALSE;

    if ((self == NULL)
        || (pKey == NULL)
        || (nKeyLength == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = self->pInternal;

    if (self->pInternal == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    dwHash = _Fnv1aHash32(pKey, nKeyLength);
    nIndex = dwHash % pInternalHashTable->nNumberOfBuckets;

    pHashEntry = pInternalHashTable->Buckets[nIndex];

    if (pHashEntry != NULL && pHashEntry->pNext == NULL)
        pInternalHashTable->nNumberOfBucketsUsed--;

    if (pHashEntry != NULL)
    {
        // follow the linked list until we find a key
        while (pHashEntry)
        {
            // if the key lengths match we can compare
            // the key to see if they match
            if (pHashEntry->nKeyLength == nKeyLength)
            {
                if (memcmp(pHashEntry->pKey, pKey, nKeyLength) == 0)
                {
                    // Remove the link in the chain
                    // if the node has a parrent
                    if (pParentHashEntry != NULL)
                    {
                        pParentHashEntry->pNext = pHashEntry->pNext;
                        bIsNode = TRUE;
                    }
                    // we are at the head of the linked list found inside
                    // of this bucket
                    else
                    {
                        pInternalHashTable->Buckets[nIndex] = pHashEntry->pNext;
                    }
                    break;
                }
            }
            pParentHashEntry = pHashEntry;
            pHashEntry = pHashEntry->pNext;
        }
    }

    
    // if the entry is not null then we have found the correct key
    if (pHashEntry)
    {
        BOOL bFreeSuccessful = TRUE;

        if (bFreeKey == TRUE)
        {
            if (HeapFree(GetProcessHeap(), 0, pHashEntry->pKey) == 0)
                bFreeSuccessful = FALSE;
        }

        if (bFreeValue == TRUE)
        {
            if (HeapFree(GetProcessHeap(), 0, pHashEntry->pValue) == 0)
                bFreeSuccessful = FALSE;
        }

        pHashEntry->bIsFull = FALSE;
        pHashEntry->nKeyLength = 0;
        pInternalHashTable->nNumberOfEntries--;
        
        if (bIsNode == TRUE)
        {
            if (HeapFree(GetProcessHeap(), 0, pHashEntry) == 0)
                bFreeSuccessful = FALSE;
        }

        // Check if any if the HeapFree calls failed
        if (bFreeSuccessful == FALSE)
        {
            return FALSE;
        }
         
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }
    
    // key does not exist
    SetLastError(ERROR_NO_MATCH);
    return FALSE;
}


STATIC
BOOL
_HashTableGetEntryCleanup(
    PHASH_TABLE_ITERATOR *sIterator)
{
    HeapFree(GetProcessHeap(), 0, *sIterator);
    *sIterator = NULL;
    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}


STATIC
BOOL
_HashTableGetNextEntry(
    PHASH_TABLE_ITERATOR *sIterator,
    PHASH_ENTRY *pHashEntry)
{
    if (sIterator == NULL || *sIterator == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = (*sIterator)->pHashTable->pInternal;
    // check if we have a linked list to work with
    if ((*sIterator)->pHashEntry->pNext == NULL)
    {
        // iterate the entries until we find the next slot
        for (size_t index = (*sIterator)->nIndex + 1; index < pInternalHashTable->nNumberOfBuckets; index++)
        {
            // make sure the entry exists first
            if ((pInternalHashTable->Buckets[index] != NULL )
                && (pInternalHashTable->Buckets[index]->bIsFull == TRUE))
            {
                // update the find entry structure
                (*sIterator)->nIndex = index;
                (*sIterator)->pHashEntry = pInternalHashTable->Buckets[index];
                // somthing bad has happened if this is true
                if ((*sIterator)->pHashEntry == NULL)
                {
                    *pHashEntry = NULL;
                    return _HashTableGetEntryCleanup(sIterator);
                }
                // we found what we came here for
                *pHashEntry = (*sIterator)->pHashEntry;
                SetLastError(ERROR_SUCCESS);
                return TRUE;
            }

        }
        // there are no more entries to process so we can cleanup
        *pHashEntry = NULL;
        return _HashTableGetEntryCleanup(sIterator);
    }
    // we are in a linked list
    else
    {
        (*sIterator)->pHashEntry = (*sIterator)->pHashEntry->pNext;
        *pHashEntry = (*sIterator)->pHashEntry;
    }

    // if we made it here all is well
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


STATIC
PHASH_TABLE_ITERATOR
_HashTableGetFirstEntry(
    PHASH_TABLE pHashTable,
    PHASH_ENTRY *pHashEntry)
{
    if (pHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (pHashTable->pInternal == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = pHashTable->pInternal;
    PHASH_TABLE_ITERATOR sIterator = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HASH_TABLE_ITERATOR));
    
    if (sIterator == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    sIterator->pHashTable = pHashTable;
    // find a bucket that is being used
    for (size_t index = 0; index < pInternalHashTable->nNumberOfBuckets; index++)
    {
        // make sure are bucket exists and is not empty
        if (pInternalHashTable->Buckets[index] != NULL 
            && pInternalHashTable->Buckets[index]->bIsFull == TRUE)
        {
            // update the iterator to contain the newest entry
            sIterator->nIndex = index;
            sIterator->pHashEntry = pInternalHashTable->Buckets[index];
            break;
        }

    }
    
    // set pHashEntry
    *pHashEntry = sIterator->pHashEntry;

    if (sIterator->pHashEntry == NULL)
    {
        _HashTableGetEntryCleanup(&sIterator);
        return NULL;
    }

    SetLastError(ERROR_SUCCESS);
    return sIterator;
}


STATIC
BOOL
_ReHashTableEntries(
    PHASH_TABLE pCurrentHashTable, 
    PHASH_TABLE pNewHashTable)
{
    if (pCurrentHashTable == NULL || pNewHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PHASH_ENTRY pHashEntry = NULL;
    PHASH_TABLE_ITERATOR sIterator = _HashTableGetFirstEntry(pCurrentHashTable, &pHashEntry);
    
    while (pHashEntry)
    {
        pNewHashTable->SetEntry(&pNewHashTable, 
            pHashEntry->pKey, 
            pHashEntry->nKeyLength,
            pHashEntry->pValue);

        _HashTableGetNextEntry(&sIterator, &pHashEntry);
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


STATIC
BOOL
_HashTableExpand(PHASH_TABLE *pHashTable)
{
    if (pHashTable == NULL || *pHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PHASH_TABLE pTempHashTable = NULL;
    PHASH_TABLE pNewHashTable = NULL;
    PINTERNAL_HASH_TABLE pInternalHashTable = (*pHashTable)->pInternal;

    if (pInternalHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    size_t nNewSize = FindClosestPrime((SIZE_T)(pInternalHashTable->nNumberOfEntries 
                                             / pInternalHashTable->dLoadFactor));
    pNewHashTable = CreateHashTable(nNewSize, pInternalHashTable->dLoadFactor);

    // see if the allocation was successful
    if (pNewHashTable == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    // rehash all of the keys found in pHashTable and store them in pNewHashTable
    _ReHashTableEntries((*pHashTable), pNewHashTable);
   
    pTempHashTable = *pHashTable;
    *pHashTable = pNewHashTable;
    // free the old hash table
    FreeHashTable(&pTempHashTable, FALSE, FALSE);
    SetLastError(ERROR_SUCCESS);
    return TRUE;

}


STATIC
SIZE_T _GetNumberOfEntries(
    struct _HASH_TABLE* self)
{
    if (self == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (self->pInternal == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
  
    SetLastError(ERROR_SUCCESS);
    return ((PINTERNAL_HASH_TABLE)self->pInternal)->nNumberOfEntries;
}


STATIC
PHASH_ENTRY
_CreateHashEntry(PVOID pKey, SIZE_T nKeyLength, PVOID pValue)
{
    if (pKey == NULL 
        || nKeyLength == 0
        || pValue == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    PHASH_ENTRY pHashEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HASH_ENTRY));
    if (pHashEntry == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    pHashEntry->bIsFull = TRUE;
    pHashEntry->pValue = pValue;
    pHashEntry->pKey = pKey;
    pHashEntry->nKeyLength = nKeyLength;

    SetLastError(ERROR_SUCCESS);
    return pHashEntry;
}


STATIC 
BOOL
_SetEntry(
    struct _HASH_TABLE** self,
    PVOID pKey,
    SIZE_T nKeyLength,
    PVOID pValue)
{
    DWORD dwHash = 0;
    SIZE_T nIndex = 0;

    if (self == NULL
        || *self == NULL
        || (nKeyLength == 0)
        || (pValue == NULL)
        || (pKey == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    PINTERNAL_HASH_TABLE pInternalHashTable = (*self)->pInternal;

    if (pInternalHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pInternalHashTable->nNumberOfBuckets == 0)
        return FALSE; 
  

   dwHash = _Fnv1aHash32(pKey, nKeyLength);
   nIndex = dwHash % pInternalHashTable->nNumberOfBuckets;

    //printf("dwHash: 0x%x nIndex: %zu  pKey: %ls\n", dwHash, nIndex, (wchar_t *)pKey);
    if (pInternalHashTable->Buckets[nIndex] != NULL)
    {
        // if the bucket is empty fill it
        if (pInternalHashTable->Buckets[nIndex]->bIsFull == FALSE)
        {
            pInternalHashTable->Buckets[nIndex]->pValue = pValue;
            pInternalHashTable->Buckets[nIndex]->bIsFull = TRUE;
            pInternalHashTable->Buckets[nIndex]->pKey = pKey;
            pInternalHashTable->Buckets[nIndex]->nKeyLength = nKeyLength;
            pInternalHashTable->nNumberOfBucketsUsed++;
        }
        else
        {
            // see if the key already exists and update the value if it does
            HASH_ENTRY* pHashEntry = pInternalHashTable->Buckets[nIndex];
            while (pHashEntry)
            {
                if (pInternalHashTable->Buckets[nIndex]->nKeyLength == nKeyLength)
                {
                    if (memcmp(pInternalHashTable->Buckets[nIndex]->pKey, pKey, nKeyLength) == 0)
                    {
                        pInternalHashTable->Buckets[nIndex]->pValue = pValue;
                        SetLastError(ERROR_SUCCESS);
                        return TRUE;
                    }
                }

                pHashEntry = pHashEntry->pNext;
            }
            
            // create a new node in the linked list
            PHASH_ENTRY pNewEntry = _CreateHashEntry(pKey, nKeyLength, pValue);
            if (pNewEntry == NULL)
                return FALSE;

            pNewEntry->pNext = pInternalHashTable->Buckets[nIndex];
            pInternalHashTable->Buckets[nIndex] = pNewEntry;
        }
    }
    // memory for the bucket has not been allocated yet
    else
    {
        PHASH_ENTRY pHashEntry = _CreateHashEntry(pKey, nKeyLength, pValue);
        if (pHashEntry == NULL)
            return FALSE;

        pInternalHashTable->Buckets[nIndex] = pHashEntry;
        pInternalHashTable->nNumberOfBucketsUsed++;
    }

    pInternalHashTable->nNumberOfEntries++;

    // see if we need to resize the hash table
    if ((pInternalHashTable->nNumberOfEntries / pInternalHashTable->dLoadFactor) > pInternalHashTable->nNumberOfBuckets)
    {
        _HashTableExpand(self);
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


STATIC
PVOID
_GetValue(
    struct _HASH_TABLE* self,
    PVOID pKey,
    SIZE_T nKeyLength)
{
    // _FindEntry sets the last error
    PHASH_ENTRY pHashEntry = _FindEntry(self, pKey, nKeyLength);

    if (pHashEntry != NULL)
    {
        return pHashEntry->pValue;
    }

    return NULL;
}


STATIC
PHASH_ENTRY
_FindEntry(
    struct _HASH_TABLE* self,
    PVOID pKey,
    SIZE_T nKeyLength)
{
    DWORD dwHash = 0;
    SIZE_T nIndex = 0;
    
    if (self == NULL
        || nKeyLength == 0
        || pKey == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = self->pInternal;

    if (pInternalHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    if (pInternalHashTable->nNumberOfBuckets == 0)
    {
        SetLastError(ERROR_NO_MATCH);
        return NULL;
    }

    dwHash = _Fnv1aHash32(pKey, nKeyLength);
    //printf("HASH: 0x%x\n", dwHash);
    nIndex = dwHash % pInternalHashTable->nNumberOfBuckets;
    
    if (pInternalHashTable->Buckets[nIndex] == NULL)
    {
        SetLastError(ERROR_NO_MATCH);
        return NULL;
    }

    // if there is nothing in the slot then nothing has been assigned
    if (pInternalHashTable->Buckets[nIndex]->bIsFull == FALSE)
    {
        SetLastError(ERROR_NO_MATCH);
        return NULL;
    }

    PHASH_ENTRY pHashEntry = pInternalHashTable->Buckets[nIndex];
    // check every slot in the bucket
    while (pHashEntry)
    {
        // if Key Lengths are different then the keys cant match
        if (pHashEntry->nKeyLength == nKeyLength)
        {
            // check if keys match
            if (memcmp(pHashEntry->pKey, pKey, nKeyLength) == 0)
            {
                SetLastError(ERROR_SUCCESS);
                return pHashEntry;
            }
        }
        pHashEntry = pHashEntry->pNext;
    }

    SetLastError(ERROR_NO_MATCH);
    return NULL;
}


/*
* Frees an entry in the hashtable
*/
STATIC
BOOL
_HashTableFreeEntry(
    PHASH_ENTRY pHashEntry, 
    BOOL bFreeKey, 
    BOOL bFreeValue)
{
    if (pHashEntry == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (bFreeKey == TRUE)
    {
        HeapFree(GetProcessHeap(), 0, pHashEntry->pKey);
        pHashEntry->pKey = NULL;
    }

    if (bFreeValue == TRUE)
    {
        HeapFree(GetProcessHeap(), 0, pHashEntry->pValue);
        pHashEntry->pValue = NULL;
    }

    HeapFree(GetProcessHeap(), 0, pHashEntry);

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


STATIC
BOOL
_HashTableFreeList(
    PHASH_ENTRY pHashEntry, 
    BOOL bFreeKey, 
    BOOL bFreeValue)
{
    PHASH_ENTRY pTempHashEntry = NULL;
    if (pHashEntry == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // free every node in the list
    while (pHashEntry)
    {
        pTempHashEntry = pHashEntry;
        pHashEntry = pHashEntry->pNext;

        _HashTableFreeEntry(pTempHashEntry, bFreeKey, bFreeValue);
        pTempHashEntry = 0;
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


BOOL
FreeHashTable(
    PHASH_TABLE *pHashTable, 
    BOOL bFreeKey, 
    BOOL bFreeValue)
{
    if (pHashTable == NULL || (*pHashTable) == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*pHashTable)->pInternal == NULL)
    {
        return FALSE;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = (*pHashTable)->pInternal;
    
    if (pInternalHashTable == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // iterate every bucket
    for (size_t index = 0; index < pInternalHashTable->nNumberOfBuckets; index++)
    {
        // make sure we have somthing to work with
        if ((pInternalHashTable->Buckets != NULL) && (pInternalHashTable->Buckets[index] != NULL))
        {
            _HashTableFreeList(pInternalHashTable->Buckets[index], bFreeKey, bFreeValue);
        }
    }

    HeapFree(GetProcessHeap(), 0, (*pHashTable)->pInternal);
    HeapFree(GetProcessHeap(), 0, (*pHashTable));
    (*pHashTable) = NULL;

    SetLastError(ERROR_SUCCESS);
    return TRUE;

}


PHASH_TABLE
CreateHashTable(
    SIZE_T nNumberOfInitalBuckets, 
    DOUBLE dLoadFactor)
{
    // do not use a load factor under 1%
    if (dLoadFactor < .01)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    // Allocate memory for the hash table
    PHASH_TABLE pHashTable = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HASH_TABLE));
    if (pHashTable == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    if (IsPrime(nNumberOfInitalBuckets) != TRUE)
    {
        nNumberOfInitalBuckets = FindClosestPrime(nNumberOfInitalBuckets);
    }

    // Allocate memory for the buckets
    PHASH_ENTRY *pBuckets = HeapAlloc(GetProcessHeap(), 
                                      HEAP_ZERO_MEMORY,
                                      sizeof(HASH_ENTRY *) * nNumberOfInitalBuckets);

    if (pBuckets == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        HeapFree(GetProcessHeap(), 0, pHashTable);
        return NULL;
    }

    PINTERNAL_HASH_TABLE pInternalHashTable = HeapAlloc(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              sizeof(INTERNAL_HASH_TABLE));

    if (pInternalHashTable == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        HeapFree(GetProcessHeap(), 0, pBuckets);
        HeapFree(GetProcessHeap(), 0, pHashTable);
        return NULL;
    }
    
    pInternalHashTable->dLoadFactor = dLoadFactor;
    pInternalHashTable->nNumberOfBuckets = nNumberOfInitalBuckets;
    pInternalHashTable->Buckets = pBuckets;
    pHashTable->pInternal = pInternalHashTable;
    pHashTable->DeleteEntry = _DeleteEntry;
    pHashTable->GetValue = _GetValue;
    pHashTable->SetEntry = _SetEntry;
    pHashTable->GetNumberOfEntries = _GetNumberOfEntries;
    pHashTable->GetFirstEntry = (HANDLE)_HashTableGetFirstEntry;
    pHashTable->GetNextEntry = (BOOL (*)(HANDLE , PHASH_ENTRY *))_HashTableGetNextEntry;

    SetLastError(ERROR_SUCCESS);
    return pHashTable;
}
