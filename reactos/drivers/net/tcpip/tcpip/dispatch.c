/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/dispatch.h
 * PURPOSE:     TDI dispatch routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <dispatch.h>
#include <routines.h>
#include <datagram.h>
#include <info.h>


NTSTATUS DispPrepareIrpForCancel(
    PTRANSPORT_CONTEXT Context,
    PIRP Irp,
    PDRIVER_CANCEL CancelRoutine)
/*
 * FUNCTION: Prepare an IRP for cancellation
 * ARGUMENTS:
 *     Context       = Pointer to context information
 *     Irp           = Pointer to an I/O request packet
 *     CancelRoutine = Routine to be called when I/O request is cancelled
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IoAcquireCancelSpinLock(&OldIrql);

    if (!Irp->Cancel) {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);
        Context->RefCount++;
        IoReleaseCancelSpinLock(OldIrql);

        TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP at 0x%X can now be cancelled).\n", Irp));

        return STATUS_SUCCESS;
    }

    /* IRP has already been cancelled */
 
    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP was already cancelled).\n"));

    return STATUS_CANCELLED;
}


VOID DispCancelComplete(
    PVOID Context)
/*
 * FUNCTION: Completes a cancel request
 * ARGUMENTS:
 *     Context = Pointer to context information (FILE_OBJECT)
 */
{
    KIRQL OldIrql;
    PFILE_OBJECT FileObject;
    PTRANSPORT_CONTEXT TranContext;
    
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    FileObject  = (PFILE_OBJECT)Context;
    TranContext = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    
    IoAcquireCancelSpinLock(&OldIrql);

    /* Remove the reference placed on the endpoint by the cancel routine.
       The cancelled IRP will be completed by the completion routine for
       the request */
    TranContext->RefCount--;

    if (TranContext->RefCount == 0) {
        TI_DbgPrint(DEBUG_IRP, ("Setting TranContext->CleanupEvent to signaled.\n"));
        /* Set the cleanup event */
        KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);
    }

    TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount (%d).\n", TranContext->RefCount));

    IoReleaseCancelSpinLock(OldIrql);

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));
}


VOID DispCancelRequest(
    PDEVICE_OBJECT Device,
    PIRP Irp)
/*
 * FUNCTION: Cancels an IRP
 * ARGUMENTS:
 *     Device = Pointer to device object
 *     Irp    = Pointer to an I/O request packet
 */
{
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    PFILE_OBJECT FileObject;
    UCHAR MinorFunction;
    NTSTATUS Status = STATUS_SUCCESS;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp         = IoGetCurrentIrpStackLocation(Irp);
    FileObject    = IrpSp->FileObject;
    TranContext   = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    MinorFunction = IrpSp->MinorFunction;

    TI_DbgPrint(DEBUG_IRP, ("IRP at (0x%X)  MinorFunction (0x%X)  IrpSp (0x%X).\n", Irp, MinorFunction, IrpSp));

#ifdef DBG
    if (!Irp->Cancel)
        TI_DbgPrint(MIN_TRACE, ("Irp->Cancel is FALSE, should be TRUE.\n"));
#endif

    /* Increase reference count to prevent accidential closure
       of the object while inside the cancel routine */
    TranContext->RefCount++;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /* Try canceling the request */
    switch(MinorFunction) {
    case TDI_SEND:

    case TDI_RECEIVE:
        /* FIXME: Close connection */
        break;

    case TDI_SEND_DATAGRAM:
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_SEND_DATAGRAM, but no address file.\n"));
            break;
        }

        DGCancelSendRequest(TranContext->Handle.AddressHandle, Irp);
        break;

    case TDI_RECEIVE_DATAGRAM:
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_RECEIVE_DATAGRAM, but no address file.\n"));
            break;
        }

        DGCancelReceiveRequest(TranContext->Handle.AddressHandle, Irp);
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Unknown IRP. MinorFunction (0x%X).\n", MinorFunction));
        break;
    }

    if (Status != STATUS_PENDING)
        DispCancelComplete(FileObject);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID DispDataRequestComplete(
    PVOID Context,
    NTSTATUS Status,
    ULONG Count)
/*
 * FUNCTION: Completes a send/receive IRP
 * ARGUMENTS:
 *     Context = Pointer to context information (IRP)
 *     Status  = Status of the request
 *     Count   = Number of bytes sent or received
 */
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    Irp         = (PIRP)Context;
    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    IoAcquireCancelSpinLock(&OldIrql);

    IoSetCancelRoutine(Irp, NULL);
    TranContext->RefCount--;
    TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount (%d).\n", TranContext->RefCount));
    if (TranContext->RefCount == 0) {
        TI_DbgPrint(DEBUG_IRP, ("Setting TranContext->CleanupEvent to signaled.\n"));

        KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);
    }

    if (Irp->Cancel || TranContext->CancelIrps) {
        /* The IRP has been cancelled */

        TI_DbgPrint(DEBUG_IRP, ("IRP is cancelled.\n"));

        Status = STATUS_CANCELLED;
        Count  = 0;
    }

    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Count;

    TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}


NTSTATUS DispTdiAccept(
    PIRP Irp)
/*
 * FUNCTION: TDI_ACCEPT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiAssociateAddress(
    PIRP Irp)
/*
 * FUNCTION: TDI_ASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiConnect(
    PIRP Irp)
/*
 * FUNCTION: TDI_CONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiDisassociateAddress(
    PIRP Irp)
/*
 * FUNCTION: TDI_DISASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiDisconnect(
    PIRP Irp)
/*
 * FUNCTION: TDI_DISCONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiListen(
    PIRP Irp)
/*
 * FUNCTION: TDI_LISTEN handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiQueryInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: TDI_QUERY_INFORMATION handler
 * ARGUMENTS:
 *     DeviceObject = Pointer to device object structure
 *     Irp          = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiReceive(
    PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiReceiveDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    PTDI_REQUEST_KERNEL_RECEIVEDG DgramInfo;
    PTRANSPORT_CONTEXT TranContext;
    TDI_REQUEST Request;
    NTSTATUS Status;
    ULONG BytesReceived;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    DgramInfo = (PTDI_REQUEST_KERNEL_RECEIVEDG)&(IrpSp->Parameters);

    TranContext = IrpSp->FileObject->FsContext;
    /* Initialize a receive request */
    Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
    Request.RequestNotifyObject  = DispDataRequestComplete;
    Request.RequestContext       = Irp;
    Status = DispPrepareIrpForCancel(IrpSp->FileObject->FsContext, Irp, (PDRIVER_CANCEL)DispCancelRequest);
    if (NT_SUCCESS(Status)) {
        Status = UDPReceiveDatagram(&Request,
            DgramInfo->ReceiveDatagramInformation,
            (PNDIS_BUFFER)Irp->MdlAddress,
            DgramInfo->ReceiveLength,
            DgramInfo->ReceiveFlags,
            DgramInfo->ReturnDatagramInformation,
            &BytesReceived);
        if (Status != STATUS_PENDING) {
            DispDataRequestComplete(Irp, Status, BytesReceived);
            /* Return STATUS_PENDING because DispPrepareIrpForCancel marks Irp as pending */
            Status = STATUS_PENDING;
        }
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

    return Status;
}


NTSTATUS DispTdiSend(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiSendDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    TDI_REQUEST Request;
    PTDI_REQUEST_KERNEL_SENDDG DgramInfo;
    PTRANSPORT_CONTEXT TranContext;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    DgramInfo = (PTDI_REQUEST_KERNEL_SENDDG)&(IrpSp->Parameters);
    
    TranContext                  = IrpSp->FileObject->FsContext;
    /* Initialize a send request */
    Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
    Request.RequestNotifyObject  = DispDataRequestComplete;
    Request.RequestContext       = Irp;

    Status = DispPrepareIrpForCancel(IrpSp->FileObject->FsContext, Irp, (PDRIVER_CANCEL)DispCancelRequest);
    if (NT_SUCCESS(Status)) {

        /* FIXME: DgramInfo->SendDatagramInformation->RemoteAddress 
           must be of type PTDI_ADDRESS_IP */

        Status = (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send)(
            &Request, DgramInfo->SendDatagramInformation,
            (PNDIS_BUFFER)Irp->MdlAddress, DgramInfo->SendLength);
        if (Status != STATUS_PENDING) {
            DispDataRequestComplete(Irp, Status, 0);
            /* Return STATUS_PENDING because DispPrepareIrpForCancel marks Irp as pending */
            Status = STATUS_PENDING;
        }
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));

    return Status;
}


NTSTATUS DispTdiSetEventHandler(
    PIRP Irp)
/*
 * FUNCTION: TDI_SET_EVENT_HANDER handler
 * ARGUMENTS:
 *     Irp = Pointer to a I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
#ifdef _MSC_VER
    PTDI_REQUEST_KERNEL_SET_EVENT Parameters;
    PTRANSPORT_CONTEXT TranContext;
    PIO_STACK_LOCATION IrpSp;
    PADDRESS_FILE AddrFile;
    NTSTATUS Status;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get associated address file object. Quit if none exists */
    TranContext = IrpSp->FileObject->FsContext;
    if (!TranContext) {
        TI_DbgPrint(MIN_TRACE, ("Bad transport context.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
    if (!AddrFile) {
        TI_DbgPrint(MIN_TRACE, ("No address file object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    Parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)&IrpSp->Parameters;
    Status     = STATUS_SUCCESS;
    
    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

    /* Set the event handler. if an event handler is associated with
       a specific event, it's flag (RegisteredXxxHandler) is TRUE.
       If an event handler is not used it's flag is FALSE */
    switch (Parameters->EventType) {
    case TDI_EVENT_CONNECT:
        if (!Parameters->EventHandler) {
            AddrFile->ConnectionHandler =
                (PTDI_IND_CONNECT)TdiDefaultConnectHandler;
            AddrFile->ConnectionHandlerContext    = NULL;
            AddrFile->RegisteredConnectionHandler = FALSE;
        } else {
            AddrFile->ConnectionHandler =
                (PTDI_IND_CONNECT)Parameters->EventHandler;
            AddrFile->ConnectionHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredConnectionHandler = TRUE;
        }
        break;

    case TDI_EVENT_DISCONNECT:
        if (!Parameters->EventHandler) {
            AddrFile->DisconnectHandler =
                (PTDI_IND_DISCONNECT)TdiDefaultDisconnectHandler;
            AddrFile->DisconnectHandlerContext    = NULL;
            AddrFile->RegisteredDisconnectHandler = FALSE;
        } else {
            AddrFile->DisconnectHandler =
                (PTDI_IND_DISCONNECT)Parameters->EventHandler;
            AddrFile->DisconnectHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredDisconnectHandler = TRUE;
        }
        break;

    case TDI_EVENT_RECEIVE:
        if (Parameters->EventHandler == NULL) {
            AddrFile->ReceiveHandler =
                (PTDI_IND_RECEIVE)TdiDefaultReceiveHandler;
            AddrFile->ReceiveHandlerContext    = NULL;
            AddrFile->RegisteredReceiveHandler = FALSE;
        } else {
            AddrFile->ReceiveHandler =
                (PTDI_IND_RECEIVE)Parameters->EventHandler;
            AddrFile->ReceiveHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredReceiveHandler = TRUE;
        }
        break;

    case TDI_EVENT_RECEIVE_EXPEDITED:
        if (Parameters->EventHandler == NULL) {
            AddrFile->ExpeditedReceiveHandler =
                (PTDI_IND_RECEIVE_EXPEDITED)TdiDefaultRcvExpeditedHandler;
            AddrFile->ExpeditedReceiveHandlerContext    = NULL;
            AddrFile->RegisteredExpeditedReceiveHandler = FALSE;
        } else {
            AddrFile->ExpeditedReceiveHandler =
                (PTDI_IND_RECEIVE_EXPEDITED)Parameters->EventHandler;
            AddrFile->ExpeditedReceiveHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredExpeditedReceiveHandler = TRUE;
        }
        break;

    case TDI_EVENT_RECEIVE_DATAGRAM:
        if (Parameters->EventHandler == NULL) {
            AddrFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)TdiDefaultRcvDatagramHandler;
            AddrFile->ReceiveDatagramHandlerContext    = NULL;
            AddrFile->RegisteredReceiveDatagramHandler = FALSE;
        } else {
            AddrFile->ReceiveDatagramHandler =
                (PTDI_IND_RECEIVE_DATAGRAM)Parameters->EventHandler;
            AddrFile->ReceiveDatagramHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredReceiveDatagramHandler = TRUE;
        }
        break;

    case TDI_EVENT_ERROR:
        if (Parameters->EventHandler == NULL) {
            AddrFile->ErrorHandler =
                (PTDI_IND_ERROR)TdiDefaultErrorHandler;
            AddrFile->ErrorHandlerContext    = NULL;
            AddrFile->RegisteredErrorHandler = FALSE;
        } else {
            AddrFile->ErrorHandler =
                (PTDI_IND_ERROR)Parameters->EventHandler;
            AddrFile->ErrorHandlerContext    = Parameters->EventContext;
            AddrFile->RegisteredErrorHandler = TRUE;
        }
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
    }

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    return Status;
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}


NTSTATUS DispTdiSetInformation(
    PIRP Irp)
/*
 * FUNCTION: TDI_SET_INFORMATION handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


VOID DispTdiQueryInformationExComplete(
    PVOID Context,
    ULONG Status,
    UINT ByteCount)
/*
 * FUNCTION: Completes a TDI QueryInformationEx request
 * ARGUMENTS:
 *     Context   = Pointer to the IRP for the request
 *     Status    = TDI status of the request
 *     ByteCount = Number of bytes returned in output buffer
 */
{
    PTI_QUERY_CONTEXT QueryContext;
    UINT Count = 0;

    QueryContext = (PTI_QUERY_CONTEXT)Context;
    if (NT_SUCCESS(Status)) {
        Count = CopyBufferToBufferChain(
            QueryContext->InputMdl,
            FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX, Context),
            (PUCHAR)&QueryContext->QueryInfo.Context,
            CONTEXT_SIZE);
    }

    MmUnlockPages(QueryContext->InputMdl);
    IoFreeMdl(QueryContext->InputMdl);
    MmUnlockPages(QueryContext->OutputMdl);
    IoFreeMdl(QueryContext->OutputMdl);

    QueryContext->Irp->IoStatus.Information = Count;
    QueryContext->Irp->IoStatus.Status      = Status;

    ExFreePool(QueryContext);
}


NTSTATUS DispTdiQueryInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI QueryInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTCP_REQUEST_QUERY_INFORMATION_EX InputBuffer;
    PTRANSPORT_CONTEXT TranContext;
    PTI_QUERY_CONTEXT QueryContext;
    PVOID OutputBuffer;
    TDI_REQUEST Request;
    UINT Size;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    BOOLEAN InputMdlLocked  = FALSE;
    BOOLEAN OutputMdlLocked = FALSE;
    PMDL InputMdl           = NULL;
    PMDL OutputMdl          = NULL;
    NTSTATUS Status         = STATUS_SUCCESS;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength == sizeof(TCP_REQUEST_QUERY_INFORMATION_EX)) &&
        (OutputBufferLength != 0)) {

        InputBuffer = (PTCP_REQUEST_QUERY_INFORMATION_EX)
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        OutputBuffer = Irp->UserBuffer;

        QueryContext = ExAllocatePool(NonPagedPool, sizeof(TI_QUERY_CONTEXT));
        if (QueryContext) {
#ifdef _MSC_VER
            try {
#endif
                InputMdl = IoAllocateMdl(InputBuffer,
                    sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                    FALSE, TRUE, NULL);

                OutputMdl = IoAllocateMdl(OutputBuffer,
                    OutputBufferLength, FALSE, TRUE, NULL);

                if (InputMdl && OutputMdl) {

                    MmProbeAndLockPages(InputMdl, Irp->RequestorMode,
                        IoModifyAccess);

                    InputMdlLocked = TRUE;

                    MmProbeAndLockPages(OutputMdl, Irp->RequestorMode,
                        IoWriteAccess);

                    OutputMdlLocked = TRUE;

                    RtlCopyMemory(&QueryContext->QueryInfo,
                        InputBuffer, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

                } else
                    Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef _MSC_VER
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }
#endif
            if (NT_SUCCESS(Status)) {
                Size = MmGetMdlByteCount(OutputMdl);

                QueryContext->Irp       = Irp;
                QueryContext->InputMdl  = InputMdl;
                QueryContext->OutputMdl = OutputMdl;

                Request.RequestNotifyObject = DispTdiQueryInformationExComplete;
                Request.RequestContext      = QueryContext;
                Status = InfoTdiQueryInformationEx(&Request,
                    &QueryContext->QueryInfo.ID, OutputMdl,
                    &Size, &QueryContext->QueryInfo.Context);
                DispTdiQueryInformationExComplete(QueryContext, Status, Size);

                TI_DbgPrint(MAX_TRACE, ("Leaving. Status = (0x%X)\n", Status));

                return Status;
            }

            /* An error occurred if we get here */

            if (InputMdl) {
                if (InputMdlLocked)
                    MmUnlockPages(InputMdl);
                IoFreeMdl(InputMdl);
            }

            if (OutputMdl) {
                if (OutputMdlLocked)
                    MmUnlockPages(OutputMdl);
                IoFreeMdl(OutputMdl);
            }

            ExFreePool(QueryContext);
        } else
            Status = STATUS_INSUFFICIENT_RESOURCES;
    } else
        Status = STATUS_INVALID_PARAMETER;

    TI_DbgPrint(MIN_TRACE, ("Leaving. Status = (0x%X)\n", Status));

    return Status;
}


NTSTATUS DispTdiSetInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI SetInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTRANSPORT_CONTEXT TranContext;
    PTCP_REQUEST_SET_INFORMATION_EX Info;
    TDI_REQUEST Request;
    TDI_STATUS Status;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;
    Info        = (PTCP_REQUEST_SET_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return STATUS_INVALID_PARAMETER;
    }

    Status = DispPrepareIrpForCancel(TranContext, Irp, NULL);
    if (NT_SUCCESS(Status)) {
        Request.RequestNotifyObject = DispDataRequestComplete;
        Request.RequestContext      = Irp;

        Status = InfoTdiSetInformationEx(&Request, &Info->ID,
            &Info->Buffer, Info->BufferSize);

        if (Status != STATUS_PENDING) {
            IoAcquireCancelSpinLock(&OldIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(OldIrql);
        }
    }

    return Status;
}

/* EOF */
