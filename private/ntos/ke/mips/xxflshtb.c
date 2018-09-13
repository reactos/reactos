/*++

Copyright (c) 1992-1994 Microsoft Corporation

Module Name:

    xxflshtb.c

Abstract:

    This module implements machine dependent functions to flush the
    translation buffer.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

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
KiFlushMultipleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    );

VOID
KiFlushSingleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
    );

VOID
KiFlushMultipleTbTarget64 (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    );

VOID
KiFlushSingleTbTarget64 (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
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
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Compute the target set of processors, disable context switching,
    // and send the flush entire parameters to the target processors,
    // if any, for execution.
    //

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        TargetProcessors = KeActiveProcessors;

    } else {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiLockContextSwap(&OldIrql);
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
                        NULL,
                        NULL,
                        NULL);
    }

#endif

    //
    // Flush TB on current processor.
    //

    KeFlushCurrentTb();

    //
    // Wait until all target processors have finished.
    //

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return;
}

#if !defined(NT_UP)


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

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KeFlushCurrentTb();
    return;
}

#endif


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
    children of the current process or flushes a multiple entries from
    the translation buffer on all processors in the host configuration.

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

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    ULONG Index;
    KIRQL OldIrql;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(Number <= FLUSH_MULTIPLE_MAXIMUM);

    //
    // Compute the target set of processors.
    //

#if !defined(NT_UP)

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        TargetProcessors = KeActiveProcessors;

    } else {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiLockContextSwap(&OldIrql);
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= PCR->NotMember;

#endif

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            *PtePointer[Index] = PteValue;
        }
    }

    //
    // If any target processors are specified, then send a flush multiple
    // packet to the target set of processor.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushMultipleTbTarget,
                        (PVOID)Number,
                        (PVOID)Virtual,
                        NULL);
    }

#endif

    //
    // Flush the specified entries from the TB on the current processor.
    //

    KiFlushMultipleTb(Invalid, &Virtual[0], Number);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return;
}

#if !defined(NT_UP)


VOID
KiFlushMultipleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Pid - Supplies the PID of the TB entries to flush.

Return Value:

    None.

--*/

{

    ULONG Index;
    PVOID Array[FLUSH_MULTIPLE_MAXIMUM];

    ASSERT((ULONG)Number <= FLUSH_MULTIPLE_MAXIMUM);

    //
    // Capture the virtual addresses that are to be flushed from the TB
    // on the current processor and clear the packet address.
    //

    for (Index = 0; Index < (ULONG)Number; Index += 1) {
        Array[Index] = ((PVOID *)(Virtual))[Index];
    }

    KiIpiSignalPacketDone(SignalDone);

    //
    // Flush the specified virtual addresses from the TB on the current
    // processor.
    //

    KiFlushMultipleTb(TRUE, &Array[0], (ULONG)Number);
    return;
}

#endif


VOID
KeFlushMultipleTb64 (
    IN ULONG Number,
    IN PULONG *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a multiple entries from
    the translation buffer on all processors in the host configuration.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual page numbers that
        are flushed from the TB.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies an optional pointer to an array of pointers to
       page table entries that receive the specified page table entry
       value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    ULONG Index;
    KIRQL OldIrql;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(Number <= FLUSH_MULTIPLE_MAXIMUM);

    //
    // Compute the target set of processors.
    //

#if !defined(NT_UP)

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        TargetProcessors = KeActiveProcessors;

    } else {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiLockContextSwap(&OldIrql);
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= PCR->NotMember;

#endif

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            *PtePointer[Index] = PteValue;
        }
    }

    //
    // If any target processors are specified, then send a flush multiple
    // packet to the target set of processor.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushMultipleTbTarget64,
                        (PVOID)Number,
                        (PVOID)Virtual,
                        NULL);
    }

#endif

    //
    // Flush the specified entries from the TB on the current processor.
    //

    KiFlushMultipleTb64(Invalid, &Virtual[0], Number);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return;
}

#if !defined(NT_UP)


VOID
KiFlushMultipleTbTarget64 (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual page numbers that
        are flushed from the TB.

    Pid - Supplies the PID of the TB entries to flush.

Return Value:

    None.

--*/

{

    ULONG Index;
    PULONG Array[FLUSH_MULTIPLE_MAXIMUM];

    ASSERT((ULONG)Number <= FLUSH_MULTIPLE_MAXIMUM);

    //
    // Capture the virtual addresses that are to be flushed from the TB
    // on the current processor and clear the packet address.
    //

    for (Index = 0; Index < (ULONG)Number; Index += 1) {
        Array[Index] = ((PULONG *)(Virtual))[Index];
    }

    KiIpiSignalPacketDone(SignalDone);

    //
    // Flush the specified virtual addresses from the TB on the current
    // processor.
    //

    KiFlushMultipleTb64(TRUE, &Array[0], (ULONG)Number);
    return;
}

#endif


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

    KIRQL OldIrql;
    HARDWARE_PTE OldPte;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Compute the target set of processors.
    //

#if !defined(NT_UP)

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        TargetProcessors = KeActiveProcessors;

    } else {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiLockContextSwap(&OldIrql);
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= PCR->NotMember;

#endif

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

    //
    // If any target processors are specified, then send a flush single
    // packet to the target set of processors.

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushSingleTbTarget,
                        (PVOID)Virtual,
                        NULL,
                        NULL);
    }

#endif

    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return OldPte;
}

#if !defined(NT_UP)


VOID
KiFlushSingleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    RequestPacket - Supplies a pointer to a flush single TB packet address.

    Pid - Not used.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry form the TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KiFlushSingleTb(TRUE, Virtual);
    return;
}

#endif


HARDWARE_PTE
KeFlushSingleTb64 (
    IN ULONG Virtual,
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

Arguments:

    Virtual - Supplies a virtual page number that is flushed from the TB.

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

    KIRQL OldIrql;
    HARDWARE_PTE OldPte;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Compute the target set of processors.
    //

#if !defined(NT_UP)

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        TargetProcessors = KeActiveProcessors;

    } else {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiLockContextSwap(&OldIrql);
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= PCR->NotMember;

#endif

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

    //
    // If any target processors are specified, then send a flush single
    // packet to the target set of processors.

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushSingleTbTarget64,
                        (PVOID)Virtual,
                        NULL,
                        NULL);
    }

#endif

    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb64(Invalid, Virtual);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return OldPte;
}

#if !defined(NT_UP)


VOID
KiFlushSingleTbTarget64 (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Virtual - Supplies a virtual page number that is flushed from the TB.

    RequestPacket - Supplies a pointer to a flush single TB packet address.

    Pid - Not used.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry form the TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KiFlushSingleTb64(TRUE, (ULONG)Virtual);
    return;
}

#endif
