/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpDoFlushAll(IN BOOLEAN ForceFlush)
{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PCMHIVE Hive;
    BOOLEAN Result = TRUE;    

    /* Make sure that the registry isn't read-only now */
    if (CmpNoWrite) return TRUE;
    
    /* Otherwise, acquire the hive list lock and disable force flush */
    CmpForceForceFlush = FALSE;
    ExAcquirePushLockShared(&CmpHiveListHeadLock);
    
    /* Loop the hive list */
    NextEntry = CmpHiveListHead.Flink;
    while (NextEntry != &CmpHiveListHead)
    {
        /* Get the hive */
        Hive = CONTAINING_RECORD(NextEntry, CMHIVE, HiveList);
        if (!(Hive->Hive.HiveFlags & HIVE_NOLAZYFLUSH))
        {
            /* Do the sync */
            Status = HvSyncHive(&Hive->Hive);
            if (!NT_SUCCESS(Status)) Result = FALSE;
        }

        /* Try the next entry */
        NextEntry = NextEntry->Flink;
    }
    
    /* Release lock and return */
    ExReleasePushLock(&CmpHiveListHeadLock);
    return Result;
}

NTSTATUS
NTAPI
CmpSetValueKeyNew(IN PHHIVE Hive,
                  IN PCM_KEY_NODE Parent,
                  IN PUNICODE_STRING ValueName,
                  IN ULONG Index,
                  IN ULONG Type,
                  IN PVOID Data,
                  IN ULONG DataSize,
                  IN ULONG StorageType,
                  IN ULONG SmallData)
{
    PCELL_DATA CellData;
    HCELL_INDEX ValueCell;
    NTSTATUS Status;

    /* Check if we already have a value list */
    if (Parent->ValueList.Count)
    {
        /* Then make sure it's valid and dirty it */
        ASSERT(Parent->ValueList.List != HCELL_NIL);
        HvMarkCellDirty(Hive, Parent->ValueList.List, FALSE);
    }

    /* Allocate  avalue cell */
    ValueCell = HvAllocateCell(Hive,
                               FIELD_OFFSET(CM_KEY_VALUE, Name) +
                               CmpNameSize(Hive, ValueName),
                               StorageType,
                               HCELL_NIL);
    if (ValueCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the actual data for it */
    CellData = HvGetCell(Hive, ValueCell);
    if (!CellData) ASSERT(FALSE);

    /* Now we can release it, make sure it's also dirty */
    HvReleaseCell(Hive, ValueCell);
    ASSERT(HvIsCellDirty(Hive, ValueCell));

    /* Set it up and copy the name */
    CellData->u.KeyValue.Signature = CM_KEY_VALUE_SIGNATURE;
    CellData->u.KeyValue.Flags = 0;
    CellData->u.KeyValue.Type = Type;
    CellData->u.KeyValue.NameLength = CmpCopyName(Hive,
                                                  CellData->u.KeyValue.Name,
                                                  ValueName);
    if (CellData->u.KeyValue.NameLength < ValueName->Length)
    {
        /* This is a compressed name */
        CellData->u.KeyValue.Flags = VALUE_COMP_NAME;
    }

    /* Check if this is a normal key */
    if (DataSize > CM_KEY_VALUE_SMALL)
    {
        /* Build a data cell for it */
        Status = CmpSetValueDataNew(Hive,
                                    Data,
                                    DataSize,
                                    StorageType,
                                    ValueCell,
                                    &CellData->u.KeyValue.Data);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, free the cell */
            HvFreeCell(Hive, ValueCell);
            return Status;
        }

        /* Otherwise, set the data length, and make sure the data is dirty */
        CellData->u.KeyValue.DataLength = DataSize;
        ASSERT(HvIsCellDirty(Hive, CellData->u.KeyValue.Data));
    }
    else
    {
        /* This is a small key, set the data directly inside */
        CellData->u.KeyValue.DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        CellData->u.KeyValue.Data = SmallData;
    }

    /* Add this value cell to the child list */
    Status = CmpAddValueToList(Hive,
                               ValueCell,
                               Index,
                               StorageType,
                               &Parent->ValueList);

    /* If we failed, free the entire cell, including the data */
    if (!NT_SUCCESS(Status)) CmpFreeValue(Hive, ValueCell);

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
CmpSetValueKeyExisting(IN PHHIVE Hive,
                       IN HCELL_INDEX OldChild,
                       IN PCM_KEY_VALUE Value,
                       IN ULONG Type,
                       IN PVOID Data,
                       IN ULONG DataSize,
                       IN ULONG StorageType,
                       IN ULONG TempData)
{
    HCELL_INDEX DataCell, NewCell;
    PCELL_DATA CellData;
    ULONG Length;
    BOOLEAN WasSmall, IsSmall;

    /* Mark the old child cell dirty */
    HvMarkCellDirty(Hive, OldChild, FALSE);

    /* See if this is a small or normal key */
    WasSmall = CmpIsKeyValueSmall(&Length, Value->DataLength);

    /* See if our new data can fit in a small key */
    IsSmall = (DataSize <= CM_KEY_VALUE_SMALL) ? TRUE: FALSE;

    /* Big keys are unsupported */
    ASSERT_VALUE_BIG(Hive, Length);
    ASSERT_VALUE_BIG(Hive, DataSize);

    /* Mark the old value dirty */
    CmpMarkValueDataDirty(Hive, Value);

    /* Check if we have a small key */
    if (IsSmall)
    {
        /* Check if we had a normal key with some data in it */
        if (!(WasSmall) && (Length > 0))
        {
            /* Free the previous data */
            CmpFreeValueData(Hive, Value->Data, Length);
        }

        /* Write our data directly */
        Value->DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        Value->Data = TempData;
        Value->Type = Type;
        return STATUS_SUCCESS;
    }
    else
    {
        /* We have a normal key. Was the old cell also normal and had data? */
        if (!(WasSmall) && (Length > 0))
        {
            /* Get the current data cell and actual data inside it */
            DataCell = Value->Data;
            ASSERT(DataCell != HCELL_NIL);
            CellData = HvGetCell(Hive, DataCell);
            if (!CellData) return STATUS_INSUFFICIENT_RESOURCES;

            /* Immediately release the cell */
            HvReleaseCell(Hive, DataCell);

            /* Make sure that the data cell actually has a size */
            ASSERT(HvGetCellSize(Hive, CellData) > 0);

            /* Check if the previous data cell could fit our new data */
            if (DataSize <= (ULONG)(HvGetCellSize(Hive, CellData)))
            {
                /* Re-use it then */
                NewCell = DataCell;
            }
            else
            {
                /* Otherwise, re-allocate the current data cell */
                NewCell = HvReallocateCell(Hive, DataCell, DataSize);
                if (NewCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            /* This was a small key, or a key with no data, allocate a cell */
            NewCell = HvAllocateCell(Hive, DataSize, StorageType, HCELL_NIL);
            if (NewCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Now get the actual data for our data cell */
        CellData = HvGetCell(Hive, NewCell);
        if (!CellData) ASSERT(FALSE);

        /* Release it immediately */
        HvReleaseCell(Hive, NewCell);

        /* Copy our data into the data cell's buffer, and set up the value */
        RtlCopyMemory(CellData, Data, DataSize);
        Value->Data = NewCell;
        Value->DataLength = DataSize;
        Value->Type = Type;

        /* Return success */
        ASSERT(HvIsCellDirty(Hive, NewCell));
        return STATUS_SUCCESS;
    }
}

NTSTATUS
NTAPI
CmSetValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
              IN PUNICODE_STRING ValueName,
              IN ULONG Type,
              IN PVOID Data,
              IN ULONG DataLength)
{
    PHHIVE Hive;
    PCM_KEY_NODE Parent;
    PCM_KEY_VALUE Value = NULL;
    HCELL_INDEX CurrentChild, Cell;
    NTSTATUS Status;
    BOOLEAN Found, Result;
    ULONG Count, ChildIndex, SmallData, Storage;
    VALUE_SEARCH_RETURN_TYPE SearchResult;

    /* Acquire hive lock */
    CmpLockRegistry();
    CmpAcquireKcbLockShared(Kcb);
    
    /* Sanity check */
    ASSERT(sizeof(ULONG) == CM_KEY_VALUE_SMALL);
    
    /* Don't touch deleted KCBs */
DoAgain:
    if (Kcb->Delete)
    {
        /* Fail */
        Status = STATUS_KEY_DELETED;
        goto Quickie;
    }
    
    /* Don't let anyone mess with symlinks */
    if ((Kcb->Flags & KEY_SYM_LINK) &&
        ((Type != REG_LINK) ||
         !(ValueName) ||
         !(RtlEqualUnicodeString(&CmSymbolicLinkValueName, ValueName, TRUE))))
    {
        /* Invalid modification of a symlink key */
        Status = STATUS_ACCESS_DENIED;
        goto Quickie;
    }
    
    /* Search for the value */
    SearchResult = SearchFail;
    if (SearchResult == SearchNeedExclusiveLock)
    {
        /* Try again with the exclusive lock */
        CmpConvertKcbSharedToExclusive(Kcb);
        goto DoAgain;
    }
    else if (SearchResult == SearchSuccess)
    {
        /* We don't actually need to do anything! */
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* We need the exclusive KCB lock now */
    if (!(CmpIsKcbLockedExclusive(Kcb)) && !(CmpTryToConvertKcbSharedToExclusive(Kcb)))
    {
        /* Acquire exclusive lock */
        CmpConvertKcbSharedToExclusive(Kcb);
    }

    /* Get pointer to key cell */
    Hive = Kcb->KeyHive;
    Cell = Kcb->KeyCell;

    /* Prepare to scan the key node */
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    Count = Parent->ValueList.Count;
    Found = FALSE;
    if (Count > 0)
    {
        /* Try to find the existing name */
        Result = CmpFindNameInList(Hive,
                                   &Parent->ValueList,
                                   ValueName,
                                   &ChildIndex,
                                   &CurrentChild);
        if (!Result)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Check if we found something */
        if (CurrentChild != HCELL_NIL)
        {
            /* Get its value */
            Value = (PCM_KEY_VALUE)HvGetCell(Hive, CurrentChild);
            if (!Value)
            {
                /* Fail */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Quickie;
            }

            /* Remember that we found it */
            Found = TRUE;
        }
    }
    else
    {
        /* No child list, we'll need to add it */
        ChildIndex = 0;
    }
    
    /* The KCB must be locked exclusive at this point */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    
    /* Mark the cell dirty */
    HvMarkCellDirty(Hive, Cell, FALSE);

    /* Get the storage type */
    Storage = HvGetCellType(Cell);

    /* Check if this is small data */
    SmallData = 0;
    if ((DataLength <= CM_KEY_VALUE_SMALL) && (DataLength > 0))
    {
        /* Copy it */
        RtlCopyMemory(&SmallData, Data, DataLength);
    }

    /* Check if we didn't find a matching key */
    if (!Found)
    {
        /* Call the internal routine */
        Status = CmpSetValueKeyNew(Hive,
                                   Parent,
                                   ValueName,
                                   ChildIndex,
                                   Type,
                                   Data,
                                   DataLength,
                                   Storage,
                                   SmallData);
    }
    else
    {
        /* Call the internal routine */
        Status = CmpSetValueKeyExisting(Hive,
                                        CurrentChild,
                                        Value,
                                        Type,
                                        Data,
                                        DataLength,
                                        Storage,
                                        SmallData);
    }

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Check if the maximum value name length changed */
        ASSERT(Parent->MaxValueNameLen == Kcb->KcbMaxValueNameLen);
        if (Parent->MaxValueNameLen < ValueName->Length)
        {
            /* Set the new values */
            Parent->MaxValueNameLen = ValueName->Length;
            Kcb->KcbMaxValueNameLen = ValueName->Length;
        }
    
        /* Check if the maximum data length changed */
        ASSERT(Parent->MaxValueDataLen == Kcb->KcbMaxValueDataLen);
        if (Parent->MaxValueDataLen < DataLength)
        {
            /* Update it */
            Parent->MaxValueDataLen = DataLength;
            Kcb->KcbMaxValueDataLen = Parent->MaxValueDataLen;
        }
        
        /* Save the write time */
        KeQuerySystemTime(&Parent->LastWriteTime);
        KeQuerySystemTime(&Kcb->KcbLastWriteTime);
        
        /* Check if the cell is cached */
        if ((Found) && (CMP_IS_CELL_CACHED(Kcb->ValueCache.ValueList)))
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }
        else
        {
            /* Cleanup the value cache */
            CmpCleanUpKcbValueCache(Kcb);

            /* Sanity checks */
            ASSERT(!(CMP_IS_CELL_CACHED(Kcb->ValueCache.ValueList)));
            ASSERT(!(Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND));
            
            /* Set the value cache */
            Kcb->ValueCache.Count = Parent->ValueList.Count;
            Kcb->ValueCache.ValueList = Parent->ValueList.List;
        }
    }
    
Quickie:
    /* Release the locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmDeleteValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                 IN UNICODE_STRING ValueName)
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PHHIVE Hive;
    PCM_KEY_NODE Parent;
    HCELL_INDEX ChildCell, Cell;
    PCHILD_LIST ChildList;
    PCM_KEY_VALUE Value = NULL;
    ULONG ChildIndex;
    BOOLEAN Result;

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Lock KCB exclusively */
    CmpAcquireKcbLockExclusive(Kcb);
    
    /* Don't touch deleted keys */
    if (Kcb->Delete)
    {
        /* Undo everything */
        CmpReleaseKcbLock(Kcb);
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    /* Get the hive and the cell index */
    Hive = Kcb->KeyHive;
    Cell = Kcb->KeyCell;

    /* Get the parent key node */
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Parent)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Get the value list and check if it has any entries */
    ChildList = &Parent->ValueList;
    if (ChildList->Count)
    {
        /* Try to find this value */
        Result = CmpFindNameInList(Hive,
                                   ChildList,
                                   &ValueName,
                                   &ChildIndex,
                                   &ChildCell);
        if (!Result)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Value not found, return error */
        if (ChildCell == HCELL_NIL) goto Quickie;

        /* We found the value, mark all relevant cells dirty */
        HvMarkCellDirty(Hive, Cell, FALSE);
        HvMarkCellDirty(Hive, Parent->ValueList.List, FALSE);
        HvMarkCellDirty(Hive, ChildCell, FALSE);

        /* Get the key value */
        Value = (PCM_KEY_VALUE)HvGetCell(Hive,ChildCell);
        if (!Value) ASSERT(FALSE);

        /* Mark it and all related data as dirty */
        CmpMarkValueDataDirty(Hive, Value);

        /* Ssanity checks */
        ASSERT(HvIsCellDirty(Hive, Parent->ValueList.List));
        ASSERT(HvIsCellDirty(Hive, ChildCell));

        /* Remove the value from the child list */
        Status = CmpRemoveValueFromList(Hive, ChildIndex, ChildList);
        if(!NT_SUCCESS(Status)) goto Quickie;

        /* Remove the value and its data itself */
        if (!CmpFreeValue(Hive, ChildCell))
        {
            /* Failed to free the value, fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Set the last write time */
        KeQuerySystemTime(&Parent->LastWriteTime);
        KeQuerySystemTime(&Kcb->KcbLastWriteTime);

        /* Sanity check */
        ASSERT(Parent->MaxValueNameLen == Kcb->KcbMaxValueNameLen);
        ASSERT(Parent->MaxValueDataLen == Kcb->KcbMaxValueDataLen);
        ASSERT(HvIsCellDirty(Hive, Cell));

        /* Check if the value list is empty now */
        if (!Parent->ValueList.Count)
        {
            /* Then clear key node data */
            Parent->MaxValueNameLen = 0;
            Parent->MaxValueDataLen = 0;
            Kcb->KcbMaxValueNameLen = 0;
            Kcb->KcbMaxValueDataLen = 0;
        }
        
        /* Cleanup the value cache */
        CmpCleanUpKcbValueCache(Kcb);
        
        /* Sanity checks */
        ASSERT(!(CMP_IS_CELL_CACHED(Kcb->ValueCache.ValueList)));
        ASSERT(!(Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND));
        
        /* Set the value cache */
        Kcb->ValueCache.Count = ChildList->Count;
        Kcb->ValueCache.ValueList = ChildList->List;

        /* Change default Status to success */
        Status = STATUS_SUCCESS;
    }

Quickie:
    /* Release the parent cell, if any */
    if (Parent) HvReleaseCell(Hive, Cell);

    /* Check if we had a value */
    if (Value)
    {
        /* Release the child cell */
        ASSERT(ChildCell != HCELL_NIL);
        HvReleaseCell(Hive, ChildCell);
    }

    /* Release locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmQueryValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                IN UNICODE_STRING ValueName,
                IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                IN PVOID KeyValueInformation,
                IN ULONG Length,
                IN PULONG ResultLength)
{
    NTSTATUS Status;
    PCM_KEY_VALUE ValueData;
    ULONG Index;
    BOOLEAN ValueCached = FALSE;
    PCM_CACHED_VALUE *CachedValue;
    HCELL_INDEX CellToRelease;
    VALUE_SEARCH_RETURN_TYPE Result;
    PHHIVE Hive;
    PAGED_CODE();

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Lock the KCB shared */
    CmpAcquireKcbLockShared(Kcb);
    
    /* Don't touch deleted keys */
DoAgain:
    if (Kcb->Delete)
    {
        /* Undo everything */
        CmpReleaseKcbLock(Kcb);
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }
    
    /* We don't deal with this yet */
    if (Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
    }

    /* Get the hive */
    Hive = Kcb->KeyHive;

    /* Find the key value */
    Result = CmpFindValueByNameFromCache(Kcb,
                                         &ValueName,
                                         &CachedValue,
                                         &Index,
                                         &ValueData,
                                         &ValueCached,
                                         &CellToRelease);
    if (Result == SearchNeedExclusiveLock)
    {
        /* Check if we need an exclusive lock */
        ASSERT(CellToRelease == HCELL_NIL);
        ASSERT(ValueData == NULL);
        
        /* Try with exclusive KCB lock */
        CmpConvertKcbSharedToExclusive(Kcb);
        goto DoAgain;
    }
    
    if (Result == SearchSuccess)
    {
        /* Sanity check */
        ASSERT(ValueData != NULL);

        /* Query the information requested */
        Result = CmpQueryKeyValueData(Kcb,
                                      CachedValue,
                                      ValueData,
                                      ValueCached,
                                      KeyValueInformationClass,
                                      KeyValueInformation,
                                      Length,
                                      ResultLength,
                                      &Status);
        if (Result == SearchNeedExclusiveLock)
        {            
            /* Try with exclusive KCB lock */
            CmpConvertKcbSharedToExclusive(Kcb);
            goto DoAgain;
        }
    }
    else
    {
        /* Failed to find the value */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* If we have a cell to release, do so */
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);

    /* Release locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmEnumerateValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                    IN ULONG Index,
                    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                    IN PVOID KeyValueInformation,
                    IN ULONG Length,
                    IN PULONG ResultLength)
{
    NTSTATUS Status;
    PHHIVE Hive;
    PCM_KEY_NODE Parent;
    HCELL_INDEX CellToRelease = HCELL_NIL, CellToRelease2 = HCELL_NIL;
    VALUE_SEARCH_RETURN_TYPE Result;
    BOOLEAN IndexIsCached, ValueIsCached = FALSE;
    PCELL_DATA CellData;
    PCM_CACHED_VALUE *CachedValue;
    PCM_KEY_VALUE ValueData;
    PAGED_CODE();

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Lock the KCB shared */
    CmpAcquireKcbLockShared(Kcb);

    /* Don't touch deleted keys */
DoAgain:
    if (Kcb->Delete)
    {
        /* Undo everything */
        CmpReleaseKcbLock(Kcb);
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    /* Get the hive and parent */
    Hive = Kcb->KeyHive;
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
    if (!Parent)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Make sure the index is valid */
    //if (Index >= Kcb->ValueCache.Count)
    if (Index >= Parent->ValueList.Count)
    {
        /* Release the cell and fail */
        HvReleaseCell(Hive, Kcb->KeyCell);
        Status = STATUS_NO_MORE_ENTRIES;
        goto Quickie;
    }
    
    /* We don't deal with this yet */
    if (Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
    }

    /* Find the value list */
    Result = CmpGetValueListFromCache(Kcb,
                                      &CellData,
                                      &IndexIsCached,
                                      &CellToRelease);
    if (Result == SearchNeedExclusiveLock)
    {
        /* Check if we need an exclusive lock */
        ASSERT(CellToRelease == HCELL_NIL);
        ASSERT(ValueData == NULL);
        
        /* Try with exclusive KCB lock */
        CmpConvertKcbSharedToExclusive(Kcb);
        goto DoAgain;
    }
    else if (Result != SearchSuccess)
    {
        /* Sanity check */
        ASSERT(CellData == NULL);

        /* Release the cell and fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Now get the key value */
    Result = CmpGetValueKeyFromCache(Kcb,
                                     CellData,
                                     Index,
                                     &CachedValue,
                                     &ValueData,
                                     IndexIsCached,
                                     &ValueIsCached,
                                     &CellToRelease2);
    if (Result == SearchNeedExclusiveLock)
    {
        /* Try with exclusive KCB lock */
        CmpConvertKcbSharedToExclusive(Kcb);
        goto DoAgain;
    }
    else if (Result != SearchSuccess)
    {
        /* Sanity check */
        ASSERT(ValueData == NULL);

        /* Release the cells and fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Query the information requested */
    Result = CmpQueryKeyValueData(Kcb,
                                  CachedValue,
                                  ValueData,
                                  ValueIsCached,
                                  KeyValueInformationClass,
                                  KeyValueInformation,
                                  Length,
                                  ResultLength,
                                  &Status);
    if (Result == SearchNeedExclusiveLock)
    {
        /* Try with exclusive KCB lock */
        CmpConvertKcbSharedToExclusive(Kcb);
        goto DoAgain;
    }

Quickie:
    /* If we have a cell to release, do so */
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);

    /* Release the parent cell */
    HvReleaseCell(Hive, Kcb->KeyCell);

    /* If we have a cell to release, do so */
    if (CellToRelease2 != HCELL_NIL) HvReleaseCell(Hive, CellToRelease2);

    /* Release locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmpQueryKeyData(IN PHHIVE Hive,
                IN PCM_KEY_NODE Node,
                IN KEY_INFORMATION_CLASS KeyInformationClass,
                IN OUT PVOID KeyInformation,
                IN ULONG Length,
                IN OUT PULONG ResultLength)
{
    NTSTATUS Status;
    ULONG Size, SizeLeft, MinimumSize;
    PKEY_INFORMATION Info = (PKEY_INFORMATION)KeyInformation;
    USHORT NameLength;

    /* Check if the value is compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Get the compressed name size */
        NameLength = CmpCompressedNameSize(Node->Name, Node->NameLength);
    }
    else
    {
        /* Get the real size */
        NameLength = Node->NameLength;
    }

    /* Check what kind of information is being requested */
    switch (KeyInformationClass)
    {
        /* Basic information */
        case KeyBasicInformation:

            /* This is the size we need */
            Size = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) + NameLength;

            /* And this is the minimum we can work with */
            MinimumSize = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name);

            /* Let the caller know and assume success */
            *ResultLength = Size;
            Status = STATUS_SUCCESS;

            /* Check if the bufer we got is too small */
            if (Length < MinimumSize)
            {
                /* Let the caller know and fail */
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Copy the basic information */
            Info->KeyBasicInformation.LastWriteTime = Node->LastWriteTime;
            Info->KeyBasicInformation.TitleIndex = 0;
            Info->KeyBasicInformation.NameLength = NameLength;

            /* Only the name is left */
            SizeLeft = Length - MinimumSize;
            Size = NameLength;

            /* Check if we don't have enough space for the name */
            if (SizeLeft < Size)
            {
                /* Truncate the name we'll return, and tell the caller */
                Size = SizeLeft;
                Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Check if this is a compressed key */
            if (Node->Flags & KEY_COMP_NAME)
            {
                /* Copy the compressed name */
                CmpCopyCompressedName(Info->KeyBasicInformation.Name,
                                      SizeLeft,
                                      Node->Name,
                                      Node->NameLength);
            }
            else
            {
                /* Otherwise, copy the raw name */
                RtlCopyMemory(Info->KeyBasicInformation.Name,
                              Node->Name,
                              Size);
            }
            break;

        /* Node information */
        case KeyNodeInformation:

            /* Calculate the size we need */
            Size = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) +
                   NameLength +
                   Node->ClassLength;

            /* And the minimum size we can support */
            MinimumSize = FIELD_OFFSET(KEY_NODE_INFORMATION, Name);

            /* Return the size to the caller and assume succes */
            *ResultLength = Size;
            Status = STATUS_SUCCESS;

            /* Check if the caller's buffer is too small */
            if (Length < MinimumSize)
            {
                /* Let them know, and fail */
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Copy the basic information */
            Info->KeyNodeInformation.LastWriteTime = Node->LastWriteTime;
            Info->KeyNodeInformation.TitleIndex = 0;
            Info->KeyNodeInformation.ClassLength = Node->ClassLength;
            Info->KeyNodeInformation.NameLength = NameLength;

            /* Now the name is left */
            SizeLeft = Length - MinimumSize;
            Size = NameLength;

            /* Check if the name can fit entirely */
            if (SizeLeft < Size)
            {
                /* It can't, we'll have to truncate. Tell the caller */
                Size = SizeLeft;
                Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Check if the key node name is compressed */
            if (Node->Flags & KEY_COMP_NAME)
            {
                /* Copy the compressed name */
                CmpCopyCompressedName(Info->KeyNodeInformation.Name,
                                      SizeLeft,
                                      Node->Name,
                                      Node->NameLength);
            }
            else
            {
                /* It isn't, so copy the raw name */
                RtlCopyMemory(Info->KeyNodeInformation.Name,
                              Node->Name,
                              Size);
            }

            /* Check if the node has a class */
            if (Node->ClassLength > 0)
            {
                /* It does. We don't support these yet */
                ASSERTMSG("Classes not supported\n", FALSE);
            }
            else
            {
                /* It doesn't, so set offset to -1, not 0! */
                Info->KeyNodeInformation.ClassOffset = 0xFFFFFFFF;
            }
            break;

        /* Full information requsted */
        case KeyFullInformation:

            /* This is the size we need */
            Size = FIELD_OFFSET(KEY_FULL_INFORMATION, Class) +
                   Node->ClassLength;

            /* This is what we can work with */
            MinimumSize = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

            /* Return it to caller and assume success */
            *ResultLength = Size;
            Status = STATUS_SUCCESS;

            /* Check if the caller's buffer is to small */
            if (Length < MinimumSize)
            {
                /* Let them know and fail */
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Now copy all the basic information */
            Info->KeyFullInformation.LastWriteTime = Node->LastWriteTime;
            Info->KeyFullInformation.TitleIndex = 0;
            Info->KeyFullInformation.ClassLength = Node->ClassLength;
            Info->KeyFullInformation.SubKeys = Node->SubKeyCounts[Stable] +
                                               Node->SubKeyCounts[Volatile];
            Info->KeyFullInformation.Values = Node->ValueList.Count;
            Info->KeyFullInformation.MaxNameLen = Node->MaxNameLen;
            Info->KeyFullInformation.MaxClassLen = Node->MaxClassLen;
            Info->KeyFullInformation.MaxValueNameLen = Node->MaxValueNameLen;
            Info->KeyFullInformation.MaxValueDataLen = Node->MaxValueDataLen;

            /* Check if we have a class */
            if (Node->ClassLength > 0)
            {
                /* We do, but we currently don't support this */
                ASSERTMSG("Classes not supported\n", FALSE);
            }
            else
            {
                /* We don't have a class, so set offset to -1, not 0! */
                Info->KeyNodeInformation.ClassOffset = 0xFFFFFFFF;
            }
            break;

        /* Any other class that got sent here is invalid! */
        default:

            /* Set failure code */
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmQueryKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
           IN KEY_INFORMATION_CLASS KeyInformationClass,
           IN PVOID KeyInformation,
           IN ULONG Length,
           IN PULONG ResultLength)
{
    NTSTATUS Status;
    PHHIVE Hive;
    PCM_KEY_NODE Parent;

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Lock KCB shared */
    CmpAcquireKcbLockShared(Kcb);

    /* Get the hive and parent */
    Hive = Kcb->KeyHive;
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
    if (!Parent)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }
    
    /* Don't touch deleted keys */
    if (Kcb->Delete)
    {
        /* Fail */
        Status = STATUS_KEY_DELETED;
        goto Quickie;
    }

    /* Check what class we got */
    switch (KeyInformationClass)
    {
        /* Typical information */
        case KeyFullInformation:
        case KeyBasicInformation:
        case KeyNodeInformation:

            /* Call the internal API */
            Status = CmpQueryKeyData(Hive,
                                     Parent,
                                     KeyInformationClass,
                                     KeyInformation,
                                     Length,
                                     ResultLength);
            break;

        /* Unsupported classes for now */
        case KeyNameInformation:
        case KeyCachedInformation:
        case KeyFlagsInformation:

            /* Print message and fail */
            DPRINT1("Unsupported class: %d!\n", KeyInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Illegal classes */
        default:

            /* Print message and fail */
            DPRINT1("Unsupported class: %d!\n", KeyInformationClass);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

Quickie:
    /* Release locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmEnumerateKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
               IN ULONG Index,
               IN KEY_INFORMATION_CLASS KeyInformationClass,
               IN PVOID KeyInformation,
               IN ULONG Length,
               IN PULONG ResultLength)
{
    NTSTATUS Status;
    PHHIVE Hive;
    PCM_KEY_NODE Parent, Child;
    HCELL_INDEX ChildCell;

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Lock the KCB shared */
    CmpAcquireKcbLockShared(Kcb);
    
    /* Don't touch deleted keys */
    if (Kcb->Delete)
    {
        /* Undo everything */
        CmpReleaseKcbLock(Kcb);
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    /* Get the hive and parent */
    Hive = Kcb->KeyHive;
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
    if (!Parent)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Get the child cell */
    ChildCell = CmpFindSubKeyByNumber(Hive, Parent, Index);

    /* Release the parent cell */
    HvReleaseCell(Hive, Kcb->KeyCell);

    /* Check if we found the child */
    if (ChildCell == HCELL_NIL)
    {
        /* We didn't, fail */
        Status = STATUS_NO_MORE_ENTRIES;
        goto Quickie;
    }

    /* Now get the actual child node */
    Child = (PCM_KEY_NODE)HvGetCell(Hive, ChildCell);
    if (!Child)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Query the data requested */
    Status = CmpQueryKeyData(Hive,
                             Child,
                             KeyInformationClass,
                             KeyInformation,
                             Length,
                             ResultLength);

Quickie:
    /* Release locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmDeleteKey(IN PCM_KEY_BODY KeyBody)
{
    NTSTATUS Status;
    PHHIVE Hive;
    PCM_KEY_NODE Node, Parent;
    HCELL_INDEX Cell, ParentCell;
    PCM_KEY_CONTROL_BLOCK Kcb;

    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Get the kcb */
    Kcb = KeyBody->KeyControlBlock;
    
    /* Don't allow deleting the root */
    if (!Kcb->ParentKcb)
    {
        /* Fail */
        CmpUnlockRegistry();
        return STATUS_CANNOT_DELETE;
    }
    
    /* Lock parent and child */
    CmpAcquireTwoKcbLocksExclusiveByKey(Kcb->ConvKey, Kcb->ParentKcb->ConvKey);
    
    /* Check if we're already being deleted */
    if (Kcb->Delete)
    {
        /* Don't do it twice */
        Status = STATUS_SUCCESS;
        goto Quickie2;
    }

    /* Get the hive and node */
    Hive = Kcb->KeyHive;
    Cell = Kcb->KeyCell;
    
    /* Get the key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Node)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }
   
    /* Sanity check */
    ASSERT(Node->Flags == Kcb->Flags);

    /* Check if we don't have any children */
    if (!(Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile]) &&
        !(Node->Flags & KEY_NO_DELETE))
    {
        /* Get the parent and free the cell */
        ParentCell = Node->Parent;
        Status = CmpFreeKeyByCell(Hive, Cell, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Clean up information we have on the subkey */
            CmpCleanUpSubKeyInfo(Kcb->ParentKcb);

            /* Get the parent node */
            Parent = (PCM_KEY_NODE)HvGetCell(Hive, ParentCell);
            if (Parent)
            {
                /* Update the maximum name length */
                Kcb->ParentKcb->KcbMaxNameLen = Parent->MaxNameLen;
                
                /* Make sure we're dirty */
                ASSERT(HvIsCellDirty(Hive, ParentCell));

                /* Update the write time */
                KeQuerySystemTime(&Parent->LastWriteTime);
                KeQuerySystemTime(&Kcb->ParentKcb->KcbLastWriteTime);

                /* Release the cell */
                HvReleaseCell(Hive, ParentCell);
            }
            
            /* Set the KCB in delete mode and remove it */
            Kcb->Delete = TRUE;
            CmpRemoveKeyControlBlock(Kcb);

            /* Clear the cell */
            Kcb->KeyCell = HCELL_NIL;
        }
    }
    else
    {
        /* Fail */
        Status = STATUS_CANNOT_DELETE;
    }
    
Quickie:
    /* Release the cell */
    HvReleaseCell(Hive, Cell);
    
    /* Release the KCB locks */
Quickie2:
    CmpReleaseTwoKcbLockByKey(Kcb->ConvKey, Kcb->ParentKcb->ConvKey);

    /* Release hive lock */
    CmpUnlockRegistry();
    return Status;
}

NTSTATUS
NTAPI
CmFlushKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
           IN BOOLEAN ExclusiveLock)
{
    PCMHIVE CmHive;
    NTSTATUS Status = STATUS_SUCCESS;
    PHHIVE Hive;

    /* Ignore flushes until we're ready */
    if (CmpNoWrite) return STATUS_SUCCESS;
    
    /* Get the hives */
    Hive = Kcb->KeyHive;
    CmHive = (PCMHIVE)Hive;
          
    /* Check if this is the master hive */
    if (CmHive == CmiVolatileHive)
    {
        /* Flush all the hives instead */
        CmpDoFlushAll(FALSE);
    }
    else
    {
        /* Flush only this hive */
        if (!HvSyncHive(Hive))
        {
            /* Fail */
            Status = STATUS_REGISTRY_IO_FAILED;
        }
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
CmLoadKey(IN POBJECT_ATTRIBUTES TargetKey,
          IN POBJECT_ATTRIBUTES SourceFile,
          IN ULONG Flags,
          IN PCM_KEY_BODY KeyBody)
{
    SECURITY_QUALITY_OF_SERVICE ServiceQos;
    SECURITY_CLIENT_CONTEXT ClientSecurityContext;
    HANDLE KeyHandle;
    BOOLEAN Allocate = TRUE;
    PCMHIVE CmHive;
    NTSTATUS Status;
    
    /* Check if we have a trust key */
    if (KeyBody)
    {
        /* Fail */
        DPRINT1("Trusted classes not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    
    /* Build a service QoS for a security context */
    ServiceQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ServiceQos.ImpersonationLevel = SecurityImpersonation;
    ServiceQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ServiceQos.EffectiveOnly = TRUE;
    Status = SeCreateClientSecurity(PsGetCurrentThread(),
                                    &ServiceQos,
                                    FALSE,
                                    &ClientSecurityContext);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("Security context failed\n");
        return Status;
    }
    
    /* Open the target key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, TargetKey);
    if (!NT_SUCCESS(Status)) KeyHandle = NULL;
    
    /* Open the hive */
    Status = CmpCmdHiveOpen(SourceFile,
                            &ClientSecurityContext,
                            &Allocate,
                            &CmHive,
                            0);

    /* Get rid of the security context */
    SeDeleteClientSecurity(&ClientSecurityContext);

    /* See if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* See if the target already existed */
        if (KeyHandle)
        {
            /* Lock the registry */
            CmpLockRegistryExclusive();
            
            /* FIXME: Check if we are already loaded */
            
            /* Release the registry */
            CmpUnlockRegistry();
        }
        
        /* Close the key handle if we had one */
        if (KeyHandle) ZwClose(KeyHandle);
        DPRINT1("Failed: %lx\n", Status);
        return Status;
    }
    
    /* Lock the registry shared */
    CmpLockRegistry();
    
    /* Lock the hive to this thread */
    CmHive->Hive.HiveFlags |= HIVE_IS_UNLOADING;
    CmHive->CreatorOwner = KeGetCurrentThread();
    
    /* Set flag */
    if (Flags & REG_NO_LAZY_FLUSH) CmHive->Hive.HiveFlags |= HIVE_NOLAZYFLUSH;
    
    /* Link the hive */
    Status = CmpLinkHiveToMaster(TargetKey->ObjectName,
                                 TargetKey->RootDirectory,
                                 CmHive,
                                 Allocate,
                                 TargetKey->SecurityDescriptor);
    if (NT_SUCCESS(Status))
    {
        /* FIXME: Add to HiveList key */
        
        /* Sync the hive if necessary */
        if (Allocate)
        {
            /* Sync it */
            HvSyncHive(&CmHive->Hive);
        }
        
        /* Release the hive */
        CmHive->Hive.HiveFlags &= ~HIVE_IS_UNLOADING;
        CmHive->CreatorOwner = NULL;
    }
    else
    {
        /* FIXME: TODO */
        
    }
    
    /* Unlock the registry */
    CmpUnlockRegistry();
    
    /* Close handle and return */
    if (KeyHandle) ZwClose(KeyHandle);
    return Status;
}
