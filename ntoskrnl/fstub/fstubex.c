/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/fstubex.c
* PURPOSE:         Extended FSTUB Routines (not linked to HAL)
* PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

#define PARTITION_ENTRY_SIZE 128
#define TAG_FSTUB 'BtsF'

typedef struct _DISK_INFORMATION
{
    PDEVICE_OBJECT DeviceObject;
    ULONG SectorSize;
    DISK_GEOMETRY_EX DiskGeometry;
    PUSHORT Buffer;
    ULONGLONG SectorCount;
} DISK_INFORMATION, *PDISK_INFORMATION;

/* Defines system type for MBR showing that a GPT is following */ 
#define EFI_PMBR_OSTYPE_EFI 0xEE

#define IS_VALID_DISK_INFO(Disk) \
  (Disk)               &&        \
  (Disk->DeviceObject) &&        \
  (Disk->SectorSize)   &&        \
  (Disk->Buffer)       &&        \
  (Disk->SectorCount)

VOID
NTAPI
FstubDbgPrintPartitionEx(IN PPARTITION_INFORMATION_EX PartitionEntry,
                         IN ULONG PartitionNumber
);

NTSTATUS
NTAPI
FstubGetDiskGeometry(IN PDEVICE_OBJECT DeviceObject,
                     OUT PDISK_GEOMETRY_EX Geometry
);

NTSTATUS
NTAPI
FstubReadSector(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG SectorSize,
                IN ULONGLONG StartingSector OPTIONAL,
                OUT PUSHORT Buffer
);

NTSTATUS
NTAPI
FstubAllocateDiskInformation(IN PDEVICE_OBJECT DeviceObject,
                             OUT PDISK_INFORMATION * DiskBuffer,
                             PDISK_GEOMETRY_EX DiskGeometry OPTIONAL)
{
    NTSTATUS Status;
    PDISK_INFORMATION DiskInformation;
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(DiskBuffer);

    /* Allocate internal structure */
    DiskInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(DISK_INFORMATION), TAG_FSTUB);
    if (!DiskInformation)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* If caller don't pass needed information, let's get them */
    if (!DiskGeometry)
    {
        Status = FstubGetDiskGeometry(DeviceObject, &(DiskInformation->DiskGeometry));
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(DiskInformation, TAG_FSTUB);
            return Status;
        }
    }
    else
    {
        DiskInformation->DiskGeometry = *DiskGeometry;
    }

    /* Ensure read/received information are correct */
    if (DiskInformation->DiskGeometry.Geometry.BytesPerSector == 0 ||
        DiskInformation->DiskGeometry.DiskSize.QuadPart == 0)
    {
        ExFreePoolWithTag(DiskInformation, TAG_FSTUB);
        return STATUS_DEVICE_NOT_READY;
    }

    /* Store vital information as well */
    DiskInformation->DeviceObject = DeviceObject;
    DiskInformation->SectorSize = DiskInformation->DiskGeometry.Geometry.BytesPerSector;
    DiskInformation->SectorCount = DiskInformation->DiskGeometry.DiskSize.QuadPart / DiskInformation->SectorSize;

    /* Finally, allocate the buffer that will be used for different read */
    DiskInformation->Buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, DiskInformation->SectorSize, TAG_FSTUB);
    if (!DiskInformation->Buffer)
    {
        ExFreePoolWithTag(DiskInformation, TAG_FSTUB);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return allocated internal structure */
    *DiskBuffer = DiskInformation;

    return STATUS_SUCCESS;
}

PCHAR
NTAPI
FstubDbgGuidToString(IN PGUID Guid,
                     OUT PCHAR String)
{
    sprintf(String,
            "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            Guid->Data1,
            Guid->Data2,
            Guid->Data3,
            Guid->Data4[0],
            Guid->Data4[1],
            Guid->Data4[2],
            Guid->Data4[3],
            Guid->Data4[4],
            Guid->Data4[5],
            Guid->Data4[6],
            Guid->Data4[7]);

    return String;
}

VOID
NTAPI
FstubDbgPrintDriveLayoutEx(IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout)
{
    ULONG i;
    CHAR Guid[38];
    PAGED_CODE();

    DPRINT1("FSTUB: DRIVE_LAYOUT_INFORMATION_EX: %p\n", DriveLayout);
    switch (DriveLayout->PartitionStyle)
    {
        case PARTITION_STYLE_MBR:
            if (DriveLayout->PartitionCount % 4 != 0)
            {
                DPRINT1("Warning: Partition count isn't a 4-factor: %ld!\n", DriveLayout->PartitionCount);
            }

            DPRINT1("Signature: %8.8x\n", DriveLayout->Mbr.Signature);
            for (i = 0; i < DriveLayout->PartitionCount; i++)
            {
                FstubDbgPrintPartitionEx(DriveLayout->PartitionEntry, i);
            }

            break;
        case PARTITION_STYLE_GPT:
            FstubDbgGuidToString(&(DriveLayout->Gpt.DiskId), Guid);
            DPRINT1("DiskId: %s\n", Guid);
            DPRINT1("StartingUsableOffset: %I64x\n", DriveLayout->Gpt.StartingUsableOffset.QuadPart);
            DPRINT1("UsableLength: %I64x\n", DriveLayout->Gpt.UsableLength.QuadPart);
            DPRINT1("MaxPartitionCount: %ld\n", DriveLayout->Gpt.MaxPartitionCount);
            for (i = 0; i < DriveLayout->PartitionCount; i++)
            {
                FstubDbgPrintPartitionEx(DriveLayout->PartitionEntry, i);
            }

            break;
        default:
            DPRINT1("Unsupported partition style: %ld\n", DriveLayout->PartitionStyle);
    }
}

VOID
NTAPI
FstubDbgPrintPartitionEx(IN PPARTITION_INFORMATION_EX PartitionEntry,
                         IN ULONG PartitionNumber)
{
    CHAR Guid[38];
    PAGED_CODE();

    DPRINT1("Printing partition %ld\n", PartitionNumber);

    switch (PartitionEntry[PartitionNumber].PartitionStyle)
    {
        case PARTITION_STYLE_MBR:
            DPRINT1("  StartingOffset: %I64x\n", PartitionEntry[PartitionNumber].StartingOffset.QuadPart);
            DPRINT1("  PartitionLength: %I64x\n", PartitionEntry[PartitionNumber].PartitionLength.QuadPart);
            DPRINT1("  RewritePartition: %d\n", PartitionEntry[PartitionNumber].RewritePartition);
            DPRINT1("  PartitionType: %02x\n", PartitionEntry[PartitionNumber].Mbr.PartitionType);
            DPRINT1("  BootIndicator: %d\n", PartitionEntry[PartitionNumber].Mbr.BootIndicator);
            DPRINT1("  RecognizedPartition: %d\n", PartitionEntry[PartitionNumber].Mbr.RecognizedPartition);
            DPRINT1("  HiddenSectors: %ld\n", PartitionEntry[PartitionNumber].Mbr.HiddenSectors);

            break;
        case PARTITION_STYLE_GPT:
            DPRINT1("  StartingOffset: %I64x\n", PartitionEntry[PartitionNumber].StartingOffset.QuadPart);
            DPRINT1("  PartitionLength: %I64x\n", PartitionEntry[PartitionNumber].PartitionLength.QuadPart);
            DPRINT1("  RewritePartition: %d\n", PartitionEntry[PartitionNumber].RewritePartition);
            FstubDbgGuidToString(&(PartitionEntry[PartitionNumber].Gpt.PartitionType), Guid);
            DPRINT1("  PartitionType: %s\n", Guid);
            FstubDbgGuidToString(&(PartitionEntry[PartitionNumber].Gpt.PartitionId), Guid);
            DPRINT1("  PartitionId: %s\n", Guid);
            DPRINT1("  Attributes: %16x\n", PartitionEntry[PartitionNumber].Gpt.Attributes);
            DPRINT1("  Name: %ws\n", PartitionEntry[PartitionNumber].Gpt.Name);

            break;
        default:
            DPRINT1("  Unsupported partition style: %ld\n", PartitionEntry[PartitionNumber].PartitionStyle);
    }
}

NTSTATUS
NTAPI
FstubDetectPartitionStyle(IN PDISK_INFORMATION Disk,
                          IN PARTITION_STYLE * PartitionStyle)
{
    NTSTATUS Status;
    PPARTITION_DESCRIPTOR PartitionDescriptor;
    PAGED_CODE();

    ASSERT(IS_VALID_DISK_INFO(Disk));
    ASSERT(PartitionStyle);

    /* Read disk first sector */
    Status = FstubReadSector(Disk->DeviceObject,
                             Disk->SectorSize,
                             0,
                             Disk->Buffer);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the partition descriptor array */
    PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                          &(Disk->Buffer[PARTITION_TABLE_OFFSET]);
    /* If we have not the 0xAA55 then it's raw partition */
    if (Disk->Buffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
    {
        *PartitionStyle = PARTITION_STYLE_RAW;
    }
    /* Check partitions types: if first is 0xEE and all the others 0, we have GPT */
    else if (PartitionDescriptor[0].PartitionType == EFI_PMBR_OSTYPE_EFI &&
             PartitionDescriptor[1].PartitionType == 0 &&
             PartitionDescriptor[2].PartitionType == 0 &&
             PartitionDescriptor[3].PartitionType == 0)
    {
        *PartitionStyle = PARTITION_STYLE_GPT;
    }
    /* Otherwise, partition table is in MBR */
    else
    {
        *PartitionStyle = PARTITION_STYLE_MBR;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
FstubFreeDiskInformation(IN PDISK_INFORMATION DiskBuffer)
{
    if (DiskBuffer)
    {
        if (DiskBuffer->Buffer)
        {
            ExFreePoolWithTag(DiskBuffer->Buffer, TAG_FSTUB);
        }
        ExFreePoolWithTag(DiskBuffer, TAG_FSTUB);
    }
}

NTSTATUS
NTAPI
FstubGetDiskGeometry(IN PDEVICE_OBJECT DeviceObject,
                     OUT PDISK_GEOMETRY_EX Geometry)
{
    PIRP Irp;
    NTSTATUS Status;
    PKEVENT Event = NULL;
    PDISK_GEOMETRY_EX DiskGeometry = NULL;
    PIO_STATUS_BLOCK IoStatusBlock = NULL;
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Geometry);

    /* Allocate needed components */
    DiskGeometry = ExAllocatePoolWithTag(NonPagedPool, sizeof(DISK_GEOMETRY_EX), TAG_FSTUB);
    if (!DiskGeometry)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    IoStatusBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_STATUS_BLOCK), TAG_FSTUB);
    if (!IoStatusBlock)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_FSTUB);
    if (!Event)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    /* Initialize the waiting event */
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    /* Build the request to get disk geometry */
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                        DeviceObject,
                                        0,
                                        0,
                                        DiskGeometry,
                                        sizeof(DISK_GEOMETRY_EX),
                                        FALSE,
                                        Event,
                                        IoStatusBlock);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Call the driver and wait for completion if needed */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock->Status;
    }

    /* In case of a success, return read data */
    if (NT_SUCCESS(Status))
    {
        *Geometry = *DiskGeometry;
    }

Cleanup:
    if (DiskGeometry)
    {
        ExFreePoolWithTag(DiskGeometry, TAG_FSTUB);

        if (NT_SUCCESS(Status))
        {
            ASSERT(Geometry->Geometry.BytesPerSector % PARTITION_ENTRY_SIZE == 0);
        }
    }

    if (IoStatusBlock)
    {
        ExFreePoolWithTag(IoStatusBlock, TAG_FSTUB);
    }

    if (Event)
    {
        ExFreePoolWithTag(Event, TAG_FSTUB);
    }

    return Status;
}

NTSTATUS
NTAPI
FstubReadPartitionTableEFI(IN PDISK_INFORMATION Disk,
                           IN BOOLEAN ReadBackupTable,
                           OUT struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
FstubReadPartitionTableMBR(IN PDISK_INFORMATION Disk,
                           IN BOOLEAN ReturnRecognizedPartitions,
                           OUT struct _DRIVE_LAYOUT_INFORMATION_EX** ReturnedDriveLayout)
{
    ULONG i;
    NTSTATUS Status;
    PDRIVE_LAYOUT_INFORMATION DriveLayout = NULL;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutEx = NULL;
    PAGED_CODE();

    ASSERT(IS_VALID_DISK_INFO(Disk));
    ASSERT(ReturnedDriveLayout);

    /* Zero output */
    *ReturnedDriveLayout = NULL;

    /* Read partition table the old way */
    Status = IoReadPartitionTable(Disk->DeviceObject,
                                  Disk->SectorSize,
                                  ReturnRecognizedPartitions,
                                  &DriveLayout);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Allocate a DRIVE_LAYOUT_INFORMATION_EX struct big enough */
    DriveLayoutEx = ExAllocatePoolWithTag(NonPagedPool,
                                          FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                                          DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX),
                                          TAG_FSTUB);
    if (!DriveLayoutEx)
    {
        /* Let's not leak memory as in Windows 2003 */
        ExFreePool(DriveLayout);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Start converting the DRIVE_LAYOUT_INFORMATION structure */
    DriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
    DriveLayoutEx->PartitionCount = DriveLayout->PartitionCount;
    DriveLayoutEx->Mbr.Signature = DriveLayout->Signature;

    /* Convert each found partition */
    for (i = 0; i < DriveLayout->PartitionCount; i++)
    {
        DriveLayoutEx->PartitionEntry[i].PartitionStyle = PARTITION_STYLE_MBR;
        DriveLayoutEx->PartitionEntry[i].StartingOffset = DriveLayout->PartitionEntry[i].StartingOffset;
        DriveLayoutEx->PartitionEntry[i].PartitionLength = DriveLayout->PartitionEntry[i].PartitionLength;
        DriveLayoutEx->PartitionEntry[i].PartitionNumber = DriveLayout->PartitionEntry[i].PartitionNumber;
        DriveLayoutEx->PartitionEntry[i].RewritePartition = DriveLayout->PartitionEntry[i].RewritePartition;
        DriveLayoutEx->PartitionEntry[i].Mbr.PartitionType = DriveLayout->PartitionEntry[i].PartitionType;
        DriveLayoutEx->PartitionEntry[i].Mbr.BootIndicator = DriveLayout->PartitionEntry[i].BootIndicator;
        DriveLayoutEx->PartitionEntry[i].Mbr.RecognizedPartition = DriveLayout->PartitionEntry[i].RecognizedPartition;
        DriveLayoutEx->PartitionEntry[i].Mbr.HiddenSectors = DriveLayout->PartitionEntry[i].HiddenSectors;
    }

    /* Finally, return data and free old structure */
    *ReturnedDriveLayout = DriveLayoutEx;
    ExFreePool(DriveLayout);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FstubReadSector(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG SectorSize,
                IN ULONGLONG StartingSector OPTIONAL,
                OUT PUSHORT Buffer)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    LARGE_INTEGER StartingOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStackLocation;
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Buffer);
    ASSERT(SectorSize);

    /* Compute starting offset */
    StartingOffset.QuadPart = StartingSector * SectorSize;

    /* Initialize waiting event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Prepare IRP */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       Buffer,
                                       SectorSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Override volume verify */
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    /* Then call driver, and wait for completion if needed */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoCreateDisk(IN PDEVICE_OBJECT DeviceObject,
             IN struct _CREATE_DISK* Disk)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoGetBootDiskInformation(IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
                         IN ULONG Size)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReadDiskSignature(IN PDEVICE_OBJECT DeviceObject,
                    IN ULONG BytesPerSector,
                    OUT PDISK_SIGNATURE Signature)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReadPartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                       IN struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout)
{
    NTSTATUS Status;
    PDISK_INFORMATION Disk;
    PARTITION_STYLE PartitionStyle;
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(DriveLayout);

    /* First of all, allocate internal structure */
    Status = FstubAllocateDiskInformation(DeviceObject, &Disk, 0);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    ASSERT(Disk);

    /* Then, detect partition style (MBR? GTP/EFI? RAW?) */
    Status = FstubDetectPartitionStyle(Disk, &PartitionStyle);
    if (!NT_SUCCESS(Status))
    {
        FstubFreeDiskInformation(Disk);
        return Status;
    }

    /* Here partition table is really read, depending on its style */
    switch (PartitionStyle)
    {
        case PARTITION_STYLE_MBR:
        case PARTITION_STYLE_RAW:
            Status = FstubReadPartitionTableMBR(Disk, FALSE, DriveLayout);
            break;

        case PARTITION_STYLE_GPT:
             /* Read primary table */
             Status = FstubReadPartitionTableEFI(Disk, FALSE, DriveLayout);
             /* If it failed, try reading backup table */
             if (!NT_SUCCESS(Status))
             {
                 Status = FstubReadPartitionTableEFI(Disk, TRUE, DriveLayout);
             }
             break;

        default:
             DPRINT("Unknown partition type\n");
             Status = STATUS_UNSUCCESSFUL;
    }

    /* It's over, internal structure not needed anymore */
    FstubFreeDiskInformation(Disk);

    /* In case of success, print data */
    if (NT_SUCCESS(Status))
    {
        FstubDbgPrintDriveLayoutEx(*DriveLayout);
    }

    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoSetPartitionInformationEx(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG PartitionNumber,
                            IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoVerifyPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                       IN BOOLEAN FixErrors)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWritePartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                        IN struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayfout)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
