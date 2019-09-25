/*
 * fltdefs.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Timo Kreuzer.
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

#ifndef _FLTDEFS_H
#define _FLTDEFS_H

#pragma once

#ifdef __cplusplus
#define EXTERNCDECL EXTERN_C
#else
#define EXTERNCDECL
#endif

#ifdef _M_CEE_PURE
#define PFEXPORT
#else
#define PFEXPORT /* __declspec(dllexport) Native headers have this, but this breaks exports with GCC! */
#endif

#define PFAPIENTRY EXTERNCDECL DWORD PFEXPORT WINAPI

#define ERROR_BASE 23000
#define PFERROR_NO_PF_INTERFACE    (ERROR_BASE + 0)
#define PFERROR_NO_FILTERS_GIVEN   (ERROR_BASE + 1)
#define PFERROR_BUFFER_TOO_SMALL   (ERROR_BASE + 2)
#define ERROR_IPV6_NOT_IMPLEMENTED (ERROR_BASE + 3)

#define FD_FLAGS_NOSYN        0x1
#define FD_FLAGS_ALLFLAGS     FD_FLAGS_NOSYN

#define FILTER_PROTO(ProtoId)  MAKELONG(MAKEWORD((ProtoId), 0x00), 0x00000)
#define FILTER_PROTO_ANY       FILTER_PROTO(0x00)
#define FILTER_PROTO_ICMP      FILTER_PROTO(0x01)
#define FILTER_PROTO_TCP       FILTER_PROTO(0x06)
#define FILTER_PROTO_UDP       FILTER_PROTO(0x11)

#define FILTER_TCPUDP_PORT_ANY ((WORD)0x0000)
#define FILTER_ICMP_TYPE_ANY   ((BYTE)0xff)
#define FILTER_ICMP_CODE_ANY   ((BYTE)0xff)

#define LB_SRC_ADDR_USE_SRCADDR_FLAG 0x00000001
#define LB_SRC_ADDR_USE_DSTADDR_FLAG 0x00000002
#define LB_DST_ADDR_USE_SRCADDR_FLAG 0x00000004
#define LB_DST_ADDR_USE_DSTADDR_FLAG 0x00000008
#define LB_SRC_MASK_LATE_FLAG        0x00000010
#define LB_DST_MASK_LATE_FLAG        0x00000020

typedef PVOID FILTER_HANDLE, * PFILTER_HANDLE;
typedef PVOID INTERFACE_HANDLE, * PINTERFACE_HANDLE;

typedef enum _GlobalFilter
{
    GF_FRAGMENTS = 2,
    GF_STRONGHOST = 8,
    GF_FRAGCACHE = 9
} GLOBAL_FILTER, *PGLOBAL_FILTER;

typedef enum _PfAddresType
{
    PF_IPV4,
    PF_IPV6
} PFADDRESSTYPE, *PPFADDRESSTYPE;

typedef enum _PfForwardAction
{
    PF_ACTION_FORWARD = 0,
    PF_ACTION_DROP
} PFFORWARD_ACTION, *PPFFORWARD_ACTION;

typedef enum _PfFrameType
{
    PFFT_FILTER = 1,
    PFFT_FRAG = 2,
    PFFT_SPOOF = 3
} PFFRAMETYPE, *PPFFRAMETYPE;

typedef struct _pfLogFrame
{
    LARGE_INTEGER Timestamp;
    PFFRAMETYPE pfeTypeOfFrame;
    DWORD dwTotalSizeUsed;
    DWORD dwFilterRule;
    WORD wSizeOfAdditionalData;
    WORD wSizeOfIpHeader;
    DWORD dwInterfaceName;
    DWORD dwIPIndex;
    BYTE bPacketData[1];
} PFLOGFRAME, *PPFLOGFRAME;

typedef struct _PF_FILTER_DESCRIPTOR
{
    DWORD dwFilterFlags;
    DWORD dwRule;
    PFADDRESSTYPE pfatType;
    PBYTE SrcAddr;
    PBYTE SrcMask;
    PBYTE DstAddr;
    PBYTE DstMask;
    DWORD dwProtocol;
    DWORD fLateBound;
    WORD wSrcPort;
    WORD wDstPort;
    WORD wSrcPortHighRange;
    WORD wDstPortHighRange;
} PF_FILTER_DESCRIPTOR, *PPF_FILTER_DESCRIPTOR;

#define FILTERSIZE (sizeof(PF_FILTER_DESCRIPTOR) - (DWORD)FIELD_OFFSET(PF_FILTER_DESCRIPTOR, SrcAddr))

typedef struct _PF_FILTER_STATS
{
    DWORD dwNumPacketsFiltered;
    PF_FILTER_DESCRIPTOR info;
} PF_FILTER_STATS, *PPF_FILTER_STATS;

typedef struct _PF_INTERFACE_STATS
{
    PVOID pvDriverContext;
    DWORD dwFlags;
    DWORD dwInDrops;
    DWORD dwOutDrops;
    PFFORWARD_ACTION eaInAction;
    PFFORWARD_ACTION eaOutAction;
    DWORD dwNumInFilters;
    DWORD dwNumOutFilters;
    DWORD dwFrag;
    DWORD dwSpoof;
    DWORD dwReserved1;
    DWORD dwReserved2;
    LARGE_INTEGER liSYN;
    LARGE_INTEGER liTotalLogged;
    DWORD dwLostLogEntries;
    PF_FILTER_STATS FilterInfo[1];
} PF_INTERFACE_STATS, *PPF_INTERFACE_STATS;

typedef struct _PF_LATEBIND_INFO
{
    PBYTE SrcAddr;
    PBYTE DstAddr;
    PBYTE Mask;
} PF_LATEBIND_INFO, *PPF_LATEBIND_INFO;

PFAPIENTRY
PfAddFiltersToInterface(
    INTERFACE_HANDLE ih,
    DWORD cInFilters,
    PPF_FILTER_DESCRIPTOR pfiltIn,
    DWORD cOutFilters,
    PPF_FILTER_DESCRIPTOR pfiltOut,
    PFILTER_HANDLE pfHandle);

PFAPIENTRY
PfAddGlobalFilterToInterface(
    INTERFACE_HANDLE pInterface,
    GLOBAL_FILTER gfFilter);

PFAPIENTRY
PfBindInterfaceToIPAddress(
    INTERFACE_HANDLE pInterface,
    PFADDRESSTYPE pfatType,
    PBYTE IPAddress);

PFAPIENTRY
PfBindInterfaceToIndex(
    INTERFACE_HANDLE pInterface,
    DWORD dwIndex,
    PFADDRESSTYPE pfatLinkType,
    PBYTE LinkIPAddress);

PFAPIENTRY
PfCreateInterface(
    DWORD dwName,
    PFFORWARD_ACTION inAction,
    PFFORWARD_ACTION outAction,
    BOOL bUseLog,
    BOOL bMustBeUnique,
    INTERFACE_HANDLE* ppInterface);

PFAPIENTRY
PfDeleteInterface(
    INTERFACE_HANDLE pInterface);

PFAPIENTRY
PfDeleteLog(
    VOID);

PFAPIENTRY
PfGetInterfaceStatistics(
    INTERFACE_HANDLE pInterface,
    PPF_INTERFACE_STATS ppfStats,
    PDWORD pdwBufferSize,
    BOOL fResetCounters);

PFAPIENTRY
PfMakeLog(
    HANDLE hEvent);

PFAPIENTRY
PfRebindFilters(
    INTERFACE_HANDLE pInterface,
    PPF_LATEBIND_INFO pLateBindInfo);

PFAPIENTRY
PfRemoveFilterHandles(
    INTERFACE_HANDLE pInterface,
    DWORD cFilters,
    PFILTER_HANDLE pvHandles);

PFAPIENTRY
PfRemoveFiltersFromInterface(
    INTERFACE_HANDLE ih,
    DWORD cInFilters,
    PPF_FILTER_DESCRIPTOR pfiltIn,
    DWORD cOutFilters,
    PPF_FILTER_DESCRIPTOR pfiltOut);

PFAPIENTRY
PfRemoveGlobalFilterFromInterface(
    INTERFACE_HANDLE pInterface,
    GLOBAL_FILTER gfFilter);

PFAPIENTRY
PfSetLogBuffer(
    PBYTE pbBuffer,
    DWORD dwSize,
    DWORD dwThreshold,
    DWORD dwEntries,
    PDWORD pdwLoggedEntries,
    PDWORD pdwLostEntries,
    PDWORD pdwSizeUsed);

PFAPIENTRY
PfTestPacket(
    INTERFACE_HANDLE pInInterface  OPTIONAL,
    INTERFACE_HANDLE pOutInterface OPTIONAL,
    DWORD cBytes,
    PBYTE pbPacket,
    PPFFORWARD_ACTION ppAction);

PFAPIENTRY
PfUnBindInterface(
    INTERFACE_HANDLE pInterface);

#endif // _FLTDEFS_H
