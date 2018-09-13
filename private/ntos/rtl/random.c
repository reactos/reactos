/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Random.c

Abstract:

    This module implements a simple random number generator

Author:

    Gary Kimura     [GaryKi]    26-May-1989

Environment:

    Pure utility routine

Revision History:

--*/

#include <ntrtlp.h>

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlRandom)
#endif

#define Multiplier ((ULONG)(0x80000000ul - 19)) // 2**31 - 19
#define Increment  ((ULONG)(0x80000000ul - 61)) // 2**31 - 61
#define Modulus    ((ULONG)(0x80000000ul - 1))  // 2**31 - 1

#if !defined(NTOS_KERNEL_RUNTIME)
ULONG
RtlUniform (
    IN OUT PULONG Seed
    )

/*++

Routine Description:

    A simple uniform random number generator, based on D.H. Lehmer's 1948
    alrogithm.

Arguments:

    Seed - Supplies a pointer to the random number generator seed.

Return Value:

    ULONG - returns a random number uniformly distributed over [0..MAXLONG]

--*/

{
    *Seed = ((Multiplier * (*Seed)) + Increment) % Modulus;
    return *Seed;
}
#endif

#define UniformMacro(Seed) (                                 \
    *Seed = (((Multiplier * (*Seed)) + Increment) % Modulus) \
    )


extern ULONG RtlpRandomConstantVector[];

ULONG
RtlRandom (
    IN OUT PULONG Seed
    )

/*++

Routine Description:

    An every better random number generator based on MacLaren and Marsaglia.

Arguments:

    Seed - Supplies a pointer to the random number generator seed.

Return Value:

    ULONG - returns a random number uniformly distributed over [0..MAXLONG]

--*/

{
    ULONG X;
    ULONG Y;
    ULONG j;
    ULONG Result;

    RTL_PAGED_CODE();

    X = UniformMacro(Seed);
    Y = UniformMacro(Seed);

    j = Y % 128;

    Result = RtlpRandomConstantVector[j];

    RtlpRandomConstantVector[j] = X;

    return Result;

}
