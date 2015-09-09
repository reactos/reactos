/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/pagealloc.c
 * PURPOSE:         Boot Library Memory Manager Page Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"


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
    /* FIXME: Read BCD option 'avoidlowmemory' and 'truncatememory' */
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
        EfiPrintf(L"Local Page: %08I64X Found Page: %08I64X\r\n", LocalDescriptor.BasePage, FoundDescriptor->BasePage);
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
        EfiPrintf(L"Local Page: %08I64X Found Page: %08I64X\r\n", LocalEndPage, FoundEndPage);
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


