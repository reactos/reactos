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

#include <ntoskrnl.h>
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

NTSTATUS STDCALL
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

  return(STATUS_SUCCESS);
}


VOID INIT_FUNCTION
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
   ExEventObjectType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExEventObjectType);
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


/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateEvent(OUT PHANDLE EventHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	      IN EVENT_TYPE EventType,
	      IN BOOLEAN InitialState)
{
   PKEVENT Event;
   HANDLE hEvent;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES SafeObjectAttributes;
   
   if (ObjectAttributes != NULL)
     {
       Status = MmCopyFromCaller(&SafeObjectAttributes, ObjectAttributes,
				 sizeof(OBJECT_ATTRIBUTES));
       if (!NT_SUCCESS(Status))
	 {
	   return(Status);
	 }
       ObjectAttributes = &SafeObjectAttributes;
     }

   Status = ObCreateObject(ExGetPreviousMode(),
			   ExEventObjectType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(KEVENT),
			   0,
			   0,
			   (PVOID*)&Event);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeInitializeEvent(Event,
		     EventType,
		     InitialState);

   Status = ObInsertObject ((PVOID)Event,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    &hEvent);
   ObDereferenceObject(Event);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = MmCopyToCaller(EventHandle, &hEvent, sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
     {
	ZwClose(hEvent);
	return(Status);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenEvent(OUT PHANDLE EventHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   HANDLE hEvent;

   DPRINT("ObjectName '%wZ'\n", ObjectAttributes->ObjectName);

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       &hEvent);
             
  if (!NT_SUCCESS(Status))
  {
    return(Status);
  }

   Status = MmCopyToCaller(EventHandle, &hEvent, sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
     {
       ZwClose(EventHandle);
       return(Status);
     }
     
   return(Status);
}


NTSTATUS STDCALL
NtPulseEvent(IN HANDLE EventHandle,
	     OUT PLONG PreviousState  OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;

   DPRINT("NtPulseEvent(EventHandle %x PreviousState %x)\n",
	  EventHandle, PreviousState);

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
	     OUT PVOID EventInformation,
	     IN ULONG EventInformationLength,
	     OUT PULONG ReturnLength  OPTIONAL)
{
   EVENT_BASIC_INFORMATION Info;
   PKEVENT Event;
   NTSTATUS Status;
   ULONG RetLen;

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

   Status = MmCopyToCaller(EventInformation, &Event,
			   sizeof(EVENT_BASIC_INFORMATION));
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(Event);
       return(Status);
     }

   if (ReturnLength != NULL)
     {
       RetLen = sizeof(EVENT_BASIC_INFORMATION);
       Status = MmCopyToCaller(ReturnLength, &RetLen, sizeof(ULONG));
       if (!NT_SUCCESS(Status))
         {
           ObDereferenceObject(Event);
           return(Status);
         }
     }

   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtResetEvent(IN HANDLE EventHandle,
	     OUT PLONG PreviousState  OPTIONAL)
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


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetEvent(IN HANDLE EventHandle,
	   OUT PLONG PreviousState  OPTIONAL)
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

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtTraceEvent(
	IN ULONG TraceHandle,
	IN ULONG Flags,
	IN ULONG TraceHeaderLength,
	IN struct _EVENT_TRACE_HEADER* TraceHeader
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
