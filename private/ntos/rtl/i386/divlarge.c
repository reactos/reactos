/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    largediv.c

Abstract:

    This module implements the NT runtime library large integer divide
    routines.

    N.B. These routines use a one bit at a time algorithm and is slow.
         They should be used only when absolutely necessary.

Author:

    David N. Cutler 10-Aug-1992

Revision History:

--*/

#include "ntrtlp.h"

LARGE_INTEGER
RtlLargeIntegerDivide (
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER Divisor,
    OUT PLARGE_INTEGER Remainder OPTIONAL
    )

/*++

Routine Description:

    This routine divides an unsigned 64-bit dividend by an unsigned 64-bit
    divisor and returns a 64-bit quotient, and optionally a 64-bit remainder.

Arguments:

    Dividend - Supplies the 64-bit dividend for the divide operation.

    Divisor - Supplies the 64-bit divisor for the divide operation.

    Remainder - Supplies an optional pointer to a variable which receives
        the remainder

Return Value:

    The 64-bit quotient is returned as the function value.

--*/

{

    ULONG Index = 64;
    LARGE_INTEGER Partial = {0, 0};
    LARGE_INTEGER Quotient;

#ifndef BLDR_KERNEL_RUNTIME
    //
    // Check for divide by zero
    //

    if (!(Divisor.LowPart | Divisor.HighPart)) {
        RtlRaiseStatus (STATUS_INTEGER_DIVIDE_BY_ZERO);
    }
#endif

    //
    // Loop through the dividend bits and compute the quotient and remainder.
    //

    Quotient = Dividend;
    do {

        //
        // Shift the next dividend bit into the parital remainder and shift
        // the partial quotient (dividend) left one bit.
        //

        Partial.HighPart = (Partial.HighPart << 1) | (Partial.LowPart >> 31);
        Partial.LowPart = (Partial.LowPart << 1) | ((ULONG)Quotient.HighPart >> 31);
        Quotient.HighPart = (Quotient.HighPart << 1) | (Quotient.LowPart >> 31);
        Quotient.LowPart <<= 1;

        //
        // If the partial remainder is greater than or equal to the divisor,
        // then subtract the divisor from the partial remainder and insert a
        // one bit into the quotient.
        //

        if (((ULONG)Partial.HighPart > (ULONG)Divisor.HighPart) ||
            ((Partial.HighPart == Divisor.HighPart) &&
            (Partial.LowPart >= Divisor.LowPart))) {

            Quotient.LowPart |= 1;
            Partial.HighPart -= Divisor.HighPart;
            if (Partial.LowPart < Divisor.LowPart) {
                Partial.HighPart -= 1;
            }

            Partial.LowPart -= Divisor.LowPart;
        }

        Index -= 1;
    } while (Index > 0);

    //
    // If the remainder is requested, then return the 64-bit remainder.
    //

    if (ARGUMENT_PRESENT(Remainder)) {
        *Remainder = Partial;
    }

    //
    // Return the 64-bit quotient.
    //

    return Quotient;
}
