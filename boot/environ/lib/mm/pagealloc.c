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

extern ULONG MmArchLargePageSize;

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
MmPaTruncateMemory (
    _In_ ULONGLONG BasePage
    )
{
    NTSTATUS Status;

    /* Increase nesting depth */
    ++MmDescriptorCallTreeCount;

    /* Set the maximum page to the truncated request */
    if (BasePage < PapMaximumPhysicalPage)
    {
        PapMaximumPhysicalPage = BasePage;
    }

    /* Truncate mapped and allocated memory */
    Status = MmMdTruncateDescriptors(&MmMdlMappedAllocated,
                                     &MmMdlTruncatedMemory,
                                     BasePage);
    if (NT_SUCCESS(Status))
    {
        /* Truncate unmapped and allocated memory */
        Status = MmMdTruncateDescriptors(&MmMdlUnmappedAllocated,
                                         &MmMdlTruncatedMemory,
                                         BasePage);
        if (NT_SUCCESS(Status))
        {
            /* Truncate mapped and unallocated memory */
            Status = MmMdTruncateDescriptors(&MmMdlMappedUnallocated,
                                             &MmMdlTruncatedMemory,
                                             BasePage);
            if (NT_SUCCESS(Status))
            {
                /* Truncate unmapped and unallocated memory */
                Status = MmMdTruncateDescriptors(&MmMdlUnmappedUnallocated,
                                                 &MmMdlTruncatedMemory,
                                                 BasePage);
                if (NT_SUCCESS(Status))
                {
                    /* Truncate reserved memory */
                    Status = MmMdTruncateDescriptors(&MmMdlReservedAllocated,
                                                     &MmMdlTruncatedMemory,
                                                     BasePage);
                }
            }
        }
    }

    /* Restore the nesting depth */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

NTSTATUS
BlpMmInitializeConstraints (
    VOID
    )
{
    NTSTATUS Status, ReturnStatus;
    ULONGLONG LowestAddressValid, HighestAddressValid;
    ULONGLONG LowestPage, HighestPage;

    /* Assume success */
    ReturnStatus = STATUS_SUCCESS;

    /* Check for LOWMEM */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdLibraryInteger_AvoidLowPhysicalMemory,
                                    &LowestAddressValid);
    if (NT_SUCCESS(Status))
    {
        /* Align the address */
        LowestAddressValid = (ULONG_PTR)PAGE_ALIGN(LowestAddressValid);
        LowestPage = LowestAddressValid >> PAGE_SHIFT;

        /* Make sure it's below 4GB */
        if (LowestPage <= 0x100000)
        {
            PapMinimumPhysicalPage = LowestPage;
        }
    }

    /* Check for MAXMEM */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdLibraryInteger_TruncatePhysicalMemory,
                                    &HighestAddressValid);
    if (NT_SUCCESS(Status))
    {
        /* Get the page */
        HighestPage = HighestAddressValid >> PAGE_SHIFT;

        /* Truncate memory above this page */
        ReturnStatus = MmPaTruncateMemory(HighestPage);
    }

    /* Return back to the caller */
    return ReturnStatus;
}

PWCHAR
MmMdListPointerToName (_In_ PBL_MEMORY_DESCRIPTOR_LIST MdList)
{
    if (MdList == &MmMdlUnmappedAllocated)
    {
        return L"UnmapAlloc";
    }
    else if (MdList == &MmMdlUnmappedUnallocated)
    {
        return L"UnmapUnalloc";
    }
    else if (MdList == &MmMdlMappedAllocated)
    {
        return L"MapAlloc";
    }
    else if (MdList == &MmMdlMappedUnallocated)
    {
        return L"MapUnalloc";
    }
    else
    {
        return L"Other";
    }
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
    if (!(CurrentList) || !(Request) || (!(NewList) && !(Descriptor)))
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

        /* See if it matches the request */
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
            EfiStall(10000000);
            return Status;
        }

        /* Remember we got memory from EFI */
        GotFwPages = TRUE;
    }

    /* Remove the descriptor from the original list it was on */
    MmMdRemoveDescriptorFromList(CurrentList, FoundDescriptor);

    /* Get the end pages */
    LocalEndPage = LocalDescriptor.PageCount + LocalDescriptor.BasePage;
    FoundEndPage = FoundDescriptor->PageCount + FoundDescriptor->BasePage;

    /* Are we allocating from the virtual memory list? */
    if (CurrentList == &MmMdlMappedUnallocated)
    {
        /* Check if the region matches perfectly */
        if ((LocalDescriptor.BasePage == FoundDescriptor->BasePage) &&
            (LocalEndPage == FoundEndPage))
        {
            /* Check if the original descriptor had the flag set */
            if ((FoundDescriptor->Flags & 0x40000000) && (Descriptor))
            {
                /* Make our local one have it too, even if not needed */
                LocalDescriptor.Flags |= 0x40000000;
            }
        }
        else
        {
            /* Write the 'incomplete mapping' flag */
            FoundDescriptor->Flags |= 0x40000000;
            if (Descriptor)
            {
                /* Including on the local one if there's one passed in */
                LocalDescriptor.Flags |= 0x40000000;
            }
        }
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

    /* Are we failing due to some attributes? */
    if (Request->Flags & BlMemoryValidAllocationAttributeMask)
    {
        if (Request->Flags & BlMemoryLargePages)
        {
            EfiPrintf(L"large alloc fail not yet implemented %lx\r\n", Status);
            EfiStall(1000000);
            return STATUS_NOT_IMPLEMENTED;
        }
        if (Request->Flags & BlMemoryFixed)
        {
            EfiPrintf(L"fixed alloc fail not yet implemented %lx\r\n", Status);
            EfiStall(1000000);
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    /* Nope, just fail the entire call */
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
MmPapPageAllocatorExtend (
    _In_ ULONG Attributes,
    _In_ ULONG Alignment,
    _In_ ULONGLONG PageCount,
    _In_ ULONGLONG VirtualPage,
    _In_opt_ PBL_ADDRESS_RANGE Range,
    _In_opt_ ULONG Type
    )
{
    BL_PA_REQUEST Request;
    ULONGLONG PageRange;
    BL_MEMORY_DESCRIPTOR NewDescriptor;
    ULONG AllocationFlags, CacheAttributes, AddFlags;
    NTSTATUS Status;
    PBL_MEMORY_DESCRIPTOR_LIST MdList;
    PBL_MEMORY_DESCRIPTOR Descriptor;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Is the caller requesting less pages than allowed? */
    if (!(Attributes & BlMemoryFixed) &&
        !(Range) &&
        (PageCount < PapMinimumAllocationCount))
    {
        /* Unless this is a fixed request, then adjust the original requirements */
        PageCount = PapMinimumAllocationCount;
        Alignment = PapMinimumAllocationCount;
    }

    /* Extract only the allocation attributes */
    AllocationFlags = Attributes & BlMemoryValidAllocationAttributeMask;

    /* Check if the caller wants large pages */
    if ((AllocationFlags & BlMemoryLargePages) && (MmArchLargePageSize != 1))
    {
        EfiPrintf(L"Large pages not supported!\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Set an emty virtual range */
    Request.VirtualRange.Minimum = 0;
    Request.VirtualRange.Maximum = 0;

    /* Check if the caller requested a range */
    if (Range)
    {
        /* Calculate it size in pages, minus a page as this is a 0-based range */
        PageRange = ((Range->Maximum - Range->Minimum) >> PAGE_SHIFT) - 1;

        /* Set the minimum and maximum, in pages */
        Request.BaseRange.Minimum = Range->Minimum >> PAGE_SHIFT;
        Request.BaseRange.Maximum = Request.BaseRange.Minimum + PageRange;
    }
    else
    {
        /* Initialize a range from the smallest page to the biggest */
        Request.BaseRange.Minimum = PapMinimumPhysicalPage;
        Request.BaseRange.Maximum = 0xFFFFFFFF / PAGE_SIZE;
    }

    /* Get the cache attributes */
    CacheAttributes = Attributes & BlMemoryValidCacheAttributeMask;

    /* Check if the caller requested a valid allocation type */
    if ((Type) && !(Type & ~(BL_MM_REQUEST_DEFAULT_TYPE |
                             BL_MM_REQUEST_TOP_DOWN_TYPE)))
    {
        /* Use what the caller wanted */
        Request.Type = Type;
    }
    else
    {
        /* Use the default bottom-up type */
        Request.Type = BL_MM_REQUEST_DEFAULT_TYPE;
    }

    /* Use the original protection and type, but ignore other attributes */
    Request.Flags = Attributes & ~(BlMemoryValidAllocationAttributeMask |
                                   BlMemoryValidCacheAttributeMask);
    Request.Alignment = Alignment;
    Request.Pages = PageCount;

    /* Allocate some free pages */
    Status = MmPaAllocatePages(NULL,
                               &NewDescriptor,
                               &MmMdlUnmappedUnallocated,
                               &Request,
                               BlConventionalMemory);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Failed to get unmapped, unallocated memory!\r\n");
        EfiStall(10000000);
        return Status;
    }

    /* Initialize a descriptor for these pages, adding in the allocation flags */
    Descriptor = MmMdInitByteGranularDescriptor(AllocationFlags |
                                                NewDescriptor.Flags,
                                                BlConventionalMemory,
                                                NewDescriptor.BasePage,
                                                NewDescriptor.VirtualPage,
                                                NewDescriptor.PageCount);

    /* Now map a virtual address for these physical pages */
    VirtualAddress = (PVOID)((ULONG_PTR)VirtualPage << PAGE_SHIFT);
    PhysicalAddress.QuadPart = NewDescriptor.BasePage << PAGE_SHIFT;
    Status = BlMmMapPhysicalAddressEx(&VirtualAddress,
                                      AllocationFlags | CacheAttributes,
                                      NewDescriptor.PageCount << PAGE_SHIFT,
                                      PhysicalAddress);
    if (Status == STATUS_SUCCESS)
    {
        /* Add the cache attributes now that the mapping worked */
        Descriptor->Flags |= CacheAttributes;

        /* Update the virtual page now that we mapped it */
        Descriptor->VirtualPage = (ULONG_PTR)VirtualAddress >> PAGE_SHIFT;

        /* Add this as a mapped region */
        Status = MmMdAddDescriptorToList(&MmMdlMappedUnallocated,
                                         Descriptor,
                                         BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG);

        /* Make new descriptor that we'll add in firmware allocation tracker */
        MdList = &MmMdlFwAllocationTracker;
        Descriptor = MmMdInitByteGranularDescriptor(0,
                                                    BlConventionalMemory,
                                                    NewDescriptor.BasePage,
                                                    0,
                                                    NewDescriptor.PageCount);

        /* Do not coalesce */
        AddFlags = 0;
    }
    else
    {
        /* We failed, free the physical pages */
        Status = MmFwFreePages(NewDescriptor.BasePage, NewDescriptor.PageCount);
        if (!NT_SUCCESS(Status))
        {
            /* We failed to free the pages, so this is still around */
            MdList = &MmMdlUnmappedAllocated;
        }
        else
        {
            /* This is now back to unmapped/unallocated memory */
            Descriptor->Flags = 0;
            MdList = &MmMdlUnmappedUnallocated;
        }

        /* Coalesce the free descriptor */
        AddFlags = BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG;
    }

    /* Either add to firmware list, or to unmapped list, then return result */
    MmMdAddDescriptorToList(MdList, Descriptor, AddFlags);
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
    BL_PA_REQUEST Request;
    PBL_MEMORY_DESCRIPTOR_LIST List;
    BL_MEMORY_DESCRIPTOR Descriptor;

    /* Increment nesting depth */
    ++MmDescriptorCallTreeCount;

    /* Default list */
    List = &MmMdlMappedAllocated;

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
        /* Use 1 page alignment if none was requested */
        if (!Alignment)
        {
            Alignment = 1;
        }

        /* Check if we got a range */
        if (Range)
        {
            /* We don't support virtual memory yet @TODO */
            EfiPrintf(L"virt range not yet implemented in %S\r\n", __FUNCTION__);
            EfiStall(1000000);
            Status = STATUS_NOT_IMPLEMENTED;
            goto Exit;
        }
        else
        {
            /* Use the entire range that's possible */
            Request.BaseRange.Minimum = PapMinimumPhysicalPage;
            Request.BaseRange.Maximum = 0xFFFFFFFF >> PAGE_SHIFT;
        }

        /* Check if a fixed allocation was requested */
        if (Attributes & BlMemoryFixed)
        {
            /* We don't support virtual memory yet @TODO */
            EfiPrintf(L"fixed not yet implemented in %S\r\n", __FUNCTION__);
            EfiStall(1000000);
            Status = STATUS_NOT_IMPLEMENTED;
            goto Exit;
        }
        else
        {
            /* Check if kernel range was specifically requested */
            if (Attributes & BlMemoryKernelRange)
            {
                /* Use the kernel range */
                Request.VirtualRange.Minimum = MmArchKsegAddressRange.Minimum >> PAGE_SHIFT;
                Request.VirtualRange.Maximum = MmArchKsegAddressRange.Maximum >> PAGE_SHIFT;
            }
            else
            {
                /* Set the virtual address range */
                Request.VirtualRange.Minimum = 0;
                Request.VirtualRange.Maximum = 0xFFFFFFFF >> PAGE_SHIFT;
            }
        }

        /* Check what type of allocation was requested */
        if ((Type) && !(Type & ~(BL_MM_REQUEST_DEFAULT_TYPE |
                               BL_MM_REQUEST_TOP_DOWN_TYPE)))
        {
            /* Save it if it was valid */
            Request.Type = Type;
        }
        else
        {
            /* Set the default */
            Request.Type = BL_MM_REQUEST_DEFAULT_TYPE;
        }

        /* Fill out the request of the request */
        Request.Flags = Attributes;
        Request.Alignment = Alignment;
        Request.Pages = Pages;

        /* Try to allocate the pages */
        Status = MmPaAllocatePages(List,
                                   &Descriptor,
                                   &MmMdlMappedUnallocated,
                                   &Request,
                                   MemoryType);
        if (!NT_SUCCESS(Status))
        {
            /* Extend the physical allocator */
            Status = MmPapPageAllocatorExtend(Attributes,
                                              Alignment,
                                              Pages,
                                              ((ULONG_PTR)*PhysicalAddress) >>
                                              PAGE_SHIFT,
                                              Range,
                                              Type);
            if (!NT_SUCCESS(Status))
            {
                /* Fail since we're out of memory */
                EfiPrintf(L"EXTEND OUT OF MEMORY: %lx\r\n", Status);
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            /* Try the allocation again now */
            Status = MmPaAllocatePages(&MmMdlMappedAllocated,
                                       &Descriptor,
                                       &MmMdlMappedUnallocated,
                                       &Request,
                                       MemoryType);
            if (!NT_SUCCESS(Status))
            {
                /* Fail since we're out of memory */
                EfiPrintf(L"PALLOC OUT OF MEMORY: %lx\r\n", Status);
                goto Exit;
            }
        }

        /* Return the allocated address */
        *PhysicalAddress = (PVOID)((ULONG_PTR)Descriptor.VirtualPage << PAGE_SHIFT);
    }
    else
    {
        /* Check if this is a fixed allocation */
        BaseAddress.QuadPart = (Attributes & BlMemoryFixed) ?
                                (ULONG_PTR)*PhysicalAddress : 0;

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
        *PhysicalAddress = PhysicalAddressToPtr(BaseAddress);
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
            //EfiPrintf(L"Handling existing descriptor: %llx %llx\r\n", Descriptor->BasePage, Descriptor->PageCount);
            Status = MmMdRemoveRegionFromMdlEx(&MmMdlUnmappedUnallocated,
                                               BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
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
    PBL_MEMORY_DESCRIPTOR Descriptor;
    ULONGLONG Page;
    ULONG DescriptorFlags, Flags;
    BOOLEAN DontFree, HasPageData;
    BL_LIBRARY_PARAMETERS LibraryParameters;
    NTSTATUS Status;

    /* Set some defaults */
    Flags = BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG;
    DontFree = FALSE;
    HasPageData = FALSE;

    /* Only page-aligned addresses are accepted */
    if (Address.QuadPart & (PAGE_SIZE - 1))
    {
        EfiPrintf(L"free mem fail 1\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Try to find the descriptor containing this address */
    Page = Address.QuadPart >> PAGE_SHIFT;
    Descriptor = MmMdFindDescriptor(WhichList,
                                    BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                    Page);
    if (!Descriptor)
    {
        EfiPrintf(L"free mem fail 2\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* If a page count was given, it must match, unless it's coalesced */
    DescriptorFlags = Descriptor->Flags;
    if (!(DescriptorFlags & BlMemoryCoalesced) &&
        (PageCount) && (PageCount != Descriptor->PageCount))
    {
        EfiPrintf(L"free mem fail 3\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is persistent memory in teardown status */
    if ((PapInitializationStatus == 2) &&
        (DescriptorFlags & BlMemoryPersistent))
    {
        /* Then we should keep it */
        DontFree = TRUE;
    }
    else
    {
        /* Mark it as non-persistent, since we're freeing it */
        Descriptor->Flags &= ~BlMemoryPersistent;
    }

    /* Check if this memory contains paging data */
    if ((Descriptor->Type == BlLoaderPageDirectory) ||
        (Descriptor->Type == BlLoaderReferencePage))
    {
        HasPageData = TRUE;
    }

    /* Check if a page count was given */
    if (PageCount)
    {
        /* The pages must fit within the descriptor */
        if ((PageCount + Page - Descriptor->BasePage) > Descriptor->PageCount)
        {
            EfiPrintf(L"free mem fail 4\r\n");
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* No page count given, so the address must be at the beginning then */
        if (Descriptor->BasePage != Page)
        {
            EfiPrintf(L"free mem fail 5\r\n");
            return STATUS_INVALID_PARAMETER;
        }

        /* And we'll use the page count in the descriptor */
        PageCount = Descriptor->PageCount;
    }

    /* Copy library parameters since we will read them */
    RtlCopyMemory(&LibraryParameters,
                  &BlpLibraryParameters,
                  sizeof(LibraryParameters));

    /* Check if this is teardown */
    if (PapInitializationStatus == 2)
    {
        EfiPrintf(L"Case 2 not yet handled!\r\n");
        return STATUS_NOT_SUPPORTED;
    }
    else if (!DontFree)
    {
        /* Caller wants memory to be freed -- should we zero it? */
        if (!(HasPageData) &&
            (LibraryParameters.LibraryFlags &
             BL_LIBRARY_FLAG_ZERO_HEAP_ALLOCATIONS_ON_FREE))
        {
            EfiPrintf(L"Freeing zero data not yet handled!\r\n");
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Now call into firmware to actually free the physical pages */
    Status = MmFwFreePages(Page, PageCount);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"free mem fail 6\r\n");
        return Status;
    }

    /* Remove the firmware flags */
    Descriptor->Flags &= ~(BlMemoryNonFirmware |
                           BlMemoryFirmware |
                           BlMemoryPersistent);

    /* If we're not actually freeing, don't coalesce with anyone nearby */
    if (DontFree)
    {
        Flags |= BL_MM_ADD_DESCRIPTOR_NEVER_COALESCE_FLAG;
    }

    /* Check if the entire allocation is being freed */
    if (PageCount == Descriptor->PageCount)
    {
        /* Remove the descriptor from the allocated list */
        MmMdRemoveDescriptorFromList(&MmMdlUnmappedAllocated, Descriptor);

        /* Mark the entire descriptor as free */
        Descriptor->Type = BlConventionalMemory;
    }
    else
    {
        /* Init a descriptor for what we're actually freeing */
        Descriptor = MmMdInitByteGranularDescriptor(Descriptor->Flags,
                                                    BlConventionalMemory,
                                                    Page,
                                                    0,
                                                    PageCount);
        if (!Descriptor)
        {
            return STATUS_NO_MEMORY;
        }

        /* Remove the region from the existing descriptor */
        Status = MmMdRemoveRegionFromMdlEx(&MmMdlUnmappedAllocated,
                                           BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                           Page,
                                           PageCount,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Add the new descriptor into in the list (or the old, repurposed one) */
    Descriptor->Flags &= ~BlMemoryCoalesced;
    return MmMdAddDescriptorToList(&MmMdlUnmappedUnallocated, Descriptor, Flags);
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
        EfiPrintf(L"Unimplemented free virtual path: %p %lx\r\n", Address, WhichList);
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
    _In_ PBL_BUFFER_DESCRIPTOR MemoryParameters,
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

            /* Get the firmware map, coalesced */
            Status = MmFwGetMemoryMap(&FirmwareMdList,
                                      BL_MM_FLAG_REQUEST_COALESCING);
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

            /* Get the firmware map, uncoalesced */
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

NTSTATUS
MmPaReleaseSelfMapPages (
    _In_ PHYSICAL_ADDRESS Address
    )
{
    PBL_MEMORY_DESCRIPTOR Descriptor;
    ULONGLONG BasePage;
    NTSTATUS Status;

    /* Only page-aligned addresses are accepted */
    if (Address.QuadPart & (PAGE_SIZE - 1))
    {
        EfiPrintf(L"free mem fail 1\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the base page, and find a descriptor that matches */
    BasePage = Address.QuadPart >> PAGE_SHIFT;
    Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_UNMAPPED_UNALLOCATED,
                                    BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                    BasePage);
    if (!(Descriptor) || (Descriptor->BasePage != BasePage))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Free the physical pages */
    Status = MmFwFreePages(BasePage, Descriptor->PageCount);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Remove the firmware flags */
    Descriptor->Flags &= ~(BlMemoryNonFirmware |
                           BlMemoryFirmware |
                           BlMemoryPersistent);

    /* Set it as free memory */
    Descriptor->Type = BlConventionalMemory;

    /* Create a new descriptor that's free memory, covering the old range */
    Descriptor = MmMdInitByteGranularDescriptor(0,
                                                BlConventionalMemory,
                                                BasePage,
                                                0,
                                                Descriptor->PageCount);
    if (!Descriptor)
    {
        return STATUS_NO_MEMORY;
    }

    /* Insert it into the virtual free list */
    return MmMdAddDescriptorToList(&MmMdlFreeVirtual,
                                   Descriptor,
                                   BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG |
                                   BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG);
}

NTSTATUS
MmPaReserveSelfMapPages (
    _Inout_ PPHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Alignment,
    _In_ ULONG PageCount
    )
{
    NTSTATUS Status;
    BL_PA_REQUEST Request;
    BL_MEMORY_DESCRIPTOR Descriptor;

    /* Increment descriptor usage count */
    ++MmDescriptorCallTreeCount;

    /* Bail if we don't have an address */
    if (!PhysicalAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Make a request for the required number of self-map pages */
    Request.BaseRange.Minimum = PapMinimumPhysicalPage;
    Request.BaseRange.Maximum = 0xFFFFFFFF >> PAGE_SHIFT;
    Request.VirtualRange.Minimum = 0;
    Request.VirtualRange.Maximum = 0;
    Request.Pages = PageCount;
    Request.Alignment = Alignment;
    Request.Type = BL_MM_REQUEST_DEFAULT_TYPE;
    Request.Flags = 0;
    Status = MmPaAllocatePages(&MmMdlUnmappedUnallocated,
                               &Descriptor,
                               &MmMdlUnmappedUnallocated,
                               &Request,
                               BlLoaderSelfMap);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Remove this region from free virtual memory */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       Descriptor.BasePage,
                                       Descriptor.PageCount,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Return the physical address */
    PhysicalAddress->QuadPart = Descriptor.BasePage << PAGE_SHIFT;

Quickie:
    /* Free global descriptors and reduce the count by one */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

NTSTATUS
MmSelectMappingAddress (
    _Out_ PVOID* MappingAddress,
    _In_ PVOID PreferredAddress,
    _In_ ULONGLONG Size,
    _In_ ULONG AllocationAttributes,
    _In_ ULONG Flags,
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    )
{
    BL_PA_REQUEST Request;
    NTSTATUS Status;
    BL_MEMORY_DESCRIPTOR NewDescriptor;

    /* Are we in physical mode? */
    if (MmTranslationType == BlNone)
    {
        /* Just return the physical address as the mapping address */
        PreferredAddress = PhysicalAddressToPtr(PhysicalAddress);
        goto Success;
    }

    /* If no physical address, or caller wants a fixed address... */
    if ((PhysicalAddress.QuadPart == -1) || (Flags & BlMemoryFixed))
    {
        /* Then just return the preferred address */
        goto Success;
    }

    /* Check which range of virtual memory should be used */
    if (AllocationAttributes & BlMemoryKernelRange)
    {
        /* Use kernel range */
        Request.BaseRange.Minimum = MmArchKsegAddressRange.Minimum >> PAGE_SHIFT;
        Request.BaseRange.Maximum = MmArchKsegAddressRange.Maximum >> PAGE_SHIFT;
        Request.Type = BL_MM_REQUEST_DEFAULT_TYPE;
    }
    else
    {
        /* User user/application range */
        Request.BaseRange.Minimum = 0 >> PAGE_SHIFT;
        Request.BaseRange.Maximum = MmArchTopOfApplicationAddressSpace >> PAGE_SHIFT;
        Request.Type = BL_MM_REQUEST_TOP_DOWN_TYPE;
    }

    /* Build a request */
    Request.VirtualRange.Minimum = 0;
    Request.VirtualRange.Maximum = 0;
    Request.Flags = AllocationAttributes & BlMemoryLargePages;
    Request.Alignment = 1;
    Request.Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart, Size);

    /* Allocate the physical pages */
    Status = MmPaAllocatePages(NULL,
                               &NewDescriptor,
                               &MmMdlFreeVirtual,
                               &Request,
                               BlConventionalMemory);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Return the address we got back */
    PreferredAddress = (PVOID)((ULONG_PTR)NewDescriptor.BasePage << PAGE_SHIFT);

    /* Check if the existing physical address was not aligned */
    if (PhysicalAddress.QuadPart != -1)
    {
        /* Add the offset to the returned virtual address */
        PreferredAddress = (PVOID)((ULONG_PTR)PreferredAddress +
                                   BYTE_OFFSET(PhysicalAddress.QuadPart));
    }

Success:
    /* Return the mapping address and success */
    *MappingAddress = PreferredAddress;
    return STATUS_SUCCESS;
}
