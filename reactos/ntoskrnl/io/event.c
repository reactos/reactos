/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/event.c
 * PURPOSE:         Implements named events
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
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

   Status = ZwCreateEvent(&Handle,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  NotificationEvent,
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
   KPROCESSOR_MODE PreviousMode;
   PKEVENT Event;
   HANDLE Handle;
   NTSTATUS Status;

   PreviousMode = ExGetPreviousMode();

   InitializeObjectAttributes(&ObjectAttributes,
			      EventName,
			      OBJ_OPENIF,
			      NULL,
			      NULL);

   Status = ZwCreateEvent(&Handle,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  SynchronizationEvent,
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
