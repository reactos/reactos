/* $Id: critical.c,v 1.8 2003/07/10 17:44:06 royce Exp $
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
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL KeEnterCriticalRegion (VOID)
{
   DPRINT("KeEnterCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable -= 1;
}

/*
 * @implemented
 */
VOID STDCALL KeLeaveCriticalRegion (VOID)
{
   DPRINT("KeLeaveCriticalRegion()\n");
   KeGetCurrentThread()->KernelApcDisable += 1;
}

/* EOF */
