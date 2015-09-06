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
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
BlMmRemoveBadMemory (
    VOID
    )
{
    return STATUS_NOT_IMPLEMENTED;
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
        EarlyPrint(L"Invalid translation types present\n");
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
        ++MmDescriptorCallTreeCount;

        /* Destroy the Phase 1 initialization */
        //MmPaDestroy(0);
        //MmTrDestroy();
        //MmArchDestroy();
        //MmPaDestroy(1);
    }

Quickie:
    /* Free the memory descriptors and return the initialization state */
    //MmMdFreeGlobalDescriptors();
    --MmDescriptorCallTreeCount;
    return Status;
}
