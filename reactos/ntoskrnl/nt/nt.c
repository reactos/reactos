/* $Id: nt.c,v 1.7 2001/03/16 16:05:34 dwelch Exp $
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
   NtInitializeSemaphoreImplementation();
   NtInitializeTimerImplementation();
   NiInitPort();
   NtInitializeProfileImplementation();
}

/* EOF */
