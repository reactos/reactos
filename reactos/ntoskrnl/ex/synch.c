/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/synch.c
 * PURPOSE:         Synchronization Functions (Pushlocks)
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
PVOID
FASTCALL
ExfAcquirePushLockExclusive (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
	return NULL;
}

/*
 * @unimplemented
 */
PVOID
FASTCALL
ExfAcquirePushLockShared (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
	return NULL;
}

/*
 * @unimplemented
 */
PVOID
FASTCALL
ExfReleasePushLock (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
	return NULL;
}

/* EOF */
