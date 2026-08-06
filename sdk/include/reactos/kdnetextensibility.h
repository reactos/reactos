/*
 * PROJECT:     ReactOS Kernel Debugger over Network extension driver headers
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Provide the types needed to communicate with Kernel Debugger extensions
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <reactos/kdnetshareddata.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _DEBUG_DEVICE_DESCRIPTOR;

typedef NTSTATUS (NTAPI *KD_INITIALIZE_CONTROLLER)(_In_ PKDNET_SHARED_DATA KdNet);
typedef VOID     (NTAPI *KD_SHUTDOWN_CONTROLLER)(_In_ PVOID Adapter);
typedef VOID     (NTAPI *KD_SET_HIBERNATE_RANGE)(VOID);
typedef NTSTATUS (NTAPI *KD_DEVICE_CONTROL)(_In_ PVOID Adapter, _In_ ULONG RequestCode,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength);
typedef NTSTATUS (NTAPI *KD_GET_RX_PACKET)(_In_ PVOID Adapter, _Out_ PULONG Handle,
    _Out_ PVOID *Packet, _Out_ PULONG Length);
typedef VOID     (NTAPI *KD_RELEASE_RX_PACKET)(_In_ PVOID Adapter, _In_ ULONG Handle);
typedef NTSTATUS (NTAPI *KD_GET_TX_PACKET)(_In_ PVOID Adapter, _Out_ PULONG Handle);
typedef NTSTATUS (NTAPI *KD_SEND_TX_PACKET)(_In_ PVOID Adapter, _In_ ULONG Handle, _In_ ULONG Length);
typedef PVOID    (NTAPI *KD_GET_PACKET_ADDRESS)(_In_ PVOID Adapter, _In_ ULONG Handle);
typedef ULONG    (NTAPI *KD_GET_PACKET_LENGTH)(_In_ PVOID Adapter, _In_ ULONG Handle);
typedef ULONG    (NTAPI *KD_GET_HARDWARE_CONTEXT_SIZE)(_In_ struct _DEBUG_DEVICE_DESCRIPTOR *Device);
typedef NTSTATUS (NTAPI *KD_READ_SERIAL_BYTE)(_In_ PVOID Adapter, _Out_ PUCHAR Byte);
typedef NTSTATUS (NTAPI *KD_WRITE_SERIAL_BYTE)(_In_ PVOID Adapter, _In_ UCHAR Byte);
typedef NTSTATUS (NTAPI *DEBUG_SERIAL_OUTPUT_INIT)(_In_opt_ struct _DEBUG_DEVICE_DESCRIPTOR *pDevice,
    _Out_opt_ PPHYSICAL_ADDRESS PAddress);
typedef VOID     (NTAPI *DEBUG_SERIAL_OUTPUT_BYTE)(_In_ UCHAR byte);

#define KDNET_EXT_EXPORTS 13

typedef struct _KDNET_EXTENSIBILITY_EXPORTS
{
    ULONG FunctionCount;
    KD_INITIALIZE_CONTROLLER KdInitializeController;
    KD_SHUTDOWN_CONTROLLER   KdShutdownController;
    KD_SET_HIBERNATE_RANGE   KdSetHibernateRange;
    KD_GET_RX_PACKET         KdGetRxPacket;
    KD_RELEASE_RX_PACKET     KdReleaseRxPacket;
    KD_GET_TX_PACKET         KdGetTxPacket;
    KD_SEND_TX_PACKET        KdSendTxPacket;
    KD_GET_PACKET_ADDRESS    KdGetPacketAddress;
    KD_GET_PACKET_LENGTH     KdGetPacketLength;
    KD_GET_HARDWARE_CONTEXT_SIZE KdGetHardwareContextSize;
    KD_DEVICE_CONTROL       KdDeviceControl;
    KD_READ_SERIAL_BYTE     KdReadSerialByte;
    KD_WRITE_SERIAL_BYTE     KdWriteSerialByte;
    DEBUG_SERIAL_OUTPUT_INIT DebugSerialOutputInit;
    DEBUG_SERIAL_OUTPUT_BYTE DebugSerialOutputByte;
} KDNET_EXTENSIBILITY_EXPORTS, *PKDNET_EXTENSIBILITY_EXPORTS;

typedef PHYSICAL_ADDRESS (NTAPI *KDNET_GET_PHYSICAL_ADDRESS)(_In_ PVOID Va);
typedef VOID (NTAPI *KDNET_STALL_EXECUTION_PROCESSOR)(ULONG Microseconds);
typedef UCHAR (NTAPI *KDNET_READ_REGISTER_UCHAR)(_In_ PUCHAR Register);
typedef USHORT (NTAPI *KDNET_READ_REGISTER_USHORT)(_In_ PUSHORT Register);
typedef ULONG (NTAPI *KDNET_READ_REGISTER_ULONG)(_In_ PULONG Register);
typedef ULONG64 (NTAPI *KDNET_READ_REGISTER_ULONG64)(_In_ ULONG64 *Register);
typedef VOID (NTAPI *KDNET_WRITE_REGISTER_UCHAR)(_In_ PUCHAR Register, _In_ UCHAR Value);
typedef VOID (NTAPI *KDNET_WRITE_REGISTER_USHORT)(_In_ PUSHORT Register, _In_ USHORT Value);
typedef VOID (NTAPI *KDNET_WRITE_REGISTER_ULONG)(_In_ PULONG Register, _In_ ULONG Value);
typedef VOID (NTAPI *KDNET_WRITE_REGISTER_ULONG64)(_In_ ULONG64 *Register, _In_ ULONG64 Value);
typedef UCHAR (NTAPI *KDNET_READ_PORT_UCHAR)(_In_ PUCHAR Port);
typedef USHORT (NTAPI *KDNET_READ_PORT_USHORT)(_In_ PUSHORT Port);
typedef ULONG (NTAPI *KDNET_READ_PORT_ULONG)(_In_ PULONG Port);
typedef ULONG64 (NTAPI *KDNET_READ_PORT_ULONG64)(_In_ ULONG64 *Port);
typedef VOID (NTAPI *KDNET_WRITE_PORT_UCHAR)(_In_ PUCHAR Port, _In_ UCHAR Value);
typedef VOID (NTAPI *KDNET_WRITE_PORT_USHORT)(_In_ PUSHORT Port, _In_ USHORT Value);
typedef VOID (NTAPI *KDNET_WRITE_PORT_ULONG)(_In_ PULONG Port, _In_ ULONG Value);
typedef VOID (NTAPI *KDNET_WRITE_PORT_ULONG64)(_In_ PULONG Port, _In_ ULONG64 Value);
typedef ULONG (NTAPI *KDNET_GET_PCI_DATA_BY_OFFSET)(_In_ ULONG BusNumber, _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer, _In_ ULONG Offset, _In_ ULONG Length);
typedef ULONG (NTAPI *KDNET_SET_PCI_DATA_BY_OFFSET)(_In_ ULONG BusNumber, _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer, _In_ ULONG Offset, _In_ ULONG Length);
typedef VOID (NTAPI *KDNET_SET_HIBER_RANGE)(_In_opt_ PVOID MemoryMap, _In_ ULONG Flags,
    _In_ PVOID Address, _In_ ULONG_PTR Length, _In_ ULONG Tag);
typedef VOID (NTAPI *KDNET_BUGCHECK_EX)(_In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1, _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3, _In_ ULONG_PTR BugCheckParameter4);
typedef PVOID (NTAPI *KDNET_MAP_PHYSICAL_MEMORY_64)(_In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG NumberPages, _In_ BOOLEAN FlushCurrentTLB);
typedef VOID (NTAPI *KDNET_UNMAP_VIRTUAL_ADDRESS)(_In_ PVOID VirtualAddress,
    _In_ ULONG NumberPages, _In_ BOOLEAN FlushCurrentTLB);
typedef ULONG64 (NTAPI *KDNET_READ_CYCLE_COUNTER)(_Out_opt_ ULONG64 *Frequency);
typedef VOID (NTAPI *KDNET_DBGPRINT)(_In_ PCHAR pFmt, ...);

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
    NTSTATUS *KdNetErrorStatus;
    PWCHAR *KdNetErrorString;
    PULONG KdNetHardwareID;
} KDNET_EXTENSIBILITY_IMPORTS, *PKDNET_EXTENSIBILITY_IMPORTS;

NTSTATUS NTAPI KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ struct _DEBUG_DEVICE_DESCRIPTOR *Device);


#ifdef _KDNET_EXTENSION_MACROS_
extern PKDNET_EXTENSIBILITY_IMPORTS KdNetExtensibilityImports;

#define KdGetPciDataByOffset      KdNetExtensibilityImports->GetPciDataByOffset
#define KdSetPciDataByOffset      KdNetExtensibilityImports->SetPciDataByOffset
#define KdGetPhysicalAddress      KdNetExtensibilityImports->GetPhysicalAddress
#define KeStallExecutionProcessor KdNetExtensibilityImports->StallExecutionProcessor

#undef READ_REGISTER_UCHAR
#undef READ_REGISTER_USHORT
#undef READ_REGISTER_ULONG
#undef READ_REGISTER_ULONG64
#undef WRITE_REGISTER_UCHAR
#undef WRITE_REGISTER_USHORT
#undef WRITE_REGISTER_ULONG
#undef WRITE_REGISTER_ULONG64
#undef READ_PORT_UCHAR
#undef READ_PORT_USHORT
#undef READ_PORT_ULONG
#undef READ_PORT_ULONG64
#undef WRITE_PORT_UCHAR
#undef WRITE_PORT_USHORT
#undef WRITE_PORT_ULONG
#undef WRITE_PORT_ULONG64
#define READ_REGISTER_UCHAR       KdNetExtensibilityImports->ReadRegisterUChar
#define READ_REGISTER_USHORT      KdNetExtensibilityImports->ReadRegisterUShort
#define READ_REGISTER_ULONG       KdNetExtensibilityImports->ReadRegisterULong
#define READ_REGISTER_ULONG64     KdNetExtensibilityImports->ReadRegisterULong64
#define WRITE_REGISTER_UCHAR      KdNetExtensibilityImports->WriteRegisterUChar
#define WRITE_REGISTER_USHORT     KdNetExtensibilityImports->WriteRegisterUShort
#define WRITE_REGISTER_ULONG      KdNetExtensibilityImports->WriteRegisterULong
#define WRITE_REGISTER_ULONG64    KdNetExtensibilityImports->WriteRegisterULong64
#define READ_PORT_UCHAR           KdNetExtensibilityImports->ReadPortUChar
#define READ_PORT_USHORT          KdNetExtensibilityImports->ReadPortUShort
#define READ_PORT_ULONG           KdNetExtensibilityImports->ReadPortULong
#define READ_PORT_ULONG64         KdNetExtensibilityImports->ReadPortULong64
#define WRITE_PORT_UCHAR          KdNetExtensibilityImports->WritePortUChar
#define WRITE_PORT_USHORT         KdNetExtensibilityImports->WritePortUShort
#define WRITE_PORT_ULONG          KdNetExtensibilityImports->WritePortULong
#define WRITE_PORT_ULONG64        KdNetExtensibilityImports->WritePortULong64
#define KdReadCycleCounter        KdNetExtensibilityImports->ReadCycleCounter
#define KdSetHiberRange           KdNetExtensibilityImports->SetHiberRange
#define KdBugCheckEx              KdNetExtensibilityImports->BugCheckEx
#define KdMapPhysicalMemory64     KdNetExtensibilityImports->MapPhysicalMemory64
#define KdUnmapVirtualAddress     KdNetExtensibilityImports->UnmapVirtualAddress
#define KdNetDbgPrintf            KdNetExtensibilityImports->KdNetDbgPrintf
#define KdNetErrorStatus          (*KdNetExtensibilityImports->KdNetErrorStatus)
#define KdNetErrorString          (*KdNetExtensibilityImports->KdNetErrorString)
#define KdNetHardwareID           (*KdNetExtensibilityImports->KdNetHardwareID)

#endif /* _KDNET_EXTENSION_MACROS_ */

#ifdef __cplusplus
}
#endif
