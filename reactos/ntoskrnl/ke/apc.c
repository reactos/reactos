/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  12/11/99:  Phillip Susi: Reworked the APC code
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KSPIN_LOCK PiApcLock;
extern KSPIN_LOCK PiThreadListLock;

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);

/* FUNCTIONS *****************************************************************/

VOID KeCallKernelRoutineApc(PKAPC Apc)
/*
 * FUNCTION: Call the kernel routine for an APC
 */
{  
   DPRINT("KeCallKernelRoutineApc(Apc %x)\n",Apc);
   
   Apc->KernelRoutine(Apc,
		      &Apc->NormalRoutine,
		      &Apc->NormalContext,
		      &Apc->SystemArgument1,
		      &Apc->SystemArgument2);
   DPRINT("Finished KeCallKernelRoutineApc()\n");
}

BOOLEAN KiTestAlert(PKTHREAD Thread,
		    PCONTEXT UserContext)
/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode.
 * ARGUMENTS:
 *        Thread = Thread to test for alerts
 *        UserContext = The user context saved on entry to kernel mode
 */
{
   PLIST_ENTRY current_entry;
   PKAPC Apc;
   PULONG Esp;
   KIRQL oldlvl;
   CONTEXT SavedContext;
   ULONG Top;
   BOOL ret = FALSE;
   PETHREAD EThread;
   
   DPRINT("KiTestAlert(Thread %x, UserContext %x)\n");
   while(1)
     {
       KeAcquireSpinLock(&PiApcLock, &oldlvl);
       current_entry = Thread->ApcState.ApcListHead[1].Flink;
       
       if (current_entry == &Thread->ApcState.ApcListHead[1])
	 {
	   KeReleaseSpinLock(&PiApcLock, oldlvl);
	   break;
	 }
       ret = TRUE;
       current_entry = RemoveHeadList(&Thread->ApcState.ApcListHead[1]);
       Apc = CONTAINING_RECORD(current_entry, KAPC, ApcListEntry);
       
       DPRINT("Esp %x\n", Esp);
       DPRINT("Apc->NormalContext %x\n", Apc->NormalContext);
       DPRINT("Apc->SystemArgument1 %x\n", Apc->SystemArgument1);
       DPRINT("Apc->SystemArgument2 %x\n", Apc->SystemArgument2);
       DPRINT("UserContext->Eip %x\n", UserContext->Eip);
       
       Esp = (PULONG)UserContext->Esp;
       
       memcpy(&SavedContext, UserContext, sizeof(CONTEXT));
       
       Esp = Esp - (sizeof(CONTEXT) + (5 * sizeof(ULONG)));
       memcpy(Esp, &SavedContext, sizeof(CONTEXT));
       Top = sizeof(CONTEXT) / 4;
       Esp[Top] = (ULONG)Apc->NormalRoutine;
       Esp[Top + 1] = (ULONG)Apc->NormalContext;
       Esp[Top + 2] = (ULONG)Apc->SystemArgument1;
       Esp[Top + 3] = (ULONG)Apc->SystemArgument2;
       Esp[Top + 4] = (ULONG)Esp - sizeof(CONTEXT);
       UserContext->Eip = 0;  // KiUserApcDispatcher
       
       KeReleaseSpinLock(&PiApcLock, oldlvl);
       
       /*
	* Now call for the kernel routine for the APC, which will free
	* the APC data structure
	*/
       KeCallKernelRoutineApc(Apc);
     }
   KeAcquireSpinLock(&PiThreadListLock, &oldlvl);
   EThread = CONTAINING_RECORD(Thread, ETHREAD, Tcb);
   if (EThread->DeadThread)
     {
       KeReleaseSpinLock(&PiThreadListLock, oldlvl);
       PsTerminateCurrentThread(EThread->ExitStatus);
     }
   else
     {
	KeReleaseSpinLock(&PiThreadListLock, oldlvl);
     }
   return ret;
}

VOID STDCALL KiDeliverApc(ULONG Unknown1,
			  ULONG Unknown2,
			  ULONG Unknown3)
{
   PETHREAD Thread = PsGetCurrentThread();
   PLIST_ENTRY current;
   PKAPC Apc;
   KIRQL oldlvl;

   DPRINT("KiDeliverApc()\n");
   KeAcquireSpinLock(&PiApcLock, &oldlvl);
   while(!IsListEmpty(&(Thread->Tcb.ApcState.ApcListHead[0])))
   {
      DPRINT("Delivering APC\n");
      current = RemoveTailList(&(Thread->Tcb.ApcState.ApcListHead[0]));
      Thread->Tcb.ApcState.KernelApcInProgress++;
      Thread->Tcb.ApcState.KernelApcPending--;
      KeReleaseSpinLock(&PiApcLock, oldlvl);
      
      Apc = CONTAINING_RECORD(current, KAPC, ApcListEntry);
      KeCallKernelRoutineApc(Apc);
      
      KeAcquireSpinLock(&PiApcLock, &oldlvl);
      DPRINT("Called kernel routine for APC\n");
//      PsFreezeThread(Thread, NULL, FALSE, KernelMode);
      DPRINT("Done frozen thread\n");
      Thread->Tcb.ApcState.KernelApcInProgress--;
   }
   KeReleaseSpinLock(&PiApcLock, oldlvl);
//   Thread->Tcb.WaitStatus = STATUS_KERNEL_APC;
}

VOID STDCALL KeInsertQueueApc (PKAPC	Apc,
			       PVOID	SystemArgument1,
			       PVOID	SystemArgument2,
			       UCHAR	Mode)
/*
 * FUNCTION: Queues an APC for execution
 * ARGUMENTS:
 *         Apc = APC to be queued
 *         SystemArgument[1-2] = TBD
 *         Mode = TBD
 */
{
   KIRQL oldlvl;
   PKTHREAD TargetThread;
   
   DPRINT("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
	  "SystemArgument2 %x, Mode %d)\n",Apc,SystemArgument1,
	  SystemArgument2,Mode);
   
   KeAcquireSpinLock(&PiApcLock, &oldlvl);
   
   Apc->SystemArgument1 = SystemArgument1;
   Apc->SystemArgument2 = SystemArgument2;
   
   if (Apc->Inserted)
     {
	DbgPrint("KeInsertQueueApc(): multiple APC insertations\n");
	KeBugCheck(0);
     }
   
   TargetThread = Apc->Thread;
   if (Apc->ApcMode == KernelMode)
     {
	InsertTailList(&TargetThread->ApcState.ApcListHead[0], 
		       &Apc->ApcListEntry);
	TargetThread->ApcState.KernelApcPending++;
     }
   else
     {
	InsertTailList(&TargetThread->ApcState.ApcListHead[1],
		       &Apc->ApcListEntry);
	TargetThread->ApcState.KernelApcPending++;
	TargetThread->ApcState.UserApcPending++;
     }
   Apc->Inserted = TRUE;
   
   if (Apc->ApcMode == KernelMode && TargetThread->KernelApcDisable >= 1 &&
       TargetThread->WaitIrql < APC_LEVEL)
     {
	KeRemoveAllWaitsThread(CONTAINING_RECORD(TargetThread, ETHREAD, Tcb),
			       STATUS_KERNEL_APC);
     }
   if (Apc->ApcMode == UserMode && TargetThread->Alertable == TRUE &&
       TargetThread->WaitMode == UserMode)
     {
	NTSTATUS Status;
	
	DPRINT("Resuming thread for user APC\n");
	
	Status = STATUS_USER_APC;
	KeRemoveAllWaitsThread(CONTAINING_RECORD(TargetThread, ETHREAD, Tcb),
			       STATUS_USER_APC);
     }
   KeReleaseSpinLock(&PiApcLock, oldlvl);
}

VOID STDCALL
KeInitializeApc (PKAPC			Apc,
		 PKTHREAD		Thread,
		 UCHAR			StateIndex,
		 PKKERNEL_ROUTINE	KernelRoutine,
		 PKRUNDOWN_ROUTINE	RundownRoutine,
		 PKNORMAL_ROUTINE	NormalRoutine,
		 UCHAR			Mode,
		 PVOID			Context)
/*
 * FUNCTION: Initialize an APC object
 * ARGUMENTS:
 *       Apc = Pointer to the APC object to initialized
 *       Thread = Thread the APC is to be delivered to
 *       StateIndex = TBD
 *       KernelRoutine = Routine to be called for a kernel-mode APC
 *       RundownRoutine = Routine to be called if the thread has exited with
 *                        the APC being executed
 *       NormalRoutine = Routine to be called for a user-mode APC
 *       Mode = APC mode
 *       Context = Parameter to be passed to the APC routine
 */
{   
   DPRINT("KeInitializeApc(Apc %x, Thread %x, StateIndex %d, "
	  "KernelRoutine %x, RundownRoutine %x, NormalRoutine %x, Mode %d, "
	  "Context %x)\n",Apc,Thread,StateIndex,KernelRoutine,RundownRoutine,
	  NormalRoutine,Mode,Context);
   memset(Apc, 0, sizeof(KAPC));
   Apc->Thread = Thread;
   Apc->ApcListEntry.Flink = NULL;
   Apc->ApcListEntry.Blink = NULL;
   Apc->KernelRoutine = KernelRoutine;
   Apc->RundownRoutine = RundownRoutine;
   Apc->NormalRoutine = NormalRoutine;
   Apc->NormalContext = Context;
   Apc->Inserted = FALSE;
   Apc->ApcStateIndex = StateIndex;
   if (Apc->NormalRoutine != NULL)
     {
	Apc->ApcMode = Mode;
     }
   else
     {
	Apc->ApcMode = KernelMode;
     }
}


NTSTATUS STDCALL NtQueueApcThread(HANDLE			ThreadHandle,
				  PKNORMAL_ROUTINE	ApcRoutine,
				  PVOID			NormalContext,
				  PVOID			SystemArgument1,
				  PVOID			SystemArgument2)
{
   PKAPC Apc;
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_ALL_ACCESS, /* FIXME */
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Apc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
   if (Apc == NULL)
     {
	ObDereferenceObject(Thread);
	return(STATUS_NO_MEMORY);
     }
   
   KeInitializeApc(Apc,
		   &Thread->Tcb,
		   0,
		   NULL,
		   NULL,
		   ApcRoutine,
		   UserMode,
		   NormalContext);
   KeInsertQueueApc(Apc,
		    SystemArgument1,
		    SystemArgument2,
		    UserMode);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtTestAlert(VOID)
{
   KiTestAlert(KeGetCurrentThread(),NULL);
   return(STATUS_SUCCESS);
}

VOID PiInitApcManagement(VOID)
{
   KeInitializeSpinLock(&PiApcLock);
}

