/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/mm.c
 * PURPOSE:         Boot Library Memory Manager Core
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"
#include "bcd.h"

/* DATA VARIABLES ************************************************************/

/* This is a bug in Windows, but is required for MmTrInitialize to load */
BL_TRANSLATION_TYPE MmTranslationType = BlMax;
BL_TRANSLATION_TYPE MmOriginalTranslationType;
ULONG MmDescriptorCallTreeCount;

/* FUNCTIONS *****************************************************************/

NTSTATUS
TrpGenerateMappingTracker (
    _In_ PVOID VirtualAddress,
    _In_ ULONG Flags,
    _In_ LARGE_INTEGER PhysicalAddress,
    _In_ ULONGLONG Size
    )
{
    PBL_MEMORY_DESCRIPTOR Descriptor, NextDescriptor;
    PLIST_ENTRY ListHead, NextEntry;

    /* Increment descriptor call count */
    MmDescriptorCallTreeCount++;

    /* Initialize a descriptor for this allocation */
    Descriptor = MmMdInitByteGranularDescriptor(Flags,
                                                0,
                                                PhysicalAddress.QuadPart,
                                                (ULONG_PTR)VirtualAddress,
                                                Size);

    /* Loop the current tracker list */
    ListHead = MmMdlMappingTrackers.First;
    NextEntry = ListHead->Flink;
    if (IsListEmpty(ListHead))
    {
        /* If it's empty, just add the descriptor at the end */
        InsertTailList(ListHead, &Descriptor->ListEntry);
        goto Quickie;
    }

    /* Otherwise, go to the last descriptor */
    NextDescriptor = CONTAINING_RECORD(NextEntry,
                                       BL_MEMORY_DESCRIPTOR,
                                       ListEntry);
    while (NextDescriptor->VirtualPage < Descriptor->VirtualPage)
    {
        /* Keep going... */
        NextEntry = NextEntry->Flink;
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           BL_MEMORY_DESCRIPTOR,
                                           ListEntry);

        /* If we hit the end of the list, just add it at the end */
        if (NextEntry == ListHead)
        {
            goto Quickie;
        }

        /* Otherwise, add it right after this descriptor */
        InsertTailList(&NextDescriptor->ListEntry, &Descriptor->ListEntry);
    }

Quickie:
    /* Release any global descriptors allocated */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return STATUS_SUCCESS;
}

NTSTATUS
MmTrInitialize (
    VOID
    )
{
    PBL_MEMORY_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;

    /* Nothing to track if we're using physical memory */
    if (MmTranslationType == BlNone)
    {
        return STATUS_SUCCESS;
    }

    /* Initialize all the virtual lists */
    MmMdInitializeListHead(&MmMdlMappingTrackers);
    MmMdlMappingTrackers.Type = BlMdTracker;
    MmMdInitializeListHead(&MmMdlFreeVirtual);
    MmMdlFreeVirtual.Type = BlMdVirtual;

    /* Initialize a 4GB free descriptor */
    Descriptor = MmMdInitByteGranularDescriptor(0,
                                                BlConventionalMemory,
                                                0,
                                                0,
                                                ((ULONGLONG)4 * 1024 * 1024 * 1024) >>
                                                PAGE_SHIFT);
    if (!Descriptor)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Add this 4GB region to the free virtual address space list */
    Status = MmMdAddDescriptorToList(&MmMdlFreeVirtual,
                                     Descriptor,
                                     BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG);
    if (!NT_SUCCESS(Status))
    {
        RtlZeroMemory(Descriptor, sizeof(*Descriptor));
        goto Quickie;
    }

    /* Remove any reserved regions of virtual address space */
    NextEntry = MmMdlReservedAllocated.First->Flink;
    while (NextEntry != MmMdlReservedAllocated.First)
    {
        /* Grab the descriptor and see if it's mapped */
        Descriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);
        if (Descriptor->VirtualPage)
        {
            EfiPrintf(L"Need to handle reserved allocation: %llx %llx\r\n",
                      Descriptor->VirtualPage, Descriptor->PageCount);
            EfiStall(100000);
            Status = STATUS_NOT_IMPLEMENTED;
            goto Quickie;
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Set success if we made it */
    Status = STATUS_SUCCESS;

Quickie:
    /* Return back to caller */
    return Status;
}

NTSTATUS
BlMmRemoveBadMemory (
    VOID
    )
{
    BOOLEAN AllowBad;
    NTSTATUS Status;
    PULONGLONG BadPages;
    ULONGLONG BadPageCount;

    /* First check if bad memory access is allowed */
    AllowBad = FALSE;
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdLibraryBoolean_AllowBadMemoryAccess,
                                    &AllowBad);
    if ((NT_SUCCESS(Status)) && (AllowBad))
    {
        /* No point checking the list if it is */
        return STATUS_SUCCESS;
    }

    /* Otherwise, check if there's a persisted bad page list */
    Status = BlpGetBootOptionIntegerList(BlpApplicationEntry.BcdData,
                                         BcdLibraryIntegerList_BadMemoryList,
                                         &BadPages,
                                         &BadPageCount,
                                         TRUE);
    if (NT_SUCCESS(Status))
    {
        EfiPrintf(L"Persistent bad page list not supported\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    /* All done here */
    return STATUS_SUCCESS;
}

NTSTATUS
BlMmMapPhysicalAddressEx (
    _In_ PVOID* VirtualAddress,
    _In_ ULONG Flags,
    _In_ ULONGLONG Size,
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    )
{
    NTSTATUS Status;
    PVOID MappingAddress;
    PHYSICAL_ADDRESS MappedAddress;
    PVOID MappedBase;
    ULONGLONG MapSize;
    UCHAR CacheAttributes;
    ULONGLONG BasePage, EndPage, MappedPage, FoundBasePage;
    ULONGLONG PageOffset, FoundPageCount;
    PBL_MEMORY_DESCRIPTOR Descriptor, NewDescriptor;
    PBL_MEMORY_DESCRIPTOR_LIST List;
    ULONG AddPages;

    /* Increase call depth */
    ++MmDescriptorCallTreeCount;

    /* Check if any parameters are missing */
    if (!(VirtualAddress) || !(Size))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check for fixed allocation without an actual address */
    if ((Flags & BlMemoryFixed) &&
        (PhysicalAddress.QuadPart == -1) &&
        !(*VirtualAddress))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check for invalid requirement flag, if one is present */
    if (((Flags & BlMemoryValidAllocationAttributes) != BlMemoryFixed) &&
        ((Flags & BlMemoryValidAllocationAttributes) != BlMemoryKernelRange) &&
        (Flags & BlMemoryValidAllocationAttributes))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check for invalid cache attribute flags */
    if (((Flags & BlMemoryValidCacheAttributeMask) - 1) &
         (Flags & BlMemoryValidCacheAttributeMask))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Select an address to map this at */
    Status = MmSelectMappingAddress(&MappingAddress,
                                    *VirtualAddress,
                                    Size,
                                    Flags & BlMemoryValidAllocationAttributes,
                                    Flags,
                                    PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Map the selected address, using the appropriate caching attributes */
    MappedAddress = PhysicalAddress;
    MapSize = Size;
    CacheAttributes = ((Flags & BlMemoryValidCacheAttributeMask) != 0x20) ?
                      (Flags & BlMemoryValidCacheAttributeMask) : 0;
    Status = MmMapPhysicalAddress(&MappedAddress,
                                  &MappingAddress,
                                  &MapSize,
                                  CacheAttributes);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Compute the final address where the mapping was made */
    MappedBase = (PVOID)(ULONG_PTR)((ULONG_PTR)MappingAddress +
                                    PhysicalAddress.QuadPart -
                                    MappedAddress.QuadPart);
    MappedAddress.QuadPart = (ULONG_PTR)MappedBase;

    /* Check if we're in physical or virtual mode */
    if (MmTranslationType == BlNone)
    {
        /* We are in physical mode -- just return this address directly */
        Status = STATUS_SUCCESS;
        *VirtualAddress = MappedBase;
        goto Quickie;
    }

    /* Remove the mapping address from the list of free virtual memory */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       (ULONG_PTR)MappingAddress >> PAGE_SHIFT,
                                       MapSize >> PAGE_SHIFT,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* And then add an entry for the fact we mapped it */
        Status = TrpGenerateMappingTracker(MappedBase,
                                           CacheAttributes,
                                           PhysicalAddress,
                                           MapSize);
    }

    /* Abandon if we didn't update the memory map successfully */
    if (!NT_SUCCESS(Status))
    {
        /* Unmap the virtual address so it can be used later */
        MmUnmapVirtualAddress(MappingAddress, &MapSize);
        goto Quickie;
    }

    /* Check if no real mapping into RAM was made */
    if (PhysicalAddress.QuadPart == -1)
    {
        /* Then we're done here */
        Status = STATUS_SUCCESS;
        *VirtualAddress = MappedBase;
        goto Quickie;
    }


    /* Loop over the entire allocation */
    BasePage = MappedAddress.QuadPart >> PAGE_SHIFT;
    EndPage = (MappedAddress.QuadPart + MapSize) >> PAGE_SHIFT;
    MappedPage = (ULONG_PTR)MappingAddress >> PAGE_SHIFT;
    do
    {
        /* Start with the unmapped allocated list */
        List = &MmMdlUnmappedAllocated;
        Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_UNMAPPED_ALLOCATED,
                                        BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                        BasePage);
        if (!Descriptor)
        {
            /* Try persistent next */
            List = &MmMdlPersistentMemory;
            Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_PERSISTENT_MEMORY,
                                            BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                            BasePage);
        }
        if (!Descriptor)
        {
            /* Try unmapped, unallocated, next */
            List = &MmMdlUnmappedUnallocated;
            Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_UNMAPPED_UNALLOCATED,
                                            BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                            BasePage);
        }
        if (!Descriptor)
        {
            /* Try reserved next */
            List = &MmMdlReservedAllocated;
            Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_RESERVED_ALLOCATED,
                                            BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                            BasePage);
        }

        /* Check if we have a descriptor */
        if (Descriptor)
        {
            /* Remove it from its list */
            MmMdRemoveDescriptorFromList(List, Descriptor);

            /* Check if it starts before our allocation */
            FoundBasePage = Descriptor->BasePage;
            if (FoundBasePage < BasePage)
            {
                /* Create a new descriptor to cover the gap before our allocation */
                PageOffset = BasePage - FoundBasePage;
                NewDescriptor = MmMdInitByteGranularDescriptor(Descriptor->Flags,
                                                               Descriptor->Type,
                                                               FoundBasePage,
                                                               0,
                                                               PageOffset);

                /* Insert it */
                MmMdAddDescriptorToList(List, NewDescriptor, 0);

                /* Adjust ours to ignore that piece */
                Descriptor->PageCount -= PageOffset;
                Descriptor->BasePage = BasePage;
            }

            /* Check if it goes beyond our allocation */
            FoundPageCount = Descriptor->PageCount;
            if (EndPage < (FoundPageCount + Descriptor->BasePage))
            {
                /* Create a new descriptor to cover the range after our allocation */
                PageOffset = EndPage - BasePage;
                NewDescriptor = MmMdInitByteGranularDescriptor(Descriptor->Flags,
                                                               Descriptor->Type,
                                                               EndPage,
                                                               0,
                                                               FoundPageCount -
                                                               PageOffset);

                /* Insert it */
                MmMdAddDescriptorToList(List, NewDescriptor, 0);

                /* Adjust ours to ignore that piece */
                Descriptor->PageCount = PageOffset;
            }

            /* Update the descriptor to be mapepd at this virtual page */
            Descriptor->VirtualPage = MappedPage;

            /* Check if this was one of the regular lists */
            if ((List != &MmMdlReservedAllocated) &&
                (List != &MmMdlPersistentMemory))
            {
                /* Was it allocated, or unallocated? */
                if (List != &MmMdlUnmappedAllocated)
                {
                    /* In which case use the unallocated mapped list */
                    List = &MmMdlMappedUnallocated;
                }
                else
                {
                    /* Insert it into the mapped list */
                    List = &MmMdlMappedAllocated;
                }
            }

            /* Add the descriptor that was removed, into the right list */
            MmMdAddDescriptorToList(List, Descriptor, 0);

            /* Add the pages this descriptor had */
            AddPages = Descriptor->PageCount;
        }
        else
        {
            /* Nope, so just add one page */
            AddPages = 1;
        }

        /* Increment the number of pages the descriptor had */
        MappedPage += AddPages;
        BasePage += AddPages;
    }
    while (BasePage < EndPage);

    /* We're done -- returned the address */
    Status = STATUS_SUCCESS;
    *VirtualAddress = MappedBase;

Quickie:
    /* Cleanup descriptors and reduce depth */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

NTSTATUS
MmUnmapVirtualAddress (
    _Inout_ PVOID* VirtualAddress,
    _Inout_ PULONGLONG Size
    )
{
    NTSTATUS Status;

    /* Make sure parameters were passed in and are valid */
    if ((VirtualAddress) && (Size) && (*Size <= 0xFFFFFFFF))
    {
        /* Nothing to do if translation isn't active */
        if (MmTranslationType == BlNone)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* We don't support virtual memory yet @TODO */
            EfiPrintf(L"unmap not yet implemented in %S\r\n", __FUNCTION__);
            EfiStall(1000000);
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }
    else
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* All done */
    return Status;
}

NTSTATUS
BlMmUnmapVirtualAddressEx (
    _In_ PVOID VirtualAddress,
    _In_ ULONGLONG Size
    )
{
    NTSTATUS Status;

    /* Increment call depth */
    ++MmDescriptorCallTreeCount;

    /* Make sure all parameters are there */
    if ((VirtualAddress) && (Size))
    {
        /* Unmap the virtual address */
        Status = MmUnmapVirtualAddress(&VirtualAddress, &Size);

        /* Check if we actually had a virtual mapping active */
        if ((NT_SUCCESS(Status)) && (MmTranslationType != BlNone))
        {
            /* We don't support virtual memory yet @TODO */
            EfiPrintf(L"not yet implemented in %S\r\n", __FUNCTION__);
            EfiStall(1000000);
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }
    else
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Cleanup descriptors and reduce depth */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}

BOOLEAN
BlMmTranslateVirtualAddress (
    _In_ PVOID VirtualAddress,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress
    )
{
    /* Make sure arguments are present */
    if (!(VirtualAddress) || !(PhysicalAddress))
    {
        return FALSE;
    }

    /* Do the architecture-specific translation */
    return MmArchTranslateVirtualAddress(VirtualAddress, PhysicalAddress, NULL);
}

NTSTATUS
BlpMmInitialize (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    NTSTATUS Status;

    /* Take a reference */
    MmDescriptorCallTreeCount = 1;

    /* Only support valid translation types */
    if ((TranslationType > BlPae) || (LibraryParameters->TranslationType > BlPae))
    {
        /* Bail out */
        EfiPrintf(L"Invalid translation types present\r\n");
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Initialize memory descriptors */
    MmMdInitialize(0, LibraryParameters);

    /* Remember the page type we came in with */
    MmOriginalTranslationType = TranslationType;

    /* Initialize the physical page allocator */
    Status = MmPaInitialize(MemoryData,
                            LibraryParameters->MinimumAllocationCount);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Initialize the memory tracker */
    Status = MmTrInitialize();
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"TR Mm init failed: %lx\r\n", Status);
        //MmArchDestroy();
        //MmPaDestroy(1);
        goto Quickie;
    }

    /* Initialize paging, large pages, self-mapping, PAE, if needed */
    Status = MmArchInitialize(1,
                              MemoryData,
                              TranslationType,
                              LibraryParameters->TranslationType);
    if (NT_SUCCESS(Status))
    {
        /* Save the newly active transation type */
        MmTranslationType = LibraryParameters->TranslationType;

        /* Initialize the heap allocator now */
        Status = MmHaInitialize(LibraryParameters->MinimumHeapSize,
                                LibraryParameters->HeapAllocationAttributes);
    }

    /* If Phase 1 init failed, bail out */
    if (!NT_SUCCESS(Status))
    {
        /* Kill everything set setup so far */
        EfiPrintf(L"Phase 1 Mm init failed: %lx\r\n", Status);
        //MmPaDestroy(0);
        //MmTrDestroy();
        //MmArchDestroy();
        //MmPaDestroy(1);
        goto Quickie;
    }

    /* Do we have too many descriptors? */
    if (LibraryParameters->DescriptorCount > 512)
    {
        /* Switch to using a dynamic buffer instead */
        EfiPrintf(L"Warning: too many descriptors\r\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
        //MmMdpSwitchToDynamicDescriptors(LibraryParameters->DescriptorCount);
    }

    /* Remove memory that the BCD says is bad */
    BlMmRemoveBadMemory();

    /* Now map all the memory regions as needed */
    Status = MmArchInitialize(2,
                              MemoryData,
                              TranslationType,
                              LibraryParameters->TranslationType);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the block allocator */
        Status = MmBaInitialize();
    }

    /* Check if anything in phase 2 failed */
    if (!NT_SUCCESS(Status))
    {
        /* Go back to static descriptors and kill the heap */
        EfiPrintf(L"Phase 2 Mm init failed: %lx\r\n", Status);
        //MmMdpSwitchToStaticDescriptors();
        //HapInitializationStatus = 0;
        //++MmDescriptorCallTreeCount;

        /* Destroy the Phase 1 initialization */
        //MmPaDestroy(0);
        //MmTrDestroy();
        //MmArchDestroy();
        //MmPaDestroy(1);
    }

Quickie:
    /* Free the memory descriptors and return the initialization state */
    MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}
