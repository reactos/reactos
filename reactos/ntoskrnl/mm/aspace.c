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

MADDRESS_SPACE MmKernelAddressSpace;

ULONGLONG Cycles;
ULONG TimeDelta;

/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
NTAPI
MmInitializeKernelAddressSpace(VOID)
{
    MmInitializeAddressSpace(NULL, &MmKernelAddressSpace);
}

NTSTATUS
NTAPI
MmInitializeAddressSpace(PEPROCESS Process,
                         PMADDRESS_SPACE AddressSpace)
{
    AddressSpace->MemoryAreaRoot = NULL;

    if (Process != NULL)
    {
        AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
        AddressSpace->Process = Process;
        AddressSpace->Lock = (PEX_PUSH_LOCK)&Process->AddressCreationLock;
        ExInitializePushLock((PULONG_PTR)AddressSpace->Lock);        
    }
    else
    {
        AddressSpace->LowestAddress = MmSystemRangeStart;
        AddressSpace->Process = NULL;
        AddressSpace->Lock = (PEX_PUSH_LOCK)&PsGetCurrentProcess()->AddressCreationLock;
        ExInitializePushLock((PULONG_PTR)AddressSpace->Lock);
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
    return STATUS_SUCCESS;
}

/* EOF */
