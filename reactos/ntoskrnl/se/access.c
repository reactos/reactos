/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/access.c
 * PURPOSE:         Access rights handling functions
 * 
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlAreAllAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	PAGED_CODE_RTL();

	return ((GrantedAccess & DesiredAccess) == DesiredAccess);
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlAreAnyAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	PAGED_CODE_RTL();

	return ((GrantedAccess & DesiredAccess) != 0);
}


/*
 * @implemented
 */
VOID
STDCALL
RtlMapGenericMask (
	PACCESS_MASK		AccessMask,
	PGENERIC_MAPPING	GenericMapping
	)
{
	PAGED_CODE_RTL();

	if (*AccessMask & GENERIC_READ)
		*AccessMask |= GenericMapping->GenericRead;

	if (*AccessMask & GENERIC_WRITE)
		*AccessMask |= GenericMapping->GenericWrite;

	if (*AccessMask & GENERIC_EXECUTE)
		*AccessMask |= GenericMapping->GenericExecute;

	if (*AccessMask & GENERIC_ALL)
		*AccessMask |= GenericMapping->GenericAll;

	*AccessMask &= ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

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
