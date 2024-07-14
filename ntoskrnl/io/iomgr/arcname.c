/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/io/iomgr/arcname.c
* PURPOSE:         ARC Path Initialization Functions
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Eric Kohl
*                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

UNICODE_STRING IoArcHalDeviceName, IoArcBootDeviceName;
PCHAR IoLoaderArcBootDeviceName;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNamesCd(IN PLOADER_PARAMETER_BLOCK LoaderBlock);

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNamesDisk(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN BOOLEAN SingleDisk,
                      OUT PBOOLEAN FoundBoot);

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNames(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    SIZE_T Length;
    NTSTATUS Status;
    CHAR Buffer[128];
    BOOLEAN SingleDisk;
    BOOLEAN FoundBoot = FALSE;
    UNICODE_STRING SystemDevice, LoaderPathNameW, BootDeviceName;
    PARC_DISK_INFORMATION ArcDiskInfo = LoaderBlock->ArcDiskInformation;
    ANSI_STRING ArcSystemString, ArcString, LanmanRedirector, LoaderPathNameA;

    /* Check if we only have one disk on the machine */
    SingleDisk = (ArcDiskInfo->DiskSignatureListHead.Flink->Flink ==
                 &ArcDiskInfo->DiskSignatureListHead);

    /* Create the global HAL partition name */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcHalDeviceName);
    RtlInitAnsiString(&ArcString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&IoArcHalDeviceName, &ArcString, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Create the global system partition name */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&ArcString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&IoArcBootDeviceName, &ArcString, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

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
    RtlInitAnsiString(&ArcSystemString, LoaderBlock->ArcHalDeviceName);

    /* If we are doing remote booting */
    if (IoRemoteBootClient)
    {
        /* Yes, we have found boot device */
        FoundBoot = TRUE;

        /* Get NT device name */
        RtlInitAnsiString(&LanmanRedirector, "\\Device\\LanmanRedirector");
        Status = RtlAnsiStringToUnicodeString(&SystemDevice, &LanmanRedirector, TRUE);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Get ARC booting device name (in net(0) something) */
        sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
        RtlInitAnsiString(&ArcString, Buffer);
        Status = RtlAnsiStringToUnicodeString(&BootDeviceName, &ArcString, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Map ARC to NT name */
            IoAssignArcName(&BootDeviceName, &SystemDevice);
            RtlFreeUnicodeString(&BootDeviceName);

            /* Now, get loader path name */
            RtlInitAnsiString(&LoaderPathNameA, LoaderBlock->NtHalPathName);
            Status = RtlAnsiStringToUnicodeString(&LoaderPathNameW, &LoaderPathNameA, TRUE);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeUnicodeString(&SystemDevice);
                return Status;
            }

            /* And set it has system partition */
            IopStoreSystemPartitionInformation(&SystemDevice, &LoaderPathNameW);
        }

        RtlFreeUnicodeString(&SystemDevice);

        /* Don't quit here, even if everything went fine!
         * We need IopCreateArcNamesDisk to properly map
         * devices with symlinks.
         * It will return success if the mapping process went fine
         * even if it didn't find boot device.
         * It won't reset boot device finding status as well.
         */
    }

    /* Loop every disk and try to find boot disk */
    Status = IopCreateArcNamesDisk(LoaderBlock, SingleDisk, &FoundBoot);
    /* If it succeeded but we didn't find boot device, try to browse Cds */
    if (NT_SUCCESS(Status) && !FoundBoot)
    {
        Status = IopCreateArcNamesCd(LoaderBlock);
    }

    /* Return success */
    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNamesCd(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    LARGE_INTEGER StartingOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    PULONG PartitionBuffer = NULL;
    CHAR Buffer[128], ArcBuffer[128];
    BOOLEAN NotEnabledPresent = FALSE;
    STORAGE_DEVICE_NUMBER DeviceNumber;
    ANSI_STRING DeviceStringA, ArcNameStringA;
    PWSTR SymbolicLinkList, lSymbolicLinkList;
    PARC_DISK_SIGNATURE ArcDiskSignature = NULL;
    UNICODE_STRING DeviceStringW, ArcNameStringW;
    ULONG DiskNumber, CdRomCount, CheckSum, i, EnabledDisks = 0;
    PARC_DISK_INFORMATION ArcDiskInformation = LoaderBlock->ArcDiskInformation;

    /* Get all the Cds present in the system */
    CdRomCount = IoGetConfigurationInformation()->CdRomCount;

    /* Get enabled Cds and check if result matches
     * For the record, enabled Cds (or even disk) are Cds/disks
     * that have been successfully handled by MountMgr driver
     * and that already own their device name. This is the "new" way
     * to handle them, that came with NT5.
     * Currently, Windows 2003 provides an ARC names creation based
     * on both enabled drives and not enabled drives (lack from
     * the driver).
     * Given the current ReactOS state, that's good for us.
     * To sum up, this is NOT a hack or whatsoever.
     */
    Status = IopFetchConfigurationInformation(&SymbolicLinkList,
                                              GUID_DEVINTERFACE_CDROM,
                                              CdRomCount,
                                              &EnabledDisks);
    if (!NT_SUCCESS(Status))
    {
        NotEnabledPresent = TRUE;
    }
    /* Save symbolic link list address in order to free it after */
    lSymbolicLinkList = SymbolicLinkList;
    /* For the moment, we won't fail */
    Status = STATUS_SUCCESS;

    /* Browse all the ARC devices trying to find the one matching boot device */
    for (NextEntry = ArcDiskInformation->DiskSignatureListHead.Flink;
         NextEntry != &ArcDiskInformation->DiskSignatureListHead;
         NextEntry = NextEntry->Flink)
    {
        ArcDiskSignature = CONTAINING_RECORD(NextEntry,
                                             ARC_DISK_SIGNATURE,
                                             ListEntry);

        if (strcmp(LoaderBlock->ArcBootDeviceName, ArcDiskSignature->ArcName) == 0)
        {
            break;
        }

        ArcDiskSignature = NULL;
    }

    /* Not found... Not booting from a Cd */
    if (!ArcDiskSignature)
    {
        DPRINT("Failed finding a cd that could match current boot device\n");
        goto Cleanup;
    }

    /* Allocate needed space for reading Cd */
    PartitionBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, 2048, TAG_IO);
    if (!PartitionBuffer)
    {
        DPRINT("Failed allocating resources!\n");
        /* Here, we fail, BUT we return success, some Microsoft joke */
        goto Cleanup;
    }

    /* If we have more enabled Cds, take that into account */
    if (EnabledDisks > CdRomCount)
    {
        CdRomCount = EnabledDisks;
    }

    /* If we'll have to browse for none enabled Cds, fix higher count */
    if (NotEnabledPresent && !EnabledDisks)
    {
        CdRomCount += 5;
    }

    /* Finally, if in spite of all that work, we still don't have Cds, leave */
    if (!CdRomCount)
    {
        goto Cleanup;
    }

    /* Start browsing Cds */
    for (DiskNumber = 0, EnabledDisks = 0; DiskNumber < CdRomCount; DiskNumber++)
    {
        /* Check if we have an enabled disk */
        if (lSymbolicLinkList && *lSymbolicLinkList != UNICODE_NULL)
        {
            /* Create its device name using first symbolic link */
            RtlInitUnicodeString(&DeviceStringW, lSymbolicLinkList);
            /* Then, update symbolic links list */
            lSymbolicLinkList += wcslen(lSymbolicLinkList) + (sizeof(UNICODE_NULL) / sizeof(WCHAR));

            /* Get its associated device object and file object */
            Status = IoGetDeviceObjectPointer(&DeviceStringW,
                                              FILE_READ_ATTRIBUTES,
                                              &FileObject,
                                              &DeviceObject);
            /* Failure? Good bye! */
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            /* Now, we'll ask the device its device number */
            Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &DeviceNumber,
                                                sizeof(DeviceNumber),
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            /* Failure? Good bye! */
            if (!Irp)
            {
                /* Dereference file object before leaving */
                ObDereferenceObject(FileObject);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            /* Call the driver, and wait for it if needed */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }
            if (!NT_SUCCESS(Status))
            {
                ObDereferenceObject(FileObject);
                goto Cleanup;
            }

            /* Finally, build proper device name */
            sprintf(Buffer, "\\Device\\CdRom%lu", DeviceNumber.DeviceNumber);
            RtlInitAnsiString(&DeviceStringA, Buffer);
            Status = RtlAnsiStringToUnicodeString(&DeviceStringW, &DeviceStringA, TRUE);
            if (!NT_SUCCESS(Status))
            {
                ObDereferenceObject(FileObject);
                goto Cleanup;
            }
        }
        else
        {
            /* Create device name for the cd */
            sprintf(Buffer, "\\Device\\CdRom%lu", EnabledDisks++);
            RtlInitAnsiString(&DeviceStringA, Buffer);
            Status = RtlAnsiStringToUnicodeString(&DeviceStringW, &DeviceStringA, TRUE);
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            /* Get its device object */
            Status = IoGetDeviceObjectPointer(&DeviceStringW,
                                              FILE_READ_ATTRIBUTES,
                                              &FileObject,
                                              &DeviceObject);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeUnicodeString(&DeviceStringW);
                goto Cleanup;
            }
        }

        /* Initiate data for reading cd and compute checksum */
        StartingOffset.QuadPart = 0x8000;
        CheckSum = 0;
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           PartitionBuffer,
                                           2048,
                                           &StartingOffset,
                                           &Event,
                                           &IoStatusBlock);
        if (Irp)
        {
            /* Call the driver, and wait for it if needed */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }

            /* If reading succeeded, compute checksum by adding data, 2048 bytes checksum */
            if (NT_SUCCESS(Status))
            {
                for (i = 0; i < 2048 / sizeof(ULONG); i++)
                {
                    CheckSum += PartitionBuffer[i];
                }
            }
        }

        /* Dereference file object */
        ObDereferenceObject(FileObject);

        /* If checksums are matching, we have the proper cd */
        if (CheckSum + ArcDiskSignature->CheckSum == 0)
        {
            /* Create ARC name */
            sprintf(ArcBuffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
            RtlInitAnsiString(&ArcNameStringA, ArcBuffer);
            Status = RtlAnsiStringToUnicodeString(&ArcNameStringW, &ArcNameStringA, TRUE);
            if (NT_SUCCESS(Status))
            {
                /* Create symbolic link */
                IoAssignArcName(&ArcNameStringW, &DeviceStringW);
                RtlFreeUnicodeString(&ArcNameStringW);
                DPRINT("Boot device found\n");
            }

            /* And quit, whatever happens */
            RtlFreeUnicodeString(&DeviceStringW);
            goto Cleanup;
        }

        /* Free string before trying another disk */
        RtlFreeUnicodeString(&DeviceStringW);
    }

Cleanup:
    if (PartitionBuffer)
    {
        ExFreePoolWithTag(PartitionBuffer, TAG_IO);
    }

    if (SymbolicLinkList)
    {
        ExFreePool(SymbolicLinkList);
    }

    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopCreateArcNamesDisk(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN BOOLEAN SingleDisk,
                      OUT PBOOLEAN FoundBoot)
{
    PIRP Irp;
    PVOID Data;
    KEVENT Event;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT FileObject;
    DISK_GEOMETRY DiskGeometry;
    PDEVICE_OBJECT DeviceObject;
    LARGE_INTEGER StartingOffset;
    PULONG PartitionBuffer = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    CHAR Buffer[128], ArcBuffer[128];
    BOOLEAN NotEnabledPresent = FALSE;
    STORAGE_DEVICE_NUMBER DeviceNumber;
    PARC_DISK_SIGNATURE ArcDiskSignature;
    PWSTR SymbolicLinkList, lSymbolicLinkList;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = NULL;
    UNICODE_STRING DeviceStringW, ArcNameStringW, HalPathStringW;
    ULONG DiskNumber, DiskCount, CheckSum, i, Signature, EnabledDisks = 0;
    PARC_DISK_INFORMATION ArcDiskInformation = LoaderBlock->ArcDiskInformation;
    ANSI_STRING ArcBootString, ArcSystemString, DeviceStringA, ArcNameStringA, HalPathStringA;

    /* Initialise device number */
    DeviceNumber.DeviceNumber = ULONG_MAX;
    /* Get all the disks present in the system */
    DiskCount = IoGetConfigurationInformation()->DiskCount;

    /* Get enabled disks and check if result matches */
    Status = IopFetchConfigurationInformation(&SymbolicLinkList,
                                              GUID_DEVINTERFACE_DISK,
                                              DiskCount,
                                              &EnabledDisks);
    if (!NT_SUCCESS(Status))
    {
        NotEnabledPresent = TRUE;
    }

    /* Save symbolic link list address in order to free it after */
    lSymbolicLinkList = SymbolicLinkList;

    /* Build the boot strings */
    RtlInitAnsiString(&ArcBootString, LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&ArcSystemString, LoaderBlock->ArcHalDeviceName);

    /* If we have more enabled disks, take that into account */
    if (EnabledDisks > DiskCount)
    {
        DiskCount = EnabledDisks;
    }

    /* If we'll have to browse for none enabled disks, fix higher count */
    if (NotEnabledPresent && !EnabledDisks)
    {
        DiskCount += 20;
    }

    /* Finally, if in spite of all that work, we still don't have disks, leave */
    if (!DiskCount)
    {
        goto Cleanup;
    }

    /* Start browsing disks */
    for (DiskNumber = 0; DiskNumber < DiskCount; DiskNumber++)
    {
        ASSERT(DriveLayout == NULL);

        /* Check if we have an enabled disk */
        if (lSymbolicLinkList && *lSymbolicLinkList != UNICODE_NULL)
        {
            /* Create its device name using first symbolic link */
            RtlInitUnicodeString(&DeviceStringW, lSymbolicLinkList);
            /* Then, update symbolic links list */
            lSymbolicLinkList += wcslen(lSymbolicLinkList) + (sizeof(UNICODE_NULL) / sizeof(WCHAR));

            /* Get its associated device object and file object */
            Status = IoGetDeviceObjectPointer(&DeviceStringW,
                                              FILE_READ_ATTRIBUTES,
                                              &FileObject,
                                              &DeviceObject);
            if (NT_SUCCESS(Status))
            {
                /* Now, we'll ask the device its device number */
                Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                    DeviceObject,
                                                    NULL,
                                                    0,
                                                    &DeviceNumber,
                                                    sizeof(DeviceNumber),
                                                    FALSE,
                                                    &Event,
                                                    &IoStatusBlock);
                /* Missing resources is a shame... No need to go farther */
                if (!Irp)
                {
                    ObDereferenceObject(FileObject);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                /* Call the driver, and wait for it if needed */
                KeInitializeEvent(&Event, NotificationEvent, FALSE);
                Status = IoCallDriver(DeviceObject, Irp);
                if (Status == STATUS_PENDING)
                {
                    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                    Status = IoStatusBlock.Status;
                }

                /* If we didn't get the appropriate data, just skip that disk */
                if (!NT_SUCCESS(Status))
                {
                   ObDereferenceObject(FileObject);
                   continue;
                }
            }

            /* End of enabled disks enumeration */
            if (NotEnabledPresent && *lSymbolicLinkList == UNICODE_NULL)
            {
                /* No enabled disk worked, reset field */
                if (DeviceNumber.DeviceNumber == ULONG_MAX)
                {
                    DeviceNumber.DeviceNumber = 0;
                }

                /* Update disk number to enable the following not enabled disks */
                if (DeviceNumber.DeviceNumber > DiskNumber)
                {
                    DiskNumber = DeviceNumber.DeviceNumber;
                }

                /* Increase a bit more */
                DiskCount = DiskNumber + 20;
            }
        }
        else
        {
            /* Create device name for the disk */
            sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0", DiskNumber);
            RtlInitAnsiString(&DeviceStringA, Buffer);
            Status = RtlAnsiStringToUnicodeString(&DeviceStringW, &DeviceStringA, TRUE);
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            /* Get its device object */
            Status = IoGetDeviceObjectPointer(&DeviceStringW,
                                              FILE_READ_ATTRIBUTES,
                                              &FileObject,
                                              &DeviceObject);

            RtlFreeUnicodeString(&DeviceStringW);
            /* This is a security measure, to ensure DiskNumber will be used */
            DeviceNumber.DeviceNumber = ULONG_MAX;
        }

        /* Something failed somewhere earlier, just skip the disk */
        if (!NT_SUCCESS(Status))
        {
            continue;
        }

        /* Let's ask the disk for its geometry */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            &DiskGeometry,
                                            sizeof(DiskGeometry),
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        /* Missing resources is a shame... No need to go farther */
        if (!Irp)
        {
            ObDereferenceObject(FileObject);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        /* Call the driver, and wait for it if needed */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
        /* Failure, skip disk */
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            continue;
        }

        /* Read the partition table */
        Status = IoReadPartitionTableEx(DeviceObject,
                                        &DriveLayout);
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            continue;
        }

        /* Ensure we have at least 512 bytes per sector */
        if (DiskGeometry.BytesPerSector < 512)
        {
            DiskGeometry.BytesPerSector = 512;
        }

        /* Check MBR type against EZ Drive type */
        StartingOffset.QuadPart = 0;
        HalExamineMBR(DeviceObject, DiskGeometry.BytesPerSector, PARTITION_EZDRIVE, &Data);
        if (Data)
        {
            /* If MBR is of the EZ Drive type, we'll read after it */
            StartingOffset.QuadPart = DiskGeometry.BytesPerSector;
            ExFreePool(Data);
        }

        /* Allocate for reading enough data for checksum */
        PartitionBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned, DiskGeometry.BytesPerSector, TAG_IO);
        if (!PartitionBuffer)
        {
            ObDereferenceObject(FileObject);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        /* Read the first sector for computing checksum */
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           PartitionBuffer,
                                           DiskGeometry.BytesPerSector,
                                           &StartingOffset,
                                           &Event,
                                           &IoStatusBlock);
        if (!Irp)
        {
            ExFreePoolWithTag(PartitionBuffer, TAG_IO);
            ObDereferenceObject(FileObject);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        /* Call the driver to perform reading */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* If reading succeeded, calculate checksum by adding data */
        if (NT_SUCCESS(Status))
        {
            for (i = 0, CheckSum = 0; i < 512 / sizeof(ULONG); i++)
            {
                CheckSum += PartitionBuffer[i];
            }
        }

        /* Release now unnecessary resources */
        ExFreePoolWithTag(PartitionBuffer, TAG_IO);
        ObDereferenceObject(FileObject);

        /* If we failed, release drive layout before going to next disk */
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(DriveLayout);
            DriveLayout = NULL;
            continue;
        }

        /* Browse each ARC disk */
        for (NextEntry = ArcDiskInformation->DiskSignatureListHead.Flink;
             NextEntry != &ArcDiskInformation->DiskSignatureListHead;
             NextEntry = NextEntry->Flink)
        {
            ArcDiskSignature = CONTAINING_RECORD(NextEntry,
                                                 ARC_DISK_SIGNATURE,
                                                 ListEntry);

            /*
             * If this is the only MBR disk in the ARC list and detected
             * in the device tree, just go ahead and create the ArcName link.
             * Otherwise, check whether the signatures and checksums match
             * before creating the ArcName link.
             */
            if ((SingleDisk && (DiskCount == 1) &&
                 (DriveLayout->PartitionStyle == PARTITION_STYLE_MBR)) ||
                (IopVerifyDiskSignature(DriveLayout, ArcDiskSignature, &Signature) &&
                 (ArcDiskSignature->CheckSum + CheckSum == 0)))
            {
                /* Create device name */
                sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0",
                        (DeviceNumber.DeviceNumber != ULONG_MAX) ? DeviceNumber.DeviceNumber : DiskNumber);
                RtlInitAnsiString(&DeviceStringA, Buffer);
                Status = RtlAnsiStringToUnicodeString(&DeviceStringW, &DeviceStringA, TRUE);
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                /* Create ARC name */
                sprintf(ArcBuffer, "\\ArcName\\%s", ArcDiskSignature->ArcName);
                RtlInitAnsiString(&ArcNameStringA, ArcBuffer);
                Status = RtlAnsiStringToUnicodeString(&ArcNameStringW, &ArcNameStringA, TRUE);
                if (!NT_SUCCESS(Status))
                {
                    RtlFreeUnicodeString(&DeviceStringW);
                    goto Cleanup;
                }

                /* Link both */
                IoAssignArcName(&ArcNameStringW, &DeviceStringW);

                /* And release strings */
                RtlFreeUnicodeString(&ArcNameStringW);
                RtlFreeUnicodeString(&DeviceStringW);

                /* Now, browse each partition */
                for (i = 1; i <= DriveLayout->PartitionCount; i++)
                {
                    /* Create device name */
                    sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition%lu",
                            (DeviceNumber.DeviceNumber != ULONG_MAX) ? DeviceNumber.DeviceNumber : DiskNumber, i);
                    RtlInitAnsiString(&DeviceStringA, Buffer);
                    Status = RtlAnsiStringToUnicodeString(&DeviceStringW, &DeviceStringA, TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }

                    /* Create partial ARC name */
                    sprintf(ArcBuffer, "%spartition(%lu)", ArcDiskSignature->ArcName, i);
                    RtlInitAnsiString(&ArcNameStringA, ArcBuffer);

                    /* Is that boot device? */
                    if (RtlEqualString(&ArcNameStringA, &ArcBootString, TRUE))
                    {
                        DPRINT("Found boot device\n");
                        *FoundBoot = TRUE;
                    }

                    /* Is that system partition? */
                    if (RtlEqualString(&ArcNameStringA, &ArcSystemString, TRUE))
                    {
                        /* Create HAL path name */
                        RtlInitAnsiString(&HalPathStringA, LoaderBlock->NtHalPathName);
                        Status = RtlAnsiStringToUnicodeString(&HalPathStringW, &HalPathStringA, TRUE);
                        if (!NT_SUCCESS(Status))
                        {
                            RtlFreeUnicodeString(&DeviceStringW);
                            goto Cleanup;
                        }

                        /* Then store those information to registry */
                        IopStoreSystemPartitionInformation(&DeviceStringW, &HalPathStringW);
                        RtlFreeUnicodeString(&HalPathStringW);
                    }

                    /* Create complete ARC name */
                    sprintf(ArcBuffer, "\\ArcName\\%spartition(%lu)", ArcDiskSignature->ArcName, i);
                    RtlInitAnsiString(&ArcNameStringA, ArcBuffer);
                    Status = RtlAnsiStringToUnicodeString(&ArcNameStringW, &ArcNameStringA, TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        RtlFreeUnicodeString(&DeviceStringW);
                        goto Cleanup;
                    }

                    /* Link device name & ARC name */
                    IoAssignArcName(&ArcNameStringW, &DeviceStringW);

                    /* Release strings */
                    RtlFreeUnicodeString(&ArcNameStringW);
                    RtlFreeUnicodeString(&DeviceStringW);
                }
            }
            else
            {
                /* Debugging feedback: Warn in case there's a valid partition,
                 * a matching signature, BUT a non-matching checksum: this can
                 * be the sign of a duplicate signature, or even worse a virus
                 * played with the partition table. */
                if (ArcDiskSignature->ValidPartitionTable &&
                    (ArcDiskSignature->Signature == Signature) &&
                    (ArcDiskSignature->CheckSum + CheckSum != 0))
                {
                    DPRINT("Be careful, you have a duplicate disk signature, or a virus altered your MBR!\n");
                }
            }
        }

        /* Finally, release drive layout */
        ExFreePool(DriveLayout);
        DriveLayout = NULL;
    }

    Status = STATUS_SUCCESS;

Cleanup:
    if (DriveLayout)
    {
        ExFreePool(DriveLayout);
    }

    if (SymbolicLinkList)
    {
        ExFreePool(SymbolicLinkList);
    }

    return Status;
}

CODE_SEG("INIT")
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

BOOLEAN
IopVerifyDiskSignature(
    _In_ PDRIVE_LAYOUT_INFORMATION_EX DriveLayout,
    _In_ PARC_DISK_SIGNATURE ArcDiskSignature,
    _Out_ PULONG Signature)
{
    /* Fail if the partition table is invalid */
    if (!ArcDiskSignature->ValidPartitionTable)
        return FALSE;

    /* If the partition style is MBR */
    if (DriveLayout->PartitionStyle == PARTITION_STYLE_MBR)
    {
        /* Check the MBR signature */
        if (DriveLayout->Mbr.Signature == ArcDiskSignature->Signature)
        {
            /* And return it */
            if (Signature)
                *Signature = DriveLayout->Mbr.Signature;
            return TRUE;
        }
    }
    /* If the partition style is GPT */
    else if (DriveLayout->PartitionStyle == PARTITION_STYLE_GPT)
    {
        /* Verify whether the signature is GPT and compare the GUID */
        if (ArcDiskSignature->IsGpt &&
            IsEqualGUID((PGUID)&ArcDiskSignature->GptSignature, &DriveLayout->Gpt.DiskId))
        {
            /* There is no signature to return, just zero it */
            if (Signature)
                *Signature = 0;
            return TRUE;
        }
    }

    /* If we get there, something went wrong, so fail */
    return FALSE;
}

/* EOF */
