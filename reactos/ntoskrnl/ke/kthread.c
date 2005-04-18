/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define THREAD_ALERT_INCREMENT 2

extern EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

/*
 * PURPOSE: List of threads associated with each priority level
 */
LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
static ULONG PriorityListMask = 0;
ULONG IdleProcessorMask = 0;
extern BOOLEAN DoneInitYet;
extern PETHREAD PspReaperList;

/* FUNCTIONS *****************************************************************/

STATIC 
VOID
KiRequestReschedule(CCHAR Processor)
{
    PKPCR Pcr;

    Pcr = (PKPCR)(KPCR_BASE + Processor * PAGE_SIZE);
    Pcr->Prcb->QuantumEnd = TRUE;
    KiIpiSendRequest(1 << Processor, IPI_REQUEST_DPC);
}

STATIC 
VOID
KiInsertIntoThreadList(KPRIORITY Priority, 
                       PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    ASSERT(Thread->Priority == Priority);
    
    if (Priority >= MAXIMUM_PRIORITY || Priority < LOW_PRIORITY) {
        
        DPRINT1("Invalid thread priority (%d)\n", Priority);
        KEBUGCHECK(0);
    }
    
    InsertTailList(&PriorityListHead[Priority], &Thread->QueueListEntry);
    PriorityListMask |= (1 << Priority);
}

STATIC
VOID 
KiRemoveFromThreadList(PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    RemoveEntryList(&Thread->QueueListEntry);
    if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Priority])) {
        
        PriorityListMask &= ~(1 << Thread->Priority);
    }
}

STATIC
PKTHREAD 
KiScanThreadList(KPRIORITY Priority, 
                 KAFFINITY Affinity)
{
    PLIST_ENTRY current_entry;
    PKTHREAD current;
    ULONG Mask;

    Mask = (1 << Priority);
    
    if (PriorityListMask & Mask) {
        
        current_entry = PriorityListHead[Priority].Flink;
        
        while (current_entry != &PriorityListHead[Priority]) {
           
            current = CONTAINING_RECORD(current_entry, KTHREAD, QueueListEntry);
            
            if (current->State != Ready) {
                
                DPRINT1("%d/%d\n", &current, current->State);
            }
            
            ASSERT(current->State == Ready);
            
            if (current->Affinity & Affinity) {
                
                KiRemoveFromThreadList(current);
                return(current);
            }
            
            current_entry = current_entry->Flink;
        }
    }
    
    return(NULL);
}

VOID 
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus)
{
    KPRIORITY CurrentPriority;
    PKTHREAD Candidate;
    ULONG Affinity;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    DPRINT("KiDispatchThreadNoLock() %d/%d/%d/%d\n", KeGetCurrentProcessorNumber(),
            CurrentThread, NewThreadStatus, CurrentThread->State);

    CurrentThread->State = (UCHAR)NewThreadStatus;
    
    if (NewThreadStatus == Ready) {
            
        KiInsertIntoThreadList(CurrentThread->Priority,
                               CurrentThread);
    }

    Affinity = 1 << KeGetCurrentProcessorNumber();
    
    for (CurrentPriority = HIGH_PRIORITY; CurrentPriority >= LOW_PRIORITY; CurrentPriority--) {
        
        Candidate = KiScanThreadList(CurrentPriority, Affinity);
        
        if (Candidate == CurrentThread) {

            Candidate->State = Ready;
            KeReleaseDispatcherDatabaseLockFromDpcLevel();	
            return;
        }
        
        if (Candidate != NULL) {
            
            PKTHREAD OldThread;
            PKTHREAD IdleThread;

            DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);

            Candidate->State = Ready;

            OldThread = CurrentThread;
            CurrentThread = Candidate;
            IdleThread = KeGetCurrentPrcb()->IdleThread;

            if (OldThread == IdleThread) {
                
                IdleProcessorMask &= ~Affinity;
                
            } else if (CurrentThread == IdleThread) {
                
                IdleProcessorMask |= Affinity;
            }

            MmUpdatePageDir(PsGetCurrentProcess(),((PETHREAD)CurrentThread)->ThreadsProcess, sizeof(EPROCESS));

            /* Special note for Filip: This will release the Dispatcher DB Lock ;-) -- Alex */
            KiArchContextSwitch(CurrentThread, OldThread);
            return;
        }
    }
    
    DPRINT1("CRITICAL: No threads are ready (CPU%d)\n", KeGetCurrentProcessorNumber());
    KEBUGCHECK(0);
}

VOID
STDCALL
KiBlockThread(PNTSTATUS Status, 
              UCHAR Alertable, 
              ULONG WaitMode,
              UCHAR WaitReason)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;

    if (Thread->ApcState.KernelApcPending) {
    
        DPRINT("Dispatching Thread as ready (APC!)\n");
        
        /* Remove Waits */
        WaitBlock = Thread->WaitBlockList;
        do {
            RemoveEntryList (&WaitBlock->WaitListEntry);
            WaitBlock = WaitBlock->NextWaitBlock;
        } while (WaitBlock != Thread->WaitBlockList);
        Thread->WaitBlockList = NULL;
        
        /* Dispatch it and return status */
        KiDispatchThreadNoLock (Ready);
        if (Status != NULL) *Status = STATUS_KERNEL_APC;

    } else {

        /* Set the Thread Data as Requested */
        DPRINT("Dispatching Thread as blocked: %d\n", Thread->WaitStatus);
        Thread->Alertable = Alertable;
        Thread->WaitMode = (UCHAR)WaitMode;
        Thread->WaitReason = WaitReason;
        
        /* Dispatch it and return status */
        KiDispatchThreadNoLock(Waiting);
        DPRINT("Dispatching Thread as blocked: %d\n", Thread->WaitStatus);
        if (Status != NULL) *Status = Thread->WaitStatus;
    }
    
    DPRINT("Releasing Dispatcher Lock\n");
    KfLowerIrql(Thread->WaitIrql);
}

VOID 
STDCALL
KiDispatchThread(ULONG NewThreadStatus)
{
    KIRQL OldIrql;

    if (!DoneInitYet || KeGetCurrentPrcb()->IdleThread == NULL) {
        return;
    }
    
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiDispatchThreadNoLock(NewThreadStatus);
    KeLowerIrql(OldIrql);
}

VOID
STDCALL
KiUnblockThread(PKTHREAD Thread, 
                PNTSTATUS WaitStatus, 
                KPRIORITY Increment)
{
    if (Terminated == Thread->State) {

        DPRINT("Can't unblock thread 0x%x because it's terminating\n",
               Thread);
    
    } else if (Ready == Thread->State ||
               Running == Thread->State) {
        
        DPRINT("Can't unblock thread 0x%x because it's %s\n",
               Thread, (Thread->State == Ready ? "ready" : "running"));
    
    } else {
        
        ULONG Processor;
        KAFFINITY Affinity;

        /* FIXME: This propably isn't the right way to do it... */
        /* No it's not... i'll fix it later-- Alex */
        if (Thread->Priority < LOW_REALTIME_PRIORITY &&
            Thread->BasePriority < LOW_REALTIME_PRIORITY - 2) {
          
            if (!Thread->PriorityDecrement && !Thread->DisableBoost) {
                
                Thread->Priority = Thread->BasePriority + Increment;
                Thread->PriorityDecrement = Increment;
            }
            
        } else {
            
            Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
        }
     
        if (WaitStatus != NULL) {
            
            Thread->WaitStatus = *WaitStatus;
        }
        
        Thread->State = Ready;
        KiInsertIntoThreadList(Thread->Priority, Thread);
        Processor = KeGetCurrentProcessorNumber();
        Affinity = Thread->Affinity;
        
        if (!(IdleProcessorMask & (1 << Processor) & Affinity) &&
             (IdleProcessorMask & ~(1 << Processor) & Affinity)) {
            
            ULONG i;
            
            for (i = 0; i < KeNumberProcessors - 1; i++) {
            
                Processor++;
                
                if (Processor >= KeNumberProcessors) {
                    
                    Processor = 0;
                }
                
                if (IdleProcessorMask & (1 << Processor) & Affinity) {
#if 0	        
                    /* FIXME:
                     *   Reschedule the threads on an other processor 
                     */
                    KeReleaseDispatcherDatabaseLockFromDpcLevel();
                    KiRequestReschedule(Processor);
                    KeAcquireDispatcherDatabaseLockAtDpcLevel();
#endif
                    break;
                }
            }
        } 
    }
}

VOID 
STDCALL
KiSuspendThreadKernelRoutine(PKAPC Apc,
                             PKNORMAL_ROUTINE* NormalRoutine,
                             PVOID* NormalContext,
                             PVOID* SystemArgument1,
                             PVOID* SystemArguemnt2)
{
}

VOID 
STDCALL
KiSuspendThreadNormalRoutine(PVOID NormalContext,
                             PVOID SystemArgument1,
                             PVOID SystemArgument2)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    
    /* Non-alertable kernel-mode suspended wait */
    DPRINT("Waiting...\n");
    KeWaitForSingleObject(&CurrentThread->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);
    DPRINT("Done Waiting\n");
}

#ifdef KeGetCurrentThread
#undef KeGetCurrentThread
#endif
/*
 * @implemented
 */
PKTHREAD 
STDCALL 
KeGetCurrentThread(VOID)
{
#ifdef CONFIG_SMP
    ULONG Flags;
    PKTHREAD Thread;
    Ke386SaveFlags(Flags);
    Ke386DisableInterrupts();
    Thread = KeGetCurrentPrcb()->CurrentThread;
    Ke386RestoreFlags(Flags);
    return Thread;
#else
    return(KeGetCurrentPrcb()->CurrentThread);
#endif
}

VOID
STDCALL
KeSetPreviousMode(ULONG Mode)
{
    PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}

/*
 * @implemented
 */
KPROCESSOR_MODE 
STDCALL
KeGetPreviousMode(VOID)
{
    return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}

VOID
STDCALL
KeRundownThread(VOID)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    PLIST_ENTRY CurrentEntry;
    PKMUTANT Mutant;
    
    DPRINT("KeRundownThread: %x\n", Thread);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    while (!IsListEmpty(&Thread->MutantListHead)) {
    
        /* Get the Mutant */
	CurrentEntry = RemoveHeadList(&Thread->MutantListHead);
        Mutant = CONTAINING_RECORD(CurrentEntry, KMUTANT, MutantListEntry);
        ASSERT(Mutant->ApcDisable == 0);
    
        /* Uncondtionally abandon it */
        DPRINT("Abandonning the Mutant\n");
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = NULL;
        RemoveEntryList(&Mutant->MutantListEntry);
        
        /* Check if the Wait List isn't empty */
        DPRINT("Checking whether to wake the Mutant\n");
        if (!IsListEmpty(&Mutant->Header.WaitListHead)) {
            
            /* Wake the Mutant */
            DPRINT("Waking the Mutant\n");
            KiWaitTest(&Mutant->Header, MUTANT_INCREMENT);
        }
    }
   
    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

ULONG
STDCALL
KeResumeThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;
    
    DPRINT("KeResumeThread (Thread %p called). %x, %x\n", Thread, Thread->SuspendCount, Thread->FreezeCount);

    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Check if it existed */
    if (PreviousCount) {
        
        Thread->SuspendCount--;
        
        /* Decrease the current Suspend Count and Check Freeze Count */
        if ((!Thread->SuspendCount) && (!Thread->FreezeCount)) {
            
            /* Signal the Suspend Semaphore */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }
            
    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

BOOLEAN
STDCALL
KiInsertQueueApc(PKAPC Apc,
                 KPRIORITY PriorityBoost);

/*
 * Used by the debugging code to freeze all the process's threads
 * while the debugger is examining their state.
 */
VOID
STDCALL
KeFreezeAllThreads(PKPROCESS Process)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PKTHREAD Current;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Acquire Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Loop the Process's Threads */
    CurrentEntry = Process->ThreadListHead.Flink;
    while (CurrentEntry != &Process->ThreadListHead)
    {
        /* Get the Thread */
        Current = CONTAINING_RECORD(CurrentEntry, KTHREAD, ThreadListEntry);
        
        /* Make sure it's not ours */
        if (Current == CurrentThread) continue;
        
        /* Make sure it wasn't already frozen, and that it's not suspended */
        if (!(++Current->FreezeCount) && !(Current->SuspendCount))
        {
            /* Insert the APC */
            if (!KiInsertQueueApc(&Current->SuspendApc, IO_NO_INCREMENT)) 
            {
                /* Unsignal the Semaphore, the APC already got inserted */
                Current->SuspendSemaphore.Header.SignalState--;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    /* Release the lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}
    
NTSTATUS
STDCALL
KeSuspendThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    DPRINT("KeSuspendThread (Thread %p called). %x, %x\n", Thread, Thread->SuspendCount, Thread->FreezeCount);
    
    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;
    
    /* Increment it */
    Thread->SuspendCount++;
    
    /* Check if we should suspend it */
    if (!PreviousCount && !Thread->FreezeCount) {
    
        /* Insert the APC */
        if (!KiInsertQueueApc(&Thread->SuspendApc, IO_NO_INCREMENT)) {
            
            /* Unsignal the Semaphore, the APC already got inserted */
            Thread->SuspendSemaphore.Header.SignalState--;
        }
    }
    
    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

ULONG
STDCALL
KeForceResumeThread(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    ULONG PreviousCount;
    
    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount + Thread->FreezeCount;  
    
    /* If the thread is suspended, wake it up!!! */
    if (PreviousCount) {
            
        /* Unwait it completely */
        Thread->SuspendCount = 0;
        Thread->FreezeCount = 0;
            
        /* Signal and satisfy */
        Thread->SuspendSemaphore.Header.SignalState++;
        KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
    }
    
    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

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
        if (Thread->State == Waiting &&  Thread->Alertable) {
            
            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
       
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
        if (Thread->State == Waiting && 
            (AlertMode == KernelMode || Thread->WaitMode == AlertMode) &&
            Thread->Alertable) {
            
            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
       
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
    Thread->State = Initialized;
    
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
                    KiSuspendThreadKernelRoutine,
                    NULL,
                    KiSuspendThreadNormalRoutine,
                    KernelMode,
                    NULL);
     
    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 128);
    
    /* Insert the Thread into the Process's Thread List */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
  
    /* Set up the Suspend Counts */
    Thread->FreezeCount = 0;
    Thread->SuspendCount = 0;
    ((PETHREAD)Thread)->ReaperLink = NULL; /* Union. Will also clear termination port */
   
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
        KiDispatchThreadNoLock(Ready);
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
        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);
    }
}

/*
 * @implemented
 */
LONG STDCALL
KeSetBasePriorityThread (PKTHREAD	Thread,
			 LONG		Increment)
/*
 * Sets thread's base priority relative to the process' base priority
 * Should only be passed in THREAD_PRIORITY_ constants in pstypes.h
 */
{
   KPRIORITY Priority;
   if (Increment < -2)
     {
       Increment = -2;
     }
   else if (Increment > 2)
     {
       Increment = 2;
     }
   Priority = ((PETHREAD)Thread)->ThreadsProcess->Pcb.BasePriority + Increment;
   if (Priority < LOW_PRIORITY)
   {
     Priority = LOW_PRIORITY;
   }
   else if (Priority >= MAXIMUM_PRIORITY)
     {
       Thread->BasePriority = HIGH_PRIORITY;
     }
   KeSetPriorityThread(Thread, Priority);
   return 1;
}

/*
 * @implemented
 */
KPRIORITY 
STDCALL
KeSetPriorityThread(PKTHREAD Thread, 
                    KPRIORITY Priority)
{
    KPRIORITY OldPriority;
    KIRQL OldIrql;
    PKTHREAD CurrentThread;
    ULONG Mask;
    int i;
    PKPCR Pcr;

    if (Priority < LOW_PRIORITY || Priority >= MAXIMUM_PRIORITY) {
        
        KEBUGCHECK(0);
    }

    OldIrql = KeAcquireDispatcherDatabaseLock();

    OldPriority = Thread->Priority;

    if (OldPriority != Priority) {
        
        CurrentThread = KeGetCurrentThread();
        
        if (Thread->State == Ready) {
            
            KiRemoveFromThreadList(Thread);
            Thread->BasePriority = Thread->Priority = (CHAR)Priority;
            KiInsertIntoThreadList(Priority, Thread);
            
            if (CurrentThread->Priority < Priority) {
                
                KiDispatchThreadNoLock(Ready);
                KeLowerIrql(OldIrql);
                return (OldPriority);
            }
        
        } else if (Thread->State == Running)  {
            
            Thread->BasePriority = Thread->Priority = (CHAR)Priority;
            
            if (Priority < OldPriority) {
                
                /* Check for threads with a higher priority */
                Mask = ~((1 << (Priority + 1)) - 1);
                if (PriorityListMask & Mask) {
                    
                    if (Thread == CurrentThread) {
                        
                        KiDispatchThreadNoLock(Ready);
                        KeLowerIrql(OldIrql);
                        return (OldPriority);
                        
                    } else {
                        
                        for (i = 0; i < KeNumberProcessors; i++) {
                            
                            Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
                            
                            if (Pcr->Prcb->CurrentThread == Thread) {

                                KeReleaseDispatcherDatabaseLockFromDpcLevel();
                                KiRequestReschedule(i);
                                KeLowerIrql(OldIrql);
                                return (OldPriority);
                            }
                        }
                    }
                }
            }
        }  else  {
            
            Thread->BasePriority = Thread->Priority = (CHAR)Priority;
        }
    }
    
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return(OldPriority);
}

/*
 * @implemented
 *
 * Sets thread's affinity
 */
NTSTATUS 
STDCALL
KeSetAffinityThread(PKTHREAD Thread,
                    KAFFINITY Affinity)
{
    KIRQL OldIrql;
    ULONG i;
    PKPCR Pcr;
    KAFFINITY ProcessorMask;

    DPRINT("KeSetAffinityThread(Thread %x, Affinity %x)\n", Thread, Affinity);

    ASSERT(Affinity & ((1 << KeNumberProcessors) - 1));

    OldIrql = KeAcquireDispatcherDatabaseLock();

    Thread->UserAffinity = Affinity;
    
    if (Thread->SystemAffinityActive == FALSE) {
        
        Thread->Affinity = Affinity;
        
        if (Thread->State == Running) {
            
            ProcessorMask = 1 << KeGetCurrentKPCR()->ProcessorNumber;
            if (Thread == KeGetCurrentThread()) {
                
                if (!(Affinity & ProcessorMask)) {
                    
                    KiDispatchThreadNoLock(Ready);
                    KeLowerIrql(OldIrql);
                    return STATUS_SUCCESS;
                }
                
            } else {
                
                for (i = 0; i < KeNumberProcessors; i++) {
                    
                    Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
                    if (Pcr->Prcb->CurrentThread == Thread) {
                        
                        if (!(Affinity & ProcessorMask)) {
                            
                            KeReleaseDispatcherDatabaseLockFromDpcLevel();
                            KiRequestReschedule(i);
                            KeLowerIrql(OldIrql);
                            return STATUS_SUCCESS;
                        }
                        
                        break;
                    }
                }

                ASSERT (i < KeNumberProcessors);
            }
        }
    }
    
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
 /* The Increment Argument seems to be ignored by NT and always 0 when called */
VOID
STDCALL
KeTerminateThread(IN KPRIORITY Increment)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    
    /* Lock the Dispatcher Database and the APC Queue */
    DPRINT("Terminating\n");
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Insert into the Reaper List */
    DPRINT("List: %p\n", PspReaperList);
    ((PETHREAD)Thread)->ReaperLink = PspReaperList;
    PspReaperList = (PETHREAD)Thread;
    DPRINT("List: %p\n", PspReaperList);
    
    /* Check if it's active */
    if (PspReaping == FALSE) {
        
        /* Activate it. We use the internal function for speed, and use the Hyper Critical Queue */
        PspReaping = TRUE;
        DPRINT("Terminating\n");
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue].WorkerQueue,
                      &PspReaperWorkItem.List,
                      FALSE);
    }
    
    /* Handle Kernel Queues */
    if (Thread->Queue) {
                 
        DPRINT("Waking Queue\n");
        RemoveEntryList(&Thread->QueueListEntry);
        KiWakeQueue(Thread->Queue);
    }
    
    /* Signal the thread */
    Thread->DispatcherHeader.SignalState = TRUE;
    if (IsListEmpty(&Thread->DispatcherHeader.WaitListHead) != TRUE) {
        
        /* Satisfy waits */
        KiWaitTest((PVOID)Thread, Increment);
    }
    
    /* Find a new Thread */
    KiDispatchThreadNoLock(Terminated);
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
