/* $Id: event.c,v 1.3 2000/10/06 16:54:04 ekohl Exp $
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
#include <ntos.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PKEVENT
STDCALL
IoCreateNotificationEvent(PUNICODE_STRING EventName,
			  PHANDLE EventHandle)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEVENT Event;
   HANDLE Handle;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes,
			      EventName,
			      OBJ_OPENIF,
			      NULL,
			      NULL);

   Status = NtCreateEvent(&Handle,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  FALSE,
			  TRUE);
   if (!NT_SUCCESS(Status))
     return NULL;

   ObReferenceObjectByHandle(Handle,
			     0,
			     ExEventObjectType,
			     KernelMode,
			     &Event,
			     NULL);
   ObDereferenceObject(Event);

   *EventHandle = Handle;

   return Event;
}

PKEVENT
STDCALL
IoCreateSynchronizationEvent(PUNICODE_STRING EventName,
			     PHANDLE EventHandle)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEVENT Event;
   HANDLE Handle;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes,
			      EventName,
			      OBJ_OPENIF,
			      NULL,
			      NULL);

   Status = NtCreateEvent(&Handle,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  TRUE,
			  TRUE);
   if (!NT_SUCCESS(Status))
     return NULL;

   ObReferenceObjectByHandle(Handle,
			     0,
			     ExEventObjectType,
			     KernelMode,
			     &Event,
			     NULL);
   ObDereferenceObject(Event);

   *EventHandle = Handle;

   return Event;
}

/* EOF */
