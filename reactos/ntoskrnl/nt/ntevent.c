/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
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
#include <internal/id.h>
#include <ntos/synch.h>
#include <internal/pool.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventObjectType = NULL;

static GENERIC_MAPPING ExpEventMapping = {
	STANDARD_RIGHTS_READ | SYNCHRONIZE | EVENT_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | SYNCHRONIZE | EVENT_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | EVENT_QUERY_STATE,
	EVENT_ALL_ACCESS};


/* FUNCTIONS *****************************************************************/

NTSTATUS
NtpCreateEvent(PVOID ObjectBody,
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


VOID
NtInitializeEventImplementation(VOID)
{
   ExEventObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventObjectType->TypeName, L"Event");
   
   ExEventObjectType->Tag = TAG('E', 'V', 'T', 'T');
   ExEventObjectType->MaxObjects = ULONG_MAX;
   ExEventObjectType->MaxHandles = ULONG_MAX;
   ExEventObjectType->TotalObjects = 0;
   ExEventObjectType->TotalHandles = 0;
   ExEventObjectType->PagedPoolCharge = 0;
   ExEventObjectType->NonpagedPoolCharge = sizeof(KEVENT);
   ExEventObjectType->Mapping = &ExpEventMapping;
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


NTSTATUS STDCALL
NtClearEvent(IN HANDLE EventHandle)
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


NTSTATUS STDCALL
NtCreateEvent(OUT PHANDLE UnsafeEventHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES UnsafeObjectAttributes,
	      IN BOOLEAN ManualReset,
	      IN BOOLEAN InitialState)
{
   PKEVENT Event;
   HANDLE EventHandle;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES SafeObjectAttributes;
   POBJECT_ATTRIBUTES ObjectAttributes;
   
   if (UnsafeObjectAttributes != NULL)
     {
       Status = MmCopyFromCaller(&SafeObjectAttributes, UnsafeObjectAttributes,
				 sizeof(OBJECT_ATTRIBUTES));
       if (!NT_SUCCESS(Status))
	 {
	   return(Status);
	 }
       ObjectAttributes = &SafeObjectAttributes;
     }
   else
     {
       ObjectAttributes = NULL;
     }

   Event = ObCreateObject(&EventHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ExEventObjectType);
   KeInitializeEvent(Event,
		     ManualReset ? NotificationEvent : SynchronizationEvent,
		     InitialState);
   ObDereferenceObject(Event);

   Status = MmCopyToCaller(UnsafeEventHandle, &EventHandle, sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
     {
       ZwClose(EventHandle);
       return(Status);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenEvent(OUT PHANDLE UnsafeEventHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   HANDLE EventHandle;

   DPRINT("ObjectName '%wZ'\n", ObjectAttributes->ObjectName);

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       &EventHandle);

   Status = MmCopyToCaller(UnsafeEventHandle, &EventHandle, sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
     {
       ZwClose(EventHandle);
       return(Status);
     }
   
   return(Status);
}


NTSTATUS STDCALL
NtPulseEvent(IN HANDLE EventHandle,
	     IN PULONG UnsafePulseCount OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;

   DPRINT("NtPulseEvent(EventHandle %x UnsafePulseCount %x)\n",
	  EventHandle, UnsafePulseCount);

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

   KePulseEvent(Event, EVENT_INCREMENT, FALSE);

   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtQueryEvent(IN HANDLE EventHandle,
	     IN EVENT_INFORMATION_CLASS EventInformationClass,
	     OUT PVOID UnsafeEventInformation,
	     IN ULONG EventInformationLength,
	     OUT PULONG UnsafeReturnLength)
{
   EVENT_BASIC_INFORMATION Info;
   PKEVENT Event;
   NTSTATUS Status;
   ULONG ReturnLength;

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
     Info.EventType = NotificationEvent;
   else
     Info.EventType = SynchronizationEvent;
   Info.EventState = KeReadStateEvent(Event);

   Status = MmCopyToCaller(UnsafeEventInformation, &Event, 
			   sizeof(EVENT_BASIC_INFORMATION));
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(Event);
       return(Status);
     }

   ReturnLength = sizeof(EVENT_BASIC_INFORMATION);
   Status = MmCopyToCaller(UnsafeReturnLength, &ReturnLength, sizeof(ULONG));
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(Event);
       return(Status);
     }

   ObDereferenceObject(Event);

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtResetEvent(IN HANDLE EventHandle,
	     OUT PULONG UnsafeNumberOfWaitingThreads OPTIONAL)
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


NTSTATUS STDCALL
NtSetEvent(IN HANDLE EventHandle,
	   OUT PULONG UnsafeNumberOfThreadsReleased)
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

/* EOF */
