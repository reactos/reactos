/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/access.c
 * PURPOSE:         Access state functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeCreateAccessState(
	PACCESS_STATE AccessState,
	PVOID AuxData,
	ACCESS_MASK Access,
	PGENERIC_MAPPING GenericMapping
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SeDeleteAccessState(
	IN PACCESS_STATE AccessState
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SeSetAccessStateGenericMapping(
	PACCESS_STATE AccessState,
	PGENERIC_MAPPING GenericMapping
	)
{
	UNIMPLEMENTED;
}

/* EOF */
