/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/physical.c
 * PURPOSE:         Physical Memory Manipulation Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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
 * @unimplemented
 */
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(VOID)
{
    UNIMPLEMENTED;
    return 0;
}
