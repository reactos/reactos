/* $Id: critical.c,v 1.6 2002/09/07 15:12:56 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL KeEnterCriticalRegion (VOID)
{
   DPRINT("KeEnterCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable -= 1;
}

VOID STDCALL KeLeaveCriticalRegion (VOID)
{
   DPRINT("KeLeaveCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable += 1;
}

/* EOF */
