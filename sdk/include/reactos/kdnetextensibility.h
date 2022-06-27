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
    PVOID Reserved[28];
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
