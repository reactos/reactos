/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/init.c
 * PURPOSE:         Memory Manager Initialization for amd64
 *
 * PROGRAMMERS:     Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "../ARM3/miarm.h"


/* GLOBALS *****************************************************************/

ULONG MmMaximumNonPagedPoolPercent;
ULONG MmSizeOfNonPagedPoolInBytes;
ULONG MmMaximumNonPagedPoolInBytes;

ULONG64 MmUserProbeAddress;
PVOID MmHighestUserAddress;
PVOID MmSystemRangeStart;

ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage = -1;

ULONG MmBootImageSize;

PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

RTL_BITMAP MiPfnBitMap;

PVOID MmNonPagedSystemStart;
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
PVOID MmNonPagedPoolEnd = MI_NONPAGED_POOL_END;

PVOID MmPagedPoolStart = MI_PAGED_POOL_START;
PVOID MmPagedPoolEnd;

ULONG MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;

PVOID MmSessionBase;
ULONG MmSessionSize;

PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;


NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

