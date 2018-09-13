/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdmove.c

Abstract:

    This module contains code to implement the portable kernel debugger
    memory mover.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "kdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpMoveMemory)
#pragma alloc_text(PAGEKD, KdpQuickMoveMemory)
#endif


ULONG
KdpMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine moves data to or from the message buffer and returns the
    actual length of the information that was moved. As data is moved, checks
    are made to ensure that the data is resident in memory and a page fault
    will not occur. If a page fault would occur, then the move is truncated.

Arguments:

    Destination  - Supplies a pointer to destination of the move operation.

    Source - Supplies a pointer to the source of the move operation.

    Length - Supplies the length of the move operation.

Return Value:

    The actual length of the move is returned as the fucntion value.

--*/

{

    PVOID Address1;
    PVOID Address2;
    ULONG ActualLength;
    HARDWARE_PTE Opaque;

    //
    // If the length is greater than the size of the message buffer, then
    // reduce the length to the size of the message buffer.
    //

    if (Length > KDP_MESSAGE_BUFFER_SIZE) {
        Length = KDP_MESSAGE_BUFFER_SIZE;
    }

    //
    // Move the source information to the destination address.
    //

    ActualLength = Length;
    Address1 = NULL;

    while (((ULONG_PTR)Source & 3) && (Length > 0)) {

    //
    // Check to determine if the move will succeed before actually performing
    // the operation.
    //

        Address1 = MmDbgWriteCheck((PVOID)Destination, &Opaque);
        Address2 = MmDbgReadCheck((PVOID)Source);
        if ((Address1 == NULL) || (Address2 == NULL)) {
            break;
        }
        *(PCHAR)Address1 = *(PCHAR)Address2;
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;

        Destination += 1;
        Source += 1;
        Length -= 1;
    }

    if (Address1 != NULL) {
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;
    }

    while (Length > 3) {

    //
    // Check to determine if the move will succeed before actually performing
    // the operation.
    //

        Address1 = MmDbgWriteCheck((PVOID)Destination, &Opaque);
        Address2 = MmDbgReadCheck((PVOID)Source);
        if ((Address1 == NULL) || (Address2 == NULL)) {
            break;
        }
        *(ULONG UNALIGNED *)Address1 = *(PULONG)Address2;
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;

        Destination += 4;
        Source += 4;
        Length -= 4;

    }

    if (Address1 != NULL) {
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;
    }

    while (Length > 0) {

    //
    // Check to determine if the move will succeed before actually performing
    // the operation.
    //

        Address1 = MmDbgWriteCheck((PVOID)Destination, &Opaque);
        Address2 = MmDbgReadCheck((PVOID)Source);
        if ((Address1 == NULL) || (Address2 == NULL)) {
            break;
        }
        *(PCHAR)Address1 = *(PCHAR)Address2;
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;

        Destination += 1;
        Source += 1;
        Length -= 1;
    }

    if (Address1 != NULL) {
        MmDbgReleaseAddress(Address1, &Opaque);
        Address1 = NULL;
    }

    //
    // Flush the instruction cache in case the write was into the instruction
    // stream.
    //

    KeSweepCurrentIcache();
    return ActualLength - Length;
}

VOID
KdpQuickMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine does the exact same thing as RtlMoveMemory, BUT it is
    private to the debugger.  This allows folks to set breakpoints and
    watch points in RtlMoveMemory without risk of recursive debugger
    entry and the accompanying hang.

    N.B.  UNLIKE KdpMoveMemory, this routine does NOT check for accessability
      and may fault!  Use it ONLY in the debugger and ONLY where you
      could use RtlMoveMemory.

Arguments:

    Destination  - Supplies a pointer to destination of the move operation.

    Source - Supplies a pointer to the source of the move operation.

    Length - Supplies the length of the move operation.

Return Value:

    None.

--*/
{
    while (Length > 0) {
        *Destination = *Source;
        Destination++;
        Source++;
        Length--;
    }
}
