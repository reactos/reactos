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

/* LOCAL MACROS *************************************************************/

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

/* FUNCTIONS ****************************************************************/

NTSTATUS
CmiLoadHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN PCUNICODE_STRING FileName,
            IN ULONG Flags)
{
    PCMHIVE Hive = NULL;
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
CmCloseHiveFiles(PCMHIVE RegistryHive)
{
    ZwClose(RegistryHive->FileHandles[HFILE_TYPE_PRIMARY]);
    ZwClose(RegistryHive->FileHandles[HFILE_TYPE_LOG]);
}

NTSTATUS
CmiFlushRegistryHive(PCMHIVE RegistryHive)
{
    BOOLEAN Success;
    NTSTATUS Status;
    ULONG Disposition;

    ASSERT(!IsNoFileHive(RegistryHive));

    if (RtlFindSetBits(&RegistryHive->Hive.DirtyVector, 1, 0) == ~0)
    {
        return(STATUS_SUCCESS);
    }

    Status = CmpOpenHiveFiles(&RegistryHive->FileFullPath,
                              L".LOG",
                              &RegistryHive->FileHandles[HFILE_TYPE_PRIMARY],
                              &RegistryHive->FileHandles[HFILE_TYPE_LOG],
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
    PCM_KEY_FAST_INDEX HashBlock;
    PCM_KEY_NODE CurSubKeyCell;
    ULONG MaxName;
    ULONG NameLength;
    ULONG i;
    ULONG Storage;

    MaxName = 0;
    for (Storage = Stable; Storage < HTYPE_COUNT; Storage++)
    {
        if (KeyCell->SubKeyLists[Storage] != HCELL_NIL)
        {
            HashBlock = (PCM_KEY_FAST_INDEX)HvGetCell (Hive, KeyCell->SubKeyLists[Storage]);
            ASSERT(HashBlock->Signature == CM_KEY_FAST_LEAF);

            for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
                CurSubKeyCell = (PCM_KEY_NODE)HvGetCell (Hive,
                                           HashBlock->List[i].Cell);
                NameLength = CurSubKeyCell->NameLength;
                if (CurSubKeyCell->Flags & KEY_COMP_NAME)
                    NameLength *= sizeof(WCHAR);
                if (NameLength > MaxName)
                    MaxName = NameLength;
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
    PCM_KEY_FAST_INDEX HashBlock;
    PCM_KEY_NODE CurSubKeyCell;
    ULONG MaxClass;
    ULONG i;
    ULONG Storage;

    MaxClass = 0;
    for (Storage = Stable; Storage < HTYPE_COUNT; Storage++)
    {
        if (KeyCell->SubKeyLists[Storage] != HCELL_NIL)
        {
            HashBlock = (PCM_KEY_FAST_INDEX)HvGetCell (Hive,
                                   KeyCell->SubKeyLists[Storage]);
            ASSERT(HashBlock->Signature == CM_KEY_FAST_LEAF);

            for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
                CurSubKeyCell = (PCM_KEY_NODE)HvGetCell (Hive,
                                           HashBlock->List[i].Cell);

                if (MaxClass < CurSubKeyCell->ClassLength)
                {
                    MaxClass = CurSubKeyCell->ClassLength;
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

    if (KeyCell->ValueList.List == HCELL_NIL)
    {
        return 0;
    }

    MaxValueName = 0;
    ValueListCell = (PVALUE_LIST_CELL)HvGetCell (Hive,
                               KeyCell->ValueList.List);

    for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
        CurValueCell = (PCM_KEY_VALUE)HvGetCell (Hive,
                                  ValueListCell->ValueOffset[i]);
        if (CurValueCell == NULL)
        {
            DPRINT("CmiGetBlock() failed\n");
        }

        if (CurValueCell != NULL)
        {
            Size = CurValueCell->NameLength;
            if (CurValueCell->Flags & VALUE_COMP_NAME)
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

    if (KeyCell->ValueList.List == HCELL_NIL)
    {
        return 0;
    }

    MaxValueData = 0;
    ValueListCell = (PVALUE_LIST_CELL)HvGetCell (Hive, KeyCell->ValueList.List);

    for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
        CurValueCell = (PCM_KEY_VALUE)HvGetCell (Hive,
                                  ValueListCell->ValueOffset[i]);
        if ((MaxValueData < (LONG)(CurValueCell->DataLength & REG_DATA_SIZE_MASK)))
        {
            MaxValueData = CurValueCell->DataLength & REG_DATA_SIZE_MASK;
        }
    }

    return MaxValueData;
}

NTSTATUS
CmiScanKeyForValue(IN PCMHIVE RegistryHive,
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
    *ValueCell = (PCM_KEY_VALUE)HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

NTSTATUS
CmiScanForSubKey(IN PCMHIVE RegistryHive,
                 IN PCM_KEY_NODE KeyCell,
                 OUT PCM_KEY_NODE *SubKeyCell,
                 OUT HCELL_INDEX *BlockOffset,
                 IN PCUNICODE_STRING KeyName,
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
    *SubKeyCell = (PCM_KEY_NODE)HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

/* EOF */
