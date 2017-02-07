#ifndef _NETIODEF_
#define _NETIODEF_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ws2def.h"
#include "ws2ipdef.h"
#include "mswsockdef.h"
#include "mstcpip.h"
#include "nldef.h"

#include "ifdef.h"

#ifndef ASSERT
#define ASSERT(x) ((void)0)
#define NETIODEF_DEFINED_ASSERT
#endif

#ifndef NETIO_INLINE
#if DBG
#define NETIO_INLINE __inline
#else
#define NETIO_INLINE __forceinline
#endif
#endif

#define IS_POWER_OF_TWO(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))
#define IS_VALID_IPV4_MASK(x) ((x.S_un.S_addr == (ULONG)-1) || IS_POWER_OF_TWO(~RtlUlongByteSwap(x.S_un.S_addr)+1))

#ifndef BYTE_ORDER

#define _LITTLE_ENDIAN 1234
#define _BIG_ENDIAN    4321
#define _PDP_ENDIAN    3412

#if defined(i286) || defined(i386) || defined(_AMD64_) || defined(_IA64_)
#define BYTE_ORDER _LITTLE_ENDIAN
#endif

#endif /* BYTE_ORDER */

typedef UINT8 HANDLE8, *PHANDLE8;
typedef UINT16 HANDLE16, *PHANDLE16;
typedef UINT32 HANDLE32, *PHANDLE32;
typedef UINT64 HANDLE64, *PHANDLE64;

#define MAKE_DD_DEVICE_NAME(x)  (L"\\Device\\" x)
#define MAKE_WIN_DEVICE_NAME(x) (L"\\\\.\\" x)

#define DL_ADDRESS_LENGTH_MAXIMUM         IF_MAX_PHYS_ADDRESS_LENGTH
#define DL_HEADER_LENGTH_MAXIMUM          64
#define DL_ETHERNET_HEADER_LENGTH_MAXIMUM (sizeof(ETHERNET_HEADER) + sizeof(SNAP_HEADER))
#define DL_TUNNEL_HEADER_LENGTH_MAXIMUM   max(sizeof(IPV4_HEADER), sizeof(IPV6_HEADER))

#define EUI48_BROADCAST_INIT {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

#define SNAP_DSAP                 0xaa
#define SNAP_SSAP                 0xaa
#define SNAP_CONTROL              0x03
#define SNAP_OUI                  0x00

#define SNAP_TYPE_ARP             ETHERNET_TYPE_ARP
#define SNAP_TYPE_IPV4            ETHERNET_TYPE_IPV4
#define SNAP_TYPE_IPV6            ETHERNET_TYPE_IPV6

#define ETH_LENGTH_OF_HEADER      14
#define ETH_LENGTH_OF_VLAN_HEADER 4
#define ETH_LENGTH_OF_SNAP_HEADER 8

#define ETHERNET_TYPE_MINIMUM     0x0600
#define ETHERNET_TYPE_IPV4        0x0800
#define ETHERNET_TYPE_ARP         0x0806
#define ETHERNET_TYPE_IPV6        0x86dd
#define ETHERNET_TYPE_802_1Q      0x8100

#define TOKENRING_ACCESSCONTROL   0x10
#define TOKENRING_FRAMECONTROL    0x40

#define SOURCEROUTE_RD_MAXIMUM    8
#define SOURCEROUTE_SIZE_MAXIMUM  (sizeof(SOURCEROUTE_HEADER) + 2 * SOURCEROUTE_RD_MAXIMUM)

#define SOURCEROUTE_UNSPECIFIED_INIT {{sizeof(SOURCEROUTE_HEADER), TokenRingSpanningTreeExplorer}, {0}}

#define TOKENRING_MTU_DEFAULT           1500
#define TOKENRING_SOURCEROUTE_INDICATOR 0x80

#define FDDI_FRAMECODE              0x57
#define FDDI_MTU                    4352

#define ARCNET_BROADCAST_MASK       0xff
#define ARCNET_BROADCAST_VALUE      0x00
#define ARCNET_PROTOCOL_ARP         0xd5
#define ARCNET_PROTOCOL_IPV4        0xd4
#define ARCNET_PROTOCOL_IPV6        0xc4

#define ip4_hdr          _IPV4_HEADER
#define ip4_ver_hlen     VersionAndHeaderLength
#define ip4_ver          Version
#define ip4_hlen         HeaderLength
#define ip4_tos          TypeOfService
#define ip4_len          TotalLength
#define ip4_id           Identification
#define ip4_flags_offset FlagsAndOffset
#define ip4_flags        Flags
#define ip4_offset       FragmentOffset
#define ip4_ttl          TimeToLive
#define ip4_protocol     Protocol
#define ip4_xsum         HeaderChecksum
#define ip4_src          SourceAddress
#define ip4_dest         DestinationAddress

#define IP_VER_MASK          0xF0
#define IPV4_VERSION         4
#define IPV4_DEFAULT_VERHLEN ((IPV4_VERSION << 4) | (sizeof(IPV4_HEADER) / sizeof(UINT32)))
#define IPV4_TOS_MASK        0xFC

#define MAX_IPV4_PACKET      65535
#define MAX_IPV4_PAYLOAD     (MAX_IPV4_PACKET - sizeof(IPV4_HEADER))

#define MAX_IPV4_HLEN        60

#define IPV4_MINIMUM_MTU     576
#define IPV4_MINIMUM_ULMTU   (IPV4_MINIMUM_MTU - sizeof(IPV4_HEADER))

#define MAX_IP_OPTIONS_LENGTH          ((0xF * sizeof(UINT32)) - sizeof(IPV4_HEADER))
#define SIZEOF_IP_OPT_ROUTING_HEADER   3
#define SIZEOF_IP_OPT_TIMESTAMP_HEADER 4
#define SIZEOF_IP_OPT_SECURITY         11
#define SIZEOF_IP_OPT_STREAMIDENTIFIER 4
#define SIZEOF_IP_OPT_ROUTERALERT      4
#define IP4_OFF_MASK                   0xff1f

#define icmp4_hdr       _ICMPV4_MESSAGE
#define icmp4_type      Header.Type
#define icmp4_code      Header.Code
#define icmp4_cksum     Header.Checksum
#define icmp4_un_data32 Data32
#define icmp4_un_data16 Data16
#define icmp4_un_data8  Data8
#define icmp4_dataun    Data
#define icmp4_data32    icmp4_dataun.icmp4_un_data32
#define icmp4_data16    icmp4_dataun.icmp4_un_data16
#define icmp4_data8     icmp4_dataun.icmp4_un_data8
#define icmp4_pptr      icmp4_data32[0]
#define icmp4_mtu       icmp4_data32[0]
#define icmp4_id        icmp4_data16[0]
#define icmp4_seq       icmp4_data16[1]
#define icmp4_maxdelay  icmp4_data16[0]

#define RsType     RsHeader.icmp4_type
#define RsCode     RsHeader.icmp4_code
#define RsCksum    RsHeader.icmp4_cksum
#define RsReserved RsHeader.icmp4_data32[0]

#define RaType          RaHeader.icmp4_type
#define RaCode          RaHeader.icmp4_code
#define RaCksum         RaHeader.icmp4_cksum
#define RaNumAddr       RaHeader.icmp4_data8[0]
#define RaAddrEntrySize RaHeader.icmp4_data8[1]
#define RaAddrLifetime  RaHeader.icmp4_data16[1]

#define ICMPV4_INVALID_PREFERENCE_LEVEL 0x80000000

#define icmp4_ts_type      Header.icmp4_type
#define icmp4_ts_code      Header.icmp4_code
#define icmp4_ts_cksum     Header.icmp4_cksum
#define icmp4_ts_id        Header.icmp4_id
#define icmp4_ts_seq       Header.icmp4_seq
#define icmp4_ts_originate OriginateTimestamp
#define icmp4_ts_receive   ReceiveTimestamp
#define icmp4_ts_transmit  TransmitTimestamp

#define IGMP_QUERY_TYPE           0x11
#define IGMP_VERSION1_REPORT_TYPE 0x12
#define IGMP_VERSION2_REPORT_TYPE 0x16
#define IGMP_LEAVE_GROUP_TYPE     0x17
#define IGMP_VERSION3_REPORT_TYPE 0x22

#define ip6_hdr  _IPV6_HEADER
#define ip6_flow VersionClassFlow
#define ip6_plen PayloadLength
#define ip6_nxt  NextHeader
#define ip6_hops HopLimit
#define ip6_hlim HopLimit
#define ip6_src  SourceAddress
#define ip6_dst  DestinationAddress

#define IP_VER_MASK  0xF0
#define IPV6_VERSION 0x60

#define IPV6_TRAFFIC_CLASS_MASK 0x0000C00F
#define IPV6_ECN_MASK           0x00003000
#define IPV6_FLOW_LABEL_MASK    0xFFFF0F00
#define MAX_IPV6_PAYLOAD        65535
#define MAX_IPV6_PACKET         (MAX_IPV6_PAYLOAD + sizeof(IPV6_HEADER))

#define IPV6_ECN_SHIFT          12

#define IPV6_MINIMUM_MTU   1280
#define IPV6_MINIMUM_ULMTU (IPV6_MINIMUM_MTU - sizeof(IPV6_HEADER))

#define IPV6_TRAFFIC_CLASS(VersionClassFlow)                          \
    ((UCHAR)((((VersionClassFlow) & IPV6_TRAFFIC_CLASS_MASK) >> 12) + \
             (((VersionClassFlow) & IPV6_TRAFFIC_CLASS_MASK) << 4)))

#define ip6_frag      _IPV6_FRAGMENT_HEADER
#define ip6f_nxt      NextHeader
#define ip6f_reserved Reserved
#define ip6f_offlg    OffsetAndFlags
#define ip6f_ident    Id

#define IP6F_OFF_MASK      0xf8ff
#define IP6F_RESERVED_MASK 0x0600
#define IP6F_MORE_FRAG     0x0100

#define EXT_LEN_UNIT       8

#define IPV6_EXTENSION_HEADER_LENGTH(Blocks)    ((Blocks + 1) * EXT_LEN_UNIT)
#define MAX_IPV6_EXTENSION_HEADER_LENGTH        IPV6_EXTENSION_HEADER_LENGTH(0xFF)
#define IPV6_EXTENSION_HEADER_BLOCKS(Length)    ((Length / EXT_LEN_UNIT) - 1)
#define IP_AUTHENTICATION_HEADER_LENGTH(Blocks) ((Blocks + 2) * 4)
#define IP_AUTHENTICATION_HEADER_BLOCKS(Length) (((Length + sizeof(AUTHENTICATION_HEADER)) / 4) - 2)

#define IPV6_ROUTER_ALERT_LENGTH IPV6_EXTENSION_HEADER_LENGTH(0)

#define ip6_hbh  _IPV6_EXTENSION_HEADER
#define ip6h_nxt NextHeader
#define ip6h_len Length
#define ip6_dest _IPV6_EXTENSION_HEADER
#define ip6d_nxt NextHeader
#define ip6d_len Length

#define IP6OPT_TYPE(Type)     ((Type) & 0xc0)
#define IP6OPT_TYPE_SKIP      0x00
#define IP6OPT_TYPE_DISCARD   0x40
#define IP6OPT_TYPE_FORCEICMP 0x80
#define IP6OPT_TYPE_ICMP      0xc0

#define IP6OPT_MUTABLE         0x20
#define IP6OPT_ISMUTABLE(Type) (((Type) & IP6OPT_MUTABLE) != 0)

#define ip6_opt_jumbo   _IPV6_OPTION_JUMBOGRAM
#define ip6oj_type      Header.Type
#define ip6oj_len       Header.DataLength
#define ip6oj_jumbo_len JumbogramLength

#define ip6_opt_router _IPV6_OPTION_ROUTER_ALERT
#define ip6or_type     Header.Type
#define ip6or_len      Header.DataLength
#define ip6or_value    Value

#define SIZEOF_IPV6_ROUTERALERT IPV6_EXTENSION_HEADER_LENGTH(0)

#define ip6_rthdr    _IPV6_ROUTING_HEADER
#define ip6r_nxt     NextHeader
#define ip6r_len     Length
#define ip6r_type    RoutingType
#define ip6r_segleft SegmentsLeft

#define icmp6_hdr       _ICMPV6_MESSAGE
#define icmp6_type      Header.Type
#define icmp6_code      Header.Code
#define icmp6_cksum     Header.Checksum
#define icmp6_un_data32 Data32
#define icmp6_un_data16 Data16
#define icmp6_un_data8  Data8
#define icmp6_dataun    Data

#define icmp6_data32   icmp6_dataun.icmp6_un_data32
#define icmp6_data16   icmp6_dataun.icmp6_un_data16
#define icmp6_data8    icmp6_dataun.icmp6_un_data8
#define icmp6_pptr     icmp6_data32[0]
#define icmp6_mtu      icmp6_data32[0]
#define icmp6_id       icmp6_data16[0]
#define icmp6_seq      icmp6_data16[1]
#define icmp6_maxdelay icmp6_data16[0]

#define ICMP6_INFOMSG_MASK 0x80

#define ICMP6_DST_UNREACH_NOROUTE     0
#define ICMP6_DST_UNREACH_ADMIN       1
#define ICMP6_DST_UNREACH_BEYONDSCOPE 2
#define ICMP6_DST_UNREACH_ADDR        3
#define ICMP6_DST_UNREACH_NOPORT      4

#define ICMP6_TIME_EXCEED_TRANSIT     0
#define ICMP6_TIME_EXCEED_REASSEMBLY  1

#define ICMP6_PARAMPROB_HEADER        0
#define ICMP6_PARAMPROB_NEXTHEADER    1
#define ICMP6_PARAMPROB_OPTION        2

#define ICMPV6_ECHO_REQUEST_FLAG_REVERSE 0x1

#define nd_rs_type     nd_rs_hdr.icmp6_type
#define nd_rs_code     nd_rs_hdr.icmp6_code
#define nd_rs_cksum    nd_rs_hdr.icmp6_cksum
#define nd_rs_reserved nd_rs_hdr.icmp6_data32[0]

#define nd_ra_type            nd_ra_hdr.icmp6_type
#define nd_ra_code            nd_ra_hdr.icmp6_code
#define nd_ra_cksum           nd_ra_hdr.icmp6_cksum
#define nd_ra_curhoplimit     nd_ra_hdr.icmp6_data8[0]
#define nd_ra_flags_reserved  nd_ra_hdr.icmp6_data8[1]
#define ND_RA_FLAG_MANAGED    0x80
#define ND_RA_FLAG_OTHER      0x40
#define ND_RA_FLAG_HOME_AGENT 0x20
#define ND_RA_FLAG_PREFERENCE 0x18
#define nd_ra_router_lifetime nd_ra_hdr.icmp6_data16[1]

#define nd_ns_type     nd_ns_hdr.icmp6_type
#define nd_ns_code     nd_ns_hdr.icmp6_code
#define nd_ns_cksum    nd_ns_hdr.icmp6_cksum
#define nd_ns_reserved nd_ns_hdr.icmp6_data32[0]

#define nd_na_type           nd_na_hdr.icmp6_type
#define nd_na_code           nd_na_hdr.icmp6_code
#define nd_na_cksum          nd_na_hdr.icmp6_cksum
#define nd_na_flags_reserved nd_na_hdr.icmp6_data32[0]

#if (BYTE_ORDER == _BIG_ENDIAN)
#define ND_NA_FLAG_ROUTER    0x80000000
#define ND_NA_FLAG_SOLICITED 0x40000000
#define ND_NA_FLAG_OVERRIDE  0x20000000
#else
#define ND_NA_FLAG_ROUTER    0x00000080
#define ND_NA_FLAG_SOLICITED 0x00000040
#define ND_NA_FLAG_OVERRIDE  0x00000020
#endif /* (BYTE_ORDER == _BIG_ENDIAN) */

#define nd_rd_type     nd_rd_hdr.icmp6_type
#define nd_rd_code     nd_rd_hdr.icmp6_code
#define nd_rd_cksum    nd_rd_hdr.icmp6_cksum
#define nd_rd_reserved nd_rd_hdr.icmp6_data32[0]

#define ND_OPT_PI_FLAG_ONLINK      0x80
#define ND_OPT_PI_FLAG_AUTO        0x40
#define ND_OPT_PI_FLAG_ROUTER_ADDR 0x20
#define ND_OPT_PI_FLAG_SITE_PREFIX 0x10
#define ND_OPT_PI_FLAG_ROUTE       0x01

#define ND_OPT_RI_FLAG_PREFERENCE  0x18

#define TH_MAX_LEN (0x0F << 2)
#define TH_FIN     0x01
#define TH_SYN     0x02
#define TH_RST     0x04
#define TH_PSH     0x08
#define TH_ACK     0x10
#define TH_URG     0x20
#define TH_ECE     0x40
#define TH_CWR     0x80
#define TH_ALL     (TH_FIN | TH_SYN | TH_RST | TH_PSH | TH_ACK | TH_URG | TH_ECE | TH_CWR)
#define TH_SYN_ALL (TH_FIN | TH_SYN | TH_RST | TH_ACK)

#define TH_OPT_EOL            0x00
#define TH_OPT_NOP            0x01
#define TH_OPT_MSS            0x02
#define TH_OPT_WS             0x03
#define TH_OPT_SACK_PERMITTED 0x04
#define TH_OPT_SACK           0x05
#define TH_OPT_TS             0x08

#define DL_SIZEOF_TUNNEL_ADDRESS(AddressBytes) (FIELD_OFFSET(DL_TUNNEL_ADDRESS, IpAddress) + (AddressBytes))
#define DL_SIZEOF_IPV4_TUNNEL_ADDRESS          DL_SIZEOF_TUNNEL_ADDRESS(sizeof(IN_ADDR))
#define DL_SIZEOF_IPV6_TUNNEL_ADDRESS          DL_SIZEOF_TUNNEL_ADDRESS(sizeof(IN6_ADDR))

#define FL_PACKET_TYPE_FLAGS (NDIS_PACKET_TYPE_ALL_MULTICAST | NDIS_PACKET_TYPE_PROMISCUOUS)

#define mld_type     IcmpHeader.Type
#define mld_checksum IcmpHeader.Checksum

#ifndef _DEFINE_DL_ADDRESS_TYPE_
#define _DEFINE_DL_ADDRESS_TYPE_
typedef enum {
  DlUnicast,
  DlMulticast,
  DlBroadcast
} DL_ADDRESS_TYPE, *PDL_ADDRESS_TYPE;
#endif

union _DL_OUI {
  UINT8 Byte[3];
  struct {
    UINT8 Group:1;
    UINT8 Local:1;
  };
};
typedef union _DL_OUI DL_OUI, *PDL_OUI;

union _DL_EI48 {
  UINT8 Byte[3];
};
typedef union _DL_EI48 DL_EI48, *PDL_EI48;

union _DL_EUI48 {
  UINT8 Byte[6];
  struct {
    DL_OUI Oui;
    DL_EI48 Ei48;
  };
};

C_ASSERT(DL_ADDRESS_LENGTH_MAXIMUM >= sizeof(DL_EUI48));

extern CONST DL_EUI48 eui48_broadcast;

union _DL_EI64 {
  UINT8 Byte[5];
};
typedef union _DL_EI64 DL_EI64, *PDL_EI64;

union _DL_EUI64 {
  UINT8 Byte[8];
  UINT64 Value;
  struct {
    DL_OUI Oui;
    union {
      DL_EI64 Ei64;
      struct {
        UINT8 Type;
        UINT8 Tse;
        DL_EI48 Ei48;
      };
    };
  };
};
typedef union _DL_EUI64 DL_EUI64, *PDL_EUI64;

typedef struct _SNAP_HEADER {
  UINT8 Dsap;
  UINT8 Ssap;
  UINT8 Control;
  UINT8 Oui[3];
  UINT16 Type;
} SNAP_HEADER, *PSNAP_HEADER;

typedef struct _ETHERNET_HEADER {
  DL_EUI48 Destination;
  DL_EUI48 Source;
  union {
    UINT16 Type;
    UINT16 Length;
  };
} ETHERNET_HEADER, *PETHERNET_HEADER;

C_ASSERT(ETH_LENGTH_OF_HEADER == sizeof(ETHERNET_HEADER));
C_ASSERT(ETH_LENGTH_OF_SNAP_HEADER == sizeof(SNAP_HEADER));

typedef struct _VLAN_TAG {
  union {
    UINT16 Tag;
    struct {
      UINT16 VID:12;
      UINT16 CFI:1;
      UINT16 User_Priority:3;
    };
  };
  UINT16 Type;
} VLAN_TAG;

C_ASSERT(ETH_LENGTH_OF_VLAN_HEADER == sizeof(VLAN_TAG));

NETIO_INLINE
DL_ADDRESS_TYPE
EthernetAddressType(
    _In_reads_(sizeof(DL_EUI48)) CONST UCHAR *Address)
{
    if (((PDL_EUI48) Address)->Oui.Group)
    {
        if (RtlEqualMemory(Address, &eui48_broadcast, sizeof(DL_EUI48)))
        {
            return DlBroadcast;
        }
        return DlMulticast;
    }
    else
    {
        return DlUnicast;
    }
}

typedef struct _TOKENRING_HEADER {
  UINT8 AcccessControl;
  UINT8 FrameControl;
  DL_EUI48 Destination;
  DL_EUI48 Source;
} TOKENRING_HEADER, *PTOKENRING_HEADER;

typedef struct _SOURCEROUTE_HEADER {
  union {
    struct {
      UINT8 Length:5;
      UINT8 BroadcastIndicator:3;
    };
    UINT8 Bil;
  };
  union {
    struct {
      UINT8 Direction:1;
      UINT8 LargestFrame:4;
      UINT8 Reserved:3;
    };
    UINT8 Dlf;
  };
  UINT16 Rd[0];
} SOURCEROUTE_HEADER, *PSOURCEROUTE_HEADER;

typedef enum {
  TokenRingSpecificallyRoutedFrame = 0,
  TokenRingAllRoutesExplorer = 4,
  TokenRingSpanningTreeExplorer = 6,
} TOKENRING_BROADCAST_INDICATOR, *PTOKENRING_BROADCAST_INDICATOR;

extern CONST SOURCEROUTE_HEADER sourceroute_unspecified;

C_ASSERT((sizeof(TOKENRING_HEADER) + sizeof(SNAP_HEADER) + SOURCEROUTE_SIZE_MAXIMUM) < DL_HEADER_LENGTH_MAXIMUM);

__inline
BOOLEAN
TokenRingGetSourceRouteIndicator(
    _In_ CONST UNALIGNED DL_EUI48 *Source)
{
    return ((Source->Byte[0] & TOKENRING_SOURCEROUTE_INDICATOR) != 0);
}

__inline
VOID
TokenRingSetSourceRouteIndicator(
    _Out_ UNALIGNED DL_EUI48 *Source)
{
    Source->Byte[0] |= TOKENRING_SOURCEROUTE_INDICATOR;
}

__inline
VOID
TokenRingResetSourceRouteIndicator(
    _Out_ UNALIGNED DL_EUI48 *Source)
{
    Source->Byte[0] &= ~TOKENRING_SOURCEROUTE_INDICATOR;
}

NETIO_INLINE
DL_ADDRESS_TYPE
TokenRingAddressType(
    _In_ CONST UCHAR *Address)
{
    if (TokenRingGetSourceRouteIndicator((PDL_EUI48) Address))
    {
        if (RtlEqualMemory(Address, &eui48_broadcast, sizeof(DL_EUI48)))
        {
            return DlBroadcast;
        }
        return DlMulticast;
    }
    else
    {
        return DlUnicast;
    }
}

typedef struct _FDDI_HEADER {
  UINT8 FrameCode;
  DL_EUI48 Destination;
  DL_EUI48 Source;
} FDDI_HEADER, *PFDDI_HEADER;

typedef struct _ARCNET_HEADER {
  UINT8 Source;
  UINT8 Destination;
  UINT8 Protocol;
} ARCNET_HEADER, *PARCNET_HEADER;

typedef struct _ICMP_HEADER {
  UINT8 Type;
  UINT8 Code;
  UINT16 Checksum;
} ICMP_HEADER, *PICMP_HEADER;

typedef struct _ICMP_MESSAGE {
  ICMP_HEADER Header;
  union {
    UINT32 Data32[1];
    UINT16 Data16[2];
    UINT8 Data8[4];
  } Data;
} ICMP_MESSAGE, *PICMP_MESSAGE;

typedef ICMP_HEADER ICMPV6_HEADER, *PICMPV6_HEADER;
typedef ICMP_MESSAGE ICMPV6_MESSAGE, *PICMPV6_MESSAGE;

typedef struct _IPV4_HEADER {
  union {
    UINT8 VersionAndHeaderLength;
    struct {
      UINT8 HeaderLength:4;
      UINT8 Version:4;
    };
  };
  union {
    UINT8 TypeOfServiceAndEcnField;
    struct {
      UINT8 EcnField:2;
      UINT8 TypeOfService:6;
    };
  };
  UINT16 TotalLength;
  UINT16 Identification;
  union {
    UINT16 FlagsAndOffset;
    struct {
      UINT16 DontUse1:5;
      UINT16 MoreFragments:1;
      UINT16 DontFragment:1;
      UINT16 Reserved:1;
      UINT16 DontUse2:8;
    };
  };
  UINT8 TimeToLive;
  UINT8 Protocol;
  UINT16 HeaderChecksum;
  IN_ADDR SourceAddress;
  IN_ADDR DestinationAddress;
} IPV4_HEADER, *PIPV4_HEADER;

C_ASSERT(sizeof(IPV4_HEADER) == 20);

__inline
UCHAR
Ip4HeaderLengthInBytes(
    _In_ CONST UNALIGNED IPV4_HEADER *Header)
{
    return (Header->HeaderLength << 2);
}

#if defined(_NTDDK_) || defined(_NTRTL_)
__inline
UINT16
Ip4FragmentOffset(
    _In_ CONST UNALIGNED IPV4_HEADER *Header)
{
    return RtlUshortByteSwap(Header->FlagsAndOffset & IP4_OFF_MASK) << 3;
}
#endif

typedef struct _IPV4_OPTION_HEADER {
  union {
    UINT8 OptionType;
    struct {
      UINT8 OptionNumber:5;
      UINT8 OptionClass:2;
      UINT8 CopiedFlag:1;
    };
  };
  UINT8 OptionLength;
} IPV4_OPTION_HEADER, *PIPV4_OPTION_HEADER;

C_ASSERT(sizeof(IPV4_OPTION_HEADER) == 2);

#if !defined(IP_EXPORT_INCLUDED)
typedef enum {
  IP_OPT_EOL = 0x00,
  IP_OPT_NOP = 0x01,
  IP_OPT_SECURITY = 0x82,
  IP_OPT_LSRR = 0x83,
  IP_OPT_TS = 0x44,
  IP_OPT_RR = 0x07,
  IP_OPT_SSRR = 0x89,
  IP_OPT_SID = 0x88,
  IP_OPT_ROUTER_ALERT = 0x94,
  IP_OPT_MULTIDEST = 0x95,
} IPV4_OPTION_TYPE;
#else
typedef ULONG IPV4_OPTION_TYPE;
#endif

typedef struct _IPV4_TIMESTAMP_OPTION {
#ifdef __cplusplus
  IPV4_OPTION_HEADER OptionHeader;
#else
  IPV4_OPTION_HEADER;
#endif
  UINT8 Pointer;
  union {
    UINT8 FlagsOverflow;
    struct {
      UINT8 Flags:4;
      UINT8 Overflow:4;
    };
  };
} IPV4_TIMESTAMP_OPTION, *PIPV4_TIMESTAMP_OPTION;

typedef enum {
  IP_OPTION_TIMESTAMP_ONLY = 0,
  IP_OPTION_TIMESTAMP_ADDRESS = 1,
  IP_OPTION_TIMESTAMP_SPECIFIC_ADDRESS = 3
} IP_OPTION_TIMESTAMP_FLAGS;

typedef struct _IPV4_ROUTING_HEADER {
#ifdef __cplusplus
  IPV4_OPTION_HEADER OptionHeader;
#else
  IPV4_OPTION_HEADER;
#endif
  UINT8 Pointer;
} IPV4_ROUTING_HEADER, UNALIGNED *PIPV4_ROUTING_HEADER;

typedef ICMP_HEADER ICMPV4_HEADER, *PICMPV4_HEADER;
typedef ICMP_MESSAGE ICMPV4_MESSAGE, *PICMPV4_MESSAGE;

typedef enum {
  ICMP4_UNREACH_NET = 0,
  ICMP4_UNREACH_HOST = 1,
  ICMP4_UNREACH_PROTOCOL = 2,
  ICMP4_UNREACH_PORT = 3,
  ICMP4_UNREACH_FRAG_NEEDED = 4,
  ICMP4_UNREACH_SOURCEROUTE_FAILED = 5,
  ICMP4_UNREACH_NET_UNKNOWN = 6,
  ICMP4_UNREACH_HOST_UNKNOWN = 7,
  ICMP4_UNREACH_ISOLATED = 8,
  ICMP4_UNREACH_NET_ADMIN = 9,
  ICMP4_UNREACH_HOST_ADMIN = 10,
  ICMP4_UNREACH_NET_TOS = 11,
  ICMP4_UNREACH_HOST_TOS = 12,
  ICMP4_UNREACH_ADMIN = 13,
} ICMP4_UNREACH_CODE, *PICMP4_UNREACH_CODE;

typedef enum {
  ICMP4_TIME_EXCEED_TRANSIT = 0,
  ICMP4_TIME_EXCEED_REASSEMBLY = 1,
} ICMP4_TIME_EXCEED_CODE, *PICMP4_TIME_EXCEED_CODE;

typedef struct _ICMPV4_ROUTER_SOLICIT {
  ICMPV4_MESSAGE RsHeader;
} ICMPV4_ROUTER_SOLICIT, *PICMPV4_ROUTER_SOLICIT;

typedef struct _ICMPV4_ROUTER_ADVERT_HEADER {
  ICMPV4_MESSAGE RaHeader;
} ICMPV4_ROUTER_ADVERT_HEADER, *PICMPV4_ROUTER_ADVERT_HEADER;

typedef struct _ICMPV4_ROUTER_ADVERT_ENTRY {
  IN_ADDR RouterAdvertAddr;
  LONG PreferenceLevel;
} ICMPV4_ROUTER_ADVERT_ENTRY, *PICMPV4_ROUTER_ADVERT_ENTRY;

typedef struct _ICMPV4_TIMESTAMP_MESSAGE {
  ICMPV4_MESSAGE Header;
  UINT32 OriginateTimestamp;
  UINT32 ReceiveTimestamp;
  UINT32 TransmitTimestamp;
} ICMPV4_TIMESTAMP_MESSAGE, *PICMPV4_TIMESTAMP_MESSAGE;

typedef struct _ICMPV4_ADDRESS_MASK_MESSAGE {
  ICMPV4_MESSAGE Header;
  UINT32 AddressMask;
} ICMPV4_ADDRESS_MASK_MESSAGE, *PICMPV4_ADDRESS_MASK_MESSAGE;

typedef struct _ARP_HEADER {
  USHORT HardwareAddressSpace;
  USHORT ProtocolAddressSpace;
  UCHAR HardwareAddressLength;
  UCHAR ProtocolAddressLength;
  USHORT Opcode;
  UCHAR SenderHardwareAddress[0];
} ARP_HEADER, *PARP_HEADER;

typedef enum {
  ARP_REQUEST = 1,
  ARP_RESPONSE = 2
} ARP_OPCODE;

typedef enum {
  ARP_HW_ENET = 1,
  ARP_HW_802 = 6
} ARP_HARDWARE_TYPE;

typedef struct _IGMP_HEADER {
  union {
    struct {
      UINT8 Type:4;
      UINT8 Version:4;
    };
    UINT8 VersionType;
  };
  union {
    UINT8 Reserved;
    UINT8 MaxRespTime;
    UINT8 Code;
  };
  UINT16 Checksum;
  IN_ADDR MulticastAddress;
} IGMP_HEADER, *PIGMP_HEADER;

typedef enum {
  IGMP_MAX_RESP_CODE_TYPE_NORMAL = 0,
  IGMP_MAX_RESP_CODE_TYPE_FLOAT
} IGMP_MAX_RESP_CODE_TYPE;

typedef struct _IGMPV3_QUERY_HEADER {
  UINT8 Type;
  union {
    UINT8 MaxRespCode;
    struct {
      UINT8 MaxRespCodeMantissa:4;
      UINT8 MaxRespCodeExponent:3;
      UINT8 MaxRespCodeType:1;
    };
  };
  UINT16 Checksum;
  IN_ADDR MulticastAddress;
  UINT8 QuerierRobustnessVariable:3;
  UINT8 SuppressRouterSideProcessing:1;
  UINT8 Reserved:4;
  union {
    UINT8 QueriersQueryInterfaceCode;
    struct {
      UINT8 QQCMantissa:4;
      UINT8 QQCExponent:3;
      UINT8 QQCType:1;
    };
  };
  UINT16 SourceCount;
} IGMPV3_QUERY_HEADER, *PIGMPV3_QUERY_HEADER;

typedef struct _IGMPV3_REPORT_RECORD_HEADER {
  UINT8 Type;
  UINT8 AuxillaryDataLength;
  UINT16 SourceCount;
  IN_ADDR MulticastAddress;
} IGMPV3_REPORT_RECORD_HEADER, *PIGMPV3_REPORT_RECORD_HEADER;

typedef struct _IGMPV3_REPORT_HEADER_ {
  UINT8 Type;
  UINT8 Reserved;
  UINT16 Checksum;
  UINT16 Reserved2;
  UINT16 RecordCount;
} IGMPV3_REPORT_HEADER, *PIGMPV3_REPORT_HEADER;

typedef struct _IPV6_HEADER {
  UINT32 VersionClassFlow;
  UINT16 PayloadLength;
  UINT8 NextHeader;
  UINT8 HopLimit;
  IN6_ADDR SourceAddress;
  IN6_ADDR DestinationAddress;
} IPV6_HEADER, *PIPV6_HEADER;

typedef struct _IPV6_FRAGMENT_HEADER {
  UINT8 NextHeader;
  UINT8 Reserved;
  union {
    struct {
      UINT16 DontUse1:8;
      UINT16 MoreFragments:1;
      UINT16 ReservedBits:2;
      UINT16 DontUse2:5;
    };
    UINT16 OffsetAndFlags;
  };
  UINT32 Id;
} IPV6_FRAGMENT_HEADER, *PIPV6_FRAGMENT_HEADER;

C_ASSERT(sizeof(IPV6_FRAGMENT_HEADER) == 8);

#if defined(_NTDDK_) || defined(_NTRTL_)
__inline
UINT16
Ip6FragmentOffset(
    _In_ CONST UNALIGNED IPV6_FRAGMENT_HEADER *Header)
{
    return RtlUshortByteSwap(Header->OffsetAndFlags & IP6F_OFF_MASK);
}
#endif

typedef struct _IPV6_EXTENSION_HEADER {
  UINT8 NextHeader;
  UINT8 Length;
} IPV6_EXTENSION_HEADER, *PIPV6_EXTENSION_HEADER;

typedef struct _IPV6_OPTION_HEADER {
  UINT8 Type;
  UINT8 DataLength;
} IPV6_OPTION_HEADER, *PIPV6_OPTION_HEADER;

typedef enum {
  IP6OPT_PAD1 = 0x00,
  IP6OPT_PADN = 0x01,
  IP6OPT_TUNNEL_LIMIT = 0x04,
  IP6OPT_ROUTER_ALERT = 0x05,
  IP6OPT_JUMBO = 0xc2,
  IP6OPT_NSAP_ADDR = 0xc3,
} IPV6_OPTION_TYPE, *PIPV6_OPTION_TYPE;

typedef struct _IPV6_OPTION_JUMBOGRAM {
  IPV6_OPTION_HEADER Header;
  UINT8 JumbogramLength[4];
} IPV6_OPTION_JUMBOGRAM, *PIPV6_OPTION_JUMBOGRAM;

typedef struct _IPV6_OPTION_ROUTER_ALERT {
  IPV6_OPTION_HEADER Header;
  UINT8 Value[2];
} IPV6_OPTION_ROUTER_ALERT, *PIPV6_OPTION_ROUTER_ALERT;

typedef _Struct_size_bytes_(_Inexpressible_(Length)) struct _IPV6_ROUTING_HEADER {
  UCHAR NextHeader;
  UCHAR Length;
  UCHAR RoutingType;
  UCHAR SegmentsLeft;
  UCHAR Reserved[4];
} IPV6_ROUTING_HEADER, *PIPV6_ROUTING_HEADER;

typedef struct nd_router_solicit {
  ICMPV6_MESSAGE nd_rs_hdr;
} ND_ROUTER_SOLICIT_HEADER, *PND_ROUTER_SOLICIT_HEADER;

typedef struct nd_router_advert {
  ICMPV6_MESSAGE nd_ra_hdr;
  UINT32 nd_ra_reachable;
  UINT32 nd_ra_retransmit;
} ND_ROUTER_ADVERT_HEADER, *PND_ROUTER_ADVERT_HEADER;

typedef union _IPV6_ROUTER_ADVERTISEMENT_FLAGS {
  struct {
    UINT8 Reserved:3;
    UINT8 Preference:2;
    UINT8 HomeAgent:1;
    UINT8 OtherStatefulConfiguration:1;
    UINT8 ManagedAddressConfiguration:1;
  };
  UINT8 Value;
} IPV6_ROUTER_ADVERTISEMENT_FLAGS, *PIPV6_ROUTER_ADVERTISEMENT_FLAGS;

typedef struct nd_neighbor_solicit {
  ICMPV6_MESSAGE nd_ns_hdr;
  IN6_ADDR nd_ns_target;
} ND_NEIGHBOR_SOLICIT_HEADER, *PND_NEIGHBOR_SOLICIT_HEADER;

typedef struct nd_neighbor_advert {
  ICMPV6_MESSAGE nd_na_hdr;
  IN6_ADDR nd_na_target;
} ND_NEIGHBOR_ADVERT_HEADER, *PND_NEIGHBOR_ADVERT_HEADER;

typedef union _IPV6_NEIGHBOR_ADVERTISEMENT_FLAGS {
  struct {
    UINT8 Reserved1:5;
    UINT8 Override:1;
    UINT8 Solicited:1;
    UINT8 Router:1;
    UINT8 Reserved2[3];
  };
  UINT32 Value;
} IPV6_NEIGHBOR_ADVERTISEMENT_FLAGS, *PIPV6_NEIGHBOR_ADVERTISEMENT_FLAGS;

typedef struct nd_redirect {
  ICMPV6_MESSAGE nd_rd_hdr;
  IN6_ADDR nd_rd_target;
  IN6_ADDR nd_rd_dst;
} ND_REDIRECT_HEADER, *PND_REDIRECT_HEADER;

typedef struct nd_opt_hdr {
  UINT8 nd_opt_type;
  UINT8 nd_opt_len;
} ND_OPTION_HDR, *PND_OPTION_HDR;

typedef enum {
  ND_OPT_SOURCE_LINKADDR = 1,
  ND_OPT_TARGET_LINKADDR = 2,
  ND_OPT_PREFIX_INFORMATION = 3,
  ND_OPT_REDIRECTED_HEADER = 4,
  ND_OPT_MTU = 5,
  ND_OPT_NBMA_SHORTCUT_LIMIT = 6,
  ND_OPT_ADVERTISEMENT_INTERVAL = 7,
  ND_OPT_HOME_AGENT_INFORMATION = 8,
  ND_OPT_SOURCE_ADDR_LIST = 9,
  ND_OPT_TARGET_ADDR_LIST = 10,
  ND_OPT_ROUTE_INFO = 24,
} ND_OPTION_TYPE, *PND_OPTION_TYPE;

typedef struct nd_opt_prefix_info {
  UINT8 nd_opt_pi_type;
  UINT8 nd_opt_pi_len;
  UINT8 nd_opt_pi_prefix_len;
  union {
    UINT8 nd_opt_pi_flags_reserved;
    struct {
      UINT8 Route : 1;    // Least significant bit.
      UINT8 Reserved1 : 3;
      UINT8 SitePrefix : 1;
      UINT8 RouterAddress : 1;
      UINT8 Autonomous : 1;
      UINT8 OnLink : 1;
    } Flags;
  };
  UINT32 nd_opt_pi_valid_time;
  UINT32 nd_opt_pi_preferred_time;
  union {
    UINT32 nd_opt_pi_reserved2;
    struct {
      UINT8 nd_opt_pi_reserved3[3];
      UINT8 nd_opt_pi_site_prefix_len;
    };
  };
  IN6_ADDR nd_opt_pi_prefix;
} ND_OPTION_PREFIX_INFO, *PND_OPTION_PREFIX_INFO;

typedef struct nd_opt_rd_hdr {
  UINT8 nd_opt_rh_type;
  UINT8 nd_opt_rh_len;
  UINT16 nd_opt_rh_reserved1;
  UINT32 nd_opt_rh_reserved2;
} ND_OPTION_RD_HDR, *PND_OPTION_RD_HDR;

typedef struct nd_opt_mtu {
  UINT8 nd_opt_mtu_type;
  UINT8 nd_opt_mtu_len;
  UINT16 nd_opt_mtu_reserved;
  UINT32 nd_opt_mtu_mtu;
} ND_OPTION_MTU, *PND_OPTION_MTU;

typedef struct nd_opt_route_info {
  UINT8 nd_opt_ri_type;
  UINT8 nd_opt_ri_len;
  UINT8 nd_opt_ri_prefix_len;
  union {
    UINT8 nd_opt_ri_flags_reserved;
    struct {
      UINT8 Reserved:3;
      UINT8 Preference:2;
    } Flags;
  };
  UINT32 nd_opt_ri_route_lifetime;
  IN6_ADDR nd_opt_ri_prefix;
} ND_OPTION_ROUTE_INFO, *PND_OPTION_ROUTE_INFO;

typedef struct _MLD_HEADER {
  ICMPV6_HEADER IcmpHeader;
  UINT16 MaxRespTime;
  UINT16 Reserved;
  IN6_ADDR MulticastAddress;
} MLD_HEADER, *PMLD_HEADER;

typedef enum {
  MLD_MAX_RESP_CODE_TYPE_NORMAL = 0,
  MLD_MAX_RESP_CODE_TYPE_FLOAT
} MLD_MAX_RESP_CODE_TYPE;

typedef struct _MLDV2_QUERY_HEADER {
  ICMPV6_HEADER IcmpHeader;
  union {
    UINT16 MaxRespCode;
    struct {
      UINT16 MaxRespCodeMantissaHi:4;
      UINT16 MaxRespCodeExponent:3;
      UINT16 MaxRespCodeType:1;
      UINT16 MaxRespCodeMantissaLo:8;
    };
  };
  UINT16 Reserved;
  IN6_ADDR MulticastAddress;
  UINT8 QuerierRobustnessVariable:3;
  UINT8 SuppressRouterSideProcessing:1;
  UINT8 QueryReserved:4;
  union {
    UINT8 QueriersQueryInterfaceCode;
    struct {
      UINT8 QQCMantissa:4;
      UINT8 QQCExponent:3;
      UINT8 QQCType:1;
    };
  };
  UINT16 SourceCount;
} MLDV2_QUERY_HEADER, *PMLDV2_QUERY_HEADER;

typedef struct _MLDV2_REPORT_RECORD_HEADER {
  UINT8 Type;
  UINT8 AuxillaryDataLength;
  UINT16 SourceCount;
  IN6_ADDR MulticastAddress;
} MLDV2_REPORT_RECORD_HEADER, *PMLDV2_REPORT_RECORD_HEADER;

typedef struct _MLDV2_REPORT_HEADER {
  ICMPV6_HEADER IcmpHeader;
  UINT16 Reserved;
  UINT16 RecordCount;
} MLDV2_REPORT_HEADER, *PMLDV2_REPORT_HEADER;

typedef UINT32 SEQ_NUM, *PSEQ_NUM;

#pragma pack(push, 1)

typedef struct tcp_hdr {
  UINT16 th_sport;
  UINT16 th_dport;
  SEQ_NUM th_seq;
  SEQ_NUM th_ack;
  UINT8 th_x2:4;
  UINT8 th_len:4;
  UINT8 th_flags;
  UINT16 th_win;
  UINT16 th_sum;
  UINT16 th_urp;
} TCP_HDR;
#if NDIS_RECEIVE_UNALIGNED
typedef TCP_HDR UNALIGNED *PTCP_HDR;
#else
typedef TCP_HDR *PTCP_HDR;
#endif

typedef struct tcp_opt_mss {
  UINT8 Kind;
  UINT8 Length;
  UINT16 Mss;
} TCP_OPT_MSS;

typedef struct tcp_opt_ws {
  UINT8 Kind;
  UINT8 Length;
  UINT8 ShiftCnt;
} TCP_OPT_WS;

typedef struct tcp_opt_sack_permitted {
  UINT8 Kind;
  UINT8 Length;
} TCP_OPT_SACK_PERMITTED;

typedef struct tcp_opt_sack {
  UINT8 Kind;
  UINT8 Length;
  struct tcp_opt_sack_block {
    SEQ_NUM Left;
    SEQ_NUM Right;
  } Block[0];
} TCP_OPT_SACK;

typedef struct tcp_opt_ts {
  UINT8 Kind;
  UINT8 Length;
  UINT32 Val;
  UINT32 EcR;
} TCP_OPT_TS;

typedef struct tcp_opt_unknown {
  UINT8 Kind;
  UINT8 Length;
} TCP_OPT_UNKNOWN;

#pragma pack(pop)

typedef struct DL_TUNNEL_ADDRESS {
  COMPARTMENT_ID CompartmentId;
  SCOPE_ID ScopeId;
  UCHAR IpAddress[0];
} DL_TUNNEL_ADDRESS, *PDL_TUNNEL_ADDRESS;

C_ASSERT(DL_ADDRESS_LENGTH_MAXIMUM >= DL_SIZEOF_IPV4_TUNNEL_ADDRESS);
C_ASSERT(DL_ADDRESS_LENGTH_MAXIMUM >= DL_SIZEOF_IPV6_TUNNEL_ADDRESS);

typedef enum _TUNNEL_SUB_TYPE {
  TUNNEL_SUB_TYPE_NONE = 0,
  TUNNEL_SUB_TYPE_CP = 1,
  TUNNEL_SUB_TYPE_IPTLS = 2
} TUNNEL_SUB_TYPE, *PTUNNEL_SUB_TYPE;

#pragma pack(push, 1)

typedef struct DL_TEREDO_ADDRESS {
  UINT8 Reserved[6];
  union {
    DL_EUI64 Eui64;
    struct {
      USHORT Flags;
      USHORT MappedPort;
      IN_ADDR MappedAddress;
    };
  };
} DL_TEREDO_ADDRESS, *PDL_TEREDO_ADDRESS;

typedef struct _IPTLS_METADATA {
  ULONGLONG SequenceNumber;
} IPTLS_METADATA,*PIPTLS_METADATA;

#pragma pack(pop)

C_ASSERT(DL_ADDRESS_LENGTH_MAXIMUM >= sizeof(DL_TEREDO_ADDRESS));

typedef enum _NPI_MODULEID_TYPE {
  MIT_GUID = 1,
  MIT_IF_LUID,
} NPI_MODULEID_TYPE;

typedef struct _NPI_MODULEID {
  USHORT Length;
  NPI_MODULEID_TYPE Type;
#ifdef __midl
  [switch_type(NPI_MODULEID_TYPE), switch_is(Type)]
#endif
  union {
#ifdef __midl
    [case(MIT_GUID)]
#endif
    GUID Guid;
#ifdef __midl
    [case(MIT_IF_LUID)]
#endif
    LUID IfLuid;
#ifdef __midl
    [default];
#endif
  };
} NPI_MODULEID;
typedef CONST NPI_MODULEID *PNPI_MODULEID;

#if !defined(__midl)
__inline
BOOLEAN
NmrIsEqualNpiModuleId(
    _In_ PNPI_MODULEID ModuleId1,
    _In_ PNPI_MODULEID ModuleId2)
{
    if (ModuleId1->Type == ModuleId2->Type)
    {
        if (ModuleId1->Type == MIT_GUID)
        {
#ifdef __cplusplus
            return !!InlineIsEqualGUID(ModuleId1->Guid, ModuleId2->Guid);
#else
            return (BOOLEAN)InlineIsEqualGUID(&ModuleId1->Guid,
                                              &ModuleId2->Guid);
#endif
        }
        else if (ModuleId1->Type == MIT_IF_LUID)
        {
            return (BOOLEAN)RtlEqualMemory(&ModuleId1->IfLuid,
                                           &ModuleId2->IfLuid,
                                           sizeof(LUID));
        }
    }
    return FALSE;
}
#endif

typedef GUID NPIID;
typedef CONST NPIID *PNPIID;

extern CONST NPI_MODULEID NPI_MS_NMR_MODULEID;
extern CONST NPIID NPI_FRAMING_LAYER_ID;
extern CONST NPI_MODULEID NPI_MS_TUN_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL48_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL68_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL4L_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL6L_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL4T_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL6T_MODULEID;
extern CONST NPI_MODULEID NPI_MS_WANARPV4_MODULEID;
extern CONST NPI_MODULEID NPI_MS_WANARPV6_MODULEID;
extern CONST NPI_MODULEID NPI_MS_FL_WANARP_MODULEID;
extern CONST NPI_MODULEID NPI_MS_NDIS_MODULEID;
extern CONST NPIID NPI_NETWORK_LAYER_ID;
extern CONST NPI_MODULEID NPI_MS_IPV6_MODULEID;
extern CONST NPI_MODULEID NPI_MS_IPV4_MODULEID;

#ifdef NETIODEF_DEFINED_ASSERT
#undef ASSERT
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NETIODEF_ */
