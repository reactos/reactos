/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/usercall.c
 * PURPOSE:         2E interrupt handler
 * 
 * PROGRAMMERS:     David Welch (david.welch@seh.ox.ac.uk)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
KeUserModeCallback(
    IN ULONG	FunctionID,
    IN PVOID	InputBuffer,
    IN ULONG	InputLength,
    OUT PVOID	*OutputBuffer,
    OUT PULONG	OutputLength
)
{
	UNIMPLEMENTED;
	return 0;
}

/* EOF */
