/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    probe.c

Abstract:

    This module implements the probe for write function.

Author:

    David N. Cutler (davec) 19-Jan-1990

Environment:

    Any mode.

Revision History:

--*/

#include "exp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, ProbeForWrite)
#pragma alloc_text(PAGE, ProbeForRead)
#endif


VOID
ProbeForWrite (
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for write accessibility and ensures
    correct alignment of the structure. If the structure is not accessible
    or has incorrect alignment, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{

    ULONG_PTR EndAddress;
    ULONG_PTR StartAddress;

    //
    // If the structure has zero length, then do not probe the structure for
    // write accessibility or alignment.
    //

    if (Length != 0) {

        //
        // If the structure is not properly aligned, then raise a data
        // misalignment exception.
        //

        ASSERT((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8) ||
               (Alignment == 16));

        StartAddress = (ULONG_PTR)Address;
        if ((StartAddress & (Alignment - 1)) == 0) {

            //
            // Compute the ending address of the structure and probe for
            // write accessibility.
            //

            EndAddress = StartAddress + Length - 1;
            if ((StartAddress <= EndAddress) &&
                (EndAddress < MM_USER_PROBE_ADDRESS)) {

                //
                // N.B. Only the contents of the buffer may be probed.
                //      Therefore the starting byte is probed for the
                //      first page, and then the first byte in the page
                //      for each succeeding page.
                //

                EndAddress = (EndAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;

#if defined(_ALPHA_)

                //
                // N.B. The address is truncated to a longword aligned
                //      address since Alpha has no load or store byte
                //      instructions.
                //

                StartAddress &= ~((ULONG_PTR)sizeof(LONG) - 1);

                do {
                    *(volatile LONG *)StartAddress = *(volatile LONG *)StartAddress;
                    StartAddress = (StartAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
                } while (StartAddress != EndAddress);

#else

                do {
                    *(volatile CHAR *)StartAddress = *(volatile CHAR *)StartAddress;

                    StartAddress = (StartAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
                } while (StartAddress != EndAddress);
#endif

                return;

            } else {
                ExRaiseAccessViolation();
            }

        } else {
            ExRaiseDatatypeMisalignment();
        }
    }

    return;
}

#undef ProbeForRead
NTKERNELAPI
VOID
NTAPI
ProbeForRead(
    IN CONST VOID *Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility and ensures
    correct alignment of the structure. If the structure is not accessible
    or has incorrect alignment, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/
{
    PAGED_CODE();

    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||
           ((Alignment) == 4) || ((Alignment) == 8) ||
           ((Alignment) == 16));

    if ((Length) != 0) {
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {
            ExRaiseDatatypeMisalignment();

        } else if ((((ULONG_PTR)(Address) + (Length)) < (ULONG_PTR)(Address)) ||
                   (((ULONG_PTR)(Address) + (Length)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) {
            ExRaiseAccessViolation();
        }
    }
}

