/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/krnlinit.c
 * PURPOSE:         Portable part of kernel initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <internal/napi.h>

/* GLOBALS *******************************************************************/

/* Portable CPU Features and Flags */
USHORT KeProcessorArchitecture;
USHORT KeProcessorLevel;
USHORT KeProcessorRevision;
ULONG KeFeatureBits;

/* System call count */
ULONG KiServiceLimit = NUMBER_OF_SYSCALLS;

/* ARC Loader Block */
PLOADER_PARAMETER_BLOCK KeLoaderBlock;

/* PRCB Array */
PKPRCB KiProcessorBlock[MAXIMUM_PROCESSORS];

/* NUMA Node Support */
KNODE KiNode0;
PKNODE KeNodeBlock[1] = { &KiNode0 };
UCHAR KeNumberNodes = 1;
UCHAR KeProcessNodeSeed;

/* Initial Process and Thread */
ETHREAD KiInitialThread;
EPROCESS KiInitialProcess;

/* System-defined Spinlocks */
KSPIN_LOCK KiDispatcherLock;
KSPIN_LOCK MmPfnLock;
KSPIN_LOCK MmSystemSpaceLock;
KSPIN_LOCK CcBcbSpinLock;
KSPIN_LOCK CcMasterSpinLock;
KSPIN_LOCK CcVacbSpinLock;
KSPIN_LOCK CcWorkQueueSpinLock;
KSPIN_LOCK NonPagedPoolLock;
KSPIN_LOCK MmNonPagedPoolLock;
KSPIN_LOCK IopCancelSpinLock;
KSPIN_LOCK IopVpbSpinLock;
KSPIN_LOCK IopDatabaseLock;
KSPIN_LOCK IopCompletionLock;
KSPIN_LOCK NtfsStructLock;
KSPIN_LOCK AfdWorkQueueSpinLock;
KSPIN_LOCK KiTimerTableLock[LOCK_QUEUE_TIMER_TABLE_LOCKS];
KSPIN_LOCK KiReverseStallIpiLock;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KiInitSystem(VOID)
{
    ULONG i;

    /* Initialize Bugcheck Callback data */
    InitializeListHead(&KeBugcheckCallbackListHead);
    InitializeListHead(&KeBugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);

    /* Initialize the Timer Expiration DPC */
    KeInitializeDpc(&KiTimerExpireDpc, KiTimerExpiration, NULL);
    KeSetTargetProcessorDpc(&KiTimerExpireDpc, 0);

    /* Initialize Profiling data */
    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);

    /* Loop the timer table */
    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Initialize the list and entries */
        InitializeListHead(&KiTimerTableListHead[i].Entry);
        KiTimerTableListHead[i].Time.HighPart = 0xFFFFFFFF;
        KiTimerTableListHead[i].Time.LowPart = 0;
    }

    /* Initialize the Swap event and all swap lists */
    KeInitializeEvent(&KiSwapEvent, SynchronizationEvent, FALSE);
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);

    /* Initialize the mutex for generic DPC calls */
    ExInitializeFastMutex(&KiGenericCallDpcMutex);

    /* Initialize the syscall table */
    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;

    /* Copy the the current table into the shadow table for win32k */
    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));
}

CODE_SEG("INIT")
LARGE_INTEGER
NTAPI
KiComputeReciprocal(IN LONG Divisor,
                    OUT PUCHAR Shift)
{
    LARGE_INTEGER Reciprocal = {{0, 0}};
    LONG BitCount = 0, Remainder = 1;

    /* Start by calculating the remainder */
    while (Reciprocal.HighPart >= 0)
    {
        /* Increase the loop (bit) count */
        BitCount++;

        /* Calculate the current fraction */
        Reciprocal.HighPart = (Reciprocal.HighPart << 1) |
                              (Reciprocal.LowPart >> 31);
        Reciprocal.LowPart <<= 1;

        /* Double the remainder and see if we went past the divisor */
        Remainder <<= 1;
        if (Remainder >= Divisor)
        {
            /* Set the low-bit and calculate the new remainder */
            Remainder -= Divisor;
            Reciprocal.LowPart |= 1;
        }
    }

    /* Check if we have a remainder */
    if (Remainder)
    {
        /* Check if the current fraction value is too large */
        if ((Reciprocal.LowPart == 0xFFFFFFFF) &&
            (Reciprocal.HighPart == (LONG)0xFFFFFFFF))
        {
            /* Set the high bit and reduce the bit count */
            Reciprocal.LowPart = 0;
            Reciprocal.HighPart = 0x80000000;
            BitCount--;
        }
        else
        {
            /* Check if only the lowest bits got too large */
            if (Reciprocal.LowPart == 0xFFFFFFFF)
            {
                /* Reset them and increase the high bits instead */
                Reciprocal.LowPart = 0;
                Reciprocal.HighPart++;
            }
            else
            {
                /* All is well, increase the low bits */
                Reciprocal.LowPart++;
            }
        }
    }

    /* Now calculate the actual shift and return the reciprocal */
    *Shift = (UCHAR)BitCount - 64;
    return Reciprocal;
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitSpinLocks(IN PKPRCB Prcb,
                IN CCHAR Number)
{
    ULONG i;

    /* Initialize Dispatcher Fields */
    Prcb->QueueIndex = 1;
    Prcb->ReadySummary = 0;
    Prcb->DeferredReadyListHead.Next = NULL;
    for (i = 0; i < MAXIMUM_PRIORITY; i++)
    {
        /* Initialize the ready list */
        InitializeListHead(&Prcb->DispatcherReadyListHead[i]);
    }

    /* Initialize DPC Fields */
    InitializeListHead(&Prcb->DpcData[DPC_NORMAL].DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcData[DPC_NORMAL].DpcLock);
    Prcb->DpcData[DPC_NORMAL].DpcQueueDepth = 0;
    Prcb->DpcData[DPC_NORMAL].DpcCount = 0;
    Prcb->DpcRoutineActive = FALSE;
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    KeInitializeDpc(&Prcb->CallDpc, NULL, NULL);
    KeSetTargetProcessorDpc(&Prcb->CallDpc, Number);
    KeSetImportanceDpc(&Prcb->CallDpc, HighImportance);

    /* Initialize the Wait List Head */
    InitializeListHead(&Prcb->WaitListHead);

    /* Initialize Queued Spinlocks */
    Prcb->LockQueue[LockQueueDispatcherLock].Next = NULL;
    Prcb->LockQueue[LockQueueDispatcherLock].Lock = &KiDispatcherLock;
    Prcb->LockQueue[LockQueueExpansionLock].Next = NULL;
    Prcb->LockQueue[LockQueueExpansionLock].Lock = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Next = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Lock = &MmPfnLock;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Next = NULL;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Lock = &MmSystemSpaceLock;
    Prcb->LockQueue[LockQueueBcbLock].Next = NULL;
    Prcb->LockQueue[LockQueueBcbLock].Lock = &CcBcbSpinLock;
    Prcb->LockQueue[LockQueueMasterLock].Next = NULL;
    Prcb->LockQueue[LockQueueMasterLock].Lock = &CcMasterSpinLock;
    Prcb->LockQueue[LockQueueVacbLock].Next = NULL;
    Prcb->LockQueue[LockQueueVacbLock].Lock = &CcVacbSpinLock;
    Prcb->LockQueue[LockQueueWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueWorkQueueLock].Lock = &CcWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Lock = &NonPagedPoolLock;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Lock = &MmNonPagedPoolLock;
    Prcb->LockQueue[LockQueueIoCancelLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCancelLock].Lock = &IopCancelSpinLock;
    Prcb->LockQueue[LockQueueIoVpbLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoVpbLock].Lock = &IopVpbSpinLock;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Lock = &IopDatabaseLock;
    Prcb->LockQueue[LockQueueIoCompletionLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCompletionLock].Lock = &IopCompletionLock;
    Prcb->LockQueue[LockQueueNtfsStructLock].Next = NULL;
    Prcb->LockQueue[LockQueueNtfsStructLock].Lock = &NtfsStructLock;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Lock = &AfdWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueUnusedSpare16].Next = NULL;
    Prcb->LockQueue[LockQueueUnusedSpare16].Lock = NULL;

    /* Loop timer locks (shared amongst all CPUs) */
    for (i = 0; i < LOCK_QUEUE_TIMER_TABLE_LOCKS; i++)
    {
        /* Setup the Queued Spinlock (done only once by the boot CPU) */
        if (!Number)
            KeInitializeSpinLock(&KiTimerTableLock[i]);

        /* Initialize the lock */
        Prcb->LockQueue[LockQueueTimerTableLock + i].Next = NULL;
        Prcb->LockQueue[LockQueueTimerTableLock + i].Lock =
            &KiTimerTableLock[i];
    }

    /* Initialize the PRCB lock */
    KeInitializeSpinLock(&Prcb->PrcbLock);

    /* Check if this is the boot CPU */
    if (!Number)
    {
        /* Initialize the lock themselves */
        KeInitializeSpinLock(&KiDispatcherLock);
        KeInitializeSpinLock(&KiReverseStallIpiLock);
        KeInitializeSpinLock(&MmPfnLock);
        KeInitializeSpinLock(&MmSystemSpaceLock);
        KeInitializeSpinLock(&CcBcbSpinLock);
        KeInitializeSpinLock(&CcMasterSpinLock);
        KeInitializeSpinLock(&CcVacbSpinLock);
        KeInitializeSpinLock(&CcWorkQueueSpinLock);
        KeInitializeSpinLock(&IopCancelSpinLock);
        KeInitializeSpinLock(&IopCompletionLock);
        KeInitializeSpinLock(&IopDatabaseLock);
        KeInitializeSpinLock(&IopVpbSpinLock);
        KeInitializeSpinLock(&NonPagedPoolLock);
        KeInitializeSpinLock(&MmNonPagedPoolLock);
        KeInitializeSpinLock(&NtfsStructLock);
        KeInitializeSpinLock(&AfdWorkQueueSpinLock);
    }
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
KeInitSystem(VOID)
{
    /* Check if Threaded DPCs are enabled */
    if (KeThreadDpcEnable)
    {
        /* FIXME: TODO */
        DPRINT1("Threaded DPCs not yet supported\n");
    }

    /* Initialize non-portable parts of the kernel */
    KiInitMachineDependent();
    return TRUE;
}
