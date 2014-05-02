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

#include "precomp.h"


ULONG ChecksumFold(
  ULONG Sum)
{
  /* Fold 32-bit sum to 16 bits */
  while (Sum >> 16)
    {
      Sum = (Sum & 0xFFFF) + (Sum >> 16);
    }

  return Sum;
}

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
  register ULONG Sum = Seed;

  while (Count > 1)
    {
      Sum += *(PUSHORT)Data;
      Count -= 2;
      Data = (PVOID)((ULONG_PTR) Data + 2);
    }

  /* Add left-over byte, if any */
  if (Count > 0)
    {
      Sum += *(PUCHAR)Data;
    }

  return Sum;
}

ULONG
UDPv4ChecksumCalculate(
  PIPv4_HEADER IPHeader,
  PUCHAR PacketBuffer,
  ULONG DataLength)
{
  ULONG Sum = 0;
  USHORT TmpSum;
  ULONG i;
  BOOLEAN Pad;

  /* Pad the data if needed */
  Pad = (DataLength & 1);
  if (Pad)
      DataLength++;

  /* Add from the UDP header and data */
  for (i = 0; i < DataLength; i += 2)
  {
       TmpSum = ((PacketBuffer[i] << 8) & 0xFF00) +
                ((Pad && i == DataLength - 2) ? 0 : (PacketBuffer[i+1] & 0x00FF));
       Sum += TmpSum;
  }

  /* Add the source address */
  for (i = 0; i < sizeof(IPv4_RAW_ADDRESS); i += 2)
  {
       TmpSum = ((((PUCHAR)&IPHeader->SrcAddr)[i] << 8) & 0xFF00) +
                (((PUCHAR)&IPHeader->SrcAddr)[i+1] & 0x00FF);
       Sum += TmpSum;
  }

  /* Add the destination address */
  for (i = 0; i < sizeof(IPv4_RAW_ADDRESS); i += 2)
  {
       TmpSum = ((((PUCHAR)&IPHeader->DstAddr)[i] << 8) & 0xFF00) +
                (((PUCHAR)&IPHeader->DstAddr)[i+1] & 0x00FF);
       Sum += TmpSum;
  }

  /* Add the proto number and length */
  Sum += IPPROTO_UDP + (DataLength - (Pad ? 1 : 0));

  /* Fold the checksum and return the one's complement */
  return ~ChecksumFold(Sum);
}

