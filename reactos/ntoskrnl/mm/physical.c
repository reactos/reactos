/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/physical.c
 * PURPOSE:         Physical Memory Manipulation functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 16/07/04
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmMarkPhysicalMemoryAsBad(
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmMarkPhysicalMemoryAsGood(
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PPHYSICAL_MEMORY_RANGE
STDCALL
MmGetPhysicalMemoryRanges (
    VOID
    )
{
	UNIMPLEMENTED;
	return 0;
}
