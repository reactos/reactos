/* $Id: nt.c,v 1.9 2002/09/07 15:13:04 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
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
