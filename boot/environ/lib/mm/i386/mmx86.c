/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Library
* FILE:            boot/environ/lib/mm/i386/mmx86.c
* PURPOSE:         Boot Library Memory Manager x86-Specific Code
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

ULONG_PTR MmArchKsegBase;
ULONG_PTR MmArchKsegBias;
ULONG MmArchLargePageSize;
BL_ADDRESS_RANGE MmArchKsegAddressRange;
ULONG_PTR MmArchTopOfApplicationAddressSpace;
ULONG_PTR Mmx86SelfMapBase;

typedef VOID
(*PBL_MM_FLUSH_TLB) (
    VOID
    );

typedef VOID
(*PBL_MM_RELOCATE_SELF_MAP) (
    VOID
    );

PBL_MM_RELOCATE_SELF_MAP BlMmRelocateSelfMap;
PBL_MM_FLUSH_TLB BlMmFlushTlb;

ULONG MmDeferredMappingCount;

/* FUNCTIONS *****************************************************************/

VOID
MmArchNullFunction (
    VOID
    )
{
    /* Nothing to do */
    return;
}

NTSTATUS
Mmx86pMapMemoryRegions (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData
    )
{
    BOOLEAN DoDeferred;

    /* In phase 1 we don't initialize deferred mappings*/
    if (Phase == 1)
    {
        DoDeferred = 0;
    }
    else
    {
        /* Don't do anything if there's nothing to initialize */
        if (!MmDeferredMappingCount)
        {
            return STATUS_SUCCESS;
        }

        DoDeferred = 1;
    }

    if (DoDeferred)
    {
        EfiPrintf(L"Deferred todo\r\n");
    }

    EfiPrintf(L"Phase 1 TODO\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE RequestedTranslationType
    )
{
    NTSTATUS Status;

    /* For phase 2, just map deferred regions */
    if (Phase != 1)
    {
        return Mmx86pMapMemoryRegions(2, MemoryData);
    }

    /* What translation type are we switching to? */
    switch (RequestedTranslationType)
    {
        /* Physical memory */
        case BlNone:

            /* Initialize everything to default/null values */
            MmArchLargePageSize = 1;
            MmArchKsegBase = 0;
            MmArchKsegBias = 0;
            MmArchKsegAddressRange.Minimum = 0;
            MmArchKsegAddressRange.Maximum = (ULONGLONG)~0;
            MmArchTopOfApplicationAddressSpace = 0;
            Mmx86SelfMapBase = 0;

            /* Set stub functions */
            BlMmRelocateSelfMap = MmArchNullFunction;
            BlMmFlushTlb = MmArchNullFunction;

            /* Set success */
            Status = STATUS_SUCCESS;
            break;

        case BlVirtual:

            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case BlPae:

            Status = STATUS_NOT_SUPPORTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;

}
