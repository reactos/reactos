/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/arcname.c
 * PURPOSE:         Creates ARC names for boot devices
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
INIT_FUNCTION
NTAPI
IopApplyRosCdromArcHack(IN ULONG i)
{
    ULONG DeviceNumber = -1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceName;
    WCHAR Buffer[MAX_PATH];
    CHAR AnsiBuffer[MAX_PATH];
    FILE_BASIC_INFORMATION FileInfo;
    NTSTATUS Status;
    PCHAR p, q;

    /* Only ARC Name left - Build full ARC Name */
    p = strstr(KeLoaderBlock->ArcBootDeviceName, "cdrom");
    if (p)
    {
        /* Try to find the installer */
        swprintf(Buffer, L"\\Device\\CdRom%lu\\reactos\\ntoskrnl.exe", i);
        RtlInitUnicodeString(&DeviceName, Buffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   0,
                                   NULL,
                                   NULL);
        Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);
        if (NT_SUCCESS(Status)) DeviceNumber = i;

        /* Try to find live CD boot */
        swprintf(Buffer,
                 L"\\Device\\CdRom%lu\\reactos\\system32\\ntoskrnl.exe",
                 i);
        RtlInitUnicodeString(&DeviceName, Buffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   0,
                                   NULL,
                                   NULL);
        Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);
        if (NT_SUCCESS(Status)) DeviceNumber = i;

        /* Build the name */
        sprintf(p, "cdrom(%lu)", DeviceNumber);

        /* Adjust original command line */
        q = strchr(p, ')');
        if (q)
        {
            q++;
            strcpy(AnsiBuffer, q);
            sprintf(p, "cdrom(%lu)", DeviceNumber);
            strcat(p, AnsiBuffer);
        }
    }

    /* Return whether this is the CD or not */
    if (DeviceNumber != 1) return TRUE;
    return FALSE;
}

VOID
INIT_FUNCTION
NTAPI
IopEnumerateDisks(IN PLIST_ENTRY ListHead)
{
    ULONG i, j;
    ANSI_STRING TempString;
    CHAR Buffer[256];
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
    PPARTITION_SECTOR PartitionBuffer;
    PDISKENTRY DiskEntry;

    /* Loop every detected disk */
    for (i = 0; i < IoGetConfigurationInformation()->DiskCount; i++)
    {
        /* Build the name */
        sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0", i);

        /* Convert it to Unicode */
        RtlInitAnsiString(&TempString, Buffer);
        Status = RtlAnsiStringToUnicodeString(&DeviceName, &TempString, TRUE);
        if (!NT_SUCCESS(Status)) continue;

        /* Get the device pointer */
        Status = IoGetDeviceObjectPointer(&DeviceName,
                                          FILE_READ_DATA,
                                          &FileObject,
                                          &DeviceObject);

        /* Free the string */
        RtlFreeUnicodeString(&DeviceName);

        /* Move on if we failed */
        if (!NT_SUCCESS(Status)) continue;

        /* Allocate the ROS disk Entry */
        DiskEntry = ExAllocatePoolWithTag(PagedPool, sizeof(DISKENTRY), TAG_IO);
        DiskEntry->DiskNumber = i;
        DiskEntry->DeviceObject = DeviceObject;

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
            continue;
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
            continue;
        }

        /* Read the partition table */
        Status = IoReadPartitionTable(DeviceObject,
                                      DiskGeometry.BytesPerSector,
                                      TRUE,
                                      &DriveLayout);

        /* Dereference the file object */
        ObDereferenceObject(FileObject);
        if (!NT_SUCCESS(Status)) continue;

        /* Set the offset to 0 */
        PartitionOffset.QuadPart = 0;

        /* Allocate a buffer for the partition */
        PartitionBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                DiskGeometry.BytesPerSector,
                                                TAG_IO);
        if (!PartitionBuffer) continue;

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
            continue;
        }

        /* Calculate the MBR checksum */
        DiskEntry->Checksum = 0;
        for (j = 0; j < 128; j++)
        {
            DiskEntry->Checksum += ((PULONG)PartitionBuffer)[j];
        }

        /* Save the signature and checksum */
        DiskEntry->Checksum = ~DiskEntry->Checksum + 1;
        DiskEntry->Signature = DriveLayout->Signature;
        DiskEntry->PartitionCount = DriveLayout->PartitionCount;

        /* Insert it into the list */
        InsertTailList(ListHead, &DiskEntry->ListEntry);

        /* Free the buffer */
        ExFreePool(PartitionBuffer);
        ExFreePool(DriveLayout);
    }
}

NTSTATUS
INIT_FUNCTION
NTAPI
IopAssignArcNamesToDisk(IN PDEVICE_OBJECT DeviceObject,
                        IN PCHAR BootArcName,
                        IN ULONG DiskNumber,
                        IN ULONG PartitionCount,
                        IN PBOOLEAN FoundHdBoot)
{
    CHAR Buffer[256];
    CHAR ArcBuffer[256];
    ANSI_STRING TempString, ArcNameString, BootString;
    ANSI_STRING ArcBootString, ArcSystemString;
    UNICODE_STRING DeviceName, ArcName, BootPath;
    ULONG i;
    NTSTATUS Status;

    /* Set default */
    *FoundHdBoot = FALSE;

    /* Build the boot strings */
    RtlInitAnsiString(&ArcBootString, KeLoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&ArcSystemString, KeLoaderBlock->ArcHalDeviceName);

    /* Build the NT Device Name */
    sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition0", DiskNumber);

    /* Convert it to unicode */
    RtlInitAnsiString(&TempString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&DeviceName, &TempString, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Build the ARC Device Name */
    sprintf(ArcBuffer, "\\ArcName\\%s", BootArcName);

    /* Convert it to Unicode */
    RtlInitAnsiString(&ArcNameString, ArcBuffer);
    Status = RtlAnsiStringToUnicodeString(&ArcName, &ArcNameString, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the symbolic link and free the strings */
    IoAssignArcName(&ArcName, &DeviceName);
    RtlFreeUnicodeString(&ArcName);
    RtlFreeUnicodeString(&DeviceName);

    /* Loop all the partitions */
    for (i = 0; i < PartitionCount; i++)
    {
        /* Build the partition device name */
        sprintf(Buffer, "\\Device\\Harddisk%lu\\Partition%lu", DiskNumber, i+1);

        /* Convert it to Unicode */
        RtlInitAnsiString(&TempString, Buffer);
        Status = RtlAnsiStringToUnicodeString(&DeviceName, &TempString, TRUE);
        if (!NT_SUCCESS(Status)) continue;

        /* Build the partial ARC name for this partition */
        sprintf(ArcBuffer, "%spartition(%lu)", BootArcName, i + 1);
        RtlInitAnsiString(&ArcNameString, ArcBuffer);

        /* Check if this is the boot device */
        if (RtlEqualString(&ArcNameString, &ArcBootString, TRUE))
        {
            /* Remember that we found a Hard Disk Boot Device */
            *FoundHdBoot = TRUE;
        }

        /* Check if it's the system boot partition */
        if (RtlEqualString(&ArcNameString, &ArcSystemString, TRUE))
        {
            /* It is, create a Unicode string for it */
            RtlInitAnsiString(&BootString, KeLoaderBlock->NtHalPathName);
            Status = RtlAnsiStringToUnicodeString(&BootPath, &BootString, TRUE);
            if (NT_SUCCESS(Status))
            {
                /* FIXME: Save in registry */

                /* Free the string now */
                RtlFreeUnicodeString(&BootPath);
            }
        }

        /* Build the full ARC name */
        sprintf(Buffer, "\\ArcName\\%spartition(%lu)", BootArcName, i + 1);

        /* Convert it to Unicode */
        RtlInitAnsiString(&ArcNameString, Buffer);
        Status = RtlAnsiStringToUnicodeString(&ArcName, &ArcNameString, TRUE);
        if (!NT_SUCCESS(Status)) continue;

        /* Create the symbolic link and free the strings */
        IoAssignArcName(&ArcName, &DeviceName);
        RtlFreeUnicodeString(&ArcName);
        RtlFreeUnicodeString(&DeviceName);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopAssignArcNamesToCdrom(IN PULONG Buffer,
                         IN ULONG DiskNumber)
{
    CHAR ArcBuffer[256];
    ANSI_STRING TempString, ArcNameString;
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
        RtlInitAnsiString(&ArcNameString, ArcBuffer);
        Status = RtlAnsiStringToUnicodeString(&ArcName, &ArcNameString, TRUE);
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

NTSTATUS INIT_FUNCTION
IoCreateArcNames(VOID)
{
    PCONFIGURATION_INFORMATION ConfigInfo;
    ULONG i;
    NTSTATUS Status;
    PLIST_ENTRY BiosDiskListHead, Entry;
    LIST_ENTRY DiskListHead;
    PARC_DISK_SIGNATURE ArcDiskEntry;
    PDISKENTRY DiskEntry;
    BOOLEAN FoundBoot = FALSE;
    PULONG Buffer;

    ConfigInfo = IoGetConfigurationInformation();

    /* Get the boot ARC disk list */
    BiosDiskListHead = &KeLoaderBlock->ArcDiskInformation->
                        DiskSignatureListHead;

    /* Enumerate system disks */
    InitializeListHead(&DiskListHead);
    IopEnumerateDisks(&DiskListHead);

    while (!IsListEmpty(BiosDiskListHead))
    {
        Entry = RemoveHeadList(BiosDiskListHead);
        ArcDiskEntry = CONTAINING_RECORD(Entry, ARC_DISK_SIGNATURE, ListEntry);
        Entry = DiskListHead.Flink;
        while (Entry != &DiskListHead)
        {
            DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
            DPRINT1("Entry: %s\n", ArcDiskEntry->ArcName);
            if (DiskEntry->Checksum == ArcDiskEntry->CheckSum &&
                DiskEntry->Signature == ArcDiskEntry->Signature)
            {
                Status = IopAssignArcNamesToDisk(DiskEntry->DeviceObject,
                                                 ArcDiskEntry->ArcName,
                                                 DiskEntry->DiskNumber,
                                                 DiskEntry->PartitionCount,
                                                 &FoundBoot);

                RemoveEntryList(&DiskEntry->ListEntry);
                ExFreePool(DiskEntry);
                break;
            }
            Entry = Entry->Flink;
        }
    }

    while (!IsListEmpty(&DiskListHead))
    {
        Entry = RemoveHeadList(&DiskListHead);
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        ExFreePool(DiskEntry);
    }

    /* Check if we didn't find the boot disk */
    if (!FoundBoot)
    {
        /* Allocate a buffer for the CD-ROM MBR */
        Buffer = ExAllocatePoolWithTag(NonPagedPool, 2048, TAG_IO);
        if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

        /* Loop every CD-ROM */
        for (i = 0; i < ConfigInfo->CdRomCount; i++)
        {
            /* Give it an ARC name */
            if (IopAssignArcNamesToCdrom(Buffer, i)) break;
        }

        /* Free the buffer */
        ExFreePool(Buffer);
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
