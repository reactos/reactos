/* $Id: evtpair.c,v 1.21 2004/03/07 19:59:37 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Updated 09/08/2003 by Skywing (skywing@valhallalegends.com)
 *                   to correctly maintain ownership of the dispatcher lock
 *                   between KeSetEvent and KeWaitForSingleObject calls.
 *                   Additionally, implemented the thread-eventpair routines.
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <ntos/synch.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <limits.h>

#define NDEBUG
#include <internal/debug.h>

#ifndef NTSYSAPI
#define NTSYSAPI
#endif

#ifndef NTAPI
#define NTAPI STDCALL
#endif


/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventPairObjectType = NULL;

static GENERIC_MAPPING ExEventPairMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
	EVENT_PAIR_ALL_ACCESS};

static KSPIN_LOCK ExThreadEventPairSpinLock;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtpCreateEventPair(PVOID ObjectBody,
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
  
  return(STATUS_SUCCESS);
}

VOID INIT_FUNCTION
NtInitializeEventPairImplementation(VOID)
{
   ExEventPairObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventPairObjectType->TypeName, L"EventPair");
   ExEventPairObjectType->Tag = TAG('E', 'v', 'P', 'a');
   ExEventPairObjectType->MaxObjects = ULONG_MAX;
   ExEventPairObjectType->MaxHandles = ULONG_MAX;
   ExEventPairObjectType->TotalObjects = 0;
   ExEventPairObjectType->TotalHandles = 0;
   ExEventPairObjectType->PagedPoolCharge = 0;
   ExEventPairObjectType->NonpagedPoolCharge = sizeof(KEVENT_PAIR);
   ExEventPairObjectType->Mapping = &ExEventPairMapping;
   ExEventPairObjectType->Dump = NULL;
   ExEventPairObjectType->Open = NULL;
   ExEventPairObjectType->Close = NULL;
   ExEventPairObjectType->Delete = NULL;
   ExEventPairObjectType->Parse = NULL;
   ExEventPairObjectType->Security = NULL;
   ExEventPairObjectType->QueryName = NULL;
   ExEventPairObjectType->OkayToClose = NULL;
   ExEventPairObjectType->Create = NtpCreateEventPair;
   ExEventPairObjectType->DuplicationNotify = NULL;

   KeInitializeSpinLock(&ExThreadEventPairSpinLock);
   ObpCreateTypeObject(ExEventPairObjectType);
}


NTSTATUS STDCALL
NtCreateEventPair(OUT PHANDLE EventPairHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtCreateEventPair()\n");
   Status = ObCreateObject(ExGetPreviousMode(),
			   ExEventPairObjectType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(KEVENT_PAIR),
			   0,
			   0,
			   (PVOID*)&EventPair);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   KeInitializeEvent(&EventPair->LowEvent,
		     SynchronizationEvent,
		     FALSE);
   KeInitializeEvent(&EventPair->HighEvent,
		     SynchronizationEvent,
		     FALSE);

   Status = ObInsertObject ((PVOID)EventPair,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    EventPairHandle);

   ObDereferenceObject(EventPair);

   return Status;
}


NTSTATUS STDCALL
NtOpenEventPair(OUT PHANDLE EventPairHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;

   DPRINT("NtOpenEventPair()\n");

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventPairObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       EventPairHandle);

   return Status;
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
	      TRUE);

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
	      TRUE);

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

/*
 * Author: Skywing (skywing@valhallalegends.com), 09/08/2003
 * Note that the eventpair spinlock must be acquired when setting the thread
 * eventpair via NtSetInformationThread.
 * @implemented
 */
NTSTATUS
NTSYSAPI
NTAPI
NtSetLowWaitHighThread(
	VOID
	)
{
	PETHREAD Thread;
	PKEVENT_PAIR EventPair;
	NTSTATUS Status;
	KIRQL Irql;

	Thread = PsGetCurrentThread();

	if(!Thread->EventPair)
		return STATUS_NO_EVENT_PAIR;

	KeAcquireSpinLock(&ExThreadEventPairSpinLock, &Irql);

	EventPair = Thread->EventPair;

	if(EventPair)
		ObReferenceObjectByPointer(EventPair,
					   EVENT_PAIR_ALL_ACCESS,
					   ExEventPairObjectType,
					   UserMode);
	
	KeReleaseSpinLock(&ExThreadEventPairSpinLock, Irql);

	if(EventPair == NULL)
		return STATUS_NO_EVENT_PAIR;

	KeSetEvent(&EventPair->LowEvent,
		EVENT_INCREMENT,
		TRUE);

	Status = KeWaitForSingleObject(&EventPair->HighEvent,
				       WrEventPair,
				       UserMode,
				       FALSE,
				       NULL);

	ObDereferenceObject(EventPair);

	return Status;
}


/*
 * Author: Skywing (skywing@valhallalegends.com), 09/08/2003
 * Note that the eventpair spinlock must be acquired when setting the thread
 * eventpair via NtSetInformationThread.
 * @implemented
 */
NTSTATUS
NTSYSAPI
NTAPI
NtSetHighWaitLowThread(
	VOID
	)
{
	PETHREAD Thread;
	PKEVENT_PAIR EventPair;
	NTSTATUS Status;
	KIRQL Irql;

	Thread = PsGetCurrentThread();

	if(!Thread->EventPair)
		return STATUS_NO_EVENT_PAIR;

	KeAcquireSpinLock(&ExThreadEventPairSpinLock, &Irql);

	EventPair = PsGetCurrentThread()->EventPair;

	if(EventPair)
		ObReferenceObjectByPointer(EventPair,
					   EVENT_PAIR_ALL_ACCESS,
					   ExEventPairObjectType,
					   UserMode);
	
	KeReleaseSpinLock(&ExThreadEventPairSpinLock, Irql);

	if(EventPair == NULL)
		return STATUS_NO_EVENT_PAIR;

	KeSetEvent(&EventPair->HighEvent,
		EVENT_INCREMENT,
		TRUE);

	Status = KeWaitForSingleObject(&EventPair->LowEvent,
				       WrEventPair,
				       UserMode,
				       FALSE,
				       NULL);

	ObDereferenceObject(EventPair);

	return Status;
}

/*
 * Author: Skywing (skywing@valhallalegends.com), 09/08/2003
 * Note that the eventpair spinlock must be acquired when waiting on the
 * eventpair via NtSetLow/HighWaitHigh/LowThread.  Additionally, when
 * deleting a thread object, NtpSwapThreadEventPair(Thread, NULL) should
 * be called to release any preexisting eventpair object associated with
 * the thread.  The Microsoft name for this function is not known.
 */
VOID
ExpSwapThreadEventPair(
	IN PETHREAD Thread,
	IN PKEVENT_PAIR EventPair
	)
{
	PKEVENT_PAIR OriginalEventPair;
	KIRQL Irql;

	KeAcquireSpinLock(&ExThreadEventPairSpinLock, &Irql);

	OriginalEventPair = Thread->EventPair;
	Thread->EventPair = EventPair;

	if(OriginalEventPair)
		ObDereferenceObject(OriginalEventPair);

	KeReleaseSpinLock(&ExThreadEventPairSpinLock, Irql);
}

/* EOF */
