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
ULONG KiProfileAlignmentFixupInterval;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeInitializeProfile(PKPROFILE Profile,
                    PKPROCESS Process,
                    PVOID ImageBase,
                    SIZE_T ImageSize,
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

BOOLEAN
NTAPI
KeStartProfile(IN PKPROFILE Profile,
               IN PVOID Buffer)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT SourceBuffer;
    PKPROFILE_SOURCE_OBJECT CurrentSource;
    BOOLEAN FreeBuffer = TRUE, SourceFound = FALSE, StartedProfile;
    PKPROCESS ProfileProcess;
    PLIST_ENTRY NextEntry;

    /* Allocate a buffer first, before we raise IRQL */
    SourceBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(KPROFILE_SOURCE_OBJECT),
                                         'forP');
    if (!SourceBuffer) return FALSE;
    RtlZeroMemory(SourceBuffer, sizeof(KPROFILE_SOURCE_OBJECT));

    /* Raise to profile IRQL and acquire the profile lock */
    KeRaiseIrql(KiProfileIrql, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's not running */
    if (!Profile->Started)
    {
        /* Set it as Started */
        Profile->Buffer = Buffer;
        Profile->Started = TRUE;
        StartedProfile = TRUE;

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
    else
    {
        /* Already running so nothing to start */
        StartedProfile = FALSE;
    }

    /* Release the profile lock */
    KeReleaseSpinLockFromDpcLevel(&KiProfileLock);

    /* Tell HAL to start the profile interrupt */
    HalStartProfileInterrupt(Profile->Source);

    /* Lower back to original IRQL */
    KeLowerIrql(OldIrql);

    /* Free the pool */
    if (FreeBuffer) ExFreePoolWithTag(SourceBuffer, 'forP');

    /* Return whether we could start the profile */
    return StartedProfile;
}

BOOLEAN
NTAPI
KeStopProfile(IN PKPROFILE Profile)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT CurrentSource = NULL;
    PLIST_ENTRY NextEntry;
    BOOLEAN SourceFound = FALSE, StoppedProfile;

    /* Raise to profile IRQL and acquire the profile lock */
    KeRaiseIrql(KiProfileIrql, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's running */
    if (Profile->Started)
    {
        /* Remove it from the list and disable */
        RemoveEntryList(&Profile->ProfileListEntry);
        Profile->Started = FALSE;
        StoppedProfile = TRUE;

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
    else
    {
        /* It wasn't! */
        StoppedProfile = FALSE;
    }

    /* Release the profile lock */
    KeReleaseSpinLockFromDpcLevel(&KiProfileLock);

    /* Stop the profile interrupt */
    HalStopProfileInterrupt(Profile->Source);

    /* Lower back to original IRQL */
    KeLowerIrql(OldIrql);

    /* Free the Source Object */
    if (SourceFound) ExFreePool(CurrentSource);

    /* Return whether we could stop the profile */
    return StoppedProfile;
}

ULONG
NTAPI
KeQueryIntervalProfile(IN KPROFILE_SOURCE ProfileSource)
{
    HAL_PROFILE_SOURCE_INFORMATION ProfileSourceInformation;
    ULONG ReturnLength, Interval;
    NTSTATUS Status;

    /* Check what profile this is */
    if (ProfileSource == ProfileTime)
    {
        /* Return the time interval */
        Interval = KiProfileTimeInterval;
    }
    else if (ProfileSource == ProfileAlignmentFixup)
    {
        /* Return the alignment interval */
        Interval = KiProfileAlignmentFixupInterval;
    }
    else
    {
        /* Request it from HAL */
        ProfileSourceInformation.Source = ProfileSource;
        Status = HalQuerySystemInformation(HalProfileSourceInformation,
                                           sizeof(HAL_PROFILE_SOURCE_INFORMATION),
                                           &ProfileSourceInformation,
                                           &ReturnLength);

        /* Check if HAL handled it and supports this profile */
        if (NT_SUCCESS(Status) && (ProfileSourceInformation.Supported))
        {
            /* Get the interval */
            Interval = ProfileSourceInformation.Interval;
        }
        else
        {
            /* Unsupported or invalid source, fail */
            Interval = 0;
        }
    }

    /* Return the interval we got */
    return Interval;
}

VOID
NTAPI
KeSetIntervalProfile(IN ULONG Interval,
                     IN KPROFILE_SOURCE ProfileSource)
{
    HAL_PROFILE_SOURCE_INTERVAL ProfileSourceInterval;

    /* Check what profile this is */
    if (ProfileSource == ProfileTime)
    {
        /* Set the interval through HAL */
        KiProfileTimeInterval = (ULONG)HalSetProfileInterval(Interval);
    }
    else if (ProfileSource == ProfileAlignmentFixup)
    {
        /* Set the alignment interval */
        KiProfileAlignmentFixupInterval = Interval;
    }
    else
    {
        /* HAL handles any other interval */
        ProfileSourceInterval.Source = ProfileSource;
        ProfileSourceInterval.Interval = Interval;
        HalSetSystemInformation(HalProfileSourceInterval,
                                sizeof(HAL_PROFILE_SOURCE_INTERVAL),
                                &ProfileSourceInterval);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeProfileInterrupt(IN PKTRAP_FRAME TrapFrame)
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
    ULONG_PTR ProgramCounter;

    /* Get the Program Counter */
    ProgramCounter = KeGetTrapFramePc(TrapFrame);

    /* Loop the List */
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        Profile = CONTAINING_RECORD(NextEntry, KPROFILE, ProfileListEntry);

        /* Check if the source is good, and if it's within the range */
        if ((Profile->Source != Source) ||
            (ProgramCounter < (ULONG_PTR)Profile->RangeBase) ||
            (ProgramCounter > (ULONG_PTR)Profile->RangeLimit))
        {
            continue;
        }

        /* Get the Pointer to the Bucket Value representing this Program Counter */
        BucketValue = (PULONG)((ULONG_PTR)Profile->Buffer +
                               (((ProgramCounter - (ULONG_PTR)Profile->RangeBase)
                                >> Profile->BucketShift) &~ 0x3));

        /* Increment the value */
        (*BucketValue)++;
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
