/*
 * tdiinfo.h
 *
 * TDI set and query information interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __TDIINFO_H
#define __TDIINFO_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

#include "ntddk.h"

#ifndef TDI_SUCCESS
#define TDI_SUCCESS 0
#endif/*TDI_SUCCESS*/

typedef struct TDIEntityID {
    ULONG   tei_entity;
    ULONG   tei_instance;
} TDIEntityID;

#define	MAX_TDI_ENTITIES			512
#define INVALID_ENTITY_INSTANCE     -1
#define	GENERIC_ENTITY              0
#define	ENTITY_LIST_ID              0
#define	ENTITY_TYPE_ID              1

#define	AT_ENTITY                   0x280
#define	CL_NL_ENTITY                0x301
#define	CL_TL_ENTITY                0x401
#define	CO_NL_ENTITY                0x300
#define	CO_TL_ENTITY                0x400
#define	ER_ENTITY                   0x380
#define	IF_ENTITY                   0x200

#define	AT_ARP						0x280
#define	AT_NULL						0x282
#define	CL_TL_NBF					0x401
#define	CL_TL_UDP					0x403
#define	CL_NL_IPX					0x301
#define	CL_NL_IP					0x303
#define	CO_TL_NBF					0x400
#define	CO_TL_SPX					0x402
#define	CO_TL_TCP					0x404
#define	CO_TL_SPP					0x406
#define	ER_ICMP						0x380
#define	IF_GENERIC					0x200
#define	IF_MIB						0x202

/* ID to use for requesting an IFEntry for an interface */
#define IF_MIB_STATS_ID            1

/* ID to use for requesting an IPSNMPInfo for an interface */
#define IP_MIB_STATS_ID            1 /* Hmm.  I'm not sure I like this */

/* ID to use for requesting the route table */
#define IP_MIB_ROUTETABLE_ENTRY_ID  0x101
#define IP_MIB_ADDRTABLE_ENTRY_ID 102

/* TDIObjectID.toi_class constants */
#define	INFO_CLASS_GENERIC          0x100
#define	INFO_CLASS_PROTOCOL	        0x200
#define	INFO_CLASS_IMPLEMENTATION   0x300

/* TDIObjectID.toi_type constants */
#define	INFO_TYPE_PROVIDER          0x100
#define	INFO_TYPE_ADDRESS_OBJECT    0x200
#define	INFO_TYPE_CONNECTION        0x300

typedef struct _TDIObjectID {
    TDIEntityID	toi_entity;
    ULONG   toi_class;
	ULONG   toi_type;
	ULONG   toi_id;
} TDIObjectID;

#define MAX_PHYSADDR_SIZE           010

/* Basic interface information like from SIOCGIF* */
/* 0x5c bytes without description tail */
typedef struct _IFEntry {
    ULONG if_index;
    ULONG if_type;
    ULONG if_mtu;
    ULONG if_speed;
    ULONG if_physaddrlen;
    UCHAR if_physaddr[MAX_PHYSADDR_SIZE];
    ULONG if_adminstatus;
    ULONG if_operstatus;
    ULONG if_lastchange;
    ULONG if_inoctets;
    ULONG if_inucastpkts;
    ULONG if_innucastpkts;
    ULONG if_indiscards;
    ULONG if_inerrors;
    ULONG if_inunknownprotos;
    ULONG if_outoctets;
    ULONG if_outucastpkts;
    ULONG if_outnucastpkts;
    ULONG if_outdiscards;
    ULONG if_outerrors;
    ULONG if_outqlen;
    ULONG if_descrlen;
    UCHAR if_descr[1];
} IFEntry;

/* Control information like from /proc/sys/net/ipv4/... */
/* 0x58 bytes */
#if 1
typedef struct _IPSNMPInfo {
    ULONG ipsi_index;
    ULONG ipsi_forwarding;
    ULONG ipsi_defaultttl;
    ULONG ipsi_inreceives;
    ULONG ipsi_inhdrerrors;
    ULONG ipsi_inaddrerrors;
    ULONG ipsi_inunknownprotos;
    ULONG ipsi_indiscards;
    ULONG ipsi_indelivers;
    ULONG ipsi_outrequests;
    ULONG ipsi_routingdiscards;
    ULONG ipsi_outdiscards;
    ULONG ipsi_outnoroutes;
    ULONG ipsi_reasmtimeout;
    ULONG ipsi_reasmreqds;
    ULONG ipsi_reasmoks;
    ULONG ipsi_reasmfails;
    ULONG ipsi_fragoks;
    ULONG ipsi_fragfails;
    ULONG ipsi_fragcreates;
    ULONG ipsi_numif;
    ULONG ipsi_numaddr;
    ULONG ipsi_numroutes;
} IPSNMPInfo;
#else
typedef struct _IPSNMPInfo {
    ULONG ipsi_forwarding;
    ULONG ipsi_defaultttl;
    ULONG ipsi_inreceives;
    ULONG ipsi_inhdrerrors;
    ULONG ipsi_inaddrerrors;
    ULONG ipsi_inunknownprotos;
    ULONG ipsi_indiscards;
    ULONG ipsi_indelivers;
    ULONG ipsi_outrequests;
    ULONG ipsi_routingdiscards;
    ULONG ipsi_outdiscards;
    ULONG ipsi_outnoroutes;
    ULONG ipsi_reasmtimeout;
    ULONG ipsi_reasmreqds;
    ULONG ipsi_reasmoks;
    ULONG ipsi_reasmfails;
    ULONG ipsi_fragoks;
    ULONG ipsi_fragfails;
    ULONG ipsi_fragcreates;
    ULONG ipsi_numif;
    ULONG ipsi_numaddr;
    ULONG ipsi_numroutes;
} IPSNMPInfo;
#endif

// BEGIN ORIGINAL SOURCE INFORMATION --
// Gets ip info for the current interface list.

// Tom Sanfilippo
// tsanfilippo@earthlink.net
// December 12, 1999

// Thanks are due to Thomas F. Divine <tdivine@pcausa.com>
// for pointing out the updated wshsmple in the NT4DDK.
// The headers in that sample allowed the input parameters
// to finally be discovered.
// END ORIGINAL SOURCE INFORMATION --

typedef struct _IPRouteEntry {
    ULONG ire_addr;
    ULONG ire_index;            //matches if_index in IFEntry and iae_index in IPAddrEntry
    ULONG ire_metric;
    ULONG ire_dest;             //??
    ULONG ire_unk2;             //??
    ULONG ire_unk3;             //??
    ULONG ire_gw;
    ULONG ire_unk4;             //??
    ULONG ire_unk5;             //??
    ULONG ire_unk6;             //??
    ULONG ire_mask;
    ULONG ire_unk7;             //??
    ULONG ire_unk8;             //??
} IPRouteEntry;

typedef struct _IPAddrEntry {
    ULONG iae_addr;
    ULONG iae_index;
    ULONG iae_mask;
    ULONG iae_bcastaddr;
    ULONG iae_reasmsize;
    ULONG iae_context;
    ULONG iae_pad;
} IPAddrEntry;

#define	CONTEXT_SIZE                   16
#define MAX_ADAPTER_DESCRIPTION_LENGTH 64 /* guess */

typedef struct _TCP_REQUEST_QUERY_INFORMATION_EX {
	TDIObjectID ID;
	UCHAR       Context[CONTEXT_SIZE];
} TCP_REQUEST_QUERY_INFORMATION_EX, *PTCP_REQUEST_QUERY_INFORMATION_EX;

#if defined(_WIN64)
typedef struct _TCP_REQUEST_QUERY_INFORMATION_EX32 {
  TDIObjectID  ID;
  ULONG32  Context[CONTEXT_SIZE / sizeof(ULONG32)];
} TCP_REQUEST_QUERY_INFORMATION_EX32, *PTCP_REQUEST_QUERY_INFORMATION_EX32;
#endif /* _WIN64 */

typedef struct _TCP_REQUEST_SET_INFORMATION_EX {
	TDIObjectID ID;
	UINT        BufferSize;
	UCHAR       Buffer[1];
} TCP_REQUEST_SET_INFORMATION_EX, *PTCP_REQUEST_SET_INFORMATION_EX;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __TDIINFO_H */
