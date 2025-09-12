/*
 * PROJECT:         Ramdisk Class Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/storage/class/ramdisk/ramdisk.c
 * PURPOSE:         Main Driver Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <initguid.h>
#include <ntddk.h>
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddvol.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>
#include <reactos/drivers/ntddrdsk.h>
#include "../../../filesystems/fs_rec/fs_rec.h"
#include <stdio.h>
#define NDEBUG
#include <debug.h>

#define DO_XIP   0x00020000

/* GLOBALS ********************************************************************/

#define RAMDISK_SESSION_SIZE \
    FIELD_OFFSET(CDROM_TOC, TrackData) + sizeof(TRACK_DATA)

#define RAMDISK_TOC_SIZE \
    FIELD_OFFSET(CDROM_TOC, TrackData) + 2 * sizeof(TRACK_DATA)

#define TOC_DATA_TRACK              (0x04)

typedef enum _RAMDISK_DEVICE_TYPE
{
    RamdiskBus,
    RamdiskDrive
} RAMDISK_DEVICE_TYPE;

typedef enum _RAMDISK_DEVICE_STATE
{
    RamdiskStateUninitialized,
    RamdiskStateStarted,
    RamdiskStatePaused,
    RamdiskStateStopped,
    RamdiskStateRemoved,
    RamdiskStateBusRemoved,
    RamdiskStateEnumerated,
} RAMDISK_DEVICE_STATE;

DEFINE_GUID(RamdiskBusInterface,
            0x5DC52DF0,
            0x2F8A,
            0x410F,
            0x80, 0xE4, 0x05, 0xF8, 0x10, 0xE7, 0xA8, 0x8A);

DEFINE_GUID(RamdiskDiskInterface,
            0x31D909F0,
            0x2CDF,
            0x4A20,
            0x9E, 0xD4, 0x7D, 0x65, 0x47, 0x6C, 0xA7, 0x68);

typedef struct _RAMDISK_EXTENSION
{
    RAMDISK_DEVICE_TYPE Type;
    RAMDISK_DEVICE_STATE State;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    IO_REMOVE_LOCK RemoveLock;
    UNICODE_STRING DriveDeviceName;
    UNICODE_STRING BusDeviceName;
    FAST_MUTEX DiskListLock;
    LIST_ENTRY DiskList;
} RAMDISK_EXTENSION, *PRAMDISK_EXTENSION;

typedef struct _RAMDISK_BUS_EXTENSION
{
    RAMDISK_EXTENSION;
} RAMDISK_BUS_EXTENSION, *PRAMDISK_BUS_EXTENSION;

typedef struct _RAMDISK_DRIVE_EXTENSION
{
    /* Inherited base class */
    RAMDISK_EXTENSION;

    /* Data we get from the creator */
    GUID DiskGuid;
    UNICODE_STRING GuidString;
    UNICODE_STRING SymbolicLinkName;
    ULONG DiskType;
    RAMDISK_CREATE_OPTIONS DiskOptions;
    LARGE_INTEGER DiskLength;
    LONG DiskOffset;
    WCHAR DriveLetter;
    ULONG BasePage;

    /* Data we get from the disk */
    ULONG BytesPerSector;
    ULONG SectorsPerTrack;
    ULONG NumberOfHeads;
    ULONG Cylinders;
    ULONG HiddenSectors;
} RAMDISK_DRIVE_EXTENSION, *PRAMDISK_DRIVE_EXTENSION;

ULONG MaximumViewLength;
ULONG MaximumPerDiskViewLength;
ULONG ReportDetectedDevice;
ULONG MarkRamdisksAsRemovable;
ULONG MinimumViewCount;
ULONG DefaultViewCount;
ULONG MaximumViewCount;
ULONG MinimumViewLength;
ULONG DefaultViewLength;
UNICODE_STRING DriverRegistryPath;
BOOLEAN ExportBootDiskAsCd;
BOOLEAN IsWinPEBoot;
PDEVICE_OBJECT RamdiskBusFdo;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
QueryParameters(IN PUNICODE_STRING RegistryPath)
{
    ULONG MinView, DefView, MinViewLength, DefViewLength, MaxViewLength;
    RTL_QUERY_REGISTRY_TABLE QueryTable[11];

    /* Set defaults */
    MaximumViewLength = 0x10000000u;
    MaximumPerDiskViewLength = 0x10000000u;
    ReportDetectedDevice = 0;
    MarkRamdisksAsRemovable = 0;
    MinimumViewCount = 2;
    DefaultViewCount = 16;
    MaximumViewCount = 64;
    MinimumViewLength = 0x10000u;
    DefaultViewLength = 0x100000u;

    /* Setup the query table and query the registry */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = 1;
    QueryTable[0].Name = L"Parameters";
    QueryTable[1].Flags = 32;
    QueryTable[1].Name = L"ReportDetectedDevice";
    QueryTable[1].EntryContext = &ReportDetectedDevice;
    QueryTable[2].Flags = 32;
    QueryTable[2].Name = L"MarkRamdisksAsRemovable";
    QueryTable[2].EntryContext = &MarkRamdisksAsRemovable;
    QueryTable[3].Flags = 32;
    QueryTable[3].Name = L"MinimumViewCount";
    QueryTable[3].EntryContext = &MinimumViewCount;
    QueryTable[4].Flags = 32;
    QueryTable[4].Name = L"DefaultViewCount";
    QueryTable[4].EntryContext = &DefaultViewCount;
    QueryTable[5].Flags = 32;
    QueryTable[5].Name = L"MaximumViewCount";
    QueryTable[5].EntryContext = &MaximumViewCount;
    QueryTable[6].Flags = 32;
    QueryTable[6].Name = L"MinimumViewLength";
    QueryTable[6].EntryContext = &MinimumViewLength;
    QueryTable[7].Flags = 32;
    QueryTable[7].Name = L"DefaultViewLength";
    QueryTable[7].EntryContext = &DefaultViewLength;
    QueryTable[8].Flags = 32;
    QueryTable[8].Name = L"MaximumViewLength";
    QueryTable[8].EntryContext = &MaximumViewLength;
    QueryTable[9].Flags = 32;
    QueryTable[9].Name = L"MaximumPerDiskViewLength";
    QueryTable[9].EntryContext = &MaximumPerDiskViewLength;
    RtlQueryRegistryValues(RTL_REGISTRY_OPTIONAL,
                           RegistryPath->Buffer,
                           QueryTable,
                           NULL,
                           NULL);

    /* Parse minimum view count, cannot be bigger than 256 or smaller than 2 */
    MinView = MinimumViewCount;
    if (MinimumViewCount >= 2)
    {
        if (MinimumViewCount > 256) MinView = 256;
    }
    else
    {
        MinView = 2;
    }
    MinimumViewCount = MinView;

    /* Parse default view count, cannot be bigger than 256 or smaller than minimum */
    DefView = DefaultViewCount;
    if (DefaultViewCount >= MinView)
    {
        if (DefaultViewCount > 256) DefView = 256;
    }
    else
    {
        DefView = MinView;
    }
    DefaultViewCount = DefView;

    /* Parse maximum view count, cannot be bigger than 256 or smaller than default */
    if (MaximumViewCount >= DefView)
    {
        if (MaximumViewCount > 256) MaximumViewCount = 256;
    }
    else
    {
        MaximumViewCount = DefView;
    }

    /* Parse minimum view length, cannot be bigger than 1GB or smaller than 64KB */
    MinViewLength = MinimumViewLength;
    if (MinimumViewLength >= 0x10000)
    {
        if (MinimumViewLength > 0x40000000) MinViewLength = 0x40000000u;
    }
    else
    {
        MinViewLength = 0x10000u;
    }
    MinimumViewLength = MinViewLength;

    /* Parse default view length, cannot be bigger than 1GB or smaller than minimum */
    DefViewLength = DefaultViewLength;
    if (DefaultViewLength >= MinViewLength)
    {
        if (DefaultViewLength > 0x40000000) DefViewLength = 0x40000000u;
    }
    else
    {
        DefViewLength = MinViewLength;
    }
    DefaultViewLength = DefViewLength;

    /* Parse maximum view length, cannot be bigger than 1GB or smaller than default */
    MaxViewLength = MaximumViewLength;
    if (MaximumViewLength >= DefViewLength)
    {
        if (MaximumViewLength > 0x40000000) MaxViewLength = 0x40000000u;
    }
    else
    {
        MaxViewLength = DefViewLength;
    }
    MaximumViewLength = MaxViewLength;

    /* Parse maximum view length per disk, cannot be smaller than 16MB */
    if (MaximumPerDiskViewLength >= 0x1000000)
    {
        if (MaxViewLength > 0xFFFFFFFF) MaximumPerDiskViewLength = -1;
    }
    else
    {
        MaximumPerDiskViewLength = 0x1000000u;
    }
}

PVOID
NTAPI
RamdiskMapPages(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                IN LARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PULONG OutputLength)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID MappedBase;
    ULONG PageOffset;
    SIZE_T ActualLength;
    LARGE_INTEGER ActualOffset;
    LARGE_INTEGER ActualPages;

    /* We only support boot disks for now */
    ASSERT(DeviceExtension->DiskType == RAMDISK_BOOT_DISK);

    /* Calculate the actual offset in the drive */
    ActualOffset.QuadPart = DeviceExtension->DiskOffset + Offset.QuadPart;

    /* Convert to pages */
    ActualPages.QuadPart = ActualOffset.QuadPart >> PAGE_SHIFT;

    /* Now add the base page */
    ActualPages.QuadPart = DeviceExtension->BasePage + ActualPages.QuadPart;

    /* Calculate final amount of bytes */
    PhysicalAddress.QuadPart = ActualPages.QuadPart << PAGE_SHIFT;

    /* Calculate pages spanned for the mapping */
    ActualLength = ADDRESS_AND_SIZE_TO_SPAN_PAGES(ActualOffset.QuadPart, Length);

    /* And convert this back to bytes */
    ActualLength <<= PAGE_SHIFT;

    /* Get the offset within the page */
    PageOffset = BYTE_OFFSET(ActualOffset.QuadPart);

    /* Map the I/O Space from the loader */
    MappedBase = MmMapIoSpace(PhysicalAddress, ActualLength, MmCached);

    /* Return actual offset within the page as well as the length */
    if (MappedBase) MappedBase = (PVOID)((ULONG_PTR)MappedBase + PageOffset);
    *OutputLength = Length;
    return MappedBase;
}

VOID
NTAPI
RamdiskUnmapPages(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                  IN PVOID BaseAddress,
                  IN LARGE_INTEGER Offset,
                  IN ULONG Length)
{
    LARGE_INTEGER ActualOffset;
    SIZE_T ActualLength;
    ULONG PageOffset;

    /* We only support boot disks for now */
    ASSERT(DeviceExtension->DiskType == RAMDISK_BOOT_DISK);

    /* Calculate the actual offset in the drive */
    ActualOffset.QuadPart = DeviceExtension->DiskOffset + Offset.QuadPart;

    /* Calculate pages spanned for the mapping */
    ActualLength = ADDRESS_AND_SIZE_TO_SPAN_PAGES(ActualOffset.QuadPart, Length);

    /* And convert this back to bytes */
    ActualLength <<= PAGE_SHIFT;

    /* Get the offset within the page */
    PageOffset = BYTE_OFFSET(ActualOffset.QuadPart);

    /* Calculate actual base address where we mapped this */
    BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - PageOffset);

    /* Unmap the I/O space we got from the loader */
    MmUnmapIoSpace(BaseAddress, ActualLength);
}

NTSTATUS
NTAPI
RamdiskCreateDiskDevice(IN PRAMDISK_BUS_EXTENSION DeviceExtension,
                        IN PRAMDISK_CREATE_INPUT Input,
                        IN BOOLEAN ValidateOnly,
                        OUT PRAMDISK_DRIVE_EXTENSION *NewDriveExtension)
{
    ULONG BasePage, DiskType, Length;
    //ULONG ViewCount;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    PVOID Buffer;
    WCHAR LocalBuffer[16];
    UNICODE_STRING SymbolicLinkName, DriveString, GuidString, DeviceName;
    PPACKED_BOOT_SECTOR BootSector;
    BIOS_PARAMETER_BLOCK BiosBlock;
    ULONG BytesPerSector, SectorsPerTrack, Heads, BytesRead;
    PVOID BaseAddress;
    LARGE_INTEGER CurrentOffset, CylinderSize, DiskLength;
    ULONG CylinderCount, SizeByCylinders;

    /* Check if we're a boot RAM disk */
    DiskType = Input->DiskType;
    if (DiskType >= RAMDISK_BOOT_DISK)
    {
        /* Check if we're an ISO */
        if (DiskType == RAMDISK_BOOT_DISK)
        {
            /* NTLDR mounted us somewhere */
            BasePage = Input->BasePage;
            if (!BasePage) return STATUS_INVALID_PARAMETER;

            /* Sanitize disk options */
            Input->Options.Fixed = TRUE;
            Input->Options.Readonly = Input->Options.ExportAsCd |
                                      Input->Options.Readonly;
            Input->Options.Hidden = FALSE;
            Input->Options.NoDosDevice = FALSE;
            Input->Options.NoDriveLetter = IsWinPEBoot ? TRUE : FALSE;
        }
        else
        {
            /* The only other possibility is a WIM disk */
            if (DiskType != RAMDISK_WIM_DISK)
            {
                /* Fail */
                return STATUS_INVALID_PARAMETER;
            }

            /* Read the view count instead */
            // ViewCount = Input->ViewCount;

            /* Sanitize disk options */
            Input->Options.Hidden = FALSE;
            Input->Options.NoDosDevice = FALSE;
            Input->Options.Readonly = FALSE;
            Input->Options.NoDriveLetter = TRUE;
            Input->Options.Fixed = TRUE;
        }

        /* Are we just validating and returning to the user? */
        if (ValidateOnly) return STATUS_SUCCESS;

        /* Build the GUID string */
        Status = RtlStringFromGUID(&Input->DiskGuid, &GuidString);
        if (!(NT_SUCCESS(Status)) || !(GuidString.Buffer))
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FailCreate;
        }

        /* Allocate our device name */
        Length = GuidString.Length + 32;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'dmaR');
        if (!Buffer)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FailCreate;
        }

        /* Build the device name string */
        DeviceName.Buffer = Buffer;
        DeviceName.Length = Length - 2;
        DeviceName.MaximumLength = Length;
        wcsncpy(Buffer, L"\\Device\\Ramdisk", Length / sizeof(WCHAR));
        wcsncat(Buffer, GuidString.Buffer, Length / sizeof(WCHAR));

        /* Create the drive device */
        Status = IoCreateDevice(DeviceExtension->DeviceObject->DriverObject,
                                sizeof(RAMDISK_DRIVE_EXTENSION),
                                &DeviceName,
                                (Input->Options.ExportAsCd) ?
                                FILE_DEVICE_CD_ROM : FILE_DEVICE_DISK,
                                0,
                                0,
                                &DeviceObject);
        if (!NT_SUCCESS(Status)) goto FailCreate;

        /* Grab the drive extension */
        DriveExtension = DeviceObject->DeviceExtension;

        /* Check if we need a DOS device */
        if (!Input->Options.NoDosDevice)
        {
            /* Build the symbolic link name */
            SymbolicLinkName.MaximumLength = GuidString.Length + 36;
            SymbolicLinkName.Length = GuidString.Length + 34;
            Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                           SymbolicLinkName.MaximumLength,
                                           'dmaR');
            SymbolicLinkName.Buffer = Buffer;
            if (Buffer)
            {
                /* Create it */
                wcsncpy(Buffer,
                        L"\\GLOBAL??\\Ramdisk",
                        SymbolicLinkName.MaximumLength / sizeof(WCHAR));
                wcsncat(Buffer,
                        GuidString.Buffer,
                        SymbolicLinkName.MaximumLength / sizeof(WCHAR));
                Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
                if (!NT_SUCCESS(Status))
                {
                    /* Nevermind... */
                    Input->Options.NoDosDevice = TRUE;
                    ExFreePool(Buffer);
                    SymbolicLinkName.Buffer = NULL;
                }
            }
            else
            {
                /* No DOS device */
                Input->Options.NoDosDevice = TRUE;
            }

            /* Is this an ISO boot ramdisk? */
            if (Input->DiskType == RAMDISK_BOOT_DISK)
            {
                /* Does it need a drive letter? */
                if (!Input->Options.NoDriveLetter)
                {
                    /* Build it and take over the existing symbolic link */
                    _snwprintf(LocalBuffer,
                               30,
                               L"\\DosDevices\\%wc:",
                               Input->DriveLetter);
                    RtlInitUnicodeString(&DriveString, LocalBuffer);
                    IoDeleteSymbolicLink(&DriveString);
                    IoCreateSymbolicLink(&DriveString, &DeviceName);

                    /* Save the drive letter */
                    DriveExtension->DriveLetter = Input->DriveLetter;
                }
            }

        }

        /* Setup the device object flags */
        DeviceObject->Flags |= (DO_XIP | DO_POWER_PAGABLE | DO_DIRECT_IO);
        DeviceObject->AlignmentRequirement = 1;

        /* Build the drive FDO */
        *NewDriveExtension = DriveExtension;
        DriveExtension->Type = RamdiskDrive;
        DiskLength = Input->DiskLength;
        ExInitializeFastMutex(&DriveExtension->DiskListLock);
        IoInitializeRemoveLock(&DriveExtension->RemoveLock, 'dmaR', 1, 0);
        DriveExtension->DriveDeviceName = DeviceName;
        DriveExtension->SymbolicLinkName = SymbolicLinkName;
        DriveExtension->GuidString = GuidString;
        DriveExtension->DiskGuid = Input->DiskGuid;
        DriveExtension->PhysicalDeviceObject = DeviceObject;
        DriveExtension->DeviceObject = RamdiskBusFdo;
        DriveExtension->AttachedDevice = RamdiskBusFdo;
        DriveExtension->DiskType = Input->DiskType;
        DriveExtension->DiskOptions = Input->Options;
        DriveExtension->DiskLength = DiskLength;
        DriveExtension->DiskOffset = Input->DiskOffset;
        DriveExtension->BasePage = Input->BasePage;
        DriveExtension->BytesPerSector = 0;
        DriveExtension->SectorsPerTrack = 0;
        DriveExtension->NumberOfHeads = 0;

        /* Make sure we don't free it later */
        DeviceName.Buffer = NULL;
        SymbolicLinkName.Buffer = NULL;
        GuidString.Buffer = NULL;

        /* Check if this is a boot disk, or a registry ram drive */
        if (!(Input->Options.ExportAsCd) &&
            (Input->DiskType == RAMDISK_BOOT_DISK))
        {
            /* Not an ISO boot, but it's a boot FS -- map it to figure out the
             * drive settings */
            CurrentOffset.QuadPart = 0;
            BaseAddress = RamdiskMapPages(DriveExtension,
                                          CurrentOffset,
                                          PAGE_SIZE,
                                          &BytesRead);
            if (BaseAddress)
            {
                /* Get the data */
                BootSector = (PPACKED_BOOT_SECTOR)BaseAddress;
                FatUnpackBios(&BiosBlock, &BootSector->PackedBpb);
                BytesPerSector = BiosBlock.BytesPerSector;
                SectorsPerTrack = BiosBlock.SectorsPerTrack;
                Heads = BiosBlock.Heads;

                /* Save it */
                DriveExtension->BytesPerSector = BytesPerSector;
                DriveExtension->SectorsPerTrack = SectorsPerTrack;
                DriveExtension->NumberOfHeads = Heads;

                /* Unmap now */
                CurrentOffset.QuadPart = 0;
                RamdiskUnmapPages(DriveExtension,
                                  BaseAddress,
                                  CurrentOffset,
                                  BytesRead);
            }
            else
            {
                /* Fail */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto FailCreate;
            }
        }

        /* Check if the drive settings haven't been set yet */
        if ((DriveExtension->BytesPerSector == 0) ||
            (DriveExtension->SectorsPerTrack == 0) ||
            (DriveExtension->NumberOfHeads == 0))
        {
            /* Check if this is a CD */
            if (Input->Options.ExportAsCd)
            {
                /* Setup partition parameters default for ISO 9660 */
                DriveExtension->BytesPerSector = 2048;
                DriveExtension->SectorsPerTrack = 32;
                DriveExtension->NumberOfHeads = 64;
            }
            else
            {
                /* Setup partition parameters default for FAT */
                DriveExtension->BytesPerSector = 512;
                DriveExtension->SectorsPerTrack = 128;
                DriveExtension->NumberOfHeads = 16;
            }
        }

        /* Calculate the cylinder size */
        CylinderSize.QuadPart = DriveExtension->BytesPerSector *
                                DriveExtension->SectorsPerTrack *
                                DriveExtension->NumberOfHeads;
        CylinderCount = DiskLength.QuadPart / CylinderSize.QuadPart;
        SizeByCylinders = CylinderSize.QuadPart * CylinderCount;
        DriveExtension->Cylinders = CylinderCount;
        if ((DiskLength.HighPart > 0) || (SizeByCylinders < DiskLength.LowPart))
        {
            /* Align cylinder size up */
            DriveExtension->Cylinders++;
        }

        /* Acquire the disk lock */
        KeEnterCriticalRegion();
        ExAcquireFastMutex(&DeviceExtension->DiskListLock);

        /* Insert us */
        InsertTailList(&DeviceExtension->DiskList, &DriveExtension->DiskList);

        /* Release the lock */
        ExReleaseFastMutex(&DeviceExtension->DiskListLock);
        KeLeaveCriticalRegion();

        /* Clear init flag */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }

FailCreate:
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskCreateRamdisk(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN BOOLEAN ValidateOnly)
{
    PRAMDISK_CREATE_INPUT Input;
    ULONG Length;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    ULONG DiskType;
    PWCHAR FileNameStart, FileNameEnd;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Get the device extension and our input data */
    DeviceExtension = DeviceObject->DeviceExtension;
    Length = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    Input = (PRAMDISK_CREATE_INPUT)Irp->AssociatedIrp.SystemBuffer;

    /* Validate input parameters */
    if ((Length < sizeof(RAMDISK_CREATE_INPUT)) ||
        (Input->Version != sizeof(RAMDISK_CREATE_INPUT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the disk type */
    DiskType = Input->DiskType;
    if (DiskType == RAMDISK_WIM_DISK) return STATUS_INVALID_PARAMETER;

    /* Look at the disk type */
    if (DiskType == RAMDISK_BOOT_DISK)
    {
        /* We only allow this as an early-init boot */
        if (!KeLoaderBlock) return STATUS_INVALID_PARAMETER;

        /* Save command-line flags */
        if (ExportBootDiskAsCd) Input->Options.ExportAsCd = TRUE;
        if (IsWinPEBoot) Input->Options.NoDriveLetter = TRUE;
    }

    /* Validate the disk type */
    if ((Input->Options.ExportAsCd) && (DiskType != RAMDISK_BOOT_DISK))
    {
        /* If the type isn't CDFS, it has to at least be raw CD */
        if (DiskType != RAMDISK_MEMORY_MAPPED_DISK) return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is an actual file */
    if (DiskType <= RAMDISK_MEMORY_MAPPED_DISK)
    {
        /* Validate the file name */
        FileNameStart = (PWCHAR)((ULONG_PTR)Input + Length);
        FileNameEnd = Input->FileName + 1;
        while ((FileNameEnd < FileNameStart) && *(FileNameEnd)) FileNameEnd++;
        if (FileNameEnd == FileNameStart) return STATUS_INVALID_PARAMETER;
    }

    /* Create the actual device */
    Status = RamdiskCreateDiskDevice(DeviceExtension,
                                     Input,
                                     ValidateOnly,
                                     &DriveExtension);
    if (NT_SUCCESS(Status))
    {
        /* Invalidate and set success */
        IoInvalidateDeviceRelations(DeviceExtension->PhysicalDeviceObject, 0);
        Irp->IoStatus.Information = STATUS_SUCCESS;
    }

    /* We are done */
    return Status;
}

NTSTATUS
NTAPI
RamdiskGetPartitionInfo(IN PIRP Irp,
                        IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PPARTITION_INFORMATION PartitionInfo;
    PVOID BaseAddress;
    LARGE_INTEGER Zero = {{0, 0}};
    ULONG Length;
    PIO_STACK_LOCATION IoStackLocation;

    /* Validate the length */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.
        OutputBufferLength < sizeof(PARTITION_INFORMATION))
    {
        /* Invalid length */
        Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        return Status;
    }

    /* Map the partition table */
    BaseAddress = RamdiskMapPages(DeviceExtension, Zero, PAGE_SIZE, &Length);
    if (!BaseAddress)
    {
        /* No memory */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        return Status;
    }

    /* Fill out the information */
    PartitionInfo = Irp->AssociatedIrp.SystemBuffer;
    PartitionInfo->StartingOffset.QuadPart = DeviceExtension->BytesPerSector;
    PartitionInfo->PartitionLength.QuadPart = DeviceExtension->BytesPerSector *
                                              DeviceExtension->SectorsPerTrack *
                                              DeviceExtension->NumberOfHeads *
                                              DeviceExtension->Cylinders;
    PartitionInfo->HiddenSectors = DeviceExtension->HiddenSectors;
    PartitionInfo->PartitionNumber = 0;
    PartitionInfo->PartitionType = *((PCHAR)BaseAddress + 450);
    PartitionInfo->BootIndicator = (DeviceExtension->DiskType ==
                                    RAMDISK_BOOT_DISK) ? TRUE: FALSE;
    PartitionInfo->RecognizedPartition = IsRecognizedPartition(PartitionInfo->
                                                               PartitionType);
    PartitionInfo->RewritePartition = FALSE;

    /* Unmap the partition table */
    RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, Length);

    /* Done */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskSetPartitionInfo(IN PIRP Irp,
                        IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    ULONG BytesRead;
    NTSTATUS Status;
    PVOID BaseAddress;
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER Zero = {{0, 0}};
    PPARTITION_INFORMATION PartitionInfo;

    /* First validate input */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PARTITION_INFORMATION))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto SetAndQuit;
    }

    /* Map to get MBR */
    BaseAddress = RamdiskMapPages(DeviceExtension, Zero, PAGE_SIZE, &BytesRead);
    if (BaseAddress == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SetAndQuit;
    }

    /* Set the new partition type on partition 0, field system indicator */
    PartitionInfo = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
    *((PCHAR)BaseAddress + 450) = PartitionInfo->PartitionType;

    /* And unmap */
    RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, BytesRead);
    Status = STATUS_SUCCESS;

SetAndQuit:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    return Status;
}

VOID
NTAPI
RamdiskWorkerThread(IN PDEVICE_OBJECT DeviceObject,
                    IN PVOID Context)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PIRP Irp = Context;

    /* Get the stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Free the work item */
    IoFreeWorkItem(Irp->Tail.Overlay.DriverContext[0]);

    /* Grab the device extension and lock it */
    DeviceExtension = DeviceObject->DeviceExtension;
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (NT_SUCCESS(Status))
    {
        /* Discriminate by major code */
        switch (IoStackLocation->MajorFunction)
        {
            /* Device control */
            case IRP_MJ_DEVICE_CONTROL:
            {
                /* Let's take a look at the IOCTL */
                switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
                {
                    /* Ramdisk create request */
                    case FSCTL_CREATE_RAM_DISK:
                    {
                        /* This time we'll do it for real */
                        Status = RamdiskCreateRamdisk(DeviceObject, Irp, FALSE);
                        break;
                    }

                    case IOCTL_DISK_SET_PARTITION_INFO:
                    {
                        Status = RamdiskSetPartitionInfo(Irp, (PRAMDISK_DRIVE_EXTENSION)DeviceExtension);
                        break;
                    }

                    case IOCTL_DISK_GET_DRIVE_LAYOUT:
                        UNIMPLEMENTED_DBGBREAK("Get drive layout request\n");
                        break;

                    case IOCTL_DISK_GET_PARTITION_INFO:
                    {
                        Status = RamdiskGetPartitionInfo(Irp, (PRAMDISK_DRIVE_EXTENSION)DeviceExtension);
                        break;
                    }

                    default:
                        UNIMPLEMENTED_DBGBREAK("Invalid request\n");
                        break;
                }

                /* We're here */
                break;
            }

            /* Read or write request */
            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
                UNIMPLEMENTED_DBGBREAK("Read/Write request\n");
                break;

            /* Internal request (SCSI?) */
            case IRP_MJ_INTERNAL_DEVICE_CONTROL:
                UNIMPLEMENTED_DBGBREAK("SCSI request\n");
                break;

            /* Flush request */
            case IRP_MJ_FLUSH_BUFFERS:
                UNIMPLEMENTED_DBGBREAK("Flush request\n");
                break;

            /* Anything else */
            default:
                UNIMPLEMENTED_DBGBREAK("Invalid request: %lx\n",
                                       IoStackLocation->MajorFunction);
                break;
        }

        /* Complete the I/O */
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return;
    }

    /* Fail the I/O */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
SendIrpToThread(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIO_WORKITEM WorkItem;

    /* Mark the IRP pending */
    IoMarkIrpPending(Irp);

    /* Allocate a work item */
    WorkItem = IoAllocateWorkItem(DeviceObject);
    if (WorkItem)
    {
        /* Queue it up */
        Irp->Tail.Overlay.DriverContext[0] = WorkItem;
        IoQueueWorkItem(WorkItem, RamdiskWorkerThread, DelayedWorkQueue, Irp);
        return STATUS_PENDING;
    }
    else
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
NTAPI
RamdiskReadWriteReal(IN PIRP Irp,
                     IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    PMDL Mdl;
    PVOID CurrentBase, SystemVa, BaseAddress;
    PIO_STACK_LOCATION IoStackLocation;
    LARGE_INTEGER CurrentOffset;
    ULONG BytesRead, BytesLeft, CopyLength;
    PVOID Source, Destination;
    NTSTATUS Status;

    /* Get the MDL and check if it's mapped */
    Mdl = Irp->MdlAddress;
    if (Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))
    {
        /* Use the mapped address */
        SystemVa = Mdl->MappedSystemVa;
    }
    else
    {
        /* Map it ourselves */
        SystemVa = MmMapLockedPagesSpecifyCache(Mdl,
                                                0,
                                                MmCached,
                                                NULL,
                                                0,
                                                NormalPagePriority);
    }

    /* Make sure we were able to map it */
    CurrentBase = SystemVa;
    if (!SystemVa) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize default */
    Irp->IoStatus.Information = 0;

    /* Get the I/O Stack Location and capture the data */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    CurrentOffset = IoStackLocation->Parameters.Read.ByteOffset;
    BytesLeft = IoStackLocation->Parameters.Read.Length;
    if (!BytesLeft) return STATUS_INVALID_PARAMETER;

    /* Do the copy loop */
    while (TRUE)
    {
        /* Map the pages */
        BaseAddress = RamdiskMapPages(DeviceExtension,
                                      CurrentOffset,
                                      BytesLeft,
                                      &BytesRead);
        if (!BaseAddress) return STATUS_INSUFFICIENT_RESOURCES;

        /* Update our lengths */
        Irp->IoStatus.Information += BytesRead;
        CopyLength = BytesRead;

        /* Check if this was a read or write */
        Status = STATUS_SUCCESS;
        if (IoStackLocation->MajorFunction == IRP_MJ_READ)
        {
            /* Set our copy parameters */
            Destination = CurrentBase;
            Source = BaseAddress;
            goto DoCopy;
        }
        else if (IoStackLocation->MajorFunction == IRP_MJ_WRITE)
        {
            /* Set our copy parameters */
            Destination = BaseAddress;
            Source = CurrentBase;
DoCopy:
            /* Copy the data */
            RtlCopyMemory(Destination, Source, CopyLength);
        }
        else
        {
            /* Prepare us for failure */
            BytesLeft = CopyLength;
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Unmap the pages */
        RamdiskUnmapPages(DeviceExtension, BaseAddress, CurrentOffset, BytesRead);

        /* Update offset and bytes left */
        BytesLeft -= BytesRead;
        CurrentOffset.QuadPart += BytesRead;
        CurrentBase = (PVOID)((ULONG_PTR)CurrentBase + BytesRead);

        /* Check if we are done */
        if (!BytesLeft) return Status;
    }
}

NTSTATUS
NTAPI
RamdiskOpenClose(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    /* Complete the IRP */
    Irp->IoStatus.Information = 1;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;
    // ULONG Length;
    // LARGE_INTEGER ByteOffset;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status, ReturnStatus;

    /* Get the device extension and make sure this isn't a bus */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Fail */
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }

    /* Capture parameters */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    // Length = IoStackLocation->Parameters.Read.Length;
    // ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

    /* FIXME: Validate offset */

    /* FIXME: Validate sector */

    /* Validate write */
    if ((IoStackLocation->MajorFunction == IRP_MJ_WRITE) &&
        (DeviceExtension->DiskOptions.Readonly))
    {
        /* Fail, this is read-only */
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto Complete;
    }

    /* See if we want to do this sync or async */
    if (DeviceExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
    {
        /* Do it sync */
        Status = RamdiskReadWriteReal(Irp, DeviceExtension);
        goto Complete;
    }

    /* Queue it to the worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    ReturnStatus = STATUS_PENDING;

    /* Check if we're pending or not */
    if (Status != STATUS_PENDING)
    {
Complete:
        /* Complete the IRP */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        ReturnStatus = Status;
    }

    /* Return to caller */
    return ReturnStatus;
}

NTSTATUS
NTAPI
RamdiskDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PRAMDISK_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension = (PVOID)DeviceExtension;
    ULONG Information;
    PCDROM_TOC Toc;
    PDISK_GEOMETRY DiskGeometry;

    /* Grab the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* Fail the IRP */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Setup some defaults */
    Status = STATUS_INVALID_DEVICE_REQUEST;
    Information = 0;

    /* Check if this is an bus device or the drive */
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Check what the request is */
        switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
        {
            /* Request to create a ramdisk */
            case FSCTL_CREATE_RAM_DISK:
            {
                /* Do it */
                Status = RamdiskCreateRamdisk(DeviceObject, Irp, TRUE);
                if (!NT_SUCCESS(Status)) goto CompleteRequest;
                break;
            }

            default:
            {
                /* We don't handle anything else yet */
                UNIMPLEMENTED_DBGBREAK("FSCTL: 0x%lx is UNSUPPORTED!\n",
                                       IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
            }
        }
    }
    else
    {
        /* Check what the request is */
        switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_DISK_CHECK_VERIFY:
            case IOCTL_STORAGE_CHECK_VERIFY:
            case IOCTL_STORAGE_CHECK_VERIFY2:
            case IOCTL_CDROM_CHECK_VERIFY:
            {
                /* Just pretend it's OK, don't do more */
                Status = STATUS_SUCCESS;
                break;
            }

            case IOCTL_STORAGE_GET_MEDIA_TYPES:
            case IOCTL_DISK_GET_MEDIA_TYPES:
            case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(DISK_GEOMETRY))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Fill it out */
                DiskGeometry = Irp->AssociatedIrp.SystemBuffer;
                DiskGeometry->Cylinders.QuadPart = DriveExtension->Cylinders;
                DiskGeometry->BytesPerSector = DriveExtension->BytesPerSector;
                DiskGeometry->SectorsPerTrack = DriveExtension->SectorsPerTrack;
                DiskGeometry->TracksPerCylinder = DriveExtension->NumberOfHeads;
                DiskGeometry->MediaType = DriveExtension->DiskOptions.Fixed ?
                                          FixedMedia : RemovableMedia;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(DISK_GEOMETRY);
                break;
            }

            case IOCTL_CDROM_READ_TOC:
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(CDROM_TOC))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Clear the TOC */
                Toc = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(Toc, sizeof(CDROM_TOC));

                /* Fill it out */
                Toc->Length[0] = 0;
                Toc->Length[1] = RAMDISK_TOC_SIZE - sizeof(Toc->Length);
                Toc->FirstTrack = 1;
                Toc->LastTrack = 1;
                Toc->TrackData[0].Adr = 1;
                Toc->TrackData[0].Control = TOC_DATA_TRACK;
                Toc->TrackData[0].TrackNumber = 1;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = RAMDISK_TOC_SIZE;
                break;
            }

            case IOCTL_DISK_SET_PARTITION_INFO:
            {
                Status = RamdiskSetPartitionInfo(Irp, DriveExtension);
                break;
            }

            case IOCTL_DISK_GET_PARTITION_INFO:
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(PARTITION_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Check if we need to do this sync or async */
                if (DriveExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
                {
                    /* Call the helper function */
                    Status = RamdiskGetPartitionInfo(Irp, DriveExtension);
                }
                else
                {
                    /* Do it asynchronously later */
                    goto CallWorker;
                }

                /* We are done */
                Information = Irp->IoStatus.Information;
                break;
            }

            case IOCTL_DISK_GET_LENGTH_INFO:
            {
                PGET_LENGTH_INFORMATION LengthInformation = Irp->AssociatedIrp.SystemBuffer;

                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Fill it out */
                LengthInformation->Length = DriveExtension->DiskLength;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(GET_LENGTH_INFORMATION);
                break;
            }
            case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
            {
                PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION GptInformation;

                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Fill it out */
                GptInformation = Irp->AssociatedIrp.SystemBuffer;
                GptInformation->GptAttributes = 0;

                /* Translate the Attributes */
                if (DriveExtension->DiskOptions.Readonly)
                    GptInformation->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;
                if (DriveExtension->DiskOptions.Hidden)
                    GptInformation->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
                if (DriveExtension->DiskOptions.NoDriveLetter)
                    GptInformation->GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);
                break;
            }

            case IOCTL_DISK_GET_DRIVE_LAYOUT:
            case IOCTL_DISK_IS_WRITABLE:
            case IOCTL_SCSI_MINIPORT:
            case IOCTL_STORAGE_QUERY_PROPERTY:
            case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
            case IOCTL_VOLUME_SET_GPT_ATTRIBUTES:
            case IOCTL_VOLUME_OFFLINE:
            {
                UNIMPLEMENTED_DBGBREAK("IOCTL: 0x%lx is UNIMPLEMENTED!\n",
                                       IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
                break;
            }

            default:
            {
                /* Drive code not emulated */
                DPRINT1("IOCTL: 0x%lx is UNSUPPORTED!\n",
                        IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
                break;
            }
        }

        /* If requests drop down here, we just return them complete them */
        goto CompleteRequest;
    }

    /* Queue the request to our worker thread */
CallWorker:
    Status = SendIrpToThread(DeviceObject, Irp);

CompleteRequest:
    /* Release the lock */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (Status != STATUS_PENDING)
    {
        /* Complete the request */
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = Information;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryDeviceRelations(IN DEVICE_RELATION_TYPE Type,
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    PDEVICE_RELATIONS DeviceRelations, OurDeviceRelations;
    ULONG Count, DiskCount, FinalCount;
    PLIST_ENTRY ListHead, NextEntry;
    PDEVICE_OBJECT* DriveDeviceObject;
    RAMDISK_DEVICE_STATE State;

    /* Get the device extension and check if this is a drive */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Type == RamdiskDrive)
    {
        NTSTATUS Status;
        PDEVICE_RELATIONS DeviceRelations;

        /* We're a child device, only handle target device relations */
        if (Type != TargetDeviceRelation)
        {
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* Allocate a buffer big enough to contain only one DO */
        DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(*DeviceRelations),
                                                'dmaR');
        if (DeviceRelations != NULL)
        {
            /* Reference the DO and add it to the buffer */
            ObReferenceObject(DeviceObject);
            DeviceRelations->Objects[0] = DeviceObject;
            DeviceRelations->Count = 1;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Return our processing & complete */
        Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* We don't handle anything but bus relations */
    if (Type != BusRelations) goto PassToNext;

    /* Acquire the disk list lock */
    KeEnterCriticalRegion();
    ExAcquireFastMutex(&DeviceExtension->DiskListLock);

    /* Did a device already fill relations? */
    DeviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    if (DeviceRelations)
    {
        /* Use the data */
        Count = DeviceRelations->Count;
    }
    else
    {
        /* We're the first */
        Count = 0;
    }

    /* Now loop our drives */
    DiskCount = 0;
    ListHead = &DeviceExtension->DiskList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* As long as it wasn't removed, count it in */
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);
        if (DriveExtension->State < RamdiskStateBusRemoved) DiskCount++;

        /* Move to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Now we know our final count */
    FinalCount = Count + DiskCount;

    /* Allocate the structure */
    OurDeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                               FIELD_OFFSET(DEVICE_RELATIONS,
                                                            Objects) +
                                               FinalCount *
                                               sizeof(PDEVICE_OBJECT),
                                               'dmaR');
    if (!OurDeviceRelations)
    {
        /* Fail */
        ExReleaseFastMutex(&DeviceExtension->DiskListLock);
        KeLeaveCriticalRegion();
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check if we already had some relations */
    if (Count)
    {
        /* Copy them in */
        RtlCopyMemory(OurDeviceRelations->Objects,
                      DeviceRelations->Objects,
                      Count * sizeof(PDEVICE_OBJECT));
    }

    /* Save the count */
    OurDeviceRelations->Count = FinalCount;

    /* Now loop our drives again */
    ListHead = &DeviceExtension->DiskList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Go to the end of the list */
        DriveDeviceObject = &OurDeviceRelations->Objects[Count];

        /* Get the drive state */
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);
        State = DriveExtension->State;

        /* If it was removed or enumerated, we don't touch the device object */
        if (State >= RamdiskStateBusRemoved)
        {
            /* If it was removed, we still have to keep track of this though */
            if (State == RamdiskStateBusRemoved)
            {
                /* Mark it as enumerated now, but don't actually reference it */
                DriveExtension->State = RamdiskStateEnumerated;
            }
        }
        else
        {
            /* First time it's enumerated, reference the device object */
            ObReferenceObject(DriveExtension->DeviceObject);

            /* Save the object pointer and move on */
            *DriveDeviceObject++ = DriveExtension->PhysicalDeviceObject;
        }

        if (DriveExtension->State < RamdiskStateBusRemoved) DiskCount++;

        /* Move to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock */
    ExReleaseFastMutex(&DeviceExtension->DiskListLock);
    KeLeaveCriticalRegion();

    /* Cleanup old relations */
    if (DeviceRelations) ExFreePool(DeviceRelations);

    /* Complete our IRP */
    Irp->IoStatus.Information = (ULONG_PTR)OurDeviceRelations;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Pass to the next driver */
PassToNext:
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(DeviceExtension->AttachedDevice, Irp);
}

NTSTATUS
NTAPI
RamdiskDeleteDiskDevice(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskRemoveBusDevice(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY ListHead, NextEntry;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Acquire disks list lock */
    KeEnterCriticalRegion();
    ExAcquireFastMutex(&DeviceExtension->DiskListLock);

    /* Loop over drives */
    ListHead = &DeviceExtension->DiskList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);

        /* Delete the disk */
        IoAcquireRemoveLock(&DriveExtension->RemoveLock, NULL);
        RamdiskDeleteDiskDevice(DriveExtension->PhysicalDeviceObject, NULL);

        /* RamdiskDeleteDiskDevice releases list lock, so reacquire it */
        KeEnterCriticalRegion();
        ExAcquireFastMutex(&DeviceExtension->DiskListLock);
    }

    /* Release disks list lock */
    ExReleaseFastMutex(&DeviceExtension->DiskListLock);
    KeLeaveCriticalRegion();

    /* Prepare to pass to the lower driver */
    IoSkipCurrentIrpStackLocation(Irp);
    /* Here everything went fine */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Call lower driver */
    Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);

    /* Update state */
    DeviceExtension->State = RamdiskStateBusRemoved;

    /* Release the lock and ensure that everyone has finished its job before
     * we continue. The lock has been acquired by the dispatcher */
    IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, Irp);

    /* If there's a drive name */
    if (DeviceExtension->DriveDeviceName.Buffer)
    {
        /* Inform it's going to be disabled and free the drive name */
        IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, FALSE);
        RtlFreeUnicodeString(&DeviceExtension->DriveDeviceName);
    }

    /* Part from the stack, detach from lower device */
    IoDetachDevice(DeviceExtension->AttachedDevice);

    /* Finally, delete device */
    RamdiskBusFdo = NULL;
    IoDeleteDevice(DeviceObject);

    /* Return status from lower driver */
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryId(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
               IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PWSTR OutputString = NULL;
    ULONG StringLength;

    Status = STATUS_SUCCESS;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Get what is being queried */
    switch (IoStackLocation->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
        {
            /* Allocate a buffer long enough to receive Ramdisk\RamDisk in any case
             * In case we don't have RAMDISK_REGISTRY_DISK, we then need two more
             * chars to store Ramdisk\RamVolume instead */
            StringLength = 4 * (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK) + sizeof(L"Ramdisk\\RamDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"Ramdisk\\", StringLength / sizeof(WCHAR));
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                wcsncat(OutputString, L"RamVolume", StringLength / sizeof(WCHAR));
            }
            else
            {
                wcsncat(OutputString, L"RamDisk", StringLength / sizeof(WCHAR));
            }

            break;
        }

        case BusQueryHardwareIDs:
        {
            /* Allocate a buffer long enough to receive Ramdisk\RamDisk in any case
             * In case we don't have RAMDISK_REGISTRY_DISK, we then need two more
             * chars to store Ramdisk\RamVolume instead
             * We also need an extra char, because it is required that the string
             * is null-terminated twice */
            StringLength = 4 * (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK) +
                           sizeof(UNICODE_NULL) + sizeof(L"Ramdisk\\RamDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"Ramdisk\\", StringLength / sizeof(WCHAR));
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                wcsncat(OutputString, L"RamVolume", StringLength / sizeof(WCHAR));
            }
            else
            {
                wcsncat(OutputString, L"RamDisk", StringLength / sizeof(WCHAR));
            }
            OutputString[(StringLength / sizeof(WCHAR)) - 1] = UNICODE_NULL;

            break;
        }

        case BusQueryCompatibleIDs:
        {
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            StringLength = sizeof(L"GenDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"GenDisk", StringLength / sizeof(WCHAR));
            OutputString[(StringLength / sizeof(WCHAR)) - 1] = UNICODE_NULL;

            break;
        }

        case BusQueryInstanceID:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool,
                                                 DriveExtension->GuidString.MaximumLength,
                                                 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString,
                    DriveExtension->GuidString.Buffer,
                    DriveExtension->GuidString.MaximumLength / sizeof(WCHAR));

            break;
        }

        case BusQueryDeviceSerialNumber:
        case BusQueryContainerID:
        {
            /* Nothing to do */
            break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)OutputString;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryCapabilities(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceCapabilities = IoStackLocation->Parameters.DeviceCapabilities.Capabilities;
    DriveExtension = DeviceObject->DeviceExtension;

    /* Validate our input buffer */
    if (DeviceCapabilities->Version != 1 ||
        DeviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES))
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* And set everything we know about our capabilities */
        DeviceCapabilities->Removable = MarkRamdisksAsRemovable;
        DeviceCapabilities->UniqueID = TRUE;
        DeviceCapabilities->SilentInstall = TRUE;
        DeviceCapabilities->RawDeviceOK = TRUE;
        DeviceCapabilities->SurpriseRemovalOK = (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK);
        DeviceCapabilities->NoDisplayInUI = TRUE;
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryDeviceText(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    DEVICE_TEXT_TYPE DeviceTextType;
    PWSTR OutputString = NULL;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceTextType = IoStackLocation->Parameters.QueryDeviceText.DeviceTextType;
    Status = STATUS_SUCCESS;

    /* Just copy our constants, according to the input */
    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool, sizeof(L"RamDisk"), 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"RamDisk", sizeof(L"RamDisk") / sizeof(WCHAR));

            break;
        }

        case DeviceTextLocationInformation:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool, sizeof(L"RamDisk\\0"), 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"RamDisk\\0", sizeof(L"RamDisk\\0") / sizeof(WCHAR));

            break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)OutputString;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryBusInformation(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PPNP_BUS_INFORMATION PnpBusInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Allocate output memory */
    PnpBusInfo = ExAllocatePoolWithTag(PagedPool, sizeof(*PnpBusInfo), 'dmaR');
    if (PnpBusInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        /* Copy our bus GUID and set our legacy type */
        RtlCopyMemory(&PnpBusInfo->BusTypeGuid, &GUID_BUS_TYPE_RAMDISK, sizeof(GUID));
        PnpBusInfo->LegacyBusType = PNPBus;
        PnpBusInfo->BusNumber = 0;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)PnpBusInfo;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskIoCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp,
                           IN PVOID Context)

{
    /* Just set the event to unlock caller */
    KeSetEvent((PKEVENT)Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
RamdiskPnp(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;
    UCHAR Minor;
    KEVENT Event;

    /* Get the device extension and stack location */
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Minor = IoStackLocation->MinorFunction;

    /* Check if the bus is removed */
    if (DeviceExtension->State == RamdiskStateBusRemoved)
    {
        /* Only remove-device and query-id are allowed */
        if ((Minor != IRP_MN_REMOVE_DEVICE) && (Minor != IRP_MN_QUERY_ID))
        {
            /* Fail anything else */
            Status = STATUS_NO_SUCH_DEVICE;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }

    /* Acquire the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* Fail the IRP */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Query the IRP type */
    switch (Minor)
    {
        case IRP_MN_START_DEVICE:
        {
            if (DeviceExtension->Type == RamdiskDrive)
            {
                ULONG ResultLength;
                DEVICE_INSTALL_STATE InstallState;
                PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceExtension;

                /* If we already have a drive name, free it */
                if (DriveExtension->DriveDeviceName.Buffer)
                {
                    ExFreePool(DriveExtension->DriveDeviceName.Buffer);
                }

                /* Register our device interface */
                if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
                {
                    Status = IoRegisterDeviceInterface(DeviceObject,
                                                       &GUID_DEVINTERFACE_VOLUME,
                                                       NULL,
                                                       &DriveExtension->DriveDeviceName);
                }
                else
                {
                    Status = IoRegisterDeviceInterface(DeviceObject,
                                                       &RamdiskDiskInterface,
                                                       NULL,
                                                       &DriveExtension->DriveDeviceName);
                }

                /* If we were asked not to assign a drive letter or if getting
                 * a name failed, just return saying we're now started */
                if (DriveExtension->DiskOptions.NoDriveLetter ||
                    DriveExtension->DriveDeviceName.Buffer == NULL)
                {
                    DriveExtension->State = RamdiskStateStarted;
                    Irp->IoStatus.Status = Status;
                    break;
                }

                /* Now get our installation state */
                Status = IoGetDeviceProperty(DeviceObject,
                                             DevicePropertyInstallState,
                                             sizeof(InstallState),
                                             &InstallState,
                                             &ResultLength);
                /* If querying the information failed, assume success */
                if (!NT_SUCCESS(Status))
                {
                    InstallState = InstallStateInstalled;
                }

                /* If we were properly installed, then, enable the interface */
                if (InstallState == InstallStateInstalled)
                {
                    Status = IoSetDeviceInterfaceState(&DriveExtension->DriveDeviceName, TRUE);
                }

                /* We're fine & up */
                DriveExtension->State = RamdiskStateStarted;
                Irp->IoStatus.Status = Status;
                break;
            }

            /* Prepare next stack to pass it down */
            IoCopyCurrentIrpStackLocationToNext(Irp);

            /* Initialize our notification event & our completion routine */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoSetCompletionRoutine(Irp, RamdiskIoCompletionRoutine, &Event, TRUE, TRUE, TRUE);

            /* Call lower driver */
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            /* If it succeed to start then enable ourselves and we're up! */
            if (NT_SUCCESS(Status))
            {
                Status = IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, TRUE);
                DeviceExtension->State = RamdiskStateStarted;
            }

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            UNIMPLEMENTED_DBGBREAK("PnP IRP: %lx\n", Minor);
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            /* Remove the proper device */
            if (DeviceExtension->Type == RamdiskBus)
            {
                Status = RamdiskRemoveBusDevice(DeviceObject, Irp);

                /* Return here, lower device has already been called
                 * And remove lock released. This is needed by the function. */
                return Status;
            }
            else
            {
                Status = RamdiskDeleteDiskDevice(DeviceObject, Irp);

                /* Complete the IRP here and return
                 * Here again we don't have to release remove lock
                 * This has already been done by the function. */
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
        }

        case IRP_MN_SURPRISE_REMOVAL:
            UNIMPLEMENTED_DBGBREAK("PnP IRP: %lx\n", Minor);
            break;

        case IRP_MN_QUERY_ID:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                Status = RamdiskQueryId((PRAMDISK_DRIVE_EXTENSION)DeviceExtension, Irp);
            }
            break;
        }

        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                Status = RamdiskQueryBusInformation(DeviceObject, Irp);
            }
            break;
        }

        case IRP_MN_EJECT:
            UNIMPLEMENTED_DBGBREAK("PnP IRP: %lx\n", Minor);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                Status = RamdiskQueryDeviceText((PRAMDISK_DRIVE_EXTENSION)DeviceExtension, Irp);
            }
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            /* Call our main routine */
            Status = RamdiskQueryDeviceRelations(IoStackLocation->
                                                 Parameters.
                                                 QueryDeviceRelations.Type,
                                                 DeviceObject,
                                                 Irp);
            goto ReleaseAndReturn;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                Status = RamdiskQueryCapabilities(DeviceObject, Irp);
            }
            break;
        }

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        {
            /* Complete immediately without touching it */
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto ReleaseAndReturn;
        }

        default:
            DPRINT1("Illegal IRP: %lx\n", Minor);
            break;
    }

    /* Are we the bus? */
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Do we have an attached device? */
        if (DeviceExtension->AttachedDevice)
        {
            /* Forward the IRP */
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
    }

    /* Release the lock and return status */
ReleaseAndReturn:
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    return Status;
}

NTSTATUS
NTAPI
RamdiskPower(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* If we have a device extension, take extra caution with the lower driver */
    if (DeviceExtension != NULL)
    {
        PoStartNextPowerIrp(Irp);

        /* Device has not been removed yet, so pass to the attached/lower driver */
        if (DeviceExtension->State < RamdiskStateBusRemoved)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            return PoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
        /* Otherwise, simply complete the IRP notifying that deletion is pending */
        else
        {
            Irp->IoStatus.Status = STATUS_DELETE_PENDING;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_DELETE_PENDING;
        }
    }

    /* Get stack and deal with minor functions */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStackLocation->MinorFunction)
    {
        case IRP_MN_SET_POWER:
        {
            /* If setting device power state it's all fine and return success */
            if (DevicePowerState)
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }

            /* Get appropriate status for return */
            Status = Irp->IoStatus.Status;
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IRP_MN_QUERY_POWER:
        {
            /* We can obviously accept all states so just return success */
            Status = Irp->IoStatus.Status = STATUS_SUCCESS;
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        default:
        {
            /* Just complete and save status for return */
            Status = Irp->IoStatus.Status;
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskSystemControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* If we have a device extension, forward the IRP to the attached device */
    if (DeviceExtension != NULL)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    /* Otherwise just complete the request
     * And return the status with which we complete it */
    else
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskScsi(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Having a proper device is mandatory */
    if (DeviceExtension->State > RamdiskStateStopped)
    {
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto CompleteIRP;
    }

    /* Acquire the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        goto CompleteIRP;
    }

    /* Queue the IRP for worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    if (Status != STATUS_PENDING)
    {
        goto CompleteIRP;
    }

    /* Release the remove lock */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    goto Quit;

CompleteIRP:
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

Quit:
    return Status;
}

NTSTATUS
NTAPI
RamdiskFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Ensure we have drive extension
     * Only perform flush on disks that have been created
     * from registry entries */
    if (DeviceExtension->Type != RamdiskDrive ||
        DeviceExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    /* Queue the IRP for worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    if (Status != STATUS_PENDING)
    {
        /* Queuing failed - complete the IRP and return failure */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

VOID
NTAPI
RamdiskUnload(IN PDRIVER_OBJECT DriverObject)
{
    /* Just release registry path if previously allocated */
    if (DriverRegistryPath.Buffer)
    {
        ExFreePoolWithTag(DriverRegistryPath.Buffer, 'dmaR');
    }
}

NTSTATUS
NTAPI
RamdiskAddDevice(IN PDRIVER_OBJECT DriverObject,
                 IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;

    /* Only create the bus FDO once */
    if (RamdiskBusFdo) return STATUS_DEVICE_ALREADY_ATTACHED;

    /* Create the bus FDO */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Ramdisk");
    Status = IoCreateDevice(DriverObject,
                            sizeof(RAMDISK_BUS_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            &DeviceObject);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the bus FDO extension */
        DeviceExtension = DeviceObject->DeviceExtension;
        RtlZeroMemory(DeviceExtension, sizeof(*DeviceExtension));

        /* Set bus FDO flags */
        DeviceObject->Flags |= DO_POWER_PAGABLE | DO_DIRECT_IO;

        /* Setup the bus FDO extension */
        DeviceExtension->Type = RamdiskBus;
        ExInitializeFastMutex(&DeviceExtension->DiskListLock);
        IoInitializeRemoveLock(&DeviceExtension->RemoveLock, 'dmaR', 1, 0);
        InitializeListHead(&DeviceExtension->DiskList);
        DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
        DeviceExtension->DeviceObject = DeviceObject;

        /* Register the RAM disk device interface */
        Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                           &RamdiskBusInterface,
                                           NULL,
                                           &DeviceExtension->BusDeviceName);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            IoDeleteDevice(DeviceObject);
            return Status;
        }

        /* Attach us to the device stack */
        AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
                                                     PhysicalDeviceObject);
        DeviceExtension->AttachedDevice = AttachedDevice;
        if (!AttachedDevice)
        {
            /* Fail */
            IoSetDeviceInterfaceState(&DeviceExtension->BusDeviceName, 0);
            RtlFreeUnicodeString(&DeviceExtension->BusDeviceName);
            IoDeleteDevice(DeviceObject);
            return STATUS_NO_SUCH_DEVICE;
        }

        /* Bus FDO is initialized */
        RamdiskBusFdo = DeviceObject;

        /* Loop for loader block */
        if (KeLoaderBlock)
        {
            /* Are we being booted from setup? Not yet supported */
            if (KeLoaderBlock->SetupLdrBlock)
                DPRINT1("FIXME: RamdiskAddDevice is UNSUPPORTED when being started from SETUPLDR!\n");
            // ASSERT(!KeLoaderBlock->SetupLdrBlock);
        }

        /* All done */
        DeviceObject->Flags &= DO_DEVICE_INITIALIZING;
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PCHAR BootDeviceName, CommandLine;
    PDEVICE_OBJECT PhysicalDeviceObject = NULL;
    NTSTATUS Status;
    DPRINT("RAM Disk Driver Initialized\n");

    /* Save the registry path */
    DriverRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    DriverRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      DriverRegistryPath.MaximumLength,
                                                      'dmaR');
    if (!DriverRegistryPath.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyUnicodeString(&DriverRegistryPath, RegistryPath);

    /* Query ramdisk parameters */
    QueryParameters(&DriverRegistryPath);

    /* Set device routines */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = RamdiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RamdiskPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RamdiskSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = RamdiskScsi;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = RamdiskFlushBuffers;
    DriverObject->DriverExtension->AddDevice = RamdiskAddDevice;
    DriverObject->DriverUnload = RamdiskUnload;

    /* Check for a loader block */
    if (KeLoaderBlock)
    {
        /* Get the boot device name */
        BootDeviceName = KeLoaderBlock->ArcBootDeviceName;
        if (BootDeviceName)
        {
            /* Check if we're booting from ramdisk */
            if ((strlen(BootDeviceName) >= 10) &&
                !(_strnicmp(BootDeviceName, "ramdisk(0)", 10)))
            {
                /* We'll have to tell the PnP Manager */
                ReportDetectedDevice = TRUE;

                /* Check for a command line */
                CommandLine = KeLoaderBlock->LoadOptions;
                if (CommandLine)
                {
                    /* Check if this is an ISO boot */
                    if (strstr(CommandLine, "RDEXPORTASCD"))
                    {
                        /* Remember for later */
                        ExportBootDiskAsCd = TRUE;
                    }

                    /* Check if this is PE boot */
                    if (strstr(CommandLine, "MININT"))
                    {
                        /* Remember for later */
                        IsWinPEBoot = TRUE;
                    }
                }
            }

        }
    }

    /* Installing from Ramdisk isn't supported yet */
    if (KeLoaderBlock->SetupLdrBlock)
        DPRINT1("FIXME: Installing from RamDisk is UNSUPPORTED!\n");
    // ASSERT(!KeLoaderBlock->SetupLdrBlock);

    /* Are we reporting the device */
    if (ReportDetectedDevice)
    {
        /* Do it */
        Status = IoReportDetectedDevice(DriverObject,
                                        InterfaceTypeUndefined,
                                        0xFFFFFFFF,
                                        0xFFFFFFFF,
                                        NULL,
                                        NULL,
                                        0,
                                        &PhysicalDeviceObject);
        if (NT_SUCCESS(Status))
        {
            /* Create the device object */
            Status = RamdiskAddDevice(DriverObject, PhysicalDeviceObject);
            if (NT_SUCCESS(Status))
            {
                /* We are done */
                PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                Status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        /* Done */
        Status = STATUS_SUCCESS;
    }

    /* Done */
    return Status;
}
