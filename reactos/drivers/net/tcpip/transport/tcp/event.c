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

int TCPBindEvent( void *ClientData,
		  void *WhichSocket, 
		  void *WhichConnection,
		  LPSOCKADDR address, OSK_UINT addrlen,
		  OSK_UINT reuseport ) {
    PCONNECTION_ENDPOINT Connection = 
	(PCONNECTION_ENDPOINT)WhichConnection;
    /* Select best interface */
    LPSOCKADDR_IN addr_in = (struct sockaddr_in *)address;
    KIRQL OldIrql;
    PNEIGHBOR_CACHE_ENTRY NCE = 0;

    KeAcquireSpinLock( &Connection->Lock, &OldIrql );

    addr_in->sin_family = htons(addr_in->sin_family);

    TI_DbgPrint(MID_TRACE,("Binding %08x (sin_family = %d)\n", address,
			   addr_in->sin_family));

    if( addr_in->sin_family != AF_INET ) return OSK_EPROTONOSUPPORT;

    NCE = RouterGetRoute(Connection->RemoteAddress, NULL);

    if( !NCE ) return OSK_EADDRNOTAVAIL;

    if( !Connection->LocalAddress ) {
	Connection->LocalAddress = ExAllocatePool( NonPagedPool,
						   sizeof( IP_ADDRESS ) );
	Connection->LocalAddress->Type = AF_INET;
	Connection->LocalPort = 1024; // XXX arty hack
    }

    if( !Connection->LocalAddress ) return OSK_ENOBUFS;

    GetInterfaceIPv4Address(NCE->Interface, 
			    ADE_UNICAST, 
			    &Connection->LocalAddress->Address.IPv4Address );

    /* XXX arty IPv4 */
    if( addr_in ) 
	memcpy( &addr_in->sin_addr, 
		&Connection->LocalAddress->Address.IPv4Address, 
		sizeof( addr_in->sin_addr ) );

    addr_in->sin_port = htons(Connection->LocalPort);
    KeReleaseSpinLock( &Connection->Lock, OldIrql );

    return 0;
}

/* Later: notify TCP that the send completed by calling tcp_output again. */
int TCPPacketSend(void *ClientData,
		  void *WhichSocket, 
		  void *WhichConnection,
		  POSKIT_TCP_STATE TcpState,
		  OSK_PCHAR data,
		  OSK_UINT len ) {
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    NTSTATUS Status;
    KIRQL OldIrql;
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)WhichConnection;

    Connection->SendNext = TcpState->SndNxt;
    Connection->ReceiveNext = TcpState->RcvNxt;

    /* Allocate NDIS buffer for maximum Link level, IP and TCP header */
    NdisAllocateBuffer(&NdisStatus,
		       &NdisBuffer,
		       GlobalBufferPool,
		       data,
		       len);

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	return STATUS_INSUFFICIENT_RESOURCES;
    }
    Track(NDIS_BUFFER_TAG, NdisBuffer);

    KeAcquireSpinLock( &Connection->Lock, &OldIrql );
    
    Status = TCPBuildAndTransmitRawSendRequest
	( Connection,
	  NULL,
	  NULL,
	  NdisBuffer,
	  len,
	  TcpState->Flags );

    KeReleaseSpinLock( &Connection->Lock, OldIrql );

    NdisFreeBuffer( NdisBuffer );
    Untrack( NdisBuffer );

    if( !NT_SUCCESS(Status) ) return OSK_EINVAL;
    else return 0;
}

