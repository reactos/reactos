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

#include <ddk/ntddk.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExEventType = NULL;

/* FUNCTIONS *****************************************************************/

NTSTATUS NtpCreateEvent(PVOID ObjectBody,
			PVOID Parent,
			PWSTR RemainingPath,
			POBJECT_ATTRIBUTES ObjectAttributes)
{
   
   DPRINT("NtpCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
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
   ANSI_STRING AnsiName;
   
   ExEventType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitAnsiString(&AnsiName,"Event");
   RtlAnsiStringToUnicodeString(&ExEventType->TypeName,&AnsiName,TRUE);
   
   ExEventType->MaxObjects = ULONG_MAX;
   ExEventType->MaxHandles = ULONG_MAX;
   ExEventType->TotalObjects = 0;
   ExEventType->TotalHandles = 0;
   ExEventType->PagedPoolCharge = 0;
   ExEventType->NonpagedPoolCharge = sizeof(KEVENT);
   ExEventType->Dump = NULL;
   ExEventType->Open = NULL;
   ExEventType->Close = NULL;
   ExEventType->Delete = NULL;
   ExEventType->Parse = NULL;
   ExEventType->Security = NULL;
   ExEventType->QueryName = NULL;
   ExEventType->OkayToClose = NULL;
   ExEventType->Create = NtpCreateEvent;
}

NTSTATUS STDCALL NtClearEvent (IN HANDLE EventHandle)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeClearEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtCreateEvent (OUT PHANDLE			EventHandle,
				IN ACCESS_MASK		DesiredAccess,
				IN POBJECT_ATTRIBUTES	ObjectAttributes,
				IN BOOLEAN			ManualReset,
				IN BOOLEAN	InitialState)
{
   PKEVENT Event;
   
   DPRINT("NtCreateEvent()\n");
   Event = ObCreateObject(EventHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ExEventType);
   KeInitializeEvent(Event, 
		     ManualReset ? NotificationEvent : SynchronizationEvent, 
		     InitialState );
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtOpenEvent (OUT PHANDLE			EventHandle,
			      IN ACCESS_MASK		DesiredAccess,
			      IN POBJECT_ATTRIBUTES	ObjectAttributes)
{
   NTSTATUS Status;
   PKEVENT Event;   

   
   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    ExEventType,
				    UserMode,
				    NULL,
				    (PVOID*)&Event);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Event,
			   DesiredAccess,
			   FALSE,
			   EventHandle);
   ObDereferenceObject(Event);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtPulseEvent(IN	HANDLE	EventHandle,
			      IN	PULONG	PulseCount	OPTIONAL)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtQueryEvent (IN	HANDLE	EventHandle,
			       IN	CINT	EventInformationClass,
			       OUT	PVOID	EventInformation,
			       IN	ULONG	EventInformationLength,
			       OUT	PULONG	ReturnLength)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtResetEvent(HANDLE	EventHandle,
			      PULONG	NumberOfWaitingThreads	OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   DPRINT("NtResetEvent(EventHandle %x)\n", EventHandle);
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeResetEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtSetEvent(IN	HANDLE	EventHandle,
			    PULONG NumberOfThreadsReleased)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   DPRINT("NtSetEvent(EventHandle %x)\n", EventHandle);
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeSetEvent(Event,IO_NO_INCREMENT,FALSE);
      
   
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}
