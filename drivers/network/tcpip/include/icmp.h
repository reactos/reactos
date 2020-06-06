/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/icmp.h
 * PURPOSE:     Internet Control Message Protocol definitions
 */

#pragma once

#include <pshpack1.h>
typedef struct ICMP_HEADER {
    UINT8 Type;        /* ICMP message type */
    UINT8 Code;        /* ICMP message code */
    UINT16 Checksum;   /* ICMP message checksum */
    UINT16 Identifier; /* ICMP Echo message identifier */
    UINT16 Seq;        /* ICMP Echo message sequence num */
} ICMP_HEADER, *PICMP_HEADER;
#include <poppack.h>

/* ICMP message types */
#define ICMP_TYPE_ECHO_REPLY        0  /* Echo reply */
#define ICMP_TYPE_DEST_UNREACH      3  /* Destination unreachable */
#define ICMP_TYPE_SOURCE_QUENCH     4  /* Source quench */
#define ICMP_TYPE_REDIRECT          5  /* Redirect */
#define ICMP_TYPE_ECHO_REQUEST      8  /* Echo request */
#define ICMP_TYPE_TIME_EXCEEDED     11 /* Time exceeded */
#define ICMP_TYPE_PARAMETER         12 /* Parameter problem */
#define ICMP_TYPE_TIMESTAMP_REQUEST 13 /* Timestamp request */
#define ICMP_TYPE_TIMESTAMP_REPLY   14 /* Timestamp reply */
#define ICMP_TYPE_INFO_REQUEST      15 /* Information request */
#define ICMP_TYPE_INFO_REPLY        16 /* Information reply */

/* ICMP codes for ICMP_TYPE_DEST_UNREACH */
#define ICMP_CODE_DU_NET_UNREACH         0 /* Network unreachable */
#define ICMP_CODE_DU_HOST_UNREACH        1 /* Host unreachable */
#define ICMP_CODE_DU_PROTOCOL_UNREACH    2 /* Protocol unreachable */
#define ICMP_CODE_DU_PORT_UNREACH        3 /* Port unreachable */
#define ICMP_CODE_DU_FRAG_DF_SET         4 /* Fragmentation needed and DF set */
#define ICMP_CODE_DU_SOURCE_ROUTE_FAILED 5 /* Source route failed */

/* ICMP codes for ICMP_TYPE_REDIRECT */
#define ICMP_CODE_RD_NET      0 /* Redirect datagrams for the network */
#define ICMP_CODE_RD_HOST     1 /* Redirect datagrams for the host */
#define ICMP_CODE_RD_TOS_NET  2 /* Redirect datagrams for the Type of Service and network */
#define ICMP_CODE_RD_TOS_HOST 3 /* Redirect datagrams for the Type of Service and host */

/* ICMP codes for ICMP_TYPE_TIME_EXCEEDED */
#define ICMP_CODE_TE_TTL        0 /* Time to live exceeded in transit */
#define ICMP_CODE_TE_REASSEMBLY 1 /* Fragment reassembly time exceeded */

/* ICMP codes for ICMP_TYPE_PARAMETER */
#define ICMP_CODE_TP_POINTER 1 /* Pointer indicates the error */

NTSTATUS
DispEchoRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS ICMPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG DataSize,
    PULONG DataUsed );

NTSTATUS ICMPStartup(VOID);

NTSTATUS ICMPShutdown(VOID);

VOID ICMPReceive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

VOID ICMPReply(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket,
    UCHAR Type,
    UCHAR Code);

/* EOF */
