/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/datagram.h
 * PURPOSE:     Datagram types and constants
 */
#ifndef __DATAGRAM_H
#define __DATAGRAM_H

#include <titypes.h>

NTSTATUS DGReceiveDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR Buffer,
    ULONG ReceiveLength,
    ULONG ReceiveFlags,
    PTDI_CONNECTION_INFORMATION ReturnInfo,
    PULONG BytesReceived,
    PDATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PIRP Irp);

VOID DGRemoveIRP(
    PADDRESS_FILE AddrFile,
    PIRP Irp);

VOID DGDeliverData(
  PADDRESS_FILE AddrFile,
  PIP_ADDRESS SrcAddress,
  PIP_ADDRESS DstAddress,
  USHORT SrcPort,
  USHORT DstPort,
  PIP_PACKET IPPacket,
  UINT DataSize);

#endif /* __DATAGRAM_H */

/* EOF */
