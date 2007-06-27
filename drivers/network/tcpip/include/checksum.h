/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/checksum.h
 * PURPOSE:     Checksum routine definitions
 */
#ifndef __CHECKSUM_H
#define __CHECKSUM_H


ULONG ChecksumFold(
  ULONG Sum);

ULONG ChecksumCompute(
    PVOID Data,
    UINT Count,
    ULONG Seed);

unsigned int
csum_partial(
  const unsigned char * buff,
  int len,
  unsigned int sum);

#define IPv4Checksum(Data, Count, Seed)(~ChecksumFold(ChecksumCompute(Data, Count, Seed)))
#define TCPv4Checksum(Data, Count, Seed)(~ChecksumFold(csum_partial(Data, Count, Seed)))
//#define TCPv4Checksum(Data, Count, Seed)(~ChecksumFold(ChecksumCompute(Data, Count, Seed)))

/*
 * Macro to check for a correct checksum
 * BOOLEAN IPv4CorrectChecksum(PVOID Data, UINT Count)
 */
#define IPv4CorrectChecksum(Data, Count) \
    (BOOLEAN)(IPv4Checksum(Data, Count, 0) == DH2N(0x0000FFFF))

/*
 * Macro to check for a correct checksum
 * BOOLEAN TCPv4CorrectChecksum(PTCPv4_PSEUDO_HEADER TcpPseudoHeader,
 *   PVOID Data, UINT Count)
 */
#define TCPv4CorrectChecksum(TcpPseudoHeader, Data, Count) \
    (BOOLEAN)(TCPv4Checksum(Data, Count, \
      TCPv4Checksum(TcpPseudoHeader, sizeof(TCPv4_PSEUDO_HEADER), \
      0)) == DH2N(0x0000FFFF))

#endif /* __CHECKSUM_H */
