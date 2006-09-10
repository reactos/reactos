/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/dpc.c
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
KiQuantumEnd(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;
    PKPROCESS Process;
    KPRIORITY OldPriority;
    KPRIORITY NewPriority;

    /* Check if a DPC Event was requested to be signaled */
    if (InterlockedExchange(&Prcb->DpcSetEventRequest, 0))
    {
        /* Signal it */
        KeSetEvent(&Prcb->DpcEvent, 0, 0);
    }

    /* Lock dispatcher */
    OldIrql = KeRaiseIrqlToSynchLevel();
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Get the Thread's Process */
    Process = CurrentThread->ApcState.Process;

    /* Check if Quantum expired */
    if (CurrentThread->Quantum <= 0)
    {
        /* Reset the new Quantum */
        CurrentThread->Quantum = CurrentThread->QuantumReset;

        /* Calculate new priority */
        OldPriority = CurrentThread->Priority;
        if (OldPriority < LOW_REALTIME_PRIORITY)
        {
            /* Set the New Priority and add the Priority Decrement */
            NewPriority = OldPriority - CurrentThread->PriorityDecrement - 1;

            /* Don't go out of bounds */
            if (NewPriority < CurrentThread->BasePriority)
            {
                NewPriority = CurrentThread->BasePriority;
            }

            /* Reset the priority decrement */
            CurrentThread->PriorityDecrement = 0;

            /* Set a new priority if needed */
            if (OldPriority != NewPriority)
            {
                /* Set new Priority */
                BOOLEAN Dummy; /* <- This is a hack anyways... */
                KiSetPriorityThread(CurrentThread, NewPriority, &Dummy);
            }
            else
            {
                /* Queue new thread if none is already */
                if (!Prcb->NextThread)
                {
                    /* FIXME: Schedule a New Thread, when ROS will have NT Scheduler */
                }
                else
                {
                    /* Make the current thread non-premeptive if a new thread is queued */
                    CurrentThread->Preempted = FALSE;
                }
            }
        }
        else
        {
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

VOID
FASTCALL
KiRetireDpcList(IN PKPRCB Prcb)
{
    PKDPC_DATA DpcData = Prcb->DpcData;
    PLIST_ENTRY DpcEntry;
    PKDPC Dpc;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext, SystemArgument1, SystemArgument2;

    /* Main outer loop */
    do
    {
        /* Set us as active */
        Prcb->DpcRoutineActive = TRUE;

        /* Check if this is a timer expiration request */
        if (Prcb->TimerRequest)
        {
            /* FIXME: Not yet implemented */
            ASSERT(FALSE);
        }

        /* Loop while we have entries in the queue */
        while (DpcData->DpcQueueDepth)
        {
            /* Lock the DPC data */
            KefAcquireSpinLockAtDpcLevel(&DpcData->DpcLock);

            /* Make sure we have an entry */
            if (!IsListEmpty(&DpcData->DpcListHead))
            {
                /* Remove the DPC from the list */
                DpcEntry = RemoveHeadList(&DpcData->DpcListHead);
                Dpc = CONTAINING_RECORD(DpcEntry, KDPC, DpcListEntry);

                /* Clear its DPC data and save its parameters */
                Dpc->DpcData = NULL;
                DeferredRoutine = Dpc->DeferredRoutine;
                DeferredContext = Dpc->DeferredContext;
                SystemArgument1 = Dpc->SystemArgument1;
                SystemArgument2 = Dpc->SystemArgument2;

                /* Decrease the queue depth */
                DpcData->DpcQueueDepth--;

                /* Clear DPC Time */
                Prcb->DebugDpcTime = 0;

                /* Release the lock */
                KefReleaseSpinLockFromDpcLevel(&DpcData->DpcLock);

                /* Re-enable interrupts */
                Ke386EnableInterrupts();

                /* Call the DPC */
                DeferredRoutine(Dpc,
                                DeferredContext,
                                SystemArgument1,
                                SystemArgument2);
                ASSERT_IRQL(DISPATCH_LEVEL);

                /* Disable interrupts and keep looping */
                Ke386DisableInterrupts();
            }
            else
            {
                /* The queue should be flushed now */
                ASSERT(DpcData->DpcQueueDepth == 0);

                /* Release DPC Lock */
                KefReleaseSpinLockFromDpcLevel(&DpcData->DpcLock);
                break;
            }
        }

        /* Clear DPC Flags */
        Prcb->DpcRoutineActive = FALSE;
        Prcb->DpcInterruptRequested = FALSE;

        /* Check if we have deferred threads */
        if (Prcb->DeferredReadyListHead.Next)
        {
            /* FIXME: 2K3-style scheduling not implemeted */
            ASSERT(FALSE);
        }
    } while (DpcData->DpcQueueDepth);
}

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
                        (!(AFFINITY_MASK(Cpu) & KiIdleSummary) ||
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

    /* Check if this is an UP machine */
    if (KeActiveProcessors == 1)
    {
        /* Check if there are DPCs on either queues */
        if ((KeGetCurrentPrcb()->DpcData[DPC_NORMAL].DpcQueueDepth) ||
            (KeGetCurrentPrcb()->DpcData[DPC_THREADED].DpcQueueDepth))
        {
            /* Request an interrupt */
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
    }
    else
    {
        /* FIXME: SMP support required */
        ASSERT(FALSE);
    }
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

/* EOF */
