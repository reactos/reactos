/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/clock.c
 * PURPOSE:         System Clock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LARGE_INTEGER KeBootTime;
ULONGLONG KeBootTimeBias;
volatile KSYSTEM_TIME KeTickCount = {0};
volatile ULONG KiRawTicks = 0;
ULONG KeMaximumIncrement;
ULONG KeMinimumIncrement;
ULONG KeTimeAdjustment;
ULONG KeTimeIncrement;
LONG KiTickOffset = 0;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KeSetSystemTime(IN PLARGE_INTEGER NewTime,
                OUT PLARGE_INTEGER OldTime,
                IN BOOLEAN FixInterruptTime,
                IN PLARGE_INTEGER HalTime OPTIONAL)
{
    TIME_FIELDS TimeFields;
    KIRQL OldIrql, OldIrql2;
    LARGE_INTEGER DeltaTime;
    PLIST_ENTRY ListHead, NextEntry;
    PKTIMER Timer;
    PKSPIN_LOCK_QUEUE LockQueue;
    LIST_ENTRY TempList, TempList2;
    ULONG Hand, i;
    PKTIMER_TABLE_ENTRY TimerEntry;

    /* Sanity checks */
    ASSERT((NewTime->HighPart & 0xF0000000) == 0);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /* Check if this is for the HAL */
    if (HalTime) RtlTimeToTimeFields(HalTime, &TimeFields);

    /* Set affinity to this CPU, lock the dispatcher, and raise IRQL */
    KeSetSystemAffinityThread(1);
    OldIrql = KiAcquireDispatcherLock();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql2);

    /* Query the system time now */
    KeQuerySystemTime(OldTime);

    /* Set the new system time */
    SharedUserData->SystemTime.LowPart = NewTime->LowPart;
    SharedUserData->SystemTime.High1Time = NewTime->HighPart;
    SharedUserData->SystemTime.High2Time = NewTime->HighPart;

    /* Check if this was for the HAL and set the RTC time */
    if (HalTime) ExCmosClockIsSane = HalSetRealTimeClock(&TimeFields);

    /* Calculate the difference between the new and the old time */
    DeltaTime.QuadPart = NewTime->QuadPart - OldTime->QuadPart;

    /* Update system boot time */
    KeBootTime.QuadPart += DeltaTime.QuadPart;
    KeBootTimeBias = KeBootTimeBias + DeltaTime.QuadPart;

    /* Lower IRQL back */
    KeLowerIrql(OldIrql2);

    /* Check if we need to adjust interrupt time */
    if (FixInterruptTime) KEBUGCHECK(0);

    /* Setup a temporary list of absolute timers */
    InitializeListHead(&TempList);

    /* Loop current timers */
    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Loop the entries in this table and lock the timers */
        ListHead = &KiTimerTableListHead[i].Entry;
        LockQueue = KiAcquireTimerLock(i);
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the timer */
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            NextEntry = NextEntry->Flink;

            /* Is is absolute? */
            if (Timer->Header.Absolute)
            {
                /* Remove it from the timer list */
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

                /* Insert it into our temporary list */
                InsertTailList(&TempList, &Timer->TimerListEntry);
            }
        }

        /* Release the lock */
        KiReleaseTimerLock(LockQueue);
    }

    /* Setup a temporary list of expired timers */
    InitializeListHead(&TempList2);

    /* Loop absolute timers */
    while (TempList.Flink != &TempList)
    {
        /* Get the timer */
        Timer = CONTAINING_RECORD(TempList.Flink, KTIMER, TimerListEntry);
        RemoveEntryList(&Timer->TimerListEntry);

        /* Update the due time and handle */
        Timer->DueTime.QuadPart -= DeltaTime.QuadPart;
        Hand = KiComputeTimerTableIndex(Timer->DueTime.QuadPart);
        Timer->Header.Hand = (UCHAR)Hand;

        /* Lock the timer and re-insert it */
        LockQueue = KiAcquireTimerLock(Hand);
        if (KiInsertTimerTable(Timer, Hand))
        {
            /* Remove it from the timer list */
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

            /* Insert it into our temporary list */
            InsertTailList(&TempList2, &Timer->TimerListEntry);
        }

        /* Release the lock */
        KiReleaseTimerLock(LockQueue);
    }

    /* FIXME: Process expired timers! */
    KiReleaseDispatcherLock(OldIrql);

    /* Revert affinity */
    KeRevertToUserAffinityThread();
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
ULONG
NTAPI
KeQueryTimeIncrement(VOID)
{
    /* Return the increment */
    return KeMaximumIncrement;
}

/*
 * @implemented
 */
#undef KeQueryTickCount
VOID
NTAPI
KeQueryTickCount(IN PLARGE_INTEGER TickCount)
{
    /* Loop until we get a perfect match */
    for (;;)
    {
        /* Read the tick count value */
        TickCount->HighPart = KeTickCount.High1Time;
        TickCount->LowPart = KeTickCount.LowPart;
        if (TickCount->HighPart == KeTickCount.High2Time) break;
        YieldProcessor();
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeQuerySystemTime(OUT PLARGE_INTEGER CurrentTime)
{
    /* Loop until we get a perfect match */
    for (;;)
    {
        /* Read the time value */
        CurrentTime->HighPart = SharedUserData->SystemTime.High1Time;
        CurrentTime->LowPart = SharedUserData->SystemTime.LowPart;
        if (CurrentTime->HighPart ==
            SharedUserData->SystemTime.High2Time) break;
        YieldProcessor();
    }
}

/*
 * @implemented
 */
ULONGLONG
NTAPI
KeQueryInterruptTime(VOID)
{
    LARGE_INTEGER CurrentTime;

    /* Loop until we get a perfect match */
    for (;;)
    {
        /* Read the time value */
        CurrentTime.HighPart = SharedUserData->InterruptTime.High1Time;
        CurrentTime.LowPart = SharedUserData->InterruptTime.LowPart;
        if (CurrentTime.HighPart ==
            SharedUserData->InterruptTime.High2Time) break;
        YieldProcessor();
    }

    /* Return the time value */
    return CurrentTime.QuadPart;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetTimeIncrement(IN ULONG MaxIncrement,
                   IN ULONG MinIncrement)
{
    /* Set some Internal Variables */
    KeMaximumIncrement = MaxIncrement;
    KeMinimumIncrement = max(MinIncrement, 10000);
    KeTimeAdjustment = MaxIncrement;
    KeTimeIncrement = MaxIncrement;
    KiTickOffset = MaxIncrement;
}

NTSTATUS
NTAPI
NtQueryTimerResolution(OUT PULONG MinimumResolution,
                       OUT PULONG MaximumResolution,
                       OUT PULONG ActualResolution)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetTimerResolution(IN ULONG DesiredResolution,
                     IN BOOLEAN SetResolution,
                     OUT PULONG CurrentResolution)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
