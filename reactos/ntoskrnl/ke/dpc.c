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
BOOLEAN KeThreadDpcEnable;
KMUTEX KiGenericCallDpcMutex;

/* PRIVATE FUNCTIONS *********************************************************/

//
// This routine executes at the end of a thread's quantum.
// If the thread's quantum has expired, then a new thread is attempted
// to be scheduled.
//
// If no candidate thread has been found, the routine will return, otherwise
// it will swap contexts to the next scheduled thread.
//
VOID
NTAPI
KiQuantumEnd(VOID)
{
    KPRIORITY Priority;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD NextThread, Thread = Prcb->CurrentThread;

    /* Check if a DPC Event was requested to be signaled */
    if (InterlockedExchange(&Prcb->DpcSetEventRequest, 0))
    {
        /* Signal it */
        KeSetEvent(&Prcb->DpcEvent, 0, 0);
    }

    /* Raise to synchronization level and lock the PRCB and thread */
    KeRaiseIrqlToSynchLevel();
    KiAcquireThreadLock(Thread);
    KiAcquirePrcbLock(Prcb);

    /* Check if Quantum expired */
    if (Thread->Quantum <= 0)
    {
        /* Make sure that we're not real-time or without a quantum */
        if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
            !(Thread->ApcState.Process->DisableQuantum))
        {
            /* Reset the new Quantum */
            Thread->Quantum = Thread->QuantumReset;

            /* Calculate new priority */
            Priority = Thread->Priority = KiComputeNewPriority(Thread);

            /* Check if a new thread is scheduled */
            if (!Prcb->NextThread)
            {
                /* FIXME: TODO. Add code from new scheduler */
            }
            else
            {
                /* Otherwise, make sure that this thread doesn't get preempted */
                Thread->Preempted = FALSE;
            }
        }
        else
        {
            /* Otherwise, set maximum quantum */
            Thread->Quantum = MAX_QUANTUM;
        }
    }

    /* Release the thread lock */
    KiReleaseThreadLock(Thread);

    /* Check if there's no thread scheduled */
    if (!Prcb->NextThread)
    {
        /* Just leave now */
        KiReleasePrcbLock(Prcb);
        KeLowerIrql(DISPATCH_LEVEL);
        KiDispatchThread(Ready); // FIXME: ROS
        return;
    }

    /* This shouldn't happen on ROS yet */
    DPRINT1("The impossible happened - Tell Alex\n");
    ASSERT(FALSE);

    /* Get the next thread now */
    NextThread = Prcb->NextThread;

    /* Set current thread's swap busy to true */
    KiSetThreadSwapBusy(Thread);

    /* Switch threads in PRCB */
    Prcb->NextThread = NULL;
    Prcb->CurrentThread = NextThread;

    /* Set thread to running and the switch reason to Quantum End */
    NextThread->State = Running;
    Thread->WaitReason = WrQuantumEnd;

    /* Queue it on the ready lists */
    KxQueueReadyThread(Thread, Prcb);

    /* Set wait IRQL to APC_LEVEL */
    Thread->WaitIrql = APC_LEVEL;

    /* Swap threads */
    KiSwapContext(Thread, NextThread);

    /* Lower IRQL back to DISPATCH_LEVEL */
    KeLowerIrql(DISPATCH_LEVEL);
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
