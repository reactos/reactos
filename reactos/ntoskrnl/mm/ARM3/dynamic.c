/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/dynamic.c
 * PURPOSE:         ARM Memory Manager Dynamic Physical Memory Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmAddPhysicalMemory (IN PPHYSICAL_ADDRESS StartAddress,
                     IN OUT PLARGE_INTEGER NumberOfBytes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMarkPhysicalMemoryAsBad(IN PPHYSICAL_ADDRESS StartAddress,
                          IN OUT PLARGE_INTEGER NumberOfBytes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMarkPhysicalMemoryAsGood(IN PPHYSICAL_ADDRESS StartAddress,
                           IN OUT PLARGE_INTEGER NumberOfBytes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmRemovePhysicalMemory(IN PPHYSICAL_ADDRESS StartAddress,
                       IN OUT PLARGE_INTEGER NumberOfBytes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(VOID)
{
    ULONG Size, i;
    PPHYSICAL_MEMORY_RANGE Entry, Buffer;
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Calculate how much memory we'll need
    //
    Size = sizeof(PHYSICAL_MEMORY_RANGE) * (MmPhysicalMemoryBlock->NumberOfRuns + 1);

    //
    // Allocate a copy
    //
    Entry = Buffer = ExAllocatePoolWithTag(NonPagedPool, Size, 'hPmM');
    if (!Buffer) return NULL;

    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    //
    // Make sure it hasn't changed before we had acquired the lock
    //
    ASSERT(Size == (sizeof(PHYSICAL_MEMORY_RANGE) *
                    (MmPhysicalMemoryBlock->NumberOfRuns + 1)));

    //
    // Now loop our block
    //
    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++)
    {
        //
        // Copy the data, but format it into bytes
        //
        Entry->BaseAddress.QuadPart = MmPhysicalMemoryBlock->Run[i].BasePage << PAGE_SHIFT;
        Entry->NumberOfBytes.QuadPart = MmPhysicalMemoryBlock->Run[i].PageCount << PAGE_SHIFT;
        Entry++;
    }

    //
    // Last entry is empty
    //
    Entry->BaseAddress.QuadPart = 0;
    Entry->NumberOfBytes.QuadPart = 0;

    //
    // Release the lock and return
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    return Buffer;
}
