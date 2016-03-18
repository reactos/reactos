/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 NSP
 * FILE:        include/reactos/winsock/rnr20lib.h
 * PURPOSE:     WinSock 2 NSP Header
 */

#ifndef __NSP_H
#define __NSP_H

/* DEFINES *******************************************************************/

/* Lookup Flags */
#define DONE                        0x01
#define REVERSE                     0x02
#define LOCAL                       0x04
#define IANA                        0x10
#define LOOPBACK                    0x20

/* Protocol Flags */
#define UDP                         0x01
#define TCP                         0x02
#define ATM                         0x04

/* GUID Masks */
#define NBT_MASK                    0x01
#define DNS_MASK                    0x02

/* TYPES *********************************************************************/

typedef struct _RNR_CONTEXT
{
    LIST_ENTRY ListEntry;
    HANDLE Handle;
    PDNS_BLOB CachedSaBlob;
    DWORD Signature;
    DWORD RefCount;
    DWORD Instance;
    DWORD LookupFlags;
    DWORD RnrId;
    DWORD dwNameSpace;
    DWORD RrType;
    DWORD dwControlFlags;
    DWORD UdpPort;
    DWORD TcpPort;
    DWORD ProtocolFlags;
    BLOB CachedBlob;
    GUID lpServiceClassId;
    GUID lpProviderId;
    WCHAR ServiceName[1];
} RNR_CONTEXT, *PRNR_CONTEXT;

typedef struct _RNR_TEB_DATA
{
    ULONG Foo;
} RNR_TEB_DATA, *PRNR_TEB_DATA;

/* PROTOTYPES ****************************************************************/

/*
 * proc.c
 */
BOOLEAN
WINAPI
RNRPROV_SockEnterApi(VOID);

/*
 * oldutil.c
 */
DWORD
WINAPI
GetServerAndProtocolsFromString(
    PWCHAR ServiceString,
    LPGUID ServiceType,
    PSERVENT *ReverseServent
);

DWORD
WINAPI
FetchPortFromClassInfo(
    IN DWORD Type,
    IN LPGUID Guid,
    IN LPWSASERVICECLASSINFOW ServiceClassInfo
);

PSERVENT
WSPAPI
CopyServEntry(
    IN PSERVENT Servent,
    IN OUT PULONG_PTR BufferPos,
    IN OUT PULONG BufferFreeSize,
    IN OUT PULONG BlobSize,
    IN BOOLEAN Relative
);

WORD
WINAPI
GetDnsQueryTypeFromGuid(
    IN LPGUID Guid
);

/*
 * context.c
 */
VOID
WSPAPI
RnrCtx_ListCleanup(VOID);

VOID
WSPAPI
RnrCtx_Release(PRNR_CONTEXT RnrContext);

PRNR_CONTEXT
WSPAPI
RnrCtx_Get(
    HANDLE LookupHandle,
    DWORD dwControlFlags,
    PLONG Instance
);

PRNR_CONTEXT
WSPAPI
RnrCtx_Create(
    IN HANDLE LookupHandle,
    IN LPWSTR ServiceName
);

VOID
WSPAPI
RnrCtx_DecInstance(IN PRNR_CONTEXT RnrContext);

/*
 * util.c
 */
PVOID
WSPAPI
Temp_AllocZero(IN DWORD Size);

/*
 * lookup.c
 */
PDNS_BLOB
WSPAPI
Rnr_DoHostnameLookup(IN PRNR_CONTEXT Context);

PDNS_BLOB
WSPAPI
Rnr_GetHostByAddr(IN PRNR_CONTEXT Context);

PDNS_BLOB
WSPAPI
Rnr_DoDnsLookup(IN PRNR_CONTEXT Context);

BOOLEAN
WINAPI
Rnr_CheckIfUseNbt(PRNR_CONTEXT RnrContext);

PDNS_BLOB
WINAPI
Rnr_NbtResolveAddr(IN IN_ADDR Address);

PDNS_BLOB
WINAPI
Rnr_NbtResolveName(IN LPWSTR Name);

/*
 * init.c
 */
VOID
WSPAPI
Rnr_ProcessInit(VOID);

VOID
WSPAPI
Rnr_ProcessCleanup(VOID);

BOOLEAN
WSPAPI
Rnr_ThreadInit(VOID);

VOID
WSPAPI
Rnr_ThreadCleanup(VOID);

/*
 * nsp.c
 */
VOID
WSPAPI
Nsp_GlobalCleanup(VOID);

INT
WINAPI
Dns_NSPCleanup(IN LPGUID lpProviderId);

INT
WINAPI
Dns_NSPSetService(
    IN LPGUID lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD dwControlFlags
);

INT
WINAPI
Dns_NSPInstallServiceClass(
    IN LPGUID lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
);

INT
WINAPI
Dns_NSPRemoveServiceClass(
    IN LPGUID lpProviderId,
    IN LPGUID lpServiceCallId
);

INT
WINAPI
Dns_NSPGetServiceClassInfo(
    IN LPGUID lpProviderId,
    IN OUT LPDWORD lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
);

INT 
WINAPI
Dns_NSPLookupServiceBegin(
    LPGUID lpProviderId,
    LPWSAQUERYSETW lpqsRestrictions,
    LPWSASERVICECLASSINFOW lpServiceClassInfo,
    DWORD dwControlFlags,
    LPHANDLE lphLookup
);

INT
WINAPI
Dns_NSPLookupServiceNext(
    IN HANDLE hLookup,
    IN DWORD dwControlFlags,
    IN OUT LPDWORD lpdwBufferLength,
    OUT LPWSAQUERYSETW lpqsResults
);

INT
WINAPI
Dns_NSPLookupServiceEnd(IN HANDLE hLookup);

INT 
WINAPI
Dns_NSPStartup(
    IN LPGUID lpProviderId,
    IN OUT LPNSP_ROUTINE lpsnpRoutines
);

/* Unchecked yet */
#define ATM_ADDRESS_LENGTH 20
#define WS2_INTERNAL_MAX_ALIAS 16
#define MAX_HOSTNAME_LEN 256
#define MAXADDRS 16

#endif

