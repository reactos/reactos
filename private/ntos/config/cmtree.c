/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmtree.c

Abstract:

    This module contains cm routines that understand the structure
    of the registry tree.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFindNameInList)
#pragma alloc_text(PAGE,CmpGetValueListFromCache)
#pragma alloc_text(PAGE,CmpGetValueKeyFromCache)
#pragma alloc_text(PAGE,CmpFindValueByNameFromCache)
#endif


HCELL_INDEX
CmpFindNameInList(
    IN PHHIVE  Hive,
    IN PCHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    IN OPTIONAL PCELL_DATA *ChildAddress,
    IN OPTIONAL PULONG ChildIndex
    )
/*++

Routine Description:

    Find a child object in an object list.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    List - pointer to mapped in list structure

    Count - number of elements in list structure

    Name - name of child object to find

    ChildAddress - pointer to variable to receive address of mapped in child

    ChildIndex - pointer to variable to receive index for child

Return Value:

    HCELL_INDEX for the found cell
    HCELL_NIL if not found

--*/
{
    NTSTATUS    status;
    ULONG   i;
    PCM_KEY_VALUE pchild;
    UNICODE_STRING Candidate;
    BOOLEAN Success;
    PCELL_DATA List;

    if (ChildList->Count != 0) {
        List = (PCELL_DATA)HvGetCell(Hive,ChildList->List);
        for (i = 0; i < ChildList->Count; i++) {

            pchild = (PCM_KEY_VALUE)HvGetCell(Hive, List->u.KeyList[i]);

            if (pchild->Flags & VALUE_COMP_NAME) {
                Success = (CmpCompareCompressedName(Name,
                                                    pchild->Name,
                                                    pchild->NameLength)==0);
            } else {
                Candidate.Length = pchild->NameLength;
                Candidate.MaximumLength = Candidate.Length;
                Candidate.Buffer = pchild->Name;
                Success = (RtlCompareUnicodeString(Name,
                                                   &Candidate,
                                                   TRUE)==0);
            }

            if (Success) {
                //
                // Success, return data to caller and exit
                //

                if (ARGUMENT_PRESENT(ChildIndex)) {
                    *ChildIndex = i;
                }
                if (ARGUMENT_PRESENT(ChildAddress)) {
                    *ChildAddress = (PCELL_DATA)pchild;
                }
                return(List->u.KeyList[i]);
            }
        }
    }

    return HCELL_NIL;
}

#ifndef _CM_LDR_

PCELL_DATA
CmpGetValueListFromCache(
    IN PHHIVE  Hive,
    IN PCACHED_CHILD_LIST ChildList,
    OUT BOOLEAN    *IndexCached
)
/*++

Routine Description:

    Get the Valve Index Array.  Check if it is already cached, if not, cache it and return
    the cached entry.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ChildList - pointer/index to the Value Index array

    IndexCached - Indicating whether Value Index list is cached or not.

Return Value:

    Pointer to the Valve Index Array.

--*/
{
    PCELL_DATA List;
    ULONG AllocSize;
    PCM_CACHED_VALUE_INDEX CachedValueIndex;
    ULONG i;

#ifndef _WIN64
    *IndexCached = TRUE;
    if (CMP_IS_CELL_CACHED(ChildList->ValueList)) {
        //
        // The entry is already cached.
        //
        List = CMP_GET_CACHED_CELLDATA(ChildList->ValueList);
    } else {
        //
        // The entry is not cached.  The element contains the hive index.
        //
        List = (PCELL_DATA) HvGetCell(Hive, CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList));
        //
        // Allocate a PagedPool to cache the value index cell.
        //

        AllocSize = ChildList->Count * sizeof(ULONG_PTR) + FIELD_OFFSET(CM_CACHED_VALUE_INDEX, Data);
        // Dragos: Changed to catch the memory violator
        // it didn't work
        //CachedValueIndex = (PCM_CACHED_VALUE_INDEX) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_INDEX_TAG,NormalPoolPrioritySpecialPoolUnderrun);
        CachedValueIndex = (PCM_CACHED_VALUE_INDEX) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_INDEX_TAG);

        if (CachedValueIndex) {

            CachedValueIndex->CellIndex = CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList);
            for (i=0; i<ChildList->Count; i++) {
                CachedValueIndex->Data.List[i] = (ULONG_PTR) List->u.KeyList[i];
            }

            ChildList->ValueList = CMP_MARK_CELL_CACHED(CachedValueIndex);

            // Trying to catch the BAD guy who writes over our pool.
            CmpMakeSpecialPoolReadOnly( CachedValueIndex );

            //
            // Now we have the stuff cached, use the cache data.
            //
            List = CMP_GET_CACHED_CELLDATA(ChildList->ValueList);
        } else {
            //
            // If the allocation fails, just do not cache it. continue.
            //
            *IndexCached = FALSE; 
        }
    }
#else
    List = (PCELL_DATA) HvGetCell(Hive, CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList));
    *IndexCached = FALSE;
#endif

    return (List);
}

PCM_KEY_VALUE
CmpGetValueKeyFromCache(
    IN PHHIVE         Hive,
    IN PCELL_DATA     List,
    IN ULONG          Index,
    OUT PPCM_CACHED_VALUE *ContainingList,
    IN BOOLEAN        IndexCached,
    OUT BOOLEAN    *ValueCached
)
/*++

Routine Description:

    Get the Valve Node.  Check if it is already cached, if not but the index is cached, 
    cache and return the value node.

Arguments:

    Hive - pointer to hive control structure for hive of interest.

    List - pointer to the Value Index Array (of ULOONG_PTR if cached and ULONG if non-cached)

    Index - Index in the Value index array

    ContainlingList - The address of the entry that will receive the found cached value.

    IndexCached - Indicate if the index list is cached.  If not, everything is from the
                  original registry data.

    ValueCached - Indicating whether Value is cached or not.

Return Value:

    Pointer to the Valve Node.
--*/
{
    PCM_KEY_VALUE pchild;
    PULONG_PTR    CachedList;
    ULONG AllocSize;
    ULONG CopySize;
    PCM_CACHED_VALUE CachedValue;

    if (IndexCached) {
        //
        // The index array is cached, so List is pointing to an array of ULONG_PTR.
        // Use CachedList.
        //
        CachedList = (PULONG_PTR) List;
        *ValueCached = TRUE;
        if (CMP_IS_CELL_CACHED(CachedList[Index])) {
            pchild = CMP_GET_CACHED_KEYVALUE(CachedList[Index]);
            *ContainingList = &((PCM_CACHED_VALUE) CachedList[Index]);
        } else {
            pchild = (PCM_KEY_VALUE) HvGetCell(Hive, List->u.KeyList[Index]);
            //
            // Allocate a PagedPool to cache the value node.
            //
            CopySize = (ULONG) HvGetCellSize(Hive, pchild);
            AllocSize = CopySize + FIELD_OFFSET(CM_CACHED_VALUE, KeyValue);
            
            // Dragos: Changed to catch the memory violator
            // it didn't work
            //CachedValue = (PCM_CACHED_VALUE) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_TAG,NormalPoolPrioritySpecialPoolUnderrun);
            CachedValue = (PCM_CACHED_VALUE) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_TAG);

            if (CachedValue) {
                //
                // Set the information for later use if we need to cache data as well.
                //
                CachedValue->DataCacheType = CM_CACHE_DATA_NOT_CACHED;
                CachedValue->ValueKeySize = (USHORT) CopySize;

                RtlCopyMemory((PVOID)&(CachedValue->KeyValue), pchild, CopySize);


                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(CachedList) );

                CachedList[Index] = CMP_MARK_CELL_CACHED(CachedValue);

                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadOnly( CMP_GET_CACHED_ADDRESS(CachedList) );


                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadOnly(CachedValue);

                *ContainingList = &((PCM_CACHED_VALUE) CachedList[Index]);
                //
                // Now we have the stuff cached, use the cache data.
                //
                pchild = CMP_GET_CACHED_KEYVALUE(CachedValue);
            } else {
                //
                // If the allocation fails, just do not cache it. continue.
                //
                *ValueCached = FALSE;
            }
        }
    } else {
        //
        // The Valve Index Array is from the registry hive, just get the cell and move on.
        //
        pchild = (PCM_KEY_VALUE) HvGetCell(Hive, List->u.KeyList[Index]);
        *ValueCached = FALSE;
    }
    return (pchild);
}

PCM_KEY_VALUE
CmpFindValueByNameFromCache(
    IN PHHIVE  Hive,
    IN PCACHED_CHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    OUT PPCM_CACHED_VALUE *ContainingList,
    OUT ULONG *Index,
    OUT BOOLEAN *ValueCached
    )
/*++

Routine Description:

    Find a value node given a value list array and a value name.  It sequentially walk
    through each value node to look for a match.  If the array and 
    value nodes touched are not already cached, cache them.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ChildList - pointer/index to the Value Index array

    Name - name of value to find

    ContainlingList - The address of the entry that will receive the found cached value.

    Index - pointer to variable to receive index for child
    
    ValueCached - Indicate if the value node is cached.

Return Value:

    HCELL_INDEX for the found cell
    HCELL_NIL if not found

--*/
{
    NTSTATUS    status;
    ULONG   i;
    PCM_KEY_VALUE pchild;
    UNICODE_STRING Candidate;
    BOOLEAN Success;
    PCELL_DATA List;
    BOOLEAN IndexCached;

    if (ChildList->Count != 0) {
        List = CmpGetValueListFromCache(Hive, ChildList, &IndexCached);

        for (i = 0; i < ChildList->Count; i++) {
            pchild = CmpGetValueKeyFromCache(Hive, List, i, ContainingList, IndexCached, ValueCached);

            if (pchild->Flags & VALUE_COMP_NAME) {
                Success = (CmpCompareCompressedName(Name,
                                                    pchild->Name,
                                                    pchild->NameLength)==0);
            } else {
                Candidate.Length = pchild->NameLength;
                Candidate.MaximumLength = Candidate.Length;
                Candidate.Buffer = pchild->Name;
                Success = (RtlCompareUnicodeString(Name,
                                                   &Candidate,
                                                   TRUE)==0);
            }

            if (Success) {
                //
                // Success, fill the index, return data to caller and exit
                //
                *Index = i;
                return(pchild);
            }
        }
    }

    return NULL;
}

#endif
