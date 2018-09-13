//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cache.c
//
//  This files contains code for the common cache lists
//
// History:
//  09-02-93 ScottH     Created
//  01-31-94 ScottH     Split into separate files
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers

/////////////////////////////////////////////////////  TYPEDEFS

typedef struct tagCITEM
    {
    int atomKey;        
    DEBUG_CODE( LPCTSTR pszKey; )

    LPVOID pvValue;
    UINT ucRef;
    } CITEM;        // item for generic cache


#define Cache_EnterCS(this)    EnterCriticalSection(&(this)->cs)
#define Cache_LeaveCS(this)    LeaveCriticalSection(&(this)->cs)


#define CACHE_GROW  8

#define Cache_Bogus(this)  (!(this)->hdpa || !(this)->hdpaFree || !(this)->hdsa)

// Given an index into the DPA, get the pointer to the DSA
//  
#define MyGetPtr(this, idpa)     DSA_GetItemPtr((this)->hdsa, PtrToUlong(DPA_FastGetPtr((this)->hdpa, idpa)))

#define DSA_GetPtrIndex(hdsa, ptr, cbItem)      \
                ((int)( (DWORD_PTR)(ptr) - (DWORD_PTR)DSA_GetItemPtr(hdsa, 0) ) / (cbItem))


/*----------------------------------------------------------
Purpose: Compare two CRLs by pathname
Returns: -1 if <, 0 if ==, 1 if >
Cond:    --
*/
int CALLBACK _export Cache_CompareIndexes(
    LPVOID lpv1,
    LPVOID lpv2,
    LPARAM lParam)      
    {
    int i1 = PtrToUlong(lpv1);
    int i2 = PtrToUlong(lpv2);
    HDSA hdsa = (HDSA)lParam;
    CITEM  * pitem1 = (CITEM  *)DSA_GetItemPtr(hdsa, i1);
    CITEM  * pitem2 = (CITEM  *)DSA_GetItemPtr(hdsa, i2);

    if (pitem1->atomKey < pitem2->atomKey)
        return -1;
    else if (pitem1->atomKey == pitem2->atomKey)
        return 0;
    else 
        return 1;
    }


/*----------------------------------------------------------
Purpose: Compare two CRLs by pathname
Returns: -1 if <, 0 if ==, 1 if >
Cond:    --
*/
int CALLBACK _export Cache_Compare(
    LPVOID lpv1,
    LPVOID lpv2,
    LPARAM lParam)      
    {
    // HACK: we know the first param is the address to a struct
    //  that contains the search criteria.  The second is an index 
    //  into the DSA.
    //
    int i2 = PtrToUlong(lpv2);
    HDSA hdsa = (HDSA)lParam;
    CITEM  * pitem1 = (CITEM  *)lpv1;
    CITEM  * pitem2 = (CITEM  *)DSA_GetItemPtr(hdsa, i2);

    if (pitem1->atomKey < pitem2->atomKey)
        return -1;
    else if (pitem1->atomKey == pitem2->atomKey)
        return 0;
    else 
        return 1;
    }


/*----------------------------------------------------------
Purpose: Initialize the cache structure
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Cache_Init(
    CACHE  * pcache)
    {
    BOOL bRet;

    ASSERT(pcache);

    Cache_EnterCS(pcache);
        {
        if ((pcache->hdsa = DSA_Create(sizeof(CITEM), CACHE_GROW)) != NULL)
            {
            if ((pcache->hdpa = DPA_Create(CACHE_GROW)) == NULL)
                {
                DSA_Destroy(pcache->hdsa);
                pcache->hdsa = NULL;
                }
            else
                {
                if ((pcache->hdpaFree = DPA_Create(CACHE_GROW)) == NULL)
                    {
                    DPA_Destroy(pcache->hdpa);
                    DSA_Destroy(pcache->hdsa);
                    pcache->hdpa = NULL;
                    pcache->hdsa = NULL;
                    }
                }
            }
        bRet = pcache->hdsa != NULL;
        }
    Cache_LeaveCS(pcache);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Initializes the cache's critical section.

Returns: --
Cond:    --
*/
void PUBLIC Cache_InitCS(
    CACHE  * pcache)
    {
    ASSERT(pcache);
    ZeroInit(pcache, CACHE);
    InitializeCriticalSection(&pcache->cs);
    }


/*----------------------------------------------------------
Purpose: Destroy the cache 
Returns: --
Cond:    --
*/
void PUBLIC Cache_Term(
    CACHE  * pcache,
    HWND hwndOwner,
    PFNFREEVALUE pfnFree)
    {
    ASSERT(pcache);
    ASSERT(!IsBadCodePtr((PROC)pfnFree));
    Cache_EnterCS(pcache);
        {
        if (pcache->hdpa != NULL)
            {
            CITEM  * pitem;
            int idpa;
            int cItem;
    
            ASSERT(pcache->hdsa != NULL);
    
            cItem = DPA_GetPtrCount(pcache->hdpa);
            for (idpa = 0; idpa < cItem; idpa++)
                {
                pitem = MyGetPtr(pcache, idpa);
    
                if (!IsBadCodePtr((PROC)pfnFree))
                    pfnFree(pitem->pvValue, hwndOwner);
    
                // Decrement reference count of atomKey
                Atom_Delete(pitem->atomKey);
                }
            DPA_Destroy(pcache->hdpa);
            pcache->hdpa = NULL;
            }
    
        if (pcache->hdpaFree != NULL)
            {
            DPA_Destroy(pcache->hdpaFree);
            pcache->hdpaFree = NULL;
            }
    
        if (pcache->hdsa != NULL)
            {
            DSA_Destroy(pcache->hdsa);
            pcache->hdsa = NULL;
            }
        }
    Cache_LeaveCS(pcache);
    }


/*----------------------------------------------------------
Purpose: Deletes the cache's critical section.

Returns: --
Cond:    --
*/
void PUBLIC Cache_DeleteCS(
    CACHE  * pcache)
    {
    // The cache should not be in use now (ie, it should be bogus)
    ASSERT(Cache_Bogus(pcache));

    if (Cache_Bogus(pcache))
        {
        DeleteCriticalSection(&pcache->cs);
        }
    }


/*----------------------------------------------------------
Purpose: Add an item to the cache list.  
Returns: TRUE on success

Cond:    If this fails, pvValue is not automatically freed
*/
BOOL PUBLIC Cache_AddItem(
    CACHE  * pcache,
    int atomKey,
    LPVOID pvValue)
    {
    BOOL bRet = FALSE;
    CITEM  * pitem = NULL;
    int cItem;
    int cFree;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        VALIDATE_ATOM(atomKey);
            
        if (!Cache_Bogus(pcache))
            {
            int iItem;

            // Add a new entry to the cache.  The cache has no set size limitation.
            //
            cFree = DPA_GetPtrCount(pcache->hdpaFree);
            if (cFree > 0)
                {
                // Use a free entry 
                //
                cFree--;
                iItem = PtrToUlong(DPA_DeletePtr(pcache->hdpaFree, cFree));
                pitem = DSA_GetItemPtr(pcache->hdsa, iItem);
                }
            else
                {
                CITEM itemDummy;
        
                // Allocate a new entry
                //
                cItem = DSA_GetItemCount(pcache->hdsa);
                if ((iItem = DSA_InsertItem(pcache->hdsa, cItem+1, &itemDummy)) != -1)
                    pitem = DSA_GetItemPtr(pcache->hdsa, iItem);
                }
        
            // Fill in the info
            //
            if (pitem)
                {
                pitem->ucRef = 0;
                pitem->pvValue = pvValue;
                pitem->atomKey = atomKey;
                DEBUG_CODE( pitem->pszKey = Atom_GetName(atomKey); )
            
                // Now increment the reference count on this atomKey so it doesn't
                //  get deleted from beneath us!
                Atom_AddRef(atomKey);
            
                // Add the new entry to the ptr list and sort
                //
                cItem = DPA_GetPtrCount(pcache->hdpa);
                if (DPA_InsertPtr(pcache->hdpa, cItem+1, (LPVOID)iItem) == -1)
                    goto Add_Fail;
                DPA_Sort(pcache->hdpa, Cache_CompareIndexes, (LPARAM)pcache->hdsa);
            
                // Reset the FindFirst/FindNext in case this gets called in the
                //  middle of an enumeration.
                //    
                pcache->atomPrev = ATOM_ERR;
                bRet = TRUE;
                }

Add_Fail:
            if (!bRet)
                {
                // Add the entry to the free list and fail.  If even this 
                //  fails, we simply lose some slight efficiency, but this is 
                //  not a memory leak.
                //
                DPA_InsertPtr(pcache->hdpaFree, cFree+1, (LPVOID)iItem);
                }
            }
        }
    Cache_LeaveCS(pcache);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Delete an item from the cache.
         If the reference count is 0, we do nothing.
         This also frees the actual value as well, using the
          pfnFreeValue function.

Returns: The reference count.  If 0, then we deleted it from cache.

Cond:    N.b.  Decrements the reference count.
*/
int PUBLIC Cache_DeleteItem(
    CACHE  * pcache,
    int atomKey,
    BOOL bNuke,         // TRUE to ignore reference count
    HWND hwndOwner,
    PFNFREEVALUE pfnFree)
    {
    int nRet = 0;
    CITEM item;
    CITEM  * pitem;
    int idpa;
    int cFree;

    ASSERT(pcache);
    ASSERT(!IsBadCodePtr((PROC)pfnFree));
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            item.atomKey = atomKey;
            idpa = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, (LPARAM)pcache->hdsa, 
                DPAS_SORTED);
            if (idpa != -1)
                {
                VALIDATE_ATOM(atomKey);
            
                pitem = MyGetPtr(pcache, idpa);

                if (!bNuke && pitem->ucRef-- > 0)
                    {
                    nRet = pitem->ucRef+1;
                    }
                else
                    {
                    int iItem;

                    DPA_DeletePtr(pcache->hdpa, idpa);
                
                    // Free old pointer
                    if (!IsBadCodePtr((PROC)pfnFree))
                        pfnFree(pitem->pvValue, hwndOwner);        
                
                    Atom_Delete(pitem->atomKey);
                
                    DEBUG_CODE( pitem->atomKey = -1; )
                    DEBUG_CODE( pitem->pszKey = NULL; )
                    DEBUG_CODE( pitem->pvValue = NULL; )
                    DEBUG_CODE( pitem->ucRef = 0; )

                    // Reset the FindFirst/FindNext in case this gets 
                    //  called in the middle of an enumeration.
                    //    
                    pcache->atomPrev = ATOM_ERR;
                
                    // Add ptr to the free list.  If this fails, we simply lose 
                    //  some efficiency in reusing this portion of the cache.  
                    //  This is not a memory leak.
                    //
                    cFree = DPA_GetPtrCount(pcache->hdpaFree);
                    iItem = DSA_GetPtrIndex(pcache->hdsa, pitem, sizeof(CITEM));
                    DPA_InsertPtr(pcache->hdpaFree, cFree+1, (LPVOID)iItem);
                    }
                }
            }
        }
    Cache_LeaveCS(pcache);

    return nRet;
    }


/*----------------------------------------------------------
Purpose: Replace the contents of the value in the cache list.  
          If a value does not exist for the given key, return FALSE.
Returns: TRUE if success
Cond:    --
*/
BOOL PUBLIC Cache_ReplaceItem(
    CACHE  * pcache,
    int atomKey,
    LPVOID pvBuf,
    int cbBuf)
    {
    BOOL bRet = FALSE;
    CITEM item;
    CITEM  * pitem;
    int idpa;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            // Search for an existing cache entry 
            //
            item.atomKey = atomKey;
            idpa = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, (LPARAM)pcache->hdsa, 
                DPAS_SORTED);
        
            if (idpa != -1)
                {
                // Found a value for this key.  Replace the contents.
                //
                pitem = MyGetPtr(pcache, idpa);
                ASSERT(pitem);

                BltByte(pvBuf, pitem->pvValue, cbBuf);
                bRet = TRUE;

                // No need to sort because key hasn't changed.
                }
            }
        }
    Cache_LeaveCS(pcache);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get the value of the given key and return a ptr to it
Returns: Ptr to actual entry

Cond:    Reference count is incremented
*/
LPVOID PUBLIC Cache_GetPtr(
    CACHE  * pcache,
    int atomKey)
    {
    LPVOID pvRet = NULL;
    CITEM item;
    CITEM  * pitem;
    int idpa;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            item.atomKey = atomKey;
            idpa = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, (LPARAM)pcache->hdsa, 
                DPAS_SORTED);
            if (idpa != -1)
                {
                pitem = MyGetPtr(pcache, idpa);
                ASSERT(pitem);

                pitem->ucRef++;
                pvRet = pitem->pvValue;
                }
            }
        }
    Cache_LeaveCS(pcache);

    return pvRet;
    }


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Get the current reference count
Returns: Ptr to actual entry

Cond:    Used for debugging
*/
UINT PUBLIC Cache_GetRefCount(
    CACHE  * pcache,
    int atomKey)
    {
    UINT ucRef = (UINT)-1;
    CITEM item;
    CITEM  * pitem;
    int idpa;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            item.atomKey = atomKey;
            idpa = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, (LPARAM)pcache->hdsa, 
                DPAS_SORTED);
            if (idpa != -1)
                {
                pitem = MyGetPtr(pcache, idpa);
                ASSERT(pitem);

                ucRef = pitem->ucRef;
                }
            }
        }
    Cache_LeaveCS(pcache);

    return ucRef;
    }
#endif


/*----------------------------------------------------------
Purpose: Get the value of the given key and return a copy of it
         in the supplied buffer
Returns: Copy of value in buffer
         TRUE if found, FALSE if not

Cond:    --
*/
BOOL PUBLIC Cache_GetItem(
    CACHE  * pcache,
    int atomKey,
    LPVOID pvBuf,
    int cbBuf)
    {
    BOOL bRet = FALSE;
    CITEM item;
    CITEM  * pitem;
    int idpa;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            item.atomKey = atomKey;
            idpa = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, (LPARAM)pcache->hdsa, 
                DPAS_SORTED);
            if (idpa != -1)
                {
                pitem = MyGetPtr(pcache, idpa);
                ASSERT(pitem);

                BltByte(pvBuf, pitem->pvValue, cbBuf);
                bRet = TRUE;
                }
            }
        }
    Cache_LeaveCS(pcache);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get the first key in the cache.
Returns: Atom
         ATOM_ERR if cache is empty
Cond:    --
*/
int PUBLIC Cache_FindFirstKey(
    CACHE  * pcache)
    {
    int atomRet = ATOM_ERR;
    CITEM  * pitem;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            int i;

            pcache->iPrev = 0;
            if (DPA_GetPtrCount(pcache->hdpa) > 0)
                {
                i = PtrToUlong(DPA_FastGetPtr(pcache->hdpa, 0));

                pitem = DSA_GetItemPtr(pcache->hdsa, i);

                pcache->atomPrev = pitem->atomKey;
                atomRet = pitem->atomKey;

                VALIDATE_ATOM(atomRet);
                }
            }
        }
    Cache_LeaveCS(pcache);

    return atomRet;
    }


/*----------------------------------------------------------
Purpose: Get the next key in the cache.
Returns: Atom
         ATOM_ERR if we're at the end of the cache
Cond:    --
*/
int PUBLIC Cache_FindNextKey(
    CACHE  * pcache,
    int atomPrev)
    {
    int atomRet = ATOM_ERR;
    CITEM  * pitem;

    ASSERT(pcache);
    Cache_EnterCS(pcache);
        {
        if (!Cache_Bogus(pcache))
            {
            if (atomPrev != ATOM_ERR)
                {
                int i;

                if (atomPrev != pcache->atomPrev)
                    {
                    CITEM item;
            
                    // Search for atomPrev or next one nearest to it.  
                    //
                    item.atomKey = atomPrev;
                    pcache->iPrev = DPA_Search(pcache->hdpa, &item, 0, Cache_Compare, 
                        (LPARAM)pcache->hdsa, DPAS_SORTED | DPAS_INSERTBEFORE);
                    }
                else
                    pcache->iPrev++;
            
                if (DPA_GetPtrCount(pcache->hdpa) > pcache->iPrev)
                    {
                    i = PtrToUlong(DPA_FastGetPtr(pcache->hdpa, pcache->iPrev));
                    pitem = DSA_GetItemPtr(pcache->hdsa, i);

                    pcache->atomPrev = pitem->atomKey;
                    atomRet = pitem->atomKey;

                    VALIDATE_ATOM(atomRet);
                    }
                }
            }
        }
    Cache_LeaveCS(pcache);

    return atomRet;
    }
