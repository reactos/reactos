/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol -- Events from oskittcp
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <limits.h>
#include <tcpip.h>
#include <tcp.h>
#include <pool.h>
#include <address.h>
#include <neighbor.h>
#include <datagram.h>
#include <checksum.h>
#include <routines.h>
#include <oskittcp.h>

LONG IPIdentification = 0;

NTSTATUS TCPiAddRawHeaderIPv4(
    PDATAGRAM_SEND_REQUEST SendRequest,
    PCONNECTION_ENDPOINT Connection,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET IPPacket,
    PNDIS_BUFFER PacketBody) {
/*
 * FUNCTION: Adds an IPv4 and TCP header to an IP packet
 * ARGUMENTS:
 *     SendRequest      = Pointer to send request
 *     Connection       = Pointer to connection endpoint
 *     LocalAddress     = Pointer to our local address
 *     LocalPort        = The port we send this segment from
 *     IPPacket         = Pointer to IP packet
 *     PacketBody       = NDIS_BUFFER Containing the IP payload
 * RETURNS:
 *     Status of operation
 */
    PIPv4_HEADER IPHeader;
    PVOID Header;
    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER HeaderBuffer;
    PCHAR BufferContent;
    ULONG BufferSize;
    ULONG PayloadBufferSize;
    PTCP_SEND_REQUEST TcpSendRequest = (PTCP_SEND_REQUEST)SendRequest->Context;
    
    ASSERT(SendRequest);
    ASSERT(Connection);
    ASSERT(LocalAddress);
    ASSERT(IPPacket);
    ASSERT(PacketBody);
    
    NdisQueryBuffer(PacketBody,&BufferContent,&PayloadBufferSize);
    
    BufferSize = MaxLLHeaderSize + sizeof(IPv4_HEADER) + PayloadBufferSize;
    Header     = ExAllocatePool(NonPagedPool, BufferSize);
    if (!Header)
	return STATUS_INSUFFICIENT_RESOURCES;
    
    TI_DbgPrint(MAX_TRACE, ("Allocated %d bytes for headers at 0x%X.\n", BufferSize, Header));
    
    /* Allocate NDIS buffer for maximum Link level, IP and TCP header */
    NdisAllocateBuffer(&NdisStatus,
		       &HeaderBuffer,
		       GlobalBufferPool,
		       Header,
		       BufferSize);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	ExFreePool(Header);
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	return STATUS_INSUFFICIENT_RESOURCES;
    }
    Track(NDIS_BUFFER_TAG, HeaderBuffer);
    
    /* Chain header at front of NDIS packet */
    NdisChainBufferAtFront(IPPacket->NdisPacket, HeaderBuffer);
    
    IPPacket->ContigSize = BufferSize;
    IPPacket->TotalSize  = IPPacket->ContigSize;
    IPPacket->Header     = (PVOID)((ULONG_PTR)Header + MaxLLHeaderSize);
    IPPacket->HeaderSize = 20;
    
    /* Build IPv4 header */
    IPHeader = (PIPv4_HEADER)IPPacket->Header;
    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of header and data */
    IPHeader->TotalLength = WH2N((USHORT)IPPacket->TotalSize);
    /* Identification */
    IPHeader->Id = WH2N((USHORT)InterlockedIncrement(&IPIdentification));
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = 0;
    /* Time-to-Live is 128 */
    IPHeader->Ttl = 128;
    /* Transmission Control Protocol */
    IPHeader->Protocol = IPPROTO_TCP;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = LocalAddress->Address.IPv4Address;
    /* Destination address. FIXME: IPv4 only */
    IPHeader->DstAddr = SendRequest->RemoteAddress->Address.IPv4Address;

    memcpy(&IPHeader[1],BufferContent,PayloadBufferSize);
    
    return STATUS_SUCCESS;
}

NTSTATUS TCPiBuildRawPacket(
  PVOID Context,
  PIP_ADDRESS LocalAddress,
  USHORT LocalPort,
  PIP_PACKET *IPPacket)
/*
 * FUNCTION: Builds a raw TCP packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this segment from
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     The Context field in the send request structure (pointed to
 *     by the Context field) contains a pointer to the CONNECTION_ENDPOINT
 *     structure for the connection
 */
{
  NTSTATUS Status = NDIS_STATUS_SUCCESS;
  PIP_PACKET Packet;
  NDIS_STATUS NdisStatus;
  PDATAGRAM_SEND_REQUEST SendRequest;
  PCONNECTION_ENDPOINT Connection;
  ULONG Checksum;
  TCPv4_PSEUDO_HEADER TcpPseudoHeader;
  PTCPv4_HEADER TcpHeader;
  ULONG TcpHeaderLength;
  PNDIS_BUFFER UnchainedBuffer;

  ASSERT(LocalAddress);
  ASSERT(IPPacket);

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  SendRequest = (PDATAGRAM_SEND_REQUEST)Context;
  ASSERT(SendRequest);
  Connection = (PCONNECTION_ENDPOINT)SendRequest->Context;
  ASSERT(Connection);

  /* Prepare packet */

  /* FIXME: Assumes IPv4 */
  Packet = IPCreatePacket(IP_ADDRESS_V4);
  if (Packet == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  Packet->TotalSize = sizeof(IPv4_HEADER) +
                      sizeof(TCPv4_HEADER)  +
                      SendRequest->BufferSize;

  /* Allocate NDIS packet */
  NdisAllocatePacket(&NdisStatus, &Packet->NdisPacket, GlobalPacketPool);
  if (NdisStatus != NDIS_STATUS_SUCCESS) {
      (*Packet->Free)(Packet);
      return STATUS_INSUFFICIENT_RESOURCES;
  }
  Track(NDIS_PACKET_TAG, Packet->NdisPacket);

#if 0
  switch (SendRequest->RemoteAddress->Type) {
  case IP_ADDRESS_V4:
#endif
    Status = TCPiAddRawHeaderIPv4( SendRequest, Connection, LocalAddress,
				   LocalPort, Packet, SendRequest->Buffer );
#if 0
    break;
  case IP_ADDRESS_V6:
    /* FIXME: Support IPv6 */
    TI_DbgPrint(MIN_TRACE, ("IPv6 TCP segments are not supported.\n"));
  default:
    Status = STATUS_UNSUCCESSFUL;
    break;
  }
#endif

  if (!NT_SUCCESS(Status)) {
      TI_DbgPrint(MIN_TRACE, ("Cannot add TCP header. Status (0x%X)\n", Status));
      (*Packet->Free)(Packet);
      return Status;
  }
  
  DISPLAY_TCP_PACKET(Packet);
  
  *IPPacket = Packet;

  return STATUS_SUCCESS;
}

VOID TCPiRawSendRequestComplete(
  PVOID Context,
  NDIS_STATUS Status,
  ULONG Count)
/*
 * FUNCTION: Completion routine for datagram send requests
 * ARGUMENTS:
 *     Context = Pointer to context information (TCP_SEND_REQUEST)
 *     Status  = Status of the request
 *     Count   = Number of bytes sent or received
 */
{
  DATAGRAM_COMPLETION_ROUTINE Complete;
  PVOID CompleteContext;
  PTCP_SEND_REQUEST SendRequest = (PTCP_SEND_REQUEST)Context;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  Complete        = SendRequest->Complete;
  CompleteContext = SendRequest->Context;
  ExFreePool(SendRequest);

  if (Complete != NULL)
    {
      TI_DbgPrint(MAX_TRACE, ("Calling completion routine with status (0x%.08x).\n", Status));

      /* Call upper level completion routine */
      (*Complete)(CompleteContext, Status, Count);
    }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

inline NTSTATUS TCPBuildRawSendRequest(
    PTCP_SEND_REQUEST *SendRequest,
    PDATAGRAM_SEND_REQUEST *DGSendRequest,
    PCONNECTION_ENDPOINT Connection,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PNDIS_BUFFER Buffer,
    DWORD BufferSize,
    ULONG Flags)
/*
 * FUNCTION: Allocates and intializes a TCP send request
 * ARGUMENTS:
 *     SendRequest   = TCP send request
 *     DGSendRequest = Datagram send request (optional)
 *     Connection    = Connection endpoint
 *     Complete      = Completion routine
 *     Context       = Pointer to context information
 *     Buffer        = Pointer to NDIS buffer to send (optional)
 *     BufferSize    = Size of Buffer
 *     Flags         = Protocol specific flags
 * RETURNS:
 *     Status of operation
 */
{
  PDATAGRAM_SEND_REQUEST DGSendReq;
  NTSTATUS Status;

  ASSERT(SendRequest);
  ASSERT(Connection);

  Status = BuildTCPSendRequest(
    SendRequest,
    Complete,
    Context,
    NULL);
  if (!NT_SUCCESS(Status))
    return Status;

  Status = BuildDatagramSendRequest(
    &DGSendReq,                   /* Datagram send request */
    Connection->RemoteAddress,    /* Address of remote peer */
    Connection->RemotePort,       /* Port of remote peer */
    Buffer,                       /* Buffer */
    BufferSize,                   /* Size of buffer */
    (DATAGRAM_COMPLETION_ROUTINE)
      TCPiRawSendRequestComplete, /* Completion function */
    *SendRequest,                 /* Context for completion function */
    TCPiBuildRawPacket,           /* Packet build function */
    Flags);                       /* Protocol specific flags */
  if (!NT_SUCCESS(Status)) { /* May leak an Ndis packet? */
    ExFreePool(*SendRequest);
    return Status;
  }

  if (DGSendRequest)
    *DGSendRequest = DGSendReq;

  return STATUS_SUCCESS;
}

inline NTSTATUS TCPBuildAndTransmitRawSendRequest(
    PCONNECTION_ENDPOINT Connection,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PNDIS_BUFFER Buffer,
    DWORD BufferSize,
    ULONG Flags)
/*
 * FUNCTION: Allocates and intializes a TCP send request
 * ARGUMENTS:
 *     Connection    = Connection endpoint
 *     Complete      = Completion routine (optional)
 *     Context       = Pointer to context information
 *     Buffer        = Pointer to NDIS buffer to send (optional)
 *     BufferSize    = Size of Buffer
 *     Flags         = Protocol specific flags
 * RETURNS:
 *     Status of operation
 */
{
  PDATAGRAM_SEND_REQUEST DGSendRequest;
  PTCP_SEND_REQUEST TCPSendRequest;
  NTSTATUS Status;

  ASSERT(Connection);

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  Status = TCPBuildRawSendRequest(
    &TCPSendRequest,
    &DGSendRequest,
    Connection,                   /* Connection endpoint */
    Complete,                     /* Completion routine */
    Context,                      /* Completion routine context */
    Buffer,                       /* Buffer */
    BufferSize,                   /* Size of buffer */
    Flags);                       /* Protocol specific flags */
  if (!NT_SUCCESS(Status))
    return Status;
  
  Status = DGTransmit(
    Connection->AddressFile,
    DGSendRequest);
  if (!NT_SUCCESS(Status)) {
    ExFreePool(DGSendRequest);
    ExFreePool(TCPSendRequest);
    return Status;
  }

  return STATUS_SUCCESS;
}

