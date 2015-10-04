/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/mm.c
 * PURPOSE:         Boot Library Memory Manager Core
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_TRANSLATION_TYPE MmTranslationType, MmOriginalTranslationType;
ULONG MmDescriptorCallTreeCount;

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmTrInitialize (
    VOID
    )
{
    /* Nothing to track if we're using physical memory */
    if (MmTranslationType == BlNone)
    {
        return STATUS_SUCCESS;
    }

    /* TODO */
    EfiPrintf(L"Required for protected mode\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlMmRemoveBadMemory (
    VOID
    )
{
    /* FIXME: Read BCD option to see what bad memory to remove */
    return STATUS_SUCCESS;
}

NTSTATUS
MmSelectMappingAddress (
    _Out_ PVOID* MappingAddress,
    _In_ ULONGLONG Size,
    _In_ ULONG AllocationAttributes,
    _In_ ULONG Flags,
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    )
{
    /* Are we in physical mode? */
    if (MmTranslationType == BlNone)
    {
        /* Just return the physical address as the mapping address */
        *MappingAddress = (PVOID)PhysicalAddress.LowPart;
        return STATUS_SUCCESS;
    }

    /* Have to allocate physical pages */
    EfiPrintf(L"VM Todo\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmMapPhysicalAddress (
    _Inout_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID VirtualAddress,
    _Inout_ PULONGLONG Size,
    _In_ ULONG CacheAttributes
    )
{
    ULONGLONG MappingSize;

    /* Fail if any parameters are missing */
    if (!(PhysicalAddress) || !(VirtualAddress) || !(Size))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Fail if the size is over 32-bits */
    MappingSize = *Size;
    if (MappingSize > 0xFFFFFFFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Nothing to do if we're in physical mode */
    if (MmTranslationType == BlNone)
    {
        return STATUS_SUCCESS;
    }

    /* Can't use virtual memory in real mode */
    if (CurrentExecutionContext->Mode == BlRealMode)
    {
        return STATUS_UNSUCCESSFUL;
    }

    EfiPrintf(L"VM todo\r\n");
    return STATUS_NOT_IMPLEMENTED;
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
        ((Flags & BlMemoryValidAllocationAttributes) != BlMemoryNonFixed) &&
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

    /* Compute the final adress where the mapping was made */
    MappedBase = (PVOID)((ULONG_PTR)MappingAddress +
                         PhysicalAddress.LowPart -
                         MappedAddress.LowPart);

    /* Check if we're in physical or virtual mode */
    if (MmTranslationType != BlNone)
    {
        /* For virtual memory, there's more to do */
        EfiPrintf(L"VM not supported for mapping\r\n");
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

        /* TODO */
        Status = STATUS_NOT_IMPLEMENTED;
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

    /* Make sure all parameters are tehre */
    if ((VirtualAddress) && (Size))
    {
        /* Unmap the virtual address */
        Status = MmUnmapVirtualAddress(&VirtualAddress, &Size);

        /* Check if we actually had a virtual mapping active */
        if ((NT_SUCCESS(Status)) && (MmTranslationType != BlNone))
        {
            /* TODO */
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
