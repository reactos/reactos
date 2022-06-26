/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O functions for the freeldr debugger
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#include <freeldr.h>

static UCHAR KdTxMessageBuffer[4096];
static UCHAR KdRxMessageBuffer[4096];

static NTSTATUS
NTAPI
KdpGetRxPacketWinKD(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle,
    _Out_ PVOID *Packet,
    _Out_ PULONG Length)
{
    ULONG DataLength = 0;
    ULONG Retries;

    do
    {
        for (Retries = 100; Retries > 0; Retries--)
        {
            if (Rs232PortPollByte(&KdRxMessageBuffer[DataLength]))
            {
                DataLength++;
                break;
            }
        }
    } while (Retries > 0);

    if (DataLength == 0)
        return STATUS_IO_TIMEOUT;

    *Handle = 1;
    *Packet = KdRxMessageBuffer;
    *Length = DataLength;
    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
KdpGetRxPacketDirectOutput(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle,
    _Out_ PVOID *Packet,
    _Out_ PULONG Length)
{
    *(PULONG)(KdRxMessageBuffer + 0) = PACKET_LEADER;
    *(PUSHORT)(KdRxMessageBuffer + 4) = PACKET_TYPE_KD_ACKNOWLEDGE;
    *(PUSHORT)(KdRxMessageBuffer + 6) = 0; // ByteCount
    *(PULONG)(KdRxMessageBuffer + 8) = INITIAL_PACKET_ID; // PacketId
    *(PULONG)(KdRxMessageBuffer + 12) = 0; // CheckSum
    *(KdRxMessageBuffer + 16) = PACKET_TRAILING_BYTE;

    *Handle = 1;
    *Packet = KdRxMessageBuffer;
    *Length = 17;
    return STATUS_SUCCESS;
}

static VOID
NTAPI
KdpReleaseRxPacket(
    _In_ PVOID Adapter,
    _In_ ULONG Handle)
{
}

static NTSTATUS
NTAPI
KdpGetTxPacket(
    _In_ PVOID Adapter,
    _Out_ PULONG Handle)
{
    *Handle = 0;
    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
KdpSendTxPacketWinKD(
    _In_ PVOID Adapter,
    _In_ ULONG Handle,
    _In_ ULONG Length)
{
    PUCHAR ByteBuffer = KdTxMessageBuffer;

    while (Length-- > 0)
    {
        Rs232PortPutByte(*ByteBuffer++);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
KdpSendTxPacketDirectOutput(
    _In_ PVOID Adapter,
    _In_ ULONG Handle,
    _In_ ULONG Length)
{
    PKD_PACKET Packet = (PKD_PACKET)KdTxMessageBuffer;
    PDBGKD_DEBUG_IO DebugIo;
    PCHAR Buffer;
    ULONG BufferLength, i;

    if (Packet->PacketType != PACKET_TYPE_KD_DEBUG_IO)
        return STATUS_SUCCESS;

    DebugIo = (PDBGKD_DEBUG_IO)(Packet + 1);
    if (DebugIo->ApiNumber != DbgKdPrintStringApi)
        return STATUS_SUCCESS;

    BufferLength = Packet->ByteCount - sizeof(DBGKD_DEBUG_IO);
    Buffer = (PCHAR)(DebugIo + 1);
    for (i = 0; i < BufferLength; i++)
        DebugPrintChar(Buffer[i]);

    return STATUS_SUCCESS;
}

static PVOID
NTAPI
KdpGetPacketAddress(
    _In_ PVOID Adapter,
    _In_ ULONG Handle)
{
    if (Handle == 0)
        return KdTxMessageBuffer;
    else if (Handle == 1)
        return KdRxMessageBuffer;
    else
        return NULL;
}

static ULONG
NTAPI
KdpGetPacketLength(
    _In_ PVOID Adapter,
    _In_ ULONG Handle)
{
    if (Handle == 0)
        return sizeof(KdTxMessageBuffer);
    else if (Handle == 1)
        return sizeof(KdRxMessageBuffer);
    else
        return 0;
}

static KDNET_EXTENSIBILITY_EXPORTS KdWinKDExports = {
    KDNET_EXT_EXPORTS,
    NULL,
    NULL,
    NULL,
    KdpGetRxPacketWinKD,
    KdpReleaseRxPacket,
    KdpGetTxPacket,
    KdpSendTxPacketWinKD,
    KdpGetPacketAddress,
    KdpGetPacketLength,
};

static KDNET_EXTENSIBILITY_EXPORTS KdDirectOutputExports = {
    KDNET_EXT_EXPORTS,
    NULL,
    NULL,
    NULL,
    KdpGetRxPacketDirectOutput,
    KdpReleaseRxPacket,
    KdpGetTxPacket,
    KdpSendTxPacketDirectOutput,
    KdpGetPacketAddress,
    KdpGetPacketLength,
};

NTSTATUS
NTAPI
KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device)
{
#ifdef _WINKD_
    if (1)
#else
    if (0)
#endif
        ImportTable->Exports = &KdWinKDExports;
    else
        ImportTable->Exports = &KdDirectOutputExports;
    return STATUS_SUCCESS;
}
