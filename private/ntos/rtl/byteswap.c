/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    byteswap.c

Abstract:

    This module defines functions for performing endian conversions.

Author:

    Forrest Foltz (forrestf) 10-Dec-1997

Revision History:


--*/

#include "nt.h"
#include "ntrtlp.h"


USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source
    )

/*++

Routine Description:

    The RtlUshortByteSwap function exchanges bytes 0 and 1 of Source
    and returns the resulting USHORT.

Arguments:

    Source - 16-bit value to byteswap.

Return Value:

    Swapped 16-bit value.

--*/
{
    USHORT swapped;

    swapped = ((Source) << (8 * 1)) |
              ((Source) >> (8 * 1));

    return swapped;
}


ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source
    )

/*++

Routine Description:

    The RtlUlongByteSwap function exchanges byte pairs 0:3 and 1:2 of
    Source and returns the resulting ULONG.

Arguments:

    Source - 32-bit value to byteswap.

Return Value:

    Swapped 32-bit value.

--*/
{
    ULONG swapped;

    swapped = ((Source)              << (8 * 3)) |
              ((Source & 0x0000FF00) << (8 * 1)) |
              ((Source & 0x00FF0000) >> (8 * 1)) |
              ((Source)              >> (8 * 3));

    return swapped;
}


ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source
    )

/*++

Routine Description:

    The RtlUlongByteSwap function exchanges byte pairs 0:7, 1:6, 2:5, and
    3:4 of Source and returns the resulting ULONGLONG.

Arguments:

    Source - 64-bit value to byteswap.

Return Value:

    Swapped 64-bit value.

--*/
{
    ULONGLONG swapped;

    swapped = ((Source)                      << (8 * 7)) |
              ((Source & 0x000000000000FF00) << (8 * 5)) |
              ((Source & 0x0000000000FF0000) << (8 * 3)) |
              ((Source & 0x00000000FF000000) << (8 * 1)) |
              ((Source & 0x000000FF00000000) >> (8 * 1)) |
              ((Source & 0x0000FF0000000000) >> (8 * 3)) |
              ((Source & 0x00FF000000000000) >> (8 * 5)) |
              ((Source)                      >> (8 * 7));

    return swapped;
}


