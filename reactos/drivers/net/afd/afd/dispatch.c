/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/dispatch.c
 * PURPOSE:     File object dispatch functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <afd.h>

NTSTATUS AfdDispBind(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Binds to an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_BIND Request;
    PFILE_REPLY_BIND Reply;
    PAFDFCB FCB;

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_BIND)) &&
        (OutputBufferLength >= sizeof(FILE_REPLY_BIND))) {
        FCB = IrpSp->FileObject->FsContext;

        Request = (PFILE_REQUEST_BIND)Irp->AssociatedIrp.SystemBuffer;
        Reply   = (PFILE_REPLY_BIND)Irp->AssociatedIrp.SystemBuffer;

        switch (Request->Name.sa_family) {
        case AF_INET:
            Status = TdiOpenAddressFileIPv4(&FCB->TdiDeviceName,
                &Request->Name,
                &FCB->TdiAddressObjectHandle,
                &FCB->TdiAddressObject);
            break;
        default:
            AFD_DbgPrint(MIN_TRACE, ("Bad address family (%d).\n", Request->Name.sa_family));
            Status = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(Status)) {
            AfdRegisterEventHandlers(FCB);
            FCB->State = SOCKET_STATE_BOUND;
        }
    } else
        Status = STATUS_INVALID_PARAMETER;

    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return Status;
}


NTSTATUS AfdDispListen(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Starts listening for connections
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_LISTEN Request;
    PFILE_REPLY_LISTEN Reply;
    PAFDFCB FCB;

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_LISTEN)) &&
        (OutputBufferLength >= sizeof(FILE_REPLY_LISTEN))) {
        FCB = IrpSp->FileObject->FsContext;

        Request = (PFILE_REQUEST_LISTEN)Irp->AssociatedIrp.SystemBuffer;
        Reply   = (PFILE_REPLY_LISTEN)Irp->AssociatedIrp.SystemBuffer;
    } else
        Status = STATUS_INVALID_PARAMETER;

    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return Status;
}



NTSTATUS AfdDispSendTo(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Sends data to an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_SENDTO Request;
    PFILE_REPLY_SENDTO Reply;
    PAFDFCB FCB;

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_SENDTO)) &&
        (OutputBufferLength >= sizeof(FILE_REPLY_SENDTO))) {
        FCB = IrpSp->FileObject->FsContext;

        Request = (PFILE_REQUEST_SENDTO)Irp->AssociatedIrp.SystemBuffer;
        Reply   = (PFILE_REPLY_SENDTO)Irp->AssociatedIrp.SystemBuffer;

        Status = TdiSend(FCB->TdiAddressObject, Request);

        Reply->NumberOfBytesSent = Request->ToLen;
        Reply->Status = Status;
    } else
        Status = STATUS_INVALID_PARAMETER;

    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return Status;
}


NTSTATUS AfdDispRecvFrom(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Receives data from an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_RECVFROM Request;
    PFILE_REPLY_RECVFROM Reply;
    PAFDFCB FCB;

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_RECVFROM)) &&
        (OutputBufferLength >= sizeof(FILE_REPLY_RECVFROM))) {
        FCB = IrpSp->FileObject->FsContext;

        Request = (PFILE_REQUEST_RECVFROM)Irp->AssociatedIrp.SystemBuffer;
        Reply   = (PFILE_REPLY_RECVFROM)Irp->AssociatedIrp.SystemBuffer;
    } else
        Status = STATUS_INVALID_PARAMETER;

    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return Status;
}

/* EOF */
