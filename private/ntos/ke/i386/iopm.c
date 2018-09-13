/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    iopm.c

Abstract:

    This module implements interfaces that support manipulation of i386
    i/o access maps (IOPMs).

    These entry points only exist on i386 machines.

Author:

    Bryan M. Willman (bryanwi) 18-Sep-91

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Our notion of alignment is different, so force use of ours
//

#undef  ALIGN_UP
#undef  ALIGN_DOWN
#define ALIGN_DOWN(address,amt) ((ULONG)(address) & ~(( amt ) - 1))
#define ALIGN_UP(address,amt) (ALIGN_DOWN( (address + (amt) - 1), (amt) ))

//
// Note on synchronization:
//
//  IOPM edits are always done by code running at synchronization level on
//  the processor whose TSS (map) is being edited.
//
//  IOPM only affects user mode code.  User mode code can never interrupt
//  synchronization level code, therefore, edits and user code never race.
//
//  Likewise, switching from one map to another occurs on the processor
//  for which the switch is being done by IPI_LEVEL code.  The active
//  map could be switched in the middle of an edit of some map, but
//  the edit will always complete before any user code gets run on that
//  processor, therefore, there is no race.
//
//  Multiple simultaneous calls to Ke386SetIoAccessMap *could* produce
//  weird mixes.  Therefore, KiIopmLock must be acquired to
//  globally serialize edits.
//

//
// Define forward referenced function prototypes.
//

VOID
KiSetIoMap(
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapSource,
    IN PVOID MapNumber,
    IN PVOID Parameter3
    );

VOID
KiLoadIopmOffset(
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

BOOLEAN
Ke386SetIoAccessMap (
    ULONG MapNumber,
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    The specified i/o access map will be set to match the
    definition specified by IoAccessMap (i.e. enable/disable
    those ports) before the call returns.  The change will take
    effect on all processors.

    Ke386SetIoAccessMap does not give any process enhanced I/O
    access, it merely defines a particular access map.

Arguments:

    MapNumber - Number of access map to set.  Map 0 is fixed.

    IoAccessMap - Pointer to bitvector (64K bits, 8K bytes) which
           defines the specified access map.  Must be in
           non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to set a map
    which does not exist, attempt to set map 0)

--*/

{

    PKPROCESS CurrentProcess;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PVOID pt;
    KAFFINITY TargetProcessors;

    //
    // Reject illegal requests
    //

    if ((MapNumber > IOPM_COUNT) || (MapNumber == IO_ACCESS_MAP_NONE)) {
        return FALSE;
    }

    //
    // Acquire the context swap lock so a context switch will not occur.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Compute set of active processors other than this one, if non-empty
    // IPI them to set their maps.
    //

    Prcb = KeGetCurrentPrcb();

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSetIoMap,
                        IoAccessMap,
                        (PVOID)MapNumber,
                        NULL);
    }

#endif

    //
    // Copy the IOPM map and load the map for the current process.
    //

    pt = &(KiPcr()->TSS->IoMaps[MapNumber-1].IoMap);
    RtlMoveMemory(pt, (PVOID)IoAccessMap, IOPM_SIZE);
    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;

    //
    // Wait until all of the target processors have finished copying the
    // new map.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Restore IRQL and unlock the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    return TRUE;
}

#if !defined(NT_UP)


VOID
KiSetIoMap(
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapSource,
    IN PVOID MapNumber,
    IN PVOID Parameter3
    )
/*++

Routine Description:

    copy the specified map into this processor's TSS.
    This procedure runs at IPI level.

Arguments:

    Argument - actually a pointer to a KIPI_SET_IOPM structure
    ReadyFlag - pointer to flag to set once setiopm has completed

Return Value:

    none

--*/

{

    PKPROCESS CurrentProcess;
    PKPRCB Prcb;
    PVOID pt;

    //
    // Copy the IOPM map and load the map for the current process.
    //

    Prcb = KeGetCurrentPrcb();
    pt = &(KiPcr()->TSS->IoMaps[((ULONG) MapNumber)-1].IoMap);
    RtlMoveMemory(pt, MapSource, IOPM_SIZE);
    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;
    KiIpiSignalPacketDone(SignalDone);
    return;
}

#endif


BOOLEAN
Ke386QueryIoAccessMap (
    ULONG MapNumber,
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    The specified i/o access map will be dumped into the buffer.
    map 0 is a constant, but will be dumped anyway.

Arguments:

    MapNumber - Number of access map to set.  map 0 is fixed.

    IoAccessMap - Pointer to buffer (64K bits, 8K bytes) which
           is to receive the definition of the access map.
           Must be in non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to query a map
    which does not exist)

--*/

{

    ULONG i;
    PVOID Map;
    KIRQL OldIrql;
    PUCHAR p;

    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
        return FALSE;
    }

    //
    // Acquire the context swap lock so a context switch will not occur.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Copy out the map
    //

    if (MapNumber == IO_ACCESS_MAP_NONE) {

        //
        // no access case, simply return a map of all 1s
        //

        p = (PUCHAR)IoAccessMap;
        for (i = 0; i < IOPM_SIZE; i++) {
            p[i] = (UCHAR)-1;
        }

    } else {

        //
        // normal case, just copy the bits
        //

        Map = (PVOID)&(KiPcr()->TSS->IoMaps[MapNumber-1].IoMap);
        RtlMoveMemory((PVOID)IoAccessMap, Map, IOPM_SIZE);
    }

    //
    // Restore IRQL and unlock the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    return TRUE;
}


BOOLEAN
Ke386IoSetAccessProcess (
    PKPROCESS Process,
    ULONG MapNumber
    )
/*++

Routine Description:

    Set the i/o access map which controls user mode i/o access
    for a particular process.

Arguments:

    Process - Pointer to kernel process object describing the
    process which for which a map is to be set.

    MapNumber - Number of the map to set.  Value of map is
    defined by Ke386IoSetAccessProcess.  Setting MapNumber
    to IO_ACCESS_MAP_NONE will disallow any user mode i/o
    access from the process.

Return Value:

    TRUE if success, FALSE if failure (illegal MapNumber)

--*/

{

    USHORT MapOffset;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
        return FALSE;
    }

    MapOffset = KiComputeIopmOffset(MapNumber);

    //
    // Acquire the context swap lock so a context switch will not occur.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Store new offset in process object,  compute current set of
    // active processors for process, if this cpu is one, set IOPM.
    //

    Process->IopmOffset = MapOffset;

    TargetProcessors = Process->ActiveProcessors;
    Prcb = KeGetCurrentPrcb();
    if (TargetProcessors & Prcb->SetMember) {
        KiPcr()->TSS->IoMapBase = MapOffset;
    }

    //
    // Compute set of active processors other than this one, if non-empty
    // IPI them to load their IOPMs, wait for them.
    //

#if !defined(NT_UP)

    TargetProcessors = TargetProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiLoadIopmOffset,
                        NULL,
                        NULL,
                        NULL);

        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Restore IRQL and unlock the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    return TRUE;
}

#if !defined(NT_UP)


VOID
KiLoadIopmOffset(
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    Edit IopmBase of Tss to match that of currently running process.

Arguments:

    Argument - actually a pointer to a KIPI_LOAD_IOPM_OFFSET structure
    ReadyFlag - Pointer to flag to be set once we are done

Return Value:

    none

--*/

{

    PKPROCESS CurrentProcess;
    PKPRCB Prcb;

    //
    // Update IOPM field in TSS from current process
    //

    Prcb = KeGetCurrentPrcb();
    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;
    KiIpiSignalPacketDone(SignalDone);
    return;
}

#endif


VOID
Ke386SetIOPL(
    IN PKPROCESS Process
    )

/*++

Routine Description:

    Gives IOPL to the specified process.

    All threads created from this point on will get IOPL.  The current
    process will get IOPL.  Must be called from context of thread and
    process that are to have IOPL.

    Iopl (to be made a boolean) in KPROCESS says all
    new threads to get IOPL.

    Iopl (to be made a boolean) in KTHREAD says given
    thread to get IOPL.

    N.B.    If a kernel mode only thread calls this procedure, the
            result is (a) poinless and (b) will break the system.

Arguments:

    Process - Pointer to the process == IGNORED!!!

Return Value:

    none

--*/

{

    PKTHREAD    Thread;
    PKPROCESS   Process2;
    PKTRAP_FRAME    TrapFrame;
    CONTEXT     Context;

    //
    // get current thread and Process2, set flag for IOPL in both of them
    //

    Thread = KeGetCurrentThread();
    Process2 = Thread->ApcState.Process;

    Process2->Iopl = 1;
    Thread->Iopl = 1;

    //
    // Force IOPL to be on for current thread
    //

    TrapFrame = (PKTRAP_FRAME)((PUCHAR)Thread->InitialStack -
                ALIGN_UP(sizeof(KTRAP_FRAME),KTRAP_FRAME_ALIGN) -
                sizeof(FX_SAVE_AREA));

    Context.ContextFlags = CONTEXT_CONTROL;
    KeContextFromKframes(TrapFrame,
                         NULL,
                         &Context);

    Context.EFlags |= (EFLAGS_IOPL_MASK & -1);  // IOPL == 3

    KeContextToKframes(TrapFrame,
                       NULL,
                       &Context,
                       CONTEXT_CONTROL,
                       UserMode);

    return;
}
