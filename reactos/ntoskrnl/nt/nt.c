/*
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
   NiInitPort();
}
