/* $Id: w32call.c,v 1.1 2002/06/18 22:03:48 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
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
#include <internal/ps.h>
#include <internal/ob.h>
#include <internal/pool.h>
#include <ntos/minmax.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct _NTW32CALL_SAVED_STATE
{
  ULONG SavedStackLimit;
  PVOID SavedStackBase;
  PVOID SavedInitialStack;
  PVOID CallerResult;
  PULONG CallerResultLength;
  PNTSTATUS CallbackStatus;
  PKTRAP_FRAME SavedTrapFrame;
} NTW32CALL_SAVED_STATE, *PNTW32CALL_SAVED_STATE;

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
NtCallbackReturn (PVOID		Result,
		  ULONG		ResultLength,
		  NTSTATUS	Status)
{
  PULONG OldStack;
  PETHREAD Thread;
  PNTSTATUS CallbackStatus;
  PULONG CallerResultLength;
  PVOID* CallerResult;
  PVOID InitialStack;
  PVOID StackBase;
  ULONG StackLimit;
  KIRQL oldIrql;
  PNTW32CALL_SAVED_STATE State;
  PKTRAP_FRAME SavedTrapFrame;

  Thread = PsGetCurrentThread();
  if (Thread->Tcb.CallbackStack == NULL)
    {
      return(STATUS_NO_CALLBACK_ACTIVE);
    }

  OldStack = (PULONG)Thread->Tcb.CallbackStack;
  Thread->Tcb.CallbackStack = NULL;

  /*
   * Get the values that NtW32Call left on the inactive stack for us.
   */
  State = (PNTW32CALL_SAVED_STATE)OldStack[0];
  CallbackStatus = State->CallbackStatus;
  CallerResultLength = State->CallerResultLength;
  CallerResult = State->CallerResult;
  InitialStack = State->SavedInitialStack;
  StackBase = State->SavedStackBase;
  StackLimit = State->SavedStackLimit;
  SavedTrapFrame = State->SavedTrapFrame;
  
  /*
   * Copy the callback status and the callback result to NtW32Call
   */
  *CallbackStatus = Status;
  if (CallerResult != NULL && CallerResultLength != NULL)
    {
      if (Result == NULL)
	{
	  *CallerResultLength = 0;
	}
      else
	{
	  *CallerResultLength = min(ResultLength, *CallerResultLength);
	  memcpy(*CallerResult, Result, *CallerResultLength);
	}
    }

  /*
   * Restore the old stack.
   */
  KeRaiseIrql(HIGH_LEVEL, &oldIrql);
  Thread->Tcb.InitialStack = InitialStack;
  Thread->Tcb.StackBase = StackBase;
  Thread->Tcb.StackLimit = StackLimit;
  Thread->Tcb.TrapFrame = SavedTrapFrame;
  KeGetCurrentKPCR()->TSS->Esp0 = (ULONG)Thread->Tcb.InitialStack;
  KeStackSwitchAndRet((PVOID)(OldStack + 1));

  /* Should never return. */
  KeBugCheck(0);
  return(STATUS_UNSUCCESSFUL);
}

VOID STATIC
PsFreeCallbackStackPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
			PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, 
			BOOLEAN Dirty)
{
  assert(SwapEntry == 0);
  if (PhysAddr.QuadPart  != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysAddr);
    }
}

VOID STATIC
PsFreeCallbackStack(PVOID StackLimit)
{
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   StackLimit,
		   MM_STACK_SIZE,
		   PsFreeCallbackStackPage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

PVOID STATIC
PsAllocateCallbackStack(ULONG StackSize)
{
  PVOID KernelStack = NULL;
  NTSTATUS Status;
  PMEMORY_AREA StackArea;
  ULONG i;

  StackSize = PAGE_ROUND_UP(StackSize);
  MmLockAddressSpace(MmGetKernelAddressSpace());
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_KERNEL_STACK,
			      &KernelStack,
			      StackSize,
			      0,
			      &StackArea,
			      FALSE);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());  
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to create thread stack\n");
      return(NULL);
    }
  for (i = 0; i < (StackSize / PAGESIZE); i++)
    {
      PHYSICAL_ADDRESS Page;
      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
      if (!NT_SUCCESS(Status))
	{
	  return(NULL);
	}
      Status = MmCreateVirtualMapping(NULL,
				      KernelStack + (i * PAGESIZE),
				      PAGE_EXECUTE_READWRITE,
				      Page,
				      TRUE);
    }
  return(KernelStack);
}

NTSTATUS STDCALL
NtW32Call (IN ULONG RoutineIndex,
	   IN PVOID Argument,
	   IN ULONG ArgumentLength,
	   OUT PVOID* Result OPTIONAL,
	   OUT PULONG ResultLength OPTIONAL)
{
  PETHREAD Thread;
  PVOID NewStack;
  ULONG StackSize;
  PKTRAP_FRAME NewFrame;
  PULONG UserEsp;
  KIRQL oldIrql;
  NTSTATUS CallbackStatus;
  NTW32CALL_SAVED_STATE SavedState;

  DPRINT1("NtW32Call(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
	  RoutineIndex, Argument, ArgumentLength);

  Thread = PsGetCurrentThread();
  if (Thread->Tcb.CallbackStack != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  /* Set up the new kernel and user environment. */
  StackSize = (ULONG)(Thread->Tcb.StackBase - Thread->Tcb.StackLimit);  
  NewStack = PsAllocateCallbackStack(StackSize);
  /* FIXME: Need to check whether we were interrupted from v86 mode. */
  memcpy(NewStack + StackSize - sizeof(KTRAP_FRAME), Thread->Tcb.TrapFrame,
	 sizeof(KTRAP_FRAME) - (4 * sizeof(DWORD)));
  NewFrame = (PKTRAP_FRAME)(NewStack + StackSize - sizeof(KTRAP_FRAME));
  NewFrame->Esp -= (ArgumentLength + (4 * sizeof(ULONG))); 
  NewFrame->Eip = (ULONG)LdrpGetSystemDllCallbackDispatcher();
  UserEsp = (PULONG)NewFrame->Esp;
  UserEsp[0] = 0;     /* Return address. */
  UserEsp[1] = RoutineIndex;
  UserEsp[2] = (ULONG)&UserEsp[4];
  UserEsp[3] = ArgumentLength;
  memcpy((PVOID)&UserEsp[4], Argument, ArgumentLength);

  /* Switch to the new environment and return to user-mode. */
  KeRaiseIrql(HIGH_LEVEL, &oldIrql);
  SavedState.SavedStackLimit = Thread->Tcb.StackLimit;
  SavedState.SavedStackBase = Thread->Tcb.StackBase;
  SavedState.SavedInitialStack = Thread->Tcb.InitialStack;
  SavedState.CallerResult = Result;
  SavedState.CallerResultLength = ResultLength;
  SavedState.CallbackStatus = &CallbackStatus;
  SavedState.SavedTrapFrame = Thread->Tcb.TrapFrame;
  Thread->Tcb.InitialStack = Thread->Tcb.StackBase = NewStack + StackSize;
  Thread->Tcb.StackLimit = (ULONG)NewStack;
  Thread->Tcb.KernelStack = NewStack + StackSize - sizeof(KTRAP_FRAME);
  KeGetCurrentKPCR()->TSS->Esp0 = (ULONG)Thread->Tcb.InitialStack;
  KePushAndStackSwitchAndSysRet((ULONG)&SavedState, Thread->Tcb.KernelStack);

  /* 
   * The callback return will have already restored most of the state we 
   * modified.
   */
  KeLowerIrql(PASSIVE_LEVEL);
  PsFreeCallbackStack(NewStack);
  return(CallbackStatus);
} 

/* EOF */
