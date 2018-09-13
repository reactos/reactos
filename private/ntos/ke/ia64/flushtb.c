/*++

Module Name:

    flushtb.c

Abstract:

    This module implement machine dependent functions to flush the
    translation buffer and synchronize PIDs in an MP system.

Author:

    Koichi Yamada 2-Jan-95

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "mm.h"
#include "..\..\mm\mi.h"

extern KSPIN_LOCK KiTbBroadcastLock;
extern KSPIN_LOCK KiMasterRidLock;

#define _x256mb (1024*1024*256)

//extern VOID MiCheckPointBreak(VOID);

#define KiFlushSingleTbGlobal(Invalid, Va) __ptcga((__int64)Va, PAGE_SHIFT << 2)

#define KiFlushSingleTbLocal(Invalid, va) __ptcl((__int64)va, PAGE_SHIFT << 2)

#define KiTbSynchronizeGlobal() { __mf(); __isrlz(); }

#define KiTbSynchronizeLocal() {  __isrlz(); }

#define KiFlush2gbTbGlobal(Invalid) \
  { \
    __ptcga((__int64)0, 28 << 2); \
    __ptcg((__int64)_x256mb, 28 << 2); \
    __ptcg((__int64)_x256mb*2,28 << 2); \
    __ptcg((__int64)_x256mb*3, 28 << 2); \
    __ptcg((__int64)_x256mb*4, 28 << 2); \
    __ptcg((__int64)_x256mb*5, 28 << 2); \
    __ptcg((__int64)_x256mb*6, 28 << 2); \
    __ptcg((__int64)_x256mb*7, 28 << 2); \
  } 

#define KiFlush2gbTbLocal(Invalid) \
  { \
    __ptcl((__int64)0, 28 << 2); \
    __ptcl((__int64)_x256mb, 28 << 2); \
    __ptcl((__int64)_x256mb*2,28 << 2); \
    __ptcl((__int64)_x256mb*3, 28 << 2); \
    __ptcl((__int64)_x256mb*4, 28 << 2); \
    __ptcl((__int64)_x256mb*5, 28 << 2); \
    __ptcl((__int64)_x256mb*6, 28 << 2); \
    __ptcl((__int64)_x256mb*7, 28 << 2); \
  } 


VOID
KiSetProcessRid (
    ULONG NewProcessRid
);

VOID
KiSetRegionRegister (
    PVOID VirtualAddress,
    ULONGLONG Contents
    );


//
// Define forward referenced prototypes.
//

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiInvalidateForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    );

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    );

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    );


VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all
    processors that are currently running threads which are children
    of the current process or flushes the entire translation buffer
    on all processors in the host configuration.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.


--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;
    BOOLEAN NeedTbFlush = FALSE;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    OldIrql = KeRaiseIrqlToSynchLevel();

#if !defined(NT_UP)
    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
                        (PVOID)NeedTbFlush,
                        NULL,
                        NULL);
    }

    if (PsGetCurrentProcess()->Wow64Process != 0) {
        KiInvalidateForwardProgressTbBuffer(TargetProcessors);
    }
#endif

    KeFlushCurrentTb();

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Wait until all target processors have finished.
    //

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    KeLowerIrql(OldIrql);

#endif

    return;
}



VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);

    KeFlushCurrentTb();

#endif

    return;
}

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes multiple entries from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entries on all processors in the host
         configuration are always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies an optional pointer to an array of pointers to
       page table entries that receive the specified page table entry
       value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    None.

--*/

{

    ULONG Index;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;
    BOOLEAN Flush2gb = FALSE;

#if 0
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
#endif

    OldIrql = KeRaiseIrqlToSynchLevel();

    Wow64Process = PsGetCurrentProcess()->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G
    if ((Wow64Process != NULL) && 
        ((Wow64Process->AltFlags & MI_ALTFLG_FLUSH2G) != 0)) { 
        Flush2gb = TRUE;
    }
#endif

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        //
        // Acquire a global lock. Only one processor at a time can issue
        // a PTC.G operation.
        //

        KiAcquireSpinLock(&KiTbBroadcastLock);

        for (Index = 0; Index < Number; Index += 1) {
            if (ARGUMENT_PRESENT(PtePointer)) {
                *PtePointer[Index] = PteValue;
            }

            //
            // Flush the specified TB on each processor. Hardware automatically
            // perform broadcasts if MP.
            //

            KiFlushSingleTbGlobal(Invalid, Virtual[Index]);
        }

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBuffer(TargetProcessors);
        }

        if (Flush2gb == TRUE) {
            KiFlush2gbTbGlobal(Invalid);
        }

        //
        // Wait for the broadcast to be complete.
        //

        KiTbSynchronizeGlobal();

        KiReleaseSpinLock(&KiTbBroadcastLock);

    }
    else {

        for (Index = 0; Index < Number; Index += 1) {
            if (ARGUMENT_PRESENT(PtePointer)) {
                *PtePointer[Index] = PteValue;
            }

            //
            // Flush the specified TB on the local processor.  No broadcast is
            // performed.
            //

            KiFlushSingleTbLocal(Invalid, Virtual[Index]);
        }

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush2gb == TRUE) {
            KiFlush2gbTbLocal(Invalid);
        }

        KiTbSynchronizeLocal();
    }

#else
    for (Index = 0; Index < Number; Index += 1) {
        if (ARGUMENT_PRESENT(PtePointer)) {
           *PtePointer[Index] = PteValue;
        }

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //

        KiFlushSingleTbLocal(Invalid, Virtual[Index]);
    }

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush2gb == TRUE) {
        KiFlush2gbTbLocal(Invalid);
    }

    KiTbSynchronizeLocal();

#endif

    KeLowerIrql(OldIrql);

    return;
}


HARDWARE_PTE
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes a single entry from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a single entry from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entry on all processors in the host
         configuration is always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies a pointer to the page table entry which
        receives the specified value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    HARDWARE_PTE OldPte;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;
    BOOLEAN Flush2gb = FALSE;

#if 0
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
#endif

    //
    // Compute the target set of processors.
    //

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Check to see if it is the Wow64 process
    //

    Wow64Process = PsGetCurrentProcess()->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G
    if ((Wow64Process != NULL) && 
        ((Wow64Process->AltFlags & MI_ALTFLG_FLUSH2G) != 0)) { 
        Flush2gb = TRUE;
    }
#endif

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        //
        // Flush the specified TB on each processor. Hardware automatically
        // perform broadcasts if MP.
        //

        KiAcquireSpinLock(&KiTbBroadcastLock);

        KiFlushSingleTbGlobal(Invalid, Virtual);

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBuffer(TargetProcessors);
        }

        if (Flush2gb) {
            KiFlush2gbTbGlobal(Invalid);
        }

        KiTbSynchronizeGlobal();

        KiReleaseSpinLock(&KiTbBroadcastLock);

    }
    else {

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //
        
        KiFlushSingleTbLocal(Invalid, Virtual);

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush2gb == TRUE) { 
            KiFlush2gbTbLocal(Invalid);
        } 

        KiTbSynchronizeLocal();

    }

#else

    //
    // Flush the specified entry from the TB on the local processor.
    //

    KiFlushSingleTbLocal(Invalid, Virtual);

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush2gb == TRUE) { 
        KiFlush2gbTbLocal(Invalid);
    }

    KiTbSynchronizeLocal();

#endif

    //
    // Wait until all target processors have finished.
    //

    KeLowerIrql(OldIrql);

    //
    // Return the previous page table entry value.
    //

    return OldPte;
}

VOID
KiInvalidateForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    )
{
    PKPRCB Prcb;
    ULONG BitNumber;
    PKPROCESS CurrentProcess;
    PKPROCESS TargetProcess;
    PKPCR Pcr;
    ULONG i;

    CurrentProcess = KeGetCurrentThread()->ApcState.Process;

    //
    // Invalidate the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        PCR->ForwardProgressBuffer[(i*2)+1] = 0;

    }

    //
    // Invalidate the ForwardProgressTb buffer on all the other processors
    //

    while (TargetProcessors != 0) {

        BitNumber = KeFindFirstSetRightMember(TargetProcessors);
        ClearMember(BitNumber, TargetProcessors);
        Prcb = KiProcessorBlock[BitNumber];

        Pcr = KSEG_ADDRESS(Prcb->PcrPage);
        
        TargetProcess = Pcr->CurrentThread->ApcState.Process;

        if (TargetProcess == CurrentProcess) {

            for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
                
                Pcr->ForwardProgressBuffer[(i*2)+1] = 0;

            }
        }
    }
}

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    )
{
    PKPRCB Prcb;
    ULONG BitNumber;
    PKPROCESS CurrentProcess;
    PKPROCESS TargetProcess;
    PKPCR Pcr;
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    CurrentProcess = KeGetCurrentThread()->ApcState.Process;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        Va = (PVOID)PCR->ForwardProgressBuffer[i*2]; 
        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            KiFlushSingleTbGlobal(Invalid, Va);
        }

    }

    //
    // Flush the ForwardProgressTb buffer on all the processors
    //
        
    while (TargetProcessors != 0) {

        BitNumber = KeFindFirstSetRightMember(TargetProcessors);
        ClearMember(BitNumber, TargetProcessors);
        Prcb = KiProcessorBlock[BitNumber];

        Pcr = KSEG_ADDRESS(Prcb->PcrPage);
        
        TargetProcess = Pcr->CurrentThread->ApcState.Process;

        if (TargetProcess == CurrentProcess) {

            for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
                
                Va = (PVOID)Pcr->ForwardProgressBuffer[i*2]; 
                PointerPte = &Pcr->ForwardProgressBuffer[(i*2)+1];

                if (*PointerPte != 0) {
                    *PointerPte = 0;
                    KiFlushSingleTbGlobal(Invalid, Va);
                }
            }
        }
    }
}

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    )
{
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        Va = (PVOID)PCR->ForwardProgressBuffer[i*2]; 
        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            KiFlushSingleTbLocal(Invalid, Va);
        }

    }
}


ULONG
KiGetNewRid (
    IN PULONG ProcessNewRid,
    IN PULONGLONG ProcessNewSequence
    )

/*++

 Routine Description:

    Generate a new region id. If the region id wraps then the TBs of
    all the processors need to be flushed.

 Arguments:

    ProcessNewRid (a0) - Supplies a pointer to rid of new process (64-bit).

    ProcessNewSeqNum (a1) - Pointer to new process sequence number (64-bit).

 Return Value:

    New rid value

 Notes:

    This routine called by KiSwapProcess only.

 Environment:

    Kernel mode.
    KiLockDispaterLock or LockQueuedDispatcherLock is held

--*/

{

    ULONG NewRid;
    KAFFINITY TargetProcessors;

    KiMasterRid += 1;

    if (KiMasterRid <= MAXIMUM_RID) {

        //
        // Update Process->ProcessRid and Process->ProcessSequence.
        //

        *ProcessNewRid = KiMasterRid;
        *ProcessNewSequence = KiMasterSequence;

        return (*ProcessNewRid);
    }

    //
    // Region ID must be recycled.
    //


    KiMasterRid = START_PROCESS_RID;

    KiMasterSequence += 1;

    if (KiMasterSequence > MAXIMUM_SEQUENCE) {

        KiMasterSequence = START_SEQUENCE;

    }

    //
    // Update new process's ProcessRid and ProcessSequence.
    //

    *ProcessNewRid = KiMasterRid;
    *ProcessNewSequence = KiMasterSequence;

#if !defined(NT_UP)

    //
    // Broadcast TB flush.
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
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

    return *ProcessNewRid;
}
