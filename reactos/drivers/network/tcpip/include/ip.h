/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/ip.h
 * PURPOSE:     Internet Protocol related definitions
 */
#ifndef __IP_H
#define __IP_H

typedef VOID (*OBJECT_FREE_ROUTINE)(PVOID Object);

#define FOURCC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

/* Raw IPv4 style address */
typedef ULONG IPv4_RAW_ADDRESS;
typedef IPv4_RAW_ADDRESS *PIPv4_RAW_ADDRESS;

/* Raw IPv6 style address */
typedef USHORT IPv6_RAW_ADDRESS[8];
typedef IPv6_RAW_ADDRESS *PIPv6_RAW_ADDRESS;

/* IP style address */
typedef struct IP_ADDRESS {
    DEFINE_TAG
    UCHAR Type;                      /* Type of IP address */
    union {
        IPv4_RAW_ADDRESS IPv4Address;/* IPv4 address (in network byte order) */
        IPv6_RAW_ADDRESS IPv6Address;/* IPv6 address (in network byte order) */
    } Address;
} IP_ADDRESS, *PIP_ADDRESS;

/* IP type constants */
#define IP_ADDRESS_V4   0x04 /* IPv4 style address */
#define IP_ADDRESS_V6   0x06 /* IPv6 style address */


/* IPv4 header format */
typedef struct IPv4_HEADER {
    UCHAR VerIHL;                /* 4-bit version, 4-bit Internet Header Length */
    UCHAR Tos;                   /* Type of Service */
    USHORT TotalLength;          /* Total Length */
    USHORT Id;                   /* Identification */
    USHORT FlagsFragOfs;         /* 3-bit Flags, 13-bit Fragment Offset */
    UCHAR Ttl;                   /* Time to Live */
    UCHAR Protocol;              /* Protocol */
    USHORT Checksum;             /* Header Checksum */
    IPv4_RAW_ADDRESS SrcAddr;    /* Source Address */
    IPv4_RAW_ADDRESS DstAddr;    /* Destination Address */
} IPv4_HEADER, *PIPv4_HEADER;

/* IPv6 header format */
typedef struct IPv6_HEADER {
    ULONG VTF;                   /* Version, Traffic Class, Flow Label */
    USHORT PayloadLength;
    UCHAR NextHeader;            /* Same as Protocol in IPv4 */
    UCHAR HopLimit;              /* Same as Ttl in IPv4 */
    IPv6_RAW_ADDRESS SrcAddr;
    IPv6_RAW_ADDRESS DstAddr;
} IPv6_HEADER, *PIPv6_HEADER;

typedef union _IP_HEADER {
    IPv4_HEADER v4;
    IPv6_HEADER v6;
} IP_HEADER, *PIP_HEADER;

#define IPv4_FRAGOFS_MASK       0x1FFF /* Fragment offset mask (host byte order) */
#define IPv4_MF_MASK            0x2000 /* More fragments (host byte order) */
#define IPv4_DF_MASK            0x4000 /* Don't fragment (host byte order) */
#define IPv4_MAX_HEADER_SIZE    60

/* Packet completion handler prototype */
typedef VOID (*PACKET_COMPLETION_ROUTINE)(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    NDIS_STATUS NdisStatus);

/* Structure for an IP packet */
typedef struct _IP_PACKET {
    DEFINE_TAG
    OBJECT_FREE_ROUTINE Free;           /* Routine used to free resources for the object */
    UCHAR Type;                         /* Type of IP packet (see IP_ADDRESS_xx above) */
    UCHAR Flags;                        /* Flags for packet (see IP_PACKET_FLAG_xx below)*/
    PVOID Header;                       /* Pointer to IP header for this packet */
    UINT HeaderSize;                    /* Size of IP header */
    PVOID Data;                         /* Current pointer into packet data */
    UINT TotalSize;                     /* Total amount of data in packet (IP header and data) */
    UINT ContigSize;                    /* Number of contiguous bytes left in current buffer */
    UINT Position;                      /* Current logical offset into packet */
    PNDIS_PACKET NdisPacket;            /* Pointer to NDIS packet */
    IP_ADDRESS SrcAddr;                 /* Source address */
    IP_ADDRESS DstAddr;                 /* Destination address */
} IP_PACKET, *PIP_PACKET;

#define IP_PACKET_FLAG_RAW      0x01    /* Raw IP packet */


/* Packet context */
typedef struct _PACKET_CONTEXT {
    PACKET_COMPLETION_ROUTINE DLComplete; /* Data link level completion handler
					   * Also used to link to next packet
					   * in a queue */
    PVOID Context;                        /* Context information for handler */
    UINT  PacketType;                     /* Type of packet */
} PACKET_CONTEXT, *PPACKET_CONTEXT;

/* The ProtocolReserved field is structured as a PACKET_CONTEXT */
#define PC(Packet) ((PPACKET_CONTEXT)(&Packet->ProtocolReserved))

/* Values for address type -- also the interface flags */
/* These values are mean to overlap meaningfully with the BSD ones */
#define ADE_UNICAST     0x01
#define ADE_BROADCAST   0x02
#define ADE_ADDRMASK    0x04
#define ADE_POINTOPOINT 0x10
#define ADE_MULTICAST   0x8000

/* There is one NTE for each source (unicast) address assigned to an interface */
/* Link layer transmit prototype */
typedef VOID (*LL_TRANSMIT_ROUTINE)(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    UINT Offset,
    PVOID LinkAddress,
    USHORT Type);

/* Link layer to IP binding information */
typedef struct _LLIP_BIND_INFO {
    PVOID Context;                /* Pointer to link layer context information */
    UINT  HeaderSize;             /* Size of link level header */
    UINT  MinFrameSize;           /* Minimum frame size in bytes */
    UINT  MTU;                    /* Maximum transmission unit */
    PUCHAR Address;               /* Pointer to interface address */
    UINT  AddressLength;          /* Length of address in bytes */
    LL_TRANSMIT_ROUTINE Transmit; /* Transmit function for this interface */
} LLIP_BIND_INFO, *PLLIP_BIND_INFO;

typedef struct _SEND_RECV_STATS {
    UINT InBytes;
    UINT InUnicast;
    UINT InNUnicast;
    UINT InDiscarded;
    UINT InErrors;
    UINT InDiscardedUnknownProto;
    UINT OutBytes;
    UINT OutUnicast;
    UINT OutNUnicast;
    UINT OutDiscarded;
    UINT OutErrors;
} SEND_RECV_STATS, *PSEND_RECV_STATS;

/* Information about an IP interface */
typedef struct _IP_INTERFACE {
    DEFINE_TAG
    LIST_ENTRY ListEntry;         /* Entry on list */
    OBJECT_FREE_ROUTINE Free;     /* Routine used to free resources used by the object */
    KSPIN_LOCK Lock;              /* Spin lock for this object */
    PVOID Context;                /* Pointer to link layer context information */
    UINT  HeaderSize;             /* Size of link level header */
    UINT  MinFrameSize;           /* Minimum frame size in bytes */
    UINT  MTU;                    /* Maximum transmission unit */
    UINT  Speed;                  /* Link speed */
    IP_ADDRESS Unicast;           /* Unicast address */
    IP_ADDRESS PointToPoint;      /* Point to point address */
    IP_ADDRESS Netmask;           /* Netmask */
    IP_ADDRESS Broadcast;         /* Broadcast */
    UNICODE_STRING Name;          /* Adapter name (GUID) */
    UNICODE_STRING Description;   /* Adapter description (Human readable) */
    PUCHAR Address;               /* Pointer to interface address */
    UINT  AddressLength;          /* Length of address in bytes */
    UINT  Index;                  /* Index of adapter (used to add ip addr) */
    LL_TRANSMIT_ROUTINE Transmit; /* Pointer to transmit function */
    PVOID TCPContext;             /* TCP Content for this interface */
    SEND_RECV_STATS Stats;        /* Send/Receive statistics */
} IP_INTERFACE, *PIP_INTERFACE;

typedef struct _IP_SET_ADDRESS {
    ULONG NteIndex;
    IPv4_RAW_ADDRESS Address;
    IPv4_RAW_ADDRESS Netmask;
} IP_SET_ADDRESS, *PIP_SET_ADDRESS;

#define IP_PROTOCOL_TABLE_SIZE 0x100

typedef VOID (*IP_PROTOCOL_HANDLER)(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

/* Loopback adapter address information (network byte order) */
#define LOOPBACK_ADDRESS_IPv4   ((IPv4_RAW_ADDRESS)DH2N(0x7F000001))
#define LOOPBACK_BCASTADDR_IPv4 ((IPv4_RAW_ADDRESS)DH2N(0x7F0000FF))
#define LOOPBACK_ADDRMASK_IPv4  ((IPv4_RAW_ADDRESS)DH2N(0xFFFFFF00))

/* Protocol definitions */
#ifndef IPPROTO_RAW
#define IPPROTO_RAW     0   /* Raw IP */
#endif
#define IPPROTO_ICMP    1   /* Internet Control Message Protocol */
#define IPPROTO_IGMP    2   /* Internet Group Management Protocol */
#define IPPROTO_TCP     6   /* Transmission Control Protocol */
#define IPPROTO_UDP     17  /* User Datagram Protocol */

/* Timeout timer constants */
#define IP_TICKS_SECOND 2                   /* Two ticks per second */
#define IP_TIMEOUT (1000 / IP_TICKS_SECOND) /* Timeout in milliseconds */
#define IP_DEFAULT_LINK_SPEED  10000

extern LIST_ENTRY InterfaceListHead;
extern KSPIN_LOCK InterfaceListLock;
extern LIST_ENTRY NetTableListHead;
extern KSPIN_LOCK NetTableListLock;
extern UINT MaxLLHeaderSize;
extern UINT MinLLFrameSize;
extern BOOLEAN IpWorkItemQueued;

PIP_PACKET IPCreatePacket(
  ULONG Type);

PIP_PACKET IPInitializePacket(
    PIP_PACKET IPPacket,
    ULONG Type);

PIP_INTERFACE IPCreateInterface(
    PLLIP_BIND_INFO BindInfo);

VOID IPAddInterfaceRoute(
    PIP_INTERFACE IF);

VOID IPRemoveInterfaceRoute(
    PIP_INTERFACE IF);

VOID IPDestroyInterface(
    PIP_INTERFACE IF);

BOOLEAN IPRegisterInterface(
    PIP_INTERFACE IF);

VOID IPUnregisterInterface(
    PIP_INTERFACE IF);

VOID NTAPI IPTimeout( PVOID Context );

VOID IPDispatchProtocol(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

VOID IPRegisterProtocol(
    UINT ProtocolNumber,
    IP_PROTOCOL_HANDLER Handler);

NTSTATUS IPStartup(PUNICODE_STRING RegistryPath);

NTSTATUS IPShutdown(VOID);



#endif /* __IP_H */

/* EOF */
