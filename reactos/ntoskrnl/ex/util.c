/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/misc.c
 * PURPOSE:         Executive Utility Functions
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ExUuidCreate(
    OUT UUID *Uuid
    )
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
ExVerifySuite(
    SUITE_TYPE SuiteType
    )
{
    if (SuiteType == Personal) return TRUE;
    return FALSE;
}

/* EOF */
