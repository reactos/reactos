/* $Id: access.c,v 1.2 2002/09/07 15:13:06 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Access rights handling functions
 * FILE:              ntoskrnl/se/access.c
 * PROGRAMER:         Eric Kohl <ekohl@rz-online.de>
 * REVISION HISTORY:
 *                    07/04/2000: Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ***************************************************************/

BOOLEAN
STDCALL
RtlAreAllAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	return ((GrantedAccess & DesiredAccess) == DesiredAccess);
}


BOOLEAN
STDCALL
RtlAreAnyAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	return ((GrantedAccess & DesiredAccess) != 0);
}


VOID
STDCALL
RtlMapGenericMask (
	PACCESS_MASK		AccessMask,
	PGENERIC_MAPPING	GenericMapping
	)
{
	if (*AccessMask & GENERIC_READ)
		*AccessMask |= GenericMapping->GenericRead;

	if (*AccessMask & GENERIC_WRITE)
		*AccessMask |= GenericMapping->GenericWrite;

	if (*AccessMask & GENERIC_EXECUTE)
		*AccessMask |= GenericMapping->GenericExecute;

	if (*AccessMask & GENERIC_ALL)
		*AccessMask |= GenericMapping->GenericAll;

	*AccessMask &= 0x0FFFFFFF;
}

/* EOF */
