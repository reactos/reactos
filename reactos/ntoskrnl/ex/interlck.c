/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/interlck.c
 * PURPOSE:         Implements interlocked functions 
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

INTERLOCKED_RESULT ExInterlockedDecrementLong(PLONG Addend,
					      PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

ULONG ExInterlockedExchangeUlong(PULONG Target,
				 ULONG Value,
				 PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

ULONG ExInterlockedAddUlong(PULONG Addend,
			    ULONG Increment,
			    PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

INTERLOCKED_RESULT ExInterlockedIncrementLong(PLONG Addend,
					      PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}
