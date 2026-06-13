/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     kdnet packet transfers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kdnet.h"

KDSTATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context)
{
    FrLdrDbgPrint("KdReceivePacket called with PacketType %lu\n", PacketType);
    return KdPacketTimedOut;
}
VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
    FrLdrDbgPrint("KdSendPacket called with PacketType %lu, MessageHeader %.*s, MessageData %.*s\n",
        PacketType,
        (int)MessageHeader->Length, MessageHeader->Buffer,
        (int)MessageData->Length, MessageData->Buffer);
}
