/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/rawip.h
 * PURPOSE:     Raw IP types and constants
 */
#ifndef __RAWIP_H
#define __RAWIP_H

NTSTATUS RawIPSendDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize);

VOID RawIPReceive(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket);

NTSTATUS RawIPStartup(
    VOID);

NTSTATUS RawIPShutdown(
    VOID);

#endif /* __RAWIP_H */

/* EOF */
