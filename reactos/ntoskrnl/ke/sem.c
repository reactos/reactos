/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/sem.c
 * PURPOSE:         Implements kernel semaphores
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInitializeSemaphore(PKSEMAPHORE Semaphore,
			   LONG Count,
			   LONG Limit)
{
   UNIMPLEMENTED;
}

LONG KeReadStateSemaphore(PKSEMAPHORE Semaphore)
{
   UNIMPLEMENTED;
}

LONG KeReleaseSemaphore(PKSEMAPHORE Semaphore,
			KPRIORITY Increment,
			LONG Adjustment,
			BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

