/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/read.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include "afd.h"

static VOID RefillSocketBuffer( PAFD_FCB FCB )
{
    /* Make sure nothing's in flight first */
    if (FCB->ReceiveIrp.InFlightRequest) return;

    /* Now ensure that receive is still allowed */
    if (FCB->TdiReceiveClosed) return;

    /* Check if the buffer is full */
    if (FCB->Recv.Content == FCB->Recv.Size)
    {
        /* If there are bytes used, we can solve this problem */
        if (FCB->Recv.BytesUsed != 0)
        {
            /* Reposition the unused portion to the beginning of the receive window */
            RtlMoveMemory(FCB->Recv.Window,
                          FCB->Recv.Window + FCB->Recv.BytesUsed,
                          FCB->Recv.Content - FCB->Recv.BytesUsed);

            FCB->Recv.Content -= FCB->Recv.BytesUsed;
            FCB->Recv.BytesUsed = 0;
        }
        else
        {
            /* No space in the buffer to receive */
            return;
        }
    }

    AFD_DbgPrint(MID_TRACE,("Replenishing buffer\n"));

    TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
                FCB->Connection.Object,
                TDI_RECEIVE_NORMAL,
                FCB->Recv.Window + FCB->Recv.Content,
                FCB->Recv.Size - FCB->Recv.Content,
                ReceiveComplete,
                FCB );
}

static VOID HandleReceiveComplete( PAFD_FCB FCB, NTSTATUS Status, ULONG_PTR Information )
{
    FCB->LastReceiveStatus = Status;

    /* We got closed while the receive was in progress */
    if (FCB->TdiReceiveClosed)
    {
        /* The received data is discarded */
    }
    /* Receive successful */
    else if (Status == STATUS_SUCCESS)
    {
        FCB->Recv.Content += Information;
        ASSERT(FCB->Recv.Content <= FCB->Recv.Size);

        /* Check for graceful closure */
        if (Information == 0)
        {
            /* Receive is closed */
            FCB->TdiReceiveClosed = TRUE;
        }
        else
        {
            /* Issue another receive IRP to keep the buffer well stocked */
            RefillSocketBuffer(FCB);
        }
    }
    /* Receive failed with no data (unexpected closure) */
    else
    {
        /* Previously received data remains intact */
        FCB->TdiReceiveClosed = TRUE;
    }
}

static BOOLEAN CantReadMore( PAFD_FCB FCB ) {
    UINT BytesAvailable = FCB->Recv.Content - FCB->Recv.BytesUsed;

    return !BytesAvailable && FCB->TdiReceiveClosed;
}

static NTSTATUS TryToSatisfyRecvRequestFromBuffer( PAFD_FCB FCB,
                                                   PAFD_RECV_INFO RecvReq,
                                                   PUINT TotalBytesCopied ) {
    UINT i, BytesToCopy = 0, FcbBytesCopied = FCB->Recv.BytesUsed,
        BytesAvailable =
        FCB->Recv.Content - FCB->Recv.BytesUsed;
    PAFD_MAPBUF Map;
    *TotalBytesCopied = 0;


    AFD_DbgPrint(MID_TRACE,("Called, BytesAvailable = %u\n", BytesAvailable));

    if( CantReadMore(FCB) ) return STATUS_SUCCESS;
    if( !BytesAvailable ) return STATUS_PENDING;

    Map = (PAFD_MAPBUF)(RecvReq->BufferArray + RecvReq->BufferCount);

    AFD_DbgPrint(MID_TRACE,("Buffer Count: %u @ %p\n",
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

            AFD_DbgPrint(MID_TRACE,("Buffer %u: %p:%u\n",
                                    i,
                                    Map[i].BufferAddress,
                                    BytesToCopy));

            RtlCopyMemory( Map[i].BufferAddress,
                           FCB->Recv.Window + FcbBytesCopied,
                           BytesToCopy );

            MmUnmapLockedPages( Map[i].BufferAddress, Map[i].Mdl );

            *TotalBytesCopied += BytesToCopy;
            FcbBytesCopied += BytesToCopy;
            BytesAvailable -= BytesToCopy;

            if (!(RecvReq->TdiFlags & TDI_RECEIVE_PEEK))
                FCB->Recv.BytesUsed += BytesToCopy;
        }
    }

    /* Issue another receive IRP to keep the buffer well stocked */
    RefillSocketBuffer(FCB);

    return STATUS_SUCCESS;
}

static NTSTATUS ReceiveActivity( PAFD_FCB FCB, PIRP Irp ) {
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;
    PIO_STACK_LOCATION NextIrpSp;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0;
    NTSTATUS Status = STATUS_SUCCESS, RetStatus = STATUS_PENDING;

    AFD_DbgPrint(MID_TRACE,("%p %p\n", FCB, Irp));

    AFD_DbgPrint(MID_TRACE,("FCB %p Receive data waiting %u\n",
                            FCB, FCB->Recv.Content));

    if( CantReadMore( FCB ) ) {
        /* Success here means that we got an EOF.  Complete a pending read
         * with zero bytes if we haven't yet overread, then kill the others.
         */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
            NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
            NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
            NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
            RecvReq = GetLockedData(NextIrp, NextIrpSp);

            AFD_DbgPrint(MID_TRACE,("Completing recv %p (%u)\n", NextIrp,
                                    TotalBytesCopied));
            UnlockBuffers( RecvReq->BufferArray,
                           RecvReq->BufferCount, FALSE );

            /* Unexpected disconnect by the remote host or graceful disconnect */
            Status = FCB->LastReceiveStatus;

            NextIrp->IoStatus.Status = Status;
            NextIrp->IoStatus.Information = 0;
            if( NextIrp == Irp ) RetStatus = Status;
            if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
            (void)IoSetCancelRoutine(NextIrp, NULL);
            IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }
    } else {
        /* Kick the user that receive would be possible now */
        /* XXX Not implemented yet */

        AFD_DbgPrint(MID_TRACE,("FCB %p Receive data waiting %u\n",
                                FCB, FCB->Recv.Content));
        /*OskitDumpBuffer( FCB->Recv.Window, FCB->Recv.Content );*/

        /* Try to clear some requests */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
            NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
            NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
            NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
            RecvReq = GetLockedData(NextIrp, NextIrpSp);

            AFD_DbgPrint(MID_TRACE,("RecvReq @ %p\n", RecvReq));

            Status = TryToSatisfyRecvRequestFromBuffer
            ( FCB, RecvReq, &TotalBytesCopied );

            if( Status == STATUS_PENDING ) {
                AFD_DbgPrint(MID_TRACE,("Ran out of data for %p\n", NextIrp));
                InsertHeadList(&FCB->PendingIrpList[FUNCTION_RECV],
                               &NextIrp->Tail.Overlay.ListEntry);
                break;
            } else {
                AFD_DbgPrint(MID_TRACE,("Completing recv %p (%u)\n", NextIrp,
                                        TotalBytesCopied));
                UnlockBuffers( RecvReq->BufferArray,
                               RecvReq->BufferCount, FALSE );
                NextIrp->IoStatus.Status = Status;
                NextIrp->IoStatus.Information = TotalBytesCopied;
                if( NextIrp == Irp ) {
                    RetStatus = Status;
                }
                if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
                (void)IoSetCancelRoutine(NextIrp, NULL);
                IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
            }
        }
    }

    if( FCB->Recv.Content - FCB->Recv.BytesUsed &&
        IsListEmpty(&FCB->PendingIrpList[FUNCTION_RECV]) ) {
        FCB->PollState |= AFD_EVENT_RECEIVE;
        FCB->PollStatus[FD_READ_BIT] = STATUS_SUCCESS;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    }
    else
    {
        FCB->PollState &= ~AFD_EVENT_RECEIVE;
    }

    /* Signal FD_CLOSE if no buffered data remains and the socket can't receive any more */
    if (CantReadMore(FCB))
    {
        if (FCB->LastReceiveStatus == STATUS_SUCCESS)
        {
            FCB->PollState |= AFD_EVENT_DISCONNECT;
        }
        else
        {
            FCB->PollState |= AFD_EVENT_CLOSE;
        }
        FCB->PollStatus[FD_CLOSE_BIT] = FCB->LastReceiveStatus;
        PollReeval(FCB->DeviceExt, FCB->FileObject);
    }

    AFD_DbgPrint(MID_TRACE,("RetStatus for irp %p is %x\n", Irp, RetStatus));

    return RetStatus;
}

NTSTATUS NTAPI ReceiveComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;
    PAFD_RECV_INFO RecvReq;
    PIO_STACK_LOCATION NextIrpSp;

    UNREFERENCED_PARAMETER(DeviceObject);

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    ASSERT(FCB->ReceiveIrp.InFlightRequest == Irp);
    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        /* Cleanup our IRP queue because the FCB is being destroyed */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
            NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
            NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
            NextIrpSp = IoGetCurrentIrpStackLocation(NextIrp);
            RecvReq = GetLockedData(NextIrp, NextIrpSp);
            NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
            NextIrp->IoStatus.Information = 0;
            UnlockBuffers(RecvReq->BufferArray, RecvReq->BufferCount, FALSE);
            if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
            (void)IoSetCancelRoutine(NextIrp, NULL);
            IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }
        SocketStateUnlock( FCB );
        return STATUS_FILE_CLOSED;
    } else if( FCB->State == SOCKET_STATE_LISTENING ) {
        AFD_DbgPrint(MIN_TRACE,("!!! LISTENER GOT A RECEIVE COMPLETE !!!\n"));
        SocketStateUnlock( FCB );
        return STATUS_INVALID_PARAMETER;
    }

    HandleReceiveComplete( FCB, Irp->IoStatus.Status, Irp->IoStatus.Information );

    ReceiveActivity( FCB, NULL );

    SocketStateUnlock( FCB );

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
SatisfyPacketRecvRequest( PAFD_FCB FCB, PIRP Irp,
                         PAFD_STORED_DATAGRAM DatagramRecv,
                         PUINT TotalBytesCopied ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PAFD_RECV_INFO RecvReq =
    GetLockedData(Irp, IrpSp);
    UINT BytesToCopy = 0, BytesAvailable = DatagramRecv->Len, AddrLen = 0;
    PAFD_MAPBUF Map;
    BOOLEAN ExtraBuffers = CheckUnlockExtraBuffers(FCB, IrpSp);

    Map = (PAFD_MAPBUF)(RecvReq->BufferArray +
                        RecvReq->BufferCount +
                        (ExtraBuffers ? EXTRA_LOCK_BUFFERS : 0));

    BytesToCopy = MIN( RecvReq->BufferArray[0].len, BytesAvailable );

    AFD_DbgPrint(MID_TRACE,("BytesToCopy: %u len %u\n", BytesToCopy,
                            RecvReq->BufferArray[0].len));

    if( Map[0].Mdl ) {
        /* Copy the address */
        if( ExtraBuffers && Map[1].Mdl && Map[2].Mdl ) {
            AFD_DbgPrint(MID_TRACE,("Checking TAAddressCount\n"));

            if( DatagramRecv->Address->TAAddressCount != 1 ) {
                AFD_DbgPrint
                (MIN_TRACE,
                 ("Wierd address count %d\n",
                  DatagramRecv->Address->TAAddressCount));
            }

            AFD_DbgPrint(MID_TRACE,("Computing addr len\n"));

            AddrLen = MIN(DatagramRecv->Address->Address->AddressLength +
                          sizeof(USHORT),
                          RecvReq->BufferArray[1].len);

            AFD_DbgPrint(MID_TRACE,("Copying %u bytes of address\n", AddrLen));

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

        AFD_DbgPrint(MID_TRACE,("Buffer %d: %p:%u\n",
                                0,
                                Map[0].BufferAddress,
                                BytesToCopy));

        RtlCopyMemory( Map[0].BufferAddress,
                      DatagramRecv->Buffer,
                      BytesToCopy );

        MmUnmapLockedPages( Map[0].BufferAddress, Map[0].Mdl );

        *TotalBytesCopied = BytesToCopy;
    }

    if (*TotalBytesCopied == DatagramRecv->Len)
    {
        /* We copied the whole datagram */
        Status = Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    else
    {
        /* We only copied part of the datagram */
        Status = Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information = *TotalBytesCopied;

    if (!(RecvReq->TdiFlags & TDI_RECEIVE_PEEK))
    {
        FCB->Recv.Content -= DatagramRecv->Len;
        ExFreePoolWithTag(DatagramRecv->Address, TAG_AFD_TRANSPORT_ADDRESS);
        ExFreePoolWithTag(DatagramRecv, TAG_AFD_STORED_DATAGRAM);
    }

    AFD_DbgPrint(MID_TRACE,("Done\n"));

    return Status;
}

NTSTATUS NTAPI
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp,
                           PIO_STACK_LOCATION IrpSp, BOOLEAN Short) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_RECV_INFO RecvReq;
    UINT TotalBytesCopied = 0;
    PAFD_STORED_DATAGRAM DatagramRecv;
    PLIST_ENTRY ListEntry;
    KPROCESSOR_MODE LockMode;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Short);

    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    FCB->EventSelectDisabled &= ~AFD_EVENT_RECEIVE;

    if( !(FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS) &&
        FCB->State != SOCKET_STATE_CONNECTED &&
        FCB->State != SOCKET_STATE_CONNECTING ) {
        AFD_DbgPrint(MIN_TRACE,("Called recv on wrong kind of socket (s%x)\n",
                                FCB->State));
        return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER,
                                       Irp, 0 );
    }

    if( !(RecvReq = LockRequest( Irp, IrpSp, FALSE, &LockMode )) )
        return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY,
                                       Irp, 0 );

    AFD_DbgPrint(MID_TRACE,("Recv flags %x\n", RecvReq->AfdFlags));

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray,
                                       RecvReq->BufferCount,
                                       NULL, NULL,
                                       TRUE, FALSE, LockMode );

    if( !RecvReq->BufferArray ) {
        return UnlockAndMaybeComplete( FCB, STATUS_ACCESS_VIOLATION,
                                      Irp, 0 );
    }

    if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS )
    {
        if (!IsListEmpty(&FCB->DatagramList))
        {
            ListEntry = RemoveHeadList(&FCB->DatagramList);
            DatagramRecv = CONTAINING_RECORD(ListEntry, AFD_STORED_DATAGRAM, ListEntry);
            Status = SatisfyPacketRecvRequest(FCB, Irp, DatagramRecv,
                                              (PUINT)&Irp->IoStatus.Information);

            if (RecvReq->TdiFlags & TDI_RECEIVE_PEEK)
            {
                InsertHeadList(&FCB->DatagramList,
                               &DatagramRecv->ListEntry);
            }

            if (!IsListEmpty(&FCB->DatagramList))
            {
                FCB->PollState |= AFD_EVENT_RECEIVE;
                FCB->PollStatus[FD_READ_BIT] = STATUS_SUCCESS;
                PollReeval( FCB->DeviceExt, FCB->FileObject );
            }
            else
                FCB->PollState &= ~AFD_EVENT_RECEIVE;

            UnlockBuffers(RecvReq->BufferArray, RecvReq->BufferCount, FALSE);

            return UnlockAndMaybeComplete(FCB, Status, Irp, Irp->IoStatus.Information);
        }
        else if (!(RecvReq->AfdFlags & AFD_OVERLAPPED) &&
                ((RecvReq->AfdFlags & AFD_IMMEDIATE) || (FCB->NonBlocking)))
        {
            AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
            Status = STATUS_CANT_WAIT;
            FCB->PollState &= ~AFD_EVENT_RECEIVE;
            UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, FALSE );
            return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
        }
        else
        {
            FCB->PollState &= ~AFD_EVENT_RECEIVE;
            return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
        }
    }

    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    InsertTailList( &FCB->PendingIrpList[FUNCTION_RECV],
                    &Irp->Tail.Overlay.ListEntry );

    /************ From this point, the IRP is not ours ************/

    Status = ReceiveActivity( FCB, Irp );

    if( Status == STATUS_PENDING &&
        !(RecvReq->AfdFlags & AFD_OVERLAPPED) &&
        ((RecvReq->AfdFlags & AFD_IMMEDIATE) || (FCB->NonBlocking))) {
        AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
        Status = STATUS_CANT_WAIT;
        TotalBytesCopied = 0;
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
        UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, FALSE );
        return UnlockAndMaybeComplete( FCB, Status, Irp,
                                       TotalBytesCopied );
    } else if( Status == STATUS_PENDING ) {
        AFD_DbgPrint(MID_TRACE,("Leaving read irp\n"));
        IoMarkIrpPending( Irp );
        (void)IoSetCancelRoutine(Irp, AfdCancelHandler);
    } else {
        AFD_DbgPrint(MID_TRACE,("Completed with status %x\n", Status));
    }

    SocketStateUnlock( FCB );
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
    PLIST_ENTRY NextIrpEntry, DatagramRecvEntry;

    UNREFERENCED_PARAMETER(DeviceObject);

    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    ASSERT(FCB->ReceiveIrp.InFlightRequest == Irp);
    FCB->ReceiveIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        /* Cleanup our IRP queue because the FCB is being destroyed */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_RECV] ) ) {
            NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_RECV]);
            NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
            NextIrpSp = IoGetCurrentIrpStackLocation( NextIrp );
            RecvReq = GetLockedData(NextIrp, NextIrpSp);
            NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
            NextIrp->IoStatus.Information = 0;
            UnlockBuffers(RecvReq->BufferArray, RecvReq->BufferCount, CheckUnlockExtraBuffers(FCB, NextIrpSp));
            if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
            (void)IoSetCancelRoutine(NextIrp, NULL);
            IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }

        /* Free all items on the datagram list */
        while( !IsListEmpty( &FCB->DatagramList ) ) {
               DatagramRecvEntry = RemoveHeadList(&FCB->DatagramList);
               DatagramRecv = CONTAINING_RECORD(DatagramRecvEntry, AFD_STORED_DATAGRAM, ListEntry);
               ExFreePoolWithTag(DatagramRecv->Address, TAG_AFD_TRANSPORT_ADDRESS);
               ExFreePoolWithTag(DatagramRecv, TAG_AFD_STORED_DATAGRAM);
        }

        SocketStateUnlock( FCB );
        return STATUS_FILE_CLOSED;
    }

    if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        SocketStateUnlock(FCB);
        return Irp->IoStatus.Status;
    }

    if (FCB->TdiReceiveClosed)
    {
        SocketStateUnlock(FCB);
        return STATUS_FILE_CLOSED;
    }

    DatagramRecv = ExAllocatePoolWithTag(NonPagedPool,
                                         DGSize,
                                         TAG_AFD_STORED_DATAGRAM);

    if( DatagramRecv ) {
        DatagramRecv->Len = Irp->IoStatus.Information;
        RtlCopyMemory( DatagramRecv->Buffer, FCB->Recv.Window,
                       DatagramRecv->Len );
        AFD_DbgPrint(MID_TRACE,("Received (A %p)\n",
                                FCB->AddressFrom->RemoteAddress));
        DatagramRecv->Address =
            TaCopyTransportAddress( FCB->AddressFrom->RemoteAddress );

        if( !DatagramRecv->Address ) Status = STATUS_NO_MEMORY;

    } else Status = STATUS_NO_MEMORY;

    if( !NT_SUCCESS( Status ) ) {

        if (DatagramRecv)
        {
            ExFreePoolWithTag(DatagramRecv, TAG_AFD_STORED_DATAGRAM);
        }

        SocketStateUnlock( FCB );
        return Status;
    } else {
        FCB->Recv.Content += DatagramRecv->Len;
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
        RecvReq = GetLockedData(NextIrp, NextIrpSp);

        AFD_DbgPrint(MID_TRACE,("RecvReq: %p, DatagramRecv: %p\n",
                                RecvReq, DatagramRecv));

        AFD_DbgPrint(MID_TRACE,("Satisfying\n"));
        Status = SatisfyPacketRecvRequest
        ( FCB, NextIrp, DatagramRecv,
         (PUINT)&NextIrp->IoStatus.Information );

        if (RecvReq->TdiFlags & TDI_RECEIVE_PEEK)
        {
            InsertHeadList(&FCB->DatagramList,
                           &DatagramRecv->ListEntry);
        }

        AFD_DbgPrint(MID_TRACE,("Unlocking\n"));
        UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, CheckUnlockExtraBuffers(FCB, NextIrpSp) );
        if ( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );

        AFD_DbgPrint(MID_TRACE,("Completing\n"));
        (void)IoSetCancelRoutine(NextIrp, NULL);
        NextIrp->IoStatus.Status = Status;

        IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
    }

    if( !IsListEmpty( &FCB->DatagramList ) && IsListEmpty(&FCB->PendingIrpList[FUNCTION_RECV]) ) {
        AFD_DbgPrint(MID_TRACE,("Signalling\n"));
        FCB->PollState |= AFD_EVENT_RECEIVE;
        FCB->PollStatus[FD_READ_BIT] = STATUS_SUCCESS;
        PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else
        FCB->PollState &= ~AFD_EVENT_RECEIVE;

    if( NT_SUCCESS(Irp->IoStatus.Status) && FCB->Recv.Content < FCB->Recv.Size ) {
        /* Now relaunch the datagram request */
        Status = TdiReceiveDatagram
            ( &FCB->ReceiveIrp.InFlightRequest,
              FCB->AddressFile.Object,
              0,
              FCB->Recv.Window,
              FCB->Recv.Size,
              FCB->AddressFrom,
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
    KPROCESSOR_MODE LockMode;

    UNREFERENCED_PARAMETER(DeviceObject);

    AFD_DbgPrint(MID_TRACE,("Called on %p\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    FCB->EventSelectDisabled &= ~AFD_EVENT_RECEIVE;

    /* Check that the socket is bound */
    if( FCB->State != SOCKET_STATE_BOUND )
    {
        AFD_DbgPrint(MIN_TRACE,("Invalid socket state\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_INVALID_PARAMETER, Irp, 0);
    }

    if (FCB->TdiReceiveClosed)
    {
        AFD_DbgPrint(MIN_TRACE,("Receive closed\n"));
        return UnlockAndMaybeComplete(FCB, STATUS_FILE_CLOSED, Irp, 0);
    }

    if( !(RecvReq = LockRequest( Irp, IrpSp, FALSE, &LockMode )) )
        return UnlockAndMaybeComplete(FCB, STATUS_NO_MEMORY, Irp, 0);

    AFD_DbgPrint(MID_TRACE,("Recv flags %x\n", RecvReq->AfdFlags));

    RecvReq->BufferArray = LockBuffers( RecvReq->BufferArray,
                                        RecvReq->BufferCount,
                                        RecvReq->Address,
                                        RecvReq->AddressLength,
                                        TRUE, TRUE, LockMode );

    if( !RecvReq->BufferArray ) { /* access violation in userspace */
        return UnlockAndMaybeComplete(FCB, STATUS_ACCESS_VIOLATION, Irp, 0);
    }

    if (!IsListEmpty(&FCB->DatagramList))
    {
        ListEntry = RemoveHeadList(&FCB->DatagramList);
        DatagramRecv = CONTAINING_RECORD(ListEntry, AFD_STORED_DATAGRAM, ListEntry);
        Status = SatisfyPacketRecvRequest(FCB, Irp, DatagramRecv,
                                          (PUINT)&Irp->IoStatus.Information);

        if (RecvReq->TdiFlags & TDI_RECEIVE_PEEK)
        {
            InsertHeadList(&FCB->DatagramList,
                           &DatagramRecv->ListEntry);
        }

        if (!IsListEmpty(&FCB->DatagramList))
        {
            FCB->PollState |= AFD_EVENT_RECEIVE;
            FCB->PollStatus[FD_READ_BIT] = STATUS_SUCCESS;
            PollReeval( FCB->DeviceExt, FCB->FileObject );
        }
        else
            FCB->PollState &= ~AFD_EVENT_RECEIVE;

        UnlockBuffers(RecvReq->BufferArray, RecvReq->BufferCount, TRUE);

        return UnlockAndMaybeComplete(FCB, Status, Irp, Irp->IoStatus.Information);
    }
    else if (!(RecvReq->AfdFlags & AFD_OVERLAPPED) &&
            ((RecvReq->AfdFlags & AFD_IMMEDIATE) || (FCB->NonBlocking)))
    {
        AFD_DbgPrint(MID_TRACE,("Nonblocking\n"));
        Status = STATUS_CANT_WAIT;
        FCB->PollState &= ~AFD_EVENT_RECEIVE;
        UnlockBuffers( RecvReq->BufferArray, RecvReq->BufferCount, TRUE );
        return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
    }
    else
    {
        FCB->PollState &= ~AFD_EVENT_RECEIVE;
        return LeaveIrpUntilLater( FCB, Irp, FUNCTION_RECV );
    }
}
