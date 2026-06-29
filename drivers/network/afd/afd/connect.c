/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/connect.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include "afd.h"

NTSTATUS
NTAPI
AfdGetConnectOptions(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                     PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectOptionsSize == 0)
    {
        AFD_DbgPrint(MIN_TRACE,("Invalid parameter\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);
    }

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
    PVOID ConnectOptions = LockRequest(Irp, IrpSp, FALSE, NULL);
    UINT ConnectOptionsSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (!ConnectOptions)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    if (FCB->ConnectOptions)
    {
        ExFreePoolWithTag(FCB->ConnectOptions, TAG_AFD_CONNECT_OPTIONS);
        FCB->ConnectOptions = NULL;
        FCB->ConnectOptionsSize = 0;
        FCB->FilledConnectOptions = 0;
    }

    FCB->ConnectOptions = ExAllocatePoolWithTag(PagedPool,
                                                ConnectOptionsSize,
                                                TAG_AFD_CONNECT_OPTIONS);

    if (!FCB->ConnectOptions)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

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
    PUINT ConnectOptionsSize = LockRequest(Irp, IrpSp, FALSE, NULL);
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (!ConnectOptionsSize)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    if (BufferSize < sizeof(UINT))
    {
        AFD_DbgPrint(MIN_TRACE,("Buffer too small\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);
    }

    if (FCB->ConnectOptions)
    {
        ExFreePoolWithTag(FCB->ConnectOptions, TAG_AFD_CONNECT_OPTIONS);
        FCB->ConnectOptionsSize = 0;
        FCB->FilledConnectOptions = 0;
    }

    FCB->ConnectOptions = ExAllocatePoolWithTag(PagedPool,
                                                *ConnectOptionsSize,
                                                TAG_AFD_CONNECT_OPTIONS);

    if (!FCB->ConnectOptions) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->ConnectOptionsSize = *ConnectOptionsSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}

NTSTATUS
NTAPI
AfdGetConnectData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (FCB->ConnectDataSize == 0)
    {
        AFD_DbgPrint(MIN_TRACE,("Invalid parameter\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);
    }

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
    PVOID ConnectData = LockRequest(Irp, IrpSp, FALSE, NULL);
    UINT ConnectDataSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (!ConnectData)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    if (FCB->ConnectData)
    {
        ExFreePoolWithTag(FCB->ConnectData, TAG_AFD_CONNECT_DATA);
        FCB->ConnectData = NULL;
        FCB->ConnectDataSize = 0;
        FCB->FilledConnectData = 0;
    }

    FCB->ConnectData = ExAllocatePoolWithTag(PagedPool,
                                             ConnectDataSize,
                                             TAG_AFD_CONNECT_DATA);

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
    PUINT ConnectDataSize = LockRequest(Irp, IrpSp, FALSE, NULL);
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB)) return LostSocket(Irp);

    if (!ConnectDataSize)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    if (BufferSize < sizeof(UINT))
    {
        AFD_DbgPrint(MIN_TRACE,("Buffer too small\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_BUFFER_TOO_SMALL, Irp, 0);
    }

    if (FCB->ConnectData)
    {
        ExFreePoolWithTag(FCB->ConnectData, TAG_AFD_CONNECT_DATA);
        FCB->ConnectDataSize = 0;
        FCB->FilledConnectData = 0;
    }

    FCB->ConnectData = ExAllocatePoolWithTag(PagedPool,
                                             *ConnectDataSize,
                                             TAG_AFD_CONNECT_DATA);

    if (!FCB->ConnectData) return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    FCB->ConnectDataSize = *ConnectDataSize;

    return UnlockAndMaybeComplete(FCB, STATUS_SUCCESS, Irp, 0);
}


NTSTATUS
WarmSocketForConnection(PAFD_FCB FCB) {
    NTSTATUS Status;

    if( !FCB->TdiDeviceName.Length || !FCB->TdiDeviceName.Buffer ) {
        AFD_DbgPrint(MIN_TRACE,("Null Device\n"));
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

NTSTATUS
MakeSocketIntoConnection(PAFD_FCB FCB) {
    NTSTATUS Status;

    ASSERT(!FCB->Recv.Window);
    ASSERT(!FCB->Send.Window);

    if (!FCB->Recv.Size)
    {
        Status = TdiQueryMaxDatagramLength(FCB->Connection.Object,
                                           &FCB->Recv.Size);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    if (!FCB->Send.Size)
    {
        Status = TdiQueryMaxDatagramLength(FCB->Connection.Object,
                                           &FCB->Send.Size);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Allocate the receive area and start receiving */
    if (!FCB->Recv.Window)
    {
        FCB->Recv.Window = ExAllocatePoolWithTag(PagedPool,
                                                 FCB->Recv.Size,
                                                 TAG_AFD_DATA_BUFFER);

        if( !FCB->Recv.Window ) return STATUS_NO_MEMORY;
    }

    if (!FCB->Send.Window)
    {
        FCB->Send.Window = ExAllocatePoolWithTag(PagedPool,
                                                 FCB->Send.Size,
                                                 TAG_AFD_DATA_BUFFER);

        if( !FCB->Send.Window ) return STATUS_NO_MEMORY;
    }

    FCB->SharedData.State = SOCKET_STATE_CONNECTED;
    FCB->SharedData.ConnectTime = 0; // Not used

    Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
                         FCB->Connection.Object,
                         TDI_RECEIVE_NORMAL,
                         FCB->Recv.Window,
                         FCB->Recv.Size,
                         ReceiveComplete,
                         FCB );

   if( Status == STATUS_PENDING ) Status = STATUS_SUCCESS;

   FCB->PollState |= AFD_EVENT_CONNECT | AFD_EVENT_SEND;
   FCB->PollStatus[FD_CONNECT_BIT] = STATUS_SUCCESS;
   FCB->PollStatus[FD_WRITE_BIT] = STATUS_SUCCESS;
   PollReeval( FCB->DeviceExt, FCB->FileObject );

   return Status;
}

static IO_COMPLETION_ROUTINE StreamSocketConnectSendComplete;

static
NTSTATUS
NTAPI
StreamSocketConnectSendComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;

    if (!SocketAcquireStateLock(FCB))
        return STATUS_FILE_CLOSED;

    ASSERT(FCB->SendIrp.InFlightRequest == Irp);
    FCB->SendIrp.InFlightRequest = NULL;

    while (!IsListEmpty(&FCB->PendingIrpList[FUNCTION_CONNECTEX]))
    {
        NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECTEX]);
        NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);

        AFD_DbgPrint(MID_TRACE,("Completing connectex %p\n", NextIrp));
        NextIrp->IoStatus.Status = Irp->IoStatus.Status;
        NextIrp->IoStatus.Information = Irp->IoStatus.Information;
        if (NextIrp->MdlAddress)
            UnlockRequest(NextIrp, IoGetCurrentIrpStackLocation(NextIrp));
        (void)IoSetCancelRoutine(NextIrp, NULL);
        IoCompleteRequest(NextIrp, IO_NETWORK_INCREMENT);
    }

    if (FCB->OnConnectSendBuffer)
    {
        ExFreePoolWithTag(FCB->OnConnectSendBuffer, TAG_AFD_SUPER_CONNECT_BUFFER);
        FCB->OnConnectSendBuffer = NULL;
        FCB->OnConnectSendBufferSize = 0;
    }
    
    SocketStateUnlock(FCB);
    
    return STATUS_SUCCESS;
}

static IO_COMPLETION_ROUTINE StreamSocketConnectComplete;
static
NTSTATUS
NTAPI
StreamSocketConnectComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                            PVOID Context) {
    NTSTATUS Status = Irp->IoStatus.Status;
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;

    AFD_DbgPrint(MID_TRACE,("Called: FCB %p, FO %p\n",
                            Context, FCB->FileObject));

    /* I was wrong about this before as we can have pending writes to a not
     * yet connected socket */
    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    AFD_DbgPrint(MID_TRACE,("Irp->IoStatus.Status = %x\n",
                            Irp->IoStatus.Status));

    ASSERT(FCB->ConnectIrp.InFlightRequest == Irp);
    FCB->ConnectIrp.InFlightRequest = NULL;

    if( FCB->SharedData.State == SOCKET_STATE_CLOSED ) {
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
        while (!IsListEmpty(&FCB->PendingIrpList[FUNCTION_CONNECTEX]))
        {
               NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECTEX]);
               NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
               NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
               NextIrp->IoStatus.Information = Irp->IoStatus.Information;
               if (NextIrp->MdlAddress)
                   UnlockRequest(NextIrp, IoGetCurrentIrpStackLocation(NextIrp));
               (void)IoSetCancelRoutine(NextIrp, NULL);
               IoCompleteRequest(NextIrp, IO_NETWORK_INCREMENT);
        }
        SocketStateUnlock( FCB );
        return STATUS_FILE_CLOSED;
    }

    if( !NT_SUCCESS(Irp->IoStatus.Status) ) {
        FCB->PollState |= AFD_EVENT_CONNECT_FAIL;
        FCB->PollStatus[FD_CONNECT_BIT] = Irp->IoStatus.Status;
        AFD_DbgPrint(MID_TRACE,("Going to bound state\n"));
        FCB->SharedData.State = SOCKET_STATE_BOUND;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }

    /* Succeed pending irps on the FUNCTION_CONNECT list */
    while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_CONNECT] ) ) {
        NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECT]);
        NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
        AFD_DbgPrint(MID_TRACE,("Completing connect %p\n", NextIrp));
        NextIrp->IoStatus.Status = Status;
        NextIrp->IoStatus.Information = NT_SUCCESS(Status) ? ((ULONG_PTR)FCB->Connection.Handle) : 0;
        if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
        (void)IoSetCancelRoutine(NextIrp, NULL);
        IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
    }

    if( NT_SUCCESS(Status) ) {
        Status = MakeSocketIntoConnection( FCB );

        if( !NT_SUCCESS(Status) ) {
            goto end;
        }

        FCB->FilledConnectData = MIN(FCB->ConnectReturnInfo->UserDataLength, FCB->ConnectDataSize);
        if (FCB->FilledConnectData)
        {
            RtlCopyMemory(FCB->ConnectData,
                          FCB->ConnectReturnInfo->UserData,
                          FCB->FilledConnectData);
        }

        FCB->FilledConnectOptions = MIN(FCB->ConnectReturnInfo->OptionsLength, FCB->ConnectOptionsSize);
        if (FCB->FilledConnectOptions)
        {
            RtlCopyMemory(FCB->ConnectOptions,
                          FCB->ConnectReturnInfo->Options,
                          FCB->FilledConnectOptions);
        }

        if( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_SEND] ) ) {
            NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_SEND]);
            NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP,
                                        Tail.Overlay.ListEntry);
            AFD_DbgPrint(MID_TRACE,("Launching send request %p\n", NextIrp));
            Status = AfdConnectedSocketWriteData
                ( DeviceObject,
                  NextIrp,
                  IoGetCurrentIrpStackLocation( NextIrp ),
                  FALSE );
        }

        if( Status == STATUS_PENDING )
            Status = STATUS_SUCCESS;

        if (FCB->OnConnectSendBuffer)
        {
            PFILE_OBJECT object = FCB->Connection.Object;
            PVOID sendBuffer = FCB->OnConnectSendBuffer;
            UINT sendBufferLength = FCB->OnConnectSendBufferSize;
            SocketStateUnlock(FCB);

            Status = TdiSend(&FCB->SendIrp.InFlightRequest,
                             object,
                             AFD_OVERLAPPED,
                             sendBuffer,
                             sendBufferLength,
                             StreamSocketConnectSendComplete,
                             FCB);

            return Status == STATUS_PENDING ? STATUS_SUCCESS : Status;
        }
    }

end:
    while (!IsListEmpty(&FCB->PendingIrpList[FUNCTION_CONNECTEX]))
    {
        NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECTEX]);
        NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);

        AFD_DbgPrint(MID_TRACE,("Completing connectex %p\n", NextIrp));
        NextIrp->IoStatus.Status = Status;
        NextIrp->IoStatus.Information = 0;
        if (NextIrp->MdlAddress)
            UnlockRequest(NextIrp, IoGetCurrentIrpStackLocation(NextIrp));
        (void)IoSetCancelRoutine(NextIrp, NULL);
        IoCompleteRequest(NextIrp, IO_NETWORK_INCREMENT);
    }
    SocketStateUnlock( FCB );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

/* Return the socket object for ths request only if it is a connected or
   stream type. */
NTSTATUS
NTAPI
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                       PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_CONNECT_INFO ConnectReq;
    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    UNREFERENCED_PARAMETER(DeviceObject);

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );
    if( !(ConnectReq = LockRequest( Irp, IrpSp, FALSE, NULL )) )
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
        if (FCB->RemoteAddress)
        {
            ExFreePoolWithTag(FCB->RemoteAddress, TAG_AFD_TRANSPORT_ADDRESS);
        }

        FCB->RemoteAddress =
            TaCopyTransportAddress( &ConnectReq->RemoteAddress );

        if( !FCB->RemoteAddress )
            Status = STATUS_NO_MEMORY;
        else
            Status = STATUS_SUCCESS;

        return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
   }

    switch(FCB->SharedData.State)
    {
    case SOCKET_STATE_CONNECTED:
        Status = STATUS_SUCCESS;
        break;

    case SOCKET_STATE_CONNECTING:
        return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECT );

    case SOCKET_STATE_CREATED:
        if (FCB->LocalAddress)
        {
            ExFreePoolWithTag(FCB->LocalAddress, TAG_AFD_TRANSPORT_ADDRESS);
        }

        FCB->LocalAddress =
            TaBuildNullTransportAddress( ConnectReq->RemoteAddress.Address[0].AddressType );

        if( FCB->LocalAddress ) {
            Status = WarmSocketForBind( FCB, AFD_SHARE_WILDCARD );

            if( NT_SUCCESS(Status) )
                FCB->SharedData.State = SOCKET_STATE_BOUND;
            else
                return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
        } else
            return UnlockAndMaybeComplete
                ( FCB, STATUS_NO_MEMORY, Irp, 0 );

    /* Drop through to SOCKET_STATE_BOUND */

    case SOCKET_STATE_BOUND:
        if (FCB->RemoteAddress)
        {
            ExFreePoolWithTag(FCB->RemoteAddress, TAG_AFD_TRANSPORT_ADDRESS);
        }

        FCB->RemoteAddress =
            TaCopyTransportAddress( &ConnectReq->RemoteAddress );

        if( !FCB->RemoteAddress ) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        Status = WarmSocketForConnection( FCB );

        if( !NT_SUCCESS(Status) )
            break;

        if (FCB->ConnectReturnInfo)
        {
            ExFreePoolWithTag(FCB->ConnectReturnInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
        }

        Status = TdiBuildConnectionInfo
            ( &FCB->ConnectReturnInfo,
              &ConnectReq->RemoteAddress );

        if( NT_SUCCESS(Status) )
        {
            if (FCB->ConnectCallInfo)
            {
                ExFreePoolWithTag(FCB->ConnectCallInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
            }

            Status = TdiBuildConnectionInfo(&FCB->ConnectCallInfo,
                                              &ConnectReq->RemoteAddress);
        }
        else break;


        if( NT_SUCCESS(Status) ) {
            FCB->ConnectCallInfo->UserData = FCB->ConnectData;
            FCB->ConnectCallInfo->UserDataLength = FCB->ConnectDataSize;
            FCB->ConnectCallInfo->Options = FCB->ConnectOptions;
            FCB->ConnectCallInfo->OptionsLength = FCB->ConnectOptionsSize;

            FCB->SharedData.State = SOCKET_STATE_CONNECTING;

            AFD_DbgPrint(MID_TRACE,("Queueing IRP %p\n", Irp));
            Status = QueueUserModeIrp(FCB, Irp, FUNCTION_CONNECT);
            if (Status == STATUS_PENDING)
            {
                Status = TdiConnect(&FCB->ConnectIrp.InFlightRequest,
                                    FCB->Connection.Object,
                                    FCB->ConnectCallInfo,
                                    FCB->ConnectReturnInfo,
                                    StreamSocketConnectComplete,
                                    FCB);
            }

            if (Status != STATUS_PENDING)
                FCB->SharedData.State = SOCKET_STATE_BOUND;

            SocketStateUnlock(FCB);

            return Status;
        }
        break;

    default:
        AFD_DbgPrint(MIN_TRACE,("Inappropriate socket state %u for connect\n",
                                FCB->SharedData.State));
        break;
    }

    return UnlockAndMaybeComplete(FCB, Status, Irp, 0);
}

/* Return the socket object for ths request only if it is a connected or stream type. */
NTSTATUS
NTAPI
AfdStreamSocketSuperConnect(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_SUPER_CONNECT_INFO ConnectReq;
    KPROCESSOR_MODE LockMode;
    UINT BufferSize = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!SocketAcquireStateLock(FCB))
        return LostSocket(Irp);

    ConnectReq = LockRequest(Irp, IrpSp, FALSE, &LockMode);
    if (!ConnectReq)
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    AFD_DbgPrint(MID_TRACE,("Connect request:\n"));
#if DBG
    OskitDumpBuffer((PCHAR)ConnectReq,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength);
#endif

    if (FCB->OnConnectSendBuffer)
    {
        ExFreePoolWithTag(FCB->OnConnectSendBuffer, TAG_AFD_SUPER_CONNECT_BUFFER);
        FCB->OnConnectSendBuffer = NULL;
        FCB->OnConnectSendBufferSize = 0;
    }

    if (Irp->UserBuffer)
    {
         FCB->OnConnectSendBufferSize = BufferSize;
         FCB->OnConnectSendBuffer = ExAllocatePoolWithTag(PagedPool,
                                                        BufferSize,
                                                        TAG_AFD_SUPER_CONNECT_BUFFER);

         if (!FCB->OnConnectSendBuffer)
             return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

         RtlCopyMemory(FCB->OnConnectSendBuffer, Irp->UserBuffer, BufferSize);
    }

    if (FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS)
    {
        AFD_DbgPrint(MIN_TRACE,("Cannot call IOCTL_CONNECTEX on connectionless socket\n",
                                 FCB->SharedData.State));
         if (FCB->RemoteAddress)
         {
             ExFreePoolWithTag(FCB->RemoteAddress, TAG_AFD_TRANSPORT_ADDRESS);
         }

         FCB->RemoteAddress =
             TaCopyTransportAddress(&ConnectReq->RemoteAddress);

         if(!FCB->RemoteAddress)
             Status = STATUS_NO_MEMORY;
         else
             Status = STATUS_SUCCESS;

         return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
    }

    switch (FCB->SharedData.State)
    {
    case SOCKET_STATE_CONNECTED:
        Status = STATUS_SUCCESS;
        break;

    case SOCKET_STATE_CONNECTING:
        return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECTEX );

    case SOCKET_STATE_BOUND:
        if (FCB->RemoteAddress)
        {
            ExFreePoolWithTag(FCB->RemoteAddress, TAG_AFD_TRANSPORT_ADDRESS);
        }

        FCB->RemoteAddress =
            TaCopyTransportAddress(&ConnectReq->RemoteAddress);
        if (!FCB->RemoteAddress)
        {
            Status = STATUS_NO_MEMORY;
            break;
        }

        Status = WarmSocketForConnection(FCB);
        if (!NT_SUCCESS(Status))
            break;

        if (FCB->ConnectReturnInfo)
        {
            ExFreePoolWithTag(FCB->ConnectReturnInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
        }

        Status = TdiBuildConnectionInfo(&FCB->ConnectReturnInfo,
                                        &ConnectReq->RemoteAddress);
        if (!NT_SUCCESS(Status))
            break;

        if (FCB->ConnectCallInfo)
        {
            ExFreePoolWithTag(FCB->ConnectCallInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
        }

        Status = TdiBuildConnectionInfo(&FCB->ConnectCallInfo,
                                        &ConnectReq->RemoteAddress);


        if (!NT_SUCCESS(Status))
            break;

        FCB->ConnectCallInfo->UserData = FCB->ConnectData ?  FCB->ConnectData : FCB->OnConnectSendBuffer;
        FCB->ConnectCallInfo->UserDataLength = FCB->ConnectData ?  FCB->ConnectDataSize : FCB->OnConnectSendBufferSize;
        FCB->ConnectCallInfo->Options = FCB->ConnectOptions;
        FCB->ConnectCallInfo->OptionsLength = FCB->ConnectOptionsSize;
        FCB->SharedData.State = SOCKET_STATE_CONNECTING;

        AFD_DbgPrint(MID_TRACE,("Queueing IRP %p\n", Irp));
        Status = QueueUserModeIrp( FCB, Irp, FUNCTION_CONNECTEX );
        if (Status == STATUS_PENDING)
        {
            Status = TdiConnect(&FCB->ConnectIrp.InFlightRequest,
                                FCB->Connection.Object,
                                FCB->ConnectCallInfo,
                                FCB->ConnectReturnInfo,
                                StreamSocketConnectComplete,
                                FCB );
        }

        if (Status != STATUS_PENDING)
            FCB->SharedData.State = SOCKET_STATE_BOUND;

        SocketStateUnlock(FCB);

        return Status;

    default:
        AFD_DbgPrint(MIN_TRACE,("Inappropriate socket state %u for connect\n",
                                FCB->SharedData.State));
        break;
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}
