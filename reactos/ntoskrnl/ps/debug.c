/*
 *  ReactOS kernel
 *  Copyright (C) 2000, 1999, 1998 David Welch <welch@cwcom.net>, 
 *                                 Philip Susi <phreak@iag.net>
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
/* $Id: debug.c,v 1.4 2001/08/27 01:22:21 ekohl Exp $
 *
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/debug.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID
KeContextToTrapFrame(PCONTEXT Context,
		     PKTRAP_FRAME TrapFrame)
{
   if (Context->ContextFlags & CONTEXT_CONTROL)
     {
	TrapFrame->Esp = Context->Esp;
	TrapFrame->Ss = Context->SegSs;
	TrapFrame->Cs = Context->SegCs;
	TrapFrame->Eip = Context->Eip;
	TrapFrame->Eflags = Context->EFlags;	
	TrapFrame->Ebp = Context->Ebp;
     }
   if (Context->ContextFlags & CONTEXT_INTEGER)
     {
	TrapFrame->Eax = Context->Eax;
	TrapFrame->Ebx = Context->Ebx;
	TrapFrame->Ecx = Context->Ecx;
	/*
	 * Edx is used in the TrapFrame to hold the old trap frame pointer
	 * so we don't want to overwrite it here
	 */
/*	TrapFrame->Edx = Context->Edx; */
	TrapFrame->Esi = Context->Esi;
	TrapFrame->Edx = Context->Edi;
     }
   if (Context->ContextFlags & CONTEXT_SEGMENTS)
     {
	TrapFrame->Ds = Context->SegDs;
	TrapFrame->Es = Context->SegEs;
	TrapFrame->Fs = Context->SegFs;
	TrapFrame->Gs = Context->SegGs;
     }
   if (Context->ContextFlags & CONTEXT_FLOATING_POINT)
     {
	/*
	 * Not handled
	 */
     }
   if (Context->ContextFlags & CONTEXT_DEBUG_REGISTERS)
     {
	/*
	 * Not handled
	 */
     }
}

VOID
KeTrapFrameToContext(PKTRAP_FRAME TrapFrame,
		     PCONTEXT Context)
{
   if (Context->ContextFlags & CONTEXT_CONTROL)
     {
	Context->SegSs = TrapFrame->Ss;
	Context->Esp = TrapFrame->Esp;
	Context->SegCs = TrapFrame->Cs;
	Context->Eip = TrapFrame->Eip;
	Context->EFlags = TrapFrame->Eflags;
	Context->Ebp = TrapFrame->Ebp;
     }
   if (Context->ContextFlags & CONTEXT_INTEGER)
     {
	Context->Eax = TrapFrame->Eax;
	Context->Ebx = TrapFrame->Ebx;
	Context->Ecx = TrapFrame->Ecx;
	/*
	 * NOTE: In the trap frame which is built on entry to a system
	 * call TrapFrame->Edx will actually hold the address of the
	 * previous TrapFrame. I don't believe leaking this information
	 * has security implications. Also EDX holds the address of the
	 * arguments to the system call in progress so it isn't of much
	 * interest to the debugger.
	 */
	Context->Edx = TrapFrame->Edx;
	Context->Esi = TrapFrame->Esi;
	Context->Edi = TrapFrame->Edi;
     }
   if (Context->ContextFlags & CONTEXT_SEGMENTS)
     {
	Context->SegDs = TrapFrame->Ds;
	Context->SegEs = TrapFrame->Es;
	Context->SegFs = TrapFrame->Fs;
	Context->SegGs = TrapFrame->Gs;
     }
   if (Context->ContextFlags & CONTEXT_DEBUG_REGISTERS)
     {
	/*
	 * FIXME: Implement this case
	 */	
     }
   if (Context->ContextFlags & CONTEXT_FLOATING_POINT)
     {
	/*
	 * FIXME: Implement this case
	 */
     }
#if 0
   if (Context->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
     {
	/*
	 * FIXME: Investigate this
	 */
     }
#endif
}

VOID STDCALL
KeGetContextRundownRoutine(PKAPC Apc)
{
   PKEVENT Event;
   PNTSTATUS Status;
   
   Event = (PKEVENT)Apc->SystemArgument1;
   Status = (PNTSTATUS)Apc->SystemArgument2;
   (*Status) = STATUS_THREAD_IS_TERMINATING;
   KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

VOID STDCALL
KeGetContextKernelRoutine(PKAPC Apc,
			  PKNORMAL_ROUTINE* NormalRoutine,
			  PVOID* NormalContext,
			  PVOID* SystemArgument1,
			  PVOID* SystemArgument2)
/*
 * FUNCTION: This routine is called by an APC sent by NtGetContextThread to
 * copy the context of a thread into a buffer.
 */
{
   PKEVENT Event;
   PCONTEXT Context;
   PNTSTATUS Status;
   
   Context = (PCONTEXT)(*NormalContext);
   Event = (PKEVENT)(*SystemArgument1);
   Status = (PNTSTATUS)(*SystemArgument2);
   
   KeTrapFrameToContext(KeGetCurrentThread()->TrapFrame, Context);
   
   *Status = STATUS_SUCCESS;
   KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS STDCALL
NtGetContextThread(IN HANDLE ThreadHandle,
		   OUT PCONTEXT Context)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_GET_CONTEXT,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (Thread == PsGetCurrentThread())
     {
	/*
	 * I don't know if trying to get your own context makes much
	 * sense but we can handle it more efficently.
	 */
	
	KeTrapFrameToContext(Thread->Tcb.TrapFrame, Context);
	ObDereferenceObject(Thread);
	return(STATUS_SUCCESS);
     }
   else
     {
	KAPC Apc;
	KEVENT Event;
	NTSTATUS AStatus;
	CONTEXT KContext;
	
	KContext.ContextFlags = Context->ContextFlags;
	KeInitializeEvent(&Event,
			  NotificationEvent,
			  FALSE);	
	AStatus = STATUS_SUCCESS;
	
	KeInitializeApc(&Apc,
			&Thread->Tcb,
			0,
			KeGetContextKernelRoutine,
			KeGetContextRundownRoutine,
			NULL,
			KernelMode,
			(PVOID)&KContext);
	KeInsertQueueApc(&Apc,
			 (PVOID)&Event,
			 (PVOID)&AStatus,
			 0);
	Status = KeWaitForSingleObject(&Event,
				       0,
				       UserMode,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	if (!NT_SUCCESS(AStatus))
	  {
	     return(AStatus);
	  }
	memcpy(Context, &KContext, sizeof(CONTEXT));
	ObDereferenceObject(Thread);
	return(STATUS_SUCCESS);
     }
}

NTSTATUS STDCALL
NtSetContextThread(IN HANDLE ThreadHandle,
		   IN PCONTEXT Context)
{
   UNIMPLEMENTED;
}

/* EOF */
