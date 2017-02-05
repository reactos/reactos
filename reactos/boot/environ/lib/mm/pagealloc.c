/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/pagealloc.c
 * PURPOSE:         Boot Library Memory Manager Page Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"
#include "bcd.h"

typedef struct _BL_PA_REQUEST
{
    BL_ADDRESS_RANGE BaseRange;
    BL_ADDRESS_RANGE VirtualRange;
    ULONG Type;
    ULONGLONG Pages;
    ULONG MemoryType;
    ULONG Alignment;
    ULONG Flags;
} BL_PA_REQUEST, *PBL_PA_REQUEST;

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
    NTSTATUS Status;
    ULONGLONG LowestAddressValid, HighestAddressValid;

    /* Check for LOWMEM */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdLibraryInteger_AvoidLowPhysicalMemory,
                                    &LowestAddressValid);
    if (NT_SUCCESS(Status))
    {
        EfiPrintf(L"/LOWMEM not supported\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Check for MAXMEM */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdLibraryInteger_TruncatePhysicalMemory,
                                    &HighestAddressValid);
    if (NT_SUCCESS(Status))
    {
        EfiPrintf(L"/MAXMEM not supported\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Return back to the caller */
    return STATUS_SUCCESS;
}

NTSTATUS
MmPapAllocateRegionFromMdl (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST NewList,
    _Out_opt_ PBL_MEMORY_DESCRIPTOR Descriptor,
    _In_ PBL_MEMORY_DESCRIPTOR_LIST CurrentList,
    _In_ PBL_PA_REQUEST Request, 
    _In_ BL_MEMORY_TYPE Type
    )
{
    NTSTATUS Status;
    BL_MEMORY_DESCRIPTOR LocalDescriptor = {{0}};
    PBL_MEMORY_DESCRIPTOR FoundDescriptor, TempDescriptor;
    PLIST_ENTRY ListHead, NextEntry;
    BOOLEAN TopDown, GotFwPages;
    EFI_PHYSICAL_ADDRESS EfiAddress;
    ULONGLONG LocalEndPage, FoundEndPage, LocalVirtualEndPage;

    /* Check if any parameters were not passed in correctly */
    if ( !(CurrentList) || !(Request) || (!(NewList) && !(Descriptor)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set failure by default */
    Status = STATUS_NO_MEMORY;

    /* Take the head and next entry in the list, as appropriate */
    ListHead = CurrentList->First;
    if (Request->Type & BL_MM_REQUEST_TOP_DOWN_TYPE)
    {
        NextEntry = ListHead->Blink;
        TopDown = TRUE;
    }
    else
    {
        NextEntry = ListHead->Flink;
        TopDown = FALSE;
    }

    /* Loop through the list */
    GotFwPages = FALSE;
    while (NextEntry != ListHead)
    {
        /* Grab a descriptor */
        FoundDescriptor = CONTAINING_RECORD(NextEntry,
                                            BL_MEMORY_DESCRIPTOR,
                                            ListEntry);

        /* See if it matches  the request */
        if (MmMdFindSatisfyingRegion(FoundDescriptor,
                                     &LocalDescriptor,
                                     Request->Pages,
                                     &Request->BaseRange,
                                     &Request->VirtualRange,
                                     TopDown,
                                     Request->MemoryType,
                                     Request->Flags,
                                     Request->Alignment))
        {
            /* It does, get out */
            break;
        }

        /* It doesn't, move to the next appropriate entry */
        if (TopDown)
        {
            NextEntry = NextEntry->Blink;
        }
        else
        {
            NextEntry = NextEntry->Flink;
        }
    }

    /* Check if we exhausted the list */
    if (NextEntry == ListHead)
    {
        EfiPrintf(L"No matching memory found\r\n");
        return Status;
    }

    /* Copy all the flags that are not request flag */
    LocalDescriptor.Flags = (Request->Flags & 0xFFFF0000) |
                            (LocalDescriptor.Flags & 0x0000FFFF);

    /* Are we using the physical memory list, and are we OK with using firmware? */
    if ((CurrentList == &MmMdlUnmappedUnallocated) &&
        !((Request->Flags & BlMemoryNonFirmware) ||
          (LocalDescriptor.Flags & BlMemoryNonFirmware)))
    {
        /* Allocate the requested address from EFI */
        EfiAddress = LocalDescriptor.BasePage << PAGE_SHIFT;
        Status = EfiAllocatePages(AllocateAddress,
                                  (ULONG)LocalDescriptor.PageCount,
                                  &EfiAddress);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"EFI memory allocation failure\r\n");
            return Status;
        }

        /* Remember we got memory from EFI */
        GotFwPages = TRUE;
    }

    /* Remove the descriptor from the original list it was on */
    MmMdRemoveDescriptorFromList(CurrentList, FoundDescriptor);

    /* Are we allocating from the virtual memory list? */
    if (CurrentList == &MmMdlMappedUnallocated)
    {
        EfiPrintf(L"Virtual memory not yet supported\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Does the memory we received not exactly fall onto the beginning of its descriptor? */
    if (LocalDescriptor.BasePage != FoundDescriptor->BasePage)
    {
        TempDescriptor = MmMdInitByteGranularDescriptor(FoundDescriptor->Flags,
                                                        FoundDescriptor->Type,
                                                        FoundDescriptor->BasePage,
                                                        FoundDescriptor->VirtualPage,
                                                        LocalDescriptor.BasePage -
                                                        FoundDescriptor->BasePage);
        Status = MmMdAddDescriptorToList(CurrentList, TempDescriptor, 0);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Does the memory we received not exactly fall onto the end of its descriptor? */
    LocalEndPage = LocalDescriptor.PageCount + LocalDescriptor.BasePage;
    FoundEndPage = FoundDescriptor->PageCount + FoundDescriptor->BasePage;
    LocalVirtualEndPage = LocalDescriptor.VirtualPage ?
                          LocalDescriptor.VirtualPage + LocalDescriptor.PageCount : 0;
    if (LocalEndPage != FoundEndPage)
    {
        TempDescriptor = MmMdInitByteGranularDescriptor(FoundDescriptor->Flags,
                                                        FoundDescriptor->Type,
                                                        LocalEndPage,
                                                        LocalVirtualEndPage,
                                                        FoundEndPage - LocalEndPage);
        Status = MmMdAddDescriptorToList(CurrentList, TempDescriptor, 0);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* We got the memory we needed */
    Status = STATUS_SUCCESS;

    /* Are we supposed to insert it into a new list? */
    if (NewList)
    {
        /* Copy the allocated region descriptor into the one we found */
        FoundDescriptor->BaseAddress = LocalDescriptor.BaseAddress;
        FoundDescriptor->VirtualPage = LocalDescriptor.VirtualPage;
        FoundDescriptor->PageCount = LocalDescriptor.PageCount;
        FoundDescriptor->Type = Type;
        FoundDescriptor->Flags = LocalDescriptor.Flags;

        /* Remember if it came from EFI */
        if (GotFwPages)
        {
            FoundDescriptor->Flags |= BlMemoryFirmware;
        }

        /* Add the descriptor to the requested list */
        Status = MmMdAddDescriptorToList(NewList, FoundDescriptor, 0);
    }
    else
    {
        /* Free the descriptor, nobody wants to know about it anymore */
        MmMdFreeDescriptor(FoundDescriptor);
    }

    /* Return the allocation region back */
    RtlCopyMemory(Descriptor, &LocalDescriptor, sizeof(LocalDescriptor));
    return Status;
}

NTSTATUS
MmPaAllocatePages (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST NewList,
    _In_ PBL_MEMORY_DESCRIPTOR Descriptor, 
    _In_ PBL_MEMORY_DESCRIPTOR_LIST CurrentList,
    _In_ PBL_PA_REQUEST Request,
    _In_ BL_MEMORY_TYPE MemoryType
    )
{
    NTSTATUS Status;

    /* Heap and page directory/table pages have a special flag */
    if ((MemoryType >= BlLoaderHeap) && (MemoryType <= BlLoaderReferencePage))
    {
        Request->Flags |= BlMemorySpecial;
    }

    /* Try to find a free region of RAM matching this range and request */
    Request->MemoryType = BlConventionalMemory;
    Status = MmPapAllocateRegionFromMdl(NewList,
                                        Descriptor,
                                        CurrentList,
                                        Request,
                                        MemoryType);
    if (Status == STATUS_NOT_FOUND)
    {
        /* Need to re-synchronize the memory map and check other lists */
        EfiPrintf(L"No RAM found -- backup plan not yet implemented\r\n");
    }

    /* Did we get the region we wanted? */
    if (NT_SUCCESS(Status))
    {
        /* All good, return back */
        return Status;
    }

    /* Nope, we have to hunt for it elsewhere */
    EfiPrintf(L"TODO\r\n");
    return Status;
}

NTSTATUS
MmPapAllocatePhysicalPagesInRange (
    _Inout_ PPHYSICAL_ADDRESS BaseAddress,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONGLONG Pages,
    _In_ ULONG Attributes,
    _In_ ULONG Alignment,
    _In_ PBL_MEMORY_DESCRIPTOR_LIST NewList,
    _In_opt_ PBL_ADDRESS_RANGE Range, 
    _In_ ULONG RangeType
    )
{
    NTSTATUS Status;
    BL_PA_REQUEST Request;
    BL_MEMORY_DESCRIPTOR Descriptor;

    /* Increase nesting depth */
    ++MmDescriptorCallTreeCount;

    /* Bail out if no address was specified */
    if (!BaseAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Bail out if no page count was passed in, or a bad list was specified  */
    if (!(Pages) ||
        ((NewList != &MmMdlUnmappedAllocated) &&
         (NewList != &MmMdlPersistentMemory)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Bail out if the passed in range is invalid */
    if ((Range) && (Range->Minimum >= Range->Maximum))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Adjust alignment as needed */
    if (!Alignment)
    {
        Alignment = 1;
    }

    /* Clear the virtual range */
    Request.VirtualRange.Minimum = 0;
    Request.VirtualRange.Maximum = 0;

    /* Check if a fixed allocation was requested*/
    if (Attributes & BlMemoryFixed)
    {
        /* Force the only available range to be the passed in address */
        Request.BaseRange.Minimum = BaseAddress->QuadPart >> PAGE_SHIFT;
        Request.BaseRange.Maximum = Request.BaseRange.Minimum + Pages - 1;
    }
    else if (Range)
    {
        /* Otherwise, a manual range was specified, use it */
        Request.BaseRange.Minimum = Range->Minimum >> PAGE_SHIFT;
        Request.BaseRange.Maximum = Request.BaseRange.Minimum +
                                    (Range->Maximum >> PAGE_SHIFT) - 1;
    }
    else
    {
        /* Otherwise, use any possible range of pages */
        Request.BaseRange.Minimum = PapMinimumPhysicalPage;
        Request.BaseRange.Maximum = MAXULONG >> PAGE_SHIFT;
    }

    /* Check if no type was specified, or if it was invalid */
    if (!(RangeType) ||
         (RangeType & ~(BL_MM_REQUEST_TOP_DOWN_TYPE | BL_MM_REQUEST_DEFAULT_TYPE)))
    {
        /* Use default type */
        Request.Type = BL_MM_REQUEST_DEFAULT_TYPE;
    }
    else
    {
        /* Use the requested type */
        Request.Type = RangeType;
    }

    /* Capture the other request parameters */
    Request.Alignment = Alignment;
    Request.Pages = Pages;
    Request.Flags = Attributes;
    Status = MmPaAllocatePages(NewList,
                               &Descriptor,
                               &MmMdlUnmappedUnallocated,
                               &Request,
                               MemoryType);
    if (NT_SUCCESS(Status))
    {
        /* We got a descriptor back, return its address */
        BaseAddress->QuadPart = Descriptor.BasePage << PAGE_SHIFT;
    }

Quickie:
    /* Restore the nesting depth */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

NTSTATUS
MmPapAllocatePagesInRange (
    _Inout_ PVOID* PhysicalAddress,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONGLONG Pages,
    _In_ ULONG Attributes,
    _In_ ULONG Alignment,
    _In_opt_ PBL_ADDRESS_RANGE Range,
    _In_ ULONG Type
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS BaseAddress;

    /* Increment nesting depth */
    ++MmDescriptorCallTreeCount;

    /* Check for missing parameters or invalid range */
    if (!(PhysicalAddress) ||
        !(Pages) ||
        ((Range) && (Range->Minimum >= Range->Maximum)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    /* What translation mode are we using? */
    if (MmTranslationType != BlNone)
    {
        /* We don't support virtual memory yet */
        Status = STATUS_NOT_IMPLEMENTED;
        goto Exit;
    }
    else
    {
        /* Check if this is a fixed allocation */
        BaseAddress.QuadPart = (Attributes & BlMemoryFixed) ? (ULONG_PTR)*PhysicalAddress : 0;

        /* Allocate the pages */
        Status = MmPapAllocatePhysicalPagesInRange(&BaseAddress,
                                                   MemoryType,
                                                   Pages,
                                                   Attributes,
                                                   Alignment,
                                                   (&MmMdlMappedAllocated !=
                                                    &MmMdlPersistentMemory) ?
                                                   &MmMdlUnmappedAllocated :
                                                   &MmMdlMappedAllocated,
                                                   Range,
                                                   Type);

        /* Return the allocated address */
        *PhysicalAddress = (PVOID)BaseAddress.LowPart;
    }

Exit:
    /* Restore the nesting depth */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

NTSTATUS
MmPaInitialize (
    __in PBL_MEMORY_DATA BootMemoryData,
    __in ULONG MinimumAllocationCount
    )
{
    NTSTATUS Status;
    ULONG ExistingDescriptors, FinalOffset;
    PBL_MEMORY_DESCRIPTOR Descriptor, NewDescriptor;

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
#if 0
        PLIST_ENTRY listHead, nextEntry;

        /* Loop the NT firmware memory list */
        EfiPrintf(L"NT MEMORY MAP\n\r\n");
        listHead = &MmMdlUnmappedUnallocated.ListHead;
        nextEntry = listHead->Flink;
        while (listHead != nextEntry)
        {
            Descriptor = CONTAINING_RECORD(nextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

            EfiPrintf(L"Type: %08lX Flags: %08lX Base: 0x%016I64X End: 0x%016I64X\r\n",
                       Descriptor->Type,
                       Descriptor->Flags,
                       Descriptor->BasePage << PAGE_SHIFT,
                       (Descriptor->BasePage + Descriptor->PageCount) << PAGE_SHIFT);

            nextEntry = nextEntry->Flink;
        }
#endif

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
            /* Remove this region from our free memory MDL */
            Status = MmMdRemoveRegionFromMdlEx(&MmMdlUnmappedUnallocated,
                                               0x40000000,
                                               Descriptor->BasePage,
                                               Descriptor->PageCount,
                                               NULL);
            if (!NT_SUCCESS(Status))
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Build a descriptor for it */
            NewDescriptor = MmMdInitByteGranularDescriptor(Descriptor->Flags,
                                                           Descriptor->Type,
                                                           Descriptor->BasePage,
                                                           Descriptor->VirtualPage,
                                                           Descriptor->PageCount);
            if (!NewDescriptor)
            {
                return STATUS_NO_MEMORY;
            }

            /* And add this region to the reserved & allocated MDL */
            Status = MmMdAddDescriptorToList(&MmMdlReservedAllocated, NewDescriptor, 0);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Move on to the next descriptor */
            ExistingDescriptors--;
            Descriptor = (PBL_MEMORY_DESCRIPTOR)((ULONG_PTR)Descriptor + BootMemoryData->DescriptorSize);
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

NTSTATUS
BlMmAllocatePhysicalPages( 
    _In_ PPHYSICAL_ADDRESS Address,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONGLONG PageCount,
    _In_ ULONG Attributes,
    _In_ ULONG Alignment
    )
{
    /* Call the physical allocator */
    return MmPapAllocatePhysicalPagesInRange(Address,
                                             MemoryType,
                                             PageCount,
                                             Attributes,
                                             Alignment,
                                             &MmMdlUnmappedAllocated,
                                             NULL,
                                             0);
}

NTSTATUS
MmPapFreePhysicalPages (
    _In_ ULONG WhichList,
    _In_ ULONGLONG PageCount,
    _In_ PHYSICAL_ADDRESS Address
    )
{
    /* TBD */
    EfiPrintf(L"Leaking memory: %p!\r\n", Address.QuadPart);
    return STATUS_SUCCESS;
}

NTSTATUS
BlMmFreePhysicalPages (
    _In_ PHYSICAL_ADDRESS Address
    )
{
    /* Call the physical allocator */
    return MmPapFreePhysicalPages(BL_MM_INCLUDE_UNMAPPED_ALLOCATED, 0, Address);
}

NTSTATUS
MmPapFreePages (
    _In_ PVOID Address,
    _In_ ULONG WhichList
    )
{
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Handle virtual memory scenario */
    if (MmTranslationType != BlNone)
    {
        EfiPrintf(L"Unimplemented virtual path\r\n");
        return STATUS_SUCCESS;
    }

    /* Physical memory should be in the unmapped allocated list */
    if (WhichList != BL_MM_INCLUDE_PERSISTENT_MEMORY)
    {
        WhichList = BL_MM_INCLUDE_UNMAPPED_ALLOCATED;
    }

    /* Free it from there */
    PhysicalAddress.QuadPart = (ULONG_PTR)Address;
    return MmPapFreePhysicalPages(WhichList, 0, PhysicalAddress);
}

NTSTATUS
BlMmGetMemoryMap (
    _In_ PLIST_ENTRY MemoryMap,
    _In_ PBL_IMAGE_PARAMETERS MemoryParameters,
    _In_ ULONG WhichTypes,
    _In_ ULONG Flags
    )
{
    BL_MEMORY_DESCRIPTOR_LIST FirmwareMdList, FullMdList;
    BOOLEAN DoFirmware, DoPersistent, DoTruncated, DoBad;
    BOOLEAN DoReserved, DoUnmapUnalloc, DoUnmapAlloc;
    BOOLEAN DoMapAlloc, DoMapUnalloc, DoFirmware2;
    ULONG LoopCount, MdListCount, MdListSize, Used;
    NTSTATUS Status;

    /* Initialize the firmware list if we use it */
    MmMdInitializeListHead(&FirmwareMdList);

    /* Make sure we got our input parameters */
    if (!(MemoryMap) || !(MemoryParameters))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Either ask for firmware memory, or don't. Not neither */
    if ((WhichTypes & ~BL_MM_INCLUDE_NO_FIRMWARE_MEMORY) &&
        (WhichTypes & ~BL_MM_INCLUDE_ONLY_FIRMWARE_MEMORY))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Either ask for firmware memory, or don't. Not both */
    if ((WhichTypes & BL_MM_INCLUDE_NO_FIRMWARE_MEMORY) &&
        (WhichTypes & BL_MM_INCLUDE_ONLY_FIRMWARE_MEMORY))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize the memory map list */
    InitializeListHead(MemoryMap);

    /* Check which types of memory to dump */
    DoFirmware = WhichTypes & BL_MM_INCLUDE_FIRMWARE_MEMORY;
    DoPersistent = WhichTypes & BL_MM_INCLUDE_PERSISTENT_MEMORY;
    DoTruncated = WhichTypes & BL_MM_INCLUDE_TRUNCATED_MEMORY;
    DoBad = WhichTypes & BL_MM_INCLUDE_BAD_MEMORY;
    DoReserved = WhichTypes & BL_MM_INCLUDE_RESERVED_ALLOCATED;
    DoUnmapUnalloc = WhichTypes & BL_MM_INCLUDE_UNMAPPED_UNALLOCATED;
    DoUnmapAlloc = WhichTypes & BL_MM_INCLUDE_UNMAPPED_ALLOCATED;
    DoMapAlloc = WhichTypes & BL_MM_INCLUDE_MAPPED_ALLOCATED;
    DoMapUnalloc = WhichTypes & BL_MM_INCLUDE_MAPPED_UNALLOCATED;
    DoFirmware2 = WhichTypes & BL_MM_INCLUDE_FIRMWARE_MEMORY_2;

    /* Begin the attempt loop */
    LoopCount = 0;
    while (TRUE)
    {
        /* Count how many entries we will need */
        MdListCount = 0;
        if (DoMapAlloc) MdListCount = MmMdCountList(&MmMdlMappedAllocated);
        if (DoMapUnalloc) MdListCount += MmMdCountList(&MmMdlMappedUnallocated);
        if (DoUnmapAlloc) MdListCount += MmMdCountList(&MmMdlUnmappedAllocated);
        if (DoUnmapUnalloc) MdListCount += MmMdCountList(&MmMdlUnmappedUnallocated);
        if (DoReserved) MdListCount += MmMdCountList(&MmMdlReservedAllocated);
        if (DoBad) MdListCount += MmMdCountList(&MmMdlBadMemory);
        if (DoTruncated) MdListCount += MmMdCountList(&MmMdlTruncatedMemory);
        if (DoPersistent) MdListCount += MmMdCountList(&MmMdlPersistentMemory);

        /* Plus firmware entries */
        if (DoFirmware)
        {
            /* Free the previous entries, if any */
            MmMdFreeList(&FirmwareMdList);

            /* Get the firmware map */
            Status = MmFwGetMemoryMap(&FirmwareMdList, 2);
            if (!NT_SUCCESS(Status))
            {
                goto Quickie;
            }

            /* We overwrite, since this type is exclusive */
            MdListCount = MmMdCountList(&FirmwareMdList);
        }

        /* Plus firmware entries-2 */
        if (DoFirmware2)
        {
            /* Free the previous entries, if any */
            MmMdFreeList(&FirmwareMdList);

            /* Get the firmware map */
            Status = MmFwGetMemoryMap(&FirmwareMdList, 0);
            if (!NT_SUCCESS(Status))
            {
                goto Quickie;
            }

            /* We overwrite, since this type is exclusive */
            MdListCount = MmMdCountList(&FirmwareMdList);
        }

        /* If there's no descriptors, we're done */
        if (!MdListCount)
        {
            Status = STATUS_SUCCESS;
            goto Quickie;
        }

        /* Check if the buffer we have is big enough */
        if (MemoryParameters->BufferSize >=
            (sizeof(BL_MEMORY_DESCRIPTOR) * MdListCount))
        {
            break;
        }

        /* It's not, allocate it, with a slack of 4 extra descriptors */
        MdListSize = sizeof(BL_MEMORY_DESCRIPTOR) * (MdListCount + 4);

        /* Except if we weren't asked to */
        if (!(Flags & BL_MM_ADD_DESCRIPTOR_ALLOCATE_FLAG))
        {
            MemoryParameters->BufferSize = MdListSize;
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Quickie;
        }

        /* Has it been less than 4 times we've tried this? */
        if (++LoopCount <= 4)
        {
            /* Free the previous attempt, if any */
            if (MemoryParameters->BufferSize)
            {
                BlMmFreeHeap(MemoryParameters->Buffer);
            }

            /* Allocate a new buffer */
            MemoryParameters->BufferSize = MdListSize;
            MemoryParameters->Buffer = BlMmAllocateHeap(MdListSize);
            if (MemoryParameters->Buffer)
            {
                /* Try again */
                continue;
            }
        }

        /* If we got here, we're out of memory after 4 attempts */
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* We should have a buffer by now... */
    if (MemoryParameters->Buffer)
    {
        /* Zero it out */
        RtlZeroMemory(MemoryParameters->Buffer,
                      MdListCount * sizeof(BL_MEMORY_DESCRIPTOR));
    }

    /* Initialize our list of descriptors */
    MmMdInitializeList(&FullMdList, 0, MemoryMap);
    Used = 0;

    /* Handle mapped, allocated */
    if (DoMapAlloc)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlMappedAllocated,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle mapped, unallocated */
    if (DoMapUnalloc)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlMappedUnallocated,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle unmapped, allocated */
    if (DoUnmapAlloc)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlUnmappedAllocated,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle unmapped, unallocated */
    if (DoUnmapUnalloc)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlUnmappedUnallocated,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle reserved, allocated */
    if (DoReserved)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlReservedAllocated,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle bad */
    if (DoBad)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlBadMemory,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle truncated */
    if (DoTruncated)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlTruncatedMemory,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle persistent */
    if (DoPersistent)
    {
        Status = MmMdCopyList(&FullMdList,
                              &MmMdlPersistentMemory,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle firmware */
    if (DoFirmware)
    {
        Status = MmMdCopyList(&FullMdList,
                              &FirmwareMdList,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Handle firmware2 */
    if (DoFirmware2)
    {
        Status = MmMdCopyList(&FullMdList,
                              &FirmwareMdList,
                              MemoryParameters->Buffer,
                              &Used,
                              MdListCount,
                              Flags);
    }

    /* Add up the final size */
    Status = RtlULongLongToULong(Used * sizeof(BL_MEMORY_DESCRIPTOR),
                                 &MemoryParameters->ActualSize);

Quickie:
    MmMdFreeList(&FirmwareMdList);
    return Status;
}
