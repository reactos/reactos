/*
 * PROJECT:     ReactOS Intel KDNET Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     global header for kd_02_8086
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntifs.h>
#include <arc/arc.h>   /* DEBUG_DEVICE_DESCRIPTOR */
#include <kdnetshareddata.h>
#include <kdnetextensibility.h>

extern PKDNET_EXTENSIBILITY_IMPORTS KdNetExtensibilityImports;

#ifndef TRACE_DBG_PRINT
#define TRACE_DBG_PRINT 0
#endif

ULONG    E1000GetHardwareContextSize(VOID);
NTSTATUS E1000InitializeController(PKDNET_SHARED_DATA KdNet);
VOID     E1000ShutdownController(PVOID Adapter);
NTSTATUS E1000GetTxPacket(PVOID Adapter, PULONG Handle);
NTSTATUS E1000SendTxPacket(PVOID Adapter, ULONG Handle, ULONG Length);
NTSTATUS E1000GetRxPacket(PVOID Adapter, PULONG Handle, PVOID *Packet, PULONG Length);
VOID     E1000ReleaseRxPacket(PVOID Adapter, ULONG Handle);
PVOID    E1000GetPacketAddress(PVOID Adapter, ULONG Handle);
ULONG    E1000GetPacketLength(PVOID Adapter, ULONG Handle);
