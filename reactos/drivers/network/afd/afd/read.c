/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/read.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 *
 * Improve buffering code
 *
 * We're keeping data receiving in one of two states:
 * A) Some data available in the FCB
 *    FCB->Recv.BytesUsed != FCB->Recv.Content
 *    FCB->ReceiveIrp.InFlightRequest == NULL
 *    AFD_EVENT_RECEIVE set in FCB->PollState
 * B) No data available in the FCB
 *    FCB->Recv.BytesUsed == FCB->Recv.Content (== 0)
 *    FCB->RecieveIrp.InFlightRequest != NULL
 *    AFD_EVENT_RECEIVED not set in FCB->PollState
 * So basically we either have data available or a TDI receive
 * in flight.
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

static BOOLEAN CantReadMore( PAFD_FCB FCB ) {
    UINT BytesAvailable = FCB->Recv.Content - FCB->Recv.BytesUsed;

    return !BytesAvailable &&
        (FCB->PollState & (AFD_EVENT_CLOSE | AFD_EVENT_DISCONNECT));
}

static VOID HandleEOFOnIrp( PAFD_FCB FCB, NTSTATUS Status, UINT Information ) {
    if( !NT_SUCCESS(Status) || 
		(Status == STATUS_SUCCESS && Information == 0) ) {
        AFD_DbgPrint(MID_TRACE,("Looks like an EOF\n"));
        FCB->PollState |= AFD_EVENT_DISCONNECT;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }
}

static NTSTATUS TryToSatisfyRecvRequestFromBuffer( PAFD_FCB FCB,
												   PAFD_RECV_INFO RecvReq,
												   PUINT TotalBytesCopied ) {
    UINT i, BytesToCopy = 0,
		BytesAvailable =
		FCB->Recv.Content - FCB->Recv.BytesUsed;
    PAFD_MAPBUF Map;
    NTSTATUS Status;
    *TotalBytesCopied = 0;


    AFD_DbgPrint(MID_TRACE,("Called, BytesAvailable = %d\n",
							BytesAvailable));

    if( CantReadMore(FCB) ) return STATUS_SUCCESS;
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

            *TotalBytesCopied += BytesToCopy;

            if (!(RecvReq->TdiFlags & TDI_RECEIVE_PEEK)) {
                FCB->Recv.BytesUsed += BytesToCopy;
                BytesAvailable -= BytesToCopy;
            }
		}
    }

    /* If there's nothing left in our buffer start a new request */
    if( FCB->Recv.BytesUsed == FCB->Recv.Content ) {
		FCB->Recv.BytesUsed = FCB->Recv.Content = 0;
        FCB->PollState &= ~AFD_EVENT_RECEIVE;
		PollReeval( FCB->DeviceExt, FCB->FileObject );

		if( !FCB->ReceiveIrp.InFlightRequest ) {
			AFD_DbgPrint(MID_TRACE,("Replenishing buffer\n"));

			Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
								 FCB->Connection.Object,
								 TDI_RECEIVE_NORMAL,
								 FCB->Recv.Window,
								 FCB->Recv.Size,
								 &FCB->ReceiveIrp.Iosb,
								 ReceiveComplete,
								 FCB );

            if( Status == STATUS_SUCCESS )
                FCB->Recv.Content = FCB->ReceiveIrp.Iosb.Information;
            HandleEOFOnIrp( FCB, Status, FCB->ReceiveIrp.Iosb.Information );
		}
    }

    return STATUS_SUCCESS;
}

static NTSTATUS ReceiveActivity( PAFD_FCB FCB, PIRP Irp ) {
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;
    PIO_STACK_LOCATION NextIrpSp;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0, RetBytesCopied = 0;
    NTSTATUS Status = STATUS_SUCCESS, RetStatus = STATUS_PENDING;

    AFD_DbgPrint(MID_TRACE,("%x %x\n", FCB, Irp));

    if( CantReadMore( FCB ) ) {
        /* Success here means that we got an EOF.  Complete a pending read
         * with zero bytes if we haven't yet overread, then kill the others.
         */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
            NextIrpEntry =
                RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
            NextIrp =
                CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
            NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
            RecvReq = NextIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            AFD_DbgPrint(MID_TRACE,("Completing recv %x (%d)\n", NextIrp,
                                    TotalBytesCopied));
            UnlockBuffers( RecvReq->BufferArray,
                           RecvReq->BufferCount, FALSE );
            Status = NextIrp->IoStatus.Status =
                FCB->Overread ? STATUS_END_OF_FILE : STATUS_SUCCESS;
            NextIrp->IoStatus.Information = 0;
            if( NextIrp == Irp ) RetStatus = Status;
            if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
            IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
            FCB->Overread = TRUE;
            //FCB->PollState |= AFD_EVENT_DISCONNECT;
            PollReeval( FCB->DeviceExt, FCB->FileObject );
        }
    } else {
		/* Kick the user that receive would be possible now */
		/* XXX Not implemented yet */

		AFD_DbgPrint(MID_TRACE,("FCB %x Receive data waiting %d\n",
								FCB, FCB->Recv.Content));
		/*OskitDumpBuffer( FCB->Recv.Window, FCB->Recv.Content );*/

		/* Try to clear some requests */
		while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
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
				if( NextIrp == Irp ) { 
					RetStatus = Status;
					RetBytesCopied = TotalBytesCopied;
				}
				if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
				IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
			}
		}
    }

    if( FCB->Recv.Content ) {
		FCB->PollState |= AFD_EVENT_RECEIVE;
    } else
		FCB->PollState &= ~AFD_EVENT_RECEIVE;

    PollReeval( FCB->DeviceExt, FCB->FileObject );

    AFD_DbgPrint(MID_TRACE,("RetStatus for irp %x is %x\n", Irp, RetStatus));

    /* Sometimes we're called with a NULL Irp */
    if( Irp ) {
        Irp->IoStatus.Status = RetStatus;
        Irp->IoStatus.Information = RetBytesCopied;
    }

    return RetStatus;
}

NTSTATUS NTAPI ReceiveComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    PAFD_FCB FCB = (PAFD_FCB)Context;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    ASSERT_IRQL(APC_LEVEL);

    if( !SocketAcquireStateLock( FCB ) ) {
        Irp->IoStatus.Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Information = 0;
        return STATUS_FILE_CLOSED;
    }

    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( Irp->Cancel ) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        SocketStateUnlock( FCB );
		return STATUS_CANCELLED;
    }

    FCB->Recv.Content = Irp->IoStatus.Information;
    FCB->Recv.BytesUsed = 0;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        AFD_DbgPrint(MIN_TRACE,("!!! CLOSED SOCK GOT A RECEIVE COMPLETE !!!\n"));
        Irp->IoStatus.Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Information = 0;
		SocketStateUnlock( FCB );
		DestroySocket( FCB );
		return STATUS_FILE_CLOSED;
    } else if( FCB->State == SOCKET_STATE_LISTENING ) {
        AFD_DbgPrint(MIN_TRACE,("!!! LISTENER GOT A RECEIVE COMPLETE !!!\n"));
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        SocketStateUnlock( FCB );
        return STATUS_INVALID_PARAMETER;
    }

    HandleEOFOnIrp( FCB, Irp->IoStatus.Status, Irp->IoStatus.Information );

	ReceiveActivity( FCB, NULL );

	PollReeval( FCB->DeviceExt, FCB->FileObject );
		
    SocketStateUnlock( FCB );

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
						   PIO_STACK_LOCATION IrpSp, BOOLEAN Short) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( FCB->State != SOCKET_STATE_CONNECTED &&
        FCB->State != SOCKET_STATE_CONNECTING ) {
        AFD_DbgPrint(MID_TRACE,("Called recv on wrong kind of socket (s%x)\n",
                                FCB->State));
        return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER,
									   Irp, 0, NULL );
    }

    if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS )
    {
		AFD_DbgPrint(MID_TRACE,("Receive on connection-less sockets not implemented\n"));
		return UnlockAndMaybeComplete( FCB, STATUS_NOT_IMPLEMENTED,
									   Irp, 0, NULL );
    }

    FCB->EventsFired &= ~AFD_EVENT_RECEIVE;
    PollReeval( FCB->DeviceExt, FCB->FileObject );

    if( !(RecvReq = LockRequest( Irp, IrpSp )) )
		return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
									   Irp, 0, NULL );

    AFD_DbgPrint(MID_TRACE,("Recv flags %x\n", RecvReq->AfdFlags));

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray,
										RecvReq->BufferCount,
										NULL, NULL,
										TRUE, FALSE );

    if( !RecvReq->BufferArray ) {
        return UnlockAndMaybeComplete( FCB, STATUS_ACCESS_VIOLATION,
                                       Irp, 0, NULL );
    }

    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    InsertTailList( &FCB->PendingIrpList[FUNCTION_RECV],
                    &Irp->Tail.Overlay.ListEntry );

    /************ From this point, the IRP is not ours ************/

    Status = ReceiveActivity( FCB, Irp );

    if( Status == STATUS_PENDING && RecvReq->AfdFlags & AFD_IMMEDIATE ) {
        AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
        Status = STATUS_CANT_WAIT;
        TotalBytesCopied = 0;
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
        UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, FALSE );
        return UnlockAndMaybeComplete( FCB, Status, Irp,
                                       TotalBytesCopied, NULL );
    } else if( Status == STATUS_PENDING ) {
        AFD_DbgPrint(MID_TRACE,("Leaving read irp\n"));
        IoMarkIrpPending( Irp );
    } else {
        AFD_DbgPrint(MID_TRACE,("Completed with status %x\n", Status));
    }

    SocketStateUnlock( FCB );
    return Status;
}


static NTSTATUS NTAPI
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

        *TotalBytesCopied = BytesToCopy;

        if (!(RecvReq->TdiFlags & TDI_RECEIVE_PEEK)) {
            FCB->Recv.BytesUsed = 0;
        }
    }

    Status = Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = BytesToCopy;
    ExFreePool( DatagramRecv->Address );
    ExFreePool( DatagramRecv );

    AFD_DbgPrint(MID_TRACE,("Done\n"));

    return Status;
}

NTSTATUS NTAPI
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

    if( !SocketAcquireStateLock( FCB ) ) {
        Irp->IoStatus.Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Information = 0;
        return STATUS_FILE_CLOSED;
    }

    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( Irp->IoStatus.Status == STATUS_CANCELLED ) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        SocketStateUnlock( FCB );
		return STATUS_CANCELLED;
    }

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        Irp->IoStatus.Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Information = 0;
		SocketStateUnlock( FCB );
		DestroySocket( FCB );
		return STATUS_FILE_CLOSED;
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

		if( !DatagramRecv->Address ) Status = STATUS_NO_MEMORY;

    } else Status = STATUS_NO_MEMORY;

    if( !NT_SUCCESS( Status ) ) {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
		if( DatagramRecv ) ExFreePool( DatagramRecv );
		SocketStateUnlock( FCB );
		return Status;
    } else {
		InsertTailList( &FCB->DatagramList, &DatagramRecv->ListEntry );
    }

    /* Satisfy as many requests as we can */

    while( !IsListEmpty( &FCB->DatagramList ) &&
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
            if ( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
			IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
		} else {
			AFD_DbgPrint(MID_TRACE,("Satisfying\n"));
			Status = SatisfyPacketRecvRequest
				( FCB, NextIrp, DatagramRecv,
				  (PUINT)&NextIrp->IoStatus.Information );
			AFD_DbgPrint(MID_TRACE,("Unlocking\n"));
			UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );
            if ( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
			AFD_DbgPrint(MID_TRACE,("Completing\n"));
			IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
		}
    }

    if( !IsListEmpty( &FCB->DatagramList ) ) {
		AFD_DbgPrint(MID_TRACE,("Signalling\n"));
		FCB->PollState |= AFD_EVENT_RECEIVE;
    } else
		FCB->PollState &= ~AFD_EVENT_RECEIVE;

    PollReeval( FCB->DeviceExt, FCB->FileObject );

    if( NT_SUCCESS(Irp->IoStatus.Status) ) {
		/* Now relaunch the datagram request */
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
    }

    SocketStateUnlock( FCB );

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
						PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_INFO_UDP RecvReq;
    PLIST_ENTRY ListEntry;
    PAFD_STORED_DATAGRAM DatagramRecv;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    FCB->EventsFired &= ~AFD_EVENT_RECEIVE;

    /* Check that the socket is bound */
    if( FCB->State != SOCKET_STATE_BOUND )
		return UnlockAndMaybeComplete
			( FCB, STATUS_INVALID_PARAMETER, Irp, 0, NULL );
    if( !(RecvReq = LockRequest( Irp, IrpSp )) )
		return UnlockAndMaybeComplete
			( FCB, STATUS_NO_MEMORY, Irp, 0, NULL );

    AFD_DbgPrint(MID_TRACE,("Recv flags %x\n", RecvReq->AfdFlags));

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray,
										RecvReq->BufferCount,
										RecvReq->Address,
										RecvReq->AddressLength,
										TRUE, TRUE );

    if( !RecvReq->BufferArray ) { /* access violation in userspace */
	    return UnlockAndMaybeComplete
			( FCB, STATUS_ACCESS_VIOLATION, Irp, 0, NULL );
    }

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

			if( IsListEmpty( &FCB->DatagramList ) )
				FCB->PollState &= ~AFD_EVENT_RECEIVE;
			else
				FCB->PollState |= AFD_EVENT_RECEIVE;

			PollReeval( FCB->DeviceExt, FCB->FileObject );

			UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );

			return UnlockAndMaybeComplete
				( FCB, Status, Irp, Irp->IoStatus.Information, NULL );
		} else {
			Status = SatisfyPacketRecvRequest
				( FCB, Irp, DatagramRecv,
				  (PUINT)&Irp->IoStatus.Information );

			if( IsListEmpty( &FCB->DatagramList ) )
				FCB->PollState &= ~AFD_EVENT_RECEIVE;
			else
				FCB->PollState |= AFD_EVENT_RECEIVE;

			PollReeval( FCB->DeviceExt, FCB->FileObject );

			UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );

			return UnlockAndMaybeComplete
				( FCB, Status, Irp, Irp->IoStatus.Information, NULL );
		}
    } else if( RecvReq->AfdFlags & AFD_IMMEDIATE ) {
		AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
		Status = STATUS_CANT_WAIT;
		PollReeval( FCB->DeviceExt, FCB->FileObject );
		UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );
		return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
    } else {
		PollReeval( FCB->DeviceExt, FCB->FileObject );
		return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
    }
}
