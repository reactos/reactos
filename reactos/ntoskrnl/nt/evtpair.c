/* $Id: evtpair.c,v 1.5 2000/10/22 11:30:00 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <ntos/synch.h>
#include <limits.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventPairObjectType = NULL;


/* FUNCTIONS *****************************************************************/

NTSTATUS NtpCreateEventPair(PVOID ObjectBody,
			    PVOID Parent,
			    PWSTR RemainingPath,
			    POBJECT_ATTRIBUTES ObjectAttributes)
{
   DPRINT("NtpCreateEventPair(ObjectBody %x, Parent %x, RemainingPath %S)\n",
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

VOID NtInitializeEventPairImplementation(VOID)
{
   ExEventPairObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventObjectType->TypeName, L"EventPair");
   
   ExEventPairObjectType->MaxObjects = ULONG_MAX;
   ExEventPairObjectType->MaxHandles = ULONG_MAX;
   ExEventPairObjectType->TotalObjects = 0;
   ExEventPairObjectType->TotalHandles = 0;
   ExEventPairObjectType->PagedPoolCharge = 0;
   ExEventPairObjectType->NonpagedPoolCharge = sizeof(KEVENT_PAIR);
   ExEventPairObjectType->Dump = NULL;
   ExEventPairObjectType->Open = NULL;
   ExEventPairObjectType->Close = NULL;
   ExEventPairObjectType->Delete = NULL;
   ExEventPairObjectType->Parse = NULL;
   ExEventPairObjectType->Security = NULL;
   ExEventPairObjectType->QueryName = NULL;
   ExEventPairObjectType->OkayToClose = NULL;
   ExEventPairObjectType->Create = NtpCreateEventPair;
}


NTSTATUS STDCALL
NtCreateEventPair(OUT PHANDLE EventPairHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   PKEVENT_PAIR EventPair;

   DPRINT("NtCreateEventPair()\n");
   EventPair = ObCreateObject(EventPairHandle,
			      DesiredAccess,
			      ObjectAttributes,
			      ExEventPairObjectType);
   KeInitializeEvent(&EventPair->LowEvent,
		     SynchronizationEvent,
		     FALSE);
   KeInitializeEvent(&EventPair->HighEvent,
		     SynchronizationEvent,
		     FALSE);
   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenEventPair(OUT PHANDLE EventPairHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtOpenEventPair()\n");
   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    ExEventPairObjectType,
				    UserMode,
				    NULL,
				    (PVOID*)&EventPair);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = ObCreateHandle(PsGetCurrentProcess(),
			   EventPair,
			   DesiredAccess,
			   FALSE,
			   EventPairHandle);
   ObDereferenceObject(EventPair);

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->HighEvent,
	      EVENT_INCREMENT,
	      FALSE);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->HighEvent,
	      EVENT_INCREMENT,
	      FALSE);

   KeWaitForSingleObject(&EventPair->LowEvent,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->LowEvent,
	      EVENT_INCREMENT,
	      FALSE);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetLowWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetLowWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->LowEvent,
	      EVENT_INCREMENT,
	      FALSE);

   KeWaitForSingleObject(&EventPair->HighEvent,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeWaitForSingleObject(&EventPair->LowEvent,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeWaitForSingleObject(&EventPair->HighEvent,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}

/* EOF */
