/* $Id: ppool.c,v 1.2 2000/03/01 22:52:28 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/ppool.c
 * PURPOSE:         Implements the paged pool
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/pool.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME							INTERNAL
 *	ExAllocatePagedPoolWithTag@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
PVOID 
STDCALL
ExAllocatePagedPoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	)
{
	PVOID	Address = NULL;
	
	UNIMPLEMENTED; /* FIXME: */

	if (NULL == Address)
	{
		ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
	}
	return Address;
}


/* EOF */
