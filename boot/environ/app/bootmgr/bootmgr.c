/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr/bootmgr.cla
 * PURPOSE:         Boot Manager Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* DATA VARIABLES ************************************************************/

DEFINE_GUID(GUID_WINDOWS_BOOTMGR,
            0x9DEA862C,
            0x5CDD,
            0x4E70,
            0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);

ULONGLONG ApplicationStartTime;
ULONGLONG PostTime;
GUID BmApplicationIdentifier;
PWCHAR BootDirectory;

BL_BOOT_ERROR BmpErrorBuffer;
PBL_BOOT_ERROR BmpInternalBootError;
BL_PACKED_BOOT_ERROR BmpPackedBootError;

BOOLEAN BmBootIniUsed;
WCHAR BmpFileNameBuffer[128];
PWCHAR ParentFileName = L"";

BOOLEAN BmDisplayStateCached;
PBL_LOADED_APPLICATION_ENTRY* BmpFailedBootEntries;
PBL_LOADED_APPLICATION_ENTRY BmpSelectedBootEntry;
BOOLEAN BmBootEntryOverridePresent;
BOOLEAN BmpDisplayBootMenu;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BmGetOptionList (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _In_ PBL_BCD_OPTION *OptionList
    )
{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    ULONG ElementSize, ElementCount, i, OptionsSize;
    BcdElementType Type;
    PBCD_ELEMENT_HEADER Header;
    PBCD_ELEMENT BcdElements;
    PBL_BCD_OPTION Options, Option, PreviousOption, DeviceOptions;
    PBCD_DEVICE_OPTION DeviceOption;
    GUID DeviceId;
    PVOID DeviceData;

    /* Open the BCD object requested */
    ObjectHandle = NULL;
    BcdElements = NULL;
    Status = BcdOpenObject(BcdHandle, ObjectId, &ObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Do the initial enumeration to get the size needed */
    ElementSize = 0;
    Status = BcdEnumerateAndUnpackElements(BcdHandle,
                                           ObjectHandle,
                                           NULL,
                                           &ElementSize,
                                           &ElementCount);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* If we got success, that doesn't make any sense */
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Bail out */
        goto Quickie;
    }

    /* Allocate a large-enough buffer */
    BcdElements = BlMmAllocateHeap(ElementSize);
    if (!BcdElements)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Now do the real enumeration to fill out the elements buffer */
    Status = BcdEnumerateAndUnpackElements(BcdHandle,
                                           ObjectHandle,
                                           BcdElements,
                                           &ElementSize,
                                           &ElementCount);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Go through each BCD option to add the sizes up */
    OptionsSize = 0;
    for (i = 0; i < ElementCount; i++)
    {
        OptionsSize += BcdElements[i].Header->Size + sizeof(BL_BCD_OPTION);
    }

    /* Allocate the required BCD option list */
    Options = BlMmAllocateHeap(OptionsSize);
    if (!Options)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(Options, OptionsSize);

    /* Start going through each option */
    PreviousOption = NULL;
    Option = Options;
    for (i = 0; i < ElementCount; i++)
    {
        /* Read the header and type */
        Header = BcdElements[i].Header;
        Type.PackedValue = Header->Type;

        /* Check if this option isn't already present */
        if (!MiscGetBootOption(Options, Type.PackedValue))
        {
            /* It's a new option. Did we have an existing one? */
            if (PreviousOption)
            {
                /* Link it to this new one */
                PreviousOption->NextEntryOffset = (ULONG_PTR)Option -
                                                  (ULONG_PTR)Options;
            }

            /* Capture the type, size, data, and offset */
            Option->Type = Type.PackedValue;
            Option->DataSize = Header->Size;
            RtlCopyMemory(Option + 1, BcdElements[i].Body, Header->Size);
            Option->DataOffset = sizeof(BL_BCD_OPTION);

            /* Check if this was a device */
            if (Type.Format == BCD_TYPE_DEVICE)
            {
                /* Grab its GUID */
                DeviceOption = (PBCD_DEVICE_OPTION)(Option + 1);
                DeviceId = DeviceOption->AssociatedEntry;

                /* Look up the options for that GUID */
                Status = BmGetOptionList(BcdHandle, &DeviceId, &DeviceOptions);
                if (NT_SUCCESS(Status))
                {
                    /* Device data is after the device option */
                    DeviceData = (PVOID)((ULONG_PTR)DeviceOption + Header->Size);

                    /* Copy it */
                    RtlCopyMemory(DeviceData,
                                  DeviceOptions,
                                  BlGetBootOptionListSize(DeviceOptions));

                    /* Don't need this anymore */
                    BlMmFreeHeap(DeviceOptions);

                    /* Write the offset of the device options */
                    Option->ListOffset = (ULONG_PTR)DeviceData -
                                         (ULONG_PTR)Option;
                }
            }

            /* Save the previous option and go to the next one */
            PreviousOption = Option;
            Option = (PBL_BCD_OPTION)((ULONG_PTR)Option +
                                      BlGetBootOptionSize(Option));
        }
    }

    /* Return the pointer back, we've made it! */
    *OptionList = Options;
    Status = STATUS_SUCCESS;

Quickie:
    /* Did we allocate a local buffer? Free it if so */
    if (BcdElements)
    {
        BlMmFreeHeap(BcdElements);
    }

    /* Was the key open? Close it if so */
    if (ObjectHandle)
    {
        BiCloseKey(ObjectHandle);
    }

    /* Return the option list parsing status */
    return Status;
}

NTSTATUS
BmpUpdateApplicationOptions (
    _In_ HANDLE BcdHandle
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Options;

    /* Get the boot option list */
    Status = BmGetOptionList(BcdHandle, &BmApplicationIdentifier, &Options);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Append the options, free the local buffer, and return success */
    BlAppendBootOptions(&BlpApplicationEntry, Options);
    BlMmFreeHeap(Options);
    return STATUS_SUCCESS;
}

NTSTATUS
BmpFwGetApplicationDirectoryPath (
    _In_ PUNICODE_STRING ApplicationDirectoryPath
    )
{
    NTSTATUS Status;
    SIZE_T i, AppPathLength;
    PWCHAR ApplicationPath, PathCopy;

    /* Clear the incoming string */
    ApplicationDirectoryPath->Length = 0;
    ApplicationDirectoryPath->MaximumLength = 0;
    ApplicationDirectoryPath->Buffer = 0;

    /* Get the boot application path */
    ApplicationPath = NULL;
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdLibraryString_ApplicationPath,
                                   &ApplicationPath);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the length of the application path */
        for (i = wcslen(ApplicationPath) - 1; i > 0; i--)
        {
            /* Keep going until the path separator */
            if (ApplicationPath[i] == OBJ_NAME_PATH_SEPARATOR)
            {
                break;
            }
        }

        /* Check if we have space for one more character */
        Status = RtlSIZETAdd(i, 1, &AppPathLength);
        if (NT_SUCCESS(Status))
        {
            /* Check if it's safe to multiply by two */
            Status = RtlSIZETMult(AppPathLength, sizeof(WCHAR), &AppPathLength);
            if (NT_SUCCESS(Status))
            {
                /* Allocate a copy for the string */
                PathCopy = BlMmAllocateHeap(AppPathLength);
                if (PathCopy)
                {
                    /* NULL-terminate it */
                    RtlCopyMemory(PathCopy,
                                  ApplicationPath,
                                  AppPathLength - sizeof(UNICODE_NULL));
                    PathCopy[AppPathLength] = UNICODE_NULL;

                    /* Finally, initialize the outgoing string */
                    RtlInitUnicodeString(ApplicationDirectoryPath, PathCopy);
                }
                else
                {
                    /* No memory, fail */
                    Status = STATUS_NO_MEMORY;
                }
            }
        }
    }

    /* Check if we had an application path */
    if (ApplicationPath)
    {
        /* No longer need this, free it */
        BlMmFreeHeap(ApplicationPath);
    }

    /* All done! */
    return Status;
}

NTSTATUS
BmFwInitializeBootDirectoryPath (
    VOID
    )
{
    PWCHAR FinalPath;
    NTSTATUS Status;
    PWCHAR BcdDirectory;
    UNICODE_STRING BcdPath;
    ULONG FinalSize;
    ULONG FileHandle, DeviceHandle;

    /* Initialize everything for failure */
    BcdPath.MaximumLength = 0;
    BcdPath.Buffer = NULL;
    BcdDirectory = NULL;
    FinalPath = NULL;
    FileHandle = -1;
    DeviceHandle = -1;

    /* Try to open the boot device */
    Status = BlpDeviceOpen(BlpBootDevice,
                           BL_DEVICE_READ_ACCESS,
                           0,
                           &DeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Device open failed: %lx\r\n", Status);
        goto Quickie;
    }

    /* Get the directory path */
    Status = BmpFwGetApplicationDirectoryPath(&BcdPath);
    BcdDirectory = BcdPath.Buffer;
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add the BCD file name to it */
    FinalSize = BcdPath.MaximumLength + sizeof(L"\\BCD") - sizeof(UNICODE_NULL);
    if (FinalSize < BcdPath.MaximumLength)
    {
        goto Quickie;
    }

    /* Allocate space for the final path */
    FinalPath = BlMmAllocateHeap(FinalSize);
    if (!FinalPath)
    {
        goto Quickie;
    }

    /* Build it */
    RtlZeroMemory(FinalPath, FinalSize);
    RtlCopyMemory(FinalPath, BcdDirectory, BcdPath.MaximumLength);
    wcsncat(FinalPath, L"\\BCD", FinalSize / sizeof(WCHAR));

    /* Try to open the file */
    Status = BlFileOpen(DeviceHandle,
                        FinalPath,
                        BL_FILE_READ_ACCESS,
                        &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        BootDirectory = BcdDirectory;
        goto Quickie;
    }

    /* Save the boot directory */
    BootDirectory = L"\\EFI\\Boot"; /* Should be EFI\\ReactOS\\Boot */

Quickie:
    /* Free all the allocations we made */
    if (BcdDirectory)
    {
        Status = BlMmFreeHeap(BcdDirectory);
    }
    if (FinalPath)
    {
        Status = BlMmFreeHeap(FinalPath);
    }

    /* Close the BCD file */
    if (FileHandle != -1)
    {
        Status = BlFileClose(FileHandle);
    }

    /* Close the boot device */
    if (DeviceHandle != -1)
    {
        Status = BlDeviceClose(DeviceHandle);
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BmOpenBootIni (
    VOID
    )
{
    /* Don't yet handled boot.ini */
    return STATUS_NOT_FOUND;
}

ULONG
BmpFatalErrorMessageFilter (
    _In_ NTSTATUS ErrorStatus,
    _Out_ PULONG ErrorResourceId
    )
{
    ULONG Result;

    /* Assume no message for now, check for known status message */
    Result = 0;
    switch (ErrorStatus)
    {
        /* Convert each status to a resource ID */
        case STATUS_UNEXPECTED_IO_ERROR:
            *ErrorResourceId = 9017;
            Result = 1;
            break;
        case STATUS_IMAGE_CHECKSUM_MISMATCH:
            *ErrorResourceId = 9018;
            break;
        case STATUS_INVALID_IMAGE_WIN_64:
            *ErrorResourceId = 9016;
            break;
        case 0xC0000428:
            *ErrorResourceId = 9019;
            Result = 2;
            break;
        case STATUS_FVE_LOCKED_VOLUME:
            *ErrorResourceId = 9013;
            break;
    }

    /* Return the type of message */
    return Result;
}

VOID
BmErrorPurge (
    VOID
    )
{
    /* Check if a boot error is present */
    if (BmpPackedBootError.BootError)
    {
        /* Purge it */
        BlMmFreeHeap(BmpPackedBootError.BootError);
        BmpPackedBootError.BootError = NULL;
    }

    /* Zero out the packed buffer */
    BmpPackedBootError.Size = 0;
    BmpInternalBootError = NULL;
    RtlZeroMemory(&BmpErrorBuffer, sizeof(BmpErrorBuffer));
}

VOID
BmpErrorLog (
    _In_ ULONG ErrorCode,
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG ErrorMsgId,
    _In_ PWCHAR FileName,
    _In_ ULONG HelpMsgId
    )
{
    PWCHAR ErrorMsgString;

    /* Check if we already had an error */
    if (BmpInternalBootError)
    {
        /* Purge it */
        BmErrorPurge();
    }

    /* Find the string for this error ID */
    ErrorMsgString = BlResourceFindMessage(ErrorMsgId);
    if (ErrorMsgString)
    {
        /* Fill out the error buffer */
        BmpErrorBuffer.Unknown1 = 0;
        BmpErrorBuffer.Unknown2 = 0;
        BmpErrorBuffer.ErrorString = ErrorMsgString;
        BmpErrorBuffer.FileName = FileName;
        BmpErrorBuffer.ErrorCode = ErrorCode;
        BmpErrorBuffer.ErrorStatus = ErrorStatus;
        BmpErrorBuffer.HelpMsgId = HelpMsgId;
        BmpInternalBootError = &BmpErrorBuffer;
    }
}

VOID
BmFatalErrorEx (
    _In_ ULONG ErrorCode,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3,
    _In_ ULONG_PTR Parameter4
    )
{
    PWCHAR FileName, Buffer;
    NTSTATUS ErrorStatus;
    WCHAR FormatString[256];
    ULONG ErrorResourceId, ErrorHelpId;
    BOOLEAN Restart, NoError;

    /* Assume no buffer for now */
    Buffer = NULL;

    /* Check what error code is being raised */
    switch (ErrorCode)
    {
        /* Error reading the BCD */
        case BL_FATAL_ERROR_BCD_READ:

            /* Check if we have a name for the BCD file */
            if (Parameter1)
            {
                /* Check if the name fits into our buffer */
                FileName = (PWCHAR)Parameter1;
                if (wcslen(FileName) < sizeof(BmpFileNameBuffer))
                {
                    /* Copy it in there */
                    Buffer = BmpFileNameBuffer;
                    wcsncpy(BmpFileNameBuffer,
                            FileName,
                            RTL_NUMBER_OF(BmpFileNameBuffer));
                }
            }

            /* If we don't have a buffer, use an empty one */
            if (!Buffer)
            {
                Buffer = ParentFileName;
            }

            /* The NTSTATUS code is in parameter 2*/
            ErrorStatus = (NTSTATUS)Parameter2;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nAn error occurred (%08x) while attempting "
                     L"to read the boot configuration data file %s\n",
                     ErrorStatus,
                     Buffer);

            /* Select the resource ID message */
            ErrorResourceId = 9002;
            break;

        case BL_FATAL_ERROR_BCD_ENTRIES:

            /* File name is in parameter 1 */
            FileName = (PWCHAR)Parameter1;

            /* The NTSTATUS code is in parameter 2*/
            ErrorStatus = (NTSTATUS)Parameter2;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nNo valid entries found in the boot configuration data file %s\n",
                     FileName);

            /* Select the resource ID message */
            ErrorResourceId = 9007;
            break;

        case BL_FATAL_ERROR_BCD_PARSE:

            /* File name isin parameter 1 */
            FileName = (PWCHAR)Parameter1;

            /* The NTSTATUS code is in parameter 2*/
            ErrorStatus = (NTSTATUS)Parameter2;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nThe boot configuration file %s is invalid (%08x).\n",
                     FileName,
                     ErrorStatus);

            /* Select the resource ID message */
            ErrorResourceId = 9015;
            break;

        case BL_FATAL_ERROR_GENERIC:

            /* The NTSTATUS code is in parameter 1*/
            ErrorStatus = (NTSTATUS)Parameter1;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nThe boot manager experienced an error (%08x).\n",
                     ErrorStatus);

            /* Select the resource ID message */
            ErrorResourceId = 9005;
            break;

        default:

            /* The rest is not yet handled */
            EfiPrintf(L"Unexpected fatal error: %lx\r\n", ErrorCode);
            while (1);
            break;
    }

    /* Check if the BCD option for restart is set */
    BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                           BcdLibraryBoolean_RestartOnFailure,
                           &Restart);
    if (Restart)
    {
        /* Yes, so no error should be shown since we'll auto-restart */
        NoError = TRUE;
    }
    else
    {
        /* Check if the option for not showing errors is set in the BCD */
        BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                               BcdBootMgrBoolean_NoErrorDisplay,
                               &NoError);
    }

    /* Do we want an error? */
    if (!NoError)
    {
        /* Yep, print it and then raise an error */
        BlStatusPrint(FormatString);
        BlStatusError(1, ErrorCode, Parameter1, Parameter2, Parameter3);
    }

    /* Get the help message ID */
    ErrorHelpId = BmpFatalErrorMessageFilter(ErrorStatus, &ErrorResourceId);
    BmpErrorLog(ErrorCode, ErrorStatus, ErrorResourceId, Buffer, ErrorHelpId);
}

NTSTATUS
BmpFwGetFullPath (
    _In_ PWCHAR FileName,
    _Out_ PWCHAR* FullPath
    )
{
    NTSTATUS Status;
    SIZE_T BootDirLength, PathLength;

    /* Compute the length of the directory, and add a NUL */
    BootDirLength = wcslen(BootDirectory);
    Status = RtlSIZETAdd(BootDirLength, 1, &BootDirLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add the length of the file, make sure it fits */
    PathLength = wcslen(FileName);
    Status = RtlSIZETAdd(PathLength, BootDirLength, &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Convert to bytes */
    Status = RtlSIZETMult(PathLength, sizeof(WCHAR), &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate the full path */
    *FullPath = BlMmAllocateHeap(PathLength);
    if (*FullPath)
    {
        /* Copy the directory followed by the file name */
        wcsncpy(*FullPath, BootDirectory, PathLength / sizeof(WCHAR));
        wcsncat(*FullPath, FileName, PathLength / sizeof(WCHAR));
    }
    else
    {
        /* Bail out since we have no memory */
        Status = STATUS_NO_MEMORY;
    }

Quickie:
    /* Return to caller */
    return Status;
}

VOID
BmCloseDataStore (
    _In_ HANDLE Handle
    )
{
    /* Check if boot.ini data needs to be freed */
    if (BmBootIniUsed)
    {
        EfiPrintf(L"Boot.ini not handled\r\n");
    }

    /* Dereference the hive and close the key */
    BiDereferenceHive(Handle);
    BiCloseKey(Handle);
}

NTSTATUS
BmOpenDataStore (
    _Out_ PHANDLE Handle
    )
{
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR BcdDevice;
    PWCHAR BcdPath, FullPath, PathBuffer;
    BOOLEAN HavePath;
    SIZE_T PathLength, FullSize;
    PVOID FinalBuffer;
    UNICODE_STRING BcdString;

    /* Initialize variables */
    PathBuffer = NULL;
    BcdDevice = NULL;
    BcdPath = NULL;
    HavePath = FALSE;

    /* Check if a boot.ini file exists */
    Status = BmOpenBootIni();
    if (NT_SUCCESS(Status))
    {
        BmBootIniUsed = TRUE;
    }

    /* Check on which device the BCD is */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdBootMgrDevice_BcdDevice,
                                   &BcdDevice,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        /* It's not on a custom device, so it must be where we are */
        Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                       BcdLibraryDevice_ApplicationDevice,
                                       &BcdDevice,
                                       NULL);
        if (!NT_SUCCESS(Status))
        {
            /* This BCD option is required */
            goto Quickie;
        }
    }

    /* Next, check what file contains the BCD */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdBootMgrString_BcdFilePath,
                                   &BcdPath);
    if (NT_SUCCESS(Status))
    {
        /* We don't handle custom BCDs yet */
        EfiPrintf(L"Custom BCD Not handled: %s\r\n", BcdPath);
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }

    /* Now check if the BCD is on a remote share */
    if (BcdDevice->DeviceType == UdpDevice)
    {
        /* Nope. Nope. Nope */
        EfiPrintf(L"UDP device Not handled\r\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }

    /* Otherwise, compute the hardcoded path of the BCD */
    Status = BmpFwGetFullPath(L"\\BCD", &FullPath);
    if (!NT_SUCCESS(Status))
    {
        /* User the raw path */
        PathBuffer = BcdPath;
    }
    else
    {
        /* Use the path we got */
        PathBuffer = FullPath;
        HavePath = TRUE;
    }

    /* Check if we failed to get the BCD path */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add a NUL to the path, make sure it'll fit */
    PathLength = wcslen(PathBuffer);
    Status = RtlSIZETAdd(PathLength, 1, &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Convert to bytes */
    Status = RtlSIZETMult(PathLength, sizeof(WCHAR), &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Now add the size of the path to the device path, check if it fits */
    Status = RtlSIZETAdd(PathLength, BcdDevice->Size, &FullSize);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate a final structure to hold both entities */
    FinalBuffer = BlMmAllocateHeap(FullSize);
    if (!FinalBuffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Copy the device path and file path into the final buffer */
    RtlCopyMemory(FinalBuffer, BcdDevice, BcdDevice->Size);
    RtlCopyMemory((PVOID)((ULONG_PTR)FinalBuffer + BcdDevice->Size),
                  PathBuffer,
                  PathLength);

    /* Now tell the BCD engine to open the store */
    BcdString.Length = FullSize;
    BcdString.MaximumLength = FullSize;
    BcdString.Buffer = FinalBuffer;
    Status = BcdOpenStoreFromFile(&BcdString, Handle);

    /* Free our final buffer */
    BlMmFreeHeap(FinalBuffer);

Quickie:
    /* Did we allocate a device? */
    if (BcdDevice)
    {
        /* Free it */
        BlMmFreeHeap(BcdDevice);
    }

    /* Is this the failure path? */
    if (!NT_SUCCESS(Status))
    {
        /* Raise a fatal error */
        BmFatalErrorEx(BL_FATAL_ERROR_BCD_READ,
                       (ULONG_PTR)PathBuffer,
                       Status,
                       0,
                       0);
    }

    /* Did we get an allocated path? */
    if ((PathBuffer) && (HavePath))
    {
        /* Free it */
        BlMmFreeHeap(PathBuffer);
    }

    /* Return back to the caller */
    return Status;
}

typedef struct _BL_BSD_LOG_OBJECT
{
    ULONG DeviceId;
    ULONG FileId;
    ULONG Unknown;
    ULONG Size;
    ULONG Flags;
} BL_BSD_LOG_OBJECT, *PBL_BSD_LOG_OBJECT;

BL_BSD_LOG_OBJECT BsdpLogObject;
BOOLEAN BsdpLogObjectInitialized;

VOID
BlBsdInitializeLog (
    _In_ PBL_DEVICE_DESCRIPTOR LogDevice,
    _In_ PWCHAR LogPath,
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;

    /* Don't initialize twice */
    if (BsdpLogObjectInitialized)
    {
        return;
    }

    /* Set invalid IDs for now */
    BsdpLogObject.DeviceId = -1;
    BsdpLogObject.FileId = -1;

    /* Open the BSD device */
    Status = BlpDeviceOpen(LogDevice,
                           BL_DEVICE_READ_ACCESS | BL_DEVICE_WRITE_ACCESS,
                           0,
                           &BsdpLogObject.DeviceId);
    if (!NT_SUCCESS(Status))
    {
        /* Welp that didn't work */
        goto FailurePath;
    }

    /* Now open the BSD itself */
    Status = BlFileOpen(BsdpLogObject.DeviceId,
                        LogPath,
                        BL_FILE_READ_ACCESS | BL_FILE_WRITE_ACCESS,
                        &BsdpLogObject.FileId);
    if (!NT_SUCCESS(Status))
    {
        /* D'oh */
        goto FailurePath;
    }

    /* The BSD is open. Start doing stuff to it */
    EfiPrintf(L"Unimplemented BSD path\r\n");
    Status = STATUS_NOT_IMPLEMENTED;

FailurePath:
    /* Close the BSD if we had it open */
    if (BsdpLogObject.FileId != -1)
    {
        BlFileClose(BsdpLogObject.FileId);
    }

    /* Close the device if we had it open */
    if (BsdpLogObject.DeviceId != -1)
    {
        BlDeviceClose(BsdpLogObject.DeviceId);
    }

    /* Set BSD object to its uninitialized state */
    BsdpLogObjectInitialized = FALSE;
    BsdpLogObject.FileId = 0;
    BsdpLogObject.DeviceId = 0;
    BsdpLogObject.Flags = 0;
    BsdpLogObject.Unknown = 0;
    BsdpLogObject.Size = 0;
}

VOID
BmpInitializeBootStatusDataLog (
    VOID
    )
{
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR BsdDevice;
    PWCHAR BsdPath;
    ULONG Flags;
    BOOLEAN PreserveBsd;

    /* Initialize locals */
    BsdPath = NULL;
    BsdDevice = NULL;
    Flags = 0;

    /* Check if the BSD is stored in a custom device */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdLibraryDevice_BsdLogDevice,
                                   &BsdDevice,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Nope, use the boot device */
        BsdDevice = BlpBootDevice;
    }

    /* Check if the path is custom as well */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdLibraryString_BsdLogPath,
                                   &BsdPath);
    if (!NT_SUCCESS(Status))
    {
        /* Nope, use our default path */
        Status = BmpFwGetFullPath(L"\\bootstat.dat", &BsdPath);
        if (!NT_SUCCESS(Status))
        {
            BsdPath = NULL;
        }

        /* Set preserve flag */
        Flags = 1;
    }
    else
    {
        /* Set preserve flag */
        Flags = 1;
    }

    /* Finally, check if the BSD should be preserved */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdLibraryBoolean_PreserveBsdLog,
                                    &PreserveBsd);
    if (!(NT_SUCCESS(Status)) || !(PreserveBsd))
    {
        /* We failed to read, or we were asked not to preserve it */
        Flags = 0;
    }

    /* Initialize the log */
    BlBsdInitializeLog(BsdDevice, BsdPath, Flags);

    /* Free the BSD device descriptor if we had one */
    if (BsdDevice)
    {
        BlMmFreeHeap(BsdDevice);
    }

    /* Free the BSD path if we had one */
    if ((Flags) && (BsdPath))
    {
        BlMmFreeHeap(BsdPath);
    }
}

VOID
BmFwMemoryInitialize (
    VOID
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    BL_ADDRESS_RANGE AddressRange;

    /* Select the range below 1MB */
    AddressRange.Maximum = 0xFFFFF;
    AddressRange.Minimum = 0;

    /* Allocate one reserved page with the "below 1MB" attribute */
    Status = MmPapAllocatePhysicalPagesInRange(&PhysicalAddress,
                                               BlApplicationReserved,
                                               1,
                                               BlMemoryBelow1MB,
                                               0,
                                               &MmMdlUnmappedAllocated,
                                               &AddressRange,
                                               BL_MM_REQUEST_DEFAULT_TYPE);
    if (!NT_SUCCESS(Status))
    {
        /* Print a message on error, but keep going */
        BlStatusPrint(L"BmFwMemoryInitialize: Failed to allocate a page below 1MB. Status: 0x%08x\r\n",
                      Status);
    }
}

NTSTATUS
BmpBgDisplayClearScreen (
    _In_ ULONG Color
    )
{
    /* Not yet supported */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlXmiWrite (
    _In_ PWCHAR XmlTag
    )
{
    /* Sigh */
    EfiPrintf(L"XML: %s\r\n", XmlTag);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlXmiInitialize (
    _In_ PWCHAR Stylesheet
    )
{
    /* Reset the cursor type */
    BlDisplaySetCursorType(0);

    /* Nope, not doing any XML stuff */
    return STATUS_SUCCESS;
}

NTSTATUS
BmFwVerifySelfIntegrity (
    VOID
    )
{
    /* Check if we're booted by UEFI off the DVD directly */
    if ((BlpBootDevice->DeviceType == LocalDevice) &&
        (BlpBootDevice->Local.Type == CdRomDevice) &&
        (BlpApplicationFlags & BL_APPLICATION_FLAG_CONVERTED_FROM_EFI))
    {
        /* Windows actually bypasses integrity checks in this case. Works for us */
        return STATUS_SUCCESS;
    }

    /* Our binaries aren't signed, so always return failure */
    return 0xC0000428;
}

NTSTATUS
BmFwRegisterRevocationList (
    VOID
    )
{
    NTSTATUS Status;
    BOOLEAN SecureBootEnabled;

    /* Is SecureBoot enabled? */
    Status = BlSecureBootIsEnabled(&SecureBootEnabled);
    if ((NT_SUCCESS(Status)) && (SecureBootEnabled))
    {
        EfiPrintf(L"SB not implemented revok\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    else
    {
        /* Nothing to do without SecureBoot */
        Status = STATUS_SUCCESS;
    }

    /* Return revocation result back to caller */
    return Status;
}

NTSTATUS
BmResumeFromHibernate (
    _Out_ PHANDLE BcdResumeHandle
    )
{
    NTSTATUS Status;
    BOOLEAN AttemptResume;

    /* Should we attempt to resume from hibernation? */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdBootMgrBoolean_AttemptResume,
                                    &AttemptResume);
    if (!NT_SUCCESS(Status))
    {
        /* Nope. Is automatic restart on crash enabled? */
        AttemptResume = FALSE;
        Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                        BcdOSLoaderBoolean_DisableCrashAutoReboot,
                                        &AttemptResume);
        AttemptResume = (NT_SUCCESS(Status) && (AttemptResume));
    }

    /* Don't do anything if there's no need to resume anything */
    if (!AttemptResume)
    {
        return STATUS_SUCCESS;
    }

    /* Not yet implemented */
    EfiPrintf(L"Resume not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BmpProcessBadMemory (
    VOID
    )
{
    BL_PD_DATA_BLOB BadMemoryData;
    NTSTATUS Status;

    /* Try to get the memory data from the memtest application */
    BadMemoryData.BlobSize = 0;
    BadMemoryData.Data = NULL;
    BadMemoryData.DataSize = 0;
    Status = BlPdQueryData(&BadMemoryGuid, NULL, &BadMemoryData);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* No results, or some other error */
        return Status;
    }

    /* Not yet implemented */
    EfiPrintf(L"Bad page list persistence not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BmPurgeOption (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _In_ ULONG Type
    )
{
    HANDLE ObjectHandle;
    NTSTATUS Status;

    /* Open the object */
    Status = BcdOpenObject(BcdHandle, ObjectId, &ObjectHandle);
    if (NT_SUCCESS(Status))
    {
        /* Delete the element */
        BcdDeleteElement(ObjectHandle, Type);

        /* Close the object and set success */
        BiCloseKey(ObjectHandle);
        Status = STATUS_SUCCESS;
    }

    /* Return the result */
    return Status;
}

NTSTATUS
BmGetEntryDescription (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _Out_ PBCD_OBJECT_DESCRIPTION Description
    )
{
    NTSTATUS Status;
    HANDLE ObjectHandle;

    /* Open the BCD object */
    Status = BcdOpenObject(BcdHandle, ObjectId, &ObjectHandle);
    if (NT_SUCCESS(Status))
    {
        /* Make sure the caller passed this argument in */
        if (!Description)
        {
            /* Fail otherwise */
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Query the description from the BCD interface */
            Status = BiGetObjectDescription(ObjectHandle, Description);
        }

        /* Close the object key */
        BiCloseKey(ObjectHandle);
    }

    /* Return the result back */
    return Status;
}

NTSTATUS
BmpPopulateBootEntryList (
    _In_ HANDLE BcdHandle,
    _In_ PGUID SequenceList,
    _In_ ULONG Flags,
    _Out_ PBL_LOADED_APPLICATION_ENTRY* BootSequence,
    _Out_ PULONG SequenceCount
    )
{
    NTSTATUS Status;
    ULONG BootIndex, i, OptionSize;
    PBL_LOADED_APPLICATION_ENTRY BootEntry;
    PBL_BCD_OPTION Options;
    BCD_OBJECT_DESCRIPTION Description;
    BcdObjectType ObjectType;
    BOOLEAN HavePath, IsWinPe, SoftReboot;
    PWCHAR LoaderPath;

    /* Initialize locals */
    Options = NULL;
    BootIndex = 0;
    Status = STATUS_NOT_FOUND;

    /* Loop through every element in the sequence */
    for (i = 0; i < *SequenceCount; i++)
    {
        /* Assume failure */
        BootEntry = NULL;

        /* Get the options for the sequence element */
        Status = BmGetOptionList(BcdHandle, SequenceList, &Options);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"option list failed: %lx\r\n", Status);
            goto LoopQuickie;
        }

        /* Make sure there's at least a path and description */
        if (!(MiscGetBootOption(Options, BcdLibraryDevice_ApplicationDevice)) ||
            !(MiscGetBootOption(Options, BcdLibraryString_Description)))
        {
            Status = STATUS_UNSUCCESSFUL;
            EfiPrintf(L"missing list failed: %lx\r\n", Status);
            goto LoopQuickie;
        }

        /* Get the size of the BCD options and allocate a large enough entry */
        OptionSize = BlGetBootOptionListSize(Options);
        BootEntry = BlMmAllocateHeap(sizeof(*BootEntry) + OptionSize);
        if (!BootEntry)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Save it as part of the sequence */
        BootSequence[BootIndex] = BootEntry;

        /* Initialize it, and copy the BCD data */
        RtlZeroMemory(BootEntry, sizeof(*BootEntry));
        BootEntry->Guid = *SequenceList;
        BootEntry->BcdData = (PBL_BCD_OPTION)(BootEntry + 1);
        BootEntry->Flags = Flags;
        RtlCopyMemory(BootEntry->BcdData, Options, OptionSize);

        /* Get the object descriptor to find out what kind of entry it is */
        Status = BmGetEntryDescription(BcdHandle,
                                       &BootEntry->Guid,
                                       &Description);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"missing desc failed: %lx\r\n", Status);
            goto LoopQuickie;
        }

        /* Check if a path was given or not */
        HavePath = MiscGetBootOption(Options, BcdLibraryString_ApplicationPath) ?
                   TRUE : FALSE;

        /* Now select based on what type of object this is -- must be an app */
        ObjectType.PackedValue = Description.Type;
        if (ObjectType.Application.ObjectCode == BCD_OBJECT_TYPE_APPLICATION)
        {
            /* Then select based on what kind of app it is */
            switch (ObjectType.Application.ApplicationCode)
            {
                /* Another boot manager */
                case BCD_APPLICATION_TYPE_BOOTMGR:
                    BootEntry->Flags |= BCD_APPLICATION_TYPE_BOOTMGR;
                    break;

                /* An OS loader */
                case BCD_APPLICATION_TYPE_OSLOADER:
                    BootEntry->Flags |= BL_APPLICATION_ENTRY_WINLOAD;

                    /* Do we have a path for it? */
                    if (!HavePath)
                    {
                        /* We'll try to make one up. Is this WinPE? */
                        IsWinPe = FALSE;
                        Status = BlGetBootOptionBoolean(Options,
                                                        BcdOSLoaderBoolean_WinPEMode,
                                                        &IsWinPe);
                        if (!(NT_SUCCESS(Status)) && (Status != STATUS_NOT_FOUND))
                        {
                            goto Quickie;
                        }

                        /* Use the appropriate path for WinPE or local install */
                        LoaderPath = IsWinPe ?
                                     L"\\Windows\\System32\\boot\\winload.efi" :
                                     L"\\Windows\\System32\\winload.efi";

                        /* Add the path to the boot entry */
                        Status = BlAppendBootOptionString(BootEntry,
                                                          BcdLibraryString_ApplicationPath,
                                                          LoaderPath);
                        if (!NT_SUCCESS(Status))
                        {
                            goto Quickie;
                        }

                        /* We have a path now */
                        HavePath = TRUE;
                    }
                    break;

                /* A hibernate-resume application */
                case BCD_APPLICATION_TYPE_RESUME:
                    BootEntry->Flags |= BL_APPLICATION_ENTRY_WINRESUME;
                    break;

                /* An older OS NTLDR */
                case BCD_APPLICATION_TYPE_NTLDR:
                    BootEntry->Flags |= BL_APPLICATION_ENTRY_NTLDR;
                    break;

                /* An older OS SETUPLDR */
                case BCD_APPLICATION_TYPE_SETUPLDR:
                    BootEntry->Flags |= BL_APPLICATION_ENTRY_SETUPLDR;
                    break;

                /* A 3rd party/Win9x boot sector */
                case BCD_APPLICATION_TYPE_BOOTSECTOR:
                    BootEntry->Flags |= BL_APPLICATION_ENTRY_BOOTSECTOR;
                    break;

                /* Something else entirely */
                default:
                    break;
            }
        }

        /* We better have a path by now */
        if (!HavePath)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto LoopQuickie;
        }

        /* Check if this is a real mode startup.com */
        if ((ObjectType.Application.ObjectCode == BCD_OBJECT_TYPE_APPLICATION) &&
            (ObjectType.Application.ImageCode == BCD_IMAGE_TYPE_REAL_MODE) &&
            (ObjectType.Application.ApplicationCode == BCD_APPLICATION_TYPE_STARTUPCOM))
        {
            /* Check if PXE soft reboot will occur */
            Status = BlGetBootOptionBoolean(Options,
                                            BcdStartupBoolean_PxeSoftReboot,
                                            &SoftReboot);
            if ((NT_SUCCESS(Status)) && (SoftReboot))
            {
                /* Then it's a valid startup.com entry */
                BootEntry->Flags |= BL_APPLICATION_ENTRY_STARTUP;
            }
        }

LoopQuickie:
        /* All done with this entry -- did we have BCD options? */
        if (Options)
        {
            /* Free them, they're part of the entry now */
            BlMmFreeHeap(Options);
            Options = NULL;
        }

        /* Did we fail anywhere? */
        if (!NT_SUCCESS(Status))
        {
            /* Yep -- did we fail with an active boot entry? */
            if (BootEntry)
            {
                /* Destroy it */
                BlDestroyBootEntry(BootEntry);
                BootSequence[BootIndex] = NULL;
            }
        }
        else
        {
            /* It worked, so populate the next index now */
            BootIndex++;
        }

        /* And move to the next GUID in the sequence list */
        SequenceList++;
    }

Quickie:
    /* All done now -- did we have any BCD options? */
    if (Options)
    {
        /* Free them */
        BlMmFreeHeap(Options);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
BmGetBootSequence (
    _In_ HANDLE BcdHandle,
    _In_ PGUID SequenceList,
    _In_ ULONG SequenceListCount,
    _In_ ULONG Flags,
    _Out_ PBL_LOADED_APPLICATION_ENTRY** BootSequence,
    _Out_ PULONG SequenceCount
    )
{
    PBL_LOADED_APPLICATION_ENTRY* Sequence;
    ULONG Count = SequenceListCount;
    NTSTATUS Status;

    /* Allocate the sequence list */
    Sequence = BlMmAllocateHeap(SequenceListCount * sizeof(*Sequence));
    if (!Sequence)
    {
        return STATUS_NO_MEMORY;
    }

    /* Populate the sequence list */
    Status = BmpPopulateBootEntryList(BcdHandle,
                                      SequenceList,
                                      Flags,
                                      Sequence,
                                      &Count);
    if (!NT_SUCCESS(Status))
    {
        /* Free the list on failure */
        BlMmFreeHeap(Sequence);
    }
    else
    {
        /* Otherwise, set success and return the list and count */
        Status = STATUS_SUCCESS;
        *BootSequence = Sequence;
        *SequenceCount = Count;
    }

    /* All done */
    return Status;
}

NTSTATUS
BmEnumerateBootEntries (
    _In_ HANDLE BcdHandle,
    _Out_ PBL_LOADED_APPLICATION_ENTRY **BootSequence,
    _Out_ PULONG SequenceCount
    )
{
    NTSTATUS Status;
    ULONG BootIndex, BootIniCount, BootEntryCount, BcdCount;
    PBL_LOADED_APPLICATION_ENTRY* Sequence;
    PGUID DisplayOrder;
    GUID DefaultObject;
    BOOLEAN UseDisplayList;

    /* Initialize locals */
    BootIndex = 0;

    /* First try to get the display list, if any */
    UseDisplayList = TRUE;
    Status = BlGetBootOptionGuidList(BlpApplicationEntry.BcdData,
                                     BcdBootMgrObjectList_DisplayOrder,
                                     &DisplayOrder,
                                     &BcdCount);
    if (!NT_SUCCESS(Status))
    {
        /* No list, get the default entry instead */
        Status = BlGetBootOptionGuid(BlpApplicationEntry.BcdData,
                                     BcdBootMgrObject_DefaultObject,
                                     &DefaultObject);
        if (NT_SUCCESS(Status))
        {
            /* Set the array to just our entry */
            UseDisplayList = FALSE;
            BcdCount = 1;
            DisplayOrder = &DefaultObject;
        }
        else
        {
            /* No default list either, return success but no entries */
            *BootSequence = NULL;
            *SequenceCount = 0;
            Status = STATUS_SUCCESS;
            DisplayOrder = NULL;
            goto Quickie;
        }
    }

    /* Check if boot.ini was used */
    BootIniCount = 0;
    if (BmBootIniUsed)
    {
        /* Get the entries from it */
        EfiPrintf(L"Boot.ini not supported\r\n");
        BootIniCount = 0;//BmBootIniGetEntryCount();
    }

    /* Allocate an array large enough for the combined boot entries */
    BootEntryCount = BootIniCount + BcdCount;
    Sequence = BlMmAllocateHeap(BootEntryCount * sizeof(*Sequence));
    if (!Sequence)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(Sequence, BootEntryCount * sizeof(*Sequence));

    /* Check if we had BCD entries */
    if (BcdCount)
    {
        /* Populate the list of bootable entries */
        Status = BmpPopulateBootEntryList(BcdHandle,
                                          DisplayOrder,
                                          BL_APPLICATION_ENTRY_DISPLAY_ORDER,
                                          Sequence,
                                          &BcdCount);
        if (!NT_SUCCESS(Status))
        {
            /* Bail out */
            goto Quickie;
        }
    }

    /* Check if we had boot.ini entries */
    if (BootIniCount)
    {
        /* TODO */
        EfiPrintf(L"Boot.ini not supported\r\n");
    }

    /* Return success and the sequence + count populated */
    Status = STATUS_SUCCESS;
    *BootSequence = Sequence;
    *SequenceCount = BootIniCount + BcdCount;

Quickie:
    /* Check if we had allocated a GUID list */
    if ((UseDisplayList) && (DisplayOrder))
    {
        /* Free it */
        BlMmFreeHeap(DisplayOrder);
    }

    /* Check if this is the failure path */
    if (!(NT_SUCCESS(Status)) && (Sequence))
    {
        /* Loop the remaining boot entries */
        while (BootIndex < BootEntryCount)
        {
            /* Check if it had been allocated */
            if (Sequence[BootIndex])
            {
                /* Free it */
                BlMmFreeHeap(Sequence[BootIndex]);
            }

            /* Next*/
            BootIndex++;
        }

        /* Free the whole sequence now */
        BlMmFreeHeap(Sequence);
    }

    /* All done, return the result */
    return Status;
}

VOID
BmpGetDefaultBootEntry (
    _In_ PBL_LOADED_APPLICATION_ENTRY* Sequence,
    _In_ ULONG Count,
    _Out_ PBL_LOADED_APPLICATION_ENTRY* DefaultEntry,
    _Out_ PULONG DefaultIndex
    )
{
    GUID DefaultObject;
    NTSTATUS Status;
    ULONG BootIndex;

    /* Assume no default */
    *DefaultEntry = *Sequence;
    *DefaultIndex = 0;

    /* Nothing to do if there's just one entry */
    if (Count == 1)
    {
        return;
    }

    /* Get the default object, bail out if there isn't one */
    Status = BlGetBootOptionGuid(BlpApplicationEntry.BcdData,
                                 BcdBootMgrObject_DefaultObject,
                                 &DefaultObject);
    if (!(NT_SUCCESS(Status)) || !(Count))
    {
        return;
    }

    /* Scan the boot sequence */
    for (BootIndex = 0; BootIndex < Count; BootIndex++)
    {
        /* Find one that matches the default */
        if (RtlEqualMemory(&Sequence[BootIndex]->Guid,
                           &DefaultObject,
                           sizeof(GUID)))
        {
            /* Return it */
            *DefaultEntry = Sequence[BootIndex];
            *DefaultIndex = BootIndex;
            return;
        }
    }
}

BL_MENU_POLICY
BmGetBootMenuPolicy (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry
    )
{
    NTSTATUS Status;
    BOOLEAN EmsEnabled;
    ULONGLONG BootMenuPolicy;
    ULONG OptionId;

    /* Check if EMS is enabled */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdOSLoaderBoolean_EmsEnabled,
                                    &EmsEnabled);
    if ((NT_SUCCESS(Status)) && (EmsEnabled))
    {
        /* No boot menu */
        return MenuPolicyLegacy;
    }

    /* Check what entry we are looking at */
    if (!BootEntry)
    {
        /* No entry, pick the selected one */
        BootEntry = BmpSelectedBootEntry;
    }

    /* Do we still not have an entry? */
    if (!BootEntry)
    {
        /* Show the menu */
        return MenuPolicyStandard;
    }

    /* Check if this is an OS loader */
    BootMenuPolicy = 0;
    if (BootEntry->Flags & BL_APPLICATION_ENTRY_WINLOAD)
    {
        /* Use the correct option ID */
        OptionId = BcdOSLoaderInteger_BootMenuPolicy;
    }
    else
    {
        /* Check if this is an OS resumer */
        if (!(BootEntry->Flags & BL_APPLICATION_ENTRY_WINRESUME))
        {
            /* Nope, so no reason for a menu */
            return MenuPolicyLegacy;
        }

        /* Use the correct option ID */
        OptionId = BcdResumeInteger_BootMenuPolicy;
    }

    /* Check the option ID for the boot menu policy */
    Status = BlGetBootOptionInteger(BootEntry->BcdData,
                                    OptionId,
                                    &BootMenuPolicy);
    if (NT_SUCCESS(Status))
    {
        /* We have one, return it */
        return BootMenuPolicy;
    }

    /* No policy, so assume no menu */
    return MenuPolicyLegacy;
}

VOID
BmDisplayGetBootMenuStatus (
    _Out_ PL_MENU_STATUS MenuStatus
    )
{
    /* For now, don't support key input at all */
    MenuStatus->AsULong = 0;
    MenuStatus->OemKey = UNICODE_NULL;
    MenuStatus->BootIndex = -1;
}

NTSTATUS
BmProcessCustomAction (
    _In_ HANDLE BcdHandle,
    _In_ PWCHAR ActionKey
    )
{
    EfiPrintf(L"Custom actions not yet handled\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID
BmpProcessBootEntry (
    _In_ HANDLE BcdHandle,
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PBOOLEAN ExitBootManager
    )
{
    BL_MENU_STATUS MenuStatus;

    /* Don't exit */
    *ExitBootManager = FALSE;

    /* If the legacy menu must be shown, or if we have a boot entry */
    if ((BmGetBootMenuPolicy(BootEntry) != MenuPolicyStandard) || (BootEntry))
    {
        /* Check if any key has been pressed */
        BmDisplayGetBootMenuStatus(&MenuStatus);
        if (MenuStatus.AnyKey)
        {
            /* Was the exit key pressed? */
            if (MenuStatus.Exit)
            {
                /* Don't display a menu, and exit */
                *ExitBootManager = TRUE;
                BmpDisplayBootMenu = FALSE;
            }
            else if (MenuStatus.OemKey)
            {
                /* Process the OEM key action */
                BmProcessCustomAction(BcdHandle, &MenuStatus.KeyValue);
            }
            else
            {
                /* Process other keys */
                EfiPrintf(L"TODO\r\n");
            }
        }
    }
}

NTSTATUS
BmpGetSelectedBootEntry (
    _In_ HANDLE BcdHandle,
    _Out_ PBL_LOADED_APPLICATION_ENTRY* SelectedBootEntry,
    _Out_ PULONG EntryIndex,
    _Out_ PBOOLEAN ExitBootManager
    )
{
    NTSTATUS Status;
    PBL_LOADED_APPLICATION_ENTRY* Sequence;
    PBL_LOADED_APPLICATION_ENTRY Entry, SelectedEntry;
    ULONG Count, BootIndex, SelectedIndex;
  //  BOOLEAN FoundFailedEntry;
    ULONGLONG Timeout;

    /* Initialize locals */
    BootIndex = 0;
    Count = 0;
    Sequence = NULL;
    SelectedEntry = NULL;

    /* Enumerate all the boot entries */
    Status = BmEnumerateBootEntries(BcdHandle, &Sequence, &Count);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out if we failed */
        goto Quickie;
    }

    /* Check if there are no entries */
    if (!Count)
    {
        /* This is fatal -- kill the system */
        Status = STATUS_FILE_INVALID;
        BmFatalErrorEx(BL_FATAL_ERROR_BCD_ENTRIES, (ULONG_PTR)L"\\BCD", Status, 0, 0);
        goto Quickie;
    }

    /* Check if we don't yet have an array of failed boot entries */
    if (!BmpFailedBootEntries)
    {
        /* Allocate it */
        BmpFailedBootEntries = BlMmAllocateHeap(Count);
        if (BmpFailedBootEntries)
        {
            /* Zero it out */
            RtlZeroMemory(BmpFailedBootEntries, Count);
        }
    }

    /* Check if we have a hardcoded boot override */
    if (BmBootEntryOverridePresent)
    {
        EfiPrintf(L"Hard-coded boot override mode not supported\r\n");
    }

    /* Log the OS count */
    //BlLogEtwWrite(BOOT_BOOTMGR_MULTI_OS_COUNT);

    /* Check if the display is already active and cached */
    if (!BmDisplayStateCached)
    {
        /* Check if we should display a boot menu */
        Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                        BcdBootMgrBoolean_DisplayBootMenu,
                                        &BmpDisplayBootMenu);
        if (!NT_SUCCESS(Status))
        {
            /* Assume not */
            BmpDisplayBootMenu = FALSE;
        }
    }

    /* Check if there's only one entry to boot anyway */
    if (Count == 1)
    {
        /* Read it */
        SelectedEntry = *Sequence;

        /* Process it */
        BmpProcessBootEntry(BcdHandle, SelectedEntry, ExitBootManager);

        /* Check if we're not displaying a boot menu */
        if (!BmpDisplayBootMenu)
        {
            /* Now we are */
            BmpDisplayBootMenu = TRUE;

            /* Return the entry and its index back */
            *EntryIndex = 0;
            *SelectedBootEntry = SelectedEntry;
            Status = STATUS_SUCCESS;
            goto Quickie;
        }
    }
    else
    {
        /* Get the default boot entry */
        BmpGetDefaultBootEntry(Sequence, Count, &SelectedEntry, &SelectedIndex);

        /* Check if we have a failed boot entry array allocated */
        //FoundFailedEntry = FALSE;
        if (BmpFailedBootEntries)
        {
            /* Check if the default entry failed to boot */
            if (BmpFailedBootEntries[SelectedIndex])
            {
                /* Loop through the current boot sequence */
                for (SelectedIndex = 0; SelectedIndex < Count; SelectedIndex++)
                {
                    /* Check if there's no sequence for this index, or it failed */
                    while (!(Sequence[SelectedIndex]) ||
                            (BmpFailedBootEntries[SelectedIndex]))
                    {
                        /* Remember that this is a failed entry */
                        SelectedEntry = Sequence[SelectedIndex];
                        //FoundFailedEntry = TRUE;
                        BmpDisplayBootMenu = FALSE;
                    }
                }
            }
        }

        /* Check if the entry is an OS loader */
        if (SelectedEntry->Flags & BL_APPLICATION_ENTRY_WINLOAD)
        {
            // todo
            EfiPrintf(L"todo path\r\n");
        }

        /* Check if there's no timeout */
        Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                        BcdBootMgrInteger_Timeout,
                                        &Timeout);
        if ((NT_SUCCESS(Status) && !(Timeout)))
        {
            /* There isn't, so just process the default entry right away */
            BmpProcessBootEntry(BcdHandle, SelectedEntry, ExitBootManager);

            /* Check if we're not displaying a boot menu */
            if (!BmpDisplayBootMenu)
            {
                /* Now we are */
                BmpDisplayBootMenu = TRUE;

                /* Return the entry and its index back */
                *EntryIndex = 0;
                *SelectedBootEntry = SelectedEntry;
                Status = STATUS_SUCCESS;
                goto Quickie;
            }

            /* Remove the timeout for this boot instance */
            BlRemoveBootOption(BlpApplicationEntry.BcdData,
                               BcdBootMgrInteger_Timeout);
        }
    }

    /* Here is where we display the menu and list of tools */
    EfiPrintf(L"Tool selection not yet implemented\r\n");
    EfiStall(10000000);
    *SelectedBootEntry = NULL;

Quickie:
    /* We are done -- did we have a sequence? */
    if (Sequence)
    {
        /* Do we have any boot entries we parsed? */
        while (BootIndex < Count)
        {
            /* Get the current boot entry */
            Entry = Sequence[BootIndex];

            /* Did we fail, or is is not the selected one? */
            if ((Entry) && ((Entry != SelectedEntry) || !(NT_SUCCESS(Status))))
            {
                /* Destroy it, as it won't be needed */
                BlDestroyBootEntry(Entry);
            }
            else if (Entry == SelectedEntry)
            {
                /* It's the selected one, return its index */
                *EntryIndex = BootIndex;
            }

            /* Move to the next entry */
            BootIndex++;
        }

        /* Free the sequence of entries */
        BlMmFreeHeap(Sequence);
    }

    /* Return the selection result */
    return Status;
}

NTSTATUS
BmLaunchRecoverySequence (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _In_ ULONG LaunchCode
    )
{
    NTSTATUS Status;
    PBL_LOADED_APPLICATION_ENTRY RecoveryEntry;
    HANDLE BcdHandle;
    PGUID RecoverySequence;
    ULONG Count, i, RecoveryIndex, SequenceCount;
    PBL_LOADED_APPLICATION_ENTRY* Sequence;

    /* Initialize locals */
    RecoveryIndex = 0;
    Sequence = NULL;
    RecoverySequence = NULL;
    Count = 0;
    BcdHandle = NULL;

    /* Open the BCD*/
    Status = BmOpenDataStore(&BcdHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the recovery sequence list */
    Status = BlGetBootOptionGuidList(BootEntry->BcdData,
                                     BcdLibraryObjectList_RecoverySequence,
                                     &RecoverySequence,
                                     &SequenceCount);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the sequence of boot entries out of it */
    Status = BmGetBootSequence(BcdHandle,
                               RecoverySequence,
                               SequenceCount,
                               BL_APPLICATION_ENTRY_RECOVERY,
                               &Sequence,
                               &Count);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Was the BCD open? */
    if (BcdHandle)
    {
        /* Close it */
        BmCloseDataStore(BcdHandle);
    }

    /* Now go over every entry in the sequence */
    for (i = 0; i < Count; ++i)
    {
        /* Check the code for this recovery launch */
        if (LaunchCode == 2 || LaunchCode == 5)
        {
            /* Remove the override if there is one, and set it to 4 */
            BlRemoveBootOption(Sequence[i]->BcdData, BcdLibraryInteger_DisplayMessageOverride);
            BlAppendBootOptionInteger(Sequence[i],
                                      BcdLibraryInteger_DisplayMessageOverride,
                                      4);
        }
        else if (LaunchCode == 3)
        {
            /* Remove the override if there is one, and set it to 10 */
            BlRemoveBootOption(Sequence[i]->BcdData, BcdLibraryInteger_DisplayMessageOverride);
            BlAppendBootOptionInteger(Sequence[i],
                                        BcdLibraryInteger_DisplayMessageOverride,
                                        10);
        }

        /* Launch the boot entry for this part of the recovery sequence */
        Status = BmpLaunchBootEntry(Sequence[i], NULL, LaunchCode, FALSE);
        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

Quickie:
    /* Did we have a sequence of entries? */
    if (Sequence)
    {
        /* Loop through each one */
        for (RecoveryIndex = 0; RecoveryIndex < Count; RecoveryIndex++)
        {
            /* Does this index have an allocated boot entry? */
            RecoveryEntry = Sequence[RecoveryIndex];
            if (RecoveryEntry)
            {
                /* Destroy it */
                BlDestroyBootEntry(RecoveryEntry);
            }
        }

        /* Free the sequence itself */
        BlMmFreeHeap(Sequence);
    }

    /* Was there a sequence list? */
    if (RecoverySequence)
    {
        /* Free it */
        BlMmFreeHeap(RecoverySequence);
    }

    /* Return back to caller */
    return Status;
}

ULONG
BmDisplayDumpError (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _In_ ULONG LaunchCode
    )
{
    ULONG BootError;
    NTSTATUS Status;
    BOOLEAN Restart, NoError;

    /* Assume we'll just reboot */
    BootError = Reboot;

    /* Should we reboot? */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdLibraryBoolean_RestartOnFailure,
                                    &Restart);
    if ((NT_SUCCESS(Status)) && (Restart))
    {
        return BootError;
    }

    /* Should we not show errors, and thus, reboot? */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdBootMgrBoolean_NoErrorDisplay,
                                    &NoError);
    if ((NT_SUCCESS(Status)) && (NoError))
    {
        return BootError;
    }

    /* Is there an internal boot error? */
    if (BmpInternalBootError)
    {
        /* Return it -- but it's a pointer? */
        return (ULONG_PTR)BmpInternalBootError; // ???
    }

    /* Otherwise, show the menu to see what to do */
    EfiPrintf(L"Error menu not yet implemented\r\n");
    return BootError;
}

NTSTATUS
BmpCreateDevices (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry
    )
{
    ULONG NextOffset, DataOffset, ListOffset;
    PBL_BCD_OPTION Option, ListOption;
    BcdElementType ElementType;
    PBCD_DEVICE_OPTION BcdDevice;

    /* Starting at offset 0, loop every BCD option */
    NextOffset = 0;
    do
    {
        /* Get the current option, and its offset */
        Option = (PBL_BCD_OPTION)((ULONG_PTR)BootEntry->BcdData + NextOffset);
        NextOffset = Option->NextEntryOffset;

        /* If it's empty, ignore it */
        if (Option->Empty)
        {
            continue;
        }

        /* If it's not a device option, ignore it */
        ElementType.PackedValue = Option->Type;
        if (ElementType.Format != BCD_TYPE_DEVICE)
        {
            continue;
        }

        /* Get the data offset */
        DataOffset = Option->DataOffset;

        /* Extract the device out of it */
        BcdDevice = (PBCD_DEVICE_OPTION)((ULONG_PTR)BootEntry->BcdData + DataOffset);

        /* If the device is already fully specified, no need to build it */
        if (!(BcdDevice->DeviceDescriptor.Flags & 1))
        {
            continue;
        }

        /* Otherwise, check if there's any list options as well */
        ListOption = NULL;
        ListOffset = Option->ListOffset;
        if (Option->ListOffset)
        {
            ListOption = (PBL_BCD_OPTION)((ULONG_PTR)BootEntry->BcdData + ListOffset);
        }

        /* And now call BlCreateDevice to build the full device descriptor */
        EfiPrintf(L"Unspecified devices not yet supported: %p\r\n", ListOption);
        return STATUS_NOT_SUPPORTED;
    } while (NextOffset != 0);

    /* Devices created successfully */
    return STATUS_SUCCESS;
}

NTSTATUS
BmpTransferExecution (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PULONG LaunchCode,
    _Out_ PBOOLEAN Recover
    )
{
    PWCHAR AppPath;
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR AppDevice;
    BL_RETURN_ARGUMENTS ReturnArgs;
    BOOLEAN AdvancedOptions;
    ULONG AppHandle;

    /* Get the application path */
    Status = BlGetBootOptionString(BootEntry->BcdData,
                                   BcdLibraryString_ApplicationPath,
                                   &AppPath);
    if (!NT_SUCCESS(Status))
    {
        /* If we couldn't find one, set this to NULL */
        AppPath = NULL;
    }

    /* Check if this is a PXE startup.com */
    if (BootEntry->Flags & BL_APPLICATION_ENTRY_STARTUP)
    {
#if BL_NET_SUPPORT
        /* Do soft reboot to launch it */
        Status = BlNetSoftReboot(BootEntry);
#else
        EfiPrintf(L"Net boot not supported\r\n");
        Status = STATUS_NOT_SUPPORTED;
#endif
        /* Nothing else for us to do */
        goto Quickie;
    }

    /* Loop as long as boot was not cancelled */
    do
    {
        /* Load the boot application */
        Status = BlImgLoadBootApplication(BootEntry, &AppHandle);

        /* Did we not find it? */
        if (Status == STATUS_NOT_FOUND)
        {
            /* Get the device for the boot application */
            Status = BlGetBootOptionDevice(BootEntry->BcdData,
                                           BcdLibraryDevice_ApplicationDevice,
                                           &AppDevice,
                                           NULL);
            if (!NT_SUCCESS(Status))
            {
                /* Force re-enumeration */
                Status = BlFwEnumerateDevice(AppDevice);
            }

            /* Did re-enumeration work? */
            if (!NT_SUCCESS(Status))
            {
                /* Nope, raise a fatal error */
                BmFatalErrorEx(BL_FATAL_ERROR_APP_LOAD,
                               (ULONG_PTR)AppPath,
                               Status,
                               0,
                               0);
                goto Quickie;
            }

            /* Yes, try booting it again */
            Status = BlImgLoadBootApplication(BootEntry, &AppHandle);
        }

        /* Was boot cancelled?*/
        if (Status == STATUS_CANCELLED)
        {
            /* Should we display the menu, or is there no launch sequence? */
            if ((BmGetBootMenuPolicy(BootEntry) != MenuPolicyStandard) ||
                !(MiscGetBootOption(BootEntry->BcdData,
                                    BcdLibraryObjectList_RecoverySequence)))
            {
                /* Bail out, the menu will take care of it */
                goto Quickie;
            }

            /* No menu and there's a sequence, launch it */
            *LaunchCode = 4;
            *Recover = TRUE;
            goto Quickie;
        }

        /* STATUS_FVE_LOCKED_VOLUME -- bitlocker volume is locked */
        if (Status == STATUS_FVE_LOCKED_VOLUME)
        {
            /* Launch recovery mode */
            *LaunchCode = 4;
            *Recover = TRUE;
            goto Quickie;
        }

        /* Was there some other error launching the boot application? */
        if (!NT_SUCCESS(Status))
        {
            /* Raise a fatal error */
            BmFatalErrorEx(BL_FATAL_ERROR_APP_LOAD,
                           (ULONG_PTR)AppPath,
                           Status,
                           0,
                           0);
            goto Quickie;
        }

        /* Zero out the return arguments */
        RtlZeroMemory(&ReturnArgs, sizeof(ReturnArgs));

        /* Log to ETW this launch */
        //BmpLogApplicationLaunchEvent(&BootEntry->Guid, AppPath);

        /* Launch the boot application*/
        Status = BlImgStartBootApplication(AppHandle, &ReturnArgs);

#if BL_BITLOCKER_SUPPORT
        /* Bitlocker stuff */
        BlFveSecureBootCheckpointAppReturn(BootEntry, &ReturnArgs);
#endif

        /* Log in the boot status log the launch */
        //BlBsdLogEntry(1, 0x12, &BootEntry->Guid, 0x14);

        /* Unloac the boot application if we've returned */
        BlImgUnloadBootApplication(AppHandle);

        /* Keep going unless STATUS_RESTART_BOOT_APPLICATION */
    } while (Status != 0xC0000453);

    /* We've come back. Assume we need to launch the recovery sequence */
    *Recover = TRUE;

    /* Why did we get back? */
    if (ReturnArgs.Flags & 1)
    {
        /* Flag 1 -- should we display advanced options? */
        Status = BlGetBootOptionBoolean(BootEntry->BcdData,
                                        BcdLibraryBoolean_DisplayAdvancedOptions,
                                        &AdvancedOptions);
        if ((NT_SUCCESS(Status)) && (AdvancedOptions))
        {
            /* Yes, so return with code 2 */
            *LaunchCode = 2;
        }
        else
        {
            /* No, return with code 1 */
            *LaunchCode = 1;
        }
    }
    else if (ReturnArgs.Flags & 4)
    {
        /* Flag 4 -- unknown */
        *LaunchCode = 1;
    }
    else if (ReturnArgs.Flags & 8)
    {
        /* Flag 5 -- unknown */
        *LaunchCode = 5;
    }
    else if (ReturnArgs.Flags & 0x10)
    {
        /* Flag 6 -- unknown */
        *LaunchCode = 6;
    }
    else if (ReturnArgs.Flags & 0x20)
    {
        /* Flag 7 -- unknown */
        *LaunchCode = 7;
    }
    else if (ReturnArgs.Flags & BL_RETURN_ARGUMENTS_NO_PAE_FLAG)
    {
        /* PAE is not supported -- refuse to boot */
        *Recover = FALSE;
        BmFatalErrorEx(BL_FATAL_ERROR_NO_PAE, Status, 0, 0, 0);
    }

Quickie:
    /* All done, did we have an application path? */
    if (AppPath)
    {
        /* Free it */
        BlMmFreeHeap(AppPath);
    }

    /* Back to the caller now */
    return Status;
}

NTSTATUS
BmpLaunchBootEntry (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PULONG EntryIndex,
    _In_ ULONG LaunchCode,
    _In_ BOOLEAN LaunchWinRe
    )
{
    HANDLE BcdHandle;
    NTSTATUS Status;
    GUID ObjectId;
    BOOLEAN DoRecovery, AutoRecovery, DoSequence, RestartOnFailure;
    ULONG ErrorCode;
    BOOLEAN AdvancedOneTime, EditOneTime;

    /* Check if this is the OS loader */
    if (BootEntry->Flags & BL_APPLICATION_ENTRY_WINLOAD)
    {
        /* Check if one-time advanced options should be shown */
        if (MiscGetBootOption(BootEntry->BcdData,
                              BcdOSLoaderBoolean_AdvancedOptionsOneTime))
        {
            /* Open the BCD */
            BcdHandle = NULL;
            Status = BmOpenDataStore(BcdHandle);
            if (NT_SUCCESS(Status))
            {
                /* Delete the option from the BCD, so it doesn't happen again */
                ObjectId = BootEntry->Guid;
                BmPurgeOption(BcdHandle,
                              &ObjectId,
                              BcdOSLoaderBoolean_AdvancedOptionsOneTime);
                BmCloseDataStore(BcdHandle);
            }
        }

        /* Check if one-time options editor should be shown */
        if (MiscGetBootOption(BootEntry->BcdData,
                              BcdOSLoaderBoolean_OptionsEditOneTime))
        {
            /* Open the BCD */
            BcdHandle = NULL;
            Status = BmOpenDataStore(BcdHandle);
            if (NT_SUCCESS(Status))
            {
                /* Delete the option from the BCD, so it doesn't happen again */
                ObjectId = BootEntry->Guid;
                BmPurgeOption(BcdHandle,
                              &ObjectId,
                              BcdOSLoaderBoolean_OptionsEditOneTime);
                BmCloseDataStore(BcdHandle);
            }
        }
    }

TryAgain:
    /* Disable recovery mode */
    DoRecovery = FALSE;

    /* Store globally which entry we are trying to boot */
    BmpSelectedBootEntry = BootEntry;

    /* Create any devices that aren't yet fully defined for this boot entry */
    Status = BmpCreateDevices(BootEntry);
    if (!NT_SUCCESS(Status))
    {
        /* That failed -- can we launch the recovery environment? */
        if (!LaunchWinRe)
        {
            return Status;
        }

        /* Yes, so return with the WinRe launch code */
        LaunchCode = 2;
        goto Quickie;
    }

    /* Is this an OS loader/ */
    if (BootEntry->Flags & BL_APPLICATION_ENTRY_WINLOAD)
    {
        /* Is the one-time advanced options menu option present? */
        Status = BlGetBootOptionBoolean(BootEntry->BcdData,
                                        BcdOSLoaderBoolean_AdvancedOptionsOneTime,
                                        &AdvancedOneTime);
        if (NT_SUCCESS(Status))
        {
            /* Is it turned on? */
            if (AdvancedOneTime)
            {
                /* Set the option this once */
                BlAppendBootOptionBoolean(BootEntry,
                                          BcdLibraryBoolean_DisplayAdvancedOptions,
                                          TRUE);
            }
            else
            {
                /* It's not, so disable the option if active */
                BlRemoveBootOption(BootEntry->BcdData,
                                   BcdLibraryBoolean_DisplayAdvancedOptions);
            }

            /* Remove the one-time option. We've already purged it earlier */
            BlRemoveBootOption(BootEntry->BcdData,
                               BcdOSLoaderBoolean_AdvancedOptionsOneTime);
        }

        /* Is the one-time options editor menu option present? */
        Status = BlGetBootOptionBoolean(BootEntry->BcdData,
                                        BcdOSLoaderBoolean_OptionsEditOneTime,
                                        &EditOneTime);
        if (NT_SUCCESS(Status))
        {
            /* Is it turned on? */
            if (EditOneTime)
            {
                /* Set the option this once */
                BlAppendBootOptionBoolean(BootEntry,
                                          BcdLibraryBoolean_DisplayOptionsEdit,
                                          TRUE);
            }
            else
            {
                /* It's not, so disable the option if active */
                BlRemoveBootOption(BootEntry->BcdData,
                                   BcdLibraryBoolean_DisplayOptionsEdit);
            }

            /* Remove the one-time option. We've already purged it earlier */
            BlRemoveBootOption(BootEntry->BcdData,
                               BcdOSLoaderBoolean_OptionsEditOneTime);
        }
    }

    /* BCD handling done, transfer execution to this entry */
    Status = BmpTransferExecution(BootEntry, &LaunchCode, &DoRecovery);
    if (!LaunchWinRe)
    {
        return Status;
    }

    /* Check if boot was successful, or  cancelled and we're not doing WinRE */
    if (((NT_SUCCESS(Status)) || (Status == STATUS_CANCELLED)) && !(DoRecovery))
    {
        return Status;
    }

    /* Boot failed -- are we doing recovery? */
    if (!DoRecovery)
    {
        /* Nope, bail out */
        LaunchCode = 2;
        goto Quickie;
    }

Quickie:
    /* Get the recovery sequence */
    if (MiscGetBootOption(BootEntry->BcdData, BcdLibraryObjectList_RecoverySequence))
    {
        /* Check if the launch depends on auto-recovery being enabled or not */
        if ((LaunchCode == 3) || (LaunchCode == 5) || (LaunchCode == 6))
        {
            Status = BlGetBootOptionBoolean(BootEntry->BcdData,
                                            BcdLibraryBoolean_AutoRecoveryEnabled,
                                            &AutoRecovery);
            if (NT_SUCCESS(Status))
            {
                /* Override the setting */
                DoRecovery = AutoRecovery;
            }
        }
    }
    else
    {
        /* There's no recovery setting */
        DoRecovery = FALSE;
    }

    /* Check if we should restart on failure */
    RestartOnFailure = FALSE;
    BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                           BcdLibraryBoolean_RestartOnFailure,
                           &RestartOnFailure);

    /* Do the sequence if recovery is on, unless we should restart instead */
    DoSequence = RestartOnFailure ? FALSE : DoRecovery;
    while (1)
    {
        /* Are we doing the recovery sequence? */
        if (DoSequence)
        {
            /* Because of automatic recovery? */
            if (AutoRecovery)
            {
#if BL_BITLOCKER_SUPPORT
                /* Do bitlocker stuff */
                BlFveRegisterBootEntryForTrustedWimBoot(BootEntry, TRUE);
#endif
            }

            /* Launch the recovery sequence*/
            Status = BmLaunchRecoverySequence(BootEntry, LaunchCode);

            /* Was it launched automatically? */
            if (AutoRecovery)
            {
#if BL_BITLOCKER_SUPPORT
                /* Do bitlocker stuff */
                BlFveRegisterBootEntryForTrustedWimBoot(BootEntry, FALSE);
#endif

                /* No need to do this again */
                AutoRecovery = FALSE;
            }

            /* Did the recovery sequence work? */
            if (NT_SUCCESS(Status))
            {
                /* All good */
                return STATUS_SUCCESS;
            }

            /* Remove the sequence, don't do it again */
            BlRemoveBootOption(BootEntry->BcdData, BcdLibraryObjectList_RecoverySequence);
        }

        /* Recovery sequence also failed, show fatal error */
        if (!BmpInternalBootError)
        {
            BmFatalErrorEx(BL_FATAL_ERROR_GENERIC, Status, 0, 0, 0);
        }

        /* Display the error menu */
        ErrorCode = BmDisplayDumpError(BootEntry, LaunchCode);
        BmErrorPurge();

        /* See what the user wants to do */
        switch (ErrorCode)
        {
            case TryAgain:
                /* Try again */
                goto TryAgain;

            case NextOs:
                /* Boot the next entry*/
                break;

            case OsSelection:
                /* Cancel the boot*/
                return STATUS_CANCELLED;

            case RecoverOem:
                /* Custom OEM recovery -- open the BCD */
                Status = BmOpenDataStore(BcdHandle);
                if (NT_SUCCESS(Status))
                {
                    /* See what the custom sequence is */
                    Status = BmProcessCustomAction(BcdHandle, NULL);
                }

                /* All done, close the BCD */
                if (BcdHandle)
                {
                    BmCloseDataStore(BcdHandle);
                }
                return Status;

            case AdvancedOptions:
                /* Show the advanced options next iteration */
                BlAppendBootOptionBoolean(BootEntry,
                                          BcdOSLoaderBoolean_AdvancedOptionsOneTime,
                                          TRUE);
                goto TryAgain;

            case BootOptions:
                /* Show the options editor next iteration */
                BlAppendBootOptionBoolean(BootEntry,
                                          BcdOSLoaderBoolean_OptionsEditOneTime,
                                          TRUE);
                goto TryAgain;

            case Recover:
                /* Try the recovery sequence next time*/
                DoSequence = TRUE;
                LaunchCode = 1;
                goto TryAgain;

            default:
                /* Something unknown */
                return STATUS_CANCELLED;
        }
    }

    /* We are booting the next OS, so return success as to not kill the boot */
    return STATUS_SUCCESS;
}

/*++
 * @name BmMain
 *
 *     The BmMain function implements the Windows Boot Application entrypoint for
 *     the Boot Manager.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @return NT_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
NTSTATUS
NTAPI
BmMain (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters
    )
{
    NTSTATUS Status, LibraryStatus;
    BL_LIBRARY_PARAMETERS LibraryParameters;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    PGUID AppIdentifier;
    HANDLE BcdHandle, ResumeBcdHandle;
    PBL_BCD_OPTION EarlyOptions;
    PWCHAR Stylesheet;
    BOOLEAN XmlLoaded, DisableIntegrity, TestSigning, PersistBootSequence;
    BOOLEAN RebootOnError, CustomActions;
    ULONG SequenceId;
    PBL_LOADED_APPLICATION_ENTRY BootEntry;
    PGUID SequenceList;
    ULONG SequenceListCount;
    PBL_LOADED_APPLICATION_ENTRY* BootSequence;
    ULONG BootIndex;
    BOOLEAN ExitBootManager;
    BOOLEAN BootFailed;
    BOOLEAN BootOk;
    ULONG SequenceCount;
    BOOLEAN GetEntry;
    EfiPrintf(L"ReactOS UEFI Boot Manager Initializing...\r\n");

    /* Reading the BCD can change this later on */
    RebootOnError = FALSE;

    /* Save the start/end-of-POST time */
#if defined(_M_IX86) || defined(_M_X64)
    ApplicationStartTime = __rdtsc();
#else
    EfiPrintf(L"No time source defined for this platform\r\n");
    ApplicationStartTime = 0;
#endif
    PostTime = ApplicationStartTime;

    /* Setup the boot library parameters for this application */
    BlSetupDefaultParameters(&LibraryParameters);
    LibraryParameters.TranslationType = BlNone;
    LibraryParameters.LibraryFlags = 0x400 | 0x8;
    LibraryParameters.MinimumAllocationCount = 16;
    LibraryParameters.MinimumHeapSize = 512 * 1024;

    /* Initialize the boot library */
    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);
    if (!NT_SUCCESS(Status))
    {
        /* Check for failure due to invalid application entry */
        if (Status != STATUS_INVALID_PARAMETER_9)
        {
            /* Specifically print out what happened */
            EfiPrintf(L"BlInitializeLibrary failed 0x%x\r\n", Status);
        }

        /* Go to exit path */
        goto Quickie;
    }

    /* Get the application identifier */
    AppIdentifier = BlGetApplicationIdentifier();
    if (!AppIdentifier)
    {
        /* None was given, so set our default one */
        AppIdentifier = (PGUID)&GUID_WINDOWS_BOOTMGR;
    }

    /* Save our identifier */
    BmApplicationIdentifier = *AppIdentifier;

    /* Initialize the file system to open a handle to our root boot directory */
    BmFwInitializeBootDirectoryPath();

    /* Load and initialize the boot configuration database (BCD) */
    Status = BmOpenDataStore(&BcdHandle);
    if (NT_SUCCESS(Status))
    {
        /* Copy the boot options */
        Status = BlCopyBootOptions(BlpApplicationEntry.BcdData, &EarlyOptions);
        if (NT_SUCCESS(Status))
        {
            /* Update them */
            Status = BmpUpdateApplicationOptions(BcdHandle);
            if (!NT_SUCCESS(Status))
            {
                /* Log a fatal error */
                BmFatalErrorEx(BL_FATAL_ERROR_BCD_PARSE,
                               (ULONG_PTR)L"\\BCD",
                               Status,
                               0,
                               0);
            }
        }
    }

#ifdef _SECURE_BOOT
    /* Initialize the secure boot machine policy */
    Status = BmSecureBootInitializeMachinePolicy();
    if (!NT_SUCCESS(Status))
    {
        BmFatalErrorEx(BL_FATAL_ERROR_SECURE_BOOT, Status, 0, 0, 0);
    }
#endif

    /* Copy the library parameters and add the re-initialization flag */
    RtlCopyMemory(&LibraryParameters,
                  &BlpLibraryParameters,
                  sizeof(LibraryParameters));
    LibraryParameters.LibraryFlags |= (BL_LIBRARY_FLAG_REINITIALIZE_ALL |
                                       BL_LIBRARY_FLAG_REINITIALIZE);

    /* Now that we've parsed the BCD, re-initialize the library */
    LibraryStatus = BlInitializeLibrary(BootParameters, &LibraryParameters);
    if (!NT_SUCCESS(LibraryStatus) && (NT_SUCCESS(Status)))
    {
        Status = LibraryStatus;
    }

    /* Initialize firmware-specific memory regions */
    BmFwMemoryInitialize();

    /* Initialize the boot status data log (BSD) */
    BmpInitializeBootStatusDataLog();

    /* Find our XSL stylesheet */
    Stylesheet = BlResourceFindHtml();
    if (!Stylesheet)
    {
        /* Awe, no XML. This is actually fatal lol. Can't boot without XML. */
        Status = STATUS_NOT_FOUND;
        EfiPrintf(L"BlResourceFindMessage failed 0x%x\r\n", STATUS_NOT_FOUND);
        goto Quickie;
    }

    /* Initialize the XML Engine (as a side-effect, resets cursor) */
    Status = BlXmiInitialize(Stylesheet);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"\r\nBlXmiInitialize failed 0x%x\r\n", Status);
        goto Failure;
    }
    XmlLoaded = TRUE;

    /* Check if there's an active bitmap visible */
    if (!BlDisplayValidOemBitmap())
    {
        /* Nope, make the screen black using BGFX */
        if (!NT_SUCCESS(BmpBgDisplayClearScreen(0xFF000000)))
        {
            /* BGFX isn't active, use standard display */
            BlDisplayClearScreen();
        }
    }

#ifdef _BIT_LOCKER_
    /* Bitlocker will take over screen UI if enabled */
    FveDisplayScreen = BmFveDisplayScreen;
#endif

    /* Check if any bypass options are enabled */
    BlImgQueryCodeIntegrityBootOptions(&BlpApplicationEntry,
                                       &DisableIntegrity,
                                       &TestSigning);
    if (!DisableIntegrity)
    {
        /* Integrity checks are enabled, so validate our signature */
        Status = BmFwVerifySelfIntegrity();
        if (!NT_SUCCESS(Status))
        {
            /* Signature invalid, fail boot */
            goto Failure;
        }
    }


    /* TEST MODE */
    EfiPrintf(L"Performing memory allocator tests...\r\n");
    {
        NTSTATUS Status;
        PHYSICAL_ADDRESS PhysicalAddress, PhysicalAddress2;

        /* Allocate 1 physical page */
        PhysicalAddress.QuadPart = 0;
        Status = BlMmAllocatePhysicalPages(&PhysicalAddress, BlLoaderData, 1, 0, 1);
        if (Status != STATUS_SUCCESS)
        {
            EfiPrintf(L"FAIL: Allocation status: %lx at address: %llx\r\n", Status, PhysicalAddress.QuadPart);
            EfiStall(100000000);
        }

        /* Write some data */
        *(PULONG)((ULONG_PTR)PhysicalAddress.QuadPart) = 0x55555151;

        /* Free it */
        Status = BlMmFreePhysicalPages(PhysicalAddress);
        if (Status != STATUS_SUCCESS)
        {
            EfiPrintf(L"FAIL: Memory free status: %lx\r\n", Status);
            EfiStall(100000000);
        }

        /* Allocate a page again */
        PhysicalAddress2.QuadPart = 0;
        Status = BlMmAllocatePhysicalPages(&PhysicalAddress2, BlLoaderData, 1, 0, 1);
        if (Status != STATUS_SUCCESS)
        {
            EfiPrintf(L"FAIL: Allocation status: %lx at address: %llx\r\n", Status, PhysicalAddress2.QuadPart);
            EfiStall(100000000);
        }

        /* It should've given us the same page, since we freed it */
        if (PhysicalAddress.QuadPart != PhysicalAddress2.QuadPart)
        {
            EfiPrintf(L"FAIL: Non-matching addresses: %llx %llx\r\n", PhysicalAddress.QuadPart, PhysicalAddress2.QuadPart);
            EfiStall(100000000);
        }

        /* The data should still be there, since zero-ing is not on for bootmgr */
        if (*(PULONG)((ULONG_PTR)PhysicalAddress2.QuadPart) != 0x55555151)
        {
            EfiPrintf(L"FAIL: Non-matching data: %lx %lx\r\n", 0x55555151, *(PULONG)((ULONG_PTR)PhysicalAddress2.QuadPart));
            EfiStall(100000000);
        }

        /* And free the second page again */
        Status = BlMmFreePhysicalPages(PhysicalAddress);
        if (Status != STATUS_SUCCESS)
        {
            EfiPrintf(L"FAIL: Memory free status: %lx\r\n", Status);
            EfiStall(100000000);
        }
    }

    /* Write out the first XML tag */
    BlXmiWrite(L"<bootmgr/>");

    /* Check for factory reset */
    BlSecureBootCheckForFactoryReset();

    /* Load the revocation list */
    Status = BmFwRegisterRevocationList();
    if (!NT_SUCCESS(Status))
    {
        goto Failure;
    }

    /* Register our custom progress routine */
    BlUtlRegisterProgressRoutine();

    /* Display state is not currently cached */
    BmDisplayStateCached = FALSE;

    /* Check if we need to resume from hibernate */
    Status = BmResumeFromHibernate(&ResumeBcdHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Failure;
    }

#ifdef BL_NET_SUPPORT
    /* Register multicast printing routine */
    BlUtlRegisterMulticastRoutine();
#endif

    /* Check if restart on failure is enabled */
    BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                           BcdLibraryBoolean_RestartOnFailure,
                           &RebootOnError);

    /* Check if the boot sequence is persisted */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdBootMgrBoolean_PersistBootSequence,
                                    &PersistBootSequence);
    if (!NT_SUCCESS(Status))
    {
        /* It usually is */
        PersistBootSequence = TRUE;
    }

    /* Check if there's custom actions to take */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdBootMgrBoolean_ProcessCustomActionsFirst,
                                    &CustomActions);
    if ((NT_SUCCESS(Status)) && (CustomActions))
    {
        /* We don't support this yet */
        EfiPrintf(L"Not implemented\r\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Failure;
    }

    //BlResourceFindMessage(BM_MSG_TEST);

    /* At last, enter the boot selection stage */
    SequenceId = 0;
    GetEntry = FALSE;
    BootFailed = FALSE;
    SequenceList = NULL;
    BootSequence = NULL;
    SequenceCount = 0;
    while (1)
    {
        /* We don't have a boot entry nor a sequence ID */
        BootEntry = NULL;
        BootOk = FALSE;

        /* Do we have a hardcoded boot sequence set? */
        if (!(BootSequence) && !(GetEntry))
        {
            /* Not yet, read the BCD to see if one is there */
            Status = BlGetBootOptionGuidList(BlpApplicationEntry.BcdData,
                                             BcdBootMgrObjectList_BootSequence,
                                             &SequenceList,
                                             &SequenceListCount);
            if (NT_SUCCESS(Status))
            {
                /* A GUID list for the boot sequence is set. Extract it */
                Status = BmGetBootSequence(BcdHandle,
                                           SequenceList,
                                           SequenceListCount,
                                           BL_APPLICATION_ENTRY_FIXED_SEQUENCE,
                                           &BootSequence,
                                           &SequenceCount);
                if (NT_SUCCESS(Status))
                {
                    /* Don't get stuck in a loop repeating this sequence */
                    BlRemoveBootOption(BlpApplicationEntry.BcdData,
                                       BcdBootMgrObjectList_BootSequence);

                    /* But do check if we should persist it */
                    if (PersistBootSequence)
                    {
                        /* Yes -- so go select an entry now */
                        GetEntry = TRUE;
                    }
                    else
                    {
                        /* We shouldn't, so wipe it from the BCD too */
                        Status = BmPurgeOption(BcdHandle,
                                               &BmApplicationIdentifier,
                                               BcdBootMgrObjectList_BootSequence);
                        if (!NT_SUCCESS(Status))
                        {
                            /* Well that failed */
                            goto LoopQuickie;
                        }
                    }
                }
            }
            else
            {
                /* No boot entry sequence for us */
                BootSequence = NULL;
            }
        }

        /* Do we have a sequence active, and are we still processing it? */
        if ((BootSequence) && ((GetEntry) || (SequenceId < SequenceCount)))
        {
            /* Extract the next entry in the sequence */
            BootEntry = BootSequence[SequenceId];
            BootSequence[SequenceId] = NULL;

            /* Move to the next entry for next time */
            SequenceId++;

            /* Unless there won't be a a next time? */
            if (SequenceId == SequenceCount)
            {
                /* Clean up, it's the last entry */
                BlMmFreeHeap(BootSequence);
                BootSequence = NULL;
            }
        }
        else
        {
            /* Get the selected boot entry from the user */
            ExitBootManager = FALSE;
            Status = BmpGetSelectedBootEntry(BcdHandle,
                                             &BootEntry,
                                             &BootIndex,
                                             &ExitBootManager);
            if (!(NT_SUCCESS(Status)) || (ExitBootManager))
            {
                /* Selection failed, or user wants to exit */
                goto LoopQuickie;
            }
        }

        /* Did we have a BCD open? */
        if (BcdHandle)
        {
            /* Close it, we'll be opening a new one */
            BmCloseDataStore(BcdHandle);
            BcdHandle = NULL;
        }

        /* Launch the selected entry */
        Status = BmpLaunchBootEntry(BootEntry, &BootIndex, 0, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* Boot worked, uncache display and process the bad memory list */
            BmDisplayStateCached = FALSE;
            BmpProcessBadMemory();
        }
        else
        {
            /* Boot failed -- was it user driven? */
            if (Status != STATUS_CANCELLED)
            {
                /* Nope, remember that booting failed */
                BootFailed = TRUE;
                goto LoopQuickie;
            }

            /* Yes -- the display is still valid */
            BmDisplayStateCached = TRUE;
        }

        /* Reopen the BCD */
        Status = BmOpenDataStore(&BcdHandle);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Put the BCD options back into our entry */
        BlReplaceBootOptions(&BlpApplicationEntry, EarlyOptions);

        /* Update our options one more time */
        Status = BmpUpdateApplicationOptions(BcdHandle);
        if (NT_SUCCESS(Status))
        {
            /* Boot was 100% OK */
            BootOk = TRUE;
        }

LoopQuickie:
        /* Did we have a boot entry? */
        if (BootEntry)
        {
            /* We can destroy it now */
            BlDestroyBootEntry(BootEntry);
        }

        /* Is this the success path? */
        if (NT_SUCCESS(Status))
        {
            /* Did we actually boot something? */
            if (!BootOk)
            {
                /* Bope, fail out */
                break;
            }
        }

        /* This is the failure path... should we reboot? */
        if (RebootOnError)
        {
            break;
        }
    };

Failure:
    if (!BootFailed)
    {
        /* Check if we got here due to an internal error */
        if (BmpInternalBootError)
        {
            /* If XML is available, display the error */
            if (XmlLoaded)
            {
                //BmDisplayDumpError(0, 0);
                //BmErrorPurge();
            }

            /* Don't do a fatal error -- return back to firmware */
            goto Quickie;
        }
    }

    /* Log a general fatal error once we're here */
    BmFatalErrorEx(BL_FATAL_ERROR_GENERIC, Status, 0, 0, 0);

Quickie:
    /* Check if we should reboot */
    if ((RebootOnError) ||
        (BlpApplicationEntry.Flags & BL_APPLICATION_ENTRY_REBOOT_ON_ERROR))
    {
        /* Reboot the box */
        BlFwReboot();
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Return back to the caller with the error argument encoded */
        ReturnArguments = (PVOID)((ULONG_PTR)BootParameters + BootParameters->ReturnArgumentsOffset);
        ReturnArguments->Version = BL_RETURN_ARGUMENTS_VERSION;
        ReturnArguments->Status = Status;

        /* Tear down the boot library */
        BlDestroyLibrary();
    }

    /* Return back status */
    return Status;
}

