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

STDCALL 
VOID
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
    Profile->RegionStart = ImageBase;
    Profile->BucketShift = BucketSize - 2; /* See ntinternals.net -- Alex */
    Profile->RegionEnd = (ULONG_PTR)ImageBase + ImageSize;
    Profile->Active = FALSE;
    Profile->Source = ProfileSource;
    Profile->Affinity = Affinity;    
}

STDCALL
VOID
KeStartProfile(PKPROFILE Profile,
               PVOID Buffer)
{
    KIRQL OldIrql;
    PKPROFILE_SOURCE_OBJECT SourceBuffer;
    PKPROFILE_SOURCE_OBJECT Source = NULL;
    PKPROFILE_SOURCE_OBJECT CurrentSource;
    BOOLEAN FreeBuffer = TRUE;
    PKPROCESS ProfileProcess;
    PLIST_ENTRY ListEntry;
    
    /* Allocate a buffer first, before we raise IRQL */
    SourceBuffer = ExAllocatePoolWithTag(NonPagedPool, 
                                          sizeof(KPROFILE_SOURCE_OBJECT),
                                          TAG('P', 'r', 'o', 'f'));
    RtlZeroMemory(Source, sizeof(KPROFILE_SOURCE_OBJECT));
    
    /* Raise to PROFILE_LEVEL */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);
    
    /* Make sure it's not running */
    if (!Profile->Active) {
    
        /* Set it as active */
        Profile->Buffer = Buffer;
        Profile->Active = TRUE;
        
        /* Get the process, if any */
        ProfileProcess = Profile->Process;
        
        /* Insert it into the Process List or Global List */
        if (ProfileProcess) {
        
            InsertTailList(&ProfileProcess->ProfileListHead, &Profile->ListEntry);
            
        } else {
        
            InsertTailList(&KiProfileListHead, &Profile->ListEntry);
        }
        
        /* Check if this type of profile (source) is already running */
        for (ListEntry = KiProfileSourceListHead.Flink; 
             ListEntry != &KiProfileSourceListHead; 
             ListEntry = ListEntry->Flink) {
                 
            /* Get the Source Object */
            CurrentSource = CONTAINING_RECORD(ListEntry, 
                                              KPROFILE_SOURCE_OBJECT,
                                              ListEntry);
            
            /* Check if it's the same as the one being requested now */
            if (CurrentSource->Source == Profile->Source) {
            
                Source = CurrentSource;
                break;
            }
        }
        
        /* See if the loop found something */
        if (!Source) {
            
            /* Nothing found, use our allocated buffer */
            Source = SourceBuffer;
            
            /* Set up the Source Object */
            Source->Source = Profile->Source;
            InsertHeadList(&KiProfileSourceListHead, &Source->ListEntry);
            
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

STDCALL
VOID
KeStopProfile(PKPROFILE Profile)
{
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PKPROFILE_SOURCE_OBJECT CurrentSource = NULL;
    
    /* Raise to PROFILE_LEVEL and acquire spinlock */
    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiProfileLock);
    
    /* Make sure it's running */
    if (Profile->Active) {
    
        /* Remove it from the list and disable */
        RemoveEntryList(&Profile->ListEntry);
        Profile->Active = FALSE;
        
        /* Find the Source Object */
        for (ListEntry = KiProfileSourceListHead.Flink; 
             CurrentSource->Source != Profile->Source; 
             ListEntry = ListEntry->Flink) {
                 
            /* Get the Source Object */
            CurrentSource = CONTAINING_RECORD(ListEntry, 
                                              KPROFILE_SOURCE_OBJECT,
                                              ListEntry);
        }
        
        /* Remove it */
        RemoveEntryList(&CurrentSource->ListEntry);
    }
    
    /* Lower IRQL */
    KeReleaseSpinLockFromDpcLevel(&KiProfileLock);
    KeLowerIrql(OldIrql);
    
    /* Stop Profiling. FIXME: Implement in HAL */
    //HalStopProfileInterrupt(Profile->Source);
    
    /* Free the Source Object */
    if (CurrentSource) ExFreePool(CurrentSource);
}

STDCALL
ULONG
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

STDCALL    
VOID 
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
STDCALL
VOID
KeProfileInterrupt(PKTRAP_FRAME TrapFrame)
{
    /* Called from HAL for Timer Profiling */
    KeProfileInterruptWithSource(TrapFrame, ProfileTime);
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeProfileInterruptWithSource(IN PKTRAP_FRAME TrapFrame,
                             IN KPROFILE_SOURCE Source)
{
    /*
     * Called from HAL, this function looks up the process
     * entries, finds the proper source object, verifies the
     * ranges with the trapframe data, and inserts the information
     * from the trap frame into the buffer, while using buckets and
     * shifting like we specified.
     * Microsoft doesn't even have profile objects fully implemented yet,
     * so implementing this function is very low on the priority list.
     * -- Alex Ionescu
     */
}

/*
 * @implemented
 */
STDCALL
VOID
KeSetProfileIrql(IN KIRQL ProfileIrql)
{
    /* Set the IRQL at which Profiling will run */
    KiProfileIrql = ProfileIrql;
}
