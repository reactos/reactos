/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <tcp.h>
#include <pool.h>
#include <address.h>
#include <datagram.h>


BOOLEAN TCPInitialized = FALSE;


NTSTATUS TCPiAddHeaderIPv4(
  PDATAGRAM_SEND_REQUEST SendRequest,
  PIP_ADDRESS LocalAddress,
  USHORT LocalPort,
  PIP_PACKET IPPacket)
/*
 * FUNCTION: Adds an IPv4 and TCP header to an IP packet
 * ARGUMENTS:
 *     SendRequest  = Pointer to send request
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this segment from
 *     IPPacket     = Pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
  PIPv4_HEADER IPHeader;
  PTCP_HEADER TCPHeader;
  PVOID Header;
	ULONG BufferSize;
  NDIS_STATUS NdisStatus;
  PNDIS_BUFFER HeaderBuffer;

	BufferSize = MaxLLHeaderSize + sizeof(IPv4_HEADER) + sizeof(TCP_HEADER);
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
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Chain header at front of NDIS packet */
  NdisChainBufferAtFront(IPPacket->NdisPacket, HeaderBuffer);
  
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
  IPHeader->Id = 0;
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

  /* Build TCP header */
  TCPHeader = (PTCP_HEADER)((ULONG_PTR)IPHeader + sizeof(IPv4_HEADER));
  /* Port values are already big-endian values */
  TCPHeader->SourcePort = LocalPort;
  TCPHeader->DestPort   = SendRequest->RemotePort;
  TCPHeader->SeqNum     = 0;
  TCPHeader->AckNum     = 0;
  TCPHeader->DataOfs    = 0;
  TCPHeader->Flags      = SendRequest->Flags;
  TCPHeader->Window     = 0;
  /* FIXME: Calculate TCP checksum and put it in TCP header */
  TCPHeader->Checksum   = 0;
  TCPHeader->Urgent     = 0;

  return STATUS_SUCCESS;
}


NTSTATUS TCPiBuildPacket(
  PVOID Context,
  PIP_ADDRESS LocalAddress,
  USHORT LocalPort,
  PIP_PACKET *IPPacket)
/*
 * FUNCTION: Builds a TCP packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this segment from
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     The ProcotolContext field in the send request structure (pointed to
 *     by the Context field) contains a pointer to the CONNECTION_ENDPOINT
 *     structure for the connection
 */
{
  NTSTATUS Status;
  PIP_PACKET Packet;
  NDIS_STATUS NdisStatus;
  PDATAGRAM_SEND_REQUEST SendRequest = (PDATAGRAM_SEND_REQUEST)Context;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  /* Prepare packet */

  /* FIXME: Assumes IPv4*/
  Packet = IPCreatePacket(IP_ADDRESS_V4);
  if (!Packet)
    return STATUS_INSUFFICIENT_RESOURCES;

  Packet->TotalSize = sizeof(IPv4_HEADER) +
                      sizeof(TCP_HEADER)  +
                      SendRequest->BufferSize;

  /* Allocate NDIS packet */
  NdisAllocatePacket(&NdisStatus, &Packet->NdisPacket, GlobalPacketPool);
  if (NdisStatus != NDIS_STATUS_SUCCESS) {
    (*Packet->Free)(Packet);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  switch (SendRequest->RemoteAddress->Type) {
  case IP_ADDRESS_V4:
    Status = TCPiAddHeaderIPv4(SendRequest, LocalAddress, LocalPort, Packet);
    break;
  case IP_ADDRESS_V6:
    /* FIXME: Support IPv6 */
    TI_DbgPrint(MIN_TRACE, ("IPv6 TCP segments are not supported.\n"));
  default:
    Status = STATUS_UNSUCCESSFUL;
    break;
  }
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MIN_TRACE, ("Cannot add TCP header. Status (0x%X)\n", Status));
    NdisFreePacket(Packet->NdisPacket);
    (*Packet->Free)(Packet);
    return Status;
  }

  /* Chain data after header */
  NdisChainBufferAtBack(Packet->NdisPacket, SendRequest->Buffer);

  //DISPLAY_IP_PACKET(Packet);

  *IPPacket = Packet;

  return STATUS_SUCCESS;
}


VOID TCPiSendRequestComplete(
  PVOID Context,
  NTSTATUS Status,
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

  Complete        = SendRequest->Complete;
  CompleteContext = SendRequest->Context;
  ExFreePool(SendRequest);

  TI_DbgPrint(MAX_TRACE, ("Calling completion routine.\n"));

  /* Call upper level completion routine */
  (*Complete)(CompleteContext, Status, Count);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID TCPTimeout(VOID)
/*
 * FUNCTION: Transmission Control Protocol timeout handler
 * NOTES:
 *     This routine is called by IPTimeout to perform several
 *     maintainance tasks
 */
{
}


inline NTSTATUS TCPBuildSendRequest(
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
 *     Buffer        = Pointer to NDIS buffer to send
 *     BufferSize    = Size of Buffer
 *     Flags         = Protocol specific flags
 * RETURNS:
 *     Status of operation
 */
{
  PDATAGRAM_SEND_REQUEST DGSendReq;
  NTSTATUS Status;

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
    TCPiSendRequestComplete,      /* Completion function */
    *SendRequest,                 /* Context for completion function */
    TCPiBuildPacket,              /* Packet build function */
    Flags);                       /* Protocol specific flags */
  if (!NT_SUCCESS(Status)) {
    ExFreePool(*SendRequest);
    return Status;
  }

  if (DGSendRequest)
    *DGSendRequest = DGSendReq;

  return STATUS_SUCCESS;
}


inline NTSTATUS TCPBuildAndTransmitSendRequest(
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
 *     Complete      = Completion routine
 *     Context       = Pointer to context information
 *     Buffer        = Pointer to NDIS buffer to send
 *     BufferSize    = Size of Buffer
 *     Flags         = Protocol specific flags
 * RETURNS:
 *     Status of operation
 */
{
  PDATAGRAM_SEND_REQUEST DGSendRequest;
  PTCP_SEND_REQUEST TCPSendRequest;
  NTSTATUS Status;

  Status = TCPBuildSendRequest(
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


NTSTATUS TCPConnect(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo)
/*
 * FUNCTION: Attempts to connect to a remote peer
 * ARGUMENTS:
 *     Request    = Pointer to TDI request
 *     ConnInfo   = Pointer to connection information
 *     ReturnInfo = Pointer to structure for return information
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the high level interface for connecting to remote peers
 */
{
  PDATAGRAM_SEND_REQUEST DGSendRequest;
  PTCP_SEND_REQUEST TCPSendRequest;
  PCONNECTION_ENDPOINT Connection;
  LARGE_INTEGER DueTime;
  NTSTATUS Status;
  KIRQL OldIrql;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = Request->Handle.ConnectionContext;

  KeAcquireSpinLock(&Connection->Lock, &OldIrql);

  if (Connection->State != ctClosed) {
    /* The connection has already been opened so return success */
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    return STATUS_SUCCESS;
  }

  Connection->LocalAddress = Connection->AddressFile->ADE->Address;
  Connection->LocalPort    = Connection->AddressFile->Port;

  Status = AddrBuildAddress(
    (PTA_ADDRESS)ConnInfo->RemoteAddress,
    &Connection->RemoteAddress,
    &Connection->RemotePort);
  if (!NT_SUCCESS(Status)) {
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    return Status;
  }

  /* Issue SYN segment */

  Status = TCPBuildAndTransmitSendRequest(
    Connection,                   /* Connection endpoint */
    Request->RequestNotifyObject, /* Completion routine */
    Request->RequestContext,      /* Completion routine context */
    NULL,                         /* Buffer */
    0,                            /* Size of buffer */
    SRF_SYN);                     /* Protocol specific flags */
  if (!NT_SUCCESS(Status)) {
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    ExFreePool(Connection->RemoteAddress);
    return Status;
  }

  KeReleaseSpinLock(&Connection->Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X)\n", Status));

  return Status;
}


NTSTATUS TCPSendDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG DataSize)
/*
 * FUNCTION: Sends TCP data to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 * RETURNS:
 *     Status of operation
 */
{
  return STATUS_SUCCESS;
}


VOID TCPReceive(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues TCP data
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving TCP data
 */
{
  PIPv4_HEADER IPv4Header;
  PADDRESS_FILE AddrFile;
  PTCP_HEADER TCPHeader;
  PIP_ADDRESS DstAddress;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  switch (IPPacket->Type) {
  /* IPv4 packet */
  case IP_ADDRESS_V4:
    IPv4Header = IPPacket->Header;
    DstAddress = &IPPacket->DstAddr;
    break;

  /* IPv6 packet */
  case IP_ADDRESS_V6:
    TI_DbgPrint(MIN_TRACE, ("Discarded IPv6 TCP data (%i bytes).\n",
      IPPacket->TotalSize));

    /* FIXME: IPv6 is not supported */
    return;

  default:
    return;
  }

  TCPHeader = (PTCP_HEADER)IPPacket->Data;

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS TCPStartup(
  VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
  tcp_init();

  /* Register this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_TCP, TCPReceive);

  TCPInitialized = TRUE;

  return STATUS_SUCCESS;
}


NTSTATUS TCPShutdown(
  VOID)
/*
 * FUNCTION: Shuts down the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
  if (!TCPInitialized)
    return STATUS_SUCCESS;

  /* Deregister this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_TCP, NULL);

  TCPInitialized = FALSE;

  return STATUS_SUCCESS;
}

/* EOF */
