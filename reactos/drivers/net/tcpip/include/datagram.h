/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/datagram.h
 * PURPOSE:     Datagram types and constants
 */
#ifndef __DATAGRAM_H
#define __DATAGRAM_H

#include <titypes.h>


VOID DGSend(
    PVOID Context,
    PDATAGRAM_SEND_REQUEST SendRequest);

VOID DGCancelSendRequest(
    PADDRESS_FILE AddrFile,
    PVOID Context);

VOID DGCancelReceiveRequest(
    PADDRESS_FILE AddrFile,
    PVOID Context);

NTSTATUS DGSendDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize,
    DATAGRAM_BUILD_ROUTINE Build);

NTSTATUS DGReceiveDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG ReceiveLength,
    ULONG ReceiveFlags,
    PTDI_CONNECTION_INFORMATION ReturnInfo,
    PULONG BytesReceived);

NTSTATUS DGStartup(
    VOID);

NTSTATUS DGShutdown(
    VOID);

#endif /* __DATAGRAM_H */

/* EOF */
