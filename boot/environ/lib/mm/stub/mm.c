/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/stub/mm.c
 * PURPOSE:         Boot Library Memory Manager Skeleton Code
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

BL_ADDRESS_RANGE MmArchKsegAddressRange;
ULONG_PTR MmArchTopOfApplicationAddressSpace;
ULONG MmArchLargePageSize;

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE RequestedTranslationType
    )
{
    EfiPrintf(L" MmArchInitialize NOT IMPLEMENTED for this platform\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmMapPhysicalAddress (
    _Inout_ PPHYSICAL_ADDRESS PhysicalAddressPtr,
    _Inout_ PVOID* VirtualAddressPtr,
    _Inout_ PULONGLONG SizePtr,
    _In_ ULONG CacheAttributes
    )
{
    EfiPrintf(L" MmMapPhysicalAddress NOT IMPLEMENTED for this platform\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
MmArchTranslateVirtualAddress (
    _In_ PVOID VirtualAddress,
    _Out_opt_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_opt_ PULONG CachingFlags
    )
{
    EfiPrintf(L" MmMapPhysicalAddress NOT IMPLEMENTED for this platform\r\n");
    return FALSE;
}