/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/verifier.c
 * PURPOSE:         Driver Verifier functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
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
