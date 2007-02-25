/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/main.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040630 Created
 *
 * Suggestions: Uniform naming (AfdXxx)
 */

/* INCLUDES */

#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

#ifdef DBG

/* See debug.h for debug/trace constants */
//DWORD DebugTraceLevel = DEBUG_ULTRA;
DWORD DebugTraceLevel = 0;

#endif /* DBG */

void OskitDumpBuffer( PCHAR Data, UINT Len ) {
    unsigned int i;

    for( i = 0; i < Len; i++ ) {
	if( i && !(i & 0xf) ) DbgPrint( "\n" );
	if( !(i & 0xf) ) DbgPrint( "%08x: ", (UINT)(Data + i) );
	DbgPrint( " %02x", Data[i] & 0xff );
    }
    DbgPrint("\n");
}

/* FUNCTIONS */

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

static NTSTATUS STDCALL
AfdCreateSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp) {
    PAFD_FCB FCB;
    PFILE_OBJECT FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt;
    PFILE_FULL_EA_INFORMATION EaInfo;
    PAFD_CREATE_PACKET ConnectInfo = NULL;
    ULONG EaLength;
    PWCHAR EaInfoValue = NULL;
    UINT Disposition, i;

    AFD_DbgPrint(MID_TRACE,
		 ("AfdCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp));

    DeviceExt = DeviceObject->DeviceExtension;
    FileObject = IrpSp->FileObject;
    Disposition = (IrpSp->Parameters.Create.Options >> 24) & 0xff;

    Irp->IoStatus.Information = 0;

    EaInfo = Irp->AssociatedIrp.SystemBuffer;

    if( EaInfo ) {
	ConnectInfo = (PAFD_CREATE_PACKET)(EaInfo->EaName + EaInfo->EaNameLength + 1);
	EaInfoValue = (PWCHAR)(((PCHAR)ConnectInfo) + sizeof(AFD_CREATE_PACKET));

	EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
	    EaInfo->EaNameLength +
	    EaInfo->EaValueLength;

	AFD_DbgPrint(MID_TRACE,("EaInfo: %x, EaInfoValue: %x\n",
				EaInfo, EaInfoValue));
    }

    AFD_DbgPrint(MID_TRACE,("About to allocate the new FCB\n"));

    FCB = ExAllocatePool(NonPagedPool, sizeof(AFD_FCB));
    if( FCB == NULL ) {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NO_MEMORY;
    }

    AFD_DbgPrint(MID_TRACE,("Initializing the new FCB @ %x (FileObject %x Flags %x)\n", FCB, FileObject, ConnectInfo ? ConnectInfo->EndpointFlags : 0));

    RtlZeroMemory( FCB, sizeof( *FCB ) );

    FCB->Flags = ConnectInfo ? ConnectInfo->EndpointFlags : 0;
    FCB->State = SOCKET_STATE_CREATED;
    FCB->FileObject = FileObject;
    FCB->DeviceExt = DeviceExt;
    FCB->Recv.Size = DEFAULT_RECEIVE_WINDOW_SIZE;
    FCB->Send.Size = DEFAULT_SEND_WINDOW_SIZE;

    KeInitializeSpinLock( &FCB->SpinLock );
    ExInitializeFastMutex( &FCB->Mutex );
    KeInitializeEvent( &FCB->StateLockedEvent, NotificationEvent, FALSE );

    for( i = 0; i < MAX_FUNCTIONS; i++ ) {
	InitializeListHead( &FCB->PendingIrpList[i] );
    }

    InitializeListHead( &FCB->DatagramList );
    InitializeListHead( &FCB->PendingConnections );

    AFD_DbgPrint(MID_TRACE,("%x: Checking command channel\n", FCB));

    if( ConnectInfo ) {
	FCB->TdiDeviceName.Length = ConnectInfo->SizeOfTransportName;
	FCB->TdiDeviceName.MaximumLength = FCB->TdiDeviceName.Length;
	FCB->TdiDeviceName.Buffer =
	    ExAllocatePool( NonPagedPool, FCB->TdiDeviceName.Length );
	RtlCopyMemory( FCB->TdiDeviceName.Buffer,
		       ConnectInfo->TransportName,
		       FCB->TdiDeviceName.Length );

	if( !FCB->TdiDeviceName.Buffer ) {
	    ExFreePool(FCB);
	    AFD_DbgPrint(MID_TRACE,("Could not copy target string\n"));
	    Irp->IoStatus.Status = STATUS_NO_MEMORY;
	    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	    return STATUS_NO_MEMORY;
	}

	AFD_DbgPrint(MID_TRACE,("Success: %s %wZ\n",
				EaInfo->EaName, &FCB->TdiDeviceName));
    } else {
	AFD_DbgPrint(MID_TRACE,("Success: Control connection\n"));
    }

    FileObject->FsContext = FCB;

    /* It seems that UDP sockets are writable from inception */
    if( FCB->Flags & SGID_CONNECTIONLESS ) {
        AFD_DbgPrint(MID_TRACE,("Packet oriented socket\n"));
	/* Allocate our backup buffer */
	FCB->Recv.Window = ExAllocatePool( NonPagedPool, FCB->Recv.Size );
        FCB->Send.Window = ExAllocatePool( NonPagedPool, FCB->Send.Size );
	/* A datagram socket is always sendable */
	FCB->PollState |= AFD_EVENT_SEND;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_SUCCESS;
}

VOID DestroySocket( PAFD_FCB FCB ) {
    UINT i;
    BOOLEAN ReturnEarly = FALSE;
    PAFD_IN_FLIGHT_REQUEST InFlightRequest[IN_FLIGHT_REQUESTS];

    AFD_DbgPrint(MIN_TRACE,("Called (%x)\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return;

    FCB->State = SOCKET_STATE_CLOSED;

    InFlightRequest[0] = &FCB->ListenIrp;
    InFlightRequest[1] = &FCB->ReceiveIrp;
    InFlightRequest[2] = &FCB->SendIrp;

    /* Return early here because we might be called in the mean time. */
    if( FCB->Critical ||
	FCB->ListenIrp.InFlightRequest ||
	FCB->ReceiveIrp.InFlightRequest ||
	FCB->SendIrp.InFlightRequest ) {
	AFD_DbgPrint(MIN_TRACE,("Leaving socket alive (%x %x %x)\n",
				FCB->ListenIrp.InFlightRequest,
				FCB->ReceiveIrp.InFlightRequest,
				FCB->SendIrp.InFlightRequest));
        ReturnEarly = TRUE;
    }

    /* After PoolReeval, this FCB should not be involved in any outstanding
     * poll requests */

    /* Cancel our pending requests */
    for( i = 0; i < IN_FLIGHT_REQUESTS; i++ ) {
	NTSTATUS Status = STATUS_NO_SUCH_FILE;
	if( InFlightRequest[i]->InFlightRequest ) {
	    AFD_DbgPrint(MID_TRACE,("Cancelling in flight irp %d (%x)\n",
				    i, InFlightRequest[i]->InFlightRequest));
	    InFlightRequest[i]->InFlightRequest->IoStatus.Status = Status;
	    InFlightRequest[i]->InFlightRequest->IoStatus.Information = 0;
	    IoCancelIrp( InFlightRequest[i]->InFlightRequest );
	}
    }

    SocketStateUnlock( FCB );

    if( ReturnEarly ) return;

    if( FCB->Recv.Window )
	ExFreePool( FCB->Recv.Window );
    if( FCB->Send.Window )
	ExFreePool( FCB->Send.Window );
    if( FCB->AddressFrom )
	ExFreePool( FCB->AddressFrom );
    if( FCB->LocalAddress )
	ExFreePool( FCB->LocalAddress );

    ExFreePool(FCB->TdiDeviceName.Buffer);

    ExFreePool(FCB);
    AFD_DbgPrint(MIN_TRACE,("Deleted (%x)\n", FCB));

    AFD_DbgPrint(MIN_TRACE,("Leaving\n"));
}

static NTSTATUS STDCALL
AfdCloseSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,
		 ("AfdClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp));

    AFD_DbgPrint(MID_TRACE,("FCB %x\n", FCB));

    FCB->PollState |= AFD_EVENT_CLOSE;
    PollReeval( FCB->DeviceExt, FileObject );
    KillSelectsForFCB( FCB->DeviceExt, FileObject, FALSE );

    if( FCB->EventSelect ) ObDereferenceObject( FCB->EventSelect );

    FileObject->FsContext = NULL;
    DestroySocket( FCB );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    AFD_DbgPrint(MID_TRACE, ("Returning success.\n"));

    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
AfdDisconnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_DISCONNECT_INFO DisReq;
    IO_STATUS_BLOCK Iosb;
    PTDI_CONNECTION_INFORMATION ConnInfo;
    NTSTATUS Status;
    USHORT Flags = 0;

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp, FALSE );

    if( !(DisReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
				       Irp, 0, NULL, FALSE );

    if (NULL == FCB->RemoteAddress)
      {
        ConnInfo = NULL;
      }
    else
      {
	Status = TdiBuildNullConnectionInfo
	    ( &ConnInfo, FCB->RemoteAddress->Address[0].AddressType );

	if( !NT_SUCCESS(Status) || !ConnInfo )
	    return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
					   Irp, 0, NULL, TRUE );
      }

    if( DisReq->DisconnectType & AFD_DISCONNECT_SEND )
	Flags |= TDI_DISCONNECT_RELEASE;
    if( DisReq->DisconnectType & AFD_DISCONNECT_RECV ||
	DisReq->DisconnectType & AFD_DISCONNECT_ABORT )
	Flags |= TDI_DISCONNECT_ABORT;

    Status = TdiDisconnect( FCB->Connection.Object,
			    &DisReq->Timeout,
			    Flags,
			    &Iosb,
			    NULL,
			    NULL,
			    FCB->AddressFrom,
			    ConnInfo);

    ExFreePool( ConnInfo );

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
}

static NTSTATUS STDCALL
AfdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;
#ifdef DBG
    PFILE_OBJECT FileObject = IrpSp->FileObject;
#endif

    AFD_DbgPrint(MID_TRACE,("AfdDispatch: %d\n", IrpSp->MajorFunction));
    if( IrpSp->MajorFunction != IRP_MJ_CREATE) {
	AFD_DbgPrint(MID_TRACE,("FO %x, IrpSp->FO %x\n",
				FileObject, IrpSp->FileObject));
	ASSERT(FileObject == IrpSp->FileObject);
    }

    switch(IrpSp->MajorFunction)
    {
	/* opening and closing handles to the device */
    case IRP_MJ_CREATE:
	/* Mostly borrowed from the named pipe file system */
	return AfdCreateSocket(DeviceObject, Irp, IrpSp);

    case IRP_MJ_CLOSE:
	/* Ditto the borrowing */
	return AfdCloseSocket(DeviceObject, Irp, IrpSp);

    /* write data */
    case IRP_MJ_WRITE:
	return AfdConnectedSocketWriteData( DeviceObject, Irp, IrpSp, TRUE );

    /* read data */
    case IRP_MJ_READ:
	return AfdConnectedSocketReadData( DeviceObject, Irp, IrpSp, TRUE );

    case IRP_MJ_DEVICE_CONTROL:
    {
	switch( IrpSp->Parameters.DeviceIoControl.IoControlCode ) {
	case IOCTL_AFD_BIND:
	    return AfdBindSocket( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_CONNECT:
	    return AfdStreamSocketConnect( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_START_LISTEN:
	    return AfdListenSocket( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_RECV:
	    return AfdConnectedSocketReadData( DeviceObject, Irp, IrpSp,
					       FALSE );

	case IOCTL_AFD_SELECT:
	    return AfdSelect( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_EVENT_SELECT:
	    return AfdEventSelect( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_ENUM_NETWORK_EVENTS:
	    return AfdEnumEvents( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_RECV_DATAGRAM:
	    return AfdPacketSocketReadData( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_SEND:
	    return AfdConnectedSocketWriteData( DeviceObject, Irp, IrpSp,
						FALSE );

	case IOCTL_AFD_SEND_DATAGRAM:
	    return AfdPacketSocketWriteData( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_GET_INFO:
	    return AfdGetInfo( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_GET_CONTEXT:
	    return AfdGetContext( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_SET_CONTEXT:
	    return AfdSetContext( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_WAIT_FOR_LISTEN:
	    return AfdWaitForListen( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_ACCEPT:
	    return AfdAccept( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_DISCONNECT:
	    return AfdDisconnect( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_GET_SOCK_NAME:
	    return AfdGetSockOrPeerName( DeviceObject, Irp, IrpSp, TRUE );

        case IOCTL_AFD_GET_PEER_NAME:
            return AfdGetSockOrPeerName( DeviceObject, Irp, IrpSp, FALSE );

	case IOCTL_AFD_GET_TDI_HANDLES:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_TDI_HANDLES\n"));
	    break;

	case IOCTL_AFD_SET_INFO:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_INFO\n"));
	    break;

	case IOCTL_AFD_SET_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_DATA\n"));
	    break;

	case IOCTL_AFD_SET_CONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_OPTIONS\n"));
	    break;

	case IOCTL_AFD_SET_DISCONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_DATA\n"));
	    break;

	case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_OPTIONS\n"));
	    break;

	case IOCTL_AFD_GET_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_CONNECT_DATA\n"));
	    break;

	case IOCTL_AFD_GET_CONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_CONNECT_OPTIONS\n"));
	    break;

	case IOCTL_AFD_GET_DISCONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_DISCONNECT_DATA\n"));
	    break;

	case IOCTL_AFD_GET_DISCONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_DISCONNECT_OPTIONS\n"));
	    break;

	case IOCTL_AFD_SET_CONNECT_DATA_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_DATA_SIZE\n"));
	    break;

	case IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE\n"));
	    break;

	case IOCTL_AFD_SET_DISCONNECT_DATA_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_DATA_SIZE\n"));
	    break;

	case IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE\n"));
	    break;

	case IOCTL_AFD_DEFER_ACCEPT:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_DEFER_ACCEPT\n"));
	    break;

	case IOCTL_AFD_GET_PENDING_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_PENDING_CONNECT_DATA\n"));
	    break;

	default:
	    Status = STATUS_NOT_IMPLEMENTED;
	    Irp->IoStatus.Information = 0;
	    AFD_DbgPrint(MIN_TRACE, ("Unknown IOCTL (0x%x)\n",
				     IrpSp->Parameters.DeviceIoControl.
				     IoControlCode));
	    break;
	}
	break;
    }

/* unsupported operations */
    default:
    {
	Status = STATUS_NOT_IMPLEMENTED;
	AFD_DbgPrint(MIN_TRACE,
		     ("Irp: Unknown Major code was %x\n",
		      IrpSp->MajorFunction));
	break;
    }
    }

    AFD_DbgPrint(MID_TRACE, ("Returning %x\n", Status));
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return (Status);
}

static VOID STDCALL
AfdUnload(PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING wstrDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Afd");
    PAFD_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    /* register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AfdDispatch;
    DriverObject->DriverUnload = AfdUnload;

    Status = IoCreateDevice
	( DriverObject,
	  sizeof(AFD_DEVICE_EXTENSION),
	  &wstrDeviceName,
	  FILE_DEVICE_NAMED_PIPE,
	  0,
	  FALSE,
	  &DeviceObject );

    /* failure */
    if(!NT_SUCCESS(Status))
    {
	return (Status);
    }

    DeviceExt = DeviceObject->DeviceExtension;
    KeInitializeSpinLock( &DeviceExt->Lock );
    InitializeListHead( &DeviceExt->Polls );

    AFD_DbgPrint(MID_TRACE,("Device created: object %x ext %x\n",
			    DeviceObject, DeviceExt));

    return (Status);
}

/* EOF */
