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
    PVOID SystemVirtualAddress;
    ULONG BufferSize;
    ULONG BytesCopied;
    PMDL Mdl;

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_SENDTO)) &&
        (OutputBufferLength >= sizeof(FILE_REPLY_SENDTO))) {

        AFD_DbgPrint(MAX_TRACE, ("FileObject at (0x%X).\n", IrpSp->FileObject));
        AFD_DbgPrint(MAX_TRACE, ("FCB at (0x%X).\n", IrpSp->FileObject->FsContext));
        AFD_DbgPrint(MAX_TRACE, ("CCB at (0x%X).\n", IrpSp->FileObject->FsContext2));

        FCB = IrpSp->FileObject->FsContext;
        Request = (PFILE_REQUEST_SENDTO)Irp->AssociatedIrp.SystemBuffer;
        Reply   = (PFILE_REPLY_SENDTO)Irp->AssociatedIrp.SystemBuffer;
        BufferSize = WSABufferSize(Request->Buffers, Request->BufferCount);

        if (BufferSize != 0) {
            AFD_DbgPrint(MAX_TRACE, ("Allocating %d bytes for send buffer.\n", BufferSize));
            SystemVirtualAddress = ExAllocatePool(NonPagedPool, BufferSize);
            if (!SystemVirtualAddress) {
                AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Status = MergeWSABuffers(
                Request->Buffers,
                Request->BufferCount,
                SystemVirtualAddress,
                BufferSize,
                &BytesCopied);
            if (!NT_SUCCESS(Status)) {
                AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
                return Status;
            }
        } else {
            SystemVirtualAddress = NULL;
            BytesCopied = 0;
        }

        AFD_DbgPrint(MAX_TRACE, ("KeGetCurrentIrql() 1 (%d).\n", KeGetCurrentIrql()));

        Mdl = IoAllocateMdl(
            SystemVirtualAddress,   /* Virtual address of buffer */
            BufferSize,             /* Length of buffer */
            FALSE,                  /* Not secondary */
            FALSE,                  /* Don't charge quota */
            NULL);                  /* Don't use IRP */
        if (!Mdl) {
            AFD_DbgPrint(MIN_TRACE, ("IoAllocateMdl() failed.\n"));
            ExFreePool(SystemVirtualAddress);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AFD_DbgPrint(MAX_TRACE, ("KeGetCurrentIrql() 2 (%d).\n", KeGetCurrentIrql()));

        AFD_DbgPrint(MIN_TRACE, ("NDIS data buffer MdlFlags 1 is (0x%X).\n", Mdl->MdlFlags));

        MmBuildMdlForNonPagedPool(Mdl);

        AFD_DbgPrint(MAX_TRACE, ("KeGetCurrentIrql() 3 (%d).\n", KeGetCurrentIrql()));

        AFD_DbgPrint(MAX_TRACE, ("System virtual address is (0x%X).\n", SystemVirtualAddress));
        AFD_DbgPrint(MAX_TRACE, ("MDL for data buffer is at (0x%X).\n", Mdl));

        AFD_DbgPrint(MAX_TRACE, ("AFD.SYS: NDIS data buffer is at (0x%X).\n", Mdl));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer MdlFlags is (0x%X).\n", Mdl->MdlFlags));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer Next is at (0x%X).\n", Mdl->Next));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer Size is (0x%X).\n", Mdl->Size));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer MappedSystemVa is (0x%X).\n", Mdl->MappedSystemVa));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer StartVa is (0x%X).\n", Mdl->StartVa));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer ByteCount is (0x%X).\n", Mdl->ByteCount));
        AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer ByteOffset is (0x%X).\n", Mdl->ByteOffset));

#if 0
#ifdef _MSC_VER
    try {
#endif
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
#ifdef _MSC_VER
    } except(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        if (BufferSize != 0) {
            ExFreePool(SystemVirtualAddress);
        }
        return STATUS_UNSUCCESSFUL;
    }
#endif
#endif

        DisplayBuffer(SystemVirtualAddress, BufferSize);

        Status = STATUS_SUCCESS;
/*        Status = TdiSendDatagram(FCB->TdiAddressObject,
            &Request->To,
            Mdl,
            BufferSize);*/

        /* FIXME: Assumes synchronous operation */
#if 0
        MmUnlockPages(Mdl);
#endif

        IoFreeMdl(Mdl);

        AFD_DbgPrint(MAX_TRACE, ("KeGetCurrentIrql() 4 (%d).\n", KeGetCurrentIrql()));

        if (BufferSize != 0) {
            ExFreePool(SystemVirtualAddress);
        }

        AFD_DbgPrint(MAX_TRACE, ("KeGetCurrentIrql() 5 (%d).\n", KeGetCurrentIrql()));

        Reply->NumberOfBytesSent = BufferSize;
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
