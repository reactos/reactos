/* $Id: critical.c,v 1.4 2000/06/04 19:50:12 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/critical.c
 * PURPOSE:         Implement critical regions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
KeEnterCriticalRegion (
	VOID
	)
{
   DPRINT("KeEnterCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable -= 1;
}

VOID
STDCALL
KeLeaveCriticalRegion (
	VOID
	)
{
   DPRINT("KeLeaveCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable += 1;
}

/* EOF */
