/* $Id: nttimer.c 12779 2005-01-04 04:45:00Z gdalsnes $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/timer.c
 * PURPOSE:         User-mode timers
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
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

static const INFORMATION_CLASS_INFO ExTimerInfoClass[] =
{
  ICI_SQ_SAME( sizeof(TIMER_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* TimerBasicInformation */
};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
ExpCreateTimer(PVOID ObjectBody,
	       PVOID Parent,
	       PWSTR RemainingPath,
	       POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("ExpCreateTimer(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID STDCALL
ExpDeleteTimer(PVOID ObjectBody)
{
   KIRQL OldIrql;
   PNTTIMER Timer = ObjectBody;

   DPRINT("ExpDeleteTimer()\n");

   OldIrql = KeRaiseIrqlToDpcLevel();

   KeCancelTimer(&Timer->Timer);
   KeRemoveQueueDpc(&Timer->Dpc);
   KeRemoveQueueApc(&Timer->Apc);
   Timer->Running = FALSE;

   KeLowerIrql(OldIrql);
}


VOID STDCALL
ExpTimerDpcRoutine(PKDPC Dpc,
		   PVOID DeferredContext,
		   PVOID SystemArgument1,
		   PVOID SystemArgument2)
{
   PNTTIMER Timer;

   DPRINT("ExpTimerDpcRoutine()\n");

   Timer = (PNTTIMER)DeferredContext;

   if ( Timer->Running )
     {
       KeInsertQueueApc(&Timer->Apc,
			SystemArgument1,
			SystemArgument2,
			IO_NO_INCREMENT);
     }
}


VOID STDCALL
ExpTimerApcKernelRoutine(PKAPC Apc,
			 PKNORMAL_ROUTINE* NormalRoutine,
			 PVOID* NormalContext,
			 PVOID* SystemArgument1,
			 PVOID* SystemArguemnt2)
{
   DPRINT("ExpTimerApcKernelRoutine()\n");

}


VOID INIT_FUNCTION
ExpInitializeTimerImplementation(VOID)
{
   ASSERT(!ExTimerType)
   ExTimerType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

   RtlCreateUnicodeString(&ExTimerType->TypeName, L"Timer");

   ExTimerType->Tag = TAG('T', 'I', 'M', 'T');
   ExTimerType->PeakObjects = 0;
   ExTimerType->PeakHandles = 0;
   ExTimerType->TotalObjects = 0;
   ExTimerType->TotalHandles = 0;
   ExTimerType->PagedPoolCharge = 0;
   ExTimerType->NonpagedPoolCharge = sizeof(NTTIMER);
   ExTimerType->Mapping = &ExpTimerMapping;
   ExTimerType->Dump = NULL;
   ExTimerType->Open = NULL;
   ExTimerType->Close = NULL;
   ExTimerType->Delete = ExpDeleteTimer;
   ExTimerType->Parse = NULL;
   ExTimerType->Security = NULL;
   ExTimerType->QueryName = NULL;
   ExTimerType->OkayToClose = NULL;
   ExTimerType->Create = ExpCreateTimer;
   ExTimerType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExTimerType);
}


NTSTATUS STDCALL
NtCancelTimer(IN HANDLE TimerHandle,
	      OUT PBOOLEAN CurrentState OPTIONAL)
{
   PNTTIMER Timer;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   DPRINT("NtCancelTimer(0x%x, 0x%x)\n", TimerHandle, CurrentState);
   
   if(CurrentState != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(CurrentState,
                     sizeof(BOOLEAN),
                     sizeof(BOOLEAN));
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

   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      PreviousMode,
				      (PVOID*)&Timer,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     BOOLEAN State;
     KIRQL OldIrql = KeRaiseIrqlToDpcLevel();

     State = KeCancelTimer(&Timer->Timer);
     KeRemoveQueueDpc(&Timer->Dpc);
     KeRemoveQueueApc(&Timer->Apc);
     Timer->Running = FALSE;

     KeLowerIrql(OldIrql);
     ObDereferenceObject(Timer);

     if(CurrentState != NULL)
     {
       _SEH_TRY
       {
         *CurrentState = State;
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
NtCreateTimer(OUT PHANDLE TimerHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	      IN TIMER_TYPE TimerType)
{
   PNTTIMER Timer;
   HANDLE hTimer;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   DPRINT("NtCreateTimer()\n");
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(TimerHandle,
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

   Status = ObCreateObject(PreviousMode,
			   ExTimerType,
			   ObjectAttributes,
			   PreviousMode,
			   NULL,
			   sizeof(NTTIMER),
			   0,
			   0,
			   (PVOID*)&Timer);
   if(NT_SUCCESS(Status))
   {
     KeInitializeTimerEx(&Timer->Timer,
		         TimerType);

     KeInitializeDpc(&Timer->Dpc,
		     &ExpTimerDpcRoutine,
		     Timer);

     Timer->Running = FALSE;

     Status = ObInsertObject ((PVOID)Timer,
			      NULL,
			      DesiredAccess,
			      0,
			      NULL,
			      &hTimer);
     ObDereferenceObject(Timer);
     
     if(NT_SUCCESS(Status))
     {
       _SEH_TRY
       {
         *TimerHandle = hTimer;
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
NtOpenTimer(OUT PHANDLE TimerHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hTimer;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtOpenTimer()\n");

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(TimerHandle,
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
			       ExTimerType,
			       NULL,
			       PreviousMode,
			       DesiredAccess,
			       NULL,
			       &hTimer);
   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *TimerHandle = hTimer;
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
NtQueryTimer(IN HANDLE TimerHandle,
	     IN TIMER_INFORMATION_CLASS TimerInformationClass,
	     OUT PVOID TimerInformation,
	     IN ULONG TimerInformationLength,
	     OUT PULONG ReturnLength  OPTIONAL)
{
   PNTTIMER Timer;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   DefaultQueryInfoBufferCheck(TimerInformationClass,
                               ExTimerInfoClass,
                               TimerInformation,
                               TimerInformationLength,
                               ReturnLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQueryTimer() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_QUERY_STATE,
				      ExTimerType,
				      PreviousMode,
				      (PVOID*)&Timer,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     switch(TimerInformationClass)
     {
       case TimerBasicInformation:
       {
         PTIMER_BASIC_INFORMATION BasicInfo = (PTIMER_BASIC_INFORMATION)TimerInformation;

         _SEH_TRY
         {
           /* FIXME - interrupt correction */
           BasicInfo->TimeRemaining.QuadPart = Timer->Timer.DueTime.QuadPart;
           BasicInfo->SignalState = (BOOLEAN)Timer->Timer.Header.SignalState;

           if(ReturnLength != NULL)
           {
             *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);
           }
         }
         _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
         _SEH_END;
         break;
       }

       default:
         Status = STATUS_NOT_IMPLEMENTED;
         break;
     }

     ObDereferenceObject(Timer);
   }

   return Status;
}


NTSTATUS STDCALL
NtSetTimer(IN HANDLE TimerHandle,
	   IN PLARGE_INTEGER DueTime,
	   IN PTIMER_APC_ROUTINE TimerApcRoutine  OPTIONAL,
	   IN PVOID TimerContext  OPTIONAL,
	   IN BOOLEAN ResumeTimer,
	   IN LONG Period  OPTIONAL,
	   OUT PBOOLEAN PreviousState  OPTIONAL)
{
   PNTTIMER Timer;
   BOOLEAN Result;
   BOOLEAN State;
   LARGE_INTEGER TimerDueTime;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtSetTimer()\n");

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(DueTime,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       TimerDueTime = *DueTime;

       if(PreviousState != NULL)
       {
         ProbeForWrite(PreviousState,
                       sizeof(BOOLEAN),
                       sizeof(BOOLEAN));
       }
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

   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      PreviousMode,
				      (PVOID*)&Timer,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       return Status;
     }

   State = KeReadStateTimer(&Timer->Timer);

   if (Timer->Running == TRUE)
     {
	/* cancel running timer */
	const KIRQL OldIrql = KeRaiseIrqlToDpcLevel();
	KeCancelTimer(&Timer->Timer);
	KeRemoveQueueDpc(&Timer->Dpc);
	KeRemoveQueueApc(&Timer->Apc);
	Timer->Running = FALSE;
	KeLowerIrql(OldIrql);
     }

   if (TimerApcRoutine)
     {
       KeInitializeApc(&Timer->Apc,
		       KeGetCurrentThread(),
		       OriginalApcEnvironment,
		       &ExpTimerApcKernelRoutine,
		       (PKRUNDOWN_ROUTINE)NULL,
		       (PKNORMAL_ROUTINE)TimerApcRoutine,
		       PreviousMode,
		       TimerContext);
     }

   Result = KeSetTimerEx(&Timer->Timer,
			 TimerDueTime,
			 Period,
			 TimerApcRoutine ? &Timer->Dpc : 0 );
   if (Result)
     {
	ObDereferenceObject(Timer);
	DPRINT1( "KeSetTimer says the timer was already running, this shouldn't be\n" );
	return STATUS_UNSUCCESSFUL;
     }

   Timer->Running = TRUE;

   ObDereferenceObject(Timer);

   if (PreviousState != NULL)
   {
     _SEH_TRY
     {
       *PreviousState = State;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   return Status;
}

/* EOF */
