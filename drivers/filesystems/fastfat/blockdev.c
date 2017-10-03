/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/blockdev.c
 * PURPOSE:          Temporary sector reading support
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

static IO_COMPLETION_ROUTINE VfatReadWritePartialCompletion;

static
NTSTATUS
NTAPI
VfatReadWritePartialCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PVFAT_IRP_CONTEXT IrpContext;
    PMDL Mdl;

    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT("VfatReadWritePartialCompletion() called\n");

    IrpContext = (PVFAT_IRP_CONTEXT)Context;

    while ((Mdl = Irp->MdlAddress))
    {
        Irp->MdlAddress = Mdl->Next;
        IoFreeMdl(Mdl);
    }

    if (Irp->PendingReturned)
    {
        IrpContext->Flags |= IRPCONTEXT_PENDINGRETURNED;
    }
    else
    {
        IrpContext->Flags &= ~IRPCONTEXT_PENDINGRETURNED;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        IrpContext->Irp->IoStatus.Status = Irp->IoStatus.Status;
    }

    if (0 == InterlockedDecrement((PLONG)&IrpContext->RefCount) &&
        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_PENDINGRETURNED))
    {
        KeSetEvent(&IrpContext->Event, IO_NO_INCREMENT, FALSE);
    }

    IoFreeIrp(Irp);

    DPRINT("VfatReadWritePartialCompletion() done\n");

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VfatReadDisk(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PLARGE_INTEGER ReadOffset,
    IN ULONG ReadLength,
    IN OUT PUCHAR Buffer,
    IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    NTSTATUS Status;

again:
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    DPRINT("VfatReadDisk(pDeviceObject %p, Offset %I64x, Length %u, Buffer %p)\n",
           pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);

    DPRINT ("Building synchronous FSD Request...\n");
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       pDeviceObject,
                                       Buffer,
                                       ReadLength,
                                       ReadOffset,
                                       &Event,
                                       &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildSynchronousFsdRequest failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver (pDeviceObject, Irp);

    DPRINT("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        DPRINT("Getting IO Status... for %p\n", Irp);
        Status = IoStatus.Status;
    }

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        PDEVICE_OBJECT DeviceToVerify;

        DPRINT1 ("Media change detected!\n");

        /* Find the device to verify and reset the thread field to empty value again. */
        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
        Status = IoVerifyVolume(DeviceToVerify,
                                FALSE);
        if (NT_SUCCESS(Status))
        {
            DPRINT1("Volume verification successful; Reissuing read request\n");
            goto again;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IO failed!!! VfatReadDisk : Error code: %x\n", Status);
        DPRINT("(pDeviceObject %p, Offset %I64x, Size %u, Buffer %p\n",
               pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);
        return Status;
    }
    DPRINT("Block request succeeded for %p\n", Irp);
    return STATUS_SUCCESS;
}

NTSTATUS
VfatReadDiskPartial(
    IN PVFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER ReadOffset,
    IN ULONG ReadLength,
    ULONG BufferOffset,
    IN BOOLEAN Wait)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    PVOID Buffer;

    DPRINT("VfatReadDiskPartial(IrpContext %p, ReadOffset %I64x, ReadLength %u, BufferOffset %u, Wait %u)\n",
           IrpContext, ReadOffset->QuadPart, ReadLength, BufferOffset, Wait);

    DPRINT("Building asynchronous FSD Request...\n");

    Buffer = (PCHAR)MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

again:
    Irp = IoAllocateIrp(IrpContext->DeviceExt->StorageDevice->StackSize, TRUE);
    if (Irp == NULL)
    {
        DPRINT("IoAllocateIrp failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    Irp->UserIosb = NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_READ;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = IrpContext->DeviceExt->StorageDevice;
    StackPtr->FileObject = NULL;
    StackPtr->CompletionRoutine = NULL;
    StackPtr->Parameters.Read.Length = ReadLength;
    StackPtr->Parameters.Read.ByteOffset = *ReadOffset;

    if (!IoAllocateMdl(Buffer, ReadLength, FALSE, FALSE, Irp))
    {
        DPRINT("IoAllocateMdl failed\n");
        IoFreeIrp(Irp);
        return STATUS_UNSUCCESSFUL;
    }

    IoBuildPartialMdl(IrpContext->Irp->MdlAddress, Irp->MdlAddress, Buffer, ReadLength);

    IoSetCompletionRoutine(Irp,
                           VfatReadWritePartialCompletion,
                           IrpContext,
                           TRUE,
                           TRUE,
                           TRUE);

    if (Wait)
    {
        KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
        IrpContext->RefCount = 1;
    }
    else
    {
        InterlockedIncrement((PLONG)&IrpContext->RefCount);
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, Irp);

    if (Wait && Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
        Status = IrpContext->Irp->IoStatus.Status;
    }

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        PDEVICE_OBJECT DeviceToVerify;

        DPRINT1("Media change detected!\n");

        /* Find the device to verify and reset the thread field to empty value again. */
        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
        Status = IoVerifyVolume(DeviceToVerify,
                                FALSE);
        if (NT_SUCCESS(Status))
        {
            DPRINT1("Volume verification successful; Reissuing read request\n");
            goto again;
        }
    }

    DPRINT("%x\n", Status);
    return Status;
}


NTSTATUS
VfatWriteDiskPartial(
    IN PVFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER WriteOffset,
    IN ULONG WriteLength,
    IN ULONG BufferOffset,
    IN BOOLEAN Wait)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    PVOID Buffer;

    DPRINT("VfatWriteDiskPartial(IrpContext %p, WriteOffset %I64x, WriteLength %u, BufferOffset %x, Wait %u)\n",
           IrpContext, WriteOffset->QuadPart, WriteLength, BufferOffset, Wait);

    Buffer = (PCHAR)MmGetMdlVirtualAddress(IrpContext->Irp->MdlAddress) + BufferOffset;

again:
    DPRINT("Building asynchronous FSD Request...\n");
    Irp = IoAllocateIrp(IrpContext->DeviceExt->StorageDevice->StackSize, TRUE);
    if (Irp == NULL)
    {
        DPRINT("IoAllocateIrp failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    Irp->UserIosb = NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_WRITE;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = IrpContext->DeviceExt->StorageDevice;
    StackPtr->FileObject = NULL;
    StackPtr->CompletionRoutine = NULL;
    StackPtr->Parameters.Read.Length = WriteLength;
    StackPtr->Parameters.Read.ByteOffset = *WriteOffset;

    if (!IoAllocateMdl(Buffer, WriteLength, FALSE, FALSE, Irp))
    {
        DPRINT("IoAllocateMdl failed\n");
        IoFreeIrp(Irp);
        return STATUS_UNSUCCESSFUL;
    }

    IoBuildPartialMdl(IrpContext->Irp->MdlAddress, Irp->MdlAddress, Buffer, WriteLength);

    IoSetCompletionRoutine(Irp,
                           VfatReadWritePartialCompletion,
                           IrpContext,
                           TRUE,
                           TRUE,
                           TRUE);

    if (Wait)
    {
        KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
        IrpContext->RefCount = 1;
    }
    else
    {
        InterlockedIncrement((PLONG)&IrpContext->RefCount);
    }

    DPRINT("Calling IO Driver...\n");
    Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, Irp);
    if (Wait && Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
        Status = IrpContext->Irp->IoStatus.Status;
    }

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        PDEVICE_OBJECT DeviceToVerify;

        DPRINT1("Media change detected!\n");

        /* Find the device to verify and reset the thread field to empty value again. */
        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
        Status = IoVerifyVolume(DeviceToVerify,
                                 FALSE);
        if (NT_SUCCESS(Status))
        {
            DPRINT1("Volume verification successful; Reissuing write request\n");
            goto again;
        }
    }

    return Status;
}

NTSTATUS
VfatBlockDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CtlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferSize,
    IN OUT PVOID OutputBuffer OPTIONAL,
    IN OUT PULONG OutputBufferSize,
    IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    DPRINT("VfatBlockDeviceIoControl(DeviceObject %p, CtlCode %x, "
           "InputBuffer %p, InputBufferSize %x, OutputBuffer %p, "
           "OutputBufferSize %p (%x)\n", DeviceObject, CtlCode,
           InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize,
           OutputBufferSize ? *OutputBufferSize : 0);

again:
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    DPRINT("Building device I/O control request ...\n");
    Irp = IoBuildDeviceIoControlRequest(CtlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        (OutputBufferSize) ? *OutputBufferSize : 0,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildDeviceIoControlRequest failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);

    DPRINT("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);
        DPRINT("Getting IO Status... for %p\n", Irp);

        Status = IoStatus.Status;
    }

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        PDEVICE_OBJECT DeviceToVerify;

        DPRINT1("Media change detected!\n");

        /* Find the device to verify and reset the thread field to empty value again. */
        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
        Status = IoVerifyVolume(DeviceToVerify,
                                FALSE);

        if (NT_SUCCESS(Status))
        {
            DPRINT1("Volume verification successful; Reissuing IOCTL request\n");
            goto again;
        }
    }

    if (OutputBufferSize)
    {
        *OutputBufferSize = IoStatus.Information;
    }

    DPRINT("Returning Status %x\n", Status);

    return Status;
}
