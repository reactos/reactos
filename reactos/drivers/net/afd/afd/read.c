/* $Id: read.c,v 1.1.2.1 2004/07/09 04:41:18 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/read.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS TryToSatisfyRecvRequestFromBuffer( PAFD_FCB FCB,
					    PAFD_RECV_REQ RecvReq,
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
    PAFD_RECV_REQ RecvReq;
    UINT TotalBytesCopied = 0;

    AFD_DbgPrint(MID_TRACE,("Called\n"));
    
    if( !SocketAcquireStateLock( FCB ) ) return Status;
    
    if( NT_SUCCESS(Irp->IoStatus.Status) ) {
	/* Update the receive window */
	FCB->Recv.Content = Irp->IoStatus.Information;
	FCB->Recv.BytesUsed = 0;
	/* Kick the user that receive would be possible now */
	/* XXX Not implemented yet */

	AFD_DbgPrint(MID_TRACE,("FCB %x Receive data waiting %d\n",
				FCB, FCB->Recv.Content));
	OskitDumpBuffer( FCB->Recv.Window, FCB->Recv.Content );

	Status = STATUS_SUCCESS;

	/* Try to clear some requests */
	while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) &&
	       NT_SUCCESS(Status) ) {
	    NextIrpEntry = 
		RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
	    NextIrp = 
		CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	    RecvReq = NextIrp->AssociatedIrp.SystemBuffer;

	    AFD_DbgPrint(MID_TRACE,("RecvReq @ %x\n", RecvReq));

	    Status = TryToSatisfyRecvRequestFromBuffer
		( FCB, RecvReq, &TotalBytesCopied );

	    if( Status == STATUS_PENDING ) {
		AFD_DbgPrint(MID_TRACE,("Ran out of data for %x\n", NextIrp));
		InsertHeadList(&FCB->PendingIrpList[FUNCTION_RECV],
			       &NextIrp->Tail.Overlay.ListEntry);
		break;
	    } else {
		AFD_DbgPrint(MID_TRACE,("Completing recv %x\n", NextIrp));
		UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount );
		NextIrp->IoStatus.Status = Status;
		NextIrp->IoStatus.Information = TotalBytesCopied;
		IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
	    }
	}

	if( FCB->Recv.Window && !FCB->Recv.Content ) {
	    AFD_DbgPrint(MID_TRACE,
			 ("Exhausted our buffer.  Requesting new: %x\n", FCB));
	    Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
				 IrpSp->FileObject,
				 TDI_RECEIVE_NORMAL,
				 FCB->Recv.Window,
				 FCB->Recv.Size,
				 &FCB->ReceiveIrp.Iosb,
				 ReceiveComplete,
				 FCB );
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

    AFD_DbgPrint(MID_TRACE,("Returned %x\n", Status));

    return Status;
}

NTSTATUS STDCALL
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			   PIO_STACK_LOCATION IrpSp, BOOLEAN Short) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_REQ RecvReq = Irp->AssociatedIrp.SystemBuffer;
    UINT TotalBytesCopied = 0;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray, 
					RecvReq->BufferCount,
					TRUE );

    Status = TryToSatisfyRecvRequestFromBuffer
	( FCB, RecvReq, &TotalBytesCopied );

    if( Status != STATUS_PENDING ) {
	UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount );
	return UnlockAndMaybeComplete( FCB, Status, Irp, 
				       TotalBytesCopied, NULL );
    } else {
	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
    }
}

NTSTATUS STDCALL
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    Irp->IoStatus.Information = 0;
    return STATUS_END_OF_FILE;
}

