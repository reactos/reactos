/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/event.c
 * PURPOSE:         Implements named events
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PKEVENT IoCreateNotificationEvent(PUNICODE_STRING EventName,
				  PHANDLE EventHandle)
{
   UNIMPLEMENTED;
}

PKEVENT IoCreateSynchronizationEvent(PUNICODE_STRING EventName,
				     PHANDLE EventHandle)
{
   UNIMPLEMENTED;
}

