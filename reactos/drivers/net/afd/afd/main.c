/* $Id: main.c,v 1.11 2004/11/16 18:07:57 chorns Exp $
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

extern NTSTATUS DDKAPI MmCopyFromCaller( PVOID Dst, PVOID Src, UINT Size );

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
AfdCreateSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp) {
    PAFD_FCB FCB;
    PFILE_OBJECT FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt;
    PFILE_FULL_EA_INFORMATION EaInfo;
    PAFD_CREATE_PACKET ConnectInfo;
    ULONG EaLength;
    PWCHAR EaInfoValue;
    UINT Disposition, i;

    AFD_DbgPrint(MID_TRACE,
		 ("AfdCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp));

    DeviceExt = DeviceObject->DeviceExtension;
    FileObject = IrpSp->FileObject;
    Disposition = (IrpSp->Parameters.Create.Options >> 24) & 0xff;
    
    Irp->IoStatus.Information = 0;
    
    EaInfo = Irp->AssociatedIrp.SystemBuffer;
    ConnectInfo = (PAFD_CREATE_PACKET)(EaInfo->EaName + EaInfo->EaNameLength + 1);
    EaInfoValue = (PWCHAR)(((PCHAR)ConnectInfo) + sizeof(AFD_CREATE_PACKET));

    if(!EaInfo) {
	AFD_DbgPrint(MIN_TRACE, ("No EA Info in IRP.\n"));
	Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_INVALID_PARAMETER;
    }

    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
	EaInfo->EaNameLength +
	EaInfo->EaValueLength;

    AFD_DbgPrint(MID_TRACE,("EaInfo: %x, EaInfoValue: %x\n", 
			    EaInfo, EaInfoValue));

    AFD_DbgPrint(MID_TRACE,("About to allocate the new FCB\n"));

    FCB = ExAllocatePool(NonPagedPool, sizeof(AFD_FCB));
    if( FCB == NULL ) {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NO_MEMORY;
    }

    AFD_DbgPrint(MID_TRACE,("Initializing the new FCB @ %x (FileObject %x Flags %x)\n", FCB, FileObject, ConnectInfo->EndpointFlags));

    RtlZeroMemory( FCB, sizeof( *FCB ) );

    FCB->Flags = ConnectInfo->EndpointFlags;
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

    AFD_DbgPrint(MID_TRACE,("%x: Checking command channel\n", FCB));

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
    FileObject->FsContext = FCB;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_SUCCESS;
}

VOID DestroySocket( PAFD_FCB FCB ) {
    UINT i;
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
	SocketStateUnlock( FCB );
	return;
    }

    FCB->PollState |= AFD_EVENT_CLOSE;
    PollReeval( FCB->DeviceExt, FCB->FileObject ); 
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

NTSTATUS STDCALL
AfdCloseSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,
		 ("AfdClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp));
    
    AFD_DbgPrint(MID_TRACE,("FCB %x\n", FCB));

    FileObject->FsContext = NULL;
    DestroySocket( FCB );
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    AFD_DbgPrint(MID_TRACE, ("Returning success.\n"));

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
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
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_WAIT_FOR_LISTEN\n"));
  case IOCTL_AFD_ACCEPT:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_ACCEPT\n"));
  case IOCTL_AFD_DISCONNECT:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_DISCONNECT\n"));
  case IOCTL_AFD_GET_TDI_HANDLES:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_TDI_HANDLES\n"));
  case IOCTL_AFD_SET_INFO:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_INFO\n"));
  case IOCTL_AFD_SET_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_DATA\n"));
  case IOCTL_AFD_SET_CONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_OPTIONS\n"));
  case IOCTL_AFD_SET_DISCONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_DATA\n"));
  case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_OPTIONS\n"));
  case IOCTL_AFD_GET_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_CONNECT_DATA\n"));
  case IOCTL_AFD_GET_CONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_CONNECT_OPTIONS\n"));
  case IOCTL_AFD_GET_DISCONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_DISCONNECT_DATA\n"));
  case IOCTL_AFD_GET_DISCONNECT_OPTIONS:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_DISCONNECT_OPTIONS\n"));
  case IOCTL_AFD_SET_CONNECT_DATA_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_DATA_SIZE\n"));
  case IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE\n"));
  case IOCTL_AFD_SET_DISCONNECT_DATA_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_DATA_SIZE\n"));
  case IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE\n"));
  case IOCTL_AFD_EVENT_SELECT:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_EVENT_SELECT\n"));
  case IOCTL_AFD_DEFER_ACCEPT:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_DEFER_ACCEPT\n"));
  case IOCTL_AFD_GET_PENDING_CONNECT_DATA:
	    AFD_DbgPrint(MIN_TRACE, ("IOCTL_AFD_GET_PENDING_CONNECT_DATA\n"));
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

VOID STDCALL
AfdUnload(PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS STDCALL 
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING wstrDeviceName;
    PAFD_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    /* register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AfdDispatch;
    DriverObject->DriverUnload = AfdUnload;

    /* create afd device */
    RtlRosInitUnicodeStringFromLiteral(&wstrDeviceName, L"\\Device\\Afd");

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
