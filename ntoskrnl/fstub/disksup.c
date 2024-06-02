/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/disksup.c
* PURPOSE:         I/O HAL Routines for Disk Access
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Eric Kohl
*                  Casper S. Hornstrup (chorns@users.sourceforge.net)
*                  Pierre Schweitzer
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/hal.h>

#define NDEBUG
#include <debug.h>

/* See fstubex.c */
#define EFI_PMBR_OSTYPE_EFI 0xEE

typedef enum _DISK_MANAGER
{
    NoDiskManager,
    OntrackDiskManager,
    EZ_Drive
} DISK_MANAGER;

typedef enum _PARTITION_TYPE
{
    BootablePartition,
    PrimaryPartition,
    LogicalPartition,
    FtPartition,
    UnknownPartition,
    DataPartition
} PARTITION_TYPE, *PPARTITION_TYPE;

/* PRIVATE FUNCTIONS *********************************************************/

static NTSTATUS
HalpQueryDriveLayout(
    _In_ PUNICODE_STRING DeviceName,
    _Outptr_ PDRIVE_LAYOUT_INFORMATION* LayoutInfo)
{
    IO_STATUS_BLOCK StatusBlock;
    PDEVICE_OBJECT DeviceObject = NULL;
    PFILE_OBJECT FileObject;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    ULONG BufferSize;
    PDRIVE_LAYOUT_INFORMATION Buffer;

    PAGED_CODE();

    /* Get device pointers */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get attached device object */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    ObDereferenceObject(FileObject);

    /* Do not handle removable media */
    if (BooleanFlagOn(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA))
    {
        ObDereferenceObject(DeviceObject);
        return STATUS_NO_MEDIA;
    }

    /* We'll loop until our buffer is big enough */
    Buffer = NULL;
    BufferSize = 0x1000;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    do
    {
        /* If we already had a buffer, it means it's not
         * big enough, so free and multiply size by two */
        if (Buffer != NULL)
        {
            ExFreePoolWithTag(Buffer, TAG_FSTUB);
            BufferSize *= 2;
        }

        /* Allocate buffer for output buffer */
        Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG_FSTUB);
        if (Buffer == NULL)
        {
            Status = STATUS_NO_MEMORY;
            break;
        }

        /* Build the IRP to query drive layout */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_LAYOUT,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            Buffer,
                                            BufferSize,
                                            FALSE,
                                            &Event,
                                            &StatusBlock);
        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Call the driver and wait if appropriate */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = StatusBlock.Status;
        }
        /* If buffer is too small, keep looping */
    } while (Status == STATUS_BUFFER_TOO_SMALL);

    /* We're done with the device */
    ObDereferenceObject(DeviceObject);

    /* If querying worked, return the buffer to the caller */
    if (NT_SUCCESS(Status))
    {
        ASSERT(Buffer != NULL);
        *LayoutInfo = Buffer;
    }
    /* Else, release the buffer if still allocated and fail */
    else
    {
        if (Buffer != NULL)
        {
            ExFreePoolWithTag(Buffer, TAG_FSTUB);
        }
    }

    return Status;
}

static NTSTATUS
HalpQueryPartitionType(
    _In_ PUNICODE_STRING DeviceName,
    _In_opt_ PDRIVE_LAYOUT_INFORMATION LayoutInfo,
    _Out_ PPARTITION_TYPE PartitionType)
{
    USHORT i;
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION_EX PartitionInfo;

    PAGED_CODE();

    /* Get device pointers */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get attached device object */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    ObDereferenceObject(FileObject);

    /* Assume logical partition for removable devices */
    if (BooleanFlagOn(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA))
    {
        ObDereferenceObject(DeviceObject);
        *PartitionType = LogicalPartition;
        return STATUS_SUCCESS;
    }

    /* For the others, query partition info */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* We're done with the device */
    ObDereferenceObject(DeviceObject);

    /* If we failed querying partition info, try to return something
     * if caller didn't provide a precise layout, assume logical
     * partition and fake success. Otherwise, just fail. */
    if (!NT_SUCCESS(Status))
    {
        if (LayoutInfo == NULL)
        {
            *PartitionType = LogicalPartition;
            return STATUS_SUCCESS;
        }

        return Status;
    }

    /* First, handle non-MBR style (easy case) */
    if (PartitionInfo.PartitionStyle != PARTITION_STYLE_MBR)
    {
        /* If not GPT, we don't know what it is */
        if (PartitionInfo.PartitionStyle != PARTITION_STYLE_GPT)
        {
            *PartitionType = UnknownPartition;
            return STATUS_SUCCESS;
        }

        /* Check whether that's data partition */
        if (RtlCompareMemory(&PartitionInfo.Gpt.PartitionType,
                             &PARTITION_BASIC_DATA_GUID,
                             sizeof(GUID)) == sizeof(GUID))
        {
            *PartitionType = DataPartition;
            return STATUS_SUCCESS;
        }

        /* Otherwise, we don't know */
        *PartitionType = UnknownPartition;
        return STATUS_SUCCESS;
    }

    /* If we don't recognize partition type, return unknown */
    if (!IsRecognizedPartition(PartitionInfo.Mbr.PartitionType))
    {
        *PartitionType = UnknownPartition;
        return STATUS_SUCCESS;
    }

    /* Check if that's a FT volume */
    if (IsFTPartition(PartitionInfo.Mbr.PartitionType))
    {
        *PartitionType = FtPartition;
        return STATUS_SUCCESS;
    }

    /* If the caller didn't provide the complete layout, just return */
    if (LayoutInfo == NULL)
    {
        *PartitionType = LogicalPartition;
        return STATUS_SUCCESS;
    }

    /* Now, evaluate the partition to the 4 in the input layout */
    for (i = 0; i < 4; ++i)
    {
        /* If we find a partition matching */
        if (LayoutInfo->PartitionEntry[i].StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
        {
            /* Return boot if boot flag is set */
            if (PartitionInfo.Mbr.BootIndicator)
            {
                *PartitionType = BootablePartition;
            }
            /* Primary otherwise */
            else
            {
                *PartitionType = PrimaryPartition;
            }

            return STATUS_SUCCESS;
        }
    }

    /* Otherwise, assume logical */
    *PartitionType = LogicalPartition;
    return STATUS_SUCCESS;
}

static PULONG
IopComputeHarddiskDerangements(
    _In_ ULONG DiskCount)
{
    PIRP Irp;
    KEVENT Event;
    ULONG i, j, k;
    PULONG Devices;
    NTSTATUS Status;
    WCHAR Buffer[100];
    UNICODE_STRING ArcName;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    STORAGE_DEVICE_NUMBER DeviceNumber;

    /* No disks, nothing to do */
    if (DiskCount == 0)
    {
        return NULL;
    }

    /* Allocate a buffer big enough to hold all the disks */
    Devices = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                    sizeof(ULONG) * DiskCount,
                                    TAG_FSTUB);
    if (Devices == NULL)
    {
        return NULL;
    }

    /* Now, we'll query all the disks */
    for (i = 0; i < DiskCount; ++i)
    {
        /* Using their ARC name */
        swprintf(Buffer, L"\\ArcName\\multi(0)disk(0)rdisk(%d)", i);
        RtlInitUnicodeString(&ArcName, Buffer);

        /* Get the attached DeviceObject */
        Status = IoGetDeviceObjectPointer(&ArcName,
                                          FILE_READ_ATTRIBUTES,
                                          &FileObject,
                                          &DeviceObject);
        if (NT_SUCCESS(Status))
        {
            DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
            ObDereferenceObject(FileObject);

            /* And query it for device number */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &DeviceNumber,
                                                sizeof(DeviceNumber),
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            if (Irp != NULL)
            {
                Status = IoCallDriver(DeviceObject, Irp);
                if (Status == STATUS_PENDING)
                {
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    Status = IoStatusBlock.Status;
                }

                ObDereferenceObject(DeviceObject);

                /* In case of a success remember device number */
                if (NT_SUCCESS(Status))
                {
                    Devices[i] = DeviceNumber.DeviceNumber;
                    /* Move on, not to fall into our default case */
                    continue;
                }
            }
            else
            {
                ObDereferenceObject(DeviceObject);
            }

            /* Default case, for failures, set -1 */
            Devices[i] = -1;
        }
    }

    /* Now, we'll check all device numbers */
    for (i = 0; i < DiskCount; ++i)
    {
        /* First of all, check if we're at the right place */
        for (j = 0; j < DiskCount; ++j)
        {
            if (Devices[j] == i)
            {
                break;
            }
        }

        /* If not, perform the change */
        if (j >= DiskCount)
        {
            k = 0;
            while (Devices[k] != -1)
            {
                if (++k >= DiskCount)
                {
                    break;
                }
            }

            if (k < DiskCount)
            {
                Devices[k] = i;
            }
        }
    }

    /* Return our device derangement map */
    return Devices;
}

static NTSTATUS
HalpNextMountLetter(
    _In_ PUNICODE_STRING DeviceName,
    _Out_ PUCHAR DriveLetter)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    UNICODE_STRING MountMgr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PMOUNTMGR_DRIVE_LETTER_TARGET Target;
    MOUNTMGR_DRIVE_LETTER_INFORMATION LetterInfo;

    /* To get next mount letter, we need the MountMgr */
    RtlInitUnicodeString(&MountMgr, L"\\Device\\MountPointManager");
    Status = IoGetDeviceObjectPointer(&MountMgr,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Allocate our input buffer */
    Target = ExAllocatePoolWithTag(PagedPool,
                                   DeviceName->Length + FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName),
                                   TAG_FSTUB);
    if (Target == NULL)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* And fill it with the device hat needs a drive letter */
    Target->DeviceNameLength = DeviceName->Length;
    RtlCopyMemory(&Target->DeviceName[0], DeviceName->Buffer, DeviceName->Length);

    /* Call the MountMgr */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                        DeviceObject,
                                        Target,
                                        DeviceName->Length + FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName),
                                        &LetterInfo,
                                        sizeof(LetterInfo),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        ExFreePoolWithTag(Target, TAG_FSTUB);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    ExFreePoolWithTag(Target, TAG_FSTUB);
    ObDereferenceObject(FileObject);

    DPRINT("Done: %d %c\n", LetterInfo.DriveLetterWasAssigned,
                            LetterInfo.CurrentDriveLetter);

    /* Return the drive letter the MountMgr potentially assigned */
    *DriveLetter = LetterInfo.CurrentDriveLetter;

    /* Also return the success */
    return Status;
}

static NTSTATUS
HalpSetMountLetter(
    _In_ PUNICODE_STRING DeviceName,
    _In_ UCHAR DriveLetter)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    WCHAR Buffer[30];
    ULONG InputBufferLength;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DosDevice, MountMgr;
    PMOUNTMGR_CREATE_POINT_INPUT InputBuffer;

    /* Setup the DosDevice name */
    swprintf(Buffer, L"\\DosDevices\\%c:", DriveLetter);
    RtlInitUnicodeString(&DosDevice, Buffer);

    /* Allocate the input buffer for the MountMgr */
    InputBufferLength = DosDevice.Length + DeviceName->Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer = ExAllocatePoolWithTag(PagedPool, InputBufferLength, TAG_FSTUB);
    if (InputBuffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill the input buffer */
    InputBuffer->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer->SymbolicLinkNameLength = DosDevice.Length;
    InputBuffer->DeviceNameOffset = DosDevice.Length + sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer->DeviceNameLength = DeviceName->Length;
    RtlCopyMemory(&InputBuffer[1], DosDevice.Buffer, DosDevice.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)InputBuffer + InputBuffer->DeviceNameOffset),
                  DeviceName->Buffer,
                  DeviceName->Length);

    /* Get the MountMgr device pointer, to send the IOCTL */
    RtlInitUnicodeString(&MountMgr, L"\\Device\\MountPointManager");
    Status = IoGetDeviceObjectPointer(&MountMgr,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(InputBuffer, TAG_FSTUB);
        return Status;
    }

    /* Call the MountMgr */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_CREATE_POINT,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        ObDereferenceObject(FileObject);
        ExFreePoolWithTag(InputBuffer, TAG_FSTUB);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(FileObject);
    ExFreePoolWithTag(InputBuffer, TAG_FSTUB);

    /* Return the MountMgr status */
    return Status;
}

static UCHAR
HalpNextDriveLetter(
    _In_ PUNICODE_STRING DeviceName,
    _In_ PSTRING NtDeviceName,
    _Out_ PUCHAR NtSystemPath,
    _In_ BOOLEAN IsRemovable)
{
    UCHAR i;
    WCHAR Buffer[40];
    UCHAR DriveLetter;
    UNICODE_STRING FloppyString, CdString, NtDeviceNameU, DosDevice;

    /* Quick path, ask directly the MountMgr
     * to assign the next free drive letter */
    if (NT_SUCCESS(HalpNextMountLetter(DeviceName, &DriveLetter)))
    {
        return DriveLetter;
    }

    /* We'll allow the MountMgr to fail only for non vital path */
    if (NtDeviceName == NULL || NtSystemPath == NULL)
    {
        return -1;
    }

    /* And for removable devices */
    if (!IsRemovable)
    {
        return 0;
    }

    /* Removable might be floppy or cdrom */
    RtlInitUnicodeString(&FloppyString, L"\\Device\\Floppy");
    RtlInitUnicodeString(&CdString, L"\\Device\\CdRom");

    /* If floppy, start at A */
    if (RtlPrefixUnicodeString(&FloppyString, DeviceName, TRUE))
    {
        DriveLetter = 'A';
    }
    /* If CD start at D */
    else if (RtlPrefixUnicodeString(&CdString, DeviceName, TRUE))
    {
        DriveLetter = 'D';
    }
    /* For the rest start at C */
    else
    {
        DriveLetter = 'C';
    }

    /* Now, try to assign a drive letter manually with the MountMgr */
    for (i = DriveLetter; i <= 'Z'; ++i)
    {
        if (NT_SUCCESS(HalpSetMountLetter(DeviceName, i)))
        {
            /* If it worked, if we were managing system path, update manually */
            if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&NtDeviceNameU, NtDeviceName, TRUE)))
            {
                if (RtlEqualUnicodeString(&NtDeviceNameU, DeviceName, TRUE))
                {
                    *NtSystemPath = i;
                }

                RtlFreeUnicodeString(&NtDeviceNameU);
            }

            return i;
        }
    }

    /* Last fall back, we're not on a PnP device... */
    for (i = DriveLetter; i <= 'Z'; ++i)
    {
        /* We'll link manually, without the MountMgr knowing anything about the device */
        swprintf(Buffer, L"\\DosDevices\\%c:", i);
        RtlInitUnicodeString(&DosDevice, Buffer);

        /* If linking worked, the letter was free ;-) */
        if (NT_SUCCESS(IoCreateSymbolicLink(&DosDevice, DeviceName)))
        {
            /* If it worked, if we were managing system path, update manually */
            if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&NtDeviceNameU, NtDeviceName, TRUE)))
            {
                if (RtlEqualUnicodeString(&NtDeviceNameU, DeviceName, TRUE))
                {
                    *NtSystemPath = i;
                }

                RtlFreeUnicodeString(&NtDeviceNameU);
            }

            return i;
        }
    }

    /* We're done, nothing happened */
    return 0;
}

static BOOLEAN
HalpIsOldStyleFloppy(
    _In_ PUNICODE_STRING DeviceName)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    MOUNTDEV_NAME DevName;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();

    /* Get the attached device object to our device */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
        return FALSE;

    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    ObDereferenceObject(FileObject);

    /* Query its device name (i.e. check floppy.sys implements MountMgr interface) */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &DevName,
                                        sizeof(DevName),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        ObDereferenceObject(DeviceObject);
        return FALSE;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* If status is not STATUS_BUFFER_OVERFLOW, it means
     * it's a pre-MountMgr driver, aka "Old style" */
    ObDereferenceObject(DeviceObject);
    return (Status != STATUS_BUFFER_OVERFLOW);
}

static NTSTATUS
HalpDeleteMountLetter(
    _In_ UCHAR DriveLetter)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    WCHAR Buffer[30];
    ULONG InputBufferLength;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PMOUNTMGR_MOUNT_POINT InputBuffer;
    UNICODE_STRING DosDevice, MountMgr;
    PMOUNTMGR_MOUNT_POINTS OutputBuffer;

    /* Setup the device name of the letter to delete */
    swprintf(Buffer, L"\\DosDevices\\%c:", DriveLetter);
    RtlInitUnicodeString(&DosDevice, Buffer);

    /* Allocate the input buffer for the MountMgr */
    InputBufferLength = DosDevice.Length + sizeof(MOUNTMGR_MOUNT_POINT);
    InputBuffer = ExAllocatePoolWithTag(PagedPool, InputBufferLength, TAG_FSTUB);
    if (InputBuffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill it in */
    RtlZeroMemory(InputBuffer, InputBufferLength);
    InputBuffer->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    InputBuffer->SymbolicLinkNameLength = DosDevice.Length;
    RtlCopyMemory(&InputBuffer[1], DosDevice.Buffer, DosDevice.Length);

    /* Allocate big enough output buffer (we don't care about the output) */
    OutputBuffer = ExAllocatePoolWithTag(PagedPool, 0x1000, TAG_FSTUB);
    if (OutputBuffer == NULL)
    {
        ExFreePoolWithTag(InputBuffer, TAG_FSTUB);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the device pointer to the MountMgr */
    RtlInitUnicodeString(&MountMgr, L"\\Device\\MountPointManager");
    Status = IoGetDeviceObjectPointer(&MountMgr,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(OutputBuffer, TAG_FSTUB);
        ExFreePoolWithTag(InputBuffer, TAG_FSTUB);
        return Status;
    }

    /* Call the MountMgr to delete the drive letter */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_DELETE_POINTS,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        0x1000,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        ObDereferenceObject(FileObject);
        ExFreePoolWithTag(OutputBuffer, TAG_FSTUB);
        ExFreePoolWithTag(InputBuffer, TAG_FSTUB);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(FileObject);
    ExFreePoolWithTag(OutputBuffer, TAG_FSTUB);
    ExFreePoolWithTag(InputBuffer, TAG_FSTUB);

    return Status;
}

static VOID
HalpEnableAutomaticDriveLetterAssignment(VOID)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    UNICODE_STRING MountMgr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get the device pointer to the MountMgr */
    RtlInitUnicodeString(&MountMgr, L"\\Device\\MountPointManager");
    Status = IoGetDeviceObjectPointer(&MountMgr,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Just send an IOCTL to enable the feature */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp == NULL)
    {
        return;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(FileObject);

    return;
}

VOID
FASTCALL
xHalIoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PSTRING NtDeviceName,
                         OUT PUCHAR NtSystemPath,
                         OUT PSTRING NtSystemPathString)
{
    USHORT i;
    PULONG Devices;
    NTSTATUS Status;
    WCHAR Buffer[50];
    HANDLE FileHandle;
    UCHAR DriveLetter;
    BOOLEAN SystemFound;
    IO_STATUS_BLOCK StatusBlock;
    PARTITION_TYPE PartitionType;
    ANSI_STRING StringA1, StringA2;
    PSTR Buffer1, Buffer2, LoadOptions;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDRIVE_LAYOUT_INFORMATION LayoutInfo;
    PCONFIGURATION_INFORMATION ConfigInfo;
    UNICODE_STRING StringU1, StringU2, StringU3;
    ULONG Increment, DiskCount, RealDiskCount, HarddiskCount, PartitionCount, SystemPartition;

    PAGED_CODE();

    /* Get our disk count */
    ConfigInfo = IoGetConfigurationInformation();
    DiskCount = ConfigInfo->DiskCount;
    RealDiskCount = 0;

    /* Allocate two generic string buffers we'll use and reuser later on */
    Buffer1 = ExAllocatePoolWithTag(NonPagedPool, 128, TAG_FSTUB);
    Buffer2 = ExAllocatePoolWithTag(NonPagedPool, 64, TAG_FSTUB);
    if (Buffer1 == NULL || Buffer2 == NULL)
    {
        KeBugCheck(ASSIGN_DRIVE_LETTERS_FAILED);
    }

    /* In case of a remote boot, setup system path */
    if (IoRemoteBootClient)
    {
        PSTR Last, Saved;

        /* Find last \ */
        Last = strrchr(LoaderBlock->NtBootPathName, '\\');
        Saved = NULL;
        /* Misformed name, fail */
        if (Last == NULL)
        {
            KeBugCheck(ASSIGN_DRIVE_LETTERS_FAILED);
        }

        /* In case the name was terminated by a \... */
        if (Last[1] == ANSI_NULL)
        {
            /* Erase it, save position and find the previous \ */
            *Last = ANSI_NULL;
            Saved = Last;
            Last = strrchr(LoaderBlock->NtBootPathName, '\\');
            *Saved = '\\';
        }

        /* Misformed name, fail */
        if (Last == NULL)
        {
            KeBugCheck(ASSIGN_DRIVE_LETTERS_FAILED);
        }

        /* For a remote boot, assign X drive letter */
        NtSystemPath[0] = 'X';
        NtSystemPath[1] = ':';
        /* And copy the end of the boot path */
        strcpy((PSTR)&NtSystemPath[2], Last);

        /* If we had to remove the trailing \, remove it here too */
        if (Saved != NULL)
        {
            NtSystemPath[strlen((PSTR)NtSystemPath) - 1] = ANSI_NULL;
        }

        /* Setup output string */
        RtlInitString(NtSystemPathString, (PSTR)NtSystemPath);
    }

    /* For each of our disks, create the physical device DOS device */
    Increment = 0;
    if (DiskCount != 0)
    {
        for (i = 0; i < DiskCount; ++i)
        {
            /* Setup the origin name */
            sprintf(Buffer1, "\\Device\\Harddisk%d\\Partition%d", i, 0);
            RtlInitAnsiString(&StringA1, Buffer1);
            if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&StringU1, &StringA1, TRUE)))
            {
                /* We cannot fail */
                KeBugCheck(ASSIGN_DRIVE_LETTERS_FAILED);
            }

            /* Open the device */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &StringU1,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            Status = ZwOpenFile(&FileHandle,
                                SYNCHRONIZE | FILE_READ_DATA,
                                &ObjectAttributes,
                                &StatusBlock,
                                FILE_SHARE_READ,
                                FILE_SYNCHRONOUS_IO_NONALERT);
            if (NT_SUCCESS(Status))
            {
                /* If we managed, create the link */
                sprintf(Buffer2, "\\DosDevices\\PhysicalDrive%d", i);
                RtlInitAnsiString(&StringA2, Buffer2);
                Status = RtlAnsiStringToUnicodeString(&StringU2, &StringA2, TRUE);
                if (NT_SUCCESS(Status))
                {
                    IoCreateSymbolicLink(&StringU2, &StringU1);
                    RtlFreeUnicodeString(&StringU2);
                }

                ZwClose(FileHandle);

                RealDiskCount = i + 1;
            }

            RtlFreeUnicodeString(&StringU1);

            if (!NT_SUCCESS(Status))
            {
                if (Increment < 50)
                {
                    ++Increment;
                    ++DiskCount;
                }
            }
        }
    }

    /* We are done with our buffers */
    ExFreePoolWithTag(Buffer1, TAG_FSTUB);
    ExFreePoolWithTag(Buffer2, TAG_FSTUB);

    /* Upcase our load options, if any */
    if (LoaderBlock->LoadOptions != NULL)
    {
        LoadOptions = _strupr(LoaderBlock->LoadOptions);
    }
    else
    {
        LoadOptions = NULL;
    }

    /* If we boot with /MININT (system hive as volatile) option, assign X letter to boot device */
    if (LoadOptions != NULL &&
        strstr(LoadOptions, "MININT") != 0 &&
        NT_SUCCESS(RtlAnsiStringToUnicodeString(&StringU1, NtDeviceName, TRUE)))
    {
        if (NT_SUCCESS(HalpSetMountLetter(&StringU1, 'X')))
        {
            *NtSystemPath = 'X';
        }

        RtlFreeUnicodeString(&StringU1);
    }

    /* Compute our disks derangements */
    DiskCount -= Increment;
    if (RealDiskCount > DiskCount)
    {
        DiskCount = RealDiskCount;
    }
    Devices = IopComputeHarddiskDerangements(DiskCount);

    /* Now, start browsing all our disks for assigning drive letters
     * Here, we'll only handle boot partition and primary partitions
     */
    HarddiskCount = 0;
    for (i = 0; i < DiskCount; ++i)
    {
        /* Get device ID according to derangements map */
        if (Devices != NULL)
        {
            HarddiskCount = Devices[i];
        }

        /* Query disk layout */
        swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition0", HarddiskCount);
        RtlInitUnicodeString(&StringU1, Buffer);
        if (!NT_SUCCESS(HalpQueryDriveLayout(&StringU1, &LayoutInfo)))
        {
            LayoutInfo = NULL;
        }

        /* Assume we didn't find system */
        SystemFound = FALSE;
        swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, 1);
        RtlInitUnicodeString(&StringU1, Buffer);
        /* Query partition info for our disk */
        if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
        {
            /* It failed, retry for all the partitions */
            for (PartitionCount = 1; ; ++PartitionCount)
            {
                swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
                RtlInitUnicodeString(&StringU1, Buffer);
                if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
                {
                    break;
                }

                /* We found a primary partition, assign a drive letter */
                if (PartitionType == PrimaryPartition)
                {
                    HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, 0);
                    break;
                }
            }
        }
        else
        {
            /* All right */
            for (PartitionCount = 2; ; ++PartitionCount)
            {
                /* If our partition is bootable (MBR) or data (GPT), that's system partition */
                if (PartitionType == BootablePartition || PartitionType == DataPartition)
                {
                    SystemFound = TRUE;

                    /* Assign a drive letter and stop here if MBR */
                    HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, 0);
                    if (PartitionType == BootablePartition)
                    {
                        break;
                    }
                }

                /* Keep looping on all the partitions */
                swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
                RtlInitUnicodeString(&StringU1, Buffer);
                if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
                {
                    /* Mount every primary partition if we didn't find system */
                    if (!SystemFound)
                    {
                        for (PartitionCount = 1; ; ++PartitionCount)
                        {
                            swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
                            RtlInitUnicodeString(&StringU1, Buffer);
                            if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
                            {
                                break;
                            }

                            if (PartitionType == PrimaryPartition)
                            {
                                HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, 0);
                                break;
                            }
                        }
                    }

                    break;
                }
            }
        }

        /* Free layout, we'll reallocate it for next device */
        if (LayoutInfo != NULL)
        {
            ExFreePoolWithTag(LayoutInfo, TAG_FSTUB);
        }

        HarddiskCount = i + 1;
    }

    /* Now, assign logical partitions */
    for (i = 0; i < DiskCount; ++i)
    {
        /* Get device ID according to derangements map */
        if (Devices != NULL)
        {
            HarddiskCount = Devices[i];
        }
        else
        {
            HarddiskCount = i;
        }

        /* Query device layout */
        swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition0", HarddiskCount);
        RtlInitUnicodeString(&StringU1, Buffer);
        if (!NT_SUCCESS(HalpQueryDriveLayout(&StringU1, &LayoutInfo)))
        {
            LayoutInfo = NULL;
        }

        /* And assign drive letter to logical partitions */
        for (PartitionCount = 1; ; ++PartitionCount)
        {
            swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
            RtlInitUnicodeString(&StringU1, Buffer);
            if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
            {
                break;
            }

            if (PartitionType == LogicalPartition)
            {
                HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, 0);
            }
        }

        /* Free layout, we'll reallocate it for next device */
        if (LayoutInfo != NULL)
        {
            ExFreePoolWithTag(LayoutInfo, 0);
        }
    }

    /* Now, assign drive letters to everything else */
    for (i = 0; i < DiskCount; ++i)
    {
        /* Get device ID according to derangements map */
        if (Devices != NULL)
        {
            HarddiskCount = Devices[i];
        }
        else
        {
            HarddiskCount = i;
        }

        /* Query device layout */
        swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition0", HarddiskCount);
        RtlInitUnicodeString(&StringU1, Buffer);
        if (!NT_SUCCESS(HalpQueryDriveLayout(&StringU1, &LayoutInfo)))
        {
            LayoutInfo = NULL;
        }

        /* Save system partition if any */
        SystemPartition = 0;
        for (PartitionCount = 1; ; ++PartitionCount)
        {
            swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
            RtlInitUnicodeString(&StringU1, Buffer);
            if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
            {
                break;
            }

            if ((PartitionType == BootablePartition || PartitionType == PrimaryPartition) && (SystemPartition == 0))
            {
                SystemPartition = PartitionCount;
            }
        }

        /* And assign drive letter to anything but system partition */
        for (PartitionCount = 1; ; ++PartitionCount)
        {
            if (PartitionCount != SystemPartition)
            {
                swprintf(Buffer, L"\\Device\\Harddisk%d\\Partition%d", HarddiskCount, PartitionCount);
                RtlInitUnicodeString(&StringU1, Buffer);
                if (!NT_SUCCESS(HalpQueryPartitionType(&StringU1, LayoutInfo, &PartitionType)))
                {
                    if (LayoutInfo != NULL)
                    {
                        ExFreePoolWithTag(LayoutInfo, 0);
                    }

                    break;
                }

                if (PartitionType == PrimaryPartition || PartitionType == FtPartition)
                {
                    HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, 0);
                }
            }
        }
    }

    /* We're done with disks, if we have a device map, free it */
    if (Devices != NULL)
    {
        ExFreePoolWithTag(Devices, TAG_FSTUB);
    }

    /* Now, assign drive letter to floppy drives */
    for (i = 0; i < ConfigInfo->FloppyCount; ++i)
    {
        swprintf(Buffer, L"\\Device\\Floppy%d", i);
        RtlInitUnicodeString(&StringU1, Buffer);
        if (HalpIsOldStyleFloppy(&StringU1))
        {
            HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, TRUE);
        }
    }

    /* And CD drives */
    for (i = 0; i < ConfigInfo->CdRomCount; ++i)
    {
        swprintf(Buffer, L"\\Device\\CdRom%d", i);
        RtlInitUnicodeString(&StringU1, Buffer);
        HalpNextDriveLetter(&StringU1, NtDeviceName, NtSystemPath, TRUE);
    }

    /* If not remote boot, handle NtDeviceName */
    if (!IoRemoteBootClient && NT_SUCCESS(RtlAnsiStringToUnicodeString(&StringU1, NtDeviceName, TRUE)))
    {
        /* Assign it a drive letter */
        DriveLetter = HalpNextDriveLetter(&StringU1, NULL, NULL, TRUE);
        if (DriveLetter != 0)
        {
            if (DriveLetter != 0xFF)
            {
                *NtSystemPath = DriveLetter;
            }
        }
        /* If it fails through the MountMgr, retry manually */
        else
        {
            RtlInitUnicodeString(&StringU2, L"\\Device\\Floppy");
            RtlInitUnicodeString(&StringU3, L"\\Device\\CdRom");

            if (RtlPrefixUnicodeString(&StringU2, &StringU1, TRUE))
            {
                DriveLetter = 'A';
            }
            else if (RtlPrefixUnicodeString(&StringU3, &StringU1, TRUE))
            {
                DriveLetter = 'D';
            }
            else
            {
                DriveLetter = 'C';
            }

            /* Try any drive letter */
            while (HalpSetMountLetter(&StringU1, DriveLetter) != STATUS_SUCCESS)
            {
                ++DriveLetter;

                if (DriveLetter > 'Z')
                {
                    break;
                }
            }

            /* If we're beyond Z (ie, no slot left) */
            if (DriveLetter > 'Z')
            {
                /* Delete Z, and reuse it for system */
                HalpDeleteMountLetter('Z');
                HalpSetMountLetter(&StringU1, 'Z');
                *NtSystemPath = 'Z';
            }
            else
            {
                /* Return matching drive letter */
                *NtSystemPath = DriveLetter;
            }
        }

        RtlFreeUnicodeString(&StringU1);
    }

    /* Enable auto assignment for MountMgr */
    HalpEnableAutomaticDriveLetterAssignment();
}

static NTSTATUS
HalpGetFullGeometry(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PDISK_GEOMETRY_EX Geometry)
{
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PKEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    /* Allocate a non-paged event */
    Event = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(KEVENT),
                                  TAG_FILE_SYSTEM);
    if (!Event) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize it */
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                        DeviceObject,
                                        NULL,
                                        0UL,
                                        Geometry,
                                        sizeof(DISK_GEOMETRY_EX),
                                        FALSE,
                                        Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        /* Fail, free the event */
        ExFreePoolWithTag(Event, TAG_FILE_SYSTEM);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver and check if it's pending */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait on the driver */
        KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Free the event and return the Status */
    ExFreePoolWithTag(Event, TAG_FILE_SYSTEM);
    return Status;
}

static BOOLEAN
HalpIsValidPartitionEntry(
    _In_ PPARTITION_DESCRIPTOR Entry,
    _In_ ULONGLONG MaxOffset,
    _In_ ULONGLONG MaxSector)
{
    ULONGLONG EndingSector;

    PAGED_CODE();

    /* Unused partitions are considered valid */
    if (Entry->PartitionType == PARTITION_ENTRY_UNUSED)
        return TRUE;

    /* Get the last sector of the partition */
    EndingSector = GET_STARTING_SECTOR(Entry) + GET_PARTITION_LENGTH(Entry);

    /* Check if it's more than the maximum sector */
    if (EndingSector > MaxSector)
    {
        /* Invalid partition */
        DPRINT1("FSTUB: entry is invalid\n");
        DPRINT1("FSTUB: offset %#08lx\n", GET_STARTING_SECTOR(Entry));
        DPRINT1("FSTUB: length %#08lx\n", GET_PARTITION_LENGTH(Entry));
        DPRINT1("FSTUB: end %#I64x\n", EndingSector);
        DPRINT1("FSTUB: max %#I64x\n", MaxSector);
        return FALSE;
    }
    else if (GET_STARTING_SECTOR(Entry) > MaxOffset)
    {
        /* Invalid partition */
        DPRINT1("FSTUB: entry is invalid\n");
        DPRINT1("FSTUB: offset %#08lx\n", GET_STARTING_SECTOR(Entry));
        DPRINT1("FSTUB: length %#08lx\n", GET_PARTITION_LENGTH(Entry));
        DPRINT1("FSTUB: end %#I64x\n", EndingSector);
        DPRINT1("FSTUB: maxOffset %#I64x\n", MaxOffset);
        return FALSE;
    }

    /* It's fine, return success */
    return TRUE;
}

static VOID
HalpCalculateChsValues(
    _In_ PLARGE_INTEGER PartitionOffset,
    _In_ PLARGE_INTEGER PartitionLength,
    _In_ CCHAR ShiftCount,
    _In_ ULONG SectorsPerTrack,
    _In_ ULONG NumberOfTracks,
    _In_ ULONG ConventionalCylinders,
    _Out_ PPARTITION_DESCRIPTOR PartitionDescriptor)
{
    LARGE_INTEGER FirstSector, SectorCount;
    ULONG LastSector, Remainder, SectorsPerCylinder;
    ULONG StartingCylinder, EndingCylinder;
    ULONG StartingTrack, EndingTrack;
    ULONG StartingSector, EndingSector;

    PAGED_CODE();

    /* Calculate the number of sectors for each cylinder */
    SectorsPerCylinder = SectorsPerTrack * NumberOfTracks;

    /* Calculate the first sector, and the sector count */
    FirstSector.QuadPart = PartitionOffset->QuadPart >> ShiftCount;
    SectorCount.QuadPart = PartitionLength->QuadPart >> ShiftCount;

    /* Now calculate the last sector */
    LastSector = FirstSector.LowPart + SectorCount.LowPart - 1;

    /* Calculate the first and last cylinders */
    StartingCylinder = FirstSector.LowPart / SectorsPerCylinder;
    EndingCylinder = LastSector / SectorsPerCylinder;

    /* Set the default number of cylinders */
    if (!ConventionalCylinders) ConventionalCylinders = 1024;

    /* Normalize the values */
    if (StartingCylinder >= ConventionalCylinders)
    {
        /* Set the maximum to 1023 */
        StartingCylinder = ConventionalCylinders - 1;
    }
    if (EndingCylinder >= ConventionalCylinders)
    {
        /* Set the maximum to 1023 */
        EndingCylinder = ConventionalCylinders - 1;
    }

    /* Calculate the starting head and sector that still remain */
    Remainder = FirstSector.LowPart % SectorsPerCylinder;
    StartingTrack = Remainder / SectorsPerTrack;
    StartingSector = Remainder % SectorsPerTrack;

    /* Calculate the ending head and sector that still remain */
    Remainder = LastSector % SectorsPerCylinder;
    EndingTrack = Remainder / SectorsPerTrack;
    EndingSector = Remainder % SectorsPerTrack;

    /* Set cylinder data for the MSB */
    PartitionDescriptor->StartingCylinderMsb = (UCHAR)StartingCylinder;
    PartitionDescriptor->EndingCylinderMsb = (UCHAR)EndingCylinder;

    /* Set the track data */
    PartitionDescriptor->StartingTrack = (UCHAR)StartingTrack;
    PartitionDescriptor->EndingTrack = (UCHAR)EndingTrack;

    /* Update cylinder data for the LSB */
    StartingCylinder = ((StartingSector + 1) & 0x3F) |
                       ((StartingCylinder >> 2) & 0xC0);
    EndingCylinder = ((EndingSector + 1) & 0x3F) |
                     ((EndingCylinder >> 2) & 0xC0);

    /* Set the cylinder data for the LSB */
    PartitionDescriptor->StartingCylinderLsb = (UCHAR)StartingCylinder;
    PartitionDescriptor->EndingCylinderLsb = (UCHAR)EndingCylinder;
}

VOID
FASTCALL
xHalGetPartialGeometry(IN PDEVICE_OBJECT DeviceObject,
                       IN PULONG ConventionalCylinders,
                       IN PLONGLONG DiskSize)
{
    PDISK_GEOMETRY DiskGeometry = NULL;
    PIO_STATUS_BLOCK IoStatusBlock = NULL;
    PKEVENT Event = NULL;
    PIRP Irp;
    NTSTATUS Status;

    /* Set defaults */
    *ConventionalCylinders = 0;
    *DiskSize = 0;

    /* Allocate the structure in nonpaged pool */
    DiskGeometry = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(DISK_GEOMETRY),
                                         TAG_FILE_SYSTEM);
    if (!DiskGeometry) goto Cleanup;

    /* Allocate the status block in nonpaged pool */
    IoStatusBlock = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(IO_STATUS_BLOCK),
                                          TAG_FILE_SYSTEM);
    if (!IoStatusBlock) goto Cleanup;

    /* Allocate the event in nonpaged pool too */
    Event = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(KEVENT),
                                  TAG_FILE_SYSTEM);
    if (!Event) goto Cleanup;

    /* Initialize the event */
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        DiskGeometry,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        Event,
                                        IoStatusBlock);
    if (!Irp) goto Cleanup;

    /* Now call the driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for it to complete */
        KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock->Status;
    }

    /* Check driver status */
    if (NT_SUCCESS(Status))
    {
        /* Return the cylinder count */
        *ConventionalCylinders = DiskGeometry->Cylinders.LowPart;

        /* Make sure it's not larger than 1024 */
        if (DiskGeometry->Cylinders.LowPart >= 1024)
        {
            /* Otherwise, normalize the value */
            *ConventionalCylinders = 1024;
        }

        /* Calculate the disk size */
        *DiskSize = DiskGeometry->Cylinders.QuadPart *
                    DiskGeometry->TracksPerCylinder *
                    DiskGeometry->SectorsPerTrack *
                    DiskGeometry->BytesPerSector;
    }

Cleanup:
    /* Free all the pointers */
    if (Event) ExFreePoolWithTag(Event, TAG_FILE_SYSTEM);
    if (IoStatusBlock) ExFreePoolWithTag(IoStatusBlock, TAG_FILE_SYSTEM);
    if (DiskGeometry) ExFreePoolWithTag(DiskGeometry, TAG_FILE_SYSTEM);
    return;
}

VOID
FASTCALL
xHalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
               IN ULONG SectorSize,
               IN ULONG MbrTypeIdentifier,
               OUT PVOID *MbrBuffer)
{
    LARGE_INTEGER Offset;
    PUCHAR Buffer;
    ULONG BufferSize;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PPARTITION_DESCRIPTOR PartitionDescriptor;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;

    Offset.QuadPart = 0;

    /* Assume failure */
    *MbrBuffer = NULL;

    /* Normalize the buffer size */
    BufferSize = max(512, SectorSize);

    /* Allocate the buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   max(PAGE_SIZE, BufferSize),
                                   TAG_FILE_SYSTEM);
    if (!Buffer) return;

    /* Initialize the Event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Build the IRP */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       Buffer,
                                       BufferSize,
                                       &Offset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        /* Failed */
        ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
        return;
    }

    /* Make sure to override volume verification */
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Check driver status */
    if (NT_SUCCESS(Status))
    {
        /* Validate the MBR Signature */
        if (*(PUINT16)&Buffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
        {
            /* Failed */
            ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
            return;
        }

        /* Get the partition entry */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)&Buffer[PARTITION_TABLE_OFFSET];

        /* Make sure it's what the caller wanted */
        if (PartitionDescriptor->PartitionType != MbrTypeIdentifier)
        {
            /* It's not, free our buffer */
            ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
        }
        else
        {
            /* Check for OnTrack Disk Manager 6.0 / EZ-Drive partitions */

            if (PartitionDescriptor->PartitionType == PARTITION_DM)
            {
                /* Return our buffer, but at sector 63 */
                *(PULONG)Buffer = 63;
                *MbrBuffer = Buffer;
            }
            else if (PartitionDescriptor->PartitionType == PARTITION_EZDRIVE)
            {
                /* EZ-Drive, return the buffer directly */
                *MbrBuffer = Buffer;
            }
            else
            {
                /* Otherwise crash on debug builds */
                ASSERT(PartitionDescriptor->PartitionType == PARTITION_EZDRIVE);
            }
        }
    }
}

VOID
NTAPI
FstubFixupEfiPartition(IN PPARTITION_DESCRIPTOR PartitionDescriptor,
                       IN ULONGLONG MaxOffset)
{
    ULONG PartitionMaxOffset, PartitionLength;
    PAGED_CODE();

    /* Compute partition length (according to MBR entry) */
    PartitionMaxOffset = GET_STARTING_SECTOR(PartitionDescriptor) + GET_PARTITION_LENGTH(PartitionDescriptor);
    /* In case the partition length goes beyond disk size... */
    if (PartitionMaxOffset > MaxOffset)
    {
        /* Resize partition to its maximum real length */
        PartitionLength = (ULONG)(PartitionMaxOffset - GET_STARTING_SECTOR(PartitionDescriptor));
        SET_PARTITION_LENGTH(PartitionDescriptor, PartitionLength);
    }
}

NTSTATUS
FASTCALL
xHalIoReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                         IN ULONG SectorSize,
                         IN BOOLEAN ReturnRecognizedPartitions,
                         IN OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PPARTITION_DESCRIPTOR PartitionDescriptor;
    CCHAR Entry;
    NTSTATUS Status;
    PPARTITION_INFORMATION PartitionInfo;
    PUCHAR Buffer = NULL;
    ULONG BufferSize = 2048, InputSize;
    PDRIVE_LAYOUT_INFORMATION DriveLayoutInfo = NULL;
    LONG j = -1, i = -1, k;
    DISK_GEOMETRY_EX DiskGeometryEx;
    LONGLONG EndSector, MaxSector, StartOffset;
    LARGE_INTEGER Offset, VolumeOffset;
    BOOLEAN IsPrimary = TRUE, IsEzDrive = FALSE, MbrFound = FALSE;
    BOOLEAN IsValid, IsEmpty = TRUE;
    PVOID MbrBuffer;
    PIO_STACK_LOCATION IoStackLocation;
    UCHAR PartitionType;
    LARGE_INTEGER HiddenSectors64;

    PAGED_CODE();

    VolumeOffset.QuadPart = Offset.QuadPart = 0;

    /* Allocate the buffer */
    *PartitionBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                             BufferSize,
                                             TAG_FILE_SYSTEM);
    if (!(*PartitionBuffer)) return STATUS_INSUFFICIENT_RESOURCES;

    /* Normalize the buffer size */
    InputSize = max(512, SectorSize);

    /* Check for EZ-Drive */
    HalExamineMBR(DeviceObject, InputSize, PARTITION_EZDRIVE, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ-Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePoolWithTag(MbrBuffer, TAG_FILE_SYSTEM);
        Offset.QuadPart = 512;
    }

    /* Get drive geometry */
    Status = HalpGetFullGeometry(DeviceObject, &DiskGeometryEx);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(*PartitionBuffer, TAG_FILE_SYSTEM);
        *PartitionBuffer = NULL;
        return Status;
    }

    /* Get the end and maximum sector */
    EndSector = DiskGeometryEx.DiskSize.QuadPart / DiskGeometryEx.Geometry.BytesPerSector;
    MaxSector = EndSector << 1;
    DPRINT("FSTUB: DiskSize = %#I64x, MaxSector = %#I64x\n",
            DiskGeometryEx.DiskSize, MaxSector);

    /* Allocate our buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, InputSize, TAG_FILE_SYSTEM);
    if (!Buffer)
    {
        /* Fail, free the input buffer */
        ExFreePoolWithTag(*PartitionBuffer, TAG_FILE_SYSTEM);
        *PartitionBuffer = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Start partition loop */
    do
    {
        /* Assume the partition is valid */
        IsValid = TRUE;

        /* Initialize the event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Clear the buffer and build the IRP */
        RtlZeroMemory(Buffer, InputSize);
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           Buffer,
                                           InputSize,
                                           &Offset,
                                           &Event,
                                           &IoStatusBlock);
        if (!Irp)
        {
            /* Failed */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Make sure to disable volume verification */
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* Normalize status code and check for failure */
        if (Status == STATUS_NO_DATA_DETECTED) Status = STATUS_SUCCESS;
        if (!NT_SUCCESS(Status)) break;

        /* If we biased for EZ-Drive, unbias now */
        if (IsEzDrive && (Offset.QuadPart == 512)) Offset.QuadPart = 0;

        /* Make sure this is a valid MBR */
        if (*(PUINT16)&Buffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
        {
            /* It's not, fail */
            DPRINT1("FSTUB: (IoReadPartitionTable) No 0xaa55 found in "
                    "partition table %d\n", j + 1);
            break;
        }

        /* At this point we have a valid MBR */
        MbrFound = TRUE;

        /* Check if we weren't given an offset */
        if (!Offset.QuadPart)
        {
            /* Then read the signature off the disk */
            (*PartitionBuffer)->Signature = *(PUINT32)&Buffer[DISK_SIGNATURE_OFFSET];
        }

        /* Get the partition descriptor array */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)&Buffer[PARTITION_TABLE_OFFSET];

        /* Start looping partitions */
        j++;
        DPRINT("FSTUB: Partition Table %d:\n", j);
        for (Entry = 1, k = 0; Entry <= NUM_PARTITION_TABLE_ENTRIES; Entry++, PartitionDescriptor++)
        {
            /* Get the partition type */
            PartitionType = PartitionDescriptor->PartitionType;

            /* Print debug messages */
            DPRINT("Partition Entry %d,%d: type %#x %s\n",
                    j,
                    Entry,
                    PartitionType,
                    (PartitionDescriptor->ActiveFlag) ? "Active" : "");
            DPRINT("\tOffset %#08lx for %#08lx Sectors\n",
                    GET_STARTING_SECTOR(PartitionDescriptor),
                    GET_PARTITION_LENGTH(PartitionDescriptor));

            /* Check whether we're facing a protective MBR */
            if (PartitionType == EFI_PMBR_OSTYPE_EFI)
            {
                /* Partition length might be bigger than disk size */
                FstubFixupEfiPartition(PartitionDescriptor, DiskGeometryEx.DiskSize.QuadPart);
            }

            /* Make sure that the partition is valid, unless it's the first */
            if (!(HalpIsValidPartitionEntry(PartitionDescriptor,
                                            DiskGeometryEx.DiskSize.QuadPart,
                                            MaxSector)) && (j == 0))
            {
                /* It's invalid, so fail */
                IsValid = FALSE;
                break;
            }

            /* Check if it's a container */
            if (IsContainerPartition(PartitionType))
            {
                /* Increase the count of containers */
                if (++k != 1)
                {
                    /* More than one table is invalid */
                    DPRINT1("FSTUB: Multiple container partitions found in "
                            "partition table %d\n - table is invalid\n",
                            j);
                    IsValid = FALSE;
                    break;
                }
            }

            /* Check if the partition is supposedly empty */
            if (IsEmpty)
            {
                /* But check if it actually has a start and/or length */
                if ((GET_STARTING_SECTOR(PartitionDescriptor)) ||
                    (GET_PARTITION_LENGTH(PartitionDescriptor)))
                {
                    /* So then it's not really empty */
                    IsEmpty = FALSE;
                }
            }

            /* Check if the caller wanted only recognized partitions */
            if (ReturnRecognizedPartitions)
            {
                /* Then check if this one is unused, or a container */
                if ((PartitionType == PARTITION_ENTRY_UNUSED) ||
                    IsContainerPartition(PartitionType))
                {
                    /* Skip it, since the caller doesn't want it */
                    continue;
                }
            }

            /* Increase the structure count and check if they can fit */
            if ((sizeof(DRIVE_LAYOUT_INFORMATION) +
                 (++i * sizeof(PARTITION_INFORMATION))) >
                BufferSize)
            {
                /* Allocate a new buffer that's twice as big */
                DriveLayoutInfo = ExAllocatePoolWithTag(NonPagedPool,
                                                        BufferSize << 1,
                                                        TAG_FILE_SYSTEM);
                if (!DriveLayoutInfo)
                {
                    /* Out of memory, undo this extra structure */
                    --i;
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Copy the contents of the old buffer */
                RtlMoveMemory(DriveLayoutInfo,
                              *PartitionBuffer,
                              BufferSize);

                /* Free the old buffer and set this one as the new one */
                ExFreePoolWithTag(*PartitionBuffer, TAG_FILE_SYSTEM);
                *PartitionBuffer = DriveLayoutInfo;

                /* Double the size */
                BufferSize <<= 1;
            }

            /* Now get the current structure being filled and initialize it */
            PartitionInfo = &(*PartitionBuffer)->PartitionEntry[i];
            PartitionInfo->PartitionType = PartitionType;
            PartitionInfo->RewritePartition = FALSE;

            /* Check if we're dealing with a partition that's in use */
            if (PartitionType != PARTITION_ENTRY_UNUSED)
            {
                /* Check if it's bootable */
                PartitionInfo->BootIndicator = PartitionDescriptor->
                                               ActiveFlag & 0x80 ?
                                               TRUE : FALSE;

                /* Check if it's a container */
                if (IsContainerPartition(PartitionType))
                {
                    /* Then don't recognize it and use the volume offset */
                    PartitionInfo->RecognizedPartition = FALSE;
                    StartOffset = VolumeOffset.QuadPart;
                }
                else
                {
                    /* Then recognize it and use the partition offset */
                    PartitionInfo->RecognizedPartition = TRUE;
                    StartOffset = Offset.QuadPart;
                }

                /* Get the starting offset */
                PartitionInfo->StartingOffset.QuadPart =
                    StartOffset +
                    UInt32x32To64(GET_STARTING_SECTOR(PartitionDescriptor),
                                  SectorSize);

                /* Calculate the number of hidden sectors */
                HiddenSectors64.QuadPart = (PartitionInfo->
                                            StartingOffset.QuadPart -
                                            StartOffset) /
                                            SectorSize;
                PartitionInfo->HiddenSectors = HiddenSectors64.LowPart;

                /* Get the partition length */
                PartitionInfo->PartitionLength.QuadPart =
                    UInt32x32To64(GET_PARTITION_LENGTH(PartitionDescriptor),
                                  SectorSize);

                /* Get the partition number */
                /* FIXME: REACTOS HACK -- Needed for xHalIoAssignDriveLetters() */
                PartitionInfo->PartitionNumber = (!IsContainerPartition(PartitionType)) ? i + 1 : 0;
            }
            else
            {
                /* Otherwise, clear all the relevant fields */
                PartitionInfo->BootIndicator = FALSE;
                PartitionInfo->RecognizedPartition = FALSE;
                PartitionInfo->StartingOffset.QuadPart = 0;
                PartitionInfo->PartitionLength.QuadPart = 0;
                PartitionInfo->HiddenSectors = 0;

                /* FIXME: REACTOS HACK -- Needed for xHalIoAssignDriveLetters() */
                PartitionInfo->PartitionNumber = 0;
            }
        }

        /* Finish debug log, and check for failure */
        DPRINT("\n");
        if (!NT_SUCCESS(Status)) break;

        /* Also check if we hit an invalid entry here */
        if (!IsValid)
        {
            /* We did, so break out of the loop minus one entry */
            j--;
            break;
        }

        /* Reset the offset */
        Offset.QuadPart = 0;

        /* Go back to the descriptor array and loop it */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)&Buffer[PARTITION_TABLE_OFFSET];
        for (Entry = 1; Entry <= NUM_PARTITION_TABLE_ENTRIES; Entry++, PartitionDescriptor++)
        {
            /* Check if this is a container partition, since we skipped them */
            if (IsContainerPartition(PartitionDescriptor->PartitionType))
            {
                /* Get its offset */
                Offset.QuadPart = VolumeOffset.QuadPart +
                                  UInt32x32To64(
                                     GET_STARTING_SECTOR(PartitionDescriptor),
                                     SectorSize);

                /* If this is a primary partition, this is the volume offset */
                if (IsPrimary) VolumeOffset = Offset;

                /* Also update the maximum sector */
                MaxSector = GET_PARTITION_LENGTH(PartitionDescriptor);
                DPRINT1("FSTUB: MaxSector now = %I64d\n", MaxSector);
                break;
            }
        }

        /* Loop the next partitions, which are not primary anymore */
        IsPrimary = FALSE;
    } while (Offset.HighPart | Offset.LowPart);

    /* Check if this is a removable device that's probably a super-floppy */
    if ((DiskGeometryEx.Geometry.MediaType == RemovableMedia) &&
        (j == 0) && (MbrFound) && (IsEmpty))
    {
        PBOOT_SECTOR_INFO BootSectorInfo = (PBOOT_SECTOR_INFO)Buffer;

        /* Read the jump bytes to detect super-floppy */
        if ((BootSectorInfo->JumpByte[0] == 0xeb) ||
            (BootSectorInfo->JumpByte[0] == 0xe9))
        {
            /* Super floppes don't have typical MBRs, so skip them */
            DPRINT1("FSTUB: Jump byte %#x found along with empty partition "
                    "table - disk is a super floppy and has no valid MBR\n",
                    BootSectorInfo->JumpByte);
            j = -1;
        }
    }

    /* Check if we're still at partition -1 */
    if (j == -1)
    {
        /* The likely cause is the super floppy detection above */
        if ((MbrFound) || (DiskGeometryEx.Geometry.MediaType == RemovableMedia))
        {
            /* Print out debugging information */
            DPRINT1("FSTUB: Drive %#p has no valid MBR. Make it into a "
                    "super-floppy\n",
                    DeviceObject);
            DPRINT1("FSTUB: Drive has %I64d sectors and is %#016I64x "
                    "bytes large\n",
                    EndSector, DiskGeometryEx.DiskSize);

            /* We should at least have some sectors */
            if (EndSector > 0)
            {
                /* Get the entry we'll use */
                PartitionInfo = &(*PartitionBuffer)->PartitionEntry[0];

                /* Fill it out with data for a super-floppy */
                PartitionInfo->RewritePartition = FALSE;
                PartitionInfo->RecognizedPartition = TRUE;
                PartitionInfo->PartitionType = PARTITION_FAT_16;
                PartitionInfo->BootIndicator = FALSE;
                PartitionInfo->HiddenSectors = 0;
                PartitionInfo->StartingOffset.QuadPart = 0;
                PartitionInfo->PartitionLength = DiskGeometryEx.DiskSize;

                /* FIXME: REACTOS HACK -- Needed for xHalIoAssignDriveLetters() */
                PartitionInfo->PartitionNumber = 0;

                /* Set the signature and set the count back to 0 */
                (*PartitionBuffer)->Signature = 1;
                i = 0;
            }
        }
        else
        {
            /* Otherwise, this isn't a super floppy, so set an invalid count */
            i = -1;
        }
    }

    /* Set the partition count */
    (*PartitionBuffer)->PartitionCount = ++i;

    /* If we have no count, delete the signature */
    if (!i) (*PartitionBuffer)->Signature = 0;

    /* Free the buffer and check for success */
    if (Buffer) ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(*PartitionBuffer, TAG_FILE_SYSTEM);
        *PartitionBuffer = NULL;
    }

    /* Return status */
    return Status;
}

NTSTATUS
FASTCALL
xHalIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                              IN ULONG SectorSize,
                              IN ULONG PartitionNumber,
                              IN ULONG PartitionType)
{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    LARGE_INTEGER Offset, VolumeOffset;
    PUCHAR Buffer = NULL;
    ULONG BufferSize;
    ULONG i = 0;
    ULONG Entry;
    PPARTITION_DESCRIPTOR PartitionDescriptor;
    BOOLEAN IsPrimary = TRUE, IsEzDrive = FALSE;
    PVOID MbrBuffer;
    PIO_STACK_LOCATION IoStackLocation;

    PAGED_CODE();

    VolumeOffset.QuadPart = Offset.QuadPart = 0;

    /* Normalize the buffer size */
    BufferSize = max(512, SectorSize);

    /* Check for EZ-Drive */
    HalExamineMBR(DeviceObject, BufferSize, PARTITION_EZDRIVE, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ-Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePoolWithTag(MbrBuffer, TAG_FILE_SYSTEM);
        Offset.QuadPart = 512;
    }

    /* Allocate our partition buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, PAGE_SIZE, TAG_FILE_SYSTEM);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the event we'll use and loop partitions */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    do
    {
        /* Reset the event since we reuse it */
        KeClearEvent(&Event);

        /* Build the read IRP */
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           Buffer,
                                           BufferSize,
                                           &Offset,
                                           &Event,
                                           &IoStatusBlock);
        if (!Irp)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Make sure to disable volume verification */
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* Check for failure */
        if (!NT_SUCCESS(Status)) break;

        /* If we biased for EZ-Drive, unbias now */
        if (IsEzDrive && (Offset.QuadPart == 512)) Offset.QuadPart = 0;

        /* Make sure this is a valid MBR */
        if (*(PUINT16)&Buffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
        {
            /* It's not, fail */
            Status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        /* Get the partition descriptors and loop them */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)&Buffer[PARTITION_TABLE_OFFSET];
        for (Entry = 1; Entry <= NUM_PARTITION_TABLE_ENTRIES; Entry++, PartitionDescriptor++)
        {
            /* Check if it's unused or a container partition */
            if ((PartitionDescriptor->PartitionType == PARTITION_ENTRY_UNUSED) ||
                (IsContainerPartition(PartitionDescriptor->PartitionType)))
            {
                /* Go to the next one */
                continue;
            }

            /* It's a valid partition, so increase the partition count */
            if (++i == PartitionNumber)
            {
                /* We found a match, set the type */
                PartitionDescriptor->PartitionType = (UCHAR)PartitionType;

                /* Reset the reusable event */
                KeClearEvent(&Event);

                /* Build the write IRP */
                Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                                   DeviceObject,
                                                   Buffer,
                                                   BufferSize,
                                                   &Offset,
                                                   &Event,
                                                   &IoStatusBlock);
                if (!Irp)
                {
                    /* Fail */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Disable volume verification */
                IoStackLocation = IoGetNextIrpStackLocation(Irp);
                IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

                /* Call the driver */
                Status = IoCallDriver(DeviceObject, Irp);
                if (Status == STATUS_PENDING)
                {
                    /* Wait for completion */
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    Status = IoStatusBlock.Status;
                }

                /* We're done, break out of the loop */
                break;
            }
        }

        /* If we looped all the partitions, break out */
        if (Entry <= NUM_PARTITION_TABLE_ENTRIES) break;

        /* Nothing found yet, get the partition array again */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)&Buffer[PARTITION_TABLE_OFFSET];
        for (Entry = 1; Entry <= NUM_PARTITION_TABLE_ENTRIES; Entry++, PartitionDescriptor++)
        {
            /* Check if this was a container partition (we skipped these) */
            if (IsContainerPartition(PartitionDescriptor->PartitionType))
            {
                /* Update the partition offset */
                Offset.QuadPart = VolumeOffset.QuadPart +
                                  GET_STARTING_SECTOR(PartitionDescriptor) *
                                  SectorSize;

                /* If this was the primary partition, update the volume too */
                if (IsPrimary) VolumeOffset = Offset;
                break;
            }
        }

        /* Check if we already searched all the partitions */
        if (Entry > NUM_PARTITION_TABLE_ENTRIES)
        {
            /* Then we failed to find a good MBR */
            Status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        /* Loop the next partitions, which are not primary anymore */
        IsPrimary = FALSE;
    } while (i < PartitionNumber);

    /* Everything done, cleanup */
    if (Buffer) ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
    return Status;
}

NTSTATUS
FASTCALL
xHalIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                          IN ULONG SectorSize,
                          IN ULONG SectorsPerTrack,
                          IN ULONG NumberOfHeads,
                          IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferSize;
    PUCHAR Buffer;
    PPTE Entry;
    PPARTITION_TABLE PartitionTable;
    LARGE_INTEGER Offset, NextOffset, ExtendedOffset, SectorOffset;
    LARGE_INTEGER StartOffset, PartitionLength;
    ULONG i, j;
    CCHAR k;
    BOOLEAN IsEzDrive = FALSE, IsSuperFloppy = FALSE, DoRewrite = FALSE, IsMbr;
    ULONG ConventionalCylinders;
    LONGLONG DiskSize;
    PDISK_LAYOUT DiskLayout = (PDISK_LAYOUT)PartitionBuffer;
    PVOID MbrBuffer;
    UCHAR PartitionType;
    PIO_STACK_LOCATION IoStackLocation;
    PPARTITION_INFORMATION PartitionInfo = PartitionBuffer->PartitionEntry;
    PPARTITION_INFORMATION TableEntry;

    PAGED_CODE();

    ExtendedOffset.QuadPart = NextOffset.QuadPart = Offset.QuadPart = 0;

    /* Normalize the buffer size */
    BufferSize = max(512, SectorSize);

    /* Get the partial drive geometry */
    xHalGetPartialGeometry(DeviceObject, &ConventionalCylinders, &DiskSize);

    /* Check for EZ-Drive */
    HalExamineMBR(DeviceObject, BufferSize, PARTITION_EZDRIVE, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ-Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePoolWithTag(MbrBuffer, TAG_FILE_SYSTEM);
        Offset.QuadPart = 512;
    }

    /* Get the number of bits to shift to multiply by the sector size */
    for (k = 0; k < 32; k++) if ((SectorSize >> k) == 1) break;

    /* Check if there's only one partition */
    if (PartitionBuffer->PartitionCount == 1)
    {
        /* Check if it has no starting offset or hidden sectors */
        if (!(PartitionInfo->StartingOffset.QuadPart) &&
            !(PartitionInfo->HiddenSectors))
        {
            /* Then it's a super floppy */
            IsSuperFloppy = TRUE;

            /* Which also means it must be non-bootable FAT-16 */
            if ((PartitionInfo->PartitionNumber) ||
                (PartitionInfo->PartitionType != PARTITION_FAT_16) ||
                (PartitionInfo->BootIndicator))
            {
                /* It's not, so we fail */
                return STATUS_INVALID_PARAMETER;
            }

            /* Check if it needs a rewrite, and disable EZ-Drive for sure */
            if (PartitionInfo->RewritePartition) DoRewrite = TRUE;
            IsEzDrive = FALSE;
        }
    }

    /* Count the number of partition tables */
    DiskLayout->TableCount = (PartitionBuffer->PartitionCount + NUM_PARTITION_TABLE_ENTRIES - 1) / NUM_PARTITION_TABLE_ENTRIES;

    /* Allocate our partition buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, PAGE_SIZE, TAG_FILE_SYSTEM);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Loop the entries */
    Entry = (PPTE)&Buffer[PARTITION_TABLE_OFFSET];
    for (i = 0; i < DiskLayout->TableCount; i++)
    {
        /* Set if this is the MBR partition */
        IsMbr= (BOOLEAN)!i;

        /* Initialize th event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Build the read IRP */
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           Buffer,
                                           BufferSize,
                                           &Offset,
                                           &Event,
                                           &IoStatusBlock);
        if (!Irp)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Make sure to disable volume verification */
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* Check for failure */
        if (!NT_SUCCESS(Status)) break;

        /* If we biased for EZ-Drive, unbias now */
        if (IsEzDrive && (Offset.QuadPart == 512)) Offset.QuadPart = 0;

        /* Check if this is a normal disk */
        if (!IsSuperFloppy)
        {
            /* Set the boot record signature */
            *(PUINT16)&Buffer[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;

            /* By default, don't require a rewrite */
            DoRewrite = FALSE;

            /* Check if we don't have an offset */
            if (!Offset.QuadPart)
            {
                /* Check if the signature doesn't match */
                if (*(PUINT32)&Buffer[DISK_SIGNATURE_OFFSET] != PartitionBuffer->Signature)
                {
                    /* Then write the signature and now we need a rewrite */
                    *(PUINT32)&Buffer[DISK_SIGNATURE_OFFSET] = PartitionBuffer->Signature;
                    DoRewrite = TRUE;
                }
            }

            /* Loop the partition table entries */
            PartitionTable = &DiskLayout->PartitionTable[i];
            for (j = 0; j < NUM_PARTITION_TABLE_ENTRIES; j++)
            {
                /* Get the current entry and type */
                TableEntry = &PartitionTable->PartitionEntry[j];
                PartitionType = TableEntry->PartitionType;

                /* Check if the entry needs a rewrite */
                if (TableEntry->RewritePartition)
                {
                    /* Then we need one too */
                    DoRewrite = TRUE;

                    /* Save the type and if it's a bootable partition */
                    Entry[j].PartitionType = TableEntry->PartitionType;
                    Entry[j].ActiveFlag = TableEntry->BootIndicator ? 0x80 : 0;

                    /* Make sure it's used */
                    if (PartitionType != PARTITION_ENTRY_UNUSED)
                    {
                        /* Make sure it's not a container (unless primary) */
                        if ((IsMbr) || !(IsContainerPartition(PartitionType)))
                        {
                            /* Use the partition offset */
                            StartOffset.QuadPart = Offset.QuadPart;
                        }
                        else
                        {
                            /* Use the extended logical partition offset */
                            StartOffset.QuadPart = ExtendedOffset.QuadPart;
                        }

                        /* Set the sector offset */
                        SectorOffset.QuadPart = TableEntry->
                                                StartingOffset.QuadPart -
                                                StartOffset.QuadPart;

                        /* Now calculate the starting sector */
                        StartOffset.QuadPart = SectorOffset.QuadPart >> k;
                        Entry[j].StartingSector = StartOffset.LowPart;

                        /* As well as the length */
                        PartitionLength.QuadPart = TableEntry->PartitionLength.
                                                   QuadPart >> k;
                        Entry[j].PartitionLength = PartitionLength.LowPart;

                        /* Calculate the CHS values */
                        HalpCalculateChsValues(&TableEntry->StartingOffset,
                                               &TableEntry->PartitionLength,
                                               k,
                                               SectorsPerTrack,
                                               NumberOfHeads,
                                               ConventionalCylinders,
                                               (PPARTITION_DESCRIPTOR)
                                               &Entry[j]);
                    }
                    else
                    {
                        /* Otherwise set up an empty entry */
                        Entry[j].StartingSector = 0;
                        Entry[j].PartitionLength = 0;
                        Entry[j].StartingTrack = 0;
                        Entry[j].EndingTrack = 0;
                        Entry[j].StartingCylinder = 0;
                        Entry[j].EndingCylinder = 0;
                    }
                }

                /* Check if this is a container partition */
                if (IsContainerPartition(PartitionType))
                {
                    /* Then update the offset to use */
                    NextOffset = TableEntry->StartingOffset;
                }
            }
        }

        /* Check if we need to write back the buffer */
        if (DoRewrite)
        {
            /* We don't need to do this again */
            DoRewrite = FALSE;

            /* Initialize the event */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);

            /* If we unbiased for EZ-Drive, rebias now */
            if (IsEzDrive && !Offset.QuadPart) Offset.QuadPart = 512;

            /* Build the write IRP */
            Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                               DeviceObject,
                                               Buffer,
                                               BufferSize,
                                               &Offset,
                                               &Event,
                                               &IoStatusBlock);
            if (!Irp)
            {
                /* Fail */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* Make sure to disable volume verification */
            IoStackLocation = IoGetNextIrpStackLocation(Irp);
            IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

            /* Call the driver */
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion */
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }

            /* Check for failure */
            if (!NT_SUCCESS(Status)) break;

            /* If we biased for EZ-Drive, unbias now */
            if (IsEzDrive && (Offset.QuadPart == 512)) Offset.QuadPart = 0;
        }

        /* Update the partition offset and set the extended offset if needed */
        Offset = NextOffset;
        if (IsMbr) ExtendedOffset = NextOffset;
    }

    /* If we had a buffer, free it, then return status */
    if (Buffer) ExFreePoolWithTag(Buffer, TAG_FILE_SYSTEM);
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
HalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
              IN ULONG SectorSize,
              IN ULONG MbrTypeIdentifier,
              OUT PVOID *MbrBuffer)
{
    HALDISPATCH->HalExamineMBR(DeviceObject,
                               SectorSize,
                               MbrTypeIdentifier,
                               MbrBuffer);
}

/*
 * @implemented
 */
NTSTATUS
FASTCALL
IoReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                     IN ULONG SectorSize,
                     IN BOOLEAN ReturnRecognizedPartitions,
                     IN OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    return HALDISPATCH->HalIoReadPartitionTable(DeviceObject,
                                                SectorSize,
                                                ReturnRecognizedPartitions,
                                                PartitionBuffer);
}

/*
 * @implemented
 */
NTSTATUS
FASTCALL
IoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                          IN ULONG SectorSize,
                          IN ULONG PartitionNumber,
                          IN ULONG PartitionType)
{
    return HALDISPATCH->HalIoSetPartitionInformation(DeviceObject,
                                                     SectorSize,
                                                     PartitionNumber,
                                                     PartitionType);
}

/*
 * @implemented
 */
NTSTATUS
FASTCALL
IoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                      IN ULONG SectorSize,
                      IN ULONG SectorsPerTrack,
                      IN ULONG NumberOfHeads,
                      IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    return HALDISPATCH->HalIoWritePartitionTable(DeviceObject,
                                                 SectorSize,
                                                 SectorsPerTrack,
                                                 NumberOfHeads,
                                                 PartitionBuffer);
}

/*
 * @implemented
 */
VOID
FASTCALL
IoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                     IN PSTRING NtDeviceName,
                     OUT PUCHAR NtSystemPath,
                     OUT PSTRING NtSystemPathString)
{
    HALDISPATCH->HalIoAssignDriveLetters(LoaderBlock,
                                         NtDeviceName,
                                         NtSystemPath,
                                         NtSystemPathString);
}

/* EOF */
