/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/verifier.c
 * PURPOSE:         Driver Verifier functions
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
MmAddVerifierThunks (
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */

ULONG
STDCALL
MmIsDriverVerifying (
    IN struct _DRIVER_OBJECT *DriverObject
    )
{
	UNIMPLEMENTED;
	return 0;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmIsVerifierEnabled (
    OUT PULONG VerifierFlags
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
