/* $Id: event.c,v 1.2 2000/03/26 19:38:22 ea Exp $
 *
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

PKEVENT
STDCALL
IoCreateNotificationEvent(PUNICODE_STRING EventName,
				  PHANDLE EventHandle)
{
   UNIMPLEMENTED;
}

PKEVENT
STDCALL
IoCreateSynchronizationEvent(PUNICODE_STRING EventName,
				     PHANDLE EventHandle)
{
   UNIMPLEMENTED;
}


/* EOF */
