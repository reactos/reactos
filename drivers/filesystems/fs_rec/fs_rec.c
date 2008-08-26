/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/fs_rec.c
 * PURPOSE:          Main Driver Entrypoint and FS Registration
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

PKEVENT FsRecLoadSync;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FsRecLoadFileSystem(IN PDEVICE_OBJECT DeviceObject,
                    IN PWCHAR DriverServiceName)
{
    UNICODE_STRING DriverName;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status = STATUS_IMAGE_ALREADY_LOADED;
    PAGED_CODE();

    /* Make sure we haven't already been called */
    if (DeviceExtension->State != Loaded)
    {
        /* Acquire the load lock */
        KeWaitForSingleObject(FsRecLoadSync,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        KeEnterCriticalRegion();

        /* Make sure we're active */
        if (DeviceExtension->State == Pending)
        {
            /* Load the FS driver */
            RtlInitUnicodeString(&DriverName, DriverServiceName);
            Status = ZwLoadDriver(&DriverName);

            /* Loop all the linked recognizer objects */
            while (DeviceExtension->State != Unloading)
            {
                /* Set them to the unload state */
                DeviceExtension->State = Unloading;

                /* Go to the next one */
                DeviceObject = DeviceExtension->Alternate;
                DeviceExtension = DeviceObject->DeviceExtension;
            }
        }

        /* Make sure that we haven't already loaded the FS */
        if (DeviceExtension->State != Loaded)
        {
            /* Unregiser us, and set us as loaded */
            IoUnregisterFileSystem(DeviceObject);
            DeviceExtension->State = Loaded;
        }

        /* Release the lock */
        KeSetEvent(FsRecLoadSync, 0, FALSE);
        KeLeaveCriticalRegion();
    }

    /* Return */
    return Status;
}

DRIVER_DISPATCH FsRecCreate;
NTSTATUS
STDCALL
FsRecCreate(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PAGED_CODE();

    /* Make sure we have a file name */
    if (IoStack->FileObject->FileName.Length)
    {
        /* Fail the request */
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
    }
    else
    {
        /* Let it through */
        Status = STATUS_SUCCESS;
    }

    /* Complete the IRP */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

DRIVER_DISPATCH FsRecClose;
NTSTATUS
STDCALL
FsRecClose(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PAGED_CODE();

    /* Just complete the IRP and return success */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

DRIVER_DISPATCH FsRecFsControl;
NTSTATUS
STDCALL
FsRecFsControl(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check the file system type */
    switch (DeviceExtension->FsType)
    {
        case FS_TYPE_VFAT:

            /* Send FAT command */
            Status = FsRecVfatFsControl(DeviceObject, Irp);
            break;

        case FS_TYPE_NTFS:

            /* Send NTFS command */
            Status = FsRecNtfsFsControl(DeviceObject, Irp);
            break;

        case FS_TYPE_CDFS:

            /* Send CDFS command */
            Status = FsRecCdfsFsControl(DeviceObject, Irp);
            break;

        case FS_TYPE_UDFS:

            /* Send UDFS command */
            Status = FsRecUdfsFsControl(DeviceObject, Irp);
            break;

        case FS_TYPE_EXT2:

            /* Send EXT2 command */
            Status = FsRecExt2FsControl(DeviceObject, Irp);
            break;

        default:

            /* Unrecognized FS */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Complete the IRP */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

DRIVER_UNLOAD FsRecUnload;
VOID
STDCALL
FsRecUnload(IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    /* Loop all driver device objects */
    while (DriverObject->DeviceObject)
    {
        /* Delete this device */
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    /* Free the lock */
    ExFreePool(FsRecLoadSync);
}

NTSTATUS
STDCALL
FsRecRegisterFs(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT ParentObject OPTIONAL,
                OUT PDEVICE_OBJECT *NewDeviceObject OPTIONAL,
                IN PCWSTR FsName,
                IN PCWSTR RecognizerName,
                IN ULONG FsType,
                IN DEVICE_TYPE DeviceType)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_EXTENSION DeviceExtension;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;
    HANDLE FileHandle;
    NTSTATUS Status;

    /* Assume failure */
    if (NewDeviceObject) *NewDeviceObject = NULL;

    /* Setup the attributes */
    RtlInitUnicodeString(&DeviceName, FsName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);

    /* Open the device */
    Status = ZwCreateFile(&FileHandle,
                          SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        /* We suceeded, close the handle */
        ZwClose(FileHandle);
    }
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* We failed with anything else then what we want to fail with */
        Status = STATUS_SUCCESS;
    }

    /* If we succeeded, there's no point in trying this again */
    if (NT_SUCCESS(Status)) return STATUS_IMAGE_ALREADY_LOADED;

    /* Create recognizer device object */
    RtlInitUnicodeString(&DeviceName, RecognizerName);
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceName,
                            DeviceType,
                            0,
                            FALSE,
                            &DeviceObject);
    if (NT_SUCCESS(Status))
    {
        /* Get the device extension and set it up */
        DeviceExtension = DeviceObject->DeviceExtension;
        DeviceExtension->FsType = FsType;
        DeviceExtension->State = Pending;

        /* Do we have a parent? */
        if (ParentObject)
        {
            /* Link it in */
            DeviceExtension->Alternate =
                ((PDEVICE_EXTENSION)ParentObject->DeviceExtension)->Alternate;
            ((PDEVICE_EXTENSION)ParentObject->DeviceExtension)->Alternate =
                DeviceObject;
        }
        else
        {
            /* Otherwise, we're the only one */
            DeviceExtension->Alternate = DeviceObject;
        }

        /* Return the DO if needed */
        if (NewDeviceObject) *NewDeviceObject = DeviceObject;

        /* Register the file system */
        IoRegisterFileSystem(DeviceObject);
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    ULONG DeviceCount = 0;
    NTSTATUS Status;
    PDEVICE_OBJECT UdfsObject;
    PAGED_CODE();

    /* Page the entire driver */
    MmPageEntireDriver(DriverEntry);

    /* Allocate the lock */
    FsRecLoadSync = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(KEVENT),
                                          FSREC_TAG);
    if (!FsRecLoadSync) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize it */
    KeInitializeEvent(FsRecLoadSync, SynchronizationEvent, TRUE);

    /* Setup the major functions */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsRecCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsRecClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FsRecClose;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FsRecFsControl;
    DriverObject->DriverUnload = FsRecUnload;

    /* Register CDFS */
    Status = FsRecRegisterFs(DriverObject,
                             NULL,
                             NULL,
                             L"\\Cdfs",
                             L"\\FileSystem\\CdfsRecognizer",
                             FS_TYPE_CDFS,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;

    /* Register UDFS for CDs */
    Status = FsRecRegisterFs(DriverObject,
                             NULL,
                             &UdfsObject,
                             L"\\UdfsCdRom",
                             L"\\FileSystem\\UdfsCdRomRecognizer",
                             FS_TYPE_UDFS,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;

    /* Register UDFS for HDDs */
    Status = FsRecRegisterFs(DriverObject,
                             UdfsObject,
                             NULL,
                             L"\\UdfsDisk",
                             L"\\FileSystem\\UdfsDiskRecognizer",
                             FS_TYPE_UDFS,
                             FILE_DEVICE_DISK_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;

    /* Register FAT */
    Status = FsRecRegisterFs(DriverObject,
                             NULL,
                             NULL,
                             L"\\Fat",
                             L"\\FileSystem\\FatRecognizer",
                             FS_TYPE_VFAT,
                             FILE_DEVICE_DISK_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;

    /* Register NTFS */
    Status = FsRecRegisterFs(DriverObject,
                             NULL,
                             NULL,
                             L"\\Ntfs",
                             L"\\FileSystem\\NtfsRecognizer",
                             FS_TYPE_NTFS,
                             FILE_DEVICE_DISK_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;

    /* Register EXT2 */
    /*Status = FsRecRegisterFs(DriverObject,
                             NULL,
                             NULL,
                             L"\\Ext2",
                             L"\\FileSystem\\Ext2Recognizer",
                             FS_TYPE_EXT2,
                             FILE_DEVICE_DISK_FILE_SYSTEM);
    if (NT_SUCCESS(Status)) DeviceCount++;*/

    /* Return appropriate Status */
    return (DeviceCount > 0) ? STATUS_SUCCESS : STATUS_IMAGE_ALREADY_LOADED;
}

/* EOF */
