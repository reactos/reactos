/* $Id: nttimer.c,v 1.8 2001/02/18 19:43:15 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/nttimer.c
 * PURPOSE:         User-mode timers
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntos/synch.h>
#include <internal/ob.h>
#include <internal/ke.h>
#include <limits.h>


#include <internal/debug.h>


/* TYPES ********************************************************************/

typedef struct _NTTIMER
{
   KTIMER Timer;
   KDPC Dpc;
   KAPC Apc;
   BOOLEAN Running;
} NTTIMER, *PNTTIMER;


/* GLOBALS ******************************************************************/

POBJECT_TYPE ExTimerType = NULL;

static GENERIC_MAPPING ExpTimerMapping = {
	STANDARD_RIGHTS_READ | TIMER_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | TIMER_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
	TIMER_ALL_ACCESS};


/* FUNCTIONS *****************************************************************/

NTSTATUS
NtpCreateTimer(PVOID ObjectBody,
	       PVOID Parent,
	       PWSTR RemainingPath,
	       POBJECT_ATTRIBUTES ObjectAttributes)
{
   DPRINT("NtpCreateTimer(ObjectBody %x, Parent %x, RemainingPath %S)\n",
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
NtpTimerDpcRoutine(PKDPC Dpc,
		   PVOID DeferredContext,
		   PVOID SystemArgument1,
		   PVOID SystemArgument2)
{
   PNTTIMER Timer;
   
   DPRINT("NtpTimerDpcRoutine()\n");
   
   Timer = (PNTTIMER)DeferredContext;
   
   if ( Timer->Running )
     {
	KeInsertQueueApc(&Timer->Apc,
			 SystemArgument1,
			 SystemArgument2,
			 KernelMode);
     }
}


VOID
NtpTimerApcKernelRoutine(PKAPC Apc,
			 PKNORMAL_ROUTINE* NormalRoutine,
			 PVOID* NormalContext,
			 PVOID* SystemArgument1,
			 PVOID* SystemArguemnt2)
{
   DPRINT("NtpTimerApcKernelRoutine()\n");

}


VOID NtInitializeTimerImplementation(VOID)
{
   ExTimerType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExTimerType->TypeName, L"Timer");
   
   ExTimerType->MaxObjects = ULONG_MAX;
   ExTimerType->MaxHandles = ULONG_MAX;
   ExTimerType->TotalObjects = 0;
   ExTimerType->TotalHandles = 0;
   ExTimerType->PagedPoolCharge = 0;
   ExTimerType->NonpagedPoolCharge = sizeof(NTTIMER);
   ExTimerType->Mapping = &ExpTimerMapping;
   ExTimerType->Dump = NULL;
   ExTimerType->Open = NULL;
   ExTimerType->Close = NULL;
   ExTimerType->Delete = NULL;
   ExTimerType->Parse = NULL;
   ExTimerType->Security = NULL;
   ExTimerType->QueryName = NULL;
   ExTimerType->OkayToClose = NULL;
   ExTimerType->Create = NtpCreateTimer;
}


NTSTATUS STDCALL
NtCancelTimer(IN HANDLE TimerHandle,
	      OUT PBOOLEAN CurrentState OPTIONAL)
{
   PNTTIMER Timer;
   NTSTATUS Status;
   BOOLEAN State;
   KIRQL OldIrql;

   DPRINT("NtCancelTimer()\n");
   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      UserMode,
				      (PVOID*)&Timer,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;
   
   OldIrql = KeRaiseIrqlToDpcLevel();
   
   State = KeCancelTimer(&Timer->Timer);
   KeRemoveQueueDpc(&Timer->Dpc);
   KeRemoveQueueApc(&Timer->Apc);
   Timer->Running = FALSE;
  
   KeLowerIrql(OldIrql);
   ObDereferenceObject(Timer);
   
   if (CurrentState != NULL)
     {
	*CurrentState = State;
     }
   
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtCreateTimer(OUT PHANDLE TimerHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	      IN TIMER_TYPE TimerType)
{
   PNTTIMER Timer;
   
   DPRINT("NtCreateTimer()\n");
   Timer = ObCreateObject(TimerHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ExTimerType);
   
   KeInitializeTimerEx(&Timer->Timer,
		       TimerType);
   
   KeInitializeDpc (&Timer->Dpc,
		    (PKDEFERRED_ROUTINE)NtpTimerDpcRoutine,
		    (PVOID)Timer);
   
   Timer->Running = FALSE;
   
   ObDereferenceObject(Timer);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenTimer(OUT PHANDLE TimerHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   Status = ObOpenObjectByName(ObjectAttributes,
			       ExTimerType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       TimerHandle);
   return Status;
}


NTSTATUS STDCALL
NtQueryTimer(IN HANDLE TimerHandle,
	     IN CINT TimerInformationClass,
	     OUT PVOID TimerInformation,
	     IN ULONG Length,
	     OUT PULONG ResultLength)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtSetTimer(IN HANDLE TimerHandle,
	   IN PLARGE_INTEGER DueTime,
	   IN PTIMERAPCROUTINE TimerApcRoutine,
	   IN PVOID TimerContext,
	   IN BOOL WakeTimer,
	   IN ULONG Period OPTIONAL,
	   OUT PBOOLEAN PreviousState OPTIONAL)
{
   PNTTIMER Timer;
   NTSTATUS Status;
   BOOLEAN Result;
   KIRQL OldIrql;
   BOOLEAN State;

   DPRINT("NtSetTimer()\n");

   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      KeGetPreviousMode(),
				      (PVOID*)&Timer,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;

   State = KeReadStateTimer(&Timer->Timer);

   if (Timer->Running == TRUE)
     {
	/* cancel running timer */
	OldIrql = KeRaiseIrqlToDpcLevel();
	KeCancelTimer(&Timer->Timer);
	KeRemoveQueueDpc(&Timer->Dpc);
	KeRemoveQueueApc(&Timer->Apc);
	Timer->Running = FALSE;
	KeLowerIrql(OldIrql);
     }
   if( TimerApcRoutine )
      KeInitializeApc(&Timer->Apc,
		      KeGetCurrentThread(),
		      0,
		      (PKKERNEL_ROUTINE)NtpTimerApcKernelRoutine,
		      (PKRUNDOWN_ROUTINE)NULL,
		      (PKNORMAL_ROUTINE)TimerApcRoutine,
		      KeGetPreviousMode(),
		      TimerContext);

   Result = KeSetTimerEx (&Timer->Timer,
			  *DueTime,
			  Period,
			  TimerApcRoutine ? &Timer->Dpc : 0 );
   if (Result == TRUE)
     {
	ObDereferenceObject(Timer);
	DPRINT1( "KeSetTimer says the timer was already running, this shouldn't be\n" );
	return STATUS_UNSUCCESSFUL;
     }

   Timer->Running = TRUE;

   ObDereferenceObject(Timer);

   if (PreviousState != NULL)
     {
	*PreviousState = State;
     }

   return STATUS_SUCCESS;
}

/* EOF */
