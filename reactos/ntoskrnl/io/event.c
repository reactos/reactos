/* $Id: event.c,v 1.8 2004/08/15 16:39:03 chorns Exp $
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

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PKEVENT STDCALL
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
     {
	return NULL;
     }

   ObReferenceObjectByHandle(Handle,
			     0,
			     ExEventObjectType,
			     KernelMode,
			     (PVOID*)&Event,
			     NULL);
   ObDereferenceObject(Event);

   *EventHandle = Handle;

   return Event;
}

/*
 * @implemented
 */
PKEVENT STDCALL
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
     {
	return NULL;
     }

   ObReferenceObjectByHandle(Handle,
			     0,
			     ExEventObjectType,
			     KernelMode,
			     (PVOID*)&Event,
			     NULL);
   ObDereferenceObject(Event);

   *EventHandle = Handle;

   return Event;
}

/* EOF */
