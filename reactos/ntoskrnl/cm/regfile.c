/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/regfile.c
 * PURPOSE:         Registry file manipulation routines
 *
 * PROGRAMMERS:     Casper Hornstrup
 *                  Eric Kohl
 *                  Filip Navara
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"
#include "../config/cm.h"

/* LOCAL MACROS *************************************************************/

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

/* FUNCTIONS ****************************************************************/

NTSTATUS
CmiLoadHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN CONST UNICODE_STRING* FileName,
            IN ULONG Flags)
{
    PEREGISTRY_HIVE Hive = NULL;
    NTSTATUS Status;
    BOOLEAN Allocate = TRUE;

    DPRINT ("CmiLoadHive(Filename %wZ)\n", FileName);

    if (Flags & ~REG_NO_LAZY_FLUSH) return STATUS_INVALID_PARAMETER;

    Status = CmpInitHiveFromFile(FileName,
                                 (Flags & REG_NO_LAZY_FLUSH) ? HIVE_NO_SYNCH : 0,
                                 &Hive,
                                 &Allocate,
                                 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpInitHiveFromFile() failed (Status %lx)\n", Status);
        if (Hive) ExFreePool(Hive);
        return Status;
    }

    Status = CmiConnectHive(KeyObjectAttributes, Hive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("CmiConnectHive() failed (Status %lx)\n", Status);
        //      CmiRemoveRegistryHive (Hive);
    }

    DPRINT ("CmiLoadHive() done\n");
    return Status;
}

VOID
CmCloseHiveFiles(PEREGISTRY_HIVE RegistryHive)
{
    ZwClose(RegistryHive->HiveHandle);
    ZwClose(RegistryHive->LogHandle);
}

NTSTATUS
CmiFlushRegistryHive(PEREGISTRY_HIVE RegistryHive)
{
    BOOLEAN Success;
    NTSTATUS Status;
    ULONG Disposition;

    ASSERT(!IsNoFileHive(RegistryHive));

    if (RtlFindSetBits(&RegistryHive->Hive.DirtyVector, 1, 0) == ~0)
    {
        return(STATUS_SUCCESS);
    }

    Status = CmpOpenHiveFiles(&RegistryHive->HiveFileName,
                              L".LOG",
                              &RegistryHive->HiveHandle,
                              &RegistryHive->LogHandle,
                              &Disposition,
                              &Disposition,
                              FALSE,
                              FALSE,
                              TRUE,
                              NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Success = HvSyncHive(&RegistryHive->Hive);

    CmCloseHiveFiles(RegistryHive);

    return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

ULONG
CmiGetMaxNameLength(PHHIVE Hive,
                    PCM_KEY_NODE KeyCell)
{
    PHASH_TABLE_CELL HashBlock;
    PCM_KEY_NODE CurSubKeyCell;
    ULONG MaxName;
    ULONG NameSize;
    ULONG i;
    ULONG Storage;

    MaxName = 0;
    for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
        if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
            HashBlock = HvGetCell (Hive, KeyCell->SubKeyLists[Storage]);
            ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

            for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
                CurSubKeyCell = HvGetCell (Hive,
                                           HashBlock->Table[i].KeyOffset);
                NameSize = CurSubKeyCell->NameSize;
                if (CurSubKeyCell->Flags & REG_KEY_NAME_PACKED)
                    NameSize *= sizeof(WCHAR);
                if (NameSize > MaxName)
                    MaxName = NameSize;
            }
        }
    }

    DPRINT ("MaxName %lu\n", MaxName);

    return MaxName;
}

ULONG
CmiGetMaxClassLength(PHHIVE Hive,
                     PCM_KEY_NODE KeyCell)
{
    PHASH_TABLE_CELL HashBlock;
    PCM_KEY_NODE CurSubKeyCell;
    ULONG MaxClass;
    ULONG i;
    ULONG Storage;

    MaxClass = 0;
    for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
        if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
            HashBlock = HvGetCell (Hive,
                                   KeyCell->SubKeyLists[Storage]);
            ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

            for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
                CurSubKeyCell = HvGetCell (Hive,
                                           HashBlock->Table[i].KeyOffset);

                if (MaxClass < CurSubKeyCell->ClassSize)
                {
                    MaxClass = CurSubKeyCell->ClassSize;
                }
            }
        }
    }

    return MaxClass;
}

ULONG
CmiGetMaxValueNameLength(PHHIVE Hive,
                         PCM_KEY_NODE KeyCell)
{
    PVALUE_LIST_CELL ValueListCell;
    PCM_KEY_VALUE CurValueCell;
    ULONG MaxValueName;
    ULONG Size;
    ULONG i;

    VERIFY_KEY_CELL(KeyCell);

    if (KeyCell->ValueList.List == HCELL_NULL)
    {
        return 0;
    }

    MaxValueName = 0;
    ValueListCell = HvGetCell (Hive,
                               KeyCell->ValueList.List);

    for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
        CurValueCell = HvGetCell (Hive,
                                  ValueListCell->ValueOffset[i]);
        if (CurValueCell == NULL)
        {
            DPRINT("CmiGetBlock() failed\n");
        }

        if (CurValueCell != NULL)
        {
            Size = CurValueCell->NameSize;
            if (CurValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
                Size *= sizeof(WCHAR);
            }
            if (MaxValueName < Size)
            {
                MaxValueName = Size;
            }
        }
    }

    return MaxValueName;
}

ULONG
CmiGetMaxValueDataLength(PHHIVE Hive,
                         PCM_KEY_NODE KeyCell)
{
    PVALUE_LIST_CELL ValueListCell;
    PCM_KEY_VALUE CurValueCell;
    LONG MaxValueData;
    ULONG i;

    VERIFY_KEY_CELL(KeyCell);

    if (KeyCell->ValueList.List == HCELL_NULL)
    {
        return 0;
    }

    MaxValueData = 0;
    ValueListCell = HvGetCell (Hive, KeyCell->ValueList.List);

    for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
        CurValueCell = HvGetCell (Hive,
                                  ValueListCell->ValueOffset[i]);
        if ((MaxValueData < (LONG)(CurValueCell->DataSize & REG_DATA_SIZE_MASK)))
        {
            MaxValueData = CurValueCell->DataSize & REG_DATA_SIZE_MASK;
        }
    }

    return MaxValueData;
}

NTSTATUS
CmiScanKeyForValue(IN PEREGISTRY_HIVE RegistryHive,
                   IN PCM_KEY_NODE KeyCell,
                   IN PUNICODE_STRING ValueName,
                   OUT PCM_KEY_VALUE *ValueCell,
                   OUT HCELL_INDEX *ValueCellOffset)
{
    HCELL_INDEX CellIndex;

    /* Assume failure */
    *ValueCell = NULL;
    if (ValueCellOffset) *ValueCellOffset = HCELL_NIL;

    /* Call newer Cm API */
    CellIndex = CmpFindValueByName(&RegistryHive->Hive, KeyCell, ValueName);
    if (CellIndex == HCELL_NIL) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Otherwise, get the cell data back too */
    if (ValueCellOffset) *ValueCellOffset = CellIndex;
    *ValueCell =  HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

NTSTATUS
CmiScanForSubKey(IN PEREGISTRY_HIVE RegistryHive,
                 IN PCM_KEY_NODE KeyCell,
                 OUT PCM_KEY_NODE *SubKeyCell,
                 OUT HCELL_INDEX *BlockOffset,
                 IN CONST UNICODE_STRING* KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 IN ULONG Attributes)
{
    HCELL_INDEX CellIndex;

    /* Assume failure */
    *SubKeyCell = NULL;

    /* Call newer Cm API */
    CellIndex = CmpFindSubKeyByName(&RegistryHive->Hive, KeyCell, KeyName);
    if (CellIndex == HCELL_NIL) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Otherwise, get the cell data back too */
    *BlockOffset = CellIndex;
    *SubKeyCell = HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

NTSTATUS
CmiAddSubKey(PEREGISTRY_HIVE RegistryHive,
             PKEY_OBJECT ParentKey,
             PKEY_OBJECT SubKey,
             PUNICODE_STRING SubKeyName,
             ULONG TitleIndex,
             PUNICODE_STRING Class,
             ULONG CreateOptions)
{
    PHASH_TABLE_CELL HashBlock;
    HCELL_INDEX NKBOffset;
    PCM_KEY_NODE NewKeyCell;
    ULONG NewBlockSize;
    PCM_KEY_NODE ParentKeyCell;
    PVOID ClassCell;
    NTSTATUS Status;
    USHORT NameSize;
    PWSTR NamePtr;
    BOOLEAN Packable;
    HV_STORAGE_TYPE Storage;
    ULONG i;

    ParentKeyCell = ParentKey->KeyCell;

    VERIFY_KEY_CELL(ParentKeyCell);

    /* Skip leading backslash */
    if (SubKeyName->Buffer[0] == L'\\')
    {
        NamePtr = &SubKeyName->Buffer[1];
        NameSize = SubKeyName->Length - sizeof(WCHAR);
    }
    else
    {
        NamePtr = SubKeyName->Buffer;
        NameSize = SubKeyName->Length;
    }

    /* Check whether key name can be packed */
    Packable = TRUE;
    for (i = 0; i < NameSize / sizeof(WCHAR); i++)
    {
        if (NamePtr[i] & 0xFF00)
        {
            Packable = FALSE;
            break;
        }
    }

    /* Adjust name size */
    if (Packable)
    {
        NameSize = NameSize / sizeof(WCHAR);
    }

    DPRINT("Key %S  Length %lu  %s\n", NamePtr, NameSize, (Packable)?"True":"False");

    Status = STATUS_SUCCESS;

    Storage = (CreateOptions & REG_OPTION_VOLATILE) ? HvVolatile : HvStable;
    NewBlockSize = sizeof(CM_KEY_NODE) + NameSize;
    NKBOffset = HvAllocateCell (&RegistryHive->Hive, NewBlockSize, Storage);
    if (NKBOffset == HCELL_NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NewKeyCell = HvGetCell (&RegistryHive->Hive, NKBOffset);
        NewKeyCell->Id = REG_KEY_CELL_ID;
        if (CreateOptions & REG_OPTION_VOLATILE)
        {
            NewKeyCell->Flags = REG_KEY_VOLATILE_CELL;
        }
        else
        {
            NewKeyCell->Flags = 0;
        }
        KeQuerySystemTime(&NewKeyCell->LastWriteTime);
        NewKeyCell->Parent = HCELL_NULL;
        NewKeyCell->SubKeyCounts[HvStable] = 0;
        NewKeyCell->SubKeyCounts[HvVolatile] = 0;
        NewKeyCell->SubKeyLists[HvStable] = HCELL_NULL;
        NewKeyCell->SubKeyLists[HvVolatile] = HCELL_NULL;
        NewKeyCell->ValueList.Count = 0;
        NewKeyCell->ValueList.List = HCELL_NULL;
        NewKeyCell->SecurityKeyOffset = HCELL_NULL;
        NewKeyCell->ClassNameOffset = HCELL_NULL;

        /* Pack the key name */
        NewKeyCell->NameSize = NameSize;
        if (Packable)
        {
            NewKeyCell->Flags |= REG_KEY_NAME_PACKED;
            for (i = 0; i < NameSize; i++)
            {
                NewKeyCell->Name[i] = (CHAR)(NamePtr[i] & 0x00FF);
            }
        }
        else
        {
            RtlCopyMemory(NewKeyCell->Name,
                          NamePtr,
                          NameSize);
        }

        VERIFY_KEY_CELL(NewKeyCell);

        if (Class != NULL && Class->Length)
        {
            NewKeyCell->ClassSize = Class->Length;
            NewKeyCell->ClassNameOffset = HvAllocateCell(
                &RegistryHive->Hive, NewKeyCell->ClassSize, HvStable);
            ASSERT(NewKeyCell->ClassNameOffset != HCELL_NULL); /* FIXME */

            ClassCell = HvGetCell(&RegistryHive->Hive, NewKeyCell->ClassNameOffset);
            RtlCopyMemory (ClassCell,
                           Class->Buffer,
                           Class->Length);
        }
    }

    if (!NT_SUCCESS(Status))
    {
      return Status;
    }

    SubKey->KeyCell = NewKeyCell;
    SubKey->KeyCellOffset = NKBOffset;

    if (ParentKeyCell->SubKeyLists[Storage] == HCELL_NULL)
    {
        Status = CmiAllocateHashTableCell (RegistryHive,
                                           &HashBlock,
                                           &ParentKeyCell->SubKeyLists[Storage],
                                           REG_INIT_HASH_TABLE_SIZE,
                                           Storage);
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
    }
    else
    {
        HashBlock = HvGetCell (&RegistryHive->Hive,
                               ParentKeyCell->SubKeyLists[Storage]);
        ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

        if (((ParentKeyCell->SubKeyCounts[Storage] + 1) >= HashBlock->HashTableSize))
        {
            PHASH_TABLE_CELL NewHashBlock;
            HCELL_INDEX HTOffset;

            /* Reallocate the hash table cell */
            Status = CmiAllocateHashTableCell (RegistryHive,
                                               &NewHashBlock,
                                               &HTOffset,
                                               HashBlock->HashTableSize +
                                               REG_EXTEND_HASH_TABLE_SIZE,
                                               Storage);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            RtlZeroMemory(&NewHashBlock->Table[0],
                sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
            RtlCopyMemory(&NewHashBlock->Table[0],
                &HashBlock->Table[0],
                sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
            HvFreeCell (&RegistryHive->Hive, ParentKeyCell->SubKeyLists[Storage]);
                ParentKeyCell->SubKeyLists[Storage] = HTOffset;
            HashBlock = NewHashBlock;
        }
    }

    Status = CmiAddKeyToHashTable(RegistryHive,
                                  HashBlock,
                                  ParentKeyCell,
                                  Storage,
                                  NewKeyCell,
                                  NKBOffset);
    if (NT_SUCCESS(Status))
    {
        ParentKeyCell->SubKeyCounts[Storage]++;
    }

    KeQuerySystemTime (&ParentKeyCell->LastWriteTime);
    HvMarkCellDirty (&RegistryHive->Hive, ParentKey->KeyCellOffset);

    return(Status);
}

NTSTATUS
CmiAllocateHashTableCell (IN PEREGISTRY_HIVE RegistryHive,
                          OUT PHASH_TABLE_CELL *HashBlock,
                          OUT HCELL_INDEX *HBOffset,
                          IN ULONG SubKeyCount,
                          IN HV_STORAGE_TYPE Storage)
{
    PHASH_TABLE_CELL NewHashBlock;
    ULONG NewHashSize;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;
    *HashBlock = NULL;
    NewHashSize = sizeof(HASH_TABLE_CELL) +
        (SubKeyCount * sizeof(HASH_RECORD));
    *HBOffset = HvAllocateCell (&RegistryHive->Hive, NewHashSize, Storage);

    if (*HBOffset == HCELL_NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        ASSERT(SubKeyCount <= 0xffff); /* should really be USHORT_MAX or similar */
        NewHashBlock = HvGetCell (&RegistryHive->Hive, *HBOffset);
        NewHashBlock->Id = REG_HASH_TABLE_CELL_ID;
        NewHashBlock->HashTableSize = (USHORT)SubKeyCount;
        *HashBlock = NewHashBlock;
    }

    return Status;
}

NTSTATUS
CmiAddKeyToHashTable(PEREGISTRY_HIVE RegistryHive,
                     PHASH_TABLE_CELL HashCell,
                     PCM_KEY_NODE KeyCell,
                     HV_STORAGE_TYPE StorageType,
                     PCM_KEY_NODE NewKeyCell,
                     HCELL_INDEX NKBOffset)
{
    ULONG i = KeyCell->SubKeyCounts[StorageType];

    HashCell->Table[i].KeyOffset = NKBOffset;
    HashCell->Table[i].HashValue = 0;
    if (NewKeyCell->Flags & REG_KEY_NAME_PACKED)
    {
        RtlCopyMemory(&HashCell->Table[i].HashValue,
                      NewKeyCell->Name,
                      min(NewKeyCell->NameSize, sizeof(ULONG)));
    }
    HvMarkCellDirty(&RegistryHive->Hive, KeyCell->SubKeyLists[StorageType]);
    return STATUS_SUCCESS;
}

/* EOF */
