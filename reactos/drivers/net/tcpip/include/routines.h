/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/routines.h
 * PURPOSE:     Common routine prototypes
 */
#ifndef __ROUTINES_H
#define __ROUTINES_H


inline NTSTATUS BuildDatagramSendRequest(
    PDATAGRAM_SEND_REQUEST *SendRequest,
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PNDIS_BUFFER Buffer,
    DWORD BufferSize,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    DATAGRAM_BUILD_ROUTINE Build,
    ULONG Flags);

inline NTSTATUS BuildTCPSendRequest(
    PTCP_SEND_REQUEST *SendRequest,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PVOID ProtocolContext);

UINT Random(
    VOID);

UINT CopyBufferToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PUCHAR SrcData,
    UINT Length);

UINT CopyBufferChainToBuffer(
    PUCHAR DstData,
    PNDIS_BUFFER SrcBuffer,
    UINT SrcOffset,
    UINT Length);

UINT CopyPacketToBuffer(
    PUCHAR DstData,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length);

UINT CopyPacketToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length);

VOID FreeNdisPacket(
    PNDIS_PACKET Packet);

PVOID AdjustPacket(
    PNDIS_PACKET Packet,
    UINT Available,
    UINT Needed);

UINT ResizePacket(
    PNDIS_PACKET Packet,
    UINT Size);

#ifdef DBG
VOID DisplayIPPacket(
    PIP_PACKET IPPacket);
#define DISPLAY_IP_PACKET(x) DisplayIPPacket(x)
#else
#define DISPLAY_IP_PACKET(x)
#endif /* DBG */

#endif /* __ROUTINES_H */

/* EOF */
