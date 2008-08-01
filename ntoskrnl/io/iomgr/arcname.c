/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/io/iomgr/arcname.c
* PURPOSE:         ARC Path Initialization Functions
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Eric Kohl (ekohl@rz-online.de)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

UNICODE_STRING IoArcHalDeviceName, IoArcBootDeviceName;
PCHAR IoLoaderArcBootDeviceName;

/* FUNCTIONS *****************************************************************/

BOOLEAN
INIT_FUNCTION
NTAPI
IopApplyRosCdromArcHack(IN ULONG i)
{
    ULONG DeviceNumber = -1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ANSI_STRING InstallName;
    UNICODE_STRING DeviceName;
    CHAR Buffer[128];
    FILE_BASIC_INFORMATION FileInfo;
    NTSTATUS Status;
    PCHAR p, q;
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();
    extern BOOLEAN InitIsWinPEMode, ExpInTextModeSetup;

    /* Only ARC Name left - Build full ARC Name */
    p = strstr(KeLoaderBlock->ArcBootDeviceName, "cdrom");
    if (p)
    {
        /* Build installer name */
        sprintf(Buffer, "\\Device\\CdRom%lu\\reactos\\ntoskrnl.exe", i);
        RtlInitAnsiString(&InstallName, Buffer);
        Status = RtlAnsiStringToUnicodeString(&DeviceName, &InstallName, TRUE);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Try to find the installer */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   0,
                                   NULL,
                                   NULL);
        Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);

        /* Free the string */
        RtlFreeUnicodeString(&DeviceName);

        /* Check if we found the file */
        if (NT_SUCCESS(Status))
        {
            /* We did, save the device number */
            DeviceNumber = i;
        }
        else
        {
            /* Build live CD kernel name */
            sprintf(Buffer,
                    "\\Device\\CdRom%lu\\reactos\\system32\\ntoskrnl.exe",
                    i);
            RtlInitAnsiString(&InstallName, Buffer);
            Status = RtlAnsiStringToUnicodeString(&DeviceName,
                                                  &InstallName,
                                                  TRUE);
            if (!NT_SUCCESS(Status)) return FALSE;

            /* Try to find it */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &DeviceName,
                                       0,
                                       NULL,
                                       NULL);
            Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);
            if (NT_SUCCESS(Status)) DeviceNumber = i;

            /* Free the string */
            RtlFreeUnicodeString(&DeviceName);
        }

        if (!InitIsWinPEMode)
        {
            /* Build the name */
            sprintf(p, "cdrom(%lu)", DeviceNumber);

            /* Adjust original command line */
            q = strchr(p, ')');
            if (q)
            {
                q++;
                strcpy(Buffer, q);
                sprintf(p, "cdrom(%lu)", DeviceNumber);
                strcat(p, Buffer);
            }
        }
    }

    /* OK, how many disks are there? */
    DeviceNumber += ConfigInfo->DiskCount;

    /* Return whether this is the CD or not */
    if ((InitIsWinPEMode) || (ExpInTextModeSetup))
    {
        /* Hack until IoAssignDriveLetters is fixed */
        swprintf(SharedUserData->NtSystemRoot, L"%c:\\reactos", 'C' + DeviceNumber);
        return TRUE;
    }

    /* Failed */
    return FALSE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopGetDiskInformation(IN ULONG i,
                      OUT PULONG CheckSum,
                      OUT PULONG Signature,
                      OUT PULONG PartitionCount,
                      OUT PDEVICE_OBJECT *DiskDeviceObject)
{
    ULONG j, Checksum;
    ANSI_STRING TempString;
    CHAR Buffer[128];
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    DISK_GEOMETRY DiskGeometry;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK StatusBlock;
    LARGE_INTEGER PartitionOffset;
    PULONG PartitionBuffer;

    /* Build the name */
    sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0", i);

    /* Convert it to Unicode */
    RtlInitAnsiString(&TempString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&DeviceName, &TempString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get the device pointer */
    Status = IoGetDeviceObjectPointer(&DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    *DiskDeviceObject = DeviceObject;

    /* Free the string */
    RtlFreeUnicodeString(&DeviceName);

    /* Move on if we failed */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Build an IRP to determine the sector size */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        &Event,
                                        &StatusBlock);
    if (!Irp)
    {
        /* Try again */
        ObDereferenceObject(FileObject);
        return FALSE;
    }

    /* Call the driver and check if we have to wait on it */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait on the driver */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = StatusBlock.Status;
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Try again */
        ObDereferenceObject(FileObject);
        return FALSE;
    }

    /* Read the partition table */
    Status = IoReadPartitionTable(DeviceObject,
                                  DiskGeometry.BytesPerSector,
                                  TRUE,
                                  &DriveLayout);

    /* Dereference the file object */
    ObDereferenceObject(FileObject);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Set the offset to 0 */
    PartitionOffset.QuadPart = 0;

    /* Allocate a buffer for the partition */
    PartitionBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                            DiskGeometry.BytesPerSector,
                                            TAG_IO);
    if (!PartitionBuffer) return FALSE;

    /* Build an IRP to read the partition sector */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       PartitionBuffer,
                                       DiskGeometry.BytesPerSector,
                                       &PartitionOffset,
                                       &Event,
                                       &StatusBlock);

    /* Call the driver and check if we have to wait */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = StatusBlock.Status;
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Try again */
        ExFreePool(PartitionBuffer);
        ExFreePool(DriveLayout);
        return FALSE;
    }

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (j = 0; j < 128; j++) Checksum += PartitionBuffer[j];

    /* Save the signature and checksum */
    *CheckSum = ~Checksum + 1;
    *Signature = DriveLayout->Signature;
    *PartitionCount = DriveLayout->PartitionCount;

    /* Free the buffer */
    ExFreePool(PartitionBuffer);
    ExFreePool(DriveLayout);
    return TRUE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopAssignArcNamesToCdrom(IN PULONG Buffer,
                         IN ULONG DiskNumber)
{
    CHAR ArcBuffer[128];
    ANSI_STRING TempString;
    UNICODE_STRING DeviceName, ArcName;
    NTSTATUS Status;
    LARGE_INTEGER PartitionOffset;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    ULONG i, CheckSum = 0;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;

    /* Build the device name */
    sprintf(ArcBuffer, "\\Device\\CdRom%lu", DiskNumber);

    /* Convert it to Unicode */
    RtlInitAnsiString(&TempString, ArcBuffer);
    Status = RtlAnsiStringToUnicodeString(&DeviceName, &TempString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get the device for it */
    Status = IoGetDeviceObjectPointer(&DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Free the string and fail */
        RtlFreeUnicodeString(&DeviceName);
        return FALSE;
    }

    /* Setup the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Set the offset and build the read IRP */
    PartitionOffset.QuadPart = 0x8000;
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       Buffer,
                                       2048,
                                       &PartitionOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        /* Free the string and fail */
        RtlFreeUnicodeString(&DeviceName);
        return FALSE;
    }

    /* Call the driver and check if we have to wait on it */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Dereference the file object */
    ObDereferenceObject(FileObject);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Now calculate the checksum */
    for (i = 0; i < 2048 / sizeof(ULONG); i++) CheckSum += Buffer[i];

    /*
     * FIXME: In normal conditions, NTLDR/FreeLdr sends the *proper* CDROM
     * ARC Path name, and what happens here is a comparision of both checksums
     * in order to see if this is the actual boot CD.
     *
     * In ReactOS this doesn't currently happen, instead we have a hack on top
     * of this file which scans the CD for the ntoskrnl.exe file, then modifies
     * the LoaderBlock's ARC Path with the right CDROM path. Consequently, we
     * get the same state as if NTLDR had properly booted us, except that we do
     * not actually need to check the signature, since the hack already did the
     * check for ntoskrnl.exe, which is just as good.
     *
     * The signature code stays however, because eventually FreeLDR will work
     * like NTLDR, and, conversly, we do want to be able to be booted by NTLDR.
     */
    if (IopApplyRosCdromArcHack(DiskNumber))
    {
        /* This is the boot CD-ROM, build the ARC name */
        sprintf(ArcBuffer, "\\ArcName\\%s", KeLoaderBlock->ArcBootDeviceName);

        /* Convert it to Unicode */
        RtlInitAnsiString(&TempString, ArcBuffer);
        Status = RtlAnsiStringToUnicodeString(&ArcName, &TempString, TRUE);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Create the symbolic link and free the strings */
        IoAssignArcName(&ArcName, &DeviceName);
        RtlFreeUnicodeString(&ArcName);
        RtlFreeUnicodeString(&DeviceName);

        /* Let caller know that we've found the boot CD */
        return TRUE;
    }

    /* No boot CD found */
    return FALSE;
}

NTSTATUS
INIT_FUNCTION
NTAPI
IopCreateArcNames(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();
    PARC_DISK_INFORMATION ArcDiskInfo = LoaderBlock->ArcDiskInformation;
    CHAR Buffer[128];
    ANSI_STRING ArcBootString, ArcSystemString, ArcString;
    UNICODE_STRING ArcName, BootPath, DeviceName;
    BOOLEAN SingleDisk;
    ULONG i, j, Length;
    PDEVICE_OBJECT DeviceObject;
    ULONG Signature, Checksum, PartitionCount;
    PLIST_ENTRY NextEntry;
    PARC_DISK_SIGNATURE ArcDiskEntry;
    NTSTATUS Status;
    BOOLEAN FoundBoot = FALSE;
    PULONG PartitionBuffer;

    /* Check if we only have one disk on the machine */
    SingleDisk = ArcDiskInfo->DiskSignatureListHead.Flink->Flink ==
                 (&ArcDiskInfo->DiskSignatureListHead);

    /* Create the global HAL partition name */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcHalDeviceName);
    RtlInitAnsiString(&ArcString, Buffer);
    RtlAnsiStringToUnicodeString(&IoArcHalDeviceName, &ArcString, TRUE);

    /* Create the global system partition name */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&ArcString, Buffer);
    RtlAnsiStringToUnicodeString(&IoArcBootDeviceName, &ArcString, TRUE);

    /* Allocate memory for the string */
    Length = strlen(LoaderBlock->ArcBootDeviceName) + sizeof(ANSI_NULL);
    IoLoaderArcBootDeviceName = ExAllocatePoolWithTag(PagedPool,
                                                      Length,
                                                      TAG_IO);
    if (IoLoaderArcBootDeviceName)
    {
        /* Copy the name */
        RtlCopyMemory(IoLoaderArcBootDeviceName,
                      LoaderBlock->ArcBootDeviceName,
                      Length);
    }

    /* Check if we only found a disk, but we're booting from CD-ROM */
    if ((SingleDisk) && strstr(LoaderBlock->ArcBootDeviceName, "cdrom"))
    {
        /* Then disable single-disk mode, since there's a CD drive out there */
        SingleDisk = FALSE;
    }

    /* Build the boot strings */
    RtlInitAnsiString(&ArcBootString, LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&ArcSystemString, LoaderBlock->ArcHalDeviceName);

    /* Loop every detected disk */
    for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
        /* Get information about the disk */
        if (!IopGetDiskInformation(i,
                                   &Checksum,
                                   &Signature,
                                   &PartitionCount,
                                   &DeviceObject))
        {
            /* Skip this disk */
            continue;
        }

        /* Loop ARC disks */
        for (NextEntry = ArcDiskInfo->DiskSignatureListHead.Flink;
             NextEntry != &ArcDiskInfo->DiskSignatureListHead;
             NextEntry = NextEntry->Flink)
        {
            /* Get the current ARC disk signature entry */
            ArcDiskEntry = CONTAINING_RECORD(NextEntry,
                                             ARC_DISK_SIGNATURE,
                                             ListEntry);

            /*
             * Now check if the signature and checksum match, unless this is
             * the only disk that was in the ARC list, and also in the device
             * tree, in which case the check is bypassed and we accept the disk
             */
            if (((SingleDisk) && (ConfigInfo->DiskCount == 1)) ||
                ((Checksum == ArcDiskEntry->CheckSum) &&
                 (Signature == ArcDiskEntry->Signature)))
            {
                /* Build the NT Device Name */
                sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0", i);

                /* Convert it to Unicode */
                RtlInitAnsiString(&ArcString, Buffer);
                Status = RtlAnsiStringToUnicodeString(&DeviceName,
                                                      &ArcString,
                                                      TRUE);
                if (!NT_SUCCESS(Status)) continue;

                /* Build the ARC Device Name */
                sprintf(Buffer, "\\ArcName\\%s", ArcDiskEntry->ArcName);

                /* Convert it to Unicode */
                RtlInitAnsiString(&ArcString, Buffer);
                Status = RtlAnsiStringToUnicodeString(&ArcName,
                                                      &ArcString,
                                                      TRUE);
                if (!NT_SUCCESS(Status)) continue;

                /* Create the symbolic link and free the strings */
                IoAssignArcName(&ArcName, &DeviceName);
                RtlFreeUnicodeString(&ArcName);
                RtlFreeUnicodeString(&DeviceName);

                /* Loop all the partitions */
                for (j = 0; j < PartitionCount; j++)
                {
                    /* Build the partition device name */
                    sprintf(Buffer,
                            "\\Device\\Harddisk%lu\\Partition%lu",
                            i,
                            j + 1);

                    /* Convert it to Unicode */
                    RtlInitAnsiString(&ArcString, Buffer);
                    Status = RtlAnsiStringToUnicodeString(&DeviceName,
                                                          &ArcString,
                                                          TRUE);
                    if (!NT_SUCCESS(Status)) continue;

                    /* Build the partial ARC name for this partition */
                    sprintf(Buffer,
                            "%spartition(%lu)",
                            ArcDiskEntry->ArcName,
                            j + 1);
                    RtlInitAnsiString(&ArcString, Buffer);

                    /* Check if this is the boot device */
                    if (RtlEqualString(&ArcString, &ArcBootString, TRUE))
                    {
                        /* Remember that we found a Hard Disk Boot Device */
                        FoundBoot = TRUE;
                    }

                    /* Check if it's the system boot partition */
                    if (RtlEqualString(&ArcString, &ArcSystemString, TRUE))
                    {
                        /* It is, create a Unicode string for it */
                        RtlInitAnsiString(&ArcString,
                                          LoaderBlock->NtHalPathName);
                        Status = RtlAnsiStringToUnicodeString(&BootPath,
                                                              &ArcString,
                                                              TRUE);
                        if (NT_SUCCESS(Status))
                        {
                            /* FIXME: Save in registry */

                            /* Free the string now */
                            RtlFreeUnicodeString(&BootPath);
                        }
                    }

                    /* Build the full ARC name */
                    sprintf(Buffer,
                            "\\ArcName\\%spartition(%lu)",
                            ArcDiskEntry->ArcName,
                            j + 1);

                    /* Convert it to Unicode */
                    RtlInitAnsiString(&ArcString, Buffer);
                    Status = RtlAnsiStringToUnicodeString(&ArcName,
                                                          &ArcString,
                                                          TRUE);
                    if (!NT_SUCCESS(Status)) continue;

                    /* Create the symbolic link and free the strings */
                    IoAssignArcName(&ArcName, &DeviceName);
                    RtlFreeUnicodeString(&ArcName);
                    RtlFreeUnicodeString(&DeviceName);
                }
            }
        }
    }

    /* Check if we didn't find the boot disk */
    if (!FoundBoot)
    {
        /* Allocate a buffer for the CD-ROM MBR */
        PartitionBuffer = ExAllocatePoolWithTag(NonPagedPool, 2048, TAG_IO);
        if (!PartitionBuffer) return STATUS_INSUFFICIENT_RESOURCES;

        /* Loop every CD-ROM */
        for (i = 0; i < ConfigInfo->CdRomCount; i++)
        {
            /* Give it an ARC name */
            if (IopAssignArcNamesToCdrom(PartitionBuffer, i)) break;
        }

        /* Free the buffer */
        ExFreePool(PartitionBuffer);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopReassignSystemRoot(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      OUT PANSI_STRING NtBootPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    CHAR Buffer[256], AnsiBuffer[256];
    WCHAR ArcNameBuffer[64];
    ANSI_STRING TargetString, ArcString, TempString;
    UNICODE_STRING LinkName, TargetName, ArcName;
    HANDLE LinkHandle;

    /* Create the Unicode name for the current ARC boot device */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&TargetString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&TargetName, &TargetString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize the attributes and open the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               &TargetName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, free the string */
        RtlFreeUnicodeString(&TargetName);
        return FALSE;
    }

    /* Query the current \\SystemRoot */
    ArcName.Buffer = ArcNameBuffer;
    ArcName.Length = 0;
    ArcName.MaximumLength = sizeof(ArcNameBuffer);
    Status = NtQuerySymbolicLinkObject(LinkHandle, &ArcName, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, free the string */
        RtlFreeUnicodeString(&TargetName);
        return FALSE;
    }

    /* Convert it to Ansi */
    ArcString.Buffer = AnsiBuffer;
    ArcString.Length = 0;
    ArcString.MaximumLength = sizeof(AnsiBuffer);
    Status = RtlUnicodeStringToAnsiString(&ArcString, &ArcName, FALSE);
    AnsiBuffer[ArcString.Length] = ANSI_NULL;

    /* Close the link handle and free the name */
    ObCloseHandle(LinkHandle, KernelMode);
    RtlFreeUnicodeString(&TargetName);

    /* Setup the system root name again */
    RtlInitAnsiString(&TempString, "\\SystemRoot");
    Status = RtlAnsiStringToUnicodeString(&LinkName, &TempString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Open the symbolic link for it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Destroy it */
    NtMakeTemporaryObject(LinkHandle);
    ObCloseHandle(LinkHandle, KernelMode);

    /* Now create the new name for it */
    sprintf(Buffer, "%s%s", ArcString.Buffer, LoaderBlock->NtBootPathName);

    /* Copy it into the passed parameter and null-terminate it */
    RtlCopyString(NtBootPath, &ArcString);
    Buffer[strlen(Buffer) - 1] = ANSI_NULL;

    /* Setup the Unicode-name for the new symbolic link value */
    RtlInitAnsiString(&TargetString, Buffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = RtlAnsiStringToUnicodeString(&ArcName, &TargetString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create it */
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &ArcName);

    /* Free all the strings and close the handle and return success */
    RtlFreeUnicodeString(&ArcName);
    RtlFreeUnicodeString(&LinkName);
    ObCloseHandle(LinkHandle, KernelMode);
    return TRUE;
}

/* EOF */
