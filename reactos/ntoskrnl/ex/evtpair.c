/* $Id: evtpair.c 12779 2005-01-04 04:45:00Z gdalsnes $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/evtpair.c
 * PURPOSE:         Support for event pairs
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
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
ExpCreateEventPair(PVOID ObjectBody,
		   PVOID Parent,
		   PWSTR RemainingPath,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("ExpCreateEventPair(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  
  return(STATUS_SUCCESS);
}

VOID INIT_FUNCTION
ExpInitializeEventPairImplementation(VOID)
{
   ExEventPairObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventPairObjectType->TypeName, L"EventPair");
   ExEventPairObjectType->Tag = TAG('E', 'v', 'P', 'a');
   ExEventPairObjectType->PeakObjects = 0;
   ExEventPairObjectType->PeakHandles = 0;
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
   ExEventPairObjectType->Create = ExpCreateEventPair;
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
   HANDLE hEventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();

   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(EventPairHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObCreateObject(ExGetPreviousMode(),
			   ExEventPairObjectType,
			   ObjectAttributes,
			   PreviousMode,
			   NULL,
			   sizeof(KEVENT_PAIR),
			   0,
			   0,
			   (PVOID*)&EventPair);
   if(NT_SUCCESS(Status))
   {
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
			      &hEventPair);
     ObDereferenceObject(EventPair);
     
     if(NT_SUCCESS(Status))
     {
       _SEH_TRY
       {
         *EventPairHandle = hEventPair;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }
   return Status;
}


NTSTATUS STDCALL
NtOpenEventPair(OUT PHANDLE EventPairHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hEventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(EventPairHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventPairObjectType,
			       NULL,
			       PreviousMode,
			       DesiredAccess,
			       NULL,
			       &hEventPair);
   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *EventPairHandle = hEventPair;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
   
   return Status;
}


NTSTATUS STDCALL
NtSetHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtSetHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeSetEvent(&EventPair->HighEvent,
	        EVENT_INCREMENT,
	        FALSE);

     ObDereferenceObject(EventPair);
   }
   
   return Status;
}


NTSTATUS STDCALL
NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeSetEvent(&EventPair->HighEvent,
	        EVENT_INCREMENT,
	        TRUE);

     KeWaitForSingleObject(&EventPair->LowEvent,
			   WrEventPair,
			   PreviousMode,
			   FALSE,
			   NULL);

     ObDereferenceObject(EventPair);
   }
   
   return Status;
}


NTSTATUS STDCALL
NtSetLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtSetLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeSetEvent(&EventPair->LowEvent,
	        EVENT_INCREMENT,
	        FALSE);

     ObDereferenceObject(EventPair);
   }
   
   return Status;
}


NTSTATUS STDCALL
NtSetLowWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtSetLowWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeSetEvent(&EventPair->LowEvent,
	        EVENT_INCREMENT,
	        TRUE);

     KeWaitForSingleObject(&EventPair->HighEvent,
			   WrEventPair,
			   PreviousMode,
			   FALSE,
			   NULL);

     ObDereferenceObject(EventPair);
   }
   
   return Status;
}


NTSTATUS STDCALL
NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeWaitForSingleObject(&EventPair->LowEvent,
			   WrEventPair,
			   PreviousMode,
			   FALSE,
			   NULL);

     ObDereferenceObject(EventPair);
   }
   
   return Status;
}


NTSTATUS STDCALL
NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   PreviousMode = ExGetPreviousMode();
   
   Status = ObReferenceObjectByHandle(EventPairHandle,
				      SYNCHRONIZE,
				      ExEventPairObjectType,
				      PreviousMode,
				      (PVOID*)&EventPair,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeWaitForSingleObject(&EventPair->HighEvent,
			   WrEventPair,
			   PreviousMode,
			   FALSE,
			   NULL);

     ObDereferenceObject(EventPair);
   }

   return Status;
}

#ifdef _ENABLE_THRDEVTPAIR

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
	
	PreviousMode = ExGetPreviousMode();

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

#else /* !_ENABLE_THRDEVTPAIR */

NTSTATUS
NTSYSAPI
NTAPI
NtSetLowWaitHighThread(
	VOID
	)
{
        DPRINT1("NtSetLowWaitHighThread() not supported anymore (NT4 only)!\n");
        return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTSYSAPI
NTAPI
NtSetHighWaitLowThread(
	VOID
	)
{
        DPRINT1("NtSetHighWaitLowThread() not supported anymore (NT4 only)!\n");
        return STATUS_NOT_IMPLEMENTED;
}

#endif /* _ENABLE_THRDEVTPAIR */

/* EOF */
