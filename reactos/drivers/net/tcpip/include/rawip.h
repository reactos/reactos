/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/rawip.h
 * PURPOSE:     Raw IP types and constants
 */
#ifndef __RAWIP_H
#define __RAWIP_H

NTSTATUS RawIPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR Buffer,
    ULONG DataSize,
    PULONG DataUsed);

VOID RawIPReceive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

NTSTATUS RawIPStartup(
    VOID);

NTSTATUS RawIPShutdown(
    VOID);

#endif /* __RAWIP_H */

/* EOF */
