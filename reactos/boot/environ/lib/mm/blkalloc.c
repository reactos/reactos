/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/blkalloc.c
 * PURPOSE:         Boot Library Memory Manager Block Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID* MmBlockAllocatorTable;
ULONG MmBlockAllocatorTableEntries;
BOOLEAN MmBlockAllocatorInitialized;

typedef struct _BL_BLOCK_DESCRIPTOR
{
    LIST_ENTRY ListHead;
    ULONG Unknown;
    BL_MEMORY_TYPE Type;
    ULONG Attributes;
    ULONG Unknown2;
    ULONG Count;
    ULONG Count2;
    ULONG Size;
    ULONG BlockId;
    ULONG ReferenceCount;
} BL_BLOCK_DESCRIPTOR, *PBL_BLOCK_DESCRIPTOR;

typedef struct _BL_BLOCK_ENTRY
{
    LIST_ENTRY ListEntry;
    ULONG Todo;
} BL_BLOCK_ENTRY, *PBL_BLOCK_ENTRY;

/* FUNCTIONS *****************************************************************/

BOOLEAN
MmBapCompareBlockAllocatorTableEntry (
    _In_ PVOID Entry,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PVOID Argument3,
    _In_ PVOID Argument4
    )
{
    PBL_BLOCK_DESCRIPTOR BlockInfo = (PBL_BLOCK_DESCRIPTOR)Entry;
    ULONG BlockId = (ULONG)Argument1;

    /* Check if the block ID matches */
    return BlockInfo->BlockId == BlockId;
}

PBL_BLOCK_DESCRIPTOR
MmBapFindBlockInformation (
    ULONG BlockId
    )
{
    ULONG EntryId;

    /* Find the block that matches */
    EntryId = BlockId;
    return BlTblFindEntry(MmBlockAllocatorTable,
                          MmBlockAllocatorTableEntries,
                          &EntryId,
                          MmBapCompareBlockAllocatorTableEntry,
                          (PVOID)EntryId,
                          NULL,
                          NULL,
                          NULL);
}

NTSTATUS
MmBapFreeBlockAllocatorDescriptor (
    _In_ PBL_BLOCK_DESCRIPTOR BlockInfo,
    _In_ PBL_BLOCK_ENTRY BlockEntry
    )
{
    /* @TODO FIXME: Later */
    EfiPrintf(L"Block free not yet implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlpMmDeleteBlockAllocator (
    _In_ ULONG BlockId
    )
{
    NTSTATUS Status, LocalStatus;
    PBL_BLOCK_DESCRIPTOR BlockInfo;
    PLIST_ENTRY ListHead, NextEntry;
    PBL_BLOCK_ENTRY BlockEntry;

    /* Nothing to delete if we're not initialized */
    if (!MmBlockAllocatorInitialized)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Find the block descriptor */
    BlockInfo = MmBapFindBlockInformation(BlockId);
    if (BlockInfo)
    {
        /* Assume success for now */
        Status = STATUS_SUCCESS;

        /* Do we have at least one reference? */
        if (BlockInfo->ReferenceCount)
        {
            /* Iterate over the allocated blocks */
            ListHead = &BlockInfo->ListHead;
            NextEntry = ListHead->Flink;
            while (NextEntry != ListHead)
            {
                /* Free each one */
                BlockEntry = CONTAINING_RECORD(NextEntry,
                                               BL_BLOCK_ENTRY,
                                               ListEntry);
                LocalStatus = MmBapFreeBlockAllocatorDescriptor(BlockInfo,
                                                                BlockEntry);
                if (!NT_SUCCESS(LocalStatus))
                {
                    /* Remember status on failure only */
                    Status = LocalStatus;
                }
            }

            /* Drop a reference */
            BlockInfo->ReferenceCount--;
        }
        else
        {
            /* There aren't any references, so why are we being called? */
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* No block exists with this ID */
        Status = STATUS_UNSUCCESSFUL;
    }

    /* Return back */
    return Status;
}

NTSTATUS
MmBapFreeBlockAllocatorTableEntry (
    _In_ PVOID Entry,
    _In_ ULONG Index
    )
{
    PBL_BLOCK_DESCRIPTOR BlockInfo = (PBL_BLOCK_DESCRIPTOR)Entry;
    NTSTATUS Status, LocalStatus;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Check if there was at least one reference */
    if (BlockInfo->ReferenceCount > 1)
    {
        /* Try to delete the allocator */
        LocalStatus = BlpMmDeleteBlockAllocator(BlockInfo->BlockId);
        if (!NT_SUCCESS(LocalStatus))
        {
            /* Remember status on failure only */
            Status = LocalStatus;
        }
    }

    /* Now destroy the allocator's descriptor */
    LocalStatus = BlMmFreeHeap(BlockInfo);
    if (!NT_SUCCESS(LocalStatus))
    {
        /* Remember status on failure only */
        Status = LocalStatus;
    }

    /* Free the entry, and return failure, if any */
    MmBlockAllocatorTable[Index] = NULL;
    return Status;
}

NTSTATUS
MmBapPurgeBlockAlloctorTableEntry (
    _In_ PVOID Entry
    )
{
    PBL_BLOCK_DESCRIPTOR BlockInfo = (PBL_BLOCK_DESCRIPTOR)Entry;
    NTSTATUS Status;

    /* Check if there's a reference on the block descriptor */
    if (BlockInfo->ReferenceCount)
    {
        /* Don't allow purging */
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Free the entry */
        Status = MmBapFreeBlockAllocatorTableEntry(BlockInfo,
                                                   BlockInfo->BlockId);
    }

    /* Return purge status */
    return Status;
}

NTSTATUS
BlpMmCreateBlockAllocator (
    VOID
    )
{
    PBL_BLOCK_DESCRIPTOR BlockInfo;
    ULONG BlockId;
    NTSTATUS Status;

    /* If the block allocator isn't initialized, bail out */
    BlockId = -1;
    if (!MmBlockAllocatorInitialized)
    {
        goto Quickie;
    }

    /* Allocate a block descriptor and zero it out */
    BlockInfo = BlMmAllocateHeap(sizeof(*BlockInfo));
    if (!BlockInfo)
    {
        goto Quickie;
    }
    RtlZeroMemory(BlockInfo, sizeof(*BlockInfo));

    /* Setup the block descriptor */
    BlockInfo->Attributes = 0;
    BlockInfo->Type = BlLoaderBlockMemory;
    BlockInfo->Unknown = 1;
    BlockInfo->Unknown2 = 1;
    BlockInfo->Size = PAGE_SIZE;
    BlockInfo->Count = 128;
    BlockInfo->Count2 = 128;
    InitializeListHead(&BlockInfo->ListHead);

    /* Add it to the list of block descriptors */
    Status = BlTblSetEntry(&MmBlockAllocatorTable,
                           &MmBlockAllocatorTableEntries,
                           BlockInfo,
                           &BlockId,
                           MmBapPurgeBlockAlloctorTableEntry);
    if (NT_SUCCESS(Status))
    {
        /* Add the initial reference and store the block ID */
        BlockInfo->ReferenceCount = 1;
        BlockInfo->BlockId = BlockId;
    }

Quickie:
    /* On failure, free the block descriptor */
    if (BlockId == -1)
    {
        BlMmFreeHeap(BlockInfo);
    }

    /* Return the block descriptor ID, or -1 on failure */
    return BlockId;
}

NTSTATUS
MmBaInitialize (
    VOID
    )
{
    NTSTATUS Status;
    ULONG Size;

    /* Allocate 8 table entries */
    MmBlockAllocatorTableEntries = 8;
    Size = sizeof(BL_BLOCK_DESCRIPTOR) * MmBlockAllocatorTableEntries;
    MmBlockAllocatorTable = BlMmAllocateHeap(Size);
    if (MmBlockAllocatorTable)
    {
        /* Zero them out -- we're all done */
        Status = STATUS_SUCCESS;
        RtlZeroMemory(MmBlockAllocatorTable, Size);
        MmBlockAllocatorInitialized = 1;
    }
    else
    {
        /* Bail out since we're out of memory */
        Status = STATUS_NO_MEMORY;
        MmBlockAllocatorInitialized = 0;
    }

    /* Return initialization status */
    return Status;
}
