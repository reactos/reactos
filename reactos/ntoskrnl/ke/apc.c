/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

extern VOID KeApcProlog(VOID);

/* FUNCTIONS *****************************************************************/

VOID KeApcProlog2(PKAPC Apc)
{
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
   
   TargetThread = Apc->Thread;
   
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
	TargetThread->Context.eip = KeApcProlog;
	TargetThread->Context.eax = (ULONG)Apc;
     }
   else
     {
	TargetThread->Context.esp = TargetThread->Context.esp - 40;
	Stack = (PULONG)TargetThread->Context.esp;
	Stack[9] = TargetThread->Context.ss;
	Stack[8] = TargetThread->Context.esp;
	Stack[7] = TargetThread->Context.gs;
	Stack[6] = TargetThread->Context.fs;
	Stack[5] = TargetThread->Context.ds;
	Stack[4] = TargetThread->Context.es;
	Stack[3] = TargetThread->Context.eflags;
	Stack[2] = TargetThread->Context.cs;
	Stack[1] = TargetThread->Context.eip;
	Stack[0] = TargetThread->Context.eax;
	TargetThread->Context.eip = KeApcProlog;
	TargetThread->Context.eax = (ULONG)Apc;
     }

   PsResumeThread(CONTAINING_RECORD(TargetThread,ETHREAD,Tcb));   
}

void KeInsertQueueApc(struct _KAPC *Apc, PVOID SystemArgument1,
		      PVOID SystemArgument2, UCHAR Mode)
{
   KIRQL oldlvl;
   
   DPRINT("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
	  "SystemArgument2 %x, Mode %d)\n",Apc,SystemArgument1,
	  SystemArgument2,Mode);
   
   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
   
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
   memset(Apc,0,sizeof(KAPC));
   Apc->Thread = Thread;
   Apc->ApcListEntry.Flink=NULL;
   Apc->ApcListEntry.Blink=NULL;
   Apc->KernelRoutine=KernelRoutine;
   Apc->RundownRoutine=RundownRoutine;
   Apc->NormalRoutine=NormalRoutine;
   Apc->NormalContext=Context;
   Apc->Inserted=FALSE;
   Apc->ApcStateIndex=StateIndex;
   Apc->ApcMode=Mode;
}


NTSTATUS STDCALL NtQueueApcThread(HANDLE ThreadHandle,
				  PKNORMAL_ROUTINE ApcRoutine,
				  PVOID NormalContext,
				  PVOID SystemArgument1,
				  PVOID SystemArgument2)
{
   return(NtQueueApcThread(ThreadHandle,
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
   UNIMPLEMENTED;
}
