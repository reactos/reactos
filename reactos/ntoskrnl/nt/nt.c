/* $Id: nt.c,v 1.6 2001/02/04 17:28:13 ekohl Exp $
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
}

/* EOF */
