/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/*
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>

/* EXTERN ********************************************************************/

extern VOID 
PiTimeoutThread(struct _KDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2);

/* FUNCTIONS *****************************************************************/

VOID 
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread)
/*
 * FUNCTION: Initialize the microkernel state of the thread
 */
{
   PVOID KernelStack;

#if 0   
   DbgPrint("Thread %x &Thread->MutantListHead %x &Thread->TrapFrame %x "
	    "&Thread->PreviousMode %x &Thread->DisableBoost %x\n",
	    Thread, &Thread->MutantListHead, &Thread->TrapFrame, 
	    &Thread->PreviousMode, &Thread->DisableBoost);
#endif   
   
   KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
                                InternalThreadType,
                                sizeof(ETHREAD),
                                FALSE);
   InitializeListHead(&Thread->MutantListHead);
   KernelStack = ExAllocatePool(NonPagedPool, MM_STACK_SIZE);
   Thread->InitialStack = KernelStack + MM_STACK_SIZE;
   Thread->StackLimit = (ULONG)KernelStack;
   /* 
    * The Native API function will initialize the TEB field later 
    */
   Thread->Teb = NULL;
   Thread->TlsArray = NULL;
   Thread->KernelStack = KernelStack + MM_STACK_SIZE;
   Thread->DebugActive = 0;
   Thread->State = THREAD_STATE_SUSPENDED;
   Thread->Alerted[0] = 0;
   Thread->Alerted[1] = 0;
   Thread->Iopl = 0;
   /*
    * FIXME: Think how this might work
    */
   Thread->NpxState = 0;
   Thread->Saturation = 0;
   Thread->Priority = 0; 
   InitializeListHead(&Thread->ApcState.ApcListHead[0]);
   InitializeListHead(&Thread->ApcState.ApcListHead[1]);
   Thread->ApcState.Process = Process;
   Thread->ApcState.KernelApcInProgress = 0;
   Thread->ApcState.KernelApcPending = 0;
   Thread->ApcState.UserApcPending = 0;
   Thread->ContextSwitches = 0;
   Thread->WaitStatus = STATUS_SUCCESS;
   Thread->WaitIrql = 0;
   Thread->WaitMode = 0;
   Thread->WaitNext = 0;
   Thread->WaitBlockList = NULL;
   Thread->WaitListEntry.Flink = NULL;
   Thread->WaitListEntry.Blink = NULL;
   Thread->WaitTime = 0;
   Thread->BasePriority = 0; 
   Thread->DecrementCount = 0;
   Thread->PriorityDecrement = 0;
   Thread->Quantum = 0;
   memset(Thread->WaitBlock, 0, sizeof(KWAIT_BLOCK)*4);
   Thread->LegoData = 0;
   /*
    * FIXME: Why this?
    */
   Thread->KernelApcDisable = 1;
   Thread->UserAffinity = 0;
   Thread->SystemAffinityActive = 0;
   Thread->Queue = NULL;
   KeInitializeSpinLock(&Thread->ApcQueueLock);
   memset(&Thread->Timer, 0, sizeof(KTIMER));
   Thread->QueueListEntry.Flink = NULL;
   Thread->QueueListEntry.Blink = NULL;
   Thread->Affinity = 0;
   Thread->Preempted = 0;
   Thread->ProcessReadyQueue = 0;
   Thread->KernelStackResident = 1;
   Thread->NextProcessor = 0;
   Thread->CallbackStack = NULL;
   Thread->Win32Thread = 0;
   Thread->TrapFrame = NULL;
   Thread->ApcStatePointer[0] = NULL;
   Thread->ApcStatePointer[1] = NULL;
   Thread->EnableStackSwap = 0;
   Thread->LargeStack = 0;
   Thread->ResourceIndex = 0;
   Thread->PreviousMode = KernelMode;
   Thread->KernelTime.QuadPart = 0;
   Thread->UserTime.QuadPart = 0;
   memset(&Thread->SavedApcState, 0, sizeof(KAPC_STATE));
   Thread->Alertable = 1;
   Thread->ApcStateIndex = 0;
   Thread->ApcQueueable = 0;
   Thread->AutoAlignment = 0;
   Thread->StackBase = KernelStack;
   memset(&Thread->SuspendApc, 0, sizeof(KAPC));
   KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 255);
   Thread->ThreadListEntry.Flink = NULL;
   Thread->ThreadListEntry.Blink = NULL;
   Thread->FreezeCount = 1;
   Thread->SuspendCount = 0;

   /*
    * Initialize ReactOS specific members
    */
   Thread->ProcessThreadListEntry.Flink = NULL;
   Thread->ProcessThreadListEntry.Blink = NULL;   
   KeInitializeDpc(&Thread->TimerDpc, (PKDEFERRED_ROUTINE)PiTimeoutThread, 
		   Thread);
   Thread->LastEip = 0;
   
   /*
    * Do x86 specific part
    */
         
}
