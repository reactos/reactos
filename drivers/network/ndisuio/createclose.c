/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        createclose.c
 * PURPOSE:     IRP_MJ_CREATE and IRP_MJ_CLOSE handling
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
NduDispatchCreate(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(DeviceObject == GlobalDeviceObject);

    DPRINT("Created file object 0x%x\n", IrpSp->FileObject);

    /* This is associated with an adapter during IOCTL_NDISUIO_OPEN_(WRITE_)DEVICE */
    IrpSp->FileObject->FsContext = NULL;
    IrpSp->FileObject->FsContext2 = NULL;

    /* Completed successfully */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NduDispatchClose(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_OPEN_ENTRY OpenEntry = IrpSp->FileObject->FsContext2;

    ASSERT(DeviceObject == GlobalDeviceObject);

    DPRINT("Closing file object 0x%x\n", IrpSp->FileObject);

    /* Check if this handle was ever associated with an adapter */
    if (AdapterContext != NULL)
    {
        ASSERT(OpenEntry != NULL);

        DPRINT("Removing binding to adapter %wZ\n", &AdapterContext->DeviceName);

        /* Call the our helper */
        DereferenceAdapterContextWithOpenEntry(AdapterContext, OpenEntry);
    }

    /* Completed */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* Return success */
    return STATUS_SUCCESS;
}
