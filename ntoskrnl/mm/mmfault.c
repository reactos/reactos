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
MmpAccessFault(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    ULONG_PTR Address,
    KPROCESSOR_MODE Mode)
{
    NTSTATUS Status;

    DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

    do
    {
        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmAccessFaultSectionView(AddressSpace,
                                            MemoryArea,
                                            (PVOID)Address);
        break;
#ifdef NEWCC
        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmAccessFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;
#endif
        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    } while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    return Status;
}

NTSTATUS
NTAPI
MmNotPresentFault(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    ULONG_PTR Address,
    KPROCESSOR_MODE Mode)
{
    NTSTATUS Status;

    DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

    /*
     * Call the memory area specific fault handler
     */
    do
    {
        if (MemoryArea->DeleteInProgress)
            return (STATUS_ACCESS_VIOLATION);

        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmNotPresentFaultSectionView(AddressSpace,
                                                  MemoryArea,
                                                  (PVOID)Address);
            break;
#ifdef NEWCC
        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmNotPresentFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;
#endif
        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    return(Status);
}

extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);

NTSTATUS
MmRosAccessFault(
    IN ULONG FaultCode,
    IN PMMSUPPORT AddressSpace,
    IN PMEMORY_AREA MemoryArea,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation)
{
    /* Keep same old ReactOS Behaviour */
    if (!MI_IS_NOT_PRESENT_FAULT(FaultCode))
    {
        /* Call access fault */
        return MmpAccessFault(AddressSpace,
                              MemoryArea,
                              (ULONG_PTR)Address,
                              Mode);
    }
    else
    {
        /* Call not present */
        return MmNotPresentFault(AddressSpace,
                                 MemoryArea,
                                 (ULONG_PTR)Address,
                                 Mode);
    }
}

