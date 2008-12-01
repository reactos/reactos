/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/profobj.c
 * PURPOSE:         Kernel Profiling
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KIRQL KiProfileIrql = PROFILE_LEVEL;
LIST_ENTRY KiProfileListHead;
LIST_ENTRY KiProfileSourceListHead;
KSPIN_LOCK KiProfileLock;
ULONG KiProfileTimeInterval = 78125; /* Default resolution 7.8ms (sysinternals) */

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeInitializeProfile(PKPROFILE Profile,
                    PKPROCESS Process,
                    PVOID ImageBase,
                    ULONG ImageSize,
                    ULONG BucketSize,
                    KPROFILE_SOURCE ProfileSource,
                    KAFFINITY Affinity)
{
    /* Initialize the Header */
    Profile->Type = ProfileObject;
    Profile->Size = sizeof(KPROFILE);

    /* Copy all the settings we were given */
    Profile->Process = Process;
    Profile->RangeBase = ImageBase;
    Profile->BucketShift = BucketSize - 2; /* See ntinternals.net -- Alex */
    Profile->RangeLimit = (PVOID)((ULONG_PTR)ImageBase + ImageSize);
    Profile->Started = FALSE;
    Profile->Source = ProfileSource;
    Profile->Affinity = Affinity;
}

VOID
NTAPI
KeStartProfile(PKPROFILE Profile,
               PVOID Buffer)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT SourceBuffer;
    PKPROFILE_SOURCE_OBJECT CurrentSource;
    BOOLEAN FreeBuffer = TRUE, SourceFound = FALSE;;
    PKPROCESS ProfileProcess;
    PLIST_ENTRY NextEntry;

    /* Allocate a buffer first, before we raise IRQL */
    SourceBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(KPROFILE_SOURCE_OBJECT),
                                          TAG('P', 'r', 'o', 'f'));
    RtlZeroMemory(SourceBuffer, sizeof(KPROFILE_SOURCE_OBJECT));

    /* Raise to PROFILE_LEVEL */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's not running */
    if (!Profile->Started)
    {
        /* Set it as Started */
        Profile->Buffer = Buffer;
        Profile->Started = TRUE;

        /* Get the process, if any */
        ProfileProcess = Profile->Process;

        /* Check where we should insert it */
        if (ProfileProcess)
        {
            /* Insert it into the Process List */
            InsertTailList(&ProfileProcess->ProfileListHead, &Profile->ProfileListEntry);
        }
        else
        {
            /* Insert it into the Global List */
            InsertTailList(&KiProfileListHead, &Profile->ProfileListEntry);
        }

        /* Start looping */
        for (NextEntry = KiProfileSourceListHead.Flink;
             NextEntry != &KiProfileSourceListHead;
             NextEntry = NextEntry->Flink)
        {
            /* Get the entry */
            CurrentSource = CONTAINING_RECORD(NextEntry,
                                              KPROFILE_SOURCE_OBJECT,
                                              ListEntry);

            /* Check if it's the same as the one being requested now */
            if (CurrentSource->Source == Profile->Source)
            {
                /* It is, break out */
                SourceFound = TRUE;
                break;
            }
        }

        /* See if the loop found something */
        if (!SourceFound)
        {
            /* Nothing found, use our allocated buffer */
            CurrentSource = SourceBuffer;

            /* Set up the Source Object */
            CurrentSource->Source = Profile->Source;
            InsertHeadList(&KiProfileSourceListHead, &CurrentSource->ListEntry);

            /* Don't free the pool later on */
            FreeBuffer = FALSE;
        }
    }

    /* Lower the IRQL */
    KeReleaseSpinLockFromDpcLevel(&KiProfileLock);
    KeLowerIrql(OldIrql);

    /* FIXME: Tell HAL to Start the Profile Interrupt */
    //HalStartProfileInterrupt(Profile->Source);

    /* Free the pool */
    if (FreeBuffer) ExFreePool(SourceBuffer);
}

BOOLEAN
NTAPI
KeStopProfile(PKPROFILE Profile)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT CurrentSource = NULL;
    PLIST_ENTRY NextEntry;
    BOOLEAN SourceFound = FALSE;

    /* Raise to PROFILE_LEVEL and acquire spinlock */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's running */
    if (Profile->Started)
    {
        /* Remove it from the list and disable */
        RemoveEntryList(&Profile->ProfileListEntry);
        Profile->Started = FALSE;

        /* Start looping */
        for (NextEntry = KiProfileSourceListHead.Flink;
             NextEntry != &KiProfileSourceListHead;
             NextEntry = NextEntry->Flink)
        {
            /* Get the entry */
            CurrentSource = CONTAINING_RECORD(NextEntry,
                                              KPROFILE_SOURCE_OBJECT,
                                              ListEntry);

            /* Check if this is the Source Object */
            if (CurrentSource->Source == Profile->Source)
            {
                /* Remember we found one */
                SourceFound = TRUE;

                /* Remove it and break out */
                RemoveEntryList(&CurrentSource->ListEntry);
                break;
            }
        }

    }

    /* Lower IRQL */
    KeReleaseSpinLockFromDpcLevel(&KiProfileLock);
    KeLowerIrql(OldIrql);

    /* Stop Profiling. FIXME: Implement in HAL */
    //HalStopProfileInterrupt(Profile->Source);

    /* Free the Source Object */
    if (SourceFound) ExFreePool(CurrentSource);

    /* FIXME */
    return FALSE;
}

ULONG
NTAPI
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource)
{
    /* Check if this is the timer profile */
    if (ProfileSource == ProfileTime)
    {
        /* Return the good old 100ns sampling interval */
        return KiProfileTimeInterval;
    }
    else
    {
        /* Request it from HAL. FIXME: What structure is used? */
        HalQuerySystemInformation(HalProfileSourceInformation,
                                  sizeof(NULL),
                                  NULL,
                                  NULL);

        return 0;
    }
}

VOID
NTAPI
KeSetIntervalProfile(KPROFILE_SOURCE ProfileSource,
                     ULONG Interval)
{
    /* Check if this is the timer profile */
    if (ProfileSource == ProfileTime)
    {
        /* Set the good old 100ns sampling interval */
        KiProfileTimeInterval = Interval;
    }
    else
    {
        /* Set it with HAL. FIXME: What structure is used? */
        HalSetSystemInformation(HalProfileSourceInformation,
                                  sizeof(NULL),
                                  NULL);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeProfileInterrupt(PKTRAP_FRAME TrapFrame)
{
    /* Called from HAL for Timer Profiling */
    KeProfileInterruptWithSource(TrapFrame, ProfileTime);
}

VOID
NTAPI
KiParseProfileList(IN PKTRAP_FRAME TrapFrame,
                   IN KPROFILE_SOURCE Source,
                   IN PLIST_ENTRY ListHead)
{
    PULONG BucketValue;
    PKPROFILE Profile;
    PLIST_ENTRY NextEntry;

    /* Loop the List */
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        Profile = CONTAINING_RECORD(NextEntry, KPROFILE, ProfileListEntry);

        /* Check if the source is good, and if it's within the range */
#ifdef _M_IX86
        if ((Profile->Source != Source) ||
            (TrapFrame->Eip < (ULONG_PTR)Profile->RangeBase) ||
            (TrapFrame->Eip > (ULONG_PTR)Profile->RangeLimit))
        {
            continue;
        }

        /* Get the Pointer to the Bucket Value representing this EIP */
        BucketValue = (PULONG)((((ULONG_PTR)Profile->Buffer +
                               (TrapFrame->Eip - (ULONG_PTR)Profile->RangeBase))
                                >> Profile->BucketShift) &~ 0x3);
#elif defined(_M_PPC)
    // XXX arty
#endif

        /* Increment the value */
        ++BucketValue;
    }
}

/*
 * @implemented
 *
 * Remarks:
 *         Called from HAL, this function looks up the process
 *         entries, finds the proper source object, verifies the
 *         ranges with the trapframe data, and inserts the information
 *         from the trap frame into the buffer, while using buckets and
 *         shifting like we specified. -- Alex
 */
VOID
NTAPI
KeProfileInterruptWithSource(IN PKTRAP_FRAME TrapFrame,
                             IN KPROFILE_SOURCE Source)
{
    PKPROCESS Process = KeGetCurrentThread()->ApcState.Process;

    /* We have to parse 2 lists. Per-Process and System-Wide */
    KiParseProfileList(TrapFrame, Source, &Process->ProfileListHead);
    KiParseProfileList(TrapFrame, Source, &KiProfileListHead);
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetProfileIrql(IN KIRQL ProfileIrql)
{
    /* Set the IRQL at which Profiling will run */
    KiProfileIrql = ProfileIrql;
}
