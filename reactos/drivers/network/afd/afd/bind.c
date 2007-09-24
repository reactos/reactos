/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/bind.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS WarmSocketForBind( PAFD_FCB FCB ) {
    NTSTATUS Status;

    AFD_DbgPrint(MID_TRACE,("Called (AF %d)\n",
                            FCB->LocalAddress->Address[0].AddressType));

    if( !FCB->TdiDeviceName.Length || !FCB->TdiDeviceName.Buffer ) {
        AFD_DbgPrint(MID_TRACE,("Null Device\n"));
        return STATUS_NO_SUCH_DEVICE;
    }
    if( !FCB->LocalAddress ) {
        AFD_DbgPrint(MID_TRACE,("No local address\n"));
        return STATUS_UNSUCCESSFUL;
    }

    Status = TdiOpenAddressFile(&FCB->TdiDeviceName,
                                FCB->LocalAddress,
                                &FCB->AddressFile.Handle,
                                &FCB->AddressFile.Object );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

NTSTATUS STDCALL
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_BIND_DATA BindReq;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp, FALSE );
    if( !(BindReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
				       Irp, 0, NULL, FALSE );

    FCB->LocalAddress = TaCopyTransportAddress( &BindReq->Address );

    if( FCB->LocalAddress )
	Status = WarmSocketForBind( FCB );
    else Status = STATUS_NO_MEMORY;

    if( NT_SUCCESS(Status) )
	FCB->State = SOCKET_STATE_BOUND;
    else return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, FALSE );

    AFD_DbgPrint(MID_TRACE,("FCB->Flags %x\n", FCB->Flags));

    if( FCB->Flags & SGID_CONNECTIONLESS ) {
	/* This will be the from address for subsequent recvfrom calls */
	TdiBuildConnectionInfo( &FCB->AddressFrom,
				FCB->LocalAddress );

	AFD_DbgPrint(MID_TRACE,("Calling TdiReceiveDatagram\n"));

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

	/* We don't want to wait for this read to complete. */
	if( Status == STATUS_PENDING ) Status = STATUS_SUCCESS;
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
}

