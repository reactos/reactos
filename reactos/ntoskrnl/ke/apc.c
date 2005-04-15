/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         NT Implementation of APCs
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Phillip Susi
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*++
 * KiKernelApcDeliveryCheck 
 * @implemented NT 5.2
 *
 *     The KiKernelApcDeliveryCheck routine is called whenever APCs have just
 *     been re-enabled in Kernel Mode, such as after leaving a Critical or
 *     Guarded Region. It delivers APCs if the environment is right.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     This routine allows KeLeave/EnterCritical/GuardedRegion to be used as a
 *     macro from inside WIN32K or other Drivers, which will then only have to
 *     do an Import API call in the case where APCs are enabled again.
 *
 *--*/
VOID
STDCALL
KiKernelApcDeliveryCheck(VOID)
{
    /* We should only deliver at passive */
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        /* Raise to APC and Deliver APCs, then lower back to Passive */
        KfRaiseIrql(APC_LEVEL);
        KiDeliverApc(KernelMode, 0, 0);
        KfLowerIrql(PASSIVE_LEVEL);
    }
    else
    {
        /*
         * If we're not at passive level it means someone raised IRQL
         * to APC level before the a critical or guarded section was entered
         * (e.g) by a fast mutex). This implies that the APCs shouldn't
         * be delivered now, but after the IRQL is lowered to passive
         * level again.
         */
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }
}

/*++
 * KeEnterCriticalRegion 
 * @implemented NT4
 *
 *     The KeEnterCriticalRegion routine temporarily disables the delivery of 
 *     normal kernel APCs; special kernel-mode APCs are still delivered.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Highest-level drivers can call this routine while running in the context
 *     of the thread that requested the current I/O operation. Any caller of 
 *     this routine should call KeLeaveCriticalRegion as quickly as possible.
 *
 *     Callers of KeEnterCriticalRegion must be running at IRQL <= APC_LEVEL.
 *
 *--*/
#undef KeEnterCriticalRegion
VOID 
STDCALL 
KeEnterCriticalRegion(VOID)
{
    /* Disable Kernel APCs */
    PKTHREAD Thread = KeGetCurrentThread();
    if (Thread) Thread->KernelApcDisable--;
}

/*++
 * KeLeaveCriticalRegion 
 * @implemented NT4
 *
 *     The KeLeaveCriticalRegion routine reenables the delivery of normal 
 *     kernel-mode APCs that were disabled by a call to KeEnterCriticalRegion.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Highest-level drivers can call this routine while running in the context
 *     of the thread that requested the current I/O operation. 
 *
 *     Callers of KeLeaveCriticalRegion must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
#undef KeLeaveCriticalRegion
VOID 
STDCALL 
KeLeaveCriticalRegion (VOID)
{
    PKTHREAD Thread = KeGetCurrentThread(); 

    /* Check if Kernel APCs are now enabled */
    if((Thread) && (++Thread->KernelApcDisable == 0)) 
    { 
        /* Check if we need to request an APC Delivery */
        if (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) 
        { 
            /* Check for the right environment */
            KiKernelApcDeliveryCheck();
        } 
    } 
}

/*++
 * KeInitializeApc 
 * @implemented NT4
 *
 *     The The KeInitializeApc routine initializes an APC object, and registers
 *     the Kernel, Rundown and Normal routines for that object.
 *
 * Params:
 *     Apc - Pointer to a KAPC structure that represents the APC object to 
 *           initialize. The caller must allocate storage for the structure
 *           from resident memory.
 *
 *     Thread - Thread to which to deliver the APC.
 *
 *     TargetEnvironment - APC Environment to be used.
 *
 *     KernelRoutine - Points to the KernelRoutine to associate with the APC.
 *                     This routine is executed for all APCs.
 *
 *     RundownRoutine - Points to the RundownRoutine to associate with the APC.
 *                      This routine is executed when the Thread exists with
 *                      the APC executing.
 *
 *     NormalRoutine - Points to the NormalRoutine to associate with the APC.
 *                     This routine is executed at PASSIVE_LEVEL. If this is
 *                     not specifed, the APC becomes a Special APC and the
 *                     Mode and Context parameters are ignored.
 *
 *     Mode - Specifies the processor mode at which to run the Normal Routine.
 *
 *     Context - Specifices the value to pass as Context parameter to the 
 *               registered routines.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     The caller can queue an initialized APC with KeInsertQueueApc.
 *
 *     Storage for the APC object must be resident, such as nonpaged pool 
 *     allocated by the caller.
 *
 *     Callers of this routine must be running at IRQL = PASSIVE_LEVEL.
 *
 *--*/
VOID
STDCALL
KeInitializeApc(IN PKAPC Apc,
                IN PKTHREAD Thread,
                IN KAPC_ENVIRONMENT TargetEnvironment,
                IN PKKERNEL_ROUTINE KernelRoutine,
                IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
                IN PKNORMAL_ROUTINE NormalRoutine,
                IN KPROCESSOR_MODE Mode,
                IN PVOID Context)
{
    DPRINT("KeInitializeApc(Apc %x, Thread %x, Environment %d, "
           "KernelRoutine %x, RundownRoutine %x, NormalRoutine %x, Mode %d, "
           "Context %x)\n",Apc,Thread,TargetEnvironment,KernelRoutine,RundownRoutine,
            NormalRoutine,Mode,Context);

    /* Set up the basic APC Structure Data */
    RtlZeroMemory(Apc, sizeof(KAPC));
    Apc->Type = ApcObject;
    Apc->Size = sizeof(KAPC);
    
    /* Set the Environment */
    if (TargetEnvironment == CurrentApcEnvironment) {
        
        Apc->ApcStateIndex = Thread->ApcStateIndex;
        
    } else {
        
        Apc->ApcStateIndex = TargetEnvironment;
    }
    
    /* Set the Thread and Routines */
    Apc->Thread = Thread;
    Apc->KernelRoutine = KernelRoutine;
    Apc->RundownRoutine = RundownRoutine;
    Apc->NormalRoutine = NormalRoutine;
   
    /* Check if this is a Special APC, in which case we use KernelMode and no Context */
    if (ARGUMENT_PRESENT(NormalRoutine)) {
        
        Apc->ApcMode = Mode;
        Apc->NormalContext = Context;
        
    } else {
        
        Apc->ApcMode = KernelMode;
    }  
}

/*++
 * KiInsertQueueApc 
 *
 *     The KiInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * Params:
 *     Apc - Pointer to an initialized control object of type DPC for which the
 *           caller provides the storage. 
 *
 *     PriorityBoost - Priority Boost to apply to the Thread.
 *
 * Returns:
 *     If the APC is already inserted or APC queueing is disabled, FALSE.
 *     Otherwise, TRUE.
 *
 * Remarks:
 *     The APC will execute at APC_LEVEL for the KernelRoutine registered, and
 *     at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *     Callers of this routine must be running at IRQL = PASSIVE_LEVEL.
 *
 *--*/
BOOLEAN
STDCALL
KiInsertQueueApc(PKAPC Apc,
                 KPRIORITY PriorityBoost)
{
    PKTHREAD Thread = Apc->Thread;
    PLIST_ENTRY ApcListEntry;
    PKAPC QueuedApc;
    
    /* Don't do anything if the APC is already inserted */
    if (Apc->Inserted) {
        
        return FALSE;
    }
    
    /* Three scenarios: 
       1) Kernel APC with Normal Routine or User APC = Put it at the end of the List
       2) User APC which is PsExitSpecialApc = Put it at the front of the List
       3) Kernel APC without Normal Routine = Put it at the end of the No-Normal Routine Kernel APC list
    */
    if ((Apc->ApcMode != KernelMode) && (Apc->KernelRoutine == (PKKERNEL_ROUTINE)PsExitSpecialApc)) {
        
        DPRINT ("Inserting the Process Exit APC into the Queue\n");
        Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->UserApcPending = TRUE;
        InsertHeadList(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode],
                       &Apc->ApcListEntry);
        
    } else if (Apc->NormalRoutine == NULL) {
        
        DPRINT ("Inserting Special APC %x into the Queue\n", Apc);
        
        for (ApcListEntry = Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode].Flink;
             ApcListEntry != &Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode];
             ApcListEntry = ApcListEntry->Flink) {

            QueuedApc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
            if (Apc->NormalRoutine != NULL) break;
        }
        
        /* We found the first "Normal" APC, so write right before it */
        ApcListEntry = ApcListEntry->Blink;
        InsertHeadList(ApcListEntry, &Apc->ApcListEntry);
        
    } else {
        
        DPRINT ("Inserting Normal APC %x into the %x Queue\n", Apc, Apc->ApcMode);
        InsertTailList(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode],
                       &Apc->ApcListEntry);
    }
    
    /* Confirm Insertion */    
    Apc->Inserted = TRUE;

    /*
     * Three possibilites here again:
     *  1) Kernel APC, The thread is Running: Request an Interrupt
     *  2) Kernel APC, The Thread is Waiting at PASSIVE_LEVEL and APCs are enabled and not in progress: Unwait the Thread
     *  3) User APC, Unwait the Thread if it is alertable
     */ 
    if (Apc->ApcMode == KernelMode) { 
        
        /* Set Kernel APC pending */
        Thread->ApcState.KernelApcPending = TRUE;
        
        /* Check the Thread State */
        if (Thread->State == THREAD_STATE_RUNNING) { 
            
            /* FIXME: Use IPI */
            DPRINT ("Requesting APC Interrupt for Running Thread \n");
            HalRequestSoftwareInterrupt(APC_LEVEL);
            
        } else if ((Thread->State == THREAD_STATE_BLOCKED) && (Thread->WaitIrql == PASSIVE_LEVEL) &&
                   ((Apc->NormalRoutine == NULL) || 
                   ((!Thread->KernelApcDisable) && (!Thread->ApcState.KernelApcInProgress)))) {
          
            DPRINT("Waking up Thread for Kernel-Mode APC Delivery \n");
            KiAbortWaitThread(Thread, STATUS_KERNEL_APC, PriorityBoost);
        }
        
   } else if ((Thread->State == THREAD_STATE_BLOCKED) && 
              (Thread->WaitMode == UserMode) && 
              (Thread->Alertable)) {
       
        DPRINT("Waking up Thread for User-Mode APC Delivery \n");
        Thread->ApcState.UserApcPending = TRUE;
        KiAbortWaitThread(Thread, STATUS_USER_APC, PriorityBoost);
    }
    
    return TRUE;
}
    
/*++
 * KeInsertQueueApc 
 * @implemented NT4
 *
 *     The KeInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * Params:
 *     Apc - Pointer to an initialized control object of type DPC for which the
 *           caller provides the storage. 
 *
 *     SystemArgument[1,2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 *     PriorityBoost - Priority Boost to apply to the Thread.
 *
 * Returns:
 *     If the APC is already inserted or APC queueing is disabled, FALSE.
 *     Otherwise, TRUE.
 *
 * Remarks:
 *     The APC will execute at APC_LEVEL for the KernelRoutine registered, and
 *     at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *     Callers of this routine must be running at IRQL = PASSIVE_LEVEL.
 *
 *--*/
BOOLEAN
STDCALL
KeInsertQueueApc(PKAPC Apc,
                 PVOID SystemArgument1,
                 PVOID SystemArgument2,
                 KPRIORITY PriorityBoost)

{
    KIRQL OldIrql;
    PKTHREAD Thread;
    BOOLEAN Inserted;
   
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
           "SystemArgument2 %x)\n",Apc,SystemArgument1,
            SystemArgument2);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Get the Thread specified in the APC */
    Thread = Apc->Thread;
       
    /* Make sure the thread allows APC Queues.
    * The thread is not apc queueable, for instance, when it's (about to be) terminated.
    */
    if (Thread->ApcQueueable == FALSE) {
        DPRINT("Thread doesn't allow APC Queues\n");
        KeReleaseDispatcherDatabaseLock(OldIrql);
        return FALSE;
    }
    
    /* Set the System Arguments */
    Apc->SystemArgument1 = SystemArgument1;
    Apc->SystemArgument2 = SystemArgument2;

    /* Call the Internal Function */
    Inserted = KiInsertQueueApc(Apc, PriorityBoost);

    /* Return Sucess if we are here */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return Inserted;
}

/*++
 * KeRemoveQueueApc 
 *
 *     The KeRemoveQueueApc routine removes a given APC object from the system 
 *     APC queue.
 *
 * Params:
 *     APC - Pointer to an initialized APC object that was queued by calling
 *           KeInsertQueueApc.
 *
 * Returns:
 *     TRUE if the APC Object is in the APC Queue. If it isn't, no operation is
 *     performed and FALSE is returned.
 *
 * Remarks:
 *     If the given APC Object is currently queued, it is removed from the queue
 *     and any calls to the registered routines are cancelled.
 *
 *     Callers of KeLeaveCriticalRegion can be running at any IRQL.
 *
 *--*/
BOOLEAN 
STDCALL
KeRemoveQueueApc(PKAPC Apc)
{
    KIRQL OldIrql;
    PKTHREAD Thread = Apc->Thread;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeRemoveQueueApc called for APC: %x \n", Apc);
    
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
    
    /* Check if it's inserted */
    if (Apc->Inserted) {
        
        /* Remove it from the Queue*/
        RemoveEntryList(&Apc->ApcListEntry);
        Apc->Inserted = FALSE;
        
        /* If the Queue is completely empty, then no more APCs are pending */
        if (IsListEmpty(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode])) {
            
            /* Set the correct State based on the Apc Mode */
            if (Apc->ApcMode == KernelMode) {
                
                Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->KernelApcPending = FALSE;
                
            } else {
                
                Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->UserApcPending = FALSE;
            }
        }
        
    } else {
        
        /* It's not inserted, fail */
        KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
        KeReleaseDispatcherDatabaseLock(OldIrql);
        return(FALSE);
    }
    
    /* Restore IRQL and Return */
    KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return(TRUE);
}

/*++
 * KiDeliverApc 
 * @implemented @NT4
 *    
 *     The KiDeliverApc routine is called from IRQL switching code if the
 *     thread is returning from an IRQL >= APC_LEVEL and Kernel-Mode APCs are
 *     pending. 
 *
 * Params:
 *     DeliveryMode - Specifies the current processor mode.
 *
 *     Reserved - Pointer to the Exception Frame on non-i386 builds.
 *
 *     TrapFrame - Pointer to the Trap Frame.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     First, Special APCs are delivered, followed by Kernel-Mode APCs and
 *     User-Mode APCs. Note that the TrapFrame is only valid if the previous
 *     mode is User.
 *
 *     Upon entry, this routine executes at APC_LEVEL.
 *
 *--*/
VOID 
STDCALL
KiDeliverApc(KPROCESSOR_MODE DeliveryMode,
             PVOID Reserved,
             PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PLIST_ENTRY ApcListEntry;
    PKAPC Apc;
    KIRQL OldIrql;
    PKKERNEL_ROUTINE KernelRoutine;
    PVOID NormalContext;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
   
    ASSERT_IRQL_EQUAL(APC_LEVEL);

    /* Lock the APC Queue and Raise IRQL to Synch */
    KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);

    /* Clear APC Pending */
    Thread->ApcState.KernelApcPending = FALSE;
    
    /* Do the Kernel APCs first */
    while (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) {
    
        /* Get the next Entry */
        ApcListEntry = Thread->ApcState.ApcListHead[KernelMode].Flink;
        Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
        
        /* Save Parameters so that it's safe to free the Object in Kernel Routine*/
        NormalRoutine   = Apc->NormalRoutine;
        KernelRoutine   = Apc->KernelRoutine;
        NormalContext   = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;
       
        /* Special APC */
       if (NormalRoutine == NULL) {
           
            /* Remove the APC from the list */
            Apc->Inserted = FALSE;
            RemoveEntryList(ApcListEntry);
            
            /* Go back to APC_LEVEL */
            KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
            
            /* Call the Special APC */
            DPRINT("Delivering a Special APC: %x\n", Apc);
            KernelRoutine(Apc,
                      &NormalRoutine,
                      &NormalContext,
                      &SystemArgument1,
                      &SystemArgument2);

            /* Raise IRQL and Lock again */
            KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
            
        } else {
            
             /* Normal Kernel APC */
            if (Thread->ApcState.KernelApcInProgress || Thread->KernelApcDisable) {
            
                /*
                 * DeliveryMode must be KernelMode in this case, since one may not
                 * return to umode while being inside a critical section or while 
                 * a regular kmode apc is running (the latter should be impossible btw).
                 * -Gunnar
                 */
                ASSERT(DeliveryMode == KernelMode);

                KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
                return;
            }
            
            /* Dequeue the APC */
            RemoveEntryList(ApcListEntry);
            Apc->Inserted = FALSE;
            
            /* Go back to APC_LEVEL */
            KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
            
            /* Call the Kernel APC */
            DPRINT("Delivering a Normal APC: %x\n", Apc);
            KernelRoutine(Apc,
                      &NormalRoutine,
                      &NormalContext,
                      &SystemArgument1,
                      &SystemArgument2);
            
            /* If There still is a Normal Routine, then we need to call this at PASSIVE_LEVEL */
            if (NormalRoutine != NULL) {
                
                /* At Passive Level, this APC can be prempted by a Special APC */
                Thread->ApcState.KernelApcInProgress = TRUE;
                KeLowerIrql(PASSIVE_LEVEL);
                
                /* Call and Raise IRQ back to APC_LEVEL */
                DPRINT("Calling the Normal Routine for a Normal APC: %x\n", Apc);
                NormalRoutine(&NormalContext, &SystemArgument1, &SystemArgument2);
                KeRaiseIrql(APC_LEVEL, &OldIrql);
            }

            /* Raise IRQL and Lock again */
            KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
            Thread->ApcState.KernelApcInProgress = FALSE;
        }
    }
    
    /* Now we do the User APCs */
    if ((!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) &&
        (DeliveryMode == UserMode) && (Thread->ApcState.UserApcPending == TRUE)) {
             
        /* It's not pending anymore */
        Thread->ApcState.UserApcPending = FALSE;

        /* Get the APC Object */
        ApcListEntry = Thread->ApcState.ApcListHead[UserMode].Flink;
        Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
        
        /* Save Parameters so that it's safe to free the Object in Kernel Routine*/
        NormalRoutine   = Apc->NormalRoutine;
        KernelRoutine   = Apc->KernelRoutine;
        NormalContext   = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2; 
        
        /* Remove the APC from Queue, restore IRQL and call the APC */
        RemoveEntryList(ApcListEntry);
        Apc->Inserted = FALSE;
      
        KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
        DPRINT("Calling the Kernel Routine for for a User APC: %x\n", Apc);
        KernelRoutine(Apc,
                  &NormalRoutine,
                  &NormalContext,
                  &SystemArgument1,
                  &SystemArgument2);

        if (NormalRoutine == NULL) {
            
            /* Check if more User APCs are Pending */
            KeTestAlertThread(UserMode);
            
        } else {
            
            /* Set up the Trap Frame and prepare for Execution in NTDLL.DLL */
            DPRINT("Delivering a User APC: %x\n", Apc);
            KiInitializeUserApc(Reserved, 
                        TrapFrame,
                        NormalRoutine,
                        NormalContext,
                        SystemArgument1,
                        SystemArgument2);
        }
        
    } else {
        
        /* Go back to APC_LEVEL */
        KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
    }
}

VOID 
STDCALL
KiFreeApcRoutine(PKAPC Apc,
                 PKNORMAL_ROUTINE* NormalRoutine,
                 PVOID* NormalContext,
                 PVOID* SystemArgument1,
                 PVOID* SystemArgument2)
{
    /* Free the APC and do nothing else */
    ExFreePool(Apc);
}

/*++
 * KiInitializeUserApc 
 *    
 *     Prepares the Context for a User-Mode APC called through NTDLL.DLL
 *
 * Params:
 *     Reserved - Pointer to the Exception Frame on non-i386 builds.
 *
 *     TrapFrame - Pointer to the Trap Frame.
 *
 *     NormalRoutine - Pointer to the NormalRoutine to call.
 *
 *     NormalContext - Pointer to the context to send to the Normal Routine.
 *
 *     SystemArgument[1-2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
STDCALL
KiInitializeUserApc(IN PVOID Reserved,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)  
{
    PCONTEXT Context;
    PULONG Esp;

    DPRINT("KiInitializeUserApc(TrapFrame %x/%x)\n", TrapFrame, KeGetCurrentThread()->TrapFrame);
   
    /*
     * Save the thread's current context (in other words the registers
     * that will be restored when it returns to user mode) so the
     * APC dispatcher can restore them later
     */
    Context = (PCONTEXT)(((PUCHAR)TrapFrame->Esp) - sizeof(CONTEXT));
    RtlZeroMemory(Context, sizeof(CONTEXT));
    Context->ContextFlags = CONTEXT_FULL;
    Context->SegGs = TrapFrame->Gs;
    Context->SegFs = TrapFrame->Fs;
    Context->SegEs = TrapFrame->Es;
    Context->SegDs = TrapFrame->Ds;
    Context->Edi = TrapFrame->Edi;
    Context->Esi = TrapFrame->Esi;
    Context->Ebx = TrapFrame->Ebx;
    Context->Edx = TrapFrame->Edx;
    Context->Ecx = TrapFrame->Ecx;
    Context->Eax = TrapFrame->Eax;
    Context->Ebp = TrapFrame->Ebp;
    Context->Eip = TrapFrame->Eip;
    Context->SegCs = TrapFrame->Cs;
    Context->EFlags = TrapFrame->Eflags;
    Context->Esp = TrapFrame->Esp;
    Context->SegSs = TrapFrame->Ss;
       
    /*
     * Setup the trap frame so the thread will start executing at the
     * APC Dispatcher when it returns to user-mode
     */
    Esp = (PULONG)(((PUCHAR)TrapFrame->Esp) - (sizeof(CONTEXT) + (6 * sizeof(ULONG))));
    Esp[0] = 0xdeadbeef;
    Esp[1] = (ULONG)NormalRoutine;
    Esp[2] = (ULONG)NormalContext;
    Esp[3] = (ULONG)SystemArgument1;
    Esp[4] = (ULONG)SystemArgument2;
    Esp[5] = (ULONG)Context;
    TrapFrame->Eip = (ULONG)LdrpGetSystemDllApcDispatcher();
    TrapFrame->Esp = (ULONG)Esp;
}

/*++
 * KeAreApcsDisabled 
 * @implemented NT4
 *    
 *     Prepares the Context for a User-Mode APC called through NTDLL.DLL
 *
 * Params:
 *     None.
 *
 * Returns:
 *     KeAreApcsDisabled returns TRUE if the thread is within a critical region
 *     or a guarded region, and FALSE otherwise.
 *
 * Remarks:
 *     A thread running at IRQL = PASSIVE_LEVEL can use KeAreApcsDisabled to 
 *     determine if normal kernel APCs are disabled. A thread that is inside a 
 *     critical region has both user APCs and normal kernel APCs disabled, but 
 *     not special kernel APCs. A thread that is inside a guarded region has 
 *     all APCs disabled, including special kernel APCs.
 *
 *     Callers of this routine must be running at IRQL <= APC_LEVEL.
 *
 *--*/
BOOLEAN
STDCALL
KeAreApcsDisabled(VOID)
{
    /* Return the Kernel APC State */
    return KeGetCurrentThread()->KernelApcDisable ? TRUE : FALSE;
}

/*++
 * NtQueueApcThread 
 * NT4
 *    
 *    This routine is used to queue an APC from user-mode for the specified 
 *    thread.
 *
 * Params:
 *     Thread Handle - Handle to the Thread. This handle must have THREAD_SET_CONTEXT privileges.
 *
 *     ApcRoutine - Pointer to the APC Routine to call when the APC executes.
 *
 *     NormalContext - Pointer to the context to send to the Normal Routine.
 *
 *     SystemArgument[1-2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 * Returns:
 *     STATUS_SUCCESS or failure cute from associated calls.
 *
 * Remarks:
 *      The thread must enter an alertable wait before the APC will be 
 *      delivered.
 *
 *--*/
NTSTATUS 
STDCALL
NtQueueApcThread(HANDLE ThreadHandle,
         PKNORMAL_ROUTINE ApcRoutine,
         PVOID NormalContext,
         PVOID SystemArgument1,
         PVOID SystemArgument2)
{
    PKAPC Apc;
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    /* Get ETHREAD from Handle */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                       THREAD_SET_CONTEXT,
                       PsThreadType,
                       PreviousMode,
                       (PVOID)&Thread,
                       NULL);
    
    /* Fail if the Handle is invalid for some reason */
    if (!NT_SUCCESS(Status)) {
        
        return(Status);
    }
    
    /* If this is a Kernel or System Thread, then fail */
    if (Thread->Tcb.Teb == NULL) {
        
        ObDereferenceObject(Thread);
        return STATUS_INVALID_HANDLE;
    }
   
    /* Allocate an APC */
    Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG('P', 's', 'a', 'p'));
    if (Apc == NULL) {
        
        ObDereferenceObject(Thread);
        return(STATUS_NO_MEMORY);
    }
   
    /* Initialize and Queue a user mode apc (always!) */
    KeInitializeApc(Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    KiFreeApcRoutine,
                    NULL,
                    ApcRoutine,
                    UserMode,
                    NormalContext);
    
    if (!KeInsertQueueApc(Apc, SystemArgument1, SystemArgument2, IO_NO_INCREMENT)) {
        
        Status = STATUS_UNSUCCESSFUL;
        
    } else {
        
        Status = STATUS_SUCCESS;
    }
   
    /* Dereference Thread and Return */
    ObDereferenceObject(Thread);
    return Status;
}

static inline 
VOID RepairList(PLIST_ENTRY Original, 
                PLIST_ENTRY Copy,
                KPROCESSOR_MODE Mode)
{
    /* Copy Source to Desination */
    if (IsListEmpty(&Original[(int)Mode])) {
        
        InitializeListHead(&Copy[(int)Mode]);
        
    } else {
        
        Copy[(int)Mode].Flink = Original[(int)Mode].Flink; 
        Copy[(int)Mode].Blink = Original[(int)Mode].Blink;
        Original[(int)Mode].Flink->Blink = &Copy[(int)Mode];
        Original[(int)Mode].Blink->Flink = &Copy[(int)Mode];
    }
}

VOID
STDCALL
KiMoveApcState(PKAPC_STATE OldState,
               PKAPC_STATE NewState)
{
    /* Restore backup of Original Environment */
    *NewState = *OldState;
    
    /* Repair Lists */
    RepairList(NewState->ApcListHead, OldState->ApcListHead, KernelMode);
    RepairList(NewState->ApcListHead, OldState->ApcListHead, UserMode);
}

