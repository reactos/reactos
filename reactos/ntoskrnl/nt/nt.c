/* $Id: nt.c,v 1.10 2002/09/08 10:23:38 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/nt.c
 * PURPOSE:         Initialization of system call interfaces
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/nt.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID NtInit(VOID)
{
   NtInitializeEventImplementation();
   NtInitializeEventPairImplementation();
   NtInitializeMutantImplementation();
   NtInitializeSemaphoreImplementation();
   NtInitializeTimerImplementation();
   NiInitPort();
   NtInitializeProfileImplementation();
}

/* EOF */
