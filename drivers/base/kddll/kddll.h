/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kddll.h
 * PURPOSE:         Base definitions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

#pragma once

#define NOEXTAPI
#include <ntifs.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"

#include <wdbgexts.h>
#include <ioaccess.h> /* port intrinsics */

typedef UCHAR BYTE, *PBYTE;

typedef ULONG (*PFNDBGPRNT)(const char *Format, ...);
extern PFNDBGPRNT KdpDbgPrint;

typedef enum
{
    KDP_PACKET_RECEIVED = 0,
    KDP_PACKET_TIMEOUT = 1,
    KDP_PACKET_RESEND = 2
} KDP_STATUS;

#ifndef KDDEBUG
#define KDDBGPRINT(...)
#else
#define KDDBGPRINT KdpDbgPrint
#endif

VOID
NTAPI
KdpSendBuffer(
    IN PVOID Buffer,
    IN ULONG Size);

KDP_STATUS
NTAPI
KdpReceiveBuffer(
    OUT PVOID Buffer,
    IN  ULONG Size);

KDP_STATUS
NTAPI
KdpReceivePacketLeader(
    OUT PULONG PacketLeader);

VOID
NTAPI
KdpSendByte(IN BYTE Byte);

KDP_STATUS
NTAPI
KdpPollByte(OUT PBYTE OutByte);

KDP_STATUS
NTAPI
KdpReceiveByte(OUT PBYTE OutByte);

KDP_STATUS
NTAPI
KdpPollBreakIn();


#if 0
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL);
#endif
