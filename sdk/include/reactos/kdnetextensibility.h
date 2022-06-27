/*
 * PROJECT:     ReactOS Kernel Debugger over Network extension driver headers
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Provide the types needed to communicate with Kernel Debugger extensions
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#pragma once

typedef NTSTATUS
(NTAPI *KD_GET_RX_PACKET)(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle,
    _Out_ PVOID *Packet,
    _Out_ PULONG Length);

typedef VOID
(NTAPI *KD_RELEASE_RX_PACKET)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

typedef NTSTATUS
(NTAPI *KD_GET_TX_PACKET)(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle);

typedef NTSTATUS
(NTAPI *KD_SEND_TX_PACKET)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle,
    _In_ ULONG Length);

typedef PVOID
(NTAPI *KD_GET_PACKET_ADDRESS)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

typedef ULONG
(NTAPI *KD_GET_PACKET_LENGTH)(
    _In_ PVOID Adapter,
    _In_ ULONG Handle);

#define KDNET_EXT_EXPORTS 13

typedef struct _KDNET_EXTENSIBILITY_EXPORTS
{
    ULONG FunctionCount;
    PVOID KdInitializeController;
    PVOID KdShutdownController;
    PVOID KdSetHibernateRange;
    KD_GET_RX_PACKET KdGetRxPacket;
    KD_RELEASE_RX_PACKET KdReleaseRxPacket;
    KD_GET_TX_PACKET KdGetTxPacket;
    KD_SEND_TX_PACKET KdSendTxPacket;
    KD_GET_PACKET_ADDRESS KdGetPacketAddress;
    KD_GET_PACKET_LENGTH KdGetPacketLength;
    PVOID KdGetHardwareContextSize;
    PVOID KdDeviceControl;
    PVOID KdReadSerialByte;
    PVOID KdWriteSerialByte;
    PVOID DebugSerialOutputInit;
    PVOID DebugSerialOutputByte;
} KDNET_EXTENSIBILITY_EXPORTS, *PKDNET_EXTENSIBILITY_EXPORTS;

#define KDNET_EXT_IMPORTS 30

typedef struct _KDNET_EXTENSIBILITY_IMPORTS
{
    ULONG FunctionCount;
    PKDNET_EXTENSIBILITY_EXPORTS Exports;
    KDNET_GET_PCI_DATA_BY_OFFSET GetPciDataByOffset;
    KDNET_SET_PCI_DATA_BY_OFFSET SetPciDataByOffset;
    KDNET_GET_PHYSICAL_ADDRESS GetPhysicalAddress;
    KDNET_STALL_EXECUTION_PROCESSOR StallExecutionProcessor;
    KDNET_READ_REGISTER_UCHAR ReadRegisterUChar;
    KDNET_READ_REGISTER_USHORT ReadRegisterUShort;
    KDNET_READ_REGISTER_ULONG ReadRegisterULong;
    KDNET_READ_REGISTER_ULONG64 ReadRegisterULong64;
    KDNET_WRITE_REGISTER_UCHAR WriteRegisterUChar;
    KDNET_WRITE_REGISTER_USHORT WriteRegisterUShort;
    KDNET_WRITE_REGISTER_ULONG WriteRegisterULong;
    KDNET_WRITE_REGISTER_ULONG64 WriteRegisterULong64;
    KDNET_READ_PORT_UCHAR ReadPortUChar;
    KDNET_READ_PORT_USHORT ReadPortUShort;
    KDNET_READ_PORT_ULONG ReadPortULong;
    KDNET_READ_PORT_ULONG64 ReadPortULong64;
    KDNET_WRITE_PORT_UCHAR WritePortUChar;
    KDNET_WRITE_PORT_USHORT WritePortUShort;
    KDNET_WRITE_PORT_ULONG WritePortULong;
    KDNET_WRITE_PORT_ULONG64 WritePortULong64;
    KDNET_SET_HIBER_RANGE SetHiberRange;
    KDNET_BUGCHECK_EX BugCheckEx;
    KDNET_MAP_PHYSICAL_MEMORY_64 MapPhysicalMemory64;
    KDNET_UNMAP_VIRTUAL_ADDRESS UnmapVirtualAddress;
    KDNET_READ_CYCLE_COUNTER ReadCycleCounter;
    KDNET_DBGPRINT KdNetDbgPrintf;
    PVOID VmbusInitialize;       /* optional */
    PVOID GetHypervisorVendorId; /* optional */
    ULONG ExecutionEnvironment;
    NTSTATUS *KdNetErrorStatus;
    PWCHAR *KdNetErrorString;
    PULONG KdNetHardwareID;
} KDNET_EXTENSIBILITY_IMPORTS, *PKDNET_EXTENSIBILITY_IMPORTS;

extern PKDNET_EXTENSIBILITY_EXPORTS KdNetExtensibilityExports;

#define KdGetRxPacket KdNetExtensibilityExports->KdGetRxPacket
#define KdReleaseRxPacket KdNetExtensibilityExports->KdReleaseRxPacket
#define KdGetTxPacket KdNetExtensibilityExports->KdGetTxPacket
#define KdSendTxPacket KdNetExtensibilityExports->KdSendTxPacket
#define KdGetPacketAddress KdNetExtensibilityExports->KdGetPacketAddress
#define KdGetPacketLength KdNetExtensibilityExports->KdGetPacketLength

NTSTATUS
NTAPI
KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device);
