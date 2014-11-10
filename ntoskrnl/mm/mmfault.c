/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmfault.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <cache/section/newmm.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
MmpAccessFault(KPROCESSOR_MODE Mode,
               ULONG_PTR Address,
               BOOLEAN FromMdl)
{
    PMMSUPPORT AddressSpace;
    MEMORY_AREA* MemoryArea;
    NTSTATUS Status;

    DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL was %u\n", KeGetCurrentIrql());
        return(STATUS_UNSUCCESSFUL);
    }

    /*
     * Find the memory area for the faulting address
     */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /*
         * Check permissions
         */
        if (Mode != KernelMode)
        {
            DPRINT1("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
            return(STATUS_ACCESS_VIOLATION);
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }
    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
        {
            if (!FromMdl)
            {
                MmUnlockAddressSpace(AddressSpace);
            }
            return (STATUS_ACCESS_VIOLATION);
        }

        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmAccessFaultSectionView(AddressSpace,
                                              MemoryArea,
                                              (PVOID)Address);
            break;

        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmAccessFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;

        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }
    return(Status);
}

NTSTATUS
NTAPI
MmNotPresentFault(KPROCESSOR_MODE Mode,
                  ULONG_PTR Address,
                  BOOLEAN FromMdl)
{
    PMMSUPPORT AddressSpace;
    MEMORY_AREA* MemoryArea;
    NTSTATUS Status;

    DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL was %u, address %x\n", KeGetCurrentIrql(), Address);
        return(STATUS_UNSUCCESSFUL);
    }

    /*
     * Find the memory area for the faulting address
     */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /*
         * Check permissions
         */
        if (Mode != KernelMode)
        {
            DPRINT1("Address: %x\n", Address);
            return(STATUS_ACCESS_VIOLATION);
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }

    /*
     * Call the memory area specific fault handler
     */
    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
        {
            if (!FromMdl)
            {
                MmUnlockAddressSpace(AddressSpace);
            }
            return (STATUS_ACCESS_VIOLATION);
        }

        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmNotPresentFaultSectionView(AddressSpace,
                                                  MemoryArea,
                                                  (PVOID)Address,
                                                  FromMdl);
            break;

        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmNotPresentFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;

        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }
    return(Status);
}

extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);

NTSTATUS
NTAPI
MmAccessFault(IN BOOLEAN StoreInstruction,
              IN PVOID Address,
              IN KPROCESSOR_MODE Mode,
              IN PVOID TrapInformation)
{
    PMEMORY_AREA MemoryArea = NULL;

    /* Cute little hack for ROS */
    if ((ULONG_PTR)Address >= (ULONG_PTR)MmSystemRangeStart)
    {
#ifdef _M_IX86
        /* Check for an invalid page directory in kernel mode */
        if (Mmi386MakeKernelPageTableGlobal(Address))
        {
            /* All is well with the world */
            return STATUS_SUCCESS;
        }
#endif
    }

    /* Is there a ReactOS address space yet? */
    if (MmGetKernelAddressSpace())
    {
        /* Check if this is an ARM3 memory area */
        MemoryArea = MmLocateMemoryAreaByAddress(MmGetKernelAddressSpace(), Address);
        if (!(MemoryArea) && (Address <= MM_HIGHEST_USER_ADDRESS))
        {
            /* Could this be a VAD fault from user-mode? */
            MemoryArea = MmLocateMemoryAreaByAddress(MmGetCurrentAddressSpace(), Address);
        }
    }

    /* Is this an ARM3 memory area, or is there no address space yet? */
    if (((MemoryArea) && (MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3)) ||
            (!(MemoryArea) && ((ULONG_PTR)Address >= (ULONG_PTR)MmPagedPoolStart)) ||
            (!MmGetKernelAddressSpace()))
    {
        /* This is an ARM3 fault */
        DPRINT("ARM3 fault %p\n", MemoryArea);
        return MmArmAccessFault(StoreInstruction, Address, Mode, TrapInformation);
    }

    /* Keep same old ReactOS Behaviour */
    if (StoreInstruction)
    {
        /* Call access fault */
        return MmpAccessFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
    else
    {
        /* Call not present */
        return MmNotPresentFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
}

