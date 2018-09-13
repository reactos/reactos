/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivesum.c

Abstract:

    Hive header checksum module.

Author:

    Bryan M. Willman (bryanwi) 9-Apr-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpHeaderCheckSum)
#endif

ULONG
HvpHeaderCheckSum(
    PHBASE_BLOCK    BaseBlock
    )
/*++

Routine Description:

    Compute the checksum for a hive disk header.

Arguments:

    BaseBlock - supplies pointer to the header to checksum

Return Value:

    the check sum.

--*/
{
    ULONG   sum;
    ULONG   i;

    sum = 0;
    for (i = 0; i < 127; i++) {
        sum ^= ((PULONG)BaseBlock)[i];
    }
    if (sum == (ULONG)-1) {
        sum = (ULONG)-2;
    }
    if (sum == 0) {
        sum = 1;
    }
    return sum;
}
