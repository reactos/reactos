/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/checksum.h
 * PURPOSE:     Checksum routine definitions
 */
#ifndef __CHECKSUM_H
#define __CHECKSUM_H


ULONG ChecksumCompute(
    PVOID Data,
    UINT Count,
    ULONG Seed);

#define IPv4Checksum(Data, Count, Seed)(ChecksumCompute(Data, Count, Seed))

/*
 * Macro to check for a correct checksum
 * BOOLEAN CorrectChecksum(PVOID Data, UINT Count)
 */
#define CorrectChecksum(Data, Count) \
    (BOOLEAN)(IPv4Checksum(Data, Count, 0) == DH2N(0x0000FFFF))

#endif /* __CHECKSUM_H */

/* EOF */
