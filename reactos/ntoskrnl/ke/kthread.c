/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Commented, reorganized some stuff, fixed/implemented some functions.
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG
STDCALL
KeAlertResumeThread(IN PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
   
    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);

    /* Return if Thread is already alerted. */
    if (Thread->Alerted[KernelMode] == FALSE) {
       
        /* If it's Blocked, unblock if it we should */
        if (Thread->State == THREAD_STATE_BLOCKED &&  Thread->Alertable) {
            
            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED);
       
        } else {
           
            /* If not, simply Alert it */
            Thread->Alerted[KernelMode] = TRUE;
        }
    }
    
    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount;    
    
    /* If the thread is suspended, decrease one of the suspend counts */
    if (PreviousCount) {
            
        /* Decrease count. If we are now zero, unwait it completely */
        if (--Thread->SuspendCount) {
            
            /* Signal and satisfy */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

BOOLEAN
STDCALL
KeAlertThread(PKTHREAD Thread, 
              KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    BOOLEAN PreviousState;

    /* Acquire the Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();
   
    /* Save the Previous State */
    PreviousState = Thread->Alerted[AlertMode];
      
    /* Return if Thread is already alerted. */
    if (PreviousState == FALSE) {
       
        /* If it's Blocked, unblock if it we should */
        if (Thread->State == THREAD_STATE_BLOCKED && 
            (AlertMode == KernelMode || Thread->WaitMode == AlertMode) &&
            Thread->Alertable) {
            
            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED);
       
        } else {
           
            /* If not, simply Alert it */
            Thread->Alerted[AlertMode] = TRUE;
        }
    }
    
    /* Release the Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    
    /* Return the old state */
    return PreviousState;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeCapturePersistentThreadState(IN PVOID CurrentThread,
                               IN ULONG Setting1,
                               IN ULONG Setting2,
                               IN ULONG Setting3,
                               IN ULONG Setting4,
                               IN ULONG Setting5,
                               IN PVOID ThreadState)
{
    UNIMPLEMENTED;
}

/*
 * FUNCTION: Initialize the microkernel state of the thread
 */
VOID
STDCALL
KeInitializeThread(PKPROCESS Process, 
                   PKTHREAD Thread, 
                   BOOLEAN First)
{
    PVOID KernelStack;
    NTSTATUS Status;
    extern unsigned int init_stack_top;
    extern unsigned int init_stack;
    PMEMORY_AREA StackArea;
    ULONG i;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
  
    /* Initialize the Boundary Address */
    BoundaryAddressMultiple.QuadPart = 0;
  
    /* Initalize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
                                 ThreadObject,
                                 sizeof(KTHREAD),
                                 FALSE);
    InitializeListHead(&Thread->MutantListHead);
    
    /* If this is isn't the first thread, allocate the Kernel Stack */
    if (!First) {
        
        PFN_TYPE Page[MM_STACK_SIZE / PAGE_SIZE];
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
      
        /* Check for Success */
        if (!NT_SUCCESS(Status)) {
            
            DPRINT1("Failed to create thread stack\n");
            KEBUGCHECK(0);
        }
        
        /* Mark the Stack */
        for (i = 0; i < (MM_STACK_SIZE / PAGE_SIZE); i++) {

            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page[i]);
            
            /* Check for success */
            if (!NT_SUCCESS(Status)) {
                
                KEBUGCHECK(0);
            }
        }
        
        /* Create a Virtual Mapping for it */
        Status = MmCreateVirtualMapping(NULL,
                                        KernelStack,
                                        PAGE_READWRITE,
                                        Page,
                                        MM_STACK_SIZE / PAGE_SIZE);
        
        /* Check for success */
        if (!NT_SUCCESS(Status)) {
            
            KEBUGCHECK(0);
        }
        
        /* Set the Kernel Stack */
        Thread->InitialStack = (PCHAR)KernelStack + MM_STACK_SIZE;
        Thread->StackBase    = (PCHAR)KernelStack + MM_STACK_SIZE;
        Thread->StackLimit   = (ULONG_PTR)KernelStack;
        Thread->KernelStack  = (PCHAR)KernelStack + MM_STACK_SIZE;
        
    } else {
        
        /* Use the Initial Stack */
        Thread->InitialStack = (PCHAR)init_stack_top;
        Thread->StackBase = (PCHAR)init_stack_top;
        Thread->StackLimit = (ULONG_PTR)init_stack;
        Thread->KernelStack = (PCHAR)init_stack_top;
    }

    /* 
     * Establish the pde's for the new stack and the thread structure within the 
     * address space of the new process. They are accessed while taskswitching or
     * while handling page faults. At this point it isn't possible to call the 
     * page fault handler for the missing pde's. 
     */
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, MM_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));

    /* Set the Thread to initalized */
    Thread->State = THREAD_STATE_INITIALIZED;
    
    /* The Native API function will initialize the TEB field later */
    Thread->Teb = NULL;
    
    /* Initialize stuff to zero */
    Thread->TlsArray = NULL;
    Thread->DebugActive = 0;
    Thread->Alerted[0] = 0;
    Thread->Alerted[1] = 0;
    Thread->Iopl = 0;
    
    /* Set up FPU/NPX Stuff */
    Thread->NpxState = NPX_STATE_INVALID;
    Thread->NpxIrql = 0;
   
    /* Setup APC Fields */
    InitializeListHead(&Thread->ApcState.ApcListHead[0]);
    InitializeListHead(&Thread->ApcState.ApcListHead[1]);
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = 0;
    Thread->ApcState.KernelApcPending = 0;
    Thread->ApcState.UserApcPending = 0;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;
    Thread->ApcQueueable = TRUE;
    RtlZeroMemory(&Thread->SavedApcState, sizeof(KAPC_STATE));
    KeInitializeSpinLock(&Thread->ApcQueueLock);
    
    /* Setup Wait Fields */
    Thread->WaitStatus = STATUS_SUCCESS;
    Thread->WaitIrql = PASSIVE_LEVEL;
    Thread->WaitMode = 0;
    Thread->WaitNext = FALSE;
    Thread->WaitListEntry.Flink = NULL;
    Thread->WaitListEntry.Blink = NULL;
    Thread->WaitTime = 0;
    Thread->WaitBlockList = NULL;
    RtlZeroMemory(Thread->WaitBlock, sizeof(KWAIT_BLOCK) * 4);
    RtlZeroMemory(&Thread->Timer, sizeof(KTIMER));
    KeInitializeTimer(&Thread->Timer);
    
    /* Setup scheduler Fields */
    Thread->BasePriority = Process->BasePriority;
    Thread->DecrementCount = 0;
    Thread->PriorityDecrement = 0;
    Thread->Quantum = Process->ThreadQuantum;
    Thread->Saturation = 0;
    Thread->Priority = Process->BasePriority; 
    Thread->UserAffinity = Process->Affinity;
    Thread->SystemAffinityActive = 0;
    Thread->Affinity = Process->Affinity;
    Thread->Preempted = 0;
    Thread->ProcessReadyQueue = 0;
    Thread->KernelStackResident = 1;
    Thread->NextProcessor = 0;
    Thread->ContextSwitches = 0;
    
    /* Setup Queue Fields */
    Thread->Queue = NULL;
    Thread->QueueListEntry.Flink = NULL;
    Thread->QueueListEntry.Blink = NULL;

    /* Setup Misc Fields */
    Thread->LegoData = 0; 
    Thread->PowerState = 0;
    Thread->ServiceTable = KeServiceDescriptorTable;
    Thread->CallbackStack = NULL;
    Thread->Win32Thread = NULL;
    Thread->TrapFrame = NULL;
    Thread->EnableStackSwap = 0;
    Thread->LargeStack = 0;
    Thread->ResourceIndex = 0;
    Thread->PreviousMode = KernelMode;
    Thread->KernelTime = 0;
    Thread->UserTime = 0;
    Thread->AutoAlignment = Process->AutoAlignment;
   
  /* FIXME OPTIMIZATION OF DOOM. DO NOT ENABLE FIXME */
#if 0
  Thread->WaitBlock[3].Object = (PVOID)&Thread->Timer;
  Thread->WaitBlock[3].Thread = Thread;
  Thread->WaitBlock[3].WaitKey = STATUS_TIMEOUT;
  Thread->WaitBlock[3].WaitType = WaitAny;
  Thread->WaitBlock[3].NextWaitBlock = NULL;
  InsertTailList(&Thread->Timer.Header.WaitListHead,
                 &Thread->WaitBlock[3].WaitListEntry);
#endif

    /* Initialize the Suspend APC */  
    KeInitializeApc(&Thread->SuspendApc,
                    Thread,
                    OriginalApcEnvironment,
                    PiSuspendThreadKernelRoutine,
                    PiSuspendThreadRundownRoutine,
                    PiSuspendThreadNormalRoutine,
                    KernelMode,
                    NULL);
     
    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 128);
    
    /* Insert the Thread into the Process's Thread List */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
  
    /* Set up the Suspend Counts */
    Thread->FreezeCount = 0;
    Thread->SuspendCount = 0;
   
    /* Do x86 specific part */
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeQueryPriorityThread (IN PKTHREAD Thread)
{
    return Thread->Priority;
}

/*
 * @implemented
 */
ULONG
STDCALL
KeQueryRuntimeThread(IN PKTHREAD Thread,
                     OUT PULONG UserTime)
{
    /* Return the User Time */
    *UserTime = Thread->UserTime;
    
    /* Return the Kernel Time */
    return Thread->KernelTime;
}

VOID
KeFreeStackPage(PVOID Context, 
                MEMORY_AREA* MemoryArea, 
                PVOID Address, 
                PFN_TYPE Page, 
                SWAPENTRY SwapEntry, 
                BOOLEAN Dirty)
{
    ASSERT(SwapEntry == 0);
    if (Page) MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
}

NTSTATUS
KeReleaseThread(PKTHREAD Thread)
/*
 * FUNCTION: Releases the resource allocated for a thread by
 * KeInitializeThread
 * NOTE: The thread had better not be running when this is called
 */
{
  extern unsigned int init_stack;

  /* FIXME - lock the process */
  RemoveEntryList(&Thread->ThreadListEntry);
  
  if (Thread->StackLimit != (ULONG_PTR)init_stack)
    {       
      MmLockAddressSpace(MmGetKernelAddressSpace());
      MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                            (PVOID)Thread->StackLimit,
                            KeFreeStackPage,
                            NULL);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
    }
  Thread->StackLimit = 0;
  Thread->InitialStack = NULL;
  Thread->StackBase = NULL;
  Thread->KernelStack = NULL;
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(IN BOOLEAN Enable)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN PreviousState;
    KIRQL OldIrql;
 
     /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old State */
    PreviousState = Thread->EnableStackSwap;

    /* Set New State */
    Thread->EnableStackSwap = Enable;

    /* No, Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
        
    /* Return Old State */
    return PreviousState;
}

/*
 * @implemented
 */
VOID
STDCALL
KeRevertToUserAffinityThread(VOID)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(CurrentThread->SystemAffinityActive != FALSE);
        
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Return to User Affinity */
    CurrentThread->Affinity = CurrentThread->UserAffinity;

    /* Disable System Affinity */
    CurrentThread->SystemAffinityActive = FALSE;
    
    /* Check if we need to Dispatch a New thread */
    if (CurrentThread->Affinity & (1 << KeGetCurrentProcessorNumber())) {
        
        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);
    
    } else {
        
        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        PsDispatchThreadNoLock(THREAD_STATE_READY);
        KeLowerIrql(OldIrql);
    }
}

/*
 * @implemented
 */
CCHAR
STDCALL
KeSetIdealProcessorThread(IN PKTHREAD Thread,
                          IN CCHAR Processor)
{
    CCHAR PreviousIdealProcessor;
    KIRQL OldIrql;
    
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old Ideal Processor */
    PreviousIdealProcessor = Thread->IdealProcessor;

    /* Set New Ideal Processor */
    Thread->IdealProcessor = Processor;
    
    /* Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);   
    
    /* Return Old Ideal Processor */
    return PreviousIdealProcessor;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetSystemAffinityThread(IN KAFFINITY Affinity)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(Affinity & ((1 << KeNumberProcessors) - 1));
        
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Set the System Affinity Specified */
    CurrentThread->Affinity = Affinity;

    /* Enable System Affinity */
    CurrentThread->SystemAffinityActive = TRUE;
    
    /* Check if we need to Dispatch a New thread */
    if (Affinity & (1 << KeGetCurrentProcessorNumber())) {
        
        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);
    
    } else {
        
        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        PsDispatchThreadNoLock(THREAD_STATE_READY);
        KeLowerIrql(OldIrql);
    }
}

/*
 * @implemented
 */
 /* The Increment Argument seems to be ignored by NT and always 0 when called */
VOID
STDCALL
KeTerminateThread(IN KPRIORITY Increment)
{
    /* Call our own internal routine */
    PsTerminateCurrentThread(0);
}

/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldState;
   
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
   
    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);
   
    /* Save the old State */
    OldState = Thread->Alerted[AlertMode];
   
    /* If the Thread is Alerted, Clear it */
    if (OldState) {
            
        Thread->Alerted[AlertMode] = FALSE;
        
    } else if ((AlertMode == UserMode) && (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))) {
        
        /* If the mode is User and the Queue isn't empty, set Pending */
        Thread->ApcState.UserApcPending = TRUE;
    }
   
    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return OldState;
}

VOID
KiServiceCheck (VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();
    
    /* Check if we need to inialize Win32 for this Thread */
    if (Thread->ServiceTable != KeServiceDescriptorTableShadow) {
    
        /* We do. Initialize it and save the new table */  
        PsInitWin32Thread((PETHREAD)Thread);
        Thread->ServiceTable = KeServiceDescriptorTableShadow;
    }
}

/* 
 *
 * NOT EXPORTED
 */
NTSTATUS 
STDCALL
NtAlertResumeThread(IN  HANDLE ThreadHandle,
                    OUT PULONG SuspendCount)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;
    ULONG PreviousState;

    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {
     
        _SEH_TRY {
            
            ProbeForWrite(SuspendCount,
                          sizeof(HANDLE),
                          sizeof(ULONG));
       
        } _SEH_HANDLE {
            
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;
    }
   
    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
   
    /* Check for Success */ 
    if (NT_SUCCESS(Status)) {
   
        /* Call the Kernel Function */
        PreviousState = KeAlertResumeThread(&Thread->Tcb);
   
        /* Dereference Object */
        ObDereferenceObject(Thread);
        
        if (SuspendCount) {
        
            _SEH_TRY {
        
                *SuspendCount = PreviousState;
            
            } _SEH_HANDLE {
            
                Status = _SEH_GetExceptionCode();
            
            } _SEH_END;
        }
    }
    
    /* Return status */
    return Status;
}

/* 
 * @implemented
 *
 * EXPORTED
 */
NTSTATUS 
STDCALL
NtAlertThread (IN HANDLE ThreadHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
   
    /* Check for Success */ 
    if (NT_SUCCESS(Status)) {
   
        /* 
         * Do an alert depending on the processor mode. If some kmode code wants to
         * enforce a umode alert it should call KeAlertThread() directly. If kmode
         * code wants to do a kmode alert it's sufficient to call it with Zw or just
         * use KeAlertThread() directly 
         */
        KeAlertThread(&Thread->Tcb, PreviousMode);
   
        /* Dereference Object */
        ObDereferenceObject(Thread);
    }
    
    /* Return status */
    return Status;
}

NTSTATUS 
STDCALL
NtDelayExecution(IN BOOLEAN Alertable,
                 IN PLARGE_INTEGER DelayInterval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeInterval;
    NTSTATUS Status;
   
    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {
     
        _SEH_TRY {
            
            ProbeForRead(DelayInterval,
                         sizeof(LARGE_INTEGER),
                         sizeof(ULONG));
            
            /* make a copy on the kernel stack and let DelayInterval point to it so
               we don't need to wrap KeDelayExecutionThread in SEH! */
            SafeInterval = *DelayInterval;
       
        } _SEH_HANDLE {
            
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
   }

   /* Call the Kernel Function */
   Status = KeDelayExecutionThread(PreviousMode,
                                   Alertable,
                                   &SafeInterval);
   
   /* Return Status */
   return Status;
}
