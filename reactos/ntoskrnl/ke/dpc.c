/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Philip Susi (phreak@iag.net)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *                  Alex Ionescu (alex@relsoft.net)
 */

/*
 * NOTE: See also the higher level support routines in ntoskrnl/io/dpc.c
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define MAX_QUANTUM 0x7F

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Initialize DPC handling
 */
VOID
INIT_FUNCTION
KeInitDpc(PKPRCB Prcb)
{
   InitializeListHead(&Prcb->DpcData[0].DpcListHead);
#if 0
   /*
    * FIXME:
    *   Prcb->DpcEvent is a NULL pointer.
    */
   KeInitializeEvent(Prcb->DpcEvent, 0, 0);
#endif
   KeInitializeSpinLock(&Prcb->DpcData[0].DpcLock);
   Prcb->MaximumDpcQueueDepth = 4;
   Prcb->MinimumDpcRate = 3;
   Prcb->DpcData[0].DpcQueueDepth = 0;
}

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeThreadedDpc(PKDPC Dpc,
                        PKDEFERRED_ROUTINE DeferredRoutine,
                        PVOID DeferredContext)
/*
 * FUNCTION:
 *          Initalizes a Threaded DPC and registers the DeferredRoutine for it.
 * ARGUMENTS:
 *          Dpc = Pointer to a caller supplied DPC to be initialized. The caller must allocate this memory.
 *          DeferredRoutine = Pointer to associated DPC callback routine.
 *          DeferredContext = Parameter to be passed to the callback routine.
 * NOTE: Callers can be running at any IRQL.
 */
{
    DPRINT("Threaded DPC Initializing: %x with Routine: %x\n", Dpc, DeferredRoutine);
    Dpc->Type = ThreadedDpcObject;
    Dpc->Number= 0;
    Dpc->Importance= MediumImportance;
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->DpcData = NULL;
}

/*
 * @implemented
 *
 * FUNCTION:
 *          Initalizes a DPC and registers the DeferredRoutine for it.
 * ARGUMENTS:
 *          Dpc = Pointer to a caller supplied DPC to be initialized. The caller must allocate this memory.
 *          DeferredRoutine = Pointer to associated DPC callback routine.
 *          DeferredContext = Parameter to be passed to the callback routine.
 * NOTE: Callers can be running at any IRQL.
 */
VOID
STDCALL
KeInitializeDpc(PKDPC Dpc,
                PKDEFERRED_ROUTINE DeferredRoutine,
                PVOID DeferredContext)
{
    DPRINT("DPC Initializing: %x with Routine: %x\n", Dpc, DeferredRoutine);
    Dpc->Type = DpcObject;
    Dpc->Number= 0;
    Dpc->Importance= MediumImportance;
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->DpcData = NULL;
}

/*
 * @implemented
 *
 * FUNCTION:
 *          Queues a DPC for execution when the IRQL of a processor
 *          drops below DISPATCH_LEVEL
 * ARGUMENTS:
 *          Dpc = Pointed to a DPC Object Initalized by KeInitializeDpc.
 *          SystemArgument1 = Driver Determined context data
 *          SystemArgument2 = Driver Determined context data
 * RETURNS:
 *          TRUE if the DPC object wasn't already in the queue
 *          FALSE otherwise
 * NOTES:
 *          If there is currently a DPC active on the target processor, or a DPC
 * interrupt has already been requested on the target processor when a
 * DPC is queued, then no further action is necessary. The DPC will be
 * executed on the target processor when its queue entry is processed.
 *
 *          If there is not a DPC active on the target processor and a DPC interrupt
 * has not been requested on the target processor, then the exact treatment
 * of the DPC is dependent on whether the host system is a UP system or an
 * MP system.
 *
 * UP system.
 * ----------
 *          If the DPC is of medium or high importance, the current DPC queue depth
 * is greater than the maximum target depth, or current DPC request rate is
 * less the minimum target rate, then a DPC interrupt is requested on the
 * host processor and the DPC will be processed when the interrupt occurs.
 * Otherwise, no DPC interupt is requested and the DPC execution will be
 * delayed until the DPC queue depth is greater that the target depth or the
 * minimum DPC rate is less than the target rate.
 *
 * MP system.
 * ----------
 *          If the DPC is being queued to another processor and the depth of the DPC
 * queue on the target processor is greater than the maximum target depth or
 * the DPC is of high importance, then a DPC interrupt is requested on the
 * target processor and the DPC will be processed when the interrupt occurs.
 * Otherwise, the DPC execution will be delayed on the target processor until
 * the DPC queue depth on the target processor is greater that the maximum
 * target depth or the minimum DPC rate on the target processor is less than
 * the target mimimum rate.
 *
 *          If the DPC is being queued to the current processor and the DPC is not of
 * low importance, the current DPC queue depth is greater than the maximum
 * target depth, or the minimum DPC rate is less than the minimum target rate,
 * then a DPC interrupt is request on the current processor and the DPV will
 * be processed whne the interrupt occurs. Otherwise, no DPC interupt is
 * requested and the DPC execution will be delayed until the DPC queue depth
 * is greater that the target depth or the minimum DPC rate is less than the
 * target rate.
 */
BOOLEAN
STDCALL
KeInsertQueueDpc(PKDPC Dpc,
                 PVOID SystemArgument1,
                 PVOID SystemArgument2)
{
    KIRQL OldIrql;
    PKPRCB Prcb;

    DPRINT("KeInsertQueueDpc(DPC %x, SystemArgument1 %x, SystemArgument2 %x)\n",
        Dpc, SystemArgument1, SystemArgument2);

    /* Check IRQL and Raise it to HIGH_LEVEL */
    ASSERT(KeGetCurrentIrql()>=DISPATCH_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check if this is a Thread DPC, which we don't support (yet) */
    if (Dpc->Type == ThreadedDpcObject) {
        return FALSE;
        KeLowerIrql(OldIrql);
    }

#ifdef CONFIG_SMP
    /* Get the right PCR for this CPU */
    if (Dpc->Number >= MAXIMUM_PROCESSORS) {

        ASSERT (Dpc->Number - MAXIMUM_PROCESSORS < KeNumberProcessors);
        Prcb = ((PKPCR)((ULONG_PTR)KPCR_BASE + ((Dpc->Number - MAXIMUM_PROCESSORS) * PAGE_SIZE)))->Prcb;

    } else {

        ASSERT (Dpc->Number < KeNumberProcessors);
        Prcb = KeGetCurrentPrcb();
        Dpc->Number = KeGetCurrentProcessorNumber();
    }

    KiAcquireSpinLock(&Prcb->DpcData[0].DpcLock);
#else
    Prcb = ((PKPCR)KPCR_BASE)->Prcb;
#endif

    /* Get the DPC Data */
    if (InterlockedCompareExchangeUL(&Dpc->DpcData, &Prcb->DpcData[0].DpcLock, 0)) {

        DPRINT("DPC Already Inserted");
#ifdef CONFIG_SMP
        KiReleaseSpinLock(&Prcb->DpcData[0].DpcLock);
#endif
        KeLowerIrql(OldIrql);
        return(FALSE);
    }

    /* Make sure the lists are free if the Queue is 0 */
    if (Prcb->DpcData[0].DpcQueueDepth == 0) {

        ASSERT(IsListEmpty(&Prcb->DpcData[0].DpcListHead));
    } else {

        ASSERT(!IsListEmpty(&Prcb->DpcData[0].DpcListHead));
    }

    /* Now we can play with the DPC safely */
    Dpc->SystemArgument1=SystemArgument1;
    Dpc->SystemArgument2=SystemArgument2;
    Prcb->DpcData[0].DpcQueueDepth++;
    Prcb->DpcData[0].DpcCount++;

    /* Insert the DPC into the list. HighImportance DPCs go at the beginning  */
    if (Dpc->Importance == HighImportance) {

        InsertHeadList(&Prcb->DpcData[0].DpcListHead, &Dpc->DpcListEntry);
    } else {

        InsertTailList(&Prcb->DpcData[0].DpcListHead, &Dpc->DpcListEntry);
    }
    DPRINT("New DPC Added. Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);

    /* Make sure a DPC isn't executing already and respect rules outlined above. */
    if ((!Prcb->DpcRoutineActive) && (!Prcb->DpcInterruptRequested)) {

#ifdef CONFIG_SMP
        /* Check if this is the same CPU */
        if (Prcb != KeGetCurrentPrcb()) {

            /* Send IPI if High Importance */
            if ((Dpc->Importance == HighImportance) ||
                (Prcb->DpcData[0].DpcQueueDepth >= Prcb->MaximumDpcQueueDepth)) {

                if (Dpc->Number >= MAXIMUM_PROCESSORS) {

                    KiIpiSendRequest(1 << (Dpc->Number - MAXIMUM_PROCESSORS), IPI_REQUEST_DPC);
                } else {

                    KiIpiSendRequest(1 << Dpc->Number, IPI_REQUEST_DPC);
                }

            }
        } else {

            /* Request an Interrupt only if the DPC isn't low priority */
            if ((Dpc->Importance != LowImportance) ||
                 (Prcb->DpcData[0].DpcQueueDepth >= Prcb->MaximumDpcQueueDepth) ||
                (Prcb->DpcRequestRate < Prcb->MinimumDpcRate)) {

                /* Request Interrupt */
                HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
                Prcb->DpcInterruptRequested = TRUE;
            }
        }
#else
        DPRINT("Requesting Interrupt. Importance: %x. QueueDepth: %x. MaxQueue: %x . RequestRate: %x. MinRate:%x \n", Dpc->Importance, Prcb->DpcData[0].DpcQueueDepth, Prcb->MaximumDpcQueueDepth, Prcb->DpcRequestRate, Prcb->MinimumDpcRate);

        /* Request an Interrupt only if the DPC isn't low priority */
        if ((Dpc->Importance != LowImportance) ||
            (Prcb->DpcData[0].DpcQueueDepth >= Prcb->MaximumDpcQueueDepth) ||
            (Prcb->DpcRequestRate < Prcb->MinimumDpcRate)) {

            /* Request Interrupt */
            DPRINT("Requesting Interrupt\n");
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
            Prcb->DpcInterruptRequested = TRUE;
        }
#endif
    }
#ifdef CONFIG_SMP
    KiReleaseSpinLock(&Prcb->DpcData[0].DpcLock);
#endif
    /* Lower IRQL */
    KeLowerIrql(OldIrql);
    return(TRUE);
}

/*
 * @implemented
 *
 * FUNCTION:
 *          Removes DPC object from the system dpc queue
 * ARGUMENTS:
 *          Dpc = Pointer to DPC to remove from the queue.
 * RETURNS:
 *          TRUE if the DPC was in the queue
 *          FALSE otherwise
 */
BOOLEAN
STDCALL
KeRemoveQueueDpc(PKDPC Dpc)
{
    BOOLEAN WasInQueue;
    KIRQL OldIrql;

    /* Raise IRQL */
    DPRINT("Removing DPC: %x\n", Dpc);
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#ifdef CONFIG_SMP
    KiAcquireSpinLock(&((PKDPC_DATA)Dpc->DpcData)->DpcLock);
#endif

    /* First make sure the DPC lock isn't being held */
    WasInQueue = Dpc->DpcData ? TRUE : FALSE;
    if (Dpc->DpcData) {

        /* Remove the DPC */
        ((PKDPC_DATA)Dpc->DpcData)->DpcQueueDepth--;
        RemoveEntryList(&Dpc->DpcListEntry);

    }
#ifdef CONFIG_SMP
        KiReleaseSpinLock(&((PKDPC_DATA)Dpc->DpcData)->DpcLock);
#endif

    /* Return if the DPC was in the queue or not */
    KeLowerIrql(OldIrql);
    return WasInQueue;
}

/*
 * @implemented
 */
VOID
STDCALL
KeFlushQueuedDpcs(VOID)
/*
 * FUNCTION:
 *          Called to Deliver DPCs if any are pending.
 * NOTES:
 *          Called when deleting a Driver.
 */
{
    /* Request an interrupt if needed */
    if (KeGetCurrentPrcb()->DpcData[0].DpcQueueDepth) HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeIsExecutingDpc(
    VOID
)
{
    /* Return if the Dpc Routine is active */
    return KeGetCurrentPrcb()->DpcRoutineActive;
}

/*
 * FUNCTION: Specifies the DPCs importance
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Importance = DPC importance
 * RETURNS: None
 *
 * @implemented
 */
VOID
STDCALL
KeSetImportanceDpc (IN PKDPC Dpc,
            IN KDPC_IMPORTANCE Importance)
{
    /* Set the DPC Importance */
    Dpc->Importance = Importance;
}

/*
 * @implemented
 *
 * FUNCTION: Specifies on which processor the DPC will run
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Number = Processor number
 * RETURNS: None
 */
VOID
STDCALL
KeSetTargetProcessorDpc(IN PKDPC Dpc,
                        IN CCHAR Number)
{
    /* Check how many CPUs are on the system */
    if (Number >= MAXIMUM_PROCESSORS) {

        /* No CPU Number */
        Dpc->Number = 0;

    } else {

        /* Set the Number Specified */
        ASSERT(Number < KeNumberProcessors);
        Dpc->Number = Number + MAXIMUM_PROCESSORS;
    }
}

/*
 * FUNCTION:
 *          Called when a quantum end occurs to check if priority should be changed
 *          and wether a new thread should be dispatched.
 * NOTES:
 *          Called when deleting a Driver.
 */
VOID
STDCALL
KiQuantumEnd(VOID)
{
    PKPRCB Prcb;
    PKTHREAD CurrentThread;
    KIRQL OldIrql;
    PKPROCESS Process;
    KPRIORITY OldPriority;
    KPRIORITY NewPriority;

    /* Lock dispatcher, get current thread */
    Prcb = KeGetCurrentPrcb();
    CurrentThread = KeGetCurrentThread();
    OldIrql = KeRaiseIrqlToSynchLevel();

    /* Get the Thread's Process */
    Process = CurrentThread->ApcState.Process;

    /* Set DPC Event if requested */
    if (Prcb->DpcSetEventRequest) {
        /*
         * FIXME:
         *   Prcb->DpcEvent is not initialized.
         */
        KEBUGCHECK(0);
        KeSetEvent(Prcb->DpcEvent, 0, 0);
    }

    /* Check if Quantum expired */
    if (CurrentThread->Quantum <= 0) {

        /* Reset the new Quantum */
        CurrentThread->Quantum = CurrentThread->QuantumReset;

        /* Calculate new priority */
        OldPriority = CurrentThread->Priority;
        if (OldPriority < LOW_REALTIME_PRIORITY) {

            /* Set the New Priority and add the Priority Decrement */
            NewPriority = OldPriority - CurrentThread->PriorityDecrement - 1;

            /* Don't go out of bounds */
            if (NewPriority < CurrentThread->BasePriority) NewPriority = CurrentThread->BasePriority;

            /* Reset the priority decrement */
            CurrentThread->PriorityDecrement = 0;

            /* Set a new priority if needed */
            if (OldPriority != NewPriority) {

                /* Set new Priority */
                BOOLEAN Dummy; /* <- This is a hack anyways... */
                KiSetPriorityThread(CurrentThread, NewPriority, &Dummy);

            } else {

                /* Queue new thread if none is already */
                if (Prcb->NextThread == NULL) {

                    /* FIXME: Schedule a New Thread, when ROS will have NT Scheduler */

                } else {

                    /* Make the current thread non-premeptive if a new thread is queued */
                    CurrentThread->Preempted = FALSE;
                }
            }


        } else {
            /* Set the Quantum back to Maximum */
            //if (CurrentThread->DisableQuantum) {
            //    CurrentThread->Quantum = MAX_QUANTUM;
            //}
        }
    }

    /* Dispatch the Thread */
    KeLowerIrql(DISPATCH_LEVEL);
    KiDispatchThread(Ready);
}

/*
 * @implemented
 *
 * FUNCTION:
 *          Called whenever a system interrupt is generated at DISPATCH_LEVEL.
 *          It delivers queued DPCs and dispatches a new thread if need be.
 */
VOID
STDCALL
KiDispatchInterrupt(VOID)
{
    PLIST_ENTRY DpcEntry;
    PKDPC Dpc;
    KIRQL OldIrql;
    PKPRCB Prcb;

    DPRINT("Dispatching Interrupts\n");
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Set DPC Deliver to Active */
    Prcb = KeGetCurrentPrcb();

    if (Prcb->DpcData[0].DpcQueueDepth > 0) {
        /* Raise IRQL */
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#ifdef CONFIG_SMP
        KiAcquireSpinLock(&Prcb->DpcData[0].DpcLock);
#endif
            Prcb->DpcRoutineActive = TRUE;

        DPRINT("&Prcb->DpcData[0].DpcListHead: %x\n", &Prcb->DpcData[0].DpcListHead);
        /* Loop while we have entries */
        while (!IsListEmpty(&Prcb->DpcData[0].DpcListHead)) {

            ASSERT(Prcb->DpcData[0].DpcQueueDepth > 0);
            DPRINT("Queue Depth: %x\n", Prcb->DpcData[0].DpcQueueDepth);

            /* Get the DPC call it */
            DpcEntry = RemoveHeadList(&Prcb->DpcData[0].DpcListHead);
            Dpc = CONTAINING_RECORD(DpcEntry, KDPC, DpcListEntry);
            DPRINT("Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);
            Dpc->DpcData = NULL;
            Prcb->DpcData[0].DpcQueueDepth--;
#ifdef CONFIG_SMP
            KiReleaseSpinLock(&Prcb->DpcData[0].DpcLock);
#endif
            /* Disable/Enabled Interrupts and Call the DPC */
            KeLowerIrql(OldIrql);
            DPRINT("Calling DPC: %x\n", Dpc);
            Dpc->DeferredRoutine(Dpc,
                         Dpc->DeferredContext,
                         Dpc->SystemArgument1,
                         Dpc->SystemArgument2);
            KeRaiseIrql(HIGH_LEVEL, &OldIrql);

#ifdef CONFIG_SMP
            KiAcquireSpinLock(&Prcb->DpcData[0].DpcLock);
            /*
             * If the dpc routine drops the irql below DISPATCH_LEVEL,
             * a thread switch can occur and after the next thread switch
             * the execution may start on an other processor.
             */
            if (Prcb != KeGetCurrentPrcb()) {

                Prcb->DpcRoutineActive = FALSE;
                KiReleaseSpinLock(&Prcb->DpcData[0].DpcLock);
                Prcb = KeGetCurrentPrcb();
                KiAcquireSpinLock(&Prcb->DpcData[0].DpcLock);
                Prcb->DpcRoutineActive = TRUE;
            }
#endif
        }
        /* Clear DPC Flags */
        Prcb->DpcRoutineActive = FALSE;
        Prcb->DpcInterruptRequested = FALSE;
#ifdef CONFIG_SMP
        KiReleaseSpinLock(&Prcb->DpcData[0].DpcLock);
#endif

        /* DPC Dispatching Ended, re-enable interrupts */
        KeLowerIrql(OldIrql);
    }

    DPRINT("Checking for Quantum End\n");

    /* If we have Quantum End, call the function */
    if (Prcb->QuantumEnd) {

        Prcb->QuantumEnd = FALSE;
        KiQuantumEnd();
    }
}

/* EOF */
