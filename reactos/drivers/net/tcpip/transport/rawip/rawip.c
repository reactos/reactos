/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/rawip/rawip.c
 * PURPOSE:     Raw IP routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <datagram.h>
#include <rawip.h>
#include <pool.h>


BOOLEAN RawIPInitialized = FALSE;


NTSTATUS BuildRawIPPacket(
    PVOID Context,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET *IPPacket)
/*
 * FUNCTION: Builds an UDP packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address (NULL)
 *     LocalPort    = The port we send this datagram from (0)
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    PIP_PACKET Packet;
    NDIS_STATUS NdisStatus;
    PDATAGRAM_SEND_REQUEST SendRequest = (PDATAGRAM_SEND_REQUEST)Context;

    /* Prepare packet */
    Packet = PoolAllocateBuffer(sizeof(IP_PACKET));
    if (!Packet)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Packet, sizeof(IP_PACKET));
    Packet->RefCount   = 1;
    Packet->TotalSize  = SendRequest->BufferSize;

    /* Allocate NDIS packet */
    NdisAllocatePacket(&NdisStatus, &Packet->NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        PoolFreeBuffer(Packet);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Chain buffer to packet */
    NdisChainBufferAtFront(Packet->NdisPacket, SendRequest->Buffer);

    *IPPacket = Packet;

    return STATUS_SUCCESS;
}


NTSTATUS RawIPSendDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize)
/*
 * FUNCTION: Sends a raw IP datagram to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 * RETURNS:
 *     Status of operation
 */
{
    return DGSendDatagram(Request, ConnInfo,
        Buffer, DataSize, BuildRawIPPacket);
}


NTSTATUS RawIPStartup(
    VOID)
/*
 * FUNCTION: Initializes the Raw IP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    RawIPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS RawIPShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the Raw IP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    if (!RawIPInitialized)
        return STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

/* EOF */
