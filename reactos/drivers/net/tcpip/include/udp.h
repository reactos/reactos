/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/udp.h
 * PURPOSE:     User Datagram Protocol definitions
 */
#ifndef __UDP_H
#define __UDP_H


/* UDPv4 header structure */
typedef struct UDP_HEADER {
  USHORT SourcePort; /* Source port */
  USHORT DestPort;   /* Destination port */
  USHORT Length;     /* Size of header and data */
  USHORT Checksum;   /* Checksum of datagram */
} __attribute__((packed)) UDP_HEADER, *PUDP_HEADER;

/* UDPv4 pseudo header */
typedef struct UDP_PSEUDO_HEADER {
  ULONG SourceAddress; /* Source address */
  ULONG DestAddress;   /* Destination address */
  UCHAR Zero;          /* Reserved */
  UCHAR Protocol;      /* Protocol */
  USHORT UDPLength;    /* Size of UDP datagram */
} __attribute__((packed)) UDP_PSEUDO_HEADER, *PUDP_PSEUDO_HEADER;


typedef struct UDP_STATISTICS {
  ULONG InputDatagrams;
  ULONG NumPorts;
  ULONG InputErrors;
  ULONG OutputDatagrams;
  ULONG NumAddresses;
} UDP_STATISTICS, *PUDP_STATISTICS;

VOID UDPSend(
  PVOID Context,
  PDATAGRAM_SEND_REQUEST SendRequest);

NTSTATUS UDPSendDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG DataSize);

NTSTATUS UDPReceiveDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  ULONG ReceiveFlags,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PULONG BytesReceived);

VOID UDPReceive(
  PNET_TABLE_ENTRY NTE,
  PIP_PACKET IPPacket);

NTSTATUS UDPStartup(
  VOID);

NTSTATUS UDPShutdown(
  VOID);

#endif /* __UDP_H */

/* EOF */
