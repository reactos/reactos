/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Drivers
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

static ULONG KsecRandomSeed = 0x62b409a1;


/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length)
{
    ULONG i, RandomValue;
    PULONG P;

    /* Try to generate a more random seed */
    KsecRandomSeed ^= _rotl(KeTickCount.LowPart, (KsecRandomSeed % 23));

    P = Buffer;
    for (i = 0; i < Length / sizeof(ULONG); i++)
    {
        P[i] = RtlRandomEx(&KsecRandomSeed);
    }

    Length &= (sizeof(ULONG) - 1);
    if (Length > 0)
    {
        RandomValue = RtlRandomEx(&KsecRandomSeed);
        RtlCopyMemory(&P[i], &RandomValue, Length);
    }

    return STATUS_SUCCESS;
}
