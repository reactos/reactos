/* $Id: ppool.c,v 1.3 2001/03/06 14:41:18 dwelch Exp $
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
PVOID STDCALL
ExAllocatePagedPoolWithTag (IN	POOL_TYPE	PoolType,
			    IN	ULONG		NumberOfBytes,
			    IN	ULONG		Tag)
{
  UNIMPLEMENTED; /* FIXME: */
  return(NULL);
}


/* EOF */
