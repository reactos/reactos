/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/lan.h
 * PURPOSE:     LAN adapter definitions
 */
#ifndef __LAN_H
#define __LAN_H


/* Medias we support */
#define MEDIA_ETH 0

#define MAX_MEDIA 1

#define IEEE_802_ADDR_LENGTH 6

/* Ethernet header layout */
typedef struct ETH_HEADER {
    UCHAR DstAddr[IEEE_802_ADDR_LENGTH]; /* Destination MAC address */
    UCHAR SrcAddr[IEEE_802_ADDR_LENGTH]; /* Source MAC address */
    USHORT EType;                        /* Ethernet protocol type */
} ETH_HEADER, *PETH_HEADER;

#define MAX_MEDIA_ETH sizeof(ETH_HEADER)

/* Broadcast masks */
#define BCAST_ETH_MASK 0x01

/* Broadcast values to check against */
#define BCAST_ETH_CHECK 0x01

/* Offset of broadcast address */
#define BCAST_ETH_OFFSET 0x00

/* Per adapter information */
typedef struct LAN_ADAPTER {
    LIST_ENTRY ListEntry;                   /* Entry on list */
    KSPIN_LOCK Lock;                        /* Lock for this structure */
    UCHAR State;                            /* State of the adapter */
    KEVENT Event;                           /* Opening event */
    PVOID Context;                          /* Upper layer context information */
    NDIS_HANDLE NdisHandle;                 /* NDIS binding handle */
    NDIS_STATUS NdisStatus;                 /* NDIS status of last request */
    NDIS_MEDIUM Media;                      /* Media type */
    UCHAR HWAddress[IEEE_802_ADDR_LENGTH];  /* Local HW address */
    UINT HWAddressLength;                   /* Length of HW address */
    UCHAR BCastMask;                        /* Mask for checking broadcast */
    UCHAR BCastCheck;                       /* Value to check against */
    UCHAR BCastOffset;                      /* Offset in frame to check against */
    UCHAR HeaderSize;                       /* Size of link-level header */
    USHORT MTU;                             /* Maximum Transfer Unit */
    UINT MinFrameSize;                      /* Minimum frame size in bytes */
    UINT MaxPacketSize;                     /* Maximum packet size when sending */
    UINT MaxSendPackets;                    /* Maximum number of packets per send */
    UINT MacOptions;                        /* MAC options for NIC driver/adapter */
    UINT Speed;                             /* Link speed */
    UINT PacketFilter;                      /* Packet filter for this adapter */
    PNDIS_PACKET TDPackets;                 /* Transfer Data packets */
} LAN_ADAPTER, *PLAN_ADAPTER;

/* LAN adapter state constants */
#define LAN_STATE_OPENING   0
#define LAN_STATE_RESETTING 1
#define LAN_STATE_STARTED   2
#define LAN_STATE_STOPPED   3

/* Size of out lookahead buffer */
#define LOOKAHEAD_SIZE  128

/* Ethernet types. We swap constants so we can compare values at runtime
   without swapping them there */
#define ETYPE_IPv4 WH2N(0x0800)
#define ETYPE_IPv6 WH2N(0x0000) /* FIXME */
#define ETYPE_ARP  WH2N(0x0806)

/* Protocols */
#define LAN_PROTO_IPv4 0x0000 /* Internet Protocol version 4 */
#define LAN_PROTO_IPv6 0x0001 /* Internet Protocol version 6 */
#define LAN_PROTO_ARP  0x0002 /* Address Resolution Protocol */


NDIS_STATUS LANRegisterAdapter(
    PNDIS_STRING AdapterName,
    PLAN_ADAPTER *Adapter);

NDIS_STATUS LANUnregisterAdapter(
    PLAN_ADAPTER Adapter);

NTSTATUS LANRegisterProtocol(
    STRING *Name);

VOID LANUnregisterProtocol(
    VOID);

#endif /* __LAN_H */

/* EOF */
