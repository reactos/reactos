/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr/efiemu.c
 * PURPOSE:         UEFI Entrypoint for Boot Manager
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* DATA STRUCTURES ***********************************************************/

typedef struct _BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH
{
    BOOT_APPLICATION_PARAMETER_BLOCK;
    BL_MEMORY_DATA BootMemoryData;
    BL_MEMORY_DESCRIPTOR MemEntry;
    UCHAR AppEntry[788];
} BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH;

/* DATA VARIABLES ************************************************************/

BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH EfiInitScratch;

/* FUNCTIONS *****************************************************************/

/*++
 * @name AhCreateLoadOptionsList
 *
 *     The AhCreateLoadOptionsList routine
 *
 * @param  CommandLine
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  BootOptions
 *         Pointer to the UEFI System Table.
 *
 * @param  MaximumLength
 *         Pointer to the UEFI System Table.
 *
 * @param  OptionSize
 *         Pointer to the UEFI System Table.
 *
 * @param  PreviousOption
 *         Pointer to the UEFI System Table.
 *
 * @param  PreviousOptionSize
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
NTSTATUS
AhCreateLoadOptionsList (
    _In_ PWCHAR CommandLine,
    _In_ PBL_BCD_OPTION BootOptions,
    _In_ ULONG MaximumLength,
    _Out_ PULONG OptionSize,
    _In_ PBL_BCD_OPTION* PreviousOption,
    _In_ PULONG PreviousOptionSize
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/*++
 * @name EfiInitpAppendPathString
 *
 *     The EfiInitpAppendPathString routine
 *
 * @param  DestinationPath
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  RemainingSize
 *         Pointer to the UEFI System Table.
 *
 * @param  AppendPath
 *         Pointer to the UEFI System Table.
 *
 * @param  AppendLength
 *         Pointer to the UEFI System Table.
 *
 * @param  BytesAppended
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
NTSTATUS
EfiInitpAppendPathString (
    _In_ PWCHAR PathString,
    _In_ ULONG MaximumLength,
    _In_ PWCHAR NewPathString,
    _In_ ULONG NewPathLength,
    _Out_ PULONG ResultLength
    )
{
    NTSTATUS Status;
    ULONG FinalPathLength;

    /* We deal in Unicode, validate the length */
    if (NewPathLength & 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Is the new element at least a character? */
    Status = STATUS_SUCCESS;
    if (NewPathLength >= sizeof(WCHAR))
    {
        /* Is the last character already a NULL character? */
        if (NewPathString[(NewPathLength - sizeof(WCHAR)) / sizeof(WCHAR)] ==
            UNICODE_NULL)
        {
            /* Then we won't need to count it */
            NewPathLength -= sizeof(UNICODE_NULL);
        }

        /* Was it more than just a NULL character? */
        if (NewPathLength >= sizeof(WCHAR))
        {
            /* Yep -- but does it have a separator? */
            if (*NewPathString == OBJ_NAME_PATH_SEPARATOR)
            {
                /* Skip it, we'll add our own later */
                NewPathString++;
                NewPathLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
            }

            /* Was it more than just a separator? */
            if (NewPathLength >= sizeof(WCHAR))
            {
                /* Yep -- but does it end with a separator? */
                if (NewPathString[(NewPathLength - sizeof(WCHAR)) / sizeof(WCHAR)] ==
                    OBJ_NAME_PATH_SEPARATOR)
                {
                    /* That's something else we won't need for now */
                    NewPathLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
                }
            }
        }
    }

    /* Check if anything needs to be appended after all */
    if (NewPathLength != 0)
    {
        /* We will append the length of the new path element, plus a separator */
        FinalPathLength = NewPathLength + sizeof(OBJ_NAME_PATH_SEPARATOR);
        if (MaximumLength >= FinalPathLength)
        {
            /* Add a separator to the existing path*/
            *PathString = OBJ_NAME_PATH_SEPARATOR;

            /* Followed by the new path element */
            RtlCopyMemory(PathString + 1, NewPathString, NewPathLength);

            /* Return the number of bytes appended */
            *ResultLength = FinalPathLength;
        }
        else
        {
            /* There's not enough space to do this */
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* Nothing to append */
       *ResultLength = 0;
    }

    return Status;
}

/*++
 * @name EfiInitpConvertEfiDevicePath
 *
 *     The EfiInitpConvertEfiDevicePath routine
 *
 * @param  DevicePath
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  DeviceType
 *         Pointer to the UEFI System Table.
 *
 * @param  Option
 *         Pointer to the UEFI System Table.
 *
 * @param  MaximumLength
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
NTSTATUS
EfiInitpConvertEfiFilePath (
    _In_ EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    _In_ ULONG PathType,
    _In_ PBL_BCD_OPTION Option,
    _In_ ULONG MaximumLength
    )
{
    ULONG BytesAppended, DataSize, StringLength;
    PWCHAR StringEntry, PathString;
    FILEPATH_DEVICE_PATH *FilePath;
    NTSTATUS Status;

    /* Make sure we have enough space for the option */
    if (MaximumLength < sizeof(*Option))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set the initial size of the option, and consume from our buffer */
    DataSize = sizeof(*Option);
    MaximumLength -= sizeof(*Option);

    /* Zero out and fill the option header */
    RtlZeroMemory(Option, DataSize);
    Option->Type = PathType;
    Option->DataOffset = sizeof(*Option);

    /* Extract the string option */
    StringEntry = (PWCHAR)(Option + 1);
    PathString = StringEntry;

    /* Start parsing the device path */
    FilePath = (FILEPATH_DEVICE_PATH*)DevicePath;
    while (IsDevicePathEndType(FilePath) == FALSE)
    {
        /* Is this a file path? */
        if ((FilePath->Header.Type == MEDIA_DEVICE_PATH) &&
            (FilePath->Header.SubType == MEDIA_FILEPATH_DP))
        {
            /* Get the length of the file path string, avoiding overflow */
            StringLength = DevicePathNodeLength(FilePath) -
                           FIELD_OFFSET(FILEPATH_DEVICE_PATH, PathName);
            if (StringLength < (ULONG)FIELD_OFFSET(FILEPATH_DEVICE_PATH, PathName))
            {
                Status = STATUS_INTEGER_OVERFLOW;
                goto Quickie;
            }

            /* Append this path string to the current path string */
            Status = EfiInitpAppendPathString(PathString,
                                              MaximumLength,
                                              FilePath->PathName,
                                              StringLength,
                                              &BytesAppended);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Increase the size of the data, consume buffer space */
            DataSize += BytesAppended;
            MaximumLength -= BytesAppended;

            /* Move to the next path string */
            PathString = (PWCHAR)((ULONG_PTR)PathString + BytesAppended);
        }

        /* Move to the next path node */
        FilePath = (FILEPATH_DEVICE_PATH*)NextDevicePathNode(FilePath);
    }

    /* Check if we still have space for a NULL-terminator */
    if (MaximumLength < sizeof(UNICODE_NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* We do -- NULL-terminate the string */
    *PathString = UNICODE_NULL;
    DataSize += sizeof(UNICODE_NULL);

    /* Check if all of this has amounted to a single NULL-char */
    if (PathString == StringEntry)
    {
        /* Then this option is empty */
        Option->Empty = TRUE;
    }

    /* Set the final size of the option */
    Option->DataSize = DataSize;

Quickie:
    return STATUS_SUCCESS;
}

/*++
 * @name EfiInitpGetDeviceNode
 *
 *     The EfiInitpGetDeviceNode routine
 *
 * @param  DevicePath
 *         UEFI Image Handle for the current loaded application.
 *
 * @return None
 *
 *--*/
EFI_DEVICE_PATH_PROTOCOL*
EfiInitpGetDeviceNode (
    _In_ EFI_DEVICE_PATH_PROTOCOL *DevicePath
    )
{
    EFI_DEVICE_PATH_PROTOCOL* NextPath;

    /* Check if we hit the end terminator */
    if (IsDevicePathEndType(DevicePath))
    {
        return DevicePath;
    }

    /* Loop each device path, until we get to the end or to a file path device node */
    for ((NextPath = NextDevicePathNode(DevicePath));
         !(IsDevicePathEndType(NextPath)) && ((NextPath->Type != MEDIA_DEVICE_PATH) ||
                                              (NextPath->SubType != MEDIA_FILEPATH_DP));
         (NextPath = NextDevicePathNode(NextPath)))
    {
        /* Keep iterating down */
        DevicePath = NextPath;
    }

    /* Return the path found */
    return DevicePath;
}

/*++
 * @name EfiInitTranslateDevicePath
 *
 *     The EfiInitTranslateDevicePath routine
 *
 * @param  DevicePath
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  DeviceEntry
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
NTSTATUS
EfiInitTranslateDevicePath (
    _In_ EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    _In_ PBL_DEVICE_DESCRIPTOR DeviceEntry
    )
{
    NTSTATUS Status;
    EFI_DEVICE_PATH_PROTOCOL* DeviceNode;
    MEMMAP_DEVICE_PATH* MemDevicePath;
    ACPI_HID_DEVICE_PATH *AcpiPath;
    HARDDRIVE_DEVICE_PATH *DiskPath;

    /* Assume failure */
    Status = STATUS_UNSUCCESSFUL;

    /* Set size first */
    DeviceEntry->Size = sizeof(*DeviceEntry);

    /* Check if we are booting from a RAM Disk */
    if ((DevicePath->Type == HARDWARE_DEVICE_PATH) &&
        (DevicePath->SubType == HW_MEMMAP_DP))
    {
        /* Get the EFI data structure matching this */
        MemDevicePath = (MEMMAP_DEVICE_PATH*)DevicePath;

        /* Set the boot library specific device types */
        DeviceEntry->DeviceType = LocalDevice;
        DeviceEntry->Local.Type = RamDiskDevice;

        /* Extract the base, size, and offset */
        DeviceEntry->Local.RamDisk.ImageBase.QuadPart = MemDevicePath->StartingAddress;
        DeviceEntry->Local.RamDisk.ImageSize.QuadPart = MemDevicePath->EndingAddress -
                                                        MemDevicePath->StartingAddress;
        DeviceEntry->Local.RamDisk.ImageOffset = 0;
        return STATUS_SUCCESS;
    }

    /* Otherwise, check what kind of device node this is */
    DeviceNode = EfiInitpGetDeviceNode(DevicePath);
    switch (DeviceNode->Type)
    {
        /* ACPI */
        case ACPI_DEVICE_PATH:

            /* We only support floppy drives */
            AcpiPath = (ACPI_HID_DEVICE_PATH*)DeviceNode;
            if ((AcpiPath->HID != EISA_PNP_ID(0x604)) &&
                (AcpiPath->HID != EISA_PNP_ID(0x700)))
            {
                return Status;
            }

            /* Set the boot library specific device types */
            DeviceEntry->DeviceType = LocalDevice;
            DeviceEntry->Local.Type = FloppyDevice;

            /* The ACPI UID is the drive number */
            DeviceEntry->Local.FloppyDisk.DriveNumber = AcpiPath->UID;
            return STATUS_SUCCESS;

        /* Network, ATAPI, SCSI, USB */
        case MESSAGING_DEVICE_PATH:

            /* Check if it's network */
            if ((DeviceNode->SubType == MSG_MAC_ADDR_DP) ||
                (DeviceNode->SubType == MSG_IPv4_DP))
            {
                /* Set the boot library specific device types */
                DeviceEntry->DeviceType = UdpDevice;
                DeviceEntry->Remote.Unknown = 256;
                return STATUS_SUCCESS;
            }

            /* Other types should come in as MEDIA_DEVICE_PATH -- Windows assumes this is a floppy */
            DeviceEntry->DeviceType = DiskDevice;
            DeviceEntry->Local.Type = FloppyDevice;
            DeviceEntry->Local.FloppyDisk.DriveNumber = 0;
            return STATUS_SUCCESS;

        /* Disk or CDROM */
        case MEDIA_DEVICE_PATH:

            /* Extract the disk path and check if it's a physical disk */
            DiskPath = (HARDDRIVE_DEVICE_PATH*)DeviceNode;
            if (DeviceNode->SubType == MEDIA_HARDDRIVE_DP)
            {
                /* Check if this is an MBR partition */
                if (DiskPath->SignatureType == SIGNATURE_TYPE_MBR)
                {
                    /* Set that this is a local partition */
                    DeviceEntry->DeviceType = LegacyPartitionDevice;
                    DeviceEntry->Partition.Disk.Type = LocalDevice;

                    DeviceEntry->Partition.Disk.HardDisk.PartitionType = MbrPartition;
                    DeviceEntry->Partition.Disk.HardDisk.Mbr.PartitionSignature =
                        *(PULONG)&DiskPath->Signature[0];
                    DeviceEntry->Partition.Mbr.PartitionNumber = DiskPath->PartitionNumber;
                    return STATUS_SUCCESS;
                }

                /* Check if it's a GPT partition */
                if (DiskPath->SignatureType == SIGNATURE_TYPE_GUID)
                {
                    /* Set that this is a local disk */
                    DeviceEntry->DeviceType = PartitionDevice;
                    DeviceEntry->Partition.Disk.Type = LocalDevice;

                    /* Set GPT partition ID */
                    DeviceEntry->Partition.Disk.HardDisk.PartitionType = GptPartition;

                    /* Copy the signature GUID */
                    RtlCopyMemory(&DeviceEntry->Partition.Gpt.PartitionGuid,
                                  DiskPath->Signature,
                                  sizeof(GUID));

                    DeviceEntry->Flags |= 4u;
                    return STATUS_SUCCESS;
                }

                /* Otherwise, raw boot is not supported */
                DeviceEntry->DeviceType = PartitionDevice;
                DeviceEntry->Partition.Disk.Type = LocalDevice;
                DeviceEntry->Partition.Disk.HardDisk.PartitionType = RawPartition;
                DeviceEntry->Partition.Disk.HardDisk.Raw.DiskNumber = 0;
            }
            else if (DeviceNode->SubType == MEDIA_CDROM_DP)
            {
                /* Set the right type for a CDROM */
                DeviceEntry->DeviceType = DiskDevice;
                DeviceEntry->Local.Type = CdRomDevice;

                /* Set the drive number to zero */
                DeviceEntry->Local.FloppyDisk.DriveNumber = 0;
                return STATUS_SUCCESS;
            }

        /* Fail anything else */
        default:
            break;
    }

    /* Return here only on failure */
    return Status;
}

/*++
 * @name EfiInitpConvertEfiDevicePath
 *
 *     The EfiInitpConvertEfiDevicePath routine
 *
 * @param  DevicePath
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  DeviceType
 *         Pointer to the UEFI System Table.
 *
 * @param  Option
 *         Pointer to the UEFI System Table.
 *
 * @param  MaximumLength
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
NTSTATUS
EfiInitpConvertEfiDevicePath (
    _In_ EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    _In_ ULONG DeviceType,
    _In_ PBL_BCD_OPTION Option,
    _In_ ULONG MaximumLength
    )
{
    PBCD_DEVICE_OPTION BcdDevice;
    NTSTATUS Status;

    /* Make sure we have enough space for the option */
    if (MaximumLength < sizeof(*Option))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Zero out the option */
    RtlZeroMemory(Option, sizeof(*Option));

    /* Make sure we have enough space for the device entry */
    if ((MaximumLength - sizeof(*Option)) <
        (ULONG)FIELD_OFFSET(BCD_DEVICE_OPTION, DeviceDescriptor))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Fill it out */
    BcdDevice = (PBCD_DEVICE_OPTION)(Option + 1);
    Status = EfiInitTranslateDevicePath(DevicePath,
                                        &BcdDevice->DeviceDescriptor);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Fill out the rest of the option structure */
    Option->DataOffset = sizeof(*Option);
    Option->Type = DeviceType;
    Option->DataSize = FIELD_OFFSET(BCD_DEVICE_OPTION, DeviceDescriptor) +
                       BcdDevice->DeviceDescriptor.Size;
    Status = STATUS_SUCCESS;

Quickie:
    return Status;
}

/*++
 * @name EfiInitpCreateApplicationEntry
 *
 *     The EfiInitpCreateApplicationEntry routine
 *
 * @param  SystemTable
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  Entry
 *         Pointer to the UEFI System Table.
 *
 * @param  MaximumLength
 *         Pointer to the UEFI System Table.
 *
 * @param  DevicePath
 *         Pointer to the UEFI System Table.
 *
 * @param  FilePath
 *         Pointer to the UEFI System Table.
 *
 * @param  LoadOptions
 *         Pointer to the UEFI System Table.
 *
 * @param  LoadOptionsSize
 *         Pointer to the UEFI System Table.
 *
 * @param  Flags
 *         Pointer to the UEFI System Table.
 *
 * @param  ResultLength
 *         Pointer to the UEFI System Table.
 *
 * @param  AppEntryDevice
 *         Pointer to the UEFI System Table.
 *
 * @return None
 *
 *--*/
VOID
EfiInitpCreateApplicationEntry (
    __in EFI_SYSTEM_TABLE *SystemTable,
    __in PBL_APPLICATION_ENTRY Entry,
    __in ULONG MaximumLength,
    __in EFI_DEVICE_PATH *DevicePath,
    __in EFI_DEVICE_PATH *FilePath,
    __in PWCHAR LoadOptions,
    __in ULONG LoadOptionsSize,
    __in ULONG Flags,
    __out PULONG ResultLength,
    __out PBL_DEVICE_DESCRIPTOR *AppEntryDevice
    )
{
    PBL_WINDOWS_LOAD_OPTIONS WindowsOptions;
    PWCHAR ObjectString, CommandLine;
    PBL_BCD_OPTION Option, PreviousOption;
    ULONG HeaderSize, TotalOptionSize, Size, CommandLineSize, RemainingSize;
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    GUID ObjectGuid;
    PBCD_DEVICE_OPTION BcdDevice;
    BOOLEAN HaveBinaryOptions, HaveGuid;
    PBL_FILE_PATH_DESCRIPTOR OsPath;
    EFI_DEVICE_PATH *OsDevicePath;

    /* Initialize everything */
    TotalOptionSize = 0;
    *AppEntryDevice = NULL;
    HeaderSize = 0;

    /* Check if the load options are in binary Windows format */
    WindowsOptions = (PBL_WINDOWS_LOAD_OPTIONS)LoadOptions;
    if ((WindowsOptions != NULL) &&
        (LoadOptionsSize >= sizeof(BL_WINDOWS_LOAD_OPTIONS)) &&
        (WindowsOptions->Length >= sizeof(BL_WINDOWS_LOAD_OPTIONS)) &&
        !(strncmp(WindowsOptions->Signature, "WINDOWS", 7)))
    {
        /* They are, so firmware must have loaded us -- extract arguments */
        CommandLine = WindowsOptions->LoadOptions;
        CommandLineSize = LoadOptionsSize - FIELD_OFFSET(BL_WINDOWS_LOAD_OPTIONS,
                                                         LoadOptions);

        /* Remember that we used binary options */
        HaveBinaryOptions = TRUE;
    }
    else
    {
        /* Nope -- so treat them as raw command-line options */
        CommandLine = LoadOptions;
        CommandLineSize = LoadOptionsSize;

        /* No binary options */
        HaveBinaryOptions = FALSE;
    }

    /* EFI uses UTF-16LE, like NT, so convert to characters */
    CommandLineSize /= sizeof(WCHAR);
    if (CommandLineSize != 0)
    {
        /* And check if the options are not NULL-terminated */
        if (wcsnlen(CommandLine, CommandLineSize) == CommandLineSize)
        {
            /* NULL-terminate them */
            CommandLine[CommandLineSize - 1] = UNICODE_NULL;
        }
    }

    /* Begin by making sure we at least have space for the app entry header */
    RemainingSize = MaximumLength;
    if (RemainingSize < sizeof(BL_APPLICATION_ENTRY))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* On exit, return that we've at least consumed this much */
    HeaderSize = FIELD_OFFSET(BL_APPLICATION_ENTRY, BcdData);

    /* Zero out the header, and write down the signature */
    RtlZeroMemory(Entry, sizeof(BL_APPLICATION_ENTRY));
    RtlCopyMemory(Entry->Signature, BL_APP_ENTRY_SIGNATURE, 7);

    /* Check if a BCD object was passed on the command-line */
    ObjectString = wcsstr(CommandLine, L"BCDOBJECT=");
    if (ObjectString != NULL)
    {
        /* Convert the BCD object to a GUID */
        RtlInitUnicodeString(&GuidString, ObjectString + 10);
        RtlGUIDFromString(&GuidString, &ObjectGuid);

        /* Store it in the application entry */
        Entry->Guid = ObjectGuid;

        /* Remember one was passed */
        HaveGuid = TRUE;
    }
    else
    {
        /* Remember that no identifier was passed */
        Entry->Flags |= BL_APPLICATION_ENTRY_FLAG_NO_GUID;
        HaveGuid = FALSE;
    }

    /* At this point, the header is consumed, and we must now handle BCD options */
    RemainingSize -= FIELD_OFFSET(BL_APPLICATION_ENTRY, BcdData);

    /* Convert the device path into a BCD option */
    Status = EfiInitpConvertEfiDevicePath(DevicePath,
                                          BcdLibraryDevice_ApplicationDevice,
                                          &Entry->BcdData,
                                          RemainingSize);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, so mark the option as such and return an empty one */
        Entry->BcdData.Empty = TRUE;
        TotalOptionSize = sizeof(BL_BCD_OPTION);
        goto Quickie;
    }

    /* Extract the device descriptor and return it */
    BcdDevice = (PVOID)((ULONG_PTR)&Entry->BcdData + Entry->BcdData.DataOffset);
    *AppEntryDevice = &BcdDevice->DeviceDescriptor;

    /* Calculate how big this option was and consume that from the buffer */
    TotalOptionSize = BlGetBootOptionSize(&Entry->BcdData);
    RemainingSize -= TotalOptionSize;

    /* Calculate where the next option should go */
    Option = (PVOID)((ULONG_PTR)&Entry->BcdData + TotalOptionSize);

    /* Check if we're PXE booting or not */
    if ((*AppEntryDevice)->DeviceType == UdpDevice)
    {
        /* lol */
        Status = STATUS_NOT_IMPLEMENTED;
    }
    else
    {
        /* Convert the local file path into a BCD option */
        Status = EfiInitpConvertEfiFilePath(FilePath,
                                            BcdLibraryString_ApplicationPath,
                                            Option,
                                            RemainingSize);
    }

    /* Bail out on failure */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* The next option is right after this one */
    Entry->BcdData.NextEntryOffset = TotalOptionSize;

    /* Now compute the size of the next option, and add to the rolling sum */
    Size = BlGetBootOptionSize(Option);
    TotalOptionSize += Size;

    /* Remember the previous option so we can update its next offset */
    PreviousOption = Option;

    /* Consume the option from the buffer */
    RemainingSize -= Size;

    /* Calculate where the next option should go */
    Option = (PVOID)((ULONG_PTR)Option + Size);

    /* Check if we were using binary options without a BCD GUID */
    if ((HaveBinaryOptions) && !(HaveGuid))
    {
        /* Then this means we have to convert the OS paths to BCD too */
        WindowsOptions = (PBL_WINDOWS_LOAD_OPTIONS)LoadOptions;
        OsPath = (PVOID)((ULONG_PTR)WindowsOptions + WindowsOptions->OsPathOffset);

        /* IS the OS path in EFI format? */
        if ((OsPath->Length > (ULONG)FIELD_OFFSET(BL_FILE_PATH_DESCRIPTOR, Path)) &&
            (OsPath->PathType == EfiPath))
        {
            /* Convert the device portion  */
            OsDevicePath = (EFI_DEVICE_PATH*)OsPath->Path;
            Status = EfiInitpConvertEfiDevicePath(OsDevicePath,
                                                  BcdOSLoaderDevice_OSDevice,
                                                  Option,
                                                  RemainingSize);
            if (!NT_SUCCESS(Status))
            {
                goto Quickie;
            }

            /* Update the offset of the previous option */
            PreviousOption->NextEntryOffset = (ULONG_PTR)Option - (ULONG_PTR)&Entry->BcdData;

            /* Now compute the size of the next option, and add to the rolling sum */
            Size = BlGetBootOptionSize(Option);
            TotalOptionSize += Size;

            /* Remember the previous option so we can update its next offset */
            PreviousOption = Option;

            /* Consume the option from the buffer */
            RemainingSize -= Size;

            /* Calculate where the next option should go */
            Option = (PVOID)((ULONG_PTR)Option + Size);

            /* Convert the path option */
            Status = EfiInitpConvertEfiFilePath(OsDevicePath,
                                                BcdOSLoaderString_SystemRoot,
                                                Option,
                                                RemainingSize);
            if (!NT_SUCCESS(Status))
            {
                goto Quickie;
            }

            /* Update the offset of the previous option */
            PreviousOption->NextEntryOffset = (ULONG_PTR)Option - (ULONG_PTR)&Entry->BcdData;

            /* Now compute the size of the next option, and add to the rolling sum */
            Size = BlGetBootOptionSize(Option);
            TotalOptionSize += Size;

            /* Remember the previous option so we can update its next offset */
            PreviousOption = Option;

            /* Consume the option from the buffer */
            RemainingSize -= Size;

            /* Calculate where the next option should go */
            Option = (PVOID)((ULONG_PTR)Option + Size);
        }
    }

    /* Now convert everything else */
    AhCreateLoadOptionsList(CommandLine,
                            &Entry->BcdData,
                            RemainingSize,
                            &TotalOptionSize,
                            &PreviousOption,
                            &Size);

Quickie:
    /* Return the final size */
    *ResultLength = HeaderSize + TotalOptionSize;
}

/*++
 * @name EfiInitCreateInputParametersEx
 *
 *     The EfiInitCreateInputParametersEx routine converts UEFI entrypoint
 *     parameters to the ones expected by Windows Boot Applications
 *
 * @param  ImageHandle
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  SystemTable
 *         Pointer to the UEFI System Table.
 *
 * @return A PBOOT_APPLICATION_PARAMETER_BLOCK structure containing the data
 *         from UEFI, translated to the Boot Library-compatible format.
 *
 *--*/
PBOOT_APPLICATION_PARAMETER_BLOCK
EfiInitCreateInputParametersEx (
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_BOOT_SERVICES* BootServices;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    PBL_FIRMWARE_DESCRIPTOR FirmwareData;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    ULONG FirmwareOffset, ConsumedSize;
    PBL_DEVICE_DESCRIPTOR AppDevice;
    EFI_STATUS Status;

    /* Initialize the header with the signature and version */
    EfiInitScratch.Signature[0] = BOOT_APPLICATION_SIGNATURE_1;
    EfiInitScratch.Signature[1] = BOOT_APPLICATION_SIGNATURE_2;
    EfiInitScratch.Version = BOOT_APPLICATION_VERSION;

    /* Set the image type to x86 */
    EfiInitScratch.ImageType = EFI_IMAGE_MACHINE_IA32;

    /* Set the translation type to physical */
    EfiInitScratch.MemoryTranslationType = BOOT_MEMORY_TRANSLATION_TYPE_PHYSICAL;

    /* Indicate that the data was converted from EFI */
    BlpApplicationFlags |= BL_APPLICATION_FLAG_CONVERTED_FROM_EFI;

    /* Grab the loaded image protocol, which has our base and size */
    BootServices = SystemTable->BootServices;
    Status = BootServices->HandleProtocol(ImageHandle,
                                          &EfiLoadedImageProtocol,
                                          (VOID**)&LoadedImage);
    if (Status != EFI_SUCCESS)
    {
        return NULL;
    }

    /* Capture it in the boot application parameters */
    EfiInitScratch.ImageBase = (ULONG_PTR)LoadedImage->ImageBase;
    EfiInitScratch.ImageSize = (ULONG)LoadedImage->ImageSize;

    /* Now grab our device path protocol, so we can convert the path later on */
    Status = BootServices->HandleProtocol(LoadedImage->DeviceHandle,
                                          &EfiDevicePathProtocol,
                                          (VOID**)&DevicePath);
    if (Status != EFI_SUCCESS)
    {
        return NULL;
    }

    /* The built-in boot memory data comes right after our block */
    EfiInitScratch.MemoryDataOffset =
        FIELD_OFFSET(BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH, BootMemoryData);

    /* Build the boot memory data structure, with 1 descriptor */
    EfiInitScratch.BootMemoryData.Version = BL_MEMORY_DATA_VERSION;
    EfiInitScratch.BootMemoryData.MdListOffset =
        FIELD_OFFSET(BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH, MemEntry) -
        EfiInitScratch.MemoryDataOffset;
    EfiInitScratch.BootMemoryData.DescriptorSize = sizeof(BL_MEMORY_DESCRIPTOR);
    EfiInitScratch.BootMemoryData.DescriptorCount = 1;
    EfiInitScratch.BootMemoryData.DescriptorOffset = FIELD_OFFSET(BL_MEMORY_DESCRIPTOR, BasePage);

    /* Build the memory entry descriptor for this image itself */
    EfiInitScratch.MemEntry.Flags = BlMemoryWriteBack;
    EfiInitScratch.MemEntry.Type = BlLoaderMemory;
    EfiInitScratch.MemEntry.BasePage = EfiInitScratch.ImageBase >> PAGE_SHIFT;
    EfiInitScratch.MemEntry.PageCount = ALIGN_UP_BY(EfiInitScratch.ImageSize, PAGE_SIZE) >> PAGE_SHIFT;

    /* The built-in application entry comes right after the memory descriptor*/
    EfiInitScratch.AppEntryOffset =
        FIELD_OFFSET(BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH, AppEntry);

    /* Go and build it */
    EfiInitpCreateApplicationEntry(SystemTable,
                                   (PBL_APPLICATION_ENTRY)&EfiInitScratch.AppEntry,
                                   sizeof(EfiInitScratch.AppEntry),
                                   DevicePath,
                                   LoadedImage->FilePath,
                                   LoadedImage->LoadOptions,
                                   LoadedImage->LoadOptionsSize,
                                   EfiInitScratch.MemEntry.PageCount,
                                   &ConsumedSize,
                                   &AppDevice);

    /* Boot device information comes right after the application entry */
    EfiInitScratch.BootDeviceOffset = ConsumedSize + EfiInitScratch.AppEntryOffset;

    /* Check if we have a boot device */
    if (AppDevice != NULL)
    {
        /* We do -- copy it */
        RtlCopyMemory(EfiInitScratch.AppEntry + ConsumedSize,
                      AppDevice,
                      AppDevice->Size);

        /* Firmware data follows right after the boot device entry */
        FirmwareOffset = AppDevice->Size + EfiInitScratch.BootDeviceOffset;
    }
    else
    {
        /* We do not, so zero out the space where a full boot device structure would fit */
        RtlZeroMemory(EfiInitScratch.AppEntry + ConsumedSize,
                      sizeof(BL_DEVICE_DESCRIPTOR));

        /* And start the firmware data past that */
        FirmwareOffset = EfiInitScratch.BootDeviceOffset + sizeof(BL_DEVICE_DESCRIPTOR);
    }

    /* Set the computed firmware data offset */
    EfiInitScratch.FirmwareParametersOffset = FirmwareOffset;

    /* Fill out the firmware data that's there */
    FirmwareData = (PVOID)((ULONG_PTR)&EfiInitScratch + EfiInitScratch.FirmwareParametersOffset);
    FirmwareData->Version = BL_FIRMWARE_DESCRIPTOR_VERSION;
    FirmwareData->ImageHandle = ImageHandle;
    FirmwareData->SystemTable = SystemTable;

    /* Finally, set the return argument offset */
    EfiInitScratch.ReturnArgumentsOffset = FirmwareOffset + sizeof(BL_FIRMWARE_DESCRIPTOR);

    /* And fill out the return argument data */
    ReturnArguments = (PVOID)((ULONG_PTR)&EfiInitScratch + EfiInitScratch.ReturnArgumentsOffset);
    ReturnArguments->Version = BL_RETURN_ARGUMENTS_VERSION;

    /* We're done, compute the final size and return the block */
    EfiInitScratch.Size = EfiInitScratch.ReturnArgumentsOffset + sizeof(BL_RETURN_ARGUMENTS);
    return (PBOOT_APPLICATION_PARAMETER_BLOCK)&EfiInitScratch;
}

/*++
 * @name EfiEntry
 *
 *     The EfiEntry routine implements the UEFI entrypoint for the application.
 *
 * @param  ImageHandle
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  SystemTable
 *         Pointer to the UEFI System Table.
 *
 * @return EFI_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
EFI_STATUS
EFIAPI
EfiEntry (
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable
    )
{
    NTSTATUS Status;
    PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters;

    /* Convert EFI parameters to Windows Boot Application parameters */
    BootParameters = EfiInitCreateInputParametersEx(ImageHandle, SystemTable);
    if (BootParameters != NULL)
    {
        /* Conversion was good -- call the Boot Manager Entrypoint */
        Status = BmMain(BootParameters);
    }
    else
    {
        /* Conversion failed, bail out */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Convert the NT status code to an EFI code */
    return EfiGetEfiStatusCode(Status);
}

