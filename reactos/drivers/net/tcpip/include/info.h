/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/info.h
 * PURPOSE:     TdiQueryInformation definitions
 */
#ifndef __INFO_H
#define __INFO_H


typedef struct IPSNMP_INFO {
	ULONG Forwarding;
	ULONG DefaultTTL;
	ULONG InReceives;
	ULONG InHdrErrors;
	ULONG InAddrErrors;
	ULONG ForwDatagrams;
	ULONG InUnknownProtos;
	ULONG InDiscards;
	ULONG InDelivers;
	ULONG OutRequests;
	ULONG RoutingDiscards;
	ULONG OutDiscards;
	ULONG OutNoRoutes;
	ULONG ReasmTimeout;
	ULONG ReasmReqds;
	ULONG ReasmOks;
	ULONG ReasmFails;
	ULONG FragOks;
	ULONG FragFails;
	ULONG FragCreates;
	ULONG NumIf;
	ULONG NumAddr;
	ULONG NumRoutes;
} IPSNMP_INFO, *PIPSNMP_INFO;

typedef struct IPADDR_ENTRY {
	ULONG  Addr;
	ULONG  Index;
	ULONG  Mask;
	ULONG  BcastAddr;
	ULONG  ReasmSize;
	USHORT Context;
	USHORT Pad;
} IPADDR_ENTRY, *PIPADDR_ENTRY;

#define	IP_MIB_STATS_ID           1
#define	IP_MIB_ADDRTABLE_ENTRY_ID 0x102

#define	MAX_PHYSADDR_SIZE 8


/* Only UDP is supported */
#define TDI_SERVICE_FLAGS (TDI_SERVICE_CONNECTIONLESS_MODE | \
                           TDI_SERVICE_BROADCAST_SUPPORTED)

#define TCP_MIB_STAT_ID     1
#define UDP_MIB_STAT_ID     1
#define TCP_MIB_TABLE_ID    0x101
#define UDP_MIB_TABLE_ID    0x101

#define TL_INSTANCE 0


typedef struct ADDRESS_INFO {
    ULONG LocalAddress;
    ULONG LocalPort;
} ADDRESS_INFO, *PADDRESS_INFO;

typedef union TDI_INFO {
    TDI_CONNECTION_INFO ConnInfo;
    TDI_ADDRESS_INFO AddrInfo;
    TDI_PROVIDER_INFO ProviderInfo;
    TDI_PROVIDER_STATISTICS ProviderStats;
} TDI_INFO, *PTDI_INFO;


TDI_STATUS InfoTdiQueryInformationEx(
    PTDI_REQUEST Request,
    TDIObjectID *ID,
    PNDIS_BUFFER Buffer,
    PUINT BufferSize,
    PVOID Context);

TDI_STATUS InfoTdiSetInformationEx(
    PTDI_REQUEST Request,
    TDIObjectID *ID,
    PVOID Buffer,
    UINT BufferSize);

#endif /* __INFO_H */

/* EOF */
