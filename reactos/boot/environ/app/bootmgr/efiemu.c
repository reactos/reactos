/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/efiemu.c
 * PURPOSE:         UEFI Entrypoint for Boot Manager
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"
#include <bcd.h>

/* DATA STRUCTURES ***********************************************************/

typedef struct _BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH
{
    BOOT_APPLICATION_PARAMETER_BLOCK;
    BL_MEMORY_DATA BootMemoryData;
    BL_MEMORY_DESCRIPTOR MemEntry;
    UCHAR AppEntry[788];
} BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH;

/* DATA VARIABLES ************************************************************/

ULONG BlpApplicationFlags;

GUID EfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
GUID EfiDevicePathProtocol = EFI_DEVICE_PATH_PROTOCOL_GUID;

BOOT_APPLICATION_PARAMETER_BLOCK_SCRATCH EfiInitScratch;

/* FUNCTIONS *****************************************************************/

NTSTATUS
AhCreateLoadOptionsList (
    _In_ PWCHAR CommandLine,
    _In_ PBOOT_ENTRY_OPTION BootOptions,
    _In_ ULONG MaximumLength,
    _Out_ PULONG OptionSize,
    _In_ PBOOT_ENTRY_OPTION* PreviousOption,
    _In_ PULONG PreviousOptionSize
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
EfiInitpConvertEfiFilePath (
    _In_ EFI_DEVICE_PATH_PROTOCOL *FilePath,
    _In_ ULONG PathType,
    _In_ PBOOT_ENTRY_OPTION Option,
    _In_ ULONG MaximumLength
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
EfiInitpConvertEfiDevicePath (
    _In_ EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    _In_ ULONG DeviceType,
    _In_ PBOOT_ENTRY_OPTION Option,
    _In_ ULONG MaximumLength
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

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
    PBOOT_ENTRY_OPTION Option, PreviousOption;
    ULONG HeaderSize, TotalOptionSize, Size, CommandLineSize, RemainingSize;
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    GUID ObjectGuid;
    PBCDE_DEVICE BcdDevice;
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
    RtlCopyMemory(Entry->Signature, "BTAPENT", 7);

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
        Entry->BcdData.Failed = TRUE;
        TotalOptionSize = sizeof(BOOT_ENTRY_OPTION);
        goto Quickie;
    }

    /* Extract the device descriptor and return it */
    BcdDevice = (PVOID)((ULONG_PTR)&Entry->BcdData + Entry->BcdData.DataOffset);
    *AppEntryDevice = &BcdDevice->Device;

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
        if ((OsPath->Length > FIELD_OFFSET(BL_FILE_PATH_DESCRIPTOR, Path)) &&
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

            /* Convert the path oprtion */
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
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle,
                                          L"Loaded image failed\n");
        return NULL;
    }

    /* Capture it in the boot application parameters */
    EfiInitScratch.ImageBase = (ULONG_PTR)LoadedImage->ImageBase;
    EfiInitScratch.ImageSize = (ULONG)LoadedImage->ImageSize;

    /* Now grab our device path protocol, so we can convert the path later on */
    Status = BootServices->HandleProtocol(ImageHandle,
                                          &EfiDevicePathProtocol,
                                          (VOID**)&DevicePath);
    if (Status != EFI_SUCCESS)
    {
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle,
                                          L"Device path failed\n");
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
    EfiInitScratch.BootMemoryData.Unknown = 8;

    /* Build the memory entry descriptor for this image itself */
    EfiInitScratch.MemEntry.Flags = 8;
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
EfiEntry (
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable
    )
{
    NTSTATUS Status;
    PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters;

    /* Temporary debugging string */
    SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"Hello from EFI\n");

    /* Convert EFI parameters to Windows Boot Application parameters */
    BootParameters = EfiInitCreateInputParametersEx(ImageHandle, SystemTable);
    if (BootParameters != NULL)
    {
        /* Conversion was good -- call the Boot Manager Entrypoint */
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"EFI input OK!\n");
        Status = BmMain(BootParameters);
    }
    else
    {
        /* Conversion failed, bail out */
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"EFI input failed\n");
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Convert the NT status code to an EFI code */
    return EfiGetEfiStatusCode(Status);
}

