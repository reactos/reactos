/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ldtsup.c

Abstract:

    This module implements interfaces that support manipulation of i386 Ldts.
    These entry points only exist on i386 machines.

Author:

    Bryan M. Willman (bryanwi) 14-May-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Low level assembler support procedures
//

VOID
KiLoadLdtr(
    VOID
    );

VOID
KiFlushDescriptors(
    VOID
    );

//
// Local service procedures
//

VOID
Ki386LoadTargetLdtr (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
Ki386FlushTargetDescriptors (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
Ke386SetLdtProcess (
    IN PKPROCESS Process,
    IN PLDT_ENTRY Ldt,
    IN ULONG Limit
    )
/*++

Routine Description:

    The specified LDT (which may be null) will be made the active Ldt of
    the specified process, for all threads thereof, on whichever
    processors they are running.  The change will take effect before the
    call returns.

    An Ldt address of NULL or a Limit of 0 will cause the process to
    receive the NULL Ldt.

    This function only exists on i386 and i386 compatible processors.

    No checking is done on the validity of Ldt entries.


    N.B.

    While a single Ldt structure can be shared amoung processes, any
    edits to the Ldt of one of those processes will only be synchronized
    for that process.  Thus, processes other than the one the change is
    applied to may not see the change correctly.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the Ldt is to be set.

    Ldt - Pointer to an array of LDT_ENTRYs (that is, a pointer to an
        Ldt.)

    Limit - Ldt limit (must be 0 mod 8)

Return Value:

    None.

--*/

{

    KGDTENTRY LdtDescriptor;
    BOOLEAN LocalProcessor;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Compute the contents of the Ldt descriptor
    //

    if ((Ldt == NULL) || (Limit == 0)) {

        //
        //  Set up an empty descriptor
        //

        LdtDescriptor.LimitLow = 0;
        LdtDescriptor.BaseLow = 0;
        LdtDescriptor.HighWord.Bytes.BaseMid = 0;
        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;
        LdtDescriptor.HighWord.Bytes.BaseHi = 0;

    } else {

        //
        // Insure that the unfilled fields of the selector are zero
        // N.B.  If this is not done, random values appear in the high
        //       portion of the Ldt limit.
        //

        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;

        //
        //  Set the limit and base
        //

        LdtDescriptor.LimitLow = (USHORT) ((ULONG) Limit - 1);
        LdtDescriptor.BaseLow = (USHORT)  ((ULONG) Ldt & 0xffff);
        LdtDescriptor.HighWord.Bytes.BaseMid = (UCHAR) (((ULONG)Ldt & 0xff0000) >> 16);
        LdtDescriptor.HighWord.Bytes.BaseHi =  (UCHAR) (((ULONG)Ldt & 0xff000000) >> 24);

        //
        //  Type is LDT, DPL = 0
        //

        LdtDescriptor.HighWord.Bits.Type = TYPE_LDT;
        LdtDescriptor.HighWord.Bits.Dpl = DPL_SYSTEM;

        //
        // Make it present
        //

        LdtDescriptor.HighWord.Bits.Pres = 1;

    }

    //
    // Acquire the context swap lock so a context switch cannot occur.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Set the Ldt fields in the process object.
    //

    Process->LdtDescriptor = LdtDescriptor;

    //
    // Tell all processors active for this process to reload their LDTs
    //

#ifdef NT_UP

    KiLoadLdtr();

#else

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = Process->ActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        Ki386LoadTargetLdtr,
                        NULL,
                        NULL,
                        NULL);
    }

    KiLoadLdtr();
    if (TargetProcessors != 0) {

        //
        //  Stall until target processor(s) release us
        //

        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Restore IRQL and release the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    return;
}

VOID
Ki386LoadTargetLdtr (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )
/*++

Routine Description:

    Reload local Ldt register and clear signal bit in TargetProcessor mask

Arguments:

    Argument - pointer to a ipi packet structure.
    ReadyFlag - Pointer to flag to be set once LDTR has been reloaded

Return Value:

    none.

--*/
{

    //
    // Reload the LDTR register from currently active process object
    //

    KiLoadLdtr();
    KiIpiSignalPacketDone(SignalDone);
    return;
}

VOID
Ke386SetDescriptorProcess (
    IN PKPROCESS Process,
    IN ULONG Offset,
    IN LDT_ENTRY LdtEntry
    )
/*++

Routine Description:

    The specified LdtEntry (which could be 0, not present, etc) will be
    edited into the specified Offset in the Ldt of the specified Process.
    This will be synchronzied accross all the processors executing the
    process.  The edit will take affect on all processors before the call
    returns.

    N.B.

    Editing an Ldt descriptor requires stalling all processors active
    for the process, to prevent accidental loading of descriptors in
    an inconsistent state.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the descriptor edit is to be performed.

    Offset - Byte offset into the Ldt of the descriptor to edit.
        Must be 0 mod 8.

    LdtEntry - Value to edit into the descriptor in hardware format.
        No checking is done on the validity of this item.

Return Value:

    none.

--*/

{

    PLDT_ENTRY Ldt;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Compute address of descriptor to edit.
    //

    Ldt =
        (PLDT_ENTRY)
         ((Process->LdtDescriptor.HighWord.Bytes.BaseHi << 24) |
         ((Process->LdtDescriptor.HighWord.Bytes.BaseMid << 16) & 0xff0000) |
         (Process->LdtDescriptor.BaseLow & 0xffff));
    Offset = Offset / 8;
    MmLockPagedPool(&Ldt[Offset], sizeof(LDT_ENTRY));
    KiLockContextSwap(&OldIrql);

#ifdef NT_UP

    //
    // Edit the Ldt.
    //

    Ldt[Offset] = LdtEntry;

#else

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = Process->ActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendSynchronousPacket(
            Prcb,
            TargetProcessors,
            Ki386FlushTargetDescriptors,
            (PVOID)&Prcb->ReverseStall,
            NULL,
            NULL);

        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // All target processors have flushed the segment descriptors and
    // are waiting to proceed. Edit the ldt on the current processor,
    // then continue the execution of target processors.
    //

    Ldt[Offset] = LdtEntry;
    if (TargetProcessors != 0) {
        Prcb->ReverseStall += 1;
    }

#endif

    //
    // Restore IRQL and release the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    MmUnlockPagedPool(&Ldt[Offset], sizeof(LDT_ENTRY));
    return;
}

VOID
Ki386FlushTargetDescriptors (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Proceed,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This function flushes the segment descriptors on the current processor.

Arguments:

    Argument - pointer to a _KIPI_FLUSH_DESCRIPTOR structure.

    ReadyFlag - pointer to flag to syncroize with

Return Value:

    none.

--*/

{
    //
    // Flush the segment descriptors on the current processor and signal that
    // the descriptors have been flushed.
    //

    KiFlushDescriptors();
    KiIpiSignalPacketDoneAndStall (SignalDone, Proceed);
    return;
}
