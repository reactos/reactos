/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: process.c,v 1.28 2004/10/13 01:42:14 ion Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Microkernel process management
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     No.
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
KeAttachProcess (PEPROCESS Process)
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   ULONG PageDir;
   
   DPRINT("KeAttachProcess(Process %x)\n",Process);
   
   CurrentThread = PsGetCurrentThread();

   if (&CurrentThread->ThreadsProcess->Pcb != CurrentThread->Tcb.ApcState.Process)
     {
	DPRINT1("Invalid attach (thread is already attached)\n");
	KEBUGCHECK(INVALID_PROCESS_ATTACH_ATTEMPT);
     }
   if (&Process->Pcb == CurrentThread->Tcb.ApcState.Process)
     {
	DPRINT1("Invalid attach (process is the same)\n");
	KEBUGCHECK(INVALID_PROCESS_ATTACH_ATTEMPT);
     }

   
   /* The stack and the thread structure of the current process may be 
      located in a page which is not present in the page directory of 
      the process we're attaching to. That would lead to a page fault 
      when this function returns. However, since the processor can't 
      call the page fault handler 'cause it can't push EIP on the stack, 
      this will show up as a stack fault which will crash the entire system.
      To prevent this, make sure the page directory of the process we're
      attaching to is up-to-date. */

   MmUpdatePageDir(Process, (PVOID)CurrentThread->Tcb.StackLimit, MM_STACK_SIZE);
   MmUpdatePageDir(Process, (PVOID)CurrentThread, sizeof(ETHREAD));

   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);

   KiSwapApcEnvironment(&CurrentThread->Tcb, &Process->Pcb);

   CurrentThread->Tcb.ApcState.Process = &Process->Pcb;
   PageDir = Process->Pcb.DirectoryTableBase.u.LowPart;
   DPRINT("Switching process context to %x\n",PageDir);
   Ke386SetPageTableDirectory(PageDir);
   KeLowerIrql(oldlvl);
}

/*
 * @implemented
 */
VOID STDCALL
KeDetachProcess (VOID)
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   ULONG PageDir;
   
   DPRINT("KeDetachProcess()\n");
   
   CurrentThread = PsGetCurrentThread();

   if (&CurrentThread->ThreadsProcess->Pcb == CurrentThread->Tcb.ApcState.Process)
     {
	DPRINT1("Invalid detach (thread was not attached)\n");
	KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);

   KiSwapApcEnvironment(&CurrentThread->Tcb, CurrentThread->Tcb.SavedApcState.Process);
   PageDir = CurrentThread->Tcb.ApcState.Process->DirectoryTableBase.u.LowPart;
   Ke386SetPageTableDirectory(PageDir);

   KeLowerIrql(oldlvl);
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeStackAttachProcess (
    IN PKPROCESS Process,
    OUT PRKAPC_STATE ApcState
    )
{
	KIRQL OldIrql;
	PKTHREAD Thread;
	
	Thread = KeGetCurrentThread();
	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Crash system if DPC is being executed! */
	if (KeIsExecutingDpc()) {
		DPRINT1("Invalid attach (Thread is executing a DPC!)\n");
		KEBUGCHECK(INVALID_PROCESS_ATTACH_ATTEMPT);
	}
	
	/* Check if the Target Process is already attached */
	if (Thread->ApcState.Process == Process) {
		ApcState->Process = (PKPROCESS)1;  /* Meaning already attached to the same Process */
	} else { 
		/* Check if the Current Thread is already attached */
		if (Thread->ApcStateIndex != 0) {
			KeAttachProcess((PEPROCESS)Process); /* FIXME: Re-write function to support stackability and fix it not to use EPROCESS */
		} else {
			KeAttachProcess((PEPROCESS)Process);
			ApcState->Process = NULL; /* FIXME: Re-write function to support stackability and fix it not to use EPROCESS */
		}
	}
	
	/* Return to old IRQL*/
	KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
KeUnstackDetachProcess (
    IN PRKAPC_STATE ApcState
    )
{
	KIRQL OldIrql;
	PKTHREAD Thread;
	ULONG PageDir;
	   
	/* If the special "We tried to attach to the process already being attached to" flag is there, don't do anything */
	if (ApcState->Process == (PKPROCESS)1) return;
	
	Thread = KeGetCurrentThread();
	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Sorry Buddy, can't help you if you've got APCs or just aren't attached */
	if ((Thread->ApcStateIndex == 0) || (Thread->ApcState.KernelApcInProgress)) {
		DPRINT1("Invalid detach (Thread not Attached, or Kernel APC in Progress!)\n");
		KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
	}
	
	/* Restore the Old APC State if a Process was present */
	if (ApcState->Process) {
		RtlMoveMemory(ApcState, &Thread->ApcState, sizeof(KAPC_STATE));
	} else {
		/* The ApcState parameter is useless, so use the saved data and reset it */
		RtlMoveMemory(&Thread->SavedApcState, &Thread->ApcState, sizeof(KAPC_STATE));
		Thread->SavedApcState.Process = NULL;
		Thread->ApcStateIndex = 0;
		Thread->ApcStatePointer[0] = &Thread->ApcState;
		Thread->ApcStatePointer[1] = &Thread->SavedApcState;
	}

	/* Do the Actual Swap */
	KiSwapApcEnvironment(Thread, Thread->SavedApcState.Process);
	PageDir = Thread->ApcState.Process->DirectoryTableBase.u.LowPart;
	Ke386SetPageTableDirectory(PageDir);
	
	/* Return to old IRQL*/
	KeReleaseDispatcherDatabaseLock(OldIrql);
}

/* EOF */
