/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/w32call.c
 * PURPOSE:         Thread managment
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined(__GNUC__)
/* void * alloca(size_t size); */
#elif defined(_MSC_VER)
void* _alloca(size_t size);
#else
#error Unknown compiler for alloca intrinsic stack allocation "function"
#endif

/* TYPES *******************************************************************/

typedef struct _NTW32CALL_SAVED_STATE
{
  ULONG_PTR SavedStackLimit;
  PVOID SavedStackBase;
  PVOID SavedInitialStack;
  PVOID CallerResult;
  PULONG CallerResultLength;
  PNTSTATUS CallbackStatus;
  PKTRAP_FRAME SavedTrapFrame;
  PVOID SavedCallbackStack;
  PVOID SavedExceptionStack;
} NTW32CALL_SAVED_STATE, *PNTW32CALL_SAVED_STATE;

typedef struct
{
  PVOID BaseAddress;
  LIST_ENTRY ListEntry;
} NTW32CALL_CALLBACK_STACK, *PNTW32CALL_CALLBACK_STACK;

KSPIN_LOCK CallbackStackListLock;
static LIST_ENTRY CallbackStackListHead;

/* FUNCTIONS ***************************************************************/

VOID INIT_FUNCTION
PsInitialiseW32Call(VOID)
{
  InitializeListHead(&CallbackStackListHead);
  KeInitializeSpinLock(&CallbackStackListLock);
}

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
  ULONG_PTR StackLimit;
  KIRQL oldIrql;
  PNTW32CALL_SAVED_STATE State;
  PKTRAP_FRAME SavedTrapFrame;
  PVOID SavedCallbackStack;
  PVOID SavedExceptionStack;
  
  PAGED_CODE();

  Thread = PsGetCurrentThread();
  if (Thread->Tcb.CallbackStack == NULL)
    {
      return(STATUS_NO_CALLBACK_ACTIVE);
    }

  OldStack = (PULONG)Thread->Tcb.CallbackStack;

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
  SavedCallbackStack = State->SavedCallbackStack;
  SavedExceptionStack = State->SavedExceptionStack;

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
  if ((Thread->Tcb.NpxState & NPX_STATE_VALID) &&
      ETHREAD_TO_KTHREAD(Thread) != KeGetCurrentKPCR()->PrcbData.NpxThread)
    {
      memcpy((char*)InitialStack - sizeof(FX_SAVE_AREA),
             (char*)Thread->Tcb.InitialStack - sizeof(FX_SAVE_AREA),
             sizeof(FX_SAVE_AREA));
    }
  Thread->Tcb.InitialStack = InitialStack;
  Thread->Tcb.StackBase = StackBase;
  Thread->Tcb.StackLimit = StackLimit;
  Thread->Tcb.TrapFrame = SavedTrapFrame;
  Thread->Tcb.CallbackStack = SavedCallbackStack;
  KeGetCurrentKPCR()->TSS->Esp0 = (ULONG)SavedExceptionStack;
  KeStackSwitchAndRet((PVOID)(OldStack + 1));

  /* Should never return. */
  KEBUGCHECK(0);
  return(STATUS_UNSUCCESSFUL);
}

VOID STATIC
PsFreeCallbackStackPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
			PFN_TYPE Page, SWAPENTRY SwapEntry, 
			BOOLEAN Dirty)
{
  ASSERT(SwapEntry == 0);
  if (Page != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
    }
}

VOID STATIC
PsFreeCallbackStack(PVOID StackLimit)
{
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
		        StackLimit,
		        PsFreeCallbackStackPage,
		        NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

VOID
PsFreeCallbackStacks(VOID)
{
  PLIST_ENTRY CurrentListEntry;
  PNTW32CALL_CALLBACK_STACK Current;

  while (!IsListEmpty(&CallbackStackListHead))
    {
      CurrentListEntry = RemoveHeadList(&CallbackStackListHead);
      Current = CONTAINING_RECORD(CurrentListEntry, NTW32CALL_CALLBACK_STACK,
				  ListEntry);
      PsFreeCallbackStack(Current->BaseAddress);
      ExFreePool(Current);
    }
}

PVOID STATIC
PsAllocateCallbackStack(ULONG StackSize)
{
  PVOID KernelStack = NULL;
  NTSTATUS Status;
  PMEMORY_AREA StackArea;
  ULONG i, j;
  PHYSICAL_ADDRESS BoundaryAddressMultiple;
  PPFN_TYPE Pages = alloca(sizeof(PFN_TYPE) * (StackSize /PAGE_SIZE));


  BoundaryAddressMultiple.QuadPart = 0;
  StackSize = PAGE_ROUND_UP(StackSize);
  MmLockAddressSpace(MmGetKernelAddressSpace());
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_KERNEL_STACK,
			      &KernelStack,
			      StackSize,
			      0,
			      &StackArea,
			      FALSE,
			      FALSE,
			      BoundaryAddressMultiple);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to create thread stack\n");
      return(NULL);
    }
  for (i = 0; i < (StackSize / PAGE_SIZE); i++)
    {
      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Pages[i]);
      if (!NT_SUCCESS(Status))
	{
	  for (j = 0; j < i; j++)
	  {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pages[j]);
	  }
	  return(NULL);
	}
    }
  Status = MmCreateVirtualMapping(NULL,
				  KernelStack,
				  PAGE_READWRITE,
				  Pages,
				  StackSize / PAGE_SIZE);
  if (!NT_SUCCESS(Status))
    {
      for (i = 0; i < (StackSize / PAGE_SIZE); i++)
        {
	  MmReleasePageMemoryConsumer(MC_NPPOOL, Pages[i]);
	}
      return(NULL);
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
  ULONG_PTR StackSize;
  PKTRAP_FRAME NewFrame;
  PULONG UserEsp;
  KIRQL oldIrql;
  NTSTATUS CallbackStatus;
  NTW32CALL_SAVED_STATE SavedState;
  PNTW32CALL_CALLBACK_STACK AssignedStack;
  
  PAGED_CODE();

  DPRINT("NtW32Call(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
	  RoutineIndex, Argument, ArgumentLength);

  Thread = PsGetCurrentThread();

  /* Set up the new kernel and user environment. */
  StackSize = (ULONG_PTR)Thread->Tcb.StackBase - Thread->Tcb.StackLimit;
  KeAcquireSpinLock(&CallbackStackListLock, &oldIrql);
  if (IsListEmpty(&CallbackStackListHead))
    {
      KeReleaseSpinLock(&CallbackStackListLock, oldIrql);
      NewStack = PsAllocateCallbackStack(StackSize);
      AssignedStack = ExAllocatePool(NonPagedPool,
				     sizeof(NTW32CALL_CALLBACK_STACK));
      AssignedStack->BaseAddress = NewStack;
    }
  else
    {
      PLIST_ENTRY StackEntry;

      StackEntry = RemoveHeadList(&CallbackStackListHead);
      KeReleaseSpinLock(&CallbackStackListLock, oldIrql);
      AssignedStack = CONTAINING_RECORD(StackEntry, NTW32CALL_CALLBACK_STACK,
					ListEntry);
      NewStack = AssignedStack->BaseAddress;
      memset(NewStack, 0, StackSize);
    }
  /* FIXME: Need to check whether we were interrupted from v86 mode. */
  memcpy((char*)NewStack + StackSize - sizeof(KTRAP_FRAME) - sizeof(FX_SAVE_AREA),
         Thread->Tcb.TrapFrame, sizeof(KTRAP_FRAME) - (4 * sizeof(DWORD)));
  NewFrame = (PKTRAP_FRAME)((char*)NewStack + StackSize - sizeof(KTRAP_FRAME) - sizeof(FX_SAVE_AREA));
  /* We need the stack pointer to remain 4-byte aligned */
  NewFrame->Esp -= (((ArgumentLength + 3) & (~ 0x3)) + (4 * sizeof(ULONG)));
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
  SavedState.SavedCallbackStack = Thread->Tcb.CallbackStack;
  SavedState.SavedExceptionStack = (PVOID)KeGetCurrentKPCR()->TSS->Esp0;
  if ((Thread->Tcb.NpxState & NPX_STATE_VALID) &&
      ETHREAD_TO_KTHREAD(Thread) != KeGetCurrentKPCR()->PrcbData.NpxThread)
    {
      memcpy((char*)NewStack + StackSize - sizeof(FX_SAVE_AREA),
             (char*)SavedState.SavedInitialStack - sizeof(FX_SAVE_AREA),
             sizeof(FX_SAVE_AREA));
    }
  Thread->Tcb.InitialStack = Thread->Tcb.StackBase = (char*)NewStack + StackSize;
  Thread->Tcb.StackLimit = (ULONG)NewStack;
  Thread->Tcb.KernelStack = (char*)NewStack + StackSize - sizeof(KTRAP_FRAME) - sizeof(FX_SAVE_AREA);
  KeGetCurrentKPCR()->TSS->Esp0 = (ULONG)Thread->Tcb.InitialStack - sizeof(FX_SAVE_AREA);
  KePushAndStackSwitchAndSysRet((ULONG)&SavedState, Thread->Tcb.KernelStack);

  /* 
   * The callback return will have already restored most of the state we 
   * modified.
   */
  KeLowerIrql(DISPATCH_LEVEL);
  KeAcquireSpinLockAtDpcLevel(&CallbackStackListLock);
  InsertTailList(&CallbackStackListHead, &AssignedStack->ListEntry);
  KeReleaseSpinLock(&CallbackStackListLock, PASSIVE_LEVEL);
  return(CallbackStatus);
} 

/* EOF */
