/*++

Module Name:

    session.c

Abstract:

    This module implements the region space management code.

Author:

    18-Feb-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "mm.h"
#include "..\..\mm\mi.h"

VOID
KiSetRegionRegister (
    PVOID VirtualAddress,
    ULONGLONG Contents
    );


#define KiMakeValidRegionRegister(Rid, Ps) \
   (((ULONGLONG)Rid << RR_RID) | (Ps << RR_PS) | (1 << RR_VE))

ULONG KiMaximumRid = MAXIMUM_RID;


VOID
KiSyncNewRegionIdTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for synchronizing the region Ids

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{
#if !defined(NT_UP)

    PKTHREAD Thread;
    PKPROCESS Process;
    PREGION_MAP_INFO ProcessRegion;
    PREGION_MAP_INFO MappedSession;
    ULONG NewRid;
 
    //
    // Flush the entire TB on the current processor.
    //

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;
    ProcessRegion = &Process->ProcessRegion;
    MappedSession = Process->SessionMapInfo;

    KiAcquireSpinLock(&KiMasterRidLock);

    if (ProcessRegion->SequenceNumber != KiMasterSequence) {
        
        KiMasterRid += 1;

        ProcessRegion->RegionId = KiMasterRid;
        ProcessRegion->SequenceNumber = KiMasterSequence;

        KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
            KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    }

    if (MappedSession->SequenceNumber != KiMasterSequence) {

        KiMasterRid += 1;
        
        MappedSession->RegionId = KiMasterRid;
        MappedSession->SequenceNumber = KiMasterSequence;

        KiSetRegionRegister((PVOID)MM_SESSION_SPACE_DEFAULT,
            KiMakeValidRegionRegister(MappedSession->RegionId, PAGE_SHIFT));
    }


    KiReleaseSpinLock(&KiMasterRidLock);

    KiIpiSignalPacketDone(SignalDone);

    KeFlushCurrentTb();

#endif
    return;
}

BOOLEAN
KiSyncNewRegionId(
    IN PREGION_MAP_INFO ProcessRegion,
    IN PREGION_MAP_INFO SessionRegion
    )
/*++

 Routine Description:

    Generate a new region id and synchronze the region Ids on all the processors
    if necessary. If the region ids wrap then flush all processor TLB's.

 Arguments:

    ProcessRegion - Supplies a pointer to REGION_MAP_INFO for the user space.
    SessionRegion - Supplies a pointer to REGION_MAP_INFO for the session space.

 Return Value:

    FALSE -- when region id has not been recycled.

    TRUE -- when region id has been recycled.

 Notes:

    This routine called by KiSwapProcess, KeAttachSessionSpace and 
    KeCreateSessionSpace.

 Environment:

    Kernel mode.
    KiLockDispaterLock or LockQueuedDispatcherLock is held

--*/

{
    BOOLEAN RidRecycled = FALSE;
    KAFFINITY TargetProcessors;
    ULONG i;

    //
    // Invalidate the ForwardProgressTb buffer
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        PCR->ForwardProgressBuffer[(i*2)+1] = 0;

    }
    
    KiAcquireSpinLock(&KiMasterRidLock);

    if ((ProcessRegion->SequenceNumber == KiMasterSequence) && 
        (SessionRegion->SequenceNumber == KiMasterSequence)) {
        
        goto not_recycled;

    }

    if (ProcessRegion->SequenceNumber != KiMasterSequence) {

        if (KiMasterRid + 1 > KiMaximumRid) {

            RidRecycled = TRUE;

        } else {

            KiMasterRid += 1;
            ProcessRegion->RegionId = KiMasterRid;
            ProcessRegion->SequenceNumber = KiMasterSequence;
        }
                
    }

    if ((RidRecycled == FALSE) && 
        (SessionRegion->SequenceNumber != KiMasterSequence)) {
        
        if (KiMasterRid + 1 > KiMaximumRid) {

            RidRecycled = TRUE;

        } else {

            KiMasterRid += 1;
            SessionRegion->RegionId = KiMasterRid;
            SessionRegion->SequenceNumber = KiMasterSequence;
        }
    }

    if (RidRecycled == FALSE) {
    
        goto not_recycled;

    }

    //
    //  Region Id must be recycled
    //

    KiMasterRid = START_PROCESS_RID;

    //
    // Since KiMasterSequence is 64-bit wide, it will not be recycled in your life time.
    //

    if (KiMasterSequence + 1 > MAXIMUM_SEQUENCE) {

        KiMasterSequence = START_SEQUENCE;

    } else {

        KiMasterSequence += 1;
    }
        
    //
    // update new process's ProcessRid and ProcessSequence
    //

    ProcessRegion->RegionId = KiMasterRid;
    ProcessRegion->SequenceNumber = KiMasterSequence;

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    KiMasterRid += 1;

    SessionRegion->RegionId = KiMasterRid;
    SessionRegion->SequenceNumber = KiMasterSequence;

    KiSetRegionRegister((PVOID)MM_SESSION_SPACE_DEFAULT,
                        KiMakeValidRegionRegister(SessionRegion->RegionId, PAGE_SHIFT));

    //
    // release mutex for master region id lock
    //

    KiReleaseSpinLock(&KiMasterRidLock);

#if !defined(NT_UP)

    //
    // broadcast Region Id sync
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSyncNewRegionIdTarget,
                        (PVOID)TRUE,
                        NULL,
                        NULL);
    }

#endif

    KeFlushCurrentTb();


#if !defined(NT_UP)

    //
    // Wait until all target processors have finished.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    return TRUE;


not_recycled:

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    KiSetRegionRegister((PVOID)MM_SESSION_SPACE_DEFAULT,
                            KiMakeValidRegionRegister(SessionRegion->RegionId, PAGE_SHIFT));

    //
    // release mutex for master region id lock
    //

    KiReleaseSpinLock(&KiMasterRidLock);
        
    return FALSE;

}

VOID
KeEnableSessionSharing(
    PREGION_MAP_INFO SessionMapInfo
    )
/*++

 Routine Description:

    This routine enables session sharing by the other processes.

 Arguments: 

    SessionMapInfo - Supplies a session map info to be shared.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
    PKPROCESS Process;
    PKTHREAD Thread;
    KIRQL OldIrql;

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

    //
    // Raise IRQL to dispatcher level and lock the dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    SessionMapInfo->RegionId = Process->SessionRegion.RegionId;
    SessionMapInfo->SequenceNumber = Process->SessionRegion.SequenceNumber;

    Process->SessionMapInfo = SessionMapInfo;

    //
    // unlock the dispatcher database
    //

    KiUnlockDispatcherDatabase(OldIrql);
}

VOID
KeDisableSessionSharing(
    PREGION_MAP_INFO SessionMapInfo
    )
/*++

 Routine Description:

    This routine disables session sharing by the other process.

 Arguments: 

    SessionMapInfo - Supplies a session map info to be disabled 
           for sharing.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
    PKPROCESS Process;
    PKTHREAD Thread;
    KIRQL OldIrql;

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

    //
    // Raise IRQL to dispatcher level and lock the dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    Process->SessionRegion.RegionId = SessionMapInfo->RegionId;
    Process->SessionRegion.SequenceNumber = SessionMapInfo->SequenceNumber;
    Process->SessionMapInfo = &Process->SessionRegion;

    //
    // unlock the dispatcher database
    //

    KiUnlockDispatcherDatabase(OldIrql);
}

VOID
KeAttachSessionSpace(
    PREGION_MAP_INFO SessionMapInfo
    )
/*++

 Routine Description:

    This routine attaches a session map info to the current process.

 Arguments: 

    SessionMapInfo - Supplies a session map info to be attached.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    PKPROCESS Process;

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

    //
    // Raise IRQL to dispatcher level and lock the dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    ASSERT(SessionMapInfo != NULL);

    //
    // Attach the given session map
    //

    Process->SessionMapInfo = SessionMapInfo;

    KiSyncNewRegionId(&Process->ProcessRegion, SessionMapInfo);

    //
    // unlock the dispatcher database
    //

    KiUnlockDispatcherDatabase(OldIrql);
    
}

VOID
KiSyncSessionTarget(
    IN PULONG SignalDone,
    IN PKPROCESS Process,
    IN PVOID Parameter1,
    IN PVOID Parameter2
    )
/*++

 Routine Description:

    This is the target function for synchronizing the new session 
    region id.  This routine is called when the session space is removed 
    and all the processors need to be notified.

 Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Process - Supplies a KPROCESS pointer which needs to be sync'ed.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
#if !defined(NT_UP)

    PKTHREAD Thread;
    ULONG NewRid;
 
    //
    // Flush the entire TB on the current processor.
    //

    Thread = KeGetCurrentThread();

    //
    // check to see if the current process is the process that needs to be 
    // sync'ed
    //

    if (Process == Thread->ApcState.Process) {
        
        KiAcquireSpinLock(&KiMasterRidLock);

        //
        // disable the session region.
        //

        KiSetRegionRegister((PVOID)MM_SESSION_SPACE_DEFAULT, 
                            KiMakeValidRegionRegister(Process->SessionMapInfo->RegionId, PAGE_SHIFT));

        KiReleaseSpinLock(&KiMasterRidLock);

        //
        // flush the entire tb.
        //

        KeFlushCurrentTb();
    }

    KiIpiSignalPacketDone(SignalDone);

#endif
    return;
}


VOID 
KeDetachSessionSpace(
    VOID
    )
/*++

 Routine Description:
    
    This routine removes the session space and synchronize the all threads on 
    the other processors.

 Arguments:
 
    DeleteSessionMapInfo - if TRUE, the session map info will be deleted.
        if FALSE, the session map info will not be deleted.

 Return Value:
  
    None.

 Environment:
 
    Kernel mode.

--*/
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    PKPROCESS Process;
#if !defined(NT_UP)
    KAFFINITY TargetProcessors;
#endif

    //
    // Raise IRQL to dispatcher level and lock the dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

    //
    // Lock the region Id resource.
    //
    
    KiAcquireSpinLock(&KiMasterRidLock);

    Process->SessionMapInfo = &Process->SessionRegion;
    
    KiSetRegionRegister((PVOID)MM_SESSION_SPACE_DEFAULT, 
                        KiMakeValidRegionRegister(Process->SessionMapInfo->RegionId, PAGE_SHIFT));

    //
    // Unlock the region Id resource.
    //

    KiReleaseSpinLock(&KiMasterRidLock);

#if !defined(NT_UP)

    //
    // broadcast Region Id sync
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSyncSessionTarget,
                        Process,
                        NULL,
                        NULL);
    }

#endif

    //
    // Unlock the dispatcher database
    //

    KiUnlockDispatcherDatabase(OldIrql);
}    

VOID
KeAddSessionSpace(
    PKPROCESS Process,
    PREGION_MAP_INFO SessionMapInfo
    )
/*++

Routine Description:
    
    Add the session map info to the KPROCESS of the new process.

Arguments:

    Process - Supplies a pointer to the process being created.

    SessionMapInfo - Supplies a pointer to the SessionMapInfo.

Return Value: 

    None.

Environment:

    Kernel mode, APCs disabled.  

Remarks:
    
    KiLockDispaterLock or LockQueuedDispatcherLock is not necessary 
    since the process has not run yet.

--*/
{
    KIRQL OldIrql;

    ASSERT (Process->SessionMapInfo == NULL);

//    KiLockDispatcherDatabase(&OldIrql);

    Process->SessionMapInfo = SessionMapInfo;

//    KiUnLockDispatherDatabase(OldIrql);

}

