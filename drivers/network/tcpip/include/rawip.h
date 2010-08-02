/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/rawip.h
 * PURPOSE:     Raw IP types and constants
 */

#pragma once

NTSTATUS RawIPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR Buffer,
    ULONG DataSize,
    PULONG DataUsed);

VOID RawIpReceive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

NTSTATUS RawIPStartup(
    VOID);

NTSTATUS RawIPShutdown(
    VOID);

NTSTATUS AddGenericHeaderIPv4(
    PADDRESS_FILE AddrFile,
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET IPPacket,
    UINT DataLength,
    UINT Protocol,
    UINT ExtraLength,
    PVOID *NextHeader );

/* EOF */
