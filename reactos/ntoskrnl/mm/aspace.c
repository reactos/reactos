/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/aspace.c
 * PURPOSE:         Manages address spaces
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeKernelAddressSpace)
#endif


/* GLOBALS ******************************************************************/

PMADDRESS_SPACE MmKernelAddressSpace;

ULONGLONG Cycles;
ULONG TimeDelta;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmInitializeAddressSpace(PEPROCESS Process,
                         PMADDRESS_SPACE AddressSpace)
{
    AddressSpace->MemoryAreaRoot = NULL;
    AddressSpace->Lock = (PEX_PUSH_LOCK)&Process->AddressCreationLock;
    ExInitializePushLock((PULONG_PTR)AddressSpace->Lock);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
    return STATUS_SUCCESS;
}

/* EOF */
