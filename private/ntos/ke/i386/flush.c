/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module implements i386 machine dependent kernel functions to flush
    the data and instruction caches and to stall processor execution.

Author:

    David N. Cutler (davec) 26-Apr-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


//
// Prototypes
//

VOID
KeInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

extern ULONG KeI386CpuType;

//  i386 and i486 have transparent caches, so these routines are nooped
//  out in macros in i386.h.

#if 0

VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    HalSweepDcache();
    return;
}

VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the instruction cache on all processors that are
    currently running threads which are children of the current process or
    flushes the instruction cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

Return Value:

    None.

--*/

{

    HalSweepIcache();

#if defined(R4000)

    HalSweepDcache();

#endif

    return;
}

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function flushes the an range of virtual addresses from the primary
    instruction cache on all processors that are currently running threads
    which are children of the current process or flushes the range of virtual
    addresses from the primary instruction cache on all processors in the host
    configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

Return Value:

    None.

--*/

{

    ULONG Offset;

    //
    // If the length of the range is greater than the size of the primary
    // instruction cache, then set the length of the flush to the size of
    // the primary instruction cache and set the ase address of zero.
    //
    // N.B. It is assumed that the size of the primary instruction and
    //      data caches are the same.
    //

    if (Length > PCR->FirstLevelIcacheSize) {
        BaseAddress = (PVOID)0;
        Length = PCR->FirstLevelIcacheSize;
    }

    //
    // Flush the specified range of virtual addresses from the primary
    // instruction cache.
    //

    Offset = (ULONG)BaseAddress & PCR->DcacheAlignment;
    Length = (Offset + Length + PCR->DcacheAlignment) & ~PCR->DcacheAlignment;
    BaseAddress = (PVOID)((ULONG)BaseAddress & ~PCR->DcacheAlignment);
    HalSweepIcacheRange(BaseAddress, Length);

#if defined(R4000)

    HalSweepDcacheRange(BaseAddress, Length);

#endif

    return;
}
#endif

BOOLEAN
KeInvalidateAllCaches (
    IN BOOLEAN AllProcessors
    )
/*++

Routine Description:

    This function writes back and invalidates the cache on all processors
    that are currently running threads which are children of the current
    process or on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    TRUE if the invalidation was done, FALSE otherwise.

--*/
{
    PKPRCB Prcb;
    PKPROCESS Process;
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    //
    // Support for wbinvd on Pentium based platforms is vendor dependent.
    // Check for family first and support on Pentium Pro based platforms
    // onward.
    //

    if (KeI386CpuType < 6 ) {
        return FALSE;
    }

#ifndef NT_UP
    //
    // Compute target set of processors.
    //

    KiLockContextSwap(&OldIrql);
    Prcb = KeGetCurrentPrcb();
    if (AllProcessors) {
        TargetProcessors = KeActiveProcessors;
    } else {
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= ~Prcb->SetMember;

    //
    // If any target processors are specified, then send writeback
    // invalidate packet to the target set of processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendSynchronousPacket(Prcb,
                                   TargetProcessors,
                                   KeInvalidateAllCachesTarget,
                                   (PVOID)&Prcb->ReverseStall,
                                   NULL,
                                   NULL);

        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // All target processors have written back and invalidated caches and
    // are waiting to proceed. Write back invalidate current cache and
    // then continue the execution of target processors.
    //
#endif
    _asm {
        ;
        ; wbinvd
        ;

        _emit 0Fh
        _emit 09h
    }

#ifndef NT_UP
    //
    // Wait until all target processors have finished and completed packet.
    //

    if (TargetProcessors != 0) {
        Prcb->ReverseStall += 1;
    }

    //
    // Release the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);

#endif

    return TRUE;
}

VOID
KeInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Proceed,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )
/*++

Routine Description:

    This is the target function for writing back and invalidating the cache.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Proceed - pointer to flag to syncronize with

  Return Value:

    None.

--*/
{
    //
    // Write back invalidate current cache
    //

    _asm {
        ;
        ; wbinvd
        ;

        _emit 0Fh
        _emit 09h

    }

    KiIpiSignalPacketDoneAndStall (SignalDone, Proceed);
}
