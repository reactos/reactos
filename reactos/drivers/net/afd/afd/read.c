/* $Id: read.c,v 1.11 2004/11/17 05:17:22 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/read.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 *
 * Improve buffering code
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS TryToSatisfyRecvRequestFromBuffer( PAFD_FCB FCB,
					    PAFD_RECV_INFO RecvReq,
					    PUINT TotalBytesCopied ) {
    UINT i, BytesToCopy = 0,
	BytesAvailable = 
	FCB->Recv.Content - FCB->Recv.BytesUsed;
    *TotalBytesCopied = 0;
    PAFD_MAPBUF Map;

    if( !BytesAvailable ) return STATUS_PENDING;

    Map = (PAFD_MAPBUF)(RecvReq->BufferArray + RecvReq->BufferCount);

    AFD_DbgPrint(MID_TRACE,("Buffer Count: %d @ %x\n", 
			    RecvReq->BufferCount,
			    RecvReq->BufferArray));
    for( i = 0; 
	 RecvReq->BufferArray && 
	     BytesAvailable && 
	     i < RecvReq->BufferCount; 
	 i++ ) {
	BytesToCopy = 
	    MIN( RecvReq->BufferArray[i].len, BytesAvailable );

	if( Map[i].Mdl ) {
	    Map[i].BufferAddress = MmMapLockedPages( Map[i].Mdl, KernelMode );
	    
	    AFD_DbgPrint(MID_TRACE,("Buffer %d: %x:%d\n", 
				    i, 
				    Map[i].BufferAddress,
				    BytesToCopy));

	    RtlCopyMemory( Map[i].BufferAddress,
			   FCB->Recv.Window + FCB->Recv.BytesUsed,
			   BytesToCopy );
	    
	    MmUnmapLockedPages( Map[i].BufferAddress, Map[i].Mdl );
	    
	    FCB->Recv.BytesUsed += BytesToCopy;
	    *TotalBytesCopied += BytesToCopy;
	    BytesAvailable -= BytesToCopy;
	}
    }

    if( FCB->Recv.BytesUsed == FCB->Recv.Content )
	FCB->Recv.BytesUsed = FCB->Recv.Content = 0;

    return STATUS_SUCCESS;
}

NTSTATUS DDKAPI ReceiveComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    NTSTATUS Status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;
    PIO_STACK_LOCATION NextIrpSp;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0;

    AFD_DbgPrint(MID_TRACE,("Called\n"));
    
    if( !SocketAcquireStateLock( FCB ) ) return Status;

    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
	SocketStateUnlock( FCB );
	DestroySocket( FCB );
	return STATUS_SUCCESS;
    }

    /* Reset in flight request because the last has been completed */
    FCB->ReceiveIrp.InFlightRequest = NULL;
    
    if( NT_SUCCESS(Irp->IoStatus.Status) ) {
	/* Update the receive window */
	FCB->Recv.Content = Irp->IoStatus.Information;
	FCB->Recv.BytesUsed = 0;
	/* Kick the user that receive would be possible now */
	/* XXX Not implemented yet */

	AFD_DbgPrint(MID_TRACE,("FCB %x Receive data waiting %d\n",
				FCB, FCB->Recv.Content));
	/*OskitDumpBuffer( FCB->Recv.Window, FCB->Recv.Content );*/

	Status = STATUS_SUCCESS;

	/* Try to clear some requests */
	while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) &&
	       NT_SUCCESS(Status) ) {
	    NextIrpEntry = 
		RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
	    NextIrp = 
		CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	    NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
	    RecvReq = NextIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

	    AFD_DbgPrint(MID_TRACE,("RecvReq @ %x\n", RecvReq));

	    Status = TryToSatisfyRecvRequestFromBuffer
		( FCB, RecvReq, &TotalBytesCopied );

	    if( Status == STATUS_PENDING ) {
		AFD_DbgPrint(MID_TRACE,("Ran out of data for %x\n", NextIrp));
		InsertHeadList(&FCB->PendingIrpList[FUNCTION_RECV],
			       &NextIrp->Tail.Overlay.ListEntry);
		break;
	    } else {
		AFD_DbgPrint(MID_TRACE,("Completing recv %x (%d)\n", NextIrp,
					TotalBytesCopied));
		UnlockBuffers( RecvReq->BufferArray, 
			       RecvReq->BufferCount, FALSE );
		NextIrp->IoStatus.Status = Status;
		NextIrp->IoStatus.Information = TotalBytesCopied;
		IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
	    }
	}

	if( NT_SUCCESS(Status) && FCB->Recv.Window && !FCB->Recv.Content &&
	    NT_SUCCESS(Irp->IoStatus.Status) ) {
	    AFD_DbgPrint(MID_TRACE,
			 ("Exhausted our buffer.  Requesting new: %x\n", FCB));

	    SocketCalloutEnter( FCB );
	    
	    Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
				 IrpSp->FileObject,
				 TDI_RECEIVE_NORMAL,
				 FCB->Recv.Window,
				 FCB->Recv.Size,
				 &FCB->ReceiveIrp.Iosb,
				 ReceiveComplete,
				 FCB );

	    SocketCalloutLeave( FCB );
	}
    } else {
	while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
	    NextIrpEntry = 
		RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
	    NextIrp = 
		CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	    AFD_DbgPrint(MID_TRACE,("Completing recv %x (%x)\n", 
				    NextIrp, Status));
	    Irp->IoStatus.Status = Status;
	    Irp->IoStatus.Information = 0;
	    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	}
    }

    if( FCB->Recv.Content ) {
	FCB->PollState |= AFD_EVENT_RECEIVE;
	PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else
	FCB->PollState &= ~AFD_EVENT_RECEIVE;

    SocketStateUnlock( FCB );

    if( Status == STATUS_PENDING ) Status = STATUS_SUCCESS;

    AFD_DbgPrint(MID_TRACE,("Returned %x\n", Status));

    return Status;
}

NTSTATUS STDCALL
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			   PIO_STACK_LOCATION IrpSp, BOOLEAN Short) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp, FALSE );
    if( !(RecvReq = LockRequest( Irp, IrpSp )) ) 
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, 
				       Irp, 0, NULL, FALSE );

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray, 
					RecvReq->BufferCount,
					NULL, NULL,
					TRUE, FALSE );

    /* Launch a new recv request if we have no data */

    if( FCB->Recv.Window && !(FCB->Recv.Content - FCB->Recv.BytesUsed) && 
	!FCB->ReceiveIrp.InFlightRequest ) {
	FCB->Recv.Content = 0;
	FCB->Recv.BytesUsed = 0;
	AFD_DbgPrint(MID_TRACE,("Replenishing buffer\n"));

	SocketCalloutEnter( FCB );

	Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
			     FCB->Connection.Object,
			     TDI_RECEIVE_NORMAL,
			     FCB->Recv.Window,
			     FCB->Recv.Size,
			     &FCB->ReceiveIrp.Iosb,
			     ReceiveComplete,
			     FCB );

	SocketCalloutLeave( FCB );
    } else Status = STATUS_SUCCESS;

    if( NT_SUCCESS(Status) ) 
	Status = TryToSatisfyRecvRequestFromBuffer
	    ( FCB, RecvReq, &TotalBytesCopied );
    
    if( Status != STATUS_PENDING || RecvReq->AfdFlags & AFD_IMMEDIATE ) {
	if( Status == STATUS_PENDING ) {
	    AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
	    Status = STATUS_CANT_WAIT;
	    TotalBytesCopied = 0;
	}
	UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, FALSE );
	return UnlockAndMaybeComplete( FCB, Status, Irp, 
				       TotalBytesCopied, NULL, TRUE );
    } else {
	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
    }
}


NTSTATUS STDCALL
SatisfyPacketRecvRequest( PAFD_FCB FCB, PIRP Irp, 
			  PAFD_STORED_DATAGRAM DatagramRecv,
			  PUINT TotalBytesCopied ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PAFD_RECV_INFO RecvReq = 
	IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT BytesToCopy = 0, BytesAvailable = DatagramRecv->Len, AddrLen = 0;
    PAFD_MAPBUF Map;

    Map = (PAFD_MAPBUF)(RecvReq->BufferArray + 
			RecvReq->BufferCount + 
			EXTRA_LOCK_BUFFERS);

    BytesToCopy = 
	MIN( RecvReq->BufferArray[0].len, BytesAvailable );
    
    AFD_DbgPrint(MID_TRACE,("BytesToCopy: %d len %d\n", BytesToCopy,
			    RecvReq->BufferArray[0].len));
		 
    if( Map[0].Mdl ) {
	/* Copy the address */
	if( Map[1].Mdl && Map[2].Mdl ) {
	    AFD_DbgPrint(MID_TRACE,("Checking TAAddressCount\n"));

	    if( DatagramRecv->Address->TAAddressCount != 1 ) {
		AFD_DbgPrint
		    (MID_TRACE,
		     ("Wierd address count %d\n",
		      DatagramRecv->Address->TAAddressCount));
	    }

	    AFD_DbgPrint(MID_TRACE,("Computing addr len\n"));

	    AddrLen = MIN(DatagramRecv->Address->Address->AddressLength +
			  sizeof(USHORT),
			  RecvReq->BufferArray[1].len);

	    AFD_DbgPrint(MID_TRACE,("Copying %d bytes of address\n", AddrLen));

	    Map[1].BufferAddress = MmMapLockedPages( Map[1].Mdl, KernelMode );

	    AFD_DbgPrint(MID_TRACE,("Done mapping, copying address\n"));

	    RtlCopyMemory( Map[1].BufferAddress,
			   &DatagramRecv->Address->Address->AddressType,
			   AddrLen );

	    MmUnmapLockedPages( Map[1].BufferAddress, Map[1].Mdl );
	    
	    AFD_DbgPrint(MID_TRACE,("Copying address len\n"));

	    Map[2].BufferAddress = MmMapLockedPages( Map[2].Mdl, KernelMode );
	    *((PINT)Map[2].BufferAddress) = AddrLen;
	    MmUnmapLockedPages( Map[2].BufferAddress, Map[2].Mdl );
	}

	AFD_DbgPrint(MID_TRACE,("Mapping data buffer pages\n"));

	Map[0].BufferAddress = MmMapLockedPages( Map[0].Mdl, KernelMode );
	
	AFD_DbgPrint(MID_TRACE,("Buffer %d: %x:%d\n", 
				0, 
				Map[0].BufferAddress,
				BytesToCopy));

	/* OskitDumpBuffer
	   ( FCB->Recv.Window + FCB->Recv.BytesUsed, BytesToCopy ); */

	RtlCopyMemory( Map[0].BufferAddress,
		       FCB->Recv.Window + FCB->Recv.BytesUsed,
		       BytesToCopy );
	
	MmUnmapLockedPages( Map[0].BufferAddress, Map[0].Mdl );

	FCB->Recv.BytesUsed = 0;
	*TotalBytesCopied = BytesToCopy;
    }

    Status = Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = BytesToCopy;
    ExFreePool( DatagramRecv->Address );
    ExFreePool( DatagramRecv );
    
    AFD_DbgPrint(MID_TRACE,("Done\n"));

    return Status;
}

NTSTATUS DDKAPI
PacketSocketRecvComplete(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PAFD_FCB FCB = Context;
    PIRP NextIrp;
    PIO_STACK_LOCATION NextIrpSp;
    PLIST_ENTRY ListEntry;
    PAFD_RECV_INFO RecvReq;
    PAFD_STORED_DATAGRAM DatagramRecv;
    UINT DGSize = Irp->IoStatus.Information + sizeof( AFD_STORED_DATAGRAM );

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return STATUS_UNSUCCESSFUL;

    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
	SocketStateUnlock( FCB );
	DestroySocket( FCB );
	return STATUS_SUCCESS;
    }
    
    DatagramRecv = ExAllocatePool( NonPagedPool, DGSize );

    if( DatagramRecv ) {
	DatagramRecv->Len = Irp->IoStatus.Information;
	RtlCopyMemory( DatagramRecv->Buffer, FCB->Recv.Window,
		       DatagramRecv->Len );
	AFD_DbgPrint(MID_TRACE,("Received (A %x)\n", 
				FCB->AddressFrom->RemoteAddress));
	DatagramRecv->Address = 
	    TaCopyTransportAddress( FCB->AddressFrom->RemoteAddress );
	
	InsertTailList( &FCB->DatagramList, &DatagramRecv->ListEntry );
    } else Status = STATUS_NO_MEMORY;

    /* Satisfy as many requests as we can */

    while( NT_SUCCESS(Status) && 
	   !IsListEmpty( &FCB->DatagramList ) && 
	   !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
	AFD_DbgPrint(MID_TRACE,("Looping trying to satisfy request\n"));
	ListEntry = RemoveHeadList( &FCB->DatagramList );
	DatagramRecv = CONTAINING_RECORD( ListEntry, AFD_STORED_DATAGRAM,
					  ListEntry );
	ListEntry = RemoveHeadList( &FCB->PendingIrpList[FUNCTION_RECV] );
	NextIrp = CONTAINING_RECORD( ListEntry, IRP, Tail.Overlay.ListEntry );
	NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
	RecvReq = NextIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

	AFD_DbgPrint(MID_TRACE,("RecvReq: %x, DatagramRecv: %x\n",
				RecvReq, DatagramRecv));

	if( DatagramRecv->Len > RecvReq->BufferArray[0].len && 
	    !(RecvReq->TdiFlags & TDI_RECEIVE_PARTIAL) ) {
	    InsertHeadList( &FCB->DatagramList,
			    &DatagramRecv->ListEntry );
	    Status = NextIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
	    NextIrp->IoStatus.Information = DatagramRecv->Len;
	    UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );
	    IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
	} else {
	    AFD_DbgPrint(MID_TRACE,("Satisfying\n"));
	    Status = SatisfyPacketRecvRequest
		( FCB, NextIrp, DatagramRecv, 
		  (PUINT)&NextIrp->IoStatus.Information );
	    AFD_DbgPrint(MID_TRACE,("Unlocking\n"));
	    UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );
	    AFD_DbgPrint(MID_TRACE,("Completing\n"));
	    IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
	}
    }

    if( !IsListEmpty( &FCB->DatagramList ) ) { 
	AFD_DbgPrint(MID_TRACE,("Signalling\n"));
	FCB->PollState |= AFD_EVENT_RECEIVE;
	PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else 
	FCB->PollState &= ~AFD_EVENT_RECEIVE;

    if( NT_SUCCESS(Irp->IoStatus.Status) ) {
	/* Now relaunch the datagram request */
	SocketCalloutEnter( FCB );

	Status = TdiReceiveDatagram
	    ( &FCB->ReceiveIrp.InFlightRequest,
	      FCB->AddressFile.Object,
	      0,
	      FCB->Recv.Window,
	      FCB->Recv.Size,
	      FCB->AddressFrom,
	      &FCB->ReceiveIrp.Iosb,
	      PacketSocketRecvComplete,
	      FCB );

	SocketCalloutLeave( FCB );
    }

    SocketStateUnlock( FCB );

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_INFO_UDP RecvReq;
    PLIST_ENTRY ListEntry;
    PAFD_STORED_DATAGRAM DatagramRecv;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp, FALSE );
    /* Check that the socket is bound */
    if( FCB->State != SOCKET_STATE_BOUND ) 
	return UnlockAndMaybeComplete
	    ( FCB, STATUS_UNSUCCESSFUL, Irp, 0, NULL, FALSE );
    if( !(RecvReq = LockRequest( Irp, IrpSp )) ) 
	return UnlockAndMaybeComplete
	    ( FCB, STATUS_NO_MEMORY, Irp, 0, NULL, FALSE );
    
    AFD_DbgPrint(MID_TRACE,("Recv flags %x\n", RecvReq->AfdFlags));

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray, 
					RecvReq->BufferCount,
					RecvReq->Address,
					RecvReq->AddressLength,
					TRUE, TRUE );

    if( !IsListEmpty( &FCB->DatagramList ) ) {
	ListEntry = RemoveHeadList( &FCB->DatagramList );
	DatagramRecv = CONTAINING_RECORD
	    ( ListEntry, AFD_STORED_DATAGRAM, ListEntry );
	if( DatagramRecv->Len > RecvReq->BufferArray[0].len && 
	    !(RecvReq->TdiFlags & TDI_RECEIVE_PARTIAL) ) {
	    InsertHeadList( &FCB->DatagramList,
			    &DatagramRecv->ListEntry );
	    Status = Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
	    Irp->IoStatus.Information = DatagramRecv->Len;
	    return UnlockAndMaybeComplete
		( FCB, Status, Irp, RecvReq->BufferArray[0].len, NULL, TRUE );
	} else {
	    Status = SatisfyPacketRecvRequest
		( FCB, Irp, DatagramRecv, 
		  (PUINT)&Irp->IoStatus.Information );
	    return UnlockAndMaybeComplete
		( FCB, Status, Irp, Irp->IoStatus.Information, NULL, TRUE );
	}
    } else if( RecvReq->AfdFlags & AFD_IMMEDIATE ) {
	FCB->PollState &= ~AFD_EVENT_RECEIVE;
	AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
	Status = STATUS_CANT_WAIT;
	return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
    } else {
	FCB->PollState &= ~AFD_EVENT_RECEIVE;
	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
    }
}
