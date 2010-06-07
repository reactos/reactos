/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/connect.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS NTAPI
AfdGetConnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	          PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectOptionsSize == 0)
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);

    ASSERT(FCB->ConnectOptions);

    if (FCB->FilledConnectOptions < BufferSize) BufferSize = FCB->FilledConnectOptions;

    RtlCopyMemory(Irp->UserBuffer,
                  FCB->ConnectOptions,
                  BufferSize);

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, BufferSize);
}

NTSTATUS
NTAPI
AfdSetConnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PVOID ConnectOptions = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT ConnectOptionsSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectOptions)
    {
        ExFreePool(FCB->ConnectOptions);
        FCB->ConnectOptions = NULL;
        FCB->ConnectOptionsSize = 0;
        FCB->FilledConnectOptions = 0;
    }

    FCB->ConnectOptions = ExAllocatePool(PagedPool, ConnectOptionsSize);
    if (!FCB->ConnectOptions) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    RtlCopyMemory(FCB->ConnectOptions,
                  ConnectOptions,
                  ConnectOptionsSize);

    FCB->ConnectOptionsSize = ConnectOptionsSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS
NTAPI
AfdSetConnectOptionsSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PUINT ConnectOptionsSize = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (BufferSize < sizeof(UINT))
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);

    if (FCB->ConnectOptions)
    {
        ExFreePool(FCB->ConnectOptions);
        FCB->ConnectOptionsSize = 0;
        FCB->FilledConnectOptions = 0;
    }

    FCB->ConnectOptions = ExAllocatePool(PagedPool, *ConnectOptionsSize);
    if (!FCB->ConnectOptions) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->ConnectOptionsSize = *ConnectOptionsSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS NTAPI
AfdGetConnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	          PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectDataSize == 0)
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);

    ASSERT(FCB->ConnectData);

    if (FCB->FilledConnectData < BufferSize) BufferSize = FCB->FilledConnectData;

    RtlCopyMemory(Irp->UserBuffer,
                  FCB->ConnectData,
                  BufferSize);

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, BufferSize);
}

NTSTATUS
NTAPI
AfdSetConnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PVOID ConnectData = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT ConnectDataSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectData)
    {
        ExFreePool(FCB->ConnectData);
        FCB->ConnectData = NULL;
        FCB->ConnectDataSize = 0;
        FCB->FilledConnectData = 0;
    }

    FCB->ConnectData = ExAllocatePool(PagedPool, ConnectDataSize);
    if (!FCB->ConnectData) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    RtlCopyMemory(FCB->ConnectData,
                  ConnectData,
                  ConnectDataSize);

    FCB->ConnectDataSize = ConnectDataSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS
NTAPI
AfdSetConnectDataSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PUINT ConnectDataSize = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (BufferSize < sizeof(UINT))
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);

    if (FCB->ConnectData)
    {
        ExFreePool(FCB->ConnectData);
        FCB->ConnectDataSize = 0;
        FCB->FilledConnectData = 0;
    }

    FCB->ConnectData = ExAllocatePool(PagedPool, *ConnectDataSize);
    if (!FCB->ConnectData) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->ConnectDataSize = *ConnectDataSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}


NTSTATUS WarmSocketForConnection( PAFD_FCB FCB ) {
    NTSTATUS Status;

    if( !FCB->TdiDeviceName.Length || !FCB->TdiDeviceName.Buffer ) {
        AFD_DbgPrint(MID_TRACE,("Null Device\n"));
        return STATUS_NO_SUCH_DEVICE;
    }

    Status = TdiOpenConnectionEndpointFile(&FCB->TdiDeviceName,
                                           &FCB->Connection.Handle,
                                           &FCB->Connection.Object );

    if( NT_SUCCESS(Status) ) {
        Status = TdiAssociateAddressFile( FCB->AddressFile.Handle,
                                          FCB->Connection.Object );
    }

    return Status;
}

NTSTATUS MakeSocketIntoConnection( PAFD_FCB FCB ) {
    NTSTATUS Status;

    ASSERT(!FCB->Recv.Window);
    ASSERT(!FCB->Send.Window);

    Status = TdiQueryMaxDatagramLength(FCB->Connection.Object,
                                       &FCB->Send.Size);
    if (!NT_SUCCESS(Status))
        return Status;

    FCB->Recv.Size = FCB->Send.Size;

    /* Allocate the receive area and start receiving */
    FCB->Recv.Window =
	ExAllocatePool( PagedPool, FCB->Recv.Size );

    if( !FCB->Recv.Window ) return STATUS_NO_MEMORY;

    FCB->Send.Window =
	ExAllocatePool( PagedPool, FCB->Send.Size );

    if( !FCB->Send.Window ) {
	ExFreePool( FCB->Recv.Window );
	FCB->Recv.Window = NULL;
	return STATUS_NO_MEMORY;
    }

    FCB->State = SOCKET_STATE_CONNECTED;

    Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
		         FCB->Connection.Object,
		         TDI_RECEIVE_NORMAL,
		         FCB->Recv.Window,
		         FCB->Recv.Size,
		         &FCB->ReceiveIrp.Iosb,
		         ReceiveComplete,
		         FCB );

   if( Status == STATUS_PENDING ) Status = STATUS_SUCCESS;

   FCB->PollState |= AFD_EVENT_CONNECT | AFD_EVENT_SEND;
   FCB->PollStatus[FD_CONNECT_BIT] = STATUS_SUCCESS;
   FCB->PollStatus[FD_WRITE_BIT] = STATUS_SUCCESS;
   PollReeval( FCB->DeviceExt, FCB->FileObject );

   return Status;
}

static NTSTATUS NTAPI StreamSocketConnectComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    NTSTATUS Status = Irp->IoStatus.Status;
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;

    AFD_DbgPrint(MID_TRACE,("Called: FCB %x, FO %x\n",
			    Context, FCB->FileObject));

    /* I was wrong about this before as we can have pending writes to a not
     * yet connected socket */
    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    AFD_DbgPrint(MID_TRACE,("Irp->IoStatus.Status = %x\n",
			    Irp->IoStatus.Status));

    FCB->ConnectIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        /* Cleanup our IRP queue because the FCB is being destroyed */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_CONNECT] ) ) {
	       NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECT]);
	       NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	       NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
	       NextIrp->IoStatus.Information = 0;
	       if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
               (void)IoSetCancelRoutine(NextIrp, NULL);
	       IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }
	SocketStateUnlock( FCB );
	return STATUS_FILE_CLOSED;
    }

    if( !NT_SUCCESS(Irp->IoStatus.Status) ) {
	FCB->PollState |= AFD_EVENT_CONNECT_FAIL;
        FCB->PollStatus[FD_CONNECT_BIT] = Irp->IoStatus.Status;
	AFD_DbgPrint(MID_TRACE,("Going to bound state\n"));
	FCB->State = SOCKET_STATE_BOUND;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }

    /* Succeed pending irps on the FUNCTION_CONNECT list */
    while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_CONNECT] ) ) {
	NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECT]);
	NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	AFD_DbgPrint(MID_TRACE,("Completing connect %x\n", NextIrp));
	NextIrp->IoStatus.Status = Status;
	NextIrp->IoStatus.Information = NT_SUCCESS(Status) ? ((ULONG_PTR)FCB->Connection.Handle) : 0;
	if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
        (void)IoSetCancelRoutine(NextIrp, NULL);
	IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
    }

    if( NT_SUCCESS(Status) ) {
	Status = MakeSocketIntoConnection( FCB );

	if( !NT_SUCCESS(Status) ) {
	    SocketStateUnlock( FCB );
	    return Status;
	}

        FCB->FilledConnectData = MIN(FCB->ConnectInfo->UserDataLength, FCB->ConnectDataSize);
        if (FCB->FilledConnectData)
        {
            RtlCopyMemory(FCB->ConnectData,
                          FCB->ConnectInfo->UserData,
                          FCB->FilledConnectData);
        }

        FCB->FilledConnectOptions = MIN(FCB->ConnectInfo->OptionsLength, FCB->ConnectOptionsSize);
        if (FCB->FilledConnectOptions)
        {
            RtlCopyMemory(FCB->ConnectOptions,
                          FCB->ConnectInfo->Options,
                          FCB->FilledConnectOptions);
        }

	if( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_SEND] ) ) {
	    NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_SEND]);
	    NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP,
					Tail.Overlay.ListEntry);
	    AFD_DbgPrint(MID_TRACE,("Launching send request %x\n", NextIrp));
	    Status = AfdConnectedSocketWriteData
		( DeviceObject,
		  NextIrp,
		  IoGetCurrentIrpStackLocation( NextIrp ),
		  FALSE );
	}

	if( Status == STATUS_PENDING )
	    Status = STATUS_SUCCESS;
    }

    SocketStateUnlock( FCB );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

/* Return the socket object for ths request only if it is a connected or
   stream type. */
NTSTATUS NTAPI
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		       PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_CONNECT_INFO ConnectReq;
    PTDI_CONNECTION_INFORMATION TargetAddress;
    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );
    if( !(ConnectReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
				       0 );

    AFD_DbgPrint(MID_TRACE,("Connect request:\n"));
#if 0
    OskitDumpBuffer
	( (PCHAR)ConnectReq,
	  IrpSp->Parameters.DeviceIoControl.InputBufferLength );
#endif

   if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS )
   {
	if( FCB->RemoteAddress ) ExFreePool( FCB->RemoteAddress );
	FCB->RemoteAddress =
	    TaCopyTransportAddress( &ConnectReq->RemoteAddress );

	if( !FCB->RemoteAddress )
	    Status = STATUS_NO_MEMORY;
	else
	    Status = STATUS_SUCCESS;

	return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
   }

    switch( FCB->State ) {
    case SOCKET_STATE_CONNECTED:
	Status = STATUS_SUCCESS;
	break;

    case SOCKET_STATE_CONNECTING:
	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECT );

    case SOCKET_STATE_CREATED:
	if( FCB->LocalAddress ) ExFreePool( FCB->LocalAddress );
	FCB->LocalAddress =
	    TaCopyTransportAddress( &ConnectReq->RemoteAddress );

	if( FCB->LocalAddress ) {
	    Status = WarmSocketForBind( FCB );

	    if( NT_SUCCESS(Status) )
		FCB->State = SOCKET_STATE_BOUND;
	    else
		return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
	} else
	    return UnlockAndMaybeComplete
		( FCB, STATUS_NO_MEMORY, Irp, 0 );
    
    /* Drop through to SOCKET_STATE_BOUND */

    case SOCKET_STATE_BOUND:
	if( FCB->RemoteAddress ) ExFreePool( FCB->RemoteAddress );
	FCB->RemoteAddress =
	    TaCopyTransportAddress( &ConnectReq->RemoteAddress );

	if( !FCB->RemoteAddress ) {
	    Status = STATUS_NO_MEMORY;
	    break;
	}

	Status = WarmSocketForConnection( FCB );

	if( !NT_SUCCESS(Status) )
	    break;

	Status = TdiBuildConnectionInfo
	    ( &FCB->ConnectInfo,
	      &ConnectReq->RemoteAddress );

        if( NT_SUCCESS(Status) )
            Status = TdiBuildConnectionInfo(&TargetAddress,
                                  	    &ConnectReq->RemoteAddress);
        else break;


	if( NT_SUCCESS(Status) ) {
            TargetAddress->UserData = FCB->ConnectData;
            TargetAddress->UserDataLength = FCB->ConnectDataSize;
            TargetAddress->Options = FCB->ConnectOptions;
            TargetAddress->OptionsLength = FCB->ConnectOptionsSize;

	    Status = TdiConnect( &FCB->ConnectIrp.InFlightRequest,
				 FCB->Connection.Object,
				 TargetAddress,
				 FCB->ConnectInfo,
				 &FCB->ConnectIrp.Iosb,
				 StreamSocketConnectComplete,
				 FCB );

            ExFreePool(TargetAddress);

	    AFD_DbgPrint(MID_TRACE,("Queueing IRP %x\n", Irp));

	    if( Status == STATUS_PENDING ) {
                FCB->State = SOCKET_STATE_CONNECTING;
		return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECT );
            }
	}
	break;

    default:
	AFD_DbgPrint(MID_TRACE,("Inappropriate socket state %d for connect\n",
				FCB->State));
	break;
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}
