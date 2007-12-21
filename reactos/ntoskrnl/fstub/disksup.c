/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/disksup.c
* PURPOSE:         I/O HAL Routines for Disk Access
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Eric Kohl (ekohl@rz-online.de)
*                  Casper S. Hornstrup (chorns@users.sourceforge.net)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#include <internal/hal.h>

/* DEPRECATED FUNCTIONS ******************************************************/

#if 1
const WCHAR DiskMountString[] = L"\\DosDevices\\%C:";

#define AUTO_DRIVE         ((ULONG)-1)

#define PARTITION_MAGIC    0xaa55

#include <pshpack1.h>

typedef struct _REG_DISK_MOUNT_INFO
{
    ULONG Signature;
    LARGE_INTEGER StartingOffset;
} REG_DISK_MOUNT_INFO, *PREG_DISK_MOUNT_INFO;

#include <poppack.h>

typedef enum _DISK_MANAGER
{
    NoDiskManager,
    OntrackDiskManager,
    EZ_Drive
} DISK_MANAGER;

static BOOLEAN
HalpAssignDrive(IN PUNICODE_STRING PartitionName,
                IN ULONG DriveNumber,
                IN UCHAR DriveType,
                IN ULONG Signature,
                IN LARGE_INTEGER StartingOffset,
                IN HANDLE hKey)
{
    WCHAR DriveNameBuffer[16];
    UNICODE_STRING DriveName;
    ULONG i;
    NTSTATUS Status;
    REG_DISK_MOUNT_INFO DiskMountInfo;

    DPRINT("HalpAssignDrive()\n");

    if ((DriveNumber != AUTO_DRIVE) && (DriveNumber < 26))
    {
        /* Force assignment */
        if ((ObSystemDeviceMap->DriveMap & (1 << DriveNumber)) != 0)
        {
            DbgPrint("Drive letter already used!\n");
            return FALSE;
        }
    }
    else
    {
        /* Automatic assignment */
        DriveNumber = AUTO_DRIVE;

        for (i = 2; i < 26; i++)
        {
            if ((ObSystemDeviceMap->DriveMap & (1 << i)) == 0)
            {
                DriveNumber = i;
                break;
            }
        }

        if (DriveNumber == AUTO_DRIVE)
        {
            DbgPrint("No drive letter available!\n");
            return FALSE;
        }
    }

    DPRINT("DriveNumber %d\n", DriveNumber);

    /* Update the System Device Map */
    ObSystemDeviceMap->DriveMap |= (1 << DriveNumber);
    ObSystemDeviceMap->DriveType[DriveNumber] = DriveType;

    /* Build drive name */
    swprintf(DriveNameBuffer,
        L"\\??\\%C:",
        'A' + DriveNumber);
    RtlInitUnicodeString(&DriveName,
        DriveNameBuffer);

    DPRINT("  %wZ ==> %wZ\n",
        &DriveName,
        PartitionName);

    /* Create symbolic link */
    Status = IoCreateSymbolicLink(&DriveName,
        PartitionName);

    if (hKey &&
        DriveType == DOSDEVICE_DRIVE_FIXED &&
        Signature)
    {
        DiskMountInfo.Signature = Signature;
        DiskMountInfo.StartingOffset = StartingOffset;
        swprintf(DriveNameBuffer, DiskMountString, L'A' + DriveNumber);
        RtlInitUnicodeString(&DriveName, DriveNameBuffer);

        Status = ZwSetValueKey(hKey,
            &DriveName,
            0,
            REG_BINARY,
            &DiskMountInfo,
            sizeof(DiskMountInfo));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateValueKey failed for %wZ, status=%x\n", &DriveName, Status);
        }
    }
    return TRUE;
}

ULONG
xHalpGetRDiskCount(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ArcName;
    PWCHAR ArcNameBuffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    POBJECT_DIRECTORY_INFORMATION DirectoryInfo;
    ULONG Skip;
    ULONG ResultLength;
    ULONG CurrentRDisk;
    ULONG RDiskCount;
    BOOLEAN First = TRUE;
    ULONG Count;

    DirectoryInfo = ExAllocatePool(PagedPool, 2 * PAGE_SIZE);
    if (DirectoryInfo == NULL)
    {
        return 0;
    }

    RtlInitUnicodeString(&ArcName, L"\\ArcName");
    InitializeObjectAttributes(&ObjectAttributes,
        &ArcName,
        0,
        NULL,
        NULL);

    Status = ZwOpenDirectoryObject (&DirectoryHandle,
        SYMBOLIC_LINK_ALL_ACCESS,
        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenDirectoryObject for %wZ failed, status=%lx\n", &ArcName, Status);
        ExFreePool(DirectoryInfo);
        return 0;
    }

    RDiskCount = 0;
    Skip = 0;
    while (NT_SUCCESS(Status))
    {
        Status = NtQueryDirectoryObject (DirectoryHandle,
            DirectoryInfo,
            2 * PAGE_SIZE,
            FALSE,
            First,
            &Skip,
            &ResultLength);
        First = FALSE;
        if (NT_SUCCESS(Status))
        {
            Count = 0;
            while (DirectoryInfo[Count].Name.Buffer)
            {
                DPRINT("Count %x\n", Count);
                DirectoryInfo[Count].Name.Buffer[DirectoryInfo[Count].Name.Length / sizeof(WCHAR)] = 0;
                ArcNameBuffer = DirectoryInfo[Count].Name.Buffer;
                if (DirectoryInfo[Count].Name.Length >= sizeof(L"multi(0)disk(0)rdisk(0)") - sizeof(WCHAR) &&
                    !_wcsnicmp(ArcNameBuffer, L"multi(0)disk(0)rdisk(", (sizeof(L"multi(0)disk(0)rdisk(") - sizeof(WCHAR)) / sizeof(WCHAR)))
                {
                    DPRINT("%S\n", ArcNameBuffer);
                    ArcNameBuffer += (sizeof(L"multi(0)disk(0)rdisk(") - sizeof(WCHAR)) / sizeof(WCHAR);
                    CurrentRDisk = 0;
                    while (iswdigit(*ArcNameBuffer))
                    {
                        CurrentRDisk = CurrentRDisk * 10 + *ArcNameBuffer - L'0';
                        ArcNameBuffer++;
                    }
                    if (!_wcsicmp(ArcNameBuffer, L")") &&
                        CurrentRDisk >= RDiskCount)
                    {
                        RDiskCount = CurrentRDisk + 1;
                    }
                }
                Count++;
            }
        }
    }
    ExFreePool(DirectoryInfo);
    return RDiskCount;
}

NTSTATUS
xHalpGetDiskNumberFromRDisk(ULONG RDisk, PULONG DiskNumber)
{
    WCHAR NameBuffer[80];
    UNICODE_STRING ArcName;
    UNICODE_STRING LinkName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;
    NTSTATUS Status;

    swprintf(NameBuffer,
        L"\\ArcName\\multi(0)disk(0)rdisk(%lu)",
        RDisk);

    RtlInitUnicodeString(&ArcName, NameBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
        &ArcName,
        0,
        NULL,
        NULL);
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
        SYMBOLIC_LINK_ALL_ACCESS,
        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenSymbolicLinkObject failed for %wZ, status=%lx\n", &ArcName, Status);
        return Status;
    }

    LinkName.Buffer = NameBuffer;
    LinkName.Length = 0;
    LinkName.MaximumLength = sizeof(NameBuffer);
    Status = ZwQuerySymbolicLinkObject(LinkHandle,
        &LinkName,
        NULL);
    ZwClose(LinkHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQuerySymbolicLinkObject failed, status=%lx\n", Status);
        return Status;
    }
    if (LinkName.Length < sizeof(L"\\Device\\Harddisk0\\Partition0") - sizeof(WCHAR) ||
        LinkName.Length >= sizeof(NameBuffer))
    {
        return STATUS_UNSUCCESSFUL;
    }

    NameBuffer[LinkName.Length / sizeof(WCHAR)] = 0;
    if (_wcsnicmp(NameBuffer, L"\\Device\\Harddisk", (sizeof(L"\\Device\\Harddisk") - sizeof(WCHAR)) / sizeof(WCHAR)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    LinkName.Buffer += (sizeof(L"\\Device\\Harddisk") - sizeof(WCHAR)) / sizeof(WCHAR);

    if (!iswdigit(*LinkName.Buffer))
    {
        return STATUS_UNSUCCESSFUL;
    }
    *DiskNumber = 0;
    while (iswdigit(*LinkName.Buffer))
    {
        *DiskNumber = *DiskNumber * 10 + *LinkName.Buffer - L'0';
        LinkName.Buffer++;
    }
    if (_wcsicmp(LinkName.Buffer, L"\\Partition0"))
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
xHalQueryDriveLayout(IN PUNICODE_STRING DeviceName,
                     OUT PDRIVE_LAYOUT_INFORMATION *LayoutInfo)
{
    IO_STATUS_BLOCK StatusBlock;
    DISK_GEOMETRY DiskGeometry;
    PDEVICE_OBJECT DeviceObject = NULL;
    PFILE_OBJECT FileObject;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    DPRINT("xHalpQueryDriveLayout %wZ %p\n",
        DeviceName,
        LayoutInfo);

    /* Get the drives sector size */
    Status = IoGetDeviceObjectPointer(DeviceName,
        FILE_READ_ATTRIBUTES,
        &FileObject,
        &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Status %x\n", Status);
        return(Status);
    }

    KeInitializeEvent(&Event,
        NotificationEvent,
        FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
        DeviceObject,
        NULL,
        0,
        &DiskGeometry,
        sizeof(DISK_GEOMETRY),
        FALSE,
        &Event,
        &StatusBlock);
    if (Irp == NULL)
    {
        ObDereferenceObject(FileObject);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = IoCallDriver(DeviceObject,
        Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        Status = StatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
        {
            DiskGeometry.BytesPerSector = 512;
        }
        else
        {
            ObDereferenceObject(FileObject);
            return(Status);
        }
    }

    DPRINT("DiskGeometry.BytesPerSector: %d\n",
        DiskGeometry.BytesPerSector);

    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        PDRIVE_LAYOUT_INFORMATION Buffer;

        /* Allocate a partition list for a single entry. */
        Buffer = ExAllocatePool(NonPagedPool,
            sizeof(DRIVE_LAYOUT_INFORMATION));
        if (Buffer != NULL)
        {
            RtlZeroMemory(Buffer,
                sizeof(DRIVE_LAYOUT_INFORMATION));
            Buffer->PartitionCount = 1;
            *LayoutInfo = Buffer;

            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        /* Read the partition table */
        Status = IoReadPartitionTable(DeviceObject,
            DiskGeometry.BytesPerSector,
            FALSE,
            LayoutInfo);
    }

    ObDereferenceObject(FileObject);

    return(Status);
}

VOID
FASTCALL
xHalIoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PSTRING NtDeviceName,
                         OUT PUCHAR NtSystemPath,
                         OUT PSTRING NtSystemPathString)
{
    PDRIVE_LAYOUT_INFORMATION *LayoutArray;
    PCONFIGURATION_INFORMATION ConfigInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING UnicodeString1;
    UNICODE_STRING UnicodeString2;
    HANDLE FileHandle;
    PWSTR Buffer1;
    PWSTR Buffer2;
    ULONG i, j, k;
    ULONG DiskNumber;
    ULONG RDisk;
    NTSTATUS Status;
    HANDLE hKey;
    ULONG Length;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;
    PREG_DISK_MOUNT_INFO DiskMountInfo;
    ULONG RDiskCount;

    DPRINT("xHalIoAssignDriveLetters()\n");

    ConfigInfo = IoGetConfigurationInformation();

    RDiskCount = xHalpGetRDiskCount();

    DPRINT("RDiskCount %d\n", RDiskCount);

    Buffer1 = (PWSTR)ExAllocatePool(PagedPool,
        64 * sizeof(WCHAR));
    Buffer2 = (PWSTR)ExAllocatePool(PagedPool,
        32 * sizeof(WCHAR));

    PartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool,
        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(REG_DISK_MOUNT_INFO));

    DiskMountInfo = (PREG_DISK_MOUNT_INFO) PartialInformation->Data;

    /* Open or Create the 'MountedDevices' key */
    RtlInitUnicodeString(&UnicodeString1, L"\\Registry\\Machine\\SYSTEM\\MountedDevices");
    InitializeObjectAttributes(&ObjectAttributes,
        &UnicodeString1,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    Status = ZwOpenKey(&hKey,
        KEY_ALL_ACCESS,
        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        Status = ZwCreateKey(&hKey,
            KEY_ALL_ACCESS,
            &ObjectAttributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            NULL);
    }
    if (!NT_SUCCESS(Status))
    {
        hKey = NULL;
        DPRINT("ZwCreateKey failed for %wZ, status=%x\n", &UnicodeString1, Status);
    }

    /* Create PhysicalDrive links */
    DPRINT("Physical disk drives: %d\n", ConfigInfo->DiskCount);
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        swprintf(Buffer1,
            L"\\Device\\Harddisk%d\\Partition0",
            i);
        RtlInitUnicodeString(&UnicodeString1,
            Buffer1);

        InitializeObjectAttributes(&ObjectAttributes,
            &UnicodeString1,
            0,
            NULL,
            NULL);

        Status = ZwOpenFile(&FileHandle,
            FILE_READ_DATA | SYNCHRONIZE,
            &ObjectAttributes,
            &StatusBlock,
            FILE_SHARE_READ,
            FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(Status))
        {
            ZwClose(FileHandle);

            swprintf(Buffer2,
                L"\\??\\PhysicalDrive%d",
                i);
            RtlInitUnicodeString(&UnicodeString2,
                Buffer2);

            DPRINT("Creating link: %S ==> %S\n",
                Buffer2,
                Buffer1);

            IoCreateSymbolicLink(&UnicodeString2,
                &UnicodeString1);
        }
    }

    /* Initialize layout array */
    LayoutArray = ExAllocatePool(NonPagedPool,
        ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
    RtlZeroMemory(LayoutArray,
        ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        swprintf(Buffer1,
            L"\\Device\\Harddisk%d\\Partition0",
            i);
        RtlInitUnicodeString(&UnicodeString1,
            Buffer1);

        Status = xHalQueryDriveLayout(&UnicodeString1,
            &LayoutArray[i]);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("xHalQueryDriveLayout() failed (Status = 0x%lx)\n",
                Status);
            LayoutArray[i] = NULL;
            continue;
        }
        /* We don't use the RewritePartition value while mounting the disks.
        * We use this value for marking pre-assigned (registry) partitions.
        */
        for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
        {
            LayoutArray[i]->PartitionEntry[j].RewritePartition = FALSE;
        }
    }

#ifndef NDEBUG
    /* Dump layout array */
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        DPRINT("Harddisk %d:\n",
            i);

        if (LayoutArray[i] == NULL)
            continue;

        DPRINT("Logical partitions: %d\n",
            LayoutArray[i]->PartitionCount);

        for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
        {
            DPRINT("  %d: nr:%x boot:%x type:%x startblock:%I64u count:%I64u\n",
                j,
                LayoutArray[i]->PartitionEntry[j].PartitionNumber,
                LayoutArray[i]->PartitionEntry[j].BootIndicator,
                LayoutArray[i]->PartitionEntry[j].PartitionType,
                LayoutArray[i]->PartitionEntry[j].StartingOffset.QuadPart,
                LayoutArray[i]->PartitionEntry[j].PartitionLength.QuadPart);
        }
    }
#endif

    /* Assign pre-assigned (registry) partitions */
    if (hKey)
    {
        for (k = 2; k < 26; k++)
        {
            swprintf(Buffer1, DiskMountString, L'A' + k);
            RtlInitUnicodeString(&UnicodeString1, Buffer1);
            Status = ZwQueryValueKey(hKey,
                &UnicodeString1,
                KeyValuePartialInformation,
                PartialInformation,
                sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(REG_DISK_MOUNT_INFO),
                &Length);
            if (NT_SUCCESS(Status) &&
                PartialInformation->Type == REG_BINARY &&
                PartialInformation->DataLength == sizeof(REG_DISK_MOUNT_INFO))
            {
                DPRINT("%wZ => %08x:%08x%08x\n", &UnicodeString1, DiskMountInfo->Signature,
                    DiskMountInfo->StartingOffset.u.HighPart, DiskMountInfo->StartingOffset.u.LowPart);
                {
                    BOOLEAN Found = FALSE;
                    for (i = 0; i < ConfigInfo->DiskCount; i++)
                    {
                        DPRINT("%x\n", LayoutArray[i]->Signature);
                        if (LayoutArray[i] &&
                            LayoutArray[i]->Signature &&
                            LayoutArray[i]->Signature == DiskMountInfo->Signature)
                        {
                            for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
                            {
                                if (LayoutArray[i]->PartitionEntry[j].StartingOffset.QuadPart == DiskMountInfo->StartingOffset.QuadPart)
                                {
                                    if (IsRecognizedPartition(LayoutArray[i]->PartitionEntry[j].PartitionType) &&
                                        LayoutArray[i]->PartitionEntry[j].RewritePartition == FALSE)
                                    {
                                        swprintf(Buffer2,
                                            L"\\Device\\Harddisk%d\\Partition%d",
                                            i,
                                            LayoutArray[i]->PartitionEntry[j].PartitionNumber);
                                        RtlInitUnicodeString(&UnicodeString2,
                                            Buffer2);

                                        /* Assign drive */
                                        DPRINT("  %wZ\n", &UnicodeString2);
                                        Found = HalpAssignDrive(&UnicodeString2,
                                            k,
                                            DOSDEVICE_DRIVE_FIXED,
                                            DiskMountInfo->Signature,
                                            DiskMountInfo->StartingOffset,
                                            NULL);
                                        /* Mark the partition as assigned */
                                        LayoutArray[i]->PartitionEntry[j].RewritePartition = TRUE;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    if (Found == FALSE)
                    {
                        /* We didn't find a partition for this entry, remove them. */
                        Status = ZwDeleteValueKey(hKey, &UnicodeString1);
                    }
                }
            }
        }
    }

    /* Assign bootable partition on first harddisk */
    DPRINT("Assigning bootable primary partition on first harddisk:\n");
    if (RDiskCount > 0)
    {
        Status = xHalpGetDiskNumberFromRDisk(0, &DiskNumber);
        if (NT_SUCCESS(Status) &&
            DiskNumber < ConfigInfo->DiskCount &&
            LayoutArray[DiskNumber])
        {
            /* Search for bootable partition */
            for (j = 0; j < NUM_PARTITION_TABLE_ENTRIES && j < LayoutArray[DiskNumber]->PartitionCount; j++)
            {
                if ((LayoutArray[DiskNumber]->PartitionEntry[j].BootIndicator == TRUE) &&
                    IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
                {
                    if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE)
                    {
                        swprintf(Buffer2,
                            L"\\Device\\Harddisk%lu\\Partition%d",
                            DiskNumber,
                            LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
                        RtlInitUnicodeString(&UnicodeString2,
                            Buffer2);

                        /* Assign drive */
                        DPRINT("  %wZ\n", &UnicodeString2);
                        HalpAssignDrive(&UnicodeString2,
                            AUTO_DRIVE,
                            DOSDEVICE_DRIVE_FIXED,
                            LayoutArray[DiskNumber]->Signature,
                            LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                            hKey);
                        /* Mark the partition as assigned */
                        LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                    }
                    break;
                }
            }
        }
    }

    /* Assign remaining primary partitions */
    DPRINT("Assigning remaining primary partitions:\n");
    for (RDisk = 0; RDisk < RDiskCount; RDisk++)
    {
        Status = xHalpGetDiskNumberFromRDisk(RDisk, &DiskNumber);
        if (NT_SUCCESS(Status) &&
            DiskNumber < ConfigInfo->DiskCount &&
            LayoutArray[DiskNumber])
        {
            /* Search for primary partitions */
            for (j = 0; (j < NUM_PARTITION_TABLE_ENTRIES) && (j < LayoutArray[DiskNumber]->PartitionCount); j++)
            {
                if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                    IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
                {
                    swprintf(Buffer2,
                        L"\\Device\\Harddisk%d\\Partition%d",
                        DiskNumber,
                        LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
                    RtlInitUnicodeString(&UnicodeString2,
                        Buffer2);

                    /* Assign drive */
                    DPRINT("  %wZ\n",
                        &UnicodeString2);
                    HalpAssignDrive(&UnicodeString2,
                        AUTO_DRIVE,
                        DOSDEVICE_DRIVE_FIXED,
                        LayoutArray[DiskNumber]->Signature,
                        LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                        hKey);
                    /* Mark the partition as assigned */
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                }
            }
        }
    }

    /* Assign extended (logical) partitions */
    DPRINT("Assigning extended (logical) partitions:\n");
    for (RDisk = 0; RDisk < RDiskCount; RDisk++)
    {
        Status = xHalpGetDiskNumberFromRDisk(RDisk, &DiskNumber);
        if (NT_SUCCESS(Status) &&
            DiskNumber < ConfigInfo->DiskCount &&
            LayoutArray[DiskNumber])
        {
            /* Search for extended partitions */
            for (j = NUM_PARTITION_TABLE_ENTRIES; j < LayoutArray[DiskNumber]->PartitionCount; j++)
            {
                if (IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType) &&
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                    LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber != 0)
                {
                    swprintf(Buffer2,
                        L"\\Device\\Harddisk%d\\Partition%d",
                        DiskNumber,
                        LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
                    RtlInitUnicodeString(&UnicodeString2,
                        Buffer2);

                    /* Assign drive */
                    DPRINT("  %wZ\n",
                        &UnicodeString2);
                    HalpAssignDrive(&UnicodeString2,
                        AUTO_DRIVE,
                        DOSDEVICE_DRIVE_FIXED,
                        LayoutArray[DiskNumber]->Signature,
                        LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                        hKey);
                    /* Mark the partition as assigned */
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                }
            }
        }
    }

    /* Assign remaining primary partitions without an arc-name */
    DPRINT("Assigning remaining primary partitions:\n");
    for (DiskNumber = 0; DiskNumber < ConfigInfo->DiskCount; DiskNumber++)
    {
        if (LayoutArray[DiskNumber])
        {
            /* Search for primary partitions */
            for (j = 0; (j < NUM_PARTITION_TABLE_ENTRIES) && (j < LayoutArray[DiskNumber]->PartitionCount); j++)
            {
                if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                    IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
                {
                    swprintf(Buffer2,
                        L"\\Device\\Harddisk%d\\Partition%d",
                        DiskNumber,
                        LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
                    RtlInitUnicodeString(&UnicodeString2,
                        Buffer2);

                    /* Assign drive */
                    DPRINT("  %wZ\n",
                        &UnicodeString2);
                    HalpAssignDrive(&UnicodeString2,
                        AUTO_DRIVE,
                        DOSDEVICE_DRIVE_FIXED,
                        LayoutArray[DiskNumber]->Signature,
                        LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                        hKey);
                    /* Mark the partition as assigned */
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                }
            }
        }
    }

    /* Assign extended (logical) partitions without an arc-name */
    DPRINT("Assigning extended (logical) partitions:\n");
    for (DiskNumber = 0; DiskNumber < ConfigInfo->DiskCount; DiskNumber++)
    {
        if (LayoutArray[DiskNumber])
        {
            /* Search for extended partitions */
            for (j = NUM_PARTITION_TABLE_ENTRIES; j < LayoutArray[DiskNumber]->PartitionCount; j++)
            {
                if (IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType) &&
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                    LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber != 0)
                {
                    swprintf(Buffer2,
                        L"\\Device\\Harddisk%d\\Partition%d",
                        DiskNumber,
                        LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
                    RtlInitUnicodeString(&UnicodeString2,
                        Buffer2);

                    /* Assign drive */
                    DPRINT("  %wZ\n",
                        &UnicodeString2);
                    HalpAssignDrive(&UnicodeString2,
                        AUTO_DRIVE,
                        DOSDEVICE_DRIVE_FIXED,
                        LayoutArray[DiskNumber]->Signature,
                        LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                        hKey);
                    /* Mark the partition as assigned */
                    LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                }
            }
        }
    }

    /* Assign removable disk drives */
    DPRINT("Assigning removable disk drives:\n");
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        if (LayoutArray[i])
        {
            /* Search for virtual partitions */
            if (LayoutArray[i]->PartitionCount == 1 &&
                LayoutArray[i]->PartitionEntry[0].PartitionType == 0)
            {
                swprintf(Buffer2,
                    L"\\Device\\Harddisk%d\\Partition1",
                    i);
                RtlInitUnicodeString(&UnicodeString2,
                    Buffer2);

                /* Assign drive */
                DPRINT("  %wZ\n",
                    &UnicodeString2);
                HalpAssignDrive(&UnicodeString2,
                    AUTO_DRIVE,
                    DOSDEVICE_DRIVE_REMOVABLE,
                    0,
                    RtlConvertLongToLargeInteger(0),
                    hKey);
            }
        }
    }

    /* Free layout array */
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        if (LayoutArray[i] != NULL)
            ExFreePool(LayoutArray[i]);
    }
    ExFreePool(LayoutArray);

    /* Assign floppy drives */
    DPRINT("Floppy drives: %d\n", ConfigInfo->FloppyCount);
    for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
        swprintf(Buffer1,
            L"\\Device\\Floppy%d",
            i);
        RtlInitUnicodeString(&UnicodeString1,
            Buffer1);

        /* Assign drive letters A: or B: or first free drive letter */
        DPRINT("  %wZ\n",
            &UnicodeString1);
        HalpAssignDrive(&UnicodeString1,
            (i < 2) ? i : AUTO_DRIVE,
            DOSDEVICE_DRIVE_REMOVABLE,
            0,
            RtlConvertLongToLargeInteger(0),
            hKey);
    }

    /* Assign cdrom drives */
    DPRINT("CD-Rom drives: %d\n", ConfigInfo->CdRomCount);
    for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
        swprintf(Buffer1,
            L"\\Device\\CdRom%d",
            i);
        RtlInitUnicodeString(&UnicodeString1,
            Buffer1);

        /* Assign first free drive letter */
        DPRINT("  %wZ\n", &UnicodeString1);
        HalpAssignDrive(&UnicodeString1,
            AUTO_DRIVE,
            DOSDEVICE_DRIVE_CDROM,
            0,
            RtlConvertLongToLargeInteger(0),
            hKey);
    }

    /* Anything else to do? */

    ExFreePool(PartialInformation);
    ExFreePool(Buffer2);
    ExFreePool(Buffer1);
    if (hKey)
    {
        ZwClose(hKey);
    }
}

#endif

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
HalpGetFullGeometry(IN PDEVICE_OBJECT DeviceObject,
                    IN PDISK_GEOMETRY Geometry,
                    OUT PULONGLONG RealSectorCount)
{
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PKEVENT Event;
    NTSTATUS Status;
    PARTITION_INFORMATION PartitionInfo;
    PAGED_CODE();

    /* Allocate a non-paged event */
    Event = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(KEVENT),
                                     TAG_FILE_SYSTEM);
    if (!Event) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize it */
    KeInitializeEvent(Event, NotificationEvent, FALSE);

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                             DeviceObject,
                                             NULL,
                                             0UL,
                                             Geometry,
                                             sizeof(DISK_GEOMETRY),
                                             FALSE,
                                             Event,
                                             &IoStatusBlock);
    if (!Irp)
    {
        /* Fail, free the event */
        ExFreePool(Event);
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

    /* Check if the driver returned success */
    if(NT_SUCCESS(Status))
    {
        /* Build another IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                                 DeviceObject,
                                                 NULL,
                                                 0UL,
                                                 &PartitionInfo,
                                                 sizeof(PARTITION_INFORMATION),
                                                 FALSE,
                                                 Event,
                                                 &IoStatusBlock);
        if (!Irp)
        {
            /* Fail, free the event */
            ExFreePool(Event);
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

        /* Check if the driver returned success */
        if(NT_SUCCESS(Status))
        {
            /* Get the number of sectors */
            *RealSectorCount = (PartitionInfo.PartitionLength.QuadPart /
                                Geometry->BytesPerSector);
        }
    }

    /* Free the event and return the Status */
    ExFreePool(Event);
    return Status;
}

BOOLEAN
NTAPI
HalpIsValidPartitionEntry(IN PPARTITION_DESCRIPTOR Entry,
                          IN ULONGLONG MaxOffset,
                          IN ULONGLONG MaxSector)
{
    ULONGLONG EndingSector;
    PAGED_CODE();

    /* Unused partitions are considered valid */
    if (Entry->PartitionType == PARTITION_ENTRY_UNUSED) return TRUE;

    /* Get the last sector of the partition */
    EndingSector = GET_STARTING_SECTOR(Entry) +  GET_PARTITION_LENGTH(Entry);

    /* Check if it's more then the maximum sector */
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
    else if(GET_STARTING_SECTOR(Entry) > MaxOffset)
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

VOID
NTAPI
HalpCalculateChsValues(IN PLARGE_INTEGER PartitionOffset,
                       IN PLARGE_INTEGER PartitionLength,
                       IN CCHAR ShiftCount,
                       IN ULONG SectorsPerTrack,
                       IN ULONG NumberOfTracks,
                       IN ULONG ConventionalCylinders,
                       OUT PPARTITION_DESCRIPTOR PartitionDescriptor)
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

        /* Make sure it's not larger then 1024 */
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
    if (Event) ExFreePool(Event);
    if (IoStatusBlock) ExFreePool(IoStatusBlock);
    if (DiskGeometry) ExFreePool(DiskGeometry);
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
    BufferSize = max(SectorSize, 512);

    /* Allocate the buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                       PAGE_SIZE > BufferSize ?
                                       PAGE_SIZE : BufferSize,
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
        ExFreePool(Buffer);
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

    /* Check driver Status */
    if (NT_SUCCESS(Status))
    {
        /* Validate the MBR Signature */
        if (((PUSHORT)Buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
        {
            /* Failed */
            ExFreePool(Buffer);
            return;
        }

        /* Get the partition entry */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                               &(((PUSHORT)Buffer)[PARTITION_TABLE_OFFSET]);

        /* Make sure it's what the caller wanted */
        if (PartitionDescriptor->PartitionType != MbrTypeIdentifier)
        {
            /* It's not, free our buffer */
        ExFreePool(Buffer);
        }
        else
        {
            /* Check if this is a secondary entry */
            if (PartitionDescriptor->PartitionType == 0x54)
            {
                /* Return our buffer, but at sector 63 */
                *(PULONG)Buffer = 63;
                *MbrBuffer = Buffer;
            }
            else if (PartitionDescriptor->PartitionType == 0x55)
            {
                /* EZ Drive, return the buffer directly */
                *MbrBuffer = Buffer;
            }
            else
            {
                /* Otherwise crash on debug builds */
                ASSERT(PartitionDescriptor->PartitionType == 0x55);
            }
        }
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
    DISK_GEOMETRY DiskGeometry;
    LONGLONG EndSector, MaxSector, StartOffset;
    ULONGLONG MaxOffset;
    LARGE_INTEGER Offset, VolumeOffset;
    BOOLEAN IsPrimary = TRUE, IsEzDrive = FALSE, MbrFound = FALSE;
    BOOLEAN IsValid, IsEmpty = TRUE;
    PVOID MbrBuffer;
    PIO_STACK_LOCATION IoStackLocation;
    PBOOT_SECTOR_INFO BootSectorInfo = (PBOOT_SECTOR_INFO)Buffer;
    UCHAR PartitionType;
    LARGE_INTEGER HiddenSectors64;
    VolumeOffset.QuadPart = Offset.QuadPart = 0;
    PAGED_CODE();

    /* Allocate the buffer */
    *PartitionBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                             BufferSize,
                                             TAG_FILE_SYSTEM);
    if (!(*PartitionBuffer)) return STATUS_INSUFFICIENT_RESOURCES;

    /* Normalize the buffer size */
    InputSize = max(512, SectorSize);

    /* Check for EZ Drive */
    HalExamineMBR(DeviceObject, InputSize, 0x55, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePool(MbrBuffer);
        Offset.QuadPart = 512;
    }

    /* Get drive geometry */
    Status = HalpGetFullGeometry(DeviceObject, &DiskGeometry, &MaxOffset);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(*PartitionBuffer);
        *PartitionBuffer = NULL;
        return Status;
    }

    /* Get the end and maximum sector */
    EndSector = MaxOffset;
    MaxSector = MaxOffset << 1;
    DPRINT("FSTUB: MaxOffset = %#I64x, MaxSector = %#I64x\n",
            MaxOffset, MaxSector);

    /* Allocate our buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_FILE_SYSTEM);
    if (!Buffer)
    {
        /* Fail, free the input buffer */
        ExFreePool(*PartitionBuffer);
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
        if (((PUSHORT)Buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
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
            (*PartitionBuffer)->Signature =  ((PULONG)Buffer)
                                             [PARTITION_TABLE_OFFSET / 2 - 1];
        }

        /* Get the partition descriptor array */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                               &(((PUSHORT)Buffer)[PARTITION_TABLE_OFFSET]);

        /* Get the partition type */
        PartitionType = PartitionDescriptor->PartitionType;

        /* Start looping partitions */
        j++;
        DPRINT("FSTUB: Partition Table %d:\n", j);
        for (Entry = 1, k = 0; Entry <= 4; Entry++, PartitionDescriptor++)
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

            /* Make sure that the partition is valid, unless it's the first */
            if (!(HalpIsValidPartitionEntry(PartitionDescriptor,
                                            MaxOffset,
                                            MaxSector)) && !(j))
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
                    /* More then one table is invalid */
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
                    /* Out of memory, unto this extra structure */
                    --i;
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Copy the contents of the old buffer */
                RtlMoveMemory(DriveLayoutInfo,
                              *PartitionBuffer,
                              BufferSize);

                /* Free the old buffer and set this one as the new one */
                ExFreePool(*PartitionBuffer);
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

                /* Check if its' a container */
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

                /* FIXME: REACTOS HACK */
                PartitionInfo->PartitionNumber = i + 1;
            }
            else
            {
                /* Otherwise, clear all the relevant fields */
                PartitionInfo->BootIndicator = FALSE;
                PartitionInfo->RecognizedPartition = FALSE;
                PartitionInfo->StartingOffset.QuadPart = 0;
                PartitionInfo->PartitionLength.QuadPart = 0;
                PartitionInfo->HiddenSectors = 0;

                /* FIXME: REACTOS HACK */
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
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                               &(((PUSHORT)Buffer)[PARTITION_TABLE_OFFSET]);
        for (Entry = 1; Entry <= 4; Entry++, PartitionDescriptor++)
        {
            /* Check if this is a container partition, since we skipped them */
            if (IsContainerPartition(PartitionType))
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
                DPRINT1("FSTUB: MaxSector now = %#08lx\n", MaxSector);
                break;
            }
        }

        /* Loop the next partitions, which are not primary anymore */
        IsPrimary = FALSE;
    } while (Offset.HighPart | Offset.LowPart);

    /* Check if this is a removable device that's probably a super-floppy */
    if ((DiskGeometry.MediaType == RemovableMedia) &&
        !(j) &&
        (MbrFound) &&
        (IsEmpty))
    {
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
        if ((MbrFound) || (DiskGeometry.MediaType == RemovableMedia))
        {
            /* Print out debugging information */
            DPRINT1("FSTUB: Drive %#p has no valid MBR. Make it into a "
                    "super-floppy\n",
                    DeviceObject);
            DPRINT1("FSTUB: Drive has %#08lx sectors and is %#016I64x "
                    "bytes large\n",
                    EndSector, EndSector * DiskGeometry.BytesPerSector);

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
                PartitionInfo->PartitionLength.QuadPart = (EndSector *
                                                           DiskGeometry.
                                                           BytesPerSector);

                /* FIXME: REACTOS HACK */
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
    if (Buffer) ExFreePool(Buffer);
    if (!NT_SUCCESS(Status)) ExFreePool(*PartitionBuffer);

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
    VolumeOffset.QuadPart = Offset.QuadPart = 0;
    PAGED_CODE();

    /* Normalize the buffer size */
    BufferSize = max(512, SectorSize);

    /* Check for EZ Drive */
    HalExamineMBR(DeviceObject, BufferSize, 0x55, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePool(MbrBuffer);
        Offset.QuadPart = 512;
    }

    /* Allocate our partition buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_FILE_SYSTEM);
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the event we'll use and loop partitions */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    do
    {
        /* Reset the event since we reuse it */
        KeResetEvent(&Event);

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
        if (((PUSHORT)Buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
        {
            /* It's not, fail */
            Status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        /* Get the partition descriptors and loop them */
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                              &(((PUSHORT)Buffer)[PARTITION_TABLE_OFFSET]);
        for (Entry = 1; Entry <= 4; Entry++, PartitionDescriptor++)
        {
            /* Check if it's unused or a container partition */
            if ((PartitionDescriptor->PartitionType ==
                 PARTITION_ENTRY_UNUSED) ||
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
                KeResetEvent(&Event);

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
        PartitionDescriptor = (PPARTITION_DESCRIPTOR)
                               &(((PUSHORT)Buffer)[PARTITION_TABLE_OFFSET]);
        for (Entry = 1; Entry <= 4; Entry++, PartitionDescriptor++)
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
    if (Buffer) ExFreePool(Buffer);
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
    PUSHORT Buffer;
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
    ExtendedOffset.QuadPart = NextOffset.QuadPart = Offset.QuadPart = 0;
    PAGED_CODE();

    /* Normalize the buffer size */
    BufferSize = max(512, SectorSize);

    /* Get the partial drive geometry */
    xHalGetPartialGeometry(DeviceObject, &ConventionalCylinders, &DiskSize);

    /* Check for EZ Drive */
    HalExamineMBR(DeviceObject, BufferSize, 0x55, &MbrBuffer);
    if (MbrBuffer)
    {
        /* EZ Drive found, bias the offset */
        IsEzDrive = TRUE;
        ExFreePool(MbrBuffer);
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

            /* Check if it needs a rewrite, and disable EZ drive for sure */
            if (PartitionInfo->RewritePartition) DoRewrite = TRUE;
            IsEzDrive = FALSE;
        }
    }

    /* Count the number of partition tables */
    DiskLayout->TableCount = (PartitionBuffer->PartitionCount + 4 - 1) / 4;

    /* Allocate our partition buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_FILE_SYSTEM);
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
            Buffer[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;

            /* By default, don't require a rewrite */
            DoRewrite = FALSE;

            /* Check if we don't have an offset */
            if (!Offset.QuadPart)
            {
                /* Check if the signature doesn't match */
                if (((PULONG)Buffer)[PARTITION_TABLE_OFFSET / 2 - 1] !=
                    PartitionBuffer->Signature)
                {
                    /* Then write the signature and now w need a rewrite */
                    ((PULONG)Buffer)[PARTITION_TABLE_OFFSET / 2 - 1] =
                        PartitionBuffer->Signature;
                    DoRewrite = TRUE;
                }
            }

            /* Loop the partition table entries */
            PartitionTable = &DiskLayout->PartitionTable[i];
            for (j = 0; j < 4; j++)
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
            if ((IsEzDrive) && !(Offset.QuadPart)) Offset.QuadPart = 512;

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
    if (Buffer) ExFreePool(Buffer);
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
