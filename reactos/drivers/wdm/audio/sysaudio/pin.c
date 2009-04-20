/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

NTSTATUS
NTAPI
Pin_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    NTSTATUS Status;
    ULONG BytesReturned;
    PIO_STACK_LOCATION IoStack;

    DPRINT("Pin_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);
    ASSERT(Context->FileObject != NULL);

    /* Re-dispatch the request to the real target pin */
    Status = KsSynchronousIoControlDevice(Context->FileObject, KernelMode, IoStack->Parameters.DeviceIoControl.IoControlCode,
                                          IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                          IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                          Irp->UserBuffer,
                                          IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                          &BytesReturned);

    /* Save status and information */
    Irp->IoStatus.Information = BytesReturned;
    Irp->IoStatus.Status = Status;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    ULONG BytesReturned;
    NTSTATUS Status;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);
    ASSERT(Context->FileObject != NULL);

    /* Re-dispatch the request to the real target pin */
    Status = KsSynchronousIoControlDevice(Context->FileObject, KernelMode, IOCTL_KS_READ_STREAM,
                                          MmGetMdlVirtualAddress(Irp->MdlAddress),
                                          IoStack->Parameters.Read.Length,
                                          NULL,
                                          0,
                                          &BytesReturned);

    if (Context->hMixerPin && Context->MixerFileObject)
    {
        // FIXME
        // call kmixer to convert stream
        UNIMPLEMENTED
    }

    /* Save status and information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    ULONG BytesReturned;
    NTSTATUS Status;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);
    ASSERT(Context->FileObject != NULL);

    if (Context->hMixerPin && Context->MixerFileObject)
    {
        // FIXME
        // call kmixer to convert stream
        UNIMPLEMENTED
    }


    /* Re-dispatch the request to the real target pin */
    Status = KsSynchronousIoControlDevice(Context->FileObject, KernelMode, IOCTL_KS_WRITE_STREAM,
                                          MmGetMdlVirtualAddress(Irp->MdlAddress),
                                          IoStack->Parameters.Read.Length,
                                          NULL,
                                          0,
                                          &BytesReturned);

    /* Save status and information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDISPATCH_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT PinDeviceObject;
    PIRP PinIrp;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* The dispatch context is stored in the FsContext2 member */
    Context = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext2;

    /* Sanity check */
    ASSERT(Context);
    ASSERT(Context->FileObject != NULL);

    /* Get Pin's device object */
    PinDeviceObject = IoGetRelatedDeviceObject(Context->FileObject);

    /* Initialize notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* build target irp */
    PinIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS, PinDeviceObject, NULL, 0, NULL, &Event, &IoStatus);
    if (PinIrp)
    {

        /* Get the next stack location */
        IoStack = IoGetNextIrpStackLocation(PinIrp);
        /* The file object must be present in the irp as it contains the KSOBJECT_HEADER */
        IoStack->FileObject = Context->FileObject;

        /* call the driver */
        Status = IoCallDriver(PinDeviceObject, PinIrp);
        /* Has request already completed ? */
        if (Status == STATUS_PENDING)
        {
            /* Wait untill the request has completed */
            KeWaitForSingleObject(&Event, UserRequest, KernelMode, FALSE, NULL);
            /* Update status */
            Status = IoStatus.Status;
        }
    }

    /* store status */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    /* Complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* Done */
    return Status;
}

NTSTATUS
NTAPI
Pin_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT("Pin_fnClose called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Pin_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT("Pin_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    DPRINT("Pin_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
Pin_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Pin_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


    return FALSE;
}


BOOLEAN
NTAPI
Pin_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Pin_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

    return FALSE;

}

BOOLEAN
NTAPI
Pin_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PDISPATCH_CONTEXT Context;
    NTSTATUS Status;

    DPRINT("Pin_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

    Context = (PDISPATCH_CONTEXT)FileObject->FsContext2;

    if (Context->hMixerPin && Context->MixerFileObject)
    {
        Status = KsStreamIo(Context->MixerFileObject, NULL, NULL, NULL, NULL, 0, IoStatus, Buffer, Length, KSSTREAM_WRITE, KernelMode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Mixing stream failed with %lx\n", Status);
            return FALSE;
        }
    }

    Status = KsStreamIo(Context->FileObject, NULL, NULL, NULL, NULL, 0, IoStatus, Buffer, Length, KSSTREAM_WRITE, KernelMode);
    if (Status == STATUS_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

static KSDISPATCH_TABLE PinTable =
{
    Pin_fnDeviceIoControl,
    Pin_fnRead,
    Pin_fnWrite,
    Pin_fnFlush,
    Pin_fnClose,
    Pin_fnQuerySecurity,
    Pin_fnSetSecurity,
    Pin_fnFastDeviceIoControl,
    Pin_fnFastRead,
    Pin_fnFastWrite,
};

NTSTATUS
CreateDispatcher(
    IN PIRP Irp)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 0, NULL, Irp, &PinTable);
    return Status;
}
