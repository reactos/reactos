/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
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
   
   current_entry = Thread->ApcState.ApcListHead[0].Flink;
   while (current_entry != &Thread->ApcState.ApcListHead[0])
     {
	Apc = CONTAINING_RECORD(current_entry, KAPC, ApcListEntry);
	
	Esp = Esp - 16;
	Esp[0] = (ULONG)Apc->SystemArgument2;
	Esp[1] = (ULONG)Apc->SystemArgument1;
	Esp[2] = (ULONG)Apc->NormalContext;
	Esp[3] = UserContext->Eip;
	UserContext->Eip = (ULONG)Apc->NormalRoutine;
	
	current_entry = current_entry->Flink;
     }
   return(TRUE);
}

VOID KeApcProlog2(PKAPC Apc)
/*
 * FUNCTION: This is called from the prolog proper (in assembly) to deliver
 * a kernel APC
 */
{
   DPRINT("KeApcProlog2(Apc %x)\n",Apc);
   Apc->KernelRoutine(Apc,
		      &Apc->NormalRoutine,
		      &Apc->NormalContext,
		      &Apc->SystemArgument2,
		      &Apc->SystemArgument2);
   PsSuspendThread(CONTAINING_RECORD(Apc->Thread,ETHREAD,Tcb));
}

VOID KeDeliverKernelApc(PKAPC Apc)
/*
 * FUNCTION: Simulates an interrupt on the target thread which will transfer
 * control to a kernel mode routine
 */
{
   PKTHREAD TargetThread;
   PULONG Stack;
   
   DPRINT("KeDeliverKernelApc(Apc %x)\n", Apc);
   
   TargetThread = Apc->Thread;

   if (TargetThread->KernelApcDisable <= 0)
     {
	DbgPrint("Queueing apc for thread %x\n", TargetThread);
	return;
     }
   
   if (TargetThread == KeGetCurrentThread())
     {	
	Apc->KernelRoutine(Apc,
			   &Apc->NormalRoutine,
			   &Apc->NormalContext,
			   &Apc->SystemArgument2,
			   &Apc->SystemArgument2);
	return;
     }
   
   if (TargetThread->Context.cs == KERNEL_CS)
     {
	TargetThread->Context.esp = TargetThread->Context.esp - 16;
	Stack = (PULONG)TargetThread->Context.esp;
	Stack[0] = TargetThread->Context.eax;
	Stack[1] = TargetThread->Context.eip;
	Stack[2] = TargetThread->Context.cs;
	Stack[3] = TargetThread->Context.eflags;
	TargetThread->Context.eip = (ULONG)KeApcProlog;
	TargetThread->Context.eax = (ULONG)Apc;
     }
   else
     {
	TargetThread->Context.esp = TargetThread->Context.esp - 40;
	Stack = (PULONG)TargetThread->Context.esp;
	Stack[9] = TargetThread->Context.gs;
	Stack[8] = TargetThread->Context.fs;
	Stack[7] = TargetThread->Context.ds;
	Stack[6] = TargetThread->Context.es;
	Stack[5] = TargetThread->Context.ss;
	Stack[4] = TargetThread->Context.esp;
	Stack[3] = TargetThread->Context.eflags;
	Stack[2] = TargetThread->Context.cs;
	Stack[1] = TargetThread->Context.eip;
	Stack[0] = TargetThread->Context.eax;
	TargetThread->Context.eip = (ULONG)KeApcProlog;
	TargetThread->Context.eax = (ULONG)Apc;
     }

   PsResumeThread(CONTAINING_RECORD(TargetThread,ETHREAD,Tcb));   
}

void KeInsertQueueApc(PKAPC Apc, 
		      PVOID SystemArgument1,
		      PVOID SystemArgument2, 
		      UCHAR Mode)
{
   KIRQL oldlvl;
   PKTHREAD TargetThread;
   
   DPRINT("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
	  "SystemArgument2 %x, Mode %d)\n",Apc,SystemArgument1,
	  SystemArgument2,Mode);
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   
   if (Apc->Inserted)
     {
	DbgPrint("KeInsertQueueApc(): multiple APC insertations\n");
	KeBugCheck(0);
     }
   
   TargetThread = Apc->Thread;
   InsertTailList(&TargetThread->ApcState.ApcListHead[0], &Apc->ApcListEntry);
   Apc->Inserted = TRUE;
   
   DPRINT("TargetThread->KernelApcDisable %d\n", 
	  TargetThread->KernelApcDisable);
   DPRINT("Apc->KernelRoutine %x\n", Apc->KernelRoutine);
   if (Apc->KernelRoutine != NULL)
     {
	KeDeliverKernelApc(Apc);
     }
   KeLowerIrql(oldlvl);
}

VOID KeInitializeApc(PKAPC Apc,
		     PKTHREAD Thread,
		     UCHAR StateIndex,
		     PKKERNEL_ROUTINE KernelRoutine,
		     PKRUNDOWN_ROUTINE RundownRoutine,
		     PKNORMAL_ROUTINE NormalRoutine,
		     UCHAR Mode,
		     PVOID Context)
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
   Apc->ApcMode = Mode;
}


NTSTATUS STDCALL NtQueueApcThread(HANDLE ThreadHandle,
				  PKNORMAL_ROUTINE ApcRoutine,
				  PVOID NormalContext,
				  PVOID SystemArgument1,
				  PVOID SystemArgument2)
{
   return(ZwQueueApcThread(ThreadHandle,
			   ApcRoutine,
			   NormalContext,
			   SystemArgument1,
			   SystemArgument2));
}

NTSTATUS STDCALL ZwQueueApcThread(HANDLE ThreadHandle,
				  PKNORMAL_ROUTINE ApcRoutine,
				  PVOID NormalContext,
				  PVOID SystemArgument1,
				  PVOID SystemArgument2)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtTestAlert(VOID)
{
   return(ZwTestAlert());
}

NTSTATUS STDCALL ZwTestAlert(VOID)
{
   KiTestAlert(KeGetCurrentThread(),
	       NULL /* ?? */);
   return(STATUS_SUCCESS);
}
