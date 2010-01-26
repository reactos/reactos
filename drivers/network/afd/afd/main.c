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

#if DBG

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

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

NTSTATUS NTAPI
AfdGetDisconnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	          PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->DisconnectOptionsSize == 0)
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);

    ASSERT(FCB->DisconnectOptions);

    if (FCB->FilledDisconnectOptions < BufferSize) BufferSize = FCB->FilledDisconnectOptions;

    RtlCopyMemory(Irp->UserBuffer,
                  FCB->DisconnectOptions,
                  BufferSize);

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, BufferSize);
}

NTSTATUS
NTAPI
AfdSetDisconnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PVOID DisconnectOptions = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT DisconnectOptionsSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->DisconnectOptions)
    {
        ExFreePool(FCB->DisconnectOptions);
        FCB->DisconnectOptions = NULL;
        FCB->DisconnectOptionsSize = 0;
        FCB->FilledDisconnectOptions = 0;
    }

    FCB->DisconnectOptions = ExAllocatePool(PagedPool, DisconnectOptionsSize);
    if (!FCB->DisconnectOptions) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    RtlCopyMemory(FCB->DisconnectOptions,
                  DisconnectOptions,
                  DisconnectOptionsSize);

    FCB->DisconnectOptionsSize = DisconnectOptionsSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS
NTAPI
AfdSetDisconnectOptionsSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PUINT DisconnectOptionsSize = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (BufferSize < sizeof(UINT))
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);

    if (FCB->DisconnectOptions)
    {
        ExFreePool(FCB->DisconnectOptions);
        FCB->DisconnectOptionsSize = 0;
        FCB->FilledDisconnectOptions = 0;
    }

    FCB->DisconnectOptions = ExAllocatePool(PagedPool, *DisconnectOptionsSize);
    if (!FCB->DisconnectOptions) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->DisconnectOptionsSize = *DisconnectOptionsSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS NTAPI
AfdGetDisconnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	          PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->DisconnectDataSize == 0)
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);

    ASSERT(FCB->DisconnectData);

    if (FCB->FilledDisconnectData < BufferSize) BufferSize = FCB->FilledDisconnectData;

    RtlCopyMemory(Irp->UserBuffer,
                  FCB->DisconnectData,
                  BufferSize);

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, BufferSize);
}

NTSTATUS
NTAPI
AfdSetDisconnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PVOID DisconnectData = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT DisconnectDataSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->DisconnectData)
    {
        ExFreePool(FCB->DisconnectData);
        FCB->DisconnectData = NULL;
        FCB->DisconnectDataSize = 0;
        FCB->FilledDisconnectData = 0;
    }

    FCB->DisconnectData = ExAllocatePool(PagedPool, DisconnectDataSize);
    if (!FCB->DisconnectData) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    RtlCopyMemory(FCB->DisconnectData,
                  DisconnectData,
                  DisconnectDataSize);

    FCB->DisconnectDataSize = DisconnectDataSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS
NTAPI
AfdSetDisconnectDataSize(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PUINT DisconnectDataSize = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (BufferSize < sizeof(UINT))
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);

    if (FCB->DisconnectData)
    {
        ExFreePool(FCB->DisconnectData);
        FCB->DisconnectDataSize = 0;
        FCB->FilledDisconnectData = 0;
    }

    FCB->DisconnectData = ExAllocatePool(PagedPool, *DisconnectDataSize);
    if (!FCB->DisconnectData) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->DisconnectDataSize = *DisconnectDataSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

static NTSTATUS NTAPI
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
    NTSTATUS Status = STATUS_SUCCESS;

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
    FCB->GroupID = ConnectInfo ? ConnectInfo->GroupID : 0;
    FCB->GroupType = 0; /* FIXME */
    FCB->State = SOCKET_STATE_CREATED;
    FCB->FileObject = FileObject;
    FCB->DeviceExt = DeviceExt;
    FCB->AddressFile.Handle = INVALID_HANDLE_VALUE;
    FCB->Connection.Handle = INVALID_HANDLE_VALUE;

    KeInitializeMutex( &FCB->Mutex, 0 );

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

	if( !FCB->TdiDeviceName.Buffer ) {
	    ExFreePool(FCB);
	    AFD_DbgPrint(MID_TRACE,("Could not copy target string\n"));
	    Irp->IoStatus.Status = STATUS_NO_MEMORY;
	    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	    return STATUS_NO_MEMORY;
	}

	RtlCopyMemory( FCB->TdiDeviceName.Buffer,
		       ConnectInfo->TransportName,
		       FCB->TdiDeviceName.Length );

	AFD_DbgPrint(MID_TRACE,("Success: %s %wZ\n",
				EaInfo->EaName, &FCB->TdiDeviceName));
    } else {
	AFD_DbgPrint(MID_TRACE,("Success: Control connection\n"));
    }

    FileObject->FsContext = FCB;

    /* It seems that UDP sockets are writable from inception */
    if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS ) {
        AFD_DbgPrint(MID_TRACE,("Packet oriented socket\n"));
        
	/* A datagram socket is always sendable */
	FCB->PollState |= AFD_EVENT_SEND;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }

    if( !NT_SUCCESS(Status) ) {
	if( FCB->TdiDeviceName.Buffer ) ExFreePool( FCB->TdiDeviceName.Buffer );
	ExFreePool( FCB );
	FileObject->FsContext = NULL;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return Status;
}

static NTSTATUS NTAPI
AfdCleanupSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                 PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PLIST_ENTRY CurrentEntry, NextEntry;
    UINT Function;
    PIRP CurrentIrp;

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket(Irp);

    for (Function = 0; Function < MAX_FUNCTIONS; Function++)
    {
        CurrentEntry = FCB->PendingIrpList[Function].Flink;
        while (CurrentEntry != &FCB->PendingIrpList[Function])
        {
           NextEntry = CurrentEntry->Flink;
           CurrentIrp = CONTAINING_RECORD(CurrentEntry, IRP, Tail.Overlay.ListEntry);

           /* The cancel routine will remove the IRP from the list */
           IoCancelIrp(CurrentIrp);

           CurrentEntry = NextEntry;
        }
    }

    KillSelectsForFCB( FCB->DeviceExt, FileObject, FALSE );

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

static NTSTATUS NTAPI
AfdCloseSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT i;
    PAFD_IN_FLIGHT_REQUEST InFlightRequest[IN_FLIGHT_REQUESTS];

    AFD_DbgPrint(MID_TRACE,
		 ("AfdClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp));

    if( !SocketAcquireStateLock( FCB ) ) return STATUS_FILE_CLOSED;

    FCB->State = SOCKET_STATE_CLOSED;
    FCB->PollState = AFD_EVENT_CLOSE;
    PollReeval( FCB->DeviceExt, FCB->FileObject );

    InFlightRequest[0] = &FCB->ListenIrp;
    InFlightRequest[1] = &FCB->ReceiveIrp;
    InFlightRequest[2] = &FCB->SendIrp;
    InFlightRequest[3] = &FCB->ConnectIrp;

    /* Cancel our pending requests */
    for( i = 0; i < IN_FLIGHT_REQUESTS; i++ ) {
	if( InFlightRequest[i]->InFlightRequest ) {
	    AFD_DbgPrint(MID_TRACE,("Cancelling in flight irp %d (%x)\n",
				    i, InFlightRequest[i]->InFlightRequest));
            IoCancelIrp(InFlightRequest[i]->InFlightRequest);
	}
    }

    KillSelectsForFCB( FCB->DeviceExt, FileObject, FALSE );

    SocketStateUnlock( FCB );

    if( FCB->EventSelect )
        ObDereferenceObject( FCB->EventSelect );

    if( FCB->Context )
        ExFreePool( FCB->Context );

    if( FCB->Recv.Window )
	ExFreePool( FCB->Recv.Window );

    if( FCB->Send.Window )
	ExFreePool( FCB->Send.Window );

    if( FCB->AddressFrom )
	ExFreePool( FCB->AddressFrom );

    if( FCB->ConnectInfo )
        ExFreePool( FCB->ConnectInfo );

    if( FCB->ConnectData )
        ExFreePool( FCB->ConnectData );

    if( FCB->DisconnectData )
        ExFreePool( FCB->DisconnectData );

    if( FCB->ConnectOptions )
        ExFreePool( FCB->ConnectOptions );

    if( FCB->DisconnectOptions )
        ExFreePool( FCB->DisconnectOptions );

    if( FCB->LocalAddress )
	ExFreePool( FCB->LocalAddress );

    if( FCB->RemoteAddress )
	ExFreePool( FCB->RemoteAddress );

    if( FCB->Connection.Object )
	ObDereferenceObject(FCB->Connection.Object);

    if( FCB->AddressFile.Object )
	ObDereferenceObject(FCB->AddressFile.Object);

    if( FCB->AddressFile.Handle != INVALID_HANDLE_VALUE )
    {
        if (ZwClose(FCB->AddressFile.Handle) == STATUS_INVALID_HANDLE)
        {
            DbgPrint("INVALID ADDRESS FILE HANDLE VALUE: %x %x\n", FCB->AddressFile.Handle, FCB->AddressFile.Object);
        }
    }

    if( FCB->Connection.Handle != INVALID_HANDLE_VALUE )
    {
        if (ZwClose(FCB->Connection.Handle) == STATUS_INVALID_HANDLE)
        {
            DbgPrint("INVALID CONNECTION HANDLE VALUE: %x %x\n", FCB->Connection.Handle, FCB->Connection.Object);
        }
    }

    if( FCB->TdiDeviceName.Buffer )
	ExFreePool(FCB->TdiDeviceName.Buffer);

    ExFreePool(FCB);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    AFD_DbgPrint(MID_TRACE, ("Returning success.\n"));

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
AfdDisconnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_DISCONNECT_INFO DisReq;
    IO_STATUS_BLOCK Iosb;
    PTDI_CONNECTION_INFORMATION ConnectionReturnInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Flags = 0;

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !(DisReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
				       Irp, 0 );

    if (!(FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS))
    {
        if( !FCB->ConnectInfo )
            return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER,
                                           Irp, 0 );

        ASSERT(FCB->RemoteAddress);

        Status = TdiBuildNullConnectionInfo
	       ( &ConnectionReturnInfo, FCB->RemoteAddress->Address[0].AddressType );

        if( !NT_SUCCESS(Status) )
	    return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
				           Irp, 0 );

        if( DisReq->DisconnectType & AFD_DISCONNECT_SEND )
	    Flags |= TDI_DISCONNECT_RELEASE;
        if( DisReq->DisconnectType & AFD_DISCONNECT_RECV ||
	    DisReq->DisconnectType & AFD_DISCONNECT_ABORT )
	    Flags |= TDI_DISCONNECT_ABORT;

        FCB->ConnectInfo->UserData = FCB->DisconnectData;
        FCB->ConnectInfo->UserDataLength = FCB->DisconnectDataSize;
        FCB->ConnectInfo->Options = FCB->DisconnectOptions;
        FCB->ConnectInfo->OptionsLength = FCB->DisconnectOptionsSize;

        Status = TdiDisconnect( FCB->Connection.Object,
			        &DisReq->Timeout,
			        Flags,
			        &Iosb,
			        NULL,
			        NULL,
			        FCB->ConnectInfo,
			        ConnectionReturnInfo);

        if (NT_SUCCESS(Status)) {
            FCB->FilledDisconnectData = MIN(FCB->DisconnectDataSize, ConnectionReturnInfo->UserDataLength);
            if (FCB->FilledDisconnectData)
            {
                RtlCopyMemory(FCB->DisconnectData,
                              ConnectionReturnInfo->UserData,
                              FCB->FilledDisconnectData);
            }

            FCB->FilledDisconnectOptions = MIN(FCB->DisconnectOptionsSize, ConnectionReturnInfo->OptionsLength);
            if (FCB->FilledDisconnectOptions)
            {
                RtlCopyMemory(FCB->DisconnectOptions,
                              ConnectionReturnInfo->Options,
                              FCB->FilledDisconnectOptions);
            }
        }

        ExFreePool( ConnectionReturnInfo );

        FCB->PollState |= AFD_EVENT_DISCONNECT;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else
        Status = STATUS_INVALID_PARAMETER;

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}

static NTSTATUS NTAPI
AfdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
#if DBG
    PFILE_OBJECT FileObject = IrpSp->FileObject;
#endif

    AFD_DbgPrint(MID_TRACE,("AfdDispatch: %d\n", IrpSp->MajorFunction));
    if( IrpSp->MajorFunction != IRP_MJ_CREATE) {
	AFD_DbgPrint(MID_TRACE,("FO %x, IrpSp->FO %x\n",
				FileObject, IrpSp->FileObject));
	ASSERT(FileObject == IrpSp->FileObject);
    }

    Irp->IoStatus.Information = 0;

    switch(IrpSp->MajorFunction)
    {
	/* opening and closing handles to the device */
    case IRP_MJ_CREATE:
	/* Mostly borrowed from the named pipe file system */
	return AfdCreateSocket(DeviceObject, Irp, IrpSp);

    case IRP_MJ_CLOSE:
	/* Ditto the borrowing */
	return AfdCloseSocket(DeviceObject, Irp, IrpSp);

    case IRP_MJ_CLEANUP:
        return AfdCleanupSocket(DeviceObject, Irp, IrpSp);

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

	case IOCTL_AFD_SET_INFO:
	    return AfdSetInfo( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_GET_CONTEXT_SIZE:
	    return AfdGetContextSize( DeviceObject, Irp, IrpSp );

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
	    return AfdGetSockName( DeviceObject, Irp, IrpSp );

        case IOCTL_AFD_GET_PEER_NAME:
            return AfdGetPeerName( DeviceObject, Irp, IrpSp );

	case IOCTL_AFD_GET_CONNECT_DATA:
	    return AfdGetConnectData(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_CONNECT_DATA:
	    return AfdSetConnectData(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_DISCONNECT_DATA:
	    return AfdSetDisconnectData(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_GET_DISCONNECT_DATA:
	    return AfdGetDisconnectData(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_CONNECT_DATA_SIZE:
	    return AfdSetConnectDataSize(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_DISCONNECT_DATA_SIZE:
	    return AfdSetDisconnectDataSize(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_CONNECT_OPTIONS:
	    return AfdSetConnectOptions(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
	    return AfdSetDisconnectOptions(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_GET_CONNECT_OPTIONS:
	    return AfdGetConnectOptions(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_GET_DISCONNECT_OPTIONS:
	    return AfdGetDisconnectOptions(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE:
	    return AfdSetConnectOptionsSize(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE:
	    return AfdSetDisconnectOptionsSize(DeviceObject, Irp, IrpSp);

	case IOCTL_AFD_GET_TDI_HANDLES:
	    DbgPrint("IOCTL_AFD_GET_TDI_HANDLES is UNIMPLEMENTED!\n");
	    break;

	case IOCTL_AFD_DEFER_ACCEPT:
	    DbgPrint("IOCTL_AFD_DEFER_ACCEPT is UNIMPLEMENTED!\n");
	    break;

	case IOCTL_AFD_GET_PENDING_CONNECT_DATA:
	    DbgPrint("IOCTL_AFD_GET_PENDING_CONNECT_DATA is UNIMPLEMENTED!\n");
	    break;

	case IOCTL_AFD_VALIDATE_GROUP:
	    DbgPrint("IOCTL_AFD_VALIDATE_GROUP is UNIMPLEMENTED!\n");
	    break;

	default:
	    Status = STATUS_NOT_SUPPORTED;
	    DbgPrint("Unknown IOCTL (0x%x)\n",
		     IrpSp->Parameters.DeviceIoControl.IoControlCode);
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

VOID NTAPI
AfdCancelHandler(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT Function;
    PAFD_RECV_INFO RecvReq;
    PAFD_SEND_INFO SendReq;
    PLIST_ENTRY CurrentEntry;
    PIRP CurrentIrp;
    PAFD_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
    KIRQL OldIrql;
    PAFD_ACTIVE_POLL Poll;
    PAFD_POLL_INFO PollReq;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (!SocketAcquireStateLock(FCB))
        return;

    ASSERT(IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_AFD_RECV:
        RecvReq = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	UnlockBuffers(RecvReq->BufferArray, RecvReq->BufferCount, FALSE);
        /* Fall through */

        case IOCTL_AFD_RECV_DATAGRAM:
        Function = FUNCTION_RECV;
        break;

        case IOCTL_AFD_SEND:
        SendReq = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        UnlockBuffers(SendReq->BufferArray, SendReq->BufferCount, FALSE);
        /* Fall through */

        case IOCTL_AFD_SEND_DATAGRAM:
        Function = FUNCTION_SEND;
        break;

        case IOCTL_AFD_CONNECT:
        Function = FUNCTION_CONNECT;
        break;

        case IOCTL_AFD_WAIT_FOR_LISTEN:
        Function = FUNCTION_PREACCEPT;
        break;

        case IOCTL_AFD_SELECT:
        KeAcquireSpinLock(&DeviceExt->Lock, &OldIrql);

        CurrentEntry = DeviceExt->Polls.Flink;
        while (CurrentEntry != &DeviceExt->Polls)
        {
            Poll = CONTAINING_RECORD(CurrentEntry, AFD_ACTIVE_POLL, ListEntry);
            CurrentIrp = Poll->Irp;
            PollReq = CurrentIrp->AssociatedIrp.SystemBuffer;

            if (CurrentIrp == Irp)
            {
                ZeroEvents(PollReq->Handles, PollReq->HandleCount);
                SignalSocket(Poll, NULL, PollReq, STATUS_CANCELLED);
                break;
            }
            else
            {
                CurrentEntry = CurrentEntry->Flink;
            }
        }

        KeReleaseSpinLock(&DeviceExt->Lock, OldIrql);

        /* IRP already completed by SignalSocket */
        SocketStateUnlock(FCB);
        return;
            
        default:
        ASSERT(FALSE);
        UnlockAndMaybeComplete(FCB, STATUS_CANCELLED, Irp, 0);
        return;
    }

    CurrentEntry = FCB->PendingIrpList[Function].Flink;
    while (CurrentEntry != &FCB->PendingIrpList[Function])
    {
        CurrentIrp = CONTAINING_RECORD(CurrentEntry, IRP, Tail.Overlay.ListEntry);

        if (CurrentIrp == Irp)
        {
            RemoveEntryList(CurrentEntry);
            break;
        }
        else
        {
            CurrentEntry = CurrentEntry->Flink;
        }
    }
    
    UnlockAndMaybeComplete(FCB, STATUS_CANCELLED, Irp, 0);
}

static VOID NTAPI
AfdUnload(PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING wstrDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Afd");
    PAFD_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    /* register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = AfdDispatch;
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
