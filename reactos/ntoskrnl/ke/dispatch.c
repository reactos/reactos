/*
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  ntoskrnl/ke/dispatch.c
 * PURPOSE:               Handles a dispatch interrupt
 * PROGRAMMER:            David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>

/* FUNCTIONS ****************************************************************/

VOID KiDispatchInterrupt(ULONG irq)
/*
 * FUNCTION: Called after an irq when the interrupted processor was at a lower
 * level than DISPATCH_LEVEL
 */
{
   if (irq == 0)
     {
	KeExpireTimers();
     }
   KeDrainDpcQueue();
   PsDispatchThread();
}
