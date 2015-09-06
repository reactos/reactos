/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/pagealloc.c
 * PURPOSE:         Boot Library Memory Manager Page Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

ULONGLONG PapMaximumPhysicalPage, PapMinimumPhysicalPage;

ULONG PapMinimumAllocationCount;

BOOLEAN PapInitializationStatus;

BL_MEMORY_DESCRIPTOR_LIST MmMdlMappedAllocated;
BL_MEMORY_DESCRIPTOR_LIST MmMdlMappedUnallocated;
BL_MEMORY_DESCRIPTOR_LIST MmMdlFwAllocationTracker;
BL_MEMORY_DESCRIPTOR_LIST MmMdlUnmappedAllocated;
BL_MEMORY_DESCRIPTOR_LIST MmMdlUnmappedUnallocated;
BL_MEMORY_DESCRIPTOR_LIST MmMdlReservedAllocated;
BL_MEMORY_DESCRIPTOR_LIST MmMdlBadMemory;
BL_MEMORY_DESCRIPTOR_LIST MmMdlTruncatedMemory;
BL_MEMORY_DESCRIPTOR_LIST MmMdlPersistentMemory;
BL_MEMORY_DESCRIPTOR_LIST MmMdlCompleteBadMemory;
BL_MEMORY_DESCRIPTOR_LIST MmMdlFreeVirtual;
BL_MEMORY_DESCRIPTOR_LIST MmMdlMappingTrackers;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BlpMmInitializeConstraints (
    VOID
    )
{
    /* FIXME: Read BCD option 'avoidlowmemory' and 'truncatememory' */
    return STATUS_SUCCESS;
}

NTSTATUS
MmPaInitialize (
    __in PBL_MEMORY_DATA BootMemoryData,
    __in ULONG MinimumAllocationCount
    )
{
    NTSTATUS Status;
    ULONG ExistingDescriptors, FinalOffset;
    PBL_MEMORY_DESCRIPTOR Descriptor;

    /* Initialize physical allocator variables */
    PapMaximumPhysicalPage = 0xFFFFFFFFFFFFF;
    PapMinimumAllocationCount = MinimumAllocationCount;
    PapMinimumPhysicalPage = 0;

    /* Initialize all the lists */
    MmMdInitializeListHead(&MmMdlMappedAllocated);
    MmMdInitializeListHead(&MmMdlMappedUnallocated);
    MmMdInitializeListHead(&MmMdlFwAllocationTracker);
    MmMdInitializeListHead(&MmMdlUnmappedAllocated);
    MmMdInitializeListHead(&MmMdlReservedAllocated);
    MmMdInitializeListHead(&MmMdlBadMemory);
    MmMdInitializeListHead(&MmMdlTruncatedMemory);
    MmMdInitializeListHead(&MmMdlPersistentMemory);
    MmMdInitializeListHead(&MmMdlUnmappedUnallocated);
    MmMdInitializeListHead(&MmMdlCompleteBadMemory);

    /* Get the BIOS memory map */
    Status = MmFwGetMemoryMap(&MmMdlUnmappedUnallocated,
                              BL_MM_FLAG_USE_FIRMWARE_FOR_MEMORY_MAP_BUFFERS |
                              BL_MM_FLAG_REQUEST_COALESCING);
    if (NT_SUCCESS(Status))
    {
        PLIST_ENTRY listHead, nextEntry;

        /* Loop the NT firmware memory list */
        EarlyPrint(L"NT MEMORY MAP\n\n");
        listHead = &MmMdlUnmappedUnallocated.ListHead;
        nextEntry = listHead->Flink;
        while (listHead != nextEntry)
        {
            Descriptor = CONTAINING_RECORD(nextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

            EarlyPrint(L"Type: %lX Flags: %lX Start: 0x%I64X End: 0x%I64X\n",
                       Descriptor->Type,
                       Descriptor->Flags,
                       Descriptor->BasePage << PAGE_SHIFT,
                       (Descriptor->BasePage + Descriptor->PageCount) << PAGE_SHIFT);

            nextEntry = nextEntry->Flink;
        }

        /*
         * Because BL supports cross x86-x64 application launches and a LIST_ENTRY
         * is of variable size, care must be taken here to ensure that we see a
         * consistent view of descriptors. BL uses some offset magic to figure out
         * where the data actually starts, since everything is ULONGLONG past the
         * LIST_ENTRY itself
         */
        FinalOffset = BootMemoryData->MdListOffset + BootMemoryData->DescriptorOffset;
        Descriptor = (PBL_MEMORY_DESCRIPTOR)((ULONG_PTR)BootMemoryData + FinalOffset -
                                             FIELD_OFFSET(BL_MEMORY_DESCRIPTOR, BasePage));

        /* Scan all of them */
        ExistingDescriptors = BootMemoryData->DescriptorCount;
        while (ExistingDescriptors != 0)
        {
            EarlyPrint(L"Existing migration of memory not supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        /* We are done, so check for any RAM constraints which will make us truncate memory */
        Status = BlpMmInitializeConstraints();
        if (NT_SUCCESS(Status))
        {
            /* The Page Allocator has initialized */
            PapInitializationStatus = TRUE;
            Status = STATUS_SUCCESS;
        }
    }

    /* Return status */
    return Status;
}


