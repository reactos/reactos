/*
 * Copyright (C) 2003 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_IPMIB_H
#define __WINE_IPMIB_H

#include <ifmib.h>
#include <nldef.h>

/* Flags used in the wType field from MIB_IPADDRROW */

#define MIB_IPADDR_PRIMARY 0x0001
#define MIB_IPADDR_DYNAMIC 0x0004
#define MIB_IPADDR_DISCONNECTED 0x0008
#define MIB_IPADDR_DELETED 0x0040
#define MIB_IPADDR_TRANSIENT 0x0080

/* IPADDR table */

typedef struct _MIB_IPADDRROW
{
    DWORD          dwAddr;
    IF_INDEX       dwIndex;
    DWORD          dwMask;
    DWORD          dwBCastAddr;
    DWORD          dwReasmSize;
    unsigned short unused1;
    unsigned short wType;
} MIB_IPADDRROW, *PMIB_IPADDRROW;

typedef struct _MIB_IPADDRTABLE
{
    DWORD         dwNumEntries;
    MIB_IPADDRROW table[1];
} MIB_IPADDRTABLE, *PMIB_IPADDRTABLE;


/* IPFORWARD table */

typedef struct _MIB_IPFORWARDNUMBER
{
    DWORD dwValue;
} MIB_IPFORWARDNUMBER, *PMIB_IPFORWARDNUMBER;

typedef enum
{
    MIB_IPROUTE_TYPE_OTHER = 1,
    MIB_IPROUTE_TYPE_INVALID = 2,
    MIB_IPROUTE_TYPE_DIRECT = 3,
    MIB_IPROUTE_TYPE_INDIRECT = 4,
} MIB_IPFORWARD_TYPE;

typedef NL_ROUTE_PROTOCOL MIB_IPFORWARD_PROTO;

typedef struct _MIB_IPFORWARDROW
{
    DWORD    dwForwardDest;
    DWORD    dwForwardMask;
    DWORD    dwForwardPolicy;
    DWORD    dwForwardNextHop;
    IF_INDEX dwForwardIfIndex;
    union
    {
        DWORD              dwForwardType;
        MIB_IPFORWARD_TYPE ForwardType;
    } DUMMYUNIONNAME1;
    union
    {
        DWORD               dwForwardProto;
        MIB_IPFORWARD_PROTO ForwardProto;
    } DUMMYUNIONNAME2;
    DWORD    dwForwardAge;
    DWORD    dwForwardNextHopAS;
    DWORD    dwForwardMetric1;
    DWORD    dwForwardMetric2;
    DWORD    dwForwardMetric3;
    DWORD    dwForwardMetric4;
    DWORD    dwForwardMetric5;
} MIB_IPFORWARDROW, *PMIB_IPFORWARDROW;

typedef struct _MIB_IPFORWARDTABLE
{
    DWORD            dwNumEntries;
    MIB_IPFORWARDROW table[1];
} MIB_IPFORWARDTABLE, *PMIB_IPFORWARDTABLE;


/* IPNET table */

typedef enum
{
    MIB_IPNET_TYPE_OTHER = 1,
    MIB_IPNET_TYPE_INVALID = 2,
    MIB_IPNET_TYPE_DYNAMIC = 3,
    MIB_IPNET_TYPE_STATIC = 4,
} MIB_IPNET_TYPE;

typedef struct _MIB_IPNETROW
{
    DWORD dwIndex;
    DWORD dwPhysAddrLen;
    BYTE  bPhysAddr[MAXLEN_PHYSADDR];
    DWORD dwAddr;
    union
    {
        DWORD          dwType;
        MIB_IPNET_TYPE Type;
    } DUMMYUNIONNAME;
} MIB_IPNETROW, *PMIB_IPNETROW;

typedef struct _MIB_IPNETTABLE
{
    DWORD        dwNumEntries;
    MIB_IPNETROW table[1];
} MIB_IPNETTABLE, *PMIB_IPNETTABLE;


/* IP statistics */

typedef enum
{
    MIB_IP_FORWARDING = 1,
    MIB_IP_NOT_FORWARDING = 2,
} MIB_IPSTATS_FORWARDING, *PMIB_IPSTATS_FORWARDING;

typedef struct _MIB_IPSTATS
{
    union
    {
        DWORD                  dwForwarding;
        MIB_IPSTATS_FORWARDING Forwarding;
    } DUMMYUNIONNAME;
    DWORD dwDefaultTTL;
    DWORD dwInReceives;
    DWORD dwInHdrErrors;
    DWORD dwInAddrErrors;
    DWORD dwForwDatagrams;
    DWORD dwInUnknownProtos;
    DWORD dwInDiscards;
    DWORD dwInDelivers;
    DWORD dwOutRequests;
    DWORD dwRoutingDiscards;
    DWORD dwOutDiscards;
    DWORD dwOutNoRoutes;
    DWORD dwReasmTimeout;
    DWORD dwReasmReqds;
    DWORD dwReasmOks;
    DWORD dwReasmFails;
    DWORD dwFragOks;
    DWORD dwFragFails;
    DWORD dwFragCreates;
    DWORD dwNumIf;
    DWORD dwNumAddr;
    DWORD dwNumRoutes;
} MIB_IPSTATS, *PMIB_IPSTATS;


/* ICMP statistics */

typedef struct _MIBICMPSTATS
{
    DWORD dwMsgs;
    DWORD dwErrors;
    DWORD dwDestUnreachs;
    DWORD dwTimeExcds;
    DWORD dwParmProbs;
    DWORD dwSrcQuenchs;
    DWORD dwRedirects;
    DWORD dwEchos;
    DWORD dwEchoReps;
    DWORD dwTimestamps;
    DWORD dwTimestampReps;
    DWORD dwAddrMasks;
    DWORD dwAddrMaskReps;
} MIBICMPSTATS, *PMIBICMPSTATS;

typedef struct _MIBICMPINFO
{
    MIBICMPSTATS icmpInStats;
    MIBICMPSTATS icmpOutStats;
} MIBICMPINFO;

typedef struct _MIB_ICMP
{
    MIBICMPINFO stats;
} MIB_ICMP, *PMIB_ICMP;

typedef enum
{
    ICMP4_ECHO_REPLY        =  0,
    ICMP4_DST_UNREACH       =  3,
    ICMP4_SOURCE_QUENCH     =  4,
    ICMP4_REDIRECT          =  5,
    ICMP4_ECHO_REQUEST      =  8,
    ICMP4_ROUTER_ADVERT     =  9,
    ICMP4_ROUTER_SOLICIT    = 10,
    ICMP4_TIME_EXCEEDED     = 11,
    ICMP4_PARAM_PROB        = 12,
    ICMP4_TIMESTAMP_REQUEST = 13,
    ICMP4_TIMESTAMP_REPLY   = 14,
    ICMP4_MASK_REQUEST      = 17,
    ICMP4_MASK_REPLY        = 18,
} ICMP4_TYPE, *PICMP4_TYPE;

typedef enum
{
    ICMP6_DST_UNREACH          =   1,
    ICMP6_PACKET_TOO_BIG       =   2,
    ICMP6_TIME_EXCEEDED        =   3,
    ICMP6_PARAM_PROB           =   4,
    ICMP6_ECHO_REQUEST         = 128,
    ICMP6_ECHO_REPLY           = 129,
    ICMP6_MEMBERSHIP_QUERY     = 130,
    ICMP6_MEMBERSHIP_REPORT    = 131,
    ICMP6_MEMBERSHIP_REDUCTION = 132,
    ND_ROUTER_SOLICIT          = 133,
    ND_ROUTER_ADVERT           = 134,
    ND_NEIGHBOR_SOLICIT        = 135,
    ND_NEIGHBOR_ADVERT         = 136,
    ND_REDIRECT                = 137,
    ICMP6_V2_MEMBERSHIP_REPORT = 143,
} ICMP6_TYPE, *PICMP6_TYPE;

typedef struct _MIBICMPSTATS_EX
{
    DWORD dwMsgs;
    DWORD dwErrors;
    DWORD rgdwTypeCount[256];
} MIBICMPSTATS_EX, *PMIBICMPSTATS_EX;

typedef struct _MIB_ICMP_EX
{
  MIBICMPSTATS_EX icmpInStats;
  MIBICMPSTATS_EX icmpOutStats;
} MIB_ICMP_EX, *PMIB_ICMP_EX;

#endif /* __WINE_IPMIB_H */
