/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpIsHiveAlreadyLoaded(IN HANDLE KeyHandle,
                       IN POBJECT_ATTRIBUTES SourceFile,
                       OUT PCMHIVE *CmHive)
{
    NTSTATUS Status;
    PCM_KEY_BODY KeyBody;
    PCMHIVE Hive;
    BOOLEAN Loaded = FALSE;
    PAGED_CODE();

    /* Sanity check */
    CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK();

    /* Reference the handle */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       KernelMode,
                                       (PVOID)&KeyBody,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Loaded;

    /* Don't touch deleted KCBs */
    if (KeyBody->KeyControlBlock->Delete) return Loaded;

    Hive = CONTAINING_RECORD(KeyBody->KeyControlBlock->KeyHive, CMHIVE, Hive);

    /* Must be the root key */
    if (!(KeyBody->KeyControlBlock->Flags & KEY_HIVE_ENTRY) ||
        !(Hive->FileUserName.Buffer))
    {
        /* It isn't */
        ObDereferenceObject(KeyBody);
        return Loaded;
    }

    /* Now compare the name of the file */
    if (!RtlCompareUnicodeString(&Hive->FileUserName,
                                 SourceFile->ObjectName,
                                 TRUE))
    {
        /* Same file found */
        Loaded = TRUE;
        *CmHive = Hive;

        /* If the hive is frozen, not sure what to do */
        if (Hive->Frozen)
        {
            /* FIXME: TODO */
            DPRINT1("ERROR: Hive is frozen\n");
            while (TRUE);
        }
     }

     /* Dereference and return result */
     ObDereferenceObject(KeyBody);
     return Loaded;
 }

BOOLEAN
NTAPI
CmpDoFlushAll(IN BOOLEAN ForceFlush)
{
    PLIST_ENTRY NextEntry;
    PCMHIVE Hive;
    NTSTATUS Status;
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
            /* Acquire the flusher lock */
            CmpLockHiveFlusherExclusive(Hive);

            /* Check for illegal state */
            if ((ForceFlush) && (Hive->UseCount))
            {
                /* Registry needs to be locked down */
                CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK();
                DPRINT1("FIXME: Hive is damaged and needs fixup\n");
                while (TRUE);
            }

            /* Only sync if we are forced to or if it won't cause a hive shrink */
            if ((ForceFlush) || (!HvHiveWillShrink(&Hive->Hive)))
            {
                /* Do the sync */
                Status = HvSyncHive(&Hive->Hive);

                /* If something failed - set the flag and continue looping */
                if (!NT_SUCCESS(Status)) Result = FALSE;
            }
            else
            {
                /* We won't flush if the hive might shrink */
                Result = FALSE;
                CmpForceForceFlush = TRUE;
            }

            /* Release the flusher lock */
            CmpUnlockHiveFlusher(Hive);
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
        if (!HvMarkCellDirty(Hive, Parent->ValueList.List, FALSE))
        {
            /* Fail if we're out of space for log changes */
            return STATUS_NO_LOG_SPACE;
        }
    }

    /* Allocate a value cell */
    ValueCell = HvAllocateCell(Hive,
                               FIELD_OFFSET(CM_KEY_VALUE, Name) +
                               CmpNameSize(Hive, ValueName),
                               StorageType,
                               HCELL_NIL);
    if (ValueCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the actual data for it */
    CellData = HvGetCell(Hive, ValueCell);
    ASSERT(CellData);

    /* Now we can release it, make sure it's also dirty */
    HvReleaseCell(Hive, ValueCell);
    ASSERT(HvIsCellDirty(Hive, ValueCell));

    /* Set it up and copy the name */
    CellData->u.KeyValue.Signature = CM_KEY_VALUE_SIGNATURE;
    _SEH2_TRY
    {
        /* This can crash since the name is coming from user-mode */
        CellData->u.KeyValue.NameLength = CmpCopyName(Hive,
                                                      CellData->u.KeyValue.Name,
                                                      ValueName);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail */
        DPRINT1("Invalid user data!\n");
        HvFreeCell(Hive, ValueCell);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check for compressed name */
    if (CellData->u.KeyValue.NameLength < ValueName->Length)
    {
        /* This is a compressed name */
        CellData->u.KeyValue.Flags = VALUE_COMP_NAME;
    }
    else
    {
        /* No flags to set */
        CellData->u.KeyValue.Flags = 0;
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

    /* Set the type now */
    CellData->u.KeyValue.Type = Type;

    /* Add this value cell to the child list */
    Status = CmpAddValueToList(Hive,
                               ValueCell,
                               Index,
                               StorageType,
                               &Parent->ValueList);

    /* If we failed, free the entire cell, including the data */
    if (!NT_SUCCESS(Status))
    {
        /* Overwrite the status with a known one */
        CmpFreeValue(Hive, ValueCell);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

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

    /* Registry writes must be blocked */
    CMP_ASSERT_FLUSH_LOCK(Hive);

    /* Mark the old child cell dirty */
    if (!HvMarkCellDirty(Hive, OldChild, FALSE)) return STATUS_NO_LOG_SPACE;

    /* See if this is a small or normal key */
    WasSmall = CmpIsKeyValueSmall(&Length, Value->DataLength);

    /* See if our new data can fit in a small key */
    IsSmall = (DataSize <= CM_KEY_VALUE_SMALL) ? TRUE: FALSE;

    /* Big keys are unsupported */
    ASSERT_VALUE_BIG(Hive, Length);
    ASSERT_VALUE_BIG(Hive, DataSize);

    /* Mark the old value dirty */
    if (!CmpMarkValueDataDirty(Hive, Value)) return STATUS_NO_LOG_SPACE;

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
    ASSERT(CellData);

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
    ULONG Size, SizeLeft, MinimumSize, Offset;
    PKEY_INFORMATION Info = (PKEY_INFORMATION)KeyInformation;
    USHORT NameLength;
    PVOID ClassData;

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
                /* Set the class offset */
                Offset = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) + NameLength;
                Offset = ALIGN_UP_BY(Offset, sizeof(ULONG));
                Info->KeyNodeInformation.ClassOffset = Offset;

                /* Get the class data */
                ClassData = HvGetCell(Hive, Node->Class);
                if (ClassData == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Check if we can copy anything */
                if (Length > Offset)
                {
                    /* Copy the class data */
                    RtlCopyMemory((PUCHAR)Info + Offset,
                                  ClassData,
                                  min(Node->ClassLength, Length - Offset));
                }

                /* Check if the buffer was large enough */
                if (Length < Offset + Node->ClassLength)
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Release the class cell */
                HvReleaseCell(Hive, Node->Class);
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
                /* Set the class offset */
                Offset = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);
                Info->KeyFullInformation.ClassOffset = Offset;

                /* Get the class data */
                ClassData = HvGetCell(Hive, Node->Class);
                if (ClassData == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Copy the class data */
                ASSERT(Length >= Offset);
                RtlCopyMemory(Info->KeyFullInformation.Class,
                              ClassData,
                              min(Node->ClassLength, Length - Offset));

                /* Check if the buffer was large enough */
                if (Length < Offset + Node->ClassLength)
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Release the class cell */
                HvReleaseCell(Hive, Node->Class);
            }
            else
            {
                /* We don't have a class, so set offset to -1, not 0! */
                Info->KeyFullInformation.ClassOffset = 0xFFFFFFFF;
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
CmSetValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
              IN PUNICODE_STRING ValueName,
              IN ULONG Type,
              IN PVOID Data,
              IN ULONG DataLength)
{
    PHHIVE Hive = NULL;
    PCM_KEY_NODE Parent;
    PCM_KEY_VALUE Value = NULL;
    HCELL_INDEX CurrentChild, Cell;
    NTSTATUS Status;
    BOOLEAN Found, Result;
    ULONG Count, ChildIndex, SmallData, Storage;
    VALUE_SEARCH_RETURN_TYPE SearchResult;
    BOOLEAN FirstTry = TRUE, FlusherLocked = FALSE;
    HCELL_INDEX ParentCell = HCELL_NIL, ChildCell = HCELL_NIL;

    /* Acquire hive and KCB lock */
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

    /* Check if this is the first attempt */
    if (FirstTry)
    {
        /* Search for the value in the cache */
        SearchResult = CmpCompareNewValueDataAgainstKCBCache(Kcb,
                                                             ValueName,
                                                             Type,
                                                             Data,
                                                             DataLength);
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
        if (!(CmpIsKcbLockedExclusive(Kcb)) &&
            !(CmpTryToConvertKcbSharedToExclusive(Kcb)))
        {
            /* Acquire exclusive lock */
            CmpConvertKcbSharedToExclusive(Kcb);
        }

        /* Cache lookup failed, so don't try it next time */
        FirstTry = FALSE;

        /* Now grab the flush lock since the key will be modified */
        ASSERT(FlusherLocked == FALSE);
        CmpLockHiveFlusherShared((PCMHIVE)Kcb->KeyHive);
        FlusherLocked = TRUE;
        goto DoAgain;
    }
    else
    {
        /* Get pointer to key cell */
        Hive = Kcb->KeyHive;
        Cell = Kcb->KeyCell;

        /* Get the parent */
        Parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        ASSERT(Parent);
        ParentCell = Cell;

        /* Prepare to scan the key node */
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
                /* Release existing child */
                if (ChildCell != HCELL_NIL)
                {
                    HvReleaseCell(Hive, ChildCell);
                    ChildCell = HCELL_NIL;
                }

                /* Get its value */
                Value = (PCM_KEY_VALUE)HvGetCell(Hive, CurrentChild);
                if (!Value)
                {
                    /* Fail */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Quickie;
                }

                /* Remember that we found it */
                ChildCell = CurrentChild;
                Found = TRUE;
            }
        }
        else
        {
            /* No child list, we'll need to add it */
            ChildIndex = 0;
        }
    }

    /* Should only get here on the second pass */
    ASSERT(FirstTry == FALSE);

    /* The KCB must be locked exclusive at this point */
    CMP_ASSERT_KCB_LOCK(Kcb);

    /* Mark the cell dirty */
    if (!HvMarkCellDirty(Hive, Cell, FALSE))
    {
        /* Not enough log space, fail */
        Status = STATUS_NO_LOG_SPACE;
        goto Quickie;
    }

    /* Get the storage type */
    Storage = HvGetCellType(Cell);

    /* Check if this is small data */
    SmallData = 0;
    if ((DataLength <= CM_KEY_VALUE_SMALL) && (DataLength > 0))
    {
        /* Need SEH because user data may be invalid */
        _SEH2_TRY
        {
            /* Copy it */
            RtlCopyMemory(&SmallData, Data, DataLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return failure code */
            Status = _SEH2_GetExceptionCode();
            _SEH2_YIELD(goto Quickie);
        }
        _SEH2_END;
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
        Kcb->KcbLastWriteTime = Parent->LastWriteTime;

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

        /* Notify registered callbacks */
        CmpReportNotify(Kcb,
                        Hive,
                        Kcb->KeyCell,
                        REG_NOTIFY_CHANGE_LAST_SET);
    }

    /* Release the cells */
Quickie:
    if ((ParentCell != HCELL_NIL) && (Hive)) HvReleaseCell(Hive, ParentCell);
    if ((ChildCell != HCELL_NIL) && (Hive)) HvReleaseCell(Hive, ChildCell);

    /* Release the locks */
    if (FlusherLocked) CmpUnlockHiveFlusher((PCMHIVE)Hive);
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

    /* Lock flushes */
    CmpLockHiveFlusherShared((PCMHIVE)Hive);

    /* Get the parent key node */
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    ASSERT(Parent);

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
        if (!((HvMarkCellDirty(Hive, Cell, FALSE)) &&
              (HvMarkCellDirty(Hive, Parent->ValueList.List, FALSE)) &&
              (HvMarkCellDirty(Hive, ChildCell, FALSE))))
        {
            /* Not enough log space, fail */
            Status = STATUS_NO_LOG_SPACE;
            goto Quickie;
        }

        /* Get the key value */
        Value = (PCM_KEY_VALUE)HvGetCell(Hive, ChildCell);
        ASSERT(Value);

        /* Mark it and all related data as dirty */
        if (!CmpMarkValueDataDirty(Hive, Value))
        {
            /* Not enough log space, fail */
            Status = STATUS_NO_LOG_SPACE;
            goto Quickie;
        }

        /* Sanity checks */
        ASSERT(HvIsCellDirty(Hive, Parent->ValueList.List));
        ASSERT(HvIsCellDirty(Hive, ChildCell));

        /* Remove the value from the child list */
        Status = CmpRemoveValueFromList(Hive, ChildIndex, ChildList);
        if (!NT_SUCCESS(Status))
        {
            /* Set known error */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Remove the value and its data itself */
        if (!CmpFreeValue(Hive, ChildCell))
        {
            /* Failed to free the value, fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        /* Set the last write time */
        KeQuerySystemTime(&Parent->LastWriteTime);
        Kcb->KcbLastWriteTime = Parent->LastWriteTime;

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

        /* Notify registered callbacks */
        CmpReportNotify(Kcb, Hive, Cell, REG_NOTIFY_CHANGE_LAST_SET);

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
    CmpUnlockHiveFlusher((PCMHIVE)Hive);
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

        /* User data, protect against exceptions */
        _SEH2_TRY
        {
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
                /* Release the value cell */
                if (CellToRelease != HCELL_NIL)
                {
                    HvReleaseCell(Hive, CellToRelease);
                    CellToRelease = HCELL_NIL;
                }

                /* Try with exclusive KCB lock */
                CmpConvertKcbSharedToExclusive(Kcb);
                _SEH2_YIELD(goto DoAgain);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
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
    PCM_KEY_VALUE ValueData = NULL;
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
    ASSERT(Parent);

    /* FIXME: Lack of cache? */
    if (Kcb->ValueCache.Count != Parent->ValueList.Count)
    {
        DPRINT1("HACK: Overriding value cache count\n");
        Kcb->ValueCache.Count = Parent->ValueList.Count;
    }

    /* Make sure the index is valid */
    if (Index >= Kcb->ValueCache.Count)
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
        HvReleaseCell(Hive, Kcb->KeyCell);

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
        /* Cleanup state */
        ASSERT(CellToRelease2 == HCELL_NIL);
        if (CellToRelease)
        {
            HvReleaseCell(Hive, CellToRelease);
            CellToRelease = HCELL_NIL;
        }
        HvReleaseCell(Hive, Kcb->KeyCell);

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

    /* User data, need SEH */
    _SEH2_TRY
    {
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
            /* Cleanup state */
            if (CellToRelease2) HvReleaseCell(Hive, CellToRelease2);
            HvReleaseCell(Hive, Kcb->KeyCell);
            if (CellToRelease) HvReleaseCell(Hive, CellToRelease);

            /* Try with exclusive KCB lock */
            CmpConvertKcbSharedToExclusive(Kcb);
            _SEH2_YIELD(goto DoAgain);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

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

static
NTSTATUS
CmpQueryKeyDataFromCache(
    _In_ PCM_KEY_CONTROL_BLOCK Kcb,
    _Out_ PKEY_CACHED_INFORMATION KeyCachedInfo,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength)
{
    PCM_KEY_NODE Node;
    PHHIVE KeyHive;
    HCELL_INDEX KeyCell;
    USHORT NameLength;
    PAGED_CODE();

    /* Get the hive and cell index */
    KeyHive = Kcb->KeyHash.KeyHive;
    KeyCell = Kcb->KeyHash.KeyCell;

#if DBG
    /* Get the cell node */
    Node = (PCM_KEY_NODE)HvGetCell(KeyHive, KeyCell);
    if (Node != NULL)
    {
        ULONG SubKeyCount;
        ASSERT(Node->ValueList.Count == Kcb->ValueCache.Count);

        if (!(Kcb->ExtFlags & CM_KCB_INVALID_CACHED_INFO))
        {
            SubKeyCount = Node->SubKeyCounts[0] + Node->SubKeyCounts[1];
            if (Kcb->ExtFlags & CM_KCB_NO_SUBKEY)
            {
                ASSERT(SubKeyCount == 0);
            }
            else if (Kcb->ExtFlags & CM_KCB_SUBKEY_ONE)
            {
                ASSERT(SubKeyCount == 1);
            }
            else if (Kcb->ExtFlags & CM_KCB_SUBKEY_HINT)
            {
                ASSERT(SubKeyCount == Kcb->IndexHint->Count);
            }
            else
            {
                ASSERT(SubKeyCount == Kcb->SubKeyCount);
            }
        }

        ASSERT(Node->LastWriteTime.QuadPart == Kcb->KcbLastWriteTime.QuadPart);
        ASSERT(Node->MaxNameLen == Kcb->KcbMaxNameLen);
        ASSERT(Node->MaxValueNameLen == Kcb->KcbMaxValueNameLen);
        ASSERT(Node->MaxValueDataLen == Kcb->KcbMaxValueDataLen);

        /* Release the cell */
        HvReleaseCell(KeyHive, KeyCell);
    }
#endif // DBG

    /* Make sure we have a name block */
    if (Kcb->NameBlock == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check for compressed name */
    if (Kcb->NameBlock->Compressed)
    {
        /* Calculate the name size */
        NameLength = CmpCompressedNameSize(Kcb->NameBlock->NameHash.Name,
                                           Kcb->NameBlock->NameHash.NameLength);
    }
    else
    {
        /* Use the stored name size */
        NameLength = Kcb->NameBlock->NameHash.NameLength;
    }

    /* Validate buffer length (we do not copy the name!) */
    *ResultLength = sizeof(*KeyCachedInfo);
    if (Length < *ResultLength)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Fill the structure */
    KeyCachedInfo->LastWriteTime = Kcb->KcbLastWriteTime;
    KeyCachedInfo->TitleIndex = 0;
    KeyCachedInfo->NameLength = NameLength;
    KeyCachedInfo->Values = Kcb->ValueCache.Count;
    KeyCachedInfo->MaxNameLen = Kcb->KcbMaxNameLen;
    KeyCachedInfo->MaxValueNameLen = Kcb->KcbMaxValueNameLen;
    KeyCachedInfo->MaxValueDataLen = Kcb->KcbMaxValueDataLen;

    /* Check the ExtFlags for what we have */
    if (Kcb->ExtFlags & CM_KCB_INVALID_CACHED_INFO)
    {
        /* Cache is not valid, do a full lookup */
        DPRINT1("Kcb cache incoherency detected, kcb = %p\n", Kcb);

        /* Get the cell node */
        Node = (PCM_KEY_NODE)HvGetCell(KeyHive, KeyCell);
        if (Node == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Calculate number of subkeys */
        KeyCachedInfo->SubKeys = Node->SubKeyCounts[0] + Node->SubKeyCounts[1];

        /* Release the cell */
        HvReleaseCell(KeyHive, KeyCell);
    }
    else if (Kcb->ExtFlags & CM_KCB_NO_SUBKEY)
    {
        /* There are no subkeys */
        KeyCachedInfo->SubKeys = 0;
    }
    else if (Kcb->ExtFlags & CM_KCB_SUBKEY_ONE)
    {
        /* There is exactly one subley */
        KeyCachedInfo->SubKeys = 1;
    }
    else if (Kcb->ExtFlags & CM_KCB_SUBKEY_HINT)
    {
        /* Get the number of subkeys from the subkey hint */
        KeyCachedInfo->SubKeys = Kcb->IndexHint->Count;
    }
    else
    {
        /* No subkey hint, use the key count field */
        KeyCachedInfo->SubKeys = Kcb->SubKeyCount;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
CmpQueryFlagsInformation(
    _In_ PCM_KEY_CONTROL_BLOCK Kcb,
    _Out_ PKEY_USER_FLAGS_INFORMATION KeyFlagsInfo,
    _In_ ULONG Length,
    _In_ PULONG ResultLength)
{
    /* Validate the buffer size */
    *ResultLength = sizeof(*KeyFlagsInfo);
    if (Length < *ResultLength)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy the user flags */
    KeyFlagsInfo->UserFlags = Kcb->KcbUserFlags;

    return STATUS_SUCCESS;
}

static
NTSTATUS
CmpQueryNameInformation(
    _In_ PCM_KEY_CONTROL_BLOCK Kcb,
    _Out_opt_ PKEY_NAME_INFORMATION KeyNameInfo,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength)
{
    ULONG NeededLength;
    PCM_KEY_CONTROL_BLOCK CurrentKcb;

    NeededLength = 0;
    CurrentKcb = Kcb;

    /* Count the needed buffer size */
    while (CurrentKcb)
    {
        if (CurrentKcb->NameBlock->Compressed)
            NeededLength += CmpCompressedNameSize(CurrentKcb->NameBlock->Name, CurrentKcb->NameBlock->NameLength);
        else
            NeededLength += CurrentKcb->NameBlock->NameLength;

        NeededLength += sizeof(OBJ_NAME_PATH_SEPARATOR);

        CurrentKcb = CurrentKcb->ParentKcb;
    }

    _SEH2_TRY
    {
        *ResultLength = FIELD_OFFSET(KEY_NAME_INFORMATION, Name) + NeededLength;
        if (Length < RTL_SIZEOF_THROUGH_FIELD(KEY_NAME_INFORMATION, NameLength))
            _SEH2_YIELD(return STATUS_BUFFER_TOO_SMALL);
        if (Length < *ResultLength)
        {
            KeyNameInfo->NameLength = NeededLength;
            _SEH2_YIELD(return STATUS_BUFFER_OVERFLOW);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Do the real copy */
    KeyNameInfo->NameLength = 0;
    CurrentKcb = Kcb;

    _SEH2_TRY
    {
        while (CurrentKcb)
        {
            ULONG NameLength;

            if (CurrentKcb->NameBlock->Compressed)
            {
                NameLength = CmpCompressedNameSize(CurrentKcb->NameBlock->Name, CurrentKcb->NameBlock->NameLength);
                /* Copy the compressed name */
                CmpCopyCompressedName(&KeyNameInfo->Name[(NeededLength - NameLength)/sizeof(WCHAR)],
                                      NameLength,
                                      CurrentKcb->NameBlock->Name,
                                      CurrentKcb->NameBlock->NameLength);
            }
            else
            {
                NameLength = CurrentKcb->NameBlock->NameLength;
                /* Otherwise, copy the raw name */
                RtlCopyMemory(&KeyNameInfo->Name[(NeededLength - NameLength)/sizeof(WCHAR)],
                              CurrentKcb->NameBlock->Name,
                              NameLength);
            }

            NeededLength -= NameLength;
            NeededLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
            /* Add path separator */
            KeyNameInfo->Name[NeededLength/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            KeyNameInfo->NameLength += NameLength + sizeof(OBJ_NAME_PATH_SEPARATOR);

            CurrentKcb = CurrentKcb->ParentKcb;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Make sure we copied everything */
    ASSERT(NeededLength == 0);
    ASSERT(KeyNameInfo->Name[0] == OBJ_NAME_PATH_SEPARATOR);

    /* We're done */
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CmQueryKey(_In_ PCM_KEY_CONTROL_BLOCK Kcb,
           _In_ KEY_INFORMATION_CLASS KeyInformationClass,
           _Out_opt_ PVOID KeyInformation,
           _In_ ULONG Length,
           _Out_ PULONG ResultLength)
{
    NTSTATUS Status;
    PHHIVE Hive;
    PCM_KEY_NODE Parent;
    HV_TRACK_CELL_REF CellReferences = {0};

    /* Acquire hive lock */
    CmpLockRegistry();

    /* Lock KCB shared */
    CmpAcquireKcbLockShared(Kcb);

    /* Don't touch deleted keys */
    if (Kcb->Delete)
    {
        /* Fail */
        Status = STATUS_KEY_DELETED;
        goto Quickie;
    }

    /* Data can be user-mode, use SEH */
    _SEH2_TRY
    {
        /* Check what class we got */
        switch (KeyInformationClass)
        {
            /* Typical information */
            case KeyFullInformation:
            case KeyBasicInformation:
            case KeyNodeInformation:
            {
                /* Get the hive and parent */
                Hive = Kcb->KeyHive;
                Parent = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
                ASSERT(Parent);

                /* Track cell references */
                if (!HvTrackCellRef(&CellReferences, Hive, Kcb->KeyCell))
                {
                    /* Not enough memory to track references */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Call the internal API */
                    Status = CmpQueryKeyData(Hive,
                                             Parent,
                                             KeyInformationClass,
                                             KeyInformation,
                                             Length,
                                             ResultLength);
                }
                break;
            }

            case KeyCachedInformation:
            {
                /* Call the internal API */
                Status = CmpQueryKeyDataFromCache(Kcb,
                                                  KeyInformation,
                                                  Length,
                                                  ResultLength);
                break;
            }

            case KeyFlagsInformation:
            {
                /* Call the internal API */
                Status = CmpQueryFlagsInformation(Kcb,
                                                  KeyInformation,
                                                  Length,
                                                  ResultLength);
                break;
            }

            case KeyNameInformation:
            {
                /* Call the internal API */
                Status = CmpQueryNameInformation(Kcb,
                                                 KeyInformation,
                                                 Length,
                                                 ResultLength);
                break;
            }

            /* Illegal classes */
            default:
            {
                /* Print message and fail */
                DPRINT1("Unsupported class: %d!\n", KeyInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail with exception code */
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto Quickie);
    }
    _SEH2_END;

Quickie:
    /* Release references */
    HvReleaseFreeCellRefArray(&CellReferences);

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
    HV_TRACK_CELL_REF CellReferences = {0};

    /* Acquire hive lock */
    CmpLockRegistry();

    /* Lock the KCB shared */
    CmpAcquireKcbLockShared(Kcb);

    /* Don't touch deleted keys */
    if (Kcb->Delete)
    {
        /* Undo everything */
        Status = STATUS_KEY_DELETED;
        goto Quickie;
    }

    /* Get the hive and parent */
    Hive = Kcb->KeyHive;
    Parent = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
    ASSERT(Parent);

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
    ASSERT(Child);

    /* Track references */
    if (!HvTrackCellRef(&CellReferences, Hive, ChildCell))
    {
        /* Can't allocate memory for tracking */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Data can be user-mode, use SEH */
    _SEH2_TRY
    {
        /* Query the data requested */
        Status = CmpQueryKeyData(Hive,
                                 Child,
                                 KeyInformationClass,
                                 KeyInformation,
                                 Length,
                                 ResultLength);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail with exception code */
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto Quickie);
    }
    _SEH2_END;

Quickie:
    /* Release references */
    HvReleaseFreeCellRefArray(&CellReferences);

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
        goto Quickie;
    }

    /* Get the hive and node */
    Hive = Kcb->KeyHive;
    Cell = Kcb->KeyCell;

    /* Lock flushes */
    CmpLockHiveFlusherShared((PCMHIVE)Hive);

    /* Get the key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    ASSERT(Node);

    /* Sanity check */
    ASSERT(Node->Flags == Kcb->Flags);

    /* Check if we don't have any children */
    if (!(Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile]) &&
        !(Node->Flags & KEY_NO_DELETE))
    {
        /* Send notification to registered callbacks */
        CmpReportNotify(Kcb, Hive, Cell, REG_NOTIFY_CHANGE_NAME);

        /* Get the parent and free the cell */
        ParentCell = Node->Parent;
        Status = CmpFreeKeyByCell(Hive, Cell, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Flush any notifications */
            CmpFlushNotifiesOnKeyBodyList(Kcb, FALSE);

            /* Clean up information we have on the subkey */
            CmpCleanUpSubKeyInfo(Kcb->ParentKcb);

            /* Get the parent node */
            Parent = (PCM_KEY_NODE)HvGetCell(Hive, ParentCell);
            if (Parent)
            {
                /* Update the maximum name length */
                Kcb->ParentKcb->KcbMaxNameLen = (USHORT)Parent->MaxNameLen;

                /* Make sure we're dirty */
                ASSERT(HvIsCellDirty(Hive, ParentCell));

                /* Update the write time */
                KeQuerySystemTime(&Parent->LastWriteTime);
                Kcb->ParentKcb->KcbLastWriteTime = Parent->LastWriteTime;

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

    /* Release the cell */
    HvReleaseCell(Hive, Cell);

    /* Release flush lock */
    CmpUnlockHiveFlusher((PCMHIVE)Hive);

    /* Release the KCB locks */
Quickie:
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
#if DBG
    CM_CHECK_REGISTRY_STATUS CheckStatus;
#endif
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
#if DBG
        /* Make sure the registry hive we're going to flush is OK */
        CheckStatus = CmCheckRegistry(CmHive, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
        ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
#endif

        /* Don't touch the hive */
        CmpLockHiveFlusherExclusive(CmHive);

        ASSERT(CmHive->ViewLock);
        KeAcquireGuardedMutex(CmHive->ViewLock);
        CmHive->ViewLockOwner = KeGetCurrentThread();

        /* Will the hive shrink? */
        if (HvHiveWillShrink(Hive))
        {
            /* I don't believe the current Hv does shrinking */
            ASSERT(FALSE);
            // CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK_OR_LOADING(CmHive);
        }
        else
        {
            /* Now we can release views */
            ASSERT(CmHive->ViewLock);
            // CMP_ASSERT_VIEW_LOCK_OWNED(CmHive);
            ASSERT((CmpSpecialBootCondition == TRUE) ||
                   (CmHive->HiveIsLoading == TRUE) ||
                   (CmHive->ViewLockOwner == KeGetCurrentThread()) ||
                   (CmpTestRegistryLockExclusive() == TRUE));
            CmHive->ViewLockOwner = NULL;
            KeReleaseGuardedMutex(CmHive->ViewLock);
        }

        /* Flush only this hive */
        if (!HvSyncHive(Hive))
        {
            /* Fail */
            Status = STATUS_REGISTRY_IO_FAILED;
        }

        /* Release the flush lock */
        CmpUnlockHiveFlusher(CmHive);
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
    PCMHIVE CmHive, LoadedHive;
    NTSTATUS Status;
    CM_PARSE_CONTEXT ParseContext;

    /* Check if we have a trust key */
    if (KeyBody)
    {
        /* Fail */
        DPRINT("Trusted classes not yet supported\n");
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
    RtlZeroMemory(&ParseContext, sizeof(ParseContext));
    ParseContext.CreateOperation = FALSE;
    Status = ObOpenObjectByName(TargetKey,
                                CmpKeyObjectType,
                                KernelMode,
                                NULL,
                                KEY_READ,
                                &ParseContext,
                                &KeyHandle);
    if (!NT_SUCCESS(Status)) KeyHandle = NULL;

    /* Open the hive */
    Status = CmpCmdHiveOpen(SourceFile,
                            &ClientSecurityContext,
                            &Allocate,
                            &CmHive,
                            CM_CHECK_REGISTRY_PURGE_VOLATILES);

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

            /* Check if we are already loaded */
            if (CmpIsHiveAlreadyLoaded(KeyHandle, SourceFile, &LoadedHive))
            {
                /* That's okay then */
                ASSERT(LoadedHive);
                Status = STATUS_SUCCESS;
            }

            /* Release the registry */
            CmpUnlockRegistry();
        }

        /* Close the key handle if we had one */
        if (KeyHandle) ZwClose(KeyHandle);
        return Status;
    }

    /* Lock the registry shared */
    CmpLockRegistry();

    /* Lock loading */
    ExAcquirePushLockExclusive(&CmpLoadHiveLock);

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
        /* Add to HiveList key */
        CmpAddToHiveFileList(CmHive);

        /* Sync the hive if necessary */
        if (Allocate)
        {
            /* Sync it under the flusher lock */
            CmpLockHiveFlusherExclusive(CmHive);
            HvSyncHive(&CmHive->Hive);
            CmpUnlockHiveFlusher(CmHive);
        }

        /* Release the hive */
        CmHive->Hive.HiveFlags &= ~HIVE_IS_UNLOADING;
        CmHive->CreatorOwner = NULL;
    }
    else
    {
        DPRINT1("CmpLinkHiveToMaster failed, Status %lx\n", Status);

        /* We're touching this hive, set the loading flag */
        CmHive->HiveIsLoading = TRUE;

        /* Close associated file handles */
        CmpCloseHiveFiles(CmHive);

        /* Cleanup its resources */
        CmpDestroyHive(CmHive);
    }

    /* Allow loads */
    ExReleasePushLock(&CmpLoadHiveLock);

    /* Is this first profile load? */
    if (!CmpProfileLoaded && !CmpWasSetupBoot)
    {
        /* User is now logged on, set quotas */
        CmpProfileLoaded = TRUE;
        CmpSetGlobalQuotaAllowed();
    }

    /* Unlock the registry */
    CmpUnlockRegistry();

    /* Close handle and return */
    if (KeyHandle) ZwClose(KeyHandle);
    return Status;
}

static
BOOLEAN
NTAPI
CmpUnlinkHiveFromMaster(IN PCMHIVE CmHive,
                        IN HCELL_INDEX Cell)
{
    PCELL_DATA CellData;
    HCELL_INDEX LinkCell;
    NTSTATUS Status;

    DPRINT("CmpUnlinkHiveFromMaster()\n");

    /* Get the cell data */
    CellData = HvGetCell(&CmHive->Hive, Cell);
    if (CellData == NULL)
        return FALSE;

    /* Get the link cell and release the current cell */
    LinkCell = CellData->u.KeyNode.Parent;
    HvReleaseCell(&CmHive->Hive, Cell);

    /* Remove the link cell from the master hive */
    CmpLockHiveFlusherExclusive(CmiVolatileHive);
    Status = CmpFreeKeyByCell((PHHIVE)CmiVolatileHive,
                              LinkCell,
                              TRUE);
    CmpUnlockHiveFlusher(CmiVolatileHive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpFreeKeyByCell() failed (Status 0x%08lx)\n", Status);
        return FALSE;
    }

    /* Remove the hive from the list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);
    RemoveEntryList(&CmHive->HiveList);
    ExReleasePushLock(&CmpHiveListHeadLock);

    return TRUE;
}

NTSTATUS
NTAPI
CmUnloadKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
            IN ULONG Flags)
{
    PHHIVE Hive;
    PCMHIVE CmHive;
    HCELL_INDEX Cell;

    DPRINT("CmUnloadKey(%p, %lx)\n", Kcb, Flags);

    /* Get the hive */
    Hive = Kcb->KeyHive;
    Cell = Kcb->KeyCell;
    CmHive = (PCMHIVE)Hive;

    /* Fail if the key is not a hive root key */
    if (Cell != Hive->BaseBlock->RootCell)
    {
        DPRINT1("Key is not a hive root key!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Fail if we try to unload the master hive */
    if (CmHive == CmiVolatileHive)
    {
        DPRINT1("Do not try to unload the master hive!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Mark this hive as being unloaded */
    Hive->HiveFlags |= HIVE_IS_UNLOADING;

    /* Search for any opened keys in this hive, and take an appropriate action */
    if (Kcb->RefCount > 1)
    {
        if (Flags != REG_FORCE_UNLOAD)
        {
            if (CmpEnumerateOpenSubKeys(Kcb, FALSE, FALSE) != 0)
            {
                /* There are open subkeys but we don't force hive unloading, fail */
                Hive->HiveFlags &= ~HIVE_IS_UNLOADING;
                return STATUS_CANNOT_DELETE;
            }
        }
        else
        {
            DPRINT1("CmUnloadKey: Force unloading is HALF-IMPLEMENTED, expect dangling KCBs problems!\n");
            if (CmpEnumerateOpenSubKeys(Kcb, TRUE, TRUE) != 0)
            {
                /* There are open subkeys that we cannot force to unload, fail */
                Hive->HiveFlags &= ~HIVE_IS_UNLOADING;
                return STATUS_CANNOT_DELETE;
            }
        }
    }

    /* Set the loading flag */
    CmHive->HiveIsLoading = TRUE;

    /* Flush the hive */
    CmFlushKey(Kcb, TRUE);

    /* Unlink the hive from the master hive */
    if (!CmpUnlinkHiveFromMaster(CmHive, Cell))
    {
        DPRINT("CmpUnlinkHiveFromMaster() failed!\n");

        /* Remove the unloading flag */
        Hive->HiveFlags &= ~HIVE_IS_UNLOADING;

        /* Reset the loading flag */
        CmHive->HiveIsLoading = FALSE;

        /* Return failure */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Flush any notifications if we force hive unloading */
    if (Flags == REG_FORCE_UNLOAD)
        CmpFlushNotifiesOnKeyBodyList(Kcb, TRUE); // Lock is already held

    /* Clean up information we have on the subkey */
    CmpCleanUpSubKeyInfo(Kcb->ParentKcb);

    /* Set the KCB in delete mode and remove it */
    Kcb->Delete = TRUE;
    CmpRemoveKeyControlBlock(Kcb);

    if (Flags != REG_FORCE_UNLOAD)
    {
        /* Release the KCB locks */
        CmpReleaseTwoKcbLockByKey(Kcb->ConvKey, Kcb->ParentKcb->ConvKey);

        /* Release the hive loading lock */
        ExReleasePushLockExclusive(&CmpLoadHiveLock);
    }

    /* Release hive lock */
    CmpUnlockRegistry();

    /* Close file handles */
    CmpCloseHiveFiles(CmHive);

    /* Remove the hive from the hive file list */
    CmpRemoveFromHiveFileList(CmHive);

/**
 ** NOTE:
 ** The following code is mostly equivalent to what we "call" CmpDestroyHive()
 **/
    /* Destroy the security descriptor cache */
    CmpDestroySecurityCache(CmHive);

    /* Destroy the view list */
    CmpDestroyHiveViewList(CmHive);

    /* Delete the flusher lock */
    ExDeleteResourceLite(CmHive->FlusherLock);
    ExFreePoolWithTag(CmHive->FlusherLock, TAG_CMHIVE);

    /* Delete the view lock */
    ExFreePoolWithTag(CmHive->ViewLock, TAG_CMHIVE);

    /* Free the hive storage */
    HvFree(Hive);

    /* Free the hive */
    CmpFree(CmHive, TAG_CM);

    return STATUS_SUCCESS;
}

ULONG
NTAPI
CmpEnumerateOpenSubKeys(
    IN PCM_KEY_CONTROL_BLOCK RootKcb,
    IN BOOLEAN RemoveEmptyCacheEntries,
    IN BOOLEAN DereferenceOpenedEntries)
{
    PCM_KEY_HASH Entry;
    PCM_KEY_CONTROL_BLOCK CachedKcb;
    PCM_KEY_CONTROL_BLOCK ParentKcb;
    ULONG ParentKeyCount;
    ULONG i, j;
    ULONG SubKeys = 0;

    DPRINT("CmpEnumerateOpenSubKeys() called\n");

    /* The root key is the only referenced key. There are no referenced sub keys. */
    if (RootKcb->RefCount == 1)
    {
        DPRINT("Open sub keys: 0\n");
        return 0;
    }

    /* Enumerate all hash lists */
    for (i = 0; i < CmpHashTableSize; i++)
    {
        /* Get the first cache entry */
        Entry = CmpCacheTable[i].Entry;

        /* Enumerate all cache entries */
        while (Entry)
        {
            /* Get the KCB of the current cache entry */
            CachedKcb = CONTAINING_RECORD(Entry, CM_KEY_CONTROL_BLOCK, KeyHash);

            /* Check keys only that are subkeys to our root key */
            if (CachedKcb->TotalLevels > RootKcb->TotalLevels)
            {
                /* Calculate the number of parent keys to the root key */
                ParentKeyCount = CachedKcb->TotalLevels - RootKcb->TotalLevels;

                /* Find a parent key that could be the root key */
                ParentKcb = CachedKcb;
                for (j = 0; j < ParentKeyCount; j++)
                {
                    ParentKcb = ParentKcb->ParentKcb;
                }

                /* Check whether the parent is the root key */
                if (ParentKcb == RootKcb)
                {
                    DPRINT("Found a sub key, RefCount = %u\n", CachedKcb->RefCount);

                    if (CachedKcb->RefCount > 0)
                    {
                        DPRINT("Found a sub key pointing to '%.*s', RefCount = %u\n",
                               CachedKcb->NameBlock->NameLength, CachedKcb->NameBlock->Name,
                               CachedKcb->RefCount);

                        /* If we dereference opened KCBs, don't touch read-only keys */
                        if (DereferenceOpenedEntries &&
                            !(CachedKcb->ExtFlags & CM_KCB_READ_ONLY_KEY))
                        {
                            /* Registry needs to be locked down */
                            CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK();

                            /* Flush any notifications */
                            CmpFlushNotifiesOnKeyBodyList(CachedKcb, TRUE); // Lock is already held

                            /* Clean up information we have on the subkey */
                            CmpCleanUpSubKeyInfo(CachedKcb->ParentKcb);

                            /* Get and cache the next cache entry */
                            // Entry = Entry->NextHash;
                            Entry = CachedKcb->NextHash;

                            /* Set the KCB in delete mode and remove it */
                            CachedKcb->Delete = TRUE;
                            CmpRemoveKeyControlBlock(CachedKcb);

                            /* Clear the cell */
                            CachedKcb->KeyCell = HCELL_NIL;

                            /* Restart with the next cache entry */
                            continue;
                        }
                        /* Else, the key cannot be dereferenced, and we count it as in use */

                        /* Count the current hash entry if it is in use */
                        SubKeys++;
                    }
                    else if ((CachedKcb->RefCount == 0) && RemoveEmptyCacheEntries)
                    {
                        /* Remove the current key from the delayed close list */
                        CmpRemoveFromDelayedClose(CachedKcb);

                        /* Remove the current cache entry */
                        CmpCleanUpKcbCacheWithLock(CachedKcb, TRUE);

                        /* Restart, because the hash list has changed */
                        Entry = CmpCacheTable[i].Entry;
                        continue;
                    }
                }
            }

            /* Get the next cache entry */
            Entry = Entry->NextHash;
        }
    }

    if (SubKeys > 0)
        DPRINT1("Open sub keys: %u\n", SubKeys);

    return SubKeys;
}

static
NTSTATUS
CmpDeepCopyKeyInternal(IN PHHIVE SourceHive,
                       IN HCELL_INDEX SrcKeyCell,
                       IN PHHIVE DestinationHive,
                       IN HCELL_INDEX Parent,
                       IN HSTORAGE_TYPE StorageType,
                       OUT PHCELL_INDEX DestKeyCell OPTIONAL)
{
    NTSTATUS Status;
    PCM_KEY_NODE SrcNode;
    PCM_KEY_NODE DestNode = NULL;
    HCELL_INDEX NewKeyCell = HCELL_NIL;
    HCELL_INDEX NewClassCell = HCELL_NIL, NewSecCell = HCELL_NIL;
    HCELL_INDEX SubKey, NewSubKey;
    ULONG Index, SubKeyCount;

    PAGED_CODE();

    DPRINT("CmpDeepCopyKeyInternal(0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n",
           SourceHive,
           SrcKeyCell,
           DestinationHive,
           Parent,
           StorageType,
           DestKeyCell);

    /* Get the source cell node */
    SrcNode = (PCM_KEY_NODE)HvGetCell(SourceHive, SrcKeyCell);
    ASSERT(SrcNode);

    /* Sanity check */
    ASSERT(SrcNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Create a simple copy of the source key */
    NewKeyCell = CmpCopyCell(SourceHive,
                             SrcKeyCell,
                             DestinationHive,
                             StorageType);
    if (NewKeyCell == HCELL_NIL)
    {
        /* Not enough storage space */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Get the destination cell node */
    DestNode = (PCM_KEY_NODE)HvGetCell(DestinationHive, NewKeyCell);
    ASSERT(DestNode);

    /* Set the parent and copy the flags */
    DestNode->Parent = Parent;
    DestNode->Flags  = (SrcNode->Flags & KEY_COMP_NAME); // Keep only the single permanent flag
    if (Parent == HCELL_NIL)
    {
        /* This is the new root node */
        DestNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;
    }

    /* Copy the class cell */
    if (SrcNode->ClassLength > 0)
    {
        NewClassCell = CmpCopyCell(SourceHive,
                                   SrcNode->Class,
                                   DestinationHive,
                                   StorageType);
        if (NewClassCell == HCELL_NIL)
        {
            /* Not enough storage space */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        DestNode->Class = NewClassCell;
        DestNode->ClassLength = SrcNode->ClassLength;
    }
    else
    {
        DestNode->Class = HCELL_NIL;
        DestNode->ClassLength = 0;
    }

    /* Copy the security cell (FIXME: HACKish poor-man version) */
    if (SrcNode->Security != HCELL_NIL)
    {
        NewSecCell = CmpCopyCell(SourceHive,
                                 SrcNode->Security,
                                 DestinationHive,
                                 StorageType);
        if (NewSecCell == HCELL_NIL)
        {
            /* Not enough storage space */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }
    DestNode->Security = NewSecCell;

    /* Copy the value list */
    Status = CmpCopyKeyValueList(SourceHive,
                                 &SrcNode->ValueList,
                                 DestinationHive,
                                 &DestNode->ValueList,
                                 StorageType);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    /* Clear the invalid subkey index */
    DestNode->SubKeyCounts[Stable] = DestNode->SubKeyCounts[Volatile] = 0;
    DestNode->SubKeyLists[Stable] = DestNode->SubKeyLists[Volatile] = HCELL_NIL;

    /* Calculate the total number of subkeys */
    SubKeyCount = SrcNode->SubKeyCounts[Stable] + SrcNode->SubKeyCounts[Volatile];

    /* Loop through all the subkeys */
    for (Index = 0; Index < SubKeyCount; Index++)
    {
        /* Get the subkey */
        SubKey = CmpFindSubKeyByNumber(SourceHive, SrcNode, Index);
        ASSERT(SubKey != HCELL_NIL);

        /* Call the function recursively for the subkey */
        //
        // FIXME: Danger!! Kernel stack exhaustion!!
        //
        Status = CmpDeepCopyKeyInternal(SourceHive,
                                        SubKey,
                                        DestinationHive,
                                        NewKeyCell,
                                        StorageType,
                                        &NewSubKey);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        /* Add the copy of the subkey to the new key */
        if (!CmpAddSubKey(DestinationHive,
                          NewKeyCell,
                          NewSubKey))
        {
            /* Cleanup allocated cell */
            HvFreeCell(DestinationHive, NewSubKey);

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    /* Set success */
    Status = STATUS_SUCCESS;

Cleanup:

    /* Release the cells */
    if (DestNode) HvReleaseCell(DestinationHive, NewKeyCell);
    if (SrcNode) HvReleaseCell(SourceHive, SrcKeyCell);

    /* Cleanup allocated cells in case of failure */
    if (!NT_SUCCESS(Status))
    {
        if (NewSecCell != HCELL_NIL)
            HvFreeCell(DestinationHive, NewSecCell);

        if (NewClassCell != HCELL_NIL)
            HvFreeCell(DestinationHive, NewClassCell);

        if (NewKeyCell != HCELL_NIL)
            HvFreeCell(DestinationHive, NewKeyCell);

        NewKeyCell = HCELL_NIL;
    }

    /* Set the cell index if requested and return status */
    if (DestKeyCell) *DestKeyCell = NewKeyCell;
    return Status;
}

NTSTATUS
NTAPI
CmpDeepCopyKey(IN PHHIVE SourceHive,
               IN HCELL_INDEX SrcKeyCell,
               IN PHHIVE DestinationHive,
               IN HSTORAGE_TYPE StorageType,
               OUT PHCELL_INDEX DestKeyCell OPTIONAL)
{
    /* Call the internal function */
    return CmpDeepCopyKeyInternal(SourceHive,
                                  SrcKeyCell,
                                  DestinationHive,
                                  HCELL_NIL,
                                  StorageType,
                                  DestKeyCell);
}

NTSTATUS
NTAPI
CmSaveKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
          IN HANDLE FileHandle,
          IN ULONG Flags)
{
#if DBG
    CM_CHECK_REGISTRY_STATUS CheckStatus;
    PCMHIVE HiveToValidate = NULL;
#endif
    NTSTATUS Status = STATUS_SUCCESS;
    PCMHIVE KeyHive = NULL;
    PAGED_CODE();

    DPRINT("CmSaveKey(0x%08X, 0x%08X, %lu)\n", Kcb, FileHandle, Flags);

    /* Lock the registry and KCB */
    CmpLockRegistry();
    CmpAcquireKcbLockShared(Kcb);

#if DBG
    /* Get the hive for validation */
    HiveToValidate = (PCMHIVE)Kcb->KeyHive;
#endif

    if (Kcb->Delete)
    {
        /* The source key has been deleted, do nothing */
        Status = STATUS_KEY_DELETED;
        goto Cleanup;
    }

    if (Kcb->KeyHive == &CmiVolatileHive->Hive)
    {
        /* Keys that are directly in the master hive can't be saved */
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

#if DBG
    /* Make sure this control block has a sane hive */
    CheckStatus = CmCheckRegistry(HiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
#endif

    /* Create a new hive that will hold the key */
    Status = CmpInitializeHive(&KeyHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE,
                               HFILE_TYPE_PRIMARY,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Copy the key recursively into the new hive */
    Status = CmpDeepCopyKey(Kcb->KeyHive,
                            Kcb->KeyCell,
                            &KeyHive->Hive,
                            Stable,
                            &KeyHive->Hive.BaseBlock->RootCell);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Set the primary handle of the hive */
    KeyHive->FileHandles[HFILE_TYPE_PRIMARY] = FileHandle;

    /* Dump the hive into the file */
    HvWriteHive(&KeyHive->Hive);

Cleanup:

    /* Free the hive */
    if (KeyHive) CmpDestroyHive(KeyHive);

#if DBG
    if (NT_SUCCESS(Status))
    {
        /* Before we say goodbye, make sure the hive is still OK */
        CheckStatus = CmCheckRegistry(HiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
        ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
    }
#endif

    /* Release the locks */
    CmpReleaseKcbLock(Kcb);
    CmpUnlockRegistry();

    return Status;
}

NTSTATUS
NTAPI
CmSaveMergedKeys(IN PCM_KEY_CONTROL_BLOCK HighKcb,
                 IN PCM_KEY_CONTROL_BLOCK LowKcb,
                 IN HANDLE FileHandle)
{
#if DBG
    CM_CHECK_REGISTRY_STATUS CheckStatus;
    PCMHIVE LowHiveToValidate = NULL;
    PCMHIVE HighHiveToValidate = NULL;
#endif
    PCMHIVE KeyHive = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DPRINT("CmSaveKey(%p, %p, %p)\n", HighKcb, LowKcb, FileHandle);

    /* Lock the registry and the KCBs */
    CmpLockRegistry();
    CmpAcquireKcbLockShared(HighKcb);
    CmpAcquireKcbLockShared(LowKcb);

#if DBG
    /* Get the high and low hives for validation */
    HighHiveToValidate = (PCMHIVE)HighKcb->KeyHive;
    LowHiveToValidate = (PCMHIVE)LowKcb->KeyHive;
#endif

    if (LowKcb->Delete || HighKcb->Delete)
    {
        /* The source key has been deleted, do nothing */
        Status = STATUS_KEY_DELETED;
        goto done;
    }

#if DBG
    /* Make sure that both the high and low precedence hives are OK */
    CheckStatus = CmCheckRegistry(HighHiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
    CheckStatus = CmCheckRegistry(LowHiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
#endif

    /* Create a new hive that will hold the key */
    Status = CmpInitializeHive(&KeyHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE,
                               HFILE_TYPE_PRIMARY,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Copy the low precedence key recursively into the new hive */
    Status = CmpDeepCopyKey(LowKcb->KeyHive,
                            LowKcb->KeyCell,
                            &KeyHive->Hive,
                            Stable,
                            &KeyHive->Hive.BaseBlock->RootCell);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Copy the high precedence key recursively into the new hive */
    Status = CmpDeepCopyKey(HighKcb->KeyHive,
                            HighKcb->KeyCell,
                            &KeyHive->Hive,
                            Stable,
                            &KeyHive->Hive.BaseBlock->RootCell);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the primary handle of the hive */
    KeyHive->FileHandles[HFILE_TYPE_PRIMARY] = FileHandle;

    /* Dump the hive into the file */
    HvWriteHive(&KeyHive->Hive);

done:
    /* Free the hive */
    if (KeyHive)
        CmpDestroyHive(KeyHive);

#if DBG
    if (NT_SUCCESS(Status))
    {
        /* Check those hives again before we say goodbye */
        CheckStatus = CmCheckRegistry(HighHiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
        ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
        CheckStatus = CmCheckRegistry(LowHiveToValidate, CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
        ASSERT(CM_CHECK_REGISTRY_SUCCESS(CheckStatus));
    }
#endif

    /* Release the locks */
    CmpReleaseKcbLock(LowKcb);
    CmpReleaseKcbLock(HighKcb);
    CmpUnlockRegistry();

    return Status;
}
