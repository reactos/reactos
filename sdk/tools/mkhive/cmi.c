/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/cmi.c
 * PURPOSE:         Registry file manipulation routines
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermès Bélusca-Maïto
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "mkhive.h"

/* FUNCTIONS ****************************************************************/

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    return (PVOID)malloc((size_t)Size);
}

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota)
{
    free(Ptr);
}

static BOOLEAN
NTAPI
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    OUT PVOID Buffer,
    IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
        return FALSE;

    return (fread(Buffer, 1, BufferLength, File) == BufferLength);
}

static BOOLEAN
NTAPI
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    IN PVOID Buffer,
    IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
        return FALSE;

    return (fwrite(Buffer, 1, BufferLength, File) == BufferLength);
}

static BOOLEAN
NTAPI
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize)
{
    DPRINT1("CmpFileSetSize() unimplemented\n");
    return FALSE;
}

static BOOLEAN
NTAPI
CmpFileFlush(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    return (fflush(File) == 0);
}

NTSTATUS
CmiInitializeHive(
    IN OUT PCMHIVE Hive,
    IN PCWSTR Name)
{
    NTSTATUS Status;

    RtlZeroMemory(Hive, sizeof(*Hive));

    DPRINT("Hive 0x%p\n", Hive);

    Status = HvInitialize(&Hive->Hive,
                          HINIT_CREATE,
                          HIVE_NOLAZYFLUSH,
                          HFILE_TYPE_PRIMARY,
                          0,
                          CmpAllocate,
                          CmpFree,
                          CmpFileSetSize,
                          CmpFileWrite,
                          CmpFileRead,
                          CmpFileFlush,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // HACK: See the HACK from r31253
    if (!CmCreateRootNode(&Hive->Hive, Name))
    {
        HvFree(&Hive->Hive);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Add the new hive to the hive list */
    InsertTailList(&CmiHiveListHead,
                   &Hive->HiveList);

    return STATUS_SUCCESS;
}

NTSTATUS
CmiCreateSecurityKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PUCHAR Descriptor,
    IN ULONG DescriptorLength)
{
    HCELL_INDEX SecurityCell;
    PCM_KEY_NODE Node;
    PCM_KEY_SECURITY Security;

    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    SecurityCell = HvAllocateCell(Hive,
                                  FIELD_OFFSET(CM_KEY_SECURITY, Descriptor) +
                                  DescriptorLength,
                                  Stable,
                                  HCELL_NIL);
    if (SecurityCell == HCELL_NIL)
    {
        HvReleaseCell(Hive, Cell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Node->Security = SecurityCell;
    Security = (PCM_KEY_SECURITY)HvGetCell(Hive, SecurityCell);
    Security->Signature = CM_KEY_SECURITY_SIGNATURE;
    Security->ReferenceCount = 1;
    Security->DescriptorLength = DescriptorLength;

    RtlMoveMemory(&Security->Descriptor,
                  Descriptor,
                  DescriptorLength);

    Security->Flink = Security->Blink = SecurityCell;

    HvReleaseCell(Hive, SecurityCell);
    HvReleaseCell(Hive, Cell);

    return STATUS_SUCCESS;
}

static NTSTATUS
CmiCreateSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN BOOLEAN VolatileKey,
    OUT HCELL_INDEX* pNKBOffset)
{
    HCELL_INDEX NKBOffset;
    PCM_KEY_NODE NewKeyCell;
    UNICODE_STRING KeyName;
    HSTORAGE_TYPE Storage;

    /* Skip leading path separator if present */
    if (SubKeyName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        KeyName.Buffer = &SubKeyName->Buffer[1];
        KeyName.Length = KeyName.MaximumLength = SubKeyName->Length - sizeof(WCHAR);
    }
    else
    {
        KeyName = *SubKeyName;
    }

    Storage = (VolatileKey ? Volatile : Stable);

    NKBOffset = HvAllocateCell(&RegistryHive->Hive,
                               FIELD_OFFSET(CM_KEY_NODE, Name) +
                               CmpNameSize(&RegistryHive->Hive, &KeyName),
                               Storage,
                               HCELL_NIL);
    if (NKBOffset == HCELL_NIL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewKeyCell = (PCM_KEY_NODE)HvGetCell(&RegistryHive->Hive, NKBOffset);
    if (NewKeyCell == NULL)
    {
        HvFreeCell(&RegistryHive->Hive, NKBOffset);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewKeyCell->Signature = CM_KEY_NODE_SIGNATURE;
    NewKeyCell->Flags = (VolatileKey ? KEY_IS_VOLATILE : 0);
    KeQuerySystemTime(&NewKeyCell->LastWriteTime);
    NewKeyCell->Parent = ParentKeyCellOffset;
    NewKeyCell->SubKeyCounts[Stable] = 0;
    NewKeyCell->SubKeyCounts[Volatile] = 0;
    NewKeyCell->SubKeyLists[Stable] = HCELL_NIL;
    NewKeyCell->SubKeyLists[Volatile] = HCELL_NIL;
    NewKeyCell->ValueList.Count = 0;
    NewKeyCell->ValueList.List = HCELL_NIL;
    NewKeyCell->Security = HCELL_NIL;
    NewKeyCell->Class = HCELL_NIL;
    NewKeyCell->ClassLength = 0;
    NewKeyCell->MaxNameLen = 0;
    NewKeyCell->MaxClassLen = 0;
    NewKeyCell->MaxValueNameLen = 0;
    NewKeyCell->MaxValueDataLen = 0;
    NewKeyCell->NameLength = CmpCopyName(&RegistryHive->Hive, NewKeyCell->Name, &KeyName);
    if (NewKeyCell->NameLength < KeyName.Length) NewKeyCell->Flags |= KEY_COMP_NAME;

    /* Inherit the security from the parent */
    if (ParentKeyCellOffset == HCELL_NIL)
    {
        // We are in fact creating a root key.
        // This is not handled there, but when we
        // call CmCreateRootNode instead.
        ASSERT(FALSE);
    }
    else
    {
        /* Get the parent node */
        PCM_KEY_NODE ParentKeyCell;
        ParentKeyCell = (PCM_KEY_NODE)HvGetCell(&RegistryHive->Hive, ParentKeyCellOffset);

        if (ParentKeyCell)
        {
            /* Inherit the security block of the parent */
            NewKeyCell->Security = ParentKeyCell->Security;
            if (NewKeyCell->Security != HCELL_NIL)
            {
                PCM_KEY_SECURITY Security;
                Security = (PCM_KEY_SECURITY)HvGetCell(&RegistryHive->Hive, NewKeyCell->Security);
                ++Security->ReferenceCount;
                HvReleaseCell(&RegistryHive->Hive, NewKeyCell->Security);
            }

            HvReleaseCell(&RegistryHive->Hive, ParentKeyCellOffset);
        }
    }

    HvReleaseCell(&RegistryHive->Hive, NKBOffset);

    *pNKBOffset = NKBOffset;
    return STATUS_SUCCESS;
}

NTSTATUS
CmiAddSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN BOOLEAN VolatileKey,
    OUT HCELL_INDEX *pBlockOffset)
{
    PCM_KEY_NODE ParentKeyCell;
    HCELL_INDEX NKBOffset;
    NTSTATUS Status;

    /* Create the new key */
    Status = CmiCreateSubKey(RegistryHive, ParentKeyCellOffset, SubKeyName, VolatileKey, &NKBOffset);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Mark the parent cell as dirty */
    HvMarkCellDirty(&RegistryHive->Hive, ParentKeyCellOffset, FALSE);

    if (!CmpAddSubKey(&RegistryHive->Hive, ParentKeyCellOffset, NKBOffset))
    {
        /* FIXME: delete newly created cell */
        // CmpFreeKeyByCell(&RegistryHive->Hive, NewCell /*NKBOffset*/, FALSE);
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the parent node */
    ParentKeyCell = (PCM_KEY_NODE)HvGetCell(&RegistryHive->Hive, ParentKeyCellOffset);
    if (!ParentKeyCell)
    {
        /* FIXME: delete newly created cell */
        return STATUS_UNSUCCESSFUL;
    }
    VERIFY_KEY_CELL(ParentKeyCell);

    /* Update the timestamp */
    KeQuerySystemTime(&ParentKeyCell->LastWriteTime);

    /* Check if we need to update name maximum, update it if so */
    if (ParentKeyCell->MaxNameLen < SubKeyName->Length)
        ParentKeyCell->MaxNameLen = SubKeyName->Length;

    /* Release the cell */
    HvReleaseCell(&RegistryHive->Hive, ParentKeyCellOffset);

    *pBlockOffset = NKBOffset;
    return STATUS_SUCCESS;
}

NTSTATUS
CmiAddValueKey(
    IN PCMHIVE RegistryHive,
    IN PCM_KEY_NODE Parent,
    IN ULONG ChildIndex,
    IN PCUNICODE_STRING ValueName,
    OUT PCM_KEY_VALUE *pValueCell,
    OUT HCELL_INDEX *pValueCellOffset)
{
    NTSTATUS Status;
    HSTORAGE_TYPE Storage;
    PCM_KEY_VALUE NewValueCell;
    HCELL_INDEX NewValueCellOffset;

    Storage = (Parent->Flags & KEY_IS_VOLATILE) ? Volatile : Stable;

    NewValueCellOffset = HvAllocateCell(&RegistryHive->Hive,
                               FIELD_OFFSET(CM_KEY_VALUE, Name) +
                               CmpNameSize(&RegistryHive->Hive, (PUNICODE_STRING)ValueName),
                               Storage,
                               HCELL_NIL);
    if (NewValueCellOffset == HCELL_NIL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewValueCell = (PCM_KEY_VALUE)HvGetCell(&RegistryHive->Hive, NewValueCellOffset);
    if (NewValueCell == NULL)
    {
        HvFreeCell(&RegistryHive->Hive, NewValueCellOffset);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewValueCell->Signature = CM_KEY_VALUE_SIGNATURE;
    NewValueCell->NameLength = CmpCopyName(&RegistryHive->Hive,
                                           NewValueCell->Name,
                                           (PUNICODE_STRING)ValueName);

    /* Check for compressed name */
    if (NewValueCell->NameLength < ValueName->Length)
    {
        /* This is a compressed name */
        NewValueCell->Flags = VALUE_COMP_NAME;
    }
    else
    {
        /* No flags to set */
        NewValueCell->Flags = 0;
    }

    NewValueCell->Type = 0;
    NewValueCell->DataLength = 0;
    NewValueCell->Data = HCELL_NIL;

    HvMarkCellDirty(&RegistryHive->Hive, NewValueCellOffset, FALSE);

    /* Check if we already have a value list */
    if (Parent->ValueList.Count)
    {
        /* Then make sure it's valid and dirty it */
        ASSERT(Parent->ValueList.List != HCELL_NIL);
        HvMarkCellDirty(&RegistryHive->Hive, Parent->ValueList.List, FALSE);
    }

    /* Add this value cell to the child list */
    Status = CmpAddValueToList(&RegistryHive->Hive,
                               NewValueCellOffset,
                               ChildIndex,
                               Storage,
                               &Parent->ValueList);

    /* If we failed, free the entire cell, including the data */
    if (!NT_SUCCESS(Status))
    {
        /* Overwrite the status with a known one */
        CmpFreeValue(&RegistryHive->Hive, NewValueCellOffset);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        *pValueCell = NewValueCell;
        *pValueCellOffset = NewValueCellOffset;
        Status = STATUS_SUCCESS;
    }

    return Status;
}
