/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/lan.h
 * PURPOSE:     LAN adapter definitions
 */
#ifndef __LAN_H
#define __LAN_H

/* NDIS version this driver supports */
#define NDIS_VERSION_MAJOR 4
#define NDIS_VERSION_MINOR 0

/* Macros */

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#define NDIS_BUFFER_TAG FOURCC('n','b','u','f')
#define NDIS_PACKET_TAG FOURCC('n','p','k','t')

/* Media we support */
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

typedef struct _LAN_ADDRESS_C {
    LIST_ENTRY  ListEntry;
    LAN_ADDRESS ClientPart;
} LAN_ADDRESS_C, *PLAN_ADDRESS_C;

/* Per adapter information */
typedef struct LAN_ADAPTER {
    LIST_ENTRY ListEntry;                   /* Entry on list */
    LIST_ENTRY AddressList;                 /* Addresses associated */
    LIST_ENTRY ForeignList;                 /* List of known addresses */
    KSPIN_LOCK Lock;                        /* Lock for this structure */
    UINT Index;                             /* Adapter Index */
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
    UINT Lookahead;                         /* Lookahead for adapter */
    UNICODE_STRING RegistryPath;            /* Registry path for later query */
} LAN_ADAPTER, *PLAN_ADAPTER;

typedef struct _LAN_PACKET {
    PNDIS_PACKET NdisPacket;
    PETH_HEADER  EthHeader;
    UINT         TotalSize;
} LAN_PACKET, *PLAN_PACKET;

typedef struct _LAN_PROTOCOL {
    LIST_ENTRY   ListEntry;
    LIST_ENTRY   ReadIrpListHead;
    UINT         Id;
    UINT         LastServicePass;
    UINT         Buffered;
    UINT         NumEtherTypes;
    USHORT       EtherType[1];
} LAN_PROTOCOL, *PLAN_PROTOCOL;

typedef struct _LAN_DEVICE_EXT {
    NDIS_HANDLE NdisProtocolHandle;
    KSPIN_LOCK  Lock;
    LIST_ENTRY  AdapterListHead;
    LIST_ENTRY  ProtocolListHead;
    UINT        AdapterId;
    UINT        ProtoId;
} LAN_DEVICE_EXT, *PLAN_DEVICE_EXT;

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
#define ETYPE_IPv6 WH2N(0x86DD)
#define ETYPE_ARP  WH2N(0x0806)

/* Protocols */
#define LAN_PROTO_IPv4 0x0000 /* Internet Protocol version 4 */
#define LAN_PROTO_IPv6 0x0001 /* Internet Protocol version 6 */
#define LAN_PROTO_ARP  0x0002 /* Address Resolution Protocol */


NDIS_STATUS LANRegisterAdapter(
    PNDIS_STRING AdapterName,
		PNDIS_STRING RegistryPath);

NDIS_STATUS LANUnregisterAdapter(PLAN_ADAPTER Adapter);

NTSTATUS LANRegisterProtocol(PNDIS_STRING Name);

VOID LANUnregisterProtocol(VOID);

NDIS_STATUS NDISCall(
    PLAN_ADAPTER Adapter,
    NDIS_REQUEST_TYPE Type,
    NDIS_OID OID,
    PVOID Buffer,
    UINT Length);

void GetDataPtr( PNDIS_PACKET Packet,
		 UINT Offset,
		 PCHAR *DataOut,
		 PUINT Size );

NDIS_STATUS AllocatePacketWithBufferX( PNDIS_PACKET *NdisPacket,
				       PCHAR Data, UINT Len,
				       PCHAR File, UINT Line );

VOID FreeNdisPacketX( PNDIS_PACKET Packet, PCHAR File, UINT Line );

NDIS_STATUS InitNdisPools();
VOID CloseNdisPools();

PLAN_ADAPTER FindAdapterByIndex( PLAN_DEVICE_EXT DeviceExt, UINT Index );

#endif /* __LAN_H */

/* EOF */
