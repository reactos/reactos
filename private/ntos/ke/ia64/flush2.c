/*++

Module Name:

    flush2.c

Abstract:

    This module implements IA64 version of KeFlushIoBuffers.

    N.B. May be implemented as a macro.

Author:

    07-July-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )
/*++

Routine Description:

   This function flushes the I/O buffer specified by the memory descriptor
   list from the data cache on the processor which executes.

Arugements:

   Mdl - Supplies a pointer to a memory descriptor list that describes the
       I/O buffer location.

   ReadOperation - Supplies a boolean value that determines whether the I/O
       operation is a read into memory.

   DmaOperation - Supplies a boolean value that deternines whether the I/O
       operation is a DMA operation.

Return Value:

   None.

--*/
{
    KIRQL  OldIrql;
    ULONG  Length, PartialLength, Offset;
    PFN_NUMBER  PageFrameIndex;
    PPFN_NUMBER Page;
    PVOID CurrentVAddress = 0;

    ASSERT(KeGetCurrentIrql() <=  KiSynchIrql);

    //
    // If the operation is a DMA operation, then check if the flush
    // can be avoided because the host system supports the right set
    // of cache coherency attributes. Otherwise, the flush can also
    // be avoided if the operation is a programmed I/O and not a page
    // read.
    //

    if (DmaOperation != FALSE) {
        if (ReadOperation != FALSE ) {

        //
        // Yes, it is a DMA operation, and yes, it is a read. IA64
        // I-Caches DO snoop for DMA cycles.
        //
            return;
        } else {
             //
             // It is a DMA Write operation
             //
             __mf();
             return;
        }

    } else if ((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0) {
        //
        // It is a PIO operation and it is not Page in operation
        //
        return;
    } else if (ReadOperation != FALSE) {

        //
        // It is a PIO operation, it is Read operation and is Page in
        // operation.
        // We need to sweep the cache.
        // Sweeping the range covered by the mdl will be broadcast to the
        // other processors by the h/w coherency mechanism.
        //
        // Raise IRQL to synchronization level to prevent a context switch.
        //

        OldIrql = KeRaiseIrqlToSynchLevel();

        //
        // Compute the number of pages to flush and the starting MDL page
        // frame address.
        //

        Length = Mdl->ByteCount;

        if ( !Length ) {
            return;
        }
        Offset = Mdl->ByteOffset;
        PartialLength = PAGE_SIZE - Offset;
        if (PartialLength > Length) {
            PartialLength = Length;
        }

        Page = (PPFN_NUMBER)(Mdl + 1);
        PageFrameIndex = *Page;
        CurrentVAddress = ((PVOID)(KSEG3_BASE
                          | ((ULONG_PTR)(PageFrameIndex) << PAGE_SHIFT)
                          | Offset));

        //
        // Region 4 maps 1:1 Virtual address to physical address
        //

        HalSweepIcacheRange (
            CurrentVAddress,
            PartialLength
            );

        Page++;
        Length -= PartialLength;

        if (Length) {
            PartialLength = PAGE_SIZE;
            do {
                PageFrameIndex = *Page;
                CurrentVAddress = ((PVOID)(KSEG3_BASE
                    | ((ULONG_PTR)(PageFrameIndex) << PAGE_SHIFT)
                    | Offset));

                if (PartialLength > Length) {
                    PartialLength = Length;
                }

                HalSweepIcacheRange (
                    CurrentVAddress,
                    PartialLength
                    );

                Page++;

                Length -= PartialLength;
            } while (Length != 0);
        }

    //
    // Synchronize the Instruction Prefetch pipe in the local processor.
    //

    __synci();
    __isrlz();

    //
    // Lower IRQL to its previous level and return.
    //
   
    KeLowerIrql(OldIrql);
    return;
    }
}
