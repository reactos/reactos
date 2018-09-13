/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    recip.c

Abstract:

    This module generates reciprocol fractions for implementing integer
    division by multiplication.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    User mode.

Revision History:

--*/

#include <stdio.h>

typedef struct _large_integer {
    unsigned long LowPart;
    long HighPart;
    } large_integer;

//long Divisors[] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0};
long Divisors[] = {10, 10000, 10000000, 86400000, 0};

void
main (argc, argv)
    int argc;
    char *argv[];
{

    large_integer Fraction;
    long Index;
    long NumberBits;
    long Remainder;

    long i;

    //
    // Compute first few reciprocols.
    //

    for (Index = Divisors[i = 0]; Index != 0L; Index = Divisors[++i]) {
        NumberBits = 0L;
        Remainder = 1L;
        Fraction.LowPart = 0L;
        Fraction.HighPart = 0L;
        while (Fraction.HighPart >= 0L) {
            NumberBits += 1L;
            Fraction.HighPart <<= 1L;
            if ((Fraction.LowPart & 0x80000000) != 0L) {
                Fraction.HighPart += 1L;
            }
            Fraction.LowPart <<= 1L;
            Remainder <<= 1L;
            if (Remainder >= Index) {
                Remainder -= Index;
                Fraction.LowPart |= 1L;
            }
        }
        if (Remainder) {
            if ((Fraction.LowPart == -1L) && (Fraction.HighPart == -1L)) {
                Fraction.LowPart = 0L;
                Fraction.HighPart = 0x80000000;
                NumberBits -= 1L;
            } else {
                if (Fraction.LowPart == -1L) {
                    Fraction.LowPart = 0L;
                    Fraction.HighPart += 1L;
                } else {
                    Fraction.LowPart += 1L;
                }
            }
        }

        printf("Divisor %2ld,  Fraction %8lx, %8lx Shift  %ld\n", Index,
               Fraction.HighPart, Fraction.LowPart, NumberBits - 64L);
    }

    return;
}

