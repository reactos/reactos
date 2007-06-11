/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Deferred Procedure Call (DPC) Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Philip Susi (phreak@iag.net)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KiMaximumDpcQueueDepth = 4;
ULONG KiMinimumDpcRate = 3;
ULONG KiAdjustDpcThreshold = 20;
ULONG KiIdealDpcRate = 20;
BOOLEAN KeThreadDpcEnable;
FAST_MUTEX KiGenericCallDpcMutex;
KDPC KiTimerExpireDpc;
ULONG KiTimeLimitIsrMicroseconds;
ULONG KiDPCTimeout = 110;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiCheckTimerTable(IN ULARGE_INTEGER CurrentTime)
{
#if DBG
    ULONG i = 0;
    PLIST_ENTRY ListHead, NextEntry;
    KIRQL OldIrql;
    PKTIMER Timer;

    /* Raise IRQL to high and loop timers */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    do
    {
        /* Loop the current list */
        ListHead = &KiTimerTableListHead[i].Entry;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the timer and move to the next one */
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            NextEntry = NextEntry->Flink;

            /* Check if it expired */
            if (Timer->DueTime.QuadPart <= CurrentTime.QuadPart)
            {
                /* Check if the DPC was queued, but didn't run */
                if (!(KeGetCurrentPrcb()->TimerRequest) &&
                    !(*((volatile PULONG*)(&KiTimerExpireDpc.DpcData))))
                {
                    /* This is bad, breakpoint! */
                    DPRINT1("Invalid timer state!\n");
                    DbgBreakPoint();
                }
            }
        }

        /* Move to the next timer */
        i++;
    } while(i < TIMER_TABLE_SIZE);

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
#endif
}

VOID
NTAPI
KiTimerExpiration(IN PKDPC Dpc,
                  IN PVOID DeferredContext,
                  IN PVOID SystemArgument1,
                  IN PVOID SystemArgument2)
{
    ULARGE_INTEGER SystemTime, InterruptTime;
    LARGE_INTEGER Interval;
    LONG Limit, Index, i;
    ULONG Timers, ActiveTimers, DpcCalls;
    PLIST_ENTRY ListHead, NextEntry;
    PKTIMER_TABLE_ENTRY TimerEntry;
    KIRQL OldIrql;
    PKTIMER Timer;
    PKDPC TimerDpc;
    ULONG Period;
    DPC_QUEUE_ENTRY DpcEntry[MAX_TIMER_DPCS];
    PKSPIN_LOCK_QUEUE LockQueue;

    /* Disable interrupts */
    _disable();

    /* Query system and interrupt time */
    KeQuerySystemTime((PLARGE_INTEGER)&SystemTime);
    InterruptTime.QuadPart = KeQueryInterruptTime();
    Limit = KeTickCount.LowPart;

    /* Bring interrupts back */
    _enable();

    /* Get the index of the timer and normalize it */
    Index = PtrToLong(SystemArgument1);
    if ((Limit - Index) >= TIMER_TABLE_SIZE)
    {
        /* Normalize it */
        Limit = Index + TIMER_TABLE_SIZE - 1;
    }

    /* Setup index and actual limit */
    Index--;
    Limit &= (TIMER_TABLE_SIZE - 1);

    /* Setup accounting data */
    DpcCalls = 0;
    Timers = 24;
    ActiveTimers = 4;

    /* Lock the Database and Raise IRQL */
    OldIrql = KiAcquireDispatcherLock();

    /* Start expiration loop */
    do
    {
        /* Get the current index */
        Index = (Index + 1) & (TIMER_TABLE_SIZE - 1);

        /* Get list pointers and loop the list */
        ListHead = &KiTimerTableListHead[Index].Entry;
        while (ListHead != ListHead->Flink)
        {
            /* Lock the timer and go to the next entry */
            LockQueue = KiAcquireTimerLock(Index);
            NextEntry = ListHead->Flink;

            /* Get the current timer and check its due time */
            Timers--;
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            if ((NextEntry != ListHead) &&
                (Timer->DueTime.QuadPart <= InterruptTime.QuadPart))
            {
                /* It's expired, remove it */
                ActiveTimers--;
                if (RemoveEntryList(&Timer->TimerListEntry))
                {
                    /* Get the entry and check if it's empty */
                    TimerEntry = &KiTimerTableListHead[Timer->Header.Hand];
                    if (IsListEmpty(&TimerEntry->Entry))
                    {
                        /* Clear the time then */
                        TimerEntry->Time.HighPart = 0xFFFFFFFF;
                    }
                }

                /* Make it non-inserted, unlock it, and signal it */
                Timer->Header.Inserted = FALSE;
                KiReleaseTimerLock(LockQueue);
                Timer->Header.SignalState = 1;

                /* Get the DPC and period */
                TimerDpc = Timer->Dpc;
                Period = Timer->Period;

                /* Check if there's any waiters */
                if (!IsListEmpty(&Timer->Header.WaitListHead))
                {
                    /* Check the type of event */
                    if (Timer->Header.Type == TimerNotificationObject)
                    {
                        /* Unwait the thread */
                        KxUnwaitThread(&Timer->Header, IO_NO_INCREMENT);
                    }
                    else
                    {
                        /* Otherwise unwait the thread and signal the timer */
                        KxUnwaitThreadForEvent((PKEVENT)Timer, IO_NO_INCREMENT);
                    }
                }

                /* Check if we have a period */
                if (Period)
                {
                    /* Calculate the interval and insert the timer */
                    Interval.QuadPart = Int32x32To64(Period, -10000);
                    while (!KiInsertTreeTimer(Timer, Interval));
                }

                /* Check if we have a DPC */
                if (TimerDpc)
                {
                    /* Setup the DPC Entry */
                    DpcEntry[DpcCalls].Dpc = TimerDpc;
                    DpcEntry[DpcCalls].Routine = TimerDpc->DeferredRoutine;
                    DpcEntry[DpcCalls].Context = TimerDpc->DeferredContext;
                    DpcCalls++;
                }

                /* Check if we're done processing */
                if (!(ActiveTimers) || !(Timers))
                {
                    /* Release the dispatcher while doing DPCs */
                    KiReleaseDispatcherLock(DISPATCH_LEVEL);

                    /* Start looping all DPC Entries */
                    for (i = 0; DpcCalls; DpcCalls--, i++)
                    {
                        /* Call the DPC */
                        DpcEntry[i].Routine(DpcEntry[i].Dpc,
                                            DpcEntry[i].Context,
                                            UlongToPtr(SystemTime.LowPart),
                                            UlongToPtr(SystemTime.HighPart));
                    }

                    /* Reset accounting */
                    Timers = 24;
                    ActiveTimers = 4;

                    /* Lock the dispatcher database */
                    KiAcquireDispatcherLock();
                }
            }
            else
            {
                /* Check if the timer list is empty */
                if (NextEntry != ListHead)
                {
                    /* Sanity check */
                    ASSERT(KiTimerTableListHead[Index].Time.QuadPart <=
                           Timer->DueTime.QuadPart);

                    /* Update the time */
                    _disable();
                    KiTimerTableListHead[Index].Time.QuadPart =
                        Timer->DueTime.QuadPart;
                    _enable();
                }

                /* Release the lock */
                KiReleaseTimerLock(LockQueue);

                /* Check if we've scanned all the timers we could */
                if (!Timers)
                {
                    /* Release the dispatcher while doing DPCs */
                    KiReleaseDispatcherLock(DISPATCH_LEVEL);

                    /* Start looping all DPC Entries */
                    for (i = 0; DpcCalls; DpcCalls--, i++)
                    {
                        /* Call the DPC */
                        DpcEntry[i].Routine(DpcEntry[i].Dpc,
                                            DpcEntry[i].Context,
                                            UlongToPtr(SystemTime.LowPart),
                                            UlongToPtr(SystemTime.HighPart));
                    }

                    /* Reset accounting */
                    Timers = 24;
                    ActiveTimers = 4;

                    /* Lock the dispatcher database */
                    KiAcquireDispatcherLock();
                }

                /* Done looping */
                break;
            }
        }
    } while (Index != Limit);

    /* Verify the timer table, on debug builds */
    if (KeNumberProcessors == 1) KiCheckTimerTable(InterruptTime);

    /* Check if we still have DPC entries */
    if (DpcCalls)
    {
        /* Release the dispatcher while doing DPCs */
        KiReleaseDispatcherLock(DISPATCH_LEVEL);

        /* Start looping all DPC Entries */
        for (i = 0; DpcCalls; DpcCalls--, i++)
        {
            /* Call the DPC */
            DpcEntry[i].Routine(DpcEntry[i].Dpc,
                                DpcEntry[i].Context,
                                UlongToPtr(SystemTime.LowPart),
                                UlongToPtr(SystemTime.HighPart));
        }

        /* Lower IRQL if we need to */
        if (OldIrql != DISPATCH_LEVEL) KeLowerIrql(OldIrql);
    }
    else
    {
        /* Unlock the dispatcher */
        KiReleaseDispatcherLock(OldIrql);
    }
}

VOID
NTAPI
KiQuantumEnd(VOID)
{
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
        /* Check if we're real-time and with quantums disabled */
        if ((Thread->Priority >= LOW_REALTIME_PRIORITY) &&
            (Thread->ApcState.Process->DisableQuantum))
        {
            /* Otherwise, set maximum quantum */
            Thread->Quantum = MAX_QUANTUM;
        }
        else
        {
            /* Reset the new Quantum */
            Thread->Quantum = Thread->QuantumReset;

            /* Calculate new priority */
            Thread->Priority = KiComputeNewPriority(Thread, 1);

            /* Check if a new thread is scheduled */
            if (!Prcb->NextThread)
            {
                /* Get a new ready thread */
                NextThread = KiSelectReadyThread(Thread->Priority, Prcb);
                if (NextThread)
                {
                    /* Found one, set it on standby */
                    NextThread->State = Standby;
                    Prcb->NextThread = NextThread;
                }
            }
            else
            {
                /* Otherwise, make sure that this thread doesn't get preempted */
                Thread->Preempted = FALSE;
            }
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
        return;
    }

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
    PKDPC_DATA DpcData;
    PLIST_ENTRY ListHead, DpcEntry;
    PKDPC Dpc;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext, SystemArgument1, SystemArgument2;
    ULONG_PTR TimerHand;

    /* Get data and list variables before starting anything else */
    DpcData = &Prcb->DpcData[DPC_NORMAL];
    ListHead = &DpcData->DpcListHead;

    /* Main outer loop */
    do
    {
        /* Set us as active */
        Prcb->DpcRoutineActive = TRUE;

        /* Check if this is a timer expiration request */
        if (Prcb->TimerRequest)
        {
            /* It is, get the timer hand and disable timer request */
            TimerHand = Prcb->TimerHand;
            Prcb->TimerRequest = 0;

            /* Expire timers with interrups enabled */
            _enable();
            KiTimerExpiration(NULL, NULL, (PVOID)TimerHand, NULL);
            _disable();
        }

        /* Loop while we have entries in the queue */
        while (DpcData->DpcQueueDepth != 0)
        {
            /* Lock the DPC data and get the DPC entry*/
            KefAcquireSpinLockAtDpcLevel(&DpcData->DpcLock);
            DpcEntry = ListHead->Flink;

            /* Make sure we have an entry */
            if (DpcEntry != ListHead)
            {
                /* Remove the DPC from the list */
                RemoveEntryList(DpcEntry);
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
                _enable();

                /* Call the DPC */
                DeferredRoutine(Dpc,
                                DeferredContext,
                                SystemArgument1,
                                SystemArgument2);
                ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

                /* Disable interrupts and keep looping */
                _disable();
            }
            else
            {
                /* The queue should be flushed now */
                ASSERT(DpcData->DpcQueueDepth == 0);

                /* Release DPC Lock */
                KefReleaseSpinLockFromDpcLevel(&DpcData->DpcLock);
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
    } while (DpcData->DpcQueueDepth != 0);
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
    Dpc->Number = 0;
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
    PKPRCB Prcb, CurrentPrcb;
    ULONG Cpu;
    PKDPC_DATA DpcData;
    BOOLEAN DpcConfigured = FALSE, DpcInserted = FALSE;
    ASSERT_DPC(Dpc);

    /* Check IRQL and Raise it to HIGH_LEVEL */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    CurrentPrcb = KeGetCurrentPrcb();

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

    /* ROS Sanity Check */
    ASSERT(Prcb == CurrentPrcb);

    /* Check if this is a threaded DPC and threaded DPCs are enabled */
    if ((Dpc->Type == ThreadedDpcObject) && (Prcb->ThreadDpcEnable))
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
        if (&Prcb->DpcData[DPC_THREADED] == DpcData)
        {
            /* Make sure a threaded DPC isn't already active */
            if (!(Prcb->DpcThreadActive) && !(Prcb->DpcThreadRequested))
            {
                /* FIXME: Setup Threaded DPC */
                DPRINT1("Threaded DPC not supported\n");
                while (TRUE);
            }
        }
        else
        {
            /* Make sure a DPC isn't executing already */
            if (!(Prcb->DpcRoutineActive) && !(Prcb->DpcInterruptRequested))
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
    BOOLEAN Enable;
    ASSERT_DPC(Dpc);

    /* Disable interrupts */
    Enable = KeDisableInterrupts();

    /* Get DPC data */
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
    if (Enable) _enable();

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
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PAGED_CODE();

    /* Check if this is an UP machine */
    if (KeActiveProcessors == 1)
    {
        /* Check if there are DPCs on either queues */
        if ((CurrentPrcb->DpcData[DPC_NORMAL].DpcQueueDepth > 0) ||
            (CurrentPrcb->DpcData[DPC_THREADED].DpcQueueDepth > 0))
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
