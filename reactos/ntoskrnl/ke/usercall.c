/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/usercall.c
 * PURPOSE:         User-Mode callbacks. Portable part.
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

#if ALEX_CB_REWRITE

NTSTATUS 
STDCALL 
KiSwitchToUserMode(IN PVOID *OutputBuffer,
                   IN PULONG OutputLength);

#else

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

VOID INIT_FUNCTION
PsInitialiseW32Call(VOID)
{
  InitializeListHead(&CallbackStackListHead);
  KeInitializeSpinLock(&CallbackStackListLock);
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
#endif

/*
 * @implemented
 */
NTSTATUS
STDCALL
KeUserModeCallback(IN ULONG RoutineIndex,
                   IN PVOID Argument,
                   IN ULONG ArgumentLength,
                   OUT PVOID *Result,
                   OUT PULONG ResultLength)
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

  DPRINT("KeUserModeCallback(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
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
      RtlZeroMemory(NewStack, StackSize);
    }
  /* FIXME: Need to check whether we were interrupted from v86 mode. */
  RtlCopyMemory((char*)NewStack + StackSize - sizeof(KTRAP_FRAME) - sizeof(FX_SAVE_AREA),
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
  RtlCopyMemory((PVOID)&UserEsp[4], Argument, ArgumentLength);

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
      ETHREAD_TO_KTHREAD(Thread) != KeGetCurrentPrcb()->NpxThread)
    {
      RtlCopyMemory((char*)NewStack + StackSize - sizeof(FX_SAVE_AREA),
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
