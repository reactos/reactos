/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/checksum.c
 * PURPOSE:     Checksum routines
 * NOTES:       The checksum routine is from RFC 1071
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <checksum.h>


ULONG ChecksumCompute(
    PVOID Data,
    UINT Count,
    ULONG Seed)
/*
 * FUNCTION: Calculate checksum of a buffer
 * ARGUMENTS:
 *     Data  = Pointer to buffer with data
 *     Count = Number of bytes in buffer
 *     Seed  = Previously calculated checksum (if any)
 * RETURNS:
 *     Checksum of buffer
 */
{
    /* FIXME: This should be done in assembler */

    register ULONG Sum = Seed;

    while (Count > 1) {
        Sum             += *(PUSHORT)Data;
        Count           -= 2;
        (ULONG_PTR)Data += 2;
    }

    /* Add left-over byte, if any */
    if (Count > 0)
        Sum += *(PUCHAR)Data;

    /* Fold 32-bit sum to 16 bits */
    while (Sum >> 16)
        Sum = (Sum & 0xFFFF) + (Sum >> 16);

    return ~Sum;
}

/* EOF */
