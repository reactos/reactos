/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/listen.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include "afd.h"
#include "ntdef.h"

static NTSTATUS SatisfyAccept(PAFD_DEVICE_EXTENSION DeviceExt,
                              PIRP Irp,
                              PFILE_OBJECT NewFileObject,
                              PAFD_TDI_OBJECT_QELT Qelt,
                              BOOL SuperAccept)
{
    PAFD_FCB FCB = NewFileObject->FsContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceExt);

    if (!SocketAcquireStateLock(FCB))
        return LostSocket( Irp );

    /* Transfer the connection to the new socket, launch the opening read */
    AFD_DbgPrint(MID_TRACE,("Completing a real accept (FCB %p)\n", FCB));

    FCB->Connection = Qelt->Object;

    if (FCB->RemoteAddress)
    {
        ExFreePoolWithTag(FCB->RemoteAddress, TAG_AFD_TRANSPORT_ADDRESS);
    }

    FCB->RemoteAddress =
        TaCopyTransportAddress( Qelt->ConnInfo->RemoteAddress );

    if( !FCB->RemoteAddress )
        Status = STATUS_NO_MEMORY;
    else
        Status = MakeSocketIntoConnection(FCB, !SuperAccept);

    if (NT_SUCCESS(Status))
        Status = TdiBuildConnectionInfo(&FCB->ConnectCallInfo, FCB->RemoteAddress);

    if (NT_SUCCESS(Status))
        Status = TdiBuildConnectionInfo(&FCB->ConnectReturnInfo, FCB->RemoteAddress);

    if (SuperAccept)
    {
        SocketStateUnlock(FCB);
        return Status;
    }

    return UnlockAndMaybeComplete(FCB, Status, Irp, 0);
}

static NTSTATUS SatisfyPreAccept( PIRP Irp, PAFD_TDI_OBJECT_QELT Qelt ) {
    PAFD_RECEIVED_ACCEPT_DATA ListenReceive =
        (PAFD_RECEIVED_ACCEPT_DATA)Irp->AssociatedIrp.SystemBuffer;
    PTA_IP_ADDRESS IPAddr;

    ListenReceive->SequenceNumber = Qelt->Seq;

    AFD_DbgPrint(MID_TRACE,("Giving SEQ %u to userland\n", Qelt->Seq));
    AFD_DbgPrint(MID_TRACE,("Socket Address (K) %p (U) %p\n",
                            &ListenReceive->Address,
                            Qelt->ConnInfo->RemoteAddress));

    TaCopyTransportAddressInPlace( &ListenReceive->Address,
                                   Qelt->ConnInfo->RemoteAddress );

    IPAddr = (PTA_IP_ADDRESS)&ListenReceive->Address;

    AFD_DbgPrint(MID_TRACE,("IPAddr->TAAddressCount %d\n",
                            IPAddr->TAAddressCount));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].AddressType %u\n",
                            IPAddr->Address[0].AddressType));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].AddressLength %u\n",
                            IPAddr->Address[0].AddressLength));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].Address[0].sin_port %x\n",
                            IPAddr->Address[0].Address[0].sin_port));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].Address[0].sin_addr %x\n",
                            IPAddr->Address[0].Address[0].in_addr));

    if( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );

    Irp->IoStatus.Information = ((PCHAR)&IPAddr[1]) - ((PCHAR)ListenReceive);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    (void)IoSetCancelRoutine(Irp, NULL);
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS SatisfySuperAccept(PAFD_FCB FCB, PIRP Irp, PAFD_TDI_OBJECT_QELT Qelt)
{
    PFILE_OBJECT NewFileObject = (PFILE_OBJECT)Irp->Tail.Overlay.DriverContext[2];
    PAFD_SUPER_ACCEPT_INFO AcceptInfo = (PAFD_SUPER_ACCEPT_INFO)Irp->Tail.Overlay.DriverContext[0]; /* LockRequest stores the request in index 0 */
    PAFD_FCB FCB2 = NewFileObject->FsContext;
    NTSTATUS Status = SatisfyAccept(NULL, Irp, NewFileObject, Qelt, TRUE);
    
    ObDereferenceObject(NewFileObject);
    Irp->Tail.Overlay.DriverContext[2] = NULL;

    BYTE *BufferPtr = MmGetSystemAddressForMdlSafe((PMDL)Irp->Tail.Overlay.DriverContext[3], NormalPagePriority);
    if (BufferPtr)
    {
        /* Query TCPIP to find the local address of the socket */
        LONG RequestSize = (FIELD_OFFSET(TDI_ADDRESS_INFO, Address.Address[0].Address) + FCB->LocalAddress->Address[0].AddressLength);
        PTRANSPORT_ADDRESS RemoteAddress = (PTRANSPORT_ADDRESS)Qelt->ConnInfo->RemoteAddress;
        PTDI_ADDRESS_INFO Buffer = ExAllocatePoolWithTag(NonPagedPool, RequestSize, 'bufT');
        if (Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        PMDL Mdl = IoAllocateMdl(Buffer, RequestSize, FALSE, FALSE, NULL);
        if (Mdl == NULL)
        {
            ExFreePool(Buffer);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        /* TCP eventually calls MmUnlockPages and IoFreeMdl*/
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);

        Status = TdiQueryInformation(
            Qelt->Object.Object, TDI_QUERY_ADDRESS_INFO, Mdl);
        if (!NT_SUCCESS(Status))
            goto end;

        /* Write the local address */
        LONG LocalAddressLength = min(AcceptInfo->LocalAddressLength, Buffer->Address.Address->AddressLength);
        BufferPtr += AcceptInfo->ReceiveDataLength;
        RtlCopyMemory(BufferPtr, &Buffer->Address.Address->AddressType, LocalAddressLength);
        *(USHORT *)(BufferPtr + AcceptInfo->LocalAddressLength - sizeof(USHORT)) = LocalAddressLength;
        ExFreePool(Buffer);

        /* Write the remote address*/
        LONG RemoteAddressLength = min(AcceptInfo->RemoteAddressLength, RemoteAddress->Address[0].AddressLength);
        BufferPtr += AcceptInfo->LocalAddressLength;
        RtlCopyMemory(BufferPtr, &RemoteAddress->Address[0].AddressType, RemoteAddressLength);

        *(USHORT *)(BufferPtr + AcceptInfo->RemoteAddressLength - sizeof(USHORT)) = RemoteAddressLength;

        if (AcceptInfo->ReceiveDataLength)
        {
            /* Save this IRP so that it can be completed once the data is receive */
            FCB2->AcceptIrp = Irp;

            Status = TdiReceive(&FCB2->ReceiveIrp.InFlightRequest,
                                FCB2->Connection.Object, TDI_RECEIVE_NORMAL,
                                (PCHAR)BufferPtr,
                                AcceptInfo->ReceiveDataLength,
                                AcceptExReceiveComplete,
                                FCB2);

            if (Status == STATUS_PENDING)
                Status = STATUS_SUCCESS;
            return Status;
        }
        else
        {
            /* Begin receive buffer */
            Status = TdiReceive(
                &FCB2->ReceiveIrp.InFlightRequest, FCB2->Connection.Object, TDI_RECEIVE_NORMAL, FCB2->Recv.Window,
                FCB2->Recv.Size, ReceiveComplete, FCB2);

            if (Status == STATUS_PENDING)
                Status = STATUS_SUCCESS;
        }
    }

end:
    /* Complete the IRP, there is no data to receive */
    if (NT_SUCCESS(Status))
    {
        FCB2->SharedData.State = SOCKET_STATE_CONNECTED;
    }
    else
    {
        FCB2->SharedData.State = SOCKET_STATE_CREATED;
    }
    
    MmUnlockPages((PMDL)Irp->Tail.Overlay.DriverContext[3]);
    IoFreeMdl((PMDL)Irp->Tail.Overlay.DriverContext[3]); 
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    if( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );
    (void)IoSetCancelRoutine(Irp, NULL);
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_SUCCESS;
}

static IO_COMPLETION_ROUTINE ListenComplete;
static NTSTATUS NTAPI ListenComplete( PDEVICE_OBJECT DeviceObject,
                                      PIRP Irp,
                                      PVOID Context ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PAFD_TDI_OBJECT_QELT Qelt;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;

    UNREFERENCED_PARAMETER(DeviceObject);

    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    ASSERT(FCB->ListenIrp.InFlightRequest == Irp);
    FCB->ListenIrp.InFlightRequest = NULL;

    if( FCB->SharedData.State == SOCKET_STATE_CLOSED ) {
        /* Cleanup our IRP queue because the FCB is being destroyed */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_PREACCEPT] ) ) {
           NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_PREACCEPT]);
           NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
           NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
           NextIrp->IoStatus.Information = 0;
           if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
           (void)IoSetCancelRoutine(NextIrp, NULL);
           IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }

        /* Free ConnectionReturnInfo and ConnectionCallInfo */
        if (FCB->ListenIrp.ConnectionReturnInfo)
        {
            ExFreePoolWithTag(FCB->ListenIrp.ConnectionReturnInfo,
                              TAG_AFD_TDI_CONNECTION_INFORMATION);

            FCB->ListenIrp.ConnectionReturnInfo = NULL;
        }

        if (FCB->ListenIrp.ConnectionCallInfo)
        {
            ExFreePoolWithTag(FCB->ListenIrp.ConnectionCallInfo,
                              TAG_AFD_TDI_CONNECTION_INFORMATION);

            FCB->ListenIrp.ConnectionCallInfo = NULL;
        }

        SocketStateUnlock( FCB );
        return STATUS_FILE_CLOSED;
    }

    AFD_DbgPrint(MID_TRACE,("Completing listen request.\n"));
    AFD_DbgPrint(MID_TRACE,("IoStatus was %x\n", Irp->IoStatus.Status));

    if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        SocketStateUnlock(FCB);
        return Irp->IoStatus.Status;
    }

    Qelt = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(*Qelt),
                                 TAG_AFD_ACCEPT_QUEUE);

    if( !Qelt ) {
        Status = STATUS_NO_MEMORY;
    } else {
        UINT AddressType =
            FCB->LocalAddress->Address[0].AddressType;

        Qelt->Object = FCB->Connection;
        Qelt->Seq = FCB->ConnSeq++;
        AFD_DbgPrint(MID_TRACE,("Address Type: %u (RA %p)\n",
                                AddressType,
                                FCB->ListenIrp.
                                ConnectionReturnInfo->RemoteAddress));

        Status = TdiBuildNullConnectionInfo( &Qelt->ConnInfo, AddressType );
        if( NT_SUCCESS(Status) ) {
            TaCopyTransportAddressInPlace
               ( Qelt->ConnInfo->RemoteAddress,
                 FCB->ListenIrp.ConnectionReturnInfo->RemoteAddress );
            InsertTailList( &FCB->PendingConnections, &Qelt->ListEntry );
        }
    }

    /* Satisfy a pre-accept request if one is available */
    if( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_PREACCEPT] ) &&
        !IsListEmpty( &FCB->PendingConnections ) ) {
        PLIST_ENTRY PendingIrp  =
            RemoveHeadList( &FCB->PendingIrpList[FUNCTION_PREACCEPT] );
        PLIST_ENTRY PendingConn = FCB->PendingConnections.Flink;
        PIRP PendingIrpPtr = CONTAINING_RECORD(PendingIrp, IRP, Tail.Overlay.ListEntry);
        PAFD_TDI_OBJECT_QELT ConnectionData = CONTAINING_RECORD( PendingConn, AFD_TDI_OBJECT_QELT, ListEntry);
        if (PendingIrpPtr->Tail.Overlay.DriverContext[2] && PendingIrpPtr->Tail.Overlay.DriverContext[3])
        {
            RemoveEntryList(PendingConn);
            SatisfySuperAccept(FCB, PendingIrpPtr, ConnectionData);
        }
        else
        {
            SatisfyPreAccept(PendingIrpPtr, ConnectionData);
        }
    }

    /* Launch new accept socket */
    Status = WarmSocketForConnection( FCB );

    if (NT_SUCCESS(Status))
    {
        Status = TdiBuildNullConnectionInfoInPlace(FCB->ListenIrp.ConnectionCallInfo,
                                                   FCB->LocalAddress->Address[0].AddressType);
        ASSERT(Status == STATUS_SUCCESS);

        Status = TdiBuildNullConnectionInfoInPlace(FCB->ListenIrp.ConnectionReturnInfo,
                                                   FCB->LocalAddress->Address[0].AddressType);
        ASSERT(Status == STATUS_SUCCESS);

        Status = TdiListen( &FCB->ListenIrp.InFlightRequest,
                            FCB->Connection.Object,
                            &FCB->ListenIrp.ConnectionCallInfo,
                            &FCB->ListenIrp.ConnectionReturnInfo,
                            ListenComplete,
                            FCB );

        if (Status == STATUS_PENDING)
            Status = STATUS_SUCCESS;
    }

    /* Trigger a select return if appropriate */
    if( !IsListEmpty( &FCB->PendingConnections ) ) {
        FCB->PollState |= AFD_EVENT_ACCEPT;
        FCB->PollStatus[FD_ACCEPT_BIT] = STATUS_SUCCESS;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else
        FCB->PollState &= ~AFD_EVENT_ACCEPT;

    SocketStateUnlock( FCB );

    return Status;
}

NTSTATUS AfdListenSocket( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                          PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_LISTEN_DATA ListenReq;

    UNREFERENCED_PARAMETER(DeviceObject);

    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !(ListenReq = LockRequest( Irp, IrpSp, FALSE, NULL )) )
        return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
                                       0 );

    if( FCB->SharedData.State != SOCKET_STATE_BOUND ) {
        Status = STATUS_INVALID_PARAMETER;
        AFD_DbgPrint(MIN_TRACE,("Could not listen an unbound socket\n"));
        return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
    }

    FCB->DelayedAccept = ListenReq->UseDelayedAcceptance;

    AFD_DbgPrint(MID_TRACE,("ADDRESSFILE: %p\n", FCB->AddressFile.Handle));

    Status = WarmSocketForConnection( FCB );

    AFD_DbgPrint(MID_TRACE,("Status from warmsocket %x\n", Status));

    if( !NT_SUCCESS(Status) ) return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );

    Status = TdiBuildNullConnectionInfo
        ( &FCB->ListenIrp.ConnectionCallInfo,
          FCB->LocalAddress->Address[0].AddressType );

    if (!NT_SUCCESS(Status)) return UnlockAndMaybeComplete(FCB, Status, Irp, 0);

    Status = TdiBuildNullConnectionInfo
        ( &FCB->ListenIrp.ConnectionReturnInfo,
          FCB->LocalAddress->Address[0].AddressType );

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(FCB->ListenIrp.ConnectionCallInfo,
                          TAG_AFD_TDI_CONNECTION_INFORMATION);

        FCB->ListenIrp.ConnectionCallInfo = NULL;
        return UnlockAndMaybeComplete(FCB, Status, Irp, 0);
    }

    FCB->SharedData.State = SOCKET_STATE_LISTENING;

    Status = TdiListen( &FCB->ListenIrp.InFlightRequest,
                        FCB->Connection.Object,
                        &FCB->ListenIrp.ConnectionCallInfo,
                        &FCB->ListenIrp.ConnectionReturnInfo,
                        ListenComplete,
                        FCB );

    if( Status == STATUS_PENDING )
        Status = STATUS_SUCCESS;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));
    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}

NTSTATUS AfdWaitForListen( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                           PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !IsListEmpty( &FCB->PendingConnections ) ) {
        PLIST_ENTRY PendingConn = FCB->PendingConnections.Flink;

        /* We have a pending connection ... complete this irp right away */
        Status = SatisfyPreAccept
            ( Irp,
              CONTAINING_RECORD
              ( PendingConn, AFD_TDI_OBJECT_QELT, ListEntry ) );

        AFD_DbgPrint(MID_TRACE,("Completed a wait for accept\n"));

        if ( !IsListEmpty( &FCB->PendingConnections ) )
        {
             FCB->PollState |= AFD_EVENT_ACCEPT;
             FCB->PollStatus[FD_ACCEPT_BIT] = STATUS_SUCCESS;
             PollReeval( FCB->DeviceExt, FCB->FileObject );
        } else
             FCB->PollState &= ~AFD_EVENT_ACCEPT;

        SocketStateUnlock( FCB );
        return Status;
    } else if (FCB->NonBlocking) {
        AFD_DbgPrint(MIN_TRACE,("No connection ready on a non-blocking socket\n"));

        return UnlockAndMaybeComplete(FCB, STATUS_CANT_WAIT, Irp, 0);
    } else {
        AFD_DbgPrint(MID_TRACE,("Holding\n"));

        return LeaveIrpUntilLater( FCB, Irp, FUNCTION_PREACCEPT );
    }
}

NTSTATUS AfdAccept( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                    PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt =
        (PAFD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_ACCEPT_DATA AcceptData = Irp->AssociatedIrp.SystemBuffer;
    PLIST_ENTRY PendingConn;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    FCB->EventSelectDisabled &= ~AFD_EVENT_ACCEPT;

    for( PendingConn = FCB->PendingConnections.Flink;
         PendingConn != &FCB->PendingConnections;
         PendingConn = PendingConn->Flink ) {
        PAFD_TDI_OBJECT_QELT PendingConnObj =
            CONTAINING_RECORD( PendingConn, AFD_TDI_OBJECT_QELT, ListEntry );

        AFD_DbgPrint(MID_TRACE,("Comparing Seq %u to Q %u\n",
                                AcceptData->SequenceNumber,
                                PendingConnObj->Seq));

        if( PendingConnObj->Seq == AcceptData->SequenceNumber ) {
            PFILE_OBJECT NewFileObject = NULL;

            RemoveEntryList( PendingConn );

            Status = ObReferenceObjectByHandle
                ( AcceptData->ListenHandle,
                  FILE_ALL_ACCESS,
                  NULL,
                  KernelMode,
                  (PVOID *)&NewFileObject,
                  NULL );

            if( !NT_SUCCESS(Status) ) return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );

            ASSERT(NewFileObject != FileObject);
            ASSERT(NewFileObject->FsContext != FCB);

            /* We have a pending connection ... complete this irp right away */
            Status = SatisfyAccept( DeviceExt, Irp, NewFileObject, PendingConnObj, FALSE );

            ObDereferenceObject( NewFileObject );

            AFD_DbgPrint(MID_TRACE,("Completed a wait for accept\n"));

            ExFreePoolWithTag(PendingConnObj, TAG_AFD_ACCEPT_QUEUE);

            if( !IsListEmpty( &FCB->PendingConnections ) )
            {
                FCB->PollState |= AFD_EVENT_ACCEPT;
                FCB->PollStatus[FD_ACCEPT_BIT] = STATUS_SUCCESS;
                PollReeval( FCB->DeviceExt, FCB->FileObject );
            } else
                FCB->PollState &= ~AFD_EVENT_ACCEPT;

            SocketStateUnlock( FCB );
            return Status;
        }
    }

    AFD_DbgPrint(MIN_TRACE,("No connection waiting\n"));

    return UnlockAndMaybeComplete( FCB, STATUS_UNSUCCESSFUL, Irp, 0 );
}

NTSTATUS AfdSuperAccept( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                    PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_SUPER_ACCEPT_INFO AcceptRequest;

    PFILE_OBJECT NewFileObject = NULL;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !(AcceptRequest = LockRequest( Irp, IrpSp, FALSE, NULL )) )
        return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
                                       0 );

    FCB->EventSelectDisabled &= ~AFD_EVENT_ACCEPT;

    /* Allow the IRP to access UserBuffer later */
    Irp->Tail.Overlay.DriverContext[3] = IoAllocateMdl(Irp->UserBuffer, IrpSp->Parameters.DeviceIoControl.OutputBufferLength, FALSE, FALSE, NULL);
    _SEH2_TRY
    {
        MmProbeAndLockPages((PMDL)Irp->Tail.Overlay.DriverContext[3], Irp->RequestorMode, IoWriteAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        return UnlockAndMaybeComplete(FCB, Status, Irp, 0);
    }
    _SEH2_END;

    /* Validate that accepting socket is valid, and has not been bound or connected */
    Status = ObReferenceObjectByHandle(
        AcceptRequest->AcceptHandle, FILE_ALL_ACCESS, NULL, KernelMode, (PVOID *)&NewFileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        IoFreeMdl((PMDL)Irp->Tail.Overlay.DriverContext[3]);
        Irp->Tail.Overlay.DriverContext[3] = NULL;
        return UnlockAndMaybeComplete(FCB, Status, Irp, 0);
    }

    PAFD_FCB FCB2 = NewFileObject->FsContext;

    if (!SocketAcquireStateLock(FCB2))
    {
        ObDereferenceObject(NewFileObject);
        return LostSocket(Irp);
    }

    /* Verify that the acceptor socket has not been already binded/connected or that it has been used by AcceptEx */
    if (FCB2->SharedData.State != SOCKET_STATE_CREATED)
    {
        IoFreeMdl((PMDL)Irp->Tail.Overlay.DriverContext[3]);
        Irp->Tail.Overlay.DriverContext[3] = NULL;
        SocketStateUnlock(FCB2);
        ObDereferenceObject(NewFileObject);
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);
    }
    FCB2->SharedData.State = SOCKET_STATE_CONNECTING;

    Irp->Tail.Overlay.DriverContext[2] = NewFileObject;
    
    /* Proceed later in SatisfyAcceptEx */
    SocketStateUnlock(FCB2);

    return LeaveIrpUntilLater(FCB, Irp, FUNCTION_PREACCEPT);
}
