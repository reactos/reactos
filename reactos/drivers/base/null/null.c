/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/base/null/null.c
 * PURPOSE:         Beep Device Driver
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntddk.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NullQueryFileInformation(OUT PVOID Buffer,
                         IN PULONG Length,
                         IN FILE_INFORMATION_CLASS InformationClass)
{
    PFILE_STANDARD_INFORMATION StandardInfo = Buffer;
    PAGED_CODE();

    /* We only support one class */
    if (InformationClass != FileStandardInformation)
    {
        /* Fail */
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Fill out the information */
    RtlZeroMemory(StandardInfo, sizeof(FILE_STANDARD_INFORMATION));
    StandardInfo->NumberOfLinks = 1;

    /* Return the length and success */
    *Length = sizeof(FILE_STANDARD_INFORMATION);
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
NullRead(IN PFILE_OBJECT FileObject,
         IN PLARGE_INTEGER FileOffset,
         IN ULONG Length,
         IN BOOLEAN Wait,
         IN ULONG LockKey,
         OUT PVOID Buffer,
         OUT PIO_STATUS_BLOCK IoStatus,
         IN PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    /* Complete successfully */
    IoStatus->Status = STATUS_END_OF_FILE;
    IoStatus->Information = 0;
    return TRUE;
}

BOOLEAN
NTAPI
NullWrite(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          IN BOOLEAN Wait,
          IN ULONG LockKey,
          IN PVOID Buffer,
          OUT PIO_STATUS_BLOCK IoStatus,
          IN PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    /* Complete successfully */
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;
    return TRUE;
}

NTSTATUS
STDCALL
NullDispatch(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    ULONG Length;
    PAGED_CODE();

    /* Get the file object and check what kind of request this is */
    FileObject = IoStack->FileObject;
    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:

            /* Check if this is synch I/O */
            if (FileObject->Flags & FO_SYNCHRONOUS_IO)
            {
                /* Set distinguished value for Cc */
                FileObject->PrivateCacheMap = (PVOID)1;
            }

            /* Complete successfully */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

         case IRP_MJ_READ:

            /* Return as if we read the entire file */
            Irp->IoStatus.Status = STATUS_END_OF_FILE;
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_WRITE:

            /* Return as if we wrote the entire request */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = IoStack->Parameters.Write.Length;
            break;

        case IRP_MJ_LOCK_CONTROL:

            /* Dummy */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_QUERY_INFORMATION:

            /* Get the length inputted and do the request */
            Length = IoStack->Parameters.QueryFile.Length;
            Irp->IoStatus.Status = NullQueryFileInformation(Irp->AssociatedIrp.
                                                            SystemBuffer,
                                                            &Length,
                                                            IoStack->
                                                            Parameters.
                                                            QueryFile.
                                                            FileInformationClass);

            /* Return the actual length */
            Irp->IoStatus.Information = Length;
            break;
    }

    /* Complete the request */
    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Null");
    NTSTATUS Status;
    PFAST_IO_DISPATCH FastIoDispatch;
    PAGED_CODE();

    /* Page the driver */
    MmPageEntireDriver(DriverEntry);

    /* Create null device */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_NULL,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    /* Register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = NullDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = NullDispatch;

    /* Allocate the fast I/O dispatch table */
    FastIoDispatch = ExAllocatePool(NonPagedPool, sizeof(FAST_IO_DISPATCH));
    if (!FastIoDispatch)
    {
        /* Failed, cleanup */
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize it */
    RtlZeroMemory(FastIoDispatch, sizeof(FAST_IO_DISPATCH));
    FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);

    /* Setup our pointers */
    FastIoDispatch->FastIoRead = NullRead;
    FastIoDispatch->FastIoWrite = NullWrite;
    DriverObject->FastIoDispatch = FastIoDispatch;

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
