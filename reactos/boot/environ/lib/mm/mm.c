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
    MappedBase = (PVOID)((ULONG_PTR)MappingAddress +
                         PhysicalAddress.LowPart -
                         MappedAddress.LowPart);

    /* Check if we're in physical or virtual mode */
    if (MmTranslationType != BlNone)
    {
        /* We don't support virtual memory yet @TODO */
        EfiPrintf(L"not yet implemented in BlMmMapPhysicalAddressEx\r\n");
        EfiStall(1000000);
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }

    /* Return the mapped virtual address */
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
