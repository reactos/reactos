/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/event.c
 * PURPOSE:         Named event support
 * PROGRAMMER:      Philip Susi and David Welch
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <ntos/synch.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventObjectType = NULL;

/* FUNCTIONS *****************************************************************/

NTSTATUS NtpCreateEvent(PVOID ObjectBody,
			PVOID Parent,
			PWSTR RemainingPath,
			POBJECT_ATTRIBUTES ObjectAttributes)
{
   
   DPRINT("NtpCreateEvent(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (Parent != NULL && RemainingPath != NULL)
     {
	ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
     }
   return(STATUS_SUCCESS);
}

VOID NtInitializeEventImplementation(VOID)
{
   ExEventObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventObjectType->TypeName, L"Event");
   
   ExEventObjectType->MaxObjects = ULONG_MAX;
   ExEventObjectType->MaxHandles = ULONG_MAX;
   ExEventObjectType->TotalObjects = 0;
   ExEventObjectType->TotalHandles = 0;
   ExEventObjectType->PagedPoolCharge = 0;
   ExEventObjectType->NonpagedPoolCharge = sizeof(KEVENT);
   ExEventObjectType->Dump = NULL;
   ExEventObjectType->Open = NULL;
   ExEventObjectType->Close = NULL;
   ExEventObjectType->Delete = NULL;
   ExEventObjectType->Parse = NULL;
   ExEventObjectType->Security = NULL;
   ExEventObjectType->QueryName = NULL;
   ExEventObjectType->OkayToClose = NULL;
   ExEventObjectType->Create = NtpCreateEvent;
}

NTSTATUS STDCALL NtClearEvent (IN HANDLE EventHandle)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeClearEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtCreateEvent (OUT PHANDLE		EventHandle,
				IN ACCESS_MASK		DesiredAccess,
				IN POBJECT_ATTRIBUTES	ObjectAttributes,
				IN BOOLEAN		ManualReset,
				IN BOOLEAN		InitialState)
{
   PKEVENT Event;

   DPRINT("NtCreateEvent()\n");
   Event = ObCreateObject(EventHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ExEventObjectType);
   KeInitializeEvent(Event, 
		     ManualReset ? NotificationEvent : SynchronizationEvent, 
		     InitialState );
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenEvent(OUT PHANDLE EventHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;

   DPRINT("ObjectName '%wZ'\n", ObjectAttributes->ObjectName);

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       EventHandle);
   
   return(Status);
}


NTSTATUS STDCALL NtPulseEvent(IN	HANDLE	EventHandle,
			      IN	PULONG	PulseCount	OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;

   DPRINT("NtPulseEvent(EventHandle %x PulseCount %x)\n",
	  EventHandle, PulseCount);

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KePulseEvent(Event,EVENT_INCREMENT,FALSE);

   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtQueryEvent (IN	HANDLE	EventHandle,
			       IN	EVENT_INFORMATION_CLASS	EventInformationClass,
			       OUT	PVOID	EventInformation,
			       IN	ULONG	EventInformationLength,
			       OUT	PULONG	ReturnLength)
{
   PEVENT_BASIC_INFORMATION Info;
   PKEVENT Event;
   NTSTATUS Status;

   Info = (PEVENT_BASIC_INFORMATION)EventInformation;

   if (EventInformationClass > EventBasicInformation)
     return STATUS_INVALID_INFO_CLASS;

   if (EventInformationLength < sizeof(EVENT_BASIC_INFORMATION))
     return STATUS_INFO_LENGTH_MISMATCH;

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_QUERY_STATE,
				      ExEventObjectType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;

   if (Event->Header.Type == InternalNotificationEvent)
     Info->EventType = NotificationEvent;
   else
     Info->EventType = SynchronizationEvent;
   Info->EventState = KeReadStateEvent(Event);

   *ReturnLength = sizeof(EVENT_BASIC_INFORMATION);

   ObDereferenceObject(Event);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL NtResetEvent(HANDLE	EventHandle,
			      PULONG	NumberOfWaitingThreads	OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   DPRINT("NtResetEvent(EventHandle %x)\n", EventHandle);
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeResetEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtSetEvent(IN	HANDLE	EventHandle,
			    OUT	PULONG	NumberOfThreadsReleased)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   DPRINT("NtSetEvent(EventHandle %x)\n", EventHandle);
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeSetEvent(Event,EVENT_INCREMENT,FALSE);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}
