/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Philip Susi (phreak@iag.net)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES ******************************************************************/

#define NTDDI_VERSION NTDDI_WS03

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KiMaximumDpcQueueDepth = 4;
ULONG KiMinimumDpcRate = 3;
ULONG KiAdjustDpcThreshold = 20;
ULONG KiIdealDpcRate = 20;
KMUTEX KiGenericCallDpcMutex;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiInitializeDpc(IN PKDPC Dpc,
                IN PKDEFERRED_ROUTINE DeferredRoutine,
                IN PVOID DeferredContext,
                IN KOBJECTS Type)
{
    /* Setup the DPC Object */
    Dpc->Type = Type;
    Dpc->Number= 0;
    Dpc->Importance= MediumImportance;
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->DpcData = NULL;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeThreadedDpc(IN PKDPC Dpc,
                        IN PKDEFERRED_ROUTINE DeferredRoutine,
                        IN PVOID DeferredContext)
{
    /* Call the internal routine */
    KiInitializeDpc(Dpc, DeferredRoutine, DeferredContext, ThreadedDpcObject);
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeDpc(IN PKDPC Dpc,
                IN PKDEFERRED_ROUTINE DeferredRoutine,
                IN PVOID DeferredContext)
{
    /* Call the internal routine */
    KiInitializeDpc(Dpc, DeferredRoutine, DeferredContext, DpcObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeInsertQueueDpc(IN PKDPC Dpc,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    KIRQL OldIrql;
    PKPRCB Prcb, CurrentPrcb = KeGetCurrentPrcb();
    ULONG Cpu;
    PKDPC_DATA DpcData;
    BOOLEAN DpcConfigured = FALSE, DpcInserted = FALSE;
    ASSERT_DPC(Dpc);

    /* Check IRQL and Raise it to HIGH_LEVEL */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check if the DPC has more then the maximum number of CPUs */
    if (Dpc->Number >= MAXIMUM_PROCESSORS)
    {
        /* Then substract the maximum and get that PRCB. */
        Cpu = Dpc->Number - MAXIMUM_PROCESSORS;
        Prcb = KiProcessorBlock[Cpu];
    }
    else
    {
        /* Use the current one */
        Prcb = CurrentPrcb;
        Cpu = Prcb->Number;
    }

    /* Check if this is a threaded DPC and threaded DPCs are enabled */
    if ((Dpc->Type = ThreadedDpcObject) && (Prcb->ThreadDpcEnable))
    {
        /* Then use the threaded data */
        DpcData = &Prcb->DpcData[DPC_THREADED];
    }
    else
    {
        /* Otherwise, use the regular data */
        DpcData = &Prcb->DpcData[DPC_NORMAL];
    }

    /* Acquire the DPC lock */
    KiAcquireSpinLock(&DpcData->DpcLock);

    /* Get the DPC Data */
    if (!InterlockedCompareExchangePointer(&Dpc->DpcData, DpcData, NULL))
    {
        /* Now we can play with the DPC safely */
        Dpc->SystemArgument1 = SystemArgument1;
        Dpc->SystemArgument2 = SystemArgument2;
        DpcData->DpcQueueDepth++;
        DpcData->DpcCount++;
        DpcConfigured = TRUE;

        /* Check if this is a high importance DPC */
        if (Dpc->Importance == HighImportance)
        {
            /* Pre-empty other DPCs */
            InsertHeadList(&DpcData->DpcListHead, &Dpc->DpcListEntry);
        }
        else
        {
            /* Add it at the end */
            InsertTailList(&DpcData->DpcListHead, &Dpc->DpcListEntry);
        }

        /* Check if this is the DPC on the threaded list */
        if (&Prcb->DpcData[DPC_THREADED].DpcListHead == &DpcData->DpcListHead)
        {
            /* Make sure a threaded DPC isn't already active */
            if (!(Prcb->DpcThreadActive) && (!Prcb->DpcThreadRequested))
            {
                /* FIXME: Setup Threaded DPC */
                ASSERT(FALSE);
            }
        }
        else
        {
            /* Make sure a DPC isn't executing already */
            if ((!Prcb->DpcRoutineActive) && (!Prcb->DpcInterruptRequested))
            {
                /* Check if this is the same CPU */
                if (Prcb != CurrentPrcb)
                {
                    /*
                     * Check if the DPC is of high importance or above the
                     * maximum depth. If it is, then make sure that the CPU
                     * isn't idle, or that it's sleeping.
                     */
                    if (((Dpc->Importance == HighImportance) ||
                        (DpcData->DpcQueueDepth >=
                         Prcb->MaximumDpcQueueDepth)) &&
                        (!(AFFINITY_MASK(Cpu) & IdleProcessorMask) ||
                         (Prcb->Sleeping)))
                    {
                        /* Set interrupt requested */
                        Prcb->DpcInterruptRequested = TRUE;

                        /* Set DPC inserted */
                        DpcInserted = TRUE;
                    }
                }
                else
                {
                    /* Check if the DPC is of anything but low importance */
                    if ((Dpc->Importance != LowImportance) ||
                        (DpcData->DpcQueueDepth >=
                         Prcb->MaximumDpcQueueDepth) ||
                        (Prcb->DpcRequestRate < Prcb->MinimumDpcRate))
                    {
                        /* Set interrupt requested */
                        Prcb->DpcInterruptRequested = TRUE;

                        /* Set DPC inserted */
                        DpcInserted = TRUE;
                    }
                }
            }
        }
    }

    /* Release the lock */
    KiReleaseSpinLock(&DpcData->DpcLock);

    /* Check if the DPC was inserted */
    if (DpcInserted)
    {
        /* Check if this was SMP */
        if (Prcb != CurrentPrcb)
        {
            /* It was, request and IPI */
            KiIpiSendRequest(AFFINITY_MASK(Cpu), IPI_DPC);
        }
        else
        {
            /* It wasn't, request an interrupt from HAL */
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
    }

    /* Lower IRQL */
    KeLowerIrql(OldIrql);
    return DpcConfigured;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRemoveQueueDpc(IN PKDPC Dpc)
{
    PKDPC_DATA DpcData;
    UCHAR DpcType;
    ASSERT_DPC(Dpc);

    /* Disable interrupts */
    Ke386DisableInterrupts();

    /* Get DPC data and type */
    DpcType = Dpc->Type;
    DpcData = Dpc->DpcData;
    if (DpcData)
    {
        /* Acquire the DPC lock */
        KiAcquireSpinLock(&DpcData->DpcLock);

        /* Make sure that the data didn't change */
        if (DpcData == Dpc->DpcData)
        {
            /* Remove the DPC */
            DpcData->DpcQueueDepth--;
            RemoveEntryList(&Dpc->DpcListEntry);
            Dpc->DpcData = NULL;
        }

        /* Release the lock */
        KiReleaseSpinLock(&DpcData->DpcLock);
    }

    /* Re-enable interrupts */
    Ke386EnableInterrupts();

    /* Return if the DPC was in the queue or not */
    return DpcData ? TRUE : FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushQueuedDpcs(VOID)
{
    PAGED_CODE();

    /* Request an interrupt if needed */
    DPRINT1("%s - FIXME!!!\n", __FUNCTION__);
    if (KeGetCurrentPrcb()->DpcData[0].DpcQueueDepth) HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeIsExecutingDpc(VOID)
{
    /* Return if the Dpc Routine is active */
    return KeGetCurrentPrcb()->DpcRoutineActive;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetImportanceDpc (IN PKDPC Dpc,
                    IN KDPC_IMPORTANCE Importance)
{
    /* Set the DPC Importance */
    ASSERT_DPC(Dpc);
    Dpc->Importance = Importance;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetTargetProcessorDpc(IN PKDPC Dpc,
                        IN CCHAR Number)
{
    /* Set a target CPU */
    ASSERT_DPC(Dpc);
    Dpc->Number = Number + MAXIMUM_PROCESSORS;
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
    if (Prcb->DpcSetEventRequest)
    {
        KeSetEvent(&Prcb->DpcEvent, 0, 0);
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
