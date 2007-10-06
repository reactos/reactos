/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/profile.c
 * PURPOSE:         Kernel Profiling
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KIRQL KiProfileIrql = PROFILE_LEVEL;
LIST_ENTRY KiProfileListHead;
LIST_ENTRY KiProfileSourceListHead;
KSPIN_LOCK KiProfileLock;
ULONG KiProfileTimeInterval = 78125; /* Default resolution 7.8ms (sysinternals) */

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
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
STDCALL
KeStartProfile(PKPROFILE Profile,
               PVOID Buffer)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT SourceBuffer;
    PKPROFILE_SOURCE_OBJECT CurrentSource;
    BOOLEAN FreeBuffer = TRUE;
    PKPROCESS ProfileProcess;

    /* Allocate a buffer first, before we raise IRQL */
    SourceBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(KPROFILE_SOURCE_OBJECT),
                                          TAG('P', 'r', 'o', 'f'));
    RtlZeroMemory(SourceBuffer, sizeof(KPROFILE_SOURCE_OBJECT));

    /* Raise to PROFILE_LEVEL */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's not running */
    if (!Profile->Started) {

        /* Set it as Started */
        Profile->Buffer = Buffer;
        Profile->Started = TRUE;

        /* Get the process, if any */
        ProfileProcess = Profile->Process;

        /* Insert it into the Process List or Global List */
        if (ProfileProcess) {

            InsertTailList(&ProfileProcess->ProfileListHead, &Profile->ProfileListEntry);

        } else {

            InsertTailList(&KiProfileListHead, &Profile->ProfileListEntry);
        }

        /* Check if this type of profile (source) is already running */
        LIST_FOR_EACH(CurrentSource, &KiProfileSourceListHead, KPROFILE_SOURCE_OBJECT, ListEntry)
        {
            /* Check if it's the same as the one being requested now */
            if (CurrentSource->Source == Profile->Source) {
                break;
            }
        }

        /* See if the loop found something */
        if (!CurrentSource) {

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
    if (!FreeBuffer) ExFreePool(SourceBuffer);
}

BOOLEAN
STDCALL
KeStopProfile(PKPROFILE Profile)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT CurrentSource = NULL;

    /* Raise to PROFILE_LEVEL and acquire spinlock */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);

    /* Make sure it's running */
    if (Profile->Started) {

        /* Remove it from the list and disable */
        RemoveEntryList(&Profile->ProfileListEntry);
        Profile->Started = FALSE;

        /* Find the Source Object */
        LIST_FOR_EACH(CurrentSource, &KiProfileSourceListHead, KPROFILE_SOURCE_OBJECT, ListEntry) 
        {
            if (CurrentSource->Source == Profile->Source) {
                /* Remove it */
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
    if (CurrentSource) ExFreePool(CurrentSource);

    /* FIXME */
    return FALSE;
}

ULONG
STDCALL
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource)
{
    /* Check if this is the timer profile */
    if (ProfileSource == ProfileTime) {

        /* Return the good old 100ns sampling interval */
        return KiProfileTimeInterval;

    } else {

        /* Request it from HAL. FIXME: What structure is used? */
        HalQuerySystemInformation(HalProfileSourceInformation,
                                  sizeof(NULL),
                                  NULL,
                                  NULL);

        return 0;
    }
}

VOID
STDCALL
KeSetIntervalProfile(KPROFILE_SOURCE ProfileSource,
                     ULONG Interval)
{
    /* Check if this is the timer profile */
    if (ProfileSource == ProfileTime) {

        /* Set the good old 100ns sampling interval */
        KiProfileTimeInterval = Interval;

    } else {

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
STDCALL
KeProfileInterrupt(PKTRAP_FRAME TrapFrame)
{
    /* Called from HAL for Timer Profiling */
    KeProfileInterruptWithSource(TrapFrame, ProfileTime);
}

VOID
STDCALL
KiParseProfileList(IN PKTRAP_FRAME TrapFrame,
                   IN KPROFILE_SOURCE Source,
                   IN PLIST_ENTRY ListHead)
{
    PULONG BucketValue;
    PKPROFILE Profile;

    /* Loop the List */
    LIST_FOR_EACH(Profile, ListHead, KPROFILE, ProfileListEntry)
    {
        /* Check if the source is good, and if it's within the range */
#ifdef _M_IX86
        if ((Profile->Source != Source) ||
            (TrapFrame->Eip < (ULONG_PTR)Profile->RangeBase) ||
            (TrapFrame->Eip > (ULONG_PTR)Profile->RangeLimit)) {

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
STDCALL
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
STDCALL
KeSetProfileIrql(IN KIRQL ProfileIrql)
{
    /* Set the IRQL at which Profiling will run */
    KiProfileIrql = ProfileIrql;
}
