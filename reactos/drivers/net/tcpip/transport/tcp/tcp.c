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
#include <routines.h>


static BOOLEAN TCPInitialized = FALSE;


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
  PNDIS_BUFFER NdisBuffer;
  NDIS_STATUS NdisStatus;
  PVOID DataBuffer;
  ULONG Size;

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

  /* Allocate NDIS buffer */

  Size = sizeof(IPv4_HEADER);
  DataBuffer = ExAllocatePool(NonPagedPool, Size);
  if (!DataBuffer) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  NdisAllocateBuffer(&NdisStatus, &NdisBuffer, GlobalBufferPool,
    DataBuffer, Size);
  if (NdisStatus != NDIS_STATUS_SUCCESS) {
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    ExFreePool(Connection->RemoteAddress);
    return NdisStatus;
  }

  /* Issue SYN segment */

  Status = TCPBuildAndTransmitSendRequest(
    Connection,                   /* Connection endpoint */
    Request->RequestNotifyObject, /* Completion routine */
    Request->RequestContext,      /* Completion routine context */
    NdisBuffer,                   /* Buffer */
    0,                            /* Size of buffer */
    SRF_SYN);                     /* Protocol specific flags */
  if (!NT_SUCCESS(Status)) {
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    ExFreePool(Connection->RemoteAddress);
    return Status;
  }

  /* Free the NDIS buffer */
  NdisFreeBuffer(NdisBuffer);
  ExFreePool(DataBuffer);

  KeReleaseSpinLock(&Connection->Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X)\n", Status));

  return Status;
}


NTSTATUS TCPListen(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo)
/*
 * FUNCTION: Start listening for a connection from a remote peer
 * ARGUMENTS:
 *     Request    = Pointer to TDI request
 *     ConnInfo   = Pointer to connection information
 *     ReturnInfo = Pointer to structure for return information
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the high level interface for listening for connections from remote peers
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
    /* The connection has already been opened so return unsuccessful */
    KeReleaseSpinLock(&Connection->Lock, OldIrql);
    return STATUS_UNSUCCESSFUL;
  }

  Connection->LocalAddress = Connection->AddressFile->ADE->Address;
  Connection->LocalPort    = Connection->AddressFile->Port;

  TI_DbgPrint(MIN_TRACE, ("Connection->LocalAddress (%s).\n", A2S(Connection->LocalAddress)));
  TI_DbgPrint(MIN_TRACE, ("Connection->LocalPort (%d).\n", Connection->LocalPort));

  /* Start listening for connection requests */
  Connection->State = ctListen;

  KeReleaseSpinLock(&Connection->Lock, OldIrql);

  Status = STATUS_PENDING;

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


static VOID TCPiReceive(
  PADDRESS_FILE AddrFile,
  PIP_PACKET IPPacket,
  PTCP_HEADER TCPHeader)
{
  register CONNECTION_STATE State;

  if (AddrFile->Connection == NULL || AddrFile->Connection->State == ctClosed)
    {
      if ((TCPHeader->Flags & TCP_RST) == 0)
        {
          /* FIXME: Send RST
           * If the ACK bit is off, sequence number zero is used,
           *
           * <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>
           *
           * If the ACK bit is on,
           *
           * <SEQ=SEG.ACK><CTL=RST>
           */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
        }
      return;
    }

  if (AddrFile->Connection->State == ctListen)
    {
      if ((TCPHeader->Flags & TCP_RST) > 0)
        {
          /* Discard */
          return;
        }

      if ((TCPHeader->Flags & TCP_ACK) > 0)
        {
          /* FIXME: Send RST
             <SEQ=SEG.ACK><CTL=RST> */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
          return;
        }

      if ((TCPHeader->Flags & TCP_SYN) > 0)
        {
          /* FIXME: If the SEG.PRC is greater than the TCB.PRC then if allowed by
             the user and the system set TCB.PRC<-SEG.PRC, if not allowed
             send a reset and return. */
          if (FALSE)
            {
              /* FIXME: Send RST
               * <SEQ=SEG.ACK><CTL=RST>
               */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
              return;
            }

           /* Set RCV.NXT to SEG.SEQ+1, IRS is set to SEG.SEQ and any other
              control or text should be queued for processing later.  ISS
              should be selected and a SYN segment sent of the form:

                <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>

              SND.NXT is set to ISS+1 and SND.UNA to ISS.  The connection
              state should be changed to SYN-RECEIVED.  Note that any other
              incoming control or data (combined with SYN) will be processed
              in the SYN-RECEIVED state, but processing of SYN and ACK should
              not be repeated.  If the listen was not fully specified (i.e.,
              the foreign socket was not fully specified), then the
              unspecified fields should be filled in now.
            */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Go to ctSynReceived connection state.\n"));
          return;
        }

      /* Discard the segment as it is invalid */
      return;
    }

  if (AddrFile->Connection->State == ctSynSent)
    {
      if ((TCPHeader->Flags & TCP_ACK) > 0)
        {
          /* FIXME: If SEG.ACK =< ISS, or SEG.ACK > SND.NXT, send a reset (unless
             the RST bit is set, if so drop the segment and return)
             <SEQ=SEG.ACK><CTL=RST> */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
          return;
        }

      /* FIXME: If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable. */

      if ((TCPHeader->Flags & TCP_RST) > 0)
        {
          if (TRUE /* ACK is acceptable */)
            {
              AddrFile->Connection->State = ctClosed;
              /* FIXME: Signal client */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Signal client.\n"));
            }
          else
            {
              /* Discard segment */
            }
          return;
        }

      /* FIXME: If the security/compartment in the segment does not exactly
         match the security/compartment in the TCB */
      if (FALSE)
        {
          if ((TCPHeader->Flags & TCP_ACK) > 0)
            {
              /* FIXME: Send RST
                 <SEQ=SEG.ACK><CTL=RST> */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
            }
          else
            {
              /* FIXME: Send RST
                 <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK> */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
            }
          return;
        }

      if ((TCPHeader->Flags & TCP_ACK) > 0)
        {
          /* FIXME: If the precedence in the segment does not match the precedence in the TCB */
          if (FALSE)
            {
              /* FIXME: Send RST
                 <SEQ=SEG.ACK><CTL=RST> */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
              return;
            }
          else
            {
              /* FIXME: If the precedence in the segment is higher than the precedence
                 in the TCB then if allowed by the user and the system raise
                 the precedence in the TCB to that in the segment, if not
                 allowed to raise the prec then send a reset. */
              if (FALSE)
                {
                  /* FIXME: Send RST
                     <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK> */
                  TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
                  return;
                }
              else
                {
                  /* Continue */
                }
            }
          return;
        }

      /* The ACK is ok, or there is no ACK, and it the segment did not contain a RST */

      if ((TCPHeader->Flags & TCP_SYN) > 0)
        {
          /* FIXME: The security/compartment and precedence are acceptable */
          if (TRUE)
            {
              /* FIXME: RCV.NXT is set to SEG.SEQ+1, IRS is set to
                 SEG.SEQ.  SND.UNA should be advanced to equal SEG.ACK (if there
                 is an ACK), and any segments on the retransmission queue which
                 are thereby acknowledged should be removed.

                 If SND.UNA > ISS (our SYN has been ACKed), change the connection
                 state to ESTABLISHED, form an ACK segment

                   <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

                 Data or controls which were queued for
                 transmission may be included.  If there are other controls or
                 text in the segment then continue processing at the sixth step
                 below where the URG bit is checked, otherwise return.

                 Otherwise enter SYN-RECEIVED, form a SYN,ACK segment

                    <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>

                 and send it.  If there are other controls or text in the
                 segment, queue them for processing after the ESTABLISHED state
                 has been reached, return. */

              TI_DbgPrint(MIN_TRACE, ("FIXME: Maybe go to ctEstablished connection state.\n"));
            }
          else
            {
              /* FIXME: What happens here? */
            }
        }

      /* FIXME: Send RST
          <SEQ=SEG.ACK><CTL=RST> */
      TI_DbgPrint(MIN_TRACE, ("FIXME: Send RST.\n"));
      return;
    }

  State = AddrFile->Connection->State;
  if (State == ctSynReceived
    || State == ctEstablished
    || State == ctFinWait1
    || State == ctFinWait2
    || State == ctCloseWait
    || State == ctClosing
    || State == ctLastAck
    || State == ctTimeWait)
    {
      /* Segments are processed in sequence.  Initial tests on arrival
          are used to discard old duplicates, but further processing is
          done in SEG.SEQ order.  If a segment's contents straddle the
          boundary between old and new, only the new parts should be
          processed.

          There are four cases for the acceptability test for an incoming
          segment:

          Segment Receive  Test
          Length  Window
          ------- -------  -------------------------------------------

            0       0     SEG.SEQ = RCV.NXT

            0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND

            >0       0     not acceptable

            >0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
                        or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND

          If the RCV.WND is zero, no segments will be acceptable, but
          special allowance should be made to accept valid ACKs, URGs and
          RSTs.

          If an incoming segment is not acceptable, an acknowledgment
          should be sent in reply (unless the RST bit is set, if so drop
          the segment and return):

            <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

          After sending the acknowledgment, drop the unacceptable segment
          and return. */

      if ((TCPHeader->Flags & TCP_RST) > 0)
        {
          if (AddrFile->Connection->State == ctSynReceived)
            {
              /* FIXME: If this connection was initiated with a passive OPEN (i.e.,
                  came from the LISTEN state), then return this connection to
                  LISTEN state and return.  The user need not be informed.  If
                  this connection was initiated with an active OPEN (i.e., came
                  from SYN-SENT state) then the connection was refused, signal
                  the user "connection refused".  In either case, all segments
                  on the retransmission queue should be removed.  And in the
                  active OPEN case, enter the CLOSED state and delete the TCB,
                  and return. */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Maybe go to ctListen or ctClosed connection state.\n"));
              return;
            }

          State = AddrFile->Connection->State;
          if (State == ctEstablished
            || State == ctFinWait1
            || State == ctFinWait2
            || State == ctCloseWait)
            {
              /* FIXME: any outstanding RECEIVEs and SEND
                  should receive "reset" responses.  All segment queues should be
                  flushed.  Users should also receive an unsolicited general
                  "connection reset" signal.  Enter the CLOSED state, delete the
                  TCB, and return. */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Go to ctClosed connection state.\n"));
              return;
            }

          State = AddrFile->Connection->State;
          if (State == ctClosing
            || State == ctLastAck
            || State == ctTimeWait)
            {
              AddrFile->Connection->State = ctClosed;
              return;
            }

        }

      /* FIXME: check security and precedence */

      if (AddrFile->Connection->State == ctSynReceived)
        {
          /* FIXME: If the security/compartment and precedence in the segment do not
             exactly match the security/compartment and precedence in the TCB
             then send a reset, and return. */
        }

      if (AddrFile->Connection->State == ctSynReceived)
        {
          /* FIXME: If the security/compartment and precedence in the segment do not
              exactly match the security/compartment and precedence in the TCB
              then send a reset, any outstanding RECEIVEs and SEND should
              receive "reset" responses.  All segment queues should be
              flushed.  Users should also receive an unsolicited general
              "connection reset" signal.  Enter the CLOSED state, delete the
              TCB, and return. */
        }

      /* Note the previous check is placed following the sequence check to prevent
          a segment from an old connection between these ports with a
          different security or precedence from causing an abort of the
          current connection. */

      if ((TCPHeader->Flags & TCP_SYN) > 0)
        {
          State = AddrFile->Connection->State;
          if (State == ctSynReceived
            || State == ctEstablished
            || State == ctFinWait1
            || State == ctFinWait2
            || State == ctCloseWait
            || State == ctClosing
            || State == ctLastAck
            || State == ctTimeWait)
            {
              /* FIXME: If the SYN is in the window it is an error, send a reset, any
                  outstanding RECEIVEs and SEND should receive "reset" responses,
                  all segment queues should be flushed, the user should also
                  receive an unsolicited general "connection reset" signal, enter
                  the CLOSED state, delete the TCB, and return.

                  If the SYN is not in the window this step would not be reached
                  and an ack would have been sent in the first step (sequence
                  number check). */

              TI_DbgPrint(MIN_TRACE, ("FIXME: Maybe go to ctClosed connection state.\n"));
              return;
            }
        }

      if ((TCPHeader->Flags & TCP_ACK) == 0)
        {
          /* Discard the segment */
          return;
        }

      if (AddrFile->Connection->State == ctSynReceived)
        {
          /* FIXME: If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
              and continue processing.

                If the segment acknowledgment is not acceptable, form a
                reset segment,

                  <SEQ=SEG.ACK><CTL=RST>

                and send it. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Maybe go to ctEstablished connection state.\n"));
          return;
        }

      State = AddrFile->Connection->State;
      if (State == ctEstablished
        || State == ctCloseWait)
        {
          /* FIXME: If SND.UNA < SEG.ACK =< SND.NXT then, set SND.UNA <- SEG.ACK.
              Any segments on the retransmission queue which are thereby
              entirely acknowledged are removed.  Users should receive
              positive acknowledgments for buffers which have been SENT and
              fully acknowledged (i.e., SEND buffer should be returned with
              "ok" response).  If the ACK is a duplicate
              (SEG.ACK < SND.UNA), it can be ignored.  If the ACK acks
              something not yet sent (SEG.ACK > SND.NXT) then send an ACK,
              drop the segment, and return.

              If SND.UNA < SEG.ACK =< SND.NXT, the send window should be
              updated.  If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and
              SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND, set
              SND.WL1 <- SEG.SEQ, and set SND.WL2 <- SEG.ACK.

              Note that SND.WND is an offset from SND.UNA, that SND.WL1
              records the sequence number of the last segment used to update
              SND.WND, and that SND.WL2 records the acknowledgment number of
              the last segment used to update SND.WND.  The check here
              prevents using old segments to update the window. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Maybe send ACK.\n"));
          return;
        }

      if (AddrFile->Connection->State == ctFinWait1)
        {
          /* FIXME: In addition to the processing for the ESTABLISHED state, if
          our FIN is now acknowledged then enter FIN-WAIT-2 and continue
          processing in that state. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle ctFinWait1 connection state.\n"));
          return;
        }

      if (AddrFile->Connection->State == ctFinWait2)
        {
          /* FIXME: In addition to the processing for the ESTABLISHED state, if
          the retransmission queue is empty, the user's CLOSE can be
          acknowledged ("ok") but do not delete the TCB. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle ctFinWait2 connection state.\n"));
          return;
        }

      if (AddrFile->Connection->State == ctClosing)
        {
          /* FIXME: In addition to the processing for the ESTABLISHED state, if
              the ACK acknowledges our FIN then enter the TIME-WAIT state,
              otherwise ignore the segment. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle ctClosing connection state.\n"));
          return;
        }

      if (AddrFile->Connection->State == ctLastAck)
        {
          /* FIXME: The only thing that can arrive in this state is an
              acknowledgment of our FIN.  If our FIN is now acknowledged,
              delete the TCB, enter the CLOSED state, and return. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle ctLastAck connection state.\n"));
          return;
        }

      if (AddrFile->Connection->State == ctTimeWait)
        {
          /* FIXME: The only thing that can arrive in this state is a
              retransmission of the remote FIN.  Acknowledge it, and restart
              the 2 MSL timeout. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle ctTimeWait connection state.\n"));
          return;
        }

      if ((TCPHeader->Flags & TCP_URG) > 0)
        {
          State = AddrFile->Connection->State;
          if (State == ctEstablished
            || State == ctFinWait1
            || State == ctFinWait2)
            {
              /* FIXME: If the URG bit is set, RCV.UP <- max(RCV.UP,SEG.UP), and signal
                  the user that the remote side has urgent data if the urgent
                  pointer (RCV.UP) is in advance of the data consumed.  If the
                  user has already been signaled (or is still in the "urgent
                  mode") for this continuous sequence of urgent data, do not
                  signal the user again. */
              TI_DbgPrint(MIN_TRACE, ("FIXME: Handle URG flag.\n"));
              return;
            }

          State = AddrFile->Connection->State;
          if (State == ctCloseWait
            || State == ctClosing
            || State == ctLastAck
            || State == ctTimeWait)
            {
              /* This should not occur, since a FIN has been received from the
                 remote side. Ignore the URG. */
            }
        }

      State = AddrFile->Connection->State;
      if (State == ctEstablished
        || State == ctFinWait1
        || State == ctFinWait2)
        {
          /* FIXME: Once in the ESTABLISHED state, it is possible to deliver segment
              text to user RECEIVE buffers.  Text from segments can be moved
              into buffers until either the buffer is full or the segment is
              empty.  If the segment empties and carries an PUSH flag, then
              the user is informed, when the buffer is returned, that a PUSH
              has been received.

              When the TCP takes responsibility for delivering the data to the
              user it must also acknowledge the receipt of the data.

              Once the TCP takes responsibility for the data it advances
              RCV.NXT over the data accepted, and adjusts RCV.WND as
              apporopriate to the current buffer availability.  The total of
              RCV.NXT and RCV.WND should not be reduced.

              Please note the window management suggestions in section 3.7.

              Send an acknowledgment of the form:

                <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

              This acknowledgment should be piggybacked on a segment being
              transmitted if possible without incurring undue delay. */
          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle data.\n"));
          return;
        }

      State = AddrFile->Connection->State;
      if (State == ctCloseWait
        || State == ctClosing
        || State == ctLastAck
        || State == ctTimeWait)
        {
          /* This should not occur, since a FIN has been received from the
             remote side.  Ignore the segment text. */
        }

      if ((TCPHeader->Flags & TCP_FIN) > 0)
        {
          /* Do not process the FIN if the state is CLOSED, LISTEN or SYN-SENT
              since the SEG.SEQ cannot be validated; drop the segment and
              return. */
          State = AddrFile->Connection->State;
          if (State == ctClosed
            || State == ctListen
            || State == ctSynSent)
            {
              /* Discard segment */
              return;
            }

          /* FIXME: If the FIN bit is set, signal the user "connection closing" and
              return any pending RECEIVEs with same message, advance RCV.NXT
              over the FIN, and send an acknowledgment for the FIN.  Note that
              FIN implies PUSH for any segment text not yet delivered to the
              user. */

          TI_DbgPrint(MIN_TRACE, ("FIXME: Handle FIN flag.\n"));

          State = AddrFile->Connection->State;
          switch (State)
            {
              case ctSynReceived:
              case ctEstablished:
                {
                  /* FIXME: Enter ctClosed state */
                  break;
                }
              case ctFinWait1:
                {
                  /* FIXME: If our FIN has been ACKed (perhaps in this segment), then
                      enter TIME-WAIT, start the time-wait timer, turn off the other
                      timers; otherwise enter the CLOSING state. */
                  break;
                }
              case ctFinWait2:
                {
                  /* FIXME: Enter the TIME-WAIT state.  Start the time-wait timer, turn
                      off the other timers. */
                  break;
                }
              case ctCloseWait:
              case ctClosing:
              case ctLastAck:
                {
                  /* Remain in ctCloseWait, ctClosing or ctLastAck connection state */
                  break;
                }
              case ctTimeWait:
                {
                  /* Remain in ctTimeWait connection state. Restart the 2 MSL time-wait
                     timeout */
                  return;
                }
              default:
                ASSERT(FALSE);
                return;
            }
        }
      return;
    }
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
  AF_SEARCH SearchContext;
  PIPv4_HEADER IPv4Header;
  PADDRESS_FILE AddrFile;
  PTCP_HEADER TCPHeader;
  PIP_ADDRESS DstAddress;
  UINT DataSize, i;

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

  DISPLAY_TCP_PACKET(IPPacket);

  TCPHeader = (PTCP_HEADER)IPPacket->Data;

  /* FIXME: Calculate and validate TCP checksum */

  /* FIXME: Sanity checks */

    /* Locate the on destination address file object and deliver the
       packet if one is found. If no matching address file object can be
       found, drop the packet */

  AddrFile = AddrSearchFirst(DstAddress,
                             TCPHeader->DestPort,
                             IPPROTO_TCP,
                             &SearchContext);
  if (AddrFile) {
    /* There can be only one client */
    TI_DbgPrint(MID_TRACE, ("Found address file object for IPv4 TCP datagram to address (0x%X).\n",
      DN2H(DstAddress->Address.IPv4Address)));
    TCPiReceive(AddrFile, IPPacket, TCPHeader);
  } else {
    /* There are no open address files that will take this datagram */
    /* FIXME: IPv4 only */
    TI_DbgPrint(MID_TRACE, ("Cannot deliver IPv4 TCP datagram to address (0x%X).\n",
      DN2H(DstAddress->Address.IPv4Address)));

    /* FIXME: Send ICMP reply */
  }

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
