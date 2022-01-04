/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            modules/rosapps/drivers/vcdrom/vcdrom.c
 * PURPOSE:         Virtual CD-ROM class driver
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntddk.h>
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

#include "vcdioctl.h"

typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    /* File object to the ISO when opened */
    PFILE_OBJECT VolumeObject;
    /* Size of the ISO */
    LARGE_INTEGER VolumeSize;
    /* Mandatory change count for verify */
    ULONG ChangeCount;
    /* Will contain the device name or the drive letter */
    UNICODE_STRING GlobalName;
    /* Will contain the image path */
    UNICODE_STRING ImageName;
    /* Flags on mount (supp. of UDF/Joliet) */
    ULONG Flags;
    /* Faking CD structure */
    ULONG SectorSize;
    ULONG SectorShift;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Taken from CDFS */
#define TOC_DATA_TRACK 0x04
#define TOC_LAST_TRACK 0xaa

/* Should cover most of our usages */
#define DEFAULT_STRING_SIZE 50

/* Increment on device creation, protection by the mutex */
FAST_MUTEX ViMutex;
ULONG ViDevicesCount;

VOID
ViInitializeDeviceExtension(PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Zero mandatory fields, the rest will be zeroed when needed */
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->VolumeObject = NULL;
    DeviceExtension->ChangeCount = 0;
    DeviceExtension->GlobalName.Buffer = NULL;
    DeviceExtension->GlobalName.MaximumLength = 0;
    DeviceExtension->GlobalName.Length = 0;
    DeviceExtension->ImageName.Buffer = NULL;
    DeviceExtension->ImageName.MaximumLength = 0;
    DeviceExtension->ImageName.Length = 0;
}

NTSTATUS
ViAllocateUnicodeString(USHORT BufferLength, PUNICODE_STRING UnicodeString)
{
    PVOID Buffer;

    /* Allocate the buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferLength, ' dCV');
    /* Initialize */
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferLength;
    UnicodeString->Buffer = Buffer;

    /* Return success if it went fine */
    if (Buffer != NULL)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_NO_MEMORY;
}

VOID
ViFreeUnicodeString(PUNICODE_STRING UnicodeString)
{
    /* Only free if allocate, that allows using this
     * on cleanup in short memory situations
     */
    if (UnicodeString->Buffer != NULL)
    {
        ExFreePoolWithTag(UnicodeString->Buffer, 0);
        UnicodeString->Buffer = NULL;
    }

    /* Zero the rest */
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = 0;
}

NTSTATUS
ViSetIoStatus(NTSTATUS Status, ULONG_PTR Information, PIRP Irp)
{
    /* Only set what we got */
    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;

    /* And return the status, so that caller can return with us */
    return Status;
}

NTSTATUS
NTAPI
VcdHandle(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    /* Stub for CREATE, CLEANUP, CLOSE, always a succes */
    ViSetIoStatus(STATUS_SUCCESS, 0, Irp);
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
ViDeleteDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Delete the drive letter */
    Status = IoDeleteSymbolicLink(&DeviceExtension->GlobalName);
    ViFreeUnicodeString(&DeviceExtension->GlobalName);

    /* Close the ISO */
    if (DeviceExtension->VolumeObject != NULL)
    {
        ObDereferenceObject(DeviceExtension->VolumeObject);
        DeviceExtension->VolumeObject = NULL;
    }

    /* Free the ISO name */
    ViFreeUnicodeString(&DeviceExtension->ImageName);

    /* And delete the device */
    IoDeleteDevice(DeviceObject);
    return ViSetIoStatus(Status, 0, Irp);
}

VOID
NTAPI
VcdUnload(PDRIVER_OBJECT DriverObject)
{
    IRP FakeIrp;
    PDEVICE_OBJECT DeviceObject;

    /* No device, nothing to free */
    DeviceObject = DriverObject->DeviceObject;
    if (DeviceObject == NULL)
    {
        return;
    }

    /* Delete any device we could have created */
    do
    {
        PDEVICE_OBJECT NextDevice;

        NextDevice = DeviceObject->NextDevice;
        /* This is normally called on IoCtl, so fake
         * the IRP so that status can be dummily set
         */
        ViDeleteDevice(DeviceObject, &FakeIrp);
        DeviceObject = NextDevice;
    } while (DeviceObject != NULL);
}

NTSTATUS
ViCreateDriveLetter(PDRIVER_OBJECT DriverObject, WCHAR Letter, WCHAR *EffectiveLetter, PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    WCHAR DriveLetter[3], CurLetter;
    PDEVICE_OBJECT LocalDeviceObject;
    PDEVICE_EXTENSION DeviceExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceCount, DeviceName;

    *DeviceObject = NULL;

    /* Allocate our buffers */
    ViAllocateUnicodeString(DEFAULT_STRING_SIZE, &DeviceCount);
    ViAllocateUnicodeString(DEFAULT_STRING_SIZE, &DeviceName);

    /* For easier cleanup */
    _SEH2_TRY
    {
        /* Get our device number */
        ExAcquireFastMutex(&ViMutex);
        Status = RtlIntegerToUnicodeString(ViDevicesCount, 10, &DeviceCount);
        ++ViDevicesCount;
        ExReleaseFastMutex(&ViMutex);

        /* Conversion to string failed, bail out */
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        /* Create our device name */
        Status = RtlAppendUnicodeToString(&DeviceName, L"\\Device\\VirtualCdRom");
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        Status = RtlAppendUnicodeStringToString(&DeviceName, &DeviceCount);
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        /* And create the device! */
        Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &DeviceName,
                                FILE_DEVICE_CD_ROM,
                                FILE_READ_ONLY_DEVICE | FILE_FLOPPY_DISKETTE,
                                FALSE, &LocalDeviceObject);
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        /* Initialize our DE */
        ViInitializeDeviceExtension(LocalDeviceObject);
        DeviceExtension = LocalDeviceObject->DeviceExtension;
        ViAllocateUnicodeString(DEFAULT_STRING_SIZE, &DeviceExtension->GlobalName);

        /* Now, we'll try to find a free drive letter
         * We always start from Z and go backward
         */
        for (CurLetter = Letter; CurLetter >= 'A'; CurLetter--)
        {
            /* By default, no flags. These will be set
             * on mount, by the caller
             */
            DeviceExtension->Flags = 0;

            /* Create a drive letter name */
            DeviceExtension->GlobalName.Length = 0;
            Status = RtlAppendUnicodeToString(&DeviceExtension->GlobalName, L"\\??\\");
            if (!NT_SUCCESS(Status))
            {
                _SEH2_LEAVE;
            }

            DriveLetter[0] = CurLetter;
            DriveLetter[1] = L':';
            DriveLetter[2] = UNICODE_NULL;
            Status = RtlAppendUnicodeToString(&DeviceExtension->GlobalName, DriveLetter);
            if (!NT_SUCCESS(Status))
            {
                _SEH2_LEAVE;
            }

            /* And try to open it */
            InitializeObjectAttributes(&ObjectAttributes, &DeviceExtension->GlobalName, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, NULL, NULL);
            Status = ZwOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &ObjectAttributes);
            /* It failed; good news, that letter is free, jump on it! */
            if (!NT_SUCCESS(Status))
            {
                /* Create a symbolic link to our device */
                Status = IoCreateSymbolicLink(&DeviceExtension->GlobalName, &DeviceName);
                /* If created... */
                if (NT_SUCCESS(Status))
                {
                    /* Return the drive letter to the caller */
                    *EffectiveLetter = CurLetter;
                    ClearFlag(LocalDeviceObject->Flags, DO_DEVICE_INITIALIZING);
                    /* No caching! */
                    SetFlag(LocalDeviceObject->Flags, DO_DIRECT_IO);

                    /* And return the DO */
                    *DeviceObject = LocalDeviceObject;
                    break;
                }
            }
            /* This letter is taken, try another one */
            else
            {
                ZwClose(LinkHandle);
                Status = STATUS_OBJECT_NAME_EXISTS;
            }
        }

        /* We're out of drive letters, so fail :-( */
        if (CurLetter == (L'A' - 1))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    _SEH2_FINALLY
    {
        /* No longer need these */
        ViFreeUnicodeString(&DeviceName);
        ViFreeUnicodeString(&DeviceCount);

        /* And delete in case of a failure */
        if (!NT_SUCCESS(Status) && Status != STATUS_VERIFY_REQUIRED && LocalDeviceObject != NULL)
        {
            IoDeleteDevice(LocalDeviceObject);
        }
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
ViMountImage(PDEVICE_OBJECT DeviceObject, PUNICODE_STRING Image)
{
    NTSTATUS Status;
    HANDLE ImgHandle;
    IO_STATUS_BLOCK IoStatus;
    ULONG Buffer[2], i, SectorShift;
    PDEVICE_EXTENSION DeviceExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_STANDARD_INFORMATION FileStdInfo;

    /* Try to open the image */
    InitializeObjectAttributes(&ObjectAttributes, Image, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwCreateFile(&ImgHandle, FILE_READ_DATA | SYNCHRONIZE, &ObjectAttributes,
                          &IoStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open image: %wZ\n", Image);
        return Status;
    }

    /* Query its information */
    Status = ZwQueryInformationFile(ImgHandle, &IoStatus, &FileStdInfo,
                                    sizeof(FileStdInfo), FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query image information\n");
        ZwClose(ImgHandle);
        return Status;
    }

    /* Set image size */
    DeviceExtension = DeviceObject->DeviceExtension;
    DeviceExtension->VolumeSize.QuadPart = FileStdInfo.EndOfFile.QuadPart;

    /* Read the first 8-bytes to determine our sector size */
    Status = ZwReadFile(ImgHandle, NULL, NULL, NULL, &IoStatus, Buffer, sizeof(Buffer), NULL, NULL);
    if (!NT_SUCCESS(Status) || Buffer[1] == 0 || (Buffer[1] & 0x1FF) != 0)
    {
        DeviceExtension->SectorSize = 2048;
    }
    else
    {
        DeviceExtension->SectorSize = Buffer[1];
    }

    /* Now, compute the sector shift */
    if (DeviceExtension->SectorSize == 512)
    {
        DeviceExtension->SectorShift = 9;
    }
    else
    {
        for (i = 512, SectorShift = 9; i < DeviceExtension->SectorSize; i = i * 2, ++SectorShift);
        if (i == DeviceExtension->SectorSize)
        {
            DeviceExtension->SectorShift = SectorShift;
        }
        else
        {
            DeviceExtension->SectorShift = 11;
        }
    }

    /* We're good, get the image file object to close the handle */
    Status = ObReferenceObjectByHandle(ImgHandle, 0, NULL, KernelMode,
                                       (PVOID *)&DeviceExtension->VolumeObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(ImgHandle);
        DeviceExtension->VolumeObject = NULL;
        return Status;
    }

    ZwClose(ImgHandle);
    /* Increase change count to force a verify */
    ++DeviceExtension->ChangeCount;

    /* And ask for a verify! */
    SetFlag(DeviceObject->Flags, DO_VERIFY_VOLUME);

    /* Free old string (if any) */
    ViFreeUnicodeString(&DeviceExtension->ImageName);
    /* And save our new image name */
    ViAllocateUnicodeString(Image->MaximumLength, &DeviceExtension->ImageName);
    RtlCopyUnicodeString(&DeviceExtension->ImageName, Image);

    return STATUS_SUCCESS;
}

VOID
ViCreateDriveAndMountImage(PDRIVER_OBJECT DriverObject, WCHAR Letter, LPCWSTR ImagePath)
{
    UNICODE_STRING Image;
    WCHAR EffectiveLetter;
    PDEVICE_OBJECT DeviceObject;

    /* Create string from path */
    DeviceObject = NULL;
    RtlInitUnicodeString(&Image, ImagePath);

    /* Create the drive letter (ignore output, it comes from registry, nothing to do */
    ViCreateDriveLetter(DriverObject, Letter, &EffectiveLetter, &DeviceObject);
    /* And mount the image on the created drive */
    ViMountImage(DeviceObject, &Image);
}

VOID
ViLoadImagesFromRegistry(PDRIVER_OBJECT DriverObject, LPCWSTR RegistryPath)
{
    WCHAR Letter[2];
    WCHAR PathBuffer[100], ResBuffer[100];

    /* We'll browse the registry for
     * DeviceX\IMAGE = {Path}
     * When found, we create (or at least, attempt to)
     * device X: and mount image Path
     */
    Letter[0] = L'A';
    Letter[1] = UNICODE_NULL;
    do
    {
        NTSTATUS Status;
        UNICODE_STRING Path, ResString;
        RTL_QUERY_REGISTRY_TABLE QueryTable[3];

        /* Our path is always Device + drive letter */
        RtlZeroMemory(PathBuffer, sizeof(PathBuffer));
        Path.Buffer = PathBuffer;
        Path.Length = 0;
        Path.MaximumLength = sizeof(PathBuffer);
        RtlAppendUnicodeToString(&Path, L"Parameters\\Device");
        RtlAppendUnicodeToString(&Path, Letter);

        ResString.Buffer = ResBuffer;
        ResString.Length = 0;
        ResString.MaximumLength = sizeof(ResBuffer);

        RtlZeroMemory(&QueryTable[0], sizeof(QueryTable));
        QueryTable[0].Name = Path.Buffer;
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[1].Name = L"IMAGE";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &ResString;

        /* If query went fine, attempt to create and mount */
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, RegistryPath, QueryTable, NULL, NULL);
        if (NT_SUCCESS(Status))
        {
            ViCreateDriveAndMountImage(DriverObject, Letter[0], ResString.Buffer);
        }

        ++(Letter[0]);
    } while (Letter[0] <= L'Z');
}

NTSTATUS
ViVerifyVolume(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    /* Force verify */
    SetFlag(DeviceObject->Flags, DO_VERIFY_VOLUME);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;

    IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);

    return STATUS_VERIFY_REQUIRED;
}

NTSTATUS
ViReadFile(PFILE_OBJECT File, PMDL Mdl, PLARGE_INTEGER Offset, PKEVENT Event, ULONG Length, PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP LowerIrp;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;

    /* Get the lower DO */
    DeviceObject = IoGetRelatedDeviceObject(File);
    /* Allocate an IRP to deal with it */
    LowerIrp = IoAllocateIrp(DeviceObject->StackSize, 0);
    if (LowerIrp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize it */
    LowerIrp->RequestorMode = KernelMode;
    /* Our status block */
    LowerIrp->UserIosb = IoStatusBlock;
    LowerIrp->MdlAddress = Mdl;
    /* We don't cache! */
    LowerIrp->Flags = IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO;
    /* Sync event for us to know when read is done */
    LowerIrp->UserEvent = Event;
    LowerIrp->UserBuffer = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    /* The FO of our image */
    LowerIrp->Tail.Overlay.OriginalFileObject = File;
    LowerIrp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* We basically perform a read operation, nothing complex here */
    Stack = IoGetNextIrpStackLocation(LowerIrp);
    Stack->Parameters.Read.Length = Length;
    Stack->MajorFunction = IRP_MJ_READ;
    Stack->FileObject = File;
    Stack->Parameters.Read.ByteOffset.QuadPart = Offset->QuadPart;

    /* And call lower driver */
    return IoCallDriver(DeviceObject, LowerIrp);
}

NTSTATUS
NTAPI
VcdRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    /* Catch any exception that could happen during read attempt */
    _SEH2_TRY
    {
        /* First of all, check if there's an image mounted */
        DeviceExtension = DeviceObject->DeviceExtension;
        if (DeviceExtension->VolumeObject == NULL)
        {
            Status = ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
            _SEH2_LEAVE;
        }

        /* Check if volume has to be verified (or if we override, for instance
         * FSD performing initial reads for mouting FS)
         */
        Stack = IoGetCurrentIrpStackLocation(Irp);
        if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
            !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
        {
            Status = ViVerifyVolume(DeviceObject, Irp);
            _SEH2_LEAVE;
        }

        /* Check if we have enough room for output */
        if (Stack->Parameters.Read.Length > Irp->MdlAddress->ByteCount)
        {
            Status = ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, 0, Irp);
            _SEH2_LEAVE;
        }

        /* Initialize our event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Perform actual read */
        Status = ViReadFile(DeviceExtension->VolumeObject, Irp->MdlAddress,
                            &Stack->Parameters.Read.ByteOffset, &Event,
                            Stack->Parameters.Read.Length, &IoStatus);
        if (!NT_SUCCESS(Status))
        {
            Status = ViSetIoStatus(Status, 0, Irp);
            _SEH2_LEAVE;
        }

        /* Make sure we wait until its done */
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        if (!NT_SUCCESS(Status))
        {
            Status = ViSetIoStatus(Status, 0, Irp);
            _SEH2_LEAVE;
        }

        /* Look whether we're reading first bytes of the volume, if so, our
         * "suppression" might be asked for
         */
        if (Stack->Parameters.Read.ByteOffset.QuadPart / DeviceExtension->SectorSize < 0x20)
        {
            /* Check our output buffer is in a state where we can play with it */
            if ((Irp->MdlAddress->MdlFlags & (MDL_ALLOCATED_FIXED_SIZE | MDL_PAGES_LOCKED | MDL_MAPPED_TO_SYSTEM_VA)) ==
                (MDL_ALLOCATED_FIXED_SIZE | MDL_PAGES_LOCKED | MDL_MAPPED_TO_SYSTEM_VA))
            {
                PUCHAR Buffer;

                /* Do we have to delete any UDF mark? */
                if (BooleanFlagOn(DeviceExtension->Flags, MOUNT_FLAG_SUPP_UDF))
                {
                    /* Kill any NSR0X mark from UDF */
                    Buffer = (PUCHAR)((ULONG_PTR)Irp->MdlAddress->StartVa + Irp->MdlAddress->ByteOffset);
                    if (Buffer != NULL)
                    {
                        if (Buffer[0] == 0 && Buffer[1] == 'N' && Buffer[2] == 'S' &&
                            Buffer[3] == 'R' && Buffer[4] == '0')
                        {
                            Buffer[5] = '0';
                        }
                    }
                }

                /* Do we have to delete any Joliet mark? */
                if (BooleanFlagOn(DeviceExtension->Flags, MOUNT_FLAG_SUPP_JOLIET))
                {
                    /* Kill the CD001 mark from Joliet */
                    Buffer = (PUCHAR)((ULONG_PTR)Irp->MdlAddress->StartVa + Irp->MdlAddress->ByteOffset);
                    if (Buffer != NULL)
                    {
                        if (Buffer[0] == 2 && Buffer[1] == 'C' && Buffer[2] == 'D' &&
                            Buffer[3] == '0' && Buffer[4] == '0' && Buffer[5] == '1')
                        {
                            Buffer[5] = '0';
                        }
                    }
                }
            }
        }

        /* Set status */
        Status = ViSetIoStatus(Status, Stack->Parameters.Read.Length, Irp);
    }
    _SEH2_FINALLY
    {
        /* And complete! */
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    } _SEH2_END;

    return Status;
}

NTSTATUS
ViCreateDevice(PDRIVER_OBJECT DriverObject, PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Make sure output buffer is big enough to receive the drive letter
     * when we create the drive
     */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(WCHAR))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(WCHAR), Irp);
    }

    /* And start creation. We always start from bottom */
    Status = ViCreateDriveLetter(DriverObject, L'Z', Irp->AssociatedIrp.SystemBuffer, &DeviceObject);
    return ViSetIoStatus(Status, sizeof(WCHAR), Irp);
}

NTSTATUS
ViGetDriveGeometry(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDISK_GEOMETRY DiskGeo;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    /* No geometry if no image mounted */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject == NULL)
    {
        return ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
    }

    /* No geometry if volume is to be verified */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        return ViVerifyVolume(DeviceObject, Irp);
    }

    /* No geometry if too small output buffer */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(DISK_GEOMETRY), Irp);
    }

    /* Finally, set what we're asked for */
    DiskGeo = Irp->AssociatedIrp.SystemBuffer;
    DiskGeo->MediaType = RemovableMedia;
    DiskGeo->BytesPerSector = DeviceExtension->SectorSize;
    DiskGeo->SectorsPerTrack = DeviceExtension->VolumeSize.QuadPart >> DeviceExtension->SectorShift;
    DiskGeo->Cylinders.QuadPart = 1;
    DiskGeo->TracksPerCylinder = 1;

    return ViSetIoStatus(STATUS_SUCCESS, sizeof(DISK_GEOMETRY), Irp);
}

NTSTATUS
ViCheckVerify(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PULONG Buffer;
    ULONG_PTR Information;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    /* Nothing to verify if no mounted */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject == NULL)
    {
        return ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
    }

    /* Do we have to verify? */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        return ViSetIoStatus(STATUS_VERIFY_REQUIRED, 0, Irp);
    }

    /* If caller provided a buffer, that's to get the change count */
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    if (Buffer != NULL)
    {
        *Buffer = DeviceExtension->ChangeCount;
        Information = sizeof(ULONG);
    }
    else
    {
        Information = 0;
    }

    /* Done */
    return ViSetIoStatus(STATUS_SUCCESS, Information, Irp);
}

NTSTATUS
ViIssueMountImage(PDEVICE_OBJECT DeviceObject, PUNICODE_STRING Image, PIRP Irp)
{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;

    /* We cannot mount an image if there's already one mounted */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject != NULL)
    {
        return ViSetIoStatus(STATUS_DEVICE_NOT_READY, 0, Irp);
    }

    /* Perform the mount */
    Status = ViMountImage(DeviceObject, Image);
    return ViSetIoStatus(Status, 0, Irp);
}

ULONG
ViComputeAddress(ULONG Address)
{
    UCHAR Local[4];

    /* Convert LBA to MSF */
    Local[0] = 0;
    Local[1] = Address / 4500;
    Local[2] = Address % 4500 / 75;
    Local[3] = Address + 108 * Local[1] - 75 * Local[2];

    return *(ULONG *)(&Local[0]);
}

VOID
ViFillInTrackData(PTRACK_DATA TrackData, UCHAR Control, UCHAR Adr, UCHAR TrackNumber, ULONG Address)
{
    /* Fill in our track data with provided information */
    TrackData->Reserved = 0;
    TrackData->Reserved1 = 0;
    TrackData->Control = Control & 0xF;
    TrackData->Adr = Adr;
    TrackData->TrackNumber = TrackNumber;
    *(ULONG *)(&TrackData->Address[0]) = ViComputeAddress(Address);
}

NTSTATUS
ViReadToc(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PCDROM_TOC Toc;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    /* No image mounted, no TOC */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject == NULL)
    {
        return ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
    }

    /* No TOC if we have to verify */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        return ViVerifyVolume(DeviceObject, Irp);
    }

    /* Check we have enough room for TOC */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CDROM_TOC))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(CDROM_TOC), Irp);
    }

    /* Start filling the TOC */
    Toc = Irp->AssociatedIrp.SystemBuffer;
    Toc->Length[0] = 0;
    Toc->Length[1] = 8;
    Toc->FirstTrack = 1;
    Toc->LastTrack = 1;
    /* And fill our single (an ISO file always have a single track) track with 2sec gap */
    ViFillInTrackData(Toc->TrackData, TOC_DATA_TRACK, ADR_NO_MODE_INFORMATION, 1, 150);
    /* And add last track termination */
    ViFillInTrackData(&Toc->TrackData[1], TOC_DATA_TRACK, ADR_NO_MODE_INFORMATION, TOC_LAST_TRACK, (DeviceExtension->VolumeSize.QuadPart >> DeviceExtension->SectorShift) + 150);

    return ViSetIoStatus(STATUS_SUCCESS, sizeof(CDROM_TOC), Irp);
}

NTSTATUS
ViReadTocEx(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PCDROM_TOC Toc;
    PIO_STACK_LOCATION Stack;
    PCDROM_READ_TOC_EX TocEx;
    PDEVICE_EXTENSION DeviceExtension;

    /* No image mounted, no TOC */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject == NULL)
    {
        return ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
    }

    /* No TOC if we have to verify */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        return ViVerifyVolume(DeviceObject, Irp);
    }

    /* We need an input buffer */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CDROM_READ_TOC_EX))
    {
        return ViSetIoStatus(STATUS_INFO_LENGTH_MISMATCH, sizeof(CDROM_READ_TOC_EX), Irp);
    }

    /* Validate output buffer is big enough */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < MAXIMUM_CDROM_SIZE)
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, MAXIMUM_CDROM_SIZE, Irp);
    }

    /* Validate the input buffer - see cdrom_new */
    TocEx = Irp->AssociatedIrp.SystemBuffer;
    if ((TocEx->Reserved1 != 0) || (TocEx->Reserved2 != 0) ||
        (TocEx->Reserved3 != 0))
    {
        return ViSetIoStatus(STATUS_INVALID_PARAMETER, 0, Irp);
    }

    if (((TocEx->Format == CDROM_READ_TOC_EX_FORMAT_SESSION) ||
         (TocEx->Format == CDROM_READ_TOC_EX_FORMAT_PMA) ||
         (TocEx->Format == CDROM_READ_TOC_EX_FORMAT_ATIP)) &&
        TocEx->SessionTrack != 0)
    {
        return ViSetIoStatus(STATUS_INVALID_PARAMETER, 0, Irp);
    }

    if ((TocEx->Format != CDROM_READ_TOC_EX_FORMAT_TOC) &&
        (TocEx->Format != CDROM_READ_TOC_EX_FORMAT_FULL_TOC) &&
        (TocEx->Format != CDROM_READ_TOC_EX_FORMAT_CDTEXT) &&
        (TocEx->Format != CDROM_READ_TOC_EX_FORMAT_SESSION) &&
        (TocEx->Format != CDROM_READ_TOC_EX_FORMAT_PMA) &&
        (TocEx->Format == CDROM_READ_TOC_EX_FORMAT_ATIP))
    {
        return ViSetIoStatus(STATUS_INVALID_PARAMETER, 0, Irp);
    }

    /* Start filling the TOC */
    Toc = Irp->AssociatedIrp.SystemBuffer;
    Toc->Length[0] = 0;
    Toc->Length[1] = 8;
    Toc->FirstTrack = 1;
    Toc->LastTrack = 1;
    /* And fill our single (an ISO file always have a single track) track with 2sec gap */
    ViFillInTrackData(Toc->TrackData, TOC_DATA_TRACK, ADR_NO_MODE_INFORMATION, 1, 150);
    /* And add last track termination */
    ViFillInTrackData(&Toc->TrackData[1], TOC_DATA_TRACK, ADR_NO_MODE_INFORMATION, TOC_LAST_TRACK, (DeviceExtension->VolumeSize.QuadPart >> DeviceExtension->SectorShift) + 150);

    return ViSetIoStatus(STATUS_SUCCESS, MAXIMUM_CDROM_SIZE, Irp);
}

NTSTATUS
ViGetLastSession(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{

    PIO_STACK_LOCATION Stack;
    PCDROM_TOC_SESSION_DATA Toc;
    PDEVICE_EXTENSION DeviceExtension;

    /* No image, no last session */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject == NULL)
    {
        return ViSetIoStatus(STATUS_NO_MEDIA_IN_DEVICE, 0, Irp);
    }

    /* No last session if we have to verify */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (BooleanFlagOn(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        !BooleanFlagOn(Stack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        return ViVerifyVolume(DeviceObject, Irp);
    }

    /* Check we have enough room for last session data */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CDROM_TOC_SESSION_DATA))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(CDROM_TOC_SESSION_DATA), Irp);
    }

    /* Fill in data */
    Toc = Irp->AssociatedIrp.SystemBuffer;
    Toc->Length[0] = 0;
    Toc->Length[1] = 8;
    Toc->FirstCompleteSession = 1;
    Toc->LastCompleteSession = 1;
    /* And return our track with 2sec gap (cf TOC function) */
    ViFillInTrackData(Toc->TrackData, TOC_DATA_TRACK, ADR_NO_MODE_INFORMATION, 1, 150);

    return ViSetIoStatus(STATUS_SUCCESS, sizeof(CDROM_TOC_SESSION_DATA), Irp);
}

NTSTATUS
ViEnumerateDrives(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDRIVES_LIST DrivesList;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT CurrentDO;
    PDEVICE_EXTENSION DeviceExtension;

    /* Check we have enough room for output */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DRIVES_LIST))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(DRIVES_LIST), Irp);
    }

    /* Get the output buffer */
    DrivesList = Irp->AssociatedIrp.SystemBuffer;
    DrivesList->Count = 0;

    /* And now, starting from our main DO, start browsing all the DO we created */
    for (CurrentDO = DeviceObject->DriverObject->DeviceObject; CurrentDO != NULL;
         CurrentDO = CurrentDO->NextDevice)
    {
        /* Check we won't output our main DO */
        DeviceExtension = CurrentDO->DeviceExtension;
        if (DeviceExtension->GlobalName.Length !=
            RtlCompareMemory(DeviceExtension->GlobalName.Buffer,
                             L"\\??\\VirtualCdRom",
                             DeviceExtension->GlobalName.Length))
        {
            /* When we return, we extract the drive letter
             * See ViCreateDriveLetter(), it's \??\Z:
             */
            DrivesList->Drives[DrivesList->Count++] = DeviceExtension->GlobalName.Buffer[4];
        }
    }

    return ViSetIoStatus(STATUS_SUCCESS, sizeof(DRIVES_LIST), Irp);
}

NTSTATUS
ViGetImagePath(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIMAGE_PATH ImagePath;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    /* Check we have enough room for output */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IMAGE_PATH))
    {
        return ViSetIoStatus(STATUS_BUFFER_TOO_SMALL, sizeof(IMAGE_PATH), Irp);
    }

    /* Get our image path from DO */
    DeviceExtension = DeviceObject->DeviceExtension;
    ImagePath = Irp->AssociatedIrp.SystemBuffer;
    ImagePath->Mounted = (DeviceExtension->VolumeObject != NULL);
    ImagePath->Length = DeviceExtension->ImageName.Length;
    /* And if it's set, copy it back to the caller */
    if (DeviceExtension->ImageName.Length != 0)
    {
        RtlCopyMemory(ImagePath->Path, DeviceExtension->ImageName.Buffer, DeviceExtension->ImageName.Length);
    }

    return ViSetIoStatus(STATUS_SUCCESS, sizeof(IMAGE_PATH), Irp);
}

NTSTATUS
ViEjectMedia(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* Eject will force a verify */
    SetFlag(DeviceObject->Flags, DO_VERIFY_VOLUME);

    /* If we have an image mounted, unmount it
     * But don't free anything related, so that
     * we can perform quick remount.
     * See: IOCTL_STORAGE_LOAD_MEDIA
     */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->VolumeObject != NULL)
    {
        ObDereferenceObject(DeviceExtension->VolumeObject);
        /* Device changed, so mandatory increment */
        ++DeviceExtension->ChangeCount;
        DeviceExtension->VolumeObject = NULL;
    }

    return ViSetIoStatus(STATUS_SUCCESS, 0, Irp);
}

NTSTATUS
ViRemountMedia(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status;
    UNICODE_STRING Image;
    PDEVICE_EXTENSION DeviceExtension;

    /* Get the device extension */
    DeviceExtension = DeviceObject->DeviceExtension;
    /* Allocate a new string as mount parameter */
    Status = ViAllocateUnicodeString(DeviceExtension->ImageName.MaximumLength, &Image);
    if (!NT_SUCCESS(Status))
    {
        return ViSetIoStatus(Status, 0, Irp);
    }

    /* To allow cleanup in case of troubles */
    _SEH2_TRY
    {
        /* Copy our current image name and mount */
        RtlCopyUnicodeString(&Image, &DeviceExtension->ImageName);
        Status = ViIssueMountImage(DeviceObject, &Image, Irp);
    }
    _SEH2_FINALLY
    {
        ViFreeUnicodeString(&Image);
    }
    _SEH2_END;

    return ViSetIoStatus(Status, 0, Irp);
}

NTSTATUS
NTAPI
VcdDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status;
    UNICODE_STRING Image;
    PIO_STACK_LOCATION Stack;
    PMOUNT_PARAMETERS MountParameters;
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    _SEH2_TRY
    {
        switch (Stack->Parameters.DeviceIoControl.IoControlCode)
        {
            /* First of all, our private IOCTLs */
            case IOCTL_VCDROM_CREATE_DRIVE:
                Status = ViCreateDevice(DeviceObject->DriverObject, Irp);
                break;

            case IOCTL_VCDROM_DELETE_DRIVE:
                Status = ViDeleteDevice(DeviceObject, Irp);
                break;

            case IOCTL_VCDROM_MOUNT_IMAGE:
                MountParameters = Irp->AssociatedIrp.SystemBuffer;
                Image.MaximumLength = 255 * sizeof(WCHAR);
                Image.Length = MountParameters->Length;
                Image.Buffer = MountParameters->Path;
                DeviceExtension->Flags = MountParameters->Flags;
                Status = ViIssueMountImage(DeviceObject, &Image, Irp);
                break;

            case IOCTL_VCDROM_ENUMERATE_DRIVES:
                Status = ViEnumerateDrives(DeviceObject, Irp);
                break;

            case IOCTL_VCDROM_GET_IMAGE_PATH:
                Status = ViGetImagePath(DeviceObject, Irp);
                break;

            /* Now, IOCTLs we have to handle as class driver */
            case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
                Status = ViGetDriveGeometry(DeviceObject, Irp);
                break;

            case IOCTL_DISK_CHECK_VERIFY:
            case IOCTL_CDROM_CHECK_VERIFY:
                Status = ViCheckVerify(DeviceObject, Irp);
                break;

            case IOCTL_CDROM_READ_TOC:
                Status = ViReadToc(DeviceObject, Irp);
                break;

            case IOCTL_CDROM_READ_TOC_EX:
                Status = ViReadTocEx(DeviceObject, Irp);
                break;

            case IOCTL_CDROM_GET_LAST_SESSION:
                Status = ViGetLastSession(DeviceObject, Irp);
                break;

            case IOCTL_STORAGE_EJECT_MEDIA:
            case IOCTL_CDROM_EJECT_MEDIA:
                Status = ViEjectMedia(DeviceObject, Irp);
                break;

            /* That one is a bit specific
             * It gets unmounted image mounted again
             */
            case IOCTL_STORAGE_LOAD_MEDIA:
                /* That means it can only be performed if:
                 * - We had an image previously
                 * - It's no longer mounted
                 * Otherwise, we just return success
                 */
                if (DeviceExtension->ImageName.Buffer == NULL || DeviceExtension->VolumeObject != NULL)
                {
                    Status = ViSetIoStatus(STATUS_SUCCESS, 0, Irp);
                }
                else
                {
                    Status = ViRemountMedia(DeviceObject, Irp);
                }
                break;

            default:
                Status = STATUS_INVALID_DEVICE_REQUEST;
                DPRINT1("IOCTL: %x not supported\n", Stack->Parameters.DeviceIoControl.IoControlCode);
                break;
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    /* Depending on the failure code, we may force a verify */
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_DEVICE_NOT_READY || Status == STATUS_IO_TIMEOUT ||
            Status == STATUS_MEDIA_WRITE_PROTECTED || Status == STATUS_NO_MEDIA_IN_DEVICE ||
            Status == STATUS_VERIFY_REQUIRED || Status == STATUS_UNRECOGNIZED_MEDIA ||
            Status == STATUS_WRONG_VOLUME)
        {
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }
    }

    IoCompleteRequest(Irp, IO_DISK_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;

    /* Set our entry points (rather limited :-)) */
    DriverObject->DriverUnload = VcdUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = VcdHandle;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = VcdHandle;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VcdHandle;
    DriverObject->MajorFunction[IRP_MJ_READ] = VcdRead;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VcdDeviceControl;

    /* Create our main device to receive private IOCTLs */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\VirtualCdRom");
    Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &DeviceName,
                            FILE_DEVICE_CD_ROM, FILE_READ_ONLY_DEVICE | FILE_FLOPPY_DISKETTE,
                            FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Initialize our device extension */
    ViInitializeDeviceExtension(DeviceObject);
    DeviceExtension = DeviceObject->DeviceExtension;

    /* And create our accessible name from umode */
    ViAllocateUnicodeString(DEFAULT_STRING_SIZE, &DeviceExtension->GlobalName);
    RtlAppendUnicodeToString(&DeviceExtension->GlobalName, L"\\??\\VirtualCdRom");
    Status = IoCreateSymbolicLink(&DeviceExtension->GlobalName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    /* Initialize our mutex for device count */
    ExInitializeFastMutex(&ViMutex);

    /* And try to load images that would have been stored in registry */
    ViLoadImagesFromRegistry(DriverObject, RegistryPath->Buffer);

    return STATUS_SUCCESS;
}
