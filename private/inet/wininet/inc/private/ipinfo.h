/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** IPINFO.H - IP SNMP information definitions..
//
// This file contains all of the definitions for IP that are
// related to SNMP information gathering.

#ifndef IPINFO_INCLUDED
#define IPINFO_INCLUDED


#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED


typedef struct IPSNMPInfo {
        ulong           ipsi_forwarding;
        ulong           ipsi_defaultttl;
        ulong           ipsi_inreceives;
        ulong           ipsi_inhdrerrors;
        ulong           ipsi_inaddrerrors;
        ulong           ipsi_forwdatagrams;
        ulong           ipsi_inunknownprotos;
        ulong           ipsi_indiscards;
        ulong           ipsi_indelivers;
        ulong           ipsi_outrequests;
        ulong           ipsi_routingdiscards;
        ulong           ipsi_outdiscards;
        ulong           ipsi_outnoroutes;
        ulong           ipsi_reasmtimeout;
        ulong           ipsi_reasmreqds;
        ulong           ipsi_reasmoks;
        ulong           ipsi_reasmfails;
        ulong           ipsi_fragoks;
        ulong           ipsi_fragfails;
        ulong           ipsi_fragcreates;
        ulong           ipsi_numif;
        ulong           ipsi_numaddr;
        ulong           ipsi_numroutes;
} IPSNMPInfo;

typedef struct ICMPStats {
        ulong           icmps_msgs;
        ulong           icmps_errors;
        ulong           icmps_destunreachs;
        ulong           icmps_timeexcds;
        ulong           icmps_parmprobs;
        ulong           icmps_srcquenchs;
        ulong           icmps_redirects;
        ulong           icmps_echos;
        ulong           icmps_echoreps;
        ulong           icmps_timestamps;
        ulong           icmps_timestampreps;
        ulong           icmps_addrmasks;
        ulong           icmps_addrmaskreps;
} ICMPStats;

typedef struct ICMPSNMPInfo {
        ICMPStats       icsi_instats;
        ICMPStats       icsi_outstats;
} ICMPSNMPInfo;

#define IP_FORWARDING           1
#define IP_NOT_FORWARDING       2

typedef struct IPAddrEntry {
        ulong           iae_addr;
        ulong           iae_index;
        ulong           iae_mask;
        ulong           iae_bcastaddr;
        ulong           iae_reasmsize;
        ushort          iae_context;
        ushort          iae_pad;
} IPAddrEntry;

typedef struct IPRouteEntry {
        ulong           ire_dest;
        ulong           ire_index;
        ulong           ire_metric1;
        ulong           ire_metric2;
        ulong           ire_metric3;
        ulong           ire_metric4;
        ulong           ire_nexthop;
        ulong           ire_type;
        ulong           ire_proto;
        ulong           ire_age;
        ulong           ire_mask;
        ulong           ire_metric5;
        void            *ire_context;
} IPRouteEntry;

typedef struct IPRouteEntry95 {
        ulong           ire_dest;
        ulong           ire_index;
        ulong           ire_metric1;
        ulong           ire_metric2;
        ulong           ire_metric3;
        ulong           ire_metric4;
        ulong           ire_nexthop;
        ulong           ire_type;
        ulong           ire_proto;
        ulong           ire_age;
        ulong           ire_mask;
        ulong           ire_metric5;
} IPRouteEntry95;

typedef struct AddrXlatInfo {
        ulong           axi_count;
        ulong           axi_index;
} AddrXlatInfo;

#define IRE_TYPE_OTHER          1
#define IRE_TYPE_INVALID        2
#define IRE_TYPE_DIRECT         3
#define IRE_TYPE_INDIRECT       4

#define IRE_PROTO_OTHER         1
#define IRE_PROTO_LOCAL         2
#define IRE_PROTO_NETMGMT       3
#define IRE_PROTO_ICMP          4
#define IRE_PROTO_EGP           5
#define IRE_PROTO_GGP           6
#define IRE_PROTO_HELLO         7
#define IRE_PROTO_RIP           8
#define IRE_PROTO_IS_IS         9
#define IRE_PROTO_ES_IS         10
#define IRE_PROTO_CISCO         11
#define IRE_PROTO_BBN           12
#define IRE_PROTO_OSPF          13
#define IRE_PROTO_BGP           14

#define IRE_METRIC_UNUSED       0xffffffff

#define IP_MIB_STATS_ID                                 1
#define ICMP_MIB_STATS_ID                               1

#define AT_MIB_ADDRXLAT_INFO_ID                 1
#define AT_MIB_ADDRXLAT_ENTRY_ID                0x101

#define IP_MIB_RTTABLE_ENTRY_ID                 0x101
#define IP_MIB_ADDRTABLE_ENTRY_ID               0x102

#define IP_INTFC_FLAG_P2P   1

typedef struct IPInterfaceInfo {
    ulong       iii_flags;
    ulong       iii_mtu;
    ulong       iii_speed;
    ulong       iii_addrlength;
    uchar       iii_addr[1];
} IPInterfaceInfo;

#define IP_INTFC_INFO_ID                0x103

#endif // IPINFO_INCLUDED
