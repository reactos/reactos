/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

HANDLE IdleThreadHandle = NULL;

/* FUNCTIONS *****************************************************************/

static VOID PsIdleThreadMain(PVOID Context)
{
   for(;;);
}

VOID PsInitIdleThread(VOID)
{
   PsCreateSystemThread(&IdleThreadHandle,
			0,
			NULL,
			NULL,
			NULL,
			PsIdleThreadMain,
			NULL);
}
