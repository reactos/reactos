/* $Id$
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
   UNICODE_STRING CapturedEventName;
   KPROCESSOR_MODE PreviousMode;
   PKEVENT Event;
   HANDLE Handle;
   NTSTATUS Status;
   
   PreviousMode = ExGetPreviousMode();
   
   Status = RtlCaptureUnicodeString(&CapturedEventName,
                                    PreviousMode,
                                    NonPagedPool,
                                    FALSE,
                                    EventName);
   if (!NT_SUCCESS(Status))
     {
	return NULL;
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &CapturedEventName,
			      OBJ_OPENIF,
			      NULL,
			      NULL);

   Status = ZwCreateEvent(&Handle,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  SynchronizationEvent,
			  TRUE);

   RtlRelaseCapturedUnicodeString(&CapturedEventName,
                                  PreviousMode,
                                  FALSE);

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
