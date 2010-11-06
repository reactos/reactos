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
        return STATUS_INVALID_PARAMETER;
    }

    Status = TdiOpenAddressFile(&FCB->TdiDeviceName,
                                FCB->LocalAddress,
                                &FCB->AddressFile.Handle,
                                &FCB->AddressFile.Object );
    if (!NT_SUCCESS(Status))
        return Status;

    if (FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS)
    {
        Status = TdiQueryMaxDatagramLength(FCB->AddressFile.Object,
                                           &FCB->Recv.Size);
        if (NT_SUCCESS(Status))
        {
            FCB->Recv.Window = ExAllocatePool(PagedPool, FCB->Recv.Size);
            if (!FCB->Recv.Window)
                Status = STATUS_NO_MEMORY;
        }
    }

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

NTSTATUS NTAPI
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_BIND_DATA BindReq;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );
    if( !(BindReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
				       Irp, 0 );

    if( FCB->LocalAddress ) ExFreePool( FCB->LocalAddress );
    FCB->LocalAddress = TaCopyTransportAddress( &BindReq->Address );

    if( FCB->LocalAddress )
	Status = TdiBuildConnectionInfo( &FCB->AddressFrom,
					 FCB->LocalAddress );

    if( NT_SUCCESS(Status) )
	Status = WarmSocketForBind( FCB );
    AFD_DbgPrint(MID_TRACE,("FCB->Flags %x\n", FCB->Flags));

    if( !NT_SUCCESS(Status) )
        return UnlockAndMaybeComplete(FCB, Status, Irp, 0);

    if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS ) {
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

    if (NT_SUCCESS(Status))
        FCB->State = SOCKET_STATE_BOUND;

    /* MSAFD relies on us returning the address file handle in the IOSB */
    return UnlockAndMaybeComplete( FCB, Status, Irp, (ULONG_PTR)FCB->AddressFile.Handle );
}

