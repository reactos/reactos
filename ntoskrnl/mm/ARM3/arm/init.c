/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/init.c
 * PURPOSE:         ARM Memory Manager Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::INIT"
#define MODULE_INVOLVED_IN_ARM3
#include "../../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

ULONG MmMaximumNonPagedPoolPercent;
ULONG MmSizeOfNonPagedPoolInBytes;
ULONG MmMaximumNonPagedPoolInBytes;
PVOID MmNonPagedSystemStart;
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
PVOID MmNonPagedPoolEnd = (PVOID)0xFFBE0000;
ULONG MmNumberOfSystemPtes;
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Always return success for now
    //
    DPRINT1("NEVER TELL ME THE ODDS!\n");
    while (TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
