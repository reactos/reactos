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

#define NDEBUG
#include <internal/debug.h>

extern VOID KeApcProlog(VOID);
static KSPIN_LOCK PiApcLock;

/* PROTOTYPES ****************************************************************/

VOID KeApcProlog3(PKAPC Apc);

/* FUNCTIONS *****************************************************************/

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
   PULONG Esp = (PULONG)UserContext->Esp;
   KIRQL oldlvl;
   
   DPRINT("KiTestAlert(Thread %x, UserContext %x)\n");
   KeAcquireSpinLock( &PiApcLock, &oldlvl );
   current_entry = Thread->ApcState.ApcListHead[1].Flink;
   
   if (current_entry == &Thread->ApcState.ApcListHead[1])
     {
	KeReleaseSpinLock( &PiApcLock, oldlvl );
	return(FALSE);
     }
   
   while (current_entry != &Thread->ApcState.ApcListHead[1])
     {
	Apc = CONTAINING_RECORD(current_entry, KAPC, ApcListEntry);
	
	DPRINT("Esp %x\n", Esp);
	DPRINT("Apc->NormalContext %x\n", Apc->NormalContext);
	DPRINT("Apc->SystemArgument1 %x\n", Apc->SystemArgument1);
	DPRINT("Apc->SystemArgument2 %x\n", Apc->SystemArgument2);
	DPRINT("UserContext->Eip %x\n", UserContext->Eip);
	
	Esp = Esp - 16;
	Esp[3] = (ULONG)Apc->SystemArgument2;
	Esp[2] = (ULONG)Apc->SystemArgument1;
	Esp[1] = (ULONG)Apc->NormalContext;
	Esp[0] = UserContext->Eip;
	UserContext->Eip = (ULONG)Apc->NormalRoutine;
	
	current_entry = current_entry->Flink;
	
	/*
	 * Now call for the kernel routine for the APC, which will free
	 * the APC data structure
	 */
	KeApcProlog3(Apc);
     }
   UserContext->Esp = (ULONG)Esp;
   InitializeListHead(&Thread->ApcState.ApcListHead[1]);
   return(TRUE);
}

VOID KeApcProlog2()
{
   PETHREAD Thread = PsGetCurrentThread();
   PLIST_ENTRY current;
   PKAPC Apc;
   KIRQL oldlvl;

   KeLowerIrql( APC_LEVEL );
   KeAcquireSpinLock( &PiApcLock, &oldlvl );
   while( !IsListEmpty( current ) )
   {
	   current = RemoveTailList( &(Thread->Tcb.ApcState.ApcListHead[0]) );
	   KeReleaseSpinLock( &PiApcLock, oldlvl );
	   Apc = CONTAINING_RECORD(current, KAPC, ApcListEntry);
	   KeApcProlog3(Apc);
	   KeAcquireSpinLock( &PiApcLock, &oldlvl );
   }
   KeReleaseSpinLock( &PiApcLock, oldlvl );
   Thread->Tcb.WaitStatus = STATUS_ALERTED;
   KeLowerIrql( PASSIVE_LEVEL );
}

VOID KeApcProlog3(PKAPC Apc)
/*
 * FUNCTION: This is called from the prolog proper (in assembly) to deliver
 * a kernel APC
 */
{  
   PKTHREAD Thread = KeGetCurrentThread();
   DPRINT("KeApcProlog2(Apc %x)\n",Apc);
   InterlockedIncrement( &(Thread->ApcState.KernelApcInProgress) );
   InterlockedDecrement( &(Thread->ApcState.KernelApcPending) );
   
   Apc->KernelRoutine(Apc,
		      &Apc->NormalRoutine,
		      &Apc->NormalContext,
		      &Apc->SystemArgument1,
		      &Apc->SystemArgument2);
   InterlockedDecrement( &(Thread->ApcState.KernelApcInProgress) );
}

VOID KeDeliverKernelApc( PKTHREAD TargetThread )
/*
 * FUNCTION: Simulates an interrupt on the target thread which will transfer
 * control to a kernel mode routine
 * Must be called while holding the PiApcLock
 */
{
   PULONG Stack;
   
   DPRINT("KeDeliverKernelApc(Apc %x)\n", Apc);
   if (TargetThread == KeGetCurrentThread())
     {	
	KeApcProlog2();
	return;
     }
   
   	TargetThread->Context.esp = TargetThread->Context.esp - 12;
	Stack = (PULONG)TargetThread->Context.esp;
	Stack[0] = TargetThread->Context.eip;
	Stack[1] = TargetThread->Context.cs;
	Stack[2] = TargetThread->Context.eflags;
	TargetThread->Context.eip = (ULONG)KeApcProlog;
   
   PsResumeThread(CONTAINING_RECORD(TargetThread,ETHREAD,Tcb),
		  NULL);   
}

VOID KeInsertQueueApc(PKAPC Apc, 
		      PVOID SystemArgument1,
		      PVOID SystemArgument2, 
		      UCHAR Mode)
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
   
   KeAcquireSpinLock( &PiApcLock, &oldlvl );
   
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
   
   DPRINT("TargetThread->KernelApcDisable %d\n", 
	  TargetThread->KernelApcDisable);
   DPRINT("Apc->KernelRoutine %x\n", Apc->KernelRoutine);
   if (Apc->ApcMode == KernelMode && TargetThread->KernelApcDisable >= 1 )
     if( TargetThread != PsGetCurrentThread() )
	 {
	   PsSuspendThread( CONTAINING_RECORD( TargetThread, ETHREAD, Tcb ), NULL, TRUE, KernelMode );
	   KeReleaseSpinLock( &PiApcLock, oldlvl );
	   if( TargetThread->Alertable && TargetThread->WaitIrql < APC_LEVEL )
		  KeDeliverKernelApc( TargetThread );
     }
	 else if( TargetThread->Alertable && TargetThread->WaitIrql < APC_LEVEL )
		  KeDeliverKernelApc( TargetThread );
   else
     {
	DPRINT("Queuing APC for later delivery\n");
     }
   if (Apc->ApcMode == UserMode && TargetThread->Alertable == TRUE &&
       TargetThread->WaitMode == UserMode)
     {
	NTSTATUS Status;
	
	DPRINT("Resuming thread for user APC\n");
	
	Status = STATUS_USER_APC;
	PsResumeThread((PETHREAD)TargetThread,
		       &Status);
     }
}

VOID KeInitializeApc(PKAPC Apc,
		     PKTHREAD Thread,
		     UCHAR StateIndex,
		     PKKERNEL_ROUTINE KernelRoutine,
		     PKRUNDOWN_ROUTINE RundownRoutine,
		     PKNORMAL_ROUTINE NormalRoutine,
		     UCHAR Mode,
		     PVOID Context)
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

VOID PiInitApcManagement()
{
	KeInitializeSpinLock( &PiApcLock );
}

