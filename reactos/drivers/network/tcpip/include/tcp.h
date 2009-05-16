/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/tcp.h
 * PURPOSE:     Transmission Control Protocol definitions
 */
#ifndef __TCP_H
#define __TCP_H

typedef VOID
(*PTCP_COMPLETION_ROUTINE)( PVOID Context, NTSTATUS Status, ULONG Count );

/* TCPv4 header structure */
#include <pshpack1.h>
typedef struct TCPv4_HEADER {
  USHORT SourcePort;        /* Source port */
  USHORT DestinationPort;   /* Destination port */
  ULONG  SequenceNumber;    /* Sequence number */
  ULONG  AckNumber;         /* Acknowledgement number */
  UCHAR  DataOffset;        /* Data offset; 32-bit words (leftmost 4 bits) */
  UCHAR  Flags;             /* Control bits (rightmost 6 bits) */
  USHORT Window;            /* Maximum acceptable receive window */
  USHORT Checksum;          /* Checksum of segment */
  USHORT Urgent;            /* Pointer to urgent data */
} TCPv4_HEADER, *PTCPv4_HEADER;

/* TCPv4 header flags */
#define TCP_URG   0x20
#define TCP_ACK   0x10
#define TCP_PSH   0x08
#define TCP_RST   0x04
#define TCP_SYN   0x02
#define TCP_FIN   0x01


#define TCPOPT_END_OF_LIST  0x0
#define TCPOPT_NO_OPERATION 0x1
#define TCPOPT_MAX_SEG_SIZE 0x2

#define TCPOPTLEN_MAX_SEG_SIZE  0x4

/* Data offset; 32-bit words (leftmost 4 bits); convert to bytes */
#define TCP_DATA_OFFSET(DataOffset)(((DataOffset) & 0xF0) >> (4-2))


/* TCPv4 pseudo header */
typedef struct TCPv4_PSEUDO_HEADER {
  ULONG SourceAddress;      /* Source address */
  ULONG DestinationAddress; /* Destination address */
  UCHAR Zero;               /* Reserved */
  UCHAR Protocol;           /* Protocol */
  USHORT TCPLength;         /* Size of TCP segment */
} TCPv4_PSEUDO_HEADER, *PTCPv4_PSEUDO_HEADER;
#include <poppack.h>

typedef struct _SLEEPING_THREAD {
    LIST_ENTRY Entry;
    PVOID SleepToken;
    KEVENT Event;
} SLEEPING_THREAD, *PSLEEPING_THREAD;

/* Retransmission timeout constants */

/* Lower bound for retransmission timeout in TCP timer ticks */
#define TCP_MIN_RETRANSMISSION_TIMEOUT    1*1000          /* 1 tick */

/* Upper bound for retransmission timeout in TCP timer ticks */
#define TCP_MAX_RETRANSMISSION_TIMEOUT    1*60*1000       /* 1 tick */

/* Smoothing factor */
#define TCP_ALPHA_RETRANSMISSION_TIMEOUT(x)(((x)*8)/10)   /* 0.8 */

/* Delay variance factor */
#define TCP_BETA_RETRANSMISSION_TIMEOUT(x)(((x)*16)/10)   /* 1.6 */


/* Datagram/segment send request flags */

#define SRF_URG   TCP_URG
#define SRF_ACK   TCP_ACK
#define SRF_PSH   TCP_PSH
#define SRF_RST   TCP_RST
#define SRF_SYN   TCP_SYN
#define SRF_FIN   TCP_FIN

extern LONG TCP_IPIdentification;
extern LIST_ENTRY SignalledConnections;
extern LIST_ENTRY SleepingThreadsList;
extern FAST_MUTEX SleepingThreadsLock;
extern RECURSIVE_MUTEX TCPLock;

/* accept.c */
NTSTATUS TCPServiceListeningSocket( PCONNECTION_ENDPOINT Listener,
				    PCONNECTION_ENDPOINT Connection,
				    PTDI_REQUEST_KERNEL Request );
NTSTATUS TCPListen( PCONNECTION_ENDPOINT Connection, UINT Backlog );
VOID TCPAbortListenForSocket( PCONNECTION_ENDPOINT Listener,
			      PCONNECTION_ENDPOINT Connection );
NTSTATUS TCPAccept
( PTDI_REQUEST Request,
  PCONNECTION_ENDPOINT Listener,
  PCONNECTION_ENDPOINT Connection,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context );

/* tcp.c */
PCONNECTION_ENDPOINT TCPAllocateConnectionEndpoint( PVOID ClientContext );
VOID TCPFreeConnectionEndpoint( PCONNECTION_ENDPOINT Connection );

NTSTATUS TCPSocket( PCONNECTION_ENDPOINT Connection,
		    UINT Family, UINT Type, UINT Proto );

PTCP_SEGMENT TCPCreateSegment(
  PIP_PACKET IPPacket,
  PTCPv4_HEADER TCPHeader,
  ULONG SegmentLength);

VOID TCPFreeSegment(
  PTCP_SEGMENT Segment);

VOID TCPAddSegment(
  PCONNECTION_ENDPOINT Connection,
  PTCP_SEGMENT Segment,
  PULONG Acknowledged);

NTSTATUS TCPConnect(
  PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context);

NTSTATUS TCPDisconnect(
  PCONNECTION_ENDPOINT Connection,
  UINT Flags,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context);

NTSTATUS TCPReceiveData(
  PCONNECTION_ENDPOINT Connection,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  PULONG BytesReceived,
  ULONG ReceiveFlags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context);

NTSTATUS TCPSendData(
  PCONNECTION_ENDPOINT Connection,
  PCHAR Buffer,
  ULONG DataSize,
  PULONG DataUsed,
  ULONG Flags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context);

NTSTATUS TCPClose( PCONNECTION_ENDPOINT Connection );

NTSTATUS TCPTranslateError( int OskitError );

VOID TCPTimeout();

UINT TCPAllocatePort( UINT HintPort );

VOID TCPFreePort( UINT Port );

NTSTATUS TCPGetSockAddress
( PCONNECTION_ENDPOINT Connection,
  PTRANSPORT_ADDRESS TransportAddress,
  BOOLEAN RemoteAddress );

NTSTATUS TCPStartup(
  VOID);

NTSTATUS TCPShutdown(
  VOID);

VOID TCPRemoveIRP( PCONNECTION_ENDPOINT Connection, PIRP Irp );

#endif /* __TCP_H */
