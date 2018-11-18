/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/info.h
 * PURPOSE:     TdiQueryInformation definitions
 */

#pragma once

#include <tcpioctl.h>
/* FIXME */
#define DWORD ULONG
#include <in6addr.h>
#include <tcpmib.h>
#include <udpmib.h>

#define MAX_PHYSADDR_LEN 8
#define MAX_IFDESCR_LEN  256

typedef struct IPADDR_ENTRY {
	ULONG  Addr;
	ULONG  Index;
	ULONG  Mask;
	ULONG  BcastAddr;
	ULONG  ReasmSize;
	USHORT Context;
	USHORT Pad;
} IPADDR_ENTRY, *PIPADDR_ENTRY;

#define ARP_ENTRY_STATIC 4
#define ARP_ENTRY_DYNAMIC 3
#define ARP_ENTRY_INVALID 2
#define ARP_ENTRY_OTHER 1

typedef struct IPARP_ENTRY {
    ULONG Index;
    ULONG AddrSize;
    UCHAR PhysAddr[8];
    ULONG LogAddr;
    ULONG Type;
} IPARP_ENTRY, *PIPARP_ENTRY;

typedef struct IPROUTE_ENTRY {
    ULONG Dest;
    ULONG Index;    //matches if_index in IFEntry and iae_index in IPAddrEntry
    ULONG Metric1;
    ULONG Metric2;
    ULONG Metric3;
    ULONG Metric4;
    ULONG Gw;
    ULONG Type;
    ULONG Proto;
    ULONG Age;
    ULONG Mask;
    ULONG Metric5;
    ULONG Info;
} IPROUTE_ENTRY, *PIPROUTE_ENTRY;

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

TDI_STATUS InfoCopyOut( PCHAR DataOut, UINT SizeOut,
			PNDIS_BUFFER ClientBuf, PUINT ClientBufSize );

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

TDI_STATUS InfoTdiQueryGetAddrTable(TDIEntityID ID,
				    PNDIS_BUFFER Buffer,
				    PUINT BufferSize);

TDI_STATUS InfoTdiQueryGetInterfaceMIB(TDIEntityID ID,
				       PIP_INTERFACE Interface,
				       PNDIS_BUFFER Buffer,
				       PUINT BufferSize);

TDI_STATUS InfoTdiQueryGetIPSnmpInfo( TDIEntityID ID,
                                      PIP_INTERFACE IF,
				      PNDIS_BUFFER Buffer,
				      PUINT BufferSize );

TDI_STATUS InfoTdiQueryGetRouteTable( PIP_INTERFACE IF,
                                      PNDIS_BUFFER Buffer,
                                      PUINT BufferSize );

TDI_STATUS InfoTdiQueryGetConnectionTcpTable( PADDRESS_FILE AddrFile,
                                              PNDIS_BUFFER Buffer,
                                              PUINT BufferSize);

TDI_STATUS InfoTdiQueryGetConnectionUdpTable( PADDRESS_FILE AddrFile,
                                              PNDIS_BUFFER Buffer,
                                              PUINT BufferSize);

TDI_STATUS InfoTdiSetRoute(PIP_INTERFACE IF,
                           PVOID Buffer,
                           UINT BufferSize);

TDI_STATUS InfoTdiSetArptableMIB(PIP_INTERFACE IF,
                                 PVOID Buffer,
                                 UINT BufferSize);

TDI_STATUS InfoTdiQueryGetArptableMIB(TDIEntityID ID,
				      PIP_INTERFACE Interface,
				      PNDIS_BUFFER Buffer,
				      PUINT BufferSize);

TDI_STATUS SetAddressFileInfo(TDIObjectID *ID,
                              PADDRESS_FILE AddrFile,
                              PVOID Buffer,
                              UINT BufferSize);

TDI_STATUS GetAddressFileInfo(TDIObjectID *ID,
                              PADDRESS_FILE AddrFile,
                              PVOID Buffer,
                              PUINT BufferSize);

TDI_STATUS SetConnectionInfo(TDIObjectID *ID,
                             PCONNECTION_ENDPOINT Connection,
                             PVOID Buffer,
                             UINT BufferSize);

/* Insert and remove entities */
VOID InsertTDIInterfaceEntity( PIP_INTERFACE Interface );

VOID RemoveTDIInterfaceEntity( PIP_INTERFACE Interface );

VOID AddEntity(ULONG EntityType,
               PVOID Context,
               ULONG Flags);

VOID RemoveEntityByContext(PVOID Context);

/* EOF */
