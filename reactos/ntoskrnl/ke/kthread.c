/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
/* $Id: kthread.c,v 1.47 2004/06/23 22:31:51 ion Exp $
 *
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
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
STDCALL
VOID
KeCapturePersistentThreadState(
	IN PVOID	CurrentThread,
	IN ULONG	Setting1,
	IN ULONG	Setting2,
	IN ULONG	Setting3,
	IN ULONG	Setting4,
	IN ULONG	Setting5,
	IN PVOID	ThreadState
)
{
	UNIMPLEMENTED;
}

VOID
KeFreeStackPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
		PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
  assert(SwapEntry == 0);
  if (PhysAddr.QuadPart  != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysAddr);
    }
}

/*
 * @unimplemented
 */
STDCALL
KPRIORITY
KeQueryPriorityThread (
    IN PKTHREAD Thread
    )
{
	UNIMPLEMENTED;
	return 0;
}

NTSTATUS 
KeReleaseThread(PETHREAD Thread)
/*
 * FUNCTION: Releases the resource allocated for a thread by
 * KeInitializeThread
 * NOTE: The thread had better not be running when this is called
 */
{
  extern unsigned int init_stack;

  if (Thread->Tcb.StackLimit != (ULONG)&init_stack)
    {       
      MmLockAddressSpace(MmGetKernelAddressSpace());
      MmFreeMemoryArea(MmGetKernelAddressSpace(),
		       (PVOID)Thread->Tcb.StackLimit,
		       MM_STACK_SIZE,
		       KeFreeStackPage,
		       NULL);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
    }
  Thread->Tcb.StackLimit = 0;
  Thread->Tcb.InitialStack = NULL;
  Thread->Tcb.StackBase = NULL;
  Thread->Tcb.KernelStack = NULL;
  return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
STDCALL
BOOLEAN
KeSetKernelStackSwapEnable(
	IN BOOLEAN Enable
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

VOID
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread, BOOLEAN First)
/*
 * FUNCTION: Initialize the microkernel state of the thread
 */
{
  PVOID KernelStack;
  NTSTATUS Status;
  extern unsigned int init_stack_top;
  extern unsigned int init_stack;
  PMEMORY_AREA StackArea;
  ULONG i;
  PHYSICAL_ADDRESS BoundaryAddressMultiple;
  
  BoundaryAddressMultiple.QuadPart = 0;
  
  KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
			       InternalThreadType,
			       sizeof(ETHREAD),
			       FALSE);
  InitializeListHead(&Thread->MutantListHead);
  if (!First)
    {
      KernelStack = NULL;
      
      MmLockAddressSpace(MmGetKernelAddressSpace());
      Status = MmCreateMemoryArea(NULL,
				  MmGetKernelAddressSpace(),
				  MEMORY_AREA_KERNEL_STACK,
				  &KernelStack,
				  MM_STACK_SIZE,
				  0,
				  &StackArea,
				  FALSE,
				  FALSE,
				  BoundaryAddressMultiple);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("Failed to create thread stack\n");
	  KEBUGCHECK(0);
	}
      for (i = 0; i < (MM_STACK_SIZE / PAGE_SIZE); i++)
	{
	  PHYSICAL_ADDRESS Page;
	  Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
	  if (!NT_SUCCESS(Status))
	    {
	      KEBUGCHECK(0);
	    }
	  Status = MmCreateVirtualMapping(NULL,
					  (char*)KernelStack + (i * PAGE_SIZE),
					  PAGE_EXECUTE_READWRITE,
					  Page,
					  TRUE);
	}
      Thread->InitialStack = (char*)KernelStack + MM_STACK_SIZE;
      Thread->StackBase    = (char*)KernelStack + MM_STACK_SIZE;
      Thread->StackLimit   = (ULONG)KernelStack;
      Thread->KernelStack  = (char*)KernelStack + MM_STACK_SIZE;
    }
  else
    {
      Thread->InitialStack = (PVOID)&init_stack_top;
      Thread->StackBase = (PVOID)&init_stack_top;
      Thread->StackLimit = (ULONG)&init_stack;
      Thread->KernelStack = (PVOID)&init_stack_top;
    }
  
  /* 
   * The Native API function will initialize the TEB field later 
   */
  Thread->Teb = NULL;
  Thread->TlsArray = NULL;
  Thread->DebugActive = 0;
  Thread->State = THREAD_STATE_INITIALIZED;
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
//  Thread->KernelApcDisable = 1;
/*
It may be correct to have regular kmode APC disabled
until the thread has been fully created, BUT the problem is: they are 
currently never enabled again! So until somone figures out how 
this really work, I'm setting regular kmode APC's intially enabled.
-Gunnar

UPDATE: After enabling regular kmode APC's I have experienced random
crashes. I'm disabling it again, until we fix the APC implementation...
-Gunnar
*/

  Thread->KernelApcDisable = -1;

  
  Thread->UserAffinity = Process->Affinity;
  Thread->SystemAffinityActive = 0;
  Thread->PowerState = 0;
  Thread->NpxIrql = 0;
  Thread->ServiceTable = KeServiceDescriptorTable;
  Thread->Queue = NULL;
  KeInitializeSpinLock(&Thread->ApcQueueLock);
  memset(&Thread->Timer, 0, sizeof(KTIMER));
  KeInitializeTimer(&Thread->Timer);
  Thread->QueueListEntry.Flink = NULL;
  Thread->QueueListEntry.Blink = NULL;
  Thread->Affinity = Process->Affinity;
  Thread->Preempted = 0;
  Thread->ProcessReadyQueue = 0;
  Thread->KernelStackResident = 1;
  Thread->NextProcessor = 0;
  Thread->CallbackStack = NULL;
  Thread->Win32Thread = 0;
  Thread->TrapFrame = NULL;
  Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
  Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
  Thread->EnableStackSwap = 0;
  Thread->LargeStack = 0;
  Thread->ResourceIndex = 0;
  Thread->PreviousMode = KernelMode;
  Thread->KernelTime = 0;
  Thread->UserTime = 0;
  memset(&Thread->SavedApcState, 0, sizeof(KAPC_STATE));
  
  /* FIXME: is this correct? */
  Thread->Alertable = 1;
  
  Thread->ApcStateIndex = OriginalApcEnvironment;
  
  /* FIXME: not all thread are ApcQueueable! */
  Thread->ApcQueueable = TRUE;
  
  Thread->AutoAlignment = 0;
  KeInitializeApc(&Thread->SuspendApc,
		  Thread,
		  OriginalApcEnvironment,
		  PiSuspendThreadKernelRoutine,
		  PiSuspendThreadRundownRoutine,
		  PiSuspendThreadNormalRoutine,
		  KernelMode,
		  NULL);
  KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 128);
  Thread->ThreadListEntry.Flink = NULL;
  Thread->ThreadListEntry.Blink = NULL;
  Thread->FreezeCount = 0;
  Thread->SuspendCount = 0;
  
  /*
   * Initialize ReactOS specific members
   */
  Thread->ProcessThreadListEntry.Flink = NULL;
  Thread->ProcessThreadListEntry.Blink = NULL;
   
   /*
    * Do x86 specific part
    */
}

VOID STDCALL
KeRescheduleThread()
{
  PsDispatchThread(THREAD_STATE_READY);
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeRevertToUserAffinityThread(
    VOID
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
CCHAR
KeSetIdealProcessorThread (
	IN PKTHREAD Thread,
	IN CCHAR Processor
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeSetSystemAffinityThread(
    IN KAFFINITY Affinity
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
VOID 
KeTerminateThread(
	IN KPRIORITY   	 Increment  	 
)
{
	UNIMPLEMENTED;
}
