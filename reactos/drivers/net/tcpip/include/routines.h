/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/routines.h
 * PURPOSE:     Common routine prototypes
 */
#ifndef __ROUTINES_H
#define __ROUTINES_H


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

#endif /* __ROUTINES_H */

/* EOF */
